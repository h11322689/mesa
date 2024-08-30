   #!/bin/env python
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
   import os.path
   import re
   import sys
      from grl_parser import parse_grl_file
      class Writer(object):
      def __init__(self, file):
      self._file = file
   self._indent = 0
         def push_indent(self, levels=4):
            def pop_indent(self, levels=4):
            def write(self, s, *fmt):
      if self._new_line:
         self._new_line = False
   if s.endswith('\n'):
         self._new_line = True
   if fmt:
            # Internal Representation
      class Value(object):
      def __init__(self, name=None, zone=None):
      self.name = name
   self._zone = zone
         @property
   def zone(self):
      assert self._zone is not None
         def is_reg(self):
            def c_val(self):
      if not self.name:
         assert self.name
         def c_cpu_val(self):
      assert self.zone == 'cpu'
         def c_gpu_val(self):
      if self.zone == 'gpu':
         else:
      class Constant(Value):
      def __init__(self, value):
      super().__init__(zone='cpu')
         def c_val(self):
      if self.value < 100:
         elif self.value < (1 << 32):
         else:
      class Register(Value):
      def __init__(self, name):
            def is_reg(self):
         class FixedGPR(Register):
      def __init__(self, num):
      super().__init__('REG{}'.format(num))
         def write_c(self, w):
      w.write('UNUSED struct mi_value {} = mi_reserve_gpr(&b, {});\n',
      class GroupSizeRegister(Register):
      def __init__(self, comp):
      super().__init__('DISPATCHDIM_' + 'XYZ'[comp])
      class Member(Value):
      def __init__(self, value, member):
      super().__init__(zone=value.zone)
   self.value = value
         def is_reg(self):
            def c_val(self):
      c_val = self.value.c_val()
   if self.zone == 'gpu':
         assert isinstance(self.value, Register)
   if self.member == 'hi':
         elif self.member == 'lo':
         else:
   else:
      class OffsetOf(Value):
      def __init__(self, mk, expr):
      super().__init__(zone='cpu')
   assert isinstance(expr, tuple) and expr[0] == 'member'
   self.type = mk.m.get_type(expr[1])
         def c_val(self):
         class Scope(object):
      def __init__(self, m, mk, parent):
      self.m = m
   self.mk = mk
   self.parent = parent
         def add_def(self, d, name=None):
      if name is None:
         assert name not in self.defs
         def get_def(self, name):
      if name in self.defs:
         assert self.parent, 'Unknown definition: "{}"'.format(name)
      class Statement(object):
      def __init__(self, srcs=[]):
      assert isinstance(srcs, (list, tuple))
      class SSAStatement(Statement, Value):
               def __init__(self, zone, srcs):
      Statement.__init__(self, srcs)
   Value.__init__(self, None, zone)
   self.c_name = '_tmp{}'.format(SSAStatement._count)
         def c_val(self):
            def write_c_refs(self, w):
      assert self.zone == 'gpu'
   assert self.uses > 0
   if self.uses > 1:
            class Half(SSAStatement):
      def __init__(self, value, half):
      assert half in ('hi', 'lo')
   super().__init__(None, [value])
         @property
   def zone(self):
            def write_c(self, w):
      assert self.half in ('hi', 'lo')
   if self.zone == 'cpu':
         if self.half == 'hi':
      w.write('uint32_t {} = (uint64_t)({}) >> 32;\n',
      else:
         else:
         if self.half == 'hi':
      w.write('struct mi_value {} = mi_value_half({}, true);\n',
      else:
            class Expression(SSAStatement):
      def __init__(self, mk, op, *srcs):
      super().__init__(None, srcs)
         @property
   def zone(self):
      zone = 'cpu'
   for s in self.srcs:
         if s.zone == 'gpu':
         def write_c(self, w):
      if self.zone == 'cpu':
         w.write('uint64_t {} = ', self.c_name)
   c_cpu_vals = [s.c_cpu_val() for s in self.srcs]
   if len(self.srcs) == 1:
         elif len(self.srcs) == 2:
         else:
      assert len(self.srcs) == 3 and op == '?'
               w.write('struct mi_value {} = ', self.c_name)
   if self.op == '~':
         elif self.op == '+':
         w.write('mi_iadd(&b, {}, {});\n',
   elif self.op == '-':
         w.write('mi_isub(&b, {}, {});\n',
   elif self.op == '&':
         w.write('mi_iand(&b, {}, {});\n',
   elif self.op == '|':
         w.write('mi_ior(&b, {}, {});\n',
   elif self.op == '<<':
         if self.srcs[1].zone == 'cpu':
      w.write('mi_ishl_imm(&b, {}, {});\n',
      else:
         elif self.op == '>>':
         if self.srcs[1].zone == 'cpu':
      w.write('mi_ushr_imm(&b, {}, {});\n',
      else:
         elif self.op == '==':
         w.write('mi_ieq(&b, {}, {});\n',
   elif self.op == '<':
         w.write('mi_ult(&b, {}, {});\n',
   elif self.op == '>':
         w.write('mi_ult(&b, {}, {});\n',
   elif self.op == '<=':
         w.write('mi_uge(&b, {}, {});\n',
   else:
            class StoreReg(Statement):
      def __init__(self, mk, reg, value):
      super().__init__([mk.load_value(value)])
   self.reg = mk.parse_value(reg)
         def write_c(self, w):
      value = self.srcs[0]
   w.write('mi_store(&b, {}, {});\n',
      class LoadMem(SSAStatement):
      def __init__(self, mk, bit_size, addr):
      super().__init__('gpu', [mk.load_value(addr)])
         def write_c(self, w):
      addr = self.srcs[0]
   w.write('struct mi_value {} = ', self.c_name)
   if addr.zone == 'cpu':
         w.write('mi_mem{}(anv_address_from_u64({}));\n',
   else:
         assert self.bit_size == 64
   w.write('mi_load_mem64_offset(&b, anv_address_from_u64(0), {});\n',
      class StoreMem(Statement):
      def __init__(self, mk, bit_size, addr, src):
      super().__init__([mk.load_value(addr), mk.load_value(src)])
         def write_c(self, w):
      addr, data = tuple(self.srcs)
   if addr.zone == 'cpu':
         w.write('mi_store(&b, mi_mem{}(anv_address_from_u64({})), {});\n',
   else:
         assert self.bit_size == 64
      class GoTo(Statement):
      def __init__(self, mk, target_id, cond=None, invert=False):
      cond = [mk.load_value(cond)] if cond is not None else []
   super().__init__(cond)
   self.target_id = target_id
   self.invert = invert
         def write_c(self, w):
      # Now that we've parsed the entire metakernel, we can look up the
   # actual target from the id
            if self.srcs:
         cond = self.srcs[0]
   if self.invert:
         else:
   else:
      class GoToTarget(Statement):
      def __init__(self, mk, name):
      super().__init__()
   self.name = name
   self.c_name = '_goto_target_' + name
                  def write_decl(self, w):
      w.write('struct mi_goto_target {} = MI_GOTO_TARGET_INIT;\n',
         def write_c(self, w):
         class Dispatch(Statement):
      def __init__(self, mk, kernel, group_size, args, postsync):
      if group_size is None:
         else:
         srcs += [mk.load_value(a) for a in args]
   super().__init__(srcs)
   self.kernel = mk.m.kernels[kernel]
   self.indirect = group_size is None
         def write_c(self, w):
      w.write('{\n')
            group_size = self.srcs[:3]
   args = self.srcs[3:]
   if not self.indirect:
         w.write('const uint32_t _group_size[3] = {{ {}, {}, {} }};\n',
         else:
            w.write('const struct anv_kernel_arg _args[] = {\n')
   w.push_indent()
   for arg in args:
         w.pop_indent()
            w.write('genX(grl_dispatch)(cmd_buffer, {},\n', self.kernel.c_name)
   w.write('                   {}, ARRAY_SIZE(_args), _args);\n', gs)
   w.pop_indent()
      class SemWait(Statement):
      def __init__(self, scope, wait):
      super().__init__()
      class Control(Statement):
      def __init__(self, scope, wait):
      super().__init__()
         def write_c(self, w):
      w.write('cmd_buffer->state.pending_pipe_bits |=\n')
   w.write('    ANV_PIPE_CS_STALL_BIT |\n')
   w.write('    ANV_PIPE_DATA_CACHE_FLUSH_BIT |\n')
   w.write('    ANV_PIPE_UNTYPED_DATAPORT_CACHE_FLUSH_BIT;\n')
      TYPE_REMAPS = {
      'dword' : 'uint32_t',
      }
      class Module(object):
      def __init__(self, grl_dir, elems):
      assert isinstance(elems[0], tuple)
   assert elems[0][0] == 'module-name'
   self.grl_dir = grl_dir
   self.name = elems[0][1]
   self.kernels = {}
   self.structs = {}
   self.constants = []
   self.metakernels = []
            scope = Scope(self, None, None)
   for e in elems[1:]:
         if e[0] == 'kernel':
      k = Kernel(self, *e[1:])
   assert k.name not in self.kernels
      elif e[0] == 'kernel-module':
      m = KernelModule(self, *e[1:])
   for k in m.kernels:
      assert k.name not in self.kernels
   elif e[0] == 'struct':
      s = Struct(self, *e[1:])
   assert s.name not in self.kernels
      elif e[0] == 'named-constant':
      c = NamedConstant(*e[1:])
   scope.add_def(c)
      elif e[0] == 'meta-kernel':
      mk = MetaKernel(self, scope, *e[1:])
      elif e[0] == 'import':
      assert e[2] == 'struct'
            def import_struct(self, filename, struct_name):
      elems = parse_grl_file(os.path.join(self.grl_dir, filename), [])
   assert elems
   for e in elems[1:]:
         if e[0] == 'struct' and e[1] == struct_name:
      s = Struct(self, *e[1:])
   assert s.name not in self.kernels
            def get_type(self, name):
      if name in self.structs:
               def get_fixed_gpr(self, num):
      assert isinstance(num, int)
   if num in self.regs:
            reg = FixedGPR(num)
   self.regs[num] = reg
         def optimize(self):
      progress = True
   while progress:
                  # Copy Propagation
   for mk in self.metakernels:
                  # Dead Code Elimination
   for r in self.regs.values():
         for c in self.constants:
         for mk in self.metakernels:
         for mk in self.metakernels:
      if mk.opt_dead_code2():
      for n in list(self.regs.keys()):
      if not self.regs[n].live:
            def compact_regs(self):
      old_regs = self.regs
   self.regs = {}
   for i, reg in enumerate(old_regs.values()):
               def write_h(self, w):
      for s in self.structs.values():
         for mk in self.metakernels:
         def write_c(self, w):
      for c in self.constants:
         for mk in self.metakernels:
      class Kernel(object):
      def __init__(self, m, name, ann):
      self.name = name
   self.source_file = ann['source']
   self.kernel_name = self.source_file.replace('/', '_')[:-3].upper()
            assert self.source_file.endswith('.cl')
   self.c_name = '_'.join([
         'GRL_CL_KERNEL',
   self.kernel_name,
      class KernelModule(object):
      def __init__(self, m, name, source, kernels):
      self.name = name
   self.kernels = []
            for k in kernels:
         if k[0] == 'kernel':
      k[2]['source'] = source
      elif k[0] == 'library':
      class BasicType(object):
      def __init__(self, name):
      self.name = name
      class Struct(object):
      def __init__(self, m, name, fields, align):
      assert align == 0
   self.name = name
   self.c_name = 'struct ' + '_'.join(['grl', m.name, self.name])
         def write_h(self, w):
      w.write('{} {{\n', self.c_name)
   w.push_indent()
   for f in self.fields:
         w.pop_indent()
      class NamedConstant(Value):
      def __init__(self, name, value):
      super().__init__(name, 'cpu')
   self.name = name
   self.value = Constant(value)
         def set_module(self, m):
            def write_c(self, w):
      if self.written:
         w.write('static const uint64_t {} = {};\n',
            class MetaKernelParameter(Value):
      def __init__(self, mk, type, name):
      super().__init__(name, 'cpu')
      class MetaKernel(object):
      def __init__(self, m, m_scope, name, params, ann, statements):
      self.m = m
   self.name = name
   self.c_name = '_'.join(['grl', m.name, self.name])
   self.goto_targets = {}
                     self.params = [MetaKernelParameter(self, *p) for p in params]
   for p in self.params:
            mk_scope.add_def(GroupSizeRegister(0), name='DISPATCHDIM_X')
   mk_scope.add_def(GroupSizeRegister(1), name='DISPATCHDIM_Y')
            self.statements = []
   self.parse_stmt(mk_scope, statements)
         def get_tmp(self):
      tmpN = '_tmp{}'.format(self.num_tmps)
   self.num_tmps += 1
         def add_stmt(self, stmt):
      self.statements.append(stmt)
         def parse_value(self, v):
      if isinstance(v, Value):
         elif isinstance(v, str):
         if re.match(r'REG\d+', v):
         else:
   elif isinstance(v, int):
         elif isinstance(v, tuple):
         if v[0] == 'member':
         elif v[0] == 'offsetof':
         else:
      op = v[0]
      else:
         def load_value(self, v):
      v = self.parse_value(v)
   if isinstance(v, Member) and v.zone == 'gpu':
               def parse_stmt(self, scope, s):
      self.scope = scope
   if isinstance(s, list):
         subscope = Scope(self.m, self, scope)
   for stmt in s:
   elif s[0] == 'define':
         elif s[0] == 'assign':
         elif s[0] == 'dispatch':
         elif s[0] == 'load-dword':
         v = self.add_stmt(LoadMem(self, 32, s[2]))
   elif s[0] == 'load-qword':
         v = self.add_stmt(LoadMem(self, 64, s[2]))
   elif s[0] == 'store-dword':
         elif s[0] == 'store-qword':
         elif s[0] == 'goto':
         elif s[0] == 'goto-if':
         elif s[0] == 'goto-if-not':
         elif s[0] == 'label':
         elif s[0] == 'control':
         elif s[0] == 'sem-wait-while':
         else:
         def add_goto_target(self, t):
      assert t.name not in self.goto_targets
         def get_goto_target(self, name):
            def opt_copy_prop(self):
      progress = False
   copies = {}
   for stmt in self.statements:
         for i in range(len(stmt.srcs)):
      src = stmt.srcs[i]
                     if isinstance(stmt, StoreReg):
                           if isinstance(reg, FixedGPR):
      copies.pop(reg.num, None)
   if not stmt.srcs[0].is_reg():
                  def opt_dead_code1(self):
      for stmt in self.statements:
         # Mark every register which is read as live
   for src in stmt.srcs:
                  # Initialize every SSA statement to dead
         def opt_dead_code2(self):
      def yield_live(statements):
         gprs_read = set(self.m.regs.keys())
   for stmt in statements:
      if isinstance(stmt, SSAStatement):
      if not stmt.live:
      elif isinstance(stmt, StoreReg):
                                          if isinstance(reg, FixedGPR):
         if reg.num in gprs_read:
                           for src in stmt.srcs:
      src.live = True
            old_stmt_list = self.statements
   old_stmt_list.reverse()
   self.statements = list(yield_live(old_stmt_list))
   self.statements.reverse()
         def count_ssa_value_uses(self):
      for stmt in self.statements:
                        for src in stmt.srcs:
         def write_h(self, w):
      w.write('void\n')
   w.write('genX({})(\n', self.c_name)
   w.push_indent()
   w.write('struct anv_cmd_buffer *cmd_buffer')
   for p in self.params:
         w.write(');\n')
         def write_c(self, w):
      w.write('void\n')
   w.write('genX({})(\n', self.c_name)
   w.push_indent()
   w.write('struct anv_cmd_buffer *cmd_buffer')
   for p in self.params:
         w.write(')\n')
   w.pop_indent()
   w.write('{\n')
            w.write('struct mi_builder b;\n')
   w.write('mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);\n')
   w.write('/* TODO: use anv_mocs? */\n');
   w.write('const uint32_t mocs = isl_mocs(&cmd_buffer->device->isl_dev, 0, false);\n');
   w.write('mi_builder_set_mocs(&b, mocs);\n');
            for r in self.m.regs.values():
                  for t in self.goto_targets.values():
                  self.count_ssa_value_uses()
   for s in self.statements:
                        HEADER_PROLOGUE = COPYRIGHT + '''
   #include "anv_private.h"
   #include "grl/genX_grl.h"
      #ifndef {0}
   #define {0}
      #ifdef __cplusplus
   extern "C" {{
   #endif
      '''
      HEADER_EPILOGUE = '''
   #ifdef __cplusplus
   }}
   #endif
      #endif /* {0} */
   '''
      C_PROLOGUE = COPYRIGHT + '''
   #include "{0}"
      #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
   #include "genxml/genX_rt_pack.h"
      /* We reserve :
   *    - GPR 14 for secondary command buffer returns
   *    - GPR 15 for conditional rendering
   */
   #define MI_BUILDER_NUM_ALLOC_GPRS 14
   #define __gen_get_batch_dwords anv_batch_emit_dwords
   #define __gen_address_offset anv_address_add
   #define __gen_get_batch_address(b, a) anv_batch_address(b, a)
   #include "common/mi_builder.h"
      #define MI_PREDICATE_RESULT mi_reg32(0x2418)
   #define DISPATCHDIM_X mi_reg32(0x2500)
   #define DISPATCHDIM_Y mi_reg32(0x2504)
   #define DISPATCHDIM_Z mi_reg32(0x2508)
   '''
      def parse_libraries(filenames):
      libraries = {}
   for fname in filenames:
      lib_package = parse_grl_file(fname, [])
   for lib in lib_package:
         assert lib[0] == 'library'
   # Add the directory of the library so that CL files can be found.
            def main():
      argparser = argparse.ArgumentParser()
   argparser.add_argument('--out-c', help='Output C file')
   argparser.add_argument('--out-h', help='Output C file')
   argparser.add_argument('--library', dest='libraries', action='append',
         argparser.add_argument('grl', help="Input  file")
                                       m = Module(grl_dir, ir)
   m.optimize()
            with open(args.out_h, 'w') as f:
      guard = os.path.splitext(os.path.basename(args.out_h))[0].upper()
   w = Writer(f)
   w.write(HEADER_PROLOGUE, guard)
   m.write_h(w)
         with open(args.out_c, 'w') as f:
      w = Writer(f)
   w.write(C_PROLOGUE, os.path.basename(args.out_h))
      if __name__ == '__main__':
      main()
