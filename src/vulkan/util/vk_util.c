   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   * Copyright © 2017 Intel Corporation
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include "vk_util.h"
   #include "util/u_debug.h"
      #include "compiler/spirv/nir_spirv.h"
      uint32_t vk_get_driver_version(void)
   {
      const char *minor_string = strchr(PACKAGE_VERSION, '.');
   const char *patch_string = minor_string ? strchr(minor_string + 1, '.') : NULL;
   int major = atoi(PACKAGE_VERSION);
   int minor = minor_string ? atoi(minor_string + 1) : 0;
   int patch = patch_string ? atoi(patch_string + 1) : 0;
   if (strstr(PACKAGE_VERSION, "devel")) {
      if (patch == 0) {
      patch = 99;
   if (minor == 0) {
      minor = 99;
      } else
      } else
      }
      }
      uint32_t vk_get_version_override(void)
   {
      const char *str = getenv("MESA_VK_VERSION_OVERRIDE");
   if (str == NULL)
            const char *minor_str = strchr(str, '.');
            int major = atoi(str);
   int minor = minor_str ? atoi(minor_str + 1) : 0;
            /* Do some basic version sanity checking */
   if (major < 1 || minor < 0 || patch < 0 || minor > 1023 || patch > 4095)
               }
      void
   vk_warn_non_conformant_implementation(const char *driver_name)
   {
      if (debug_get_bool_option("MESA_VK_IGNORE_CONFORMANCE_WARNING", false))
            fprintf(stderr, "WARNING: %s is not a conformant Vulkan implementation, "
      }
      struct nir_spirv_specialization*
   vk_spec_info_to_nir_spirv(const VkSpecializationInfo *spec_info,
         {
      if (spec_info == NULL || spec_info->mapEntryCount == 0)
            uint32_t num_spec_entries = spec_info->mapEntryCount;
   struct nir_spirv_specialization *spec_entries =
            for (uint32_t i = 0; i < num_spec_entries; i++) {
      VkSpecializationMapEntry entry = spec_info->pMapEntries[i];
   const void *data = (uint8_t *)spec_info->pData + entry.offset;
   assert((uint8_t *)data + entry.size <=
            spec_entries[i].id = spec_info->pMapEntries[i].constantID;
   switch (entry.size) {
   case 8:
      spec_entries[i].value.u64 = *(const uint64_t *)data;
      case 4:
      spec_entries[i].value.u32 = *(const uint32_t *)data;
      case 2:
      spec_entries[i].value.u16 = *(const uint16_t *)data;
      case 1:
      spec_entries[i].value.u8 = *(const uint8_t *)data;
      case 0:
   default:
      /* The Vulkan spec says:
   *
   *    "For a constantID specialization constant declared in a
   *    shader, size must match the byte size of the constantID. If
   *    the specialization constant is of type boolean, size must be
   *    the byte size of VkBool32."
   *
   * Therefore, since only scalars can be decorated as
   * specialization constants, we can assume that if it doesn't have
   * a size of 1, 2, 4, or 8, any use in a shader would be invalid
   * usage.  The spec further says:
   *
   *    "If a constantID value is not a specialization constant ID
   *    used in the shader, that map entry does not affect the
   *    behavior of the pipeline."
   *
   * so we should ignore any invalid specialization constants rather
   * than crash or error out when we see one.
   */
                  *out_num_spec_entries = num_spec_entries;
      }
