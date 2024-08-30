   #!/usr/bin/env python3
   # coding=utf-8
   ##########################################################################
   #
   # enums2names - Parse and convert enums to translator code
   # (C) Copyright 2021 Matti 'ccr' Hämäläinen <ccr@tnsp.org>
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
   import os.path
   import re
   import signal
   import argparse
   import textwrap
      assert sys.version_info >= (3, 6)
         #
   # List of enums we wish to include in output.
   # NOTE: This needs to be updated if such enums are added.
   #
   lst_enum_include = [
      "pipe_texture_target",
   "pipe_shader_cap",
   "pipe_shader_ir",
   "pipe_map_flags",
   "pipe_cap",
   "pipe_capf",
   "pipe_compute_cap",
   "pipe_video_cap",
   "pipe_video_profile",
   "pipe_video_entrypoint",
   "pipe_video_vpp_orientation",
   "pipe_video_vpp_blend_mode",
   "pipe_resource_param",
   "pipe_fd_type",
   "pipe_blendfactor",
   "pipe_blend_func",
      ]
         ###
   ### Utility functions
   ###
   ## Fatal error handler
   def pkk_fatal(smsg):
      print("ERROR: "+ smsg)
            ## Handler for SIGINT signals
   def pkk_signal_handler(signal, frame):
      print("\nQuitting due to SIGINT / Ctrl+C!")
            ## Argument parser subclass
   class PKKArgumentParser(argparse.ArgumentParser):
      def print_help(self):
      print("enums2names - Parse and convert enums to translator code\n"
   "(C) Copyright 2021 Matti 'ccr' Hämäläinen <ccr@tnsp.org>\n")
         def error(self, msg):
      self.print_help()
   print(f"\nERROR: {msg}", file=sys.stderr)
         def pkk_get_argparser():
      optparser = PKKArgumentParser(
      usage="%(prog)s [options] <infile|->\n"
   "example: %(prog)s ../../include/pipe/p_defines.h -C tr_util.c -H tr_util.h"
         optparser.add_argument("in_files",
      type=str,
   metavar="infiles",
   nargs="+",
         optparser.add_argument("-C",
      type=str,
   metavar="outfile",
   dest="out_source",
         optparser.add_argument("-H",
      type=str,
   metavar="outfile",
   dest="out_header",
         optparser.add_argument("-I",
      type=str,
   metavar="include",
   dest="include_file",
                  class PKKHeaderParser:
         def __init__(self, nfilename):
      self.filename = nfilename
   self.enums = {}
   self.state = 0
   self.nline = 0
   self.mdata = []
   self.start = 0
   self.name = None
         def error(self, msg):
            def parse_line(self, sline: str):
      start = sline.find('/*')
   end = sline.find('*/')
   if not self.in_multiline_comment and start >= 0:
         if end >= 0:
      assert end > start
      else:
         elif self.in_multiline_comment and end >= 0:
         self.in_multiline_comment = False
   elif self.in_multiline_comment:
         # A kingdom for Py3.8 := operator ...
   smatch = re.match(r'^enum\s+([A-Za-z0-9_]+)\s+.*;', sline)
   if smatch:
         else:
         smatch = re.match(r'^enum\s+([A-Za-z0-9_]+)', sline)
                                    self.name = stmp
   self.state = 1
   self.start = self.nline
      else:
      smatch = re.match(r'^}(\s*|\s*[A-Z][A-Z_]+\s*);', sline)
   if smatch:
      if self.state == 1:
                                    self.enums[self.name] = {
                              elif self.state == 1:
      smatch = re.match(r'([A-Za-z0-9_]+)\s*=\s*(.+)\s*,?', sline)
   if smatch:
         else:
         def parse_file(self, fh):
      self.nline = 0
   for line in fh:
                        def pkk_output_header(fh):
      prototypes = [f"const char *\n"
            print(textwrap.dedent("""\
      /*
      * File generated with {program}, please do not edit manually.
      #ifndef {include_header_guard}
               #include "pipe/p_defines.h"
               #ifdef __cplusplus
   extern "C" {{
                     #ifdef __cplusplus
   }}
            #endif /* {include_header_guard} */\
   """).format(
         program=pkk_progname,
   include_header_guard=re.sub(r'[^A-Z]', '_', os.path.basename(pkk_cfg.out_header).upper()),
         def pkk_output_source(fh):
      if pkk_cfg.include_file == None:
            print(textwrap.dedent("""\
      /*
      * File generated with {program}, please do not edit manually.
      #include "{include_file}"
   """).format(
         program=pkk_progname,
         for name in lst_enum_include:
      cases = [f"      case {eid}: return \"{eid}\";\n"
                           const char *
   tr_util_{name}_name(enum {name} value)
   {{
         {cases}
               }}
   """).format(
      name=name,
         ###
   ### Main program starts
   ###
   if __name__ == "__main__":
               ### Parse arguments
   pkk_progname = sys.argv[0]
   optparser = pkk_get_argparser()
            ### Parse input files
   enums = {}
   for file in pkk_cfg.in_files:
               try:
         if file != "-":
      with open(file, "r", encoding="UTF-8") as fh:
      else:
         except OSError as e:
         ### Check if any of the required enums are missing
   errors = False
   for name in lst_enum_include:
      if name not in enums:
               if errors:
            ### Perform output
   if pkk_cfg.out_header:
      with open(pkk_cfg.out_header, "w", encoding="UTF-8") as fh:
         if pkk_cfg.out_source:
      with open(pkk_cfg.out_source, "w", encoding="UTF-8") as fh:
         pkk_output_source(fh)
