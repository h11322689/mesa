   # Copyright © 2022 Intel Corporation
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
      # Converts GLSL shader to SPIR-V library 
      import argparse
   import subprocess
   import sys
   import os
   import typing as T
      if T.TYPE_CHECKING:
      class Arguments(T.Protocol):
      input: str
   output: str
   glslang: str
   create_entry: T.Optional[str]
   glsl_ver: T.Optional[str]
   Olib: bool
   extra: T.Optional[str]
   vn: str
         def get_args() -> 'Arguments':
      parser = argparse.ArgumentParser()
   parser.add_argument('input', help="Name of input file.")
   parser.add_argument('output', help="Name of output file.")
            parser.add_argument("--create-entry",
                  parser.add_argument('--glsl-version',
                        parser.add_argument("-Olib",
                  parser.add_argument("--extra-flags",
                  parser.add_argument("--vn",
                        parser.add_argument("--stage",
                        parser.add_argument("-I",
                              parser.add_argument("-D",
                              args = parser.parse_args()
            def create_include_guard(lines: T.List[str], filename: str) -> T.List[str]:
      filename = filename.replace('.', '_')
            guard_head = [f"#ifndef {upper_name}\n",
                  # remove '#pragma once'
   for idx, l in enumerate(lines):
      if '#pragma once' in l:
                        def convert_to_static_variable(lines: T.List[str], varname: str) -> T.List[str]:
      for idx, l in enumerate(lines):
      if varname in l:
                     def override_version(lines: T.List[str], glsl_version: str) -> T.List[str]:
      for idx, l in enumerate(lines):
      if '#version ' in l:
                     def postprocess_file(args: 'Arguments') -> None:
      with open(args.output, "r") as r:
            # glslangValidator creates a variable without the static modifier.
            # '#pragma once' is unstandardised.
            with open(args.output, "w") as w:
            def preprocess_file(args: 'Arguments', origin_file: T.TextIO, directory: os.PathLike) -> str:
      with open(os.path.join(directory, os.path.basename(origin_file.name)), "w") as copy_file:
               if args.create_entry is not None:
            if args.glsl_ver is not None:
                           def process_file(args: 'Arguments') -> None:
      with open(args.input, "r") as infile:
      copy_file = preprocess_file(args, infile,
                  if args.Olib:
            if args.vn is not None:
            if args.extra is not None:
            if args.create_entry is not None:
            for f in args.includes:
            for d in args.defines:
            cmd_list.extend([
      '-V',
   '-o', args.output,
   '-S', args.stage,
               ret = subprocess.run(cmd_list, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=30)
   if ret.returncode != 0:
      print(ret.stdout)
   print(ret.stderr, file=sys.stderr)
         if args.vn is not None:
            if args.create_entry is not None:
            def main() -> None:
      args = get_args()
            if __name__ == "__main__":
      main()
