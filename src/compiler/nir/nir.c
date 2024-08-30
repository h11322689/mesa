   /*
   * Copyright © 2014 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   *
   * Authors:
   *    Connor Abbott (cwabbott0@gmail.com)
   *
   */
      #include "nir.h"
   #include <assert.h>
   #include <limits.h>
   #include <math.h>
   #include "util/half_float.h"
   #include "util/u_math.h"
   #include "util/u_qsort.h"
   #include "nir_builder.h"
   #include "nir_control_flow_private.h"
   #include "nir_worklist.h"
      #include "main/menums.h" /* BITFIELD64_MASK */
      #ifndef NDEBUG
   uint32_t nir_debug = 0;
   bool nir_debug_print_shader[MESA_SHADER_KERNEL + 1] = { 0 };
      static const struct debug_named_value nir_debug_control[] = {
      { "clone", NIR_DEBUG_CLONE,
   "Test cloning a shader at each successful lowering/optimization call" },
   { "serialize", NIR_DEBUG_SERIALIZE,
   "Test serialize and deserialize shader at each successful lowering/optimization call" },
   { "novalidate", NIR_DEBUG_NOVALIDATE,
   "Disable shader validation at each successful lowering/optimization call" },
   { "validate_ssa_dominance", NIR_DEBUG_VALIDATE_SSA_DOMINANCE,
   "Validate SSA dominance in shader at each successful lowering/optimization call" },
   { "tgsi", NIR_DEBUG_TGSI,
   "Dump NIR/TGSI shaders when doing a NIR<->TGSI translation" },
   { "print", NIR_DEBUG_PRINT,
   "Dump resulting shader after each successful lowering/optimization call" },
   { "print_vs", NIR_DEBUG_PRINT_VS,
   "Dump resulting vertex shader after each successful lowering/optimization call" },
   { "print_tcs", NIR_DEBUG_PRINT_TCS,
   "Dump resulting tessellation control shader after each successful lowering/optimization call" },
   { "print_tes", NIR_DEBUG_PRINT_TES,
   "Dump resulting tessellation evaluation shader after each successful lowering/optimization call" },
   { "print_gs", NIR_DEBUG_PRINT_GS,
   "Dump resulting geometry shader after each successful lowering/optimization call" },
   { "print_fs", NIR_DEBUG_PRINT_FS,
   "Dump resulting fragment shader after each successful lowering/optimization call" },
   { "print_cs", NIR_DEBUG_PRINT_CS,
   "Dump resulting compute shader after each successful lowering/optimization call" },
   { "print_ts", NIR_DEBUG_PRINT_TS,
   "Dump resulting task shader after each successful lowering/optimization call" },
   { "print_ms", NIR_DEBUG_PRINT_MS,
   "Dump resulting mesh shader after each successful lowering/optimization call" },
   { "print_rgs", NIR_DEBUG_PRINT_RGS,
   "Dump resulting raygen shader after each successful lowering/optimization call" },
   { "print_ahs", NIR_DEBUG_PRINT_AHS,
   "Dump resulting any-hit shader after each successful lowering/optimization call" },
   { "print_chs", NIR_DEBUG_PRINT_CHS,
   "Dump resulting closest-hit shader after each successful lowering/optimization call" },
   { "print_mhs", NIR_DEBUG_PRINT_MHS,
   "Dump resulting miss-hit shader after each successful lowering/optimization call" },
   { "print_is", NIR_DEBUG_PRINT_IS,
   "Dump resulting intersection shader after each successful lowering/optimization call" },
   { "print_cbs", NIR_DEBUG_PRINT_CBS,
   "Dump resulting callable shader after each successful lowering/optimization call" },
   { "print_ks", NIR_DEBUG_PRINT_KS,
   "Dump resulting kernel shader after each successful lowering/optimization call" },
   { "print_no_inline_consts", NIR_DEBUG_PRINT_NO_INLINE_CONSTS,
   "Do not print const value near each use of const SSA variable" },
   { "print_internal", NIR_DEBUG_PRINT_INTERNAL,
   "Print shaders even if they are marked as internal" },
   { "print_pass_flags", NIR_DEBUG_PRINT_PASS_FLAGS,
   "Print pass_flags for every instruction when pass_flags are non-zero" },
      };
      DEBUG_GET_ONCE_FLAGS_OPTION(nir_debug, "NIR_DEBUG", nir_debug_control, 0)
      static void
   nir_process_debug_variable_once(void)
   {
               /* clang-format off */
   nir_debug_print_shader[MESA_SHADER_VERTEX]       = NIR_DEBUG(PRINT_VS);
   nir_debug_print_shader[MESA_SHADER_TESS_CTRL]    = NIR_DEBUG(PRINT_TCS);
   nir_debug_print_shader[MESA_SHADER_TESS_EVAL]    = NIR_DEBUG(PRINT_TES);
   nir_debug_print_shader[MESA_SHADER_GEOMETRY]     = NIR_DEBUG(PRINT_GS);
   nir_debug_print_shader[MESA_SHADER_FRAGMENT]     = NIR_DEBUG(PRINT_FS);
   nir_debug_print_shader[MESA_SHADER_COMPUTE]      = NIR_DEBUG(PRINT_CS);
   nir_debug_print_shader[MESA_SHADER_TASK]         = NIR_DEBUG(PRINT_TS);
   nir_debug_print_shader[MESA_SHADER_MESH]         = NIR_DEBUG(PRINT_MS);
   nir_debug_print_shader[MESA_SHADER_RAYGEN]       = NIR_DEBUG(PRINT_RGS);
   nir_debug_print_shader[MESA_SHADER_ANY_HIT]      = NIR_DEBUG(PRINT_AHS);
   nir_debug_print_shader[MESA_SHADER_CLOSEST_HIT]  = NIR_DEBUG(PRINT_CHS);
   nir_debug_print_shader[MESA_SHADER_MISS]         = NIR_DEBUG(PRINT_MHS);
   nir_debug_print_shader[MESA_SHADER_INTERSECTION] = NIR_DEBUG(PRINT_IS);
   nir_debug_print_shader[MESA_SHADER_CALLABLE]     = NIR_DEBUG(PRINT_CBS);
   nir_debug_print_shader[MESA_SHADER_KERNEL]       = NIR_DEBUG(PRINT_KS);
      }
      void
   nir_process_debug_variable(void)
   {
      static once_flag flag = ONCE_FLAG_INIT;
      }
   #endif
      /** Return true if the component mask "mask" with bit size "old_bit_size" can
   * be re-interpreted to be used with "new_bit_size".
   */
   bool
   nir_component_mask_can_reinterpret(nir_component_mask_t mask,
               {
      assert(util_is_power_of_two_nonzero(old_bit_size));
            if (old_bit_size == new_bit_size)
            if (old_bit_size == 1 || new_bit_size == 1)
            if (old_bit_size > new_bit_size) {
      unsigned ratio = old_bit_size / new_bit_size;
               unsigned iter = mask;
   while (iter) {
      int start, count;
   u_bit_scan_consecutive_range(&iter, &start, &count);
   start *= old_bit_size;
   count *= old_bit_size;
   if (start % new_bit_size != 0)
         if (count % new_bit_size != 0)
      }
      }
      /** Re-interprets a component mask "mask" with bit size "old_bit_size" so that
   * it can be used can be used with "new_bit_size".
   */
   nir_component_mask_t
   nir_component_mask_reinterpret(nir_component_mask_t mask,
               {
               if (old_bit_size == new_bit_size)
            nir_component_mask_t new_mask = 0;
   unsigned iter = mask;
   while (iter) {
      int start, count;
   u_bit_scan_consecutive_range(&iter, &start, &count);
   start = start * old_bit_size / new_bit_size;
   count = count * old_bit_size / new_bit_size;
      }
      }
      nir_shader *
   nir_shader_create(void *mem_ctx,
                     {
                     #ifndef NDEBUG
         #endif
                           if (si) {
      assert(si->stage == stage);
      } else {
                           shader->num_inputs = 0;
   shader->num_outputs = 0;
               }
      void
   nir_shader_add_variable(nir_shader *shader, nir_variable *var)
   {
      switch (var->data.mode) {
   case nir_var_function_temp:
      assert(!"nir_shader_add_variable cannot be used for local variables");
         case nir_var_shader_temp:
   case nir_var_shader_in:
   case nir_var_shader_out:
   case nir_var_uniform:
   case nir_var_mem_ubo:
   case nir_var_mem_ssbo:
   case nir_var_image:
   case nir_var_mem_shared:
   case nir_var_system_value:
   case nir_var_mem_push_const:
   case nir_var_mem_constant:
   case nir_var_shader_call_data:
   case nir_var_ray_hit_attrib:
   case nir_var_mem_task_payload:
   case nir_var_mem_node_payload:
   case nir_var_mem_node_payload_in:
   case nir_var_mem_global:
            default:
      assert(!"invalid mode");
                  }
      nir_variable *
   nir_variable_create(nir_shader *shader, nir_variable_mode mode,
         {
      nir_variable *var = rzalloc(shader, nir_variable);
   var->name = ralloc_strdup(var, name);
   var->type = type;
   var->data.mode = mode;
            if ((mode == nir_var_shader_in &&
      shader->info.stage != MESA_SHADER_VERTEX &&
   shader->info.stage != MESA_SHADER_KERNEL) ||
   (mode == nir_var_shader_out &&
   shader->info.stage != MESA_SHADER_FRAGMENT))
         if (mode == nir_var_shader_in || mode == nir_var_uniform)
                        }
      nir_variable *
   nir_local_variable_create(nir_function_impl *impl,
         {
      nir_variable *var = rzalloc(impl->function->shader, nir_variable);
   var->name = ralloc_strdup(var, name);
   var->type = type;
                        }
      nir_variable *
   nir_state_variable_create(nir_shader *shader,
                     {
      nir_variable *var = nir_variable_create(shader, nir_var_uniform, type, name);
   var->num_state_slots = 1;
   var->state_slots = rzalloc_array(var, nir_state_slot, 1);
   memcpy(var->state_slots[0].tokens, tokens,
         shader->num_uniforms++;
      }
      nir_variable *
   nir_create_variable_with_location(nir_shader *shader, nir_variable_mode mode, int location,
         {
      /* Only supporting non-array, or arrayed-io types, because otherwise we don't
   * know how much to increment num_inputs/outputs
   */
            const char *name;
   switch (mode) {
   case nir_var_shader_in:
      if (shader->info.stage == MESA_SHADER_VERTEX)
         else
               case nir_var_shader_out:
      if (shader->info.stage == MESA_SHADER_FRAGMENT)
         else
               case nir_var_system_value:
      name = gl_system_value_name(location);
         default:
                  nir_variable *var = nir_variable_create(shader, mode, type, name);
            switch (mode) {
   case nir_var_shader_in:
      var->data.driver_location = shader->num_inputs++;
         case nir_var_shader_out:
      var->data.driver_location = shader->num_outputs++;
         case nir_var_system_value:
            default:
                     }
      nir_variable *
   nir_get_variable_with_location(nir_shader *shader, nir_variable_mode mode, int location,
         {
      nir_variable *var = nir_find_variable_with_location(shader, mode, location);
   if (var) {
      /* If this shader has location_fracs, this builder function is not suitable. */
            /* The variable for the slot should match what we expected. */
   assert(type == var->type);
                  }
      nir_variable *
   nir_find_variable_with_location(nir_shader *shader,
               {
      assert(util_bitcount(mode) == 1 && mode != nir_var_function_temp);
   nir_foreach_variable_with_modes(var, shader, mode) {
      if (var->data.location == location)
      }
      }
      nir_variable *
   nir_find_variable_with_driver_location(nir_shader *shader,
               {
      assert(util_bitcount(mode) == 1 && mode != nir_var_function_temp);
   nir_foreach_variable_with_modes(var, shader, mode) {
      if (var->data.driver_location == location)
      }
      }
      nir_variable *
   nir_find_state_variable(nir_shader *s,
         {
      nir_foreach_variable_with_modes(var, s, nir_var_uniform) {
      if (var->num_state_slots == 1 &&
      !memcmp(var->state_slots[0].tokens, tokens,
         }
      }
      nir_variable *nir_find_sampler_variable_with_tex_index(nir_shader *shader,
         {
      nir_foreach_variable_with_modes(var, shader, nir_var_uniform) {
      unsigned size =
         if ((glsl_type_is_texture(glsl_without_array(var->type)) ||
      glsl_type_is_sampler(glsl_without_array(var->type))) &&
   (var->data.binding == texture_index ||
   (var->data.binding < texture_index &&
         }
      }
      /* Annoyingly, qsort_r is not in the C standard library and, in particular, we
   * can't count on it on MSV and Android.  So we stuff the CMP function into
   * each array element.  It's a bit messy and burns more memory but the list of
   * variables should hever be all that long.
   */
   struct var_cmp {
      nir_variable *var;
      };
      static int
   var_sort_cmp(const void *_a, const void *_b, void *_cmp)
   {
      const struct var_cmp *a = _a;
   const struct var_cmp *b = _b;
   assert(a->cmp == b->cmp);
      }
      void
   nir_sort_variables_with_modes(nir_shader *shader,
                     {
      unsigned num_vars = 0;
   nir_foreach_variable_with_modes(var, shader, modes) {
         }
   struct var_cmp *vars = ralloc_array(shader, struct var_cmp, num_vars);
   unsigned i = 0;
   nir_foreach_variable_with_modes_safe(var, shader, modes) {
      exec_node_remove(&var->node);
   vars[i++] = (struct var_cmp){
      .var = var,
         }
                     for (i = 0; i < num_vars; i++)
               }
      nir_function *
   nir_function_create(nir_shader *shader, const char *name)
   {
                        func->name = ralloc_strdup(func, name);
   func->shader = shader;
   func->num_params = 0;
   func->params = NULL;
   func->impl = NULL;
   func->is_entrypoint = false;
   func->is_preamble = false;
   func->dont_inline = false;
               }
      void
   nir_alu_src_copy(nir_alu_src *dest, const nir_alu_src *src)
   {
      dest->src = nir_src_for_ssa(src->src.ssa);
   for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++)
      }
      bool
   nir_alu_src_is_trivial_ssa(const nir_alu_instr *alu, unsigned srcn)
   {
      static uint8_t trivial_swizzle[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
            const nir_alu_src *src = &alu->src[srcn];
            return (src->src.ssa->num_components == num_components) &&
      }
      static void
   cf_init(nir_cf_node *node, nir_cf_node_type type)
   {
      exec_node_init(&node->node);
   node->parent = NULL;
      }
      nir_function_impl *
   nir_function_impl_create_bare(nir_shader *shader)
   {
               impl->function = NULL;
                     exec_list_make_empty(&impl->body);
   exec_list_make_empty(&impl->locals);
   impl->ssa_alloc = 0;
   impl->num_blocks = 0;
   impl->valid_metadata = nir_metadata_none;
            /* create start & end blocks */
   nir_block *start_block = nir_block_create(shader);
   nir_block *end_block = nir_block_create(shader);
   start_block->cf_node.parent = &impl->cf_node;
   end_block->cf_node.parent = &impl->cf_node;
                     start_block->successors[0] = end_block;
   _mesa_set_add(end_block->predecessors, start_block);
      }
      nir_function_impl *
   nir_function_impl_create(nir_function *function)
   {
               nir_function_impl *impl = nir_function_impl_create_bare(function->shader);
   nir_function_set_impl(function, impl);
      }
      nir_block *
   nir_block_create(nir_shader *shader)
   {
                        block->successors[0] = block->successors[1] = NULL;
   block->predecessors = _mesa_pointer_set_create(block);
   block->imm_dom = NULL;
   /* XXX maybe it would be worth it to defer allocation?  This
   * way it doesn't get allocated for shader refs that never run
   * nir_calc_dominance?  For example, state-tracker creates an
   * initial IR, clones that, runs appropriate lowering pass, passes
   * to driver which does common lowering/opt, and then stores ref
   * which is later used to do state specific lowering and futher
   * opt.  Do any of the references not need dominance metadata?
   */
                        }
      static inline void
   src_init(nir_src *src)
   {
         }
      nir_if *
   nir_if_create(nir_shader *shader)
   {
                        cf_init(&if_stmt->cf_node, nir_cf_node_if);
            nir_block *then = nir_block_create(shader);
   exec_list_make_empty(&if_stmt->then_list);
   exec_list_push_tail(&if_stmt->then_list, &then->cf_node.node);
            nir_block *else_stmt = nir_block_create(shader);
   exec_list_make_empty(&if_stmt->else_list);
   exec_list_push_tail(&if_stmt->else_list, &else_stmt->cf_node.node);
               }
      nir_loop *
   nir_loop_create(nir_shader *shader)
   {
               cf_init(&loop->cf_node, nir_cf_node_loop);
   /* Assume that loops are divergent until proven otherwise */
            nir_block *body = nir_block_create(shader);
   exec_list_make_empty(&loop->body);
   exec_list_push_tail(&loop->body, &body->cf_node.node);
            body->successors[0] = body;
                        }
      static void
   instr_init(nir_instr *instr, nir_instr_type type)
   {
      instr->type = type;
   instr->block = NULL;
      }
      static void
   alu_src_init(nir_alu_src *src)
   {
      src_init(&src->src);
   for (int i = 0; i < NIR_MAX_VEC_COMPONENTS; ++i)
      }
      nir_alu_instr *
   nir_alu_instr_create(nir_shader *shader, nir_op op)
   {
      unsigned num_srcs = nir_op_infos[op].num_inputs;
            instr_init(&instr->instr, nir_instr_type_alu);
   instr->op = op;
   for (unsigned i = 0; i < num_srcs; i++)
               }
      nir_deref_instr *
   nir_deref_instr_create(nir_shader *shader, nir_deref_type deref_type)
   {
                        instr->deref_type = deref_type;
   if (deref_type != nir_deref_type_var)
            if (deref_type == nir_deref_type_array ||
      deref_type == nir_deref_type_ptr_as_array)
            }
      nir_jump_instr *
   nir_jump_instr_create(nir_shader *shader, nir_jump_type type)
   {
      nir_jump_instr *instr = gc_alloc(shader->gctx, nir_jump_instr, 1);
   instr_init(&instr->instr, nir_instr_type_jump);
   src_init(&instr->condition);
   instr->type = type;
   instr->target = NULL;
               }
      nir_load_const_instr *
   nir_load_const_instr_create(nir_shader *shader, unsigned num_components,
         {
      nir_load_const_instr *instr =
                              }
      nir_intrinsic_instr *
   nir_intrinsic_instr_create(nir_shader *shader, nir_intrinsic_op op)
   {
      unsigned num_srcs = nir_intrinsic_infos[op].num_srcs;
   nir_intrinsic_instr *instr =
            instr_init(&instr->instr, nir_instr_type_intrinsic);
            for (unsigned i = 0; i < num_srcs; i++)
               }
      nir_call_instr *
   nir_call_instr_create(nir_shader *shader, nir_function *callee)
   {
      const unsigned num_params = callee->num_params;
   nir_call_instr *instr =
            instr_init(&instr->instr, nir_instr_type_call);
   instr->callee = callee;
   instr->num_params = num_params;
   for (unsigned i = 0; i < num_params; i++)
               }
      static int8_t default_tg4_offsets[4][2] = {
      { 0, 1 },
   { 1, 1 },
   { 1, 0 },
      };
      nir_tex_instr *
   nir_tex_instr_create(nir_shader *shader, unsigned num_srcs)
   {
      nir_tex_instr *instr = gc_zalloc(shader->gctx, nir_tex_instr, 1);
            instr->num_srcs = num_srcs;
   instr->src = gc_alloc(shader->gctx, nir_tex_src, num_srcs);
   for (unsigned i = 0; i < num_srcs; i++)
            instr->texture_index = 0;
   instr->sampler_index = 0;
               }
      void
   nir_tex_instr_add_src(nir_tex_instr *tex,
               {
               for (unsigned i = 0; i < tex->num_srcs; i++) {
      new_srcs[i].src_type = tex->src[i].src_type;
   nir_instr_move_src(&tex->instr, &new_srcs[i].src,
               gc_free(tex->src);
            tex->src[tex->num_srcs].src_type = src_type;
   nir_instr_init_src(&tex->instr, &tex->src[tex->num_srcs].src, src);
      }
      void
   nir_tex_instr_remove_src(nir_tex_instr *tex, unsigned src_idx)
   {
               /* First rewrite the source to NIR_SRC_INIT */
            /* Now, move all of the other sources down */
   for (unsigned i = src_idx + 1; i < tex->num_srcs; i++) {
      tex->src[i - 1].src_type = tex->src[i].src_type;
      }
      }
      bool
   nir_tex_instr_has_explicit_tg4_offsets(nir_tex_instr *tex)
   {
      if (tex->op != nir_texop_tg4)
         return memcmp(tex->tg4_offsets, default_tg4_offsets,
      }
      nir_phi_instr *
   nir_phi_instr_create(nir_shader *shader)
   {
      nir_phi_instr *instr = gc_alloc(shader->gctx, nir_phi_instr, 1);
                        }
      /**
   * Adds a new source to a NIR instruction.
   *
   * Note that this does not update the def/use relationship for src, assuming
   * that the instr is not in the shader.  If it is, you have to do:
   *
   * list_addtail(&phi_src->src.use_link, &src.ssa->uses);
   */
   nir_phi_src *
   nir_phi_instr_add_src(nir_phi_instr *instr, nir_block *pred, nir_def *src)
   {
               phi_src = gc_zalloc(gc_get_context(instr), nir_phi_src, 1);
   phi_src->pred = pred;
   phi_src->src = nir_src_for_ssa(src);
   nir_src_set_parent_instr(&phi_src->src, &instr->instr);
               }
      nir_parallel_copy_instr *
   nir_parallel_copy_instr_create(nir_shader *shader)
   {
      nir_parallel_copy_instr *instr = gc_alloc(shader->gctx, nir_parallel_copy_instr, 1);
                        }
      nir_undef_instr *
   nir_undef_instr_create(nir_shader *shader,
               {
      nir_undef_instr *instr = gc_alloc(shader->gctx, nir_undef_instr, 1);
                        }
      static nir_const_value
   const_value_float(double d, unsigned bit_size)
   {
      nir_const_value v;
            /* clang-format off */
   switch (bit_size) {
   case 16: v.u16 = _mesa_float_to_half(d);  break;
   case 32: v.f32 = d;                       break;
   case 64: v.f64 = d;                       break;
   default:
         }
               }
      static nir_const_value
   const_value_int(int64_t i, unsigned bit_size)
   {
      nir_const_value v;
            /* clang-format off */
   switch (bit_size) {
   case 1:  v.b   = i & 1; break;
   case 8:  v.i8  = i;     break;
   case 16: v.i16 = i;     break;
   case 32: v.i32 = i;     break;
   case 64: v.i64 = i;     break;
   default:
         }
               }
      nir_const_value
   nir_alu_binop_identity(nir_op binop, unsigned bit_size)
   {
      const int64_t max_int = (1ull << (bit_size - 1)) - 1;
   const int64_t min_int = -max_int - 1;
   switch (binop) {
   case nir_op_iadd:
         case nir_op_fadd:
         case nir_op_imul:
         case nir_op_fmul:
         case nir_op_imin:
         case nir_op_umin:
         case nir_op_fmin:
         case nir_op_imax:
         case nir_op_umax:
         case nir_op_fmax:
         case nir_op_iand:
         case nir_op_ior:
         case nir_op_ixor:
         default:
            }
      nir_function_impl *
   nir_cf_node_get_function(nir_cf_node *node)
   {
      while (node->type != nir_cf_node_function) {
                     }
      /* Reduces a cursor by trying to convert everything to after and trying to
   * go up to block granularity when possible.
   */
   static nir_cursor
   reduce_cursor(nir_cursor cursor)
   {
      switch (cursor.option) {
   case nir_cursor_before_block:
      if (exec_list_is_empty(&cursor.block->instr_list)) {
      /* Empty block.  After is as good as before. */
      }
         case nir_cursor_after_block:
            case nir_cursor_before_instr: {
      nir_instr *prev_instr = nir_instr_prev(cursor.instr);
   if (prev_instr) {
      /* Before this instruction is after the previous */
   cursor.instr = prev_instr;
      } else {
      /* No previous instruction.  Switch to before block */
   cursor.block = cursor.instr->block;
      }
               case nir_cursor_after_instr:
      if (nir_instr_next(cursor.instr) == NULL) {
      /* This is the last instruction, switch to after block */
   cursor.option = nir_cursor_after_block;
      }
         default:
            }
      bool
   nir_cursors_equal(nir_cursor a, nir_cursor b)
   {
      /* Reduced cursors should be unique */
   a = reduce_cursor(a);
               }
      static bool
   add_use_cb(nir_src *src, void *state)
   {
               nir_src_set_parent_instr(src, instr);
               }
      static bool
   add_ssa_def_cb(nir_def *def, void *state)
   {
               if (instr->block && def->index == UINT_MAX) {
      nir_function_impl *impl =
                                    }
      static void
   add_defs_uses(nir_instr *instr)
   {
      nir_foreach_src(instr, add_use_cb, instr);
      }
      void
   nir_instr_insert(nir_cursor cursor, nir_instr *instr)
   {
      switch (cursor.option) {
   case nir_cursor_before_block:
      /* Only allow inserting jumps into empty blocks. */
   if (instr->type == nir_instr_type_jump)
            instr->block = cursor.block;
   add_defs_uses(instr);
   exec_list_push_head(&cursor.block->instr_list, &instr->node);
      case nir_cursor_after_block: {
      /* Inserting instructions after a jump is illegal. */
   nir_instr *last = nir_block_last_instr(cursor.block);
   assert(last == NULL || last->type != nir_instr_type_jump);
            instr->block = cursor.block;
   add_defs_uses(instr);
   exec_list_push_tail(&cursor.block->instr_list, &instr->node);
      }
   case nir_cursor_before_instr:
      assert(instr->type != nir_instr_type_jump);
   instr->block = cursor.instr->block;
   add_defs_uses(instr);
   exec_node_insert_node_before(&cursor.instr->node, &instr->node);
      case nir_cursor_after_instr:
      /* Inserting instructions after a jump is illegal. */
            /* Only allow inserting jumps at the end of the block. */
   if (instr->type == nir_instr_type_jump)
            instr->block = cursor.instr->block;
   add_defs_uses(instr);
   exec_node_insert_after(&cursor.instr->node, &instr->node);
               if (instr->type == nir_instr_type_jump)
            nir_function_impl *impl = nir_cf_node_get_function(&instr->block->cf_node);
      }
      bool
   nir_instr_move(nir_cursor cursor, nir_instr *instr)
   {
      /* If the cursor happens to refer to this instruction (either before or
   * after), don't do anything.
   */
   if ((cursor.option == nir_cursor_before_instr ||
      cursor.option == nir_cursor_after_instr) &&
   cursor.instr == instr)
         nir_instr_remove(instr);
   nir_instr_insert(cursor, instr);
      }
      static bool
   src_is_valid(const nir_src *src)
   {
         }
      static bool
   remove_use_cb(nir_src *src, void *state)
   {
               if (src_is_valid(src))
               }
      static void
   remove_defs_uses(nir_instr *instr)
   {
         }
      void
   nir_instr_remove_v(nir_instr *instr)
   {
      remove_defs_uses(instr);
            if (instr->type == nir_instr_type_jump) {
      nir_jump_instr *jump_instr = nir_instr_as_jump(instr);
         }
      void
   nir_instr_free(nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_tex:
      gc_free(nir_instr_as_tex(instr)->src);
         case nir_instr_type_phi: {
      nir_phi_instr *phi = nir_instr_as_phi(instr);
   nir_foreach_phi_src_safe(phi_src, phi)
                     default:
                     }
      void
   nir_instr_free_list(struct exec_list *list)
   {
      struct exec_node *node;
   while ((node = exec_list_pop_head(list))) {
      nir_instr *removed_instr = exec_node_data(nir_instr, node, node);
         }
      static bool
   nir_instr_free_and_dce_live_cb(nir_def *def, void *state)
   {
               if (!nir_def_is_unused(def)) {
      *live = true;
      } else {
            }
      static bool
   nir_instr_free_and_dce_is_live(nir_instr *instr)
   {
      /* Note: don't have to worry about jumps because they don't have dests to
   * become unused.
   */
   if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   const nir_intrinsic_info *info = &nir_intrinsic_infos[intr->intrinsic];
   if (!(info->flags & NIR_INTRINSIC_CAN_ELIMINATE))
               bool live = false;
   nir_foreach_def(instr, nir_instr_free_and_dce_live_cb, &live);
      }
      static bool
   nir_instr_dce_add_dead_srcs_cb(nir_src *src, void *state)
   {
               list_del(&src->use_link);
   if (!nir_instr_free_and_dce_is_live(src->ssa->parent_instr))
            /* Stop nir_instr_remove from trying to delete the link again. */
               }
      static void
   nir_instr_dce_add_dead_ssa_srcs(nir_instr_worklist *wl, nir_instr *instr)
   {
         }
      /**
   * Frees an instruction and any SSA defs that it used that are now dead,
   * returning a nir_cursor where the instruction previously was.
   */
   nir_cursor
   nir_instr_free_and_dce(nir_instr *instr)
   {
               nir_instr_dce_add_dead_ssa_srcs(worklist, instr);
            struct exec_list to_free;
            nir_instr *dce_instr;
   while ((dce_instr = nir_instr_worklist_pop_head(worklist))) {
               /* If we're removing the instr where our cursor is, then we have to
   * point the cursor elsewhere.
   */
   if ((c.option == nir_cursor_before_instr ||
      c.option == nir_cursor_after_instr) &&
   c.instr == dce_instr)
      else
                                          }
      /*@}*/
      nir_def *
   nir_instr_def(nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_alu:
            case nir_instr_type_deref:
            case nir_instr_type_tex:
            case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (nir_intrinsic_infos[intrin->intrinsic].has_dest) {
         } else {
                     case nir_instr_type_phi:
            case nir_instr_type_parallel_copy:
            case nir_instr_type_load_const:
            case nir_instr_type_undef:
            case nir_instr_type_call:
   case nir_instr_type_jump:
                     }
      bool
   nir_foreach_phi_src_leaving_block(nir_block *block,
               {
      for (unsigned i = 0; i < ARRAY_SIZE(block->successors); i++) {
      if (block->successors[i] == NULL)
            nir_foreach_phi(phi, block->successors[i]) {
      nir_foreach_phi_src(phi_src, phi) {
      if (phi_src->pred == block) {
      if (!cb(&phi_src->src, state))
                           }
      nir_const_value
   nir_const_value_for_float(double f, unsigned bit_size)
   {
      nir_const_value v;
            /* clang-format off */
   switch (bit_size) {
   case 16: v.u16 = _mesa_float_to_half(f);  break;
   case 32: v.f32 = f;                       break;
   case 64: v.f64 = f;                       break;
   default: unreachable("Invalid bit size");
   }
               }
      double
   nir_const_value_as_float(nir_const_value value, unsigned bit_size)
   {
      /* clang-format off */
   switch (bit_size) {
   case 16: return _mesa_half_to_float(value.u16);
   case 32: return value.f32;
   case 64: return value.f64;
   default: unreachable("Invalid bit size");
   }
      }
      nir_const_value *
   nir_src_as_const_value(nir_src src)
   {
      if (src.ssa->parent_instr->type != nir_instr_type_load_const)
                        }
      /**
   * Returns true if the source is known to be always uniform. Otherwise it
   * returns false which means it may or may not be uniform but it can't be
   * determined.
   *
   * For a more precise analysis of uniform values, use nir_divergence_analysis.
   */
   bool
   nir_src_is_always_uniform(nir_src src)
   {
      /* Constants are trivially uniform */
   if (src.ssa->parent_instr->type == nir_instr_type_load_const)
            if (src.ssa->parent_instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(src.ssa->parent_instr);
   /* As are uniform variables */
   if (intr->intrinsic == nir_intrinsic_load_uniform &&
      nir_src_is_always_uniform(intr->src[0]))
      /* From the Vulkan specification 15.6.1. Push Constant Interface:
   * "Any member of a push constant block that is declared as an array must
   * only be accessed with dynamically uniform indices."
   */
   if (intr->intrinsic == nir_intrinsic_load_push_constant)
         if (intr->intrinsic == nir_intrinsic_load_deref &&
      nir_deref_mode_is(nir_src_as_deref(intr->src[0]), nir_var_mem_push_const))
            /* Operating together uniform expressions produces a uniform result */
   if (src.ssa->parent_instr->type == nir_instr_type_alu) {
      nir_alu_instr *alu = nir_instr_as_alu(src.ssa->parent_instr);
   for (int i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      if (!nir_src_is_always_uniform(alu->src[i].src))
                           /* XXX: this could have many more tests, such as when a sampler function is
   * called with uniform arguments.
   */
      }
      static void
   src_remove_all_uses(nir_src *src)
   {
      if (src && src_is_valid(src))
      }
      static void
   src_add_all_uses(nir_src *src, nir_instr *parent_instr, nir_if *parent_if)
   {
      if (!src)
            if (!src_is_valid(src))
            if (parent_instr) {
         } else {
      assert(parent_if);
                  }
      void
   nir_instr_init_src(nir_instr *instr, nir_src *src, nir_def *def)
   {
      *src = nir_src_for_ssa(def);
      }
      void
   nir_instr_clear_src(nir_instr *instr, nir_src *src)
   {
      src_remove_all_uses(src);
      }
      void
   nir_instr_move_src(nir_instr *dest_instr, nir_src *dest, nir_src *src)
   {
               src_remove_all_uses(dest);
   src_remove_all_uses(src);
   *dest = *src;
   *src = NIR_SRC_INIT;
      }
      void
   nir_def_init(nir_instr *instr, nir_def *def,
               {
      def->parent_instr = instr;
   list_inithead(&def->uses);
   def->num_components = num_components;
   def->bit_size = bit_size;
            if (instr->block) {
      nir_function_impl *impl =
                        } else {
            }
      void
   nir_def_rewrite_uses(nir_def *def, nir_def *new_ssa)
   {
      assert(def != new_ssa);
   nir_foreach_use_including_if_safe(use_src, def) {
            }
      void
   nir_def_rewrite_uses_src(nir_def *def, nir_src new_src)
   {
         }
      static bool
   is_instr_between(nir_instr *start, nir_instr *end, nir_instr *between)
   {
               if (between->block != start->block)
            /* Search backwards looking for "between" */
   while (start != end) {
      if (between == end)
            end = nir_instr_prev(end);
                  }
      /* Replaces all uses of the given SSA def with the given source but only if
   * the use comes after the after_me instruction.  This can be useful if you
   * are emitting code to fix up the result of some instruction: you can freely
   * use the result in that code and then call rewrite_uses_after and pass the
   * last fixup instruction as after_me and it will replace all of the uses you
   * want without touching the fixup code.
   *
   * This function assumes that after_me is in the same block as
   * def->parent_instr and that after_me comes after def->parent_instr.
   */
   void
   nir_def_rewrite_uses_after(nir_def *def, nir_def *new_ssa,
         {
      if (def == new_ssa)
            nir_foreach_use_including_if_safe(use_src, def) {
      if (!nir_src_is_if(use_src)) {
               /* Since def already dominates all of its uses, the only way a use can
   * not be dominated by after_me is if it is between def and after_me in
   * the instruction list.
   */
   if (is_instr_between(def->parent_instr, after_me, nir_src_parent_instr(use_src)))
                     }
      static nir_def *
   get_store_value(nir_intrinsic_instr *intrin)
   {
      assert(nir_intrinsic_has_write_mask(intrin));
   /* deref stores have the deref in src[0] and the store value in src[1] */
   if (intrin->intrinsic == nir_intrinsic_store_deref ||
      intrin->intrinsic == nir_intrinsic_store_deref_block_intel)
         /* all other stores have the store value in src[0] */
      }
      nir_component_mask_t
   nir_src_components_read(const nir_src *src)
   {
               if (nir_src_parent_instr(src)->type == nir_instr_type_alu) {
      nir_alu_instr *alu = nir_instr_as_alu(nir_src_parent_instr(src));
   nir_alu_src *alu_src = exec_node_data(nir_alu_src, src, src);
   int src_idx = alu_src - &alu->src[0];
   assert(src_idx >= 0 && src_idx < nir_op_infos[alu->op].num_inputs);
      } else if (nir_src_parent_instr(src)->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(nir_src_parent_instr(src));
   if (nir_intrinsic_has_write_mask(intrin) && src->ssa == get_store_value(intrin))
         else
      } else {
            }
      nir_component_mask_t
   nir_def_components_read(const nir_def *def)
   {
               nir_foreach_use_including_if(use, def) {
               if (read_mask == (1 << def->num_components) - 1)
                  }
      bool
   nir_def_all_uses_are_fsat(const nir_def *def)
   {
      nir_foreach_use(src, def) {
      if (nir_src_is_if(src))
            nir_instr *use = nir_src_parent_instr(src);
   if (use->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(use);
   if (alu->op != nir_op_fsat)
                  }
      nir_block *
   nir_block_unstructured_next(nir_block *block)
   {
      if (block == NULL) {
      /* nir_foreach_block_unstructured_safe() will call this function on a
   * NULL block after the last iteration, but it won't use the result so
   * just return NULL here.
   */
               nir_cf_node *cf_next = nir_cf_node_next(&block->cf_node);
   if (cf_next == NULL && block->cf_node.parent->type == nir_cf_node_function)
            if (cf_next && cf_next->type == nir_cf_node_block)
               }
      nir_block *
   nir_unstructured_start_block(nir_function_impl *impl)
   {
         }
      nir_block *
   nir_block_cf_tree_next(nir_block *block)
   {
      if (block == NULL) {
      /* nir_foreach_block_safe() will call this function on a NULL block
   * after the last iteration, but it won't use the result so just return
   * NULL here.
   */
                        nir_cf_node *cf_next = nir_cf_node_next(&block->cf_node);
   if (cf_next)
            nir_cf_node *parent = block->cf_node.parent;
   if (parent->type == nir_cf_node_function)
            /* Is this the last block of a cf_node? Return the following block */
   if (block == nir_cf_node_cf_tree_last(parent))
            switch (parent->type) {
   case nir_cf_node_if: {
      /* We are at the end of the if. Go to the beginning of the else */
   nir_if *if_stmt = nir_cf_node_as_if(parent);
   assert(block == nir_if_last_then_block(if_stmt));
               case nir_cf_node_loop: {
      /* We are at the end of the body and there is a continue construct */
   nir_loop *loop = nir_cf_node_as_loop(parent);
   assert(block == nir_loop_last_block(loop) &&
                     default:
            }
      nir_block *
   nir_block_cf_tree_prev(nir_block *block)
   {
      if (block == NULL) {
      /* do this for consistency with nir_block_cf_tree_next() */
                        nir_cf_node *cf_prev = nir_cf_node_prev(&block->cf_node);
   if (cf_prev)
            nir_cf_node *parent = block->cf_node.parent;
   if (parent->type == nir_cf_node_function)
            /* Is this the first block of a cf_node? Return the previous block */
   if (block == nir_cf_node_cf_tree_first(parent))
            switch (parent->type) {
   case nir_cf_node_if: {
      /* We are at the beginning of the else. Go to the end of the if */
   nir_if *if_stmt = nir_cf_node_as_if(parent);
   assert(block == nir_if_first_else_block(if_stmt));
      }
   case nir_cf_node_loop: {
      /* We are at the beginning of the continue construct. */
   nir_loop *loop = nir_cf_node_as_loop(parent);
   assert(nir_loop_has_continue_construct(loop) &&
                     default:
            }
      nir_block *
   nir_cf_node_cf_tree_first(nir_cf_node *node)
   {
      switch (node->type) {
   case nir_cf_node_function: {
      nir_function_impl *impl = nir_cf_node_as_function(node);
               case nir_cf_node_if: {
      nir_if *if_stmt = nir_cf_node_as_if(node);
               case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(node);
               case nir_cf_node_block: {
                  default:
            }
      nir_block *
   nir_cf_node_cf_tree_last(nir_cf_node *node)
   {
      switch (node->type) {
   case nir_cf_node_function: {
      nir_function_impl *impl = nir_cf_node_as_function(node);
               case nir_cf_node_if: {
      nir_if *if_stmt = nir_cf_node_as_if(node);
               case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(node);
   if (nir_loop_has_continue_construct(loop))
         else
               case nir_cf_node_block: {
                  default:
            }
      nir_block *
   nir_cf_node_cf_tree_next(nir_cf_node *node)
   {
      if (node->type == nir_cf_node_block)
         else if (node->type == nir_cf_node_function)
         else
      }
      nir_block *
   nir_cf_node_cf_tree_prev(nir_cf_node *node)
   {
      if (node->type == nir_cf_node_block)
         else if (node->type == nir_cf_node_function)
         else
      }
      nir_if *
   nir_block_get_following_if(nir_block *block)
   {
      if (exec_node_is_tail_sentinel(&block->cf_node.node))
            if (nir_cf_node_is_last(&block->cf_node))
                     if (next_node->type != nir_cf_node_if)
               }
      nir_loop *
   nir_block_get_following_loop(nir_block *block)
   {
      if (exec_node_is_tail_sentinel(&block->cf_node.node))
            if (nir_cf_node_is_last(&block->cf_node))
                     if (next_node->type != nir_cf_node_loop)
               }
      static int
   compare_block_index(const void *p1, const void *p2)
   {
      const nir_block *block1 = *((const nir_block **)p1);
               }
      nir_block **
   nir_block_get_predecessors_sorted(const nir_block *block, void *mem_ctx)
   {
      nir_block **preds =
            unsigned i = 0;
   set_foreach(block->predecessors, entry)
                  qsort(preds, block->predecessors->entries, sizeof(nir_block *),
               }
      void
   nir_index_blocks(nir_function_impl *impl)
   {
               if (impl->valid_metadata & nir_metadata_block_index)
            nir_foreach_block_unstructured(block, impl) {
                  /* The end_block isn't really part of the program, which is why its index
   * is >= num_blocks.
   */
      }
      static bool
   index_ssa_def_cb(nir_def *def, void *state)
   {
      unsigned *index = (unsigned *)state;
               }
      /**
   * The indices are applied top-to-bottom which has the very nice property
   * that, if A dominates B, then A->index <= B->index.
   */
   void
   nir_index_ssa_defs(nir_function_impl *impl)
   {
                        nir_foreach_block_unstructured(block, impl) {
      nir_foreach_instr(instr, block)
                  }
      /**
   * The indices are applied top-to-bottom which has the very nice property
   * that, if A dominates B, then A->index <= B->index.
   */
   unsigned
   nir_index_instrs(nir_function_impl *impl)
   {
               nir_foreach_block(block, impl) {
               nir_foreach_instr(instr, block)
                           }
      void
   nir_shader_clear_pass_flags(nir_shader *shader)
   {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                  }
      unsigned
   nir_shader_index_vars(nir_shader *shader, nir_variable_mode modes)
   {
      unsigned count = 0;
   nir_foreach_variable_with_modes(var, shader, modes)
            }
      unsigned
   nir_function_impl_index_vars(nir_function_impl *impl)
   {
      unsigned count = 0;
   nir_foreach_function_temp_variable(var, impl)
            }
      static nir_instr *
   cursor_next_instr(nir_cursor cursor)
   {
      switch (cursor.option) {
   case nir_cursor_before_block:
      for (nir_block *block = cursor.block; block;
      block = nir_block_cf_tree_next(block)) {
   nir_instr *instr = nir_block_first_instr(block);
   if (instr)
      }
         case nir_cursor_after_block:
      cursor.block = nir_block_cf_tree_next(cursor.block);
   if (cursor.block == NULL)
            cursor.option = nir_cursor_before_block;
         case nir_cursor_before_instr:
            case nir_cursor_after_instr:
      if (nir_instr_next(cursor.instr))
            cursor.option = nir_cursor_after_block;
   cursor.block = cursor.instr->block;
                  }
      bool
   nir_function_impl_lower_instructions(nir_function_impl *impl,
                     {
               nir_metadata preserved = nir_metadata_block_index |
            bool progress = false;
   nir_cursor iter = nir_before_impl(impl);
   nir_instr *instr;
   while ((instr = cursor_next_instr(iter)) != NULL) {
      if (filter && !filter(instr, cb_data)) {
      iter = nir_after_instr(instr);
               nir_def *old_def = nir_instr_def(instr);
   struct list_head old_uses;
   if (old_def != NULL) {
      /* We're about to ask the callback to generate a replacement for instr.
   * Save off the uses from instr's SSA def so we know what uses to
   * rewrite later.  If we use nir_def_rewrite_uses, it fails in the
   * case where the generated replacement code uses the result of instr
   * itself.  If we use nir_def_rewrite_uses_after (which is the
   * normal solution to this problem), it doesn't work well if control-
   * flow is inserted as part of the replacement, doesn't handle cases
   * where the replacement is something consumed by instr, and suffers
   * from performance issues.  This is the only way to 100% guarantee
                  list_replace(&old_def->uses, &old_uses);
               b.cursor = nir_after_instr(instr);
   nir_def *new_def = lower(&b, instr, cb_data);
   if (new_def && new_def != NIR_LOWER_INSTR_PROGRESS &&
      new_def != NIR_LOWER_INSTR_PROGRESS_REPLACE) {
   assert(old_def != NULL);
                                 if (nir_def_is_unused(old_def)) {
         } else {
         }
      } else {
      /* We didn't end up lowering after all.  Put the uses back */
                  if (new_def == NIR_LOWER_INSTR_PROGRESS_REPLACE) {
      /* Only instructions without a return value can be removed like this */
   assert(!old_def);
   iter = nir_instr_free_and_dce(instr);
                     if (new_def == NIR_LOWER_INSTR_PROGRESS)
                  if (progress) {
         } else {
                     }
      bool
   nir_shader_lower_instructions(nir_shader *shader,
                     {
               nir_foreach_function_impl(impl, shader) {
      if (nir_function_impl_lower_instructions(impl, filter, lower, cb_data))
                  }
      /**
   * Returns true if the shader supports quad-based implicit derivatives on
   * texture sampling.
   */
   bool
   nir_shader_supports_implicit_lod(nir_shader *shader)
   {
      return (shader->info.stage == MESA_SHADER_FRAGMENT ||
            }
      nir_intrinsic_op
   nir_intrinsic_from_system_value(gl_system_value val)
   {
      switch (val) {
   case SYSTEM_VALUE_VERTEX_ID:
         case SYSTEM_VALUE_INSTANCE_ID:
         case SYSTEM_VALUE_DRAW_ID:
         case SYSTEM_VALUE_BASE_INSTANCE:
         case SYSTEM_VALUE_VERTEX_ID_ZERO_BASE:
         case SYSTEM_VALUE_IS_INDEXED_DRAW:
         case SYSTEM_VALUE_FIRST_VERTEX:
         case SYSTEM_VALUE_BASE_VERTEX:
         case SYSTEM_VALUE_INVOCATION_ID:
         case SYSTEM_VALUE_FRAG_COORD:
         case SYSTEM_VALUE_POINT_COORD:
         case SYSTEM_VALUE_LINE_COORD:
         case SYSTEM_VALUE_FRONT_FACE:
         case SYSTEM_VALUE_SAMPLE_ID:
         case SYSTEM_VALUE_SAMPLE_POS:
         case SYSTEM_VALUE_SAMPLE_POS_OR_CENTER:
         case SYSTEM_VALUE_SAMPLE_MASK_IN:
         case SYSTEM_VALUE_LOCAL_INVOCATION_ID:
         case SYSTEM_VALUE_LOCAL_INVOCATION_INDEX:
         case SYSTEM_VALUE_WORKGROUP_ID:
         case SYSTEM_VALUE_WORKGROUP_INDEX:
         case SYSTEM_VALUE_NUM_WORKGROUPS:
         case SYSTEM_VALUE_PRIMITIVE_ID:
         case SYSTEM_VALUE_TESS_COORD:
         case SYSTEM_VALUE_TESS_LEVEL_OUTER:
         case SYSTEM_VALUE_TESS_LEVEL_INNER:
         case SYSTEM_VALUE_TESS_LEVEL_OUTER_DEFAULT:
         case SYSTEM_VALUE_TESS_LEVEL_INNER_DEFAULT:
         case SYSTEM_VALUE_VERTICES_IN:
         case SYSTEM_VALUE_HELPER_INVOCATION:
         case SYSTEM_VALUE_COLOR0:
         case SYSTEM_VALUE_COLOR1:
         case SYSTEM_VALUE_VIEW_INDEX:
         case SYSTEM_VALUE_SUBGROUP_SIZE:
         case SYSTEM_VALUE_SUBGROUP_INVOCATION:
         case SYSTEM_VALUE_SUBGROUP_EQ_MASK:
         case SYSTEM_VALUE_SUBGROUP_GE_MASK:
         case SYSTEM_VALUE_SUBGROUP_GT_MASK:
         case SYSTEM_VALUE_SUBGROUP_LE_MASK:
         case SYSTEM_VALUE_SUBGROUP_LT_MASK:
         case SYSTEM_VALUE_NUM_SUBGROUPS:
         case SYSTEM_VALUE_SUBGROUP_ID:
         case SYSTEM_VALUE_WORKGROUP_SIZE:
         case SYSTEM_VALUE_GLOBAL_INVOCATION_ID:
         case SYSTEM_VALUE_BASE_GLOBAL_INVOCATION_ID:
         case SYSTEM_VALUE_GLOBAL_INVOCATION_INDEX:
         case SYSTEM_VALUE_WORK_DIM:
         case SYSTEM_VALUE_USER_DATA_AMD:
         case SYSTEM_VALUE_RAY_LAUNCH_ID:
         case SYSTEM_VALUE_RAY_LAUNCH_SIZE:
         case SYSTEM_VALUE_RAY_LAUNCH_SIZE_ADDR_AMD:
         case SYSTEM_VALUE_RAY_WORLD_ORIGIN:
         case SYSTEM_VALUE_RAY_WORLD_DIRECTION:
         case SYSTEM_VALUE_RAY_OBJECT_ORIGIN:
         case SYSTEM_VALUE_RAY_OBJECT_DIRECTION:
         case SYSTEM_VALUE_RAY_T_MIN:
         case SYSTEM_VALUE_RAY_T_MAX:
         case SYSTEM_VALUE_RAY_OBJECT_TO_WORLD:
         case SYSTEM_VALUE_RAY_WORLD_TO_OBJECT:
         case SYSTEM_VALUE_RAY_HIT_KIND:
         case SYSTEM_VALUE_RAY_FLAGS:
         case SYSTEM_VALUE_RAY_GEOMETRY_INDEX:
         case SYSTEM_VALUE_RAY_INSTANCE_CUSTOM_INDEX:
         case SYSTEM_VALUE_CULL_MASK:
         case SYSTEM_VALUE_RAY_TRIANGLE_VERTEX_POSITIONS:
         case SYSTEM_VALUE_MESH_VIEW_COUNT:
         case SYSTEM_VALUE_FRAG_SHADING_RATE:
         case SYSTEM_VALUE_FULLY_COVERED:
         case SYSTEM_VALUE_FRAG_SIZE:
         case SYSTEM_VALUE_FRAG_INVOCATION_COUNT:
         case SYSTEM_VALUE_SHADER_INDEX:
         case SYSTEM_VALUE_COALESCED_INPUT_COUNT:
         default:
            }
      gl_system_value
   nir_system_value_from_intrinsic(nir_intrinsic_op intrin)
   {
      switch (intrin) {
   case nir_intrinsic_load_vertex_id:
         case nir_intrinsic_load_instance_id:
         case nir_intrinsic_load_draw_id:
         case nir_intrinsic_load_base_instance:
         case nir_intrinsic_load_vertex_id_zero_base:
         case nir_intrinsic_load_first_vertex:
         case nir_intrinsic_load_is_indexed_draw:
         case nir_intrinsic_load_base_vertex:
         case nir_intrinsic_load_invocation_id:
         case nir_intrinsic_load_frag_coord:
         case nir_intrinsic_load_point_coord:
         case nir_intrinsic_load_line_coord:
         case nir_intrinsic_load_front_face:
         case nir_intrinsic_load_sample_id:
         case nir_intrinsic_load_sample_pos:
         case nir_intrinsic_load_sample_pos_or_center:
         case nir_intrinsic_load_sample_mask_in:
         case nir_intrinsic_load_local_invocation_id:
         case nir_intrinsic_load_local_invocation_index:
         case nir_intrinsic_load_num_workgroups:
         case nir_intrinsic_load_workgroup_id:
         case nir_intrinsic_load_workgroup_index:
         case nir_intrinsic_load_primitive_id:
         case nir_intrinsic_load_tess_coord:
   case nir_intrinsic_load_tess_coord_xy:
         case nir_intrinsic_load_tess_level_outer:
         case nir_intrinsic_load_tess_level_inner:
         case nir_intrinsic_load_tess_level_outer_default:
         case nir_intrinsic_load_tess_level_inner_default:
         case nir_intrinsic_load_patch_vertices_in:
         case nir_intrinsic_load_helper_invocation:
         case nir_intrinsic_load_color0:
         case nir_intrinsic_load_color1:
         case nir_intrinsic_load_view_index:
         case nir_intrinsic_load_subgroup_size:
         case nir_intrinsic_load_subgroup_invocation:
         case nir_intrinsic_load_subgroup_eq_mask:
         case nir_intrinsic_load_subgroup_ge_mask:
         case nir_intrinsic_load_subgroup_gt_mask:
         case nir_intrinsic_load_subgroup_le_mask:
         case nir_intrinsic_load_subgroup_lt_mask:
         case nir_intrinsic_load_num_subgroups:
         case nir_intrinsic_load_subgroup_id:
         case nir_intrinsic_load_workgroup_size:
         case nir_intrinsic_load_global_invocation_id:
         case nir_intrinsic_load_base_global_invocation_id:
         case nir_intrinsic_load_global_invocation_index:
         case nir_intrinsic_load_work_dim:
         case nir_intrinsic_load_user_data_amd:
         case nir_intrinsic_load_barycentric_model:
         case nir_intrinsic_load_gs_header_ir3:
         case nir_intrinsic_load_tcs_header_ir3:
         case nir_intrinsic_load_ray_launch_id:
         case nir_intrinsic_load_ray_launch_size:
         case nir_intrinsic_load_ray_launch_size_addr_amd:
         case nir_intrinsic_load_ray_world_origin:
         case nir_intrinsic_load_ray_world_direction:
         case nir_intrinsic_load_ray_object_origin:
         case nir_intrinsic_load_ray_object_direction:
         case nir_intrinsic_load_ray_t_min:
         case nir_intrinsic_load_ray_t_max:
         case nir_intrinsic_load_ray_object_to_world:
         case nir_intrinsic_load_ray_world_to_object:
         case nir_intrinsic_load_ray_hit_kind:
         case nir_intrinsic_load_ray_flags:
         case nir_intrinsic_load_ray_geometry_index:
         case nir_intrinsic_load_ray_instance_custom_index:
         case nir_intrinsic_load_cull_mask:
         case nir_intrinsic_load_ray_triangle_vertex_positions:
         case nir_intrinsic_load_frag_shading_rate:
         case nir_intrinsic_load_mesh_view_count:
         case nir_intrinsic_load_fully_covered:
         case nir_intrinsic_load_frag_size:
         case nir_intrinsic_load_frag_invocation_count:
         case nir_intrinsic_load_shader_index:
         case nir_intrinsic_load_coalesced_input_count:
         default:
            }
      /* OpenGL utility method that remaps the location attributes if they are
   * doubles. Not needed for vulkan due the differences on the input location
   * count for doubles on vulkan vs OpenGL
   *
   * The bitfield returned in dual_slot is one bit for each double input slot in
   * the original OpenGL single-slot input numbering.  The mapping from old
   * locations to new locations is as follows:
   *
   *    new_loc = loc + util_bitcount(dual_slot & BITFIELD64_MASK(loc))
   */
   void
   nir_remap_dual_slot_attributes(nir_shader *shader, uint64_t *dual_slot)
   {
               *dual_slot = 0;
   nir_foreach_shader_in_variable(var, shader) {
      if (glsl_type_is_dual_slot(glsl_without_array(var->type))) {
      unsigned slots = glsl_count_attribute_slots(var->type, true);
                  nir_foreach_shader_in_variable(var, shader) {
      var->data.location +=
         }
      /* Returns an attribute mask that has been re-compacted using the given
   * dual_slot mask.
   */
   uint64_t
   nir_get_single_slot_attribs_mask(uint64_t attribs, uint64_t dual_slot)
   {
      while (dual_slot) {
      unsigned loc = u_bit_scan64(&dual_slot);
   /* mask of all bits up to and including loc */
   uint64_t mask = BITFIELD64_MASK(loc + 1);
      }
      }
      void
   nir_rewrite_image_intrinsic(nir_intrinsic_instr *intrin, nir_def *src,
         {
               /* Image intrinsics only have one of these */
   assert(!nir_intrinsic_has_src_type(intrin) ||
            nir_alu_type data_type = nir_type_invalid;
   if (nir_intrinsic_has_src_type(intrin))
         if (nir_intrinsic_has_dest_type(intrin))
            nir_atomic_op atomic_op = 0;
   if (nir_intrinsic_has_atomic_op(intrin))
               #define CASE(op)                                                       \
      case nir_intrinsic_image_deref_##op:                                \
      intrin->intrinsic = bindless ? nir_intrinsic_bindless_image_##op \
         break;
   CASE(load)
   CASE(sparse_load)
   CASE(store)
   CASE(atomic)
   CASE(atomic_swap)
   CASE(size)
   CASE(samples)
   CASE(load_raw_intel)
   CASE(store_raw_intel)
   #undef CASE
      default:
                           /* Only update the format if the intrinsic doesn't have one set */
   if (nir_intrinsic_format(intrin) == PIPE_FORMAT_NONE)
            nir_intrinsic_set_access(intrin, access | var->data.access);
   if (nir_intrinsic_has_src_type(intrin))
         if (nir_intrinsic_has_dest_type(intrin))
            if (nir_intrinsic_has_atomic_op(intrin))
               }
      unsigned
   nir_image_intrinsic_coord_components(const nir_intrinsic_instr *instr)
   {
      enum glsl_sampler_dim dim = nir_intrinsic_image_dim(instr);
   int coords = glsl_get_sampler_dim_coordinate_components(dim);
   if (dim == GLSL_SAMPLER_DIM_CUBE)
         else
      }
      nir_src *
   nir_get_shader_call_payload_src(nir_intrinsic_instr *call)
   {
      switch (call->intrinsic) {
   case nir_intrinsic_trace_ray:
   case nir_intrinsic_rt_trace_ray:
         case nir_intrinsic_execute_callable:
   case nir_intrinsic_rt_execute_callable:
         default:
      unreachable("Not a call intrinsic");
         }
      nir_binding
   nir_chase_binding(nir_src rsrc)
   {
      nir_binding res = { 0 };
   if (rsrc.ssa->parent_instr->type == nir_instr_type_deref) {
      const struct glsl_type *type = glsl_without_array(nir_src_as_deref(rsrc)->type);
   bool is_image = glsl_type_is_image(type) || glsl_type_is_sampler(type);
   while (rsrc.ssa->parent_instr->type == nir_instr_type_deref) {
               if (deref->deref_type == nir_deref_type_var) {
      res.success = true;
   res.var = deref->var;
   res.desc_set = deref->var->data.descriptor_set;
   res.binding = deref->var->data.binding;
      } else if (deref->deref_type == nir_deref_type_array && is_image) {
      if (res.num_indices == ARRAY_SIZE(res.indices))
                                    /* Skip copies and trimming. Trimming can appear as nir_op_mov instructions
   * when removing the offset from addresses. We also consider
   * nir_op_is_vec_or_mov() instructions to skip trimming of
   * vec2_index_32bit_offset addresses after lowering ALU to scalar.
   */
   unsigned num_components = nir_src_num_components(rsrc);
   while (true) {
      nir_alu_instr *alu = nir_src_as_alu_instr(rsrc);
   nir_intrinsic_instr *intrin = nir_src_as_intrinsic(rsrc);
   if (alu && alu->op == nir_op_mov) {
      for (unsigned i = 0; i < num_components; i++) {
      if (alu->src[0].swizzle[i] != i)
      }
      } else if (alu && nir_op_is_vec(alu->op)) {
      for (unsigned i = 0; i < num_components; i++) {
      if (alu->src[i].swizzle[0] != i || alu->src[i].src.ssa != alu->src[0].src.ssa)
      }
      } else if (intrin && intrin->intrinsic == nir_intrinsic_read_first_invocation) {
      /* The caller might want to be aware if only the first invocation of
   * the indices are used.
   */
   res.read_first_invocation = true;
      } else {
                     if (nir_src_is_const(rsrc)) {
      /* GL binding model after deref lowering */
   res.success = true;
   /* Can't use just nir_src_as_uint. Vulkan resource index produces a
   * vec2. Some drivers lower it to vec1 (to handle get_ssbo_size for
   * example) but others just keep it around as a vec2 (v3dv).
   */
   res.binding = nir_src_comp_as_uint(rsrc, 0);
                        nir_intrinsic_instr *intrin = nir_src_as_intrinsic(rsrc);
   if (!intrin)
            /* Intel resource, similar to load_vulkan_descriptor after it has been
   * lowered.
   */
   if (intrin->intrinsic == nir_intrinsic_resource_intel) {
      res.success = true;
   res.desc_set = nir_intrinsic_desc_set(intrin);
   res.binding = nir_intrinsic_binding(intrin);
   /* nir_intrinsic_resource_intel has 3 sources, but src[2] is included in
   * src[1], it is kept around for other purposes.
   */
   res.num_indices = 2;
   res.indices[0] = intrin->src[0];
   res.indices[1] = intrin->src[1];
               /* skip load_vulkan_descriptor */
   if (intrin->intrinsic == nir_intrinsic_load_vulkan_descriptor) {
      intrin = nir_src_as_intrinsic(intrin->src[0]);
   if (!intrin)
               if (intrin->intrinsic != nir_intrinsic_vulkan_resource_index)
            assert(res.num_indices == 0);
   res.success = true;
   res.desc_set = nir_intrinsic_desc_set(intrin);
   res.binding = nir_intrinsic_binding(intrin);
   res.num_indices = 1;
   res.indices[0] = intrin->src[0];
      }
      nir_variable *
   nir_get_binding_variable(nir_shader *shader, nir_binding binding)
   {
      nir_variable *binding_var = NULL;
            if (!binding.success)
            if (binding.var)
            nir_foreach_variable_with_modes(var, shader, nir_var_mem_ubo | nir_var_mem_ssbo) {
      if (var->data.descriptor_set == binding.desc_set && var->data.binding == binding.binding) {
      binding_var = var;
                  /* Be conservative if another variable is using the same binding/desc_set
   * because the access mask might be different and we can't get it reliably.
   */
   if (count > 1)
               }
      nir_scalar
   nir_scalar_chase_movs(nir_scalar s)
   {
      while (nir_scalar_is_alu(s)) {
      nir_alu_instr *alu = nir_instr_as_alu(s.def->parent_instr);
   if (alu->op == nir_op_mov) {
      s.def = alu->src[0].src.ssa;
      } else if (nir_op_is_vec(alu->op)) {
      s.def = alu->src[s.comp].src.ssa;
      } else {
                        }
      nir_alu_type
   nir_get_nir_type_for_glsl_base_type(enum glsl_base_type base_type)
   {
      switch (base_type) {
   /* clang-format off */
   case GLSL_TYPE_BOOL:    return nir_type_bool1;
   case GLSL_TYPE_UINT:    return nir_type_uint32;
   case GLSL_TYPE_INT:     return nir_type_int32;
   case GLSL_TYPE_UINT16:  return nir_type_uint16;
   case GLSL_TYPE_INT16:   return nir_type_int16;
   case GLSL_TYPE_UINT8:   return nir_type_uint8;
   case GLSL_TYPE_INT8:    return nir_type_int8;
   case GLSL_TYPE_UINT64:  return nir_type_uint64;
   case GLSL_TYPE_INT64:   return nir_type_int64;
   case GLSL_TYPE_FLOAT:   return nir_type_float32;
   case GLSL_TYPE_FLOAT16: return nir_type_float16;
   case GLSL_TYPE_DOUBLE:  return nir_type_float64;
            case GLSL_TYPE_COOPERATIVE_MATRIX:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_ATOMIC_UINT:
   case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE:
   case GLSL_TYPE_ARRAY:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_SUBROUTINE:
   case GLSL_TYPE_ERROR:
                     }
      enum glsl_base_type
   nir_get_glsl_base_type_for_nir_type(nir_alu_type base_type)
   {
      /* clang-format off */
   switch (base_type) {
   case nir_type_bool1:    return GLSL_TYPE_BOOL;
   case nir_type_uint32:   return GLSL_TYPE_UINT;
   case nir_type_int32:    return GLSL_TYPE_INT;
   case nir_type_uint16:   return GLSL_TYPE_UINT16;
   case nir_type_int16:    return GLSL_TYPE_INT16;
   case nir_type_uint8:    return GLSL_TYPE_UINT8;
   case nir_type_int8:     return GLSL_TYPE_INT8;
   case nir_type_uint64:   return GLSL_TYPE_UINT64;
   case nir_type_int64:    return GLSL_TYPE_INT64;
   case nir_type_float32:  return GLSL_TYPE_FLOAT;
   case nir_type_float16:  return GLSL_TYPE_FLOAT16;
   case nir_type_float64:  return GLSL_TYPE_DOUBLE;
   default: unreachable("Not a sized nir_alu_type");
   }
      }
      nir_op
   nir_op_vec(unsigned num_components)
   {
      /* clang-format off */
   switch (num_components) {
   case  1: return nir_op_mov;
   case  2: return nir_op_vec2;
   case  3: return nir_op_vec3;
   case  4: return nir_op_vec4;
   case  5: return nir_op_vec5;
   case  8: return nir_op_vec8;
   case 16: return nir_op_vec16;
   default: unreachable("bad component count");
   }
      }
      bool
   nir_op_is_vec(nir_op op)
   {
      switch (op) {
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_vec5:
   case nir_op_vec8:
   case nir_op_vec16:
         default:
            }
      bool
   nir_alu_instr_channel_used(const nir_alu_instr *instr, unsigned src,
         {
      if (nir_op_infos[instr->op].input_sizes[src] > 0)
               }
      nir_component_mask_t
   nir_alu_instr_src_read_mask(const nir_alu_instr *instr, unsigned src)
   {
      nir_component_mask_t read_mask = 0;
   for (unsigned c = 0; c < NIR_MAX_VEC_COMPONENTS; c++) {
      if (!nir_alu_instr_channel_used(instr, src, c))
               }
      }
      unsigned
   nir_ssa_alu_instr_src_components(const nir_alu_instr *instr, unsigned src)
   {
      if (nir_op_infos[instr->op].input_sizes[src] > 0)
               }
      #define CASE_ALL_SIZES(op) \
      case op:                \
   case op##8:             \
   case op##16:            \
         bool
   nir_alu_instr_is_comparison(const nir_alu_instr *instr)
   {
      switch (instr->op) {
      CASE_ALL_SIZES(nir_op_flt)
   CASE_ALL_SIZES(nir_op_fge)
   CASE_ALL_SIZES(nir_op_feq)
   CASE_ALL_SIZES(nir_op_fneu)
   CASE_ALL_SIZES(nir_op_ilt)
   CASE_ALL_SIZES(nir_op_ult)
   CASE_ALL_SIZES(nir_op_ige)
   CASE_ALL_SIZES(nir_op_uge)
   CASE_ALL_SIZES(nir_op_ieq)
   CASE_ALL_SIZES(nir_op_ine)
   CASE_ALL_SIZES(nir_op_bitz)
      case nir_op_inot:
         default:
            }
      #undef CASE_ALL_SIZES
      unsigned
   nir_intrinsic_src_components(const nir_intrinsic_instr *intr, unsigned srcn)
   {
      const nir_intrinsic_info *info = &nir_intrinsic_infos[intr->intrinsic];
   assert(srcn < info->num_srcs);
   if (info->src_components[srcn] > 0)
         else if (info->src_components[srcn] == 0)
         else
      }
      unsigned
   nir_intrinsic_dest_components(nir_intrinsic_instr *intr)
   {
      const nir_intrinsic_info *info = &nir_intrinsic_infos[intr->intrinsic];
   if (!info->has_dest)
         else if (info->dest_components)
         else
      }
      nir_alu_type
   nir_intrinsic_instr_src_type(const nir_intrinsic_instr *intrin, unsigned src)
   {
      /* We could go nuts here, but we'll just handle a few simple
   * cases and let everything else be untyped.
   */
   switch (intrin->intrinsic) {
   case nir_intrinsic_store_deref: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   if (src == 1)
                     case nir_intrinsic_store_output:
      if (src == 0)
               default:
                  /* For the most part, we leave other intrinsics alone.  Most
   * of them don't matter in OpenGL ES 2.0 drivers anyway.
   * However, we should at least check if this is some sort of
   * IO intrinsic and flag it's offset and index sources.
   */
   {
      int offset_src_idx = nir_get_io_offset_src_number(intrin);
   if (src == offset_src_idx) {
      const nir_src *offset_src = offset_src_idx >= 0 ? &intrin->src[offset_src_idx] : NULL;
   if (offset_src)
                     }
      nir_alu_type
   nir_intrinsic_instr_dest_type(const nir_intrinsic_instr *intrin)
   {
      /* We could go nuts here, but we'll just handle a few simple
   * cases and let everything else be untyped.
   */
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
               case nir_intrinsic_load_input:
   case nir_intrinsic_load_uniform:
            default:
                     }
      /**
   * Helper to copy const_index[] from src to dst, without assuming they
   * match in order.
   */
   void
   nir_intrinsic_copy_const_indices(nir_intrinsic_instr *dst, nir_intrinsic_instr *src)
   {
      if (src->intrinsic == dst->intrinsic) {
      memcpy(dst->const_index, src->const_index, sizeof(dst->const_index));
               const nir_intrinsic_info *src_info = &nir_intrinsic_infos[src->intrinsic];
            for (unsigned i = 0; i < NIR_INTRINSIC_NUM_INDEX_FLAGS; i++) {
      if (src_info->index_map[i] == 0)
            /* require that dst instruction also uses the same const_index[]: */
            dst->const_index[dst_info->index_map[i] - 1] =
         }
      bool
   nir_tex_instr_need_sampler(const nir_tex_instr *instr)
   {
      switch (instr->op) {
   case nir_texop_txf:
   case nir_texop_txf_ms:
   case nir_texop_txs:
   case nir_texop_query_levels:
   case nir_texop_texture_samples:
   case nir_texop_samples_identical:
   case nir_texop_descriptor_amd:
         default:
            }
      unsigned
   nir_tex_instr_result_size(const nir_tex_instr *instr)
   {
      switch (instr->op) {
   case nir_texop_txs: {
      unsigned ret;
   switch (instr->sampler_dim) {
   case GLSL_SAMPLER_DIM_1D:
   case GLSL_SAMPLER_DIM_BUF:
      ret = 1;
      case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_CUBE:
   case GLSL_SAMPLER_DIM_MS:
   case GLSL_SAMPLER_DIM_RECT:
   case GLSL_SAMPLER_DIM_EXTERNAL:
   case GLSL_SAMPLER_DIM_SUBPASS:
      ret = 2;
      case GLSL_SAMPLER_DIM_3D:
      ret = 3;
      default:
         }
   if (instr->is_array)
                     case nir_texop_lod:
            case nir_texop_texture_samples:
   case nir_texop_query_levels:
   case nir_texop_samples_identical:
   case nir_texop_fragment_mask_fetch_amd:
   case nir_texop_lod_bias_agx:
            case nir_texop_descriptor_amd:
            case nir_texop_sampler_descriptor_amd:
            case nir_texop_hdr_dim_nv:
   case nir_texop_tex_type_nv:
            default:
      if (instr->is_shadow && instr->is_new_style_shadow)
                  }
      bool
   nir_tex_instr_is_query(const nir_tex_instr *instr)
   {
      switch (instr->op) {
   case nir_texop_txs:
   case nir_texop_lod:
   case nir_texop_texture_samples:
   case nir_texop_query_levels:
   case nir_texop_descriptor_amd:
   case nir_texop_sampler_descriptor_amd:
   case nir_texop_lod_bias_agx:
         case nir_texop_tex:
   case nir_texop_txb:
   case nir_texop_txl:
   case nir_texop_txd:
   case nir_texop_txf:
   case nir_texop_txf_ms:
   case nir_texop_txf_ms_fb:
   case nir_texop_txf_ms_mcs_intel:
   case nir_texop_tg4:
   case nir_texop_samples_identical:
   case nir_texop_fragment_mask_fetch_amd:
   case nir_texop_fragment_fetch_amd:
         default:
            }
      bool
   nir_tex_instr_has_implicit_derivative(const nir_tex_instr *instr)
   {
      switch (instr->op) {
   case nir_texop_tex:
   case nir_texop_txb:
   case nir_texop_lod:
         default:
            }
      nir_alu_type
   nir_tex_instr_src_type(const nir_tex_instr *instr, unsigned src)
   {
      switch (instr->src[src].src_type) {
   case nir_tex_src_coord:
      switch (instr->op) {
   case nir_texop_txf:
   case nir_texop_txf_ms:
   case nir_texop_txf_ms_fb:
   case nir_texop_txf_ms_mcs_intel:
   case nir_texop_samples_identical:
   case nir_texop_fragment_fetch_amd:
   case nir_texop_fragment_mask_fetch_amd:
            default:
               case nir_tex_src_lod:
      switch (instr->op) {
   case nir_texop_txs:
   case nir_texop_txf:
   case nir_texop_txf_ms:
   case nir_texop_fragment_fetch_amd:
   case nir_texop_fragment_mask_fetch_amd:
            default:
               case nir_tex_src_projector:
   case nir_tex_src_comparator:
   case nir_tex_src_bias:
   case nir_tex_src_min_lod:
   case nir_tex_src_ddx:
   case nir_tex_src_ddy:
   case nir_tex_src_backend1:
   case nir_tex_src_backend2:
            case nir_tex_src_offset:
   case nir_tex_src_ms_index:
   case nir_tex_src_plane:
            case nir_tex_src_ms_mcs_intel:
   case nir_tex_src_texture_deref:
   case nir_tex_src_sampler_deref:
   case nir_tex_src_texture_offset:
   case nir_tex_src_sampler_offset:
   case nir_tex_src_texture_handle:
   case nir_tex_src_sampler_handle:
            case nir_num_tex_src_types:
                     }
      unsigned
   nir_tex_instr_src_size(const nir_tex_instr *instr, unsigned src)
   {
      if (instr->src[src].src_type == nir_tex_src_coord)
            /* The MCS value is expected to be a vec4 returned by a txf_ms_mcs_intel */
   if (instr->src[src].src_type == nir_tex_src_ms_mcs_intel)
            if (instr->src[src].src_type == nir_tex_src_ddx ||
               if (instr->is_array && !instr->array_is_lowered_cube)
         else
               if (instr->src[src].src_type == nir_tex_src_offset) {
      if (instr->is_array)
         else
               if (instr->src[src].src_type == nir_tex_src_backend1 ||
      instr->src[src].src_type == nir_tex_src_backend2)
         /* For AMD, this can be a vec8/vec4 image/sampler descriptor. */
   if (instr->src[src].src_type == nir_tex_src_texture_handle ||
      instr->src[src].src_type == nir_tex_src_sampler_handle)
            }
      /**
   * Return which components are written into transform feedback buffers.
   * The result is relative to 0, not "component".
   */
   unsigned
   nir_instr_xfb_write_mask(nir_intrinsic_instr *instr)
   {
               if (nir_intrinsic_has_io_xfb(instr)) {
      unsigned wr_mask = nir_intrinsic_write_mask(instr) << nir_intrinsic_component(instr);
            unsigned iter_mask = wr_mask;
   while (iter_mask) {
      unsigned i = u_bit_scan(&iter_mask);
   nir_io_xfb xfb = i < 2 ? nir_intrinsic_io_xfb(instr) : nir_intrinsic_io_xfb2(instr);
   if (xfb.out[i % 2].num_components)
                     }
      /**
   * Whether an output slot is consumed by fixed-function logic.
   */
   bool
   nir_slot_is_sysval_output(gl_varying_slot slot, gl_shader_stage next_shader)
   {
      switch (next_shader) {
   case MESA_SHADER_FRAGMENT:
      return slot == VARYING_SLOT_POS ||
         slot == VARYING_SLOT_PSIZ ||
   slot == VARYING_SLOT_EDGE ||
   slot == VARYING_SLOT_CLIP_VERTEX ||
   slot == VARYING_SLOT_CLIP_DIST0 ||
   slot == VARYING_SLOT_CLIP_DIST1 ||
   slot == VARYING_SLOT_CULL_DIST0 ||
   slot == VARYING_SLOT_CULL_DIST1 ||
   slot == VARYING_SLOT_LAYER ||
   slot == VARYING_SLOT_VIEWPORT ||
   slot == VARYING_SLOT_VIEW_INDEX ||
   slot == VARYING_SLOT_VIEWPORT_MASK ||
   slot == VARYING_SLOT_PRIMITIVE_SHADING_RATE ||
   /* NV_mesh_shader_only */
         case MESA_SHADER_TESS_EVAL:
      return slot == VARYING_SLOT_TESS_LEVEL_OUTER ||
         slot == VARYING_SLOT_TESS_LEVEL_INNER ||
         case MESA_SHADER_MESH:
      /* NV_mesh_shader only */
         case MESA_SHADER_NONE:
      /* NONE means unknown. Check all possibilities. */
   return nir_slot_is_sysval_output(slot, MESA_SHADER_FRAGMENT) ||
               default:
      /* No other shaders have preceding shaders with sysval outputs. */
         }
      /**
   * Whether an input/output slot is consumed by the next shader stage,
   * or written by the previous shader stage.
   */
   bool
   nir_slot_is_varying(gl_varying_slot slot)
   {
      return slot >= VARYING_SLOT_VAR0 ||
         slot == VARYING_SLOT_COL0 ||
   slot == VARYING_SLOT_COL1 ||
   slot == VARYING_SLOT_BFC0 ||
   slot == VARYING_SLOT_BFC1 ||
   slot == VARYING_SLOT_FOGC ||
   (slot >= VARYING_SLOT_TEX0 && slot <= VARYING_SLOT_TEX7) ||
   slot == VARYING_SLOT_PNTC ||
   slot == VARYING_SLOT_CLIP_DIST0 ||
   slot == VARYING_SLOT_CLIP_DIST1 ||
   slot == VARYING_SLOT_CULL_DIST0 ||
   slot == VARYING_SLOT_CULL_DIST1 ||
   slot == VARYING_SLOT_PRIMITIVE_ID ||
   slot == VARYING_SLOT_LAYER ||
   slot == VARYING_SLOT_VIEWPORT ||
      }
      bool
   nir_slot_is_sysval_output_and_varying(gl_varying_slot slot,
         {
      return nir_slot_is_sysval_output(slot, next_shader) &&
      }
      /**
   * This marks the output store instruction as not feeding the next shader
   * stage. If the instruction has no other use, it's removed.
   */
   bool
   nir_remove_varying(nir_intrinsic_instr *intr, gl_shader_stage next_shader)
   {
               if ((!sem.no_sysval_output &&
      nir_slot_is_sysval_output(sem.location, next_shader)) ||
   nir_instr_xfb_write_mask(intr)) {
   /* Demote the store instruction. */
   sem.no_varying = true;
   nir_intrinsic_set_io_semantics(intr, sem);
      } else {
      nir_instr_remove(&intr->instr);
         }
      /**
   * This marks the output store instruction as not feeding fixed-function
   * logic. If the instruction has no other use, it's removed.
   */
   void
   nir_remove_sysval_output(nir_intrinsic_instr *intr)
   {
               if ((!sem.no_varying && nir_slot_is_varying(sem.location)) ||
      nir_instr_xfb_write_mask(intr)) {
   /* Demote the store instruction. */
   sem.no_sysval_output = true;
      } else {
            }
      void
   nir_remove_non_entrypoints(nir_shader *nir)
   {
      nir_foreach_function_safe(func, nir) {
      if (!func->is_entrypoint)
      }
      }
