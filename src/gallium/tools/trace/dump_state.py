   #!/usr/bin/env python3
   ##########################################################################
   #
   # Copyright 2008-2021, VMware, Inc.
   # All Rights Reserved.
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
   import struct
   import json
   import binascii
   import re
   import copy
   import argparse
      import model
   import format
   import parse as parser
         try:
         except ImportError:
      def unpack_from(fmt, buf, offset=0):
      size = struct.calcsize(fmt)
      #
   # Some constants
   #
   PIPE_BUFFER = 'PIPE_BUFFER'
         def serialize(obj):
      '''JSON serializer function for non-standard Python objects.'''
   if isinstance(obj, bytearray) or isinstance(obj, bytes):
      # TODO: Decide on a single way of dumping blobs
   if False:
         # Don't dump full blobs, but merely a description of their size and
   # CRC32 hash.
   crc32 = binascii.crc32(obj)
   if crc32 < 0:
         if True:
         # Dump blobs as an array of 16byte hexadecimals
   res = []
   for i in range(0, len(obj), 16):
         # Dump blobs as a single hexadecimal string
         # If the object has a __json__ method, use it.
   try:
         except AttributeError:
         else:
            class Struct:
               Python doesn't have C structs, but do its dynamic nature, any object is
   pretty close.
            def __json__(self):
      '''Convert the structure to a standard Python dict, so it can be
            obj = {}
   for name, value in self.__dict__.items():
         if not name.startswith('_'):
         def __repr__(self):
            class Translator(model.Visitor):
               def __init__(self, interpreter):
      self.interpreter = interpreter
         def visit(self, node):
      self.result = None
   node.visit(self)
         def visit_literal(self, node):
            def visit_blob(self, node):
            def visit_named_constant(self, node):
            def visit_array(self, node):
      array = []
   for element in node.elements:
               def visit_struct(self, node):
      struct = Struct()
   for member_name, member_node in node.members:
         member_name = member_name.replace('.', '_')
   member_value = self.visit(member_node)
         def visit_pointer(self, node):
            class Dispatcher:
               def __init__(self, interpreter):
            class Global(Dispatcher):
               For calls that are not associated with objects, i.e, functions and not
   methods.
            def pipe_screen_create(self):
            def pipe_context_create(self, screen):
            class Transfer:
               def __init__(self, resource, usage, subresource, box):
      self.resource = resource
   self.usage = usage
   self.subresource = subresource
         class Screen(Dispatcher):
               def __init__(self, interpreter):
            def destroy(self):
            def context_create(self, priv=None, flags=0):
            def resource_create(self, templat):
      resource = templat
   # Normalize state to avoid spurious differences
   if resource.nr_samples == 0:
         if resource.target == PIPE_BUFFER:
         # We will keep track of buffer contents
   resource.data = bytearray(resource.width)
   # Ignore format
                  def allocate_memory(screen, size):
            def free_memory(screen, pmem):
            def map_memory(screen, pmem):
            def unmap_memory(screen, pmem):
            def resource_bind_backing(screen, resource, pmem, offset):
            def resource_destroy(self, resource):
            def fence_finish(self, fence, timeout=None, ctx=None):
            def fence_signalled(self, fence):
            def fence_reference(self, dst, src):
            def flush_frontbuffer(self, resource):
            class Context(Dispatcher):
                        def __init__(self, interpreter):
               # Setup initial state
   self._state = Struct()
   self._state.scissors = []
   self._state.viewports = []
   self._state.vertex_buffers = []
   self._state.vertex_elements = []
   self._state.vs = Struct()
   self._state.tcs = Struct()
   self._state.tes = Struct()
   self._state.gs = Struct()
   self._state.fs = Struct()
   self._state.vs.shader = None
   self._state.tcs.shader = None
   self._state.tes.shader = None
   self._state.gs.shader = None
   self._state.fs.shader = None
   self._state.vs.sampler = []
   self._state.tcs.sampler = []
   self._state.tes.sampler = []
   self._state.gs.sampler = []
   self._state.fs.sampler = []
   self._state.vs.sampler_views = []
   self._state.tcs.sampler_views = []
   self._state.tes.sampler_views = []
   self._state.gs.sampler_views = []
   self._state.fs.sampler_views = []
   self._state.vs.constant_buffer = []
   self._state.tcs.constant_buffer = []
   self._state.tes.constant_buffer = []
   self._state.gs.constant_buffer = []
   self._state.fs.constant_buffer = []
   self._state.render_condition_condition = 0
   self._state.render_condition_mode = 0
                  def destroy(self):
            def create_blend_state(self, state):
      # Normalize state to avoid spurious differences
   if not state.logicop_enable:
         if not state.rt[0].blend_enable:
         del state.rt[0].rgb_src_factor
   del state.rt[0].rgb_dst_factor
   del state.rt[0].rgb_func
   del state.rt[0].alpha_src_factor
   del state.rt[0].alpha_dst_factor
         def bind_blend_state(self, state):
      # Normalize state
         def delete_blend_state(self, state):
            def create_sampler_state(self, state):
            def delete_sampler_state(self, state):
            def bind_sampler_states(self, shader, start, num_states, states):
      # FIXME: Handle non-zero start
   assert start == 0
         def bind_vertex_sampler_states(self, num_states, states):
      # XXX: deprecated method
         def bind_geometry_sampler_states(self, num_states, states):
      # XXX: deprecated method
         def bind_fragment_sampler_states(self, num_states, states):
      # XXX: deprecated method
         def create_rasterizer_state(self, state):
            def bind_rasterizer_state(self, state):
            def delete_rasterizer_state(self, state):
            def create_depth_stencil_alpha_state(self, state):
      # Normalize state to avoid spurious differences
   if not state.alpha_enabled:
         del state.alpha_func
   for i in range(2):
         if not state.stencil[i].enabled:
         def bind_depth_stencil_alpha_state(self, state):
            def delete_depth_stencil_alpha_state(self, state):
                     def _create_shader_state(self, state):
      # Strip the labels from the tokens
   if state.tokens is not None:
               create_vs_state = _create_shader_state
   create_tcs_state = _create_shader_state
   create_tes_state = _create_shader_state
   create_gs_state = _create_shader_state
            def bind_vs_state(self, state):
            def bind_tcs_state(self, state):
            def bind_tes_state(self, state):
            def bind_gs_state(self, state):
            def bind_fs_state(self, state):
            def bind_tcz_state(self, state):
            def _delete_shader_state(self, state):
            delete_vs_state = _delete_shader_state
   delete_tcs_state = _delete_shader_state
   delete_tes_state = _delete_shader_state
   delete_gs_state = _delete_shader_state
            def set_blend_color(self, state):
            def set_stencil_ref(self, state):
            def set_clip_state(self, state):
            def _dump_constant_buffer(self, buffer):
      if not self.interpreter.verbosity(2):
            data = self.real.buffer_read(buffer)
   format = '4f'
   index = 0
   for offset in range(0, len(data), struct.calcsize(format)):
         x, y, z, w = unpack_from(format, data, offset)
   sys.stdout.write('\tCONST[%2u] = {%10.4f, %10.4f, %10.4f, %10.4f}\n' % (index, x, y, z, w))
         def _get_stage_state(self, shader):
      if shader == 'PIPE_SHADER_VERTEX':
         if shader == 'PIPE_SHADER_TESS_CTRL':
         if shader == 'PIPE_SHADER_TESS_EVAL':
         if shader == 'PIPE_SHADER_GEOMETRY':
         if shader == 'PIPE_SHADER_FRAGMENT':
               def set_constant_buffer(self, shader, index, take_ownership, constant_buffer):
            def set_framebuffer_state(self, state):
            def set_polygon_stipple(self, state):
            def _update(self, array, start_slot, num_slots, states):
      if not isinstance(states, list):
         # XXX: trace is not serializing multiple scissors/viewports properly yet
   num_slots = 1
   while len(array) < start_slot + num_slots:
         for i in range(num_slots):
         def set_scissor_states(self, start_slot, num_scissors, states):
            def set_viewport_states(self, start_slot, num_viewports, states):
            def create_sampler_view(self, resource, templ):
      templ.resource = resource
         def sampler_view_destroy(self, view):
            def set_sampler_views(self, shader, start, num, unbind_num_trailing_slots, take_ownership, views):
      # FIXME: Handle non-zero start
   assert start == 0
         def set_fragment_sampler_views(self, num, views):
      # XXX: deprecated
         def set_geometry_sampler_views(self, num, views):
      # XXX: deprecated
         def set_vertex_sampler_views(self, num, views):
      # XXX: deprecated
         def set_vertex_buffers(self, num_buffers, unbind_num_trailing_slots, take_ownership, buffers):
            def create_vertex_elements_state(self, num_elements, elements):
            def bind_vertex_elements_state(self, state):
            def delete_vertex_elements_state(self, state):
            def set_index_buffer(self, ib):
            def set_patch_vertices(self, patch_vertices):
            # Don't dump more than this number of indices/vertices
            def _merge_indices(self, info):
                        format = {
         1: 'B',
   2: 'H',
                     if self._state.index_buffer.buffer is None:
                  data = self._state.index_buffer.buffer.data
            count = min(info.count, self.MAX_ELEMENTS)
   indices = []
   for i in range(info.start, info.start + count):
         offset = self._state.index_buffer.offset + i*index_size
   if offset + index_size > len(data):
         else:
         indices.append(index)
                           def _merge_vertices(self, start, count):
               count = min(count, self.MAX_ELEMENTS)
   vertices = []
   for index in range(start, start + count):
         if index >= start + 16:
      sys.stdout.write('\t...\n')
      vertex = []
   for velem in self._state.vertex_elements:
      vbuf = self._state.vertex_buffers[velem.vertex_buffer_index]
                                 offset = vbuf.buffer_offset + velem.src_offset + vbuf.stride*index
   format = {
      'PIPE_FORMAT_R32_FLOAT': 'f',
   'PIPE_FORMAT_R32G32_FLOAT': '2f',
   'PIPE_FORMAT_R32G32B32_FLOAT': '3f',
   'PIPE_FORMAT_R32G32B32A32_FLOAT': '4f',
   'PIPE_FORMAT_R32_UINT': 'I',
   'PIPE_FORMAT_R32G32_UINT': '2I',
   'PIPE_FORMAT_R32G32B32_UINT': '3I',
   'PIPE_FORMAT_R32G32B32A32_UINT': '4I',
   'PIPE_FORMAT_R8_UINT': 'B',
   'PIPE_FORMAT_R8G8_UINT': '2B',
   'PIPE_FORMAT_R8G8B8_UINT': '3B',
   'PIPE_FORMAT_R8G8B8A8_UINT': '4B',
   'PIPE_FORMAT_A8R8G8B8_UNORM': '4B',
   'PIPE_FORMAT_R8G8B8A8_UNORM': '4B',
                                                def render_condition(self, query, condition = 0, mode = 0):
      self._state.render_condition_query = query
   self._state.render_condition_condition = condition
         def set_stream_output_targets(self, num_targets, tgs, offsets):
      self._state.so_targets = tgs
         def set_sample_mask(self, sample_mask):
            def set_min_samples(self, min_samples):
            def draw_vbo(self, info, drawid_offset, indirect, draws, num_draws):
               if self.interpreter.call_no < self.interpreter.options.call and \
                                    if info.index_size != 0:
         else:
         min_index = draws[0].start
                           def _normalize_stage_state(self, stage):
      if stage.shader is not None and stage.shader.tokens is not None:
                  for mo in self._dclRE.finditer(stage.shader.tokens):
      file_ = mo.group(1)
                                    mapping = [
      #("CONST", "constant_buffer"),
                     for fileName, attrName in mapping:
      register = registers.setdefault(fileName, set())
   attr = getattr(stage, attrName)
   for index in range(len(attr)):
      if index not in register:
         def _dump_state(self):
                        self._normalize_stage_state(state.vs)
   self._normalize_stage_state(state.tcs)
   self._normalize_stage_state(state.tes)
   self._normalize_stage_state(state.gs)
            json.dump(
         obj = state,
   fp = sys.stdout,
   default = serialize,
   sort_keys = True,
   indent = 4,
                  def resource_copy_region(self, dst, dst_level, dstx, dsty, dstz, src, src_level, src_box):
      if dst.target == PIPE_BUFFER or src.target == PIPE_BUFFER:
         assert dst.target == PIPE_BUFFER and src.target == PIPE_BUFFER
   assert dst_level == 0
   assert dsty == 0
   assert dstz == 0
   assert src_level == 0
   assert src_box.y == 0
   assert src_box.z == 0
   assert src_box.height == 1
   assert src_box.depth == 1
         def is_resource_referenced(self, texture, face, level):
            def get_transfer(self, texture, sr, usage, box):
      if texture is None:
         transfer = Transfer(texture, sr, usage, box)
         def tex_transfer_destroy(self, transfer):
            def buffer_subdata(self, resource, usage, data, box=None, offset=None, size=None, level=None, stride=None, layer_stride=None):
      if box is not None:
         # XXX trace_context_transfer_unmap generates brokens buffer_subdata
   assert offset is None
   assert size is None
   assert level == 0
   offset = box.x
            if resource is not None and resource.target == PIPE_BUFFER:
         data = data.getValue()
   assert len(data) >= size
         def texture_subdata(self, resource, level, usage, box, data, stride, layer_stride):
            def transfer_inline_write(self, resource, level, usage, box, stride, layer_stride, data):
      if resource is not None and resource.target == PIPE_BUFFER:
         data = data.getValue()
   assert len(data) >= box.width
         def flush(self, flags):
      # Return a fake fence
         def clear(self, buffers, color, depth, stencil, scissor_state=None):
            def clear_render_target(self, dst, rgba, dstx, dsty, width, height, render_condition_enabled):
            def clear_depth_stencil(self, dst, clear_flags, depth, stencil, dstx, dsty, width, height, render_condition_enabled):
            def clear_texture(self, res, level, box, **color):
            def create_surface(self, resource, surf_tmpl):
      assert resource is not None
   surf_tmpl.resource = resource
         def surface_destroy(self, surface):
            def create_query(self, query_type, index):
            def destroy_query(self, query):
            def begin_query(self, query):
            def end_query(self, query):
            def create_stream_output_target(self, res, buffer_offset, buffer_size):
      so_target = Struct()
   so_target.resource = res
   so_target.offset = buffer_offset
   so_target.size = buffer_size
         class Interpreter(parser.SimpleTraceDumper):
      '''Specialization of a trace parser that interprets the calls as it goes
            ignoredCalls = set((
            ('pipe_screen', 'is_format_supported'),
   ('pipe_screen', 'get_name'),
   ('pipe_screen', 'get_vendor'),
   ('pipe_screen', 'get_device_uuid'),
   ('pipe_screen', 'get_driver_uuid'),
   ('pipe_screen', 'get_compiler_options'),
   ('pipe_screen', 'get_param'),
   ('pipe_screen', 'get_paramf'),
   ('pipe_screen', 'get_shader_param'),
   ('pipe_screen', 'get_compute_param'),
   ('pipe_screen', 'get_disk_shader_cache'),
   ('pipe_context', 'clear_render_target'), # XXX workaround trace bugs
   ('pipe_context', 'flush_resource'),
   ('pipe_context', 'buffer_map'),
               def __init__(self, stream, options, formatter, state):
      parser.SimpleTraceDumper.__init__(self, stream, options, formatter, state)
   self.objects = {}
   self.result = None
   self.globl = Global(self)
         def register_object(self, address, object):
            def unregister_object(self, object):
      # TODO
         def lookup_object(self, address):
      try:
         except KeyError:
               def interpret(self, trace):
      for call in trace.calls:
         def handle_call(self, call):
      if (call.klass, call.method) in self.ignoredCalls:
                     if self.verbosity(1):
         # Write the call to stderr (as stdout would corrupt the JSON output)
   sys.stderr.flush()
   sys.stdout.flush()
   parser.TraceDumper.handle_call(self, call)
                     if call.klass:
         name, obj = args[0]
   else:
            method = getattr(obj, call.method)
            # Keep track of created pointer objects.
   if call.ret and isinstance(call.ret, model.Pointer):
         if ret is None:
                  def interpret_arg(self, node):
      translator = Translator(self)
         def verbosity(self, level):
            class DumpStateOptions(parser.ParseOptions):
                     # These will get initialized in ModelOptions.__init__()
   self.verbosity = None
   self.call = None
                  class Main(parser.Main):
         def get_optparser(self):
      optparser = argparse.ArgumentParser(
            optparser.add_argument("filename", action="extend", nargs="+",
            optparser.add_argument("-v", "--verbose", action="count", default=0, dest="verbosity", help="increase verbosity level")
   optparser.add_argument("-q", "--quiet", action="store_const", const=0, dest="verbosity", help="no messages")
   optparser.add_argument("-c", "--call", action="store", type=int, dest="call", default=0xffffffff, help="dump on this call")
   optparser.add_argument("-d", "--draw", action="store", type=int, dest="draw", default=0xffffffff, help="dump on this draw")
         def make_options(self, args):
            def process_arg(self, stream, options):
      formatter = format.Formatter(sys.stderr)
   parser = Interpreter(stream, options, formatter, model.TraceStateData())
         if __name__ == '__main__':
      Main().main()
