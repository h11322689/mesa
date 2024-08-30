   COPYRIGHT=u"""
   /* Copyright © 2022 Collabora, Ltd.
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
   """
      import argparse
   import os
   import re
   from collections import namedtuple
   import xml.etree.ElementTree as et
      from mako.template import Template
      TEMPLATE_H = Template(COPYRIGHT + """\
   /* This file generated from ${filename}, don't edit directly. */
      #ifndef VK_FORMAT_INFO_H
   #define VK_FORMAT_INFO_H
      #include <vulkan/vulkan_core.h>
      #ifdef __cplusplus
   extern "C" {
   #endif
      enum vk_format_class {
         % for name in format_classes:
         % endfor
   };
      struct vk_format_class_info {
      const VkFormat *formats;
      };
      const struct vk_format_class_info *
   vk_format_class_get_info(enum vk_format_class class);
      const struct vk_format_class_info *
   vk_format_get_class_info(VkFormat format);
      #ifdef __cplusplus
   }
   #endif
      #endif
   """, output_encoding='utf-8')
      TEMPLATE_C = Template(COPYRIGHT + """
   /* This file generated from ${filename}, don't edit directly. */
      #include "${header}"
      #include "util/macros.h"
      #include "vk_format.h"
      struct vk_format_info {
         };
      % for id, ext in extensions.items():
   static const struct vk_format_info ext${id}_format_infos[] = {
   %   for name, format in ext.formats.items():
      [${format.offset}] = {
            %   endfor
   };
      % endfor
   static const struct vk_format_info *
   vk_format_get_info(VkFormat format)
   {
      uint32_t extnumber =
                     % for id, ext in extensions.items():
      case ${id}:
      assert(offset < ARRAY_SIZE(ext${id}_format_infos));
   % endfor
      default:
            }
      % for clsname, cls in format_classes.items():
   %   if len(cls.formats) > 0:
   static const VkFormat ${to_enum_name('MESA_VK_FORMAT_CLASS_', clsname).lower() + '_formats'}[] = {
   %     for fname in cls.formats:
         %     endfor
   %   endif
   };
      % endfor
   static const struct vk_format_class_info class_infos[] = {
   % for clsname, cls in format_classes.items():
         %   if len(cls.formats) > 0:
         .formats = ${to_enum_name('MESA_VK_FORMAT_CLASS_', clsname).lower() + '_formats'},
   %   else:
         %   endif
         % endfor
   };
      const struct vk_format_class_info *
   vk_format_class_get_info(enum vk_format_class class)
   {
      assert(class < ARRAY_SIZE(class_infos));
      }
      const struct vk_format_class_info *
   vk_format_get_class_info(VkFormat format)
   {
      const struct vk_format_info *format_info = vk_format_get_info(format);
      }
   """, output_encoding='utf-8')
      def to_enum_name(prefix, name):
            Format = namedtuple('Format', ['name', 'cls', 'ext', 'offset'])
   FormatClass = namedtuple('FormatClass', ['name', 'formats'])
   Extension = namedtuple('Extension', ['id', 'formats'])
      def get_formats(doc):
      """Extract the formats from the registry."""
            for fmt in doc.findall('./formats/format'):
      xpath = './/enum[@name="{}"]'.format(fmt.attrib['name'])
   enum = doc.find(xpath)
   ext = None
   if 'extends' in enum.attrib:
         assert(enum.attrib['extends'] == 'VkFormat')
   if 'extnumber' in enum.attrib:
         else:
      xpath = xpath + '/..'
   parent = doc.find(xpath)
   while parent != None and ext == None:
      if parent.tag == 'extension':
         assert('number' in parent.attrib)
   xpath = xpath + '/..'
   else:
                  assert(ext != None)
   format = Format(fmt.attrib['name'], fmt.attrib['class'], ext, offset)
               def get_formats_from_xml(xml_files):
               for filename in xml_files:
      doc = et.parse(filename)
               def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--out-c', required=True, help='Output C file.')
   parser.add_argument('--out-h', required=True, help='Output H file.')
   parser.add_argument('--xml',
                        formats = get_formats_from_xml(args.xml_files)
   classes = {}
   extensions = {}
   for n, f in formats.items():
      if f.cls not in classes:
         classes[f.cls].formats[f.name] = f
   if f.ext not in extensions:
                        environment = {
      'header': os.path.basename(args.out_h),
   'formats': formats,
   'format_classes': classes,
   'extensions': extensions,
   'filename': os.path.basename(__file__),
               try:
      with open(args.out_h, 'wb') as f:
         guard = os.path.basename(args.out_h).replace('.', '_').upper()
   with open(args.out_c, 'wb') as f:
      except Exception:
      # In the event there's an error, this imports some helpers from mako
   # to print a useful stack trace and prints it, then exits with
   # status 1, if python is run with debug; otherwise it just raises
   # the exception
   import sys
   from mako import exceptions
   print(exceptions.text_error_template().render(), file=sys.stderr)
      if __name__ == '__main__':
      main()
