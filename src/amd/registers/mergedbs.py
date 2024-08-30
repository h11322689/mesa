   #
   # Copyright 2017-2019 Advanced Micro Devices, Inc.
   #
   # SPDX-License-Identifier: MIT
   #
   """
   Helper script to merge register database JSON files.
      usage: python3 mergedbs.py [FILES...]
      Will merge the given JSON files and output the result on stdout.
   """
      from collections import defaultdict
   import json
   import re
   import sys
      from regdb import RegisterDatabase, deduplicate_enums, deduplicate_register_types
      def main():
      regdb = RegisterDatabase()
   for filename in sys.argv[1:]:
      with open(filename, 'r') as filp:
         deduplicate_enums(regdb)
                     if __name__ == '__main__':
            # kate: space-indent on; indent-width 4; replace-tabs on;
