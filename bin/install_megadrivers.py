   #!/usr/bin/env python3
   # encoding=utf-8
   # Copyright 2017-2018 Intel Corporation
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
      """Script to install megadriver symlinks for meson."""
      import argparse
   import os
         def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('megadriver')
   parser.add_argument('libdir')
   parser.add_argument('drivers', nargs='+')
            if os.path.isabs(args.libdir):
      destdir = os.environ.get('DESTDIR')
   if destdir:
         else:
      else:
                     if not os.path.exists(to):
      if os.path.lexists(to):
               for driver in args.drivers:
               if os.path.lexists(abs_driver):
         print('installing {} to {}'.format(args.megadriver, abs_driver))
            try:
                        name, ext = os.path.splitext(driver)
   while ext != '.so':
      if os.path.lexists(name):
            finally:
         # Remove meson-created master .so and symlinks
   os.unlink(master)
   name, ext = os.path.splitext(master)
   while ext != '.so':
      if os.path.lexists(name):
               if __name__ == '__main__':
      main()
