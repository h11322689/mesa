   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
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
      #include "util/u_inlines.h"
   #include "util/u_hash_table.h"
   #include "util/u_memory.h"
      #include "tr_screen.h"
   #include "tr_context.h"
   #include "tr_texture.h"
         struct pipe_surface *
   trace_surf_create(struct trace_context *tr_ctx,
               {
               if (!surface)
                     tr_surf = CALLOC_STRUCT(trace_surface);
   if (!tr_surf)
            memcpy(&tr_surf->base, surface, sizeof(struct pipe_surface));
            pipe_reference_init(&tr_surf->base.reference, 1);
   tr_surf->base.texture = NULL;
   pipe_resource_reference(&tr_surf->base.texture, res);
                  error:
      pipe_surface_reference(&surface, NULL);
      }
         void
   trace_surf_destroy(struct trace_surface *tr_surf)
   {
      pipe_resource_reference(&tr_surf->base.texture, NULL);
   pipe_surface_reference(&tr_surf->surface, NULL);
      }
         struct pipe_transfer *
   trace_transfer_create(struct trace_context *tr_ctx,
         struct pipe_resource *res,
   {
               if (!transfer)
            tr_trans = CALLOC_STRUCT(trace_transfer);
   if (!tr_trans)
                     tr_trans->base.b.resource = NULL;
            pipe_resource_reference(&tr_trans->base.b.resource, res);
                  error:
      if (res->target == PIPE_BUFFER)
         else
            }
         void
   trace_transfer_destroy(struct trace_context *tr_context,
         {
      pipe_resource_reference(&tr_trans->base.b.resource, NULL);
      }
      /* Arbitrarily large refcount to "avoid having the driver bypass the samplerview wrapper and destroying
   the samplerview prematurely" see 7f5a3530125 ("aux/trace: use private refcounts for samplerviews") */
   #define SAMPLER_VIEW_PRIVATE_REFCOUNT 100000000
      struct pipe_sampler_view *
   trace_sampler_view_create(struct trace_context *tr_ctx,
               {
      assert(tr_res == view->texture);
   struct trace_sampler_view *tr_view = CALLOC_STRUCT(trace_sampler_view);
   memcpy(&tr_view->base, view, sizeof(struct pipe_sampler_view));
   tr_view->base.reference.count = 1;
   tr_view->base.texture = NULL;
   pipe_resource_reference(&tr_view->base.texture, tr_res);
   tr_view->base.context = &tr_ctx->base;
   tr_view->sampler_view = view;
   view->reference.count += SAMPLER_VIEW_PRIVATE_REFCOUNT;
   tr_view->refcount = SAMPLER_VIEW_PRIVATE_REFCOUNT;
      }
      void
   trace_sampler_view_destroy(struct trace_sampler_view *tr_view)
   {
      p_atomic_add(&tr_view->sampler_view->reference.count, -tr_view->refcount);
   pipe_sampler_view_reference(&tr_view->sampler_view, NULL);
   pipe_resource_reference(&tr_view->base.texture, NULL);
      }
      struct pipe_sampler_view *
   trace_sampler_view_unwrap(struct trace_sampler_view *tr_view)
   {
      if (!tr_view)
         tr_view->refcount--;
   if (!tr_view->refcount) {
      tr_view->refcount = SAMPLER_VIEW_PRIVATE_REFCOUNT;
      }
      }
      #undef SAMPLER_VIEW_PRIVATE_REFCOUNT
