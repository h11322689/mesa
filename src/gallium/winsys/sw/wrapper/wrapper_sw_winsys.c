   /**********************************************************
   * Copyright 2010 VMware, Inc.  All rights reserved.
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
         #include "wrapper_sw_winsys.h"
      #include "util/format/u_formats.h"
   #include "pipe/p_state.h"
      #include "frontend/sw_winsys.h"
      #include "util/u_memory.h"
   #include "util/u_inlines.h"
      /*
   * This code wraps a pipe_screen and exposes a sw_winsys interface for use
   * with software resterizers. This code is used by the DRM based winsys to
   * allow access to the drm driver.
   *
   * We must borrow the whole stack because only the pipe screen knows how
   * to decode the content of a buffer. Or how to create a buffer that
   * can still be used by drivers using real hardware (as the case is
   * with software st/xorg but hw st/dri).
   *
   * We also need a pipe context for the transfers.
   */
      struct wrapper_sw_winsys
   {
      struct sw_winsys base;
   struct pipe_screen *screen;
   struct pipe_context *pipe;
      };
      struct wrapper_sw_displaytarget
   {
      struct wrapper_sw_winsys *winsys;
   struct pipe_resource *tex;
            unsigned map_count;
   unsigned stride; /**< because we get stride at create */
      };
      static inline struct wrapper_sw_winsys *
   wrapper_sw_winsys(struct sw_winsys *ws)
   {
         }
      static inline struct wrapper_sw_displaytarget *
   wrapper_sw_displaytarget(struct sw_displaytarget *dt)
   {
         }
         /*
   * Functions
   */
         static bool
   wsw_is_dt_format_supported(struct sw_winsys *ws,
               {
               return wsw->screen->is_format_supported(wsw->screen, format,
                  }
      static bool
   wsw_dt_get_stride(struct wrapper_sw_displaytarget *wdt, unsigned *stride)
   {
      struct pipe_context *pipe = wdt->winsys->pipe;
   struct pipe_resource *tex = wdt->tex;
   struct pipe_transfer *tr;
            map = pipe_texture_map(pipe, tex, 0, 0,
               if (!map)
            *stride = tr->stride;
                        }
      static struct sw_displaytarget *
   wsw_dt_wrap_texture(struct wrapper_sw_winsys *wsw,
         {
      struct wrapper_sw_displaytarget *wdt = CALLOC_STRUCT(wrapper_sw_displaytarget);
   if (!wdt)
            wdt->tex = tex;
            if (!wsw_dt_get_stride(wdt, stride))
                  err_free:
         err_unref:
      pipe_resource_reference(&tex, NULL);
      }
      static struct sw_displaytarget *
   wsw_dt_create(struct sw_winsys *ws,
               unsigned bind,
   enum pipe_format format,
   unsigned width, unsigned height,
   unsigned alignment,
   {
      struct wrapper_sw_winsys *wsw = wrapper_sw_winsys(ws);
   struct pipe_resource templ;
            /*
   * XXX Why don't we just get the template.
   */
   memset(&templ, 0, sizeof(templ));
   templ.target = wsw->target;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.format = format;
                     tex = wsw->screen->resource_create(wsw->screen, &templ);
   if (!tex)
               }
      static struct sw_displaytarget *
   wsw_dt_from_handle(struct sw_winsys *ws,
                     {
      struct wrapper_sw_winsys *wsw = wrapper_sw_winsys(ws);
            tex = wsw->screen->resource_from_handle(wsw->screen, templ, whandle,
         if (!tex)
               }
      static bool
   wsw_dt_get_handle(struct sw_winsys *ws,
               {
      struct wrapper_sw_winsys *wsw = wrapper_sw_winsys(ws);
   struct wrapper_sw_displaytarget *wdt = wrapper_sw_displaytarget(dt);
            return wsw->screen->resource_get_handle(wsw->screen, NULL, tex, whandle,
      }
      static void *
   wsw_dt_map(struct sw_winsys *ws,
               {
      struct wrapper_sw_displaytarget *wdt = wrapper_sw_displaytarget(dt);
   struct pipe_context *pipe = wdt->winsys->pipe;
   struct pipe_resource *tex = wdt->tex;
   struct pipe_transfer *tr;
                                 ptr = pipe_texture_map(pipe, tex, 0, 0,
               if (!ptr)
            wdt->transfer = tr;
            /* XXX Handle this case */
                              err:
      pipe->texture_unmap(pipe, tr);
      }
      static void
   wsw_dt_unmap(struct sw_winsys *ws,
         {
      struct wrapper_sw_displaytarget *wdt = wrapper_sw_displaytarget(dt);
                              if (wdt->map_count)
            pipe->texture_unmap(pipe, wdt->transfer);
   pipe->flush(pipe, NULL, 0);
      }
      static void
   wsw_dt_destroy(struct sw_winsys *ws,
         {
                           }
      static void
   wsw_destroy(struct sw_winsys *ws)
   {
               wsw->pipe->destroy(wsw->pipe);
               }
      struct sw_winsys *
   wrapper_sw_winsys_wrap_pipe_screen(struct pipe_screen *screen)
   {
               if (!wsw)
            wsw->base.is_displaytarget_format_supported = wsw_is_dt_format_supported;
   wsw->base.displaytarget_create = wsw_dt_create;
   wsw->base.displaytarget_from_handle = wsw_dt_from_handle;
   wsw->base.displaytarget_get_handle = wsw_dt_get_handle;
   wsw->base.displaytarget_map = wsw_dt_map;
   wsw->base.displaytarget_unmap = wsw_dt_unmap;
   wsw->base.displaytarget_destroy = wsw_dt_destroy;
            wsw->screen = screen;
   wsw->pipe = screen->context_create(screen, NULL, 0);
   if (!wsw->pipe)
            if(screen->get_param(screen, PIPE_CAP_NPOT_TEXTURES))
         else
                  err_free:
         err:
         }
      struct pipe_screen *
   wrapper_sw_winsys_dewrap_pipe_screen(struct sw_winsys *ws)
   {
      struct wrapper_sw_winsys *wsw = wrapper_sw_winsys(ws);
            wsw->pipe->destroy(wsw->pipe);
            FREE(wsw);
      }
