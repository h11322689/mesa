   /*
   * Copyright 2013 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      /* Resource binding slots and sampler states (each described with 8 or
   * 4 dwords) are stored in lists in memory which is accessed by shaders
   * using scalar load instructions.
   *
   * This file is responsible for managing such lists. It keeps a copy of all
   * descriptors in CPU memory and re-uploads a whole list if some slots have
   * been changed.
   *
   * This code is also responsible for updating shader pointers to those lists.
   *
   * Note that CP DMA can't be used for updating the lists, because a GPU hang
   * could leave the list in a mid-IB state and the next IB would get wrong
   * descriptors and the whole context would be unusable at that point.
   * (Note: The register shadowing can't be used due to the same reason)
   *
   * Also, uploading descriptors to newly allocated memory doesn't require
   * a KCACHE flush.
   *
   *
   * Possible scenarios for one 16 dword image+sampler slot:
   *
   *       | Image        | w/ FMASK   | Buffer       | NULL
   * [ 0: 3] Image[0:3]   | Image[0:3] | Null[0:3]    | Null[0:3]
   * [ 4: 7] Image[4:7]   | Image[4:7] | Buffer[0:3]  | 0
   * [ 8:11] Null[0:3]    | Fmask[0:3] | Null[0:3]    | Null[0:3]
   * [12:15] Sampler[0:3] | Fmask[4:7] | Sampler[0:3] | Sampler[0:3]
   *
   * FMASK implies MSAA, therefore no sampler state.
   * Sampler states are never unbound except when FMASK is bound.
   */
      #include "si_pipe.h"
   #include "si_build_pm4.h"
   #include "sid.h"
   #include "util/format/u_format.h"
   #include "util/hash_table.h"
   #include "util/u_idalloc.h"
   #include "util/u_memory.h"
   #include "util/u_upload_mgr.h"
      /* NULL image and buffer descriptor for textures (alpha = 1) and images
   * (alpha = 0).
   *
   * For images, all fields must be zero except for the swizzle, which
   * supports arbitrary combinations of 0s and 1s. The texture type must be
   * any valid type (e.g. 1D). If the texture type isn't set, the hw hangs.
   *
   * For buffers, all fields must be zero. If they are not, the hw hangs.
   *
   * This is the only reason why the buffer descriptor must be in words [4:7].
   */
   static uint32_t null_texture_descriptor[8] = {
      0, 0, 0, S_008F1C_DST_SEL_W(V_008F1C_SQ_SEL_1) | S_008F1C_TYPE(V_008F1C_SQ_RSRC_IMG_1D)
   /* the rest must contain zeros, which is also used by the buffer
      };
      static uint32_t null_image_descriptor[8] = {
      0, 0, 0, S_008F1C_TYPE(V_008F1C_SQ_RSRC_IMG_1D)
   /* the rest must contain zeros, which is also used by the buffer
      };
      static uint64_t si_desc_extract_buffer_address(const uint32_t *desc)
   {
               /* Sign-extend the 48-bit address. */
   va <<= 16;
   va = (int64_t)va >> 16;
      }
      static void si_init_descriptor_list(uint32_t *desc_list, unsigned element_dw_size,
         {
               /* Initialize the array to NULL descriptors if the element size is 8. */
   if (null_descriptor) {
      assert(element_dw_size % 8 == 0);
   for (i = 0; i < num_elements * element_dw_size / 8; i++)
         }
      static void si_init_descriptors(struct si_descriptors *desc, short shader_userdata_rel_index,
         {
      desc->list = CALLOC(num_elements, element_dw_size * 4);
   desc->element_dw_size = element_dw_size;
   desc->num_elements = num_elements;
   desc->shader_userdata_offset = shader_userdata_rel_index * 4;
      }
      static void si_release_descriptors(struct si_descriptors *desc)
   {
      si_resource_reference(&desc->buffer, NULL);
      }
      static void si_upload_descriptors(struct si_context *sctx, struct si_descriptors *desc)
   {
      unsigned slot_size = desc->element_dw_size * 4;
   unsigned first_slot_offset = desc->first_active_slot * slot_size;
            /* Skip the upload if no shader is using the descriptors. dirty_mask
   * will stay dirty and the descriptors will be uploaded when there is
   * a shader using them.
   */
   if (!upload_size)
            /* If there is just one active descriptor, bind it directly. */
   if ((int)desc->first_active_slot == desc->slot_index_to_bind_directly &&
      desc->num_active_slots == 1) {
            /* The buffer is already in the buffer list. */
   si_resource_reference(&desc->buffer, NULL);
   desc->gpu_list = NULL;
   desc->gpu_address = si_desc_extract_buffer_address(descriptor);
               uint32_t *ptr;
   unsigned buffer_offset;
   u_upload_alloc(sctx->b.const_uploader, first_slot_offset, upload_size,
               if (!desc->buffer) {
      sctx->ws->ctx_set_sw_reset_status(sctx->ctx, PIPE_GUILTY_CONTEXT_RESET,
                     util_memcpy_cpu_to_le32(ptr, (char *)desc->list + first_slot_offset, upload_size);
            radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, desc->buffer,
            /* The shader pointer should point to slot 0. */
   buffer_offset -= first_slot_offset;
            assert(desc->buffer->flags & RADEON_FLAG_32BIT);
   assert((desc->buffer->gpu_address >> 32) == sctx->screen->info.address32_hi);
      }
      static void
   si_add_descriptors_to_bo_list(struct si_context *sctx, struct si_descriptors *desc)
   {
      if (!desc->buffer)
            radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, desc->buffer,
      }
      /* SAMPLER VIEWS */
      static inline unsigned si_get_sampler_view_priority(struct si_resource *res)
   {
      if (res->b.b.target == PIPE_BUFFER)
            if (res->b.b.nr_samples > 1)
               }
      static struct si_descriptors *si_sampler_and_image_descriptors(struct si_context *sctx,
         {
         }
      static void si_release_sampler_views(struct si_samplers *samplers)
   {
               for (i = 0; i < ARRAY_SIZE(samplers->views); i++) {
            }
      static void si_sampler_view_add_buffer(struct si_context *sctx, struct pipe_resource *resource,
         {
      struct si_texture *tex = (struct si_texture *)resource;
            if (!resource)
            /* Use the flushed depth texture if direct sampling is unsupported. */
   if (resource->target != PIPE_BUFFER && tex->is_depth &&
      !si_can_sample_zs(tex, is_stencil_sampler))
         priority = si_get_sampler_view_priority(&tex->buffer);
      }
      static void si_sampler_views_begin_new_cs(struct si_context *sctx, struct si_samplers *samplers)
   {
               /* Add buffers to the CS. */
   while (mask) {
      int i = u_bit_scan(&mask);
            si_sampler_view_add_buffer(sctx, sview->base.texture, RADEON_USAGE_READ,
         }
      static bool si_sampler_views_check_encrypted(struct si_context *sctx, struct si_samplers *samplers,
         {
               /* Verify if a samplers uses an encrypted resource */
   while (mask) {
      int i = u_bit_scan(&mask);
            struct si_resource *res = si_resource(sview->base.texture);
   if (res->flags & RADEON_FLAG_ENCRYPTED)
      }
      }
      /* Set buffer descriptor fields that can be changed by reallocations. */
   static void si_set_buf_desc_address(struct si_resource *buf, uint64_t offset, uint32_t *state)
   {
               state[0] = va;
   state[1] &= C_008F04_BASE_ADDRESS_HI;
      }
      /* Set texture descriptor fields that can be changed by reallocations.
   *
   * \param tex              texture
   * \param base_level_info  information of the level of BASE_ADDRESS
   * \param base_level       the level of BASE_ADDRESS
   * \param first_level      pipe_sampler_view.u.tex.first_level
   * \param block_width      util_format_get_blockwidth()
   * \param is_stencil       select between separate Z & Stencil
   * \param state            descriptor to update
   */
   void si_set_mutable_tex_desc_fields(struct si_screen *sscreen, struct si_texture *tex,
                           {
               if (tex->is_depth && !si_can_sample_zs(tex, is_stencil)) {
      tex = tex->flushed_depth_texture;
                        if (sscreen->info.gfx_level >= GFX9) {
      /* Only stencil_offset needs to be added here. */
   if (is_stencil)
         else
      } else {
                  if (!sscreen->info.has_image_opcodes) {
      /* Set it as a buffer descriptor. */
   state[0] = va;
   state[1] |= S_008F04_BASE_ADDRESS_HI(va >> 32);
               state[0] = va >> 8;
            if (sscreen->info.gfx_level >= GFX8) {
      if (!(access & SI_IMAGE_ACCESS_DCC_OFF) && vi_dcc_enabled(tex, first_level)) {
               if (sscreen->info.gfx_level == GFX8) {
      meta_va += tex->surface.u.legacy.color.dcc_level[base_level].dcc_offset;
               unsigned dcc_tile_swizzle = tex->surface.tile_swizzle << 8;
   dcc_tile_swizzle &= (1 << tex->surface.meta_alignment_log2) - 1;
      } else if (vi_tc_compat_htile_enabled(tex, first_level,
                           if (sscreen->info.gfx_level >= GFX10) {
               if (is_stencil) {
         } else {
                  /* GFX10.3+ can set a custom pitch for 1D and 2D non-array, but it must be a multiple
   * of 256B.
   */
   if (sscreen->info.gfx_level >= GFX10_3 && tex->surface.u.gfx9.uses_custom_pitch) {
      ASSERTED unsigned min_alignment = 256;
   assert((tex->surface.u.gfx9.surf_pitch * tex->surface.bpe) % min_alignment == 0);
   assert(tex->buffer.b.b.target == PIPE_TEXTURE_2D ||
                        /* Subsampled images have the pitch in the units of blocks. */
                  state[4] |= S_00A010_DEPTH(pitch - 1) | /* DEPTH contains low bits of PITCH. */
               if (meta_va) {
      struct gfx9_surf_meta_flags meta = {
      .rb_aligned = 1,
                              state[6] |= S_00A018_COMPRESSION_EN(1) |
               S_00A018_META_PIPE_ALIGNED(meta.pipe_aligned) |
   S_00A018_META_DATA_ADDRESS_LO(meta_va >> 8) |
   /* DCC image stores require the following settings:
   * - INDEPENDENT_64B_BLOCKS = 0
   * - INDEPENDENT_128B_BLOCKS = 1
   * - MAX_COMPRESSED_BLOCK_SIZE = 128B
   * - MAX_UNCOMPRESSED_BLOCK_SIZE = 256B (always used)
   *
   * The same limitations apply to SDMA compressed stores because
                  /* TC-compatible MSAA HTILE requires ITERATE_256. */
                        } else if (sscreen->info.gfx_level == GFX9) {
               if (is_stencil) {
      state[3] |= S_008F1C_SW_MODE(tex->surface.u.gfx9.zs.stencil_swizzle_mode);
      } else {
                              /* epitch is surf_pitch - 1 and are in elements unit.
   * For some reason I don't understand, when a packed YUV format
   * like UYUV is used, we have to double epitch (making it a pixel
   * pitch instead of an element pitch). Note that it's only done
   * when sampling the texture using its native format; we don't
   * need to do this when sampling it as UINT32 (as done by
   * SI_IMAGE_ACCESS_BLOCK_FORMAT_AS_UINT).
   * This looks broken, so it's possible that surf_pitch / epitch
   * are computed incorrectly, but that's the only way I found
   * to get these use cases to work properly:
   *   - yuyv dmabuf import (#6131)
   *   - jpeg vaapi decode
   *   - yuyv texture sampling (!26947)
   *   - jpeg vaapi get image (#10375)
   */
   if ((tex->buffer.b.b.format == PIPE_FORMAT_R8G8_R8B8_UNORM ||
      tex->buffer.b.b.format == PIPE_FORMAT_G8R8_B8R8_UNORM) &&
   (hw_format == V_008F14_IMG_DATA_FORMAT_GB_GR ||
                                 if (meta_va) {
      struct gfx9_surf_meta_flags meta = {
      .rb_aligned = 1,
                              state[5] |= S_008F24_META_DATA_ADDRESS(meta_va >> 40) |
               state[6] |= S_008F28_COMPRESSION_EN(1);
         } else {
      /* GFX6-GFX8 */
   unsigned pitch = base_level_info->nblk_x * block_width;
            /* Only macrotiled modes can set tile swizzle. */
   if (base_level_info->mode == RADEON_SURF_MODE_2D)
            state[3] |= S_008F1C_TILING_INDEX(index);
            if (sscreen->info.gfx_level == GFX8 && meta_va) {
      state[6] |= S_008F28_COMPRESSION_EN(1);
                  if (tex->swap_rgb_to_bgr) {
      unsigned swizzle_x = G_008F1C_DST_SEL_X(state[3]);
            state[3] &= C_008F1C_DST_SEL_X;
   state[3] |= S_008F1C_DST_SEL_X(swizzle_z);
   state[3] &= C_008F1C_DST_SEL_Z;
         }
      static void si_set_sampler_state_desc(struct si_sampler_state *sstate,
               {
      if (tex && tex->upgraded_depth && sview && !sview->is_stencil_sampler)
         else
      }
      static void si_set_sampler_view_desc(struct si_context *sctx, struct si_sampler_view *sview,
                     {
      struct pipe_sampler_view *view = &sview->base;
                     if (tex->buffer.b.b.target == PIPE_BUFFER) {
      memcpy(desc, sview->state, 8 * 4);
   memcpy(desc + 8, null_texture_descriptor, 4 * 4); /* Disable FMASK. */
   si_set_buf_desc_address(&tex->buffer, sview->base.u.buf.offset, desc + 4);
               if (unlikely(sview->dcc_incompatible)) {
      if (vi_dcc_enabled(tex, view->u.tex.first_level))
                                       memcpy(desc, sview->state, 8 * 4);
   si_set_mutable_tex_desc_fields(sctx->screen, tex, sview->base_level_info, 0,
                  if (tex->surface.fmask_size) {
         } else {
      /* Disable FMASK and bind sampler state in [12:15]. */
            if (sstate)
         }
      static bool color_needs_decompression(struct si_texture *tex)
   {
               if (sscreen->info.gfx_level >= GFX11 || tex->is_depth)
            return tex->surface.fmask_size ||
      }
      static bool depth_needs_decompression(struct si_texture *tex, bool is_stencil)
   {
      /* If the depth/stencil texture is TC-compatible, no decompression
   * will be done. The decompression function will only flush DB caches
   * to make it coherent with shaders. That's necessary because the driver
   * doesn't flush DB caches in any other case.
   */
      }
      static void si_reset_sampler_view_slot(struct si_samplers *samplers, unsigned slot,
         {
      pipe_sampler_view_reference(&samplers->views[slot], NULL);
   memcpy(desc, null_texture_descriptor, 8 * 4);
   /* Only clear the lower dwords of FMASK. */
   memcpy(desc + 8, null_texture_descriptor, 4 * 4);
   /* Re-set the sampler state if we are transitioning from FMASK. */
   if (samplers->sampler_states[slot])
      }
      static void si_set_sampler_views(struct si_context *sctx, unsigned shader,
                           {
      struct si_samplers *samplers = &sctx->samplers[shader];
   struct si_descriptors *descs = si_sampler_and_image_descriptors(sctx, shader);
            if (views) {
      for (unsigned i = 0; i < count; i++) {
      unsigned slot = start_slot + i;
   struct si_sampler_view *sview = (struct si_sampler_view *)views[i];
   unsigned desc_slot = si_get_sampler_slot(slot);
                  if (samplers->views[slot] == &sview->base && !disallow_early_out) {
      if (take_ownership) {
      struct pipe_sampler_view *view = views[i];
      }
                                          if (tex->buffer.b.b.target == PIPE_BUFFER) {
      tex->buffer.bind_history |= SI_BIND_SAMPLER_BUFFER(shader);
   samplers->needs_depth_decompress_mask &= ~(1u << slot);
      } else {
                              if (depth_needs_decompression(tex, sview->is_stencil_sampler)) {
         } else {
                                    if (color_needs_decompression(tex)) {
         } else {
                     if (vi_dcc_enabled(tex, sview->base.u.tex.first_level) &&
                     if (take_ownership) {
      pipe_sampler_view_reference(&samplers->views[slot], NULL);
      } else {
                        /* Since this can flush, it must be done after enabled_mask is
   * updated. */
   si_sampler_view_add_buffer(sctx, &tex->buffer.b.b, RADEON_USAGE_READ,
      } else {
      si_reset_sampler_view_slot(samplers, slot, desc);
            } else {
      unbind_num_trailing_slots += count;
               for (unsigned i = 0; i < unbind_num_trailing_slots; i++) {
      unsigned slot = start_slot + count + i;
   unsigned desc_slot = si_get_sampler_slot(slot);
            if (samplers->views[slot])
               unbound_mask |= BITFIELD_RANGE(start_slot + count, unbind_num_trailing_slots);
   samplers->enabled_mask &= ~unbound_mask;
   samplers->has_depth_tex_mask &= ~unbound_mask;
   samplers->needs_depth_decompress_mask &= ~unbound_mask;
            sctx->descriptors_dirty |= 1u << si_sampler_and_image_descriptors_idx(shader);
   if (shader != PIPE_SHADER_COMPUTE)
      }
      static void si_update_shader_needs_decompress_mask(struct si_context *sctx, unsigned shader)
   {
      struct si_samplers *samplers = &sctx->samplers[shader];
            if (samplers->needs_depth_decompress_mask || samplers->needs_color_decompress_mask ||
      sctx->images[shader].needs_color_decompress_mask)
      else
            if (samplers->has_depth_tex_mask)
         else
      }
      static void si_pipe_set_sampler_views(struct pipe_context *ctx, enum pipe_shader_type shader,
                     {
               if ((!count && !unbind_num_trailing_slots) || shader >= SI_NUM_SHADERS)
            si_set_sampler_views(sctx, shader, start, count, unbind_num_trailing_slots,
            }
      static void si_samplers_update_needs_color_decompress_mask(struct si_samplers *samplers)
   {
               while (mask) {
      int i = u_bit_scan(&mask);
            if (res && res->target != PIPE_BUFFER) {
               if (color_needs_decompression(tex)) {
         } else {
                  }
      /* IMAGE VIEWS */
      static void si_release_image_views(struct si_images *images)
   {
               for (i = 0; i < SI_NUM_IMAGES; ++i) {
                     }
      static void si_image_views_begin_new_cs(struct si_context *sctx, struct si_images *images)
   {
               /* Add buffers to the CS. */
   while (mask) {
      int i = u_bit_scan(&mask);
                           }
      static bool si_image_views_check_encrypted(struct si_context *sctx, struct si_images *images,
         {
               while (mask) {
      int i = u_bit_scan(&mask);
                     struct si_texture *tex = (struct si_texture *)view->resource;
   if (tex->buffer.flags & RADEON_FLAG_ENCRYPTED)
      }
      }
      static void si_disable_shader_image(struct si_context *ctx, unsigned shader, unsigned slot)
   {
               if (images->enabled_mask & (1u << slot)) {
      struct si_descriptors *descs = si_sampler_and_image_descriptors(ctx, shader);
            pipe_resource_reference(&images->views[slot].resource, NULL);
            memcpy(descs->list + desc_slot * 8, null_image_descriptor, 8 * 4);
   images->enabled_mask &= ~(1u << slot);
   images->display_dcc_store_mask &= ~(1u << slot);
   ctx->descriptors_dirty |= 1u << si_sampler_and_image_descriptors_idx(shader);
   if (shader != PIPE_SHADER_COMPUTE)
         }
      static void si_mark_image_range_valid(const struct pipe_image_view *view)
   {
               if (res->b.b.target != PIPE_BUFFER)
            util_range_add(&res->b.b, &res->valid_buffer_range, view->u.buf.offset,
      }
      static void si_set_shader_image_desc(struct si_context *ctx, const struct pipe_image_view *view,
         {
      struct si_screen *screen = ctx->screen;
                     if (res->b.b.target == PIPE_BUFFER) {
      if (view->access & PIPE_IMAGE_ACCESS_WRITE)
         uint32_t elements = si_clamp_texture_texel_count(screen->max_texel_buffer_elements,
            si_make_buffer_descriptor(screen, res, view->format, view->u.buf.offset, elements,
            } else {
      static const unsigned char swizzle[4] = {0, 1, 2, 3};
   struct si_texture *tex = (struct si_texture *)res;
   unsigned level = view->u.tex.level;
   bool uses_dcc = vi_dcc_enabled(tex, level);
            if (uses_dcc && screen->always_allow_dcc_stores)
            assert(!tex->is_depth);
            if (uses_dcc && !skip_decompress &&
      !(access & SI_IMAGE_ACCESS_DCC_OFF) &&
   ((!(access & SI_IMAGE_ACCESS_ALLOW_DCC_STORE) && (access & PIPE_IMAGE_ACCESS_WRITE)) ||
   !vi_dcc_formats_compatible(screen, res->b.b.format, view->format))) {
   /* If DCC can't be disabled, at least decompress it.
   * The decompression is relatively cheap if the surface
   * has been decompressed already.
   */
   if (!si_texture_disable_dcc(ctx, tex))
               unsigned width = res->b.b.width0;
   unsigned height = res->b.b.height0;
   unsigned depth = res->b.b.depth0;
            if (ctx->gfx_level <= GFX8) {
      /* Always force the base level to the selected level.
   *
   * This is required for 3D textures, where otherwise
   * selecting a single slice for non-layered bindings
   * fails. It doesn't hurt the other targets.
   */
   width = u_minify(width, level);
   height = u_minify(height, level);
   depth = u_minify(depth, level);
               if (access & SI_IMAGE_ACCESS_BLOCK_FORMAT_AS_UINT) {
      if (ctx->gfx_level >= GFX9) {
      /* Since the aligned width and height are derived from the width and height
   * by the hw, set them directly as the width and height, so that UINT formats
   * get exactly the same layout as BCn formats.
   */
   width = tex->surface.u.gfx9.base_mip_width;
      } else {
      width = util_format_get_nblocksx(tex->buffer.b.b.format, width);
                  screen->make_texture_descriptor(
      screen, tex, false, res->b.b.target, view->format, swizzle, hw_level, hw_level,
   view->u.tex.first_layer, view->u.tex.last_layer, width, height, depth, false,
      si_set_mutable_tex_desc_fields(screen, tex, &tex->surface.u.legacy.level[level], level, level,
               }
      static void si_set_shader_image(struct si_context *ctx, unsigned shader, unsigned slot,
         {
      struct si_images *images = &ctx->images[shader];
   struct si_descriptors *descs = si_sampler_and_image_descriptors(ctx, shader);
            if (!view || !view->resource) {
      si_disable_shader_image(ctx, shader, slot);
                        si_set_shader_image_desc(ctx, view, skip_decompress, descs->list + si_get_image_slot(slot) * 8,
            if (&images->views[slot] != view)
            if (res->b.b.target == PIPE_BUFFER) {
      images->needs_color_decompress_mask &= ~(1 << slot);
   images->display_dcc_store_mask &= ~(1u << slot);
      } else {
      struct si_texture *tex = (struct si_texture *)res;
            if (color_needs_decompression(tex)) {
         } else {
                  if (tex->surface.display_dcc_offset && view->access & PIPE_IMAGE_ACCESS_WRITE) {
               /* Set displayable_dcc_dirty for non-compute stages conservatively (before draw calls). */
   if (shader != PIPE_SHADER_COMPUTE)
      } else {
                  if (vi_dcc_enabled(tex, level) && p_atomic_read(&tex->framebuffers_bound))
               images->enabled_mask |= 1u << slot;
   ctx->descriptors_dirty |= 1u << si_sampler_and_image_descriptors_idx(shader);
   if (shader != PIPE_SHADER_COMPUTE)
            /* Since this can flush, it must be done after enabled_mask is updated. */
   si_sampler_view_add_buffer(ctx, &res->b.b,
            }
      static void si_set_shader_images(struct pipe_context *pipe, enum pipe_shader_type shader,
                     {
      struct si_context *ctx = (struct si_context *)pipe;
                     if (!count && !unbind_num_trailing_slots)
                     if (views) {
      for (i = 0, slot = start_slot; i < count; ++i, ++slot)
      } else {
      for (i = 0, slot = start_slot; i < count; ++i, ++slot)
               for (i = 0; i < unbind_num_trailing_slots; ++i, ++slot)
            if (shader == PIPE_SHADER_COMPUTE &&
      ctx->cs_shader_state.program &&
   start_slot < ctx->cs_shader_state.program->sel.cs_num_images_in_user_sgprs)
            }
      static void si_images_update_needs_color_decompress_mask(struct si_images *images)
   {
               while (mask) {
      int i = u_bit_scan(&mask);
            if (res && res->target != PIPE_BUFFER) {
               if (color_needs_decompression(tex)) {
         } else {
                  }
      void si_force_disable_ps_colorbuf0_slot(struct si_context *sctx)
   {
      if (sctx->ps_uses_fbfetch) {
      sctx->ps_uses_fbfetch = false;
         }
      void si_update_ps_colorbuf0_slot(struct si_context *sctx)
   {
      struct si_buffer_resources *buffers = &sctx->internal_bindings;
   struct si_descriptors *descs = &sctx->descriptors[SI_DESCS_INTERNAL];
   unsigned slot = SI_PS_IMAGE_COLORBUF0;
   struct pipe_surface *surf = NULL;
            /* FBFETCH is always disabled for u_blitter, and will be re-enabled after u_blitter is done. */
   if (sctx->blitter_running || sctx->suppress_update_ps_colorbuf0_slot) {
      assert(!sctx->ps_uses_fbfetch);
               /* Get the color buffer if FBFETCH should be enabled. */
   if (sctx->shader.ps.cso && sctx->shader.ps.cso->info.base.fs.uses_fbfetch_output &&
      sctx->framebuffer.state.nr_cbufs && sctx->framebuffer.state.cbufs[0]) {
   surf = sctx->framebuffer.state.cbufs[0];
   if (surf) {
      tex = (struct si_texture *)surf->texture;
                  /* Return if FBFETCH transitions from disabled to disabled. */
   if (!sctx->ps_uses_fbfetch && !surf)
            if (surf) {
      bool disable_dcc = tex->surface.meta_offset != 0;
            /* Disable DCC and eliminate fast clear because the texture is used as both a sampler
   * and color buffer.
   */
   if (disable_dcc || disable_cmask) {
      /* Disable fbfetch only for decompression. */
                           if (disable_cmask) {
      assert(tex->cmask_buffer != &tex->buffer);
   si_eliminate_fast_color_clear(sctx, tex, NULL);
                           /* Bind color buffer 0 as a shader image. */
   struct pipe_image_view view = {0};
   view.resource = surf->texture;
   view.format = surf->format;
   view.access = PIPE_IMAGE_ACCESS_READ;
   view.u.tex.first_layer = surf->u.tex.first_layer;
   view.u.tex.last_layer = surf->u.tex.last_layer;
            /* Set the descriptor. */
   uint32_t *desc = descs->list + slot * 4;
   memset(desc, 0, 16 * 4);
            pipe_resource_reference(&buffers->buffers[slot], &tex->buffer.b.b);
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, &tex->buffer,
            } else {
      /* Clear the descriptor. */
   memset(descs->list + slot * 4, 0, 8 * 4);
   pipe_resource_reference(&buffers->buffers[slot], NULL);
               sctx->descriptors_dirty |= 1u << SI_DESCS_INTERNAL;
   si_mark_atom_dirty(sctx, &sctx->atoms.s.gfx_shader_pointers);
   sctx->ps_uses_fbfetch = surf != NULL;
   si_update_ps_iter_samples(sctx);
      }
      /* SAMPLER STATES */
      static void si_bind_sampler_states(struct pipe_context *ctx, enum pipe_shader_type shader,
         {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_samplers *samplers = &sctx->samplers[shader];
   struct si_descriptors *desc = si_sampler_and_image_descriptors(sctx, shader);
   struct si_sampler_state **sstates = (struct si_sampler_state **)states;
            if (!count || shader >= SI_NUM_SHADERS || !sstates)
            for (i = 0; i < count; i++) {
      unsigned slot = start + i;
            if (!sstates[i] || sstates[i] == samplers->sampler_states[slot])
      #ifndef NDEBUG
         #endif
                  /* If FMASK is bound, don't overwrite it.
   * The sampler state will be set after FMASK is unbound.
   */
                     if (sview && sview->base.texture && sview->base.texture->target != PIPE_BUFFER)
            if (tex && tex->surface.fmask_size)
                     sctx->descriptors_dirty |= 1u << si_sampler_and_image_descriptors_idx(shader);
   if (shader != PIPE_SHADER_COMPUTE)
         }
      /* BUFFER RESOURCES */
      static void si_init_buffer_resources(struct si_context *sctx,
                                 {
      buffers->priority = priority;
   buffers->priority_constbuf = priority_constbuf;
   buffers->buffers = CALLOC(num_buffers, sizeof(struct pipe_resource *));
                     /* Initialize buffer descriptors, so that we don't have to do it at bind time. */
   for (unsigned i = 0; i < num_buffers; i++) {
               desc[3] = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) | S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
            if (sctx->gfx_level >= GFX11) {
      desc[3] |= S_008F0C_FORMAT(V_008F0C_GFX11_FORMAT_32_FLOAT) |
      } else if (sctx->gfx_level >= GFX10) {
      desc[3] |= S_008F0C_FORMAT(V_008F0C_GFX10_FORMAT_32_FLOAT) |
      } else {
      desc[3] |= S_008F0C_NUM_FORMAT(V_008F0C_BUF_NUM_FORMAT_FLOAT) |
            }
      static void si_release_buffer_resources(struct si_buffer_resources *buffers,
         {
               for (i = 0; i < descs->num_elements; i++) {
                  FREE(buffers->buffers);
      }
      static void si_buffer_resources_begin_new_cs(struct si_context *sctx,
         {
               /* Add buffers to the CS. */
   while (mask) {
               radeon_add_to_buffer_list(
      sctx, &sctx->gfx_cs, si_resource(buffers->buffers[i]),
   (buffers->writable_mask & (1llu << i) ? RADEON_USAGE_READWRITE : RADEON_USAGE_READ) |
      }
      static bool si_buffer_resources_check_encrypted(struct si_context *sctx,
         {
               while (mask) {
               if (si_resource(buffers->buffers[i])->flags & RADEON_FLAG_ENCRYPTED)
                  }
      static void si_get_buffer_from_descriptors(struct si_buffer_resources *buffers,
                     {
      pipe_resource_reference(buf, buffers->buffers[idx]);
   if (*buf) {
      struct si_resource *res = si_resource(*buf);
   const uint32_t *desc = descs->list + idx * 4;
                     assert(G_008F04_STRIDE(desc[1]) == 0);
            assert(va >= res->gpu_address && va + *size <= res->gpu_address + res->bo_size);
         }
      /* CONSTANT BUFFERS */
      static struct si_descriptors *si_const_and_shader_buffer_descriptors(struct si_context *sctx,
         {
         }
      static void si_upload_const_buffer(struct si_context *sctx, struct si_resource **buf,
         {
               u_upload_alloc(sctx->b.const_uploader, 0, size, si_optimal_tcc_alignment(sctx, size),
         if (*buf)
      }
      static void si_set_constant_buffer(struct si_context *sctx, struct si_buffer_resources *buffers,
               {
      struct si_descriptors *descs = &sctx->descriptors[descriptors_idx];
   assert(slot < descs->num_elements);
            /* GFX7 cannot unbind a constant buffer (S_BUFFER_LOAD is buggy
   * with a NULL buffer). We need to use a dummy buffer instead. */
   if (sctx->gfx_level == GFX7 && (!input || (!input->buffer && !input->user_buffer)))
            if (input && (input->buffer || input->user_buffer)) {
      struct pipe_resource *buffer = NULL;
   uint64_t va;
            /* Upload the user buffer if needed. */
   if (input->user_buffer) {
      si_upload_const_buffer(sctx, (struct si_resource **)&buffer, input->user_buffer,
         if (!buffer) {
      /* Just unbind on failure. */
   si_set_constant_buffer(sctx, buffers, descriptors_idx, slot, false, NULL);
         } else {
      if (take_ownership) {
         } else {
         }
                        /* Set the descriptor. */
   uint32_t *desc = descs->list + slot * 4;
   desc[0] = va;
   desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32) | S_008F04_STRIDE(0);
            buffers->buffers[slot] = buffer;
   buffers->offsets[slot] = buffer_offset;
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(buffer),
            } else {
      /* Clear the descriptor. Only 3 dwords are cleared. The 4th dword is immutable. */
   memset(descs->list + slot * 4, 0, sizeof(uint32_t) * 3);
               sctx->descriptors_dirty |= 1u << descriptors_idx;
   if (descriptors_idx < SI_DESCS_FIRST_COMPUTE)
      }
      void si_get_inline_uniform_state(union si_shader_key *key, enum pipe_shader_type shader,
         {
      if (shader == PIPE_SHADER_FRAGMENT) {
      *inline_uniforms = key->ps.opt.inline_uniforms;
      } else {
      *inline_uniforms = key->ge.opt.inline_uniforms;
         }
      void si_invalidate_inlinable_uniforms(struct si_context *sctx, enum pipe_shader_type shader)
   {
      if (shader == PIPE_SHADER_COMPUTE)
            bool inline_uniforms;
   uint32_t *inlined_values;
            if (inline_uniforms) {
      if (shader == PIPE_SHADER_FRAGMENT)
         else
            memset(inlined_values, 0, MAX_INLINABLE_UNIFORMS * 4);
         }
      static void si_pipe_set_constant_buffer(struct pipe_context *ctx, enum pipe_shader_type shader,
               {
               if (shader >= SI_NUM_SHADERS)
            if (input) {
      if (input->buffer) {
      if (slot == 0 &&
      !(si_resource(input->buffer)->flags & RADEON_FLAG_32BIT)) {
   assert(!"constant buffer 0 must have a 32-bit VM address, use const_uploader");
      }
               if (slot == 0)
               slot = si_get_constbuf_slot(slot);
   si_set_constant_buffer(sctx, &sctx->const_and_shader_buffers[shader],
            }
      static void si_set_inlinable_constants(struct pipe_context *ctx,
               {
               if (shader == PIPE_SHADER_COMPUTE)
            bool inline_uniforms;
   uint32_t *inlined_values;
            if (!inline_uniforms) {
      /* It's the first time we set the constants. Always update shaders. */
   if (shader == PIPE_SHADER_FRAGMENT)
         else
            memcpy(inlined_values, values, num_values * 4);
   sctx->do_update_shaders = true;
               /* We have already set inlinable constants for this shader. Update the shader only if
   * the constants are being changed so as not to update shaders needlessly.
   */
   if (memcmp(inlined_values, values, num_values * 4)) {
      memcpy(inlined_values, values, num_values * 4);
         }
      void si_get_pipe_constant_buffer(struct si_context *sctx, uint shader, uint slot,
         {
      cbuf->user_buffer = NULL;
   si_get_buffer_from_descriptors(
      &sctx->const_and_shader_buffers[shader], si_const_and_shader_buffer_descriptors(sctx, shader),
   }
      /* SHADER BUFFERS */
      static void si_set_shader_buffer(struct si_context *sctx, struct si_buffer_resources *buffers,
                     {
      struct si_descriptors *descs = &sctx->descriptors[descriptors_idx];
            if (!sbuffer || !sbuffer->buffer) {
      pipe_resource_reference(&buffers->buffers[slot], NULL);
   /* Clear the descriptor. Only 3 dwords are cleared. The 4th dword is immutable. */
   memset(desc, 0, sizeof(uint32_t) * 3);
   buffers->enabled_mask &= ~(1llu << slot);
   buffers->writable_mask &= ~(1llu << slot);
   sctx->descriptors_dirty |= 1u << descriptors_idx;
   if (descriptors_idx < SI_DESCS_FIRST_COMPUTE)
                     struct si_resource *buf = si_resource(sbuffer->buffer);
            desc[0] = va;
   desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32) | S_008F04_STRIDE(0);
            pipe_resource_reference(&buffers->buffers[slot], &buf->b.b);
   buffers->offsets[slot] = sbuffer->buffer_offset;
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, buf,
         if (writable)
         else
            buffers->enabled_mask |= 1llu << slot;
   sctx->descriptors_dirty |= 1lu << descriptors_idx;
   if (descriptors_idx < SI_DESCS_FIRST_COMPUTE)
            util_range_add(&buf->b.b, &buf->valid_buffer_range, sbuffer->buffer_offset,
      }
      void si_set_shader_buffers(struct pipe_context *ctx, enum pipe_shader_type shader,
                     {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_buffer_resources *buffers = &sctx->const_and_shader_buffers[shader];
   unsigned descriptors_idx = si_const_and_shader_buffer_descriptors_idx(shader);
                     if (shader == PIPE_SHADER_COMPUTE &&
      sctx->cs_shader_state.program &&
   start_slot < sctx->cs_shader_state.program->sel.cs_num_shaderbufs_in_user_sgprs)
         for (i = 0; i < count; ++i) {
      const struct pipe_shader_buffer *sbuffer = sbuffers ? &sbuffers[i] : NULL;
            /* Don't track bind history for internal blits, such as clear_buffer and copy_buffer
   * to prevent unnecessary synchronization before compute blits later.
   */
   if (!internal_blit && sbuffer && sbuffer->buffer)
            si_set_shader_buffer(sctx, buffers, descriptors_idx, slot, sbuffer,
         }
      static void si_pipe_set_shader_buffers(struct pipe_context *ctx, enum pipe_shader_type shader,
                     {
         }
      void si_get_shader_buffers(struct si_context *sctx, enum pipe_shader_type shader, uint start_slot,
         {
      struct si_buffer_resources *buffers = &sctx->const_and_shader_buffers[shader];
            for (unsigned i = 0; i < count; ++i) {
      si_get_buffer_from_descriptors(buffers, descs, si_get_shaderbuf_slot(start_slot + i),
         }
      /* RING BUFFERS */
      void si_set_internal_const_buffer(struct si_context *sctx, uint slot,
         {
         }
      void si_set_internal_shader_buffer(struct si_context *sctx, uint slot,
         {
      si_set_shader_buffer(sctx, &sctx->internal_bindings, SI_DESCS_INTERNAL, slot, sbuffer, true,
      }
      void si_set_ring_buffer(struct si_context *sctx, uint slot, struct pipe_resource *buffer,
               {
      struct si_buffer_resources *buffers = &sctx->internal_bindings;
            /* The stride field in the resource descriptor has 14 bits */
            assert(slot < descs->num_elements);
            if (buffer) {
                        switch (element_size) {
   default:
         case 0:
   case 2:
      element_size = 0;
      case 4:
      element_size = 1;
      case 8:
      element_size = 2;
      case 16:
      element_size = 3;
               switch (index_stride) {
   default:
         case 0:
   case 8:
      index_stride = 0;
      case 16:
      index_stride = 1;
      case 32:
      index_stride = 2;
      case 64:
      index_stride = 3;
               if (sctx->gfx_level >= GFX8 && stride)
            /* Set the descriptor. */
   uint32_t *desc = descs->list + slot * 4;
   desc[0] = va;
   desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32) | S_008F04_STRIDE(stride);
   desc[2] = num_records;
   desc[3] = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) | S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
                  if (sctx->gfx_level >= GFX11) {
      assert(!swizzle || element_size == 1 || element_size == 3); /* 4 or 16 bytes */
      } else if (sctx->gfx_level >= GFX9) {
      assert(!swizzle || element_size == 1); /* only 4 bytes on GFX9 */
      } else {
      desc[1] |= S_008F04_SWIZZLE_ENABLE_GFX6(swizzle);
               if (sctx->gfx_level >= GFX11) {
      desc[3] |= S_008F0C_FORMAT(V_008F0C_GFX11_FORMAT_32_FLOAT) |
      } else if (sctx->gfx_level >= GFX10) {
      desc[3] |= S_008F0C_FORMAT(V_008F0C_GFX10_FORMAT_32_FLOAT) |
      } else {
      desc[3] |= S_008F0C_NUM_FORMAT(V_008F0C_BUF_NUM_FORMAT_FLOAT) |
               pipe_resource_reference(&buffers->buffers[slot], buffer);
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(buffer),
            } else {
      /* Clear the descriptor. */
   memset(descs->list + slot * 4, 0, sizeof(uint32_t) * 4);
               sctx->descriptors_dirty |= 1u << SI_DESCS_INTERNAL;
      }
      /* INTERNAL CONST BUFFERS */
      static void si_set_polygon_stipple(struct pipe_context *ctx, const struct pipe_poly_stipple *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct pipe_constant_buffer cb = {};
   unsigned stipple[32];
            for (i = 0; i < 32; i++)
            cb.user_buffer = stipple;
               }
      /* TEXTURE METADATA ENABLE/DISABLE */
      static void si_resident_handles_update_needs_color_decompress(struct si_context *sctx)
   {
      util_dynarray_clear(&sctx->resident_tex_needs_color_decompress);
            util_dynarray_foreach (&sctx->resident_tex_handles, struct si_texture_handle *, tex_handle) {
      struct pipe_resource *res = (*tex_handle)->view->texture;
            if (!res || res->target == PIPE_BUFFER)
            tex = (struct si_texture *)res;
   if (!color_needs_decompression(tex))
            util_dynarray_append(&sctx->resident_tex_needs_color_decompress, struct si_texture_handle *,
               util_dynarray_foreach (&sctx->resident_img_handles, struct si_image_handle *, img_handle) {
      struct pipe_image_view *view = &(*img_handle)->view;
   struct pipe_resource *res = view->resource;
            if (!res || res->target == PIPE_BUFFER)
            tex = (struct si_texture *)res;
   if (!color_needs_decompression(tex))
            util_dynarray_append(&sctx->resident_img_needs_color_decompress, struct si_image_handle *,
         }
      /* CMASK can be enabled (for fast clear) and disabled (for texture export)
   * while the texture is bound, possibly by a different context. In that case,
   * call this function to update needs_*_decompress_masks.
   */
   void si_update_needs_color_decompress_masks(struct si_context *sctx)
   {
               for (int i = 0; i < SI_NUM_SHADERS; ++i) {
      si_samplers_update_needs_color_decompress_mask(&sctx->samplers[i]);
   si_images_update_needs_color_decompress_mask(&sctx->images[i]);
                  }
      /* BUFFER DISCARD/INVALIDATION */
      /* Reset descriptors of buffer resources after \p buf has been invalidated.
   * If buf == NULL, reset all descriptors.
   */
   static bool si_reset_buffer_resources(struct si_context *sctx, struct si_buffer_resources *buffers,
               {
      struct si_descriptors *descs = &sctx->descriptors[descriptors_idx];
   bool noop = true;
            while (mask) {
      unsigned i = u_bit_scan64(&mask);
            if (buffer && (!buf || buffer == buf)) {
      si_set_buf_desc_address(si_resource(buffer), buffers->offsets[i], descs->list + i * 4);
   sctx->descriptors_dirty |= 1u << descriptors_idx;
                  radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(buffer),
                     }
      }
      static void si_mark_bindless_descriptors_dirty(struct si_context *sctx)
   {
      sctx->bindless_descriptors_dirty = true;
   /* gfx_shader_pointers uploads bindless descriptors. */
   si_mark_atom_dirty(sctx, &sctx->atoms.s.gfx_shader_pointers);
   /* gfx_shader_pointers can flag cache flags, so we need to dirty this too. */
      }
      /* Update all buffer bindings where the buffer is bound, including
   * all resource descriptors. This is invalidate_buffer without
   * the invalidation.
   *
   * If buf == NULL, update all buffer bindings.
   */
   void si_rebind_buffer(struct si_context *sctx, struct pipe_resource *buf)
   {
      struct si_resource *buffer = si_resource(buf);
   unsigned i;
            /* We changed the buffer, now we need to bind it where the old one
   * was bound. This consists of 2 things:
   *   1) Updating the resource descriptor and dirtying it.
   *   2) Adding a relocation to the CS, so that it's usable.
            /* Vertex buffers. */
   if (!buffer) {
               /* We don't know which buffer was invalidated, so we have to add all of them. */
   for (unsigned i = 0; i < ARRAY_SIZE(sctx->vertex_buffer); i++) {
      struct si_resource *buf = si_resource(sctx->vertex_buffer[i].buffer.resource);
   if (buf) {
      radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, buf,
                  } else if (buffer->bind_history & SI_BIND_VERTEX_BUFFER) {
      for (i = 0; i < num_elems; i++) {
               if (vb >= ARRAY_SIZE(sctx->vertex_buffer))
                        if (sctx->vertex_buffer[vb].buffer.resource == buf) {
      sctx->vertex_buffers_dirty = num_elems > 0;
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, buffer,
                                 /* Streamout buffers. (other internal buffers can't be invalidated) */
   if (!buffer || buffer->bind_history & SI_BIND_STREAMOUT_BUFFER) {
      for (i = SI_VS_STREAMOUT_BUF0; i <= SI_VS_STREAMOUT_BUF3; i++) {
      struct si_buffer_resources *buffers = &sctx->internal_bindings;
                                 si_set_buf_desc_address(si_resource(buffer), buffers->offsets[i], descs->list + i * 4);
                                 /* Update the streamout state. */
   if (sctx->streamout.begin_emitted)
         sctx->streamout.append_bitmask = sctx->streamout.enabled_mask;
                  /* Constant and shader buffers. */
   if (!buffer || buffer->bind_history & SI_BIND_CONSTANT_BUFFER_ALL) {
      unsigned mask = buffer ? (buffer->bind_history & SI_BIND_CONSTANT_BUFFER_ALL) >>
         u_foreach_bit(shader, mask) {
      si_reset_buffer_resources(sctx, &sctx->const_and_shader_buffers[shader],
                              if (!buffer || buffer->bind_history & SI_BIND_SHADER_BUFFER_ALL) {
      unsigned mask = buffer ? (buffer->bind_history & SI_BIND_SHADER_BUFFER_ALL) >>
         u_foreach_bit(shader, mask) {
      if (si_reset_buffer_resources(sctx, &sctx->const_and_shader_buffers[shader],
                        shader == PIPE_SHADER_COMPUTE) {
                     if (!buffer || buffer->bind_history & SI_BIND_SAMPLER_BUFFER_ALL) {
      unsigned mask = buffer ? (buffer->bind_history & SI_BIND_SAMPLER_BUFFER_ALL) >>
         /* Texture buffers - update bindings. */
   u_foreach_bit(shader, mask) {
      struct si_samplers *samplers = &sctx->samplers[shader];
                  while (mask) {
                                       si_set_buf_desc_address(si_resource(buffer), samplers->views[i]->u.buf.offset,
                              radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(buffer), RADEON_USAGE_READ |
                        /* Shader images */
   if (!buffer || buffer->bind_history & SI_BIND_IMAGE_BUFFER_ALL) {
      unsigned mask = buffer ? (buffer->bind_history & SI_BIND_IMAGE_BUFFER_SHIFT) >>
         u_foreach_bit(shader, mask) {
      struct si_images *images = &sctx->images[shader];
                  while (mask) {
                                                      si_set_buf_desc_address(si_resource(buffer), images->views[i].u.buf.offset,
                                                   if (shader == PIPE_SHADER_COMPUTE)
                        /* Bindless texture handles */
   if (!buffer || buffer->texture_handle_allocated) {
               util_dynarray_foreach (&sctx->resident_tex_handles, struct si_texture_handle *, tex_handle) {
      struct pipe_sampler_view *view = (*tex_handle)->view;
                  if (buffer && buffer->target == PIPE_BUFFER && (!buf || buffer == buf)) {
                                    radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(buffer), RADEON_USAGE_READ |
                     /* Bindless image handles */
   if (!buffer || buffer->image_handle_allocated) {
               util_dynarray_foreach (&sctx->resident_img_handles, struct si_image_handle *, img_handle) {
      struct pipe_image_view *view = &(*img_handle)->view;
                  if (buffer && buffer->target == PIPE_BUFFER && (!buf || buffer == buf)) {
                                                   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(buffer),
                     if (buffer) {
      /* Do the same for other contexts. They will invoke this function
   * with buffer == NULL.
   */
            /* Skip the update for the current context, because we have already updated
   * the buffer bindings.
   */
   if (new_counter == sctx->last_dirty_buf_counter + 1)
         }
      static void si_upload_bindless_descriptor(struct si_context *sctx, unsigned desc_slot,
         {
      struct si_descriptors *desc = &sctx->bindless_descriptors;
   unsigned desc_slot_offset = desc_slot * 16;
   uint32_t *data;
            data = desc->list + desc_slot_offset;
            si_cp_write_data(sctx, desc->buffer, va - desc->buffer->gpu_address, num_dwords * 4, V_370_TC_L2,
      }
      static void si_upload_bindless_descriptors(struct si_context *sctx)
   {
      if (!sctx->bindless_descriptors_dirty)
            /* Wait for graphics/compute to be idle before updating the resident
   * descriptors directly in memory, in case the GPU is using them.
   */
   sctx->flags |= SI_CONTEXT_PS_PARTIAL_FLUSH | SI_CONTEXT_CS_PARTIAL_FLUSH;
            util_dynarray_foreach (&sctx->resident_tex_handles, struct si_texture_handle *, tex_handle) {
               if (!(*tex_handle)->desc_dirty)
            si_upload_bindless_descriptor(sctx, desc_slot, 16);
               util_dynarray_foreach (&sctx->resident_img_handles, struct si_image_handle *, img_handle) {
               if (!(*img_handle)->desc_dirty)
            si_upload_bindless_descriptor(sctx, desc_slot, 8);
               /* Invalidate scalar L0 because the cache doesn't know that L2 changed. */
   sctx->flags |= SI_CONTEXT_INV_SCACHE;
      }
      /* Update mutable image descriptor fields of all resident textures. */
   static void si_update_bindless_texture_descriptor(struct si_context *sctx,
         {
      struct si_sampler_view *sview = (struct si_sampler_view *)tex_handle->view;
   struct si_descriptors *desc = &sctx->bindless_descriptors;
   unsigned desc_slot_offset = tex_handle->desc_slot * 16;
            if (sview->base.texture->target == PIPE_BUFFER)
            memcpy(desc_list, desc->list + desc_slot_offset, sizeof(desc_list));
            if (memcmp(desc_list, desc->list + desc_slot_offset, sizeof(desc_list))) {
      tex_handle->desc_dirty = true;
         }
      static void si_update_bindless_image_descriptor(struct si_context *sctx,
         {
      struct si_descriptors *desc = &sctx->bindless_descriptors;
   unsigned desc_slot_offset = img_handle->desc_slot * 16;
   struct pipe_image_view *view = &img_handle->view;
   struct pipe_resource *res = view->resource;
   uint32_t image_desc[16];
            if (res->target == PIPE_BUFFER)
            memcpy(image_desc, desc->list + desc_slot_offset, desc_size);
   si_set_shader_image_desc(sctx, view, true, desc->list + desc_slot_offset,
            if (memcmp(image_desc, desc->list + desc_slot_offset, desc_size)) {
      img_handle->desc_dirty = true;
         }
      static void si_update_all_resident_texture_descriptors(struct si_context *sctx)
   {
      util_dynarray_foreach (&sctx->resident_tex_handles, struct si_texture_handle *, tex_handle) {
                  util_dynarray_foreach (&sctx->resident_img_handles, struct si_image_handle *, img_handle) {
            }
      /* Update mutable image descriptor fields of all bound textures. */
   void si_update_all_texture_descriptors(struct si_context *sctx)
   {
               for (shader = 0; shader < SI_NUM_SHADERS; shader++) {
      struct si_samplers *samplers = &sctx->samplers[shader];
   struct si_images *images = &sctx->images[shader];
            /* Images. */
   mask = images->enabled_mask;
   while (mask) {
                                                /* Sampler views. */
   mask = samplers->enabled_mask;
   while (mask) {
                                                            si_update_all_resident_texture_descriptors(sctx);
      }
      /* SHADER USER DATA */
      static void si_mark_shader_pointers_dirty(struct si_context *sctx, unsigned shader)
   {
      sctx->shader_pointers_dirty |=
            if (shader == PIPE_SHADER_VERTEX)
               }
      void si_shader_pointers_mark_dirty(struct si_context *sctx)
   {
      sctx->shader_pointers_dirty = u_bit_consecutive(0, SI_NUM_DESCS);
   sctx->vertex_buffers_dirty = sctx->num_vertex_elements > 0;
   si_mark_atom_dirty(sctx, &sctx->atoms.s.gfx_shader_pointers);
   sctx->graphics_bindless_pointer_dirty = sctx->bindless_descriptors.buffer != NULL;
   sctx->compute_bindless_pointer_dirty = sctx->bindless_descriptors.buffer != NULL;
   sctx->compute_shaderbuf_sgprs_dirty = true;
   sctx->compute_image_sgprs_dirty = true;
   if (sctx->gfx_level >= GFX11)
      }
      /* Set a base register address for user data constants in the given shader.
   * This assigns a mapping from PIPE_SHADER_* to SPI_SHADER_USER_DATA_*.
   */
   static void si_set_user_data_base(struct si_context *sctx, unsigned shader, uint32_t new_base)
   {
               if (*base != new_base) {
               if (new_base)
            /* Any change in enabled shader stages requires re-emitting
   * the VS state SGPR, because it contains the clamp_vertex_color
   * state, which can be done in VS, TES, and GS.
   */
   sctx->last_vs_state = ~0;
         }
      /* This must be called when these are changed between enabled and disabled
   * - geometry shader
   * - tessellation evaluation shader
   * - NGG
   */
   void si_shader_change_notify(struct si_context *sctx)
   {
      si_set_user_data_base(sctx, PIPE_SHADER_VERTEX,
                        si_get_user_data_base(sctx->gfx_level,
         si_set_user_data_base(sctx, PIPE_SHADER_TESS_EVAL,
                        si_get_user_data_base(sctx->gfx_level,
         /* Update as_* flags in shader keys. Ignore disabled shader stages.
   *   as_ls = VS before TCS
   *   as_es = VS before GS or TES before GS
   *   as_ngg = NGG enabled for the last geometry stage.
   *            If GS sets as_ngg, the previous stage must set as_ngg too.
   */
   if (sctx->shader.tes.cso) {
      sctx->shader.vs.key.ge.as_ls = 1;
   sctx->shader.vs.key.ge.as_es = 0;
            if (sctx->shader.gs.cso) {
      sctx->shader.tes.key.ge.as_es = 1;
   sctx->shader.tes.key.ge.as_ngg = sctx->ngg;
      } else {
      sctx->shader.tes.key.ge.as_es = 0;
         } else if (sctx->shader.gs.cso) {
      sctx->shader.vs.key.ge.as_ls = 0;
   sctx->shader.vs.key.ge.as_es = 1;
   sctx->shader.vs.key.ge.as_ngg = sctx->ngg;
      } else {
      sctx->shader.vs.key.ge.as_ls = 0;
   sctx->shader.vs.key.ge.as_es = 0;
         }
      #define si_emit_consecutive_shader_pointers(sctx, pointer_mask, sh_base, type) do { \
      unsigned sh_reg_base = (sh_base); \
   if (sh_reg_base) { \
      unsigned mask = shader_pointers_dirty & (pointer_mask); \
   \
   if (sctx->screen->info.has_set_pairs_packets) { \
      u_foreach_bit(i, mask) { \
      struct si_descriptors *descs = &sctx->descriptors[i]; \
   unsigned sh_reg = sh_reg_base + descs->shader_userdata_offset; \
   \
         } else { \
      while (mask) { \
      int start, count; \
   u_bit_scan_consecutive_range(&mask, &start, &count); \
   \
   struct si_descriptors *descs = &sctx->descriptors[start]; \
   unsigned sh_offset = sh_reg_base + descs->shader_userdata_offset; \
   \
   radeon_set_sh_reg_seq(sh_offset, count); \
   for (int i = 0; i < count; i++) \
               } while (0)
      static void si_emit_global_shader_pointers(struct si_context *sctx, struct si_descriptors *descs)
   {
               if (sctx->screen->info.has_set_pairs_packets) {
      radeon_push_gfx_sh_reg(R_00B030_SPI_SHADER_USER_DATA_PS_0 + descs->shader_userdata_offset,
         radeon_push_gfx_sh_reg(R_00B230_SPI_SHADER_USER_DATA_GS_0 + descs->shader_userdata_offset,
         radeon_push_gfx_sh_reg(R_00B430_SPI_SHADER_USER_DATA_HS_0 + descs->shader_userdata_offset,
      } else if (sctx->gfx_level >= GFX11) {
      radeon_emit_one_32bit_pointer(sctx, descs, R_00B030_SPI_SHADER_USER_DATA_PS_0);
   radeon_emit_one_32bit_pointer(sctx, descs, R_00B230_SPI_SHADER_USER_DATA_GS_0);
      } else if (sctx->gfx_level >= GFX10) {
      radeon_emit_one_32bit_pointer(sctx, descs, R_00B030_SPI_SHADER_USER_DATA_PS_0);
   /* HW VS stage only used in non-NGG mode. */
   radeon_emit_one_32bit_pointer(sctx, descs, R_00B130_SPI_SHADER_USER_DATA_VS_0);
   radeon_emit_one_32bit_pointer(sctx, descs, R_00B230_SPI_SHADER_USER_DATA_GS_0);
      } else if (sctx->gfx_level == GFX9 && sctx->shadowing.registers) {
      /* We can't use the COMMON registers with register shadowing. */
   radeon_emit_one_32bit_pointer(sctx, descs, R_00B030_SPI_SHADER_USER_DATA_PS_0);
   radeon_emit_one_32bit_pointer(sctx, descs, R_00B130_SPI_SHADER_USER_DATA_VS_0);
   radeon_emit_one_32bit_pointer(sctx, descs, R_00B330_SPI_SHADER_USER_DATA_ES_0);
      } else if (sctx->gfx_level == GFX9) {
      /* Broadcast it to all shader stages. */
      } else {
      radeon_emit_one_32bit_pointer(sctx, descs, R_00B030_SPI_SHADER_USER_DATA_PS_0);
   radeon_emit_one_32bit_pointer(sctx, descs, R_00B130_SPI_SHADER_USER_DATA_VS_0);
   radeon_emit_one_32bit_pointer(sctx, descs, R_00B330_SPI_SHADER_USER_DATA_ES_0);
   radeon_emit_one_32bit_pointer(sctx, descs, R_00B230_SPI_SHADER_USER_DATA_GS_0);
   radeon_emit_one_32bit_pointer(sctx, descs, R_00B430_SPI_SHADER_USER_DATA_HS_0);
                  }
      void si_emit_graphics_shader_pointers(struct si_context *sctx, unsigned index)
   {
      uint32_t *sh_base = sctx->shader_pointers.sh_base;
   unsigned all_gfx_desc_mask = BITFIELD_RANGE(0, SI_DESCS_FIRST_COMPUTE);
   unsigned descriptors_dirty = sctx->descriptors_dirty & all_gfx_desc_mask;
            /* Blits shouldn't set VS shader pointers. */
   if (sctx->num_vs_blit_sgprs)
            /* Upload descriptors. */
   if (descriptors_dirty) {
               do {
                              /* Set shader pointers. */
   if (shader_pointers_dirty & (1 << SI_DESCS_INTERNAL)) {
                  radeon_begin(&sctx->gfx_cs);
   si_emit_consecutive_shader_pointers(sctx, SI_DESCS_SHADER_MASK(VERTEX),
         si_emit_consecutive_shader_pointers(sctx, SI_DESCS_SHADER_MASK(TESS_EVAL),
         si_emit_consecutive_shader_pointers(sctx, SI_DESCS_SHADER_MASK(FRAGMENT),
         si_emit_consecutive_shader_pointers(sctx, SI_DESCS_SHADER_MASK(TESS_CTRL),
         si_emit_consecutive_shader_pointers(sctx, SI_DESCS_SHADER_MASK(GEOMETRY),
            if (sctx->gs_attribute_ring_pointer_dirty) {
      if (sctx->screen->info.has_set_pairs_packets) {
      radeon_push_gfx_sh_reg(R_00B230_SPI_SHADER_USER_DATA_GS_0 +
            } else {
      radeon_set_sh_reg(R_00B230_SPI_SHADER_USER_DATA_GS_0 +
            }
      }
                     if (sctx->graphics_bindless_pointer_dirty) {
      si_emit_global_shader_pointers(sctx, &sctx->bindless_descriptors);
         }
      void si_emit_compute_shader_pointers(struct si_context *sctx)
   {
      /* This does not update internal bindings as that is not needed for compute shaders. */
   unsigned descriptors_dirty = sctx->descriptors_dirty & SI_DESCS_SHADER_MASK(COMPUTE);
            /* Upload descriptors. */
   if (descriptors_dirty) {
               do {
                              /* Set shader pointers. */
   struct radeon_cmdbuf *cs = &sctx->gfx_cs;
            radeon_begin(cs);
   si_emit_consecutive_shader_pointers(sctx, SI_DESCS_SHADER_MASK(COMPUTE),
                  if (sctx->compute_bindless_pointer_dirty) {
      if (sctx->screen->info.has_set_pairs_packets) {
      radeon_push_compute_sh_reg(base + sctx->bindless_descriptors.shader_userdata_offset,
      } else {
         }
               /* Set shader buffer descriptors in user SGPRs. */
   struct si_shader_selector *shader = &sctx->cs_shader_state.program->sel;
            if (num_shaderbufs && sctx->compute_shaderbuf_sgprs_dirty) {
               radeon_set_sh_reg_seq(R_00B900_COMPUTE_USER_DATA_0 +
                  for (unsigned i = 0; i < num_shaderbufs; i++)
                        /* Set image descriptors in user SGPRs. */
   unsigned num_images = shader->cs_num_images_in_user_sgprs;
   if (num_images && sctx->compute_image_sgprs_dirty) {
               radeon_set_sh_reg_seq(R_00B900_COMPUTE_USER_DATA_0 +
                  for (unsigned i = 0; i < num_images; i++) {
                     /* Image buffers are in desc[4..7]. */
   if (BITSET_TEST(shader->info.base.image_buffers, i)) {
      desc_offset += 4;
                              }
      }
      /* BINDLESS */
      static void si_init_bindless_descriptors(struct si_context *sctx, struct si_descriptors *desc,
         {
               si_init_descriptors(desc, shader_userdata_rel_index, 16, num_elements);
            /* The first bindless descriptor is stored at slot 1, because 0 is not
   * considered to be a valid handle.
   */
            /* Track which bindless slots are used (or not). */
            /* Reserve slot 0 because it's an invalid handle for bindless. */
   desc_slot = util_idalloc_alloc(&sctx->bindless_used_slots);
      }
      static void si_release_bindless_descriptors(struct si_context *sctx)
   {
      si_release_descriptors(&sctx->bindless_descriptors);
      }
      static unsigned si_get_first_free_bindless_slot(struct si_context *sctx)
   {
      struct si_descriptors *desc = &sctx->bindless_descriptors;
            desc_slot = util_idalloc_alloc(&sctx->bindless_used_slots);
   if (desc_slot >= desc->num_elements) {
      /* The array of bindless descriptors is full, resize it. */
   unsigned slot_size = desc->element_dw_size * 4;
            desc->list =
         desc->num_elements = new_num_elements;
               assert(desc_slot);
      }
      static unsigned si_create_bindless_descriptor(struct si_context *sctx, uint32_t *desc_list,
         {
      struct si_descriptors *desc = &sctx->bindless_descriptors;
            /* Find a free slot. */
            /* For simplicity, sampler and image bindless descriptors use fixed
   * 16-dword slots for now. Image descriptors only need 8-dword but this
   * doesn't really matter because no real apps use image handles.
   */
            /* Copy the descriptor into the array. */
            /* Re-upload the whole array of bindless descriptors into a new buffer.
   */
            /* Make sure to re-emit the shader pointers for all stages. */
   sctx->graphics_bindless_pointer_dirty = true;
   sctx->compute_bindless_pointer_dirty = true;
               }
      static void si_update_bindless_buffer_descriptor(struct si_context *sctx, unsigned desc_slot,
               {
      struct si_descriptors *desc = &sctx->bindless_descriptors;
   struct si_resource *buf = si_resource(resource);
   unsigned desc_slot_offset = desc_slot * 16;
   uint32_t *desc_list = desc->list + desc_slot_offset + 4;
                     /* Retrieve the old buffer addr from the descriptor. */
            if (old_desc_va != buf->gpu_address + offset) {
      /* The buffer has been invalidated when the handle wasn't
   * resident, update the descriptor and the dirty flag.
   */
                  }
      static uint64_t si_create_texture_handle(struct pipe_context *ctx, struct pipe_sampler_view *view,
         {
      struct si_sampler_view *sview = (struct si_sampler_view *)view;
   struct si_context *sctx = (struct si_context *)ctx;
   struct si_texture_handle *tex_handle;
   struct si_sampler_state *sstate;
   uint32_t desc_list[16];
            tex_handle = CALLOC_STRUCT(si_texture_handle);
   if (!tex_handle)
            memset(desc_list, 0, sizeof(desc_list));
            sstate = ctx->create_sampler_state(ctx, state);
   if (!sstate) {
      FREE(tex_handle);
               si_set_sampler_view_desc(sctx, sview, sstate, &desc_list[0]);
   memcpy(&tex_handle->sstate, sstate, sizeof(*sstate));
            tex_handle->desc_slot = si_create_bindless_descriptor(sctx, desc_list, sizeof(desc_list));
   if (!tex_handle->desc_slot) {
      FREE(tex_handle);
                        if (!_mesa_hash_table_insert(sctx->tex_handles, (void *)(uintptr_t)handle, tex_handle)) {
      FREE(tex_handle);
                                    }
      static void si_delete_texture_handle(struct pipe_context *ctx, uint64_t handle)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_texture_handle *tex_handle;
            entry = _mesa_hash_table_search(sctx->tex_handles, (void *)(uintptr_t)handle);
   if (!entry)
                     /* Allow this descriptor slot to be re-used. */
            pipe_sampler_view_reference(&tex_handle->view, NULL);
   _mesa_hash_table_remove(sctx->tex_handles, entry);
      }
      static void si_make_texture_handle_resident(struct pipe_context *ctx, uint64_t handle,
         {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_texture_handle *tex_handle;
   struct si_sampler_view *sview;
            entry = _mesa_hash_table_search(sctx->tex_handles, (void *)(uintptr_t)handle);
   if (!entry)
            tex_handle = (struct si_texture_handle *)entry->data;
            if (resident) {
      if (sview->base.texture->target != PIPE_BUFFER) {
               if (depth_needs_decompression(tex, sview->is_stencil_sampler)) {
      util_dynarray_append(&sctx->resident_tex_needs_depth_decompress,
               if (color_needs_decompression(tex)) {
      util_dynarray_append(&sctx->resident_tex_needs_color_decompress,
               if (vi_dcc_enabled(tex, sview->base.u.tex.first_level) &&
                     } else {
      si_update_bindless_buffer_descriptor(sctx, tex_handle->desc_slot, sview->base.texture,
               /* Re-upload the descriptor if it has been updated while it
   * wasn't resident.
   */
   if (tex_handle->desc_dirty)
            /* Add the texture handle to the per-context list. */
            /* Add the buffers to the current CS in case si_begin_new_cs()
   * is not going to be called.
   */
   si_sampler_view_add_buffer(sctx, sview->base.texture, RADEON_USAGE_READ,
      } else {
      /* Remove the texture handle from the per-context list. */
   util_dynarray_delete_unordered(&sctx->resident_tex_handles, struct si_texture_handle *,
            if (sview->base.texture->target != PIPE_BUFFER) {
                     util_dynarray_delete_unordered(&sctx->resident_tex_needs_color_decompress,
            }
      static uint64_t si_create_image_handle(struct pipe_context *ctx, const struct pipe_image_view *view)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_image_handle *img_handle;
   uint32_t desc_list[16];
            if (!view || !view->resource)
            img_handle = CALLOC_STRUCT(si_image_handle);
   if (!img_handle)
            memset(desc_list, 0, sizeof(desc_list));
                     img_handle->desc_slot = si_create_bindless_descriptor(sctx, desc_list, sizeof(desc_list));
   if (!img_handle->desc_slot) {
      FREE(img_handle);
                        if (!_mesa_hash_table_insert(sctx->img_handles, (void *)(uintptr_t)handle, img_handle)) {
      FREE(img_handle);
                                    }
      static void si_delete_image_handle(struct pipe_context *ctx, uint64_t handle)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_image_handle *img_handle;
            entry = _mesa_hash_table_search(sctx->img_handles, (void *)(uintptr_t)handle);
   if (!entry)
                     util_copy_image_view(&img_handle->view, NULL);
   _mesa_hash_table_remove(sctx->img_handles, entry);
      }
      static void si_make_image_handle_resident(struct pipe_context *ctx, uint64_t handle,
         {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_image_handle *img_handle;
   struct pipe_image_view *view;
   struct si_resource *res;
            entry = _mesa_hash_table_search(sctx->img_handles, (void *)(uintptr_t)handle);
   if (!entry)
            img_handle = (struct si_image_handle *)entry->data;
   view = &img_handle->view;
            if (resident) {
      if (res->b.b.target != PIPE_BUFFER) {
                     if (color_needs_decompression(tex)) {
      util_dynarray_append(&sctx->resident_img_needs_color_decompress,
                                 } else {
      si_update_bindless_buffer_descriptor(sctx, img_handle->desc_slot, view->resource,
               /* Re-upload the descriptor if it has been updated while it
   * wasn't resident.
   */
   if (img_handle->desc_dirty)
            /* Add the image handle to the per-context list. */
            /* Add the buffers to the current CS in case si_begin_new_cs()
   * is not going to be called.
   */
   si_sampler_view_add_buffer(sctx, view->resource,
            } else {
      /* Remove the image handle from the per-context list. */
   util_dynarray_delete_unordered(&sctx->resident_img_handles, struct si_image_handle *,
            if (res->b.b.target != PIPE_BUFFER) {
      util_dynarray_delete_unordered(&sctx->resident_img_needs_color_decompress,
            }
      static void si_resident_buffers_add_all_to_bo_list(struct si_context *sctx)
   {
               num_resident_tex_handles = sctx->resident_tex_handles.size / sizeof(struct si_texture_handle *);
            /* Add all resident texture handles. */
   util_dynarray_foreach (&sctx->resident_tex_handles, struct si_texture_handle *, tex_handle) {
               si_sampler_view_add_buffer(sctx, sview->base.texture, RADEON_USAGE_READ,
               /* Add all resident image handles. */
   util_dynarray_foreach (&sctx->resident_img_handles, struct si_image_handle *, img_handle) {
                           sctx->num_resident_handles += num_resident_tex_handles + num_resident_img_handles;
   assert(sctx->bo_list_add_all_resident_resources);
      }
      static void si_emit_gfx_resources_add_all_to_bo_list(struct si_context *sctx, unsigned index);
      /* INIT/DEINIT/UPLOAD */
      void si_init_all_descriptors(struct si_context *sctx)
   {
      int i;
   unsigned first_shader = sctx->has_graphics ? 0 : PIPE_SHADER_COMPUTE;
            if (sctx->gfx_level >= GFX11) {
      hs_sgpr0 = R_00B420_SPI_SHADER_PGM_LO_HS;
      } else {
      hs_sgpr0 = R_00B408_SPI_SHADER_USER_DATA_ADDR_LO_HS;
               for (i = first_shader; i < SI_NUM_SHADERS; i++) {
      bool is_2nd =
         unsigned num_sampler_slots = SI_NUM_IMAGE_SLOTS / 2 + SI_NUM_SAMPLERS;
   unsigned num_buffer_slots = SI_NUM_SHADER_BUFFERS + SI_NUM_CONST_BUFFERS;
   int rel_dw_offset;
            if (is_2nd) {
      if (i == PIPE_SHADER_TESS_CTRL) {
      rel_dw_offset =
      } else if (sctx->gfx_level >= GFX10) { /* PIPE_SHADER_GEOMETRY */
      rel_dw_offset =
      } else {
      rel_dw_offset =
         } else {
         }
   desc = si_const_and_shader_buffer_descriptors(sctx, i);
   si_init_buffer_resources(sctx, &sctx->const_and_shader_buffers[i], desc, num_buffer_slots,
                        if (is_2nd) {
      if (i == PIPE_SHADER_TESS_CTRL) {
      rel_dw_offset =
      } else if (sctx->gfx_level >= GFX10) { /* PIPE_SHADER_GEOMETRY */
      rel_dw_offset =
      } else {
      rel_dw_offset =
         } else {
                  desc = si_sampler_and_image_descriptors(sctx, i);
            int j;
   for (j = 0; j < SI_NUM_IMAGE_SLOTS; j++)
         for (; j < SI_NUM_IMAGE_SLOTS + SI_NUM_SAMPLERS * 2; j++)
               si_init_buffer_resources(sctx, &sctx->internal_bindings, &sctx->descriptors[SI_DESCS_INTERNAL],
                                    /* Initialize an array of 1024 bindless descriptors, when the limit is
   * reached, just make it larger and re-upload the whole array.
   */
   si_init_bindless_descriptors(sctx, &sctx->bindless_descriptors,
                     /* Set pipe_context functions. */
   sctx->b.bind_sampler_states = si_bind_sampler_states;
   sctx->b.set_shader_images = si_set_shader_images;
   sctx->b.set_constant_buffer = si_pipe_set_constant_buffer;
   sctx->b.set_inlinable_constants = si_set_inlinable_constants;
   sctx->b.set_shader_buffers = si_pipe_set_shader_buffers;
   sctx->b.set_sampler_views = si_pipe_set_sampler_views;
   sctx->b.create_texture_handle = si_create_texture_handle;
   sctx->b.delete_texture_handle = si_delete_texture_handle;
   sctx->b.make_texture_handle_resident = si_make_texture_handle_resident;
   sctx->b.create_image_handle = si_create_image_handle;
   sctx->b.delete_image_handle = si_delete_image_handle;
            if (!sctx->has_graphics)
                     sctx->atoms.s.gfx_add_all_to_bo_list.emit = si_emit_gfx_resources_add_all_to_bo_list;
            /* Set default and immutable mappings. */
   si_set_user_data_base(sctx, PIPE_SHADER_VERTEX,
               si_set_user_data_base(sctx, PIPE_SHADER_TESS_CTRL,
               si_set_user_data_base(sctx, PIPE_SHADER_GEOMETRY,
                  }
      void si_release_all_descriptors(struct si_context *sctx)
   {
               for (i = 0; i < SI_NUM_SHADERS; i++) {
      si_release_buffer_resources(&sctx->const_and_shader_buffers[i],
         si_release_sampler_views(&sctx->samplers[i]);
      }
   si_release_buffer_resources(&sctx->internal_bindings, &sctx->descriptors[SI_DESCS_INTERNAL]);
   for (i = 0; i < SI_NUM_VERTEX_BUFFERS; i++)
            for (i = 0; i < SI_NUM_DESCS; ++i)
               }
      bool si_gfx_resources_check_encrypted(struct si_context *sctx)
   {
               for (unsigned i = 0; i < SI_NUM_GRAPHICS_SHADERS && !use_encrypted_bo; i++) {
      struct si_shader_ctx_state *current_shader = &sctx->shaders[i];
   if (!current_shader->cso)
            use_encrypted_bo |=
         use_encrypted_bo |=
      si_sampler_views_check_encrypted(sctx, &sctx->samplers[i],
      use_encrypted_bo |= si_image_views_check_encrypted(sctx, &sctx->images[i],
      }
            struct si_state_blend *blend = sctx->queued.named.blend;
   for (int i = 0; i < sctx->framebuffer.state.nr_cbufs && !use_encrypted_bo; i++) {
      struct pipe_surface *surf = sctx->framebuffer.state.cbufs[i];
   if (surf && surf->texture) {
      struct si_texture *tex = (struct si_texture *)surf->texture;
                  /* Are we reading from this framebuffer */
   if (((blend->blend_enable_4bit >> (4 * i)) & 0xf) ||
      vi_dcc_enabled(tex, 0)) {
                     if (sctx->framebuffer.state.zsbuf) {
      struct si_texture* zs = (struct si_texture *)sctx->framebuffer.state.zsbuf->texture;
   if (zs &&
      (zs->buffer.flags & RADEON_FLAG_ENCRYPTED)) {
   /* TODO: This isn't needed if depth.func is PIPE_FUNC_NEVER or PIPE_FUNC_ALWAYS */
               #ifndef NDEBUG
      if (use_encrypted_bo) {
      /* Verify that color buffers are encrypted */
   for (int i = 0; i < sctx->framebuffer.state.nr_cbufs; i++) {
      struct pipe_surface *surf = sctx->framebuffer.state.cbufs[i];
   if (!surf)
         struct si_texture *tex = (struct si_texture *)surf->texture;
      }
   /* Verify that depth/stencil buffer is encrypted */
   if (sctx->framebuffer.state.zsbuf) {
      struct pipe_surface *surf = sctx->framebuffer.state.zsbuf;
   struct si_texture *tex = (struct si_texture *)surf->texture;
            #endif
            }
      static void si_emit_gfx_resources_add_all_to_bo_list(struct si_context *sctx, unsigned index)
   {
      for (unsigned i = 0; i < SI_NUM_GRAPHICS_SHADERS; i++) {
      si_buffer_resources_begin_new_cs(sctx, &sctx->const_and_shader_buffers[i]);
   si_sampler_views_begin_new_cs(sctx, &sctx->samplers[i]);
      }
            for (unsigned i = 0; i < ARRAY_SIZE(sctx->vertex_buffer); i++) {
      struct si_resource *buf = si_resource(sctx->vertex_buffer[i].buffer.resource);
   if (buf) {
      radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, buf,
                  if (sctx->bo_list_add_all_resident_resources)
      }
      bool si_compute_resources_check_encrypted(struct si_context *sctx)
   {
                        /* TODO: we should assert that either use_encrypted_bo is false,
   * or all writable buffers are encrypted.
   */
   return si_buffer_resources_check_encrypted(sctx, &sctx->const_and_shader_buffers[sh]) ||
         si_sampler_views_check_encrypted(sctx, &sctx->samplers[sh], info->base.textures_used[0]) ||
      }
      void si_compute_resources_add_all_to_bo_list(struct si_context *sctx)
   {
               si_buffer_resources_begin_new_cs(sctx, &sctx->const_and_shader_buffers[sh]);
   si_sampler_views_begin_new_cs(sctx, &sctx->samplers[sh]);
   si_image_views_begin_new_cs(sctx, &sctx->images[sh]);
            if (sctx->bo_list_add_all_resident_resources)
            assert(sctx->bo_list_add_all_compute_resources);
      }
      void si_add_all_descriptors_to_bo_list(struct si_context *sctx)
   {
      for (unsigned i = 0; i < SI_NUM_DESCS; ++i)
                  sctx->bo_list_add_all_resident_resources = true;
   si_mark_atom_dirty(sctx, &sctx->atoms.s.gfx_add_all_to_bo_list);
      }
      void si_set_active_descriptors(struct si_context *sctx, unsigned desc_idx, uint64_t new_active_mask)
   {
               /* Ignore no-op updates and updates that disable all slots. */
   if (!new_active_mask ||
      new_active_mask == u_bit_consecutive64(desc->first_active_slot, desc->num_active_slots))
         int first, count;
   u_bit_scan_consecutive_range64(&new_active_mask, &first, &count);
            /* Upload/dump descriptors if slots are being enabled. */
   if (first < desc->first_active_slot ||
      first + count > desc->first_active_slot + desc->num_active_slots) {
   sctx->descriptors_dirty |= 1u << desc_idx;
   if (desc_idx < SI_DESCS_FIRST_COMPUTE)
               desc->first_active_slot = first;
      }
      void si_set_active_descriptors_for_shader(struct si_context *sctx, struct si_shader_selector *sel)
   {
      if (!sel)
            si_set_active_descriptors(sctx, sel->const_and_shader_buf_descriptors_index,
         si_set_active_descriptors(sctx, sel->sampler_and_images_descriptors_index,
      }
