   #include "frontend/drm_driver.h"
   #include "i915_drm_winsys.h"
   #include "util/u_memory.h"
      #include "drm-uapi/i915_drm.h"
      static char *i915_drm_type_to_name(enum i915_winsys_buffer_type type)
   {
               if (type == I915_NEW_TEXTURE) {
         } else if (type == I915_NEW_VERTEX) {
         } else if (type == I915_NEW_SCANOUT) {
         } else {
      assert(0);
                  }
      static struct i915_winsys_buffer *
   i915_drm_buffer_create(struct i915_winsys *iws,
               {
      struct i915_drm_buffer *buf = CALLOC_STRUCT(i915_drm_buffer);
            if (!buf)
            buf->magic = 0xDEAD1337;
   buf->flinked = false;
            buf->bo = drm_intel_bo_alloc(idws->gem_manager,
            if (!buf->bo)
                  err:
      assert(0);
   FREE(buf);
      }
      static struct i915_winsys_buffer *
   i915_drm_buffer_create_tiled(struct i915_winsys *iws,
                     {
      struct i915_drm_buffer *buf = CALLOC_STRUCT(i915_drm_buffer);
   struct i915_drm_winsys *idws = i915_drm_winsys(iws);
   unsigned long pitch = 0;
            if (!buf)
            buf->magic = 0xDEAD1337;
   buf->flinked = false;
            buf->bo = drm_intel_bo_alloc_tiled(idws->gem_manager,
                        if (!buf->bo)
            *stride = pitch;
   *tiling = tiling_mode;
         err:
      assert(0);
   FREE(buf);
      }
      static struct i915_winsys_buffer *
   i915_drm_buffer_from_handle(struct i915_winsys *iws,
                           {
      struct i915_drm_winsys *idws = i915_drm_winsys(iws);
   struct i915_drm_buffer *buf;
            if ((whandle->type != WINSYS_HANDLE_TYPE_SHARED) && (whandle->type != WINSYS_HANDLE_TYPE_FD))
            if (whandle->offset != 0)
            buf = CALLOC_STRUCT(i915_drm_buffer);
   if (!buf)
                     if (whandle->type == WINSYS_HANDLE_TYPE_SHARED)
         else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
      int fd = (int) whandle->handle;
               buf->flinked = true;
            if (!buf->bo)
                     *stride = whandle->stride;
                  err:
      FREE(buf);
      }
      static bool
   i915_drm_buffer_get_handle(struct i915_winsys *iws,
                     {
               if (whandle->type == WINSYS_HANDLE_TYPE_SHARED) {
      if (!buf->flinked) {
      if (drm_intel_bo_flink(buf->bo, &buf->flink))
                        } else if (whandle->type == WINSYS_HANDLE_TYPE_KMS) {
         } else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
               if (drm_intel_bo_gem_export_to_prime(buf->bo, &fd))
            } else {
      assert(!"unknown usage");
               whandle->stride = stride;
      }
      static void *
   i915_drm_buffer_map(struct i915_winsys *iws,
               {
      struct i915_drm_buffer *buf = i915_drm_buffer(buffer);
   drm_intel_bo *bo = intel_bo(buffer);
                     if (buf->map_count)
                                 out:
      if (ret)
            buf->map_count++;
      }
      static void
   i915_drm_buffer_unmap(struct i915_winsys *iws,
         {
               if (--buf->map_count)
               }
      static int
   i915_drm_buffer_write(struct i915_winsys *iws,
                           {
                  }
      static void
   i915_drm_buffer_destroy(struct i915_winsys *iws,
         {
            #ifdef DEBUG
      i915_drm_buffer(buffer)->magic = 0;
      #endif
            }
      static bool
   i915_drm_buffer_is_busy(struct i915_winsys *iws,
         {
      struct i915_drm_buffer* i915_buffer = i915_drm_buffer(buffer);
   if (!i915_buffer)
            }
         void
   i915_drm_winsys_init_buffer_functions(struct i915_drm_winsys *idws)
   {
      idws->base.buffer_create = i915_drm_buffer_create;
   idws->base.buffer_create_tiled = i915_drm_buffer_create_tiled;
   idws->base.buffer_from_handle = i915_drm_buffer_from_handle;
   idws->base.buffer_get_handle = i915_drm_buffer_get_handle;
   idws->base.buffer_map = i915_drm_buffer_map;
   idws->base.buffer_unmap = i915_drm_buffer_unmap;
   idws->base.buffer_write = i915_drm_buffer_write;
   idws->base.buffer_destroy = i915_drm_buffer_destroy;
      }
