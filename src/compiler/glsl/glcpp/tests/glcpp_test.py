   # encoding=utf-8
   # Copyright © 2018 Intel Corporation
      # Permission is hereby granted, free of charge, to any person obtaining a copy
   # of this software and associated documentation files (the "Software"), to deal
   # in the Software without restriction, including without limitation the rights
   # to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   # copies of the Software, and to permit persons to whom the Software is
   # furnished to do so, subject to the following conditions:
      # The above copyright notice and this permission notice shall be included in
   # all copies or substantial portions of the Software.
      # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   # AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   # OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   # SOFTWARE.
      """Run glcpp tests with various line endings."""
      import argparse
   import difflib
   import errno
   import io
   import os
   import subprocess
   import sys
      # The meson version handles windows paths better, but if it's not available
   # fall back to shlex
   try:
         except ImportError:
               def arg_parser():
      parser = argparse.ArgumentParser()
   parser.add_argument('glcpp', help='Path to the he glcpp binary.')
   parser.add_argument('testdir', help='Path to tests and expected output.')
   parser.add_argument('--unix', action='store_true', help='Run tests for Unix style newlines')
   parser.add_argument('--windows', action='store_true', help='Run tests for Windows/Dos style newlines')
   parser.add_argument('--oldmac', action='store_true', help='Run tests for Old Mac (pre-OSX) style newlines')
   parser.add_argument('--bizarro', action='store_true', help='Run tests for Bizarro world style newlines')
            def parse_test_file(contents, nl_format):
      """Check for any special arguments and return them as a list."""
   # Disable "universal newlines" mode; we can't directly use `nl_format` as
   # the `newline` argument, because the "bizarro" test uses something Python
   # considers invalid.
   for l in contents.decode('utf-8').split(nl_format):
                        def test_output(glcpp, contents, expfile, nl_format='\n'):
      """Test that the output of glcpp is what we expect."""
            proc = subprocess.Popen(
      glcpp + extra_args,
   stdout=subprocess.PIPE,
   stderr=subprocess.STDOUT,
      actual, _ = proc.communicate(contents)
            if proc.returncode == 255:
      print("Test returned general error, possibly missing linker")
         with open(expfile, 'rb') as f:
            # Bison 3.6 changed '$end' to 'end of file' in its error messages
   # See: https://gitlab.freedesktop.org/mesa/mesa/-/issues/3181
            if actual == expected:
                  def test_unix(args):
      """Test files with unix style (\n) new lines."""
   total = 0
            print('============= Testing for Correctness (Unix) =============')
   for filename in os.listdir(args.testdir):
      if not filename.endswith('.c'):
            print(   '{}:'.format(os.path.splitext(filename)[0]), end=' ')
            testfile = os.path.join(args.testdir, filename)
   with open(testfile, 'rb') as f:
         valid, diff = test_output(args.glcpp, contents, testfile + '.expected')
   if valid:
         passed += 1
   else:
         print('FAIL')
         if not total:
            print('{}/{}'.format(passed, total), 'tests returned correct results')
            def _replace_test(args, replace):
      """Test files with non-unix style line endings. Print your own header."""
   total = 0
            for filename in os.listdir(args.testdir):
      if not filename.endswith('.c'):
            print(   '{}:'.format(os.path.splitext(filename)[0]), end=' ')
   total += 1
            with open(testfile, 'rt') as f:
         contents = contents.replace('\n', replace).encode('utf-8')
   valid, diff = test_output(
            if valid:
         passed += 1
   else:
         print('FAIL')
         if not total:
            print('{}/{}'.format(passed, total), 'tests returned correct results')
            def test_windows(args):
      """Test files with windows/dos style (\r\n) new lines."""
   print('============= Testing for Correctness (Windows) =============')
            def test_oldmac(args):
      """Test files with Old Mac style (\r) new lines."""
   print('============= Testing for Correctness (Old Mac) =============')
            def test_bizarro(args):
      """Test files with Bizarro world style (\n\r) new lines."""
   # This is allowed by the spec, but why?
   print('============= Testing for Correctness (Bizarro) =============')
            def main():
               wrapper = os.environ.get('MESON_EXE_WRAPPER')
   if wrapper is not None:
         else:
            success = True
   try:
      if args.unix:
         if args.windows:
         if args.oldmac:
         if args.bizarro:
      except OSError as e:
      if e.errno == errno.ENOEXEC:
         print('Skipping due to inability to run host binaries.',
                        if __name__ == '__main__':
      main()
