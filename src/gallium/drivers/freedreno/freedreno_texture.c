   /*
   * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "pipe/p_state.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "freedreno_context.h"
   #include "freedreno_resource.h"
   #include "freedreno_texture.h"
   #include "freedreno_util.h"
      static void
   fd_sampler_state_delete(struct pipe_context *pctx, void *hwcso)
   {
         }
      static void
   fd_sampler_view_destroy(struct pipe_context *pctx,
         {
      pipe_resource_reference(&view->texture, NULL);
      }
      void
   fd_sampler_states_bind(struct pipe_context *pctx, enum pipe_shader_type shader,
         {
      struct fd_context *ctx = fd_context(pctx);
            for (unsigned i = 0; i < nr; i++) {
      unsigned p = i + start;
   tex->samplers[p] = hwcso ? hwcso[i] : NULL;
   if (tex->samplers[p])
         else
                           }
      void
   fd_set_sampler_views(struct pipe_context *pctx, enum pipe_shader_type shader,
                           {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_texture_stateobj *tex = &ctx->tex[shader];
            for (i = 0; i < nr; i++) {
      struct pipe_sampler_view *view = views ? views[i] : NULL;
            if (take_ownership) {
      pipe_sampler_view_reference(&tex->textures[p], NULL);
      } else {
                  if (tex->textures[p]) {
      fd_resource_set_usage(tex->textures[p]->texture, FD_DIRTY_TEX);
   fd_dirty_shader_resource(ctx, tex->textures[p]->texture,
            } else {
            }
   for (; i < nr + unbind_num_trailing_slots; i++) {
      unsigned p = i + start;
   pipe_sampler_view_reference(&tex->textures[p], NULL);
                           }
      void
   fd_texture_init(struct pipe_context *pctx)
   {
      if (!pctx->delete_sampler_state)
         if (!pctx->sampler_view_destroy)
      }
      /* helper for setting up border-color buffer for a3xx/a4xx: */
   void
   fd_setup_border_colors(struct fd_texture_stateobj *tex, void *ptr,
         {
               for (i = 0; i < tex->num_samplers; i++) {
      struct pipe_sampler_state *sampler = tex->samplers[i];
   uint16_t *bcolor =
      (uint16_t *)((uint8_t *)ptr + (BORDERCOLOR_SIZE * offset) +
               if (!sampler)
                     const struct util_format_description *desc =
         for (j = 0; j < 4; j++) {
                     const struct util_format_channel_description *chan =
         uint8_t native = desc->swizzle[j];
   /* Special cases:
   *  - X24S8 is implemented with 8_8_8_8_UINT, so the 'native'
   *    location is actually 0 rather than 1
   *  - X32_S8X24_UINT has stencil with a secretly-S8_UINT resource
   *    so again we want 0 rather than 1
   *
   * In both cases, there is only one non-void format, so we don't
   * have to be too careful.
   *
   * Note that this only affects a4xx -- a3xx did not support
   * stencil texturing, and a5xx+ don't use this helper.
   */
   if (format == PIPE_FORMAT_X24S8_UINT ||
                        if (chan->pure_integer) {
      bcolor32[native + 4] = sampler->border_color.i[j];
      } else {
      bcolor32[native] = fui(sampler->border_color.f[j]);
   bcolor[native] =
               }
