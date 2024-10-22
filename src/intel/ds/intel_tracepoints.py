   #
   # Copyright © 2021 Intel Corporation
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
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   # OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   # SOFTWARE.
   #
      import argparse
   import sys
      # List of the default tracepoints enabled. By default most tracepoints are
   # enabled, set tp_default=False to disable them by default.
   #
   # Currently only stall is disabled by default
   intel_default_tps = []
      #
   # Tracepoint definitions:
   #
   def define_tracepoints(args):
      from u_trace import Header, HeaderScope
   from u_trace import ForwardDecl
   from u_trace import Tracepoint
   from u_trace import TracepointArg as Arg
            Header('intel_driver_ds.h', scope=HeaderScope.SOURCE)
   Header('blorp/blorp_priv.h', scope=HeaderScope.HEADER)
            def begin_end_tp(name, tp_args=[], tp_struct=None, tp_print=None,
                  global intel_default_tps
   if tp_default_enabled:
         Tracepoint('intel_begin_{0}'.format(name),
               toggle_name=name,
   Tracepoint('intel_end_{0}'.format(name),
               toggle_name=name,
   args=tp_args,
   tp_struct=tp_struct,
   tp_perfetto='intel_ds_end_{0}'.format(name),
         # Frame tracepoints
   begin_end_tp('frame',
                        # Annotations for Queue(Begin|End)DebugUtilsLabelEXT
   begin_end_tp('queue_annotation',
               tp_args=[ArgStruct(type='unsigned', var='len'),
         tp_struct=[Arg(type='uint8_t', name='dummy', var='0', c_format='%hhu'),
            # Batch buffer tracepoints, only for Iris
   begin_end_tp('batch',
                  # Command buffer tracepoints, only for Anv
   begin_end_tp('cmd_buffer',
                  # Annotations for Cmd(Begin|End)DebugUtilsLabelEXT
   begin_end_tp('cmd_buffer_annotation',
               tp_args=[ArgStruct(type='unsigned', var='len'),
                  # Transform feedback, only for Anv
   begin_end_tp('xfb',
            # Dynamic rendering tracepoints, only for Anv
   begin_end_tp('render_pass',
               tp_args=[Arg(type='uint16_t', var='width', c_format='%hu'),
            # Blorp operations, Anv & Iris
   begin_end_tp('blorp',
               tp_args=[Arg(type='enum blorp_op', name='op', var='op', c_format='%s', to_prim_type='blorp_op_to_name({})'),
            Arg(type='uint32_t', name='width', var='width', c_format='%u'),
   Arg(type='uint32_t', name='height', var='height', c_format='%u'),
   Arg(type='uint32_t', name='samples', var='samples', c_format='%u'),
         # Indirect draw generation, only for Anv
            # vkCmdResetQuery, only for Anv
   begin_end_tp('query_clear_blorp',
         begin_end_tp('query_clear_cs',
                  # vkCmdCopyQueryResults, only for Anv
   begin_end_tp('query_copy_cs',
               begin_end_tp('query_copy_shader',
            # Various draws/dispatch, Anv & Iris
   begin_end_tp('draw',
         begin_end_tp('draw_multi',
         begin_end_tp('draw_indexed',
         begin_end_tp('draw_indexed_multi',
         begin_end_tp('draw_indirect_byte_count',
         begin_end_tp('draw_indirect',
         begin_end_tp('draw_indexed_indirect',
         begin_end_tp('draw_indirect_count',
         begin_end_tp('draw_indexed_indirect_count',
            begin_end_tp('draw_mesh',
               tp_args=[Arg(type='uint32_t', var='group_x', c_format='%u'),
         begin_end_tp('draw_mesh_indirect',
         begin_end_tp('draw_mesh_indirect_count',
            begin_end_tp('compute',
               tp_args=[Arg(type='uint32_t', var='group_x', c_format='%u'),
            # Used to identify copies generated by utrace
   begin_end_tp('trace_copy',
         begin_end_tp('trace_copy_cb',
                           begin_end_tp('rays',
               tp_args=[Arg(type='uint32_t', var='group_x', c_format='%u'),
            def flag_bits(args):
      bits = [Arg(type='enum intel_ds_stall_flag', name='flags', var='decode_cb(flags)', c_format='0x%x')]
   for a in args:
               def stall_args(args):
      fmt = ''
   exprs = []
   for a in args:
         fmt += '%s'
   fmt += ' : %s'
   exprs.append('__entry->reason ? __entry->reason : "unknown"')
   # To printout flags
   # fmt += '(0x%08x)'
   # exprs.append('__entry->flags')
   fmt = [fmt]
   fmt += exprs
         stall_flags = [['DEPTH_CACHE_FLUSH',             'depth_flush'],
                  ['DATA_CACHE_FLUSH',              'dc_flush'],
   ['HDC_PIPELINE_FLUSH',            'hdc_flush'],
   ['RENDER_TARGET_CACHE_FLUSH',     'rt_flush'],
   ['TILE_CACHE_FLUSH',              'tile_flush'],
   ['STATE_CACHE_INVALIDATE',        'state_inval'],
   ['CONST_CACHE_INVALIDATE',        'const_inval'],
   ['VF_CACHE_INVALIDATE',           'vf_inval'],
   ['TEXTURE_CACHE_INVALIDATE',      'tex_inval'],
   ['INST_CACHE_INVALIDATE',         'ic_inval'],
   ['STALL_AT_SCOREBOARD',           'pb_stall'],
   ['DEPTH_STALL',                   'depth_stall'],
   ['CS_STALL',                      'cs_stall'],
   ['UNTYPED_DATAPORT_CACHE_FLUSH',  'udp_flush'],
         begin_end_tp('stall',
               tp_args=[ArgStruct(type='uint32_t', var='flags'),
               tp_struct=[Arg(type='uint32_t', name='flags', var='decode_cb(flags)', c_format='0x%x'),
                  def generate_code(args):
      from u_trace import utrace_generate
            utrace_generate(cpath=args.utrace_src, hpath=args.utrace_hdr,
                     utrace_generate_perfetto_utils(hpath=args.perfetto_hdr,
            def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('-p', '--import-path', required=True)
   parser.add_argument('--utrace-src', required=True)
   parser.add_argument('--utrace-hdr', required=True)
   parser.add_argument('--perfetto-hdr', required=True)
   args = parser.parse_args()
   sys.path.insert(0, args.import_path)
   define_tracepoints(args)
            if __name__ == '__main__':
      main()
