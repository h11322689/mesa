      # (C) Copyright IBM Corporation 2005
   # All Rights Reserved.
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # on the rights to use, copy, modify, merge, publish, distribute, sub
   # license, and/or sell copies of the Software, and to permit persons to whom
   # the Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   # IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
   #
   # Authors:
   #    Ian Romanick <idr@us.ibm.com>
      import copy
      class type_node(object):
      def __init__(self):
      self.pointer = 0  # bool
   self.const = 0    # bool
   self.signed = 1   # bool
            # If elements is set to non-zero, then field is an array.
            self.name = None
   self.size = 0     # type's size in bytes
            def string(self):
      """Return string representation of this type_node."""
            if self.pointer:
            if self.const:
            if not self.pointer:
         if self.integer:
      if self.signed:
                                    class type_table(object):
      def __init__(self):
      self.types_by_name = {}
            def add_type(self, type_expr):
      self.types_by_name[ type_expr.get_base_name() ] = type_expr
            def find_type(self, name):
      if name in self.types_by_name:
         else:
         def create_initial_types():
               basic_types = [
            ("char",   1, 1),
   ("short",  2, 1),
   ("int",    4, 1),
   ("long",   4, 1),
   ("float",  4, 0),
               for (type_name, type_size, integer) in basic_types:
      te = type_expression(None)
   tn = type_node()
   tn.name = type_name
   tn.size = type_size
   tn.integer = integer
   te.expr.append(tn)
         type_expression.built_in_types = tt
            class type_expression(object):
               def __init__(self, type_string, extra_types = None):
               if not type_string:
                     if not type_expression.built_in_types:
            # Replace '*' with ' * ' in type_string.  Then, split the string
   # into tokens, separated by spaces.
            const = 0
   t = None
   signed = 0
            for i in tokens:
         if i == "const":
      if t and t.pointer:
         else:
      elif i == "signed":
         elif i == "unsigned":
         elif i == "*":
      # This is a quirky special-case because of the
                        if unsigned:
      self.set_base_type( "int", signed, unsigned, const, extra_types )
                                                   t = type_node()
   t.pointer = 1
      else:
                     self.set_base_type( i, signed, unsigned, const, extra_types )
                                 if const:
            if unsigned:
            if signed:
                     def set_base_type(self, type_name, signed, unsigned, const, extra_types):
      te = type_expression.built_in_types.find_type( type_name )
   if not te:
            if not te:
                     t = self.expr[ len(self.expr) - 1 ]
   t.const = const
   if signed:
         elif unsigned:
            def set_base_type_node(self, tn):
      self.expr = [tn]
            def set_elements(self, count):
               tn.elements = count
            def string(self):
      s = ""
   for t in self.expr:
                     def get_base_type_node(self):
               def get_base_name(self):
      if len(self.expr):
         else:
            def get_element_size(self):
               if tn.elements:
         else:
            def get_element_count(self):
      tn = self.expr[0]
            def get_stack_size(self):
               if tn.elements or tn.pointer:
         elif not tn.integer:
         else:
            def is_pointer(self):
      tn = self.expr[ -1 ]
            def format_string(self):
      tn = self.expr[ -1 ]
   if tn.pointer:
         elif not tn.integer:
         else:
            if __name__ == '__main__':
         types_to_try = [ "int", "int *", "const int *", "int * const", "const int * const", \
                           for t in types_to_try:
      print('Trying "%s"...' % (t))
   te = type_expression( t )
   print('Got "%s" (%u, %u).' % (te.string(), te.get_stack_size(), te.get_element_size()))
