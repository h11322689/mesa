      #include "i915_drm_winsys.h"
   #include "util/u_memory.h"
      #include "drm-uapi/i915_drm.h"
   #include "i915/i915_debug.h"
   #include <xf86drm.h>
   #include <stdio.h>
      #define BATCH_RESERVED 16
      #define INTEL_DEFAULT_RELOCS 100
   #define INTEL_MAX_RELOCS 400
      #define INTEL_BATCH_NO_CLIPRECTS 0x1
   #define INTEL_BATCH_CLIPRECTS    0x2
      #undef INTEL_RUN_SYNC
      struct i915_drm_batchbuffer
   {
                           };
      static inline struct i915_drm_batchbuffer *
   i915_drm_batchbuffer(struct i915_winsys_batchbuffer *batch)
   {
         }
      static void
   i915_drm_batchbuffer_reset(struct i915_drm_batchbuffer *batch)
   {
               if (batch->bo)
         batch->bo = drm_intel_bo_alloc(idws->gem_manager,
                        memset(batch->base.map, 0, batch->actual_size);
   batch->base.ptr = batch->base.map;
   batch->base.size = batch->actual_size - BATCH_RESERVED;
      }
      static struct i915_winsys_batchbuffer *
   i915_drm_batchbuffer_create(struct i915_winsys *iws)
   {
      struct i915_drm_winsys *idws = i915_drm_winsys(iws);
                     batch->base.map = MALLOC(batch->actual_size);
   batch->base.ptr = NULL;
                                          }
      static bool
   i915_drm_batchbuffer_validate_buffers(struct i915_winsys_batchbuffer *batch,
               {
      struct i915_drm_batchbuffer *drm_batch = i915_drm_batchbuffer(batch);
   drm_intel_bo *bos[num_of_buffers + 1];
            bos[0] = drm_batch->bo;
   for (i = 0; i < num_of_buffers; i++)
            ret = drm_intel_bufmgr_check_aperture_space(bos, num_of_buffers);
   if (ret != 0)
               }
      static int
   i915_drm_batchbuffer_reloc(struct i915_winsys_batchbuffer *ibatch,
                     {
      struct i915_drm_batchbuffer *batch = i915_drm_batchbuffer(ibatch);
   unsigned write_domain = 0;
   unsigned read_domain = 0;
   unsigned offset;
            switch (usage) {
   case I915_USAGE_SAMPLER:
      write_domain = 0;
   read_domain = I915_GEM_DOMAIN_SAMPLER;
      case I915_USAGE_RENDER:
      write_domain = I915_GEM_DOMAIN_RENDER;
   read_domain = I915_GEM_DOMAIN_RENDER;
      case I915_USAGE_2D_TARGET:
      write_domain = I915_GEM_DOMAIN_RENDER;
   read_domain = I915_GEM_DOMAIN_RENDER;
      case I915_USAGE_2D_SOURCE:
      write_domain = 0;
   read_domain = I915_GEM_DOMAIN_RENDER;
      case I915_USAGE_VERTEX:
      write_domain = 0;
   read_domain = I915_GEM_DOMAIN_VERTEX;
      default:
      assert(0);
                        if (fenced)
      ret = drm_intel_bo_emit_reloc_fence(batch->bo, offset,
   intel_bo(buffer), pre_add,
   read_domain,
      else
      ret = drm_intel_bo_emit_reloc(batch->bo, offset,
   intel_bo(buffer), pre_add,
   read_domain,
         ((uint32_t*)batch->base.ptr)[0] = intel_bo(buffer)->offset + pre_add;
            if (!ret)
               }
      static void
   i915_drm_throttle(struct i915_drm_winsys *idws)
   {
         }
      static void
   i915_drm_batchbuffer_flush(struct i915_winsys_batchbuffer *ibatch,
               {
      struct i915_drm_batchbuffer *batch = i915_drm_batchbuffer(ibatch);
   unsigned used;
            /* MI_BATCH_BUFFER_END */
            used = batch->base.ptr - batch->base.map;
   if (used & 4) {
      /* MI_NOOP */
   i915_winsys_batchbuffer_dword_unchecked(ibatch, 0);
               /* Do the sending to HW */
   ret = drm_intel_bo_subdata(batch->bo, 0, used, batch->base.map);
   if (ret == 0 && i915_drm_winsys(ibatch->iws)->send_cmd)
            if (flags & I915_FLUSH_END_OF_FRAME)
            if (ret != 0 || i915_drm_winsys(ibatch->iws)->dump_cmd) {
      i915_dump_batchbuffer(ibatch);
               if (i915_drm_winsys(ibatch->iws)->dump_raw_file) {
      FILE *file = fopen(i915_drm_winsys(ibatch->iws)->dump_raw_file, "a");
   fwrite(batch->base.map, used, 1, file);
   fclose(file);
                  #ifdef INTEL_RUN_SYNC
         #endif
         if (fence) {
         #ifdef INTEL_RUN_SYNC
         /* we run synced to GPU so just pass null */
   #else
         #endif
                  }
      static void
   i915_drm_batchbuffer_destroy(struct i915_winsys_batchbuffer *ibatch)
   {
               if (batch->bo)
            FREE(batch->base.map);
      }
      void i915_drm_winsys_init_batchbuffer_functions(struct i915_drm_winsys *idws)
   {
      idws->base.batchbuffer_create = i915_drm_batchbuffer_create;
   idws->base.validate_buffers = i915_drm_batchbuffer_validate_buffers;
   idws->base.batchbuffer_reloc = i915_drm_batchbuffer_reloc;
   idws->base.batchbuffer_flush = i915_drm_batchbuffer_flush;
      }
