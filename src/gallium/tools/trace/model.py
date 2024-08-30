   #!/usr/bin/env python3
   ##########################################################################
   # 
   # Copyright 2008 VMware, Inc.
   # All Rights Reserved.
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
   # 
   ##########################################################################
         '''Trace data model.'''
         import sys
   import string
   import binascii
   from io import StringIO
   import format
         class ModelOptions:
         def __init__(self, args=None):
      # Initialize the options we need to exist,
   # with some reasonable defaults
   self.plain = False
   self.suppress_variants = False
   self.named_ptrs = False
            # If args is specified, we assume it is the result object
   # from ArgumentParser.parse_args(). Copy the attribute values
   # we have from it, if they exist.
   if args is not None:
         for var in self.__dict__:
         class TraceStateData:
         def __init__(self):
      self.ptr_list = {}
   self.ptr_type_list = {}
         class Node:
         def visit(self, visitor):
            def __str__(self):
      stream = StringIO()
   formatter = format.Formatter(stream)
   pretty_printer = PrettyPrinter(formatter, {})
   self.visit(pretty_printer)
         def __hash__(self):
            class Literal(Node):
         def __init__(self, value):
            def visit(self, visitor):
            def __hash__(self):
            class Blob(Node):
         def __init__(self, value):
            def getValue(self):
            def visit(self, visitor):
            def __hash__(self):
            class NamedConstant(Node):
         def __init__(self, name):
            def visit(self, visitor):
            def __hash__(self):
               class Array(Node):
         def __init__(self, elements):
            def visit(self, visitor):
            def __hash__(self):
      tmp = 0
   for mobj in self.elements:
               class Struct(Node):
         def __init__(self, name, members):
      self.name = name
         def visit(self, visitor):
            def __hash__(self):
      tmp = hash(self.name)
   for mname, mobj in self.members:
               class Pointer(Node):
                  def __init__(self, state, address, pname):
      self.address = address
            # Check if address exists in list and if it is a return value address
   t1 = address in state.ptr_list
   if t1:
         rname = state.ptr_type_list[address]
   else:
                  # If address does NOT exist (add it), OR IS a ret value (update with new type)
   if not t1 or t2:
         # If previously set to ret value, remove one from count
                  # Add / update
   self.adjust_ptr_type_count(pname, 1)
   tmp = "{}_{}".format(pname, state.ptr_types_list[pname])
         def adjust_ptr_type_count(self, pname, delta):
      if pname not in self.state.ptr_types_list:
                  def named_address(self):
            def visit(self, visitor):
            def __hash__(self):
            class Call:
         def __init__(self, no, klass, method, args, ret, time):
      self.no = no
   self.klass = klass
   self.method = method
   self.args = args
   self.ret = ret
            # Calculate hashvalue "cached" into a variable
   self.hashvalue = hash(self.klass) ^ hash(self.method)
   for mname, mobj in self.args:
            def visit(self, visitor):
            def __hash__(self):
            def __eq__(self, other):
            class Trace:
         def __init__(self, calls):
      self.calls = calls
      def visit(self, visitor):
               class Visitor:
         def visit_literal(self, node):
            def visit_blob(self, node):
            def visit_named_constant(self, node):
            def visit_array(self, node):
            def visit_struct(self, node):
            def visit_pointer(self, node):
            def visit_call(self, node):
            def visit_trace(self, node):
            class PrettyPrinter:
         def __init__(self, formatter, options):
      self.formatter = formatter
         def visit_literal(self, node):
      if node.value is None:
                  if isinstance(node.value, str):
                        def visit_blob(self, node):
            def visit_named_constant(self, node):
            def visit_array(self, node):
      self.formatter.text('{')
   sep = ''
   for value in node.elements:
         self.formatter.text(sep)
   value.visit(self) 
         def visit_struct(self, node):
      self.formatter.text('{')
   sep = ''
   for name, value in node.members:
         self.formatter.text(sep)
   self.formatter.variable(name)
   self.formatter.text(' = ')
   value.visit(self) 
         def visit_pointer(self, node):
      if self.options.named_ptrs:
         else:
         def visit_call(self, node):
      if not self.options.suppress_variants:
            if node.klass is not None:
         else:
            if not self.options.method_only:
         self.formatter.text('(')
   sep = ''
   for name, value in node.args:
      self.formatter.text(sep)
   self.formatter.variable(name)
   self.formatter.text(' = ')
   value.visit(self)
      self.formatter.text(')')
   if node.ret is not None:
            if not self.options.suppress_variants and node.time is not None:
                        def visit_trace(self, node):
      for call in node.calls:
   