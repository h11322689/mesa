   /*
   * Copyright © 2015 Intel Corporation
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
   */
      #include "util/hash_table.h"
   #include "util/set.h"
   #include "nir.h"
   #include "nir_builder.h"
      /* This file contains various little helpers for doing simple linking in
   * NIR.  Eventually, we'll probably want a full-blown varying packing
   * implementation in here.  Right now, it just deletes unused things.
   */
      /**
   * Returns the bits in the inputs_read, or outputs_written
   * bitfield corresponding to this variable.
   */
   static uint64_t
   get_variable_io_mask(nir_variable *var, gl_shader_stage stage)
   {
      if (var->data.location < 0)
                     assert(var->data.mode == nir_var_shader_in ||
         assert(var->data.location >= 0);
            const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, stage) || var->data.per_view) {
      assert(glsl_type_is_array(type));
               unsigned slots = glsl_count_attribute_slots(type, false);
      }
      static bool
   is_non_generic_patch_var(nir_variable *var)
   {
      return var->data.location == VARYING_SLOT_TESS_LEVEL_INNER ||
         var->data.location == VARYING_SLOT_TESS_LEVEL_OUTER ||
      }
      static uint8_t
   get_num_components(nir_variable *var)
   {
      if (glsl_type_is_struct_or_ifc(glsl_without_array(var->type)))
               }
      static void
   tcs_add_output_reads(nir_shader *shader, uint64_t *read, uint64_t *patches_read)
   {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                  nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                  nir_variable *var = nir_deref_instr_get_variable(deref);
   for (unsigned i = 0; i < get_num_components(var); i++) {
                              patches_read[var->data.location_frac + i] |=
      } else {
      read[var->data.location_frac + i] |=
                     }
      /**
   * Helper for removing unused shader I/O variables, by demoting them to global
   * variables (which may then by dead code eliminated).
   *
   * Example usage is:
   *
   * progress = nir_remove_unused_io_vars(producer, nir_var_shader_out,
   *                                      read, patches_read) ||
   *                                      progress;
   *
   * The "used" should be an array of 4 uint64_ts (probably of VARYING_BIT_*)
   * representing each .location_frac used.  Note that for vector variables,
   * only the first channel (.location_frac) is examined for deciding if the
   * variable is used!
   */
   bool
   nir_remove_unused_io_vars(nir_shader *shader,
                     {
      bool progress = false;
                     nir_foreach_variable_with_modes_safe(var, shader, mode) {
      if (var->data.patch)
         else
            if (var->data.location < VARYING_SLOT_VAR0 && var->data.location >= 0)
                  if (var->data.always_active_io)
            if (var->data.explicit_xfb_buffer)
                     if (!(other_stage & get_variable_io_mask(var, shader->info.stage))) {
      /* This one is invalid, make it a global variable instead */
   if (shader->info.stage == MESA_SHADER_MESH &&
      (shader->info.outputs_read & BITFIELD64_BIT(var->data.location)))
      else
                                 nir_function_impl *impl = nir_shader_get_entrypoint(shader);
   if (progress) {
      nir_metadata_preserve(impl, nir_metadata_dominance |
            } else {
                     }
      bool
   nir_remove_unused_varyings(nir_shader *producer, nir_shader *consumer)
   {
      assert(producer->info.stage != MESA_SHADER_FRAGMENT);
            uint64_t read[4] = { 0 }, written[4] = { 0 };
            nir_foreach_shader_out_variable(var, producer) {
      for (unsigned i = 0; i < get_num_components(var); i++) {
      if (var->data.patch) {
                     patches_written[var->data.location_frac + i] |=
      } else {
      written[var->data.location_frac + i] |=
                     nir_foreach_shader_in_variable(var, consumer) {
      for (unsigned i = 0; i < get_num_components(var); i++) {
      if (var->data.patch) {
                     patches_read[var->data.location_frac + i] |=
      } else {
      read[var->data.location_frac + i] |=
                     /* Each TCS invocation can read data written by other TCS invocations,
   * so even if the outputs are not used by the TES we must also make
   * sure they are not read by the TCS before demoting them to globals.
   */
   if (producer->info.stage == MESA_SHADER_TESS_CTRL)
            bool progress = false;
   progress = nir_remove_unused_io_vars(producer, nir_var_shader_out, read,
            progress = nir_remove_unused_io_vars(consumer, nir_var_shader_in, written,
                     }
      static uint8_t
   get_interp_type(nir_variable *var, const struct glsl_type *type,
         {
      if (var->data.per_primitive)
         if (glsl_type_is_integer(type))
         else if (var->data.interpolation != INTERP_MODE_NONE)
         else if (default_to_smooth_interp)
         else
      }
      #define INTERPOLATE_LOC_SAMPLE   0
   #define INTERPOLATE_LOC_CENTROID 1
   #define INTERPOLATE_LOC_CENTER   2
      static uint8_t
   get_interp_loc(nir_variable *var)
   {
      if (var->data.sample)
         else if (var->data.centroid)
         else
      }
      static bool
   is_packing_supported_for_type(const struct glsl_type *type)
   {
      /* We ignore complex types such as arrays, matrices, structs and bitsizes
   * other then 32bit. All other vector types should have been split into
   * scalar variables by the lower_io_to_scalar pass. The only exception
   * should be OpenGL xfb varyings.
   * TODO: add support for more complex types?
   */
      }
      struct assigned_comps {
      uint8_t comps;
   uint8_t interp_type;
   uint8_t interp_loc;
   bool is_32bit;
   bool is_mediump;
      };
      /* Packing arrays and dual slot varyings is difficult so to avoid complex
   * algorithms this function just assigns them their existing location for now.
   * TODO: allow better packing of complex types.
   */
   static void
   get_unmoveable_components_masks(nir_shader *shader,
                           {
      nir_foreach_variable_with_modes_safe(var, shader, mode) {
               /* Only remap things that aren't built-ins. */
   if (var->data.location >= VARYING_SLOT_VAR0 &&
               const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, stage) || var->data.per_view) {
      assert(glsl_type_is_array(type));
               /* If we can pack this varying then don't mark the components as
   * used.
   */
   if (is_packing_supported_for_type(type) &&
                                          bool dual_slot = glsl_type_is_dual_slot(glsl_without_array(type));
   unsigned slots = glsl_count_attribute_slots(type, false);
   unsigned dmul = glsl_type_is_64bit(glsl_without_array(type)) ? 2 : 1;
   unsigned comps_slot2 = 0;
   for (unsigned i = 0; i < slots; i++) {
      if (dual_slot) {
      if (i & 1) {
                                 /* Assume ARB_enhanced_layouts packing rules for doubles */
                        comps[location + i].comps |=
         } else {
                        comps[location + i].interp_type =
         comps[location + i].interp_loc = get_interp_loc(var);
   comps[location + i].is_32bit =
         comps[location + i].is_mediump =
      var->data.precision == GLSL_PRECISION_MEDIUM ||
                  }
      struct varying_loc {
      uint8_t component;
      };
      static void
   mark_all_used_slots(nir_variable *var, uint64_t *slots_used,
         {
               slots_used[var->data.patch ? 1 : 0] |= slots_used_mask &
      }
      static void
   mark_used_slot(nir_variable *var, uint64_t *slots_used, unsigned offset)
   {
               slots_used[var->data.patch ? 1 : 0] |=
      }
      static void
   remap_slots_and_components(nir_shader *shader, nir_variable_mode mode,
                     {
      const gl_shader_stage stage = shader->info.stage;
   uint64_t out_slots_read_tmp[2] = { 0 };
            /* We don't touch builtins so just copy the bitmask */
            nir_foreach_variable_with_modes(var, shader, mode) {
               /* Only remap things that aren't built-ins */
   if (var->data.location >= VARYING_SLOT_VAR0 &&
               const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, stage) || var->data.per_view) {
      assert(glsl_type_is_array(type));
               unsigned num_slots = glsl_count_attribute_slots(type, false);
                                 unsigned loc_offset = var->data.patch ? VARYING_SLOT_PATCH0 : 0;
   uint64_t used = var->data.patch ? *p_slots_used : *slots_used;
   uint64_t outs_used =
                                                      if (new_loc->location) {
      var->data.location = new_loc->location;
               if (var->data.always_active_io) {
      /* We can't apply link time optimisations (specifically array
   * splitting) to these so we need to copy the existing mask
   * otherwise we will mess up the mask for things like partially
   * marked arrays.
   */
                  if (outputs_read) {
      mark_all_used_slots(var, out_slots_read_tmp, outs_used,
         } else {
      for (unsigned i = 0; i < num_slots; i++) {
                     if (outputs_read)
                        *slots_used = slots_used_tmp[0];
   *out_slots_read = out_slots_read_tmp[0];
   *p_slots_used = slots_used_tmp[1];
      }
      struct varying_component {
      nir_variable *var;
   uint8_t interp_type;
   uint8_t interp_loc;
   bool is_32bit;
   bool is_patch;
   bool is_per_primitive;
   bool is_mediump;
   bool is_intra_stage_only;
      };
      static int
   cmp_varying_component(const void *comp1_v, const void *comp2_v)
   {
      struct varying_component *comp1 = (struct varying_component *)comp1_v;
            /* We want patches to be order at the end of the array */
   if (comp1->is_patch != comp2->is_patch)
            /* Sort per-primitive outputs after per-vertex ones to allow
   * better compaction when they are mixed in the shader's source.
   */
   if (comp1->is_per_primitive != comp2->is_per_primitive)
            /* We want to try to group together TCS outputs that are only read by other
   * TCS invocations and not consumed by the follow stage.
   */
   if (comp1->is_intra_stage_only != comp2->is_intra_stage_only)
            /* Group mediump varyings together. */
   if (comp1->is_mediump != comp2->is_mediump)
            /* We can only pack varyings with matching interpolation types so group
   * them together.
   */
   if (comp1->interp_type != comp2->interp_type)
            /* Interpolation loc must match also. */
   if (comp1->interp_loc != comp2->interp_loc)
            /* If everything else matches just use the original location to sort */
   const struct nir_variable_data *const data1 = &comp1->var->data;
   const struct nir_variable_data *const data2 = &comp2->var->data;
   if (data1->location != data2->location)
            }
      static void
   gather_varying_component_info(nir_shader *producer, nir_shader *consumer,
                     {
      unsigned store_varying_info_idx[MAX_VARYINGS_INCL_PATCH][4] = { { 0 } };
            /* Count the number of varying that can be packed and create a mapping
   * of those varyings to the array we will pass to qsort.
   */
               /* Only remap things that aren't builtins. */
   if (var->data.location >= VARYING_SLOT_VAR0 &&
               /* We can't repack xfb varyings. */
                  const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, producer->info.stage) || var->data.per_view) {
      assert(glsl_type_is_array(type));
                              unsigned loc = var->data.location - VARYING_SLOT_VAR0;
   store_varying_info_idx[loc][var->data.location_frac] =
                  *varying_comp_info_size = num_of_comps_to_pack;
   *varying_comp_info = rzalloc_array(NULL, struct varying_component,
                     /* Walk over the shader and populate the varying component info array */
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_load_deref &&
      intr->intrinsic != nir_intrinsic_interp_deref_at_centroid &&
   intr->intrinsic != nir_intrinsic_interp_deref_at_sample &&
   intr->intrinsic != nir_intrinsic_interp_deref_at_offset &&
               nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
                  /* We only remap things that aren't builtins. */
   nir_variable *in_var = nir_deref_instr_get_variable(deref);
                  /* Do not remap per-vertex shader inputs because it's an array of
   * 3-elements and this isn't supported.
   */
                  unsigned location = in_var->data.location - VARYING_SLOT_VAR0;
                  unsigned var_info_idx =
                                       if (!vc_info->initialised) {
      const struct glsl_type *type = in_var->type;
   if (nir_is_arrayed_io(in_var, consumer->info.stage) ||
      in_var->data.per_view) {
                     vc_info->var = in_var;
   vc_info->interp_type =
         vc_info->interp_loc = get_interp_loc(in_var);
   vc_info->is_32bit = glsl_type_is_32bit(type);
   vc_info->is_patch = in_var->data.patch;
   vc_info->is_per_primitive = in_var->data.per_primitive;
   vc_info->is_mediump = !producer->options->linker_ignore_precision &&
               vc_info->is_intra_stage_only = false;
                     /* Walk over the shader and populate the varying component info array
   * for varyings which are read by other TCS instances but are not consumed
   * by the TES.
   */
   if (producer->info.stage == MESA_SHADER_TESS_CTRL) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
                  /* We only remap things that aren't builtins. */
   nir_variable *out_var = nir_deref_instr_get_variable(deref);
                  unsigned location = out_var->data.location - VARYING_SLOT_VAR0;
                  unsigned var_info_idx =
         if (!var_info_idx) {
      /* Something went wrong, the shader interfaces didn't match, so
   * abandon packing. This can happen for example when the
   * inputs are scalars but the outputs are struct members.
   */
                                    if (!vc_info->initialised) {
      const struct glsl_type *type = out_var->type;
   if (nir_is_arrayed_io(out_var, producer->info.stage)) {
                        vc_info->var = out_var;
   vc_info->interp_type =
         vc_info->interp_loc = get_interp_loc(out_var);
   vc_info->is_32bit = glsl_type_is_32bit(type);
   vc_info->is_patch = out_var->data.patch;
   vc_info->is_per_primitive = out_var->data.per_primitive;
   vc_info->is_mediump = !producer->options->linker_ignore_precision &&
               vc_info->is_intra_stage_only = true;
                        for (unsigned i = 0; i < *varying_comp_info_size; i++) {
      struct varying_component *vc_info = &(*varying_comp_info)[i];
   if (!vc_info->initialised) {
      /* Something went wrong, the shader interfaces didn't match, so
   * abandon packing. This can happen for example when the outputs are
   * scalars but the inputs are struct members.
   */
   *varying_comp_info_size = 0;
            }
      static bool
   allow_pack_interp_type(nir_pack_varying_options options, int type)
   {
               switch (type) {
   case INTERP_MODE_NONE:
      sel = nir_pack_varying_interp_mode_none;
      case INTERP_MODE_SMOOTH:
      sel = nir_pack_varying_interp_mode_smooth;
      case INTERP_MODE_FLAT:
      sel = nir_pack_varying_interp_mode_flat;
      case INTERP_MODE_NOPERSPECTIVE:
      sel = nir_pack_varying_interp_mode_noperspective;
      default:
                     }
      static bool
   allow_pack_interp_loc(nir_pack_varying_options options, int loc)
   {
               switch (loc) {
   case INTERPOLATE_LOC_SAMPLE:
      sel = nir_pack_varying_interp_loc_sample;
      case INTERPOLATE_LOC_CENTROID:
      sel = nir_pack_varying_interp_loc_centroid;
      case INTERPOLATE_LOC_CENTER:
      sel = nir_pack_varying_interp_loc_center;
      default:
                     }
      static void
      assign_remap_locations(struct varying_loc (*remap)[4],
                        struct assigned_comps *assigned_comps,
   {
      unsigned tmp_cursor = *cursor;
                        if (assigned_comps[tmp_cursor].comps) {
      /* Don't pack per-primitive and per-vertex varyings together. */
   if (assigned_comps[tmp_cursor].is_per_primitive != info->is_per_primitive) {
      tmp_comp = 0;
               /* We can only pack varyings with matching precision. */
   if (assigned_comps[tmp_cursor].is_mediump != info->is_mediump) {
      tmp_comp = 0;
               /* We can only pack varyings with matching interpolation type
   * if driver does not support it.
   */
   if (assigned_comps[tmp_cursor].interp_type != info->interp_type &&
      (!allow_pack_interp_type(options, assigned_comps[tmp_cursor].interp_type) ||
   !allow_pack_interp_type(options, info->interp_type))) {
   tmp_comp = 0;
               /* We can only pack varyings with matching interpolation location
   * if driver does not support it.
   */
   if (assigned_comps[tmp_cursor].interp_loc != info->interp_loc &&
      (!allow_pack_interp_loc(options, assigned_comps[tmp_cursor].interp_loc) ||
   !allow_pack_interp_loc(options, info->interp_loc))) {
   tmp_comp = 0;
               /* We can only pack varyings with matching types, and the current
   * algorithm only supports packing 32-bit.
   */
   if (!assigned_comps[tmp_cursor].is_32bit) {
      tmp_comp = 0;
               while (tmp_comp < 4 &&
                           if (tmp_comp == 4) {
      tmp_comp = 0;
                        /* Once we have assigned a location mark it as used */
   assigned_comps[tmp_cursor].comps |= (1 << tmp_comp);
   assigned_comps[tmp_cursor].interp_type = info->interp_type;
   assigned_comps[tmp_cursor].interp_loc = info->interp_loc;
   assigned_comps[tmp_cursor].is_32bit = info->is_32bit;
   assigned_comps[tmp_cursor].is_mediump = info->is_mediump;
            /* Assign remap location */
   remap[location][info->var->data.location_frac].component = tmp_comp++;
   remap[location][info->var->data.location_frac].location =
                        *cursor = tmp_cursor;
      }
      /* If there are empty components in the slot compact the remaining components
   * as close to component 0 as possible. This will make it easier to fill the
   * empty components with components from a different slot in a following pass.
   */
   static void
   compact_components(nir_shader *producer, nir_shader *consumer,
               {
      struct varying_loc remap[MAX_VARYINGS_INCL_PATCH][4] = { { { 0 }, { 0 } } };
   struct varying_component *varying_comp_info;
            /* Gather varying component info */
   gather_varying_component_info(producer, consumer, &varying_comp_info,
                  /* Sort varying components. */
   qsort(varying_comp_info, varying_comp_info_size,
                     unsigned cursor = 0;
            /* Set the remap array based on the sorted components */
   for (unsigned i = 0; i < varying_comp_info_size; i++) {
               assert(info->is_patch || cursor < MAX_VARYING);
   if (info->is_patch) {
      /* The list should be sorted with all non-patch inputs first followed
   * by patch inputs.  When we hit our first patch input, we need to
   * reset the cursor to MAX_VARYING so we put them in the right slot.
   */
   if (cursor < MAX_VARYING) {
      cursor = MAX_VARYING;
               assign_remap_locations(remap, assigned_comps, info,
            } else {
      assign_remap_locations(remap, assigned_comps, info,
                  /* Check if we failed to assign a remap location. This can happen if
   * for example there are a bunch of unmovable components with
   * mismatching interpolation types causing us to skip over locations
   * that would have been useful for packing later components.
   * The solution is to iterate over the locations again (this should
   * happen very rarely in practice).
   */
   if (cursor == MAX_VARYING) {
      cursor = 0;
   comp = 0;
   assign_remap_locations(remap, assigned_comps, info,
                                    uint64_t zero = 0;
   uint32_t zero32 = 0;
   remap_slots_and_components(consumer, nir_var_shader_in, remap,
               remap_slots_and_components(producer, nir_var_shader_out, remap,
                        }
      /* We assume that this has been called more-or-less directly after
   * remove_unused_varyings.  At this point, all of the varyings that we
   * aren't going to be using have been completely removed and the
   * inputs_read and outputs_written fields in nir_shader_info reflect
   * this.  Therefore, the total set of valid slots is the OR of the two
   * sets of varyings;  this accounts for varyings which one side may need
   * to read/write even if the other doesn't.  This can happen if, for
   * instance, an array is used indirectly from one side causing it to be
   * unsplittable but directly from the other.
   */
   void
   nir_compact_varyings(nir_shader *producer, nir_shader *consumer,
         {
      assert(producer->info.stage != MESA_SHADER_FRAGMENT);
                     get_unmoveable_components_masks(producer, nir_var_shader_out,
                     get_unmoveable_components_masks(consumer, nir_var_shader_in,
                        compact_components(producer, consumer, assigned_comps,
      }
      /*
   * Mark XFB varyings as always_active_io in the consumer so the linking opts
   * don't touch them.
   */
   void
   nir_link_xfb_varyings(nir_shader *producer, nir_shader *consumer)
   {
               nir_foreach_shader_in_variable(var, consumer) {
      if (var->data.location >= VARYING_SLOT_VAR0 &&
               unsigned location = var->data.location - VARYING_SLOT_VAR0;
                  nir_foreach_shader_out_variable(var, producer) {
      if (var->data.location >= VARYING_SLOT_VAR0 &&
                              unsigned location = var->data.location - VARYING_SLOT_VAR0;
   if (input_vars[location][var->data.location_frac]) {
                  }
      static bool
   does_varying_match(nir_variable *out_var, nir_variable *in_var)
   {
      return in_var->data.location == out_var->data.location &&
            }
      static nir_variable *
   get_matching_input_var(nir_shader *consumer, nir_variable *out_var)
   {
      nir_foreach_shader_in_variable(var, consumer) {
      if (does_varying_match(out_var, var))
                  }
      static bool
   can_replace_varying(nir_variable *out_var)
   {
      /* Skip types that require more complex handling.
   * TODO: add support for these types.
   */
   if (glsl_type_is_array(out_var->type) ||
      glsl_type_is_dual_slot(out_var->type) ||
   glsl_type_is_matrix(out_var->type) ||
   glsl_type_is_struct_or_ifc(out_var->type))
         /* Limit this pass to scalars for now to keep things simple. Most varyings
   * should have been lowered to scalars at this point anyway.
   */
   if (!glsl_type_is_scalar(out_var->type))
            if (out_var->data.location < VARYING_SLOT_VAR0 ||
      out_var->data.location - VARYING_SLOT_VAR0 >= MAX_VARYING)
            }
      static bool
   replace_varying_input_by_constant_load(nir_shader *shader,
         {
                                 bool progress = false;
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  nir_deref_instr *in_deref = nir_src_as_deref(intr->src[0]);
                                                                  /* Add new const to replace the input */
   nir_def *nconst = nir_build_imm(&b, store_intr->num_components,
                                             }
      static bool
   replace_duplicate_input(nir_shader *shader, nir_variable *input_var,
         {
                                          bool progress = false;
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  nir_deref_instr *in_deref = nir_src_as_deref(intr->src[0]);
                           if (!does_varying_match(dup_out_var, in_var) ||
      in_var->data.interpolation != input_var->data.interpolation ||
   get_interp_loc(in_var) != get_interp_loc(input_var) ||
                                                         }
      static bool
   is_direct_uniform_load(nir_def *def, nir_scalar *s)
   {
      /* def is sure to be scalar as can_replace_varying() filter out vector case. */
            /* Uniform load may hide behind some move instruction for converting
   * vector to scalar:
   *
   *     vec1 32 ssa_1 = deref_var &color (uniform vec3)
   *     vec3 32 ssa_2 = intrinsic load_deref (ssa_1) (0)
   *     vec1 32 ssa_3 = mov ssa_2.x
   *     vec1 32 ssa_4 = deref_var &color_out (shader_out float)
   *     intrinsic store_deref (ssa_4, ssa_3) (1, 0)
   */
            nir_def *ssa = s->def;
   if (ssa->parent_instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(ssa->parent_instr);
   if (intr->intrinsic != nir_intrinsic_load_deref)
            nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
   /* TODO: support nir_var_mem_ubo. */
   if (!nir_deref_mode_is(deref, nir_var_uniform))
            /* Does not support indirect uniform load. */
      }
      /**
   * Add a uniform variable from one shader to a different shader.
   *
   * \param nir     The shader where to add the uniform
   * \param uniform The uniform that's declared in another shader.
   */
   nir_variable *
   nir_clone_uniform_variable(nir_shader *nir, nir_variable *uniform, bool spirv)
   {
      /* Find if uniform already exists in consumer. */
   nir_variable *new_var = NULL;
   nir_foreach_variable_with_modes(v, nir, uniform->data.mode) {
      if ((spirv && uniform->data.mode & nir_var_mem_ubo &&
      v->data.binding == uniform->data.binding) ||
   (!spirv && !strcmp(uniform->name, v->name))) {
   new_var = v;
                  /* Create a variable if not exist. */
   if (!new_var) {
      new_var = nir_variable_clone(uniform, nir);
                  }
      nir_deref_instr *
   nir_clone_deref_instr(nir_builder *b, nir_variable *var,
         {
      if (deref->deref_type == nir_deref_type_var)
            nir_deref_instr *parent_deref = nir_deref_instr_parent(deref);
            /* Build array and struct deref instruction.
   * "deref" instr is sure to be direct (see is_direct_uniform_load()).
   */
   switch (deref->deref_type) {
   case nir_deref_type_array: {
      nir_load_const_instr *index =
            }
   case nir_deref_type_ptr_as_array: {
      nir_load_const_instr *index =
         nir_def *ssa = nir_imm_intN_t(b, index->value->i64,
            }
   case nir_deref_type_struct:
         default:
      unreachable("invalid type");
         }
      static bool
   replace_varying_input_by_uniform_load(nir_shader *shader,
               {
                                 nir_intrinsic_instr *load = nir_instr_as_intrinsic(scalar->def->parent_instr);
   nir_deref_instr *deref = nir_src_as_deref(load->src[0]);
   nir_variable *uni_var = nir_deref_instr_get_variable(deref);
            bool progress = false;
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  nir_deref_instr *in_deref = nir_src_as_deref(intr->src[0]);
                                                   /* Clone instructions start from deref load to variable deref. */
                  /* Add a vector to scalar move if uniform is a vector. */
   if (uni_def->num_components > 1) {
      nir_alu_src src = { 0 };
   src.src = nir_src_for_ssa(uni_def);
   src.swizzle[0] = scalar->comp;
                                                }
      /* The GLSL ES 3.20 spec says:
   *
   * "The precision of a vertex output does not need to match the precision of
   * the corresponding fragment input. The minimum precision at which vertex
   * outputs are interpolated is the minimum of the vertex output precision and
   * the fragment input precision, with the exception that for highp,
   * implementations do not have to support full IEEE 754 precision." (9.1 "Input
   * Output Matching by Name in Linked Programs")
   *
   * To implement this, when linking shaders we will take the minimum precision
   * qualifier (allowing drivers to interpolate at lower precision). For
   * input/output between non-fragment stages (e.g. VERTEX to GEOMETRY), the spec
   * requires we use the *last* specified precision if there is a conflict.
   *
   * Precisions are ordered as (NONE, HIGH, MEDIUM, LOW). If either precision is
   * NONE, we'll return the other precision, since there is no conflict.
   * Otherwise for fragment interpolation, we'll pick the smallest of (HIGH,
   * MEDIUM, LOW) by picking the maximum of the raw values - note the ordering is
   * "backwards". For non-fragment stages, we'll pick the latter precision to
   * comply with the spec. (Note that the order matters.)
   *
   * For streamout, "Variables declared with lowp or mediump precision are
   * promoted to highp before being written." (12.2 "Transform Feedback", p. 341
   * of OpenGL ES 3.2 specification). So drivers should promote them
   * the transform feedback memory store, but not the output store.
   */
      static unsigned
   nir_link_precision(unsigned producer, unsigned consumer, bool fs)
   {
      if (producer == GLSL_PRECISION_NONE)
         else if (consumer == GLSL_PRECISION_NONE)
         else
      }
      static nir_variable *
   find_consumer_variable(const nir_shader *consumer,
         {
      nir_foreach_variable_with_modes(var, consumer, nir_var_shader_in) {
      if (var->data.location == producer_var->data.location &&
      var->data.location_frac == producer_var->data.location_frac)
   }
      }
      void
   nir_link_varying_precision(nir_shader *producer, nir_shader *consumer)
   {
               nir_foreach_shader_out_variable(producer_var, producer) {
      /* Skip if the slot is not assigned */
   if (producer_var->data.location < 0)
            nir_variable *consumer_var = find_consumer_variable(consumer,
            /* Skip if the variable will be eliminated */
   if (!consumer_var)
            /* Now we have a pair of variables. Let's pick the smaller precision. */
   unsigned precision_1 = producer_var->data.precision;
   unsigned precision_2 = consumer_var->data.precision;
            /* Propagate the new precision */
         }
      bool
   nir_link_opt_varyings(nir_shader *producer, nir_shader *consumer)
   {
      /* TODO: Add support for more shader stage combinations */
   if (consumer->info.stage != MESA_SHADER_FRAGMENT ||
      (producer->info.stage != MESA_SHADER_VERTEX &&
   producer->info.stage != MESA_SHADER_TESS_EVAL))
                                    /* If we find a store in the last block of the producer we can be sure this
   * is the only possible value for this output.
   */
   nir_block *last_block = nir_impl_last_block(impl);
   nir_foreach_instr_reverse(instr, last_block) {
      if (instr->type != nir_instr_type_intrinsic)
                     if (intr->intrinsic != nir_intrinsic_store_deref)
            nir_deref_instr *out_deref = nir_src_as_deref(intr->src[0]);
   if (!nir_deref_mode_is(out_deref, nir_var_shader_out))
            nir_variable *out_var = nir_deref_instr_get_variable(out_deref);
   if (!can_replace_varying(out_var))
            nir_def *ssa = intr->src[1].ssa;
   if (ssa->parent_instr->type == nir_instr_type_load_const) {
      progress |= replace_varying_input_by_constant_load(consumer, intr);
               nir_scalar uni_scalar;
   if (is_direct_uniform_load(ssa, &uni_scalar)) {
      if (consumer->options->lower_varying_from_uniform) {
      progress |= replace_varying_input_by_uniform_load(consumer, intr,
            } else {
      nir_variable *in_var = get_matching_input_var(consumer, out_var);
   /* The varying is loaded from same uniform, so no need to do any
   * interpolation. Mark it as flat explicitly.
   */
   if (!consumer->options->no_integers &&
      in_var && in_var->data.interpolation <= INTERP_MODE_NOPERSPECTIVE) {
   in_var->data.interpolation = INTERP_MODE_FLAT;
                     struct hash_entry *entry = _mesa_hash_table_search(varying_values, ssa);
   if (entry) {
      progress |= replace_duplicate_input(consumer,
            } else {
      nir_variable *in_var = get_matching_input_var(consumer, out_var);
   if (in_var) {
                                    }
      /* TODO any better helper somewhere to sort a list? */
      static void
   insert_sorted(struct exec_list *var_list, nir_variable *new_var)
   {
      nir_foreach_variable_in_list(var, var_list) {
      /* Use the `per_primitive` bool to sort per-primitive variables
   * to the end of the list, so they get the last driver locations
   * by nir_assign_io_var_locations.
   *
   * This is done because AMD HW requires that per-primitive outputs
   * are the last params.
   * In the future we can add an option for this, if needed by other HW.
   */
   if (new_var->data.per_primitive < var->data.per_primitive ||
      (new_var->data.per_primitive == var->data.per_primitive &&
   (var->data.location > new_var->data.location ||
      (var->data.location == new_var->data.location &&
      exec_node_insert_node_before(&var->node, &new_var->node);
         }
      }
      static void
   sort_varyings(nir_shader *shader, nir_variable_mode mode,
         {
      exec_list_make_empty(sorted_list);
   nir_foreach_variable_with_modes_safe(var, shader, mode) {
      exec_node_remove(&var->node);
         }
      void
   nir_sort_variables_by_location(nir_shader *shader, nir_variable_mode mode)
   {
               sort_varyings(shader, mode, &vars);
      }
      void
   nir_assign_io_var_locations(nir_shader *shader, nir_variable_mode mode,
         {
      unsigned location = 0;
   unsigned assigned_locations[VARYING_SLOT_TESS_MAX];
            struct exec_list io_vars;
            int ASSERTED last_loc = 0;
   bool ASSERTED last_per_prim = false;
   bool last_partial = false;
   nir_foreach_variable_in_list(var, &io_vars) {
      const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, stage)) {
      assert(glsl_type_is_array(type));
               int base;
   if (var->data.mode == nir_var_shader_in && stage == MESA_SHADER_VERTEX)
         else if (var->data.mode == nir_var_shader_out &&
               else
            unsigned var_size, driver_size;
   if (var->data.compact) {
      /* If we are inside a partial compact,
   * don't allow another compact to be in this slot
   * if it starts at component 0.
   */
   if (last_partial && var->data.location_frac == 0) {
                  /* compact variables must be arrays of scalars */
   assert(!var->data.per_view);
   assert(glsl_type_is_array(type));
   assert(glsl_type_is_scalar(glsl_get_array_element(type)));
   unsigned start = 4 * location + var->data.location_frac;
   unsigned end = start + glsl_get_length(type);
   var_size = driver_size = end / 4 - location;
      } else {
      /* Compact variables bypass the normal varying compacting pass,
   * which means they cannot be in the same vec4 slot as a normal
   * variable. If part of the current slot is taken up by a compact
   * variable, we need to go to the next one.
   */
   if (last_partial) {
      location++;
               /* per-view variables have an extra array dimension, which is ignored
   * when counting user-facing slots (var->data.location), but *not*
   * with driver slots (var->data.driver_location). That is, each user
   * slot maps to multiple driver slots.
   */
   driver_size = glsl_count_attribute_slots(type, false);
   if (var->data.per_view) {
      assert(glsl_type_is_array(type));
   var_size =
      } else {
                     /* Builtins don't allow component packing so we only need to worry about
   * user defined varyings sharing the same location.
   */
   bool processed = false;
   if (var->data.location >= base) {
               for (unsigned i = 0; i < var_size; i++) {
      if (processed_locs[var->data.index] &
      ((uint64_t)1 << (glsl_location + i)))
      else
      processed_locs[var->data.index] |=
               /* Because component packing allows varyings to share the same location
   * we may have already have processed this location.
   */
   if (processed) {
      /* TODO handle overlapping per-view variables */
   assert(!var->data.per_view);
                  /* An array may be packed such that is crosses multiple other arrays
   * or variables, we need to make sure we have allocated the elements
   * consecutively if the previously proccessed var was shorter than
   * the current array we are processing.
   *
   * NOTE: The code below assumes the var list is ordered in ascending
   * location order, but per-vertex/per-primitive outputs may be
   * grouped separately.
   */
   assert(last_loc <= var->data.location ||
         last_loc = var->data.location;
   last_per_prim = var->data.per_primitive;
   unsigned last_slot_location = driver_location + var_size;
   if (last_slot_location > location) {
      unsigned num_unallocated_slots = last_slot_location - location;
   unsigned first_unallocated_slot = var_size - num_unallocated_slots;
   for (unsigned i = first_unallocated_slot; i < var_size; i++) {
      assigned_locations[var->data.location + i] = location;
         }
               for (unsigned i = 0; i < var_size; i++) {
                  var->data.driver_location = location;
               if (last_partial)
            exec_list_append(&shader->variables, &io_vars);
      }
      static uint64_t
   get_linked_variable_location(unsigned location, bool patch)
   {
      if (!patch)
            /* Reserve locations 0...3 for special patch variables
   * like tess factors and bounding boxes, and the generic patch
   * variables will come after them.
   */
   if (location >= VARYING_SLOT_PATCH0)
         else if (location >= VARYING_SLOT_TESS_LEVEL_OUTER &&
               else
      }
      static uint64_t
   get_linked_variable_io_mask(nir_variable *variable, gl_shader_stage stage)
   {
               if (nir_is_arrayed_io(variable, stage)) {
      assert(glsl_type_is_array(type));
               unsigned slots = glsl_count_attribute_slots(type, false);
   if (variable->data.compact) {
      unsigned component_count = variable->data.location_frac + glsl_get_length(type);
               uint64_t mask = u_bit_consecutive64(0, slots);
      }
      nir_linked_io_var_info
   nir_assign_linked_io_var_locations(nir_shader *producer, nir_shader *consumer)
   {
      assert(producer);
            uint64_t producer_output_mask = 0;
            nir_foreach_shader_out_variable(variable, producer) {
      uint64_t mask = get_linked_variable_io_mask(variable, producer->info.stage);
            if (variable->data.patch)
         else
               uint64_t consumer_input_mask = 0;
            nir_foreach_shader_in_variable(variable, consumer) {
      uint64_t mask = get_linked_variable_io_mask(variable, consumer->info.stage);
            if (variable->data.patch)
         else
               uint64_t io_mask = producer_output_mask | consumer_input_mask;
            nir_foreach_shader_out_variable(variable, producer) {
               if (variable->data.patch)
         else
               nir_foreach_shader_in_variable(variable, consumer) {
               if (variable->data.patch)
         else
               nir_linked_io_var_info result = {
      .num_linked_io_vars = util_bitcount64(io_mask),
                  }
