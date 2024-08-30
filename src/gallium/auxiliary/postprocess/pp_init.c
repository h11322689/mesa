   /**************************************************************************
   *
   * Copyright 2011 Lauri Kasanen
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
   * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "util/compiler.h"
      #include "postprocess/filters.h"
   #include "postprocess/pp_private.h"
      #include "pipe/p_screen.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "cso_cache/cso_context.h"
      const struct pp_filter_t pp_filters[PP_FILTERS] = {
   /*    name			inner	shaders	verts	init			run                       free   */
      { "pp_noblue",		0,	2,	1,	pp_noblue_init,		pp_nocolor,               pp_nocolor_free },
   { "pp_nogreen",		0,	2,	1,	pp_nogreen_init,	pp_nocolor,               pp_nocolor_free },
   { "pp_nored",		0,	2,	1,	pp_nored_init,		pp_nocolor,               pp_nocolor_free },
   { "pp_celshade",		0,	2,	1,	pp_celshade_init,	pp_nocolor,               pp_celshade_free },
   { "pp_jimenezmlaa",		2,	5,	2,	pp_jimenezmlaa_init,	pp_jimenezmlaa,           pp_jimenezmlaa_free },
      };
      /** Initialize the post-processing queue. */
   struct pp_queue_t *
   pp_init(struct pipe_context *pipe, const unsigned int *enabled,
         struct cso_context *cso, struct st_context *st,
   {
      unsigned int num_filters = 0;
   unsigned int curpos = 0, i, tmp_req = 0;
                     /* How many filters were requested? */
   for (i = 0; i < PP_FILTERS; i++) {
      if (enabled[i])
      }
   if (num_filters == 0)
                     if (!ppq) {
      pp_debug("Unable to allocate memory for ppq.\n");
               ppq->pp_queue = CALLOC(num_filters, sizeof(pp_func));
   if (ppq->pp_queue == NULL) {
      pp_debug("Unable to allocate memory for pp_queue.\n");
               ppq->shaders = CALLOC(num_filters, sizeof(void *));
            if ((ppq->shaders == NULL) ||
      (ppq->filters == NULL)) {
   pp_debug("Unable to allocate memory for shaders and filter arrays.\n");
               ppq->p = pp_init_prog(ppq, pipe, cso, st, st_invalidate_state);
   if (ppq->p == NULL) {
      pp_debug("pp_init_prog returned NULL.\n");
               /* Add the enabled filters to the queue, in order */
   curpos = 0;
   for (i = 0; i < PP_FILTERS; i++) {
      if (enabled[i]) {
      ppq->pp_queue[curpos] = pp_filters[i].main;
                  if (pp_filters[i].shaders) {
      ppq->shaders[curpos] =
         if (!ppq->shaders[curpos]) {
      pp_debug("Unable to allocate memory for shader list.\n");
                  /* Call the initialization function for the filter. */
   if (!pp_filters[i].init(ppq, curpos, enabled[i])) {
      pp_debug("Initialization for filter %u failed.\n", i);
                              ppq->n_filters = curpos;
   ppq->n_tmp = (curpos > 2 ? 2 : 1);
                     for (i = 0; i < curpos; i++)
            pp_debug("Queue successfully allocated. %u filter(s).\n", curpos);
            error:
         if (ppq) {
      /* Assign curpos, since we only need to destroy initialized filters. */
            /* Call the common free function which must handle partial initialization. */
                  }
      /** Free any allocated FBOs (temp buffers). Called after resizing for example. */
   void
   pp_free_fbos(struct pp_queue_t *ppq)
   {
                  if (!ppq->fbos_init)
            for (i = 0; i < ppq->n_tmp; i++) {
      pipe_surface_reference(&ppq->tmps[i], NULL);
      }
   for (i = 0; i < ppq->n_inner_tmp; i++) {
      pipe_surface_reference(&ppq->inner_tmps[i], NULL);
      }
   pipe_surface_reference(&ppq->stencils, NULL);
               }
      /** 
   * Free the pp queue. Called on context termination and failure in
   * pp_init.
   */
   void
   pp_free(struct pp_queue_t *ppq)
   {
               if (!ppq)
                     if (ppq->p) {
      if (ppq->p->pipe && ppq->filters && ppq->shaders) {
                        if (ppq->shaders[i] == NULL) {
                  /*
   * Common shader destruction code for all postprocessing
   * filters.
   */
   for (j = 0; j < pp_filters[filter].shaders; j++) {
      if (ppq->shaders[i][j] == NULL) {
                                                                  if (j >= pp_filters[filter].verts) {
      assert(ppq->p->pipe->delete_fs_state);
   ppq->p->pipe->delete_fs_state(ppq->p->pipe,
            } else {
      assert(ppq->p->pipe->delete_vs_state);
   ppq->p->pipe->delete_vs_state(ppq->p->pipe,
                        /* Finally call each filter type's free functionality. */
                              /*
   * Handle partial initialization for common resource destruction
   * in the create path.
   */
   FREE(ppq->filters);
   FREE(ppq->shaders);
   FREE(ppq->pp_queue);
                  }
      /** Internal debug function. Should be available to final users. */
   void
   pp_debug(const char *fmt, ...)
   {
               if (!debug_get_bool_option("PP_DEBUG", false))
            va_start(ap, fmt);
   _debug_vprintf(fmt, ap);
      }
      /** Allocate the temp FBOs. Called on makecurrent and resize. */
   void
   pp_init_fbos(struct pp_queue_t *ppq, unsigned int w,
         {
                  unsigned int i;
            if (ppq->fbos_init)
            pp_debug("Initializing FBOs, size %ux%u\n", w, h);
   pp_debug("Requesting %u temps and %u inner temps\n", ppq->n_tmp,
            memset(&tmp_res, 0, sizeof(tmp_res));
   tmp_res.target = PIPE_TEXTURE_2D;
   tmp_res.format = p->surf.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   tmp_res.width0 = w;
   tmp_res.height0 = h;
   tmp_res.depth0 = 1;
   tmp_res.array_size = 1;
   tmp_res.last_level = 0;
            if (!p->screen->is_format_supported(p->screen, tmp_res.format,
                  for (i = 0; i < ppq->n_tmp; i++) {
      ppq->tmp[i] = p->screen->resource_create(p->screen, &tmp_res);
            if (!ppq->tmp[i] || !ppq->tmps[i])
               for (i = 0; i < ppq->n_inner_tmp; i++) {
      ppq->inner_tmp[i] = p->screen->resource_create(p->screen, &tmp_res);
   ppq->inner_tmps[i] = p->pipe->create_surface(p->pipe,
                  if (!ppq->inner_tmp[i] || !ppq->inner_tmps[i])
                                 if (!p->screen->is_format_supported(p->screen, tmp_res.format,
                        if (!p->screen->is_format_supported(p->screen, tmp_res.format,
                     ppq->stencil = p->screen->resource_create(p->screen, &tmp_res);
   ppq->stencils = p->pipe->create_surface(p->pipe, ppq->stencil, &p->surf);
   if (!ppq->stencil || !ppq->stencils)
            p->framebuffer.width = w;
            p->viewport.scale[0] = p->viewport.translate[0] = (float) w / 2.0f;
   p->viewport.scale[1] = p->viewport.translate[1] = (float) h / 2.0f;
   p->viewport.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   p->viewport.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   p->viewport.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
                           error:
         }
