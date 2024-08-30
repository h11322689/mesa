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
         import sys
         class Formatter:
               def __init__(self, stream):
            def text(self, text):
            def newline(self):
            def function(self, name):
            def variable(self, name):
            def literal(self, value):
            def address(self, addr):
            class AnsiFormatter(Formatter):
      '''Formatter for plain-text files which outputs ANSI escape codes. See
   http://en.wikipedia.org/wiki/ANSI_escape_code for more information
   concerning ANSI escape codes.
                     _normal = '0m'
   _bold = '1m'
   _italic = '3m' # Not widely supported
   _red = '31m'
   _green = '32m'
            def _escape(self, code):
            def function(self, name):
      self._escape(self._bold)
   Formatter.function(self, name)
         def variable(self, name):
            def literal(self, value):
      self._escape(self._blue)
   Formatter.literal(self, value)
         def address(self, value):
      self._escape(self._green)
   Formatter.address(self, value)
         class WindowsConsoleFormatter(Formatter):
      '''Formatter for the Windows Console. See 
   http://code.activestate.com/recipes/496901/ for more information.
            STD_INPUT_HANDLE  = -10
   STD_OUTPUT_HANDLE = -11
            FOREGROUND_BLUE      = 0x01
   FOREGROUND_GREEN     = 0x02
   FOREGROUND_RED       = 0x04
   FOREGROUND_INTENSITY = 0x08
   BACKGROUND_BLUE      = 0x10
   BACKGROUND_GREEN     = 0x20
   BACKGROUND_RED       = 0x40
            _normal = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED
   _bold = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY
   _italic = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED
   _red = FOREGROUND_RED | FOREGROUND_INTENSITY
   _green = FOREGROUND_GREEN | FOREGROUND_INTENSITY
            def __init__(self, stream):
               if stream is sys.stdin:
         elif stream is sys.stdout:
         elif stream is sys.stderr:
         else:
            if nStdHandle:
         import ctypes
   else:
         def _attribute(self, attr):
      if self.handle:
               def function(self, name):
      self._attribute(self._bold)
   Formatter.function(self, name)
         def variable(self, name):
      self._attribute(self._italic)
   Formatter.variable(self, name)
         def literal(self, value):
      self._attribute(self._blue)
   Formatter.literal(self, value)
         def address(self, value):
      self._attribute(self._green)
   Formatter.address(self, value)
         def DefaultFormatter(stream):
      if sys.platform in ('linux2', 'linux', 'cygwin'):
         elif sys.platform in ('win32', ):
         else:
      