   #
   # Copyright (C) 2014 Connor Abbott
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
   # Authors:
   #    Connor Abbott (cwabbott0@gmail.com)
      from nir_opcodes import opcodes, type_sizes
   from mako.template import Template
      template = Template("""
   #include "nir.h"
      nir_op
   nir_type_conversion_op(nir_alu_type src, nir_alu_type dst, nir_rounding_mode rnd)
   {
      nir_alu_type src_base = (nir_alu_type) nir_alu_type_get_base_type(src);
   nir_alu_type dst_base = (nir_alu_type) nir_alu_type_get_base_type(dst);
   unsigned src_bit_size = nir_alu_type_get_type_size(src);
            if (src == dst && src_base == nir_type_float) {
         } else if (src == dst && src_base == nir_type_bool) {
         } else if ((src_base == nir_type_int || src_base == nir_type_uint) &&
            (dst_base == nir_type_int || dst_base == nir_type_uint) &&
   /* Integer <-> integer conversions with the same bit-size on both
   * ends are just no-op moves.
   */
               /* f2b, i2b, and u2b do not exist.  Use ine or fne (via nir_type_conversion)
   * instead.
   */
               %     for src_t in ['int', 'uint', 'float', 'bool']:
         case nir_type_${src_t}:
   %           for dst_t in ['int', 'uint', 'float', 'bool']:
         %              if src_t in ['int', 'uint'] and dst_t in ['int', 'uint']:
   %                 if dst_t == 'int':
   <%                   continue %>
   %                 else:
   <%                   dst_t = src_t %>
   %                 endif
   %              elif src_t == 'bool' and dst_t in ['int', 'uint', 'bool']:
   %                 if dst_t == 'int':
   <%                   continue %>
   %                 else:
   <%                   dst_t = 'int' %>
   %                 endif
   %              elif src_t != 'bool' and dst_t == 'bool':
   <%                   continue %>
   %              endif
         %                 for dst_bits in type_sizes(dst_t):
         %                    if src_t == 'float' and dst_t == 'float' and dst_bits == 16:
         %                       for rnd_t in [('rtne', '_rtne'), ('rtz', '_rtz'), ('undef', '')]:
                     %                       endfor
                     %                    else:
               %                    endif
   %                 endfor
                     %           endfor
               default:
   %     endfor
         default:
         }
      const nir_op_info nir_op_infos[nir_num_opcodes] = {
   % for name, opcode in sorted(opcodes.items()):
   {
      .name = "${name}",
   .num_inputs = ${opcode.num_inputs},
   .output_size = ${opcode.output_size},
   .output_type = ${"nir_type_" + opcode.output_type},
   .input_sizes = {
         },
   .input_types = {
         },
   .is_conversion = ${"true" if opcode.is_conversion else "false"},
   .algebraic_properties =
      ${ "0" if opcode.algebraic_properties == "" else " | ".join(
         },
   % endfor
   };
   """)
      print(template.render(opcodes=opcodes, type_sizes=type_sizes))
