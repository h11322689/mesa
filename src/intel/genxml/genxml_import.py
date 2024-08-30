   #!/usr/bin/env python3
   # Copyright © 2019, 2022 Intel Corporation
   # SPDX-License-Identifier: MIT
      from __future__ import annotations
   import argparse
   import copy
   import intel_genxml
   import pathlib
   import typing
         def main() -> None:
      parser = argparse.ArgumentParser()
   parser.add_argument('files', nargs='*',
                  g = parser.add_mutually_exclusive_group(required=True)
   g.add_argument('--import', dest='_import', action='store_true',
         g.add_argument('--flatten', action='store_true',
         g.add_argument('--validate', action='store_true',
            parser.add_argument('--quiet', action='store_true')
            filenames = list(args.files)
   intel_genxml.sort_genxml_files(filenames)
   for filename in filenames:
      if not args.quiet:
                     if args.validate:
         original = copy.deepcopy(genxml)
   genxml.optimize_xml_import()
   assert genxml.is_equivalent_xml(original), \
   elif args._import:
         genxml.add_xml_imports()
   genxml.optimize_xml_import()
   elif args.flatten:
                  if not args.quiet:
         if __name__ == '__main__':
      main()
