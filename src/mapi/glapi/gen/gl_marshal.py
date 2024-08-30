      # Copyright (C) 2012 Intel Corporation
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
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
      import contextlib
   import gl_XML
   import license
   import marshal_XML
   import sys
   import collections
   import apiexec
      header = """
   #include "context.h"
   #include "glthread_marshal.h"
   #include "bufferobj.h"
   #include "dispatch.h"
      #define COMPAT (ctx->API != API_OPENGL_CORE)
      UNUSED static inline int safe_mul(int a, int b)
   {
      if (a < 0 || b < 0) return -1;
   if (a == 0 || b == 0) return 0;
   if (a > INT_MAX / b) return -1;
      }
   """
         file_index = 0
   file_count = 1
   current_indent = 0
         def out(str):
      if str:
         else:
            @contextlib.contextmanager
   def indent(delta = 3):
      global current_indent
   current_indent += delta
   yield
            class PrintCode(gl_XML.gl_print_base):
      def __init__(self):
               self.name = 'gl_marshal.py'
   self.license = license.bsd_license_template % (
         def printRealHeader(self):
            def printRealFooter(self):
            def print_call(self, func, unmarshal=0):
      ret = 'return ' if func.return_type != 'void' and not unmarshal else '';
   call = 'CALL_{0}(ctx->Dispatch.Current, ({1}))'.format(
         out('{0}{1};'.format(ret, call))
   if func.marshal_call_after and ret == '' and not unmarshal:
         def print_sync_body(self, func):
      out('/* {0}: marshalled synchronously */'.format(func.name))
   out('{0}{1} GLAPIENTRY'.format('static ' if func.marshal_is_static() else '', func.return_type))
   out('_mesa_marshal_{0}({1})'.format(func.name, func.get_parameter_string()))
   out('{')
   with indent():
         out('GET_CURRENT_CONTEXT(ctx);')
   if func.marshal_call_before:
         out('_mesa_glthread_finish_before(ctx, "{0}");'.format(func.name))
   out('}')
   out('')
         def get_type_size(self, str):
      if str.find('*') != -1:
            mapping = {
         'GLboolean': 1,
   'GLbyte': 1,
   'GLubyte': 1,
   'GLenum': 2, # uses GLenum16, clamped to 0xffff (invalid enum)
   'GLshort': 2,
   'GLushort': 2,
   'GLhalfNV': 2,
   'GLint': 4,
   'GLuint': 4,
   'GLbitfield': 4,
   'GLsizei': 4,
   'GLfloat': 4,
   'GLclampf': 4,
   'GLfixed': 4,
   'GLclampx': 4,
   'GLhandleARB': 4,
   'int': 4,
   'float': 4,
   'GLdouble': 8,
   'GLclampd': 8,
   'GLintptr': 8,
   'GLsizeiptr': 8,
   'GLint64': 8,
   'GLuint64': 8,
   'GLuint64EXT': 8,
   }
   val = mapping.get(str, 9999)
   if val == 9999:
         print('Unhandled type in gl_marshal.py.get_type_size: ' + str, file=sys.stderr)
         def print_async_body(self, func):
      # We want glthread to ignore variable-sized parameters if the only thing
   # we want is to pass the pointer parameter as-is, e.g. when a PBO is bound.
   # Making it conditional on marshal_sync is kinda hacky, but it's the easiest
   # path towards handling PBOs in glthread, which use marshal_sync to check whether
   # a PBO is bound.
   if func.marshal_sync:
         fixed_params = func.fixed_params + func.variable_params
   else:
                  out('/* {0}: marshalled asynchronously */'.format(func.name))
   out('struct marshal_cmd_{0}'.format(func.name))
   out('{')
   with indent():
                  # Sort the parameters according to their size to pack the structure optimally
   for p in sorted(fixed_params, key=lambda p: self.get_type_size(p.type_string())):
      if p.count:
      out('{0} {1}[{2}];'.format(
      else:
      type = p.type_string()
                  for p in variable_params:
                        for p in variable_params:
      if p.count_scale != 1:
      out(('/* Next {0} bytes are '
         '{1} {2}[{3}][{4}] */').format(
      else:
      out(('/* Next {0} bytes are '
                  out('uint32_t')
   out(('_mesa_unmarshal_{0}(struct gl_context *ctx, '
         out('{')
   with indent():
         for p in fixed_params:
      if p.count:
      p_decl = '{0} *{1} = cmd->{1};'.format(
                           if not p_decl.startswith('const ') and p.count:
                              if variable_params:
      for p in variable_params:
      out('{0} *{1};'.format(
      out('const char *variable_data = (const char *) (cmd + 1);')
   i = 1
                           if p.img_null_flag:
         out('if (cmd->{0}_null)'.format(p.name))
   with indent():
         if i < len(variable_params):
      out('else')
                     self.print_call(func, unmarshal=1)
   if variable_params:
         else:
      struct = 'struct marshal_cmd_{0}'.format(func.name)
   out('const unsigned cmd_size = (align(sizeof({0}), 8) / 8);'.format(struct))
               out('{0}{1} GLAPIENTRY'.format('static ' if func.marshal_is_static() else '', func.return_type))
   out('_mesa_marshal_{0}({1})'.format(
         out('{')
   with indent():
         out('GET_CURRENT_CONTEXT(ctx);')
                  if not func.marshal_sync:
                  struct = 'struct marshal_cmd_{0}'.format(func.name)
   size_terms = ['sizeof({0})'.format(struct)]
   if not func.marshal_sync:
      for p in func.variable_params:
      if p.img_null_flag:
         else:
                  if func.marshal_sync:
      out('if ({0}) {{'.format(func.marshal_sync))
   with indent():
      out('_mesa_glthread_finish_before(ctx, "{0}");'.format(func.name))
   self.print_call(func)
         else:
      # Fall back to syncing if variable-length sizes can't be handled.
   #
   # Check that any counts for variable-length arguments might be < 0, in
   # which case the command alloc or the memcpy would blow up before we
   # get to the validation in Mesa core.
   list = []
   for p in func.parameters:
                                          out('if (unlikely({0})) {{'.format(' || '.join(list)))
   with indent():
                        # Add the call into the batch.
                  for p in fixed_params:
      if p.count:
      out('memcpy(cmd->{0}, {0}, {1});'.format(
      elif p.type_string() == 'GLenum':
         else:
      if variable_params:
      out('char *variable_data = (char *) (cmd + 1);')
   i = 1
   for p in variable_params:
      if p.img_null_flag:
         out('cmd->{0}_null = !{0};'.format(p.name))
   out('if (!cmd->{0}_null) {{'.format(p.name))
   with indent():
      out(('memcpy(variable_data, {0}, {0}_size);').format(p.name))
   if i < len(variable_params):
      else:
                                                                     if func.return_type == 'GLboolean':
   out('}')
   out('')
         def print_init_marshal_table(self, functions):
      out('void')
   out('_mesa_glthread_init_dispatch%u(struct gl_context *ctx, '
         out('{')
   with indent():
         # Collect SET_* calls by the condition under which they should
                  for func in functions:
                                       # Print out an if statement for each unique condition, with
   # the SET_* calls nested inside it.
   for condition in sorted(settings_by_condition.keys()):
      out('if ({0}) {{'.format(condition))
   with indent():
      for setting in sorted(settings_by_condition[condition]):
               def printBody(self, api):
      # Don't generate marshal/unmarshal functions for skipped and custom functions
   functions = [func for func in api.functionIterateAll()
         # Divide the functions between files
   func_per_file = len(functions) // file_count + 1
            for func in functions:
         flavor = func.marshal_flavor()
   if flavor == 'async':
         elif flavor == 'sync':
                  # The first file will also set custom functions
   if file_index == 0:
                        def show_usage():
      print('Usage: %s [file_name] [file_index] [total file count]' % sys.argv[0])
            if __name__ == '__main__':
      try:
      file_name = sys.argv[1]
   file_index = int(sys.argv[2])
      except Exception:
                     api = gl_XML.parse_GL_API(file_name, marshal_XML.marshal_item_factory())
   printer.Print(api)
