   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "compiler/spirv/nir_spirv.h"
   #include "nir/nir.h"
   #include "rogue.h"
   #include "util/macros.h"
      #include <stdbool.h>
      /**
   * \file rogue_nir.c
   *
   * \brief Contains SPIR-V and NIR-specific functions.
   */
      /**
   * \brief SPIR-V to NIR compilation options.
   */
   static const struct spirv_to_nir_options spirv_options = {
               /* Buffer address: (descriptor_set, binding), offset. */
      };
      static const nir_shader_compiler_options nir_options = {
         };
      static int rogue_glsl_type_size(const struct glsl_type *type, bool bindless)
   {
         }
      /**
   * \brief Applies optimizations and passes required to lower the NIR shader into
   * a form suitable for lowering to Rogue IR.
   *
   * \param[in] ctx Shared multi-stage build context.
   * \param[in] shader Rogue shader.
   * \param[in] stage Shader stage.
   */
   static void rogue_nir_passes(struct rogue_build_ctx *ctx,
               {
            #if !defined(NDEBUG)
      bool nir_debug_print_shader_prev = nir_debug_print_shader[nir->info.stage];
      #endif /* !defined(NDEBUG) */
                           /* Splitting. */
   NIR_PASS_V(nir, nir_split_var_copies);
            /* Replace references to I/O variables with intrinsics. */
   NIR_PASS_V(nir,
            nir_lower_io,
   nir_var_shader_in | nir_var_shader_out,
         /* Load inputs to scalars (single registers later). */
   /* TODO: Fitrp can process multiple frag inputs at once, scalarise I/O. */
            /* Optimize GL access qualifiers. */
   const nir_opt_access_options opt_access_options = {
         };
            /* Apply PFO code to the fragment shader output. */
   if (nir->info.stage == MESA_SHADER_FRAGMENT)
            /* Load outputs to scalars (single registers later). */
            /* Lower ALU operations to scalars. */
            /* Lower load_consts to scalars. */
            /* Additional I/O lowering. */
   NIR_PASS_V(nir,
            nir_lower_explicit_io,
      NIR_PASS_V(nir, nir_lower_io_to_scalar, nir_var_mem_ubo, NULL, NULL);
            /* Algebraic opts. */
   do {
               NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_cse);
   NIR_PASS(progress, nir, nir_opt_algebraic);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
   NIR_PASS(progress, nir, nir_opt_dce);
               /* Late algebraic opts. */
   do {
               NIR_PASS(progress, nir, nir_opt_algebraic_late);
   NIR_PASS_V(nir, nir_opt_constant_folding);
   NIR_PASS_V(nir, nir_copy_prop);
   NIR_PASS_V(nir, nir_opt_dce);
               /* Remove unused constant registers. */
            /* Move loads to just before they're needed. */
   /* Disabled for now since we want to try and keep them vectorised and group
   * them. */
   /* TODO: Investigate this further. */
               #if 0
   /* Instruction scheduling. */
   struct nir_schedule_options schedule_options = {
   .threshold = ROGUE_MAX_REG_TEMP / 2,
   };
   NIR_PASS_V(nir, nir_schedule, &schedule_options);
   #endif
         /* Assign I/O locations. */
   nir_assign_io_var_locations(nir,
                     nir_assign_io_var_locations(nir,
                        /* Renumber SSA defs. */
            /* Gather info into nir shader struct. */
            /* Clean-up after passes. */
            nir_validate_shader(nir, "after passes");
   if (ROGUE_DEBUG(NIR)) {
      fputs("after passes\n", stdout);
            #if !defined(NDEBUG)
         #endif /* !defined(NDEBUG) */
   }
      /**
   * \brief Converts a SPIR-V shader to NIR.
   *
   * \param[in] ctx Shared multi-stage build context.
   * \param[in] entry Shader entry-point function name.
   * \param[in] stage Shader stage.
   * \param[in] spirv_size SPIR-V data length in DWORDs.
   * \param[in] spirv_data SPIR-V data.
   * \param[in] num_spec Number of SPIR-V specializations.
   * \param[in] spec SPIR-V specializations.
   * \return A nir_shader* if successful, or NULL if unsuccessful.
   */
   PUBLIC
   nir_shader *rogue_spirv_to_nir(rogue_build_ctx *ctx,
                                 gl_shader_stage stage,
   {
               nir = spirv_to_nir(spirv_data,
                     spirv_size,
   spec,
   num_spec,
   stage,
   if (!nir)
                     /* Apply passes. */
            /* Collect I/O data to pass back to the driver. */
               }
