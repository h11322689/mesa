      #include "i915_drm_winsys.h"
   #include "util/u_memory.h"
   #include "util/u_atomic.h"
   #include "util/u_inlines.h"
      /**
   * Because gem does not have fence's we have to create our own fences.
   *
   * They work by keeping the batchbuffer around and checking if that has
   * been idled. If bo is NULL fence has expired.
   */
   struct i915_drm_fence
   {
      struct pipe_reference reference;
      };
         struct pipe_fence_handle *
   i915_drm_fence_create(drm_intel_bo *bo)
   {
               pipe_reference_init(&fence->reference, 1);
   /* bo is null if fence already expired */
   if (bo) {
      drm_intel_bo_reference(bo);
                  }
      static void
   i915_drm_fence_reference(struct i915_winsys *iws,
               {
      struct i915_drm_fence *old = (struct i915_drm_fence *)*ptr;
            if (pipe_reference(&((struct i915_drm_fence *)(*ptr))->reference, &f->reference)) {
      if (old->bo)
            }
      }
      static int
   i915_drm_fence_signalled(struct i915_winsys *iws,
         {
               /* fence already expired */
   if (!f->bo)
               }
      static int
   i915_drm_fence_finish(struct i915_winsys *iws,
         {
               /* fence already expired */
   if (!f->bo)
            drm_intel_bo_wait_rendering(f->bo);
   drm_intel_bo_unreference(f->bo);
               }
      void
   i915_drm_winsys_init_fence_functions(struct i915_drm_winsys *idws)
   {
      idws->base.fence_reference = i915_drm_fence_reference;
   idws->base.fence_signalled = i915_drm_fence_signalled;
      }
