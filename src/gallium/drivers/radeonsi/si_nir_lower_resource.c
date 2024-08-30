   /*
   * Copyright 2022 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      /*
   * This lowering pass converts index based buffer/image/texture access to
   * explicite descriptor based, which simplify the compiler backend translation.
   *
   * For example: load_ubo(1) -> load_ubo(vec4), where the vec4 is the buffer
   * descriptor with index==1, so compiler backend don't need to do index-to-descriptor
   * finding which is the most complicated part (move to nir now).
   */
      #include "nir_builder.h"
      #include "ac_nir.h"
   #include "si_pipe.h"
   #include "si_shader_internal.h"
   #include "sid.h"
      struct lower_resource_state {
      struct si_shader *shader;
      };
      static nir_def *load_ubo_desc_fast_path(nir_builder *b, nir_def *addr_lo,
         {
      nir_def *addr_hi =
            uint32_t rsrc3 =
      S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) | S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
         if (sel->screen->info.gfx_level >= GFX11)
      rsrc3 |= S_008F0C_FORMAT(V_008F0C_GFX11_FORMAT_32_FLOAT) |
      else if (sel->screen->info.gfx_level >= GFX10)
      rsrc3 |= S_008F0C_FORMAT(V_008F0C_GFX10_FORMAT_32_FLOAT) |
      else
      rsrc3 |= S_008F0C_NUM_FORMAT(V_008F0C_BUF_NUM_FORMAT_FLOAT) |
         return nir_vec4(b, addr_lo, addr_hi, nir_imm_int(b, sel->info.constbuf0_num_slots * 16),
      }
      static nir_def *clamp_index(nir_builder *b, nir_def *index, unsigned max)
   {
      if (util_is_power_of_two_or_zero(max))
         else {
      nir_def *clamp = nir_imm_int(b, max - 1);
   nir_def *cond = nir_uge(b, clamp, index);
         }
      static nir_def *load_ubo_desc(nir_builder *b, nir_def *index,
         {
                        if (sel->info.base.num_ubos == 1 && sel->info.base.num_ssbos == 0)
            index = clamp_index(b, index, sel->info.base.num_ubos);
            nir_def *offset = nir_ishl_imm(b, index, 4);
      }
      static nir_def *load_ssbo_desc(nir_builder *b, nir_src *index,
         {
               /* Fast path if the shader buffer is in user SGPRs. */
   if (nir_src_is_const(*index)) {
      unsigned slot = nir_src_as_uint(*index);
   if (slot < sel->cs_num_shaderbufs_in_user_sgprs)
               nir_def *addr = ac_nir_load_arg(b, &s->args->ac, s->args->const_and_shader_buffers);
   nir_def *slot = clamp_index(b, index->ssa, sel->info.base.num_ssbos);
            nir_def *offset = nir_ishl_imm(b, slot, 4);
      }
      static nir_def *fixup_image_desc(nir_builder *b, nir_def *rsrc, bool uses_store,
         {
      struct si_shader_selector *sel = s->shader->selector;
            /**
   * Given a 256-bit resource descriptor, force the DCC enable bit to off.
   *
   * At least on Tonga, executing image stores on images with DCC enabled and
   * non-trivial can eventually lead to lockups. This can occur when an
   * application binds an image as read-only but then uses a shader that writes
   * to it. The OpenGL spec allows almost arbitrarily bad behavior (including
   * program termination) in this case, but it doesn't cost much to be a bit
   * nicer: disabling DCC in the shader still leads to undefined results but
   * avoids the lockup.
   */
   if (uses_store &&
      screen->info.gfx_level <= GFX9 &&
   screen->info.gfx_level >= GFX8) {
   nir_def *tmp = nir_channel(b, rsrc, 6);
   tmp = nir_iand_imm(b, tmp, C_008F28_COMPRESSION_EN);
               if (!uses_store &&
      screen->info.has_image_load_dcc_bug &&
   screen->always_allow_dcc_stores) {
   nir_def *tmp = nir_channel(b, rsrc, 6);
   tmp = nir_iand_imm(b, tmp, C_00A018_WRITE_COMPRESS_ENABLE);
                  }
      /* AC_DESC_FMASK is handled exactly like AC_DESC_IMAGE. The caller should
   * adjust "index" to point to FMASK.
   */
   static nir_def *load_image_desc(nir_builder *b, nir_def *list, nir_def *index,
               {
      /* index is in uvec8 unit, convert to offset in bytes */
            unsigned num_channels;
   if (desc_type == AC_DESC_BUFFER) {
      offset = nir_iadd_imm(b, offset, 16);
      } else {
      assert(desc_type == AC_DESC_IMAGE || desc_type == AC_DESC_FMASK);
                        if (desc_type == AC_DESC_IMAGE)
               }
      static nir_def *deref_to_index(nir_builder *b,
                           {
      unsigned const_index = 0;
   nir_def *dynamic_index = NULL;
   while (deref->deref_type != nir_deref_type_var) {
      assert(deref->deref_type == nir_deref_type_array);
            if (nir_src_is_const(deref->arr.index)) {
         } else {
      nir_def *tmp = nir_imul_imm(b, deref->arr.index.ssa, array_size);
                           unsigned base_index = deref->var->data.binding;
            /* Redirect invalid resource indices to the first array element. */
   if (const_index >= max_slots)
            nir_def *index = nir_imm_int(b, const_index);
   if (dynamic_index) {
               /* From the GL_ARB_shader_image_load_store extension spec:
   *
   *    If a shader performs an image load, store, or atomic
   *    operation using an image variable declared as an array,
   *    and if the index used to select an individual element is
   *    negative or greater than or equal to the size of the
   *    array, the results of the operation are undefined but may
   *    not lead to termination.
   */
               if (dynamic_index_ret)
         if (const_index_ret)
               }
      static nir_def *load_deref_image_desc(nir_builder *b, nir_deref_instr *deref,
               {
      unsigned const_index;
   nir_def *dynamic_index;
   nir_def *index = deref_to_index(b, deref, s->shader->selector->info.base.num_images,
            nir_def *desc;
   if (!dynamic_index && desc_type != AC_DESC_FMASK &&
      const_index < s->shader->selector->cs_num_images_in_user_sgprs) {
   /* Fast path if the image is in user SGPRs. */
            if (desc_type == AC_DESC_IMAGE)
      } else {
      /* FMASKs are separate from images. */
   if (desc_type == AC_DESC_FMASK)
                     nir_def *list = ac_nir_load_arg(b, &s->args->ac, s->args->samplers_and_images);
                  }
      static nir_def *load_bindless_image_desc(nir_builder *b, nir_def *index,
               {
      /* Bindless image descriptors use 16-dword slots. */
            /* FMASK is right after the image. */
   if (desc_type == AC_DESC_FMASK)
            nir_def *list = ac_nir_load_arg(b, &s->args->ac, s->args->bindless_samplers_and_images);
      }
      static bool lower_resource_intrinsic(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      switch (intrin->intrinsic) {
   case nir_intrinsic_load_ubo: {
               nir_def *desc = load_ubo_desc(b, intrin->src[0].ssa, s);
   nir_src_rewrite(&intrin->src[0], desc);
      }
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap: {
               nir_def *desc = load_ssbo_desc(b, &intrin->src[0], s);
   nir_src_rewrite(&intrin->src[0], desc);
      }
   case nir_intrinsic_store_ssbo: {
               nir_def *desc = load_ssbo_desc(b, &intrin->src[1], s);
   nir_src_rewrite(&intrin->src[1], desc);
      }
   case nir_intrinsic_get_ssbo_size: {
               nir_def *desc = load_ssbo_desc(b, &intrin->src[0], s);
   nir_def *size = nir_channel(b, desc, 2);
   nir_def_rewrite_uses(&intrin->def, size);
   nir_instr_remove(&intrin->instr);
      }
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_sparse_load:
   case nir_intrinsic_image_deref_fragment_mask_load_amd:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_descriptor_amd: {
                        enum ac_descriptor_type desc_type;
   if (intrin->intrinsic == nir_intrinsic_image_deref_fragment_mask_load_amd) {
         } else {
      enum glsl_sampler_dim dim = glsl_get_sampler_dim(deref->type);
               bool is_load =
      intrin->intrinsic == nir_intrinsic_image_deref_load ||
   intrin->intrinsic == nir_intrinsic_image_deref_sparse_load ||
                        if (intrin->intrinsic == nir_intrinsic_image_deref_descriptor_amd) {
      nir_def_rewrite_uses(&intrin->def, desc);
      } else {
      nir_intrinsic_set_image_dim(intrin, glsl_get_sampler_dim(deref->type));
   nir_intrinsic_set_image_array(intrin, glsl_sampler_type_is_array(deref->type));
      }
      }
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_sparse_load:
   case nir_intrinsic_bindless_image_fragment_mask_load_amd:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap: {
               enum ac_descriptor_type desc_type;
   if (intrin->intrinsic == nir_intrinsic_bindless_image_fragment_mask_load_amd) {
         } else {
      enum glsl_sampler_dim dim = nir_intrinsic_image_dim(intrin);
               bool is_load =
      intrin->intrinsic == nir_intrinsic_bindless_image_load ||
   intrin->intrinsic == nir_intrinsic_bindless_image_sparse_load ||
                                 if (intrin->intrinsic == nir_intrinsic_bindless_image_descriptor_amd) {
      nir_def_rewrite_uses(&intrin->def, desc);
      } else {
         }
      }
   default:
                     }
      static nir_def *load_sampler_desc(nir_builder *b, nir_def *list, nir_def *index,
         {
      /* index is in 16 dword unit, convert to offset in bytes */
            unsigned num_channels = 0;
   switch (desc_type) {
   case AC_DESC_IMAGE:
      /* The image is at [0:7]. */
   num_channels = 8;
      case AC_DESC_BUFFER:
      /* The buffer is in [4:7]. */
   offset = nir_iadd_imm(b, offset, 16);
   num_channels = 4;
      case AC_DESC_FMASK:
      /* The FMASK is at [8:15]. */
   offset = nir_iadd_imm(b, offset, 32);
   num_channels = 8;
      case AC_DESC_SAMPLER:
      /* The sampler state is at [12:15]. */
   offset = nir_iadd_imm(b, offset, 48);
   num_channels = 4;
      default:
      unreachable("invalid desc type");
                  }
      static nir_def *load_deref_sampler_desc(nir_builder *b, nir_deref_instr *deref,
                     {
      unsigned max_slots = BITSET_LAST_BIT(b->shader->info.textures_used);
   nir_def *index = deref_to_index(b, deref, max_slots, NULL, NULL);
            /* return actual desc when required by caller */
   if (return_descriptor) {
      nir_def *list = ac_nir_load_arg(b, &s->args->ac, s->args->samplers_and_images);
               /* Just use index here and let nir-to-llvm backend to translate to actual
   * descriptor. This is because we need waterfall to handle non-dynamic-uniform
   * index there.
   */
      }
      static nir_def *load_bindless_sampler_desc(nir_builder *b, nir_def *index,
               {
               /* 64 bit to 32 bit */
               }
      static nir_def *fixup_sampler_desc(nir_builder *b,
                     {
               if (tex->op != nir_texop_tg4 || sel->screen->info.conformant_trunc_coord)
            /* Set TRUNC_COORD=0 for textureGather(). */
   nir_def *dword0 = nir_channel(b, sampler, 0);
   dword0 = nir_iand_imm(b, dword0, C_008F30_TRUNC_COORD);
   sampler = nir_vector_insert_imm(b, sampler, dword0, 0);
      }
      static bool lower_resource_tex(nir_builder *b, nir_tex_instr *tex,
         {
      nir_deref_instr *texture_deref = NULL;
   nir_deref_instr *sampler_deref = NULL;
   nir_def *texture_handle = NULL;
            for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_texture_deref:
      texture_deref = nir_src_as_deref(tex->src[i].src);
      case nir_tex_src_sampler_deref:
      sampler_deref = nir_src_as_deref(tex->src[i].src);
      case nir_tex_src_texture_handle:
      texture_handle = tex->src[i].src.ssa;
      case nir_tex_src_sampler_handle:
      sampler_handle = tex->src[i].src.ssa;
      default:
                     enum ac_descriptor_type desc_type;
   if (tex->op == nir_texop_fragment_mask_fetch_amd)
         else
            if (tex->op == nir_texop_descriptor_amd) {
      nir_def *image;
   if (texture_deref)
         else
         nir_def_rewrite_uses(&tex->def, image);
   nir_instr_remove(&tex->instr);
               if (tex->op == nir_texop_sampler_descriptor_amd) {
      nir_def *sampler;
   if (sampler_deref)
         else
         nir_def_rewrite_uses(&tex->def, sampler);
   nir_instr_remove(&tex->instr);
               nir_def *image = texture_deref ?
      load_deref_sampler_desc(b, texture_deref, desc_type, s, !tex->texture_non_uniform) :
         nir_def *sampler = NULL;
   if (sampler_deref)
         else if (sampler_handle)
            if (sampler && sampler->num_components > 1)
            for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_texture_deref:
      tex->src[i].src_type = nir_tex_src_texture_handle;
      case nir_tex_src_texture_handle:
      nir_src_rewrite(&tex->src[i].src, image);
      case nir_tex_src_sampler_deref:
      tex->src[i].src_type = nir_tex_src_sampler_handle;
      case nir_tex_src_sampler_handle:
      nir_src_rewrite(&tex->src[i].src, sampler);
      default:
                        }
      static bool lower_resource_instr(nir_builder *b, nir_instr *instr, void *state)
   {
                        switch (instr->type) {
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
      }
   case nir_instr_type_tex: {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
      }
   default:
            }
      bool si_nir_lower_resource(nir_shader *nir, struct si_shader *shader,
         {
      struct lower_resource_state state = {
      .shader = shader,
               return nir_shader_instructions_pass(nir, lower_resource_instr,
            }
