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
      #include <assert.h>
   #include "c11/threads.h"
   #include "util/simple_mtx.h"
   #include "nir.h"
   #include "nir_xfb_info.h"
      /*
   * This file checks for invalid IR indicating a bug somewhere in the compiler.
   */
      /* Since this file is just a pile of asserts, don't bother compiling it if
   * we're not building a debug build.
   */
   #ifndef NDEBUG
      typedef struct {
               /* the current shader being validated */
            /* the current instruction being validated */
            /* the current variable being validated */
            /* the current basic block being validated */
            /* the current if statement being validated */
            /* the current loop being visited */
            /* weather the loop continue construct is being visited */
            /* the parent of the current cf node being visited */
            /* the current function implementation being validated */
            /* Set of all blocks in the list */
            /* Set of seen SSA sources */
            /* bitset of ssa definitions we have found; used to check uniqueness */
            /* map of variable -> function implementation where it is defined or NULL
   * if it is a global variable
   */
            /* map of instruction/var/etc to failed assert string */
      } validate_state;
      static void
   log_error(validate_state *state, const char *cond, const char *file, int line)
   {
               if (state->instr)
         else if (state->var)
         else
            char *msg = ralloc_asprintf(state->errors, "error: %s (%s:%d)",
               }
      static bool
   validate_assert_impl(validate_state *state, bool cond, const char *str,
         {
      if (!cond)
            }
      #define validate_assert(state, cond) \
            static void validate_src(nir_src *src, validate_state *state,
            static void
   validate_num_components(validate_state *state, unsigned num_components)
   {
         }
      static void
   validate_ssa_src(nir_src *src, validate_state *state,
         {
               /* As we walk SSA defs, we add every use to this set.  We need to make sure
   * our use is seen in a use list.
   */
   struct set_entry *entry = _mesa_set_search(state->ssa_srcs, src);
            /* This will let us prove that we've seen all the sources */
   if (entry)
            if (bit_sizes)
         if (num_components)
               }
      static void
   validate_src(nir_src *src, validate_state *state,
         {
      if (state->instr)
         else
               }
      static void
   validate_alu_src(nir_alu_instr *instr, unsigned index, validate_state *state)
   {
               unsigned num_components = nir_src_num_components(src->src);
   for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
               if (nir_alu_instr_channel_used(instr, index, i))
                  }
      static void
   validate_def(nir_def *def, validate_state *state,
         {
      if (bit_sizes)
         if (num_components)
            validate_assert(state, def->index < state->impl->ssa_alloc);
   validate_assert(state, !BITSET_TEST(state->ssa_defs_found, def->index));
            validate_assert(state, def->parent_instr == state->instr);
            list_validate(&def->uses);
   nir_foreach_use_including_if(src, def) {
               bool already_seen = false;
   _mesa_set_search_and_add(state->ssa_srcs, src, &already_seen);
   /* A nir_src should only appear once and only in one SSA def use list */
         }
      static void
   validate_alu_instr(nir_alu_instr *instr, validate_state *state)
   {
               unsigned instr_bit_size = 0;
   for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      nir_alu_type src_type = nir_op_infos[instr->op].input_types[i];
   unsigned src_bit_size = nir_src_bit_size(instr->src[i].src);
   if (nir_alu_type_get_type_size(src_type)) {
         } else if (instr_bit_size) {
         } else {
                  if (nir_alu_type_get_base_type(src_type) == nir_type_float) {
      /* 8-bit float isn't a thing */
   validate_assert(state, src_bit_size == 16 || src_bit_size == 32 ||
               /* In nir_opcodes.py, these are defined to take general uint or int
   * sources.  However, they're really only defined for 32-bit or 64-bit
   * sources.  This seems to be the only place to enforce this
   * restriction.
   */
   switch (instr->op) {
   case nir_op_ufind_msb:
   case nir_op_ufind_msb_rev:
                  default:
                              nir_alu_type dest_type = nir_op_infos[instr->op].output_type;
   unsigned dest_bit_size = instr->def.bit_size;
   if (nir_alu_type_get_type_size(dest_type)) {
         } else if (instr_bit_size) {
         } else {
                  if (nir_alu_type_get_base_type(dest_type) == nir_type_float) {
      /* 8-bit float isn't a thing */
   validate_assert(state, dest_bit_size == 16 || dest_bit_size == 32 ||
                  }
      static void
   validate_var_use(nir_variable *var, validate_state *state)
   {
      struct hash_entry *entry = _mesa_hash_table_search(state->var_defs, var);
   validate_assert(state, entry);
   if (entry && var->data.mode == nir_var_function_temp)
      }
      static void
   validate_deref_instr(nir_deref_instr *instr, validate_state *state)
   {
      if (instr->deref_type == nir_deref_type_var) {
      /* Variable dereferences are stupid simple. */
   validate_assert(state, instr->modes == instr->var->data.mode);
   validate_assert(state, instr->type == instr->var->type);
      } else if (instr->deref_type == nir_deref_type_cast) {
      /* For cast, we simply have to trust the instruction.  It's up to
   * lowering passes and front/back-ends to make them sane.
   */
            /* Most variable modes in NIR can only exist by themselves. */
   if (instr->modes & ~nir_var_mem_generic)
            nir_deref_instr *parent = nir_src_as_deref(instr->parent);
   if (parent) {
      /* Casts can change the mode but it can't change completely.  The new
   * mode must have some bits in common with the old.
   */
      } else {
      /* If our parent isn't a deref, just assert the mode is there */
               /* We just validate that the type is there */
   validate_assert(state, instr->type);
   if (instr->cast.align_mul > 0) {
      validate_assert(state, util_is_power_of_two_nonzero(instr->cast.align_mul));
      } else {
            } else {
      /* The parent pointer value must have the same number of components
   * as the destination.
   */
   validate_src(&instr->parent, state, instr->def.bit_size,
                     /* The parent must come from another deref instruction */
                              switch (instr->deref_type) {
   case nir_deref_type_struct:
      validate_assert(state, glsl_type_is_struct_or_ifc(parent->type));
   validate_assert(state,
         validate_assert(state, instr->type ==
               case nir_deref_type_array:
   case nir_deref_type_array_wildcard:
      if (instr->modes & nir_var_vec_indexable_modes) {
      /* Shared variables and UBO/SSBOs have a bit more relaxed rules
   * because we need to be able to handle array derefs on vectors.
   * Fortunately, nir_lower_io handles these just fine.
   */
   validate_assert(state, glsl_type_is_array(parent->type) ||
            } else {
      /* Most of NIR cannot handle array derefs on vectors */
   validate_assert(state, glsl_type_is_array(parent->type) ||
      }
                  if (instr->deref_type == nir_deref_type_array) {
      validate_src(&instr->arr.index, state,
                  case nir_deref_type_ptr_as_array:
      /* ptr_as_array derefs must have a parent that is either an array,
   * ptr_as_array, or cast.  If the parent is a cast, we get the stride
   * information (if any) from the cast deref.
   */
   validate_assert(state,
                     validate_src(&instr->arr.index, state,
               default:
                     /* We intentionally don't validate the size of the destination because we
   * want to let other compiler components such as SPIR-V decide how big
   * pointers should be.
   */
            /* Certain modes cannot be used as sources for phi instructions because
   * way too many passes assume that they can always chase deref chains.
   */
   nir_foreach_use_including_if(use, &instr->def) {
      /* Deref instructions as if conditions don't make sense because if
   * conditions expect well-formed Booleans.  If you want to compare with
   * NULL, an explicit comparison operation should be used.
   */
   if (!validate_assert(state, !nir_src_is_if(use)))
            if (nir_src_parent_instr(use)->type == nir_instr_type_phi) {
      validate_assert(state, !(instr->modes & (nir_var_shader_in |
                        }
      static bool
   vectorized_intrinsic(nir_intrinsic_instr *intr)
   {
               if (info->dest_components == 0)
            for (unsigned i = 0; i < info->num_srcs; i++)
      if (info->src_components[i] == 0)
            }
      /** Returns the image format or PIPE_FORMAT_COUNT for incomplete derefs
   *
   * We use PIPE_FORMAT_COUNT for incomplete derefs because PIPE_FORMAT_NONE
   * indicates that we found the variable but it has no format specified.
   */
   static enum pipe_format
   image_intrin_format(nir_intrinsic_instr *instr)
   {
      if (nir_intrinsic_format(instr) != PIPE_FORMAT_NONE)
            /* If this not a deref intrinsic, PIPE_FORMAT_NONE is the best we can do */
   if (nir_intrinsic_infos[instr->intrinsic].src_components[0] != -1)
            nir_variable *var = nir_intrinsic_get_var(instr, 0);
   if (var == NULL)
               }
      static void
   validate_register_handle(nir_src handle_src,
                     {
      nir_def *handle = handle_src.ssa;
            if (!validate_assert(state, parent->type == nir_instr_type_intrinsic))
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(parent);
   if (!validate_assert(state, intr->intrinsic == nir_intrinsic_decl_reg))
            validate_assert(state, nir_intrinsic_num_components(intr) == num_components);
      }
      static void
   validate_intrinsic_instr(nir_intrinsic_instr *instr, validate_state *state)
   {
      unsigned dest_bit_size = 0;
   unsigned src_bit_sizes[NIR_INTRINSIC_MAX_INPUTS] = {
         };
   switch (instr->intrinsic) {
   case nir_intrinsic_decl_reg:
      assert(state->block == nir_start_block(state->impl));
         case nir_intrinsic_load_reg:
   case nir_intrinsic_load_reg_indirect:
      validate_register_handle(instr->src[0],
                     case nir_intrinsic_store_reg:
   case nir_intrinsic_store_reg_indirect:
      validate_register_handle(instr->src[1],
                     case nir_intrinsic_convert_alu_types: {
      nir_alu_type src_type = nir_intrinsic_src_type(instr);
   nir_alu_type dest_type = nir_intrinsic_dest_type(instr);
   dest_bit_size = nir_alu_type_get_type_size(dest_type);
   src_bit_sizes[0] = nir_alu_type_get_type_size(src_type);
   validate_assert(state, dest_bit_size != 0);
   validate_assert(state, src_bit_sizes[0] != 0);
               case nir_intrinsic_load_param: {
      unsigned param_idx = nir_intrinsic_param_idx(instr);
   validate_assert(state, param_idx < state->impl->function->num_params);
   nir_parameter *param = &state->impl->function->params[param_idx];
   validate_assert(state, instr->num_components == param->num_components);
   dest_bit_size = param->bit_size;
               case nir_intrinsic_load_deref: {
      nir_deref_instr *src = nir_src_as_deref(instr->src[0]);
   assert(src);
   validate_assert(state, glsl_type_is_vector_or_scalar(src->type) ||
               validate_assert(state, instr->num_components ==
         dest_bit_size = glsl_get_bit_size(src->type);
   /* Also allow 32-bit boolean load operations */
   if (glsl_type_is_boolean(src->type))
                     case nir_intrinsic_store_deref: {
      nir_deref_instr *dst = nir_src_as_deref(instr->src[0]);
   assert(dst);
   validate_assert(state, glsl_type_is_vector_or_scalar(dst->type));
   validate_assert(state, instr->num_components ==
         src_bit_sizes[1] = glsl_get_bit_size(dst->type);
   /* Also allow 32-bit boolean store operations */
   if (glsl_type_is_boolean(dst->type))
         validate_assert(state, !nir_deref_mode_may_be(dst, nir_var_read_only_modes));
               case nir_intrinsic_copy_deref: {
      nir_deref_instr *dst = nir_src_as_deref(instr->src[0]);
   nir_deref_instr *src = nir_src_as_deref(instr->src[1]);
   validate_assert(state, glsl_get_bare_type(dst->type) ==
         validate_assert(state, !nir_deref_mode_may_be(dst, nir_var_read_only_modes));
   /* FIXME: now that we track if the var copies were lowered, it would be
   * good to validate here that no new copy derefs were added. Right now
   * we can't as there are some specific cases where copies are added even
   * after the lowering. One example is the Intel compiler, that calls
   * nir_lower_io_to_temporaries when linking some shader stages.
   */
               case nir_intrinsic_load_ubo_vec4: {
      int bit_size = instr->def.bit_size;
   validate_assert(state, bit_size >= 8);
   validate_assert(state, (nir_intrinsic_component(instr) +
                                 case nir_intrinsic_load_ubo:
      /* Make sure that the creator didn't forget to set the range_base+range. */
   validate_assert(state, nir_intrinsic_range(instr) != 0);
      case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_shared:
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_scratch:
   case nir_intrinsic_load_constant:
      /* These memory load operations must have alignments */
   validate_assert(state,
         validate_assert(state, nir_intrinsic_align_offset(instr) <
               case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_output:
   case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_load_per_primitive_output:
   case nir_intrinsic_load_push_constant:
      /* All memory load operations must load at least a byte */
   validate_assert(state, instr->def.bit_size >= 8);
         case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_scratch:
      /* These memory store operations must also have alignments */
   validate_assert(state,
         validate_assert(state, nir_intrinsic_align_offset(instr) <
               case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
      /* All memory store operations must store at least a byte */
   validate_assert(state, nir_src_bit_size(instr->src[0]) >= 8);
         case nir_intrinsic_deref_mode_is:
   case nir_intrinsic_addr_mode_is:
      validate_assert(state,
               case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap: {
               enum pipe_format format = image_intrin_format(instr);
   if (format != PIPE_FORMAT_COUNT) {
                     switch (format) {
   case PIPE_FORMAT_R32_FLOAT:
      allowed = is_float || op == nir_atomic_op_xchg;
      case PIPE_FORMAT_R16_FLOAT:
   case PIPE_FORMAT_R64_FLOAT:
      allowed = op == nir_atomic_op_fmin || op == nir_atomic_op_fmax;
      case PIPE_FORMAT_R32_UINT:
   case PIPE_FORMAT_R32_SINT:
   case PIPE_FORMAT_R64_UINT:
   case PIPE_FORMAT_R64_SINT:
      allowed = !is_float;
      default:
                  validate_assert(state, allowed);
   validate_assert(state, instr->def.bit_size ==
      }
               case nir_intrinsic_store_buffer_amd:
      if (nir_intrinsic_access(instr) & ACCESS_USES_FORMAT_AMD) {
               /* Make sure the writemask is derived from the component count. */
   validate_assert(state,
            }
         default:
                  if (instr->num_components > 0)
            const nir_intrinsic_info *info = &nir_intrinsic_infos[instr->intrinsic];
   unsigned num_srcs = info->num_srcs;
   for (unsigned i = 0; i < num_srcs; i++) {
                                    if (nir_intrinsic_infos[instr->intrinsic].has_dest) {
      unsigned components_written = nir_intrinsic_dest_components(instr);
   unsigned bit_sizes = info->dest_bit_sizes;
   if (!bit_sizes && info->bit_size_src >= 0)
            validate_num_components(state, components_written);
   if (dest_bit_size && bit_sizes)
         else
                        if (!vectorized_intrinsic(instr))
            if (nir_intrinsic_has_write_mask(instr)) {
      unsigned component_mask = BITFIELD_MASK(instr->num_components);
               if (nir_intrinsic_has_io_xfb(instr)) {
               for (unsigned i = 0; i < 4; i++) {
                     /* Each component can be used only once by transform feedback info. */
   validate_assert(state, (xfb_mask & used_mask) == 0);
                  if (nir_intrinsic_has_io_semantics(instr) &&
      !nir_intrinsic_infos[instr->intrinsic].has_dest) {
            /* An output that has no effect shouldn't be present in the IR. */
   validate_assert(state,
                  (nir_slot_is_sysval_output(sem.location, MESA_SHADER_NONE) &&
      }
      static void
   validate_tex_instr(nir_tex_instr *instr, validate_state *state)
   {
      bool src_type_seen[nir_num_tex_src_types];
   for (unsigned i = 0; i < nir_num_tex_src_types; i++)
            for (unsigned i = 0; i < instr->num_srcs; i++) {
      validate_assert(state, !src_type_seen[instr->src[i].src_type]);
   src_type_seen[instr->src[i].src_type] = true;
   validate_src(&instr->src[i].src, state,
                     case nir_tex_src_comparator:
                  case nir_tex_src_bias:
      validate_assert(state, instr->op == nir_texop_txb ||
               case nir_tex_src_lod:
      validate_assert(state, instr->op != nir_texop_tex &&
                           case nir_tex_src_ddx:
   case nir_tex_src_ddy:
                  case nir_tex_src_texture_deref: {
      nir_deref_instr *deref = nir_src_as_deref(instr->src[i].src);
                  validate_assert(state, glsl_type_is_image(deref->type) ||
               switch (instr->op) {
   case nir_texop_descriptor_amd:
   case nir_texop_sampler_descriptor_amd:
         case nir_texop_lod:
   case nir_texop_lod_bias_agx:
      validate_assert(state, nir_alu_type_get_base_type(instr->dest_type) == nir_type_float);
      case nir_texop_samples_identical:
      validate_assert(state, nir_alu_type_get_base_type(instr->dest_type) == nir_type_bool);
      case nir_texop_txs:
   case nir_texop_texture_samples:
   case nir_texop_query_levels:
   case nir_texop_fragment_mask_fetch_amd:
   case nir_texop_txf_ms_mcs_intel:
                        default:
      validate_assert(state,
                  glsl_get_sampler_result_type(deref->type) == GLSL_TYPE_VOID ||
   }
               case nir_tex_src_sampler_deref: {
      nir_deref_instr *deref = nir_src_as_deref(instr->src[i].src);
                  validate_assert(state, glsl_type_is_sampler(deref->type));
               case nir_tex_src_coord:
   case nir_tex_src_projector:
   case nir_tex_src_offset:
   case nir_tex_src_min_lod:
   case nir_tex_src_ms_index:
   case nir_tex_src_texture_offset:
   case nir_tex_src_sampler_offset:
   case nir_tex_src_plane:
   case nir_tex_src_texture_handle:
   case nir_tex_src_sampler_handle:
            default:
                     bool msaa = (instr->sampler_dim == GLSL_SAMPLER_DIM_MS ||
            if (msaa)
         else
            if (instr->op != nir_texop_tg4)
            if (nir_tex_instr_has_explicit_tg4_offsets(instr)) {
      validate_assert(state, instr->op == nir_texop_tg4);
               if (instr->is_gather_implicit_lod)
                     unsigned bit_size = nir_alu_type_get_type_size(instr->dest_type);
   validate_assert(state,
            }
      static void
   validate_call_instr(nir_call_instr *instr, validate_state *state)
   {
               for (unsigned i = 0; i < instr->num_params; i++) {
      validate_src(&instr->params[i], state,
               }
      static void
   validate_const_value(nir_const_value *val, unsigned bit_size,
         {
      /* In order for block copies to work properly for things like instruction
   * comparisons and [de]serialization, we require the unused bits of the
   * nir_const_value to be zero.
   */
   nir_const_value cmp_val;
   memset(&cmp_val, 0, sizeof(cmp_val));
   if (!is_null_constant) {
      switch (bit_size) {
   case 1:
      cmp_val.b = val->b;
      case 8:
      cmp_val.u8 = val->u8;
      case 16:
      cmp_val.u16 = val->u16;
      case 32:
      cmp_val.u32 = val->u32;
      case 64:
      cmp_val.u64 = val->u64;
      default:
            }
      }
      static void
   validate_load_const_instr(nir_load_const_instr *instr, validate_state *state)
   {
               for (unsigned i = 0; i < instr->def.num_components; i++)
      }
      static void
   validate_ssa_undef_instr(nir_undef_instr *instr, validate_state *state)
   {
         }
      static void
   validate_phi_instr(nir_phi_instr *instr, validate_state *state)
   {
      /*
   * don't validate the sources until we get to them from their predecessor
   * basic blocks, to avoid validating an SSA use before its definition.
                     exec_list_validate(&instr->srcs);
   validate_assert(state, exec_list_length(&instr->srcs) ==
      }
      static void
   validate_jump_instr(nir_jump_instr *instr, validate_state *state)
   {
      nir_block *block = state->block;
            switch (instr->type) {
   case nir_jump_return:
   case nir_jump_halt:
      validate_assert(state, block->successors[0] == state->impl->end_block);
   validate_assert(state, block->successors[1] == NULL);
   validate_assert(state, instr->target == NULL);
   validate_assert(state, instr->else_target == NULL);
   validate_assert(state, !state->in_loop_continue_construct);
         case nir_jump_break:
      validate_assert(state, state->impl->structured);
   validate_assert(state, state->loop != NULL);
   if (state->loop) {
      nir_block *after =
            }
   validate_assert(state, block->successors[1] == NULL);
   validate_assert(state, instr->target == NULL);
   validate_assert(state, instr->else_target == NULL);
         case nir_jump_continue:
      validate_assert(state, state->impl->structured);
   validate_assert(state, state->loop != NULL);
   if (state->loop) {
      nir_block *cont_block = nir_loop_continue_target(state->loop);
      }
   validate_assert(state, block->successors[1] == NULL);
   validate_assert(state, instr->target == NULL);
   validate_assert(state, instr->else_target == NULL);
   validate_assert(state, !state->in_loop_continue_construct);
         case nir_jump_goto:
      validate_assert(state, !state->impl->structured);
   validate_assert(state, instr->target == block->successors[0]);
   validate_assert(state, instr->target != NULL);
   validate_assert(state, instr->else_target == NULL);
         case nir_jump_goto_if:
      validate_assert(state, !state->impl->structured);
   validate_assert(state, instr->target == block->successors[1]);
   validate_assert(state, instr->else_target == block->successors[0]);
   validate_src(&instr->condition, state, 0, 1);
   validate_assert(state, instr->target != NULL);
   validate_assert(state, instr->else_target != NULL);
         default:
      validate_assert(state, !"Invalid jump instruction type");
         }
      static void
   validate_instr(nir_instr *instr, validate_state *state)
   {
                        switch (instr->type) {
   case nir_instr_type_alu:
      validate_alu_instr(nir_instr_as_alu(instr), state);
         case nir_instr_type_deref:
      validate_deref_instr(nir_instr_as_deref(instr), state);
         case nir_instr_type_call:
      validate_call_instr(nir_instr_as_call(instr), state);
         case nir_instr_type_intrinsic:
      validate_intrinsic_instr(nir_instr_as_intrinsic(instr), state);
         case nir_instr_type_tex:
      validate_tex_instr(nir_instr_as_tex(instr), state);
         case nir_instr_type_load_const:
      validate_load_const_instr(nir_instr_as_load_const(instr), state);
         case nir_instr_type_phi:
      validate_phi_instr(nir_instr_as_phi(instr), state);
         case nir_instr_type_undef:
      validate_ssa_undef_instr(nir_instr_as_undef(instr), state);
         case nir_instr_type_jump:
      validate_jump_instr(nir_instr_as_jump(instr), state);
         default:
      validate_assert(state, !"Invalid ALU instruction type");
                  }
      static void
   validate_phi_src(nir_phi_instr *instr, nir_block *pred, validate_state *state)
   {
               exec_list_validate(&instr->srcs);
   nir_foreach_phi_src(src, instr) {
      if (src->pred == pred) {
      validate_src(&src->src, state, instr->def.bit_size,
         state->instr = NULL;
         }
   validate_assert(state, !"Phi does not have a source corresponding to one "
      }
      static void
   validate_phi_srcs(nir_block *block, nir_block *succ, validate_state *state)
   {
      nir_foreach_phi(phi, succ) {
            }
      static void
   collect_blocks(struct exec_list *cf_list, validate_state *state)
   {
      /* We walk the blocks manually here rather than using nir_foreach_block for
   * a few reasons:
   *
   *  1. nir_foreach_block() doesn't work properly for unstructured NIR and
   *     we need to be able to handle all forms of NIR here.
   *
   *  2. We want to call exec_list_validate() on every linked list in the IR
   *     which means we need to touch every linked and just walking blocks
   *     with nir_foreach_block() would make that difficult.  In particular,
   *     we want to validate each list before the first time we walk it so
   *     that we catch broken lists in exec_list_validate() instead of
   *     getting stuck in a hard-to-debug infinite loop in the validator.
   *
   *  3. nir_foreach_block() depends on several invariants of the CF node
   *     hierarchy which nir_validate_shader() is responsible for verifying.
   *     If we used nir_foreach_block() in nir_validate_shader(), we could
   *     end up blowing up on a bad list walk instead of throwing the much
   *     easier to debug validation error.
   */
   exec_list_validate(cf_list);
   foreach_list_typed(nir_cf_node, node, node, cf_list) {
      switch (node->type) {
   case nir_cf_node_block:
                  case nir_cf_node_if:
      collect_blocks(&nir_cf_node_as_if(node)->then_list, state);
               case nir_cf_node_loop:
      collect_blocks(&nir_cf_node_as_loop(node)->body, state);
               default:
               }
      static void validate_cf_node(nir_cf_node *node, validate_state *state);
      static void
   validate_block_predecessors(nir_block *block, validate_state *state)
   {
      for (unsigned i = 0; i < 2; i++) {
      if (block->successors[i] == NULL)
            /* The block has to exist in the nir_function_impl */
   validate_assert(state, _mesa_set_search(state->blocks,
            /* And we have to be in our successor's predecessors set */
   validate_assert(state,
                        /* The start block cannot have any predecessors */
   if (block == nir_start_block(state->impl))
            set_foreach(block->predecessors, entry) {
      const nir_block *pred = entry->key;
   validate_assert(state, _mesa_set_search(state->blocks, pred));
   validate_assert(state, pred->successors[0] == block ||
         }
      static void
   validate_block(nir_block *block, validate_state *state)
   {
                        exec_list_validate(&block->instr_list);
   nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_phi) {
      validate_assert(state, instr == nir_block_first_instr(block) ||
                           validate_assert(state, block->successors[0] != NULL);
   validate_assert(state, block->successors[0] != block->successors[1]);
            if (!state->impl->structured) {
         } else if (!nir_block_ends_in_jump(block)) {
      nir_cf_node *next = nir_cf_node_next(&block->cf_node);
   if (next == NULL) {
      switch (state->parent_node->type) {
   case nir_cf_node_loop: {
      if (block == nir_loop_last_block(state->loop)) {
      nir_block *cont = nir_loop_continue_target(state->loop);
      } else {
      validate_assert(state, nir_loop_has_continue_construct(state->loop) &&
         nir_block *head = nir_loop_first_block(state->loop);
      }
   /* due to the hack for infinite loops, block->successors[1] may
   * point to the block after the loop.
   */
               case nir_cf_node_if: {
      nir_block *after =
         validate_assert(state, block->successors[0] == after);
   validate_assert(state, block->successors[1] == NULL);
               case nir_cf_node_function:
      validate_assert(state, block->successors[0] == state->impl->end_block);
               default:
            } else {
      if (next->type == nir_cf_node_if) {
      nir_if *if_stmt = nir_cf_node_as_if(next);
   validate_assert(state, block->successors[0] ==
         validate_assert(state, block->successors[1] ==
      } else if (next->type == nir_cf_node_loop) {
      nir_loop *loop = nir_cf_node_as_loop(next);
   validate_assert(state, block->successors[0] ==
            } else {
      validate_assert(state,
               }
      static void
   validate_end_block(nir_block *block, validate_state *state)
   {
               exec_list_validate(&block->instr_list);
            validate_assert(state, block->successors[0] == NULL);
   validate_assert(state, block->successors[1] == NULL);
      }
      static void
   validate_if(nir_if *if_stmt, validate_state *state)
   {
                        validate_assert(state, !exec_node_is_head_sentinel(if_stmt->cf_node.node.prev));
   nir_cf_node *prev_node = nir_cf_node_prev(&if_stmt->cf_node);
            validate_assert(state, !exec_node_is_tail_sentinel(if_stmt->cf_node.node.next));
   nir_cf_node *next_node = nir_cf_node_next(&if_stmt->cf_node);
            validate_assert(state, nir_src_is_if(&if_stmt->condition));
            validate_assert(state, !exec_list_is_empty(&if_stmt->then_list));
            nir_cf_node *old_parent = state->parent_node;
            foreach_list_typed(nir_cf_node, cf_node, node, &if_stmt->then_list) {
                  foreach_list_typed(nir_cf_node, cf_node, node, &if_stmt->else_list) {
                  state->parent_node = old_parent;
      }
      static void
   validate_loop(nir_loop *loop, validate_state *state)
   {
               validate_assert(state, !exec_node_is_head_sentinel(loop->cf_node.node.prev));
   nir_cf_node *prev_node = nir_cf_node_prev(&loop->cf_node);
            validate_assert(state, !exec_node_is_tail_sentinel(loop->cf_node.node.next));
   nir_cf_node *next_node = nir_cf_node_next(&loop->cf_node);
                     nir_cf_node *old_parent = state->parent_node;
   state->parent_node = &loop->cf_node;
   nir_loop *old_loop = state->loop;
   bool old_continue_construct = state->in_loop_continue_construct;
   state->loop = loop;
            foreach_list_typed(nir_cf_node, cf_node, node, &loop->body) {
         }
   state->in_loop_continue_construct = true;
   foreach_list_typed(nir_cf_node, cf_node, node, &loop->continue_list) {
         }
   state->in_loop_continue_construct = false;
   state->parent_node = old_parent;
   state->loop = old_loop;
      }
      static void
   validate_cf_node(nir_cf_node *node, validate_state *state)
   {
               switch (node->type) {
   case nir_cf_node_block:
      validate_block(nir_cf_node_as_block(node), state);
         case nir_cf_node_if:
      validate_if(nir_cf_node_as_if(node), state);
         case nir_cf_node_loop:
      validate_loop(nir_cf_node_as_loop(node), state);
         default:
            }
      static void
   validate_constant(nir_constant *c, const struct glsl_type *type,
         {
      if (glsl_type_is_vector_or_scalar(type)) {
      unsigned num_components = glsl_get_vector_elements(type);
   unsigned bit_size = glsl_get_bit_size(type);
   for (unsigned i = 0; i < num_components; i++)
         for (unsigned i = num_components; i < NIR_MAX_VEC_COMPONENTS; i++)
      } else {
      validate_assert(state, c->num_elements == glsl_get_length(type));
   if (glsl_type_is_struct_or_ifc(type)) {
      for (unsigned i = 0; i < c->num_elements; i++) {
      const struct glsl_type *elem_type = glsl_get_struct_field(type, i);
   validate_constant(c->elements[i], elem_type, state);
         } else if (glsl_type_is_array_or_matrix(type)) {
      const struct glsl_type *elem_type = glsl_get_array_element(type);
   for (unsigned i = 0; i < c->num_elements; i++) {
      validate_constant(c->elements[i], elem_type, state);
         } else {
               }
      static void
   validate_var_decl(nir_variable *var, nir_variable_mode valid_modes,
         {
               /* Must have exactly one mode set */
   validate_assert(state, util_is_power_of_two_nonzero(var->data.mode));
            if (var->data.compact) {
      /* The "compact" flag is only valid on arrays of scalars. */
            const struct glsl_type *type = glsl_get_array_element(var->type);
   if (nir_is_arrayed_io(var, state->shader->info.stage)) {
      if (var->data.per_view) {
      assert(glsl_type_is_array(type));
      }
   assert(glsl_type_is_array(type));
      } else {
                     if (var->num_members > 0) {
      const struct glsl_type *without_array = glsl_without_array(var->type);
   validate_assert(state, glsl_type_is_struct_or_ifc(without_array));
   validate_assert(state, var->num_members == glsl_get_length(without_array));
               if (var->data.per_view)
            if (var->constant_initializer)
            if (var->data.mode == nir_var_image) {
      validate_assert(state, !var->data.bindless);
               if (var->data.per_vertex)
            /*
   * TODO validate some things ir_validate.cpp does (requires more GLSL type
   * support)
            _mesa_hash_table_insert(state->var_defs, var,
               }
      static bool
   validate_ssa_def_dominance(nir_def *def, void *_state)
   {
               validate_assert(state, def->index < state->impl->ssa_alloc);
   validate_assert(state, !BITSET_TEST(state->ssa_defs_found, def->index));
               }
      static bool
   validate_src_dominance(nir_src *src, void *_state)
   {
               if (src->ssa->parent_instr->block == nir_src_parent_instr(src)->block) {
      validate_assert(state, src->ssa->index < state->impl->ssa_alloc);
   validate_assert(state, BITSET_TEST(state->ssa_defs_found,
      } else {
      validate_assert(state, nir_block_dominates(src->ssa->parent_instr->block,
      }
      }
      static void
   validate_ssa_dominance(nir_function_impl *impl, validate_state *state)
   {
               nir_foreach_block(block, impl) {
      state->block = block;
   nir_foreach_instr(instr, block) {
      state->instr = instr;
   if (instr->type == nir_instr_type_phi) {
      nir_phi_instr *phi = nir_instr_as_phi(instr);
   nir_foreach_phi_src(src, phi) {
      validate_assert(state,
               } else {
         }
            }
      static void
   validate_function_impl(nir_function_impl *impl, validate_state *state)
   {
      /* Resize the ssa_srcs set.  It's likely that the size of this set will
   * never actually hit the number of SSA defs because we remove sources from
   * the set as we visit them.  (It could actually be much larger because
   * each SSA def can be used more than once.)  However, growing it now costs
   * us very little (the extra memory is already dwarfed by the SSA defs
   * themselves) and makes collisions much less likely.
   */
            validate_assert(state, impl->function->impl == impl);
            if (impl->preamble) {
      validate_assert(state, impl->function->is_entrypoint);
               validate_assert(state, exec_list_is_empty(&impl->end_block->instr_list));
   validate_assert(state, impl->end_block->successors[0] == NULL);
            state->impl = impl;
            exec_list_validate(&impl->locals);
   nir_foreach_function_temp_variable(var, impl) {
                  state->ssa_defs_found = reralloc(state->mem_ctx, state->ssa_defs_found,
                  _mesa_set_clear(state->blocks, NULL);
   _mesa_set_resize(state->blocks, impl->num_blocks);
   collect_blocks(&impl->body, state);
   _mesa_set_add(state->blocks, impl->end_block);
   validate_assert(state, !exec_list_is_empty(&impl->body));
   foreach_list_typed(nir_cf_node, node, node, &impl->body) {
         }
            validate_assert(state, state->ssa_srcs->entries == 0);
            static int validate_dominance = -1;
   if (validate_dominance < 0) {
      validate_dominance =
      }
   if (validate_dominance) {
      memset(state->ssa_defs_found, 0, BITSET_WORDS(impl->ssa_alloc) * sizeof(BITSET_WORD));
         }
      static void
   validate_function(nir_function *func, validate_state *state)
   {
      if (func->impl != NULL) {
      validate_assert(state, func->impl->function == func);
         }
      static void
   init_validate_state(validate_state *state)
   {
      state->mem_ctx = ralloc_context(NULL);
   state->ssa_srcs = _mesa_pointer_set_create(state->mem_ctx);
   state->ssa_defs_found = NULL;
   state->blocks = _mesa_pointer_set_create(state->mem_ctx);
   state->var_defs = _mesa_pointer_hash_table_create(state->mem_ctx);
            state->loop = NULL;
   state->in_loop_continue_construct = false;
   state->instr = NULL;
      }
      static void
   destroy_validate_state(validate_state *state)
   {
         }
      simple_mtx_t fail_dump_mutex = SIMPLE_MTX_INITIALIZER;
      static void
   dump_errors(validate_state *state, const char *when)
   {
               /* Lock around dumping so that we get clean dumps in a multi-threaded
   * scenario
   */
            if (when) {
      fprintf(stderr, "NIR validation failed %s\n", when);
      } else {
      fprintf(stderr, "NIR validation failed with %d errors:\n",
                        if (_mesa_hash_table_num_entries(errors) > 0) {
      fprintf(stderr, "%d additional errors:\n",
         hash_table_foreach(errors, entry) {
                                 }
      void
   nir_validate_shader(nir_shader *shader, const char *when)
   {
      if (NIR_DEBUG(NOVALIDATE))
            validate_state state;
                     nir_variable_mode valid_modes =
      nir_var_shader_in |
   nir_var_shader_out |
   nir_var_shader_temp |
   nir_var_uniform |
   nir_var_mem_ubo |
   nir_var_system_value |
   nir_var_mem_ssbo |
   nir_var_mem_shared |
   nir_var_mem_global |
   nir_var_mem_push_const |
   nir_var_mem_constant |
         if (gl_shader_stage_is_callable(shader->info.stage))
            if (shader->info.stage == MESA_SHADER_ANY_HIT ||
      shader->info.stage == MESA_SHADER_CLOSEST_HIT ||
   shader->info.stage == MESA_SHADER_INTERSECTION)
         if (shader->info.stage == MESA_SHADER_TASK ||
      shader->info.stage == MESA_SHADER_MESH)
         if (shader->info.stage == MESA_SHADER_COMPUTE)
      valid_modes |= nir_var_mem_node_payload |
         exec_list_validate(&shader->variables);
   nir_foreach_variable_in_shader(var, shader)
            exec_list_validate(&shader->functions);
   foreach_list_typed(nir_function, func, node, &shader->functions) {
                  if (shader->xfb_info != NULL) {
      /* At least validate that, if nir_shader::xfb_info exists, the shader
   * has real transform feedback going on.
   */
   validate_assert(&state, shader->info.stage == MESA_SHADER_VERTEX ||
               validate_assert(&state, shader->xfb_info->buffers_written != 0);
   validate_assert(&state, shader->xfb_info->streams_written != 0);
               if (_mesa_hash_table_num_entries(state.errors) > 0)
               }
      void
   nir_validate_ssa_dominance(nir_shader *shader, const char *when)
   {
      if (NIR_DEBUG(NOVALIDATE))
            validate_state state;
                     nir_foreach_function_impl(impl, shader) {
      state.ssa_defs_found = reralloc(state.mem_ctx, state.ssa_defs_found,
                        state.impl = impl;
               if (_mesa_hash_table_num_entries(state.errors) > 0)
               }
      #endif /* NDEBUG */
