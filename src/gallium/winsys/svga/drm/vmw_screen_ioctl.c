   /**********************************************************
   * Copyright 2009-2015 VMware, Inc.  All rights reserved.
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
      /**
   * @file
   *
   * Wrappers for DRM ioctl functionlaity used by the rest of the vmw
   * drm winsys.
   *
   * Based on svgaicd_escape.c
   */
         #include "svga_cmd.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "svgadump/svga_dump.h"
   #include "frontend/drm_driver.h"
   #include "vmw_screen.h"
   #include "vmw_context.h"
   #include "vmw_fence.h"
   #include "xf86drm.h"
   #include "vmwgfx_drm.h"
   #include "svga3d_caps.h"
   #include "svga3d_reg.h"
      #include "util/os_mman.h"
      #include <errno.h>
   #include <unistd.h>
      #define VMW_MAX_DEFAULT_TEXTURE_SIZE   (128 * 1024 * 1024)
   #define VMW_FENCE_TIMEOUT_SECONDS 3600UL
      #define SVGA3D_FLAGS_64(upper32, lower32) (((uint64_t)upper32 << 32) | lower32)
   #define SVGA3D_FLAGS_UPPER_32(svga3d_flags) (svga3d_flags >> 32)
   #define SVGA3D_FLAGS_LOWER_32(svga3d_flags) \
            struct vmw_region
   {
      uint32_t handle;
   uint64_t map_handle;
   void *data;
   uint32_t map_count;
   int drm_fd;
      };
      uint32_t
   vmw_region_size(struct vmw_region *region)
   {
         }
      #if defined(__DragonFly__) || defined(__FreeBSD__) || \
         #define ERESTART EINTR
   #endif
      uint32
   vmw_ioctl_context_create(struct vmw_winsys_screen *vws)
   {
      struct drm_vmw_context_arg c_arg;
                     ret = drmCommandRead(vws->ioctl.drm_fd, DRM_VMW_CREATE_CONTEXT,
            if (ret)
            vmw_printf("Context id is %d\n", c_arg.cid);
      }
      uint32
   vmw_ioctl_extended_context_create(struct vmw_winsys_screen *vws,
         {
      union drm_vmw_extended_context_arg c_arg;
            VMW_FUNC;
   memset(&c_arg, 0, sizeof(c_arg));
   c_arg.req = (vgpu10 ? drm_vmw_context_dx : drm_vmw_context_legacy);
   ret = drmCommandWriteRead(vws->ioctl.drm_fd,
                  if (ret)
            vmw_printf("Context id is %d\n", c_arg.cid);
      }
      void
   vmw_ioctl_context_destroy(struct vmw_winsys_screen *vws, uint32 cid)
   {
                        memset(&c_arg, 0, sizeof(c_arg));
            (void)drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_UNREF_CONTEXT,
         }
      uint32
   vmw_ioctl_surface_create(struct vmw_winsys_screen *vws,
                           SVGA3dSurface1Flags flags,
   SVGA3dSurfaceFormat format,
   {
      union drm_vmw_surface_create_arg s_arg;
   struct drm_vmw_surface_create_req *req = &s_arg.req;
   struct drm_vmw_surface_arg *rep = &s_arg.rep;
   struct drm_vmw_size sizes[DRM_VMW_MAX_SURFACE_FACES*
         struct drm_vmw_size *cur_size;
   uint32_t iFace;
   uint32_t iMipLevel;
                     memset(&s_arg, 0, sizeof(s_arg));
   req->flags = (uint32_t) flags;
   req->scanout = !!(usage & SVGA_SURFACE_USAGE_SCANOUT);
   req->format = (uint32_t) format;
            assert(numFaces * numMipLevels < DRM_VMW_MAX_SURFACE_FACES*
   DRM_VMW_MAX_MIP_LEVELS);
   cur_size = sizes;
   for (iFace = 0; iFace < numFaces; ++iFace) {
               req->mip_levels[iFace] = numMipLevels;
   cur_size->width = mipSize.width;
   cur_size->height = mipSize.height;
   cur_size->depth = mipSize.depth;
   mipSize.width = MAX2(mipSize.width >> 1, 1);
   mipSize.height = MAX2(mipSize.height >> 1, 1);
   mipSize.depth = MAX2(mipSize.depth >> 1, 1);
   cur_size++;
            }
   for (iFace = numFaces; iFace < SVGA3D_MAX_SURFACE_FACES; ++iFace) {
                           ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_CREATE_SURFACE,
            if (ret)
                        }
         uint32
   vmw_ioctl_gb_surface_create(struct vmw_winsys_screen *vws,
                              SVGA3dSurfaceAllFlags flags,
   SVGA3dSurfaceFormat format,
   unsigned usage,
   SVGA3dSize size,
   uint32_t numFaces,
   uint32_t numMipLevels,
      {
      union {
      union drm_vmw_gb_surface_create_ext_arg ext_arg;
      } s_arg;
   struct drm_vmw_gb_surface_create_rep *rep;
   struct vmw_region *region = NULL;
                     if (p_region) {
      region = CALLOC_STRUCT(vmw_region);
   if (!region)
                        if (vws->ioctl.have_drm_2_15) {
      struct drm_vmw_gb_surface_create_ext_req *req = &s_arg.ext_arg.req;
            req->version = drm_vmw_gb_surface_v1;
   req->multisample_pattern = multisamplePattern;
   req->quality_level = qualityLevel;
   req->buffer_byte_stride = 0;
   req->must_be_zero = 0;
   req->base.svga3d_flags = SVGA3D_FLAGS_LOWER_32(flags);
   req->svga3d_flags_upper_32_bits = SVGA3D_FLAGS_UPPER_32(flags);
            if (usage & SVGA_SURFACE_USAGE_SCANOUT)
            if ((usage & SVGA_SURFACE_USAGE_COHERENT) || vws->force_coherent)
            req->base.drm_surface_flags |= drm_vmw_surface_flag_shareable;
   req->base.drm_surface_flags |= drm_vmw_surface_flag_create_buffer;
   req->base.base_size.width = size.width;
   req->base.base_size.height = size.height;
   req->base.base_size.depth = size.depth;
   req->base.mip_levels = numMipLevels;
   req->base.multisample_count = 0;
            if (vws->base.have_vgpu10) {
      req->base.array_size = numFaces;
      } else {
      assert(numFaces * numMipLevels < DRM_VMW_MAX_SURFACE_FACES*
   DRM_VMW_MAX_MIP_LEVELS);
               req->base.buffer_handle = buffer_handle ?
            ret = drmCommandWriteRead(vws->ioctl.drm_fd,
                  if (ret)
      } else {
      struct drm_vmw_gb_surface_create_req *req = &s_arg.arg.req;
            req->svga3d_flags = (uint32_t) flags;
            if (usage & SVGA_SURFACE_USAGE_SCANOUT)
                     req->drm_surface_flags |= drm_vmw_surface_flag_create_buffer;
   req->base_size.width = size.width;
   req->base_size.height = size.height;
   req->base_size.depth = size.depth;
   req->mip_levels = numMipLevels;
   req->multisample_count = 0;
            if (vws->base.have_vgpu10) {
      req->array_size = numFaces;
      } else {
      assert(numFaces * numMipLevels < DRM_VMW_MAX_SURFACE_FACES*
   DRM_VMW_MAX_MIP_LEVELS);
               req->buffer_handle = buffer_handle ?
            ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GB_SURFACE_CREATE,
            if (ret)
               if (p_region) {
      region->handle = rep->buffer_handle;
   region->map_handle = rep->buffer_map_handle;
   region->drm_fd = vws->ioctl.drm_fd;
   region->size = rep->backup_size;
               vmw_printf("Surface id is %d\n", rep->sid);
         out_fail_create:
      FREE(region);
      }
      /**
   * vmw_ioctl_surface_req - Fill in a struct surface_req
   *
   * @vws: Winsys screen
   * @whandle: Surface handle
   * @req: The struct surface req to fill in
   * @needs_unref: This call takes a kernel surface reference that needs to
   * be unreferenced.
   *
   * Returns 0 on success, negative error type otherwise.
   * Fills in the surface_req structure according to handle type and kernel
   * capabilities.
   */
   static int
   vmw_ioctl_surface_req(const struct vmw_winsys_screen *vws,
                     {
               switch(whandle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
   case WINSYS_HANDLE_TYPE_KMS:
      *needs_unref = false;
   req->handle_type = DRM_VMW_HANDLE_LEGACY;
   req->sid = whandle->handle;
      case WINSYS_HANDLE_TYPE_FD:
      if (!vws->ioctl.have_drm_2_6) {
               ret = drmPrimeFDToHandle(vws->ioctl.drm_fd, whandle->handle, &handle);
   if (ret) {
      vmw_error("Failed to get handle from prime fd %d.\n",
                     *needs_unref = true;
   req->handle_type = DRM_VMW_HANDLE_LEGACY;
      } else {
      *needs_unref = false;
   req->handle_type = DRM_VMW_HANDLE_PRIME;
      }
      default:
      vmw_error("Attempt to import unsupported handle type %d.\n",
                        }
      /**
   * vmw_ioctl_gb_surface_ref - Put a reference on a guest-backed surface and
   * get surface information
   *
   * @vws: Screen to register the reference on
   * @handle: Kernel handle of the guest-backed surface
   * @flags: flags used when the surface was created
   * @format: Format used when the surface was created
   * @numMipLevels: Number of mipmap levels of the surface
   * @p_region: On successful return points to a newly allocated
   * struct vmw_region holding a reference to the surface backup buffer.
   *
   * Returns 0 on success, a system error on failure.
   */
   int
   vmw_ioctl_gb_surface_ref(struct vmw_winsys_screen *vws,
                           const struct winsys_handle *whandle,
   SVGA3dSurfaceAllFlags *flags,
   {
      struct vmw_region *region = NULL;
   bool needs_unref = false;
            assert(p_region != NULL);
   region = CALLOC_STRUCT(vmw_region);
   if (!region)
            if (vws->ioctl.have_drm_2_15) {
      union drm_vmw_gb_surface_reference_ext_arg s_arg;
   struct drm_vmw_surface_arg *req = &s_arg.req;
            memset(&s_arg, 0, sizeof(s_arg));
   ret = vmw_ioctl_surface_req(vws, whandle, req, &needs_unref);
   if (ret)
            *handle = req->sid;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GB_SURFACE_REF_EXT,
            if (ret)
            region->handle = rep->crep.buffer_handle;
   region->map_handle = rep->crep.buffer_map_handle;
   region->drm_fd = vws->ioctl.drm_fd;
   region->size = rep->crep.backup_size;
            *handle = rep->crep.handle;
   *flags = SVGA3D_FLAGS_64(rep->creq.svga3d_flags_upper_32_bits,
         *format = rep->creq.base.format;
      } else {
      union drm_vmw_gb_surface_reference_arg s_arg;
   struct drm_vmw_surface_arg *req = &s_arg.req;
            memset(&s_arg, 0, sizeof(s_arg));
   ret = vmw_ioctl_surface_req(vws, whandle, req, &needs_unref);
   if (ret)
            *handle = req->sid;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GB_SURFACE_REF,
            if (ret)
            region->handle = rep->crep.buffer_handle;
   region->map_handle = rep->crep.buffer_map_handle;
   region->drm_fd = vws->ioctl.drm_fd;
   region->size = rep->crep.backup_size;
            *handle = rep->crep.handle;
   *flags = rep->creq.svga3d_flags;
   *format = rep->creq.format;
                        if (needs_unref)
               out_fail_ref:
      if (needs_unref)
      out_fail_req:
      FREE(region);
      }
      void
   vmw_ioctl_surface_destroy(struct vmw_winsys_screen *vws, uint32 sid)
   {
                        memset(&s_arg, 0, sizeof(s_arg));
            (void)drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_UNREF_SURFACE,
      }
      void
   vmw_ioctl_command(struct vmw_winsys_screen *vws, int32_t cid,
                     {
      struct drm_vmw_execbuf_arg arg;
   struct drm_vmw_fence_rep rep;
   int ret;
         #ifdef DEBUG
      {
      static bool firsttime = true;
   static bool debug = false;
   static bool skip = false;
   if (firsttime) {
      debug = debug_get_bool_option("SVGA_DUMP_CMD", false);
      }
   if (debug) {
      VMW_FUNC;
      }
   firsttime = false;
   if (skip) {
               #endif
         memset(&arg, 0, sizeof(arg));
            if (flags & SVGA_HINT_FLAG_EXPORT_FENCE_FD) {
                  if (imported_fence_fd != -1) {
                  rep.error = -EFAULT;
   if (pfence)
         arg.commands = (unsigned long)commands;
   arg.command_size = size;
   arg.throttle_us = throttle_us;
   arg.version = vws->ioctl.drm_execbuf_version;
            /* Older DRM module requires this to be zero */
   if (vws->base.have_fence_fd)
            /* In DRM_VMW_EXECBUF_VERSION 1, the drm_vmw_execbuf_arg structure ends with
   * the flags field. The structure size sent to drmCommandWrite must match
   * the drm_execbuf_version. Otherwise, an invalid value will be returned.
   */
   argsize = vws->ioctl.drm_execbuf_version > 1 ? sizeof(arg) :
         do {
      ret = drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_EXECBUF, &arg, argsize);
   if (ret == -EBUSY)
      } while(ret == -ERESTART || ret == -EBUSY);
   if (ret) {
      vmw_error("%s error %s.\n", __func__, strerror(-ret));
                           /*
   * Kernel has already synced, or caller requested no fence.
   */
   *pfence = NULL;
      } else {
      if (pfence) {
                     /* Older DRM module will set this to zero, but -1 is the proper FD
   * to use for no Fence FD support */
                  *pfence = vmw_fence_create(vws->fence_ops, rep.handle,
         if (*pfence == NULL) {
      /*
   * Fence creation failed. Need to sync.
   */
   (void) vmw_ioctl_fence_finish(vws, rep.handle, rep.mask);
               }
         struct vmw_region *
   vmw_ioctl_region_create(struct vmw_winsys_screen *vws, uint32_t size)
   {
      struct vmw_region *region;
   union drm_vmw_alloc_dmabuf_arg arg;
   struct drm_vmw_alloc_dmabuf_req *req = &arg.req;
   struct drm_vmw_dmabuf_rep *rep = &arg.rep;
                     region = CALLOC_STRUCT(vmw_region);
   if (!region)
            memset(&arg, 0, sizeof(arg));
   req->size = size;
   do {
         sizeof(arg));
            if (ret) {
      vmw_error("IOCTL failed %d: %s\n", ret, strerror(-ret));
               region->data = NULL;
   region->handle = rep->handle;
   region->map_handle = rep->map_handle;
   region->map_count = 0;
   region->size = size;
            vmw_printf("   gmrId = %u, offset = %u\n",
                  out_err1:
      FREE(region);
      }
      void
   vmw_ioctl_region_destroy(struct vmw_region *region)
   {
               vmw_printf("%s: gmrId = %u, offset = %u\n", __func__,
            if (region->data) {
      os_munmap(region->data, region->size);
               memset(&arg, 0, sizeof(arg));
   arg.handle = region->handle;
               }
      SVGAGuestPtr
   vmw_ioctl_region_ptr(struct vmw_region *region)
   {
      SVGAGuestPtr ptr = {region->handle, 0};
      }
      void *
   vmw_ioctl_region_map(struct vmw_region *region)
   {
               vmw_printf("%s: gmrId = %u, offset = %u\n", __func__,
            if (region->data == NULL) {
         region->drm_fd, region->map_handle);
      vmw_error("%s: Map failed.\n", __func__);
   return NULL;
            // MADV_HUGEPAGE only exists on Linux
   #ifdef MADV_HUGEPAGE
         #endif
                                 }
      void
   vmw_ioctl_region_unmap(struct vmw_region *region)
   {
      vmw_printf("%s: gmrId = %u, offset = %u\n", __func__,
            --region->map_count;
   os_munmap(region->data, region->size);
      }
      /**
   * vmw_ioctl_syncforcpu - Synchronize a buffer object for CPU usage
   *
   * @region: Pointer to a struct vmw_region representing the buffer object.
   * @dont_block: Dont wait for GPU idle, but rather return -EBUSY if the
   * GPU is busy with the buffer object.
   * @readonly: Hint that the CPU access is read-only.
   * @allow_cs: Allow concurrent command submission while the buffer is
   * synchronized for CPU. If FALSE command submissions referencing the
   * buffer will block until a corresponding call to vmw_ioctl_releasefromcpu.
   *
   * This function idles any GPU activities touching the buffer and blocks
   * command submission of commands referencing the buffer, even from
   * other processes.
   */
   int
   vmw_ioctl_syncforcpu(struct vmw_region *region,
                     {
               memset(&arg, 0, sizeof(arg));
   arg.op = drm_vmw_synccpu_grab;
   arg.handle = region->handle;
   arg.flags = drm_vmw_synccpu_read;
   if (!readonly)
         if (dont_block)
         if (allow_cs)
               }
      /**
   * vmw_ioctl_releasefromcpu - Undo a previous syncforcpu.
   *
   * @region: Pointer to a struct vmw_region representing the buffer object.
   * @readonly: Should hold the same value as the matching syncforcpu call.
   * @allow_cs: Should hold the same value as the matching syncforcpu call.
   */
   void
   vmw_ioctl_releasefromcpu(struct vmw_region *region,
               {
               memset(&arg, 0, sizeof(arg));
   arg.op = drm_vmw_synccpu_release;
   arg.handle = region->handle;
   arg.flags = drm_vmw_synccpu_read;
   if (!readonly)
         if (allow_cs)
               }
      void
   vmw_ioctl_fence_unref(struct vmw_winsys_screen *vws,
         {
      struct drm_vmw_fence_arg arg;
   int ret;
      memset(&arg, 0, sizeof(arg));
            ret = drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_FENCE_UNREF,
   &arg, sizeof(arg));
   if (ret != 0)
      }
      static inline uint32_t
   vmw_drm_fence_flags(uint32_t flags)
   {
                  dflags |= DRM_VMW_FENCE_FLAG_EXEC;
         dflags |= DRM_VMW_FENCE_FLAG_QUERY;
            }
         int
   vmw_ioctl_fence_signalled(struct vmw_winsys_screen *vws,
      uint32_t handle,
      {
      struct drm_vmw_fence_signaled_arg arg;
   uint32_t vflags = vmw_drm_fence_flags(flags);
            memset(&arg, 0, sizeof(arg));
   arg.handle = handle;
            ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_FENCE_SIGNALED,
            if (ret != 0)
                        }
            int
   vmw_ioctl_fence_finish(struct vmw_winsys_screen *vws,
               {
      struct drm_vmw_fence_wait_arg arg;
   uint32_t vflags = vmw_drm_fence_flags(flags);
                     arg.handle = handle;
   arg.timeout_us = VMW_FENCE_TIMEOUT_SECONDS*1000000;
   arg.lazy = 0;
            ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_FENCE_WAIT,
            if (ret != 0)
               }
      uint32
   vmw_ioctl_shader_create(struct vmw_winsys_screen *vws,
      SVGA3dShaderType type,
      {
      struct drm_vmw_shader_create_arg sh_arg;
                              sh_arg.size = code_len;
   sh_arg.buffer_handle = SVGA3D_INVALID_ID;
   sh_arg.shader_handle = SVGA3D_INVALID_ID;
   switch (type) {
   case SVGA3D_SHADERTYPE_VS:
      sh_arg.shader_type = drm_vmw_shader_type_vs;
      case SVGA3D_SHADERTYPE_PS:
      sh_arg.shader_type = drm_vmw_shader_type_ps;
      default:
      assert(!"Invalid shader type.");
               ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_CREATE_SHADER,
            if (ret)
               }
      void
   vmw_ioctl_shader_destroy(struct vmw_winsys_screen *vws, uint32 shid)
   {
                        memset(&sh_arg, 0, sizeof(sh_arg));
            (void)drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_UNREF_SHADER,
         }
      static int
   vmw_ioctl_parse_caps(struct vmw_winsys_screen *vws,
         {
               if (vws->base.have_gb_objects) {
      vws->ioctl.cap_3d[i].has_cap = true;
   vws->ioctl.cap_3d[i].result.u = cap_buffer[i];
         }
      } else {
      const uint32 *capsBlock;
   const SVGA3dCapsRecord *capsRecord = NULL;
   uint32 offset;
   const SVGA3dCapPair *capArray;
            /*
   * Search linearly through the caps block records for the specified type.
   */
   capsBlock = cap_buffer;
   const SVGA3dCapsRecord *record;
   assert(offset < SVGA_FIFO_3D_CAPS_SIZE);
   record = (const SVGA3dCapsRecord *) (capsBlock + offset);
   if ((record->header.type >= SVGA3DCAPS_RECORD_DEVCAPS_MIN) &&
         (record->header.type <= SVGA3DCAPS_RECORD_DEVCAPS_MAX) &&
         }
                  return -1;
            /*
   * Calculate the number of caps from the size of the record.
   */
   capArray = (const SVGA3dCapPair *) capsRecord->data;
                  index = capArray[i][0];
   if (index < vws->ioctl.num_cap_3d) {
      vws->ioctl.cap_3d[index].has_cap = true;
      } else {
         }
            }
      }
      bool
   vmw_ioctl_init(struct vmw_winsys_screen *vws)
   {
      struct drm_vmw_getparam_arg gp_arg;
   struct drm_vmw_get_3d_cap_arg cap_arg;
   unsigned int size;
   int ret;
   uint32_t *cap_buffer;
   drmVersionPtr version;
   bool drm_gb_capable;
   bool have_drm_2_5;
                     version = drmGetVersion(vws->ioctl.drm_fd);
   if (!version)
            have_drm_2_5 = version->version_major > 2 ||
         vws->ioctl.have_drm_2_6 = version->version_major > 2 ||
         vws->ioctl.have_drm_2_9 = version->version_major > 2 ||
         vws->ioctl.have_drm_2_15 = version->version_major > 2 ||
         vws->ioctl.have_drm_2_16 = version->version_major > 2 ||
         vws->ioctl.have_drm_2_17 = version->version_major > 2 ||
         vws->ioctl.have_drm_2_18 = version->version_major > 2 ||
         vws->ioctl.have_drm_2_19 = version->version_major > 2 ||
         vws->ioctl.have_drm_2_20 = version->version_major > 2 ||
                              memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_3D;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
         if (ret || gp_arg.value == 0) {
      vmw_error("No 3D enabled (%i, %s).\n", ret, strerror(-ret));
               memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_FIFO_HW_VERSION;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
         if (ret) {
      vmw_error("Failed to get fifo hw version (%i, %s).\n",
            }
   vws->ioctl.hwversion = gp_arg.value;
   getenv_val = getenv("SVGA_FORCE_HOST_BACKED");
   if (!getenv_val || strcmp(getenv_val, "0") == 0) {
      memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_HW_CAPS;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
      } else {
         }
   if (ret)
         else
      vws->base.have_gb_objects =
         if (vws->base.have_gb_objects && !drm_gb_capable)
            vws->base.have_vgpu10 = false;
   vws->base.have_sm4_1 = false;
            memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_DEVICE_ID;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
         if (ret || gp_arg.value == 0) {
         } else {
                  if (vws->base.have_gb_objects) {
      memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_MAX_MOB_MEMORY;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
         if (ret) {
      /* Just guess a large enough value. */
      } else {
                  memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_MAX_MOB_SIZE;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
            if (ret || gp_arg.value == 0) {
         } else {
                  /* Never early flush surfaces, mobs do accounting. */
            if (vws->ioctl.have_drm_2_9) {
      memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_DX;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
                           debug_printf("Have VGPU10 interface and hardware.\n");
   vws->base.have_vgpu10 = true;
   vgpu10_val = getenv("SVGA_VGPU10");
   if (vgpu10_val && strcmp(vgpu10_val, "0") == 0) {
      debug_printf("Disabling VGPU10 interface.\n");
      } else {
                        if (vws->ioctl.have_drm_2_15 && vws->base.have_vgpu10) {
      memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_HW_CAPS2;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
         if (ret == 0 && gp_arg.value != 0) {
                  memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_SM4_1;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
         if (ret == 0 && gp_arg.value != 0) {
                     if (vws->ioctl.have_drm_2_18 && vws->base.have_sm4_1) {
      memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_SM5;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
         if (ret == 0 && gp_arg.value != 0) {
                     if (vws->ioctl.have_drm_2_20 && vws->base.have_sm5) {
      memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_GL43;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
         if (ret == 0 && gp_arg.value != 0) {
                     memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_3D_CAPS_SIZE;
   ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
         if (ret)
         else
            if (vws->base.have_gb_objects)
         else
            if (vws->ioctl.have_drm_2_16) {
      vws->base.have_coherent = true;
   getenv_val = getenv("SVGA_FORCE_COHERENT");
   if (getenv_val && strcmp(getenv_val, "0") != 0)
         } else {
               memset(&gp_arg, 0, sizeof(gp_arg));
   gp_arg.param = DRM_VMW_PARAM_MAX_SURF_MEMORY;
   if (have_drm_2_5)
      ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_GET_PARAM,
      if (!have_drm_2_5 || ret) {
      /* Just guess a large enough value, around 800mb. */
      } else {
                                       debug_printf("VGPU10 interface is %s.\n",
            cap_buffer = calloc(1, size);
   if (!cap_buffer) {
      debug_printf("Failed alloc fifo 3D caps buffer.\n");
               vws->ioctl.cap_3d = calloc(vws->ioctl.num_cap_3d, 
         if (!vws->ioctl.cap_3d) {
      debug_printf("Failed alloc fifo 3D caps buffer.\n");
               memset(&cap_arg, 0, sizeof(cap_arg));
   cap_arg.buffer = (uint64_t) (unsigned long) (cap_buffer);
            /*
   * This call must always be after DRM_VMW_PARAM_MAX_MOB_MEMORY and
   * DRM_VMW_PARAM_SM4_1. This is because, based on these calls, kernel
   * driver sends the supported cap.
   */
   ret = drmCommandWrite(vws->ioctl.drm_fd, DRM_VMW_GET_3D_CAP,
            if (ret) {
         " (%i, %s).\n", ret, strerror(-ret));
                  ret = vmw_ioctl_parse_caps(vws, cap_buffer);
   if (ret) {
         " (%i, %s).\n", ret, strerror(-ret));
                  if (((version->version_major == 2 && version->version_minor >= 10)
            /* support for these commands didn't make it into vmwgfx kernel
      * modules before 2.10.
   */
   vws->base.have_generate_mipmap_cmd = true;
               if (version->version_major == 2 && version->version_minor >= 14) {
                  free(cap_buffer);
   drmFreeVersion(version);
   vmw_printf("%s OK\n", __func__);
      out_no_caps:
         out_no_caparray:
         out_no_3d:
         out_no_version:
      vws->ioctl.num_cap_3d = 0;
   debug_printf("%s Failed\n", __func__);
      }
            void
   vmw_ioctl_cleanup(struct vmw_winsys_screen *vws)
   {
                  }
