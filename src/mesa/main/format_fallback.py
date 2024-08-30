   COPYRIGHT = """\
   /*
   * Copyright 2017 Google
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
      # stdlib
   import argparse
   from sys import stdout
   from mako.template import Template
      # local
   import format_parser
      def parse_args():
      p = argparse.ArgumentParser()
   p.add_argument("csv")
   p.add_argument("out")
         def get_unorm_to_srgb_map(formats):
               for fmt in formats:
      if fmt.colorspace != 'srgb':
            replacements = [
         ('SRGB', 'RGB'),
   ('SRGB', 'UNORM'),
   ('SRGB8_ALPHA8', 'RGBA'),
   ('SRGB8_ALPHA8', 'RGBA8'),
   ]
   found_unorm_name = False
   for rep in replacements:
                        unorm_name = fmt.name.replace(rep[0], rep[1])
   if unorm_name in names:
                  # Every sRGB format MUST have a UNORM equivalent
      def get_intensity_to_red_map(formats):
               for fmt in formats:
      if str(fmt.swizzle) != 'xxxx':
            i_name = fmt.name
                        TEMPLATE = Template(COPYRIGHT + """
   #include "formats.h"
   #include "util/macros.h"
      /**
   * For an sRGB format, return the corresponding linear color space format.
   * For non-sRGB formats, return the format as-is.
   */
   mesa_format
   _mesa_get_srgb_format_linear(mesa_format format)
   {
         %for unorm, srgb in unorm_to_srgb_map:
      case ${srgb}:
      %endfor
      default:
            }
      /**
   * For an intensity format, return the corresponding red format.  For other
   * formats, return the format as-is.
   */
   mesa_format
   _mesa_get_intensity_format_red(mesa_format format)
   {
         %for i, r in intensity_to_red_map:
      case ${i}:
      %endfor
      default:
            }
   """);
      def main():
                        template_env = {
      'unorm_to_srgb_map': list(get_unorm_to_srgb_map(formats)),
               with open(pargs.out, 'w') as f:
         if __name__ == "__main__":
      main()
