   COPYRIGHT = """\
   /*
   * Copyright 2021 Intel Corporation
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
   import os
      from grl_parser import parse_grl_file
   from mako.template import Template
      TEMPLATE_H = Template(COPYRIGHT + """
   /* This file generated from ${filename}, don't edit directly. */
      #ifndef GRL_CL_KERNEL_H
   #define GRL_CL_KERNEL_H
      #include "genxml/gen_macros.h"
   #include "compiler/brw_kernel.h"
      #ifdef __cplusplus
   extern "C" {
   #endif
      enum grl_cl_kernel {
   % for k in kernels:
         % endfor
         };
      const char *genX(grl_cl_kernel_name)(enum grl_cl_kernel kernel);
      const char *genX(grl_get_cl_kernel_sha1)(enum grl_cl_kernel id);
      void genX(grl_get_cl_kernel)(struct brw_kernel *kernel, enum grl_cl_kernel id);
      #ifdef __cplusplus
   } /* extern "C" */
   #endif
      #endif /* INTEL_GRL_H */
   """, output_encoding='utf-8')
      TEMPLATE_C = Template(COPYRIGHT + """
   /* This file generated from ${filename}, don't edit directly. */
      #include "grl_cl_kernel.h"
      % for k in kernels:
   #include "${prefix}_${k}.h"
   % endfor
      const char *
   genX(grl_cl_kernel_name)(enum grl_cl_kernel kernel)
   {
         % for k in kernels:
         % endfor
      default: return "unknown";
      }
      const char *
   genX(grl_get_cl_kernel_sha1)(enum grl_cl_kernel id)
   {
         % for k in kernels:
         % endfor
      default:
            };
      void
   ${prefix}_grl_get_cl_kernel(struct brw_kernel *kernel, enum grl_cl_kernel id)
   {
         % for k in kernels:
      case GRL_CL_KERNEL_${k.upper()}:
      *kernel = ${prefix}_${k};
   % endfor
      default:
            }
   """, output_encoding='utf-8')
      def get_libraries_files(kernel_module):
      lib_files = []
   for item in kernel_module[3]:
      if item[0] != 'library':
         default_file = None
   fallback_file = None
   path_directory = None
   for props in item[2]:
         if props[0] == 'fallback':
         elif props[0] == 'default':
         elif props[0] == 'path':
   assert path_directory
   assert default_file or fallback_file
   if fallback_file:
         else:
            def add_kernels(kernels, cl_file, entrypoint, libs):
      assert cl_file.endswith('.cl')
   for lib_file in libs:
               def get_kernels(grl_nodes):
      kernels = []
   for item in grl_nodes:
      assert isinstance(item, tuple)
   if item[0] == 'kernel':
         ann = item[2]
   elif item[0] == 'kernel-module':
         cl_file = item[2]
   libfiles = get_libraries_files(item)
   for kernel_def in item[3]:
      if kernel_def[0] == 'kernel':
         def parse_libraries(filenames):
      libraries = {}
   for fname in filenames:
      lib_package = parse_grl_file(fname, [])
   for lib in lib_package:
         assert lib[0] == 'library'
   # Add the directory of the library so that CL files can be found.
            def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--out-c', help='Output C file')
   parser.add_argument('--out-h', help='Output H file')
   parser.add_argument('--ls-kernels', action='store_const', const=True,
         parser.add_argument('--prefix', help='Prefix')
   parser.add_argument('--library', dest='libraries', action='append',
         parser.add_argument('files', type=str, nargs='*', help='GRL files')
                     kernels = []
   for fname in args.files:
            # Make the list of kernels unique and sorted
            if args.ls_kernels:
      for cl_file, entrypoint, libs in kernels:
         if not os.path.isabs(cl_file):
         kernel_c_names = []
   for cl_file, entrypoint, libs in kernels:
      cl_file = os.path.splitext(cl_file)[0]
   cl_file_name = cl_file.replace('/', '_')
         try:
      if args.out_h:
         with open(args.out_h, 'wb') as f:
            if args.out_c:
         with open(args.out_c, 'wb') as f:
      f.write(TEMPLATE_C.render(kernels=kernel_c_names,
   except Exception:
      # In the event there's an error, this imports some helpers from mako
   # to print a useful stack trace and prints it, then exits with
   # status 1, if python is run with debug; otherwise it just raises
   # the exception
   if __debug__:
         import sys
   from mako import exceptions
   sys.stderr.write(exceptions.text_error_template().render() + '\n')
      if __name__ == '__main__':
      main()
