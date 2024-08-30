   /**********************************************************
   * Copyright 2009-2011 VMware, Inc. All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   *********************************************************
   * Authors:
   * Zack Rusin <zackr-at-vmware-dot-com>
   * Thomas Hellstrom <thellstrom-at-vmware-dot-com>
   */
      #include "xa_composite.h"
   #include "xa_context.h"
   #include "xa_priv.h"
   #include "cso_cache/cso_context.h"
   #include "util/u_sampler.h"
   #include "util/u_inlines.h"
         /*XXX also in Xrender.h but the including it here breaks compilition */
   #define XFixedToDouble(f)    (((double) (f)) / 65536.)
      struct xa_composite_blend {
               unsigned alpha_dst : 4;
            unsigned rgb_src : 8;    /**< PIPE_BLENDFACTOR_x */
      };
      #define XA_BLEND_OP_OVER 3
   static const struct xa_composite_blend xa_blends[] = {
      { xa_op_clear,
         { xa_op_src,
         { xa_op_dst,
         { xa_op_over,
         { xa_op_over_reverse,
         { xa_op_in,
         { xa_op_in_reverse,
         { xa_op_out,
         { xa_op_out_reverse,
         { xa_op_atop,
         { xa_op_atop_reverse,
         { xa_op_xor,
         { xa_op_add,
      };
      /*
   * The alpha value stored in a L8 texture is read by the
   * hardware as color, and R8 is read as red. The source alpha value
   * at the end of the fragment shader is stored in all color channels,
   * so the correct approach is to blend using DST_COLOR instead of
   * DST_ALPHA and then output any color channel (L8) or the red channel (R8).
   */
   static unsigned
   xa_convert_blend_for_luminance(unsigned factor)
   {
      switch(factor) {
      return PIPE_BLENDFACTOR_DST_COLOR;
         return PIPE_BLENDFACTOR_INV_DST_COLOR;
         break;
      }
      }
      static bool
   blend_for_op(struct xa_composite_blend *blend,
         enum xa_composite_op op,
   struct xa_picture *src_pic,
   struct xa_picture *mask_pic,
   {
         sizeof(xa_blends)/sizeof(struct xa_composite_blend);
      int i;
            /*
   * our default in case something goes wrong
   */
               if (xa_blends[i].op == op) {
      *blend = xa_blends[i];
   supported = true;
      }
               /*
   * No component alpha yet.
   */
      return false;
            return supported;
         if ((dst_pic->srf->tex->format == PIPE_FORMAT_L8_UNORM ||
            blend->rgb_src = xa_convert_blend_for_luminance(blend->rgb_src);
               /*
   * If there's no dst alpha channel, adjust the blend op so that we'll treat
   * it as always 1.
               if (blend->rgb_src == PIPE_BLENDFACTOR_DST_ALPHA)
         else if (blend->rgb_src == PIPE_BLENDFACTOR_INV_DST_ALPHA)
      blend->rgb_src = PIPE_BLENDFACTOR_ZERO;
               }
         static inline int
   xa_repeat_to_gallium(int mode)
   {
      switch(mode) {
      return PIPE_TEX_WRAP_CLAMP_TO_BORDER;
         return PIPE_TEX_WRAP_REPEAT;
         return PIPE_TEX_WRAP_MIRROR_REPEAT;
         return PIPE_TEX_WRAP_CLAMP_TO_EDGE;
         break;
      }
      }
      static inline bool
   xa_filter_to_gallium(int xrender_filter, int *out_filter)
   {
         switch (xrender_filter) {
      *out_filter = PIPE_TEX_FILTER_NEAREST;
   break;
         *out_filter = PIPE_TEX_FILTER_LINEAR;
   break;
         *out_filter = PIPE_TEX_FILTER_NEAREST;
   return false;
      }
      }
      static int
   xa_is_filter_accelerated(struct xa_picture *pic)
   {
      int filter;
      return 0;
         }
      /**
   * xa_src_pict_is_accelerated - Check whether we support acceleration
   * of the given src_pict type
   *
   * \param src_pic[in]: Pointer to a union xa_source_pict to check.
   *
   * \returns TRUE if accelerated, FALSE otherwise.
   */
   static bool
   xa_src_pict_is_accelerated(const union xa_source_pict *src_pic)
   {
      if (!src_pic)
            if (src_pic->type == xa_src_pict_solid_fill ||
      src_pic->type == xa_src_pict_float_solid_fill)
            }
      XA_EXPORT int
   xa_composite_check_accelerated(const struct xa_composite *comp)
   {
      struct xa_picture *src_pic = comp->src;
   struct xa_picture *mask_pic = comp->mask;
               !xa_is_filter_accelerated(comp->mask)) {
   return -XA_ERR_INVAL;
               if (!xa_src_pict_is_accelerated(src_pic->src_pict) ||
      (mask_pic && !xa_src_pict_is_accelerated(mask_pic->src_pict)))
            return -XA_ERR_INVAL;
         /*
   * No component alpha yet.
   */
      return -XA_ERR_INVAL;
            }
      static int
   bind_composite_blend_state(struct xa_context *ctx,
         {
      struct xa_composite_blend blend_opt;
               return -XA_ERR_INVAL;
         memset(&blend, 0, sizeof(struct pipe_blend_state));
   blend.rt[0].blend_enable = 1;
            blend.rt[0].rgb_src_factor   = blend_opt.rgb_src;
   blend.rt[0].alpha_src_factor = blend_opt.rgb_src;
   blend.rt[0].rgb_dst_factor   = blend_opt.rgb_dst;
            cso_set_blend(ctx->cso, &blend);
      }
      static unsigned int
   picture_format_fixups(struct xa_picture *src_pic,
         {
      bool set_alpha = false;
   bool swizzle = false;
   unsigned ret = 0;
   struct xa_surface *src = src_pic->srf;
   enum xa_formats src_hw_format, src_pic_format;
               return 0;
         src_hw_format = xa_surface_format(src);
            set_alpha = (xa_format_type_is_color(src_hw_format) &&
               ret |= mask ? FS_MASK_SET_ALPHA : FS_SRC_SET_ALPHA;
            if (src->tex->format == PIPE_FORMAT_L8_UNORM ||
                  return ret;
               src_hw_type = xa_format_type(src_hw_format);
               src_pic_type == xa_type_abgr) ||
                     if (!swizzle && (src_hw_type != src_pic_type))
               ret |= mask ? FS_MASK_SWIZZLE_RGB : FS_SRC_SWIZZLE_RGB;
            }
      static void
   xa_src_in_mask(float src[4], const float mask[4])
   {
   src[0] *= mask[3];
   src[1] *= mask[3];
   src[2] *= mask[3];
   src[3] *= mask[3];
   }
      /**
   * xa_handle_src_pict - Set up xa_context state and fragment shader
   * input based on scr_pict type
   *
   * \param ctx[in, out]: Pointer to the xa context.
   * \param src_pict[in]: Pointer to the union xa_source_pict to consider.
   * \param is_mask[in]: Whether we're considering a mask picture.
   *
   * \returns TRUE if succesful, FALSE otherwise.
   *
   * This function computes some xa_context state used to determine whether
   * to upload the solid color and also the solid color itself used as an input
   * to the fragment shader.
   */
   static bool
   xa_handle_src_pict(struct xa_context *ctx,
               {
               switch(src_pict->type) {
   case xa_src_pict_solid_fill:
      xa_pixel_to_float4(src_pict->solid_fill.color, solid_color);
      case xa_src_pict_float_solid_fill:
      memcpy(solid_color, src_pict->float_solid_fill.color,
            default:
                  if (is_mask && ctx->has_solid_src)
         else
            if (is_mask)
         else
               }
      static int
   bind_shaders(struct xa_context *ctx, const struct xa_composite *comp)
   {
      unsigned vs_traits = 0, fs_traits = 0;
   struct xa_shader shader;
   struct xa_picture *src_pic = comp->src;
   struct xa_picture *mask_pic = comp->mask;
            ctx->has_solid_src = false;
            if (dst_pic && xa_format_type(dst_pic->pict_format) !=
      xa_format_type(xa_surface_format(dst_pic->srf)))
            if (src_pic->wrap == xa_wrap_clamp_to_border && src_pic->has_transform)
                  fs_traits |= FS_COMPOSITE;
      if (src_pic->src_pict) {
               if (!xa_handle_src_pict(ctx, src_pic->src_pict, false))
         } else
                        vs_traits |= VS_MASK;
   fs_traits |= FS_MASK;
         if (mask_pic->component_alpha)
         if (mask_pic->src_pict) {
                        if (ctx->has_solid_src) {
      vs_traits &= ~VS_MASK;
      } else {
      vs_traits |= VS_MASK_SRC;
      } else {
         if (mask_pic->wrap == xa_wrap_clamp_to_border &&
                              if (ctx->srf->format == PIPE_FORMAT_L8_UNORM ||
      fs_traits |= FS_DST_LUMINANCE;
         shader = xa_shaders_get(ctx->shaders, vs_traits, fs_traits);
   cso_set_vertex_shader_handle(ctx->cso, shader.vs);
   cso_set_fragment_shader_handle(ctx->cso, shader.fs);
      }
      static void
   bind_samplers(struct xa_context *ctx,
         {
      struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_state src_sampler, mask_sampler;
   struct pipe_sampler_view view_templ;
   struct pipe_sampler_view *src_view;
   struct pipe_context *pipe = ctx->pipe;
   struct xa_picture *src_pic = comp->src;
   struct xa_picture *mask_pic = comp->mask;
            xa_ctx_sampler_views_destroy(ctx);
   memset(&src_sampler, 0, sizeof(struct pipe_sampler_state));
               unsigned src_wrap = xa_repeat_to_gallium(src_pic->wrap);
   int filter;
      (void) xa_filter_to_gallium(src_pic->filter, &filter);
      src_sampler.wrap_s = src_wrap;
   src_sampler.wrap_t = src_wrap;
   src_sampler.min_img_filter = filter;
   src_sampler.mag_img_filter = filter;
   src_sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
   samplers[0] = &src_sampler;
   u_sampler_view_default_template(&view_templ,
         src_view = pipe->create_sampler_view(pipe, src_pic->srf->tex,
         ctx->bound_sampler_views[0] = src_view;
   num_samplers++;
               if (mask_pic && !ctx->has_solid_mask) {
      int filter;
      (void) xa_filter_to_gallium(mask_pic->filter, &filter);
      mask_sampler.wrap_s = mask_wrap;
   mask_sampler.wrap_t = mask_wrap;
   mask_sampler.min_img_filter = filter;
   mask_sampler.mag_img_filter = filter;
   src_sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
         u_sampler_view_default_template(&view_templ,
      mask_pic->srf->tex,
      src_view = pipe->create_sampler_view(pipe, mask_pic->srf->tex,
               ctx->bound_sampler_views[num_samplers] = src_view;
               cso_set_samplers(ctx->cso, PIPE_SHADER_FRAGMENT, num_samplers,
         pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, num_samplers, 0,
            }
      XA_EXPORT int
   xa_composite_prepare(struct xa_context *ctx,
         {
      struct xa_surface *dst_srf = comp->dst->srf;
            ret = xa_ctx_srf_create(ctx, dst_srf);
      return ret;
         ctx->dst = dst_srf;
            ret = bind_composite_blend_state(ctx, comp);
      return ret;
      ret = bind_shaders(ctx, comp);
      return ret;
                  renderer_begin_solid(ctx);
         renderer_begin_textures(ctx);
   ctx->comp = comp;
               xa_ctx_srf_destroy(ctx);
      }
      XA_EXPORT void
   xa_composite_rect(struct xa_context *ctx,
      int srcX, int srcY, int maskX, int maskY,
      {
         xa_scissor_update(ctx, dstX, dstY, dstX + width, dstY + height);
   renderer_solid(ctx, dstX, dstY, dstX + width, dstY + height);
         const struct xa_composite *comp = ctx->comp;
   int pos[6] = {srcX, srcY, maskX, maskY, dstX, dstY};
   const float *src_matrix = NULL;
   const float *mask_matrix = NULL;
      xa_scissor_update(ctx, dstX, dstY, dstX + width, dstY + height);
      if (comp->src->has_transform)
         if (comp->mask && comp->mask->has_transform)
            renderer_texture(ctx, pos, width, height,
      src_matrix, mask_matrix);
      }
      XA_EXPORT void
   xa_composite_done(struct xa_context *ctx)
   {
               ctx->comp = NULL;
   ctx->has_solid_src = false;
   ctx->has_solid_mask = false;
      }
      static const struct xa_composite_allocation a = {
      .xa_composite_size = sizeof(struct xa_composite),
   .xa_picture_size = sizeof(struct xa_picture),
      };
      XA_EXPORT const struct xa_composite_allocation *
   xa_composite_allocation(void)
   {
         }
