   /**************************************************************************
   *
   * Copyright 2012 Francisco Jerez
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
      #include "pipe_loader_priv.h"
      #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
   #include "util/u_dl.h"
   #include "util/u_file.h"
   #include "util/xmlconfig.h"
   #include "util/driconf.h"
      #include <string.h>
      #ifdef _MSC_VER
   #include <stdlib.h>
   #define PATH_MAX _MAX_PATH
   #endif
      #define MODULE_PREFIX "pipe_"
      static int (*backends[])(struct pipe_loader_device **, int) = {
   #ifdef HAVE_LIBDRM
         #endif
         };
      const driOptionDescription gallium_driconf[] = {
   #include "driinfo_gallium.h"
   };
      int
   pipe_loader_probe(struct pipe_loader_device **devs, int ndev, bool with_zink)
   {
               for (i = 0; i < ARRAY_SIZE(backends); i++)
         #if defined(HAVE_ZINK) && defined(HAVE_LIBDRM)
      if (with_zink)
      #endif
            }
      void
   pipe_loader_release(struct pipe_loader_device **devs, int ndev)
   {
               for (i = 0; i < ndev; i++)
      }
      void
   pipe_loader_base_release(struct pipe_loader_device **dev)
   {
      driDestroyOptionCache(&(*dev)->option_cache);
            FREE(*dev);
      }
      static driOptionDescription *
   merge_driconf(const driOptionDescription *driver_driconf, unsigned driver_count,
         {
      unsigned gallium_count = ARRAY_SIZE(gallium_driconf);
   driOptionDescription *merged = malloc((driver_count + gallium_count) *
         if (!merged) {
      *merged_count = 0;
               if (gallium_count)
         if (driver_count) {
      memcpy(&merged[gallium_count], driver_driconf,
               *merged_count = driver_count + gallium_count;
      }
      /**
   * Ensure that dev->option_cache is initialized appropriately for the driver.
   *
   * This function can be called multiple times.
   *
   * \param dev Device for which options should be loaded.
   */
   static void
   pipe_loader_load_options(struct pipe_loader_device *dev)
   {
      if (dev->option_info.info)
            unsigned driver_count, merged_count;
   const driOptionDescription *driver_driconf =
            const driOptionDescription *merged_driconf =
         driParseOptionInfo(&dev->option_info, merged_driconf, merged_count);
      }
      void
   pipe_loader_config_options(struct pipe_loader_device *dev)
   {
      if (!dev->option_cache.info) {
      driParseConfigFiles(&dev->option_cache, &dev->option_info, 0,
         }
      char *
   pipe_loader_get_driinfo_xml(const char *driver_name)
   {
      unsigned driver_count = 0;
         #ifdef HAVE_LIBDRM
      driver_driconf = pipe_loader_drm_get_driconf_by_name(driver_name,
      #endif
         unsigned merged_count;
   const driOptionDescription *merged_driconf =
                                       }
      struct pipe_screen *
   pipe_loader_create_screen_vk(struct pipe_loader_device *dev, bool sw_vk)
   {
               pipe_loader_load_options(dev);
   config.options_info = &dev->option_info;
               }
      struct pipe_screen *
   pipe_loader_create_screen(struct pipe_loader_device *dev)
   {
         }
      struct util_dl_library *
   pipe_loader_find_module(const char *driver_name,
         {
      struct util_dl_library *lib;
   const char *next;
   char path[PATH_MAX];
            for (next = library_paths; *next; library_paths = next + 1) {
      next = strchrnul(library_paths, ':');
            if (len)
      ret = snprintf(path, sizeof(path), "%.*s/%s%s%s",
            else
                  if (ret > 0 && ret < sizeof(path) && u_file_access(path, 0) != -1) {
      lib = util_dl_open(path);
   if (lib) {
         }
   fprintf(stderr, "ERROR: Failed to load pipe driver at `%s': %s\n",
                     }
