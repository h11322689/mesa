   /**************************************************************************
   *
   * Copyright 2009, VMware, Inc.
   * All Rights Reserved.
   * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
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
      #ifdef HAVE_SYS_SHM_H
   #include <sys/ipc.h>
   #include <sys/shm.h>
   #ifdef __FreeBSD__
   /* sys/ipc.h -> sys/_types.h -> machine/param.h
   * - defines ALIGN which clashes with our ALIGN
   */
   #undef ALIGN
   #endif
   #endif
      #include "util/compiler.h"
   #include "util/format/u_formats.h"
   #include "pipe/p_state.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "frontend/sw_winsys.h"
   #include "dri_sw_winsys.h"
         struct dri_sw_displaytarget
   {
      enum pipe_format format;
   unsigned width;
   unsigned height;
            unsigned map_flags;
   int shmid;
   void *data;
   void *mapped;
      };
      struct dri_sw_winsys
   {
                  };
      static inline struct dri_sw_displaytarget *
   dri_sw_displaytarget( struct sw_displaytarget *dt )
   {
         }
      static inline struct dri_sw_winsys *
   dri_sw_winsys( struct sw_winsys *ws )
   {
         }
         static bool
   dri_sw_is_displaytarget_format_supported( struct sw_winsys *ws,
               {
      /* TODO: check visuals or other sensible thing here */
      }
      #ifdef HAVE_SYS_SHM_H
   static char *
   alloc_shm(struct dri_sw_displaytarget *dri_sw_dt, unsigned size)
   {
               /* 0600 = user read+write */
   dri_sw_dt->shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
   if (dri_sw_dt->shmid < 0)
            addr = (char *) shmat(dri_sw_dt->shmid, NULL, 0);
   /* mark the segment immediately for deletion to avoid leaks */
            if (addr == (char *) -1)
               }
   #endif
      static struct sw_displaytarget *
   dri_sw_displaytarget_create(struct sw_winsys *winsys,
                              unsigned tex_usage,
      {
      UNUSED struct dri_sw_winsys *ws = dri_sw_winsys(winsys);
   struct dri_sw_displaytarget *dri_sw_dt;
            dri_sw_dt = CALLOC_STRUCT(dri_sw_displaytarget);
   if(!dri_sw_dt)
            dri_sw_dt->format = format;
   dri_sw_dt->width = width;
   dri_sw_dt->height = height;
            format_stride = util_format_get_stride(format, width);
            nblocksy = util_format_get_nblocksy(format, height);
                  #ifdef HAVE_SYS_SHM_H
      if (ws->lf->put_image_shm)
      #endif
         if(!dri_sw_dt->data)
            if(!dri_sw_dt->data)
            *stride = dri_sw_dt->stride;
         no_data:
         no_dt:
         }
      static void
   dri_sw_displaytarget_destroy(struct sw_winsys *ws,
         {
                  #ifdef HAVE_SYS_SHM_H
         shmdt(dri_sw_dt->data);
   #endif
      } else {
                     }
      static void *
   dri_sw_displaytarget_map(struct sw_winsys *ws,
               {
      struct dri_sw_displaytarget *dri_sw_dt = dri_sw_displaytarget(dt);
            if (dri_sw_dt->front_private && (flags & PIPE_MAP_READ)) {
      struct dri_sw_winsys *dri_sw_ws = dri_sw_winsys(ws);
      }
   dri_sw_dt->map_flags = flags;
      }
      static void
   dri_sw_displaytarget_unmap(struct sw_winsys *ws,
         {
      struct dri_sw_displaytarget *dri_sw_dt = dri_sw_displaytarget(dt);
   if (dri_sw_dt->front_private && (dri_sw_dt->map_flags & PIPE_MAP_WRITE)) {
      struct dri_sw_winsys *dri_sw_ws = dri_sw_winsys(ws);
      }
   dri_sw_dt->map_flags = 0;
      }
      static struct sw_displaytarget *
   dri_sw_displaytarget_from_handle(struct sw_winsys *winsys,
                     {
      assert(0);
      }
      static bool
   dri_sw_displaytarget_get_handle(struct sw_winsys *winsys,
               {
               if (whandle->type == WINSYS_HANDLE_TYPE_SHMID) {
      if (dri_sw_dt->shmid < 0)
         whandle->handle = dri_sw_dt->shmid;
                  }
      static void
   dri_sw_displaytarget_display(struct sw_winsys *ws,
                     {
      struct dri_sw_winsys *dri_sw_ws = dri_sw_winsys(ws);
   struct dri_sw_displaytarget *dri_sw_dt = dri_sw_displaytarget(dt);
   struct dri_drawable *dri_drawable = (struct dri_drawable *)context_private;
   unsigned width, height, x = 0, y = 0;
   unsigned blsize = util_format_get_blocksize(dri_sw_dt->format);
   unsigned offset = 0;
   unsigned offset_x = 0;
   char *data = dri_sw_dt->data;
   bool is_shm = dri_sw_dt->shmid != -1;
   /* Set the width to 'stride / cpp'.
   *
   * PutImage correctly clips to the width of the dst drawable.
   */
   if (box) {
      offset = dri_sw_dt->stride * box->y;
   offset_x = box->x * blsize;
   data += offset;
   /* don't add x offset for shm, the put_image_shm will deal with it */
   if (!is_shm)
         x = box->x;
   y = box->y;
   width = box->width;
      } else {
      width = dri_sw_dt->stride / blsize;
               if (is_shm) {
      dri_sw_ws->lf->put_image_shm(dri_drawable, dri_sw_dt->shmid, dri_sw_dt->data, offset, offset_x,
                     if (box)
      dri_sw_ws->lf->put_image2(dri_drawable, data,
      else
      }
      static void
   dri_destroy_sw_winsys(struct sw_winsys *winsys)
   {
         }
      struct sw_winsys *
   dri_create_sw_winsys(const struct drisw_loader_funcs *lf)
   {
               ws = CALLOC_STRUCT(dri_sw_winsys);
   if (!ws)
            ws->lf = lf;
                     /* screen texture functions */
   ws->base.displaytarget_create = dri_sw_displaytarget_create;
   ws->base.displaytarget_destroy = dri_sw_displaytarget_destroy;
   ws->base.displaytarget_from_handle = dri_sw_displaytarget_from_handle;
            /* texture functions */
   ws->base.displaytarget_map = dri_sw_displaytarget_map;
                        }
      /* vim: set sw=3 ts=8 sts=3 expandtab: */
