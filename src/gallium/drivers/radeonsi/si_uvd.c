   /**************************************************************************
   *
   * Copyright 2011 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   *
   **************************************************************************/
      #include "drm-uapi/drm_fourcc.h"
   #include "radeon_uvd.h"
   #include "radeon_uvd_enc.h"
   #include "radeon_vce.h"
   #include "radeon_vcn_dec.h"
   #include "radeon_vcn_enc.h"
   #include "radeon_video.h"
   #include "si_pipe.h"
   #include "util/u_video.h"
      /**
   * creates an video buffer with an UVD compatible memory layout
   */
   struct pipe_video_buffer *si_video_buffer_create(struct pipe_context *pipe,
         {
      struct pipe_video_buffer vidbuf = *tmpl;
   uint64_t *modifiers = NULL;
   int modifiers_count = 0;
            /* To get tiled buffers, users need to explicitly provide a list of
   * modifiers. */
            if (pipe->screen->resource_create_with_modifiers) {
      modifiers = &mod;
               return vl_video_buffer_create_as_resource(pipe, &vidbuf, modifiers,
      }
      struct pipe_video_buffer *si_video_buffer_create_with_modifiers(struct pipe_context *pipe,
                     {
      uint64_t *allowed_modifiers;
            /* Filter out DCC modifiers, because we don't support them for video
   * for now. */
   allowed_modifiers = calloc(modifiers_count, sizeof(uint64_t));
   if (!allowed_modifiers)
            allowed_modifiers_count = 0;
   for (i = 0; i < modifiers_count; i++) {
      if (ac_modifier_has_dcc(modifiers[i]))
                     struct pipe_video_buffer *buf =
         free(allowed_modifiers);
      }
      /* set the decoding target buffer offsets */
   static struct pb_buffer *si_uvd_set_dtb(struct ruvd_msg *msg, struct vl_video_buffer *buf)
   {
      struct si_screen *sscreen = (struct si_screen *)buf->base.context->screen;
   struct si_texture *luma = (struct si_texture *)buf->resources[0];
   struct si_texture *chroma = (struct si_texture *)buf->resources[1];
   enum ruvd_surface_type type =
                                 }
      /* get the radeon resources for VCE */
   static void si_vce_get_buffer(struct pipe_resource *resource, struct pb_buffer **handle,
         {
               if (handle)
            if (surface)
      }
      /**
   * creates an UVD compatible decoder
   */
   struct pipe_video_codec *si_uvd_create_decoder(struct pipe_context *context,
         {
      struct si_context *ctx = (struct si_context *)context;
            if (templ->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
      if (vcn) {
         } else {
      if (u_reduce_video_profile(templ->profile) == PIPE_VIDEO_FORMAT_HEVC)
         else
                  if (ctx->vcn_ip_ver == VCN_4_0_0)
            return (vcn) ? radeon_create_decoder(context, templ)
      }
