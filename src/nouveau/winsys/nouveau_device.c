   #include "nouveau_device.h"
      #include "nouveau_context.h"
      #include "drm-uapi/nouveau_drm.h"
   #include "util/hash_table.h"
   #include "util/u_debug.h"
   #include "util/os_file.h"
   #include "util/os_misc.h"
      #include <fcntl.h>
   #include <nouveau/nvif/ioctl.h>
   #include <nvif/cl0080.h>
   #include <nvif/class.h>
   #include <unistd.h>
   #include <xf86drm.h>
      static uint8_t
   sm_for_chipset(uint16_t chipset)
   {
      if (chipset >= 0x190)
         // GH100 is older than AD10X, but is SM90
   else if (chipset >= 0x180)
         else if (chipset == 0x17b)
         else if (chipset >= 0x172)
         else if (chipset >= 0x170)
         else if (chipset >= 0x160)
         else if (chipset >= 0x14b)
         else if (chipset >= 0x140)
         else if (chipset >= 0x13b)
         else if (chipset >= 0x132)
         else if (chipset >= 0x130)
         else if (chipset >= 0x12b)
         else if (chipset >= 0x120)
         else if (chipset >= 0x110)
         // TODO: 37
   else if (chipset >= 0x0f0)
         else if (chipset >= 0x0ea)
         else if (chipset >= 0x0e0)
         // GF110 is SM20
   else if (chipset == 0x0c8)
         else if (chipset >= 0x0c1)
         else if (chipset >= 0x0c0)
         else if (chipset >= 0x0a3)
         // GT200 is SM13
   else if (chipset >= 0x0a0)
         else if (chipset >= 0x080)
         // this has to be == because 0x63 is older than 0x50 and has no compute
   else if (chipset == 0x050)
         // no compute
      }
      static uint8_t
   max_warps_per_mp_for_sm(uint8_t sm)
   {
      switch (sm) {
   case 10:
   case 11:
         case 12:
   case 13:
   case 75:
         case 20:
   case 21:
   case 86:
   case 87:
   case 89:
         case 30:
   case 32:
   case 35:
   case 37:
   case 50:
   case 52:
   case 53:
   case 60:
   case 61:
   case 62:
   case 70:
   case 72:
   case 80:
   case 90:
         default:
      assert(!"unkown SM version");
   // return the biggest known value
         }
      static uint8_t
   mp_per_tpc_for_chipset(uint16_t chipset)
   {
      // GP100 is special and has two, otherwise it's a Volta and newer thing to have two
   if (chipset == 0x130 || chipset >= 0x140)
            }
      static void
   nouveau_ws_device_set_dbg_flags(struct nouveau_ws_device *dev)
   {
      const struct debug_control flags[] = {
      { "push_dump", NVK_DEBUG_PUSH_DUMP },
   { "push_sync", NVK_DEBUG_PUSH_SYNC },
   { "zero_memory", NVK_DEBUG_ZERO_MEMORY },
   { "vm", NVK_DEBUG_VM },
                  }
      static int
   nouveau_ws_param(int fd, uint64_t param, uint64_t *value)
   {
               int ret = drmCommandWriteRead(fd, DRM_NOUVEAU_GETPARAM, &data, sizeof(data));
   if (ret)
            *value = data.value;
      }
      static int
   nouveau_ws_device_alloc(int fd, struct nouveau_ws_device *dev)
   {
      struct {
      struct nvif_ioctl_v0 ioctl;
   struct nvif_ioctl_new_v0 new;
      } args = {
      .ioctl = {
      .object = 0,
   .owner = NVIF_IOCTL_V0_OWNER_ANY,
   .route = 0x00,
   .type = NVIF_IOCTL_V0_NEW,
      },
   .new = {
      .handle = 0,
   .object = (uintptr_t)dev,
   .oclass = NV_DEVICE,
   .route = NVIF_IOCTL_V0_ROUTE_NVIF,
   .token = (uintptr_t)dev,
      },
   .dev = {
                        }
      static int
   nouveau_ws_device_info(int fd, struct nouveau_ws_device *dev)
   {
      struct {
      struct nvif_ioctl_v0 ioctl;
   struct nvif_ioctl_mthd_v0 mthd;
      } args = {
      .ioctl = {
      .object = (uintptr_t)dev,
   .owner = NVIF_IOCTL_V0_OWNER_ANY,
   .route = 0x00,
   .type = NVIF_IOCTL_V0_MTHD,
      },
   .mthd = {
      .method = NV_DEVICE_V0_INFO,
      },
   .info = {
                     int ret = drmCommandWriteRead(fd, DRM_NOUVEAU_NVIF, &args, sizeof(args));
   if (ret)
            dev->info.chipset = args.info.chipset;
            switch (args.info.platform) {
   case NV_DEVICE_INFO_V0_IGP:
      dev->info.type = NV_DEVICE_TYPE_IGP;
      case NV_DEVICE_INFO_V0_SOC:
      dev->info.type = NV_DEVICE_TYPE_SOC;
      case NV_DEVICE_INFO_V0_PCI:
   case NV_DEVICE_INFO_V0_AGP:
   case NV_DEVICE_INFO_V0_PCIE:
   default:
      dev->info.type = NV_DEVICE_TYPE_DIS;
               STATIC_ASSERT(sizeof(dev->info.device_name) >= sizeof(args.info.name));
            STATIC_ASSERT(sizeof(dev->info.chipset_name) >= sizeof(args.info.chip));
               }
      struct nouveau_ws_device *
   nouveau_ws_device_new(drmDevicePtr drm_device)
   {
      const char *path = drm_device->nodes[DRM_NODE_RENDER];
   struct nouveau_ws_device *device = CALLOC_STRUCT(nouveau_ws_device);
   uint64_t value = 0;
            int fd = open(path, O_RDWR | O_CLOEXEC);
   if (fd < 0)
            ver = drmGetVersion(fd);
   if (!ver)
            if (strncmp("nouveau", ver->name, ver->name_len) != 0) {
      fprintf(stderr,
         "DRM kernel driver '%.*s' in use. NVK requires nouveau.\n",
               uint32_t version =
      ver->version_major << 24 |
   ver->version_minor << 8  |
      drmFreeVersion(ver);
            if (version < 0x01000301)
            const uint64_t TOP = 1ull << 40;
   const uint64_t KERN = 1ull << 39;
   struct drm_nouveau_vm_init vminit = { TOP-KERN, KERN };
   int ret = drmCommandWrite(fd, DRM_NOUVEAU_VM_INIT, &vminit, sizeof(vminit));
   if (ret == 0) {
      device->has_vm_bind = true;
   util_vma_heap_init(&device->vma_heap, 4096, (TOP - KERN) - 4096);
               if (nouveau_ws_device_alloc(fd, device))
            if (nouveau_ws_param(fd, NOUVEAU_GETPARAM_PCI_DEVICE, &value))
                     if (nouveau_ws_device_info(fd, device))
            if (drm_device->bustype == DRM_BUS_PCI) {
      assert(device->info.type == NV_DEVICE_TYPE_DIS);
            device->info.pci.domain       = drm_device->businfo.pci->domain;
   device->info.pci.bus          = drm_device->businfo.pci->bus;
   device->info.pci.dev          = drm_device->businfo.pci->dev;
   device->info.pci.func         = drm_device->businfo.pci->func;
                        if (nouveau_ws_param(fd, NOUVEAU_GETPARAM_EXEC_PUSH_MAX, &value))
         else
            if (device->info.vram_size_B == 0)
         else
            if (nouveau_ws_param(fd, NOUVEAU_GETPARAM_GRAPH_UNITS, &value))
            device->info.gpc_count = (value >> 0) & 0x000000ff;
                     struct nouveau_ws_context *tmp_ctx;
   if (nouveau_ws_context_create(device, &tmp_ctx))
            device->info.sm = sm_for_chipset(device->info.chipset);
   device->info.cls_copy = tmp_ctx->copy.cls;
   device->info.cls_eng2d = tmp_ctx->eng2d.cls;
   device->info.cls_eng3d = tmp_ctx->eng3d.cls;
   device->info.cls_m2mf = tmp_ctx->m2mf.cls;
            // for now we hardcode those values, but in the future Nouveau could provide that information to
   // us instead.
   device->info.max_warps_per_mp = max_warps_per_mp_for_sm(device->info.sm);
                     simple_mtx_init(&device->bos_lock, mtx_plain);
                  out_err:
      if (device->has_vm_bind) {
      util_vma_heap_finish(&device->vma_heap);
      }
   if (ver)
      out_open:
      FREE(device);
   close(fd);
      }
      void
   nouveau_ws_device_destroy(struct nouveau_ws_device *device)
   {
      if (!device)
            _mesa_hash_table_destroy(device->bos, NULL);
            if (device->has_vm_bind) {
      util_vma_heap_finish(&device->vma_heap);
               close(device->fd);
      }
