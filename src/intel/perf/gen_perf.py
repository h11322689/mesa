   # Copyright (c) 2015-2017 Intel Corporation
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
      import argparse
   import builtins
   import collections
   import os
   import re
   import sys
   import textwrap
      import xml.etree.ElementTree as et
      hashed_funcs = {}
      c_file = None
   _c_indent = 0
      def c(*args):
      code = ' '.join(map(str,args))
   for line in code.splitlines():
      text = ''.rjust(_c_indent) + line
      # indented, but no trailing newline...
   def c_line_start(code):
         def c_raw(code):
            def c_indent(n):
      global _c_indent
      def c_outdent(n):
      global _c_indent
         header_file = None
   _h_indent = 0
      def h(*args):
      code = ' '.join(map(str,args))
   for line in code.splitlines():
      text = ''.rjust(_h_indent) + line
      def h_indent(n):
      global _c_indent
      def h_outdent(n):
      global _c_indent
            def emit_fadd(tmp_id, args):
      c("double tmp{0} = {1} + {2};".format(tmp_id, args[1], args[0]))
         # Be careful to check for divide by zero...
   def emit_fdiv(tmp_id, args):
      c("double tmp{0} = {1};".format(tmp_id, args[1]))
   c("double tmp{0} = {1};".format(tmp_id + 1, args[0]))
   c("double tmp{0} = tmp{1} ? tmp{2} / tmp{1} : 0;".format(tmp_id + 2, tmp_id + 1, tmp_id))
         def emit_fmax(tmp_id, args):
      c("double tmp{0} = {1};".format(tmp_id, args[1]))
   c("double tmp{0} = {1};".format(tmp_id + 1, args[0]))
   c("double tmp{0} = MAX(tmp{1}, tmp{2});".format(tmp_id + 2, tmp_id, tmp_id + 1))
         def emit_fmul(tmp_id, args):
      c("double tmp{0} = {1} * {2};".format(tmp_id, args[1], args[0]))
         def emit_fsub(tmp_id, args):
      c("double tmp{0} = {1} - {2};".format(tmp_id, args[1], args[0]))
         def emit_read(tmp_id, args):
      type = args[1].lower()
   c("uint64_t tmp{0} = results->accumulator[query->{1}_offset + {2}];".format(tmp_id, type, args[0]))
         def emit_uadd(tmp_id, args):
      c("uint64_t tmp{0} = {1} + {2};".format(tmp_id, args[1], args[0]))
         # Be careful to check for divide by zero...
   def emit_udiv(tmp_id, args):
      c("uint64_t tmp{0} = {1};".format(tmp_id, args[1]))
   c("uint64_t tmp{0} = {1};".format(tmp_id + 1, args[0]))
   if args[0].isdigit():
      assert int(args[0]) > 0
      else:
               def emit_umul(tmp_id, args):
      c("uint64_t tmp{0} = {1} * {2};".format(tmp_id, args[1], args[0]))
         def emit_usub(tmp_id, args):
      c("uint64_t tmp{0} = {1} - {2};".format(tmp_id, args[1], args[0]))
         def emit_umin(tmp_id, args):
      c("uint64_t tmp{0} = MIN({1}, {2});".format(tmp_id, args[1], args[0]))
         def emit_lshft(tmp_id, args):
      c("uint64_t tmp{0} = {1} << {2};".format(tmp_id, args[1], args[0]))
         def emit_rshft(tmp_id, args):
      c("uint64_t tmp{0} = {1} >> {2};".format(tmp_id, args[1], args[0]))
         def emit_and(tmp_id, args):
      c("uint64_t tmp{0} = {1} & {2};".format(tmp_id, args[1], args[0]))
         def emit_ulte(tmp_id, args):
      c("uint64_t tmp{0} = {1} <= {2};".format(tmp_id, args[1], args[0]))
         def emit_ult(tmp_id, args):
      c("uint64_t tmp{0} = {1} < {2};".format(tmp_id, args[1], args[0]))
         def emit_ugte(tmp_id, args):
      c("uint64_t tmp{0} = {1} >= {2};".format(tmp_id, args[1], args[0]))
         def emit_ugt(tmp_id, args):
      c("uint64_t tmp{0} = {1} > {2};".format(tmp_id, args[1], args[0]))
         ops = {}
   #             (n operands, emitter)
   ops["FADD"] = (2, emit_fadd)
   ops["FDIV"] = (2, emit_fdiv)
   ops["FMAX"] = (2, emit_fmax)
   ops["FMUL"] = (2, emit_fmul)
   ops["FSUB"] = (2, emit_fsub)
   ops["READ"] = (2, emit_read)
   ops["UADD"] = (2, emit_uadd)
   ops["UDIV"] = (2, emit_udiv)
   ops["UMUL"] = (2, emit_umul)
   ops["USUB"] = (2, emit_usub)
   ops["UMIN"] = (2, emit_umin)
   ops["<<"]   = (2, emit_lshft)
   ops[">>"]   = (2, emit_rshft)
   ops["AND"]  = (2, emit_and)
   ops["UGTE"] = (2, emit_ugte)
   ops["UGT"]  = (2, emit_ugt)
   ops["ULTE"] = (2, emit_ulte)
   ops["ULT"]  = (2, emit_ult)
         def brkt(subexp):
      if " " in subexp:
         else:
         def splice_bitwise_and(args):
            def splice_bitwise_or(args):
            def splice_logical_and(args):
            def splice_umul(args):
            def splice_ult(args):
            def splice_ugte(args):
            def splice_ulte(args):
            def splice_ugt(args):
            def splice_lshft(args):
            def splice_equal(args):
            exp_ops = {}
   #                 (n operands, splicer)
   exp_ops["AND"]  = (2, splice_bitwise_and)
   exp_ops["OR"]   = (2, splice_bitwise_or)
   exp_ops["UGTE"] = (2, splice_ugte)
   exp_ops["ULT"]  = (2, splice_ult)
   exp_ops["&&"]   = (2, splice_logical_and)
   exp_ops["UMUL"] = (2, splice_umul)
   exp_ops["<<"]   = (2, splice_lshft)
   exp_ops["=="]   = (2, splice_equal)
         hw_vars = {}
   hw_vars["$EuCoresTotalCount"] = "perf->sys_vars.n_eus"
   hw_vars["$VectorEngineTotalCount"] = "perf->sys_vars.n_eus"
   hw_vars["$EuSlicesTotalCount"] = "perf->sys_vars.n_eu_slices"
   hw_vars["$EuSubslicesTotalCount"] = "perf->sys_vars.n_eu_sub_slices"
   hw_vars["$XeCoreTotalCount"] = "perf->sys_vars.n_eu_sub_slices"
   hw_vars["$EuDualSubslicesTotalCount"] = "perf->sys_vars.n_eu_sub_slices"
   hw_vars["$EuDualSubslicesSlice0123Count"] = "perf->sys_vars.n_eu_slice0123"
   hw_vars["$EuThreadsCount"] = "perf->devinfo.num_thread_per_eu"
   hw_vars["$VectorEngineThreadsCount"] = "perf->devinfo.num_thread_per_eu"
   hw_vars["$SliceMask"] = "perf->sys_vars.slice_mask"
   hw_vars["$SliceTotalCount"] = "perf->sys_vars.n_eu_slices"
   # subslice_mask is interchangeable with subslice/dual-subslice since Gfx12+
   # only has dual subslices which can be assimilated with 16EUs subslices.
   hw_vars["$SubsliceMask"] = "perf->sys_vars.subslice_mask"
   hw_vars["$DualSubsliceMask"] = "perf->sys_vars.subslice_mask"
   hw_vars["$XeCoreMask"] = "perf->sys_vars.subslice_mask"
   hw_vars["$GpuTimestampFrequency"] = "perf->devinfo.timestamp_frequency"
   hw_vars["$GpuMinFrequency"] = "perf->sys_vars.gt_min_freq"
   hw_vars["$GpuMaxFrequency"] = "perf->sys_vars.gt_max_freq"
   hw_vars["$SkuRevisionId"] = "perf->devinfo.revision"
   hw_vars["$QueryMode"] = "perf->sys_vars.query_mode"
      def resolve_variable(name, set, allow_counters):
      if name in hw_vars:
         m = re.search(r'\$GtSlice([0-9]+)$', name)
   if m:
         m = re.search(r'\$GtSlice([0-9]+)XeCore([0-9]+)$', name)
   if m:
         if allow_counters and name in set.counter_vars:
               def output_rpn_equation_code(set, counter, equation):
      c("/* RPN equation: " + equation + " */")
   tokens = equation.split()
   stack = []
   tmp_id = 0
            for token in tokens:
      stack.append(token)
   while stack and stack[-1] in ops:
         op = stack.pop()
   argc, callback = ops[op]
   args = []
   for i in range(0, argc):
      operand = stack.pop()
   if operand[0] == "$":
      resolved_variable = resolve_variable(operand, set, True)
   if resolved_variable == None:
                                 if len(stack) != 1:
      raise Exception("Spurious empty rpn code for " + set.name + " :: " +
                        if value[0] == "$":
      resolved_variable = resolve_variable(value, set, True)
   if resolved_variable == None:
                     def splice_rpn_expression(set, counter_name, expression):
      tokens = expression.split()
            for token in tokens:
      stack.append(token)
   while stack and stack[-1] in exp_ops:
         op = stack.pop()
   argc, callback = exp_ops[op]
   args = []
   for i in range(0, argc):
      operand = stack.pop()
   if operand[0] == "$":
      resolved_variable = resolve_variable(operand, set, False)
   if resolved_variable == None:
                           if len(stack) != 1:
      raise Exception("Spurious empty rpn expression for " + set.name + " :: " +
                        if value[0] == "$":
      resolved_variable = resolve_variable(value, set, False)
   if resolved_variable == None:
                     def output_counter_read(gen, set, counter):
      c("\n")
            if counter.read_hash in hashed_funcs:
      c("#define %s \\" % counter.read_sym)
   c_indent(3)
   c("%s" % hashed_funcs[counter.read_hash])
      else:
      ret_type = counter.get('data_type')
   if ret_type == "uint64":
                     c("static " + ret_type)
   c(counter.read_sym + "(UNUSED struct intel_perf_config *perf,\n")
   c_indent(len(counter.read_sym) + 1)
   c("const struct intel_perf_query_info *query,\n")
   c("const struct intel_perf_query_result *results)\n")
            c("{")
   c_indent(3)
   output_rpn_equation_code(set, counter, read_eq)
   c_outdent(3)
                  def output_counter_max(gen, set, counter):
               if not counter.has_custom_max_func():
            c("\n")
            if counter.max_hash in hashed_funcs:
      c("#define %s \\" % counter.max_sym)
   c_indent(3)
   c("%s" % hashed_funcs[counter.max_hash])
      else:
      ret_type = counter.get('data_type')
   if ret_type == "uint64":
            c("static " + ret_type)
   c(counter.max_sym + "(struct intel_perf_config *perf,\n")
   c_indent(len(counter.read_sym) + 1)
   c("const struct intel_perf_query_info *query,\n")
   c("const struct intel_perf_query_result *results)\n")
   c_outdent(len(counter.read_sym) + 1)
   c("{")
   c_indent(3)
   output_rpn_equation_code(set, counter, max_eq)
   c_outdent(3)
                  c_type_sizes = { "uint32_t": 4, "uint64_t": 8, "float": 4, "double": 8, "bool": 4 }
   def sizeof(c_type):
            def pot_align(base, pot_alignment):
            semantic_type_map = {
      "duration": "raw",
   "ratio": "event"
         def output_availability(set, availability, counter_name):
      expression = splice_rpn_expression(set, counter_name, availability)
   lines = expression.split(' && ')
   n_lines = len(lines)
   if n_lines == 1:
         else:
      c("if (" + lines[0] + " &&")
   c_indent(4)
   for i in range(1, (n_lines - 1)):
         c(lines[(n_lines - 1)] + ") {")
         def output_units(unit):
               # should a unit be visible in description?
   units_map = {
      "bytes" : True,
   "cycles" : True,
   "eu atomic requests to l3 cache lines" : False,
   "eu bytes per l3 cache line" : False,
   "eu requests to l3 cache lines" : False,
   "eu sends to l3 cache lines" : False,
   "events" : True,
   "hz" : True,
   "messages" : True,
   "ns" : True,
   "number" : False,
   "percent" : True,
   "pixels" : True,
   "texels" : True,
   "threads" : True,
   "us" : True,
   "utilization" : False,
   "gbps" : True,
            def desc_units(unit):
      val = units_map.get(unit)
   if val is None:
         if val == False:
         if unit == 'hz':
                  counter_key_tuple = collections.namedtuple(
      'counter_key',
   [
      'name',
   'description',
   'symbol_name',
   'mdapi_group',
   'semantic_type',
   'data_type',
         )
         def counter_key(counter):
               def output_counter_struct(set, counter, idx,
                  data_type = counter.data_type
            semantic_type = counter.semantic_type
   if semantic_type in semantic_type_map:
                     c("[" + str(idx) + "] = {\n")
   c_indent(3)
   c(".name_idx = " + str(name_to_idx[counter.name]) + ",\n")
   c(".desc_idx = " + str(desc_to_idx[counter.description + " " + desc_units(counter.units)]) + ",\n")
   c(".symbol_name_idx = " + str(symbol_name_to_idx[counter.symbol_name]) + ",\n")
   c(".category_idx = " + str(category_to_idx[counter.mdapi_group]) + ",\n")
   c(".type = INTEL_PERF_COUNTER_TYPE_" + semantic_type_uc + ",\n")
   c(".data_type = INTEL_PERF_COUNTER_DATA_TYPE_" + data_type_uc + ",\n")
   c(".units = INTEL_PERF_COUNTER_UNITS_" + output_units(counter.units) + ",\n")
   c_outdent(3)
            def output_counter_report(set, counter, counter_to_idx, current_offset):
      data_type = counter.get('data_type')
   data_type_uc = data_type.upper()
            if "uint" in c_type:
            semantic_type = counter.get('semantic_type')
   if semantic_type in semantic_type_map:
                              availability = counter.get('availability')
   if availability:
      output_availability(set, availability, counter.get('name'))
         key = counter_key(counter)
                     if data_type == 'uint64':
      c("intel_perf_query_add_counter_uint64(query, " + idx + ", " +
      str(current_offset) + ", " +
   set.max_funcs[counter.get('symbol_name')] + "," +
   else:
      c("intel_perf_query_add_counter_float(query, " + idx + ", " +
      str(current_offset) + ", " +
               if availability:
      c_outdent(3);
                  def str_to_idx_table(strs):
               str_to_idx = collections.OrderedDict()
   str_to_idx[sorted_strs[0]] = 0
            for i in range(1, len(sorted_strs)):
      str_to_idx[sorted_strs[i]] = str_to_idx[previous] + len(previous) + 1
                  def output_str_table(name: str, str_to_idx):
      c("\n")
   c("static const char " + name + "[] = {\n")
   c_indent(3)
   c("\n".join(f"/* {idx} */ \"{val}\\0\"" for val, idx in str_to_idx.items()))
   c_outdent(3)
            register_types = {
      'FLEX': 'flex_regs',
   'NOA': 'mux_regs',
      }
      def compute_register_lengths(set):
      register_lengths = {}
   register_configs = set.findall('register_config')
   for register_config in register_configs:
      t = register_types[register_config.get('type')]
   if t not in register_lengths:
         else:
                  def generate_register_configs(set):
               for register_config in register_configs:
               availability = register_config.get('availability')
   if availability:
                  registers = register_config.findall('register')
   c("static const struct intel_perf_query_register_prog %s[] = {" % t)
   c_indent(3)
   for register in registers:
         c_outdent(3)
   c("};")
   c("query->config.%s = %s;" % (t, t))
            if availability:
         c_outdent(3)
         # Wraps a <counter> element from the oa-*.xml files.
   class Counter:
      def __init__(self, set, xml):
      self.xml = xml
   self.set = set
   self.read_hash = None
            self.read_sym = "{0}__{1}__{2}__read".format(self.set.gen.chipset,
                     def get(self, prop):
            # Compute the hash of a counter's equation by expanding (including all the
   # sub-equations it depends on)
   def compute_hashes(self):
      if self.read_hash is not None:
            def replace_token(token):
         if token[0] != "$":
         if token not in self.set.counter_vars:
                  read_eq = self.xml.get('equation')
            max_eq = self.xml.get('max_equation')
   if max_eq:
         def has_custom_max_func(self):
      max_eq = self.xml.get('max_equation')
   if not max_eq:
            try:
         val = float(max_eq)
   if val == 100:
   except ValueError:
            for token in max_eq.split():
         if token[0] == '$' and resolve_variable(token, self.set, True) == None:
               def build_max_sym(self):
      max_eq = self.xml.get('max_equation')
   if not max_eq:
            try:
         val = float(max_eq)
   if val == 100:
      if self.xml.get('data_type') == 'uint64':
            except ValueError:
            assert self.has_custom_max_func()
   return "{0}__{1}__{2}__max".format(self.set.gen.chipset,
               # Wraps a <set> element from the oa-*.xml files.
   class Set:
      def __init__(self, gen, xml):
      self.gen = gen
            self.counter_vars = {}
   self.max_funcs = {}
            xml_counters = self.xml.findall("counter")
   self.counters = []
   for xml_counter in xml_counters:
         counter = Counter(self, xml_counter)
   self.counters.append(counter)
   self.counter_vars['$' + counter.get('symbol_name')] = counter
            for counter in self.counters:
         @property
   def hw_config_guid(self):
            @property
   def name(self):
            @property
   def symbol_name(self):
            @property
   def underscore_name(self):
            def findall(self, path):
            def find(self, path):
            # Wraps an entire oa-*.xml file.
   class Gen:
      def __init__(self, filename):
      self.filename = filename
   self.xml = et.parse(self.filename)
   self.chipset = self.xml.find('.//set').get('chipset').lower()
            for xml_set in self.xml.findall(".//set"):
         def main():
      global c_file
            parser = argparse.ArgumentParser()
   parser.add_argument("--header", help="Header file to write", required=True)
   parser.add_argument("--code", help="C file to write", required=True)
                     c_file = open(args.code, 'w')
            gens = []
   for xml_file in args.xml_files:
               copyright = textwrap.dedent("""\
      /* Autogenerated file, DO NOT EDIT manually! generated by {}
      *
   * Copyright (c) 2015 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
                     h(copyright)
   h(textwrap.dedent("""\
                              c(copyright)
   c(textwrap.dedent("""\
      #include <stdint.h>
                     #include "util/hash_table.h"
                           c(textwrap.dedent("""\
      #include "perf/intel_perf.h"
   #include "perf/intel_perf_setup.h"
         names = builtins.set()
   descs = builtins.set()
   symbol_names = builtins.set()
   categories = builtins.set()
   for gen in gens:
      for set in gen.sets:
         for counter in set.counters:
      names.add(counter.get('name'))
            name_to_idx = str_to_idx_table(names)
            desc_to_idx = str_to_idx_table(descs)
            symbol_name_to_idx = str_to_idx_table(symbol_names)
            category_to_idx = str_to_idx_table(categories)
            # Print out all equation functions.
   for gen in gens:
      for set in gen.sets:
         for counter in set.counters:
         c("\n")
   c("static const struct intel_perf_query_counter_data counters[] = {\n")
            counter_to_idx = collections.OrderedDict()
   idx = 0
   for gen in gens:
      for set in gen.sets:
         for counter in set.counters:
      key = counter_key(counter)
   if key not in counter_to_idx:
      counter_to_idx[key] = idx
   output_counter_struct(set, key, idx,
                     c_outdent(3)
            c(textwrap.dedent("""\
      static void ATTRIBUTE_NOINLINE
   intel_perf_query_add_counter_uint64(struct intel_perf_query_info *query,
                     {
                     dest->name = &name[counter->name_idx];
   dest->desc = &desc[counter->desc_idx];
                  dest->offset = offset;
   dest->type = counter->type;
   dest->data_type = counter->data_type;
   dest->units = counter->units;
   dest->oa_counter_max_uint64 = oa_counter_max;
               static void ATTRIBUTE_NOINLINE
   intel_perf_query_add_counter_float(struct intel_perf_query_info *query,
                     {
                     dest->name = &name[counter->name_idx];
   dest->desc = &desc[counter->desc_idx];
                  dest->offset = offset;
   dest->type = counter->type;
   dest->data_type = counter->data_type;
   dest->units = counter->units;
   dest->oa_counter_max_float = oa_counter_max;
               static float ATTRIBUTE_NOINLINE
   percentage_max_float(struct intel_perf_config *perf,
               {
                  static uint64_t ATTRIBUTE_NOINLINE
   percentage_max_uint64(struct intel_perf_config *perf,
               {
         }
         # Print out all metric sets registration functions for each set in each
   # generation.
   for gen in gens:
      for set in gen.sets:
                  c("\n")
   c("\nstatic void\n")
   c("{0}_register_{1}_counter_query(struct intel_perf_config *perf)\n".format(gen.chipset, set.underscore_name))
                  c("struct intel_perf_query_info *query = intel_query_alloc(perf, %u);\n" % len(counters))
   c("\n")
   c("query->name = \"" + set.name + "\";\n")
                                 c("\n")
   c("/* Note: we're assuming there can't be any variation in the definition ")
   c(" * of a query between contexts so it's ok to describe a query within a ")
   c(" * global variable which only needs to be initialized once... */")
                           offset = 0
                                                                              c("\nvoid")
   c("intel_oa_register_queries_" + gen.chipset + "(struct intel_perf_config *perf)")
   c("{")
            for set in gen.sets:
            c_outdent(3)
         if __name__ == '__main__':
      main()
