   /* SPDX-License-Identifier: MIT */
      #include "nir.h"
   #include "nir_builder.h"
      static bool
   var_is_inline_sampler(const nir_variable *var)
   {
      if (var->data.mode != nir_var_uniform)
            return glsl_type_is_sampler(var->type) &&
      }
      static bool
   inline_sampler_vars_equal(const nir_variable *a, const nir_variable *b)
   {
               if (a == b)
            return a->data.sampler.addressing_mode == b->data.sampler.addressing_mode &&
            }
      static nir_variable *
   find_identical_inline_sampler(nir_shader *nir,
               {
      nir_foreach_variable_in_list(var, inline_samplers) {
      if (inline_sampler_vars_equal(var, sampler))
               nir_foreach_uniform_variable(var, nir) {
      if (!var_is_inline_sampler(var) ||
                  exec_node_remove(&var->node);
   exec_list_push_tail(inline_samplers, &var->node);
      }
      }
      static bool
   nir_dedup_inline_samplers_instr(nir_builder *b,
               {
               if (instr->type != nir_instr_type_deref)
            nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (deref->deref_type != nir_deref_type_var)
            nir_variable *sampler = nir_deref_instr_get_variable(deref);
   if (!var_is_inline_sampler(sampler))
            nir_variable *replacement =
         deref->var = replacement;
      }
      /** De-duplicates inline sampler variables
   *
   * Any dead or redundant inline sampler variables are removed any live inline
   * sampler variables are placed at the end of the variables list.
   */
   bool
   nir_dedup_inline_samplers(nir_shader *nir)
   {
      struct exec_list inline_samplers;
            nir_shader_instructions_pass(nir, nir_dedup_inline_samplers_instr,
                        /* If we found any inline samplers in the instructions pass, they'll now be
   * in the inline_samplers list.
   */
            /* Remove any dead samplers */
   nir_foreach_uniform_variable_safe(var, nir) {
      if (var_is_inline_sampler(var)) {
      exec_node_remove(&var->node);
                  exec_node_insert_list_after(exec_list_get_tail(&nir->variables),
               }
      bool
   nir_lower_cl_images(nir_shader *shader, bool lower_image_derefs, bool lower_sampler_derefs)
   {
               ASSERTED int last_loc = -1;
            BITSET_ZERO(shader->info.image_buffers);
   BITSET_ZERO(shader->info.msaa_images);
   nir_foreach_variable_with_modes(var, shader, nir_var_image | nir_var_uniform) {
      if (!glsl_type_is_image(var->type) && !glsl_type_is_texture(var->type))
            /* Assume they come in order */
   assert(var->data.location > last_loc);
            assert(glsl_type_is_image(var->type) || var->data.access & ACCESS_NON_WRITEABLE);
   if (var->data.access & ACCESS_NON_WRITEABLE)
         else
                  switch (glsl_get_sampler_dim(var->type)) {
   case GLSL_SAMPLER_DIM_BUF:
      BITSET_SET(shader->info.image_buffers, var->data.binding);
      case GLSL_SAMPLER_DIM_MS:
      BITSET_SET(shader->info.msaa_images, var->data.binding);
      default:
            }
   shader->info.num_textures = num_rd_images;
   BITSET_ZERO(shader->info.textures_used);
   if (num_rd_images)
            BITSET_ZERO(shader->info.images_used);
   if (num_wr_images)
                  last_loc = -1;
   int num_samplers = 0;
   nir_foreach_uniform_variable(var, shader) {
      if (var->type == glsl_bare_sampler_type()) {
      /* Assume they come in order */
   assert(var->data.location > last_loc);
   last_loc = var->data.location;
      } else {
      /* CL shouldn't have any sampled images */
         }
   BITSET_ZERO(shader->info.samplers_used);
   if (num_samplers)
                     /* don't need any lowering if we can keep the derefs */
   if (!lower_image_derefs && !lower_sampler_derefs) {
      nir_metadata_preserve(impl, nir_metadata_all);
               bool progress = false;
   nir_foreach_block_reverse(block, impl) {
      nir_foreach_instr_reverse_safe(instr, block) {
      switch (instr->type) {
   case nir_instr_type_deref: {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
                  if (!glsl_type_is_image(deref->type) &&
                                       if (!lower_sampler_derefs &&
                  b.cursor = nir_instr_remove(&deref->instr);
   nir_def *loc =
      nir_imm_intN_t(&b, deref->var->data.driver_location,
      nir_def_rewrite_uses(&deref->def, loc);
   progress = true;
               case nir_instr_type_tex: {
                     nir_tex_instr *tex = nir_instr_as_tex(instr);
   unsigned count = 0;
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if (tex->src[i].src_type == nir_tex_src_texture_deref ||
      tex->src[i].src_type == nir_tex_src_sampler_deref) {
   nir_deref_instr *deref = nir_src_as_deref(tex->src[i].src);
   if (deref->deref_type == nir_deref_type_var) {
      /* In this case, we know the actual variable */
   if (tex->src[i].src_type == nir_tex_src_texture_deref)
         else
         /* This source gets discarded */
   nir_instr_clear_src(&tex->instr, &tex->src[i].src);
      } else {
      b.cursor = nir_before_instr(&tex->instr);
   /* Back-ends expect a 32-bit thing, not 64-bit */
   nir_def *offset = nir_u2u32(&b, tex->src[i].src.ssa);
   if (tex->src[i].src_type == nir_tex_src_texture_deref)
         else
               } else {
      /* If we've removed a source, move this one down */
   if (count != i) {
      assert(count < i);
   tex->src[count].src_type = tex->src[i].src_type;
   nir_instr_move_src(&tex->instr, &tex->src[count].src,
         }
      }
   tex->num_srcs = count;
   progress = true;
               case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples: {
                     b.cursor = nir_before_instr(&intrin->instr);
   /* Back-ends expect a 32-bit thing, not 64-bit */
   nir_def *offset = nir_u2u32(&b, intrin->src[0].ssa);
   nir_rewrite_image_intrinsic(intrin, offset, false);
                     default:
         }
               default:
                        if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
