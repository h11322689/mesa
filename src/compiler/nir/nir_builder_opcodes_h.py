   template = """\
   /* Copyright (C) 2015 Broadcom
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
      #ifndef _NIR_BUILDER_OPCODES_
   #define _NIR_BUILDER_OPCODES_
      <%
   def src_decl_list(num_srcs):
            def src_list(num_srcs):
            def needs_num_components(opcode):
            def intrinsic_prefix(name):
      if name in build_prefixed_intrinsics:
         else:
      %>
      % for name, opcode in sorted(opcodes.items()):
   % if not needs_num_components(opcode):
   static inline nir_def *
   nir_${name}(nir_builder *build, ${src_decl_list(opcode.num_inputs)})
   {
   % if opcode.is_conversion and \
      type_base_type(opcode.output_type) == opcode.input_types[0]:
   if (src0->bit_size == ${type_size(opcode.output_type)})
      %endif
   % if opcode.num_inputs <= 4:
         % else:
      nir_def *srcs[${opcode.num_inputs}] = {${src_list(opcode.num_inputs)}};
      % endif
   }
   % endif
   % endfor
      % for name, opcode in sorted(INTR_OPCODES.items()):
   % if opcode.indices:
   struct _nir_${name}_indices {
         % for index in opcode.indices:
         % endfor
   };
   % endif
   % endfor
      <%
   def intrinsic_decl_list(opcode):
      need_components = opcode.dest_components == 0 and \
            res = ''
   if (opcode.has_dest or opcode.num_srcs) and need_components:
         if opcode.has_dest and len(opcode.bit_sizes) != 1 and opcode.bit_size_src == -1:
         for i in range(opcode.num_srcs):
         if opcode.indices:
               def intrinsic_macro_list(opcode):
      need_components = opcode.dest_components == 0 and \
            res = ''
   if (opcode.has_dest or opcode.num_srcs) and need_components:
         if opcode.has_dest and len(opcode.bit_sizes) != 1 and opcode.bit_size_src == -1:
         for i in range(opcode.num_srcs):
               def get_intrinsic_bitsize(opcode):
      if len(opcode.bit_sizes) == 1:
         elif opcode.bit_size_src != -1:
         else:
      %>
      % for name, opcode in sorted(INTR_OPCODES.items()):
   % if opcode.has_dest:
   static inline nir_def *
   % else:
   static inline nir_intrinsic_instr *
   % endif
   _nir_build_${name}(nir_builder *build${intrinsic_decl_list(opcode)})
   {
      nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
            % if 0 in opcode.src_components:
   intrin->num_components = src${opcode.src_components.index(0)}->num_components;
   % elif opcode.dest_components == 0:
   intrin->num_components = num_components;
   % endif
   % if opcode.has_dest:
      % if opcode.dest_components == 0:
   nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, ${get_intrinsic_bitsize(opcode)});
   % else:
   nir_def_init(&intrin->instr, &intrin->def, ${opcode.dest_components}, ${get_intrinsic_bitsize(opcode)});
      % endif
   % for i in range(opcode.num_srcs):
   intrin->src[${i}] = nir_src_for_ssa(src${i});
   % endfor
   % if WRITE_MASK in opcode.indices and 0 in opcode.src_components:
   if (!indices.write_mask)
         % endif
   % if ALIGN_MUL in opcode.indices and 0 in opcode.src_components:
   if (!indices.align_mul)
         % elif ALIGN_MUL in opcode.indices and opcode.dest_components == 0:
   if (!indices.align_mul)
         % endif
   % for index in opcode.indices:
   nir_intrinsic_set_${index.name}(intrin, indices.${index.name});
            nir_builder_instr_insert(build, &intrin->instr);
   % if opcode.has_dest:
   return &intrin->def;
   % else:
   return intrin;
      }
   % endfor
      % for name, opcode in sorted(INTR_OPCODES.items()):
   % if opcode.indices:
   #ifdef __cplusplus
   #define ${intrinsic_prefix(name)}_${name}(build${intrinsic_macro_list(opcode)}, ...) ${'\\\\'}
   _nir_build_${name}(build${intrinsic_macro_list(opcode)}, _nir_${name}_indices{0, __VA_ARGS__})
   #else
   #define ${intrinsic_prefix(name)}_${name}(build${intrinsic_macro_list(opcode)}, ...) ${'\\\\'}
   _nir_build_${name}(build${intrinsic_macro_list(opcode)}, (struct _nir_${name}_indices){0, __VA_ARGS__})
   #endif
   % else:
   #define nir_${name} _nir_build_${name}
   % endif
   % if name in build_prefixed_intrinsics:
   #define nir_${name} nir_build_${name}
   % endif
   % endfor
      % for name in ['flt', 'fge', 'feq', 'fneu']:
   static inline nir_def *
   nir_${name}_imm(nir_builder *build, nir_def *src1, double src2)
   {
         }
   % endfor
      % for name in ['ilt', 'ige', 'ieq', 'ine', 'ult', 'uge']:
   static inline nir_def *
   nir_${name}_imm(nir_builder *build, nir_def *src1, uint64_t src2)
   {
         }
   % endfor
      % for prefix in ['i', 'u']:
   static inline nir_def *
   nir_${prefix}gt_imm(nir_builder *build, nir_def *src1, uint64_t src2)
   {
         }
      static inline nir_def *
   nir_${prefix}le_imm(nir_builder *build, nir_def *src1, uint64_t src2)
   {
         }
   % endfor
      #endif /* _NIR_BUILDER_OPCODES_ */"""
      from nir_opcodes import opcodes, type_size, type_base_type
   from nir_intrinsics import INTR_OPCODES, WRITE_MASK, ALIGN_MUL
   from mako.template import Template
      # List of intrinsics that also need a nir_build_ prefixed factory macro.
   build_prefixed_intrinsics = [
      "load_deref",
   "store_deref",
   "copy_deref",
                     "load_global",
   "load_global_constant",
            "load_reg",
               ]
      print(Template(template).render(opcodes=opcodes,
                                 type_size=type_size,
   type_base_type=type_base_type,
   INTR_OPCODES=INTR_OPCODES,
   WRITE_MASK=WRITE_MASK,
   ALIGN_MUL=ALIGN_MUL,
   build_prefixed_intrinsics=build_prefixed_intrinsics))
