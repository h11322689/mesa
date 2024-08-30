   # Copyright © 2020 Hoe Hao Cheng
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # the rights to use, copy, modify, merge, publish, distribute, sublicense,
   # and/or sell copies of the Software, and to permit persons to whom the
   # Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   # THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
   # 
      import re
   from xml.etree import ElementTree
   from typing import List,Tuple
      class Version:
      device_version = (1,0,0)
            def __init__(self, version, struct=()):
               if not struct:
         else:
         # e.g. "VK_MAKE_VERSION(1,2,0)"
   def version(self):
      return ("VK_MAKE_VERSION("
            + str(self.device_version[0])
   + ","
   + str(self.device_version[1])
            # e.g. "10"
   def struct(self):
            # the sType of the extension's struct
   # e.g. VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT
   # for VK_EXT_transform_feedback and struct="FEATURES"
   def stype(self, struct: str):
      return ("VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_"
            class Extension:
      name           = None
   alias          = None
   is_required    = False
   is_nonstandard = False
   enable_conds   = None
            # these are specific to zink_device_info.py:
   has_properties      = False
   has_features        = False
   guard               = False
   features_promoted   = False
   properties_promoted = False
            # these are specific to zink_instance.py:
            def __init__(self, name, alias="", required=False, nonstandard=False,
            self.name = name
   self.alias = alias
   self.is_required = required
   self.is_nonstandard = nonstandard
   self.has_properties = properties
   self.has_features = features
   self.enable_conds = conditions
            if alias == "" and (properties == True or features == True):
         # e.g.: "VK_EXT_robustness2" -> "robustness2"
   def pure_name(self):
            # e.g.: "VK_EXT_robustness2" -> "EXT_robustness2"
   def name_with_vendor(self):
            # e.g.: "VK_EXT_robustness2" -> "Robustness2"
   def name_in_camel_case(self):
            # e.g.: "VK_EXT_robustness2" -> "VK_EXT_ROBUSTNESS_2"
   def name_in_snake_uppercase(self):
      def replace(original):
                                                                     if match_alphanumeric is not None:
                           replaced = list(map(replace, self.name.split('_')))
         # e.g.: "VK_EXT_robustness2" -> "ROBUSTNESS_2"
   def pure_name_in_snake_uppercase(self):
            # e.g.: "VK_EXT_robustness2" -> "VK_EXT_ROBUSTNESS_2_EXTENSION_NAME"
   def extension_name(self):
            # generate a C string literal for the extension
   def extension_name_literal(self):
            # get the field in zink_device_info that refers to the extension's
   # feature/properties struct
   # e.g. rb2_<suffix> for VK_EXT_robustness2
   def field(self, suffix: str):
            def physical_device_struct(self, struct: str):
      if self.name_in_camel_case().endswith(struct):
            return ("VkPhysicalDevice"
                     # the sType of the extension's struct
   # e.g. VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT
   # for VK_EXT_transform_feedback and struct="FEATURES"
   def stype(self, struct: str):
      return ("VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_" 
                     # e.g. EXT in VK_EXT_robustness2
   def vendor(self):
         # Type aliases
   Layer = Extension
      class ExtensionRegistryEntry:
      # type of extension - right now it's either "instance" or "device"
   ext_type          = ""
   # the version in which the extension is promoted to core VK
   promoted_in       = None
   # functions added by the extension are referred to as "commands" in the registry
   device_commands   = None
   pdevice_commands  = None
   instance_commands = None
   constants         = None
   features_struct   = None
   features_fields   = None
   features_promoted = False
   properties_struct = None
   properties_fields = None
   properties_promoted = False
   # some instance extensions are locked behind certain platforms
         class ExtensionRegistry:
      # key = extension name, value = registry entry
            def __init__(self, vkxml_path: str):
               commands_type = dict()
   command_aliases = dict()
   platform_guards = dict()
            for cmd in vkxml.findall("commands/command"):
                  if name is not None and name.text:
                  for typ in vkxml.findall("types/type"):
                                                for (cmd, alias) in command_aliases.items():
            for platform in vkxml.findall("platforms/platform"):
         name = platform.get("name")
            for ext in vkxml.findall("extensions/extension"):
         # Reserved extensions are marked with `supported="disabled"`
                           entry = ExtensionRegistryEntry()
                  entry.device_commands = []
   entry.pdevice_commands = []
   entry.instance_commands = []
                  for cmd in ext.findall("require/command"):
      cmd_name = cmd.get("name")
   if cmd_name:
      if commands_type[cmd_name] in ("VkDevice", "VkCommandBuffer", "VkQueue"):
         elif commands_type[cmd_name] in ("VkPhysicalDevice"):
                  entry.constants = []
   for enum in ext.findall("require/enum"):
      enum_name = enum.get("name")
   enum_extends = enum.get("extends")
   # we are only interested in VK_*_EXTENSION_NAME, which does not
                     for ty in ext.findall("require/type"):
      ty_name = ty.get("name")
   if (self.is_features_struct(ty_name) and
      entry.features_struct is None):
   entry.features_struct = ty_name
                        if entry.features_struct:
      struct_name = entry.features_struct
                        elif entry.promoted_in is not None:
      # if the extension is promoted but a core-Vulkan alias is not
                     for field in vkxml.findall("./types/type[@name='{}']/member".format(struct_name)):
      field_name = field.find("name").text
                        if entry.properties_struct:
      struct_name = entry.properties_struct
                        elif entry.promoted_in is not None:
      # if the extension is promoted but a core-Vulkan alias is not
   # available for the properties, then it is not promoted to core
                                                            def in_registry(self, ext_name: str):
            def get_registry_entry(self, ext_name: str):
      if self.in_registry(ext_name):
         # Parses e.g. "VK_VERSION_x_y" to integer tuple (x, y)
   # For any erroneous inputs, None is returned
   def parse_promotedto(self, promotedto: str):
               if promotedto and promotedto.startswith("VK_VERSION_"):
                        def is_features_struct(self, struct: str):
            def is_properties_struct(self, struct: str):
      return re.match(r"VkPhysicalDevice.*Properties.*", struct) is not None
