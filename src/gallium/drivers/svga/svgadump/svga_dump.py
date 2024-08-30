   '''
   Generates dumper for the SVGA 3D command stream using pygccxml.
      Jose Fonseca <jfonseca@vmware.com>
   '''
      copyright = '''
   /**********************************************************
   * Copyright 2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
   '''
      import os
   import sys
      from pygccxml import parser
   from pygccxml import declarations
      from pygccxml.declarations import algorithm
   from pygccxml.declarations import decl_visitor
   from pygccxml.declarations import type_traits
   from pygccxml.declarations import type_visitor
         enums = True
         class decl_dumper_t(decl_visitor.decl_visitor_t):
         def __init__(self, instance = '', decl = None):
      decl_visitor.decl_visitor_t.__init__(self)
   self._instance = instance
         def clone(self):
            def visit_class(self):
      class_ = self.decl
            for variable in class_.variables():
         if variable.name != '':
         def visit_enumeration(self):
      if enums:
         print '   switch(%s) {' % ("(*cmd)" + self._instance,)
   for name, value in self.decl.values:
      print '   case %s:' % (name,)
   print '      _debug_printf("\\t\\t%s = %s\\n");' % (self._instance, name)
      print '   default:'
   print '      _debug_printf("\\t\\t%s = %%i\\n", %s);' % (self._instance, "(*cmd)" + self._instance)
   print '      break;'
   else:
         def dump_decl(instance, decl):
      dumper = decl_dumper_t(instance, decl)
            class type_dumper_t(type_visitor.type_visitor_t):
         def __init__(self, instance, type_):
      type_visitor.type_visitor_t.__init__(self)
   self.instance = instance
         def clone(self):
            def visit_char(self):
      self.print_instance('%i')
      def visit_unsigned_char(self):
            def visit_signed_char(self):
            def visit_wchar(self):
      self.print_instance('%i')
      def visit_short_int(self):
      self.print_instance('%i')
      def visit_short_unsigned_int(self):
      self.print_instance('%u')
      def visit_bool(self):
      self.print_instance('%i')
      def visit_int(self):
      self.print_instance('%i')
      def visit_unsigned_int(self):
      self.print_instance('%u')
      def visit_long_int(self):
      self.print_instance('%li')
      def visit_long_unsigned_int(self):
      self.print_instance('%lu')
      def visit_long_long_int(self):
      self.print_instance('%lli')
      def visit_long_long_unsigned_int(self):
      self.print_instance('%llu')
      def visit_float(self):
      self.print_instance('%f')
      def visit_double(self):
      self.print_instance('%f')
      def visit_array(self):
      for i in range(type_traits.array_size(self.type)):
         def visit_pointer(self):
            def visit_declarated(self):
      #print 'decl = %r' % self.type.decl_string
   decl = type_traits.remove_declarated(self.type)
         def print_instance(self, format):
            def dump_type(instance, type_):
      type_ = type_traits.remove_alias(type_)
   visitor = type_dumper_t(instance, type_)
            def dump_struct(decls, class_):
      print 'static void'
   print 'dump_%s(const %s *cmd)' % (class_.name, class_.name)
   print '{'
   dump_decl('', class_)
   print '}'
            cmds = [
      ('SVGA_3D_CMD_SURFACE_DEFINE', 'SVGA3dCmdDefineSurface', (), 'SVGA3dSize'),
   ('SVGA_3D_CMD_SURFACE_DESTROY', 'SVGA3dCmdDestroySurface', (), None),
   ('SVGA_3D_CMD_SURFACE_COPY', 'SVGA3dCmdSurfaceCopy', (), 'SVGA3dCopyBox'),
   ('SVGA_3D_CMD_SURFACE_STRETCHBLT', 'SVGA3dCmdSurfaceStretchBlt', (), None),
   ('SVGA_3D_CMD_SURFACE_DMA', 'SVGA3dCmdSurfaceDMA', (), 'SVGA3dCopyBox'),
   ('SVGA_3D_CMD_CONTEXT_DEFINE', 'SVGA3dCmdDefineContext', (), None),
   ('SVGA_3D_CMD_CONTEXT_DESTROY', 'SVGA3dCmdDestroyContext', (), None),
   ('SVGA_3D_CMD_SETTRANSFORM', 'SVGA3dCmdSetTransform', (), None),
   ('SVGA_3D_CMD_SETZRANGE', 'SVGA3dCmdSetZRange', (), None),
   ('SVGA_3D_CMD_SETRENDERSTATE', 'SVGA3dCmdSetRenderState', (), 'SVGA3dRenderState'),
   ('SVGA_3D_CMD_SETRENDERTARGET', 'SVGA3dCmdSetRenderTarget', (), None),
   ('SVGA_3D_CMD_SETTEXTURESTATE', 'SVGA3dCmdSetTextureState', (), 'SVGA3dTextureState'),
   ('SVGA_3D_CMD_SETMATERIAL', 'SVGA3dCmdSetMaterial', (), None),
   ('SVGA_3D_CMD_SETLIGHTDATA', 'SVGA3dCmdSetLightData', (), None),
   ('SVGA_3D_CMD_SETLIGHTENABLED', 'SVGA3dCmdSetLightEnabled', (), None),
   ('SVGA_3D_CMD_SETVIEWPORT', 'SVGA3dCmdSetViewport', (), None),
   ('SVGA_3D_CMD_SETCLIPPLANE', 'SVGA3dCmdSetClipPlane', (), None),
   ('SVGA_3D_CMD_CLEAR', 'SVGA3dCmdClear', (), 'SVGA3dRect'),
   ('SVGA_3D_CMD_PRESENT', 'SVGA3dCmdPresent', (), 'SVGA3dCopyRect'),
   ('SVGA_3D_CMD_SHADER_DEFINE', 'SVGA3dCmdDefineShader', (), None),
   ('SVGA_3D_CMD_SHADER_DESTROY', 'SVGA3dCmdDestroyShader', (), None),
   ('SVGA_3D_CMD_SET_SHADER', 'SVGA3dCmdSetShader', (), None),
   ('SVGA_3D_CMD_SET_SHADER_CONST', 'SVGA3dCmdSetShaderConst', (), None),
   ('SVGA_3D_CMD_DRAW_PRIMITIVES', 'SVGA3dCmdDrawPrimitives', (('SVGA3dVertexDecl', 'numVertexDecls'), ('SVGA3dPrimitiveRange', 'numRanges')), 'SVGA3dVertexDivisor'),
   ('SVGA_3D_CMD_SETSCISSORRECT', 'SVGA3dCmdSetScissorRect', (), None),
   ('SVGA_3D_CMD_BEGIN_QUERY', 'SVGA3dCmdBeginQuery', (), None),
   ('SVGA_3D_CMD_END_QUERY', 'SVGA3dCmdEndQuery', (), None),
   ('SVGA_3D_CMD_WAIT_FOR_QUERY', 'SVGA3dCmdWaitForQuery', (), None),
   #('SVGA_3D_CMD_PRESENT_READBACK', None, (), None),
      ]
      def dump_cmds():
         void            
   svga_dump_command(uint32_t cmd_id, const void *data, uint32_t size)
   {
      const uint8_t *body = (const uint8_t *)data;
      '''
      print '   switch(cmd_id) {'
   indexes = 'ijklmn'
   for id, header, body, footer in cmds:
      print '   case %s:' % id
   print '      _debug_printf("\\t%s\\n");' % id
   print '      {'
   print '         const %s *cmd = (const %s *)body;' % (header, header)
   if len(body):
         print '         dump_%s(cmd);' % header
   print '         body = (const uint8_t *)&cmd[1];'
   for i in range(len(body)):
         struct, count = body[i]
   idx = indexes[i]
   print '         for(%s = 0; %s < cmd->%s; ++%s) {' % (idx, idx, count, idx)
   print '            dump_%s((const %s *)body);' % (struct, struct)
   print '            body += sizeof(%s);' % struct
   if footer is not None:
         print '         while(body + sizeof(%s) <= next) {' % footer
   print '            dump_%s((const %s *)body);' % (footer, footer)
   print '            body += sizeof(%s);' % footer
   if id == 'SVGA_3D_CMD_SHADER_DEFINE':
         print '         svga_shader_dump((const uint32_t *)body,'
   print '                          (unsigned)(next - body)/sizeof(uint32_t),'
   print '                          FALSE);'
   print '      }'
      print '   default:'
   print '      _debug_printf("\\t0x%08x\\n", cmd_id);'
   print '      break;'
   print '   }'
   print r'''
   while(body + sizeof(uint32_t) <= next) {
      _debug_printf("\t\t0x%08x\n", *(const uint32_t *)body);
      }
   while(body + sizeof(uint32_t) <= next)
      }
   '''
         void            
   svga_dump_commands(const void *commands, uint32_t size)
   {
      const uint8_t *next = commands;
   const uint8_t *last = next + size;
      assert(size % sizeof(uint32_t) == 0);
      while(next < last) {
               if(SVGA_3D_CMD_BASE <= cmd_id && cmd_id < SVGA_3D_CMD_MAX) {
                     next = body + header->size;
                     }
   else if(cmd_id == SVGA_CMD_FENCE) {
      _debug_printf("\tSVGA_CMD_FENCE\n");
   _debug_printf("\t\t0x%08x\n", ((const uint32_t *)next)[1]);
      }
   else {
      _debug_printf("\t0x%08x\n", cmd_id);
            }
   '''
      def main():
      print copyright.strip()
   print
   print '/**'
   print ' * @file'
   print ' * Dump SVGA commands.'
   print ' *'
   print ' * Generated automatically from svga3d_reg.h by svga_dump.py.'
   print ' */'
   print
   print '#include "svga_types.h"'
   print '#include "svga_shader_dump.h"'
   print '#include "svga3d_reg.h"'
   print
   print '#include "util/u_debug.h"'
   print '#include "svga_dump.h"'
            config = parser.config_t(
      include_paths = ['../../../include', '../include'],
               headers = [
      'svga_types.h', 
               decls = parser.parse(headers, config, parser.COMPILATION_MODE.ALL_AT_ONCE)
            names = set()
   for id, header, body, footer in cmds:
      names.add(header)
   for struct, count in body:
         if footer is not None:
         for class_ in global_ns.classes(lambda decl: decl.name in names):
                     if __name__ == '__main__':
      main()
