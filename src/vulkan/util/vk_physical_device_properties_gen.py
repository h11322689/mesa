   COPYRIGHT=u"""
   /* Copyright © 2021 Intel Corporation
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
   from collections import OrderedDict
   from dataclasses import dataclass
   import os
   import sys
   import typing
   import xml.etree.ElementTree as et
   import re
      import mako
   from mako.template import Template
      from vk_extensions import get_all_required, filter_api
      def str_removeprefix(s, prefix):
      if s.startswith(prefix):
               RENAMED_PROPERTIES = {
      ("DrmPropertiesEXT", "hasPrimary"): "drmHasPrimary",
   ("DrmPropertiesEXT", "primaryMajor"): "drmPrimaryMajor",
   ("DrmPropertiesEXT", "primaryMinor"): "drmPrimaryMinor",
   ("DrmPropertiesEXT", "hasRender"): "drmHasRender",
   ("DrmPropertiesEXT", "renderMajor"): "drmRenderMajor",
   ("DrmPropertiesEXT", "renderMinor"): "drmRenderMinor",
   ("SparseProperties", "residencyStandard2DBlockShape"): "sparseResidencyStandard2DBlockShape",
   ("SparseProperties", "residencyStandard2DMultisampleBlockShape"): "sparseResidencyStandard2DMultisampleBlockShape",
   ("SparseProperties", "residencyStandard3DBlockShape"): "sparseResidencyStandard3DBlockShape",
   ("SparseProperties", "residencyAlignedMipSize"): "sparseResidencyAlignedMipSize",
   ("SparseProperties", "residencyNonResidentStrict"): "sparseResidencyNonResidentStrict",
   ("SubgroupProperties", "supportedStages"): "subgroupSupportedStages",
   ("SubgroupProperties", "supportedOperations"): "subgroupSupportedOperations",
      }
      SPECIALIZED_PROPERTY_STRUCTS = [
         ]
      @dataclass
   class Property:
      decl: str
   name: str
   actual_name: str
            def __init__(self, p, property_struct_name):
      self.decl = ""
   for element in p:
         if element.tag != "comment":
                  self.name = p.find("./name").text
            length = p.attrib.get("len", "1")
               @dataclass
   class PropertyStruct:
      c_type: str
   s_type: str
   name: str
         def copy_property(dst, src, decl, length="1"):
               if "[" in decl:
         else:
         TEMPLATE_H = Template(COPYRIGHT + """
   /* This file generated from ${filename}, don"t edit directly. */
   #ifndef VK_PROPERTIES_H
   #define VK_PROPERTIES_H
      #ifdef __cplusplus
   extern "C" {
   #endif
      struct vk_properties {
   % for prop in all_properties:
         % endfor
   };
      #ifdef __cplusplus
   }
   #endif
      #endif
   """, output_encoding="utf-8")
      TEMPLATE_C = Template(COPYRIGHT + """
   /* This file generated from ${filename}, don"t edit directly. */
      #include "vk_common_entrypoints.h"
   #include "vk_log.h"
   #include "vk_physical_device.h"
   #include "vk_physical_device_properties.h"
   #include "vk_util.h"
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,
         {
            % for prop in pdev_properties:
         % endfor
         vk_foreach_struct(ext, pProperties) {
      % for property_struct in property_structs:
   % if property_struct.name not in SPECIALIZED_PROPERTY_STRUCTS:
         case ${property_struct.s_type}: {
   % for prop in property_struct.properties:
         % endfor
               % endif
   % endfor
                     case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_PROPERTIES_EXT: {
               if (properties->pCopySrcLayouts) {
      uint32_t written_layout_count = MIN2(properties->copySrcLayoutCount,
         memcpy(properties->pCopySrcLayouts, pdevice->properties.pCopySrcLayouts,
            } else {
                  if (properties->pCopyDstLayouts) {
      uint32_t written_layout_count = MIN2(properties->copyDstLayoutCount,
         memcpy(properties->pCopyDstLayouts, pdevice->properties.pCopyDstLayouts,
            } else {
                  memcpy(properties->optimalTilingLayoutUUID, pdevice->properties.optimalTilingLayoutUUID, VK_UUID_SIZE);
   properties->identicalMemoryTypeRequirements = pdevice->properties.identicalMemoryTypeRequirements;
               default:
               }
   """, output_encoding="utf-8")
      def get_pdev_properties(doc, struct_name):
      _type = doc.find(".types/type[@name=\"VkPhysicalDevice%s\"]" % struct_name)
   if _type is not None:
      properties = []
   for p in _type.findall("./member"):
                  def filter_api(elem, api):
      if "api" not in elem.attrib:
                  def get_property_structs(doc, api, beta):
                        # parse all struct types where structextends VkPhysicalDeviceProperties2
   for _type in doc.findall("./types/type[@category=\"struct\"]"):
      if _type.attrib.get("structextends") != "VkPhysicalDeviceProperties2":
            full_name = _type.attrib["name"]
   if full_name not in required:
            # Skip extensions with a define for now
   guard = required[full_name].guard
   if guard is not None and (guard != "VK_ENABLE_BETA_EXTENSIONS" or beta != "true"):
            # find Vulkan structure type
   for elem in _type:
                           # collect a list of properties
            for p in _type.findall("./member"):
                        m_name = p.find("./name").text
   if m_name == "pNext":
         elif m_name == "sType":
                  property_struct = PropertyStruct(c_type=full_name, s_type=s_type, name=name, properties=properties)
               def get_property_structs_from_xml(xml_files, beta, api="vulkan"):
               pdev_properties = None
            for filename in xml_files:
      doc = et.parse(filename)
   property_structs += get_property_structs(doc, api, beta)
   if not pdev_properties:
                        limits = get_pdev_properties(doc, "Limits")
   for limit in limits:
                  sparse_properties = get_pdev_properties(doc, "SparseProperties")
   for prop in sparse_properties:
         # Gather all properties, make sure that aliased declarations match up.
   property_names = OrderedDict()
   all_properties = []
   for prop in pdev_properties:
      property_names[prop.actual_name] = prop
         for property_struct in property_structs:
      for prop in property_struct.properties:
         if prop.actual_name not in property_names:
      property_names[prop.actual_name] = prop
                     def main():
      parser = argparse.ArgumentParser()
   parser.add_argument("--out-c", required=True, help="Output C file.")
   parser.add_argument("--out-h", required=True, help="Output H file.")
   parser.add_argument("--beta", required=True, help="Enable beta extensions.")
   parser.add_argument("--xml",
                                 environment = {
      "filename": os.path.basename(__file__),
   "pdev_properties": pdev_properties,
   "property_structs": property_structs,
   "all_properties": all_properties,
   "copy_property": copy_property,
               try:
      with open(args.out_c, "wb") as f:
         with open(args.out_h, "wb") as f:
      except Exception:
      # In the event there"s an error, this uses some helpers from mako
   # to print a useful stack trace and prints it, then exits with
   # status 1, if python is run with debug; otherwise it just raises
   # the exception
   print(mako.exceptions.text_error_template().render(), file=sys.stderr)
      if __name__ == "__main__":
      main()
