   #! /usr/bin/env python3
   # Taken from Crucible and modified to parse declarations
      import argparse
   import io
   import os
   import re
   import shutil
   import struct
   import subprocess
   import sys
   import tempfile
   from textwrap import dedent
      class ShaderCompileError(RuntimeError):
      def __init__(self, *args):
         target_env_re = re.compile(r'QO_TARGET_ENV\s+(\S+)')
      stage_to_glslang_stage = {
      'VERTEX': 'vert',
   'TESS_CONTROL': 'tesc',
   'TESS_EVALUATION': 'tese',
   'GEOMETRY': 'geom',
   'FRAGMENT': 'frag',
      }
      base_layout_qualifier_id_re = r'({0}\s*=\s*(?P<{0}>\d+))'
   id_re = r'(?P<name_%d>[^(gl_)]\w+)'
   type_re = r'(?P<dtype_%d>\w+)'
   location_re = base_layout_qualifier_id_re.format('location')
   component_re = base_layout_qualifier_id_re.format('component')
   binding_re = base_layout_qualifier_id_re.format('binding')
   set_re = base_layout_qualifier_id_re.format('set')
   unk_re = r'\w+(=\d+)?'
   layout_qualifier_re = r'layout\W*\((%s)+\)' % '|'.join([location_re, binding_re, set_re, unk_re, '[, ]+'])
   ubo_decl_re = r'uniform\W+%s(\W*{)?(?P<type_ubo>)' % (id_re%0)
   ssbo_decl_re = r'buffer\W+%s(\W*{)?(?P<type_ssbo>)' % (id_re%1)
   image_buffer_decl_re = r'uniform\W+imageBuffer\w+%s;(?P<type_img_buf>)' % (id_re%2)
   image_decl_re = r'uniform\W+image\w+\W+%s;(?P<type_img>)' % (id_re%3)
   texture_buffer_decl_re = r'uniform\W+textureBuffer\w+%s;(?P<type_tex_buf>)' % (id_re%4)
   combined_texture_sampler_decl_re = r'uniform\W+sampler\w+\W+%s;(?P<type_combined>)' % (id_re%5)
   texture_decl_re = r'uniform\W+texture\w+\W+%s;(?P<type_tex>)' % (id_re%6)
   sampler_decl_re = r'uniform\W+sampler\w+%s;(?P<type_samp>)' % (id_re%7)
   input_re = r'in\W+%s\W+%s;(?P<type_in>)' % (type_re%0, id_re%8)
   output_re = r'out\W+%s\W+%s;(?P<type_out>)' % (type_re%1, id_re%9)
   match_decl_re = re.compile(layout_qualifier_re + r'\W*((' + r')|('.join([ubo_decl_re, ssbo_decl_re, image_buffer_decl_re, image_decl_re, texture_buffer_decl_re, combined_texture_sampler_decl_re, texture_decl_re, sampler_decl_re, input_re, output_re]) + r'))$')
      class Shader:
      def __init__(self, stage):
      self.glsl = None
   self.stream = io.StringIO()
   self.stage = stage
   self.dwords = None
   self.target_env = ""
         def add_text(self, s):
            def finish_text(self, start_line, end_line):
      self.glsl = self.stream.getvalue()
            # Handle the QO_EXTENSION macro
            # Handle the QO_DEFINE macro
            m = target_env_re.search(self.glsl)
   if m:
                  self.start_line = start_line
         def __run_glslang(self, extra_args=[]):
      stage = stage_to_glslang_stage[self.stage]
            in_file = tempfile.NamedTemporaryFile(suffix='.'+stage)
   src = ('#version 450\n' + self.glsl).encode('utf-8')
   in_file.write(src)
   in_file.flush()
   out_file = tempfile.NamedTemporaryFile(suffix='.spirv')
   args = [glslang, '-H'] + extra_args + stage_flags
   if self.target_env:
         args += ['-o', out_file.name, in_file.name]
   with subprocess.Popen(args,
                                             if proc.returncode != 0:
      # Unfortunately, glslang dumps errors to standard out.
   # However, since we don't really want to count on that,
                     out_file.seek(0)
   spirv = out_file.read()
         def _parse_declarations(self):
      for line in self.glsl.splitlines():
         res = re.match(match_decl_re, line.lstrip().rstrip())
   if res == None:
         res = {k:v for k, v in res.groupdict().items() if v != None}
   name = [v for k, v in res.items() if k.startswith('name_')][0]
   data_type = ([v for k, v in res.items() if k.startswith('dtype_')] + [''])[0]
   decl_type = [k for k, v in res.items() if k.startswith('type_')][0][5:]
   location = int(res.get('location', 0))
   component = int(res.get('component', 0))
   binding = int(res.get('binding', 0))
   desc_set = int(res.get('set', 0))
         def compile(self):
      def dwords(f):
         while True:
      dword_str = f.read(4)
   if not dword_str:
               (spirv, assembly) = self.__run_glslang()
   self.dwords = list(dwords(io.BytesIO(spirv)))
                  def _dump_glsl_code(self, f):
      # Dump GLSL code for reference.  Use // instead of /* */
   # comments so we don't need to escape the GLSL code.
   f.write('// GLSL code:\n')
   f.write('//')
   for line in self.glsl.splitlines():
               def _dump_spirv_code(self, f, var_name):
      f.write('/* SPIR-V Assembly:\n')
   f.write(' *\n')
   for line in self.assembly.splitlines():
                  f.write('static const uint32_t {0}[] = {{'.format(var_name))
   line_start = 0
   while line_start < len(self.dwords):
         f.write('\n    ')
   for i in range(line_start, min(line_start + 6, len(self.dwords))):
               def dump_c_code(self, f):
      f.write('\n\n')
            self._dump_glsl_code(f)
   self._dump_spirv_code(f, var_prefix + '_spir_v_src')
            f.write(dedent("""\
         static const QoShaderModuleCreateInfo {0}_info = {{
      .spirvSize = sizeof({0}_spir_v_src),
   .pSpirv = {0}_spir_v_src,
                                 f.write('#define __qonos_shader{0}_info __qonos_shader{1}_info\n'\
      token_exp = re.compile(r'(qoShaderModuleCreateInfoGLSL|qoCreateShaderModuleGLSL|\(|\)|,)')
      class Parser:
      def __init__(self, f):
      self.infile = f
   self.paren_depth = 0
   self.shader = None
   self.line_number = 1
            def tokenize(f):
         leftover = ''
   for line in f:
      pos = 0
   while True:
      m = token_exp.search(line, pos)
   if m:
                                                                                             def handle_shader_src(self):
      paren_depth = 1
   for t in self.token_iter:
         if t == '(':
         elif t == ')':
                        def handle_macro(self, macro):
      t = next(self.token_iter)
                     if macro == 'qoCreateShaderModuleGLSL':
         # Throw away the device parameter
   t = next(self.token_iter)
                     t = next(self.token_iter)
            self.current_shader = Shader(stage)
   self.handle_shader_src()
            self.shaders.append(self.current_shader)
         def run(self):
      for t in self.token_iter:
            def open_file(name, mode):
      if name == '-':
      if mode == 'w':
         elif mode == 'r':
         else:
      else:
         def parse_args():
      description = dedent("""\
      This program scrapes a C file for any instance of the
   qoShaderModuleCreateInfoGLSL and qoCreateShaderModuleGLSL macaros,
   grabs the GLSL source code, compiles it to SPIR-V.  The resulting
   SPIR-V code is written to another C file as an array of 32-bit
            If '-' is passed as the input file or output file, stdin or stdout
         p = argparse.ArgumentParser(
               p.add_argument('-o', '--outfile', default='-',
         p.add_argument('--with-glslang', metavar='PATH',
                                       args = parse_args()
   infname = args.infile
   outfname = args.outfile
   glslang = args.glslang
      with open_file(infname, 'r') as infile:
      parser = Parser(infile)
         for shader in parser.shaders:
            with open_file(outfname, 'w') as outfile:
      outfile.write(dedent("""\
      /* ==========================  DO NOT EDIT!  ==========================
                           #define __QO_SHADER_INFO_VAR2(_line) __qonos_shader ## _line ## _info
            #define qoShaderModuleCreateInfoGLSL(stage, ...)  \\
            #define qoCreateShaderModuleGLSL(dev, stage, ...) \\
               for shader in parser.shaders:
      shader.dump_c_code(outfile)
