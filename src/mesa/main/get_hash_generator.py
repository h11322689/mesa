   # coding=utf-8
   # -*- Mode: Python; py-indent-offset: 4 -*-
   #
   # Copyright © 2012 Intel Corporation
   #
   # Based on code by Kristian Høgsberg <krh@bitplanet.net>,
   #   extracted from mesa/main/get.c
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # on the rights to use, copy, modify, merge, publish, distribute, sub
   # license, and/or sell copies of the Software, and to permit persons to whom
   # the Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   # IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
      # Generate a C header file containing hash tables of glGet parameter
   # names for each GL API. The generated file is to be included by glGet.c
      import os, sys, getopt
   from collections import defaultdict
   import get_hash_params
      param_desc_file = os.path.join(os.path.dirname(__file__), "get_hash_params.py")
      GLAPI = os.path.join(os.path.dirname(__file__), "..", "..", "mapi", "glapi", "gen")
   sys.path.insert(0, GLAPI)
   import gl_XML
      prime_factor = 89
   prime_step = 281
   hash_table_size = 1024
      gl_apis=set(["GL", "GL_CORE", "GLES", "GLES2", "GLES3", "GLES31", "GLES32"])
      def print_header():
      print("typedef const unsigned short table_t[%d];\n" % (hash_table_size))
   print("static const int prime_factor = %d, prime_step = %d;\n" % \
         def print_params(params):
      print("static const struct value_desc values[] = {")
   for p in params:
                  def api_name(api):
            # This must match gl_api enum in src/mesa/main/mtypes.h
   api_enum = [
      'GL',
   'GLES',
   'GLES2',
   'GL_CORE',
   'GLES3', # Not in gl_api enum in mtypes.h
   'GLES31', # Not in gl_api enum in mtypes.h
      ]
      def api_index(api):
            def table_name(api):
            def print_table(api, table):
               # convert sparse (index, value) table into a dense table
   dense_table = [0] * hash_table_size
   for i, v in table:
            row_size = 4
   for i in range(0, hash_table_size, row_size):
      row = dense_table[i : i + row_size]
   idx_val = ["%4d" % v for v in row]
               def print_tables(tables):
      for table in tables:
            dense_tables = ['NULL'] * len(api_enum)
   for table in tables:
      tname = table_name(table["apis"][0])
   for api in table["apis"]:
               print("static table_t *table_set[] = {")
   for expr in dense_tables:
                        # Merge tables with matching parameter lists (i.e. GL and GL_CORE)
   def merge_tables(tables):
      merged_tables = []
   for api, indices in sorted(tables.items()):
      matching_table = list(filter(lambda mt:mt["indices"] == indices,
         if matching_table:
         else:
               def add_to_hash_table(table, hash_val, value):
      while True:
      index = hash_val & (hash_table_size - 1)
   if index not in table:
      table[index] = value
         def die(msg):
      sys.stderr.write("%s: %s\n" % (program, msg))
         program = os.path.basename(sys.argv[0])
      def generate_hash_tables(enum_list, enabled_apis, param_descriptors):
               # the first entry should be invalid, so that get.c:find_value can use
   # its index for the 'enum not found' condition.
            for param_block in param_descriptors:
      if set(["apis", "params"]) != set(param_block):
      die("missing fields (%s) in param descriptor file (%s)" %
               valid_apis = set(param_block["apis"])
   if valid_apis - gl_apis:
                  if not (valid_apis & enabled_apis):
                     for param in param_block["params"]:
      enum_name = param[0]
                  for api in valid_apis:
      add_to_hash_table(tables[api], hash_val, len(params))
   # Also add GLES2 items to the GLES3+ hash tables
   if api == "GLES2":
      add_to_hash_table(tables["GLES3"], hash_val, len(params))
   add_to_hash_table(tables["GLES31"], hash_val, len(params))
      # Also add GLES3 items to the GLES31+ hash tables
   if api == "GLES3":
      add_to_hash_table(tables["GLES31"], hash_val, len(params))
      # Also add GLES31 items to the GLES32+ hash tables
   if api == "GLES31":
         sorted_tables={}
   for api, indices in tables.items():
                     def show_usage():
         """Usage: %s [OPTIONS]
   -f <file>          specify GL API XML file
   """ % (program))
            if __name__ == '__main__':
      try:
         except Exception:
            if len(args) != 0:
                     for opt_name, opt_val in opts:
      if opt_name == "-f":
         if not api_desc_file:
            # generate the code for all APIs
   enabled_apis = set(["GLES", "GLES2", "GLES3", "GLES31", "GLES32",
            try:
         except Exception:
            (params, hash_tables) = generate_hash_tables(api_desc.enums_by_name,
            print_header()
   print_params(params)
   print_tables(hash_tables)
