   COPYRIGHT = """\
   /*
   * Copyright 2020 Intel Corporation
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
   import math
   import os
      from mako.template import Template
      # Mesa-local imports must be declared in meson variable
   # '{file_without_suffix}_depend_files'.
   from vk_entrypoints import get_entrypoints_from_xml
      # We generate a static hash table for entry point lookup
   # (vkGetProcAddress). We use a linear congruential generator for our hash
   # function and a power-of-two size table. The prime numbers are determined
   # experimentally.
      TEMPLATE_H = Template(COPYRIGHT + """\
   /* This file generated from ${filename}, don't edit directly. */
      #ifndef VK_DISPATCH_TABLE_H
   #define VK_DISPATCH_TABLE_H
      #include "vulkan/vulkan.h"
   #include "vulkan/vk_android_native_buffer.h"
      #include "vk_extensions.h"
      /* Windows api conflict */
   #ifdef _WIN32
   #include <windows.h>
   #ifdef CreateSemaphore
   #undef CreateSemaphore
   #endif
   #ifdef CreateEvent
   #undef CreateEvent
   #endif
   #endif
      #ifdef __cplusplus
   extern "C" {
   #endif
      #ifdef _MSC_VER
   VKAPI_ATTR void VKAPI_CALL vk_entrypoint_stub(void);
   #endif
      <%def name="dispatch_table(entrypoints)">
   % for e in entrypoints:
   % if e.alias:
         % endif
   % if e.guard is not None:
   #ifdef ${e.guard}
   % endif
   % if e.aliases:
      union {
      PFN_vk${e.name} ${e.name};
   % for a in e.aliases:
   PFN_vk${a.name} ${a.name};
         % else:
         % endif
   % if e.guard is not None:
   #else
      % if e.aliases:
   union {
      PFN_vkVoidFunction ${e.name};
   % for a in e.aliases:
   PFN_vkVoidFunction ${a.name};
      };
   % else:
   PFN_vkVoidFunction ${e.name};
      #endif
   % endif
   % endfor
   </%def>
      <%def name="entrypoint_table(type, entrypoints)">
   struct vk_${type}_entrypoint_table {
   % for e in entrypoints:
   % if e.guard is not None:
   #ifdef ${e.guard}
   % endif
         % if e.guard is not None:
   #else
         # endif
   % endif
   % endfor
   };
   </%def>
      struct vk_instance_dispatch_table {
   ${dispatch_table(instance_entrypoints)}
   };
      struct vk_physical_device_dispatch_table {
   ${dispatch_table(physical_device_entrypoints)}
   };
      struct vk_device_dispatch_table {
   ${dispatch_table(device_entrypoints)}
   };
      struct vk_dispatch_table {
      union {
      struct {
         struct vk_instance_dispatch_table instance;
   struct vk_physical_device_dispatch_table physical_device;
            struct {
         ${dispatch_table(instance_entrypoints)}
   ${dispatch_table(physical_device_entrypoints)}
         };
      ${entrypoint_table('instance', instance_entrypoints)}
   ${entrypoint_table('physical_device', physical_device_entrypoints)}
   ${entrypoint_table('device', device_entrypoints)}
      void
   vk_instance_dispatch_table_load(struct vk_instance_dispatch_table *table,
               void
   vk_physical_device_dispatch_table_load(struct vk_physical_device_dispatch_table *table,
               void
   vk_device_dispatch_table_load(struct vk_device_dispatch_table *table,
                  void vk_instance_dispatch_table_from_entrypoints(
      struct vk_instance_dispatch_table *dispatch_table,
   const struct vk_instance_entrypoint_table *entrypoint_table,
         void vk_physical_device_dispatch_table_from_entrypoints(
      struct vk_physical_device_dispatch_table *dispatch_table,
   const struct vk_physical_device_entrypoint_table *entrypoint_table,
         void vk_device_dispatch_table_from_entrypoints(
      struct vk_device_dispatch_table *dispatch_table,
   const struct vk_device_entrypoint_table *entrypoint_table,
         PFN_vkVoidFunction
   vk_instance_dispatch_table_get(const struct vk_instance_dispatch_table *table,
            PFN_vkVoidFunction
   vk_physical_device_dispatch_table_get(const struct vk_physical_device_dispatch_table *table,
            PFN_vkVoidFunction
   vk_device_dispatch_table_get(const struct vk_device_dispatch_table *table,
            PFN_vkVoidFunction
   vk_instance_dispatch_table_get_if_supported(
      const struct vk_instance_dispatch_table *table,
   const char *name,
   uint32_t core_version,
         PFN_vkVoidFunction
   vk_physical_device_dispatch_table_get_if_supported(
      const struct vk_physical_device_dispatch_table *table,
   const char *name,
   uint32_t core_version,
         PFN_vkVoidFunction
   vk_device_dispatch_table_get_if_supported(
      const struct vk_device_dispatch_table *table,
   const char *name,
   uint32_t core_version,
   const struct vk_instance_extension_table *instance_exts,
         #ifdef __cplusplus
   }
   #endif
      #endif /* VK_DISPATCH_TABLE_H */
   """)
      TEMPLATE_C = Template(COPYRIGHT + """\
   /* This file generated from ${filename}, don't edit directly. */
      #include "vk_dispatch_table.h"
      #include "util/macros.h"
   #include "string.h"
      <%def name="load_dispatch_table(type, VkType, ProcAddr, entrypoints)">
   void
   vk_${type}_dispatch_table_load(struct vk_${type}_dispatch_table *table,
               {
   % if type != 'physical_device':
         % endif
   % for e in entrypoints:
   % if e.alias or e.name == '${ProcAddr}':
         % endif
   % if e.guard is not None:
   #ifdef ${e.guard}
   % endif
         % for a in e.aliases:
      if (table->${e.name} == NULL) {
            % endfor
   % if e.guard is not None:
   #endif
   % endif
   % endfor
   }
   </%def>
      ${load_dispatch_table('instance', 'VkInstance', 'GetInstanceProcAddr',
            ${load_dispatch_table('physical_device', 'VkInstance', 'GetInstanceProcAddr',
            ${load_dispatch_table('device', 'VkDevice', 'GetDeviceProcAddr',
               struct string_map_entry {
      uint32_t name;
   uint32_t hash;
      };
      /* We use a big string constant to avoid lots of reloctions from the entry
   * point table to lots of little strings. The entries in the entry point table
   * store the index into this big string.
   */
      <%def name="strmap(strmap, prefix)">
   static const char ${prefix}_strings[] =
   % for s in strmap.sorted_strings:
         % endfor
   ;
      static const struct string_map_entry ${prefix}_string_map_entries[] = {
   % for s in strmap.sorted_strings:
         % endfor
   };
      /* Hash table stats:
   * size ${len(strmap.sorted_strings)} entries
   * collisions entries:
   % for i in range(10):
   *     ${i}${'+' if i == 9 else ' '}     ${strmap.collisions[i]}
   % endfor
   */
      #define none 0xffff
   static const uint16_t ${prefix}_string_map[${strmap.hash_size}] = {
   % for e in strmap.mapping:
         % endfor
   };
      static int
   ${prefix}_string_map_lookup(const char *str)
   {
      static const uint32_t prime_factor = ${strmap.prime_factor};
   static const uint32_t prime_step = ${strmap.prime_step};
   const struct string_map_entry *e;
   uint32_t hash, h;
   uint16_t i;
            hash = 0;
   for (p = str; *p; p++)
            h = hash;
   while (1) {
      i = ${prefix}_string_map[h & ${strmap.hash_mask}];
   if (i == none)
         e = &${prefix}_string_map_entries[i];
   if (e->hash == hash && strcmp(str, ${prefix}_strings + e->name) == 0)
                        }
   </%def>
      ${strmap(instance_strmap, 'instance')}
   ${strmap(physical_device_strmap, 'physical_device')}
   ${strmap(device_strmap, 'device')}
      <% assert len(instance_entrypoints) < 2**8 %>
   static const uint8_t instance_compaction_table[] = {
   % for e in instance_entrypoints:
         % endfor
   };
      <% assert len(physical_device_entrypoints) < 2**8 %>
   static const uint8_t physical_device_compaction_table[] = {
   % for e in physical_device_entrypoints:
         % endfor
   };
      <% assert len(device_entrypoints) < 2**16 %>
   static const uint16_t device_compaction_table[] = {
   % for e in device_entrypoints:
         % endfor
   };
      static bool
   vk_instance_entrypoint_is_enabled(int index, uint32_t core_version,
         {
         % for e in instance_entrypoints:
      case ${e.entry_table_index}:
         % if e.core_version:
         % elif e.extensions:
   % for ext in e.extensions:
      % if ext.type == 'instance':
   if (instance->${ext.name[3:]}) return true;
   % else:
   /* All device extensions are considered enabled at the instance level */
   return true;
      % endfor
         % else:
            % endfor
      default:
            }
      /** Return true if the core version or extension in which the given entrypoint
   * is defined is enabled.
   *
   * If device is NULL, all device extensions are considered enabled.
   */
   static bool
   vk_physical_device_entrypoint_is_enabled(int index, uint32_t core_version,
         {
         % for e in physical_device_entrypoints:
      case ${e.entry_table_index}:
         % if e.core_version:
         % elif e.extensions:
   % for ext in e.extensions:
      % if ext.type == 'instance':
   if (instance->${ext.name[3:]}) return true;
   % else:
   /* All device extensions are considered enabled at the instance level */
   return true;
      % endfor
         % else:
            % endfor
      default:
            }
      /** Return true if the core version or extension in which the given entrypoint
   * is defined is enabled.
   *
   * If device is NULL, all device extensions are considered enabled.
   */
   static bool
   vk_device_entrypoint_is_enabled(int index, uint32_t core_version,
               {
         % for e in device_entrypoints:
      case ${e.entry_table_index}:
         % if e.core_version:
         % elif e.extensions:
   % for ext in e.extensions:
      % if ext.type == 'instance':
   if (instance->${ext.name[3:]}) return true;
   % else:
   if (!device || device->${ext.name[3:]}) return true;
      % endfor
         % else:
            % endfor
      default:
            }
      #ifdef _MSC_VER
   VKAPI_ATTR void VKAPI_CALL vk_entrypoint_stub(void)
   {
         }
      static const void *get_function_target(const void *func)
   {
         #ifdef _M_X64
      /* Incremental linking may indirect through relative jump */
   if (*address == 0xE9)
   {
      /* Compute JMP target if the first byte is opcode 0xE9 */
   uint32_t offset;
   memcpy(&offset, address + 1, 4);
         #else
         #endif
         }
      static bool vk_function_is_stub(PFN_vkVoidFunction func)
   {
         }
   #endif
      <%def name="dispatch_table_from_entrypoints(type)">
   void vk_${type}_dispatch_table_from_entrypoints(
      struct vk_${type}_dispatch_table *dispatch_table,
   const struct vk_${type}_entrypoint_table *entrypoint_table,
      {
      PFN_vkVoidFunction *disp = (PFN_vkVoidFunction *)dispatch_table;
            if (overwrite) {
      memset(dispatch_table, 0, sizeof(*dispatch_table));
   #ifdef _MSC_VER
               #else
         #endif
                     unsigned disp_index = ${type}_compaction_table[i];
   assert(disp[disp_index] == NULL);
      } else {
      for (unsigned i = 0; i < ARRAY_SIZE(${type}_compaction_table); i++) {
   #ifdef _MSC_VER
               #else
         #endif
                     }
   </%def>
      ${dispatch_table_from_entrypoints('instance')}
   ${dispatch_table_from_entrypoints('physical_device')}
   ${dispatch_table_from_entrypoints('device')}
      <%def name="lookup_funcs(type)">
   static PFN_vkVoidFunction
   vk_${type}_dispatch_table_get_for_entry_index(
         {
      assert(entry_index < ARRAY_SIZE(${type}_compaction_table));
   int disp_index = ${type}_compaction_table[entry_index];
      }
      PFN_vkVoidFunction
   vk_${type}_dispatch_table_get(
         {
      int entry_index = ${type}_string_map_lookup(name);
   if (entry_index < 0)
               }
   </%def>
      ${lookup_funcs('instance')}
   ${lookup_funcs('physical_device')}
   ${lookup_funcs('device')}
      PFN_vkVoidFunction
   vk_instance_dispatch_table_get_if_supported(
      const struct vk_instance_dispatch_table *table,
   const char *name,
   uint32_t core_version,
      {
      int entry_index = instance_string_map_lookup(name);
   if (entry_index < 0)
            if (!vk_instance_entrypoint_is_enabled(entry_index, core_version,
                     }
      PFN_vkVoidFunction
   vk_physical_device_dispatch_table_get_if_supported(
      const struct vk_physical_device_dispatch_table *table,
   const char *name,
   uint32_t core_version,
      {
      int entry_index = physical_device_string_map_lookup(name);
   if (entry_index < 0)
            if (!vk_physical_device_entrypoint_is_enabled(entry_index, core_version,
                     }
      PFN_vkVoidFunction
   vk_device_dispatch_table_get_if_supported(
      const struct vk_device_dispatch_table *table,
   const char *name,
   uint32_t core_version,
   const struct vk_instance_extension_table *instance_exts,
      {
      int entry_index = device_string_map_lookup(name);
   if (entry_index < 0)
            if (!vk_device_entrypoint_is_enabled(entry_index, core_version,
                     }
   """)
      U32_MASK = 2**32 - 1
      PRIME_FACTOR = 5024183
   PRIME_STEP = 19
      class StringIntMapEntry:
      def __init__(self, string, num):
      self.string = string
            # Calculate the same hash value that we will calculate in C.
   h = 0
   for c in string:
                     def round_to_pow2(x):
            class StringIntMap:
      def __init__(self):
      self.baked = False
         def add_string(self, string, num):
      assert not self.baked
   assert string not in self.strings
   assert 0 <= num < 2**31
         def bake(self):
      self.sorted_strings = \
         offset = 0
   for entry in self.sorted_strings:
                  # Save off some values that we'll need in C
   self.hash_size = round_to_pow2(len(self.strings) * 1.25)
   self.hash_mask = self.hash_size - 1
   self.prime_factor = PRIME_FACTOR
            self.mapping = [-1] * self.hash_size
   self.collisions = [0] * 10
   for idx, s in enumerate(self.sorted_strings):
         level = 0
   h = s.hash
   while self.mapping[h & self.hash_mask] >= 0:
      h = h + PRIME_STEP
         def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--out-c', help='Output C file.')
   parser.add_argument('--out-h', help='Output H file.')
   parser.add_argument('--beta', required=True, help='Enable beta extensions.')
   parser.add_argument('--xml',
                                             device_entrypoints = []
   physical_device_entrypoints = []
   instance_entrypoints = []
   for e in entrypoints:
      if e.is_device_entrypoint():
         elif e.is_physical_device_entrypoint():
         else:
         for i, e in enumerate(e for e in device_entrypoints if not e.alias):
            device_strmap = StringIntMap()
   for i, e in enumerate(device_entrypoints):
      e.entry_table_index = i
               for i, e in enumerate(e for e in physical_device_entrypoints if not e.alias):
            physical_device_strmap = StringIntMap()
   for i, e in enumerate(physical_device_entrypoints):
      e.entry_table_index = i
               for i, e in enumerate(e for e in instance_entrypoints if not e.alias):
            instance_strmap = StringIntMap()
   for i, e in enumerate(instance_entrypoints):
      e.entry_table_index = i
               # For outputting entrypoints.h we generate a anv_EntryPoint() prototype
   # per entry point.
   try:
      if args.out_h:
         with open(args.out_h, 'w') as f:
      f.write(TEMPLATE_H.render(instance_entrypoints=instance_entrypoints,
            if args.out_c:
         with open(args.out_c, 'w') as f:
      f.write(TEMPLATE_C.render(instance_entrypoints=instance_entrypoints,
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
