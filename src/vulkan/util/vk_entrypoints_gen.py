   COPYRIGHT=u"""
   /* Copyright © 2015-2021 Intel Corporation
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
      from mako.template import Template
      # Mesa-local imports must be declared in meson variable
   # '{file_without_suffix}_depend_files'.
   from vk_entrypoints import get_entrypoints_from_xml
      TEMPLATE_H = Template(COPYRIGHT + """\
   /* This file generated from ${filename}, don't edit directly. */
      #include "vk_dispatch_table.h"
      % for i in includes:
   #include "${i}"
   % endfor
      #ifndef ${guard}
   #define ${guard}
      % if not tmpl_prefix:
   #ifdef __cplusplus
   extern "C" {
   #endif
   % endif
      /* clang wants function declarations in the header to have weak attribute */
   #if !defined(_MSC_VER) && !defined(__MINGW32__)
   #define ATTR_WEAK __attribute__ ((weak))
   #else
   #define ATTR_WEAK
   #endif
      % for p in instance_prefixes:
   extern const struct vk_instance_entrypoint_table ${p}_instance_entrypoints;
   % endfor
      % for p in physical_device_prefixes:
   extern const struct vk_physical_device_entrypoint_table ${p}_physical_device_entrypoints;
   % endfor
      % for p in device_prefixes:
   extern const struct vk_device_entrypoint_table ${p}_device_entrypoints;
   % endfor
      % for v in tmpl_variants_sanitized:
   extern const struct vk_device_entrypoint_table ${tmpl_prefix}_device_entrypoints_${v};
   % endfor
      % if gen_proto:
   % for e in instance_entrypoints:
   % if e.guard is not None:
   #ifdef ${e.guard}
   % endif
   % for p in physical_device_prefixes:
   VKAPI_ATTR ${e.return_type} VKAPI_CALL ${p}_${e.name}(${e.decl_params()});
   % endfor
   % if e.guard is not None:
   #endif // ${e.guard}
   % endif
   % endfor
      % for e in physical_device_entrypoints:
   % if e.guard is not None:
   #ifdef ${e.guard}
   % endif
   % for p in physical_device_prefixes:
   VKAPI_ATTR ${e.return_type} VKAPI_CALL ${p}_${e.name}(${e.decl_params()});
   % endfor
   % if e.guard is not None:
   #endif // ${e.guard}
   % endif
   % endfor
      % for e in device_entrypoints:
   % if e.guard is not None:
   #ifdef ${e.guard}
   % endif
   % for p in device_prefixes:
   VKAPI_ATTR ${e.return_type} VKAPI_CALL ${p}_${e.name}(${e.decl_params()}) ATTR_WEAK;
   % endfor
      % if tmpl_prefix:
   template <${tmpl_param}>
   VKAPI_ATTR ${e.return_type} VKAPI_CALL ${tmpl_prefix}_${e.name}(${e.decl_params()});
      #define ${tmpl_prefix}_${e.name}_GENS(X) \
   template VKAPI_ATTR ${e.return_type} VKAPI_CALL ${tmpl_prefix}_${e.name}<X>(${e.decl_params()});
   % endif
      % if e.guard is not None:
   #endif // ${e.guard}
   % endif
   % endfor
   % endif
      % if not tmpl_prefix:
   #ifdef __cplusplus
   }
   #endif
   % endif
      #endif /* ${guard} */
   """)
      TEMPLATE_C = Template(COPYRIGHT + """
   /* This file generated from ${filename}, don't edit directly. */
      #include "${header}"
      /* Weak aliases for all potential implementations. These will resolve to
   * NULL if they're not defined, which lets the resolve_entrypoint() function
   * either pick the correct entry point.
   *
   * MSVC uses different decorated names for 32-bit versus 64-bit. Declare
   * all argument sizes for 32-bit because computing the actual size would be
   * difficult.
   */
      <%def name="entrypoint_table(type, entrypoints, prefixes)">
   % if gen_weak:
   % for e in entrypoints:
         #ifdef ${e.guard}
      % endif
      #ifdef _MSC_VER
   #ifdef _M_IX86
            #pragma comment(linker, "/alternatename:_${p}_${e.name}@${args_size}=_vk_entrypoint_stub@0")
      #else
         #if defined(_M_ARM64EC)
         #endif
   #endif
   #else
               % if entrypoints == device_entrypoints:
         extern template
   VKAPI_ATTR __attribute__ ((weak)) ${e.return_type} VKAPI_CALL ${tmpl_prefix}_${e.name}${v}(${e.decl_params()});
            #endif
                  #endif // ${e.guard}
         % endfor
   % endif
      % for p in prefixes:
   const struct vk_${type}_entrypoint_table ${p}_${type}_entrypoints = {
   % for e in entrypoints:
         #ifdef ${e.guard}
      % endif
   .${e.name} = ${p}_${e.name},
      #elif defined(_MSC_VER)
         #endif // ${e.guard}
         % endfor
   };
   % endfor
      % if entrypoints == device_entrypoints:
   % for v, entrypoint_v in zip(tmpl_variants, tmpl_variants_sanitized):
   const struct vk_${type}_entrypoint_table ${tmpl_prefix}_${type}_entrypoints_${entrypoint_v} = {
   % for e in entrypoints:
         #ifdef ${e.guard}
      % endif
   .${e.name} = ${tmpl_prefix}_${e.name}${v},
      #elif defined(_MSC_VER)
         #endif // ${e.guard}
         % endfor
   };
   % endfor
   % endif
   </%def>
      ${entrypoint_table('instance', instance_entrypoints, instance_prefixes)}
   ${entrypoint_table('physical_device', physical_device_entrypoints, physical_device_prefixes)}
   ${entrypoint_table('device', device_entrypoints, device_prefixes)}
   """)
         def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--out-c', required=True, help='Output C file.')
   parser.add_argument('--out-h', required=True, help='Output H file.')
   parser.add_argument('--beta', required=True, help='Enable beta extensions.')
   parser.add_argument('--xml',
               parser.add_argument('--proto', help='Generate entrypoint prototypes',
         parser.add_argument('--weak', help='Generate weak entrypoint declarations',
         parser.add_argument('--prefix',
               parser.add_argument('--device-prefix',
               parser.add_argument('--include',
               parser.add_argument('--tmpl-prefix',
               parser.add_argument('--tmpl-param',
               parser.add_argument('--tmpl-variants',
                        instance_prefixes = args.prefixes
   physical_device_prefixes = args.prefixes
            tmpl_variants_sanitized = [
                     device_entrypoints = []
   physical_device_entrypoints = []
   instance_entrypoints = []
   for e in entrypoints:
      if e.is_device_entrypoint():
         elif e.is_physical_device_entrypoint():
         else:
                  environment = {
      'gen_proto': args.gen_proto,
   'gen_weak': args.gen_weak,
   'header': os.path.basename(args.out_h),
   'instance_entrypoints': instance_entrypoints,
   'instance_prefixes': instance_prefixes,
   'physical_device_entrypoints': physical_device_entrypoints,
   'physical_device_prefixes': physical_device_prefixes,
   'device_entrypoints': device_entrypoints,
   'device_prefixes': device_prefixes,
   'includes': args.includes,
   'tmpl_prefix': args.tmpl_prefix,
   'tmpl_param': args.tmpl_param,
   'tmpl_variants': args.tmpl_variants,
   'tmpl_variants_sanitized': tmpl_variants_sanitized,
               # For outputting entrypoints.h we generate a anv_EntryPoint() prototype
   # per entry point.
   try:
      with open(args.out_h, 'w', encoding='utf-8') as f:
         guard = os.path.basename(args.out_h).replace('.', '_').upper()
   with open(args.out_c, 'w', encoding='utf-8') as f:
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
