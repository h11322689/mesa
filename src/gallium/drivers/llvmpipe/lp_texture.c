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
      #include <stdio.h>
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
      #include "util/simple_mtx.h"
   #include "util/u_inlines.h"
   #include "util/u_cpu_detect.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_transfer.h"
      #include "lp_context.h"
   #include "lp_flush.h"
   #include "lp_screen.h"
   #include "lp_texture.h"
   #include "lp_setup.h"
   #include "lp_state.h"
   #include "lp_rast.h"
      #include "frontend/sw_winsys.h"
   #include "git_sha1.h"
      #ifndef _WIN32
   #include "drm-uapi/drm_fourcc.h"
   #endif
         #ifdef DEBUG
   static struct llvmpipe_resource resource_list;
   static simple_mtx_t resource_list_mutex = SIMPLE_MTX_INITIALIZER;
   #endif
   static unsigned id_counter = 0;
         /**
   * Conventional allocation path for non-display textures:
   * Compute strides and allocate data (unless asked not to).
   */
   static bool
   llvmpipe_texture_layout(struct llvmpipe_screen *screen,
               {
      struct pipe_resource *pt = &lpr->base;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;
   uint64_t total_size = 0;
   unsigned layers = pt->array_size;
            /* XXX: This alignment here (same for displaytarget) was added for the
   * purpose of ARB_map_buffer_alignment. I am not convinced it's needed for
   * non-buffer resources. Otherwise we'd want the max of cacheline size and
   * 16 (max size of a block for all formats) though this should not be
   * strictly necessary neither. In any case it can only affect compressed or
   * 1d textures.
   */
            /* KVM on Linux requires memory mapping to be aligned to the page size,
   * otherwise Linux kernel errors out on trying to map host GPU mapping
   * to guest (ARB_map_buffer_range). The improper alignment creates trouble
   * for the virgl driver when host uses llvmpipe, causing Qemu and crosvm to
   * bail out on the KVM error.
   */
   if (lpr->base.flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT)
            assert(LP_MAX_TEXTURE_2D_LEVELS <= LP_MAX_TEXTURE_LEVELS);
            for (unsigned level = 0; level <= pt->last_level; level++) {
      uint64_t mipsize;
                     /* For non-compressed formats we need 4x4 pixel alignment
   * so we can read/write LP_RASTER_BLOCK_SIZE when rendering to them.
   * We also want cache line size in x direction,
   * otherwise same cache line could end up in multiple threads.
   * For explicit 1d resources however we reduce this to 4x1 and
   * handle specially in render output code (as we need to do special
   * handling there for buffers in any case).
   */
   if (util_format_is_compressed(pt->format)) {
         } else {
      align_x = LP_RASTER_BLOCK_SIZE;
   if (llvmpipe_resource_is_1d(&lpr->base))
         else
               nblocksx = util_format_get_nblocksx(pt->format,
         nblocksy = util_format_get_nblocksy(pt->format,
                  if (util_format_is_compressed(pt->format))
         else
                           /* Number of 3D image slices, cube faces or texture array layers */
   if (lpr->base.target == PIPE_TEXTURE_CUBE) {
                  if (lpr->base.target == PIPE_TEXTURE_3D)
         else if (lpr->base.target == PIPE_TEXTURE_1D_ARRAY ||
            lpr->base.target == PIPE_TEXTURE_2D_ARRAY ||
   lpr->base.target == PIPE_TEXTURE_CUBE ||
      else
            mipsize = lpr->img_stride[level] * num_slices;
                     /* Compute size of next mipmap level */
   width = u_minify(width, 1);
   height = u_minify(height, 1);
               lpr->sample_stride = total_size;
            lpr->size_required = total_size;
   if (allocate) {
      if (total_size > LP_MAX_TEXTURE_SIZE)
            lpr->tex_data = align_malloc(total_size, mip_align);
   if (!lpr->tex_data) {
         } else {
                           fail:
         }
         /**
   * Check the size of the texture specified by 'res'.
   * \return TRUE if OK, FALSE if too large.
   */
   static bool
   llvmpipe_can_create_resource(struct pipe_screen *screen,
         {
      struct llvmpipe_resource lpr;
   memset(&lpr, 0, sizeof(lpr));
   lpr.base = *res;
   if (!llvmpipe_texture_layout(llvmpipe_screen(screen), &lpr, false))
               }
         static bool
   llvmpipe_displaytarget_layout(struct llvmpipe_screen *screen,
               {
               /* Round up the surface size to a multiple of the tile size to
   * avoid tile clipping.
   */
   const unsigned width = MAX2(1, align(lpr->base.width0, TILE_SIZE));
            lpr->dt = winsys->displaytarget_create(winsys,
                                             }
         static struct pipe_resource *
   llvmpipe_resource_create_all(struct pipe_screen *_screen,
                     {
      struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct llvmpipe_resource *lpr = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpr)
            lpr->base = *templat;
   lpr->screen = screen;
   pipe_reference_init(&lpr->base.reference, 1);
                     if (llvmpipe_resource_is_texture(&lpr->base)) {
      if (lpr->base.bind & (PIPE_BIND_DISPLAY_TARGET |
                  /* displayable surface */
   if (!llvmpipe_displaytarget_layout(screen, lpr, map_front_private))
      } else {
      /* texture map */
   if (!llvmpipe_texture_layout(screen, lpr, alloc_backing))
         } else {
      /* other data (vertex buffer, const buffer, etc) */
   const uint bytes = templat->width0;
   assert(util_format_get_blocksize(templat->format) == 1);
   assert(templat->height0 == 1);
   assert(templat->depth0 == 1);
   assert(templat->last_level == 0);
   /*
   * Reserve some extra storage since if we'd render to a buffer we
   * read/write always LP_RASTER_BLOCK_SIZE pixels, but the element
   * offset doesn't need to be aligned to LP_RASTER_BLOCK_SIZE.
   */
   /*
   * buffers don't really have stride but it's probably safer
   * (for code doing same calculations for buffers and textures)
   * to put something sane in there.
   */
            lpr->size_required = bytes;
   if (!(templat->flags & PIPE_RESOURCE_FLAG_DONT_OVER_ALLOCATE))
            if (alloc_backing) {
                                       if (!lpr->data)
                              #ifdef DEBUG
      simple_mtx_lock(&resource_list_mutex);
   list_addtail(&lpr->list, &resource_list.list);
      #endif
               fail:
      FREE(lpr);
      }
         static struct pipe_resource *
   llvmpipe_resource_create_front(struct pipe_screen *_screen,
               {
      return llvmpipe_resource_create_all(_screen, templat,
      }
         static struct pipe_resource *
   llvmpipe_resource_create(struct pipe_screen *_screen,
         {
         }
         static struct pipe_resource *
   llvmpipe_resource_create_unbacked(struct pipe_screen *_screen,
               {
      struct pipe_resource *pt =
         if (!pt)
         struct llvmpipe_resource *lpr = llvmpipe_resource(pt);
   lpr->backable = true;
   *size_required = lpr->size_required;
      }
         static struct pipe_memory_object *
   llvmpipe_memobj_create_from_handle(struct pipe_screen *pscreen,
               {
   #ifdef PIPE_MEMORY_FD
               if (handle->type == WINSYS_HANDLE_TYPE_FD &&
      pscreen->import_memory_fd(pscreen, handle->handle, &memobj->data, &memobj->size)) {
      }
      #endif
         }
         static void
   llvmpipe_memobj_destroy(struct pipe_screen *pscreen,
         {
      if (!memobj)
            #ifdef PIPE_MEMORY_FD
         #endif
         }
         static struct pipe_resource *
   llvmpipe_resource_from_memobj(struct pipe_screen *pscreen,
                     {
      if (!memobj)
         struct llvmpipe_screen *screen = llvmpipe_screen(pscreen);
   struct llvmpipe_memory_object *lpmo = llvmpipe_memory_object(memobj);
   struct llvmpipe_resource *lpr = CALLOC_STRUCT(llvmpipe_resource);
            lpr->screen = screen;
   pipe_reference_init(&lpr->base.reference, 1);
            if (llvmpipe_resource_is_texture(&lpr->base)) {
      /* texture map */
   if (!llvmpipe_texture_layout(screen, lpr, false))
         if (lpmo->size < lpr->size_required)
            } else {
      /* other data (vertex buffer, const buffer, etc) */
   const uint bytes = templat->width0;
   assert(util_format_get_blocksize(templat->format) == 1);
   assert(templat->height0 == 1);
   assert(templat->depth0 == 1);
   assert(templat->last_level == 0);
   /*
   * Reserve some extra storage since if we'd render to a buffer we
   * read/write always LP_RASTER_BLOCK_SIZE pixels, but the element
   * offset doesn't need to be aligned to LP_RASTER_BLOCK_SIZE.
   */
   /*
   * buffers don't really have stride but it's probably safer
   * (for code doing same calculations for buffers and textures)
   * to put something reasonable in there.
   */
            lpr->size_required = bytes;
   if (!(templat->flags & PIPE_RESOURCE_FLAG_DONT_OVER_ALLOCATE))
            if (lpmo->size < lpr->size_required)
            }
   lpr->id = id_counter++;
         #ifdef DEBUG
      simple_mtx_lock(&resource_list_mutex);
   list_addtail(&lpr->list, &resource_list.list);
      #endif
               fail:
      free(lpr);
      }
      static void
   llvmpipe_resource_destroy(struct pipe_screen *pscreen,
         {
      struct llvmpipe_screen *screen = llvmpipe_screen(pscreen);
            if (!lpr->backable && !lpr->user_ptr) {
      if (lpr->dt) {
      /* display target */
   struct sw_winsys *winsys = screen->winsys;
      } else if (llvmpipe_resource_is_texture(pt)) {
      /* free linear image data */
   if (lpr->tex_data) {
      if (!lpr->imported_memory)
               } else if (lpr->data) {
      if (!lpr->imported_memory)
            #ifdef DEBUG
      simple_mtx_lock(&resource_list_mutex);
   if (!list_is_empty(&lpr->list))
            #endif
            }
         /**
   * Map a resource for read/write.
   */
   void *
   llvmpipe_resource_map(struct pipe_resource *resource,
                     {
      struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
            assert(level < LP_MAX_TEXTURE_LEVELS);
            assert(tex_usage == LP_TEX_USAGE_READ ||
                  if (lpr->dt) {
      /* display target */
   struct llvmpipe_screen *screen = lpr->screen;
   struct sw_winsys *winsys = screen->winsys;
            if (tex_usage == LP_TEX_USAGE_READ) {
         } else {
                  assert(level == 0);
            /* FIXME: keep map count? */
            /* install this linear image in texture data structure */
                           map = llvmpipe_get_texture_image_address(lpr, layer, level);
      } else {
            }
         /**
   * Unmap a resource.
   */
   void
   llvmpipe_resource_unmap(struct pipe_resource *resource,
               {
               if (lpr->dt) {
      /* display target */
   struct llvmpipe_screen *lp_screen = lpr->screen;
            assert(level == 0);
                  }
         void *
   llvmpipe_resource_data(struct pipe_resource *resource)
   {
                           }
         static struct pipe_resource *
   llvmpipe_resource_from_handle(struct pipe_screen *_screen,
                     {
      struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct sw_winsys *winsys = screen->winsys;
                     lpr = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpr) {
                  lpr->base = *template;
   lpr->screen = screen;
   pipe_reference_init(&lpr->base.reference, 1);
            /*
   * Looks like unaligned displaytargets work just fine,
   * at least sampler/render ones.
      #if 0
      assert(lpr->base.width0 == width);
      #endif
         lpr->dt = winsys->displaytarget_from_handle(winsys,
                     if (!lpr->dt) {
                        #ifdef DEBUG
      simple_mtx_lock(&resource_list_mutex);
   list_addtail(&lpr->list, &resource_list.list);
      #endif
               no_dt:
         no_lpr:
         }
         static bool
   llvmpipe_resource_get_handle(struct pipe_screen *screen,
                           {
      struct sw_winsys *winsys = llvmpipe_screen(screen)->winsys;
            assert(lpr->dt);
   if (!lpr->dt)
               }
         static struct pipe_resource *
   llvmpipe_resource_from_user_memory(struct pipe_screen *_screen,
               {
      struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
            lpr = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpr) {
                  lpr->base = *resource;
   lpr->screen = screen;
   pipe_reference_init(&lpr->base.reference, 1);
            if (llvmpipe_resource_is_texture(&lpr->base)) {
      if (!llvmpipe_texture_layout(screen, lpr, false))
               } else
            #ifdef DEBUG
      simple_mtx_lock(&resource_list_mutex);
   list_addtail(&lpr->list, &resource_list.list);
      #endif
         fail:
      FREE(lpr);
      }
         void *
   llvmpipe_transfer_map_ms(struct pipe_context *pipe,
                           struct pipe_resource *resource,
   unsigned level,
   {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
   struct llvmpipe_transfer *lpt;
   struct pipe_transfer *pt;
   uint8_t *map;
            assert(resource);
            /*
   * Transfers, like other pipe operations, must happen in order, so flush
   * the context if necessary.
   */
   if (!(usage & PIPE_MAP_UNSYNCHRONIZED)) {
      bool read_only = !(usage & PIPE_MAP_WRITE);
   bool do_not_block = !!(usage & PIPE_MAP_DONTBLOCK);
   if (!llvmpipe_flush_resource(pipe, resource,
                              level,
   /*
   * It would have blocked, but gallium frontend requested no to.
   */
   assert(do_not_block);
                  /* Check if we're mapping a current constant buffer */
   if ((usage & PIPE_MAP_WRITE) &&
      (resource->bind & PIPE_BIND_CONSTANT_BUFFER)) {
   unsigned i;
   for (i = 0; i < ARRAY_SIZE(llvmpipe->constants[PIPE_SHADER_FRAGMENT]); ++i) {
      if (resource == llvmpipe->constants[PIPE_SHADER_FRAGMENT][i].buffer) {
      /* constants may have changed */
   llvmpipe->dirty |= LP_NEW_FS_CONSTANTS;
                     lpt = CALLOC_STRUCT(llvmpipe_transfer);
   if (!lpt)
         pt = &lpt->base;
   pipe_resource_reference(&pt->resource, resource);
   pt->box = *box;
   pt->level = level;
   pt->stride = lpr->row_stride[level];
   pt->layer_stride = lpr->img_stride[level];
   pt->usage = usage;
                     /*
   printf("tex_transfer_map(%d, %d  %d x %d of %d x %d,  usage %d)\n",
         transfer->x, transfer->y, transfer->width, transfer->height,
   transfer->texture->width0,
   transfer->texture->height0,
            enum lp_texture_usage tex_usage;
   const char *mode;
   if (usage == PIPE_MAP_READ) {
      tex_usage = LP_TEX_USAGE_READ;
      } else {
      tex_usage = LP_TEX_USAGE_READ_WRITE;
               if (0) {
                                       /* May want to do different things here depending on read/write nature
   * of the map:
   */
   if (usage & PIPE_MAP_WRITE) {
      /* Do something to notify sharing contexts of a texture change.
   */
               map +=
      box->y / util_format_get_blockheight(format) * pt->stride +
         map += sample * lpr->sample_stride;
      }
         static void *
   llvmpipe_transfer_map(struct pipe_context *pipe,
                        struct pipe_resource *resource,
      {
      return llvmpipe_transfer_map_ms(pipe, resource, level, usage, 0,
      }
         static void
   llvmpipe_transfer_unmap(struct pipe_context *pipe,
         {
               llvmpipe_resource_unmap(transfer->resource,
                  /* Effectively do the texture_update work here - if texture images
   * needed post-processing to put them into hardware layout, this is
   * where it would happen.  For llvmpipe, nothing to do.
   */
   assert (transfer->resource);
   pipe_resource_reference(&transfer->resource, NULL);
      }
         unsigned int
   llvmpipe_is_resource_referenced(struct pipe_context *pipe,
               {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   if (!(presource->bind & (PIPE_BIND_DEPTH_STENCIL |
                           PIPE_BIND_RENDER_TARGET |
               }
         /**
   * Returns the largest possible alignment for a format in llvmpipe
   */
   unsigned
   llvmpipe_get_format_alignment(enum pipe_format format)
   {
      const struct util_format_description *desc = util_format_description(format);
            for (unsigned i = 0; i < desc->nr_channels; ++i) {
                           if (!util_is_power_of_two_or_zero(bytes)) {
                  if (bytes % 2 || bytes < 1) {
         } else {
            }
         /**
   * Create buffer which wraps user-space data.
   * XXX unreachable.
   */
   struct pipe_resource *
   llvmpipe_user_buffer_create(struct pipe_screen *screen,
                     {
               buffer = CALLOC_STRUCT(llvmpipe_resource);
   if (!buffer)
            buffer->screen = llvmpipe_screen(screen);
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.screen = screen;
   buffer->base.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   buffer->base.bind = bind_flags;
   buffer->base.usage = PIPE_USAGE_IMMUTABLE;
   buffer->base.flags = 0;
   buffer->base.width0 = bytes;
   buffer->base.height0 = 1;
   buffer->base.depth0 = 1;
   buffer->base.array_size = 1;
   buffer->user_ptr = true;
               }
         /**
   * Compute size (in bytes) need to store a texture image / mipmap level,
   * for just one cube face, one array layer or one 3D texture slice
   */
   static unsigned
   tex_image_face_size(const struct llvmpipe_resource *lpr, unsigned level)
   {
         }
         /**
   * Return pointer to a 2D texture image/face/slice.
   * No tiled/linear conversion is done.
   */
   uint8_t *
   llvmpipe_get_texture_image_address(struct llvmpipe_resource *lpr,
         {
                        if (face_slice > 0)
               }
         /**
   * Return size of resource in bytes
   */
   unsigned
   llvmpipe_resource_size(const struct pipe_resource *resource)
   {
      const struct llvmpipe_resource *lpr = llvmpipe_resource_const(resource);
            if (llvmpipe_resource_is_texture(resource)) {
      /* Note this will always return 0 for displaytarget resources */
      } else {
         }
      }
         static void
   llvmpipe_memory_barrier(struct pipe_context *pipe,
         {
      /* this may be an overly large hammer for this nut. */
      }
         static struct pipe_memory_allocation *
   llvmpipe_allocate_memory(struct pipe_screen *screen, uint64_t size)
   {
      uint64_t alignment;
   if (!os_get_page_size(&alignment))
            }
         static void
   llvmpipe_free_memory(struct pipe_screen *screen,
         {
         }
         #ifdef PIPE_MEMORY_FD
      static const char *driver_id = "llvmpipe" MESA_GIT_SHA1;
         static struct pipe_memory_allocation *
   llvmpipe_allocate_memory_fd(struct pipe_screen *screen,
               {
      uint64_t alignment;
   if (!os_get_page_size(&alignment))
         return os_malloc_aligned_fd(size, alignment, fd,
      }
         static bool
   llvmpipe_import_memory_fd(struct pipe_screen *screen,
                     {
         }
         static void
   llvmpipe_free_memory_fd(struct pipe_screen *screen,
         {
         }
      #endif
         static bool
   llvmpipe_resource_bind_backing(struct pipe_screen *screen,
                     {
               if (!lpr->backable)
            if (llvmpipe_resource_is_texture(&lpr->base)) {
      if (lpr->size_required > LP_MAX_TEXTURE_SIZE)
               } else
                     }
         static void *
   llvmpipe_map_memory(struct pipe_screen *screen,
         {
         }
         static void
   llvmpipe_unmap_memory(struct pipe_screen *screen,
         {
   }
         #ifdef DEBUG
   void
   llvmpipe_print_resources(void)
   {
      struct llvmpipe_resource *lpr;
            debug_printf("LLVMPIPE: current resources:\n");
   simple_mtx_lock(&resource_list_mutex);
   LIST_FOR_EACH_ENTRY(lpr, &resource_list.list, list) {
      unsigned size = llvmpipe_resource_size(&lpr->base);
   debug_printf("resource %u at %p, size %ux%ux%u: %u bytes, refcount %u\n",
               lpr->id, (void *) lpr,
   total += size;
      }
   simple_mtx_unlock(&resource_list_mutex);
      }
   #endif
         static void
   llvmpipe_get_resource_info(struct pipe_screen *screen,
                     {
               *stride = lpr->row_stride[0];
      }
         static bool
   llvmpipe_resource_get_param(struct pipe_screen *screen,
                              struct pipe_context *context,
   struct pipe_resource *resource,
   unsigned plane,
      {
      struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
            switch (param) {
   case PIPE_RESOURCE_PARAM_NPLANES:
      *value = 1;
      case PIPE_RESOURCE_PARAM_STRIDE:
      *value = lpr->row_stride[level];
      case PIPE_RESOURCE_PARAM_OFFSET:
      *value = lpr->mip_offsets[level] + (lpr->img_stride[level] * layer);
      case PIPE_RESOURCE_PARAM_LAYER_STRIDE:
      *value = lpr->img_stride[level];
   #ifndef _WIN32
      case PIPE_RESOURCE_PARAM_MODIFIER:
      *value = DRM_FORMAT_MOD_INVALID;
   #endif
      case PIPE_RESOURCE_PARAM_HANDLE_TYPE_SHARED:
   case PIPE_RESOURCE_PARAM_HANDLE_TYPE_KMS:
   case PIPE_RESOURCE_PARAM_HANDLE_TYPE_FD:
      if (!lpr->dt)
            memset(&whandle, 0, sizeof(whandle));
   if (param == PIPE_RESOURCE_PARAM_HANDLE_TYPE_SHARED)
         else if (param == PIPE_RESOURCE_PARAM_HANDLE_TYPE_KMS)
         else if (param == PIPE_RESOURCE_PARAM_HANDLE_TYPE_FD)
            if (!llvmpipe_resource_get_handle(screen, context, resource,
               }
   *value = (uint64_t)(uintptr_t)whandle.handle;
      default:
         }
   assert(0);
   *value = 0;
      }
         void
   llvmpipe_init_screen_resource_funcs(struct pipe_screen *screen)
   {
   #ifdef DEBUG
      /* init linked list for tracking resources */
   {
      static bool first_call = true;
   if (first_call) {
      memset(&resource_list, 0, sizeof(resource_list));
   list_inithead(&resource_list.list);
            #endif
            /*   screen->resource_create_front = llvmpipe_resource_create_front; */
      screen->resource_destroy = llvmpipe_resource_destroy;
   screen->resource_from_handle = llvmpipe_resource_from_handle;
   screen->resource_from_memobj = llvmpipe_resource_from_memobj;
   screen->resource_get_handle = llvmpipe_resource_get_handle;
                     screen->memobj_create_from_handle = llvmpipe_memobj_create_from_handle;
            screen->resource_get_info = llvmpipe_get_resource_info;
   screen->resource_get_param = llvmpipe_resource_get_param;
   screen->resource_from_user_memory = llvmpipe_resource_from_user_memory;
   screen->allocate_memory = llvmpipe_allocate_memory;
      #ifdef PIPE_MEMORY_FD
      screen->allocate_memory_fd = llvmpipe_allocate_memory_fd;
   screen->import_memory_fd = llvmpipe_import_memory_fd;
      #endif
      screen->map_memory = llvmpipe_map_memory;
               }
         void
   llvmpipe_init_context_resource_funcs(struct pipe_context *pipe)
   {
      pipe->buffer_map = llvmpipe_transfer_map;
   pipe->buffer_unmap = llvmpipe_transfer_unmap;
   pipe->texture_map = llvmpipe_transfer_map;
            pipe->transfer_flush_region = u_default_transfer_flush_region;
   pipe->buffer_subdata = u_default_buffer_subdata;
               }
