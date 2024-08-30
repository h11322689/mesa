   #!/usr/bin/env python3
   # coding=utf-8
   ##########################################################################
   #
   # pytracediff - Compare Gallium XML trace files
   # (C) Copyright 2022 Matti 'ccr' Hämäläinen <ccr@tnsp.org>
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
   ##########################################################################
      from parse import *
   import os
   import sys
   import re
   import signal
   import functools
   import argparse
   import difflib
   import subprocess
      assert sys.version_info >= (3, 6), 'Python >= 3.6 required'
         ###
   ### ANSI color codes
   ###
   PKK_ANSI_ESC       = '\33['
   PKK_ANSI_NORMAL    = '0m'
   PKK_ANSI_RED       = '31m'
   PKK_ANSI_GREEN     = '32m'
   PKK_ANSI_YELLOW    = '33m'
   PKK_ANSI_PURPLE    = '35m'
   PKK_ANSI_BOLD      = '1m'
   PKK_ANSI_ITALIC    = '3m'
         ###
   ### Utility functions and classes
   ###
   def pkk_fatal(msg):
      print(f"ERROR: {msg}", file=sys.stderr)
   if outpipe is not None:
                  def pkk_info(msg):
               def pkk_output(outpipe, msg):
      if outpipe is not None:
         else:
            def pkk_signal_handler(signal, frame):
      print("\nQuitting due to SIGINT / Ctrl+C!")
   if outpipe is not None:
                  def pkk_arg_range(vstr, vmin, vmax):
      try:
         except Exception as e:
            value = int(vstr)
   if value < vmin or value > vmax:
         else:
            class PKKArgumentParser(argparse.ArgumentParser):
      def print_help(self):
      print("pytracediff - Compare Gallium XML trace files\n"
   "(C) Copyright 2022 Matti 'ccr' Hämäläinen <ccr@tnsp.org>\n")
   super().print_help()
   print("\nList of junk calls:")
   for klass, call in sorted(trace_ignore_calls):
         def error(self, msg):
      self.print_help()
   print(f"\nERROR: {msg}", file=sys.stderr)
         class PKKTraceParser(TraceParser):
      def __init__(self, stream, options, state):
      TraceParser.__init__(self, stream, options, state)
         def handle_call(self, call):
            class PKKPrettyPrinter(PrettyPrinter):
      def __init__(self, options):
            def entry_start(self, show_args):
      self.data = []
   self.line = ""
         def entry_get(self):
      if self.line != "":
               def text(self, text):
            def literal(self, text):
            def function(self, text):
            def variable(self, text):
            def address(self, text):
            def newline(self):
      self.data.append(self.line)
            def visit_literal(self, node):
      if node.value is None:
         elif isinstance(node.value, str):
         else:
         def visit_blob(self, node):
            def visit_named_constant(self, node):
            def visit_array(self, node):
      self.text("{")
   sep = ""
   for value in node.elements:
         self.text(sep)
   if sep != "":
         value.visit(self)
         def visit_struct(self, node):
      self.text("{")
   sep = ""
   for name, value in node.members:
         self.text(sep)
   if sep != "":
         self.variable(name)
   self.text(" = ")
   value.visit(self)
         def visit_pointer(self, node):
      if self.options.named_ptrs:
         else:
         def visit_call(self, node):
      if not self.options.suppress_variants:
            if node.klass is not None:
         else:
            if not self.options.method_only or self.show_args:
         self.text("(")
   if len(node.args):
      self.newline()
   sep = ""
   for name, value in node.args:
      self.text(sep)
   if sep != "":
         self.variable(name)
   self.text(" = ")
                           if node.ret is not None:
            if not self.options.suppress_variants and node.time is not None:
               def pkk_parse_trace(filename, options, state):
      pkk_info(f"Parsing {filename} ...")
   try:
      if filename.endswith(".gz"):
         from gzip import GzipFile
   elif filename.endswith(".bz2"):
         from bz2 import BZ2File
   else:
         except OSError as e:
            parser = PKKTraceParser(stream, options, state)
                     def pkk_get_line(data, nline):
      if nline < len(data):
         else:
            def pkk_format_line(line, indent, width):
      if line is not None:
      tmp = indent + line
   if len(tmp) > width:
         else:
      else:
            ###
   ### Main program starts
   ###
   if __name__ == "__main__":
      ### Check if output is a terminal
   outpipe = None
            try:
      defwidth = os.get_terminal_size().columns
      except OSError:
                     ### Parse arguments
   optparser = PKKArgumentParser(
            optparser.add_argument("filename1",
      type=str, action="store",
   metavar="<tracefile #1>",
         optparser.add_argument("filename2",
      type=str, action="store",
   metavar="<tracefile #2>",
         optparser.add_argument("-p", "--plain",
      dest="plain",
   action="store_true",
         optparser.add_argument("-S", "--sup-variants",
      dest="suppress_variants",
   action="store_true",
         optparser.add_argument("-C", "--sup-common",
      dest="suppress_common",
   action="store_true",
         optparser.add_argument("-N", "--named",
      dest="named_ptrs",
   action="store_true",
         optparser.add_argument("-M", "--method-only",
      dest="method_only",
   action="store_true",
         optparser.add_argument("-I", "--ignore-junk",
      dest="ignore_junk",
   action="store_true",
         optparser.add_argument("-w", "--width",
      dest="output_width",
   type=functools.partial(pkk_arg_range, vmin=16, vmax=512), default=defwidth,
   metavar="N",
                  ### Parse input files
   stack1 = pkk_parse_trace(options.filename1, options, TraceStateData())
            ### Perform diffing
   pkk_info("Matching trace sequences ...")
            pkk_info("Sequencing diff ...")
   opcodes = sequence.get_opcodes()
   if len(opcodes) == 1 and opcodes[0][0] == "equal":
      print("The files are identical.")
         ### Redirect output to 'less' if stdout is a tty
   try:
      if redirect:
            ### Output results
   pkk_info("Outputting diff ...")
   colwidth = int((options.output_width - 3) / 2)
                     prevtag = ""
   for tag, start1, end1, start2, end2 in opcodes:
         if tag == "equal":
      show_args = False
   if options.suppress_common:
                        sep = "|"
   ansi1 = ansi2 = ansiend = ""
      elif tag == "insert":
      sep = "+"
   ansi1 = ""
   ansi2 = PKK_ANSI_ESC + PKK_ANSI_GREEN
      elif tag == "delete":
      sep = "-"
   ansi1 = PKK_ANSI_ESC + PKK_ANSI_RED
   ansi2 = ""
      elif tag == "replace":
      sep = ">"
   ansi1 = ansi2 = PKK_ANSI_ESC + PKK_ANSI_BOLD
                     # No ANSI, please
   if options.plain:
         else:
                     # Print out the block
   ncall1 = start1
   ncall2 = start2
   last1 = last2 = False
   while True:
      # Get line data
   if ncall1 < end1:
      if not options.ignore_junk or not stack1[ncall1].is_junk:
         printer.entry_start(show_args)
   stack1[ncall1].visit(printer)
   else:
                                 if ncall2 < end2:
      if not options.ignore_junk or not stack2[ncall2].is_junk:
         printer.entry_start(show_args)
   stack2[ncall2].visit(printer)
   else:
                                                      nline = 0
   while nline < len(data1) or nline < len(data2):
      # Determine line start indentation
   if nline > 0:
         if options.suppress_variants:
                                             # Highlight differing lines if not plain
   if not options.plain and line1 != line2:
                              # Output line
   pkk_output(outpipe, colfmt.format(
                                                except Exception as e:
            if outpipe is not None:
      outpipe.communicate()
