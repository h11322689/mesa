   #
   # Copyright (c) 2020 Valve Corporation
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
   import re
   import sys
   import os.path
   import struct
   import string
   import copy
   from math import floor
      if os.isatty(sys.stdout.fileno()):
      set_red = "\033[31m"
   set_green = "\033[1;32m"
      else:
      set_red = ''
   set_green = ''
         initial_code = '''
   import re
      def insert_code(code):
            def insert_pattern(pattern):
            def vector_gpr(prefix, name, size, align):
      insert_code(f'{name} = {name}0')
   for i in range(size):
         insert_code(f'success = {name}0 + {size - 1} == {name}{size - 1}')
   insert_code(f'success = {name}0 % {align} == 0')
         def sgpr_vector(name, size, align):
            funcs.update({
      's64': lambda name: vector_gpr('s', name, 2, 2),
   's96': lambda name: vector_gpr('s', name, 3, 2),
   's128': lambda name: vector_gpr('s', name, 4, 4),
   's256': lambda name: vector_gpr('s', name, 8, 4),
      })
   for i in range(2, 14):
            def _match_func(names):
      for name in names.split(' '):
               funcs['match_func'] = _match_func
      def search_re(pattern):
      global success
         '''
      class Check:
      def __init__(self, data, position):
      self.data = data.rstrip()
         def run(self, state):
         class CodeCheck(Check):
      def run(self, state):
      indent = 0
   first_line = [l for l in self.data.split('\n') if l.strip() != ''][0]
   indent_amount = len(first_line) - len(first_line.lstrip())
   indent = first_line[:indent_amount]
   new_lines = []
   for line in self.data.split('\n'):
         if line.strip() == '':
      new_lines.append('')
      if line[:indent_amount] != indent:
      state.result.log += 'unexpected indent in code check:\n'
   state.result.log += self.data + '\n'
               try:
         exec(code, state.g)
   state.result.log += state.g['log']
   except BaseException as e:
         state.result.log += 'code check at %s raised exception:\n' % self.position
   state.result.log += code + '\n'
   state.result.log += str(e)
   if not state.g['success']:
         state.result.log += 'code check at %s failed:\n' % self.position
   state.result.log += code + '\n'
      class StringStream:
      class Pos:
      def __init__(self):
               def __init__(self, data, name):
      self.name = name
   self.data = data
   self.offset = 0
         def reset(self):
      self.offset = 0
         def peek(self, num=1):
            def peek_test(self, chars):
      c = self.peek(1)
         def read(self, num=4294967296):
      res = self.peek(num)
   self.offset += len(res)
   for c in res:
         if c == '\n':
      self.pos.line += 1
      else:
         def get_line(self, num):
            def read_line(self):
      line = ''
   while self.peek(1) not in ['\n', '']:
         self.read(1)
         def skip_whitespace(self, inc_line):
      chars = [' ', '\t'] + (['\n'] if inc_line else [])
   while self.peek(1) in chars:
         def get_number(self):
      num = ''
   while self.peek() in string.digits:
               def check_identifier(self):
            def get_identifier(self):
      res = ''
   if self.check_identifier():
         while self.peek_test(string.ascii_letters + string.digits + '_'):
      def format_error_lines(at, line_num, column_num, ctx, line):
      pred = '%s line %d, column %d of %s: "' % (at, line_num, column_num, ctx)
   return [pred + line + '"',
         class MatchResult:
      def __init__(self, pattern):
      self.success = True
   self.func_res = None
   self.pattern = pattern
   self.pattern_pos = StringStream.Pos()
   self.output_pos = StringStream.Pos()
         def set_pos(self, pattern, output):
      self.pattern_pos.line = pattern.pos.line
   self.pattern_pos.column = pattern.pos.column
   self.output_pos.line = output.pos.line
         def fail(self, msg):
      self.success = False
         def format_pattern_pos(self):
      pat_pos = self.pattern_pos
   pat_line = self.pattern.get_line(pat_pos.line)
   res = format_error_lines('at', pat_pos.line, pat_pos.column, 'pattern', pat_line)
   func_res = self.func_res
   while func_res:
         pat_pos = func_res.pattern_pos
   pat_line = func_res.pattern.get_line(pat_pos.line)
   res += format_error_lines('in', pat_pos.line, pat_pos.column, func_res.pattern.name, pat_line)
      def do_match(g, pattern, output, skip_lines, in_func=False):
               if not in_func:
                  old_g = copy.copy(g)
   old_g_keys = list(g.keys())
   res = MatchResult(pattern)
   escape = False
   while True:
               c = pattern.read(1)
   fail = False
   if c == '':
         elif output.peek() == '':
         elif c == '\\':
         escape = True
   elif c == '\n':
         old_line = output.pos.line
   output.skip_whitespace(True)
   if output.pos.line == old_line:
   elif not escape and c == '#':
         num = output.get_number()
   if num == '':
         elif pattern.check_identifier():
      name = pattern.get_identifier()
   if name in g and int(num) != g[name]:
            elif not escape and c == '$':
                  val = ''
                  if name in g and val != g[name]:
         elif name != '_':
   elif not escape and c == '%' and pattern.check_identifier():
         if output.read(1) != '%':
         else:
      num = output.get_number()
   if num == '':
         else:
      name = pattern.get_identifier()
   if name in g and int(num) != g[name]:
         elif not escape and c == '@' and pattern.check_identifier():
         name = pattern.get_identifier()
   args = ''
   if pattern.peek_test('('):
      pattern.read(1)
   while pattern.peek() not in ['', ')']:
            func_res = g['funcs'][name](args)
   match_res = do_match(g, StringStream(func_res, 'expansion of "%s(%s)"' % (name, args)), output, False, True)
   if not match_res.success:
      res.func_res = match_res
      elif not escape and c == ' ':
                        read_whitespace = False
   while output.peek_test(' \t'):
      output.read(1)
      if not read_whitespace:
   else:
         outc = output.peek(1)
   if outc != c:
         else:
   if not res.success:
         if skip_lines and output.peek() != '':
      g.clear()
   g.update(old_g)
   res.success = True
   output.read_line()
   pattern.reset()
   output.skip_whitespace(False)
                     if not in_func:
      while output.peek() in [' ', '\t']:
            if output.read(1) not in ['', '\n']:
                     class PatternCheck(Check):
      def __init__(self, data, search, position):
      Check.__init__(self, data, position)
         def run(self, state):
      pattern_stream = StringStream(self.data.rstrip(), 'pattern')
   res = do_match(state.g, pattern_stream, state.g['output'], self.search)
   if not res.success:
         state.result.log += 'pattern at %s failed: %s\n' % (self.position, res.fail_message)
   state.result.log += res.format_pattern_pos() + '\n\n'
   if not self.search:
      out_line = state.g['output'].get_line(res.output_pos.line)
      else:
      state.result.log += 'output was:\n'
         class CheckState:
      def __init__(self, result, variant, checks, output):
      self.result = result
   self.variant = variant
            self.checks.insert(0, CodeCheck(initial_code, None))
            self.g = {'success': True, 'funcs': {}, 'insert_queue': self.insert_queue,
                  class TestResult:
      def __init__(self, expected):
      self.result = ''
   self.expected = expected
      def check_output(result, variant, checks, output):
               while len(state.checks):
      check = state.checks.pop(0)
   state.current_position = check.position
   if not check.run(state):
                  for check in state.insert_queue[::-1]:
               result.result = 'passed'
         def parse_check(variant, line, checks, pos):
      if line.startswith(';'):
      line = line[1:]
   if len(checks) and isinstance(checks[-1], CodeCheck):
         else:
      elif line.startswith('!'):
         elif line.startswith('>>'):
         elif line.startswith('~'):
      end = len(line)
   start = len(line)
   for c in [';', '!', '>>']:
         if line.find(c) != -1 and line.find(c) < end:
   if end != len(line):
         match = re.match(line[1:end], variant)
      def parse_test_source(test_name, variant, fname):
      in_test = False
   test = []
   expected_result = 'passed'
   line_num = 1
   for line in open(fname, 'r').readlines():
      if line.startswith('BEGIN_TEST(%s)' % test_name):
         elif line.startswith('BEGIN_TEST_TODO(%s)' % test_name):
         in_test = True
   elif line.startswith('BEGIN_TEST_FAIL(%s)' % test_name):
         in_test = True
   elif line.startswith('END_TEST'):
         elif in_test:
               checks = []
   for line_num, check in [(line_num, l[2:]) for line_num, l in test if l.startswith('//')]:
                  def parse_and_check_test(test_name, variant, test_file, output, current_result):
               result = TestResult(expected)
   if len(checks) == 0:
      result.result = 'empty'
      elif current_result != None:
         else:
      check_output(result, variant, checks, output)
   if result.result == 'failed' and expected == 'todo':
               def print_results(results, output, expected):
      results = {name: result for name, result in results.items() if result.result == output}
            if not results:
            print('%s tests (%s):' % (output, 'expected' if expected else 'unexpected'))
   for test, result in results.items():
      color = '' if expected else set_red
   print('   %s%s%s' % (color, test, set_normal))
   if result.log.strip() != '':
                           def get_cstr(fp):
      res = b''
   while True:
      c = fp.read(1)
   if c == b'\x00':
         else:
      if __name__ == "__main__":
               stdin = sys.stdin.buffer
   while True:
      packet_type = stdin.read(4)
   if packet_type == b'':
            test_name = get_cstr(stdin)
   test_variant = get_cstr(stdin)
   if test_variant != '':
         else:
            test_source_file = get_cstr(stdin)
   current_result = None
   if ord(stdin.read(1)):
         code_size = struct.unpack("=L", stdin.read(4))[0]
                  result_types = ['passed', 'failed', 'todo', 'empty']
   num_expected = 0
   num_unexpected = 0
   for t in result_types:
         for t in result_types:
         num_expected_skipped = print_results(results, 'skipped', True)
            num_unskipped = len(results) - num_expected_skipped - num_unexpected_skipped
   color = set_red if num_unexpected else set_green
   print('%s%d (%.0f%%) of %d unskipped tests had an expected result%s' % (color, num_expected, floor(num_expected / num_unskipped * 100), num_unskipped, set_normal))
   if num_unexpected_skipped:
            if num_unexpected:
      sys.exit(1)
