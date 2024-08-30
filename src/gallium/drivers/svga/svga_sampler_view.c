   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
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
   **********************************************************/
      #include "svga_cmd.h"
      #include "pipe/p_state.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_thread.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "svga_format.h"
   #include "svga_screen.h"
   #include "svga_context.h"
   #include "svga_resource_texture.h"
   #include "svga_sampler_view.h"
   #include "svga_debug.h"
   #include "svga_surface.h"
         void
   svga_debug_describe_sampler_view(char *buf, const struct svga_sampler_view *sv)
   {
      char res[128];
   debug_describe_resource(res, sv->texture);
   sprintf(buf, "svga_sampler_view<%s,[%u,%u]>",
      }
         struct svga_sampler_view *
   svga_get_tex_sampler_view(struct pipe_context *pipe,
      struct pipe_resource *pt,
      {
      struct svga_context *svga = svga_context(pipe);
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_texture *tex = svga_texture(pt);
   struct svga_sampler_view *sv = NULL;
   SVGA3dSurface1Flags flags = SVGA3D_SURFACE_HINT_TEXTURE;
   SVGA3dSurfaceFormat format = svga_translate_format(ss, pt->format,
                  assert(pt);
   assert(min_lod <= max_lod);
   assert(max_lod <= pt->last_level);
            /* Is a view needed */
   {
      /*
   * Can't control max lod. For first level views and when we only
   * look at one level we disable mip filtering to achive the same
   * results as a view.
   */
   if (min_lod == 0 && max_lod >= pt->last_level)
            if (ss->debug.no_sampler_view)
            if (ss->debug.force_sampler_view)
               /* First try the cache */
   if (view) {
      mtx_lock(&ss->tex_mutex);
   if (tex->cached_view &&
      tex->cached_view->min_lod == min_lod &&
   tex->cached_view->max_lod == max_lod) {
   svga_sampler_view_reference(&sv, tex->cached_view);
   mtx_unlock(&ss->tex_mutex);
   SVGA_DBG(DEBUG_VIEWS, "svga: Sampler view: reuse %p, %u %u, last %u\n",
         svga_validate_sampler_view(svga_context(pipe), sv);
      }
               sv = CALLOC_STRUCT(svga_sampler_view);
   if (!sv)
                     /* Note: we're not refcounting the texture resource here to avoid
   * a circular dependency.
   */
            sv->min_lod = min_lod;
            /* No view needed just use the whole texture */
   if (!view) {
      SVGA_DBG(DEBUG_VIEWS,
            "svga: Sampler view: no %p, mips %u..%u, nr %u, size (%ux%ux%u), last %u\n",
   pt, min_lod, max_lod,
   max_lod - min_lod + 1,
   pt->width0,
   pt->height0,
      sv->key.cachable = 0;
   sv->handle = tex->handle;
   debug_reference(&sv->reference,
                     SVGA_DBG(DEBUG_VIEWS,
            "svga: Sampler view: yes %p, mips %u..%u, nr %u, size (%ux%ux%u), last %u\n",
   pt, min_lod, max_lod,
   max_lod - min_lod + 1,
   pt->width0,
   pt->height0,
         sv->age = tex->age;
   sv->handle = svga_texture_view_surface(svga, tex,
                                          if (!sv->handle) {
      sv->key.cachable = 0;
   sv->handle = tex->handle;
   debug_reference(&sv->reference,
                           mtx_lock(&ss->tex_mutex);
   svga_sampler_view_reference(&tex->cached_view, sv);
            debug_reference(&sv->reference,
                     }
         void
   svga_validate_sampler_view(struct svga_context *svga,
         {
      struct svga_texture *tex = svga_texture(v->texture);
   unsigned numFaces;
   unsigned age = 0;
   int i;
            assert(svga);
            if (v->handle == tex->handle)
                     if (tex->b.target == PIPE_TEXTURE_CUBE)
         else
            for (i = v->min_lod; i <= v->max_lod; i++) {
      for (k = 0; k < numFaces; k++) {
      assert(i < ARRAY_SIZE(tex->view_age));
   if (v->age < tex->view_age[i])
      svga_texture_copy_handle(svga,
                                          }
         void
   svga_destroy_sampler_view_priv(struct svga_sampler_view *v)
   {
               if (v->handle != tex->handle) {
      struct svga_screen *ss = svga_screen(v->texture->screen);
   SVGA_DBG(DEBUG_DMA, "unref sid %p (sampler view)\n", v->handle);
   svga_screen_surface_destroy(ss, &v->key,
                     /* Note: we're not refcounting the texture resource here to avoid
   * a circular dependency.
   */
               }
