   /**************************************************************************
   * 
   * Copyright 2006 VMware, Inc.
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
   /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   *   Michel Dänzer <daenzer@vmware.com>
   */
      #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
      #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_transfer.h"
   #include "util/u_surface.h"
      #include "sp_context.h"
   #include "sp_flush.h"
   #include "sp_texture.h"
   #include "sp_screen.h"
      #include "frontend/sw_winsys.h"
         /**
   * Conventional allocation path for non-display textures:
   * Use a simple, maximally packed layout.
   */
   static bool
   softpipe_resource_layout(struct pipe_screen *screen,
               {
      struct pipe_resource *pt = &spr->base;
   unsigned level;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;
            for (level = 0; level <= pt->last_level; level++) {
                        if (pt->target == PIPE_TEXTURE_CUBE)
            if (pt->target == PIPE_TEXTURE_3D)
         else
                              /* if row_stride * height > SP_MAX_TEXTURE_SIZE */
   if ((uint64_t)spr->stride[level] * nblocksy > SP_MAX_TEXTURE_SIZE) {
      /* image too large */
                                 width  = u_minify(width, 1);
   height = u_minify(height, 1);
               if (buffer_size > SP_MAX_TEXTURE_SIZE)
            if (allocate) {
      spr->data = align_malloc(buffer_size, 64);
      }
   else {
            }
         /**
   * Check the size of the texture specified by 'res'.
   * \return TRUE if OK, FALSE if too large.
   */
   static bool
   softpipe_can_create_resource(struct pipe_screen *screen,
         {
      struct softpipe_resource spr;
   memset(&spr, 0, sizeof(spr));
   spr.base = *res;
      }
         /**
   * Texture layout for simple color buffers.
   */
   static bool
   softpipe_displaytarget_layout(struct pipe_screen *screen,
               {
               /* Round up the surface size to a multiple of the tile size?
   */
   spr->dt = winsys->displaytarget_create(winsys,
                                                   }
         /**
   * Create new pipe_resource given the template information.
   */
   static struct pipe_resource *
   softpipe_resource_create_front(struct pipe_screen *screen,
               {
      struct softpipe_resource *spr = CALLOC_STRUCT(softpipe_resource);
   if (!spr)
                     spr->base = *templat;
   pipe_reference_init(&spr->base.reference, 1);
            spr->pot = (util_is_power_of_two_or_zero(templat->width0) &&
                  if (spr->base.bind & (PIPE_BIND_DISPLAY_TARGET |
   PIPE_BIND_SCANOUT |
   PIPE_BIND_SHARED)) {
      if (!softpipe_displaytarget_layout(screen, spr, map_front_private))
      }
   else {
      if (!softpipe_resource_layout(screen, spr, true))
      }
            fail:
      FREE(spr);
      }
      static struct pipe_resource *
   softpipe_resource_create(struct pipe_screen *screen,
         {
         }
      static void
   softpipe_resource_destroy(struct pipe_screen *pscreen,
         {
      struct softpipe_screen *screen = softpipe_screen(pscreen);
            if (spr->dt) {
      /* display target */
   struct sw_winsys *winsys = screen->winsys;
      }
   else if (!spr->userBuffer) {
      /* regular texture */
                  }
         static struct pipe_resource *
   softpipe_resource_from_handle(struct pipe_screen *screen,
                     {
      struct sw_winsys *winsys = softpipe_screen(screen)->winsys;
   struct softpipe_resource *spr = CALLOC_STRUCT(softpipe_resource);
   if (!spr)
            spr->base = *templat;
   pipe_reference_init(&spr->base.reference, 1);
            spr->pot = (util_is_power_of_two_or_zero(templat->width0) &&
                  spr->dt = winsys->displaytarget_from_handle(winsys,
                     if (!spr->dt)
                  fail:
      FREE(spr);
      }
         static bool
   softpipe_resource_get_handle(struct pipe_screen *screen,
                           {
      struct sw_winsys *winsys = softpipe_screen(screen)->winsys;
            assert(spr->dt);
   if (!spr->dt)
               }
         /**
   * Helper function to compute offset (in bytes) for a particular
   * texture level/face/slice from the start of the buffer.
   */
   unsigned
   softpipe_get_tex_image_offset(const struct softpipe_resource *spr,
         {
                           }
         /**
   * Get a pipe_surface "view" into a texture resource.
   */
   static struct pipe_surface *
   softpipe_create_surface(struct pipe_context *pipe,
               {
               ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
   pipe_resource_reference(&ps->texture, pt);
   ps->context = pipe;
   ps->format = surf_tmpl->format;
   if (pt->target != PIPE_BUFFER) {
      assert(surf_tmpl->u.tex.level <= pt->last_level);
   ps->width = u_minify(pt->width0, surf_tmpl->u.tex.level);
   ps->height = u_minify(pt->height0, surf_tmpl->u.tex.level);
   ps->u.tex.level = surf_tmpl->u.tex.level;
   ps->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
   ps->u.tex.last_layer = surf_tmpl->u.tex.last_layer;
   if (ps->u.tex.first_layer != ps->u.tex.last_layer) {
            }
   else {
      /* setting width as number of elements should get us correct renderbuffer width */
   ps->width = surf_tmpl->u.buf.last_element - surf_tmpl->u.buf.first_element + 1;
   ps->height = pt->height0;
   ps->u.buf.first_element = surf_tmpl->u.buf.first_element;
   ps->u.buf.last_element = surf_tmpl->u.buf.last_element;
   assert(ps->u.buf.first_element <= ps->u.buf.last_element);
         }
      }
         /**
   * Free a pipe_surface which was created with softpipe_create_surface().
   */
   static void 
   softpipe_surface_destroy(struct pipe_context *pipe,
         {
      /* Effectively do the texture_update work here - if texture images
   * needed post-processing to put them into hardware layout, this is
   * where it would happen.  For softpipe, nothing to do.
   */
   assert(surf->texture);
   pipe_resource_reference(&surf->texture, NULL);
      }
         /**
   * Geta pipe_transfer object which is used for moving data in/out of
   * a resource object.
   * \param pipe  rendering context
   * \param resource  the resource to transfer in/out of
   * \param level  which mipmap level
   * \param usage  bitmask of PIPE_MAP_x flags
   * \param box  the 1D/2D/3D region of interest
   */
   static void *
   softpipe_transfer_map(struct pipe_context *pipe,
                        struct pipe_resource *resource,
      {
      struct sw_winsys *winsys = softpipe_screen(pipe->screen)->winsys;
   struct softpipe_resource *spr = softpipe_resource(resource);
   struct softpipe_transfer *spt;
   struct pipe_transfer *pt;
   enum pipe_format format = resource->format;
            assert(resource);
            /* make sure the requested region is in the image bounds */
   assert(box->x + box->width <= (int) u_minify(resource->width0, level));
   if (resource->target == PIPE_TEXTURE_1D_ARRAY) {
         }
   else {
      assert(box->y + box->height <= (int) u_minify(resource->height0, level));
   if (resource->target == PIPE_TEXTURE_2D_ARRAY) {
         }
   else if (resource->target == PIPE_TEXTURE_CUBE) {
         }
   else if (resource->target == PIPE_TEXTURE_CUBE_ARRAY) {
         }
   else {
                     /*
   * Transfers, like other pipe operations, must happen in order, so flush the
   * context if necessary.
   */
   if (!(usage & PIPE_MAP_UNSYNCHRONIZED)) {
      bool read_only = !(usage & PIPE_MAP_WRITE);
   bool do_not_block = !!(usage & PIPE_MAP_DONTBLOCK);
   if (!softpipe_flush_resource(pipe, resource,
                              level, box->depth > 1 ? -1 : box->z,
   /*
   * It would have blocked, but state tracker requested no to.
   */
   assert(do_not_block);
                  spt = CALLOC_STRUCT(softpipe_transfer);
   if (!spt)
                     pipe_resource_reference(&pt->resource, resource);
   pt->level = level;
   pt->usage = usage;
   pt->box = *box;
   pt->stride = spr->stride[level];
                     spt->offset +=
                  /* resources backed by display target treated specially:
   */
   if (spr->dt) {
         }
   else {
                  if (!map) {
      pipe_resource_reference(&pt->resource, NULL);
   FREE(spt);
               *transfer = pt;
      }
         /**
   * Unmap memory mapping for given pipe_transfer object.
   */
   static void
   softpipe_transfer_unmap(struct pipe_context *pipe,
         {
               assert(transfer->resource);
            if (spr->dt) {
      /* display target */
   struct sw_winsys *winsys = softpipe_screen(pipe->screen)->winsys;
               if (transfer->usage & PIPE_MAP_WRITE) {
      /* Mark the texture as dirty to expire the tile caches. */
               pipe_resource_reference(&transfer->resource, NULL);
      }
      /**
   * Create buffer which wraps user-space data.
   */
   struct pipe_resource *
   softpipe_user_buffer_create(struct pipe_screen *screen,
                     {
               spr = CALLOC_STRUCT(softpipe_resource);
   if (!spr)
            pipe_reference_init(&spr->base.reference, 1);
   spr->base.screen = screen;
   spr->base.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   spr->base.bind = bind_flags;
   spr->base.usage = PIPE_USAGE_IMMUTABLE;
   spr->base.flags = 0;
   spr->base.width0 = bytes;
   spr->base.height0 = 1;
   spr->base.depth0 = 1;
   spr->base.array_size = 1;
   spr->userBuffer = true;
               }
         void
   softpipe_init_texture_funcs(struct pipe_context *pipe)
   {
      pipe->buffer_map = softpipe_transfer_map;
   pipe->buffer_unmap = softpipe_transfer_unmap;
   pipe->texture_map = softpipe_transfer_map;
            pipe->transfer_flush_region = u_default_transfer_flush_region;
   pipe->buffer_subdata = u_default_buffer_subdata;
            pipe->create_surface = softpipe_create_surface;
   pipe->surface_destroy = softpipe_surface_destroy;
      }
         void
   softpipe_init_screen_texture_funcs(struct pipe_screen *screen)
   {
      screen->resource_create = softpipe_resource_create;
   screen->resource_create_front = softpipe_resource_create_front;
   screen->resource_destroy = softpipe_resource_destroy;
   screen->resource_from_handle = softpipe_resource_from_handle;
   screen->resource_get_handle = softpipe_resource_get_handle;
      }
