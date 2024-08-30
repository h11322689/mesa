   #encoding=utf-8
      # Copyright (C) 2020 Collabora, Ltd.
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
      from asm import parse_asm, ParseError
   import sys
   import struct
      def parse_hex_8(s):
      b = [int(x, base=16) for x in s.split(' ')]
         def hex_8(u64):
      as_bytes = struct.pack('<Q', u64)
   as_strings = [('0' + hex(byte)[2:])[-2:] for byte in as_bytes]
         # These should not throw exceptions
   def positive_test(machine, assembly):
      try:
      expected = parse_hex_8(machine)
   val = parse_asm(assembly)
   if val != expected:
      except ParseError as exc:
         # These should throw exceptions
   def negative_test(assembly):
      try:
      parse_asm(assembly)
      except Exception:
         PASS = []
   FAIL = []
      def record_case(case, error):
      if error is None:
         else:
         if len(sys.argv) < 3:
      print("Expected positive and negative case lists")
         with open(sys.argv[1], "r") as f:
      cases = f.read().split('\n')
            for case in cases:
      (machine, assembly) = case.split('    ')
      with open(sys.argv[2], "r") as f:
      cases = f.read().split('\n')
            for case in cases:
         print("Passed {}/{} tests.".format(len(PASS), len(PASS) + len(FAIL)))
      if len(FAIL) > 0:
      print("Failures:")
   for (fail, err) in FAIL:
      print("")
   print(fail)
      sys.exit(1)
