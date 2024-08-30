   #!/usr/bin/env python3
   ##########################################################################
   #
   # Copyright 2011 Jose Fonseca
   # All Rights Reserved.
   #
   # Permission is hereby granted, free of charge, to any person obtaining a copy
   # of this software and associated documentation files (the "Software"), to deal
   # in the Software without restriction, including without limitation the rights
   # to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   # copies of the Software, and to permit persons to whom the Software is
   # furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice shall be included in
   # all copies or substantial portions of the Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   # AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   # OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   # THE SOFTWARE.
   #
   ##########################################################################/
         import json
   import argparse
   import re
   import difflib
   import sys
         def strip_object_hook(obj):
      if '__class__' in obj:
         for name in obj.keys():
      if name.startswith('__') and name.endswith('__'):
               class Visitor:
         def visit(self, node, *args, **kwargs):
      if isinstance(node, dict):
         elif isinstance(node, list):
         else:
         def visitObject(self, node, *args, **kwargs):
            def visitArray(self, node, *args, **kwargs):
            def visitValue(self, node, *args, **kwargs):
            class Dumper(Visitor):
         def __init__(self, stream = sys.stdout):
      self.stream = stream
         def _write(self, s):
            def _indent(self):
            def _newline(self):
            def visitObject(self, node):
               members = sorted(node)
   for i in range(len(members)):
         name = members[i]
   value = node[name]
   self.enter_member(name)
   self.visit(value)
         def enter_object(self):
      self._write('{')
   self._newline()
         def enter_member(self, name):
      self._indent()
         def leave_member(self, last):
      if not last:
               def leave_object(self):
      self.level -= 1
   self._indent()
   self._write('}')
   if self.level <= 0:
         def visitArray(self, node):
      self.enter_array()
   for i in range(len(node)):
         value = node[i]
   self._indent()
   self.visit(value)
   if i != len(node) - 1:
               def enter_array(self):
      self._write('[')
   self._newline()
         def leave_array(self):
      self.level -= 1
   self._indent()
         def visitValue(self, node):
               class Comparer(Visitor):
         def __init__(self, ignore_added = False, tolerance = 2.0 ** -24):
      self.ignore_added = ignore_added
         def visitObject(self, a, b):
      if not isinstance(b, dict):
         if len(a) != len(b) and not self.ignore_added:
         ak = sorted(a)
   bk = sorted(b)
   if ak != bk and not self.ignore_added:
         for k in ak:
         ae = a[k]
   try:
         except KeyError:
         if not self.visit(ae, be):
         def visitArray(self, a, b):
      if not isinstance(b, list):
         if len(a) != len(b):
         for ae, be in zip(a, b):
         if not self.visit(ae, be):
         def visitValue(self, a, b):
      if isinstance(a, float) and isinstance(b, float):
         if a == 0:
         else:
   else:
         class Differ(Visitor):
         def __init__(self, stream = sys.stdout, ignore_added = False):
      self.dumper = Dumper(stream)
         def visit(self, a, b):
      if self.comparer.visit(a, b):
               def visitObject(self, a, b):
      if not isinstance(b, dict):
         else:
         self.dumper.enter_object()
   names = set(a.keys())
   if not self.comparer.ignore_added:
                        for i in range(len(names)):
      name = names[i]
   ae = a.get(name, None)
   be = b.get(name, None)
   if not self.comparer.visit(ae, be):
                     def visitArray(self, a, b):
      if not isinstance(b, list):
         else:
         self.dumper.enter_array()
   max_len = max(len(a), len(b))
   for i in range(max_len):
      try:
         except IndexError:
         try:
         except IndexError:
         self.dumper._indent()
   if self.comparer.visit(ae, be):
         else:
                           def visitValue(self, a, b):
      if a != b:
         def replace(self, a, b):
      if isinstance(a, str) and isinstance(b, str):
         if '\n' in a or '\n' in b:
      a = a.splitlines()
   b = b.splitlines()
   differ = difflib.Differ()
   result = differ.compare(a, b)
   self.dumper.level += 1
   for entry in result:
      self.dumper._newline()
   self.dumper._indent()
   tag = entry[:2]
   text = entry[2:]
   if tag == '? ':
         tag = '  '
   prefix = ' '
   text = text.rstrip()
   else:
         prefix = '"'
   line = tag + prefix + text + suffix
         self.dumper.visit(a)
   self.dumper._write(' -> ')
         def isMultilineString(self, value):
            def replaceMultilineString(self, a, b):
      self.dumper.visit(a)
   self.dumper._write(' -> ')
         #
   # Unfortunately JSON standard does not include comments, but this is a quite
   # useful feature to have on regressions tests
   #
      _token_res = [
      r'//[^\r\n]*', # comment
      ]
      _tokens_re = re.compile(r'|'.join(['(' + token_re + ')' for token_re in _token_res]), re.DOTALL)
         def _strip_comment(mo):
      if mo.group(1):
         else:
            def _strip_comments(data):
      '''Strip (non-standard) JSON comments.'''
            assert _strip_comments('''// a comment
   "// a comment in a string
   "''') == '''
   "// a comment in a string
   "'''
         def load(stream, strip_images = True, strip_comments = True):
      if strip_images:
         else:
         if strip_comments:
      data = stream.read()
   data = _strip_comments(data)
      else:
            def main():
      optparser = argparse.ArgumentParser(
         optparser.add_argument("-k", "--keep-images",
      action="store_false", dest="strip_images", default=True,
         optparser.add_argument("ref_json", action="store",
         optparser.add_argument("src_json", action="store",
                     a = load(open(args.ref_json, 'rt'), args.strip_images)
            if False:
      dumper = Dumper()
         differ = Differ()
            if __name__ == '__main__':
      main()
