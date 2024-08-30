   /*
   * Copyright 2009 Marek Olšák <maraeo@gmail.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "r300_context.h"
   #include "r300_emit.h"
   #include "r300_texture.h"
   #include "r300_reg.h"
      #include "util/format/u_format.h"
   #include "util/half_float.h"
   #include "util/u_pack_color.h"
   #include "util/u_surface.h"
      enum r300_blitter_op /* bitmask */
   {
      R300_STOP_QUERY         = 1,
   R300_SAVE_TEXTURES      = 2,
   R300_SAVE_FRAMEBUFFER   = 4,
                              R300_COPY          = R300_STOP_QUERY | R300_SAVE_FRAMEBUFFER |
            R300_BLIT          = R300_STOP_QUERY | R300_SAVE_FRAMEBUFFER |
               };
      static void r300_blitter_begin(struct r300_context* r300, enum r300_blitter_op op)
   {
      if ((op & R300_STOP_QUERY) && r300->query_current) {
      r300->blitter_saved_query = r300->query_current;
               /* Yeah we have to save all those states to ensure the blitter operation
   * is really transparent. The states will be restored by the blitter once
   * copying is done. */
   util_blitter_save_blend(r300->blitter, r300->blend_state.state);
   util_blitter_save_depth_stencil_alpha(r300->blitter, r300->dsa_state.state);
   util_blitter_save_stencil_ref(r300->blitter, &(r300->stencil_ref));
   util_blitter_save_rasterizer(r300->blitter, r300->rs_state.state);
   util_blitter_save_fragment_shader(r300->blitter, r300->fs.state);
   util_blitter_save_vertex_shader(r300->blitter, r300->vs_state.state);
   util_blitter_save_viewport(r300->blitter, &r300->viewport);
   util_blitter_save_scissor(r300->blitter, r300->scissor_state.state);
   util_blitter_save_sample_mask(r300->blitter, *(unsigned*)r300->sample_mask.state, 0);
   util_blitter_save_vertex_buffer_slot(r300->blitter, r300->vertex_buffer);
            struct pipe_constant_buffer cb = {
      /* r300 doesn't use the size for FS at all. The shader determines it.
   * Set something for blitter.
   */
   .buffer_size = 4,
      };
            if (op & R300_SAVE_FRAMEBUFFER) {
                  if (op & R300_SAVE_TEXTURES) {
      struct r300_textures_state* state =
            util_blitter_save_fragment_sampler_states(
                  util_blitter_save_fragment_sampler_views(
                     if (op & R300_IGNORE_RENDER_COND) {
      /* Save the flag. */
   r300->blitter_saved_skip_rendering = r300->skip_rendering+1;
      } else {
            }
      static void r300_blitter_end(struct r300_context *r300)
   {
      if (r300->blitter_saved_query) {
      r300_resume_query(r300, r300->blitter_saved_query);
               if (r300->blitter_saved_skip_rendering) {
      /* Restore the flag. */
         }
      static uint32_t r300_depth_clear_cb_value(enum pipe_format format,
         {
      union util_color uc;
            if (util_format_get_blocksizebits(format) == 32)
         else
      }
      static bool r300_cbzb_clear_allowed(struct r300_context *r300,
         {
      struct pipe_framebuffer_state *fb =
            /* Only color clear allowed, and only one colorbuffer. */
   if ((clear_buffers & ~PIPE_CLEAR_COLOR) != 0 || fb->nr_cbufs != 1 || !fb->cbufs[0])
               }
      static bool r300_fast_zclear_allowed(struct r300_context *r300,
         {
      struct pipe_framebuffer_state *fb =
               }
      static bool r300_hiz_clear_allowed(struct r300_context *r300)
   {
      struct pipe_framebuffer_state *fb =
               }
      static uint32_t r300_depth_clear_value(enum pipe_format format,
         {
      switch (format) {
      case PIPE_FORMAT_Z16_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
            case PIPE_FORMAT_S8_UINT_Z24_UNORM:
            default:
               }
      static uint32_t r300_hiz_clear_value(double depth)
   {
      uint32_t r = (uint32_t)(CLAMP(depth, 0, 1) * 255.5);
   assert(r <= 255);
      }
      static void r300_set_clear_color(struct r300_context *r300,
         {
      struct pipe_framebuffer_state *fb =
                  memset(&uc, 0, sizeof(uc));
            if (fb->cbufs[0]->format == PIPE_FORMAT_R16G16B16A16_FLOAT ||
      fb->cbufs[0]->format == PIPE_FORMAT_R16G16B16X16_FLOAT) {
   /* (0,1,2,3) maps to (B,G,R,A) */
   r300->color_clear_value_gb = uc.h[0] | ((uint32_t)uc.h[1] << 16);
      } else {
            }
      DEBUG_GET_ONCE_BOOL_OPTION(hyperz, "RADEON_HYPERZ", false)
      /* Clear currently bound buffers. */
   static void r300_clear(struct pipe_context* pipe,
                        unsigned buffers,
      {
      /* My notes about Zbuffer compression:
   *
   * 1) The zbuffer must be micro-tiled and whole microtiles must be
   *    written if compression is enabled. If microtiling is disabled,
   *    it locks up.
   *
   * 2) There is ZMASK RAM which contains a compressed zbuffer.
   *    Each dword of the Z Mask contains compression information
   *    for 16 4x4 pixel tiles, that is 2 bits for each tile.
   *    On chips with 2 Z pipes, every other dword maps to a different
   *    pipe. On newer chipsets, there is a new compression mode
   *    with 8x8 pixel tiles per 2 bits.
   *
   * 3) The FASTFILL bit has nothing to do with filling. It only tells hw
   *    it should look in the ZMASK RAM first before fetching from a real
   *    zbuffer.
   *
   * 4) If a pixel is in a cleared state, ZB_DEPTHCLEARVALUE is returned
   *    during zbuffer reads instead of the value that is actually stored
   *    in the zbuffer memory. A pixel is in a cleared state when its ZMASK
   *    is equal to 0. Therefore, if you clear ZMASK with zeros, you may
   *    leave the zbuffer memory uninitialized, but then you must enable
   *    compression, so that the ZMASK RAM is actually used.
   *
   * 5) Each 4x4 (or 8x8) tile is automatically decompressed and recompressed
   *    during zbuffer updates. A special decompressing operation should be
   *    used to fully decompress a zbuffer, which basically just stores all
   *    compressed tiles in ZMASK to the zbuffer memory.
   *
   * 6) For a 16-bit zbuffer, compression causes a hung with one or
   *    two samples and should not be used.
   *
   * 7) FORCE_COMPRESSED_STENCIL_VALUE should be enabled for stencil clears
   *    to avoid needless decompression.
   *
   * 8) Fastfill must not be used if reading of compressed Z data is disabled
   *    and writing of compressed Z data is enabled (RD/WR_COMP_ENABLE),
   *    i.e. it cannot be used to compress the zbuffer.
   *
   * 9) ZB_CB_CLEAR does not interact with zbuffer compression in any way.
   *
   * - Marek
            struct r300_context* r300 = r300_context(pipe);
   struct pipe_framebuffer_state *fb =
         struct r300_hyperz_state *hyperz =
         uint32_t width = fb->width;
   uint32_t height = fb->height;
            /* Use fast Z clear.
   * The zbuffer must be in micro-tiled mode, otherwise it locks up. */
   if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
               /* If both depth and stencil are present, they must be cleared together. */
   if (fb->zsbuf->texture->format == PIPE_FORMAT_S8_UINT_Z24_UNORM &&
         (buffers & PIPE_CLEAR_DEPTHSTENCIL) != PIPE_CLEAR_DEPTHSTENCIL) {
   zmask_clear = false;
   } else {
         zmask_clear = r300_fast_zclear_allowed(r300, buffers);
            /* If we need Hyper-Z. */
   if (zmask_clear || hiz_clear) {
         /* Try to obtain the access to Hyper-Z buffers if we don't have one. */
   if (!r300->hyperz_enabled &&
      (r300->screen->caps.is_r500 || debug_get_option_hyperz())) {
   r300->hyperz_enabled =
      r300->rws->cs_request_feature(&r300->cs,
            if (r300->hyperz_enabled) {
      /* Need to emit HyperZ buffer regs for the first time. */
                  /* Setup Hyper-Z clears. */
   if (r300->hyperz_enabled) {
                              r300_mark_atom_dirty(r300, &r300->zmask_clear);
                     if (hiz_clear) {
      r300->hiz_clear_value = r300_hiz_clear_value(depth);
   r300_mark_atom_dirty(r300, &r300->hiz_clear);
      }
                  /* Use fast color clear for an AA colorbuffer.
   * The CMASK is shared between all colorbuffers, so we use it
   * if there is only one colorbuffer bound. */
   if ((buffers & PIPE_CLEAR_COLOR) && fb->nr_cbufs == 1 && fb->cbufs[0] &&
      r300_resource(fb->cbufs[0]->texture)->tex.cmask_dwords) {
   /* Try to obtain the access to the CMASK if we don't have one. */
   if (!r300->cmask_access) {
         r300->cmask_access =
      r300->rws->cs_request_feature(&r300->cs,
               /* Setup the clear. */
   if (r300->cmask_access) {
         /* Pair the resource with the CMASK to avoid other resources
   * accessing it. */
   if (!r300->screen->cmask_resource) {
      mtx_lock(&r300->screen->cmask_mutex);
   /* Double checking (first unlocked, then locked). */
   if (!r300->screen->cmask_resource) {
      /* Don't reference this, so that the texture can be
      * destroyed while set in cmask_resource.
                           if (r300->screen->cmask_resource == fb->cbufs[0]->texture) {
      r300_set_clear_color(r300, color);
   r300_mark_atom_dirty(r300, &r300->cmask_clear);
   r300_mark_atom_dirty(r300, &r300->gpu_flush);
         }
   /* Enable CBZB clear. */
   else if (r300_cbzb_clear_allowed(r300, buffers)) {
               hyperz->zb_depthclearvalue =
            width = surf->cbzb_width;
            r300->cbzb_clear = true;
               /* Clear. */
   if (buffers) {
      /* Clear using the blitter. */
   r300_blitter_begin(r300, R300_CLEAR);
   util_blitter_clear(r300->blitter, width, height, 1,
                  } else if (r300->zmask_clear.dirty ||
                  /* Just clear zmask and hiz now, this does not use the standard draw
         /* Calculate zmask_clear and hiz_clear atom sizes. */
   unsigned dwords =
         r300->gpu_flush.size +
   (r300->zmask_clear.dirty ? r300->zmask_clear.size : 0) +
   (r300->hiz_clear.dirty ? r300->hiz_clear.size : 0) +
            /* Reserve CS space. */
   if (!r300->rws->cs_check_space(&r300->cs, dwords)) {
                  /* Emit clear packets. */
   r300_emit_gpu_flush(r300, r300->gpu_flush.size, r300->gpu_flush.state);
            if (r300->zmask_clear.dirty) {
         r300_emit_zmask_clear(r300, r300->zmask_clear.size,
         }
   if (r300->hiz_clear.dirty) {
         r300_emit_hiz_clear(r300, r300->hiz_clear.size,
         }
   if (r300->cmask_clear.dirty) {
         r300_emit_cmask_clear(r300, r300->cmask_clear.size,
            } else {
                  /* Disable CBZB clear. */
   if (r300->cbzb_clear) {
      r300->cbzb_clear = false;
   hyperz->zb_depthclearvalue = hyperz_dcv;
               /* Enable fastfill and/or hiz.
   *
   * If we cleared zmask/hiz, it's in use now. The Hyper-Z state update
   * looks if zmask/hiz is in use and programs hardware accordingly. */
   if (r300->zmask_in_use || r300->hiz_in_use) {
            }
      /* Clear a region of a color surface to a constant value. */
   static void r300_clear_render_target(struct pipe_context *pipe,
                                 {
               r300_blitter_begin(r300, R300_CLEAR_SURFACE |
         util_blitter_clear_render_target(r300->blitter, dst, color,
            }
      /* Clear a region of a depth stencil surface. */
   static void r300_clear_depth_stencil(struct pipe_context *pipe,
                                       struct pipe_surface *dst,
   {
      struct r300_context *r300 = r300_context(pipe);
   struct pipe_framebuffer_state *fb =
            if (r300->zmask_in_use && !r300->locked_zbuffer) {
      if (fb->zsbuf->texture == dst->texture) {
                     /* XXX Do not decompress ZMask of the currently-set zbuffer. */
   r300_blitter_begin(r300, R300_CLEAR_SURFACE |
         util_blitter_clear_depth_stencil(r300->blitter, dst, clear_flags, depth, stencil,
            }
      void r300_decompress_zmask(struct r300_context *r300)
   {
      struct pipe_framebuffer_state *fb =
            if (!r300->zmask_in_use || r300->locked_zbuffer)
            r300->zmask_decompress = true;
            r300_blitter_begin(r300, R300_DECOMPRESS);
   util_blitter_custom_clear_depth(r300->blitter, fb->width, fb->height, 0,
                  r300->zmask_decompress = false;
   r300->zmask_in_use = false;
      }
      void r300_decompress_zmask_locked_unsafe(struct r300_context *r300)
   {
               memset(&fb, 0, sizeof(fb));
   fb.width = r300->locked_zbuffer->width;
   fb.height = r300->locked_zbuffer->height;
            r300->context.set_framebuffer_state(&r300->context, &fb);
      }
      void r300_decompress_zmask_locked(struct r300_context *r300)
   {
               memset(&saved_fb, 0, sizeof(saved_fb));
   util_copy_framebuffer_state(&saved_fb, r300->fb_state.state);
   r300_decompress_zmask_locked_unsafe(r300);
   r300->context.set_framebuffer_state(&r300->context, &saved_fb);
               }
      bool r300_is_blit_supported(enum pipe_format format)
   {
      const struct util_format_description *desc =
            return desc->layout == UTIL_FORMAT_LAYOUT_PLAIN ||
            }
      /* Copy a block of pixels from one surface to another. */
   static void r300_resource_copy_region(struct pipe_context *pipe,
                                       {
      struct pipe_screen *screen = pipe->screen;
   struct r300_context *r300 = r300_context(pipe);
   struct pipe_framebuffer_state *fb =
         unsigned src_width0 = r300_resource(src)->tex.width0;
   unsigned src_height0 = r300_resource(src)->tex.height0;
   unsigned dst_width0 = r300_resource(dst)->tex.width0;
   unsigned dst_height0 = r300_resource(dst)->tex.height0;
   unsigned layout;
   struct pipe_box box, dstbox;
   struct pipe_sampler_view src_templ, *src_view;
            /* Fallback for buffers. */
   if ((dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) ||
      !r300_is_blit_supported(dst->format)) {
   util_resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
                     /* Can't read MSAA textures. */
   if (src->nr_samples > 1 || dst->nr_samples > 1) {
                  /* The code below changes the texture format so that the copy can be done
   * on hardware. E.g. depth-stencil surfaces are copied as RGBA
            util_blitter_default_dst_texture(&dst_templ, dst, dst_level, dstz);
                     /* Handle non-renderable plain formats. */
   if (layout == UTIL_FORMAT_LAYOUT_PLAIN &&
      (!screen->is_format_supported(screen, src_templ.format, src->target,
                  !screen->is_format_supported(screen, dst_templ.format, dst->target,
            switch (util_format_get_blocksize(dst_templ.format)) {
         case 1:
      dst_templ.format = PIPE_FORMAT_I8_UNORM;
      case 2:
      dst_templ.format = PIPE_FORMAT_B4G4R4A4_UNORM;
      case 4:
      dst_templ.format = PIPE_FORMAT_B8G8R8A8_UNORM;
      case 8:
      dst_templ.format = PIPE_FORMAT_R16G16B16A16_UNORM;
      default:
      debug_printf("r300: copy_region: Unhandled format: %s. Falling back to software.\n"
      }
               /* Handle compressed formats. */
   if (layout == UTIL_FORMAT_LAYOUT_S3TC ||
      layout == UTIL_FORMAT_LAYOUT_RGTC) {
            box = *src_box;
            dst_width0 = align(dst_width0, 4);
   dst_height0 = align(dst_height0, 4);
   src_width0 = align(src_width0, 4);
   src_height0 = align(src_height0, 4);
   box.width = align(box.width, 4);
            switch (util_format_get_blocksize(dst_templ.format)) {
   case 8:
         /* one 4x4 pixel block has 8 bytes.
   * we set 1 pixel = 4 bytes ===> 1 block corresponds to 2 pixels. */
   dst_templ.format = PIPE_FORMAT_R8G8B8A8_UNORM;
   dst_width0 = dst_width0 / 2;
   src_width0 = src_width0 / 2;
   dstx /= 2;
   box.x /= 2;
   box.width /= 2;
   case 16:
         /* one 4x4 pixel block has 16 bytes.
   * we set 1 pixel = 4 bytes ===> 1 block corresponds to 4 pixels. */
   dst_templ.format = PIPE_FORMAT_R8G8B8A8_UNORM;
   }
            dst_height0 = dst_height0 / 4;
   src_height0 = src_height0 / 4;
   dsty /= 4;
   box.y /= 4;
               /* Fallback for textures. */
   if (!screen->is_format_supported(screen, dst_templ.format,
                  !screen->is_format_supported(screen, src_templ.format,
                           assert(0 && "this shouldn't happen, update r300_is_blit_supported");
   util_resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
                     /* Decompress ZMASK. */
   if (r300->zmask_in_use && !r300->locked_zbuffer) {
      if (fb->zsbuf->texture == src ||
         fb->zsbuf->texture == dst) {
               dst_view = r300_create_surface_custom(pipe, dst, &dst_templ, dst_width0, dst_height0);
            u_box_3d(dstx, dsty, dstz, abs(src_box->width), abs(src_box->height),
            r300_blitter_begin(r300, R300_COPY);
   util_blitter_blit_generic(r300->blitter, dst_view, &dstbox,
                              pipe_surface_reference(&dst_view, NULL);
      }
      static bool r300_is_simple_msaa_resolve(const struct pipe_blit_info *info)
   {
      unsigned dst_width = u_minify(info->dst.resource->width0, info->dst.level);
            return info->src.resource->nr_samples > 1 &&
         info->dst.resource->nr_samples <= 1 &&
   info->dst.resource->format == info->src.resource->format &&
   info->dst.resource->format == info->dst.format &&
   info->src.resource->format == info->src.format &&
   !info->scissor_enable &&
   info->mask == PIPE_MASK_RGBA &&
   dst_width == info->src.resource->width0 &&
   dst_height == info->src.resource->height0 &&
   info->dst.box.x == 0 &&
   info->dst.box.y == 0 &&
   info->dst.box.width == dst_width &&
   info->dst.box.height == dst_height &&
   info->src.box.x == 0 &&
   info->src.box.y == 0 &&
   info->src.box.width == dst_width &&
   info->src.box.height == dst_height &&
      }
      static void r300_simple_msaa_resolve(struct pipe_context *pipe,
                                 {
      struct r300_context *r300 = r300_context(pipe);
   struct r300_surface *srcsurf, *dstsurf;
   struct pipe_surface surf_tmpl;
            memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   surf_tmpl.format = format;
            surf_tmpl.format = format;
   surf_tmpl.u.tex.level = dst_level;
   surf_tmpl.u.tex.first_layer =
   surf_tmpl.u.tex.last_layer = dst_layer;
            /* COLORPITCH should contain the tiling info of the resolve buffer.
   * The tiling of the AA buffer isn't programmable anyway. */
   srcsurf->pitch &= ~(R300_COLOR_TILE(1) | R300_COLOR_MICROTILE(3));
            /* Enable AA resolve. */
   aa->dest = dstsurf;
   r300->aa_state.size = 8;
            /* Resolve the surface. */
   r300_blitter_begin(r300, R300_CLEAR_SURFACE);
   util_blitter_custom_color(r300->blitter, &srcsurf->base, NULL);
            /* Disable AA resolve. */
   aa->dest = NULL;
   r300->aa_state.size = 4;
            pipe_surface_reference((struct pipe_surface**)&srcsurf, NULL);
      }
      static void r300_msaa_resolve(struct pipe_context *pipe,
         {
      struct r300_context *r300 = r300_context(pipe);
   struct pipe_screen *screen = pipe->screen;
   struct pipe_resource *tmp, templ;
            assert(info->src.level == 0);
   assert(info->src.box.z == 0);
   assert(info->src.box.depth == 1);
            if (r300_is_simple_msaa_resolve(info)) {
      r300_simple_msaa_resolve(pipe, info->dst.resource, info->dst.level,
                           /* resolve into a temporary texture, then blit */
   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = info->src.resource->format;
   templ.width0 = info->src.resource->width0;
   templ.height0 = info->src.resource->height0;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.usage = PIPE_USAGE_DEFAULT;
                     /* resolve */
   r300_simple_msaa_resolve(pipe, tmp, 0, 0, info->src.resource,
            /* blit */
   blit = *info;
   blit.src.resource = tmp;
            r300_blitter_begin(r300, R300_BLIT | R300_IGNORE_RENDER_COND);
   util_blitter_blit(r300->blitter, &blit);
               }
      static void r300_blit(struct pipe_context *pipe,
         {
      struct r300_context *r300 = r300_context(pipe);
   struct pipe_framebuffer_state *fb =
                  /* The driver supports sRGB textures but not framebuffers. Blitting
   * from sRGB to sRGB should be the same as blitting from linear
   * to linear, so use that, This avoids incorrect linearization.
   */
   if (util_format_is_srgb(info.src.format)) {
      info.src.format = util_format_linear(info.src.format);
               /* MSAA resolve. */
   if (info.src.resource->nr_samples > 1 &&
      !util_format_is_depth_or_stencil(info.src.resource->format)) {
   r300_msaa_resolve(pipe, &info);
               /* Can't read MSAA textures. */
   if (info.src.resource->nr_samples > 1) {
                  /* Blit a combined depth-stencil resource as color.
   * S8Z24 is the only supported stencil format. */
   if ((info.mask & PIPE_MASK_S) &&
      info.src.format == PIPE_FORMAT_S8_UINT_Z24_UNORM &&
   info.dst.format == PIPE_FORMAT_S8_UINT_Z24_UNORM) {
   if (info.dst.resource->nr_samples > 1) {
         /* Cannot do that with MSAA buffers. */
   info.mask &= ~PIPE_MASK_S;
   if (!(info.mask & PIPE_MASK_Z)) {
         } else {
         /* Single-sample buffer. */
   info.src.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   info.dst.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   if (info.mask & PIPE_MASK_Z) {
         } else {
                     /* Decompress ZMASK. */
   if (r300->zmask_in_use && !r300->locked_zbuffer) {
      if (fb->zsbuf->texture == info.src.resource ||
         fb->zsbuf->texture == info.dst.resource) {
               r300_blitter_begin(r300, R300_BLIT |
         util_blitter_blit(r300->blitter, &info);
      }
      static void r300_flush_resource(struct pipe_context *ctx,
         {
   }
      void r300_init_blit_functions(struct r300_context *r300)
   {
      r300->context.clear = r300_clear;
   r300->context.clear_render_target = r300_clear_render_target;
   r300->context.clear_depth_stencil = r300_clear_depth_stencil;
   r300->context.resource_copy_region = r300_resource_copy_region;
   r300->context.blit = r300_blit;
      }
