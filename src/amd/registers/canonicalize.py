   #
   # Copyright 2019 Advanced Micro Devices, Inc.
   #
   # SPDX-License-Identifier: MIT
   #
   """
   Helper script that was used during the generation of the JSON data.
      usage: python3 canonicalize.py FILE
      Reads the register database from FILE, performs canonicalization
   (de-duplication of enums and register types, implicitly sorting JSON by name)
   and attempts to deduce missing register types.
      Notes about deduced register types as well as the output JSON are printed on
   stdout.
   """
      from collections import defaultdict
   import json
   import re
   import sys
      from regdb import RegisterDatabase, deduplicate_enums, deduplicate_register_types
      RE_number = re.compile('[0-9]+')
      def deduce_missing_register_types(regdb):
      """
   This is a heuristic for filling in missing register types based on
   sequentially named registers.
   """
   buckets = defaultdict(list)
   for regmap in regdb.register_mappings():
            for bucket in buckets.values():
      if len(bucket) <= 1:
            regtypenames = set(
         )
   if len(regtypenames) == 1:
         regtypename = regtypenames.pop()
   for regmap in bucket:
               def json_canonicalize(filp, chips = None):
               if chips is not None:
      for regmap in regdb.register_mappings():
               deduplicate_enums(regdb)
   deduplicate_register_types(regdb)
   deduce_missing_register_types(regdb)
                     def main():
            if __name__ == '__main__':
            # kate: space-indent on; indent-width 4; replace-tabs on;
