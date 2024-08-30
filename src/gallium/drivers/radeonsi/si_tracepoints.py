   #
   # Copyright 2023 Advanced Micro Devices, Inc.
   #
   # SPDX-License-Identifier: MIT
   #
      import argparse
   import sys
      # List of the default tracepoints enabled. By default most tracepoints are
   # enabled, set tp_default=False to disable them by default.
   #
   si_default_tps = []
      #
   # Tracepoint definitions:
   #
   def define_tracepoints(args):
      from u_trace import Header, HeaderScope
   from u_trace import ForwardDecl
   from u_trace import Tracepoint
   from u_trace import TracepointArg as Arg
            Header('si_perfetto.h', scope=HeaderScope.HEADER)
            def begin_end_tp(name, tp_args=[], tp_struct=None, tp_print=None,
                  global si_default_tps
   if tp_default_enabled:
         Tracepoint('si_begin_{0}'.format(name),
               toggle_name=name,
   Tracepoint('si_end_{0}'.format(name),
               toggle_name=name,
   args=tp_args,
   tp_struct=tp_struct,
   tp_perfetto='si_ds_end_{0}'.format(name),
         # Various draws/dispatch, radeonsi
   begin_end_tp('draw',
            begin_end_tp('compute',
               tp_args=[Arg(type='uint32_t', var='group_x', c_format='%u'),
         def generate_code(args):
      from u_trace import utrace_generate
            utrace_generate(cpath=args.src, hpath=args.hdr,
                           def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('-p', '--import-path', required=True)
   parser.add_argument('-C','--src', required=True)
   parser.add_argument('-H','--hdr', required=True)
   parser.add_argument('--perfetto-hdr', required=True)
   args = parser.parse_args()
   sys.path.insert(0, args.import_path)
   define_tracepoints(args)
         if __name__ == '__main__':
      main()
