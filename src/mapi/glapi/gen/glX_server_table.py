      # (C) Copyright IBM Corporation 2005, 2006
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
      import argparse
      import gl_XML, glX_XML, glX_proto_common, license
         def log2(value):
      for i in range(0, 30):
      p = 1 << i
   if p >= value:
                  def round_down_to_power_of_two(n):
               for i in range(30, 0, -1):
      p = 1 << i
   if p <= n:
                  class function_table:
      def __init__(self, name, do_size_check):
      self.name_base = name
               self.max_bits = 1
   self.next_opcode_threshold = (1 << self.max_bits)
            self.functions = {}
            # Minimum number of opcodes in a leaf node.
   self.min_op_bits = 3
   self.min_op_count = (1 << self.min_op_bits)
            def append(self, opcode, func):
               if opcode > self.max_opcode:
                  if opcode > self.next_opcode_threshold:
                                       def divide_group(self, min_opcode, total):
      """Divide the group starting min_opcode into subgroups.
   Returns a tuple containing the number of bits consumed by
   the node, the list of the children's tuple, and the number
   of entries in the final array used by this node and its
            remaining_bits = self.max_bits - total
   next_opcode = min_opcode + (1 << remaining_bits)
            for M in range(0, remaining_bits):
                        empty_children = 0
   full_children = 0
   for i in range(min_opcode, next_opcode, op_count):
                     for j in range(i, i + op_count):
      if self.functions.has_key(j):
                                                               # If all the remaining bits are used by this node, as is the
            if (M == 0) or (M == remaining_bits):
         else:
         children = []
   count = 1
   depth = 1
   all_children_are_nonempty_leaf_nodes = 1
                                                                              if all_children_are_nonempty_leaf_nodes:
                  def is_empty_leaf(self, base_opcode, M):
      for op in range(base_opcode, base_opcode + (1 << M)):
         if self.functions.has_key(op):
                     def dump_tree(self, node, base_opcode, remaining_bits, base_entry, depth):
      M = node[0]
   children = node[1]
               # This actually an error condition.
   if children == []:
            print '    /* [%u] -> opcode range [%u, %u], node depth %u */' % (base_entry, base_opcode, base_opcode + (1 << remaining_bits), depth)
                     child_index = base_entry
   child_base_opcode = base_opcode
   for child in children:
         if child[1] == []:
      if self.is_empty_leaf(child_base_opcode, child_M):
         else:
      # Emit the index of the next dispatch
   # function.  Then add all the
                                 for op in range(child_base_opcode, child_base_opcode + (1 << child_M)):
                                                                                                                           else:
                              child_index = base_entry
   for child in children:
         if child[1] != []:
                     def Print(self):
      # Each dispatch table consists of two data structures.
   #
   # The first structure is an N-way tree where the opcode for
   # the function is the key.  Each node switches on a range of
   # bits from the opcode.  M bits are extracted from the opcde
   # and are used as an index to select one of the N, where
   # N = 2^M, children.
   #
   # The tree is stored as a flat array.  The first value is the
   # number of bits, M, used by the node.  For inner nodes, the
   # following 2^M values are indexes into the array for the
   # child nodes.  For leaf nodes, the followign 2^M values are
   # indexes into the second data structure.
   #
   # If an inner node's child index is 0, the child is an empty
   # leaf node.  That is, none of the opcodes selectable from
   # that child exist.  Since most of the possible opcode space
   # is unused, this allows compact data storage.
   #
   # The second data structure is an array of pairs of function
   # pointers.  Each function contains a pointer to a protocol
   # decode function and a pointer to a byte-swapped protocol
   # decode function.  Elements in this array are selected by the
   # leaf nodes of the first data structure.
   #
   # As the tree is traversed, an accumulator is kept.  This
   # accumulator counts the bits of the opcode consumed by the
   # traversal.  When accumulator + M = B, where B is the
   # maximum number of bits in an opcode, the traversal has
   # reached a leaf node.  The traversal starts with the most
   # significant bits and works down to the least significant
   # bits.
   #
   # Creation of the tree is the most complicated part.  At
   # each node the elements are divided into groups of 2^M
   # elements.  The value of M selected is the smallest possible
   # value where all of the groups are either empty or full, or
   # the groups are a preset minimum size.  If all the children
   # of a node are non-empty leaf nodes, the children are merged
                     print '/*****************************************************************/'
   print '/* tree depth = %u */' % (tree[3])
   print 'static const int_fast16_t %s_dispatch_tree[%u] = {' % (self.name_base, tree[2])
   self.dump_tree(tree, 0, self.max_bits, 0, 1)
                     print 'static const void *%s_function_table[%u][2] = {' % (self.name_base, len(self.lookup_table))
   index = 0
   for func in self.lookup_table:
         opcode = func[0]
                                       if self.do_size_check:
                  print 'static const int_fast16_t %s_size_table[%u][2] = {' % (self.name_base, len(self.lookup_table))
   index = 0
   var_table = []
   for func in self.lookup_table:
                           if var != "":
      var_offset = "%2u" % (len(var_table))
                                                print 'static const gl_proto_size_func %s_size_func_table[%u] = {' % (self.name_base, len(var_table))
                        print 'const struct __glXDispatchInfo %s_dispatch_info = {' % (self.name_base)
   print '    %u,' % (self.max_bits)
   print '    %s_dispatch_tree,' % (self.name_base)
   print '    %s_function_table,' % (self.name_base)
   if self.do_size_check:
         print '    %s_size_table,' % (self.name_base)
   else:
         print '    NULL,'
   print '};\n'
         class PrintGlxDispatchTables(glX_proto_common.glx_print_proto):
      def __init__(self):
      gl_XML.gl_print_base.__init__(self)
   self.name = "glX_server_table.py (from Mesa)"
            self.rop_functions = function_table("Render", 1)
   self.sop_functions = function_table("Single", 0)
   self.vop_functions = function_table("VendorPriv", 0)
            def printRealHeader(self):
      print '#include <inttypes.h>'
   print '#include "glxserver.h"'
   print '#include "glxext.h"'
   print '#include "indirect_dispatch.h"'
   print '#include "indirect_reqsize.h"'
   print '#include "indirect_table.h"'
   print ''
            def printBody(self, api):
      for f in api.functionIterateAll():
         if not f.ignore and f.vectorequiv == None:
      if f.glx_rop != 0:
         if f.glx_sop != 0:
               self.sop_functions.Print()
   self.rop_functions.Print()
   self.vop_functions.Print()
         def _parser():
      """Parse arguments and return namespace."""
   parser = argparse.ArgumentParser()
   parser.add_argument('-f',
                              if __name__ == '__main__':
      args = _parser()
   printer = PrintGlxDispatchTables()
            printer.Print(api)
