   #include <limits.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include "common/amd_family.h"
   #include "drm-shim/drm_shim.h"
   #include "util/log.h"
   #include <util/u_math.h>
   #include <radeon_drm.h>
      bool drm_shim_driver_prefers_first_render_node = true;
      static enum radeon_family radeon_family = CHIP_RV515;
   static uint16_t device_id = 0x7140;
      static int
   radeon_ioctl_noop(int fd, unsigned long request, void *arg)
   {
         }
      static int
   radeon_ioctl_info(int fd, unsigned long request, void *arg)
   {
      struct drm_radeon_info *info = arg;
            switch (info->request) {
   case RADEON_INFO_DEVICE_ID:
      *value = device_id;
         case RADEON_INFO_RING_WORKING:
   case RADEON_INFO_ACCEL_WORKING2:
   case RADEON_INFO_VA_UNMAP_WORKING:
      *value = true;
         case RADEON_INFO_GPU_RESET_COUNTER:
      *value = 0;
         case RADEON_INFO_IB_VM_MAX_SIZE:
      if (radeon_family < CHIP_CAYMAN)
         *value = 64 << 10;
         case RADEON_INFO_VCE_FW_VERSION:
   case RADEON_INFO_TILING_CONFIG:
   case RADEON_INFO_BACKEND_MAP:
      *value = 0; /* dummy */
         case RADEON_INFO_VA_START:
            case RADEON_INFO_MAX_SCLK:
   case RADEON_INFO_CLOCK_CRYSTAL_FREQ:
   case RADEON_INFO_NUM_GB_PIPES:
   case RADEON_INFO_NUM_Z_PIPES:
   case RADEON_INFO_MAX_PIPES:
   case RADEON_INFO_MAX_SE:
   case RADEON_INFO_MAX_SH_PER_SE:
   case RADEON_INFO_ACTIVE_CU_COUNT:
   case RADEON_INFO_NUM_BACKENDS:
   case RADEON_INFO_NUM_TILE_PIPES:
      *value = 1; /* dummy */
         default:
      fprintf(stderr, "Unknown DRM_IOCTL_RADEON_INFO request 0x%02X\n", info->request);
         }
      static int
   radeon_ioctl_gem_info(int fd, unsigned long request, void *arg)
   {
               /* Dummy values. */
   info->vram_size = 256 * 1024 * 1024;
   info->vram_visible = info->vram_size;
               }
      static int
   radeon_ioctl_gem_create(int fd, unsigned long request, void *arg)
   {
               struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct shim_bo *bo = calloc(1, sizeof(*bo));
                                          }
      static int
   radeon_ioctl_gem_mmap(int fd, unsigned long request, void *arg)
   {
               struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
                        }
      static int
   radeon_ioctl_gem_userptr(int fd, unsigned long request, void *arg)
   {
      /* probed at winsys init, just return no support. */
      }
      static ioctl_fn_t driver_ioctls[] = {
      [DRM_RADEON_CS] = radeon_ioctl_noop,
   [DRM_RADEON_INFO] = radeon_ioctl_info,
   [DRM_RADEON_GEM_SET_DOMAIN] = radeon_ioctl_noop,
   [DRM_RADEON_GEM_SET_TILING] = radeon_ioctl_noop,
   [DRM_RADEON_GEM_WAIT_IDLE] = radeon_ioctl_noop,
   [DRM_RADEON_GEM_INFO] = radeon_ioctl_gem_info,
   [DRM_RADEON_GEM_CREATE] = radeon_ioctl_gem_create,
   [DRM_RADEON_GEM_MMAP] = radeon_ioctl_gem_mmap,
      };
      struct radeon_pci_id {
      uint16_t device_id;
   const char *name;
   enum radeon_family family;
      };
      #define CHIPSET(d, n, f) {.device_id = (d), .name = #n, .family = CHIP_##f, .family_name = #f},
   static const struct radeon_pci_id radeon_pci_ids[] = {
   #include "pci_ids/r300_pci_ids.h"
   #include "pci_ids/r600_pci_ids.h"
   };
   #undef CHIPSET
      static void
   radeon_get_device_id()
   {
      const char *gpu_id = getenv("RADEON_GPU_ID");
   if (!gpu_id)
            if (strncmp(gpu_id, "0x", 2) == 0) {
      device_id = strtoll(gpu_id + 2, NULL, 16);
               for (int i = 0; i < ARRAY_SIZE(radeon_pci_ids); i++) {
      if (strcasecmp(gpu_id, radeon_pci_ids[i].name) == 0 ||
      strcasecmp(gpu_id, radeon_pci_ids[i].family_name) == 0) {
   device_id = radeon_pci_ids[i].device_id;
                  mesa_loge("Failed to find radeon GPU named \"%s\"\n", gpu_id);
      }
      void
   drm_shim_driver_init(void)
   {
               shim_device.bus_type = DRM_BUS_PCI;
   shim_device.driver_name = "radeon";
   shim_device.driver_ioctls = driver_ioctls;
            shim_device.version_major = 2;
   shim_device.version_minor = 50;
            /* nothing looks at the pci id, so fix it to a RV515 */
   static const char uevent_content[] =
      "DRIVER=radeon\n"
   "PCI_CLASS=30000\n"
   "PCI_ID=1002:7140\n"
   "PCI_SUBSYS_ID=1028:075B\n"
   "PCI_SLOT_NAME=0000:01:00.0\n"
      drm_shim_override_file(uevent_content,
               drm_shim_override_file("0x0\n",
               drm_shim_override_file("0x1002",
               drm_shim_override_file("0x1002",
         drm_shim_override_file("0x7140",
               drm_shim_override_file("0x7140",
         drm_shim_override_file("0x1234",
               drm_shim_override_file("0x1234",
         drm_shim_override_file("0x1234",
               drm_shim_override_file("0x1234",
      }
