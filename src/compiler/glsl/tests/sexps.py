   # coding=utf-8
   #
   # Copyright © 2011 Intel Corporation
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
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   # DEALINGS IN THE SOFTWARE.
      # This file contains helper functions for manipulating sexps in Python.
   #
   # We represent a sexp in Python using nested lists containing strings.
   # So, for example, the sexp (constant float (1.000000)) is represented
   # as ['constant', 'float', ['1.000000']].
      import re
      def check_sexp(sexp):
               That is, raise an exception if the argument is not a string or a
   list, or if it contains anything that is not a string or a list at
   any nesting level.
   """
   if isinstance(sexp, list):
      for s in sexp:
      elif not isinstance(sexp, str):
         def parse_sexp(sexp):
      """Convert a string, of the form that would be output by mesa,
   into a sexp represented as nested lists containing strings.
   """
   sexp_token_regexp = re.compile(
         stack = [[]]
   for match in sexp_token_regexp.finditer(sexp):
      token = match.group(0)
   if token == '(':
         elif token == ')':
         if len(stack) == 1:
         sexp = stack.pop()
   else:
      if len(stack) != 1:
         if len(stack[0]) != 1:
               def sexp_to_string(sexp):
      """Convert a sexp, represented as nested lists containing strings,
   into a single string of the form parseable by mesa.
   """
   if isinstance(sexp, str):
         assert isinstance(sexp, list)
   result = ''
   for s in sexp:
      sub_result = sexp_to_string(s)
   if result == '':
         elif '\n' not in result and '\n' not in sub_result and \
               else:
            def sort_decls(sexp):
               This is used to work around the fact that
   ir_reader::read_instructions reorders declarations.
   """
   assert isinstance(sexp, list)
   decls = []
   other_code = []
   for s in sexp:
      if isinstance(s, list) and len(s) >= 4 and s[0] == 'declare':
         else:
         