   # Copyright © 2021 Igalia S.L.
   # SPDX-License-Identifier: MIT
      import argparse
   import sys
      #
   # TODO can we do this with less boilerplate?
   #
   parser = argparse.ArgumentParser()
   parser.add_argument('-p', '--import-path', required=True)
   parser.add_argument('--utrace-src', required=True)
   parser.add_argument('--utrace-hdr', required=True)
   parser.add_argument('--perfetto-hdr', required=True)
   args = parser.parse_args()
   sys.path.insert(0, args.import_path)
         from u_trace import Header, HeaderScope
   from u_trace import ForwardDecl
   from u_trace import Tracepoint
   from u_trace import TracepointArg as Arg
   from u_trace import TracepointArgStruct as ArgStruct
   from u_trace import utrace_generate
   from u_trace import utrace_generate_perfetto_utils
      Header('vk_enum_to_str.h', scope=HeaderScope.SOURCE|HeaderScope.PERFETTO)
   Header('vk_format.h')
   Header('tu_cmd_buffer.h', scope=HeaderScope.SOURCE)
   Header('tu_device.h', scope=HeaderScope.SOURCE)
      # we can't use tu_common.h because it includes ir3 headers which are not
   # compatible with C++
   ForwardDecl('struct tu_cmd_buffer')
   ForwardDecl('struct tu_device')
   ForwardDecl('struct tu_framebuffer')
   ForwardDecl('struct tu_tiling_config')
      # List of the default tracepoints enabled. By default tracepoints are enabled,
   # set tp_default_enabled=False to disable them by default.
   tu_default_tps = []
      #
   # Tracepoint definitions:
   #
      def begin_end_tp(name, args=[], tp_struct=None, tp_print=None,
                  global tu_default_tps
   if tp_default_enabled:
         Tracepoint('start_{0}'.format(name),
               toggle_name=name,
   args=args,
   tp_struct=tp_struct,
   tp_perfetto='tu_perfetto_start_{0}'.format(name) if queue_tp else None,
   Tracepoint('end_{0}'.format(name),
                     begin_end_tp('cmd_buffer',
      args=[ArgStruct(type='const struct tu_cmd_buffer *', var='cmd')],
   tp_struct=[Arg(type='VkCommandBufferLevel', name='level', var='cmd->vk.level', c_format='%s', to_prim_type='vk_CommandBufferLevel_to_str({})'),
         begin_end_tp('render_pass',
      args=[ArgStruct(type='const struct tu_framebuffer *', var='fb'),
         tp_struct=[Arg(type='uint16_t', name='width',        var='fb->width',                                    c_format='%u'),
               Arg(type='uint16_t', name='height',       var='fb->height',                                   c_format='%u'),
      #    Arg(type='uint8_t',  name='samples',      var='fb->samples',                                  c_format='%u'),
            begin_end_tp('binning_ib')
   begin_end_tp('draw_ib_sysmem')
   begin_end_tp('draw_ib_gmem')
      begin_end_tp('gmem_clear',
      args=[Arg(type='enum VkFormat',  var='format',  c_format='%s', to_prim_type='vk_format_description({})->short_name'),
         begin_end_tp('sysmem_clear',
      args=[Arg(type='enum VkFormat',  var='format',      c_format='%s', to_prim_type='vk_format_description({})->short_name'),
               begin_end_tp('sysmem_clear_all',
      args=[Arg(type='uint8_t',        var='mrt_count',   c_format='%u'),
         begin_end_tp('gmem_load',
      args=[Arg(type='enum VkFormat',  var='format',   c_format='%s', to_prim_type='vk_format_description({})->short_name'),
         begin_end_tp('gmem_store',
      args=[Arg(type='enum VkFormat',  var='format',   c_format='%s', to_prim_type='vk_format_description({})->short_name'),
               begin_end_tp('sysmem_resolve',
            begin_end_tp('blit',
      # TODO: add source megapixels count and target megapixels count arguments
   args=[Arg(type='uint8_t',        var='uses_3d_blit', c_format='%u'),
         Arg(type='enum VkFormat',  var='src_format',   c_format='%s', to_prim_type='vk_format_description({})->short_name'),
         begin_end_tp('compute',
      args=[Arg(type='uint8_t',  var='indirect',       c_format='%u'),
         Arg(type='uint16_t', var='local_size_x',   c_format='%u'),
   Arg(type='uint16_t', var='local_size_y',   c_format='%u'),
   Arg(type='uint16_t', var='local_size_z',   c_format='%u'),
   Arg(type='uint16_t', var='num_groups_x',   c_format='%u'),
            # Annotations for Cmd(Begin|End)DebugUtilsLabelEXT
   for suffix in ["", "_rp"]:
      begin_end_tp('cmd_buffer_annotation' + suffix,
                  args=[ArgStruct(type='unsigned', var='len'),
      utrace_generate(cpath=args.utrace_src,
                  hpath=args.utrace_hdr,
      utrace_generate_perfetto_utils(hpath=args.perfetto_hdr)
