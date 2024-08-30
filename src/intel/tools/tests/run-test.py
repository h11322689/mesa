   #!/usr/bin/env python3
      import argparse
   import difflib
   import errno
   import os
   import pathlib
   import subprocess
   import sys
      # The meson version handles windows paths better, but if it's not available
   # fall back to shlex
   try:
         except ImportError:
            parser = argparse.ArgumentParser()
   parser.add_argument('--i965_asm',
         parser.add_argument('--gen_name',
         parser.add_argument('--gen_folder',
               args = parser.parse_args()
      wrapper = os.environ.get('MESON_EXE_WRAPPER')
   if wrapper is not None:
         else:
            success = True
      for asm_file in args.gen_folder.glob('*.asm'):
      expected_file = asm_file.stem + '.expected'
            try:
      command = i965_asm + [
         '--type', 'hex',
   '--gen', args.gen_name,
   ]
   with subprocess.Popen(command,
                  except OSError as e:
      if e.errno == errno.ENOEXEC:
         print('Skipping due to inability to run host binaries.',
               with expected_path.open() as f:
            diff = ''.join(difflib.unified_diff(lines_before, lines_after,
            if diff:
      print('Output comparison for {}:'.format(asm_file.name))
   print(diff)
      else:
         if not success:
      exit(1)
