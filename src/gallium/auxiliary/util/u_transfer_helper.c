   /*
   * Copyright © 2017 Red Hat
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "pipe/p_screen.h"
      #include "util/u_box.h"
   #include "util/format/u_format.h"
   #include "util/format/u_format_zs.h"
   #include "util/u_inlines.h"
   #include "util/u_transfer_helper.h"
         struct u_transfer_helper {
      const struct u_transfer_vtbl *vtbl;
   bool separate_z32s8; /**< separate z32 and s8 */
   bool separate_stencil; /**< separate stencil for all formats */
   bool msaa_map;
   bool z24_in_z32f; /* the z24 values are stored in a z32 - translate them. */
      };
      /* If we need to take the path for PIPE_MAP_DEPTH/STENCIL_ONLY on the parent
   * depth/stencil resource an interleaving those to/from a staging buffer. The
   * other path for z/s interleave is when separate z and s resources are
   * created at resource create time.
   */
   static inline bool needs_in_place_zs_interleave(struct u_transfer_helper *helper,
         {
      if (!helper->interleave_in_place)
         if (helper->separate_stencil && util_format_is_depth_and_stencil(format))
         if (helper->separate_z32s8 && format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT)
         /* this isn't interleaving, but still needs conversions on that path. */
   if (helper->z24_in_z32f && format == PIPE_FORMAT_Z24X8_UNORM)
            }
      static inline bool handle_transfer(struct pipe_resource *prsc)
   {
               if (helper->vtbl->get_internal_format) {
      enum pipe_format internal_format =
         if (internal_format != prsc->format)
               if (helper->msaa_map && (prsc->nr_samples > 1))
            if (needs_in_place_zs_interleave(helper, prsc->format))
               }
      /* The pipe_transfer ptr could either be the driver's, or u_transfer,
   * depending on whether we are intervening or not.  Check handle_transfer()
   * before dereferencing.
   */
   struct u_transfer {
      struct pipe_transfer base;
   /* Note that in case of MSAA resolve for transfer plus z32s8
   * we end up with stacked u_transfer's.  The MSAA resolve case doesn't call
   * helper->vtbl fxns directly, but calls back to pctx->transfer_map()/etc
   * so the format related handling can work in conjunction with MSAA resolve.
   */
   struct pipe_transfer *trans;   /* driver's transfer */
   struct pipe_transfer *trans2;  /* 2nd transfer for s8 stencil buffer in z32s8 */
   void *ptr, *ptr2;              /* ptr to trans, and trans2 */
   void *staging;                 /* staging buffer */
      };
      static inline struct u_transfer *
   u_transfer(struct pipe_transfer *ptrans)
   {
      assert(handle_transfer(ptrans->resource));
      }
      struct pipe_resource *
   u_transfer_helper_resource_create(struct pipe_screen *pscreen,
         {
      struct u_transfer_helper *helper = pscreen->transfer_helper;
   enum pipe_format format = templ->format;
            if (((helper->separate_stencil && util_format_is_depth_and_stencil(format)) ||
      (format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT && helper->separate_z32s8)) &&
   !helper->interleave_in_place) {
   struct pipe_resource t = *templ;
                     if (t.format == PIPE_FORMAT_Z24X8_UNORM && helper->z24_in_z32f)
            prsc = helper->vtbl->resource_create(pscreen, &t);
   if (!prsc)
                     t.format = PIPE_FORMAT_S8_UINT;
            if (!stencil) {
      helper->vtbl->resource_destroy(pscreen, prsc);
                  } else if (format == PIPE_FORMAT_Z24X8_UNORM && helper->z24_in_z32f) {
      struct pipe_resource t = *templ;
            prsc = helper->vtbl->resource_create(pscreen, &t);
   if (!prsc)
               } else {
      /* normal case, no special handling: */
   prsc = helper->vtbl->resource_create(pscreen, templ);
   if (!prsc)
                  }
      void
   u_transfer_helper_resource_destroy(struct pipe_screen *pscreen,
         {
               if (helper->vtbl->get_stencil && !helper->interleave_in_place) {
                              }
      static bool needs_pack(unsigned usage)
   {
      return (usage & PIPE_MAP_READ) &&
      }
      /* In the case of transfer_map of a multi-sample resource, call back into
   * pctx->transfer_map() to map the staging resource, to handle cases of
   * MSAA + separate_z32s8
   */
   static void *
   transfer_map_msaa(struct pipe_context *pctx,
                     struct pipe_resource *prsc,
   {
      struct pipe_screen *pscreen = pctx->screen;
   struct u_transfer *trans = calloc(1, sizeof(*trans));
   if (!trans)
                  pipe_resource_reference(&ptrans->resource, prsc);
   ptrans->level = level;
   ptrans->usage = usage;
            struct pipe_resource tmpl = {
         .target = prsc->target,
   .format = prsc->format,
   .width0 = box->width,
   .height0 = box->height,
   .depth0 = 1,
   };
   if (util_format_is_depth_or_stencil(tmpl.format))
         else
         trans->ss = pscreen->resource_create(pscreen, &tmpl);
   if (!trans->ss) {
      free(trans);
               if (needs_pack(usage)) {
      struct pipe_blit_info blit;
            blit.src.resource = ptrans->resource;
   blit.src.format = ptrans->resource->format;
   blit.src.level = ptrans->level;
            blit.dst.resource = trans->ss;
   blit.dst.format = trans->ss->format;
   blit.dst.box.width = box->width;
   blit.dst.box.height = box->height;
            blit.mask = util_format_get_mask(prsc->format);
                        struct pipe_box map_box = *box;
   map_box.x = 0;
            void *ss_map = pctx->texture_map(pctx, trans->ss, 0, usage, &map_box,
         if (!ss_map) {
      free(trans);
               ptrans->stride = trans->trans->stride;
   *pptrans = ptrans;
      }
      void *
   u_transfer_helper_transfer_map(struct pipe_context *pctx,
                           {
      struct u_transfer_helper *helper = pctx->screen->transfer_helper;
   struct u_transfer *trans;
   struct pipe_transfer *ptrans;
   enum pipe_format format = prsc->format;
   unsigned width = box->width;
   unsigned height = box->height;
            if (!handle_transfer(prsc))
            if (helper->msaa_map && (prsc->nr_samples > 1))
                     trans = calloc(1, sizeof(*trans));
   if (!trans)
            ptrans = &trans->base;
   pipe_resource_reference(&ptrans->resource, prsc);
   ptrans->level = level;
   ptrans->usage = usage;
   ptrans->box   = *box;
   ptrans->stride = util_format_get_stride(format, box->width);
            trans->staging = malloc(ptrans->layer_stride);
   if (!trans->staging)
            trans->ptr = helper->vtbl->transfer_map(pctx, prsc, level,
               if (!trans->ptr)
            if (util_format_is_depth_and_stencil(prsc->format)) {
               if (in_place_zs_interleave)
      else
            trans->ptr2 = helper->vtbl->transfer_map(pctx, stencil, level,
                  if (needs_pack(usage)) {
      switch (prsc->format) {
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      util_format_z32_float_s8x24_uint_pack_z_float(trans->staging,
                           util_format_z32_float_s8x24_uint_pack_s_8uint(trans->staging,
                              case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (in_place_zs_interleave) {
      if (helper->z24_in_z32f) {
      util_format_z24_unorm_s8_uint_pack_separate_z32(trans->staging,
                                    } else {
      util_format_z24_unorm_s8_uint_pack_separate(trans->staging,
                                       } else {
      if (helper->z24_in_z32f) {
      util_format_z24_unorm_s8_uint_pack_z_float(trans->staging,
                           util_format_z24_unorm_s8_uint_pack_s_8uint(trans->staging,
                        } else {
      util_format_z24_unorm_s8_uint_pack_separate(trans->staging,
                                       }
      case PIPE_FORMAT_Z24X8_UNORM:
      assert(helper->z24_in_z32f);
   util_format_z24x8_unorm_pack_z_float(trans->staging, ptrans->stride,
                  default:
               } else if (prsc->format == PIPE_FORMAT_Z24X8_UNORM) {
         assert(helper->z24_in_z32f);
   util_format_z24x8_unorm_pack_z_float(trans->staging, ptrans->stride,
         } else {
                  *pptrans = ptrans;
         fail:
      if (trans->trans)
         if (trans->trans2)
         pipe_resource_reference(&ptrans->resource, NULL);
   free(trans->staging);
   free(trans);
      }
      static void
   flush_region(struct pipe_context *pctx, struct pipe_transfer *ptrans,
         {
      struct u_transfer_helper *helper = pctx->screen->transfer_helper;
   /* using the function here hits an assert for the deinterleave cases */
   struct u_transfer *trans = (struct u_transfer *)ptrans;
   enum pipe_format iformat, format = ptrans->resource->format;
   unsigned width = box->width;
   unsigned height = box->height;
            if (!(ptrans->usage & PIPE_MAP_WRITE))
            if (trans->ss) {
      struct pipe_blit_info blit;
            blit.src.resource = trans->ss;
   blit.src.format = trans->ss->format;
            blit.dst.resource = ptrans->resource;
   blit.dst.format = ptrans->resource->format;
            u_box_2d(ptrans->box.x + box->x,
                        blit.mask = util_format_get_mask(ptrans->resource->format);
                                          src = (uint8_t *)trans->staging +
         (box->y * ptrans->stride) +
   dst = (uint8_t *)trans->ptr +
                  switch (format) {
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      util_format_z32_float_s8x24_uint_unpack_z_float(dst,
                              case PIPE_FORMAT_X32_S8X24_UINT:
      dst = (uint8_t *)trans->ptr2 +
                  util_format_z32_float_s8x24_uint_unpack_s_8uint(dst,
                              case PIPE_FORMAT_Z24X8_UNORM:
      util_format_z24x8_unorm_unpack_z_float(dst, trans->trans->stride,
                  case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (helper->z24_in_z32f) {
      util_format_z24_unorm_s8_uint_unpack_z_float(dst, trans->trans->stride,
            } else {
      /* just do a strided 32-bit copy for depth; s8 can become garbage x8 */
   util_format_z32_unorm_unpack_z_32unorm(dst, trans->trans->stride,
            }
      case PIPE_FORMAT_X24S8_UINT:
      dst = (uint8_t *)trans->ptr2 +
                  util_format_z24_unorm_s8_uint_unpack_s_8uint(dst, trans->trans2->stride,
                     default:
      assert(!"Unexpected staging transfer type");
         }
      void
   u_transfer_helper_transfer_flush_region(struct pipe_context *pctx,
               {
               if (handle_transfer(ptrans->resource)) {
               /* handle MSAA case, since there could be multiple levels of
   * wrapped transfer, call pctx->transfer_flush_region()
   * instead of helper->vtbl->transfer_flush_region()
   */
   if (trans->ss) {
      pctx->transfer_flush_region(pctx, trans->trans, box);
   flush_region(pctx, ptrans, box);
                        helper->vtbl->transfer_flush_region(pctx, trans->trans, box);
   if (trans->trans2)
         } else {
            }
      void
   u_transfer_helper_transfer_unmap(struct pipe_context *pctx,
         {
               if (handle_transfer(ptrans->resource)) {
               if (!(ptrans->usage & PIPE_MAP_FLUSH_EXPLICIT)) {
      struct pipe_box box;
   u_box_2d(0, 0, ptrans->box.width, ptrans->box.height, &box);
   if (trans->ss)
                     /* in MSAA case, there could be multiple levels of wrapping
   * so don't call helper->vtbl->transfer_unmap() directly
   */
   if (trans->ss) {
      pctx->texture_unmap(pctx, trans->trans);
      } else {
      helper->vtbl->transfer_unmap(pctx, trans->trans);
   if (trans->trans2)
                        free(trans->staging);
      } else {
            }
      struct u_transfer_helper *
   u_transfer_helper_create(const struct u_transfer_vtbl *vtbl,
         {
               helper->vtbl = vtbl;
   helper->separate_z32s8 = flags & U_TRANSFER_HELPER_SEPARATE_Z32S8;
   helper->separate_stencil = flags & U_TRANSFER_HELPER_SEPARATE_STENCIL;
   helper->msaa_map = flags & U_TRANSFER_HELPER_MSAA_MAP;
   helper->z24_in_z32f = flags & U_TRANSFER_HELPER_Z24_IN_Z32F;
               }
      void
   u_transfer_helper_destroy(struct u_transfer_helper *helper)
   {
         }
