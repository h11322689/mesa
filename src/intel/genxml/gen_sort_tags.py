   #!/usr/bin/env python3
   # Copyright © 2019, 2022 Intel Corporation
   # SPDX-License-Identifier: MIT
      from __future__ import annotations
   import argparse
   import copy
   import intel_genxml
   import pathlib
   import xml.etree.ElementTree as et
   import typing
         def main() -> None:
      parser = argparse.ArgumentParser()
   parser.add_argument('files', nargs='*',
               parser.add_argument('--validate', action='store_true')
   parser.add_argument('--quiet', action='store_true')
            for filename in args.files:
      if not args.quiet:
                     if args.validate:
         assert genxml.is_equivalent_xml(genxml.sorted_copy()), \
   else:
                  if not args.quiet:
         if __name__ == '__main__':
      main()
