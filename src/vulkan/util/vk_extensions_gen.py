   COPYRIGHT = """\
   /*
   * Copyright 2017 Intel Corporation
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
   */
   """
      import argparse
      from mako.template import Template
      # Mesa-local imports must be declared in meson variable
   # '{file_without_suffix}_depend_files'.
   from vk_extensions import get_all_exts_from_xml, init_exts_from_xml
      _TEMPLATE_H = Template(COPYRIGHT + """
      #ifndef ${driver.upper()}_EXTENSIONS_H
   #define ${driver.upper()}_EXTENSIONS_H
      #include <stdbool.h>
      %if driver == 'vk':
      <%def name="extension_table(type, extensions)">
   #define VK_${type.upper()}_EXTENSION_COUNT ${len(extensions)}
      extern const VkExtensionProperties vk_${type}_extensions[];
      struct vk_${type}_extension_table {
      union {
      bool extensions[VK_${type.upper()}_EXTENSION_COUNT];
   %for ext in extensions:
         %endfor
                  /* Workaround for "error: too many initializers for vk_${type}_extension_table" */
   %for ext in extensions:
         %endfor
               };
   </%def>
      ${extension_table('instance', instance_extensions)}
   ${extension_table('device', device_extensions)}
      %else:
   #include "vk_extensions.h"
   %endif
      struct ${driver}_physical_device;
      %if driver == 'vk':
   #ifdef ANDROID
   extern const struct vk_instance_extension_table vk_android_allowed_instance_extensions;
   extern const struct vk_device_extension_table vk_android_allowed_device_extensions;
   #endif
   %else:
   extern const struct vk_instance_extension_table ${driver}_instance_extensions_supported;
      void
   ${driver}_physical_device_get_supported_extensions(const struct ${driver}_physical_device *device,
         %endif
      #endif /* ${driver.upper()}_EXTENSIONS_H */
   """)
      _TEMPLATE_C = Template(COPYRIGHT + """
   #include "vulkan/vulkan_core.h"
   %if driver != 'vk':
   #include "${driver}_private.h"
   %endif
      #include "${driver}_extensions.h"
      %if driver == 'vk':
   const VkExtensionProperties ${driver}_instance_extensions[${driver.upper()}_INSTANCE_EXTENSION_COUNT] = {
   %for ext in instance_extensions:
         %endfor
   };
      const VkExtensionProperties ${driver}_device_extensions[${driver.upper()}_DEVICE_EXTENSION_COUNT] = {
   %for ext in device_extensions:
         %endfor
   };
      #ifdef ANDROID
   const struct vk_instance_extension_table vk_android_allowed_instance_extensions = {
   %for ext in instance_extensions:
         %endfor
   };
      const struct vk_device_extension_table vk_android_allowed_device_extensions = {
   %for ext in device_extensions:
         %endfor
   };
   #endif
   %endif
      %if driver != 'vk':
   #include "vk_util.h"
      /* Convert the VK_USE_PLATFORM_* defines to booleans */
   %for platform_define in platform_defines:
   #ifdef ${platform_define}
   #   undef ${platform_define}
   #   define ${platform_define} true
   #else
   #   define ${platform_define} false
   #endif
   %endfor
      /* And ANDROID too */
   #ifdef ANDROID
   #   undef ANDROID
   #   define ANDROID true
   #else
   #   define ANDROID false
   #   define ANDROID_API_LEVEL 0
   #endif
      #define ${driver.upper()}_HAS_SURFACE (VK_USE_PLATFORM_WIN32_KHR || \\
                              static const uint32_t MAX_API_VERSION = ${MAX_API_VERSION.c_vk_version()};
      VKAPI_ATTR VkResult VKAPI_CALL ${driver}_EnumerateInstanceVersion(
         {
      *pApiVersion = MAX_API_VERSION;
      }
      const struct vk_instance_extension_table ${driver}_instance_extensions_supported = {
   %for ext in instance_extensions:
         %endfor
   };
      uint32_t
   ${driver}_physical_device_api_version(struct ${driver}_physical_device *device)
   {
               uint32_t override = vk_get_version_override();
   if (override)
         %for version in API_VERSIONS:
      if (!(${version.enable}))
               %endfor
         }
      void
   ${driver}_physical_device_get_supported_extensions(const struct ${driver}_physical_device *device,
         {
         %for ext in device_extensions:
         %endfor
         }
   %endif
   """)
      def gen_extensions(driver, xml_files, api_versions, max_api_version,
            platform_defines = []
   for filename in xml_files:
            for ext in extensions:
            template_env = {
      'driver': driver,
   'API_VERSIONS': api_versions,
   'MAX_API_VERSION': max_api_version,
   'instance_extensions': [e for e in extensions if e.type == 'instance'],
   'device_extensions': [e for e in extensions if e.type == 'device'],
               if out_h:
      with open(out_h, 'w') as f:
         if out_c:
      with open(out_c, 'w') as f:
         def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--out-c', help='Output C file.')
   parser.add_argument('--out-h', help='Output H file.')
   parser.add_argument('--xml',
                                    extensions = []
   for filename in args.xml_files:
            gen_extensions('vk', args.xml_files, None, None,
         if __name__ == '__main__':
      main()
