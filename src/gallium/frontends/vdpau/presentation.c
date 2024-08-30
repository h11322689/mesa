   /**************************************************************************
   *
   * Copyright 2010 Thomas Balling Sørensen.
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
      #include <stdio.h>
   #include <vdpau/vdpau.h>
      #include "util/u_debug.h"
   #include "util/u_memory.h"
      #include "vdpau_private.h"
      /**
   * Create a VdpPresentationQueue.
   */
   VdpStatus
   vlVdpPresentationQueueCreate(VdpDevice device,
               {
      vlVdpPresentationQueue *pq = NULL;
            if (!presentation_queue)
            vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
            vlVdpPresentationQueueTarget *pqt = vlGetDataHTAB(presentation_queue_target);
   if (!pqt)
            if (dev != pqt->device)
            pq = CALLOC(1, sizeof(vlVdpPresentationQueue));
   if (!pq)
            DeviceReference(&pq->device, dev);
            mtx_lock(&dev->mutex);
   if (!vl_compositor_init_state(&pq->cstate, dev->context)) {
      mtx_unlock(&dev->mutex);
   ret = VDP_STATUS_ERROR;
      }
            *presentation_queue = vlAddDataHTAB(pq);
   if (*presentation_queue == 0) {
      ret = VDP_STATUS_ERROR;
                     no_handle:
   no_compositor:
      DeviceReference(&pq->device, NULL);
   FREE(pq);
      }
      /**
   * Destroy a VdpPresentationQueue.
   */
   VdpStatus
   vlVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
   {
               pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
            mtx_lock(&pq->device->mutex);
   vl_compositor_cleanup_state(&pq->cstate);
            vlRemoveDataHTAB(presentation_queue);
   DeviceReference(&pq->device, NULL);
               }
      /**
   * Configure the background color setting.
   */
   VdpStatus
   vlVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
         {
      vlVdpPresentationQueue *pq;
            if (!background_color)
            pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
            color.f[0] = background_color->red;
   color.f[1] = background_color->green;
   color.f[2] = background_color->blue;
            mtx_lock(&pq->device->mutex);
   vl_compositor_set_clear_color(&pq->cstate, &color);
               }
      /**
   * Retrieve the current background color setting.
   */
   VdpStatus
   vlVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
         {
      vlVdpPresentationQueue *pq;
            if (!background_color)
            pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
            mtx_lock(&pq->device->mutex);
   vl_compositor_get_clear_color(&pq->cstate, &color);
            background_color->red = color.f[0];
   background_color->green = color.f[1];
   background_color->blue = color.f[2];
               }
      /**
   * Retrieve the presentation queue's "current" time.
   */
   VdpStatus
   vlVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue,
         {
               if (!current_time)
            pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
            mtx_lock(&pq->device->mutex);
   *current_time = pq->device->vscreen->get_timestamp(pq->device->vscreen,
                     }
      /**
   * Enter a surface into the presentation queue.
   */
   VdpStatus
   vlVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue,
                           {
               vlVdpPresentationQueue *pq;
            struct pipe_context *pipe;
   struct pipe_resource *tex;
   struct pipe_surface surf_templ, *surf_draw = NULL;
            struct vl_compositor *compositor;
   struct vl_compositor_state *cstate;
            pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
            surf = vlGetDataHTAB(surface);
   if (!surf)
            pipe = pq->device->context;
   compositor = &pq->device->compositor;
   cstate = &pq->cstate;
            mtx_lock(&pq->device->mutex);
   if (vscreen->set_back_texture_from_output && surf->send_to_X)
         tex = vscreen->texture_from_drawable(vscreen, (void *)pq->drawable);
   if (!tex) {
      mtx_unlock(&pq->device->mutex);
               if (!vscreen->set_back_texture_from_output || !surf->send_to_X) {
               memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = tex->format;
            dst_clip.x0 = 0;
   dst_clip.y0 = 0;
   dst_clip.x1 = clip_width ? clip_width : surf_draw->width;
            src_rect.x0 = 0;
   src_rect.y0 = 0;
   src_rect.x1 = surf_draw->width;
            vl_compositor_clear_layers(cstate);
   vl_compositor_set_rgba_layer(cstate, compositor, 0, surf->sampler_view, &src_rect, NULL, NULL);
   vl_compositor_set_dst_clip(cstate, &dst_clip);
                        // flush before calling flush_frontbuffer so that rendering is flushed
   //  to back buffer so the texture can be copied in flush_frontbuffer
   pipe->screen->fence_reference(pipe->screen, &surf->fence, NULL);
   pipe->flush(pipe, &surf->fence, 0);
   pipe->screen->flush_frontbuffer(pipe->screen, pipe, tex, 0, 0,
                     if (dump_window == -1) {
                  if (dump_window) {
      static unsigned int framenum = 0;
            if (framenum) {
      sprintf(cmd, "xwd -id %d -silent -out vdpau_frame_%08d.xwd", (int)pq->drawable, framenum);
   if (system(cmd) != 0)
      }
               if (!vscreen->set_back_texture_from_output || !surf->send_to_X) {
      pipe_resource_reference(&tex, NULL);
      }
               }
      /**
   * Wait for a surface to finish being displayed.
   */
   VdpStatus
   vlVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
               {
      vlVdpPresentationQueue *pq;
   vlVdpOutputSurface *surf;
            if (!first_presentation_time)
            pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
            surf = vlGetDataHTAB(surface);
   if (!surf)
            mtx_lock(&pq->device->mutex);
   if (surf->fence) {
      screen = pq->device->vscreen->pscreen;
   screen->fence_finish(screen, NULL, surf->fence, OS_TIMEOUT_INFINITE);
      }
               }
      /**
   * Poll the current queue status of a surface.
   */
   VdpStatus
   vlVdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                     {
      vlVdpPresentationQueue *pq;
   vlVdpOutputSurface *surf;
            if (!(status && first_presentation_time))
            pq = vlGetDataHTAB(presentation_queue);
   if (!pq)
            surf = vlGetDataHTAB(surface);
   if (!surf)
                     if (!surf->fence) {
      if (pq->last_surf == surf)
         else
      } else {
      mtx_lock(&pq->device->mutex);
   screen = pq->device->vscreen->pscreen;
   if (screen->fence_finish(screen, NULL, surf->fence, 0)) {
      screen->fence_reference(screen, &surf->fence, NULL);
                  // We actually need to query the timestamp of the last VSYNC event from the hardware
   vlVdpPresentationQueueGetTime(presentation_queue, first_presentation_time);
      } else {
      *status = VDP_PRESENTATION_QUEUE_STATUS_QUEUED;
                     }
