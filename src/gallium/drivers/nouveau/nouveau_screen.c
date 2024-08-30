   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
      #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/format/u_format_s3tc.h"
   #include "util/u_string.h"
   #include "util/hex.h"
      #include "util/os_mman.h"
   #include "util/os_time.h"
      #include <stdio.h>
   #include <errno.h>
   #include <stdlib.h>
      #include <nouveau_drm.h>
   #include <xf86drm.h>
   #include <nvif/class.h>
   #include <nvif/cl0080.h>
      #include "nouveau_winsys.h"
   #include "nouveau_screen.h"
   #include "nouveau_context.h"
   #include "nouveau_fence.h"
   #include "nouveau_mm.h"
   #include "nouveau_buffer.h"
      #include <compiler/glsl_types.h>
      /* XXX this should go away */
   #include "frontend/drm_driver.h"
      /* Even though GPUs might allow addresses with more bits, some engines do not.
   * Stick with 40 for compatibility.
   */
   #define NV_GENERIC_VM_LIMIT_SHIFT 39
      int nouveau_mesa_debug = 0;
      static const char *
   nouveau_screen_get_name(struct pipe_screen *pscreen)
   {
      struct nouveau_screen *screen = nouveau_screen(pscreen);
      }
      static const char *
   nouveau_screen_get_vendor(struct pipe_screen *pscreen)
   {
         }
      static const char *
   nouveau_screen_get_device_vendor(struct pipe_screen *pscreen)
   {
         }
      static uint64_t
   nouveau_screen_get_timestamp(struct pipe_screen *pscreen)
   {
                           }
      static struct disk_cache *
   nouveau_screen_get_disk_shader_cache(struct pipe_screen *pscreen)
   {
         }
      static void
   nouveau_screen_fence_ref(struct pipe_screen *pscreen,
               {
         }
      static bool
   nouveau_screen_fence_finish(struct pipe_screen *screen,
                     {
      if (!timeout)
               }
         struct nouveau_bo *
   nouveau_screen_bo_from_handle(struct pipe_screen *pscreen,
               {
      struct nouveau_device *dev = nouveau_screen(pscreen)->device;
   struct nouveau_bo *bo = NULL;
            if (whandle->offset != 0) {
      debug_printf("%s: attempt to import unsupported winsys offset %d\n",
                     if (whandle->type != WINSYS_HANDLE_TYPE_SHARED &&
      whandle->type != WINSYS_HANDLE_TYPE_FD) {
   debug_printf("%s: attempt to import unsupported handle type %d\n",
                     if (whandle->type == WINSYS_HANDLE_TYPE_SHARED)
         else
            if (ret) {
      debug_printf("%s: ref name 0x%08x failed with %d\n",
                     *out_stride = whandle->stride;
      }
         bool
   nouveau_screen_bo_get_handle(struct pipe_screen *pscreen,
                     {
               if (whandle->type == WINSYS_HANDLE_TYPE_SHARED) {
         } else if (whandle->type == WINSYS_HANDLE_TYPE_KMS) {
      int fd;
            /* The handle is exported in this case, but the global list of
   * handles is in libdrm and there is no libdrm API to add
   * handles to the list without additional side effects. The
   * closest API available also gets a fd for the handle, which
   * is not necessary in this case. Call it and close the fd.
   */
   ret = nouveau_bo_set_prime(bo, &fd);
   if (ret != 0)
                     whandle->handle = bo->handle;
      } else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
         } else {
            }
      static void
   nouveau_disk_cache_create(struct nouveau_screen *screen)
   {
      struct mesa_sha1 ctx;
   unsigned char sha1[20];
   char cache_id[20 * 2 + 1];
            _mesa_sha1_init(&ctx);
   if (!disk_cache_get_function_identifier(nouveau_disk_cache_create,
                  _mesa_sha1_final(&ctx, sha1);
                     screen->disk_shader_cache =
      disk_cache_create(nouveau_screen_get_name(&screen->base),
   }
      static void*
   reserve_vma(uintptr_t start, uint64_t reserved_size)
   {
      void *reserved = os_mmap((void*)start, reserved_size, PROT_NONE,
         if (reserved == MAP_FAILED)
            }
      static void
   nouveau_query_memory_info(struct pipe_screen *pscreen,
         {
      const struct nouveau_screen *screen = nouveau_screen(pscreen);
            info->total_device_memory = dev->vram_size / 1024;
            info->avail_device_memory = dev->vram_limit / 1024;
      }
      static void
   nouveau_pushbuf_cb(struct nouveau_pushbuf *push)
   {
               if (p->context)
         else
               }
      int
   nouveau_pushbuf_create(struct nouveau_screen *screen, struct nouveau_context *context,
               {
      int ret;
   ret = nouveau_pushbuf_new(client, chan, nr, size, immediate, push);
   if (ret)
            struct nouveau_pushbuf_priv *p = MALLOC_STRUCT(nouveau_pushbuf_priv);
   if (!p) {
      nouveau_pushbuf_del(push);
      }
   p->screen = screen;
   p->context = context;
   (*push)->kick_notify = nouveau_pushbuf_cb;
   (*push)->user_priv = p;
      }
      void
   nouveau_pushbuf_destroy(struct nouveau_pushbuf **push)
   {
      if (!*push)
         FREE((*push)->user_priv);
      }
      static bool
   nouveau_check_for_uma(int chipset, struct nouveau_object *obj)
   {
      struct nv_device_info_v0 info = {
                              }
      static int
   nouveau_screen_get_fd(struct pipe_screen *pscreen)
   {
                  }
      int
   nouveau_screen_init(struct nouveau_screen *screen, struct nouveau_device *dev)
   {
      struct pipe_screen *pscreen = &screen->base;
   struct nv04_fifo nv04_data = { .vram = 0xbeef0201, .gart = 0xbeef0202 };
   struct nvc0_fifo nvc0_data = { };
   uint64_t time;
   int size, ret;
   void *data;
            char *nv_dbg = getenv("NOUVEAU_MESA_DEBUG");
   if (nv_dbg)
            screen->force_enable_cl = debug_get_bool_option("NOUVEAU_ENABLE_CL", false);
            /* These must be set before any failure is possible, as the cleanup
   * paths assume they're responsible for deleting them.
   */
   screen->drm = nouveau_drm(&dev->object);
            /*
   * this is initialized to 1 in nouveau_drm_screen_create after screen
   * is fully constructed and added to the global screen list.
   */
            if (dev->chipset < 0xc0) {
      data = &nv04_data;
      } else {
      data = &nvc0_data;
               bool enable_svm = debug_get_bool_option("NOUVEAU_SVM", false);
   screen->has_svm = false;
   /* we only care about HMM with OpenCL enabled */
   if (dev->chipset > 0x130 && enable_svm) {
      /* Before being able to enable SVM we need to carve out some memory for
   * driver bo allocations. Let's just base the size on the available VRAM.
   *
   * 40 bit is the biggest we care about and for 32 bit systems we don't
   * want to allocate all of the available memory either.
   *
   * Also we align the size we want to reserve to the next POT to make use
   * of hugepages.
   */
   const int vram_shift = util_logbase2_ceil64(dev->vram_size);
   const int limit_bit =
         screen->svm_cutout_size =
            size_t start = screen->svm_cutout_size;
   do {
      screen->svm_cutout = reserve_vma(start, screen->svm_cutout_size);
   if (!screen->svm_cutout) {
      start += screen->svm_cutout_size;
               struct drm_nouveau_svm_init svm_args = {
      .unmanaged_addr = (uintptr_t)screen->svm_cutout,
               ret = drmCommandWrite(screen->drm->fd, DRM_NOUVEAU_SVM_INIT,
         screen->has_svm = !ret;
   if (!screen->has_svm)
                        switch (dev->chipset) {
   case 0x0ea: /* TK1, GK20A */
   case 0x12b: /* TX1, GM20B */
   case 0x13b: /* TX2, GP10B */
      screen->tegra_sector_layout = true;
      default:
      /* Xavier's GPU and everything else */
   screen->tegra_sector_layout = false;
               /*
   * Set default VRAM domain if not overridden
   */
   if (!screen->vram_domain) {
      if (dev->vram_size > 0)
         else
               ret = nouveau_object_new(&dev->object, 0, NOUVEAU_FIFO_CHANNEL_CLASS,
         if (ret)
            ret = nouveau_client_new(screen->device, &screen->client);
   if (ret)
         ret = nouveau_pushbuf_create(screen, NULL, screen->client, screen->channel,
               if (ret)
            /* getting CPU time first appears to be more accurate */
            ret = nouveau_getparam(dev, NOUVEAU_GETPARAM_PTIMER_TIME, &time);
   if (!ret)
            snprintf(screen->chipset_name, sizeof(screen->chipset_name), "NV%02X", dev->chipset);
   pscreen->get_name = nouveau_screen_get_name;
   pscreen->get_screen_fd = nouveau_screen_get_fd;
   pscreen->get_vendor = nouveau_screen_get_vendor;
   pscreen->get_device_vendor = nouveau_screen_get_device_vendor;
                     pscreen->fence_reference = nouveau_screen_fence_ref;
                              screen->transfer_pushbuf_threshold = 192;
   screen->lowmem_bindings = PIPE_BIND_GLOBAL; /* gallium limit */
   screen->vidmem_bindings =
      PIPE_BIND_RENDER_TARGET | PIPE_BIND_DEPTH_STENCIL |
   PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT |
   PIPE_BIND_CURSOR |
   PIPE_BIND_SAMPLER_VIEW |
   PIPE_BIND_SHADER_BUFFER | PIPE_BIND_SHADER_IMAGE |
   PIPE_BIND_COMPUTE_RESOURCE |
      screen->sysmem_bindings =
      PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_STREAM_OUTPUT |
                  memset(&mm_config, 0, sizeof(mm_config));
            screen->mm_GART = nouveau_mm_create(dev,
                                       err:
      if (screen->svm_cutout)
            }
      void
   nouveau_screen_fini(struct nouveau_screen *screen)
   {
               glsl_type_singleton_decref();
   if (screen->has_svm)
            nouveau_mm_destroy(screen->mm_GART);
                     nouveau_client_del(&screen->client);
            nouveau_device_del(&screen->device);
   nouveau_drm_del(&screen->drm);
            disk_cache_destroy(screen->disk_shader_cache);
      }
      static void
   nouveau_set_debug_callback(struct pipe_context *pipe,
         {
               if (cb)
         else
      }
      int
   nouveau_context_init(struct nouveau_context *context, struct nouveau_screen *screen)
   {
               context->pipe.set_debug_callback = nouveau_set_debug_callback;
            ret = nouveau_client_new(screen->device, &context->client);
   if (ret)
            ret = nouveau_pushbuf_create(screen, context, context->client, screen->channel,
               if (ret)
               }
