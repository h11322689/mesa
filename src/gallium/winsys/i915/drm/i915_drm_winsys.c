   #include <stdio.h>
   #include <sys/ioctl.h>
      #include "drm-uapi/i915_drm.h"
      #include "frontend/drm_driver.h"
      #include "i915_drm_winsys.h"
   #include "i915_drm_public.h"
   #include "util/u_memory.h"
      #include "intel/common/intel_gem.h"
      /*
   * Helper functions
   */
         static void
   i915_drm_get_device_id(int fd, unsigned int *device_id)
   {
      ASSERTED bool ret = intel_gem_get_param(fd, I915_PARAM_CHIPSET_ID, (int *)device_id);
      }
      static int
   i915_drm_aperture_size(struct i915_winsys *iws)
   {
      struct i915_drm_winsys *idws = i915_drm_winsys(iws);
                        }
      static void
   i915_drm_winsys_destroy(struct i915_winsys *iws)
   {
                           }
      static int
   i915_drm_winsys_get_fd(struct i915_winsys *iws)
   {
                  }
      struct i915_winsys *
   i915_drm_winsys_create(int drmFD)
   {
      struct i915_drm_winsys *idws;
            idws = CALLOC_STRUCT(i915_drm_winsys);
   if (!idws)
                     i915_drm_winsys_init_batchbuffer_functions(idws);
   i915_drm_winsys_init_buffer_functions(idws);
            idws->fd = drmFD;
   idws->base.pci_id = deviceID;
            idws->base.aperture_size = i915_drm_aperture_size;
   idws->base.destroy = i915_drm_winsys_destroy;
            idws->gem_manager = drm_intel_bufmgr_gem_init(idws->fd, idws->max_batch_size);
   drm_intel_bufmgr_gem_enable_reuse(idws->gem_manager);
            idws->dump_cmd = debug_get_bool_option("I915_DUMP_CMD", false);
   idws->dump_raw_file = debug_get_option("I915_DUMP_RAW_FILE", NULL);
               }
