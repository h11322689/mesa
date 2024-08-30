   #!/usr/bin/env python3
   #
   # Copyright 2012 VMware Inc
   # Copyright 2008-2009 Jose Fonseca
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
      """Perf annotate for JIT code.
      Linux `perf annotate` does not work with JIT code.  This script takes the data
   produced by `perf script` command, plus the diassemblies outputted by gallivm
   into /tmp/perf-XXXXX.map.asm and produces output similar to `perf annotate`.
      See docs/llvmpipe.rst for usage instructions.
      The `perf script` output parser was derived from the gprof2dot.py script.
   """
         import sys
   import os.path
   import re
   import optparse
   import subprocess
         class Parser:
               def __init__(self):
            def parse(self):
            class LineParser(Parser):
               def __init__(self, file):
      Parser.__init__(self)
   self._file = file
   self.__line = None
   self.__eof = False
         def readline(self):
      line = self._file.readline()
   if not line:
         self.__line = ''
   else:
               def lookahead(self):
      assert self.__line is not None
         def consume(self):
      assert self.__line is not None
   line = self.__line
   self.readline()
         def eof(self):
      assert self.__line is not None
         mapFile = None
      def lookupMap(filename, matchSymbol):
      global mapFile
   mapFile = filename
   stream = open(filename, 'rt')
   for line in stream:
               start = int(start, 16)
            if symbol == matchSymbol:
               def lookupAsm(filename, desiredFunction):
      stream = open(filename + '.asm', 'rt')
   while stream.readline() != desiredFunction + ':\n':
            asm = []
   line = stream.readline().strip()
   while line:
      addr, instr = line.split(':', 1)
   addr = int(addr)
   asm.append((addr, instr))
                     samples = {}
         class PerfParser(LineParser):
                           perf record -g
               def __init__(self, infile, symbol):
      LineParser.__init__(self, infile)
         def readline(self):
      # Override LineParser.readline to ignore comment lines
   while True:
         LineParser.readline(self)
         def parse(self):
      # read lookahead
            while not self.eof():
                     addresses = samples.keys()
   addresses.sort()
            sys.stdout.write('%s:\n' % self.symbol)
   for address, instr in asm:
         try:
         except KeyError:
         else:
      sys.stdout.write('%6u' % (sample))
      print('total:', total_samples)
                  def parse_event(self):
      if self.eof():
            line = self.consume()
            callchain = self.parse_callchain()
   if not callchain:
         def parse_callchain(self):
      callchain = []
   while self.lookahead():
         function = self.parse_call(len(callchain) == 0)
   if function is None:
         if self.lookahead() == '':
                        def parse_call(self, first):
      line = self.consume()
   mo = self.call_re.match(line)
   assert mo
   if not mo:
            if not first:
            function_name = mo.group('symbol')
   if not function_name:
                              address = mo.group('address')
            if function_name != self.symbol:
            start_address = lookupMap(module, function_name)
                                    def main():
               optparser = optparse.OptionParser(
         (options, args) = optparser.parse_args(sys.argv[1:])
   if len(args) != 1:
                     p = subprocess.Popen(['perf', 'script'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
   parser = PerfParser(p.stdout, symbol)
            if __name__ == '__main__':
               # vim: set sw=4 et:
