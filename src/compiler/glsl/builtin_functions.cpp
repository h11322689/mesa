   /*
   * Copyright © 2013 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file builtin_functions.cpp
   *
   * Support for GLSL built-in functions.
   *
   * This file is split into several main components:
   *
   * 1. Availability predicates
   *
   *    A series of small functions that check whether the current shader
   *    supports the version/extensions required to expose a built-in.
   *
   * 2. Core builtin_builder class functionality
   *
   * 3. Lists of built-in functions
   *
   *    The builtin_builder::create_builtins() function contains lists of all
   *    built-in function signatures, where they're available, what types they
   *    take, and so on.
   *
   * 4. Implementations of built-in function signatures
   *
   *    A series of functions which create ir_function_signatures and emit IR
   *    via ir_builder to implement them.
   *
   * 5. External API
   *
   *    A few functions the rest of the compiler can use to interact with the
   *    built-in function module.  For example, searching for a built-in by
   *    name and parameters.
   */
         /**
   * Unfortunately, some versions of MinGW produce bad code if this file
   * is compiled with -O2 or -O3.  The resulting driver will crash in random
   * places if the app uses GLSL.
   * The work-around is to disable optimizations for just this file.  Luckily,
   * this code is basically just executed once.
   *
   * MinGW 4.6.3 (in Ubuntu 13.10) does not have this bug.
   * MinGW 5.3.1 (in Ubuntu 16.04) definitely has this bug.
   * MinGW 6.2.0 (in Ubuntu 16.10) definitely has this bug.
   * MinGW 7.3.0 (in Ubuntu 18.04) does not have this bug.  Assume versions before 7.3.x are buggy
   */
      #include "util/detect_cc.h"
      #if defined(__MINGW32__) && (DETECT_CC_GCC_VERSION < 703)
   #warning "disabling optimizations for this file to work around compiler bug"
   #pragma GCC optimize("O1")
   #endif
         #include <stdarg.h>
   #include <stdio.h>
   #include "util/simple_mtx.h"
   #include "main/consts_exts.h"
   #include "main/shader_types.h"
   #include "main/shaderobj.h"
   #include "ir_builder.h"
   #include "glsl_parser_extras.h"
   #include "program/prog_instruction.h"
   #include <math.h>
   #include "builtin_functions.h"
   #include "util/hash_table.h"
      #ifndef M_PIf
   #define M_PIf   ((float) M_PI)
   #endif
   #ifndef M_PI_2f
   #define M_PI_2f ((float) M_PI_2)
   #endif
   #ifndef M_PI_4f
   #define M_PI_4f ((float) M_PI_4)
   #endif
      using namespace ir_builder;
      static simple_mtx_t builtins_lock = SIMPLE_MTX_INITIALIZER;
      /**
   * Availability predicates:
   *  @{
   */
   static bool
   always_available(const _mesa_glsl_parse_state *)
   {
         }
      static bool
   compatibility_vs_only(const _mesa_glsl_parse_state *state)
   {
      return state->stage == MESA_SHADER_VERTEX &&
            }
      static bool
   derivatives_only(const _mesa_glsl_parse_state *state)
   {
      return state->stage == MESA_SHADER_FRAGMENT ||
            }
      static bool
   gs_only(const _mesa_glsl_parse_state *state)
   {
         }
      /* For texture functions moved to compat profile in GLSL 4.20 */
   static bool
   deprecated_texture(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   deprecated_texture_derivatives_only(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v110(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v110_deprecated_texture(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v110_derivatives_only_deprecated_texture(const _mesa_glsl_parse_state *state)
   {
      return v110_deprecated_texture(state) &&
      }
      static bool
   v120(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v130(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v130_desktop(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v460_desktop(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v130_derivatives_only(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(130, 300) &&
      }
      static bool
   v140_or_es3(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v400_derivatives_only(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(400, 0) &&
      }
      static bool
   texture_rectangle(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   texture_external(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   texture_external_es3(const _mesa_glsl_parse_state *state)
   {
      return state->OES_EGL_image_external_essl3_enable &&
      state->es_shader &&
   }
      /** True if texturing functions with explicit LOD are allowed. */
   static bool
   lod_exists_in_stage(const _mesa_glsl_parse_state *state)
   {
      /* Texturing functions with "Lod" in their name exist:
   * - In the vertex shader stage (for all languages)
   * - In any stage for GLSL 1.30+ or GLSL ES 3.00
   * - In any stage for desktop GLSL with ARB_shader_texture_lod enabled.
   *
   * Since ARB_shader_texture_lod can only be enabled on desktop GLSL, we
   * don't need to explicitly check state->es_shader.
   */
   return state->stage == MESA_SHADER_VERTEX ||
         state->is_version(130, 300) ||
      }
      static bool
   lod_deprecated_texture(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v110_lod_deprecated_texture(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   texture_buffer(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(140, 320) ||
      state->EXT_texture_buffer_enable ||
   }
      static bool
   shader_texture_lod(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_texture_lod_and_rect(const _mesa_glsl_parse_state *state)
   {
      return state->ARB_shader_texture_lod_enable &&
      }
      static bool
   shader_bit_encoding(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(330, 300) ||
            }
      static bool
   shader_integer_mix(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(450, 310) ||
            }
      static bool
   shader_packing_or_es3(const _mesa_glsl_parse_state *state)
   {
      return state->ARB_shading_language_packing_enable ||
      }
      static bool
   shader_packing_or_es3_or_gpu_shader5(const _mesa_glsl_parse_state *state)
   {
      return state->ARB_shading_language_packing_enable ||
            }
      static bool
   gpu_shader4(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   gpu_shader4_integer(const _mesa_glsl_parse_state *state)
   {
      return state->EXT_gpu_shader4_enable &&
      }
      static bool
   gpu_shader4_array(const _mesa_glsl_parse_state *state)
   {
      return state->EXT_gpu_shader4_enable &&
      }
      static bool
   gpu_shader4_array_integer(const _mesa_glsl_parse_state *state)
   {
      return gpu_shader4_array(state) &&
      }
      static bool
   gpu_shader4_rect(const _mesa_glsl_parse_state *state)
   {
      return state->EXT_gpu_shader4_enable &&
      }
      static bool
   gpu_shader4_rect_integer(const _mesa_glsl_parse_state *state)
   {
      return gpu_shader4_rect(state) &&
      }
      static bool
   gpu_shader4_tbo(const _mesa_glsl_parse_state *state)
   {
      return state->EXT_gpu_shader4_enable &&
      }
      static bool
   gpu_shader4_tbo_integer(const _mesa_glsl_parse_state *state)
   {
      return gpu_shader4_tbo(state) &&
      }
      static bool
   gpu_shader4_derivs_only(const _mesa_glsl_parse_state *state)
   {
      return state->EXT_gpu_shader4_enable &&
      }
      static bool
   gpu_shader4_integer_derivs_only(const _mesa_glsl_parse_state *state)
   {
      return gpu_shader4_derivs_only(state) &&
      }
      static bool
   gpu_shader4_array_derivs_only(const _mesa_glsl_parse_state *state)
   {
      return gpu_shader4_derivs_only(state) &&
      }
      static bool
   gpu_shader4_array_integer_derivs_only(const _mesa_glsl_parse_state *state)
   {
      return gpu_shader4_array_derivs_only(state) &&
      }
      static bool
   v130_or_gpu_shader4(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v130_or_gpu_shader4_and_tex_shadow_lod(const _mesa_glsl_parse_state *state)
   {
      return v130_or_gpu_shader4(state) &&
      }
      static bool
   gpu_shader5(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   gpu_shader5_es(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(400, 320) ||
         state->ARB_gpu_shader5_enable ||
      }
      static bool
   gpu_shader5_or_OES_texture_cube_map_array(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(400, 320) ||
         state->ARB_gpu_shader5_enable ||
      }
      static bool
   es31_not_gs5(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   gpu_shader5_or_es31(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_packing_or_es31_or_gpu_shader5(const _mesa_glsl_parse_state *state)
   {
      return state->ARB_shading_language_packing_enable ||
            }
      static bool
   gpu_shader5_or_es31_or_integer_functions(const _mesa_glsl_parse_state *state)
   {
      return gpu_shader5_or_es31(state) ||
      }
      static bool
   fs_interpolate_at(const _mesa_glsl_parse_state *state)
   {
      return state->stage == MESA_SHADER_FRAGMENT &&
         (state->is_version(400, 320) ||
      }
         static bool
   texture_array_lod(const _mesa_glsl_parse_state *state)
   {
      return lod_exists_in_stage(state) &&
         (state->EXT_texture_array_enable ||
      }
      static bool
   texture_array(const _mesa_glsl_parse_state *state)
   {
      return state->EXT_texture_array_enable ||
            }
      static bool
   texture_array_derivs_only(const _mesa_glsl_parse_state *state)
   {
      return derivatives_only(state) &&
      }
      static bool
   texture_multisample(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(150, 310) ||
      }
      static bool
   texture_multisample_array(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(150, 320) ||
            }
      static bool
   texture_samples_identical(const _mesa_glsl_parse_state *state)
   {
      return texture_multisample(state) &&
      }
      static bool
   texture_samples_identical_array(const _mesa_glsl_parse_state *state)
   {
      return texture_multisample_array(state) &&
      }
      static bool
   derivatives_texture_cube_map_array(const _mesa_glsl_parse_state *state)
   {
      return state->has_texture_cube_map_array() &&
      }
      static bool
   texture_cube_map_array(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v130_or_gpu_shader4_and_tex_cube_map_array(const _mesa_glsl_parse_state *state)
   {
      return texture_cube_map_array(state) &&
            }
      static bool
   texture_query_levels(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(430, 0) ||
      }
      static bool
   texture_query_lod(const _mesa_glsl_parse_state *state)
   {
      return derivatives_only(state) &&
            }
      static bool
   texture_gather_cube_map_array(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(400, 320) ||
         state->ARB_texture_gather_enable ||
   state->ARB_gpu_shader5_enable ||
      }
      static bool
   texture_texture4(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   texture_gather_or_es31(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(400, 310) ||
            }
      /* Only ARB_texture_gather but not GLSL 4.0 or ARB_gpu_shader5.
   * used for relaxation of const offset requirements.
   */
   static bool
   texture_gather_only_or_es31(const _mesa_glsl_parse_state *state)
   {
      return !state->is_version(400, 320) &&
         !state->ARB_gpu_shader5_enable &&
   !state->EXT_gpu_shader5_enable &&
   !state->OES_gpu_shader5_enable &&
      }
      /* Desktop GL or OES_standard_derivatives */
   static bool
   derivatives(const _mesa_glsl_parse_state *state)
   {
      return derivatives_only(state) &&
         (state->is_version(110, 300) ||
      }
      static bool
   derivative_control(const _mesa_glsl_parse_state *state)
   {
      return derivatives_only(state) &&
            }
      /** True if sampler3D exists */
   static bool
   tex3d(const _mesa_glsl_parse_state *state)
   {
      /* sampler3D exists in all desktop GLSL versions, GLSL ES 1.00 with the
   * OES_texture_3D extension, and in GLSL ES 3.00.
   */
   return (!state->es_shader ||
            }
      static bool
   derivatives_tex3d(const _mesa_glsl_parse_state *state)
   {
      return (!state->es_shader || state->OES_texture_3D_enable) &&
      }
      static bool
   tex3d_lod(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_atomic_counters(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_atomic_counter_ops(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_atomic_counter_ops_or_v460_desktop(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_ballot(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   supports_arb_fragment_shader_interlock(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   supports_nv_fragment_shader_interlock(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_clock(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_clock_int64(const _mesa_glsl_parse_state *state)
   {
      return state->ARB_shader_clock_enable &&
            }
      static bool
   shader_storage_buffer_object(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_trinary_minmax(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_image_load_store(const _mesa_glsl_parse_state *state)
   {
      return (state->is_version(420, 310) ||
            }
      static bool
   shader_image_load_store_ext(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_image_atomic(const _mesa_glsl_parse_state *state)
   {
      return (state->is_version(420, 320) ||
         state->ARB_shader_image_load_store_enable ||
      }
      static bool
   shader_image_atomic_exchange_float(const _mesa_glsl_parse_state *state)
   {
      return (state->is_version(450, 320) ||
         state->ARB_ES3_1_compatibility_enable ||
      }
      static bool
   shader_image_atomic_add_float(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_image_size(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(430, 310) ||
      }
      static bool
   shader_samples(const _mesa_glsl_parse_state *state)
   {
      return state->is_version(450, 0) ||
      }
      static bool
   gs_streams(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   fp64(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   int64_avail(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   int64_fp64(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   compute_shader(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   compute_shader_supported(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   buffer_atomics_supported(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   buffer_int64_atomics_supported(const _mesa_glsl_parse_state *state)
   {
      return state->NV_shader_atomic_int64_enable &&
      }
      static bool
   barrier_supported(const _mesa_glsl_parse_state *state)
   {
      return compute_shader(state) ||
      }
      static bool
   vote(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   vote_ext(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   vote_or_v460_desktop(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   NV_shader_atomic_float_supported(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_atomic_float_add(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_atomic_float_exchange(const _mesa_glsl_parse_state *state)
   {
      return state->NV_shader_atomic_float_enable ||
      }
      static bool
   INTEL_shader_atomic_float_minmax_supported(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_atomic_float_minmax(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   demote_to_helper_invocation(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_integer_functions2(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   shader_integer_functions2_int64(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   sparse_enabled(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v130_desktop_and_sparse(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   texture_cube_map_array_and_sparse(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v130_derivatives_only_and_sparse(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   derivatives_texture_cube_map_array_and_sparse(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   texture_gather_and_sparse(const _mesa_glsl_parse_state *state)
   {
      return (gpu_shader5(state) || state->ARB_texture_gather_enable) &&
      }
      static bool
   gpu_shader5_and_sparse(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   texture_multisample_and_sparse(const _mesa_glsl_parse_state *state)
   {
      return texture_multisample(state) &&
      }
      static bool
   texture_multisample_array_and_sparse(const _mesa_glsl_parse_state *state)
   {
      return texture_multisample_array(state) &&
      }
      static bool
   shader_image_load_store_and_sparse(const _mesa_glsl_parse_state *state)
   {
      return shader_image_load_store(state) &&
      }
      static bool
   v130_desktop_and_clamp(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   texture_cube_map_array_and_clamp(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   v130_derivatives_only_and_clamp(const _mesa_glsl_parse_state *state)
   {
         }
      static bool
   derivatives_texture_cube_map_array_and_clamp(const _mesa_glsl_parse_state *state)
   {
         }
      /** @} */
      /******************************************************************************/
      namespace {
      /**
   * builtin_builder: A singleton object representing the core of the built-in
   * function module.
   *
   * It generates IR for every built-in function signature, and organizes them
   * into functions.
   */
   class builtin_builder {
   public:
      builtin_builder();
            void initialize();
   void release();
   ir_function_signature *find(_mesa_glsl_parse_state *state,
            /**
   * A shader to hold all the built-in signatures; created by this module.
   *
   * This includes signatures for every built-in, regardless of version or
   * enabled extensions.  The availability predicate associated with each
   * signature allows matching_signature() to filter out the irrelevant ones.
   */
         private:
               void create_shader();
   void create_intrinsics();
            /**
   * IR builder helpers:
   *
   * These convenience functions assist in emitting IR, but don't necessarily
   * fit in ir_builder itself.  Many of them rely on having a mem_ctx class
   * member available.
   */
   ir_variable *in_var(const glsl_type *type, const char *name);
   ir_variable *in_mediump_var(const glsl_type *type, const char *name);
   ir_variable *in_highp_var(const glsl_type *type, const char *name);
   ir_variable *out_var(const glsl_type *type, const char *name);
   ir_variable *out_lowp_var(const glsl_type *type, const char *name);
   ir_variable *out_highp_var(const glsl_type *type, const char *name);
   ir_variable *as_highp(ir_factory &body, ir_variable *var);
   ir_constant *imm(float f, unsigned vector_elements=1);
   ir_constant *imm(bool b, unsigned vector_elements=1);
   ir_constant *imm(int i, unsigned vector_elements=1);
   ir_constant *imm(unsigned u, unsigned vector_elements=1);
   ir_constant *imm(double d, unsigned vector_elements=1);
   ir_constant *imm(const glsl_type *type, const ir_constant_data &);
   ir_dereference_variable *var_ref(ir_variable *var);
   ir_dereference_array *array_ref(ir_variable *var, int i);
   ir_swizzle *matrix_elt(ir_variable *var, int col, int row);
            ir_expression *asin_expr(ir_variable *x, float p0, float p1);
            /**
   * Call function \param f with parameters specified as the linked
   * list \param params of \c ir_variable objects.  \param ret should
   * point to the ir_variable that will hold the function return
   * value, or be \c NULL if the function has void return type.
   */
            /** Create a new function and add the given signatures. */
            typedef ir_function_signature *(builtin_builder::*image_prototype_ctr)(const glsl_type *image_type,
                  /**
   * Create a new image built-in function for all known image types.
   * \p flags is a bitfield of \c image_function_flags flags.
   */
   void add_image_function(const char *name,
                                    /**
   * Create new functions for all known image built-ins and types.
   * If \p glsl is \c true, use the GLSL built-in names and emit code
   * to call into the actual compiler intrinsic.  If \p glsl is
   * false, emit a function prototype with no body for each image
   * intrinsic name.
   */
            ir_function_signature *new_sig(const glsl_type *return_type,
                  /**
   * Function signature generators:
   *  @{
   */
   ir_function_signature *unop(builtin_available_predicate avail,
                     ir_function_signature *unop_precision(builtin_available_predicate avail,
                     ir_function_signature *binop(builtin_available_predicate avail,
                                 #define B0(X) ir_function_signature *_##X();
   #define B1(X) ir_function_signature *_##X(const glsl_type *);
   #define B2(X) ir_function_signature *_##X(const glsl_type *, const glsl_type *);
   #define B3(X) ir_function_signature *_##X(const glsl_type *, const glsl_type *, const glsl_type *);
   #define BA1(X) ir_function_signature *_##X(builtin_available_predicate, const glsl_type *);
   #define BA2(X) ir_function_signature *_##X(builtin_available_predicate, const glsl_type *, const glsl_type *);
      B1(radians)
   B1(degrees)
   B1(sin)
   B1(cos)
   B1(tan)
   B1(asin)
   B1(acos)
   B1(atan2)
   B1(atan)
   B1(sinh)
   B1(cosh)
   B1(tanh)
   B1(asinh)
   B1(acosh)
   B1(atanh)
   B1(pow)
   B1(exp)
   B1(log)
   B1(exp2)
   B1(log2)
   BA1(sqrt)
   BA1(inversesqrt)
   BA1(abs)
   BA1(sign)
   BA1(floor)
   BA1(truncate)
   BA1(trunc)
   BA1(round)
   BA1(roundEven)
   BA1(ceil)
   BA1(fract)
   BA2(mod)
   BA1(modf)
   BA2(min)
   BA2(max)
   BA2(clamp)
   BA2(mix_lrp)
   ir_function_signature *_mix_sel(builtin_available_predicate avail,
               BA2(step)
   BA2(smoothstep)
   BA1(isnan)
   BA1(isinf)
   B1(floatBitsToInt)
   B1(floatBitsToUint)
   B1(intBitsToFloat)
            BA1(doubleBitsToInt64)
   BA1(doubleBitsToUint64)
   BA1(int64BitsToDouble)
            ir_function_signature *_packUnorm2x16(builtin_available_predicate avail);
   ir_function_signature *_packSnorm2x16(builtin_available_predicate avail);
   ir_function_signature *_packUnorm4x8(builtin_available_predicate avail);
   ir_function_signature *_packSnorm4x8(builtin_available_predicate avail);
   ir_function_signature *_unpackUnorm2x16(builtin_available_predicate avail);
   ir_function_signature *_unpackSnorm2x16(builtin_available_predicate avail);
   ir_function_signature *_unpackUnorm4x8(builtin_available_predicate avail);
   ir_function_signature *_unpackSnorm4x8(builtin_available_predicate avail);
   ir_function_signature *_packHalf2x16(builtin_available_predicate avail);
   ir_function_signature *_unpackHalf2x16(builtin_available_predicate avail);
   ir_function_signature *_packDouble2x32(builtin_available_predicate avail);
   ir_function_signature *_unpackDouble2x32(builtin_available_predicate avail);
   ir_function_signature *_packInt2x32(builtin_available_predicate avail);
   ir_function_signature *_unpackInt2x32(builtin_available_predicate avail);
   ir_function_signature *_packUint2x32(builtin_available_predicate avail);
            BA1(length)
   BA1(distance);
   BA1(dot);
   BA1(cross);
   BA1(normalize);
   B0(ftransform);
   BA1(faceforward);
   BA1(reflect);
   BA1(refract);
   BA1(matrixCompMult);
   BA1(outerProduct);
   BA1(determinant_mat2);
   BA1(determinant_mat3);
   BA1(determinant_mat4);
   BA1(inverse_mat2);
   BA1(inverse_mat3);
   BA1(inverse_mat4);
   BA1(transpose);
   BA1(lessThan);
   BA1(lessThanEqual);
   BA1(greaterThan);
   BA1(greaterThanEqual);
   BA1(equal);
   BA1(notEqual);
   B1(any);
   B1(all);
   B1(not);
   BA2(textureSize);
            B0(is_sparse_texels_resident);
         /** Flags to _texture() */
   #define TEX_PROJECT 1
   #define TEX_OFFSET  2
   #define TEX_COMPONENT 4
   #define TEX_OFFSET_NONCONST 8
   #define TEX_OFFSET_ARRAY 16
   #define TEX_SPARSE 32
   #define TEX_CLAMP 64
         ir_function_signature *_texture(ir_texture_opcode opcode,
                                 ir_function_signature *_textureCubeArrayShadow(ir_texture_opcode opcode,
                     ir_function_signature *_texelFetch(builtin_available_predicate avail,
                                    B0(EmitVertex)
   B0(EndPrimitive)
   ir_function_signature *_EmitStreamVertex(builtin_available_predicate avail,
         ir_function_signature *_EndStreamPrimitive(builtin_available_predicate avail,
                  BA2(textureQueryLod);
   BA1(textureQueryLevels);
   BA2(textureSamplesIdentical);
   B1(dFdx);
   B1(dFdy);
   B1(fwidth);
   B1(dFdxCoarse);
   B1(dFdyCoarse);
   B1(fwidthCoarse);
   B1(dFdxFine);
   B1(dFdyFine);
   B1(fwidthFine);
   B1(noise1);
   B1(noise2);
   B1(noise3);
            B1(bitfieldExtract)
   B1(bitfieldInsert)
   B1(bitfieldReverse)
   B1(bitCount)
   B1(findLSB)
   B1(findMSB)
   BA1(countLeadingZeros)
   BA1(countTrailingZeros)
   BA1(fma)
   B2(ldexp)
   B2(frexp)
   B2(dfrexp)
   B1(uaddCarry)
   B1(usubBorrow)
   BA1(addSaturate)
   BA1(subtractSaturate)
   BA1(absoluteDifference)
   BA1(average)
   BA1(averageRounded)
   B1(mulExtended)
   BA1(multiply32x16)
   B1(interpolateAtCentroid)
   B1(interpolateAtOffset)
            ir_function_signature *_atomic_counter_intrinsic(builtin_available_predicate avail,
         ir_function_signature *_atomic_counter_intrinsic1(builtin_available_predicate avail,
         ir_function_signature *_atomic_counter_intrinsic2(builtin_available_predicate avail,
         ir_function_signature *_atomic_counter_op(const char *intrinsic,
         ir_function_signature *_atomic_counter_op1(const char *intrinsic,
         ir_function_signature *_atomic_counter_op2(const char *intrinsic,
            ir_function_signature *_atomic_intrinsic2(builtin_available_predicate avail,
               ir_function_signature *_atomic_op2(const char *intrinsic,
               ir_function_signature *_atomic_intrinsic3(builtin_available_predicate avail,
               ir_function_signature *_atomic_op3(const char *intrinsic,
                  B1(min3)
   B1(max3)
            ir_function_signature *_image_prototype(const glsl_type *image_type,
               ir_function_signature *_image_size_prototype(const glsl_type *image_type,
               ir_function_signature *_image_samples_prototype(const glsl_type *image_type,
               ir_function_signature *_image(image_prototype_ctr prototype,
                                    ir_function_signature *_memory_barrier_intrinsic(
      builtin_available_predicate avail,
      ir_function_signature *_memory_barrier(const char *intrinsic_name,
            ir_function_signature *_ballot_intrinsic();
   ir_function_signature *_ballot();
   ir_function_signature *_read_first_invocation_intrinsic(const glsl_type *type);
   ir_function_signature *_read_first_invocation(const glsl_type *type);
   ir_function_signature *_read_invocation_intrinsic(const glsl_type *type);
               ir_function_signature *_invocation_interlock_intrinsic(
      builtin_available_predicate avail,
      ir_function_signature *_invocation_interlock(
      const char *intrinsic_name,
         ir_function_signature *_shader_clock_intrinsic(builtin_available_predicate avail,
         ir_function_signature *_shader_clock(builtin_available_predicate avail,
            ir_function_signature *_vote_intrinsic(builtin_available_predicate avail,
         ir_function_signature *_vote(const char *intrinsic_name,
            ir_function_signature *_helper_invocation_intrinsic();
         #undef B0
   #undef B1
   #undef B2
   #undef B3
   #undef BA1
   #undef BA2
         };
      enum image_function_flags {
      IMAGE_FUNCTION_EMIT_STUB = (1 << 0),
   IMAGE_FUNCTION_RETURNS_VOID = (1 << 1),
   IMAGE_FUNCTION_HAS_VECTOR_DATA_TYPE = (1 << 2),
   IMAGE_FUNCTION_SUPPORTS_FLOAT_DATA_TYPE = (1 << 3),
   IMAGE_FUNCTION_READ_ONLY = (1 << 4),
   IMAGE_FUNCTION_WRITE_ONLY = (1 << 5),
   IMAGE_FUNCTION_AVAIL_ATOMIC = (1 << 6),
   IMAGE_FUNCTION_MS_ONLY = (1 << 7),
   IMAGE_FUNCTION_AVAIL_ATOMIC_EXCHANGE = (1 << 8),
   IMAGE_FUNCTION_AVAIL_ATOMIC_ADD = (1 << 9),
   IMAGE_FUNCTION_EXT_ONLY = (1 << 10),
   IMAGE_FUNCTION_SUPPORTS_SIGNED_DATA_TYPE = (1 << 11),
      };
      } /* anonymous namespace */
      /**
   * Core builtin_builder functionality:
   *  @{
   */
   builtin_builder::builtin_builder()
         {
         }
      builtin_builder::~builtin_builder()
   {
               ralloc_free(mem_ctx);
            ralloc_free(shader);
               }
      ir_function_signature *
   builtin_builder::find(_mesa_glsl_parse_state *state,
         {
      /* The shader currently being compiled requested a built-in function;
   * it needs to link against builtin_builder::shader in order to get them.
   *
   * Even if we don't find a matching signature, we still need to do this so
   * that the "no matching signature" error will list potential candidates
   * from the available built-ins.
   */
            ir_function *f = shader->symbols->get_function(name);
   if (f == NULL)
            ir_function_signature *sig =
         if (sig == NULL)
               }
      void
   builtin_builder::initialize()
   {
      /* If already initialized, don't do it again. */
   if (mem_ctx != NULL)
                     mem_ctx = ralloc_context(NULL);
   create_shader();
   create_intrinsics();
      }
      void
   builtin_builder::release()
   {
      ralloc_free(mem_ctx);
            ralloc_free(shader);
               }
      void
   builtin_builder::create_shader()
   {
      /* The target doesn't actually matter.  There's no target for generic
   * GLSL utility code that could be linked against any stage, so just
   * arbitrarily pick GL_VERTEX_SHADER.
   */
   shader = _mesa_new_shader(0, MESA_SHADER_VERTEX);
      }
      /** @} */
      /**
   * Create ir_function and ir_function_signature objects for each
   * intrinsic.
   */
   void
   builtin_builder::create_intrinsics()
   {
      add_function("__intrinsic_atomic_read",
               _atomic_counter_intrinsic(shader_atomic_counters,
   add_function("__intrinsic_atomic_increment",
               _atomic_counter_intrinsic(shader_atomic_counters,
   add_function("__intrinsic_atomic_predecrement",
                        add_function("__intrinsic_atomic_add",
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(NV_shader_atomic_float_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_counter_intrinsic1(shader_atomic_counter_ops_or_v460_desktop,
   add_function("__intrinsic_atomic_min",
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(INTEL_shader_atomic_float_minmax_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_counter_intrinsic1(shader_atomic_counter_ops_or_v460_desktop,
   add_function("__intrinsic_atomic_max",
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(INTEL_shader_atomic_float_minmax_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_counter_intrinsic1(shader_atomic_counter_ops_or_v460_desktop,
   add_function("__intrinsic_atomic_and",
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_counter_intrinsic1(shader_atomic_counter_ops_or_v460_desktop,
   add_function("__intrinsic_atomic_or",
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_counter_intrinsic1(shader_atomic_counter_ops_or_v460_desktop,
   add_function("__intrinsic_atomic_xor",
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_counter_intrinsic1(shader_atomic_counter_ops_or_v460_desktop,
   add_function("__intrinsic_atomic_exchange",
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_atomics_supported,
               _atomic_intrinsic2(buffer_int64_atomics_supported,
               _atomic_intrinsic2(NV_shader_atomic_float_supported,
               _atomic_counter_intrinsic1(shader_atomic_counter_ops_or_v460_desktop,
   add_function("__intrinsic_atomic_comp_swap",
               _atomic_intrinsic3(buffer_atomics_supported,
               _atomic_intrinsic3(buffer_atomics_supported,
               _atomic_intrinsic3(buffer_int64_atomics_supported,
               _atomic_intrinsic3(INTEL_shader_atomic_float_minmax_supported,
                                 add_function("__intrinsic_memory_barrier",
               _memory_barrier_intrinsic(shader_image_load_store,
   add_function("__intrinsic_group_memory_barrier",
               _memory_barrier_intrinsic(compute_shader,
   add_function("__intrinsic_memory_barrier_atomic_counter",
               _memory_barrier_intrinsic(compute_shader_supported,
   add_function("__intrinsic_memory_barrier_buffer",
               _memory_barrier_intrinsic(compute_shader_supported,
   add_function("__intrinsic_memory_barrier_image",
               _memory_barrier_intrinsic(compute_shader_supported,
   add_function("__intrinsic_memory_barrier_shared",
                        add_function("__intrinsic_begin_invocation_interlock",
                        add_function("__intrinsic_end_invocation_interlock",
                        add_function("__intrinsic_shader_clock",
                        add_function("__intrinsic_vote_all",
               add_function("__intrinsic_vote_any",
               add_function("__intrinsic_vote_eq",
                           add_function("__intrinsic_read_invocation",
               _read_invocation_intrinsic(glsl_type::float_type),
                        _read_invocation_intrinsic(glsl_type::int_type),
                        _read_invocation_intrinsic(glsl_type::uint_type),
   _read_invocation_intrinsic(glsl_type::uvec2_type),
            add_function("__intrinsic_read_first_invocation",
               _read_first_invocation_intrinsic(glsl_type::float_type),
                        _read_first_invocation_intrinsic(glsl_type::int_type),
                        _read_first_invocation_intrinsic(glsl_type::uint_type),
   _read_first_invocation_intrinsic(glsl_type::uvec2_type),
            add_function("__intrinsic_helper_invocation",
            add_function("__intrinsic_is_sparse_texels_resident",
      }
      /**
   * Create ir_function and ir_function_signature objects for each built-in.
   *
   * Contains a list of every available built-in.
   */
   void
   builtin_builder::create_builtins()
   {
   #define F(NAME)                                 \
      add_function(#NAME,                          \
               _##NAME(glsl_type::float_type), \
   _##NAME(glsl_type::vec2_type),  \
         #define FD(NAME)                                 \
      add_function(#NAME,                          \
               _##NAME(always_available, glsl_type::float_type), \
   _##NAME(always_available, glsl_type::vec2_type),  \
   _##NAME(always_available, glsl_type::vec3_type),  \
   _##NAME(always_available, glsl_type::vec4_type),  \
   _##NAME(fp64, glsl_type::double_type),  \
   _##NAME(fp64, glsl_type::dvec2_type),    \
         #define FD130(NAME)                                 \
      add_function(#NAME,                          \
               _##NAME(v130, glsl_type::float_type), \
   _##NAME(v130, glsl_type::vec2_type),  \
   _##NAME(v130, glsl_type::vec3_type),                  \
   _##NAME(v130, glsl_type::vec4_type),  \
   _##NAME(fp64, glsl_type::double_type),  \
   _##NAME(fp64, glsl_type::dvec2_type),    \
         #define FD130GS4(NAME)                          \
      add_function(#NAME,                          \
               _##NAME(v130_or_gpu_shader4, glsl_type::float_type), \
   _##NAME(v130_or_gpu_shader4, glsl_type::vec2_type),  \
   _##NAME(v130_or_gpu_shader4, glsl_type::vec3_type),  \
   _##NAME(v130_or_gpu_shader4, glsl_type::vec4_type),  \
   _##NAME(fp64, glsl_type::double_type),  \
   _##NAME(fp64, glsl_type::dvec2_type),    \
         #define FDGS5(NAME)                                 \
      add_function(#NAME,                          \
               _##NAME(gpu_shader5_es, glsl_type::float_type), \
   _##NAME(gpu_shader5_es, glsl_type::vec2_type),  \
   _##NAME(gpu_shader5_es, glsl_type::vec3_type),                  \
   _##NAME(gpu_shader5_es, glsl_type::vec4_type),  \
   _##NAME(fp64, glsl_type::double_type),  \
   _##NAME(fp64, glsl_type::dvec2_type),    \
         #define FI(NAME)                                \
      add_function(#NAME,                          \
               _##NAME(glsl_type::float_type), \
   _##NAME(glsl_type::vec2_type),  \
   _##NAME(glsl_type::vec3_type),  \
   _##NAME(glsl_type::vec4_type),  \
   _##NAME(glsl_type::int_type),   \
   _##NAME(glsl_type::ivec2_type), \
         #define FI64(NAME)                                \
      add_function(#NAME,                          \
               _##NAME(always_available, glsl_type::float_type), \
   _##NAME(always_available, glsl_type::vec2_type),  \
   _##NAME(always_available, glsl_type::vec3_type),  \
   _##NAME(always_available, glsl_type::vec4_type),  \
   _##NAME(always_available, glsl_type::int_type),   \
   _##NAME(always_available, glsl_type::ivec2_type), \
   _##NAME(always_available, glsl_type::ivec3_type), \
   _##NAME(always_available, glsl_type::ivec4_type), \
   _##NAME(fp64, glsl_type::double_type), \
   _##NAME(fp64, glsl_type::dvec2_type),  \
   _##NAME(fp64, glsl_type::dvec3_type),  \
   _##NAME(fp64, glsl_type::dvec4_type),  \
   _##NAME(int64_avail, glsl_type::int64_t_type), \
   _##NAME(int64_avail, glsl_type::i64vec2_type),  \
         #define FIUD_VEC(NAME)                                            \
      add_function(#NAME,                                            \
               _##NAME(always_available, glsl_type::vec2_type),  \
   _##NAME(always_available, glsl_type::vec3_type),  \
   _##NAME(always_available, glsl_type::vec4_type),  \
         _##NAME(always_available, glsl_type::ivec2_type), \
   _##NAME(always_available, glsl_type::ivec3_type), \
   _##NAME(always_available, glsl_type::ivec4_type), \
         _##NAME(v130_or_gpu_shader4, glsl_type::uvec2_type), \
   _##NAME(v130_or_gpu_shader4, glsl_type::uvec3_type), \
   _##NAME(v130_or_gpu_shader4, glsl_type::uvec4_type), \
   _##NAME(fp64, glsl_type::dvec2_type),  \
   _##NAME(fp64, glsl_type::dvec3_type),  \
   _##NAME(fp64, glsl_type::dvec4_type),  \
   _##NAME(int64_avail, glsl_type::int64_t_type), \
   _##NAME(int64_avail, glsl_type::i64vec2_type),  \
   _##NAME(int64_avail, glsl_type::i64vec3_type),  \
   _##NAME(int64_avail, glsl_type::i64vec4_type),  \
   _##NAME(int64_avail, glsl_type::uint64_t_type), \
   _##NAME(int64_avail, glsl_type::u64vec2_type),  \
         #define IU(NAME)                                \
      add_function(#NAME,                          \
               _##NAME(glsl_type::int_type),   \
   _##NAME(glsl_type::ivec2_type), \
   _##NAME(glsl_type::ivec3_type), \
   _##NAME(glsl_type::ivec4_type), \
         _##NAME(glsl_type::uint_type),  \
   _##NAME(glsl_type::uvec2_type), \
         #define FIUBD_VEC(NAME)                                           \
      add_function(#NAME,                                            \
               _##NAME(always_available, glsl_type::vec2_type),  \
   _##NAME(always_available, glsl_type::vec3_type),  \
   _##NAME(always_available, glsl_type::vec4_type),  \
         _##NAME(always_available, glsl_type::ivec2_type), \
   _##NAME(always_available, glsl_type::ivec3_type), \
   _##NAME(always_available, glsl_type::ivec4_type), \
         _##NAME(v130_or_gpu_shader4, glsl_type::uvec2_type), \
   _##NAME(v130_or_gpu_shader4, glsl_type::uvec3_type), \
   _##NAME(v130_or_gpu_shader4, glsl_type::uvec4_type), \
         _##NAME(always_available, glsl_type::bvec2_type), \
   _##NAME(always_available, glsl_type::bvec3_type), \
   _##NAME(always_available, glsl_type::bvec4_type), \
         _##NAME(fp64, glsl_type::dvec2_type), \
   _##NAME(fp64, glsl_type::dvec3_type), \
   _##NAME(fp64, glsl_type::dvec4_type), \
   _##NAME(int64_avail, glsl_type::int64_t_type), \
   _##NAME(int64_avail, glsl_type::i64vec2_type),  \
   _##NAME(int64_avail, glsl_type::i64vec3_type),  \
   _##NAME(int64_avail, glsl_type::i64vec4_type),  \
   _##NAME(int64_avail, glsl_type::uint64_t_type), \
   _##NAME(int64_avail, glsl_type::u64vec2_type),  \
         #define FIUD2_MIXED(NAME)                                                                 \
      add_function(#NAME,                                                                   \
               _##NAME(always_available, glsl_type::float_type, glsl_type::float_type), \
   _##NAME(always_available, glsl_type::vec2_type,  glsl_type::float_type), \
   _##NAME(always_available, glsl_type::vec3_type,  glsl_type::float_type), \
   _##NAME(always_available, glsl_type::vec4_type,  glsl_type::float_type), \
         _##NAME(always_available, glsl_type::vec2_type,  glsl_type::vec2_type),  \
   _##NAME(always_available, glsl_type::vec3_type,  glsl_type::vec3_type),  \
   _##NAME(always_available, glsl_type::vec4_type,  glsl_type::vec4_type),  \
         _##NAME(always_available, glsl_type::int_type,   glsl_type::int_type),   \
   _##NAME(always_available, glsl_type::ivec2_type, glsl_type::int_type),   \
   _##NAME(always_available, glsl_type::ivec3_type, glsl_type::int_type),   \
   _##NAME(always_available, glsl_type::ivec4_type, glsl_type::int_type),   \
         _##NAME(always_available, glsl_type::ivec2_type, glsl_type::ivec2_type), \
   _##NAME(always_available, glsl_type::ivec3_type, glsl_type::ivec3_type), \
   _##NAME(always_available, glsl_type::ivec4_type, glsl_type::ivec4_type), \
         _##NAME(v130_or_gpu_shader4, glsl_type::uint_type,  glsl_type::uint_type),  \
   _##NAME(v130_or_gpu_shader4, glsl_type::uvec2_type, glsl_type::uint_type),  \
   _##NAME(v130_or_gpu_shader4, glsl_type::uvec3_type, glsl_type::uint_type),  \
   _##NAME(v130_or_gpu_shader4, glsl_type::uvec4_type, glsl_type::uint_type),  \
         _##NAME(v130_or_gpu_shader4, glsl_type::uvec2_type, glsl_type::uvec2_type), \
   _##NAME(v130_or_gpu_shader4, glsl_type::uvec3_type, glsl_type::uvec3_type), \
   _##NAME(v130_or_gpu_shader4, glsl_type::uvec4_type, glsl_type::uvec4_type), \
         _##NAME(fp64, glsl_type::double_type, glsl_type::double_type),           \
   _##NAME(fp64, glsl_type::dvec2_type, glsl_type::double_type),           \
   _##NAME(fp64, glsl_type::dvec3_type, glsl_type::double_type),           \
   _##NAME(fp64, glsl_type::dvec4_type, glsl_type::double_type),           \
   _##NAME(fp64, glsl_type::dvec2_type, glsl_type::dvec2_type),           \
   _##NAME(fp64, glsl_type::dvec3_type, glsl_type::dvec3_type),           \
   _##NAME(fp64, glsl_type::dvec4_type, glsl_type::dvec4_type),           \
         _##NAME(int64_avail, glsl_type::int64_t_type, glsl_type::int64_t_type),     \
   _##NAME(int64_avail, glsl_type::i64vec2_type, glsl_type::int64_t_type),     \
   _##NAME(int64_avail, glsl_type::i64vec3_type, glsl_type::int64_t_type),     \
   _##NAME(int64_avail, glsl_type::i64vec4_type, glsl_type::int64_t_type),     \
   _##NAME(int64_avail, glsl_type::i64vec2_type, glsl_type::i64vec2_type),     \
   _##NAME(int64_avail, glsl_type::i64vec3_type, glsl_type::i64vec3_type),     \
   _##NAME(int64_avail, glsl_type::i64vec4_type, glsl_type::i64vec4_type),     \
   _##NAME(int64_avail, glsl_type::uint64_t_type, glsl_type::uint64_t_type),   \
   _##NAME(int64_avail, glsl_type::u64vec2_type, glsl_type::uint64_t_type),    \
   _##NAME(int64_avail, glsl_type::u64vec3_type, glsl_type::uint64_t_type),    \
   _##NAME(int64_avail, glsl_type::u64vec4_type, glsl_type::uint64_t_type),    \
   _##NAME(int64_avail, glsl_type::u64vec2_type, glsl_type::u64vec2_type),     \
            F(radians)
   F(degrees)
   F(sin)
   F(cos)
   F(tan)
   F(asin)
            add_function("atan",
               _atan(glsl_type::float_type),
   _atan(glsl_type::vec2_type),
   _atan(glsl_type::vec3_type),
   _atan(glsl_type::vec4_type),
   _atan2(glsl_type::float_type),
   _atan2(glsl_type::vec2_type),
            F(sinh)
   F(cosh)
   F(tanh)
   F(asinh)
   F(acosh)
   F(atanh)
   F(pow)
   F(exp)
   F(log)
   F(exp2)
   F(log2)
   FD(sqrt)
   FD(inversesqrt)
   FI64(abs)
   FI64(sign)
   FD(floor)
   FD130(trunc)
   FD130GS4(round)
   FD130(roundEven)
   FD(ceil)
            add_function("truncate",
               _truncate(gpu_shader4, glsl_type::float_type),
   _truncate(gpu_shader4, glsl_type::vec2_type),
               add_function("mod",
               _mod(always_available, glsl_type::float_type, glsl_type::float_type),
                                             _mod(fp64, glsl_type::double_type, glsl_type::double_type),
                        _mod(fp64, glsl_type::dvec2_type,  glsl_type::dvec2_type),
                     FIUD2_MIXED(min)
   FIUD2_MIXED(max)
            add_function("mix",
               _mix_lrp(always_available, glsl_type::float_type, glsl_type::float_type),
                                             _mix_lrp(fp64, glsl_type::double_type, glsl_type::double_type),
                                             _mix_sel(v130, glsl_type::float_type, glsl_type::bool_type),
                        _mix_sel(fp64, glsl_type::double_type, glsl_type::bool_type),
                        _mix_sel(shader_integer_mix, glsl_type::int_type,   glsl_type::bool_type),
                        _mix_sel(shader_integer_mix, glsl_type::uint_type,  glsl_type::bool_type),
                        _mix_sel(shader_integer_mix, glsl_type::bool_type,  glsl_type::bool_type),
                        _mix_sel(int64_avail, glsl_type::int64_t_type, glsl_type::bool_type),
                        _mix_sel(int64_avail, glsl_type::uint64_t_type,  glsl_type::bool_type),
   _mix_sel(int64_avail, glsl_type::u64vec2_type, glsl_type::bvec2_type),
            add_function("step",
               _step(always_available, glsl_type::float_type, glsl_type::float_type),
                        _step(always_available, glsl_type::vec2_type,  glsl_type::vec2_type),
   _step(always_available, glsl_type::vec3_type,  glsl_type::vec3_type),
   _step(always_available, glsl_type::vec4_type,  glsl_type::vec4_type),
   _step(fp64, glsl_type::double_type, glsl_type::double_type),
                        _step(fp64, glsl_type::dvec2_type,  glsl_type::dvec2_type),
            add_function("smoothstep",
               _smoothstep(always_available, glsl_type::float_type, glsl_type::float_type),
                        _smoothstep(always_available, glsl_type::vec2_type,  glsl_type::vec2_type),
   _smoothstep(always_available, glsl_type::vec3_type,  glsl_type::vec3_type),
   _smoothstep(always_available, glsl_type::vec4_type,  glsl_type::vec4_type),
   _smoothstep(fp64, glsl_type::double_type, glsl_type::double_type),
                        _smoothstep(fp64, glsl_type::dvec2_type,  glsl_type::dvec2_type),
            FD130(isnan)
            F(floatBitsToInt)
   F(floatBitsToUint)
   add_function("intBitsToFloat",
               _intBitsToFloat(glsl_type::int_type),
   _intBitsToFloat(glsl_type::ivec2_type),
   _intBitsToFloat(glsl_type::ivec3_type),
   add_function("uintBitsToFloat",
               _uintBitsToFloat(glsl_type::uint_type),
   _uintBitsToFloat(glsl_type::uvec2_type),
            add_function("doubleBitsToInt64",
               _doubleBitsToInt64(int64_fp64, glsl_type::double_type),
   _doubleBitsToInt64(int64_fp64, glsl_type::dvec2_type),
            add_function("doubleBitsToUint64",
               _doubleBitsToUint64(int64_fp64, glsl_type::double_type),
   _doubleBitsToUint64(int64_fp64, glsl_type::dvec2_type),
            add_function("int64BitsToDouble",
               _int64BitsToDouble(int64_fp64, glsl_type::int64_t_type),
   _int64BitsToDouble(int64_fp64, glsl_type::i64vec2_type),
            add_function("uint64BitsToDouble",
               _uint64BitsToDouble(int64_fp64, glsl_type::uint64_t_type),
   _uint64BitsToDouble(int64_fp64, glsl_type::u64vec2_type),
            add_function("packUnorm2x16",   _packUnorm2x16(shader_packing_or_es3_or_gpu_shader5),   NULL);
   add_function("packSnorm2x16",   _packSnorm2x16(shader_packing_or_es3),                  NULL);
   add_function("packUnorm4x8",    _packUnorm4x8(shader_packing_or_es31_or_gpu_shader5),   NULL);
   add_function("packSnorm4x8",    _packSnorm4x8(shader_packing_or_es31_or_gpu_shader5),   NULL);
   add_function("unpackUnorm2x16", _unpackUnorm2x16(shader_packing_or_es3_or_gpu_shader5), NULL);
   add_function("unpackSnorm2x16", _unpackSnorm2x16(shader_packing_or_es3),                NULL);
   add_function("unpackUnorm4x8",  _unpackUnorm4x8(shader_packing_or_es31_or_gpu_shader5), NULL);
   add_function("unpackSnorm4x8",  _unpackSnorm4x8(shader_packing_or_es31_or_gpu_shader5), NULL);
   add_function("packHalf2x16",    _packHalf2x16(shader_packing_or_es3),                   NULL);
   add_function("unpackHalf2x16",  _unpackHalf2x16(shader_packing_or_es3),                 NULL);
   add_function("packDouble2x32",    _packDouble2x32(fp64),                   NULL);
            add_function("packInt2x32",     _packInt2x32(int64_avail),                    NULL);
   add_function("unpackInt2x32",   _unpackInt2x32(int64_avail),                  NULL);
   add_function("packUint2x32",    _packUint2x32(int64_avail),                   NULL);
            FD(length)
   FD(distance)
            add_function("cross", _cross(always_available, glsl_type::vec3_type),
            FD(normalize)
   add_function("ftransform", _ftransform(), NULL);
   FD(faceforward)
   FD(reflect)
   FD(refract)
   // ...
   add_function("matrixCompMult",
               _matrixCompMult(always_available, glsl_type::mat2_type),
   _matrixCompMult(always_available, glsl_type::mat3_type),
   _matrixCompMult(always_available, glsl_type::mat4_type),
   _matrixCompMult(always_available, glsl_type::mat2x3_type),
   _matrixCompMult(always_available, glsl_type::mat2x4_type),
   _matrixCompMult(always_available, glsl_type::mat3x2_type),
   _matrixCompMult(always_available, glsl_type::mat3x4_type),
   _matrixCompMult(always_available, glsl_type::mat4x2_type),
   _matrixCompMult(always_available, glsl_type::mat4x3_type),
   _matrixCompMult(fp64, glsl_type::dmat2_type),
   _matrixCompMult(fp64, glsl_type::dmat3_type),
   _matrixCompMult(fp64, glsl_type::dmat4_type),
   _matrixCompMult(fp64, glsl_type::dmat2x3_type),
   _matrixCompMult(fp64, glsl_type::dmat2x4_type),
   _matrixCompMult(fp64, glsl_type::dmat3x2_type),
   _matrixCompMult(fp64, glsl_type::dmat3x4_type),
   _matrixCompMult(fp64, glsl_type::dmat4x2_type),
   add_function("outerProduct",
               _outerProduct(v120, glsl_type::mat2_type),
   _outerProduct(v120, glsl_type::mat3_type),
   _outerProduct(v120, glsl_type::mat4_type),
   _outerProduct(v120, glsl_type::mat2x3_type),
   _outerProduct(v120, glsl_type::mat2x4_type),
   _outerProduct(v120, glsl_type::mat3x2_type),
   _outerProduct(v120, glsl_type::mat3x4_type),
   _outerProduct(v120, glsl_type::mat4x2_type),
   _outerProduct(v120, glsl_type::mat4x3_type),
   _outerProduct(fp64, glsl_type::dmat2_type),
   _outerProduct(fp64, glsl_type::dmat3_type),
   _outerProduct(fp64, glsl_type::dmat4_type),
   _outerProduct(fp64, glsl_type::dmat2x3_type),
   _outerProduct(fp64, glsl_type::dmat2x4_type),
   _outerProduct(fp64, glsl_type::dmat3x2_type),
   _outerProduct(fp64, glsl_type::dmat3x4_type),
   _outerProduct(fp64, glsl_type::dmat4x2_type),
   add_function("determinant",
               _determinant_mat2(v120, glsl_type::mat2_type),
   _determinant_mat3(v120, glsl_type::mat3_type),
   _determinant_mat4(v120, glsl_type::mat4_type),
                  add_function("inverse",
               _inverse_mat2(v140_or_es3, glsl_type::mat2_type),
   _inverse_mat3(v140_or_es3, glsl_type::mat3_type),
   _inverse_mat4(v140_or_es3, glsl_type::mat4_type),
   _inverse_mat2(fp64, glsl_type::dmat2_type),
   _inverse_mat3(fp64, glsl_type::dmat3_type),
   add_function("transpose",
               _transpose(v120, glsl_type::mat2_type),
   _transpose(v120, glsl_type::mat3_type),
   _transpose(v120, glsl_type::mat4_type),
   _transpose(v120, glsl_type::mat2x3_type),
   _transpose(v120, glsl_type::mat2x4_type),
   _transpose(v120, glsl_type::mat3x2_type),
   _transpose(v120, glsl_type::mat3x4_type),
   _transpose(v120, glsl_type::mat4x2_type),
   _transpose(v120, glsl_type::mat4x3_type),
   _transpose(fp64, glsl_type::dmat2_type),
   _transpose(fp64, glsl_type::dmat3_type),
   _transpose(fp64, glsl_type::dmat4_type),
   _transpose(fp64, glsl_type::dmat2x3_type),
   _transpose(fp64, glsl_type::dmat2x4_type),
   _transpose(fp64, glsl_type::dmat3x2_type),
   _transpose(fp64, glsl_type::dmat3x4_type),
   _transpose(fp64, glsl_type::dmat4x2_type),
   FIUD_VEC(lessThan)
   FIUD_VEC(lessThanEqual)
   FIUD_VEC(greaterThan)
   FIUD_VEC(greaterThanEqual)
   FIUBD_VEC(notEqual)
            add_function("any",
               _any(glsl_type::bvec2_type),
            add_function("all",
               _all(glsl_type::bvec2_type),
            add_function("not",
               _not(glsl_type::bvec2_type),
            add_function("textureSize",
                                                                                                                        _textureSize(v130, glsl_type::ivec2_type, glsl_type::sampler1DArray_type),
   _textureSize(v130, glsl_type::ivec2_type, glsl_type::isampler1DArray_type),
   _textureSize(v130, glsl_type::ivec2_type, glsl_type::usampler1DArray_type),
                                       _textureSize(texture_cube_map_array, glsl_type::ivec3_type, glsl_type::samplerCubeArray_type),
                        _textureSize(v130, glsl_type::ivec2_type, glsl_type::sampler2DRect_type),
                        _textureSize(texture_buffer, glsl_type::int_type,   glsl_type::samplerBuffer_type),
   _textureSize(texture_buffer, glsl_type::int_type,   glsl_type::isamplerBuffer_type),
   _textureSize(texture_buffer, glsl_type::int_type,   glsl_type::usamplerBuffer_type),
                                                add_function("textureSize1D",
               _textureSize(gpu_shader4, glsl_type::int_type,   glsl_type::sampler1D_type),
            add_function("textureSize2D",
               _textureSize(gpu_shader4, glsl_type::ivec2_type, glsl_type::sampler2D_type),
            add_function("textureSize3D",
               _textureSize(gpu_shader4, glsl_type::ivec3_type, glsl_type::sampler3D_type),
            add_function("textureSizeCube",
               _textureSize(gpu_shader4, glsl_type::ivec2_type, glsl_type::samplerCube_type),
            add_function("textureSize1DArray",
               _textureSize(gpu_shader4_array,         glsl_type::ivec2_type, glsl_type::sampler1DArray_type),
            add_function("textureSize2DArray",
               _textureSize(gpu_shader4_array,         glsl_type::ivec3_type, glsl_type::sampler2DArray_type),
            add_function("textureSize2DRect",
               _textureSize(gpu_shader4_rect,         glsl_type::ivec2_type, glsl_type::sampler2DRect_type),
            add_function("textureSizeBuffer",
               _textureSize(gpu_shader4_tbo,         glsl_type::int_type,   glsl_type::samplerBuffer_type),
            add_function("textureSamples",
                                    _textureSamples(shader_samples, glsl_type::sampler2DMSArray_type),
            add_function("texture",
                                                                                                                                                                                       _texture(ir_tex, v130, glsl_type::float_type, glsl_type::sampler1DArrayShadow_type, glsl_type::vec3_type),
   _texture(ir_tex, v130, glsl_type::float_type, glsl_type::sampler2DArrayShadow_type, glsl_type::vec4_type),
   /* samplerCubeArrayShadow is special; it has an extra parameter
                                                                                                                                                                                                                                                                     add_function("textureLod",
                                                                                                                                                                                 _texture(ir_txl, v130, glsl_type::float_type, glsl_type::sampler1DArrayShadow_type, glsl_type::vec3_type),
   _texture(ir_txl, v130_or_gpu_shader4_and_tex_shadow_lod, glsl_type::float_type, glsl_type::sampler2DArrayShadow_type, glsl_type::vec4_type),
            add_function("textureOffset",
                                                                                                                                                                     _texture(ir_tex, v130, glsl_type::float_type, glsl_type::sampler1DArrayShadow_type, glsl_type::vec3_type, TEX_OFFSET),
   /* The next one was forgotten in GLSL 1.30 spec. It's from
   * EXT_gpu_shader4 originally. It was added in 4.30 with the
   * wrong syntax. This was corrected in 4.40. 4.30 indicates
   * that it was intended to be included previously, so allow it
                                                                                                                                                _texture(ir_txb, v130_derivatives_only, glsl_type::float_type, glsl_type::sampler1DArrayShadow_type, glsl_type::vec3_type, TEX_OFFSET),
            add_function("texture1DOffset",
               _texture(ir_tex, gpu_shader4,             glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::float_type, TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::float_type, TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::float_type, TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_derivs_only, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::float_type, TEX_OFFSET),
            add_function("texture2DOffset",
               _texture(ir_tex, gpu_shader4,             glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec2_type, TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec2_type, TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec2_type, TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_derivs_only, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec2_type, TEX_OFFSET),
            add_function("texture3DOffset",
               _texture(ir_tex, gpu_shader4,             glsl_type::vec4_type,  glsl_type::sampler3D_type,  glsl_type::vec3_type, TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::ivec4_type, glsl_type::isampler3D_type, glsl_type::vec3_type, TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::uvec4_type, glsl_type::usampler3D_type, glsl_type::vec3_type, TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_derivs_only, glsl_type::vec4_type,  glsl_type::sampler3D_type,  glsl_type::vec3_type, TEX_OFFSET),
            add_function("texture2DRectOffset",
               _texture(ir_tex, gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::vec2_type, TEX_OFFSET),
            add_function("shadow2DRectOffset",
                  add_function("shadow1DOffset",
                        add_function("shadow2DOffset",
                        add_function("texture1DArrayOffset",
               _texture(ir_tex, gpu_shader4_array,                     glsl_type::vec4_type,  glsl_type::sampler1DArray_type,  glsl_type::vec2_type, TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_array_integer,             glsl_type::ivec4_type, glsl_type::isampler1DArray_type, glsl_type::vec2_type, TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_array_integer,             glsl_type::uvec4_type, glsl_type::usampler1DArray_type, glsl_type::vec2_type, TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_array_derivs_only,         glsl_type::vec4_type,  glsl_type::sampler1DArray_type,  glsl_type::vec2_type, TEX_OFFSET),
            add_function("texture2DArrayOffset",
               _texture(ir_tex, gpu_shader4_array,                     glsl_type::vec4_type,  glsl_type::sampler2DArray_type,  glsl_type::vec3_type, TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_array_integer,             glsl_type::ivec4_type, glsl_type::isampler2DArray_type, glsl_type::vec3_type, TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_array_integer,             glsl_type::uvec4_type, glsl_type::usampler2DArray_type, glsl_type::vec3_type, TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_array_derivs_only,         glsl_type::vec4_type,  glsl_type::sampler2DArray_type,  glsl_type::vec3_type, TEX_OFFSET),
            add_function("shadow1DArrayOffset",
                        add_function("shadow2DArrayOffset",
                  add_function("textureProj",
               _texture(ir_tex, v130, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_tex, v130, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_tex, v130, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
                        _texture(ir_tex, v130, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_tex, v130, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_tex, v130, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
                                                            _texture(ir_tex, v130, glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::vec3_type, TEX_PROJECT),
                        _texture(ir_tex, v130, glsl_type::uvec4_type, glsl_type::usampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT),
                                 _texture(ir_txb, v130_derivatives_only, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txb, v130_derivatives_only, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txb, v130_derivatives_only, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
                        _texture(ir_txb, v130_derivatives_only, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txb, v130_derivatives_only, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txb, v130_derivatives_only, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
                                                      add_function("texelFetch",
                                                                                                                                                                                                                  add_function("texelFetch1D",
               _texelFetch(gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::int_type),
            add_function("texelFetch2D",
               _texelFetch(gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::ivec2_type),
            add_function("texelFetch3D",
               _texelFetch(gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler3D_type,  glsl_type::ivec3_type),
            add_function("texelFetch2DRect",
               _texelFetch(gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::ivec2_type),
            add_function("texelFetch1DArray",
               _texelFetch(gpu_shader4_array,         glsl_type::vec4_type,  glsl_type::sampler1DArray_type,  glsl_type::ivec2_type),
            add_function("texelFetch2DArray",
               _texelFetch(gpu_shader4_array,         glsl_type::vec4_type,  glsl_type::sampler2DArray_type,  glsl_type::ivec3_type),
            add_function("texelFetchBuffer",
               _texelFetch(gpu_shader4_tbo,         glsl_type::vec4_type,  glsl_type::samplerBuffer_type,  glsl_type::int_type),
            add_function("texelFetchOffset",
                                                                                                                                          add_function("texelFetch1DOffset",
               _texelFetch(gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::int_type, glsl_type::int_type),
            add_function("texelFetch2DOffset",
               _texelFetch(gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::ivec2_type, glsl_type::ivec2_type),
            add_function("texelFetch3DOffset",
               _texelFetch(gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler3D_type,  glsl_type::ivec3_type, glsl_type::ivec3_type),
            add_function("texelFetch2DRectOffset",
               _texelFetch(gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::ivec2_type, glsl_type::ivec2_type),
            add_function("texelFetch1DArrayOffset",
               _texelFetch(gpu_shader4_array,         glsl_type::vec4_type,  glsl_type::sampler1DArray_type,  glsl_type::ivec2_type, glsl_type::int_type),
            add_function("texelFetch2DArrayOffset",
               _texelFetch(gpu_shader4_array,         glsl_type::vec4_type,  glsl_type::sampler2DArray_type,  glsl_type::ivec3_type, glsl_type::ivec2_type),
            add_function("textureProjOffset",
               _texture(ir_tex, v130, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, v130, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, v130, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
                        _texture(ir_tex, v130, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, v130, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, v130, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
                                                            _texture(ir_tex, v130, glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, v130, glsl_type::ivec4_type, glsl_type::isampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, v130, glsl_type::uvec4_type, glsl_type::usampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
                                 _texture(ir_txb, v130_derivatives_only, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, v130_derivatives_only, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, v130_derivatives_only, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
                        _texture(ir_txb, v130_derivatives_only, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, v130_derivatives_only, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, v130_derivatives_only, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
                                                      add_function("texture1DProjOffset",
               _texture(ir_tex, gpu_shader4,             glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4,             glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_derivs_only, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_derivs_only, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("texture2DProjOffset",
               _texture(ir_tex, gpu_shader4,             glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4,             glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_derivs_only, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_derivs_only, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("texture3DProjOffset",
               _texture(ir_tex, gpu_shader4,             glsl_type::vec4_type,  glsl_type::sampler3D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::ivec4_type, glsl_type::isampler3D_type, glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_integer,     glsl_type::uvec4_type, glsl_type::usampler3D_type, glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txb, gpu_shader4_derivs_only, glsl_type::vec4_type,  glsl_type::sampler3D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("shadow1DProjOffset",
                        add_function("shadow2DProjOffset",
                        add_function("texture2DRectProjOffset",
               _texture(ir_tex, gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_rect_integer, glsl_type::ivec4_type, glsl_type::isampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_rect_integer, glsl_type::uvec4_type, glsl_type::usampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_tex, gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("shadow2DRectProjOffset",
                  add_function("textureLodOffset",
                                                                                                                                                add_function("texture1DLodOffset",
               _texture(ir_txl, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::float_type, TEX_OFFSET),
            add_function("texture2DLodOffset",
               _texture(ir_txl, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec2_type, TEX_OFFSET),
            add_function("texture3DLodOffset",
               _texture(ir_txl, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler3D_type,  glsl_type::vec3_type, TEX_OFFSET),
            add_function("shadow1DLodOffset",
                  add_function("shadow2DLodOffset",
                  add_function("texture1DArrayLodOffset",
               _texture(ir_txl, gpu_shader4_array,         glsl_type::vec4_type,  glsl_type::sampler1DArray_type,  glsl_type::vec2_type, TEX_OFFSET),
            add_function("texture2DArrayLodOffset",
               _texture(ir_txl, gpu_shader4_array,         glsl_type::vec4_type,  glsl_type::sampler2DArray_type,  glsl_type::vec3_type, TEX_OFFSET),
            add_function("shadow1DArrayLodOffset",
                  add_function("textureProjLod",
               _texture(ir_txl, v130, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txl, v130, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txl, v130, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
                        _texture(ir_txl, v130, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txl, v130, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txl, v130, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
                                                      add_function("textureProjLodOffset",
               _texture(ir_txl, v130, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txl, v130, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txl, v130, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
                        _texture(ir_txl, v130, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txl, v130, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txl, v130, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
                                                      add_function("texture1DProjLodOffset",
               _texture(ir_txl, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txl, gpu_shader4_integer, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txl, gpu_shader4_integer, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txl, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("texture2DProjLodOffset",
               _texture(ir_txl, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txl, gpu_shader4_integer, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txl, gpu_shader4_integer, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txl, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("texture3DProjLodOffset",
               _texture(ir_txl, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler3D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("shadow1DProjLodOffset",
                  add_function("shadow2DProjLodOffset",
                  add_function("textureGrad",
                                                                                                                                                                                                                              add_function("textureGradOffset",
                                                                                                                                                                              add_function("texture1DGradOffset",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::float_type, TEX_OFFSET),
            add_function("texture2DGradOffset",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec2_type, TEX_OFFSET),
            add_function("texture3DGradOffset",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler3D_type,  glsl_type::vec3_type, TEX_OFFSET),
            add_function("texture2DRectGradOffset",
               _texture(ir_txd, gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::vec2_type, TEX_OFFSET),
            add_function("shadow2DRectGradOffset",
                  add_function("shadow1DGradOffset",
                  add_function("shadow2DGradOffset",
                  add_function("texture1DArrayGradOffset",
               _texture(ir_txd, gpu_shader4_array,         glsl_type::vec4_type,  glsl_type::sampler1DArray_type,  glsl_type::vec2_type, TEX_OFFSET),
            add_function("texture2DArrayGradOffset",
               _texture(ir_txd, gpu_shader4_array,         glsl_type::vec4_type,  glsl_type::sampler2DArray_type,  glsl_type::vec3_type, TEX_OFFSET),
            add_function("shadow1DArrayGradOffset",
                  add_function("shadow2DArrayGradOffset",
                  add_function("textureProjGrad",
               _texture(ir_txd, v130, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txd, v130, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txd, v130, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
                        _texture(ir_txd, v130, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txd, v130, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txd, v130, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
                                             _texture(ir_txd, v130, glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txd, v130, glsl_type::ivec4_type, glsl_type::isampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txd, v130, glsl_type::uvec4_type, glsl_type::usampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT),
                                          add_function("textureProjGradOffset",
               _texture(ir_txd, v130, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, v130, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, v130, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
                        _texture(ir_txd, v130, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, v130, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, v130, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
                                             _texture(ir_txd, v130, glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, v130, glsl_type::ivec4_type, glsl_type::isampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, v130, glsl_type::uvec4_type, glsl_type::usampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
                                          add_function("texture1DProjGradOffset",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, gpu_shader4_integer, glsl_type::ivec4_type, glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, gpu_shader4_integer, glsl_type::uvec4_type, glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("texture2DProjGradOffset",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, gpu_shader4_integer, glsl_type::ivec4_type, glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, gpu_shader4_integer, glsl_type::uvec4_type, glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("texture3DProjGradOffset",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler3D_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("texture2DRectProjGradOffset",
               _texture(ir_txd, gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, gpu_shader4_rect_integer, glsl_type::ivec4_type, glsl_type::isampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, gpu_shader4_rect_integer, glsl_type::uvec4_type, glsl_type::usampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT | TEX_OFFSET),
   _texture(ir_txd, gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type,  glsl_type::vec4_type, TEX_PROJECT | TEX_OFFSET),
            add_function("shadow2DRectProjGradOffset",
                  add_function("shadow1DProjGradOffset",
                  add_function("shadow2DProjGradOffset",
                  add_function("EmitVertex",   _EmitVertex(),   NULL);
   add_function("EndPrimitive", _EndPrimitive(), NULL);
   add_function("EmitStreamVertex",
               _EmitStreamVertex(gs_streams, glsl_type::uint_type),
   add_function("EndStreamPrimitive",
               _EndStreamPrimitive(gs_streams, glsl_type::uint_type),
            add_function("textureQueryLOD",
                                                                                                                                                                  _textureQueryLod(texture_query_lod, glsl_type::sampler1DShadow_type, glsl_type::float_type),
   _textureQueryLod(texture_query_lod, glsl_type::sampler2DShadow_type, glsl_type::vec2_type),
   _textureQueryLod(texture_query_lod, glsl_type::samplerCubeShadow_type, glsl_type::vec3_type),
   _textureQueryLod(texture_query_lod, glsl_type::sampler1DArrayShadow_type, glsl_type::float_type),
            add_function("textureQueryLod",
                                                                                                                                                                  _textureQueryLod(v400_derivatives_only, glsl_type::sampler1DShadow_type, glsl_type::float_type),
   _textureQueryLod(v400_derivatives_only, glsl_type::sampler2DShadow_type, glsl_type::vec2_type),
   _textureQueryLod(v400_derivatives_only, glsl_type::samplerCubeShadow_type, glsl_type::vec3_type),
   _textureQueryLod(v400_derivatives_only, glsl_type::sampler1DArrayShadow_type, glsl_type::float_type),
            add_function("textureQueryLevels",
               _textureQueryLevels(texture_query_levels, glsl_type::sampler1D_type),
   _textureQueryLevels(texture_query_levels, glsl_type::sampler2D_type),
   _textureQueryLevels(texture_query_levels, glsl_type::sampler3D_type),
   _textureQueryLevels(texture_query_levels, glsl_type::samplerCube_type),
   _textureQueryLevels(texture_query_levels, glsl_type::sampler1DArray_type),
   _textureQueryLevels(texture_query_levels, glsl_type::sampler2DArray_type),
   _textureQueryLevels(texture_query_levels, glsl_type::samplerCubeArray_type),
   _textureQueryLevels(texture_query_levels, glsl_type::sampler1DShadow_type),
   _textureQueryLevels(texture_query_levels, glsl_type::sampler2DShadow_type),
   _textureQueryLevels(texture_query_levels, glsl_type::samplerCubeShadow_type),
                        _textureQueryLevels(texture_query_levels, glsl_type::isampler1D_type),
   _textureQueryLevels(texture_query_levels, glsl_type::isampler2D_type),
   _textureQueryLevels(texture_query_levels, glsl_type::isampler3D_type),
   _textureQueryLevels(texture_query_levels, glsl_type::isamplerCube_type),
                        _textureQueryLevels(texture_query_levels, glsl_type::usampler1D_type),
   _textureQueryLevels(texture_query_levels, glsl_type::usampler2D_type),
   _textureQueryLevels(texture_query_levels, glsl_type::usampler3D_type),
   _textureQueryLevels(texture_query_levels, glsl_type::usamplerCube_type),
                     add_function("textureSamplesIdenticalEXT",
                                    _textureSamplesIdentical(texture_samples_identical_array, glsl_type::sampler2DMSArray_type,  glsl_type::ivec3_type),
            add_function("texture1D",
               _texture(ir_tex, v110_deprecated_texture,                      glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::float_type),
   _texture(ir_txb, v110_derivatives_only_deprecated_texture,     glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::float_type),
   _texture(ir_tex, gpu_shader4_integer,               glsl_type::ivec4_type,  glsl_type::isampler1D_type, glsl_type::float_type),
   _texture(ir_txb, gpu_shader4_integer_derivs_only,   glsl_type::ivec4_type,  glsl_type::isampler1D_type, glsl_type::float_type),
            add_function("texture1DArray",
               _texture(ir_tex, texture_array,           glsl_type::vec4_type, glsl_type::sampler1DArray_type, glsl_type::vec2_type),
   _texture(ir_txb, texture_array_derivs_only,glsl_type::vec4_type, glsl_type::sampler1DArray_type, glsl_type::vec2_type),
   _texture(ir_tex, gpu_shader4_array_integer,             glsl_type::ivec4_type, glsl_type::isampler1DArray_type, glsl_type::vec2_type),
   _texture(ir_txb, gpu_shader4_array_integer_derivs_only, glsl_type::ivec4_type, glsl_type::isampler1DArray_type, glsl_type::vec2_type),
            add_function("texture1DProj",
               _texture(ir_tex, v110_deprecated_texture,                  glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_tex, v110_deprecated_texture,                  glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txb, v110_derivatives_only_deprecated_texture, glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txb, v110_derivatives_only_deprecated_texture, glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::ivec4_type,  glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::ivec4_type,  glsl_type::isampler1D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::ivec4_type,  glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::ivec4_type,  glsl_type::isampler1D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::uvec4_type,  glsl_type::usampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::uvec4_type,  glsl_type::usampler1D_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("texture1DLod",
               _texture(ir_txl, v110_lod_deprecated_texture, glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::float_type),
            add_function("texture1DArrayLod",
               _texture(ir_txl, texture_array_lod, glsl_type::vec4_type, glsl_type::sampler1DArray_type, glsl_type::vec2_type),
            add_function("texture1DProjLod",
               _texture(ir_txl, v110_lod_deprecated_texture, glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txl, v110_lod_deprecated_texture, glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txl, gpu_shader4_integer, glsl_type::ivec4_type,  glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txl, gpu_shader4_integer, glsl_type::ivec4_type,  glsl_type::isampler1D_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("texture2D",
               _texture(ir_tex, deprecated_texture,                  glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec2_type),
   _texture(ir_txb, deprecated_texture_derivatives_only, glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec2_type),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::ivec4_type,  glsl_type::isampler2D_type, glsl_type::vec2_type),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::ivec4_type,  glsl_type::isampler2D_type, glsl_type::vec2_type),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::uvec4_type,  glsl_type::usampler2D_type, glsl_type::vec2_type),
            add_function("texture2DArray",
               _texture(ir_tex, texture_array,           glsl_type::vec4_type, glsl_type::sampler2DArray_type, glsl_type::vec3_type),
   _texture(ir_txb, texture_array_derivs_only, glsl_type::vec4_type, glsl_type::sampler2DArray_type, glsl_type::vec3_type),
   _texture(ir_tex, gpu_shader4_array_integer,             glsl_type::ivec4_type, glsl_type::isampler2DArray_type, glsl_type::vec3_type),
   _texture(ir_txb, gpu_shader4_array_integer_derivs_only, glsl_type::ivec4_type, glsl_type::isampler2DArray_type, glsl_type::vec3_type),
            add_function("texture2DProj",
               _texture(ir_tex, deprecated_texture,                  glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_tex, deprecated_texture,                  glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txb, deprecated_texture_derivatives_only, glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txb, deprecated_texture_derivatives_only, glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::ivec4_type,  glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::ivec4_type,  glsl_type::isampler2D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::ivec4_type,  glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::ivec4_type,  glsl_type::isampler2D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::uvec4_type,  glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::uvec4_type,  glsl_type::usampler2D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::uvec4_type,  glsl_type::usampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::uvec4_type,  glsl_type::usampler2D_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("texture2DLod",
               _texture(ir_txl, lod_deprecated_texture, glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec2_type),
            add_function("texture2DArrayLod",
               _texture(ir_txl, texture_array_lod, glsl_type::vec4_type, glsl_type::sampler2DArray_type, glsl_type::vec3_type),
            add_function("texture2DProjLod",
               _texture(ir_txl, lod_deprecated_texture, glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txl, lod_deprecated_texture, glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txl, gpu_shader4_integer, glsl_type::ivec4_type,  glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txl, gpu_shader4_integer, glsl_type::ivec4_type,  glsl_type::isampler2D_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("texture3D",
               _texture(ir_tex, tex3d,                   glsl_type::vec4_type,  glsl_type::sampler3D_type, glsl_type::vec3_type),
   _texture(ir_txb, derivatives_tex3d,       glsl_type::vec4_type,  glsl_type::sampler3D_type, glsl_type::vec3_type),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::ivec4_type, glsl_type::isampler3D_type, glsl_type::vec3_type),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::ivec4_type, glsl_type::isampler3D_type, glsl_type::vec3_type),
            add_function("texture3DProj",
               _texture(ir_tex, tex3d,                   glsl_type::vec4_type,  glsl_type::sampler3D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txb, derivatives_tex3d,       glsl_type::vec4_type,  glsl_type::sampler3D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::ivec4_type, glsl_type::isampler3D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::ivec4_type, glsl_type::isampler3D_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("texture3DLod",
               _texture(ir_txl, tex3d_lod, glsl_type::vec4_type,  glsl_type::sampler3D_type, glsl_type::vec3_type),
            add_function("texture3DProjLod",
               _texture(ir_txl, tex3d_lod, glsl_type::vec4_type,  glsl_type::sampler3D_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("textureCube",
               _texture(ir_tex, deprecated_texture,                  glsl_type::vec4_type,  glsl_type::samplerCube_type, glsl_type::vec3_type),
   _texture(ir_txb, deprecated_texture_derivatives_only, glsl_type::vec4_type,  glsl_type::samplerCube_type, glsl_type::vec3_type),
   _texture(ir_tex, gpu_shader4_integer,             glsl_type::ivec4_type,  glsl_type::isamplerCube_type, glsl_type::vec3_type),
   _texture(ir_txb, gpu_shader4_integer_derivs_only, glsl_type::ivec4_type,  glsl_type::isamplerCube_type, glsl_type::vec3_type),
            add_function("textureCubeLod",
               _texture(ir_txl, lod_deprecated_texture, glsl_type::vec4_type,  glsl_type::samplerCube_type, glsl_type::vec3_type),
            add_function("texture2DRect",
               _texture(ir_tex, texture_rectangle, glsl_type::vec4_type,  glsl_type::sampler2DRect_type, glsl_type::vec2_type),
            add_function("texture2DRectProj",
               _texture(ir_tex, texture_rectangle, glsl_type::vec4_type,  glsl_type::sampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_tex, texture_rectangle, glsl_type::vec4_type,  glsl_type::sampler2DRect_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_rect_integer, glsl_type::ivec4_type,  glsl_type::isampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_tex, gpu_shader4_rect_integer, glsl_type::ivec4_type,  glsl_type::isampler2DRect_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("shadow1D",
                        add_function("shadow1DArray",
                        add_function("shadow2D",
                        add_function("shadow2DArray",
                        add_function("shadow1DProj",
                        add_function("shadow2DArray",
                        add_function("shadowCube",
                        add_function("shadow2DProj",
                        add_function("shadow1DLod",
                  add_function("shadow2DLod",
                  add_function("shadow1DArrayLod",
                  add_function("shadow1DProjLod",
                  add_function("shadow2DProjLod",
                  add_function("shadow2DRect",
                  add_function("shadow2DRectProj",
                  add_function("texture1DGradARB",
                  add_function("texture1DProjGradARB",
                        add_function("texture2DGradARB",
                  add_function("texture2DProjGradARB",
                        add_function("texture3DGradARB",
                  add_function("texture3DProjGradARB",
                  add_function("textureCubeGradARB",
                  add_function("shadow1DGradARB",
                  add_function("shadow1DProjGradARB",
                  add_function("shadow2DGradARB",
                  add_function("shadow2DProjGradARB",
                  add_function("texture2DRectGradARB",
                  add_function("texture2DRectProjGradARB",
                        add_function("shadow2DRectGradARB",
                  add_function("shadow2DRectProjGradARB",
                  add_function("texture4",
                  add_function("texture1DGrad",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::float_type),
            add_function("texture1DProjGrad",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler1D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txd, gpu_shader4_integer, glsl_type::ivec4_type,  glsl_type::isampler1D_type, glsl_type::vec2_type, TEX_PROJECT),
   _texture(ir_txd, gpu_shader4_integer, glsl_type::ivec4_type,  glsl_type::isampler1D_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("texture1DArrayGrad",
               _texture(ir_txd, gpu_shader4_array,         glsl_type::vec4_type,  glsl_type::sampler1DArray_type, glsl_type::vec2_type),
            add_function("texture2DGrad",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec2_type),
            add_function("texture2DProjGrad",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler2D_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txd, gpu_shader4_integer, glsl_type::ivec4_type,  glsl_type::isampler2D_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txd, gpu_shader4_integer, glsl_type::ivec4_type,  glsl_type::isampler2D_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("texture2DArrayGrad",
               _texture(ir_txd, gpu_shader4_array,         glsl_type::vec4_type,  glsl_type::sampler2DArray_type, glsl_type::vec3_type),
            add_function("texture3DGrad",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler3D_type, glsl_type::vec3_type),
            add_function("texture3DProjGrad",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::sampler3D_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("textureCubeGrad",
               _texture(ir_txd, gpu_shader4, glsl_type::vec4_type,  glsl_type::samplerCube_type, glsl_type::vec3_type),
            add_function("shadow1DGrad",
                  add_function("shadow1DProjGrad",
                  add_function("shadow1DArrayGrad",
                  add_function("shadow2DGrad",
                  add_function("shadow2DProjGrad",
                  add_function("shadow2DArrayGrad",
                  add_function("texture2DRectGrad",
               _texture(ir_txd, gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type, glsl_type::vec2_type),
            add_function("texture2DRectProjGrad",
               _texture(ir_txd, gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txd, gpu_shader4_rect,         glsl_type::vec4_type,  glsl_type::sampler2DRect_type, glsl_type::vec4_type, TEX_PROJECT),
   _texture(ir_txd, gpu_shader4_rect_integer, glsl_type::ivec4_type,  glsl_type::isampler2DRect_type, glsl_type::vec3_type, TEX_PROJECT),
   _texture(ir_txd, gpu_shader4_rect_integer, glsl_type::ivec4_type,  glsl_type::isampler2DRect_type, glsl_type::vec4_type, TEX_PROJECT),
            add_function("shadow2DRectGrad",
                  add_function("shadow2DRectProjGrad",
                  add_function("shadowCubeGrad",
                  add_function("textureGather",
                                                                                                                                                                                                                                 _texture(ir_tg4, gpu_shader5_or_es31, glsl_type::vec4_type, glsl_type::sampler2DShadow_type, glsl_type::vec2_type),
   _texture(ir_tg4, gpu_shader5_or_es31, glsl_type::vec4_type, glsl_type::sampler2DArrayShadow_type, glsl_type::vec3_type),
   _texture(ir_tg4, gpu_shader5_or_es31, glsl_type::vec4_type, glsl_type::samplerCubeShadow_type, glsl_type::vec3_type),
            add_function("textureGatherOffset",
                                                                                                                                                                                                                                                               add_function("textureGatherOffsets",
                                                                                                                                             _texture(ir_tg4, gpu_shader5_es, glsl_type::vec4_type, glsl_type::sampler2DShadow_type, glsl_type::vec2_type, TEX_OFFSET_ARRAY),
            add_function("sparseTextureARB",
                                                                                                                                                                                                                                                                                          _texture(ir_txb, derivatives_texture_cube_map_array_and_sparse, glsl_type::vec4_type,  glsl_type::samplerCubeArray_type,  glsl_type::vec4_type, TEX_SPARSE),
            add_function("sparseTextureLodARB",
                                                                                                            _texture(ir_txl, texture_cube_map_array_and_sparse, glsl_type::vec4_type,  glsl_type::samplerCubeArray_type,  glsl_type::vec4_type, TEX_SPARSE),
            add_function("sparseTextureOffsetARB",
                                                                                                                                                                                 _texture(ir_txb, v130_derivatives_only_and_sparse, glsl_type::vec4_type,  glsl_type::sampler2DArray_type,  glsl_type::vec3_type, TEX_OFFSET|TEX_SPARSE),
            add_function("sparseTexelFetchARB",
                                                                                                                        _texelFetch(texture_multisample_array_and_sparse, glsl_type::vec4_type,  glsl_type::sampler2DMSArray_type,  glsl_type::ivec3_type, NULL, true),
            add_function("sparseTexelFetchOffsetARB",
                                                                              _texelFetch(v130_desktop_and_sparse, glsl_type::vec4_type,  glsl_type::sampler2DArray_type,  glsl_type::ivec3_type, glsl_type::ivec2_type, true),
            add_function("sparseTextureLodOffsetARB",
                                                                  _texture(ir_txl, v130_desktop_and_sparse, glsl_type::vec4_type,  glsl_type::sampler2DArray_type,  glsl_type::vec3_type, TEX_OFFSET|TEX_SPARSE),
            add_function("sparseTextureGradARB",
                                                                                                                                                                        add_function("sparseTextureGradOffsetARB",
                                                                                                                        add_function("sparseTextureGatherARB",
                                                                                                                                                                                                                                 _texture(ir_tg4, gpu_shader5_and_sparse, glsl_type::vec4_type, glsl_type::sampler2DShadow_type,        glsl_type::vec2_type, TEX_SPARSE),
   _texture(ir_tg4, gpu_shader5_and_sparse, glsl_type::vec4_type, glsl_type::sampler2DArrayShadow_type,   glsl_type::vec3_type, TEX_SPARSE),
   _texture(ir_tg4, gpu_shader5_and_sparse, glsl_type::vec4_type, glsl_type::samplerCubeShadow_type,      glsl_type::vec3_type, TEX_SPARSE),
            add_function("sparseTextureGatherOffsetARB",
                                                                                                                                             _texture(ir_tg4, gpu_shader5_and_sparse, glsl_type::vec4_type, glsl_type::sampler2DShadow_type,      glsl_type::vec2_type, TEX_OFFSET_NONCONST|TEX_SPARSE),
            add_function("sparseTextureGatherOffsetsARB",
                                                                                                                                             _texture(ir_tg4, gpu_shader5_and_sparse, glsl_type::vec4_type, glsl_type::sampler2DShadow_type,      glsl_type::vec2_type, TEX_OFFSET_ARRAY|TEX_SPARSE),
                     add_function("sparseTextureClampARB",
                                                                                                                                                                                                                                                            _texture(ir_txb, derivatives_texture_cube_map_array_and_clamp, glsl_type::vec4_type,  glsl_type::samplerCubeArray_type,  glsl_type::vec4_type, TEX_SPARSE|TEX_CLAMP),
            add_function("textureClampARB",
                                                                                                                                                                                                                                                                                                                                                                                          add_function("sparseTextureOffsetClampARB",
                                                                                                                                                   _texture(ir_txb, v130_derivatives_only_and_clamp, glsl_type::vec4_type,  glsl_type::sampler2DArray_type,  glsl_type::vec3_type, TEX_OFFSET|TEX_SPARSE|TEX_CLAMP),
            add_function("textureOffsetClampARB",
                                                                                                                                                                                                                                                            add_function("sparseTextureGradClampARB",
                                                                                                                                          add_function("textureGradClampARB",
                                                                                                                                                                                                add_function("sparseTextureGradOffsetClampARB",
                                                                                          add_function("textureGradOffsetClampARB",
                                                                                                                                                F(dFdx)
   F(dFdy)
   F(fwidth)
   F(dFdxCoarse)
   F(dFdyCoarse)
   F(fwidthCoarse)
   F(dFdxFine)
   F(dFdyFine)
   F(fwidthFine)
   F(noise1)
   F(noise2)
   F(noise3)
            IU(bitfieldExtract)
   IU(bitfieldInsert)
   IU(bitfieldReverse)
   IU(bitCount)
   IU(findLSB)
   IU(findMSB)
            add_function("ldexp",
               _ldexp(glsl_type::float_type, glsl_type::int_type),
   _ldexp(glsl_type::vec2_type,  glsl_type::ivec2_type),
   _ldexp(glsl_type::vec3_type,  glsl_type::ivec3_type),
   _ldexp(glsl_type::vec4_type,  glsl_type::ivec4_type),
   _ldexp(glsl_type::double_type, glsl_type::int_type),
   _ldexp(glsl_type::dvec2_type,  glsl_type::ivec2_type),
            add_function("frexp",
               _frexp(glsl_type::float_type, glsl_type::int_type),
   _frexp(glsl_type::vec2_type,  glsl_type::ivec2_type),
   _frexp(glsl_type::vec3_type,  glsl_type::ivec3_type),
   _frexp(glsl_type::vec4_type,  glsl_type::ivec4_type),
   _frexp(glsl_type::double_type, glsl_type::int_type),
   _frexp(glsl_type::dvec2_type,  glsl_type::ivec2_type),
   _frexp(glsl_type::dvec3_type,  glsl_type::ivec3_type),
   add_function("uaddCarry",
               _uaddCarry(glsl_type::uint_type),
   _uaddCarry(glsl_type::uvec2_type),
   _uaddCarry(glsl_type::uvec3_type),
   add_function("usubBorrow",
               _usubBorrow(glsl_type::uint_type),
   _usubBorrow(glsl_type::uvec2_type),
   _usubBorrow(glsl_type::uvec3_type),
   add_function("imulExtended",
               _mulExtended(glsl_type::int_type),
   _mulExtended(glsl_type::ivec2_type),
   _mulExtended(glsl_type::ivec3_type),
   add_function("umulExtended",
               _mulExtended(glsl_type::uint_type),
   _mulExtended(glsl_type::uvec2_type),
   _mulExtended(glsl_type::uvec3_type),
   add_function("interpolateAtCentroid",
               _interpolateAtCentroid(glsl_type::float_type),
   _interpolateAtCentroid(glsl_type::vec2_type),
   _interpolateAtCentroid(glsl_type::vec3_type),
   add_function("interpolateAtOffset",
               _interpolateAtOffset(glsl_type::float_type),
   _interpolateAtOffset(glsl_type::vec2_type),
   _interpolateAtOffset(glsl_type::vec3_type),
   add_function("interpolateAtSample",
               _interpolateAtSample(glsl_type::float_type),
   _interpolateAtSample(glsl_type::vec2_type),
            add_function("atomicCounter",
               _atomic_counter_op("__intrinsic_atomic_read",
   add_function("atomicCounterIncrement",
               _atomic_counter_op("__intrinsic_atomic_increment",
   add_function("atomicCounterDecrement",
                        add_function("atomicCounterAddARB",
               _atomic_counter_op1("__intrinsic_atomic_add",
   add_function("atomicCounterSubtractARB",
               _atomic_counter_op1("__intrinsic_atomic_sub",
   add_function("atomicCounterMinARB",
               _atomic_counter_op1("__intrinsic_atomic_min",
   add_function("atomicCounterMaxARB",
               _atomic_counter_op1("__intrinsic_atomic_max",
   add_function("atomicCounterAndARB",
               _atomic_counter_op1("__intrinsic_atomic_and",
   add_function("atomicCounterOrARB",
               _atomic_counter_op1("__intrinsic_atomic_or",
   add_function("atomicCounterXorARB",
               _atomic_counter_op1("__intrinsic_atomic_xor",
   add_function("atomicCounterExchangeARB",
               _atomic_counter_op1("__intrinsic_atomic_exchange",
   add_function("atomicCounterCompSwapARB",
                        add_function("atomicCounterAdd",
               _atomic_counter_op1("__intrinsic_atomic_add",
   add_function("atomicCounterSubtract",
               _atomic_counter_op1("__intrinsic_atomic_sub",
   add_function("atomicCounterMin",
               _atomic_counter_op1("__intrinsic_atomic_min",
   add_function("atomicCounterMax",
               _atomic_counter_op1("__intrinsic_atomic_max",
   add_function("atomicCounterAnd",
               _atomic_counter_op1("__intrinsic_atomic_and",
   add_function("atomicCounterOr",
               _atomic_counter_op1("__intrinsic_atomic_or",
   add_function("atomicCounterXor",
               _atomic_counter_op1("__intrinsic_atomic_xor",
   add_function("atomicCounterExchange",
               _atomic_counter_op1("__intrinsic_atomic_exchange",
   add_function("atomicCounterCompSwap",
                        add_function("atomicAdd",
               _atomic_op2("__intrinsic_atomic_add",
               _atomic_op2("__intrinsic_atomic_add",
               _atomic_op2("__intrinsic_atomic_add",
               _atomic_op2("__intrinsic_atomic_add",
         add_function("atomicMin",
               _atomic_op2("__intrinsic_atomic_min",
               _atomic_op2("__intrinsic_atomic_min",
               _atomic_op2("__intrinsic_atomic_min",
               _atomic_op2("__intrinsic_atomic_min",
               _atomic_op2("__intrinsic_atomic_min",
         add_function("atomicMax",
               _atomic_op2("__intrinsic_atomic_max",
               _atomic_op2("__intrinsic_atomic_max",
               _atomic_op2("__intrinsic_atomic_max",
               _atomic_op2("__intrinsic_atomic_max",
               _atomic_op2("__intrinsic_atomic_max",
         add_function("atomicAnd",
               _atomic_op2("__intrinsic_atomic_and",
               _atomic_op2("__intrinsic_atomic_and",
               _atomic_op2("__intrinsic_atomic_and",
               _atomic_op2("__intrinsic_atomic_and",
         add_function("atomicOr",
               _atomic_op2("__intrinsic_atomic_or",
               _atomic_op2("__intrinsic_atomic_or",
               _atomic_op2("__intrinsic_atomic_or",
               _atomic_op2("__intrinsic_atomic_or",
         add_function("atomicXor",
               _atomic_op2("__intrinsic_atomic_xor",
               _atomic_op2("__intrinsic_atomic_xor",
               _atomic_op2("__intrinsic_atomic_xor",
               _atomic_op2("__intrinsic_atomic_xor",
         add_function("atomicExchange",
               _atomic_op2("__intrinsic_atomic_exchange",
               _atomic_op2("__intrinsic_atomic_exchange",
               _atomic_op2("__intrinsic_atomic_exchange",
               _atomic_op2("__intrinsic_atomic_exchange",
         add_function("atomicCompSwap",
               _atomic_op3("__intrinsic_atomic_comp_swap",
               _atomic_op3("__intrinsic_atomic_comp_swap",
               _atomic_op3("__intrinsic_atomic_comp_swap",
               _atomic_op3("__intrinsic_atomic_comp_swap",
            add_function("min3",
               _min3(glsl_type::float_type),
                        _min3(glsl_type::int_type),
                        _min3(glsl_type::uint_type),
   _min3(glsl_type::uvec2_type),
            add_function("max3",
               _max3(glsl_type::float_type),
                        _max3(glsl_type::int_type),
                        _max3(glsl_type::uint_type),
   _max3(glsl_type::uvec2_type),
            add_function("mid3",
               _mid3(glsl_type::float_type),
                        _mid3(glsl_type::int_type),
                        _mid3(glsl_type::uint_type),
   _mid3(glsl_type::uvec2_type),
                     add_function("memoryBarrier",
               _memory_barrier("__intrinsic_memory_barrier",
   add_function("groupMemoryBarrier",
               _memory_barrier("__intrinsic_group_memory_barrier",
   add_function("memoryBarrierAtomicCounter",
               _memory_barrier("__intrinsic_memory_barrier_atomic_counter",
   add_function("memoryBarrierBuffer",
               _memory_barrier("__intrinsic_memory_barrier_buffer",
   add_function("memoryBarrierImage",
               _memory_barrier("__intrinsic_memory_barrier_image",
   add_function("memoryBarrierShared",
                                 add_function("readInvocationARB",
               _read_invocation(glsl_type::float_type),
                        _read_invocation(glsl_type::int_type),
                        _read_invocation(glsl_type::uint_type),
   _read_invocation(glsl_type::uvec2_type),
            add_function("readFirstInvocationARB",
               _read_first_invocation(glsl_type::float_type),
                        _read_first_invocation(glsl_type::int_type),
                        _read_first_invocation(glsl_type::uint_type),
   _read_first_invocation(glsl_type::uvec2_type),
            add_function("clock2x32ARB",
                        add_function("clockARB",
                        add_function("beginInvocationInterlockARB",
               _invocation_interlock(
            add_function("endInvocationInterlockARB",
               _invocation_interlock(
            add_function("beginInvocationInterlockNV",
               _invocation_interlock(
            add_function("endInvocationInterlockNV",
               _invocation_interlock(
            add_function("anyInvocationARB",
                  add_function("allInvocationsARB",
                  add_function("allInvocationsEqualARB",
                  add_function("anyInvocationEXT",
                  add_function("allInvocationsEXT",
                  add_function("allInvocationsEqualEXT",
                  add_function("anyInvocation",
                  add_function("allInvocations",
                  add_function("allInvocationsEqual",
                           add_function("countLeadingZeros",
               _countLeadingZeros(shader_integer_functions2,
         _countLeadingZeros(shader_integer_functions2,
         _countLeadingZeros(shader_integer_functions2,
                  add_function("countTrailingZeros",
               _countTrailingZeros(shader_integer_functions2,
         _countTrailingZeros(shader_integer_functions2,
         _countTrailingZeros(shader_integer_functions2,
                  add_function("absoluteDifference",
               _absoluteDifference(shader_integer_functions2,
         _absoluteDifference(shader_integer_functions2,
         _absoluteDifference(shader_integer_functions2,
         _absoluteDifference(shader_integer_functions2,
         _absoluteDifference(shader_integer_functions2,
         _absoluteDifference(shader_integer_functions2,
         _absoluteDifference(shader_integer_functions2,
                        _absoluteDifference(shader_integer_functions2_int64,
         _absoluteDifference(shader_integer_functions2_int64,
         _absoluteDifference(shader_integer_functions2_int64,
         _absoluteDifference(shader_integer_functions2_int64,
         _absoluteDifference(shader_integer_functions2_int64,
         _absoluteDifference(shader_integer_functions2_int64,
         _absoluteDifference(shader_integer_functions2_int64,
                  add_function("addSaturate",
               _addSaturate(shader_integer_functions2,
         _addSaturate(shader_integer_functions2,
         _addSaturate(shader_integer_functions2,
         _addSaturate(shader_integer_functions2,
         _addSaturate(shader_integer_functions2,
         _addSaturate(shader_integer_functions2,
         _addSaturate(shader_integer_functions2,
                        _addSaturate(shader_integer_functions2_int64,
         _addSaturate(shader_integer_functions2_int64,
         _addSaturate(shader_integer_functions2_int64,
         _addSaturate(shader_integer_functions2_int64,
         _addSaturate(shader_integer_functions2_int64,
         _addSaturate(shader_integer_functions2_int64,
         _addSaturate(shader_integer_functions2_int64,
                  add_function("average",
               _average(shader_integer_functions2,
         _average(shader_integer_functions2,
         _average(shader_integer_functions2,
         _average(shader_integer_functions2,
         _average(shader_integer_functions2,
         _average(shader_integer_functions2,
         _average(shader_integer_functions2,
                        _average(shader_integer_functions2_int64,
         _average(shader_integer_functions2_int64,
         _average(shader_integer_functions2_int64,
         _average(shader_integer_functions2_int64,
         _average(shader_integer_functions2_int64,
         _average(shader_integer_functions2_int64,
         _average(shader_integer_functions2_int64,
                  add_function("averageRounded",
               _averageRounded(shader_integer_functions2,
         _averageRounded(shader_integer_functions2,
         _averageRounded(shader_integer_functions2,
         _averageRounded(shader_integer_functions2,
         _averageRounded(shader_integer_functions2,
         _averageRounded(shader_integer_functions2,
         _averageRounded(shader_integer_functions2,
                        _averageRounded(shader_integer_functions2_int64,
         _averageRounded(shader_integer_functions2_int64,
         _averageRounded(shader_integer_functions2_int64,
         _averageRounded(shader_integer_functions2_int64,
         _averageRounded(shader_integer_functions2_int64,
         _averageRounded(shader_integer_functions2_int64,
         _averageRounded(shader_integer_functions2_int64,
                  add_function("subtractSaturate",
               _subtractSaturate(shader_integer_functions2,
         _subtractSaturate(shader_integer_functions2,
         _subtractSaturate(shader_integer_functions2,
         _subtractSaturate(shader_integer_functions2,
         _subtractSaturate(shader_integer_functions2,
         _subtractSaturate(shader_integer_functions2,
         _subtractSaturate(shader_integer_functions2,
                        _subtractSaturate(shader_integer_functions2_int64,
         _subtractSaturate(shader_integer_functions2_int64,
         _subtractSaturate(shader_integer_functions2_int64,
         _subtractSaturate(shader_integer_functions2_int64,
         _subtractSaturate(shader_integer_functions2_int64,
         _subtractSaturate(shader_integer_functions2_int64,
         _subtractSaturate(shader_integer_functions2_int64,
                  add_function("multiply32x16",
               _multiply32x16(shader_integer_functions2,
         _multiply32x16(shader_integer_functions2,
         _multiply32x16(shader_integer_functions2,
         _multiply32x16(shader_integer_functions2,
         _multiply32x16(shader_integer_functions2,
         _multiply32x16(shader_integer_functions2,
         _multiply32x16(shader_integer_functions2,
               #undef F
   #undef FI
   #undef FIUD_VEC
   #undef FIUBD_VEC
   #undef FIU2_MIXED
   }
      void
   builtin_builder::add_function(const char *name, ...)
   {
                        va_start(ap, name);
   while (true) {
      ir_function_signature *sig = va_arg(ap, ir_function_signature *);
   if (sig == NULL)
            if (false) {
      exec_list stuff;
   stuff.push_tail(sig);
                  }
               }
      void
   builtin_builder::add_image_function(const char *name,
                                 {
      static const glsl_type *const types[] = {
      glsl_type::image1D_type,
   glsl_type::image2D_type,
   glsl_type::image3D_type,
   glsl_type::image2DRect_type,
   glsl_type::imageCube_type,
   glsl_type::imageBuffer_type,
   glsl_type::image1DArray_type,
   glsl_type::image2DArray_type,
   glsl_type::imageCubeArray_type,
   glsl_type::image2DMS_type,
   glsl_type::image2DMSArray_type,
   glsl_type::iimage1D_type,
   glsl_type::iimage2D_type,
   glsl_type::iimage3D_type,
   glsl_type::iimage2DRect_type,
   glsl_type::iimageCube_type,
   glsl_type::iimageBuffer_type,
   glsl_type::iimage1DArray_type,
   glsl_type::iimage2DArray_type,
   glsl_type::iimageCubeArray_type,
   glsl_type::iimage2DMS_type,
   glsl_type::iimage2DMSArray_type,
   glsl_type::uimage1D_type,
   glsl_type::uimage2D_type,
   glsl_type::uimage3D_type,
   glsl_type::uimage2DRect_type,
   glsl_type::uimageCube_type,
   glsl_type::uimageBuffer_type,
   glsl_type::uimage1DArray_type,
   glsl_type::uimage2DArray_type,
   glsl_type::uimageCubeArray_type,
   glsl_type::uimage2DMS_type,
                        for (unsigned i = 0; i < ARRAY_SIZE(types); ++i) {
      if (types[i]->sampled_type == GLSL_TYPE_FLOAT && !(flags & IMAGE_FUNCTION_SUPPORTS_FLOAT_DATA_TYPE))
         if (types[i]->sampled_type == GLSL_TYPE_INT && !(flags & IMAGE_FUNCTION_SUPPORTS_SIGNED_DATA_TYPE))
         if ((types[i]->sampler_dimensionality != GLSL_SAMPLER_DIM_MS) && (flags & IMAGE_FUNCTION_MS_ONLY))
         if (flags & IMAGE_FUNCTION_SPARSE) {
      switch (types[i]->sampler_dimensionality) {
   case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_3D:
   case GLSL_SAMPLER_DIM_CUBE:
   case GLSL_SAMPLER_DIM_RECT:
   case GLSL_SAMPLER_DIM_MS:
         default:
            }
   f->add_signature(_image(prototype, types[i], intrinsic_name,
      }
      }
      void
   builtin_builder::add_image_functions(bool glsl)
   {
               add_image_function(glsl ? "imageLoad" : "__intrinsic_image_load",
                     "__intrinsic_image_load",
   &builtin_builder::_image_prototype, 0,
   (flags | IMAGE_FUNCTION_HAS_VECTOR_DATA_TYPE |
            add_image_function(glsl ? "imageStore" : "__intrinsic_image_store",
                     "__intrinsic_image_store",
   &builtin_builder::_image_prototype, 1,
   (flags | IMAGE_FUNCTION_RETURNS_VOID |
   IMAGE_FUNCTION_HAS_VECTOR_DATA_TYPE |
                     add_image_function(glsl ? "imageAtomicAdd" : "__intrinsic_image_atomic_add",
                     "__intrinsic_image_atomic_add",
   &builtin_builder::_image_prototype, 1,
            add_image_function(glsl ? "imageAtomicMin" : "__intrinsic_image_atomic_min",
                              add_image_function(glsl ? "imageAtomicMax" : "__intrinsic_image_atomic_max",
                              add_image_function(glsl ? "imageAtomicAnd" : "__intrinsic_image_atomic_and",
                              add_image_function(glsl ? "imageAtomicOr" : "__intrinsic_image_atomic_or",
                              add_image_function(glsl ? "imageAtomicXor" : "__intrinsic_image_atomic_xor",
                              add_image_function((glsl ? "imageAtomicExchange" :
                     "__intrinsic_image_atomic_exchange"),
   "__intrinsic_image_atomic_exchange",
   &builtin_builder::_image_prototype, 1,
            add_image_function((glsl ? "imageAtomicCompSwap" :
                     "__intrinsic_image_atomic_comp_swap"),
            add_image_function(glsl ? "imageSize" : "__intrinsic_image_size",
                     "__intrinsic_image_size",
            add_image_function(glsl ? "imageSamples" : "__intrinsic_image_samples",
                     "__intrinsic_image_samples",
   &builtin_builder::_image_samples_prototype, 1,
            /* EXT_shader_image_load_store */
   add_image_function(glsl ? "imageAtomicIncWrap" : "__intrinsic_image_atomic_inc_wrap",
                     "__intrinsic_image_atomic_inc_wrap",
   add_image_function(glsl ? "imageAtomicDecWrap" : "__intrinsic_image_atomic_dec_wrap",
                              /* ARB_sparse_texture2 */
   add_image_function(glsl ? "sparseImageLoadARB" : "__intrinsic_image_sparse_load",
                     "__intrinsic_image_sparse_load",
   &builtin_builder::_image_prototype, 0,
   (flags | IMAGE_FUNCTION_HAS_VECTOR_DATA_TYPE |
   IMAGE_FUNCTION_SUPPORTS_FLOAT_DATA_TYPE |
      }
      ir_variable *
   builtin_builder::in_var(const glsl_type *type, const char *name)
   {
         }
      ir_variable *
   builtin_builder::in_highp_var(const glsl_type *type, const char *name)
   {
      ir_variable *var = in_var(type, name);
   var->data.precision = GLSL_PRECISION_HIGH;
      }
      ir_variable *
   builtin_builder::in_mediump_var(const glsl_type *type, const char *name)
   {
      ir_variable *var = in_var(type, name);
   var->data.precision = GLSL_PRECISION_MEDIUM;
      }
      ir_variable *
   builtin_builder::out_var(const glsl_type *type, const char *name)
   {
         }
      ir_variable *
   builtin_builder::out_lowp_var(const glsl_type *type, const char *name)
   {
      ir_variable *var = out_var(type, name);
   var->data.precision = GLSL_PRECISION_LOW;
      }
      ir_variable *
   builtin_builder::out_highp_var(const glsl_type *type, const char *name)
   {
      ir_variable *var = out_var(type, name);
   var->data.precision = GLSL_PRECISION_HIGH;
      }
      ir_variable *
   builtin_builder::as_highp(ir_factory &body, ir_variable *var)
   {
      ir_variable *t = body.make_temp(var->type, "highp_tmp");
   body.emit(assign(t, var));
      }
      ir_constant *
   builtin_builder::imm(bool b, unsigned vector_elements)
   {
         }
      ir_constant *
   builtin_builder::imm(float f, unsigned vector_elements)
   {
         }
      ir_constant *
   builtin_builder::imm(int i, unsigned vector_elements)
   {
         }
      ir_constant *
   builtin_builder::imm(unsigned u, unsigned vector_elements)
   {
         }
      ir_constant *
   builtin_builder::imm(double d, unsigned vector_elements)
   {
         }
      ir_constant *
   builtin_builder::imm(const glsl_type *type, const ir_constant_data &data)
   {
         }
      #define IMM_FP(type, val) (type->is_double()) ? imm(val) : imm((float)val)
      ir_dereference_variable *
   builtin_builder::var_ref(ir_variable *var)
   {
         }
      ir_dereference_array *
   builtin_builder::array_ref(ir_variable *var, int idx)
   {
         }
      /** Return an element of a matrix */
   ir_swizzle *
   builtin_builder::matrix_elt(ir_variable *var, int column, int row)
   {
         }
      ir_dereference_record *
   builtin_builder::record_ref(ir_variable *var, const char *field)
   {
         }
      /**
   * Implementations of built-in functions:
   *  @{
   */
   ir_function_signature *
   builtin_builder::new_sig(const glsl_type *return_type,
                     {
               ir_function_signature *sig =
            exec_list plist;
   va_start(ap, num_params);
   for (int i = 0; i < num_params; i++) {
         }
            sig->replace_parameters(&plist);
      }
      #define MAKE_SIG(return_type, avail, ...)  \
      ir_function_signature *sig =               \
         ir_factory body(&sig->body, mem_ctx);             \
         #define MAKE_INTRINSIC(return_type, id, avail, ...)  \
      ir_function_signature *sig =                      \
               ir_function_signature *
   builtin_builder::unop(builtin_available_predicate avail,
                     {
      ir_variable *x = in_var(param_type, "x");
   MAKE_SIG(return_type, avail, 1, x);
   body.emit(ret(expr(opcode, x)));
      }
      #define UNOP(NAME, OPCODE, AVAIL)               \
   ir_function_signature *                         \
   builtin_builder::_##NAME(const glsl_type *type) \
   {                                               \
         }
      #define UNOPA(NAME, OPCODE)               \
   ir_function_signature *                         \
   builtin_builder::_##NAME(builtin_available_predicate avail, const glsl_type *type) \
   {                                               \
         }
      ir_function_signature *
   builtin_builder::binop(builtin_available_predicate avail,
                        ir_expression_operation opcode,
      {
      ir_variable *x = in_var(param0_type, "x");
   ir_variable *y = in_var(param1_type, "y");
            if (swap_operands)
         else
               }
      #define BINOP(NAME, OPCODE, AVAIL)                                      \
   ir_function_signature *                                                 \
   builtin_builder::_##NAME(const glsl_type *return_type,                  \
               {                                                                       \
         }
      /**
   * Angle and Trigonometry Functions @{
   */
      ir_function_signature *
   builtin_builder::_radians(const glsl_type *type)
   {
      ir_variable *degrees = in_var(type, "degrees");
   MAKE_SIG(type, always_available, 1, degrees);
   body.emit(ret(mul(degrees, imm(0.0174532925f))));
      }
      ir_function_signature *
   builtin_builder::_degrees(const glsl_type *type)
   {
      ir_variable *radians = in_var(type, "radians");
   MAKE_SIG(type, always_available, 1, radians);
   body.emit(ret(mul(radians, imm(57.29578f))));
      }
      UNOP(sin, ir_unop_sin, always_available)
   UNOP(cos, ir_unop_cos, always_available)
      ir_function_signature *
   builtin_builder::_tan(const glsl_type *type)
   {
      ir_variable *theta = in_var(type, "theta");
   MAKE_SIG(type, always_available, 1, theta);
   body.emit(ret(div(sin(theta), cos(theta))));
      }
      ir_expression *
   builtin_builder::asin_expr(ir_variable *x, float p0, float p1)
   {
      return mul(sign(x),
            sub(imm(M_PI_2f),
         mul(sqrt(sub(imm(1.0f), abs(x))),
      add(imm(M_PI_2f),
      mul(abs(x),
   }
      /**
   * Generate a ir_call to a function with a set of parameters
   *
   * The input \c params can either be a list of \c ir_variable or a list of
   * \c ir_dereference_variable.  In the latter case, all nodes will be removed
   * from \c params and used directly as the parameters to the generated
   * \c ir_call.
   */
   ir_call *
   builtin_builder::call(ir_function *f, ir_variable *ret, exec_list params)
   {
               foreach_in_list_safe(ir_instruction, ir, &params) {
      ir_dereference_variable *d = ir->as_dereference_variable();
   if (d != NULL) {
      d->remove();
      } else {
      ir_variable *var = ir->as_variable();
   assert(var != NULL);
                  ir_function_signature *sig =
         if (!sig)
            ir_dereference_variable *deref =
               }
      ir_function_signature *
   builtin_builder::_asin(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
                        }
      ir_function_signature *
   builtin_builder::_acos(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
                        }
      ir_function_signature *
   builtin_builder::_sinh(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
            /* 0.5 * (e^x - e^(-x)) */
               }
      ir_function_signature *
   builtin_builder::_cosh(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
            /* 0.5 * (e^x + e^(-x)) */
               }
      ir_function_signature *
   builtin_builder::_tanh(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
            /* Clamp x to [-10, +10] to avoid precision problems.
   * When x > 10, e^(-x) is so small relative to e^x that it gets flushed to
   * zero in the computation e^x + e^(-x). The same happens in the other
   * direction when x < -10.
   */
   ir_variable *t = body.make_temp(type, "tmp");
            /* (e^x - e^(-x)) / (e^x + e^(-x)) */
   body.emit(ret(div(sub(exp(t), exp(neg(t))),
               }
      ir_function_signature *
   builtin_builder::_asinh(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
            body.emit(ret(mul(sign(x), log(add(abs(x), sqrt(add(mul(x, x),
            }
      ir_function_signature *
   builtin_builder::_acosh(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
            body.emit(ret(log(add(x, sqrt(sub(mul(x, x), imm(1.0f)))))));
      }
      ir_function_signature *
   builtin_builder::_atanh(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
            body.emit(ret(mul(imm(0.5f), log(div(add(imm(1.0f), x),
            }
   /** @} */
      /**
   * Exponential Functions @{
   */
      ir_function_signature *
   builtin_builder::_pow(const glsl_type *type)
   {
         }
      UNOP(exp,         ir_unop_exp,  always_available)
   UNOP(log,         ir_unop_log,  always_available)
   UNOP(exp2,        ir_unop_exp2, always_available)
   UNOP(log2,        ir_unop_log2, always_available)
   UNOP(atan,        ir_unop_atan, always_available)
   UNOPA(sqrt,        ir_unop_sqrt)
   UNOPA(inversesqrt, ir_unop_rsq)
      /** @} */
      UNOPA(abs,       ir_unop_abs)
   UNOPA(sign,      ir_unop_sign)
   UNOPA(floor,     ir_unop_floor)
   UNOPA(truncate,  ir_unop_trunc)
   UNOPA(trunc,     ir_unop_trunc)
   UNOPA(round,     ir_unop_round_even)
   UNOPA(roundEven, ir_unop_round_even)
   UNOPA(ceil,      ir_unop_ceil)
   UNOPA(fract,     ir_unop_fract)
      ir_function_signature *
   builtin_builder::_mod(builtin_available_predicate avail,
         {
         }
      ir_function_signature *
   builtin_builder::_modf(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   ir_variable *i = out_var(type, "i");
            ir_variable *t = body.make_temp(type, "t");
   body.emit(assign(t, expr(ir_unop_trunc, x)));
   body.emit(assign(i, t));
               }
      ir_function_signature *
   builtin_builder::_min(builtin_available_predicate avail,
         {
         }
      ir_function_signature *
   builtin_builder::_max(builtin_available_predicate avail,
         {
         }
      ir_function_signature *
   builtin_builder::_clamp(builtin_available_predicate avail,
         {
      ir_variable *x = in_var(val_type, "x");
   ir_variable *minVal = in_var(bound_type, "minVal");
   ir_variable *maxVal = in_var(bound_type, "maxVal");
                        }
      ir_function_signature *
   builtin_builder::_mix_lrp(builtin_available_predicate avail, const glsl_type *val_type, const glsl_type *blend_type)
   {
      ir_variable *x = in_var(val_type, "x");
   ir_variable *y = in_var(val_type, "y");
   ir_variable *a = in_var(blend_type, "a");
                        }
      ir_function_signature *
   builtin_builder::_mix_sel(builtin_available_predicate avail,
               {
      ir_variable *x = in_var(val_type, "x");
   ir_variable *y = in_var(val_type, "y");
   ir_variable *a = in_var(blend_type, "a");
            /* csel matches the ternary operator in that a selector of true choses the
   * first argument. This differs from mix(x, y, false) which choses the
   * second argument (to remain consistent with the interpolating version of
   * mix() which takes a blend factor from 0.0 to 1.0 where 0.0 is only x.
   *
   * To handle the behavior mismatch, reverse the x and y arguments.
   */
               }
      ir_function_signature *
   builtin_builder::_step(builtin_available_predicate avail, const glsl_type *edge_type, const glsl_type *x_type)
   {
      ir_variable *edge = in_var(edge_type, "edge");
   ir_variable *x = in_var(x_type, "x");
            ir_variable *t = body.make_temp(x_type, "t");
   if (x_type->vector_elements == 1) {
      /* Both are floats */
   if (edge_type->is_double())
         else
      } else if (edge_type->vector_elements == 1) {
      /* x is a vector but edge is a float */
   for (int i = 0; i < x_type->vector_elements; i++) {
      if (edge_type->is_double())
         else
         } else {
      /* Both are vectors */
   for (int i = 0; i < x_type->vector_elements; i++) {
      if (edge_type->is_double())
      body.emit(assign(t, f2d(b2f(gequal(swizzle(x, i, 1), swizzle(edge, i, 1)))),
      else
                  }
               }
      ir_function_signature *
   builtin_builder::_smoothstep(builtin_available_predicate avail, const glsl_type *edge_type, const glsl_type *x_type)
   {
      ir_variable *edge0 = in_var(edge_type, "edge0");
   ir_variable *edge1 = in_var(edge_type, "edge1");
   ir_variable *x = in_var(x_type, "x");
            /* From the GLSL 1.10 specification:
   *
   *    genType t;
   *    t = clamp((x - edge0) / (edge1 - edge0), 0, 1);
   *    return t * t * (3 - 2 * t);
            ir_variable *t = body.make_temp(x_type, "t");
   body.emit(assign(t, clamp(div(sub(x, edge0), sub(edge1, edge0)),
                        }
      ir_function_signature *
   builtin_builder::_isnan(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
                        }
      ir_function_signature *
   builtin_builder::_isinf(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
            ir_constant_data infinities;
   for (int i = 0; i < type->vector_elements; i++) {
      switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
      infinities.f[i] = INFINITY;
      case GLSL_TYPE_DOUBLE:
      infinities.d[i] = INFINITY;
      default:
                                 }
      ir_function_signature *
   builtin_builder::_atan2(const glsl_type *x_type)
   {
         }
      ir_function_signature *
   builtin_builder::_floatBitsToInt(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   MAKE_SIG(glsl_type::ivec(type->vector_elements), shader_bit_encoding, 1, x);
   body.emit(ret(bitcast_f2i(as_highp(body, x))));
      }
      ir_function_signature *
   builtin_builder::_floatBitsToUint(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   MAKE_SIG(glsl_type::uvec(type->vector_elements), shader_bit_encoding, 1, x);
   body.emit(ret(bitcast_f2u(as_highp(body, x))));
      }
      ir_function_signature *
   builtin_builder::_intBitsToFloat(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   MAKE_SIG(glsl_type::vec(type->vector_elements), shader_bit_encoding, 1, x);
   body.emit(ret(bitcast_i2f(as_highp(body, x))));
      }
      ir_function_signature *
   builtin_builder::_uintBitsToFloat(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   MAKE_SIG(glsl_type::vec(type->vector_elements), shader_bit_encoding, 1, x);
   body.emit(ret(bitcast_u2f(as_highp(body, x))));
      }
      ir_function_signature *
   builtin_builder::_doubleBitsToInt64(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   MAKE_SIG(glsl_type::i64vec(type->vector_elements), avail, 1, x);
   body.emit(ret(bitcast_d2i64(x)));
      }
      ir_function_signature *
   builtin_builder::_doubleBitsToUint64(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   MAKE_SIG(glsl_type::u64vec(type->vector_elements), avail, 1, x);
   body.emit(ret(bitcast_d2u64(x)));
      }
      ir_function_signature *
   builtin_builder::_int64BitsToDouble(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   MAKE_SIG(glsl_type::dvec(type->vector_elements), avail, 1, x);
   body.emit(ret(bitcast_i642d(x)));
      }
      ir_function_signature *
   builtin_builder::_uint64BitsToDouble(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   MAKE_SIG(glsl_type::dvec(type->vector_elements), avail, 1, x);
   body.emit(ret(bitcast_u642d(x)));
      }
      ir_function_signature *
   builtin_builder::_packUnorm2x16(builtin_available_predicate avail)
   {
      ir_variable *v = in_highp_var(glsl_type::vec2_type, "v");
   MAKE_SIG(glsl_type::uint_type, avail, 1, v);
   sig->return_precision = GLSL_PRECISION_HIGH;
   body.emit(ret(expr(ir_unop_pack_unorm_2x16, v)));
      }
      ir_function_signature *
   builtin_builder::_packSnorm2x16(builtin_available_predicate avail)
   {
      ir_variable *v = in_var(glsl_type::vec2_type, "v");
   MAKE_SIG(glsl_type::uint_type, avail, 1, v);
   sig->return_precision = GLSL_PRECISION_HIGH;
   body.emit(ret(expr(ir_unop_pack_snorm_2x16, v)));
      }
      ir_function_signature *
   builtin_builder::_packUnorm4x8(builtin_available_predicate avail)
   {
      ir_variable *v = in_mediump_var(glsl_type::vec4_type, "v");
   MAKE_SIG(glsl_type::uint_type, avail, 1, v);
   sig->return_precision = GLSL_PRECISION_HIGH;
   body.emit(ret(expr(ir_unop_pack_unorm_4x8, v)));
      }
      ir_function_signature *
   builtin_builder::_packSnorm4x8(builtin_available_predicate avail)
   {
      ir_variable *v = in_mediump_var(glsl_type::vec4_type, "v");
   MAKE_SIG(glsl_type::uint_type, avail, 1, v);
   sig->return_precision = GLSL_PRECISION_HIGH;
   body.emit(ret(expr(ir_unop_pack_snorm_4x8, v)));
      }
      ir_function_signature *
   builtin_builder::_unpackUnorm2x16(builtin_available_predicate avail)
   {
      ir_variable *p = in_highp_var(glsl_type::uint_type, "p");
   MAKE_SIG(glsl_type::vec2_type, avail, 1, p);
   sig->return_precision = GLSL_PRECISION_HIGH;
   body.emit(ret(expr(ir_unop_unpack_unorm_2x16, p)));
      }
      ir_function_signature *
   builtin_builder::_unpackSnorm2x16(builtin_available_predicate avail)
   {
      ir_variable *p = in_highp_var(glsl_type::uint_type, "p");
   MAKE_SIG(glsl_type::vec2_type, avail, 1, p);
   sig->return_precision = GLSL_PRECISION_HIGH;
   body.emit(ret(expr(ir_unop_unpack_snorm_2x16, p)));
      }
         ir_function_signature *
   builtin_builder::_unpackUnorm4x8(builtin_available_predicate avail)
   {
      ir_variable *p = in_highp_var(glsl_type::uint_type, "p");
   MAKE_SIG(glsl_type::vec4_type, avail, 1, p);
   sig->return_precision = GLSL_PRECISION_MEDIUM;
   body.emit(ret(expr(ir_unop_unpack_unorm_4x8, p)));
      }
      ir_function_signature *
   builtin_builder::_unpackSnorm4x8(builtin_available_predicate avail)
   {
      ir_variable *p = in_highp_var(glsl_type::uint_type, "p");
   MAKE_SIG(glsl_type::vec4_type, avail, 1, p);
   sig->return_precision = GLSL_PRECISION_MEDIUM;
   body.emit(ret(expr(ir_unop_unpack_snorm_4x8, p)));
      }
      ir_function_signature *
   builtin_builder::_packHalf2x16(builtin_available_predicate avail)
   {
      ir_variable *v = in_mediump_var(glsl_type::vec2_type, "v");
   MAKE_SIG(glsl_type::uint_type, avail, 1, v);
   sig->return_precision = GLSL_PRECISION_HIGH;
   body.emit(ret(expr(ir_unop_pack_half_2x16, v)));
      }
      ir_function_signature *
   builtin_builder::_unpackHalf2x16(builtin_available_predicate avail)
   {
      ir_variable *p = in_highp_var(glsl_type::uint_type, "p");
   MAKE_SIG(glsl_type::vec2_type, avail, 1, p);
   sig->return_precision = GLSL_PRECISION_MEDIUM;
   body.emit(ret(expr(ir_unop_unpack_half_2x16, p)));
      }
      ir_function_signature *
   builtin_builder::_packDouble2x32(builtin_available_predicate avail)
   {
      ir_variable *v = in_var(glsl_type::uvec2_type, "v");
   MAKE_SIG(glsl_type::double_type, avail, 1, v);
   body.emit(ret(expr(ir_unop_pack_double_2x32, v)));
      }
      ir_function_signature *
   builtin_builder::_unpackDouble2x32(builtin_available_predicate avail)
   {
      ir_variable *p = in_var(glsl_type::double_type, "p");
   MAKE_SIG(glsl_type::uvec2_type, avail, 1, p);
   body.emit(ret(expr(ir_unop_unpack_double_2x32, p)));
      }
      ir_function_signature *
   builtin_builder::_packInt2x32(builtin_available_predicate avail)
   {
      ir_variable *v = in_var(glsl_type::ivec2_type, "v");
   MAKE_SIG(glsl_type::int64_t_type, avail, 1, v);
   body.emit(ret(expr(ir_unop_pack_int_2x32, v)));
      }
      ir_function_signature *
   builtin_builder::_unpackInt2x32(builtin_available_predicate avail)
   {
      ir_variable *p = in_var(glsl_type::int64_t_type, "p");
   MAKE_SIG(glsl_type::ivec2_type, avail, 1, p);
   body.emit(ret(expr(ir_unop_unpack_int_2x32, p)));
      }
      ir_function_signature *
   builtin_builder::_packUint2x32(builtin_available_predicate avail)
   {
      ir_variable *v = in_var(glsl_type::uvec2_type, "v");
   MAKE_SIG(glsl_type::uint64_t_type, avail, 1, v);
   body.emit(ret(expr(ir_unop_pack_uint_2x32, v)));
      }
      ir_function_signature *
   builtin_builder::_unpackUint2x32(builtin_available_predicate avail)
   {
      ir_variable *p = in_var(glsl_type::uint64_t_type, "p");
   MAKE_SIG(glsl_type::uvec2_type, avail, 1, p);
   body.emit(ret(expr(ir_unop_unpack_uint_2x32, p)));
      }
      ir_function_signature *
   builtin_builder::_length(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
                        }
      ir_function_signature *
   builtin_builder::_distance(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *p0 = in_var(type, "p0");
   ir_variable *p1 = in_var(type, "p1");
            if (type->vector_elements == 1) {
         } else {
      ir_variable *p = body.make_temp(type, "p");
   body.emit(assign(p, sub(p0, p1)));
                  }
      ir_function_signature *
   builtin_builder::_dot(builtin_available_predicate avail, const glsl_type *type)
   {
      if (type->vector_elements == 1)
            return binop(avail, ir_binop_dot,
      }
      ir_function_signature *
   builtin_builder::_cross(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *a = in_var(type, "a");
   ir_variable *b = in_var(type, "b");
            int yzx = MAKE_SWIZZLE4(SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_X, 0);
            body.emit(ret(sub(mul(swizzle(a, yzx, 3), swizzle(b, zxy, 3)),
               }
      ir_function_signature *
   builtin_builder::_normalize(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
            if (type->vector_elements == 1) {
         } else {
                     }
      ir_function_signature *
   builtin_builder::_ftransform()
   {
               /* ftransform() refers to global variables, and is always emitted
   * directly by ast_function.cpp.  Just emit a prototype here so we
   * can recognize calls to it.
   */
      }
      ir_function_signature *
   builtin_builder::_faceforward(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *N = in_var(type, "N");
   ir_variable *I = in_var(type, "I");
   ir_variable *Nref = in_var(type, "Nref");
            body.emit(if_tree(less(dot(Nref, I), IMM_FP(type, 0.0)),
               }
      ir_function_signature *
   builtin_builder::_reflect(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *I = in_var(type, "I");
   ir_variable *N = in_var(type, "N");
            /* I - 2 * dot(N, I) * N */
               }
      ir_function_signature *
   builtin_builder::_refract(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *I = in_var(type, "I");
   ir_variable *N = in_var(type, "N");
   ir_variable *eta = in_var(type->get_base_type(), "eta");
            ir_variable *n_dot_i = body.make_temp(type->get_base_type(), "n_dot_i");
            /* From the GLSL 1.10 specification:
   * k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I))
   * if (k < 0.0)
   *    return genType(0.0)
   * else
   *    return eta * I - (eta * dot(N, I) + sqrt(k)) * N
   */
   ir_variable *k = body.make_temp(type->get_base_type(), "k");
   body.emit(assign(k, sub(IMM_FP(type, 1.0),
               body.emit(if_tree(less(k, IMM_FP(type, 0.0)),
                           }
      ir_function_signature *
   builtin_builder::_matrixCompMult(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   ir_variable *y = in_var(type, "y");
            ir_variable *z = body.make_temp(type, "z");
   for (int i = 0; i < type->matrix_columns; i++) {
         }
               }
      ir_function_signature *
   builtin_builder::_outerProduct(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *c;
            if (type->is_double()) {
      r = in_var(glsl_type::dvec(type->matrix_columns), "r");
      } else {
      r = in_var(glsl_type::vec(type->matrix_columns), "r");
      }
            ir_variable *m = body.make_temp(type, "m");
   for (int i = 0; i < type->matrix_columns; i++) {
         }
               }
      ir_function_signature *
   builtin_builder::_transpose(builtin_available_predicate avail, const glsl_type *orig_type)
   {
      const glsl_type *transpose_type =
      glsl_type::get_instance(orig_type->base_type,
               ir_variable *m = in_var(orig_type, "m");
            ir_variable *t = body.make_temp(transpose_type, "t");
   for (int i = 0; i < orig_type->matrix_columns; i++) {
      for (int j = 0; j < orig_type->vector_elements; j++) {
      body.emit(assign(array_ref(t, j),
               }
               }
      ir_function_signature *
   builtin_builder::_determinant_mat2(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *m = in_var(type, "m");
            body.emit(ret(sub(mul(matrix_elt(m, 0, 0), matrix_elt(m, 1, 1)),
               }
      ir_function_signature *
   builtin_builder::_determinant_mat3(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *m = in_var(type, "m");
            ir_expression *f1 =
      sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 2, 2)),
         ir_expression *f2 =
      sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 2, 2)),
         ir_expression *f3 =
      sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 2, 1)),
         body.emit(ret(add(sub(mul(matrix_elt(m, 0, 0), f1),
                     }
      ir_function_signature *
   builtin_builder::_determinant_mat4(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *m = in_var(type, "m");
   const glsl_type *btype = type->get_base_type();
            ir_variable *SubFactor00 = body.make_temp(btype, "SubFactor00");
   ir_variable *SubFactor01 = body.make_temp(btype, "SubFactor01");
   ir_variable *SubFactor02 = body.make_temp(btype, "SubFactor02");
   ir_variable *SubFactor03 = body.make_temp(btype, "SubFactor03");
   ir_variable *SubFactor04 = body.make_temp(btype, "SubFactor04");
   ir_variable *SubFactor05 = body.make_temp(btype, "SubFactor05");
   ir_variable *SubFactor06 = body.make_temp(btype, "SubFactor06");
   ir_variable *SubFactor07 = body.make_temp(btype, "SubFactor07");
   ir_variable *SubFactor08 = body.make_temp(btype, "SubFactor08");
   ir_variable *SubFactor09 = body.make_temp(btype, "SubFactor09");
   ir_variable *SubFactor10 = body.make_temp(btype, "SubFactor10");
   ir_variable *SubFactor11 = body.make_temp(btype, "SubFactor11");
   ir_variable *SubFactor12 = body.make_temp(btype, "SubFactor12");
   ir_variable *SubFactor13 = body.make_temp(btype, "SubFactor13");
   ir_variable *SubFactor14 = body.make_temp(btype, "SubFactor14");
   ir_variable *SubFactor15 = body.make_temp(btype, "SubFactor15");
   ir_variable *SubFactor16 = body.make_temp(btype, "SubFactor16");
   ir_variable *SubFactor17 = body.make_temp(btype, "SubFactor17");
            body.emit(assign(SubFactor00, sub(mul(matrix_elt(m, 2, 2), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 2), matrix_elt(m, 2, 3)))));
   body.emit(assign(SubFactor01, sub(mul(matrix_elt(m, 2, 1), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 1), matrix_elt(m, 2, 3)))));
   body.emit(assign(SubFactor02, sub(mul(matrix_elt(m, 2, 1), matrix_elt(m, 3, 2)), mul(matrix_elt(m, 3, 1), matrix_elt(m, 2, 2)))));
   body.emit(assign(SubFactor03, sub(mul(matrix_elt(m, 2, 0), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 2, 3)))));
   body.emit(assign(SubFactor04, sub(mul(matrix_elt(m, 2, 0), matrix_elt(m, 3, 2)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 2, 2)))));
   body.emit(assign(SubFactor05, sub(mul(matrix_elt(m, 2, 0), matrix_elt(m, 3, 1)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 2, 1)))));
   body.emit(assign(SubFactor06, sub(mul(matrix_elt(m, 1, 2), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 2), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor07, sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 1), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor08, sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 3, 2)), mul(matrix_elt(m, 3, 1), matrix_elt(m, 1, 2)))));
   body.emit(assign(SubFactor09, sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor10, sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 3, 2)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 1, 2)))));
   body.emit(assign(SubFactor11, sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 1), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor12, sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 3, 1)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 1, 1)))));
   body.emit(assign(SubFactor13, sub(mul(matrix_elt(m, 1, 2), matrix_elt(m, 2, 3)), mul(matrix_elt(m, 2, 2), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor14, sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 2, 3)), mul(matrix_elt(m, 2, 1), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor15, sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 2, 2)), mul(matrix_elt(m, 2, 1), matrix_elt(m, 1, 2)))));
   body.emit(assign(SubFactor16, sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 2, 3)), mul(matrix_elt(m, 2, 0), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor17, sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 2, 2)), mul(matrix_elt(m, 2, 0), matrix_elt(m, 1, 2)))));
                     body.emit(assign(adj_0,
                  add(sub(mul(matrix_elt(m, 1, 1), SubFactor00),
      body.emit(assign(adj_0, neg(
                  add(sub(mul(matrix_elt(m, 1, 0), SubFactor00),
      body.emit(assign(adj_0,
                  add(sub(mul(matrix_elt(m, 1, 0), SubFactor01),
      body.emit(assign(adj_0, neg(
                  add(sub(mul(matrix_elt(m, 1, 0), SubFactor02),
                     }
      ir_function_signature *
   builtin_builder::_inverse_mat2(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *m = in_var(type, "m");
            ir_variable *adj = body.make_temp(type, "adj");
   body.emit(assign(array_ref(adj, 0), matrix_elt(m, 1, 1), 1 << 0));
   body.emit(assign(array_ref(adj, 0), neg(matrix_elt(m, 0, 1)), 1 << 1));
   body.emit(assign(array_ref(adj, 1), neg(matrix_elt(m, 1, 0)), 1 << 0));
            ir_expression *det =
      sub(mul(matrix_elt(m, 0, 0), matrix_elt(m, 1, 1)),
         body.emit(ret(div(adj, det)));
      }
      ir_function_signature *
   builtin_builder::_inverse_mat3(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *m = in_var(type, "m");
   const glsl_type *btype = type->get_base_type();
            ir_variable *f11_22_21_12 = body.make_temp(btype, "f11_22_21_12");
   ir_variable *f10_22_20_12 = body.make_temp(btype, "f10_22_20_12");
            body.emit(assign(f11_22_21_12,
               body.emit(assign(f10_22_20_12,
               body.emit(assign(f10_21_20_11,
                  ir_variable *adj = body.make_temp(type, "adj");
   body.emit(assign(array_ref(adj, 0), f11_22_21_12, WRITEMASK_X));
   body.emit(assign(array_ref(adj, 1), neg(f10_22_20_12), WRITEMASK_X));
            body.emit(assign(array_ref(adj, 0), neg(
                     body.emit(assign(array_ref(adj, 1),
                     body.emit(assign(array_ref(adj, 2), neg(
                        body.emit(assign(array_ref(adj, 0),
                     body.emit(assign(array_ref(adj, 1), neg(
                     body.emit(assign(array_ref(adj, 2),
                        ir_expression *det =
      add(sub(mul(matrix_elt(m, 0, 0), f11_22_21_12),
                           }
      ir_function_signature *
   builtin_builder::_inverse_mat4(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *m = in_var(type, "m");
   const glsl_type *btype = type->get_base_type();
            ir_variable *SubFactor00 = body.make_temp(btype, "SubFactor00");
   ir_variable *SubFactor01 = body.make_temp(btype, "SubFactor01");
   ir_variable *SubFactor02 = body.make_temp(btype, "SubFactor02");
   ir_variable *SubFactor03 = body.make_temp(btype, "SubFactor03");
   ir_variable *SubFactor04 = body.make_temp(btype, "SubFactor04");
   ir_variable *SubFactor05 = body.make_temp(btype, "SubFactor05");
   ir_variable *SubFactor06 = body.make_temp(btype, "SubFactor06");
   ir_variable *SubFactor07 = body.make_temp(btype, "SubFactor07");
   ir_variable *SubFactor08 = body.make_temp(btype, "SubFactor08");
   ir_variable *SubFactor09 = body.make_temp(btype, "SubFactor09");
   ir_variable *SubFactor10 = body.make_temp(btype, "SubFactor10");
   ir_variable *SubFactor11 = body.make_temp(btype, "SubFactor11");
   ir_variable *SubFactor12 = body.make_temp(btype, "SubFactor12");
   ir_variable *SubFactor13 = body.make_temp(btype, "SubFactor13");
   ir_variable *SubFactor14 = body.make_temp(btype, "SubFactor14");
   ir_variable *SubFactor15 = body.make_temp(btype, "SubFactor15");
   ir_variable *SubFactor16 = body.make_temp(btype, "SubFactor16");
   ir_variable *SubFactor17 = body.make_temp(btype, "SubFactor17");
            body.emit(assign(SubFactor00, sub(mul(matrix_elt(m, 2, 2), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 2), matrix_elt(m, 2, 3)))));
   body.emit(assign(SubFactor01, sub(mul(matrix_elt(m, 2, 1), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 1), matrix_elt(m, 2, 3)))));
   body.emit(assign(SubFactor02, sub(mul(matrix_elt(m, 2, 1), matrix_elt(m, 3, 2)), mul(matrix_elt(m, 3, 1), matrix_elt(m, 2, 2)))));
   body.emit(assign(SubFactor03, sub(mul(matrix_elt(m, 2, 0), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 2, 3)))));
   body.emit(assign(SubFactor04, sub(mul(matrix_elt(m, 2, 0), matrix_elt(m, 3, 2)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 2, 2)))));
   body.emit(assign(SubFactor05, sub(mul(matrix_elt(m, 2, 0), matrix_elt(m, 3, 1)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 2, 1)))));
   body.emit(assign(SubFactor06, sub(mul(matrix_elt(m, 1, 2), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 2), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor07, sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 1), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor08, sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 3, 2)), mul(matrix_elt(m, 3, 1), matrix_elt(m, 1, 2)))));
   body.emit(assign(SubFactor09, sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor10, sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 3, 2)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 1, 2)))));
   body.emit(assign(SubFactor11, sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 3, 3)), mul(matrix_elt(m, 3, 1), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor12, sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 3, 1)), mul(matrix_elt(m, 3, 0), matrix_elt(m, 1, 1)))));
   body.emit(assign(SubFactor13, sub(mul(matrix_elt(m, 1, 2), matrix_elt(m, 2, 3)), mul(matrix_elt(m, 2, 2), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor14, sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 2, 3)), mul(matrix_elt(m, 2, 1), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor15, sub(mul(matrix_elt(m, 1, 1), matrix_elt(m, 2, 2)), mul(matrix_elt(m, 2, 1), matrix_elt(m, 1, 2)))));
   body.emit(assign(SubFactor16, sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 2, 3)), mul(matrix_elt(m, 2, 0), matrix_elt(m, 1, 3)))));
   body.emit(assign(SubFactor17, sub(mul(matrix_elt(m, 1, 0), matrix_elt(m, 2, 2)), mul(matrix_elt(m, 2, 0), matrix_elt(m, 1, 2)))));
            ir_variable *adj = body.make_temp(btype == glsl_type::float_type ? glsl_type::mat4_type : glsl_type::dmat4_type, "adj");
   body.emit(assign(array_ref(adj, 0),
                  add(sub(mul(matrix_elt(m, 1, 1), SubFactor00),
      body.emit(assign(array_ref(adj, 1), neg(
                  add(sub(mul(matrix_elt(m, 1, 0), SubFactor00),
      body.emit(assign(array_ref(adj, 2),
                  add(sub(mul(matrix_elt(m, 1, 0), SubFactor01),
      body.emit(assign(array_ref(adj, 3), neg(
                  add(sub(mul(matrix_elt(m, 1, 0), SubFactor02),
         body.emit(assign(array_ref(adj, 0), neg(
                  add(sub(mul(matrix_elt(m, 0, 1), SubFactor00),
      body.emit(assign(array_ref(adj, 1),
                  add(sub(mul(matrix_elt(m, 0, 0), SubFactor00),
      body.emit(assign(array_ref(adj, 2), neg(
                  add(sub(mul(matrix_elt(m, 0, 0), SubFactor01),
      body.emit(assign(array_ref(adj, 3),
                  add(sub(mul(matrix_elt(m, 0, 0), SubFactor02),
         body.emit(assign(array_ref(adj, 0),
                  add(sub(mul(matrix_elt(m, 0, 1), SubFactor06),
      body.emit(assign(array_ref(adj, 1), neg(
                  add(sub(mul(matrix_elt(m, 0, 0), SubFactor06),
      body.emit(assign(array_ref(adj, 2),
                  add(sub(mul(matrix_elt(m, 0, 0), SubFactor11),
      body.emit(assign(array_ref(adj, 3), neg(
                  add(sub(mul(matrix_elt(m, 0, 0), SubFactor08),
         body.emit(assign(array_ref(adj, 0), neg(
                  add(sub(mul(matrix_elt(m, 0, 1), SubFactor13),
      body.emit(assign(array_ref(adj, 1),
                  add(sub(mul(matrix_elt(m, 0, 0), SubFactor13),
      body.emit(assign(array_ref(adj, 2), neg(
                  add(sub(mul(matrix_elt(m, 0, 0), SubFactor14),
      body.emit(assign(array_ref(adj, 3),
                  add(sub(mul(matrix_elt(m, 0, 0), SubFactor15),
         ir_expression *det =
      add(mul(matrix_elt(m, 0, 0), matrix_elt(adj, 0, 0)),
      add(mul(matrix_elt(m, 0, 1), matrix_elt(adj, 1, 0)),
                        }
         ir_function_signature *
   builtin_builder::_lessThan(builtin_available_predicate avail,
         {
      return binop(avail, ir_binop_less,
      }
      ir_function_signature *
   builtin_builder::_lessThanEqual(builtin_available_predicate avail,
         {
      return binop(avail, ir_binop_gequal,
            }
      ir_function_signature *
   builtin_builder::_greaterThan(builtin_available_predicate avail,
         {
      return binop(avail, ir_binop_less,
            }
      ir_function_signature *
   builtin_builder::_greaterThanEqual(builtin_available_predicate avail,
         {
      return binop(avail, ir_binop_gequal,
      }
      ir_function_signature *
   builtin_builder::_equal(builtin_available_predicate avail,
         {
      return binop(avail, ir_binop_equal,
      }
      ir_function_signature *
   builtin_builder::_notEqual(builtin_available_predicate avail,
         {
      return binop(avail, ir_binop_nequal,
      }
      ir_function_signature *
   builtin_builder::_any(const glsl_type *type)
   {
      ir_variable *v = in_var(type, "v");
            const unsigned vec_elem = v->type->vector_elements;
               }
      ir_function_signature *
   builtin_builder::_all(const glsl_type *type)
   {
      ir_variable *v = in_var(type, "v");
            const unsigned vec_elem = v->type->vector_elements;
               }
      UNOP(not, ir_unop_logic_not, always_available)
      static bool
   has_lod(const glsl_type *sampler_type)
   {
               switch (sampler_type->sampler_dimensionality) {
   case GLSL_SAMPLER_DIM_RECT:
   case GLSL_SAMPLER_DIM_BUF:
   case GLSL_SAMPLER_DIM_MS:
         default:
            }
      ir_function_signature *
   builtin_builder::_textureSize(builtin_available_predicate avail,
               {
      ir_variable *s = in_var(sampler_type, "sampler");
   /* The sampler always exists; add optional lod later. */
   MAKE_SIG(return_type, avail, 1, s);
            ir_texture *tex = new(mem_ctx) ir_texture(ir_txs);
            if (has_lod(sampler_type)) {
      ir_variable *lod = in_var(glsl_type::int_type, "lod");
   sig->parameters.push_tail(lod);
      } else {
                              }
      ir_function_signature *
   builtin_builder::_textureSamples(builtin_available_predicate avail,
         {
      ir_variable *s = in_var(sampler_type, "sampler");
            ir_texture *tex = new(mem_ctx) ir_texture(ir_texture_samples);
   tex->set_sampler(new(mem_ctx) ir_dereference_variable(s), glsl_type::int_type);
               }
      ir_function_signature *
   builtin_builder::_is_sparse_texels_resident(void)
   {
      ir_variable *code = in_var(glsl_type::int_type, "code");
            ir_variable *retval = body.make_temp(glsl_type::bool_type, "retval");
   ir_function *f =
            body.emit(call(f, retval, sig->parameters));
   body.emit(ret(retval));
      }
      ir_function_signature *
   builtin_builder::_is_sparse_texels_resident_intrinsic(void)
   {
      ir_variable *code = in_var(glsl_type::int_type, "code");
   MAKE_INTRINSIC(glsl_type::bool_type, ir_intrinsic_is_sparse_texels_resident,
            }
      ir_function_signature *
   builtin_builder::_texture(ir_texture_opcode opcode,
                           builtin_available_predicate avail,
   {
      ir_variable *s = in_var(sampler_type, "sampler");
   ir_variable *P = in_var(coord_type, "P");
   /* Sparse texture return residency info. */
   const glsl_type *type = flags & TEX_SPARSE ? glsl_type::int_type : return_type;
   /* The sampler and coordinate always exist; add optional parameters later. */
            ir_texture *tex = new(mem_ctx) ir_texture(opcode, flags & TEX_SPARSE);
                     if (coord_size == coord_type->vector_elements) {
         } else {
      /* The incoming coordinate also has the projector or shadow comparator,
   * so we need to swizzle those away.
   */
               /* The projector is always in the last component. */
   if (flags & TEX_PROJECT)
            if (sampler_type->sampler_shadow) {
      if (opcode == ir_tg4) {
      /* gather has refz as a separate parameter, immediately after the
   * coordinate
   */
   ir_variable *refz = in_var(glsl_type::float_type, "refz");
   sig->parameters.push_tail(refz);
      } else {
      /* The shadow comparator is normally in the Z component, but a few types
   * have sufficiently large coordinates that it's in W.
   */
                  if (opcode == ir_txl) {
      ir_variable *lod = in_var(glsl_type::float_type, "lod");
   sig->parameters.push_tail(lod);
      } else if (opcode == ir_txd) {
      int grad_size = coord_size - (sampler_type->sampler_array ? 1 : 0);
   ir_variable *dPdx = in_var(glsl_type::vec(grad_size), "dPdx");
   ir_variable *dPdy = in_var(glsl_type::vec(grad_size), "dPdy");
   sig->parameters.push_tail(dPdx);
   sig->parameters.push_tail(dPdy);
   tex->lod_info.grad.dPdx = var_ref(dPdx);
               if (flags & (TEX_OFFSET | TEX_OFFSET_NONCONST)) {
      int offset_size = coord_size - (sampler_type->sampler_array ? 1 : 0);
   ir_variable *offset =
      new(mem_ctx) ir_variable(glsl_type::ivec(offset_size), "offset",
      sig->parameters.push_tail(offset);
               if (flags & TEX_OFFSET_ARRAY) {
      ir_variable *offsets =
      new(mem_ctx) ir_variable(glsl_type::get_array_instance(glsl_type::ivec2_type, 4),
      sig->parameters.push_tail(offsets);
               if (flags & TEX_CLAMP) {
      ir_variable *clamp = in_var(glsl_type::float_type, "lodClamp");
   sig->parameters.push_tail(clamp);
               ir_variable *texel = NULL;
   if (flags & TEX_SPARSE) {
      texel = out_var(return_type, "texel");
               if (opcode == ir_tg4) {
      if (flags & TEX_COMPONENT) {
      ir_variable *component =
         sig->parameters.push_tail(component);
      }
   else {
                     /* The "bias" parameter comes /after/ the "offset" parameter, which is
   * inconsistent with both textureLodOffset and textureGradOffset.
   */
   if (opcode == ir_txb) {
      ir_variable *bias = in_var(glsl_type::float_type, "bias");
   sig->parameters.push_tail(bias);
               if (flags & TEX_SPARSE) {
      ir_variable *r = body.make_temp(tex->type, "result");
   body.emit(assign(r, tex));
   body.emit(assign(texel, record_ref(r, "texel")));
      } else
               }
      ir_function_signature *
   builtin_builder::_textureCubeArrayShadow(ir_texture_opcode opcode,
                     {
      ir_variable *s = in_var(sampler_type, "sampler");
   ir_variable *P = in_var(glsl_type::vec4_type, "P");
   ir_variable *compare = in_var(glsl_type::float_type, "compare");
   const glsl_type *return_type = glsl_type::float_type;
   bool sparse = flags & TEX_SPARSE;
   bool clamp = flags & TEX_CLAMP;
   /* Sparse texture return residency info. */
   const glsl_type *type = sparse ? glsl_type::int_type : return_type;
            ir_texture *tex = new(mem_ctx) ir_texture(opcode, sparse);
            tex->coordinate = var_ref(P);
            if (opcode == ir_txl) {
      ir_variable *lod = in_var(glsl_type::float_type, "lod");
   sig->parameters.push_tail(lod);
               if (clamp) {
      ir_variable *lod_clamp = in_var(glsl_type::float_type, "lodClamp");
   sig->parameters.push_tail(lod_clamp);
               ir_variable *texel = NULL;
   if (sparse) {
      texel = out_var(return_type, "texel");
               if (opcode == ir_txb) {
      ir_variable *bias = in_var(glsl_type::float_type, "bias");
   sig->parameters.push_tail(bias);
               if (sparse) {
      ir_variable *r = body.make_temp(tex->type, "result");
   body.emit(assign(r, tex));
   body.emit(assign(texel, record_ref(r, "texel")));
      } else
               }
      ir_function_signature *
   builtin_builder::_texelFetch(builtin_available_predicate avail,
                                 {
      ir_variable *s = in_var(sampler_type, "sampler");
   ir_variable *P = in_var(coord_type, "P");
   /* Sparse texture return residency info. */
   const glsl_type *type = sparse ? glsl_type::int_type : return_type;
   /* The sampler and coordinate always exist; add optional parameters later. */
            ir_texture *tex = new(mem_ctx) ir_texture(ir_txf, sparse);
   tex->coordinate = var_ref(P);
            if (sampler_type->sampler_dimensionality == GLSL_SAMPLER_DIM_MS) {
      ir_variable *sample = in_var(glsl_type::int_type, "sample");
   sig->parameters.push_tail(sample);
   tex->lod_info.sample_index = var_ref(sample);
      } else if (has_lod(sampler_type)) {
      ir_variable *lod = in_var(glsl_type::int_type, "lod");
   sig->parameters.push_tail(lod);
      } else {
                  if (offset_type != NULL) {
      ir_variable *offset =
         sig->parameters.push_tail(offset);
               if (sparse) {
      ir_variable *texel = out_var(return_type, "texel");
            ir_variable *r = body.make_temp(tex->type, "result");
   body.emit(assign(r, tex));
   body.emit(assign(texel, record_ref(r, "texel")));
      } else
               }
      ir_function_signature *
   builtin_builder::_EmitVertex()
   {
               ir_rvalue *stream = new(mem_ctx) ir_constant(0, 1);
               }
      ir_function_signature *
   builtin_builder::_EmitStreamVertex(builtin_available_predicate avail,
         {
      /* Section 8.12 (Geometry Shader Functions) of the GLSL 4.0 spec says:
   *
   *     "Emit the current values of output variables to the current output
   *     primitive on stream stream. The argument to stream must be a constant
   *     integral expression."
   */
   ir_variable *stream =
                                 }
      ir_function_signature *
   builtin_builder::_EndPrimitive()
   {
               ir_rvalue *stream = new(mem_ctx) ir_constant(0, 1);
               }
      ir_function_signature *
   builtin_builder::_EndStreamPrimitive(builtin_available_predicate avail,
         {
      /* Section 8.12 (Geometry Shader Functions) of the GLSL 4.0 spec says:
   *
   *     "Completes the current output primitive on stream stream and starts
   *     a new one. The argument to stream must be a constant integral
   *     expression."
   */
   ir_variable *stream =
                                 }
      ir_function_signature *
   builtin_builder::_barrier()
   {
               body.emit(new(mem_ctx) ir_barrier());
      }
      ir_function_signature *
   builtin_builder::_textureQueryLod(builtin_available_predicate avail,
               {
      ir_variable *s = in_var(sampler_type, "sampler");
   ir_variable *coord = in_var(coord_type, "coord");
   /* The sampler and coordinate always exist; add optional parameters later. */
            ir_texture *tex = new(mem_ctx) ir_texture(ir_lod);
   tex->coordinate = var_ref(coord);
                        }
      ir_function_signature *
   builtin_builder::_textureQueryLevels(builtin_available_predicate avail,
         {
      ir_variable *s = in_var(sampler_type, "sampler");
   const glsl_type *return_type = glsl_type::int_type;
            ir_texture *tex = new(mem_ctx) ir_texture(ir_query_levels);
                        }
      ir_function_signature *
   builtin_builder::_textureSamplesIdentical(builtin_available_predicate avail,
               {
      ir_variable *s = in_var(sampler_type, "sampler");
   ir_variable *P = in_var(coord_type, "P");
   const glsl_type *return_type = glsl_type::bool_type;
            ir_texture *tex = new(mem_ctx) ir_texture(ir_samples_identical);
   tex->coordinate = var_ref(P);
                        }
      UNOP(dFdx, ir_unop_dFdx, derivatives)
   UNOP(dFdxCoarse, ir_unop_dFdx_coarse, derivative_control)
   UNOP(dFdxFine, ir_unop_dFdx_fine, derivative_control)
   UNOP(dFdy, ir_unop_dFdy, derivatives)
   UNOP(dFdyCoarse, ir_unop_dFdy_coarse, derivative_control)
   UNOP(dFdyFine, ir_unop_dFdy_fine, derivative_control)
      ir_function_signature *
   builtin_builder::_fwidth(const glsl_type *type)
   {
      ir_variable *p = in_var(type, "p");
                        }
      ir_function_signature *
   builtin_builder::_fwidthCoarse(const glsl_type *type)
   {
      ir_variable *p = in_var(type, "p");
            body.emit(ret(add(abs(expr(ir_unop_dFdx_coarse, p)),
               }
      ir_function_signature *
   builtin_builder::_fwidthFine(const glsl_type *type)
   {
      ir_variable *p = in_var(type, "p");
            body.emit(ret(add(abs(expr(ir_unop_dFdx_fine, p)),
               }
      ir_function_signature *
   builtin_builder::_noise1(const glsl_type *type)
   {
      /* From the GLSL 4.60 specification:
   *
   *    "The noise functions noise1, noise2, noise3, and noise4 have been
   *    deprecated starting with version 4.4 of GLSL. When not generating
   *    SPIR-V they are defined to return the value 0.0 or a vector whose
   *    components are all 0.0. When generating SPIR-V the noise functions
   *    are not declared and may not be used."
   *
   * In earlier versions of the GLSL specification attempt to define some
   * sort of statistical noise function.  However, the function's
   * characteristics have always been such that always returning 0 is
   * valid and Mesa has always returned 0 for noise on most drivers.
   */
   ir_variable *p = in_var(type, "p");
   MAKE_SIG(glsl_type::float_type, v110, 1, p);
   body.emit(ret(imm(glsl_type::float_type, ir_constant_data())));
      }
      ir_function_signature *
   builtin_builder::_noise2(const glsl_type *type)
   {
      /* See builtin_builder::_noise1 */
   ir_variable *p = in_var(type, "p");
   MAKE_SIG(glsl_type::vec2_type, v110, 1, p);
   body.emit(ret(imm(glsl_type::vec2_type, ir_constant_data())));
      }
      ir_function_signature *
   builtin_builder::_noise3(const glsl_type *type)
   {
      /* See builtin_builder::_noise1 */
   ir_variable *p = in_var(type, "p");
   MAKE_SIG(glsl_type::vec3_type, v110, 1, p);
   body.emit(ret(imm(glsl_type::vec3_type, ir_constant_data())));
      }
      ir_function_signature *
   builtin_builder::_noise4(const glsl_type *type)
   {
      /* See builtin_builder::_noise1 */
   ir_variable *p = in_var(type, "p");
   MAKE_SIG(glsl_type::vec4_type, v110, 1, p);
   body.emit(ret(imm(glsl_type::vec4_type, ir_constant_data())));
      }
      ir_function_signature *
   builtin_builder::_bitfieldExtract(const glsl_type *type)
   {
      bool is_uint = type->base_type == GLSL_TYPE_UINT;
   ir_variable *value  = in_var(type, "value");
   ir_variable *offset = in_var(glsl_type::int_type, "offset");
   ir_variable *bits   = in_var(glsl_type::int_type, "bits");
   MAKE_SIG(type, gpu_shader5_or_es31_or_integer_functions, 3, value, offset,
            operand cast_offset = is_uint ? i2u(offset) : operand(offset);
            body.emit(ret(expr(ir_triop_bitfield_extract, value,
      swizzle(cast_offset, SWIZZLE_XXXX, type->vector_elements),
            }
      ir_function_signature *
   builtin_builder::_bitfieldInsert(const glsl_type *type)
   {
      bool is_uint = type->base_type == GLSL_TYPE_UINT;
   ir_variable *base   = in_var(type, "base");
   ir_variable *insert = in_var(type, "insert");
   ir_variable *offset = in_var(glsl_type::int_type, "offset");
   ir_variable *bits   = in_var(glsl_type::int_type, "bits");
   MAKE_SIG(type, gpu_shader5_or_es31_or_integer_functions, 4, base, insert,
            operand cast_offset = is_uint ? i2u(offset) : operand(offset);
            body.emit(ret(bitfield_insert(base, insert,
      swizzle(cast_offset, SWIZZLE_XXXX, type->vector_elements),
            }
      ir_function_signature *
   builtin_builder::_bitfieldReverse(const glsl_type *type)
   {
      ir_variable *x = in_highp_var(type, "x");
   MAKE_SIG(type, gpu_shader5_or_es31_or_integer_functions, 1, x);
   sig->return_precision = GLSL_PRECISION_HIGH;
   body.emit(ret(expr(ir_unop_bitfield_reverse, x)));
      }
      ir_function_signature *
   builtin_builder::_bitCount(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   MAKE_SIG(glsl_type::ivec(type->vector_elements), gpu_shader5_or_es31_or_integer_functions, 1, x);
   sig->return_precision = GLSL_PRECISION_LOW;
   body.emit(ret(expr(ir_unop_bit_count, x)));
      }
      ir_function_signature *
   builtin_builder::_findLSB(const glsl_type *type)
   {
      ir_variable *x = in_highp_var(type, "x");
   MAKE_SIG(glsl_type::ivec(type->vector_elements), gpu_shader5_or_es31_or_integer_functions, 1, x);
   sig->return_precision = GLSL_PRECISION_LOW;
   body.emit(ret(expr(ir_unop_find_lsb, x)));
      }
      ir_function_signature *
   builtin_builder::_findMSB(const glsl_type *type)
   {
      ir_variable *x = in_highp_var(type, "x");
   MAKE_SIG(glsl_type::ivec(type->vector_elements), gpu_shader5_or_es31_or_integer_functions, 1, x);
   sig->return_precision = GLSL_PRECISION_LOW;
   body.emit(ret(expr(ir_unop_find_msb, x)));
      }
      ir_function_signature *
   builtin_builder::_countLeadingZeros(builtin_available_predicate avail,
         {
      return unop(avail, ir_unop_clz,
      }
      ir_function_signature *
   builtin_builder::_countTrailingZeros(builtin_available_predicate avail,
         {
      ir_variable *a = in_var(type, "a");
            body.emit(ret(ir_builder::min2(
                     }
      ir_function_signature *
   builtin_builder::_fma(builtin_available_predicate avail, const glsl_type *type)
   {
      ir_variable *a = in_var(type, "a");
   ir_variable *b = in_var(type, "b");
   ir_variable *c = in_var(type, "c");
                        }
      ir_function_signature *
   builtin_builder::_ldexp(const glsl_type *x_type, const glsl_type *exp_type)
   {
      ir_variable *x = in_highp_var(x_type, "x");
   ir_variable *y = in_highp_var(exp_type, "y");
   MAKE_SIG(x_type, x_type->is_double() ? fp64 : gpu_shader5_or_es31_or_integer_functions, 2, x, y);
   sig->return_precision = GLSL_PRECISION_HIGH;
   body.emit(ret(expr(ir_binop_ldexp, x, y)));
      }
      ir_function_signature *
   builtin_builder::_frexp(const glsl_type *x_type, const glsl_type *exp_type)
   {
      ir_variable *x = in_highp_var(x_type, "x");
   ir_variable *exponent = out_var(exp_type, "exp");
   MAKE_SIG(x_type, x_type->is_double() ? fp64 : gpu_shader5_or_es31_or_integer_functions,
                           body.emit(ret(expr(ir_unop_frexp_sig, x)));
      }
      ir_function_signature *
   builtin_builder::_uaddCarry(const glsl_type *type)
   {
      ir_variable *x = in_highp_var(type, "x");
   ir_variable *y = in_highp_var(type, "y");
   ir_variable *carry = out_lowp_var(type, "carry");
   MAKE_SIG(type, gpu_shader5_or_es31_or_integer_functions, 3, x, y, carry);
            body.emit(assign(carry, ir_builder::carry(x, y)));
               }
      ir_function_signature *
   builtin_builder::_addSaturate(builtin_available_predicate avail,
         {
         }
      ir_function_signature *
   builtin_builder::_usubBorrow(const glsl_type *type)
   {
      ir_variable *x = in_highp_var(type, "x");
   ir_variable *y = in_highp_var(type, "y");
   ir_variable *borrow = out_lowp_var(type, "borrow");
   MAKE_SIG(type, gpu_shader5_or_es31_or_integer_functions, 3, x, y, borrow);
            body.emit(assign(borrow, ir_builder::borrow(x, y)));
               }
      ir_function_signature *
   builtin_builder::_subtractSaturate(builtin_available_predicate avail,
         {
         }
      ir_function_signature *
   builtin_builder::_absoluteDifference(builtin_available_predicate avail,
         {
      /* absoluteDifference returns an unsigned type that has the same number of
   * bits and number of vector elements as the type of the operands.
   */
   return binop(avail, ir_binop_abs_sub,
                  }
      ir_function_signature *
   builtin_builder::_average(builtin_available_predicate avail,
         {
         }
      ir_function_signature *
   builtin_builder::_averageRounded(builtin_available_predicate avail,
         {
         }
      /**
   * For both imulExtended() and umulExtended() built-ins.
   */
   ir_function_signature *
   builtin_builder::_mulExtended(const glsl_type *type)
   {
      const glsl_type *mul_type, *unpack_type;
            if (type->base_type == GLSL_TYPE_INT) {
      unpack_op = ir_unop_unpack_int_2x32;
   mul_type = glsl_type::get_instance(GLSL_TYPE_INT64, type->vector_elements, 1);
      } else {
      unpack_op = ir_unop_unpack_uint_2x32;
   mul_type = glsl_type::get_instance(GLSL_TYPE_UINT64, type->vector_elements, 1);
               ir_variable *x = in_highp_var(type, "x");
   ir_variable *y = in_highp_var(type, "y");
   ir_variable *msb = out_highp_var(type, "msb");
   ir_variable *lsb = out_highp_var(type, "lsb");
                     ir_expression *mul_res = new(mem_ctx) ir_expression(ir_binop_mul, mul_type,
                  if (type->vector_elements == 1) {
      body.emit(assign(unpack_val, expr(unpack_op, mul_res)));
   body.emit(assign(msb, swizzle_y(unpack_val)));
      } else {
      for (int i = 0; i < type->vector_elements; i++) {
      body.emit(assign(unpack_val, expr(unpack_op, swizzle(mul_res, i, 1))));
   body.emit(assign(array_ref(msb, i), swizzle_y(unpack_val)));
                     }
      ir_function_signature *
   builtin_builder::_multiply32x16(builtin_available_predicate avail,
         {
         }
      ir_function_signature *
   builtin_builder::_interpolateAtCentroid(const glsl_type *type)
   {
      ir_variable *interpolant = in_var(type, "interpolant");
   interpolant->data.must_be_shader_input = 1;
                        }
      ir_function_signature *
   builtin_builder::_interpolateAtOffset(const glsl_type *type)
   {
      ir_variable *interpolant = in_var(type, "interpolant");
   interpolant->data.must_be_shader_input = 1;
   ir_variable *offset = in_var(glsl_type::vec2_type, "offset");
                        }
      ir_function_signature *
   builtin_builder::_interpolateAtSample(const glsl_type *type)
   {
      ir_variable *interpolant = in_var(type, "interpolant");
   interpolant->data.must_be_shader_input = 1;
   ir_variable *sample_num = in_var(glsl_type::int_type, "sample_num");
                        }
      /* The highp isn't specified in the built-in function descriptions, but in the
   * atomic counter description: "The default precision of all atomic types is
   * highp. It is an error to declare an atomic type with a different precision or
   * to specify the default precision for an atomic type to be lowp or mediump."
   */
   ir_function_signature *
   builtin_builder::_atomic_counter_intrinsic(builtin_available_predicate avail,
         {
      ir_variable *counter = in_highp_var(glsl_type::atomic_uint_type, "counter");
   MAKE_INTRINSIC(glsl_type::uint_type, id, avail, 1, counter);
      }
      ir_function_signature *
   builtin_builder::_atomic_counter_intrinsic1(builtin_available_predicate avail,
         {
      ir_variable *counter = in_highp_var(glsl_type::atomic_uint_type, "counter");
   ir_variable *data = in_var(glsl_type::uint_type, "data");
   MAKE_INTRINSIC(glsl_type::uint_type, id, avail, 2, counter, data);
      }
      ir_function_signature *
   builtin_builder::_atomic_counter_intrinsic2(builtin_available_predicate avail,
         {
      ir_variable *counter = in_highp_var(glsl_type::atomic_uint_type, "counter");
   ir_variable *compare = in_var(glsl_type::uint_type, "compare");
   ir_variable *data = in_var(glsl_type::uint_type, "data");
   MAKE_INTRINSIC(glsl_type::uint_type, id, avail, 3, counter, compare, data);
      }
      ir_function_signature *
   builtin_builder::_atomic_intrinsic2(builtin_available_predicate avail,
               {
      ir_variable *atomic = in_var(type, "atomic");
   ir_variable *data = in_var(type, "data");
   MAKE_INTRINSIC(type, id, avail, 2, atomic, data);
      }
      ir_function_signature *
   builtin_builder::_atomic_intrinsic3(builtin_available_predicate avail,
               {
      ir_variable *atomic = in_var(type, "atomic");
   ir_variable *data1 = in_var(type, "data1");
   ir_variable *data2 = in_var(type, "data2");
   MAKE_INTRINSIC(type, id, avail, 3, atomic, data1, data2);
      }
      ir_function_signature *
   builtin_builder::_atomic_counter_op(const char *intrinsic,
         {
      ir_variable *counter = in_highp_var(glsl_type::atomic_uint_type, "atomic_counter");
            ir_variable *retval = body.make_temp(glsl_type::uint_type, "atomic_retval");
   body.emit(call(shader->symbols->get_function(intrinsic), retval,
         body.emit(ret(retval));
      }
      ir_function_signature *
   builtin_builder::_atomic_counter_op1(const char *intrinsic,
         {
      ir_variable *counter = in_highp_var(glsl_type::atomic_uint_type, "atomic_counter");
   ir_variable *data = in_var(glsl_type::uint_type, "data");
                     /* Instead of generating an __intrinsic_atomic_sub, generate an
   * __intrinsic_atomic_add with the data parameter negated.
   */
   if (strcmp("__intrinsic_atomic_sub", intrinsic) == 0) {
      ir_variable *const neg_data =
                              parameters.push_tail(new(mem_ctx) ir_dereference_variable(counter));
            ir_function *const func =
                  assert(c != NULL);
               } else {
      body.emit(call(shader->symbols->get_function(intrinsic), retval,
               body.emit(ret(retval));
      }
      ir_function_signature *
   builtin_builder::_atomic_counter_op2(const char *intrinsic,
         {
      ir_variable *counter = in_highp_var(glsl_type::atomic_uint_type, "atomic_counter");
   ir_variable *compare = in_var(glsl_type::uint_type, "compare");
   ir_variable *data = in_var(glsl_type::uint_type, "data");
            ir_variable *retval = body.make_temp(glsl_type::uint_type, "atomic_retval");
   body.emit(call(shader->symbols->get_function(intrinsic), retval,
         body.emit(ret(retval));
      }
      ir_function_signature *
   builtin_builder::_atomic_op2(const char *intrinsic,
               {
      ir_variable *atomic = in_var(type, "atomic_var");
   ir_variable *data = in_var(type, "atomic_data");
                     ir_variable *retval = body.make_temp(type, "atomic_retval");
   body.emit(call(shader->symbols->get_function(intrinsic), retval,
         body.emit(ret(retval));
      }
      ir_function_signature *
   builtin_builder::_atomic_op3(const char *intrinsic,
               {
      ir_variable *atomic = in_var(type, "atomic_var");
   ir_variable *data1 = in_var(type, "atomic_data1");
   ir_variable *data2 = in_var(type, "atomic_data2");
                     ir_variable *retval = body.make_temp(type, "atomic_retval");
   body.emit(call(shader->symbols->get_function(intrinsic), retval,
         body.emit(ret(retval));
      }
      ir_function_signature *
   builtin_builder::_min3(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   ir_variable *y = in_var(type, "y");
   ir_variable *z = in_var(type, "z");
            ir_expression *min3 = min2(x, min2(y,z));
               }
      ir_function_signature *
   builtin_builder::_max3(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   ir_variable *y = in_var(type, "y");
   ir_variable *z = in_var(type, "z");
            ir_expression *max3 = max2(x, max2(y,z));
               }
      ir_function_signature *
   builtin_builder::_mid3(const glsl_type *type)
   {
      ir_variable *x = in_var(type, "x");
   ir_variable *y = in_var(type, "y");
   ir_variable *z = in_var(type, "z");
            ir_expression *mid3 = max2(min2(x, y), max2(min2(x, z), min2(y, z)));
               }
      static builtin_available_predicate
   get_image_available_predicate(const glsl_type *type, unsigned flags)
   {
      if ((flags & IMAGE_FUNCTION_AVAIL_ATOMIC_EXCHANGE) &&
      type->sampled_type == GLSL_TYPE_FLOAT)
         if ((flags & IMAGE_FUNCTION_AVAIL_ATOMIC_ADD) &&
      type->sampled_type == GLSL_TYPE_FLOAT)
         else if (flags & (IMAGE_FUNCTION_AVAIL_ATOMIC_EXCHANGE |
                        else if (flags & IMAGE_FUNCTION_EXT_ONLY)
            else if (flags & IMAGE_FUNCTION_SPARSE)
            else
      }
      ir_function_signature *
   builtin_builder::_image_prototype(const glsl_type *image_type,
               {
      const glsl_type *data_type = glsl_type::get_instance(
      image_type->sampled_type,
   (flags & IMAGE_FUNCTION_HAS_VECTOR_DATA_TYPE ? 4 : 1),
         const glsl_type *ret_type;
   if (flags & IMAGE_FUNCTION_RETURNS_VOID)
         else if (flags & IMAGE_FUNCTION_SPARSE) {
      if (flags & IMAGE_FUNCTION_EMIT_STUB)
         else {
      /* code holds residency info */
   glsl_struct_field fields[2] = {
      glsl_struct_field(glsl_type::int_type, "code"),
      };
         } else
            /* Addressing arguments that are always present. */
   ir_variable *image = in_var(image_type, "image");
   ir_variable *coord = in_var(
            ir_function_signature *sig = new_sig(
      ret_type, get_image_available_predicate(image_type, flags),
         /* Sample index for multisample images. */
   if (image_type->sampler_dimensionality == GLSL_SAMPLER_DIM_MS)
            /* Data arguments. */
   for (unsigned i = 0; i < num_arguments; ++i) {
      char *arg_name = ralloc_asprintf(NULL, "arg%d", i);
   sig->parameters.push_tail(in_var(data_type, arg_name));
               /* Set the maximal set of qualifiers allowed for this image
   * built-in.  Function calls with arguments having fewer
   * qualifiers than present in the prototype are allowed by the
   * spec, but not with more, i.e. this will make the compiler
   * accept everything that needs to be accepted, and reject cases
   * like loads from write-only or stores to read-only images.
   */
   image->data.memory_read_only = (flags & IMAGE_FUNCTION_READ_ONLY) != 0;
   image->data.memory_write_only = (flags & IMAGE_FUNCTION_WRITE_ONLY) != 0;
   image->data.memory_coherent = true;
   image->data.memory_volatile = true;
               }
      ir_function_signature *
   builtin_builder::_image_size_prototype(const glsl_type *image_type,
               {
      const glsl_type *ret_type;
            /* From the ARB_shader_image_size extension:
   * "Cube images return the dimensions of one face."
   */
   if (image_type->sampler_dimensionality == GLSL_SAMPLER_DIM_CUBE &&
      !image_type->sampler_array) {
               /* FIXME: Add the highp precision qualifier for GLES 3.10 when it is
   * supported by mesa.
   */
            ir_variable *image = in_var(image_type, "image");
            /* Set the maximal set of qualifiers allowed for this image
   * built-in.  Function calls with arguments having fewer
   * qualifiers than present in the prototype are allowed by the
   * spec, but not with more, i.e. this will make the compiler
   * accept everything that needs to be accepted, and reject cases
   * like loads from write-only or stores to read-only images.
   */
   image->data.memory_read_only = true;
   image->data.memory_write_only = true;
   image->data.memory_coherent = true;
   image->data.memory_volatile = true;
               }
      ir_function_signature *
   builtin_builder::_image_samples_prototype(const glsl_type *image_type,
               {
      ir_variable *image = in_var(image_type, "image");
   ir_function_signature *sig =
            /* Set the maximal set of qualifiers allowed for this image
   * built-in.  Function calls with arguments having fewer
   * qualifiers than present in the prototype are allowed by the
   * spec, but not with more, i.e. this will make the compiler
   * accept everything that needs to be accepted, and reject cases
   * like loads from write-only or stores to read-only images.
   */
   image->data.memory_read_only = true;
   image->data.memory_write_only = true;
   image->data.memory_coherent = true;
   image->data.memory_volatile = true;
               }
      ir_function_signature *
   builtin_builder::_image(image_prototype_ctr prototype,
                           const glsl_type *image_type,
   {
      ir_function_signature *sig = (this->*prototype)(image_type,
            if (flags & IMAGE_FUNCTION_EMIT_STUB) {
      ir_factory body(&sig->body, mem_ctx);
            if (flags & IMAGE_FUNCTION_RETURNS_VOID) {
         } else if (flags & IMAGE_FUNCTION_SPARSE) {
      ir_function_signature *intr_sig =
                  ir_variable *ret_val = body.make_temp(intr_sig->return_type, "_ret_val");
                  /* Add texel param to builtin function after call intrinsic function
   * because they have different prototype:
   *   struct {int code; gvec4 texel;} __intrinsic_image_sparse_load(in)
   *   int sparseImageLoad(in, out texel)
   */
                  body.emit(assign(texel, texel_field));
      } else {
      ir_variable *ret_val =
         /* all non-void image functions return highp, so make our temporary and return
   * value in the signature highp.
   */
   ret_val->data.precision = GLSL_PRECISION_HIGH;
   body.emit(call(f, ret_val, sig->parameters));
                     } else {
         }
               }
      ir_function_signature *
   builtin_builder::_memory_barrier_intrinsic(builtin_available_predicate avail,
         {
      MAKE_INTRINSIC(glsl_type::void_type, id, avail, 0);
      }
      ir_function_signature *
   builtin_builder::_memory_barrier(const char *intrinsic_name,
         {
      MAKE_SIG(glsl_type::void_type, avail, 0);
   body.emit(call(shader->symbols->get_function(intrinsic_name),
            }
      ir_function_signature *
   builtin_builder::_ballot_intrinsic()
   {
      ir_variable *value = in_var(glsl_type::bool_type, "value");
   MAKE_INTRINSIC(glsl_type::uint64_t_type, ir_intrinsic_ballot, shader_ballot,
            }
      ir_function_signature *
   builtin_builder::_ballot()
   {
               MAKE_SIG(glsl_type::uint64_t_type, shader_ballot, 1, value);
            body.emit(call(shader->symbols->get_function("__intrinsic_ballot"),
         body.emit(ret(retval));
      }
      ir_function_signature *
   builtin_builder::_read_first_invocation_intrinsic(const glsl_type *type)
   {
      ir_variable *value = in_var(type, "value");
   MAKE_INTRINSIC(type, ir_intrinsic_read_first_invocation, shader_ballot,
            }
      ir_function_signature *
   builtin_builder::_read_first_invocation(const glsl_type *type)
   {
               MAKE_SIG(type, shader_ballot, 1, value);
            body.emit(call(shader->symbols->get_function("__intrinsic_read_first_invocation"),
         body.emit(ret(retval));
      }
      ir_function_signature *
   builtin_builder::_read_invocation_intrinsic(const glsl_type *type)
   {
      ir_variable *value = in_var(type, "value");
   ir_variable *invocation = in_var(glsl_type::uint_type, "invocation");
   MAKE_INTRINSIC(type, ir_intrinsic_read_invocation, shader_ballot,
            }
      ir_function_signature *
   builtin_builder::_read_invocation(const glsl_type *type)
   {
      ir_variable *value = in_var(type, "value");
            MAKE_SIG(type, shader_ballot, 2, value, invocation);
            body.emit(call(shader->symbols->get_function("__intrinsic_read_invocation"),
         body.emit(ret(retval));
      }
      ir_function_signature *
   builtin_builder::_invocation_interlock_intrinsic(builtin_available_predicate avail,
         {
      MAKE_INTRINSIC(glsl_type::void_type, id, avail, 0);
      }
      ir_function_signature *
   builtin_builder::_invocation_interlock(const char *intrinsic_name,
         {
      MAKE_SIG(glsl_type::void_type, avail, 0);
   body.emit(call(shader->symbols->get_function(intrinsic_name),
            }
      ir_function_signature *
   builtin_builder::_shader_clock_intrinsic(builtin_available_predicate avail,
         {
      MAKE_INTRINSIC(type, ir_intrinsic_shader_clock, avail, 0);
      }
      ir_function_signature *
   builtin_builder::_shader_clock(builtin_available_predicate avail,
         {
                        body.emit(call(shader->symbols->get_function("__intrinsic_shader_clock"),
            if (type == glsl_type::uint64_t_type) {
         } else {
                     }
      ir_function_signature *
   builtin_builder::_vote_intrinsic(builtin_available_predicate avail,
         {
      ir_variable *value = in_var(glsl_type::bool_type, "value");
   MAKE_INTRINSIC(glsl_type::bool_type, id, avail, 1, value);
      }
      ir_function_signature *
   builtin_builder::_vote(const char *intrinsic_name,
         {
                                 body.emit(call(shader->symbols->get_function(intrinsic_name),
         body.emit(ret(retval));
      }
      ir_function_signature *
   builtin_builder::_helper_invocation_intrinsic()
   {
      MAKE_INTRINSIC(glsl_type::bool_type, ir_intrinsic_helper_invocation,
            }
      ir_function_signature *
   builtin_builder::_helper_invocation()
   {
                        body.emit(call(shader->symbols->get_function("__intrinsic_helper_invocation"),
                     }
      /** @} */
      /******************************************************************************/
      /* The singleton instance of builtin_builder. */
   static builtin_builder builtins;
   static uint32_t builtin_users = 0;
      /**
   * External API (exposing the built-in module to the rest of the compiler):
   *  @{
   */
   extern "C" void
   _mesa_glsl_builtin_functions_init_or_ref()
   {
      simple_mtx_lock(&builtins_lock);
   if (builtin_users++ == 0)
            }
      extern "C" void
   _mesa_glsl_builtin_functions_decref()
   {
      simple_mtx_lock(&builtins_lock);
   assert(builtin_users != 0);
   if (--builtin_users == 0)
            }
      ir_function_signature *
   _mesa_glsl_find_builtin_function(_mesa_glsl_parse_state *state,
         {
      ir_function_signature *s;
   simple_mtx_lock(&builtins_lock);
   s = builtins.find(state, name, actual_parameters);
               }
      bool
   _mesa_glsl_has_builtin_function(_mesa_glsl_parse_state *state, const char *name)
   {
      ir_function *f;
   bool ret = false;
   simple_mtx_lock(&builtins_lock);
   f = builtins.shader->symbols->get_function(name);
   if (f != NULL) {
      foreach_in_list(ir_function_signature, sig, &f->signatures) {
      if (sig->is_builtin_available(state)) {
      ret = true;
            }
               }
      gl_shader *
   _mesa_glsl_get_builtin_function_shader()
   {
         }
         /**
   * Get the function signature for main from a shader
   */
   ir_function_signature *
   _mesa_get_main_function_signature(glsl_symbol_table *symbols)
   {
      ir_function *const f = symbols->get_function("main");
   if (f != NULL) {
               /* Look for the 'void main()' signature and ensure that it's defined.
   * This keeps the linker from accidentally pick a shader that just
   * contains a prototype for main.
   *
   * We don't have to check for multiple definitions of main (in multiple
   * shaders) because that would have already been caught above.
   */
   ir_function_signature *sig =
         if ((sig != NULL) && sig->is_defined) {
                        }
      /** @} */
