   /*
   * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
   * Copyright (C) 2014-2016 Emil Velikov <emil.l.velikov@gmail.com>
   * Copyright (C) 2016 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include <dlfcn.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <sys/stat.h>
   #include <stdarg.h>
   #include <stdio.h>
   #include <stdbool.h>
   #include <string.h>
   #include <unistd.h>
   #include <stdlib.h>
   #include <limits.h>
   #include <sys/param.h>
   #ifdef MAJOR_IN_MKDEV
   #include <sys/mkdev.h>
   #endif
   #ifdef MAJOR_IN_SYSMACROS
   #include <sys/sysmacros.h>
   #endif
   #include <GL/gl.h>
   #include <GL/internal/dri_interface.h>
   #include <GL/internal/mesa_interface.h>
   #include "loader.h"
   #include "util/libdrm.h"
   #include "util/os_file.h"
   #include "util/os_misc.h"
   #include "util/u_debug.h"
   #include "git_sha1.h"
      #define MAX_DRM_DEVICES 64
      #ifdef USE_DRICONF
   #include "util/xmlconfig.h"
   #include "util/driconf.h"
   #endif
      #include "util/macros.h"
      #define __IS_LOADER
   #include "pci_id_driver_map.h"
      /* For systems like Hurd */
   #ifndef PATH_MAX
   #define PATH_MAX 4096
   #endif
      static void default_logger(int level, const char *fmt, ...)
   {
      if (level <= _LOADER_WARNING) {
      va_list args;
   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
         }
      static loader_logger *log_ = default_logger;
      int
   loader_open_device(const char *device_name)
   {
         #ifdef O_CLOEXEC
      fd = open(device_name, O_RDWR | O_CLOEXEC);
      #endif
      {
      fd = open(device_name, O_RDWR);
   if (fd != -1)
      }
   if (fd == -1 && errno == EACCES) {
      log_(_LOADER_WARNING, "failed to open %s: %s\n",
      }
      }
      char *
   loader_get_kernel_driver_name(int fd)
   {
      char *driver;
            if (!version) {
      log_(_LOADER_WARNING, "failed to get driver name for fd %d\n", fd);
               driver = strndup(version->name, version->name_len);
   log_(driver ? _LOADER_DEBUG : _LOADER_WARNING, "using driver %s for %d\n",
            drmFreeVersion(version);
      }
      bool
   iris_predicate(int fd)
   {
      char *kernel_driver = loader_get_kernel_driver_name(fd);
   bool ret = kernel_driver && (strcmp(kernel_driver, "i915") == 0 ||
            free(kernel_driver);
      }
      /**
   * Goes through all the platform devices whose driver is on the given list and
   * try to open their render node. It returns the fd of the first device that
   * it can open.
   */
   int
   loader_open_render_node_platform_device(const char * const drivers[],
         {
      drmDevicePtr devices[MAX_DRM_DEVICES], device;
   int num_devices, fd = -1;
   int i, j;
            num_devices = drmGetDevices2(0, devices, MAX_DRM_DEVICES);
   if (num_devices <= 0)
            for (i = 0; i < num_devices; i++) {
               if ((device->available_nodes & (1 << DRM_NODE_RENDER)) &&
                     fd = loader_open_device(device->nodes[DRM_NODE_RENDER]);
                  version = drmGetVersion(fd);
   if (!version) {
      close(fd);
               for (j = 0; j < n_drivers; j++) {
      if (strcmp(version->name, drivers[j]) == 0) {
      found = true;
         }
   if (!found) {
      drmFreeVersion(version);
   close(fd);
               drmFreeVersion(version);
         }
            if (i == num_devices)
               }
      bool
   loader_is_device_render_capable(int fd)
   {
      drmDevicePtr dev_ptr;
            if (drmGetDevice2(fd, 0, &dev_ptr) != 0)
                                 }
      char *
   loader_get_render_node(dev_t device)
   {
      char *render_node = NULL;
            if (drmGetDeviceFromDevId(device, 0, &dev_ptr) < 0)
            if (dev_ptr->available_nodes & (1 << DRM_NODE_RENDER)) {
      render_node = strdup(dev_ptr->nodes[DRM_NODE_RENDER]);
   if (!render_node)
                           }
      #ifdef USE_DRICONF
   static const driOptionDescription __driConfigOptionsLoader[] = {
      DRI_CONF_SECTION_INITIALIZATION
      DRI_CONF_DEVICE_ID_PATH_TAG()
         };
      static char *loader_get_dri_config_driver(int fd)
   {
      driOptionCache defaultInitOptions;
   driOptionCache userInitOptions;
   char *dri_driver = NULL;
            driParseOptionInfo(&defaultInitOptions, __driConfigOptionsLoader,
         driParseConfigFiles(&userInitOptions, &defaultInitOptions, 0,
         if (driCheckOption(&userInitOptions, "dri_driver", DRI_STRING)) {
      char *opt = driQueryOptionstr(&userInitOptions, "dri_driver");
   /* not an empty string */
   if (*opt)
      }
   driDestroyOptionCache(&userInitOptions);
            free(kernel_driver);
      }
      static char *loader_get_dri_config_device_id(void)
   {
      driOptionCache defaultInitOptions;
   driOptionCache userInitOptions;
            driParseOptionInfo(&defaultInitOptions, __driConfigOptionsLoader,
         driParseConfigFiles(&userInitOptions, &defaultInitOptions, 0,
         if (driCheckOption(&userInitOptions, "device_id", DRI_STRING)) {
      char *opt = driQueryOptionstr(&userInitOptions, "device_id");
   if (*opt)
      }
   driDestroyOptionCache(&userInitOptions);
               }
   #endif
      static char *drm_construct_id_path_tag(drmDevicePtr device)
   {
               if (device->bustype == DRM_BUS_PCI) {
      if (asprintf(&tag, "pci-%04x_%02x_%02x_%1u",
               device->businfo.pci->domain,
   device->businfo.pci->bus,
            } else if (device->bustype == DRM_BUS_PLATFORM ||
                     if (device->bustype == DRM_BUS_PLATFORM)
         else
            name = strrchr(fullname, '/');
   if (!name)
         else
            address = strchr(name, '@');
   if (address) {
               if (asprintf(&tag, "platform-%s_%s", address, name) < 0)
      } else {
      if (asprintf(&tag, "platform-%s", name) < 0)
                  }
      }
      static bool drm_device_matches_tag(drmDevicePtr device, const char *prime_tag)
   {
      char *tag = drm_construct_id_path_tag(device);
            if (tag == NULL)
                     free(tag);
      }
      static char *drm_get_id_path_tag_for_fd(int fd)
   {
      drmDevicePtr device;
            if (drmGetDevice2(fd, 0, &device) != 0)
            tag = drm_construct_id_path_tag(device);
   drmFreeDevice(&device);
      }
      bool loader_get_user_preferred_fd(int *fd_render_gpu, int *original_fd)
   {
      const char *dri_prime = getenv("DRI_PRIME");
   bool debug = debug_get_bool_option("DRI_PRIME_DEBUG", false);
   char *default_tag = NULL;
   drmDevicePtr devices[MAX_DRM_DEVICES];
   int i, num_devices, fd = -1;
   struct {
      enum {
      PRIME_IS_INTEGER,
   PRIME_IS_VID_DID,
      } semantics;
   union {
      int as_integer;
   struct {
            } v;
      } prime = {};
            if (dri_prime)
      #ifdef USE_DRICONF
      else
      #endif
         if (prime.str == NULL) {
         } else {
      uint16_t vendor_id, device_id;
   if (sscanf(prime.str, "%hx:%hx", &vendor_id, &device_id) == 2) {
      prime.semantics = PRIME_IS_VID_DID;
   prime.v.as_vendor_device_ids.v = vendor_id;
      } else {
      int i = atoi(prime.str);
   if (i < 0 || strcmp(prime.str, "0") == 0) {
      printf("Invalid value (%d) for DRI_PRIME. Should be > 0\n", i);
      } else if (i == 0) {
         } else {
      prime.semantics = PRIME_IS_INTEGER;
                     default_tag = drm_get_id_path_tag_for_fd(*fd_render_gpu);
   if (default_tag == NULL)
            num_devices = drmGetDevices2(0, devices, MAX_DRM_DEVICES);
   if (num_devices <= 0)
            if (debug) {
      log_(_LOADER_WARNING, "DRI_PRIME: %d devices\n", num_devices);
   for (i = 0; i < num_devices; i++) {
      log_(_LOADER_WARNING, "  %d:", i);
   if (!(devices[i]->available_nodes & 1 << DRM_NODE_RENDER)) {
      log_(_LOADER_WARNING, "not a render node -> not usable\n");
      }
   char *tag = drm_construct_id_path_tag(devices[i]);
   if (tag) {
      log_(_LOADER_WARNING, " %s", tag);
      }
   if (devices[i]->bustype == DRM_BUS_PCI) {
      log_(_LOADER_WARNING, " %4x:%4x",
      devices[i]->deviceinfo.pci->vendor_id,
                  if (drm_device_matches_tag(devices[i], default_tag)) {
         }
                  if (prime.semantics == PRIME_IS_INTEGER &&
      prime.v.as_integer >= num_devices) {
   printf("Inconsistent value (%d) for DRI_PRIME. Should be < %d "
         "(GPU devices count). Using: %d\n",
               for (i = 0; i < num_devices; i++) {
      if (!(devices[i]->available_nodes & 1 << DRM_NODE_RENDER))
                     /* three formats of DRI_PRIME are supported:
   * "N": a >= 1 integer value. Select the Nth GPU, skipping the
   *      default one.
   * id_path_tag: (for example "pci-0000_02_00_0") choose the card
   * with this id_path_tag.
   * vendor_id:device_id
   */
   switch (prime.semantics) {
      case PRIME_IS_INTEGER: {
      /* Skip the default device */
   if (drm_device_matches_tag(devices[i], default_tag)) {
      log_(debug ? _LOADER_WARNING : _LOADER_INFO,
                           /* Skip more GPUs? */
   if (prime.v.as_integer) {
      log_(debug ? _LOADER_WARNING : _LOADER_INFO,
            }
   log_(debug ? _LOADER_WARNING : _LOADER_INFO, " -> ");
      }
   case PRIME_IS_VID_DID: {
      if (devices[i]->bustype == DRM_BUS_PCI &&
      devices[i]->deviceinfo.pci->vendor_id == prime.v.as_vendor_device_ids.v &&
   devices[i]->deviceinfo.pci->device_id == prime.v.as_vendor_device_ids.d) {
   /* Update prime for the "different_device"
   * determination below. */
   free(prime.str);
   prime.str = drm_construct_id_path_tag(devices[i]);
   log_(debug ? _LOADER_WARNING : _LOADER_INFO,
            } else {
      log_(debug ? _LOADER_WARNING : _LOADER_INFO,
      }
      }
   case PRIME_IS_PCI_TAG: {
      if (!drm_device_matches_tag(devices[i], prime.str)) {
      log_(debug ? _LOADER_WARNING : _LOADER_INFO,
            }
   log_(debug ? _LOADER_WARNING : _LOADER_INFO, " - pci tag match -> ");
                  log_(debug ? _LOADER_WARNING : _LOADER_INFO,
         fd = loader_open_device(devices[i]->nodes[DRM_NODE_RENDER]);
      }
            if (i == num_devices)
            if (fd < 0) {
      log_(debug ? _LOADER_WARNING : _LOADER_INFO,
                              bool is_render_and_display_gpu_diff = !!strcmp(default_tag, prime.str);
   if (original_fd) {
      if (is_render_and_display_gpu_diff) {
      *original_fd = *fd_render_gpu;
      } else {
      *original_fd = *fd_render_gpu;
         } else {
      close(*fd_render_gpu);
               free(default_tag);
   free(prime.str);
      err:
      log_(debug ? _LOADER_WARNING : _LOADER_INFO,
         free(default_tag);
      no_prime_gpu_offloading:
      if (original_fd)
            }
      static bool
   drm_get_pci_id_for_fd(int fd, int *vendor_id, int *chip_id)
   {
               if (drmGetDevice2(fd, 0, &device) != 0) {
      log_(_LOADER_WARNING, "MESA-LOADER: failed to retrieve device information\n");
               if (device->bustype != DRM_BUS_PCI) {
      drmFreeDevice(&device);
   log_(_LOADER_DEBUG, "MESA-LOADER: device is not located on the PCI bus\n");
               *vendor_id = device->deviceinfo.pci->vendor_id;
   *chip_id = device->deviceinfo.pci->device_id;
   drmFreeDevice(&device);
      }
      #ifdef __linux__
   static int loader_get_linux_pci_field(int maj, int min, const char *field)
   {
      char path[PATH_MAX + 1];
            char *field_str = os_read_file(path, NULL);
   if (!field_str) {
      /* Probably non-PCI device. */
               int value = (int)strtoll(field_str, NULL, 16);
               }
      static bool
   loader_get_linux_pci_id_for_fd(int fd, int *vendor_id, int *chip_id)
   {
      struct stat sbuf;
   if (fstat(fd, &sbuf) != 0) {
      log_(_LOADER_DEBUG, "MESA-LOADER: failed to fstat fd\n");
               int maj = major(sbuf.st_rdev);
            *vendor_id = loader_get_linux_pci_field(maj, min, "vendor");
               }
   #endif /* __linux__ */
      bool
   loader_get_pci_id_for_fd(int fd, int *vendor_id, int *chip_id)
   {
   #ifdef __linux__
      /* Implementation without causing full enumeration of DRM devices. */
   if (loader_get_linux_pci_id_for_fd(fd, vendor_id, chip_id))
      #endif
            }
      char *
   loader_get_device_name_for_fd(int fd)
   {
         }
      static char *
   loader_get_pci_driver(int fd)
   {
      int vendor_id, chip_id, i, j;
            if (!loader_get_pci_id_for_fd(fd, &vendor_id, &chip_id))
            for (i = 0; i < ARRAY_SIZE(driver_map); i++) {
      if (vendor_id != driver_map[i].vendor_id)
            if (driver_map[i].predicate && !driver_map[i].predicate(fd))
            if (driver_map[i].num_chips_ids == -1) {
      driver = strdup(driver_map[i].driver);
               for (j = 0; j < driver_map[i].num_chips_ids; j++)
      if (driver_map[i].chip_ids[j] == chip_id) {
      driver = strdup(driver_map[i].driver);
            out:
      log_(driver ? _LOADER_DEBUG : _LOADER_WARNING,
         "pci id for fd %d: %04x:%04x, driver %s\n",
      }
      char *
   loader_get_driver_for_fd(int fd)
   {
               /* Allow an environment variable to force choosing a different driver
   * binary.  If that driver binary can't survive on this FD, that's the
   * user's problem, but this allows vc4 simulator to run on an i965 host,
   * and may be useful for some touch testing of i915 on an i965 host.
   */
   if (__normal_user()) {
      const char *override = os_get_option("MESA_LOADER_DRIVER_OVERRIDE");
   if (override)
            #if defined(USE_DRICONF)
      driver = loader_get_dri_config_driver(fd);
   if (driver)
      #endif
         driver = loader_get_pci_driver(fd);
   if (!driver)
               }
      void
   loader_set_logger(loader_logger *logger)
   {
         }
      char *
   loader_get_extensions_name(const char *driver_name)
   {
               if (asprintf(&name, "%s_%s", __DRI_DRIVER_GET_EXTENSIONS, driver_name) < 0)
            const size_t len = strlen(name);
   for (size_t i = 0; i < len; i++) {
      if (name[i] == '-')
                  }
      bool
   loader_bind_extensions(void *data,
               {
               for (size_t j = 0; j < num_matches; j++) {
      const struct dri_extension_match *match = &matches[j];
   const __DRIextension **field = (const __DRIextension **)((char *)data + matches[j].offset);
   for (size_t i = 0; extensions[i]; i++) {
      if (strcmp(extensions[i]->name, match->name) == 0 &&
      extensions[i]->version >= match->version) {
   *field = extensions[i];
                  if (!*field) {
      log_(match->optional ? _LOADER_DEBUG : _LOADER_FATAL, "did not find extension %s version %d\n",
         if (!match->optional)
                     /* The loaders rely on the loaded DRI drivers being from the same Mesa
   * build so that we can reference the same structs on both sides.
   */
   if (strcmp(match->name, __DRI_MESA) == 0) {
      const __DRImesaCoreExtension *mesa = (const __DRImesaCoreExtension *)*field;
   if (strcmp(mesa->version_string, MESA_INTERFACE_VERSION_STRING) != 0) {
      log_(_LOADER_FATAL, "DRI driver not from this Mesa build ('%s' vs '%s')\n",
                              }
   /**
   * Opens a driver or backend using its name, returning the library handle.
   *
   * \param driverName - a name like "i965", "radeon", "nouveau", etc.
   * \param lib_suffix - a suffix to append to the driver name to generate the
   * full library name.
   * \param search_path_vars - NULL-terminated list of env vars that can be used
   * \param default_search_path - a colon-separted list of directories used if
   * search_path_vars is NULL or none of the vars are set in the environment.
   * \param warn_on_fail - Log a warning if the driver is not found.
   */
   void *
   loader_open_driver_lib(const char *driver_name,
                           {
      char path[PATH_MAX];
            search_paths = NULL;
   if (__normal_user() && search_path_vars) {
      for (int i = 0; search_path_vars[i] != NULL; i++) {
      search_paths = getenv(search_path_vars[i]);
   if (search_paths)
         }
   if (search_paths == NULL)
            void *driver = NULL;
   const char *dl_error = NULL;
   end = search_paths + strlen(search_paths);
   for (const char *p = search_paths; p < end; p = next + 1) {
      int len;
   next = strchr(p, ':');
   if (next == NULL)
            len = next - p;
   snprintf(path, sizeof(path), "%.*s/tls/%s%s.so", len,
         driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
   if (driver == NULL) {
      snprintf(path, sizeof(path), "%.*s/%s%s.so", len,
         driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
   if (driver == NULL) {
      dl_error = dlerror();
   log_(_LOADER_DEBUG, "MESA-LOADER: failed to open %s: %s\n",
         }
   /* not need continue to loop all paths once the driver is found */
   if (driver != NULL)
               if (driver == NULL) {
      if (warn_on_fail) {
      log_(_LOADER_WARNING,
      "MESA-LOADER: failed to open %s: %s (search paths %s, suffix %s)\n",
   }
                           }
      /**
   * Opens a DRI driver using its driver name, returning the __DRIextension
   * entrypoints.
   *
   * \param driverName - a name like "i965", "radeon", "nouveau", etc.
   * \param out_driver - Address where the dlopen() return value will be stored.
   * \param search_path_vars - NULL-terminated list of env vars that can be used
   * to override the DEFAULT_DRIVER_DIR search path.
   */
   const struct __DRIextensionRec **
   loader_open_driver(const char *driver_name,
               {
      char *get_extensions_name;
   const struct __DRIextensionRec **extensions = NULL;
   const struct __DRIextensionRec **(*get_extensions)(void);
   void *driver = loader_open_driver_lib(driver_name, "_dri", search_path_vars,
            if (!driver)
            get_extensions_name = loader_get_extensions_name(driver_name);
   if (get_extensions_name) {
      get_extensions = dlsym(driver, get_extensions_name);
   if (get_extensions) {
         } else {
      log_(_LOADER_DEBUG, "MESA-LOADER: driver does not expose %s(): %s\n",
      }
               if (extensions == NULL) {
      log_(_LOADER_WARNING,
         dlclose(driver);
            failed:
      *out_driver_handle = driver;
      }
