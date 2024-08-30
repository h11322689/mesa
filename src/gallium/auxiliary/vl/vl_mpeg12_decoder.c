   /**************************************************************************
   *
   * Copyright 2009 Younes Manton.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <math.h>
   #include <assert.h>
      #include "util/u_memory.h"
   #include "util/u_sampler.h"
   #include "util/u_surface.h"
   #include "util/u_video.h"
      #include "vl_mpeg12_decoder.h"
   #include "vl_defines.h"
      #define SCALE_FACTOR_SNORM (32768.0f / 256.0f)
   #define SCALE_FACTOR_SSCALED (1.0f / 256.0f)
      struct format_config {
      enum pipe_format zscan_source_format;
   enum pipe_format idct_source_format;
            float idct_scale;
      };
      static const struct format_config bitstream_format_config[] = {
   //   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SSCALED },
   //   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, 1.0f, SCALE_FACTOR_SSCALED },
      { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SNORM },
      };
      static const unsigned num_bitstream_format_configs =
            static const struct format_config idct_format_config[] = {
   //   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SSCALED },
   //   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, 1.0f, SCALE_FACTOR_SSCALED },
      { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SNORM },
      };
      static const unsigned num_idct_format_configs =
            static const struct format_config mc_format_config[] = {
      //{ PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_NONE, PIPE_FORMAT_R16_SSCALED, 0.0f, SCALE_FACTOR_SSCALED },
      };
      static const unsigned num_mc_format_configs =
            static const unsigned const_empty_block_mask_420[3][2][2] = {
      { { 0x20, 0x10 },  { 0x08, 0x04 } },
   { { 0x02, 0x02 },  { 0x02, 0x02 } },
      };
      struct video_buffer_private
   {
      struct list_head list;
            struct pipe_sampler_view *sampler_view_planes[VL_NUM_COMPONENTS];
               };
      static void
   vl_mpeg12_destroy_buffer(struct vl_mpeg12_buffer *buf);
      static void
   destroy_video_buffer_private(void *private)
   {
      struct video_buffer_private *priv = private;
                     for (i = 0; i < VL_NUM_COMPONENTS; ++i)
            for (i = 0; i < VL_MAX_SURFACES; ++i)
            if (priv->buffer)
               }
      static struct video_buffer_private *
   get_video_buffer_private(struct vl_mpeg12_decoder *dec, struct pipe_video_buffer *buf)
   {
      struct pipe_context *pipe = dec->context;
   struct video_buffer_private *priv;
   struct pipe_sampler_view **sv;
   struct pipe_surface **surf;
            priv = vl_video_buffer_get_associated_data(buf, &dec->base);
   if (priv)
                     list_add(&priv->list, &dec->buffer_privates);
            sv = buf->get_sampler_view_planes(buf);
   for (i = 0; i < VL_NUM_COMPONENTS; ++i)
      if (sv[i])
         surf = buf->get_surfaces(buf);
   for (i = 0; i < VL_MAX_SURFACES; ++i)
      if (surf[i])
                     }
      static void
   free_video_buffer_privates(struct vl_mpeg12_decoder *dec)
   {
               LIST_FOR_EACH_ENTRY_SAFE(priv, next, &dec->buffer_privates, list) {
                     }
      static bool
   init_zscan_buffer(struct vl_mpeg12_decoder *dec, struct vl_mpeg12_buffer *buffer)
   {
      struct pipe_resource *res, res_tmpl;
   struct pipe_sampler_view sv_tmpl;
                              memset(&res_tmpl, 0, sizeof(res_tmpl));
   res_tmpl.target = PIPE_TEXTURE_2D;
   res_tmpl.format = dec->zscan_source_format;
   res_tmpl.width0 = dec->blocks_per_line * VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT;
   res_tmpl.height0 = align(dec->num_blocks, dec->blocks_per_line) / dec->blocks_per_line;
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.usage = PIPE_USAGE_STREAM;
            res = dec->context->screen->resource_create(dec->context->screen, &res_tmpl);
   if (!res)
               memset(&sv_tmpl, 0, sizeof(sv_tmpl));
   u_sampler_view_default_template(&sv_tmpl, res, res->format);
   sv_tmpl.swizzle_r = sv_tmpl.swizzle_g = sv_tmpl.swizzle_b = sv_tmpl.swizzle_a = PIPE_SWIZZLE_X;
   buffer->zscan_source = dec->context->create_sampler_view(dec->context, res, &sv_tmpl);
   pipe_resource_reference(&res, NULL);
   if (!buffer->zscan_source)
            if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
         else
            if (!destination)
            for (i = 0; i < VL_NUM_COMPONENTS; ++i)
      if (!vl_zscan_init_buffer(i == 0 ? &dec->zscan_y : &dec->zscan_c,
                     error_plane:
      for (; i > 0; --i)
         error_surface:
   error_sampler:
            error_source:
         }
      static void
   cleanup_zscan_buffer(struct vl_mpeg12_buffer *buffer)
   {
                        for (i = 0; i < VL_NUM_COMPONENTS; ++i)
               }
      static bool
   init_idct_buffer(struct vl_mpeg12_decoder *dec, struct vl_mpeg12_buffer *buffer)
   {
                                 idct_source_sv = dec->idct_source->get_sampler_view_planes(dec->idct_source);
   if (!idct_source_sv)
            mc_source_sv = dec->mc_source->get_sampler_view_planes(dec->mc_source);
   if (!mc_source_sv)
            for (i = 0; i < 3; ++i)
      if (!vl_idct_init_buffer(i == 0 ? &dec->idct_y : &dec->idct_c,
                           error_plane:
      for (; i > 0; --i)
         error_mc_source_sv:
   error_source_sv:
         }
      static void
   cleanup_idct_buffer(struct vl_mpeg12_buffer *buf)
   {
      unsigned i;
               for (i = 0; i < 3; ++i)
      }
      static bool
   init_mc_buffer(struct vl_mpeg12_decoder *dec, struct vl_mpeg12_buffer *buf)
   {
               if(!vl_mc_init_buffer(&dec->mc_y, &buf->mc[0]))
            if(!vl_mc_init_buffer(&dec->mc_c, &buf->mc[1]))
            if(!vl_mc_init_buffer(&dec->mc_c, &buf->mc[2]))
                  error_mc_cr:
            error_mc_cb:
            error_mc_y:
         }
      static void
   cleanup_mc_buffer(struct vl_mpeg12_buffer *buf)
   {
                        for (i = 0; i < VL_NUM_COMPONENTS; ++i)
      }
      static inline void
   MacroBlockTypeToPipeWeights(const struct pipe_mpeg12_macroblock *mb, unsigned weights[2])
   {
               switch (mb->macroblock_type & (PIPE_MPEG12_MB_TYPE_MOTION_FORWARD | PIPE_MPEG12_MB_TYPE_MOTION_BACKWARD)) {
   case PIPE_MPEG12_MB_TYPE_MOTION_FORWARD:
      weights[0] = PIPE_VIDEO_MV_WEIGHT_MAX;
   weights[1] = PIPE_VIDEO_MV_WEIGHT_MIN;
         case (PIPE_MPEG12_MB_TYPE_MOTION_FORWARD | PIPE_MPEG12_MB_TYPE_MOTION_BACKWARD):
      weights[0] = PIPE_VIDEO_MV_WEIGHT_HALF;
   weights[1] = PIPE_VIDEO_MV_WEIGHT_HALF;
         case PIPE_MPEG12_MB_TYPE_MOTION_BACKWARD:
      weights[0] = PIPE_VIDEO_MV_WEIGHT_MIN;
   weights[1] = PIPE_VIDEO_MV_WEIGHT_MAX;
         default:
      if (mb->macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA) {
      weights[0] = PIPE_VIDEO_MV_WEIGHT_MIN;
      } else {
      /* no motion vector, but also not intra mb ->
         weights[0] = PIPE_VIDEO_MV_WEIGHT_MAX;
      }
         }
      static inline struct vl_motionvector
   MotionVectorToPipe(const struct pipe_mpeg12_macroblock *mb, unsigned vector,
         {
                        if (mb->macroblock_type & (PIPE_MPEG12_MB_TYPE_MOTION_FORWARD | PIPE_MPEG12_MB_TYPE_MOTION_BACKWARD)) {
      switch (mb->macroblock_modes.bits.frame_motion_type) {
   case PIPE_MPEG12_MO_TYPE_FRAME:
      mv.top.x = mb->PMV[0][vector][0];
   mv.top.y = mb->PMV[0][vector][1];
                  mv.bottom.x = mb->PMV[0][vector][0];
   mv.bottom.y = mb->PMV[0][vector][1];
   mv.bottom.weight = weight;
               case PIPE_MPEG12_MO_TYPE_FIELD:
      mv.top.x = mb->PMV[0][vector][0];
   mv.top.y = mb->PMV[0][vector][1];
   mv.top.field_select = (mb->motion_vertical_field_select & field_select_mask) ?
                  mv.bottom.x = mb->PMV[1][vector][0];
   mv.bottom.y = mb->PMV[1][vector][1];
   mv.bottom.field_select = (mb->motion_vertical_field_select & (field_select_mask << 2)) ?
                     default:
            } else {
      mv.top.x = mv.top.y = 0;
   mv.top.field_select = PIPE_VIDEO_FRAME;
            mv.bottom.x = mv.bottom.y = 0;
   mv.bottom.field_select = PIPE_VIDEO_FRAME;
      }
      }
      static inline void
   UploadYcbcrBlocks(struct vl_mpeg12_decoder *dec,
               {
      unsigned intra;
            assert(dec && buf);
            if (!mb->coded_block_pattern)
                     for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x) {
                  struct vl_ycbcr_block *stream = buf->ycbcr_stream[0];
   stream->x = mb->x * 2 + x;
   stream->y = mb->y * 2 + y;
   stream->intra = intra;
                                                   /* TODO: Implement 422, 444 */
            for (tb = 1; tb < 3; ++tb) {
                  struct vl_ycbcr_block *stream = buf->ycbcr_stream[tb];
   stream->x = mb->x;
   stream->y = mb->y;
   stream->intra = intra;
                                                memcpy(buf->texels, mb->blocks, 64 * sizeof(short) * num_blocks);
      }
      static void
   vl_mpeg12_destroy_buffer(struct vl_mpeg12_buffer *buf)
   {
                  cleanup_zscan_buffer(buf);
   cleanup_idct_buffer(buf);
   cleanup_mc_buffer(buf);
               }
      static void
   vl_mpeg12_destroy(struct pipe_video_codec *decoder)
   {
      struct vl_mpeg12_decoder *dec = (struct vl_mpeg12_decoder*)decoder;
                              /* Asserted in softpipe_delete_fs_state() for some reason */
   dec->context->bind_vs_state(dec->context, NULL);
            dec->context->delete_depth_stencil_alpha_state(dec->context, dec->dsa);
            vl_mc_cleanup(&dec->mc_y);
   vl_mc_cleanup(&dec->mc_c);
            if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      vl_idct_cleanup(&dec->idct_y);
   vl_idct_cleanup(&dec->idct_c);
               vl_zscan_cleanup(&dec->zscan_y);
            dec->context->delete_vertex_elements_state(dec->context, dec->ves_ycbcr);
            pipe_resource_reference(&dec->quads.buffer.resource, NULL);
            pipe_sampler_view_reference(&dec->zscan_linear, NULL);
   pipe_sampler_view_reference(&dec->zscan_normal, NULL);
            for (i = 0; i < 4; ++i)
      if (dec->dec_buffers[i])
                     }
      static struct vl_mpeg12_buffer *
   vl_mpeg12_get_decode_buffer(struct vl_mpeg12_decoder *dec, struct pipe_video_buffer *target)
   {
      struct video_buffer_private *priv;
                     priv = get_video_buffer_private(dec, target);
   if (priv->buffer)
            buffer = dec->dec_buffers[dec->current_buffer];
   if (buffer)
            buffer = CALLOC_STRUCT(vl_mpeg12_buffer);
   if (!buffer)
            if (!vl_vb_init(&buffer->vertex_stream, dec->context,
                        if (!init_mc_buffer(dec, buffer))
            if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
      if (!init_idct_buffer(dec, buffer))
         if (!init_zscan_buffer(dec, buffer))
            if (dec->base.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM)
            if (dec->base.expect_chunked_decode)
         else
                  error_zscan:
            error_idct:
            error_mc:
            error_vertex_buffer:
      FREE(buffer);
      }
      static void
   vl_mpeg12_begin_frame(struct pipe_video_codec *decoder,
               {
      struct vl_mpeg12_decoder *dec = (struct vl_mpeg12_decoder *)decoder;
   struct pipe_mpeg12_picture_desc *desc = (struct pipe_mpeg12_picture_desc *)picture;
            struct pipe_resource *tex;
            uint8_t intra_matrix[64];
                              buf = vl_mpeg12_get_decode_buffer(dec, target);
            if (dec->base.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
      memcpy(intra_matrix, desc->intra_matrix, sizeof(intra_matrix));
   memcpy(non_intra_matrix, desc->non_intra_matrix, sizeof(non_intra_matrix));
      } else {
      memset(intra_matrix, 0x10, sizeof(intra_matrix));
               for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
      struct vl_zscan *zscan = i == 0 ? &dec->zscan_y : &dec->zscan_c;
   vl_zscan_upload_quant(zscan, &buf->zscan[i], intra_matrix, true);
                        tex = buf->zscan_source->texture;
   rect.width = tex->width0;
            buf->texels =
      dec->context->texture_map(dec->context, tex, 0,
                              for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
      buf->ycbcr_stream[i] = vl_vb_get_ycbcr_stream(&buf->vertex_stream, i);
               for (i = 0; i < VL_MAX_REF_FRAMES; ++i)
            if (dec->base.entrypoint >= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      for (i = 0; i < VL_NUM_COMPONENTS; ++i)
         }
      static void
   vl_mpeg12_decode_macroblock(struct pipe_video_codec *decoder,
                           {
      struct vl_mpeg12_decoder *dec = (struct vl_mpeg12_decoder *)decoder;
   const struct pipe_mpeg12_macroblock *mb = (const struct pipe_mpeg12_macroblock *)macroblocks;
   struct pipe_mpeg12_picture_desc *desc = (struct pipe_mpeg12_picture_desc *)picture;
                     assert(dec && target && picture);
            buf = vl_mpeg12_get_decode_buffer(dec, target);
            for (; num_macroblocks > 0; --num_macroblocks) {
               if (mb->macroblock_type & (PIPE_MPEG12_MB_TYPE_PATTERN | PIPE_MPEG12_MB_TYPE_INTRA))
                     for (i = 0; i < 2; ++i) {
               buf->mv_stream[i][mb_addr] = MotionVectorToPipe
   (
      mb, i,
   i ? PIPE_MPEG12_FS_FIRST_BACKWARD : PIPE_MPEG12_FS_FIRST_FORWARD,
                  /* see section 7.6.6 of the spec */
   if (mb->num_skipped_macroblocks > 0) {
               if (desc->ref[0] && !desc->ref[1]) {
      skipped_mv[0].top.x = skipped_mv[0].top.y = 0;
      } else {
   skipped_mv[0] = buf->mv_stream[0][mb_addr];
   skipped_mv[1] = buf->mv_stream[1][mb_addr];
   }
                                 ++mb_addr;
   for (i = 0; i < mb->num_skipped_macroblocks; ++i, ++mb_addr) {
      for (j = 0; j < 2; ++j) {
                                       }
      static void
   vl_mpeg12_decode_bitstream(struct pipe_video_codec *decoder,
                                 {
      struct vl_mpeg12_decoder *dec = (struct vl_mpeg12_decoder *)decoder;
   struct pipe_mpeg12_picture_desc *desc = (struct pipe_mpeg12_picture_desc *)picture;
   struct vl_mpeg12_buffer *buf;
                        buf = vl_mpeg12_get_decode_buffer(dec, target);
            for (i = 0; i < VL_NUM_COMPONENTS; ++i)
      vl_zscan_set_layout(&buf->zscan[i], desc->alternate_scan ?
            }
      static void
   vl_mpeg12_end_frame(struct pipe_video_codec *decoder,
               {
      struct vl_mpeg12_decoder *dec = (struct vl_mpeg12_decoder *)decoder;
   struct pipe_mpeg12_picture_desc *desc = (struct pipe_mpeg12_picture_desc *)picture;
   struct pipe_sampler_view **ref_frames[2];
   struct pipe_sampler_view **mc_source_sv;
   struct pipe_surface **target_surfaces;
   struct pipe_vertex_buffer vb[3];
            const unsigned *plane_order;
   unsigned i, j, component;
            assert(dec && target && picture);
                              if (buf->tex_transfer)
            vb[0] = dec->quads;
                     for (i = 0; i < VL_MAX_REF_FRAMES; ++i) {
      if (desc->ref[i])
         else
               dec->context->bind_vertex_elements_state(dec->context, dec->ves_mv);
   for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
                        for (j = 0; j < VL_MAX_REF_FRAMES; ++j) {
                                             dec->context->bind_vertex_elements_state(dec->context, dec->ves_ycbcr);
   for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
               vb[1] = vl_vb_get_ycbcr(&buf->vertex_stream, i);
                     if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
               plane_order = vl_video_buffer_plane_order(target->buffer_format);
   mc_source_sv = dec->mc_source->get_sampler_view_planes(dec->mc_source);
   for (i = 0, component = 0; component < VL_NUM_COMPONENTS; ++i) {
               nr_components = util_format_get_nr_components(target_surfaces[i]->texture->format);
   for (j = 0; j < nr_components; ++j, ++component) {
                                    if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
         else {
      dec->context->set_sampler_views(dec->context,
               dec->context->bind_sampler_states(dec->context,
            }
         }
   dec->context->flush(dec->context, NULL, 0);
   ++dec->current_buffer;
      }
      static void
   vl_mpeg12_flush(struct pipe_video_codec *decoder)
   {
                  }
      static bool
   init_pipe_state(struct vl_mpeg12_decoder *dec)
   {
      struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_sampler_state sampler;
                     memset(&dsa, 0, sizeof dsa);
   dsa.depth_enabled = 0;
   dsa.depth_writemask = 0;
   dsa.depth_func = PIPE_FUNC_ALWAYS;
   for (i = 0; i < 2; ++i) {
      dsa.stencil[i].enabled = 0;
   dsa.stencil[i].func = PIPE_FUNC_ALWAYS;
   dsa.stencil[i].fail_op = PIPE_STENCIL_OP_KEEP;
   dsa.stencil[i].zpass_op = PIPE_STENCIL_OP_KEEP;
   dsa.stencil[i].zfail_op = PIPE_STENCIL_OP_KEEP;
   dsa.stencil[i].valuemask = 0;
      }
   dsa.alpha_enabled = 0;
   dsa.alpha_func = PIPE_FUNC_ALWAYS;
   dsa.alpha_ref_value = 0;
   dec->dsa = dec->context->create_depth_stencil_alpha_state(dec->context, &dsa);
            memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler.compare_func = PIPE_FUNC_ALWAYS;
   dec->sampler_ycbcr = dec->context->create_sampler_state(dec->context, &sampler);
   if (!dec->sampler_ycbcr)
               }
      static const struct format_config*
   find_format_config(struct vl_mpeg12_decoder *dec, const struct format_config configs[], unsigned num_configs)
   {
      struct pipe_screen *screen;
                              for (i = 0; i < num_configs; ++i) {
      if (!screen->is_format_supported(screen, configs[i].zscan_source_format, PIPE_TEXTURE_2D,
                  if (configs[i].idct_source_format != PIPE_FORMAT_NONE) {
      if (!screen->is_format_supported(screen, configs[i].idct_source_format, PIPE_TEXTURE_2D,
                  if (!screen->is_format_supported(screen, configs[i].mc_source_format, PIPE_TEXTURE_3D,
            } else {
      if (!screen->is_format_supported(screen, configs[i].mc_source_format, PIPE_TEXTURE_2D,
            }
                  }
      static bool
   init_zscan(struct vl_mpeg12_decoder *dec, const struct format_config* format_config)
   {
                        dec->zscan_source_format = format_config->zscan_source_format;
   dec->zscan_linear = vl_zscan_layout(dec->context, vl_zscan_linear, dec->blocks_per_line);
   dec->zscan_normal = vl_zscan_layout(dec->context, vl_zscan_normal, dec->blocks_per_line);
                     if (!vl_zscan_init(&dec->zscan_y, dec->context, dec->base.width, dec->base.height,
                  if (!vl_zscan_init(&dec->zscan_c, dec->context, dec->chroma_width, dec->chroma_height,
                     }
      static bool
   init_idct(struct vl_mpeg12_decoder *dec, const struct format_config* format_config)
   {
      unsigned nr_of_idct_render_targets, max_inst;
   enum pipe_format formats[3];
                     nr_of_idct_render_targets = dec->context->screen->get_param
   (
         );
      max_inst = dec->context->screen->get_shader_param
   (
                  // Just assume we need 32 inst per render target, not 100% true, but should work in most cases
   if (nr_of_idct_render_targets >= 4 && max_inst >= 32*4)
      // more than 4 render targets usually doesn't makes any seens
      else
            formats[0] = formats[1] = formats[2] = format_config->idct_source_format;
   memset(&templat, 0, sizeof(templat));
   templat.width = dec->base.width / 4;
   templat.height = dec->base.height;
   dec->idct_source = vl_video_buffer_create_ex
   (
      dec->context, &templat,
   formats, 1, 1, PIPE_USAGE_DEFAULT,
               if (!dec->idct_source)
            formats[0] = formats[1] = formats[2] = format_config->mc_source_format;
   memset(&templat, 0, sizeof(templat));
   templat.width = dec->base.width / nr_of_idct_render_targets;
   templat.height = dec->base.height / 4;
   dec->mc_source = vl_video_buffer_create_ex
   (
      dec->context, &templat,
   formats, nr_of_idct_render_targets, 1, PIPE_USAGE_DEFAULT,
               if (!dec->mc_source)
            if (!(matrix = vl_idct_upload_matrix(dec->context, format_config->idct_scale)))
            if (!vl_idct_init(&dec->idct_y, dec->context, dec->base.width, dec->base.height,
                  if(!vl_idct_init(&dec->idct_c, dec->context, dec->chroma_width, dec->chroma_height,
                                 error_c:
            error_y:
            error_matrix:
            error_mc_source:
            error_idct_source:
         }
      static bool
   init_mc_source_widthout_idct(struct vl_mpeg12_decoder *dec, const struct format_config* format_config)
   {
      enum pipe_format formats[3];
            formats[0] = formats[1] = formats[2] = format_config->mc_source_format;
   assert(pipe_format_to_chroma_format(formats[0]) == dec->base.chroma_format);
   memset(&templat, 0, sizeof(templat));
   templat.width = dec->base.width;
   templat.height = dec->base.height;
   dec->mc_source = vl_video_buffer_create_ex
   (
      dec->context, &templat,
   formats, 1, 1, PIPE_USAGE_DEFAULT,
                  }
      static void
   mc_vert_shader_callback(void *priv, struct vl_mc *mc,
                     {
      struct vl_mpeg12_decoder *dec = priv;
            assert(priv && mc);
            if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      struct vl_idct *idct = mc == &dec->mc_y ? &dec->idct_y : &dec->idct_c;
      } else {
      o_vtex = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, first_output);
         }
      static void
   mc_frag_shader_callback(void *priv, struct vl_mc *mc,
                     {
      struct vl_mpeg12_decoder *dec = priv;
            assert(priv && mc);
            if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      struct vl_idct *idct = mc == &dec->mc_y ? &dec->idct_y : &dec->idct_c;
      } else {
      src = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, first_input, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 0);
         }
      struct pipe_video_codec *
   vl_create_mpeg12_decoder(struct pipe_context *context,
         {
      const unsigned block_size_pixels = VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT;
   const struct format_config *format_config;
                              if (!dec)
            dec->base = *templat;
   dec->base.context = context;
            dec->base.destroy = vl_mpeg12_destroy;
   dec->base.begin_frame = vl_mpeg12_begin_frame;
   dec->base.decode_macroblock = vl_mpeg12_decode_macroblock;
   dec->base.decode_bitstream = vl_mpeg12_decode_bitstream;
   dec->base.end_frame = vl_mpeg12_end_frame;
            dec->blocks_per_line = MAX2(util_next_power_of_two(dec->base.width) / block_size_pixels, 4);
   dec->num_blocks = (dec->base.width * dec->base.height) / block_size_pixels;
            /* TODO: Implement 422, 444 */
            if (dec->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      dec->chroma_width = dec->base.width / 2;
   dec->chroma_height = dec->base.height / 2;
      } else if (dec->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422) {
      dec->chroma_width = dec->base.width / 2;
   dec->chroma_height = dec->base.height;
      } else {
      dec->chroma_width = dec->base.width;
   dec->chroma_height = dec->base.height;
               dec->quads = vl_vb_upload_quads(dec->context);
   dec->pos = vl_vb_upload_pos(
      dec->context,
   dec->base.width / VL_MACROBLOCK_WIDTH,
               dec->ves_ycbcr = vl_vb_get_ves_ycbcr(dec->context);
            switch (templat->entrypoint) {
   case PIPE_VIDEO_ENTRYPOINT_BITSTREAM:
      format_config = find_format_config(dec, bitstream_format_config, num_bitstream_format_configs);
         case PIPE_VIDEO_ENTRYPOINT_IDCT:
      format_config = find_format_config(dec, idct_format_config, num_idct_format_configs);
         case PIPE_VIDEO_ENTRYPOINT_MC:
      format_config = find_format_config(dec, mc_format_config, num_mc_format_configs);
         default:
      assert(0);
   FREE(dec);
               if (!format_config) {
      FREE(dec);
               if (!init_zscan(dec, format_config))
            if (templat->entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      if (!init_idct(dec, format_config))
      } else {
      if (!init_mc_source_widthout_idct(dec, format_config))
               if (!vl_mc_init(&dec->mc_y, dec->context, dec->base.width, dec->base.height,
                        // TODO
   if (!vl_mc_init(&dec->mc_c, dec->context, dec->base.width, dec->base.height,
                        if (!init_pipe_state(dec))
                           error_pipe_state:
            error_mc_c:
            error_mc_y:
      if (templat->entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      vl_idct_cleanup(&dec->idct_y);
   vl_idct_cleanup(&dec->idct_c);
      }
         error_sources:
      vl_zscan_cleanup(&dec->zscan_y);
         error_zscan:
      FREE(dec);
      }
