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
         import io
   import sys
   import xml.parsers.expat as xpat
   import argparse
      import format
   from model import *
         trace_ignore_calls = set((
      ("pipe_screen", "is_format_supported"),
   ("pipe_screen", "get_name"),
   ("pipe_screen", "get_vendor"),
   ("pipe_screen", "get_param"),
   ("pipe_screen", "get_paramf"),
   ("pipe_screen", "get_shader_param"),
   ("pipe_screen", "get_compute_param"),
      ))
         def trace_call_ignore(call):
               ELEMENT_START, ELEMENT_END, CHARACTER_DATA, EOF = range(4)
         class XmlToken:
         def __init__(self, type, name_or_data, attrs = None, line = None, column = None):
      assert type in (ELEMENT_START, ELEMENT_END, CHARACTER_DATA, EOF)
   self.type = type
   self.name_or_data = name_or_data
   self.attrs = attrs
   self.line = line
         def __str__(self):
      if self.type == ELEMENT_START:
         if self.type == ELEMENT_END:
         if self.type == CHARACTER_DATA:
         if self.type == EOF:
               class XmlTokenizer:
               def __init__(self, fp, skip_ws = True):
      self.fp = fp
   self.tokens = []
   self.index = 0
   self.final = False
   self.skip_ws = skip_ws
         self.character_pos = 0, 0
   self.character_data = []
         self.parser = xpat.ParserCreate()
   self.parser.StartElementHandler  = self.handle_element_start
   self.parser.EndElementHandler    = self.handle_element_end
         def handle_element_start(self, name, attributes):
      self.finish_character_data()
   line, column = self.pos()
   token = XmlToken(ELEMENT_START, name, attributes, line, column)
         def handle_element_end(self, name):
      self.finish_character_data()
   line, column = self.pos()
   token = XmlToken(ELEMENT_END, name, None, line, column)
         def handle_character_data(self, data):
      if not self.character_data:
               def finish_character_data(self):
      if self.character_data:
         character_data = ''.join(self.character_data)
   if not self.skip_ws or not character_data.isspace(): 
      line, column = self.character_pos
   token = XmlToken(CHARACTER_DATA, character_data, None, line, column)
      def next(self):
      size = 16*1024
   while self.index >= len(self.tokens) and not self.final:
         self.tokens = []
   self.index = 0
   data = self.fp.read(size)
   self.final = len(data) < size
   data = data.rstrip('\0')
   try:
         except xpat.ExpatError as e:
      #if e.code == xpat.errors.XML_ERROR_NO_ELEMENTS:
   if e.code == 3:
            if self.index >= len(self.tokens):
         line, column = self.pos()
   else:
         token = self.tokens[self.index]
         def pos(self):
            class TokenMismatch(Exception):
         def __init__(self, expected, found):
      self.expected = expected
         def __str__(self):
               class XmlParser:
               def __init__(self, fp):
      self.tokenizer = XmlTokenizer(fp)
         def consume(self):
            def match_element_start(self, name):
            def match_element_end(self, name):
            def element_start(self, name):
      while self.token.type == CHARACTER_DATA:
         if self.token.type != ELEMENT_START:
         if self.token.name_or_data != name:
         attrs = self.token.attrs
   self.consume()
         def element_end(self, name):
      while self.token.type == CHARACTER_DATA:
         if self.token.type != ELEMENT_END:
         if self.token.name_or_data != name:
               def character_data(self, strip = True):
      data = ''
   while self.token.type == CHARACTER_DATA:
         data += self.token.name_or_data
   if strip:
               class TraceParser(XmlParser):
         def __init__(self, fp, options, state):
      XmlParser.__init__(self, fp)
   self.last_call_no = 0
   self.state = state
         def parse(self):
      self.element_start('trace')
   while self.token.type not in (ELEMENT_END, EOF):
         call = self.parse_call()
   call.is_junk = trace_call_ignore(call)
   if self.token.type != EOF:
         def parse_call(self):
      attrs = self.element_start('call')
   try:
         except KeyError as e:
         self.last_call_no += 1
   else:
         klass = attrs['class']
   method = attrs['method']
   args = []
   ret = None
   time = None
   while self.token.type == ELEMENT_START:
         if self.token.name_or_data == 'arg':
      arg = self.parse_arg()
      elif self.token.name_or_data == 'ret':
         elif self.token.name_or_data == 'call':
      # ignore nested function calls
      elif self.token.name_or_data == 'time':
         else:
   self.element_end('call')
               def parse_arg(self):
      attrs = self.element_start('arg')
   name = attrs['name']
   value = self.parse_value(name)
                  def parse_ret(self):
      attrs = self.element_start('ret')
   value = self.parse_value('ret')
                  def parse_time(self):
      attrs = self.element_start('time')
   time = self.parse_value('time');
   self.element_end('time')
         def parse_value(self, name):
      expected_tokens = ('null', 'bool', 'int', 'uint', 'float', 'string', 'enum', 'array', 'struct', 'ptr', 'bytes')
   if self.token.type == ELEMENT_START:
         if self.token.name_or_data in expected_tokens:
               def parse_null(self, pname):
      self.element_start('null')
   self.element_end('null')
   return Literal(None)
      def parse_bool(self, pname):
      self.element_start('bool')
   value = int(self.character_data())
   self.element_end('bool')
   return Literal(value)
      def parse_int(self, pname):
      self.element_start('int')
   value = int(self.character_data())
   self.element_end('int')
   return Literal(value)
      def parse_uint(self, pname):
      self.element_start('uint')
   value = int(self.character_data())
   self.element_end('uint')
   return Literal(value)
      def parse_float(self, pname):
      self.element_start('float')
   value = float(self.character_data())
   self.element_end('float')
   return Literal(value)
      def parse_enum(self, pname):
      self.element_start('enum')
   name = self.character_data()
   self.element_end('enum')
   return NamedConstant(name)
      def parse_string(self, pname):
      self.element_start('string')
   value = self.character_data()
   self.element_end('string')
   return Literal(value)
      def parse_bytes(self, pname):
      self.element_start('bytes')
   value = self.character_data()
   self.element_end('bytes')
   return Blob(value)
      def parse_array(self, pname):
      self.element_start('array')
   elems = []
   while self.token.type != ELEMENT_END:
         self.element_end('array')
         def parse_elem(self, pname):
      self.element_start('elem')
   value = self.parse_value('elem')
   self.element_end('elem')
         def parse_struct(self, pname):
      attrs = self.element_start('struct')
   name = attrs['name']
   members = []
   while self.token.type != ELEMENT_END:
         self.element_end('struct')
         def parse_member(self, pname):
      attrs = self.element_start('member')
   name = attrs['name']
   value = self.parse_value(name)
                  def parse_ptr(self, pname):
      self.element_start('ptr')
   address = self.character_data()
                  def handle_call(self, call):
               class SimpleTraceDumper(TraceParser):
         def __init__(self, fp, options, formatter, state):
      TraceParser.__init__(self, fp, options, state)
   self.options = options
   self.formatter = formatter
         def handle_call(self, call):
      if self.options.ignore_junk and call.is_junk:
                  class TraceDumper(SimpleTraceDumper):
         def __init__(self, fp, options, formatter, state):
      SimpleTraceDumper.__init__(self, fp, options, formatter, state)
         def handle_call(self, call):
      if self.options.ignore_junk and call.is_junk:
            if self.options.named_ptrs:
         else:
         class ParseOptions(ModelOptions):
         def __init__(self, args=None):
      # Initialize options local to this module
   self.plain = False
                  class Main:
               def __init__(self):
            def main(self):
      optparser = self.get_optparser()
   args = optparser.parse_args()
            for fname in args.filename:
         try:
      if fname.endswith('.gz'):
      from gzip import GzipFile
      elif fname.endswith('.bz2'):
      from bz2 import BZ2File
      else:
      except Exception as e:
                  def make_options(self, args):
            def get_optparser(self):
      estr = "\nList of junk calls:\n"
   for klass, call in sorted(trace_ignore_calls):
            optparser = argparse.ArgumentParser(
         description="Parse and dump Gallium trace(s)",
            optparser.add_argument("filename", action="extend", nargs="+",
            optparser.add_argument("-p", "--plain",
                  optparser.add_argument("-S", "--suppress",
                  optparser.add_argument("-N", "--named",
                  optparser.add_argument("-M", "--method-only",
                  optparser.add_argument("-I", "--ignore-junk",
                        def process_arg(self, stream, options):
      if options.plain:
         else:
            dump = TraceDumper(stream, options, formatter, TraceStateData())
            if options.named_ptrs:
               if __name__ == '__main__':
      Main().main()
