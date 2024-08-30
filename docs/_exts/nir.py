   # Copyright © 2021 Intel Corporation
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the
   # "Software"), to deal in the Software without restriction, including
   # without limitation the rights to use, copy, modify, merge, publish,
   # distribute, sub license, and/or sell copies of the Software, and to
   # permit persons to whom the Software is furnished to do so, subject to
   # the following conditions:
   #
   # The above copyright notice and this permission notice (including the
   # next paragraph) shall be included in all copies or substantial portions
   # of the Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   # OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   # MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   # IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   # ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   # TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   # SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
      import docutils.nodes
   import mako.template
   import os
   import sphinx
   from sphinx.directives import SphinxDirective
   from sphinx.domains import Domain
   from sphinx.util.nodes import make_refnode
   import sys
   import textwrap
      THIS_DIR = os.path.dirname(os.path.abspath(__file__))
   MESA_DIR = os.path.join(THIS_DIR, '..', '..')
   NIR_PATH = os.path.join(MESA_DIR, 'src', 'compiler', 'nir')
   sys.path.append(NIR_PATH)
      import nir_opcodes
      OP_DESC_TEMPLATE = mako.template.Template("""
   <%
   def src_decl_list(num_srcs):
            def to_yn(b):
         %>
      **Properties:**
      .. list-table::
               * - Per-component
   - Associative
   - 2-src commutative
   * - ${to_yn(op.output_size == 0)}
   - ${to_yn('associative' in op.algebraic_properties)}
         ${("**Description:** " + op.description) if op.description != "" else ""}
      **Constant-folding:**
      .. code-block:: c
      ${textwrap.indent(op.const_expr, '    ')}
      **Builder function:**
      .. c:function:: nir_def *nir_${op.name}(nir_builder *, ${src_decl_list(op.num_inputs)})
   """)
      def parse_rst(state, parent, rst):
      vl = docutils.statemachine.ViewList(rst.splitlines())
         def nir_alu_type_name(t, s):
      if s:
         else:
         def build_alu_op_desc(state, env, op):
               # Add the signature
   sig = sphinx.addnodes.desc_signature()
   desc.append(sig)
            params = sphinx.addnodes.desc_parameterlist()
   for i, t, s in zip(range(100), op.input_types, op.input_sizes):
      params += docutils.nodes.Text(nir_alu_type_name(t, s) + ' ')
               sig += sphinx.addnodes.desc_returns('',
            nir_domain = env.get_domain('nir')
            # Build the description
   content = sphinx.addnodes.desc_content()
   desc.append(content)
                  class NIRALUOpcodesDirective(SphinxDirective):
      def run(self):
      return [build_alu_op_desc(self.state, self.env, op)
      class NIRDomain(Domain):
               To list all NIR ALU opcodes with their descriptions:
   ```rst
   .. nir:alu-opcodes::
            To reference a NIR opcode, ``:nir:alu-op:`fadd```
   """
   name = 'nir'
   roles = {
         }
   directives = {
         }
   initial_data = {
                  def add_alu_op_ref(self, op):
      """Add reference to an ALU op."""
   self.data['alu-op-refs'].append((op.name, self.env.docname))
         def resolve_xref(self, env, fromdocname, builder, typ, target, node,
            for opname, todocname in self.data['alu-op-refs']:
         if target == opname:
                     def setup(app):
      app.add_domain(NIRDomain)
