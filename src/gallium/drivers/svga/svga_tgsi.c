   /**********************************************************
   * Copyright 2008-2022 VMware, Inc.  All rights reserved.
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
         #include "util/compiler.h"
   #include "pipe/p_shader_tokens.h"
   #include "pipe/p_defines.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_scan.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_bitmask.h"
      #include "svgadump/svga_shader_dump.h"
      #include "svga_context.h"
   #include "svga_shader.h"
   #include "svga_tgsi.h"
   #include "svga_tgsi_emit.h"
   #include "svga_debug.h"
      #include "svga_hw_reg.h"
   #include "svga3d_shaderdefs.h"
         /* Sinkhole used only in error conditions.
   */
   static char err_buf[128];
         static bool
   svga_shader_expand(struct svga_shader_emitter *emit)
   {
      char *new_buf;
            if (emit->buf != err_buf)
         else
            if (!new_buf) {
      emit->ptr = err_buf;
   emit->buf = err_buf;
   emit->size = sizeof(err_buf);
               emit->size = newsize;
   emit->ptr = new_buf + (emit->ptr - emit->buf);
   emit->buf = new_buf;
      }
         static inline bool
   reserve(struct svga_shader_emitter *emit, unsigned nr_dwords)
   {
      if (emit->ptr - emit->buf + nr_dwords * sizeof(unsigned) >= emit->size) {
      if (!svga_shader_expand(emit)) {
                        }
         bool
   svga_shader_emit_dword(struct svga_shader_emitter * emit, unsigned dword)
   {
      if (!reserve(emit, 1))
            *(unsigned *) emit->ptr = dword;
   emit->ptr += sizeof dword;
      }
         bool
   svga_shader_emit_dwords(struct svga_shader_emitter * emit,
         {
      if (!reserve(emit, nr))
            memcpy(emit->ptr, dwords, nr * sizeof *dwords);
   emit->ptr += nr * sizeof *dwords;
      }
         bool
   svga_shader_emit_opcode(struct svga_shader_emitter * emit, unsigned opcode)
   {
               if (!reserve(emit, 1))
            here = (SVGA3dShaderInstToken *) emit->ptr;
            if (emit->insn_offset) {
      SVGA3dShaderInstToken *prev =
                     emit->insn_offset = emit->ptr - emit->buf;
   emit->ptr += sizeof(unsigned);
      }
         static bool
   svga_shader_emit_header(struct svga_shader_emitter *emit)
   {
                        switch (emit->unit) {
   case PIPE_SHADER_FRAGMENT:
      header.value = SVGA3D_PS_30;
      case PIPE_SHADER_VERTEX:
      header.value = SVGA3D_VS_30;
                  }
         /**
   * Parse TGSI shader and translate to SVGA/DX9 serialized
   * representation.
   *
   * In this function SVGA shader is emitted to an in-memory buffer that
   * can be dynamically grown.  Once we've finished and know how large
   * it is, it will be copied to a hardware buffer for upload.
   */
   struct svga_shader_variant *
   svga_tgsi_vgpu9_translate(struct svga_context *svga,
                     {
      struct svga_shader_variant *variant = NULL;
                              emit.size = 1024;
   emit.buf = MALLOC(emit.size);
   if (emit.buf == NULL) {
                  emit.ptr = emit.buf;
   emit.unit = unit;
                              if (unit == PIPE_SHADER_FRAGMENT)
            if (unit == PIPE_SHADER_VERTEX) {
                  emit.nr_hw_float_const =
                     if (emit.nr_hw_temp >= SVGA3D_TEMPREG_MAX) {
      debug_printf("svga: too many temporary registers (%u)\n",
                     if (emit.info.indirect_files & (1 << TGSI_FILE_TEMPORARY)) {
      debug_printf(
                              if (!svga_shader_emit_header(&emit)) {
      debug_printf("svga: emit header failed\n");
               if (!svga_shader_emit_instructions(&emit, shader->tokens)) {
      debug_printf("svga: emit instructions failed\n");
               variant = svga_new_shader_variant(svga, unit);
   if (!variant)
            variant->shader = shader;
   variant->tokens = (const unsigned *) emit.buf;
   variant->nr_tokens = (emit.ptr - emit.buf) / sizeof(unsigned);
   memcpy(&variant->key, key, sizeof(*key));
            if (unit == PIPE_SHADER_FRAGMENT) {
                        /* If there was exactly one write to a fragment shader output register
   * and it came from a constant buffer, we know all fragments will have
   * the same color (except for blending).
   */
   fs_variant->constant_color_output =
            #if 0
      if (!svga_shader_verify(variant->tokens, variant->nr_tokens) ||
      SVGA_DEBUG & DEBUG_TGSI) {
   debug_printf("#####################################\n");
   debug_printf("Shader %u below\n", shader->id);
   tgsi_dump(shader->tokens, 0);
   if (SVGA_DEBUG & DEBUG_TGSI) {
      debug_printf("Shader %u compiled below\n", shader->id);
      }
         #endif
               fail:
      FREE(variant);
   if (emit.buf != err_buf)
               done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         /**
   * Helper function to convert tgsi semantic name to vertex attribute
   * semantic name.
   */
   static gl_vert_attrib
   svga_tgsi_to_gl_vert_attrib_semantic(unsigned sem_name,
         {
      switch (sem_name) {
   case TGSI_SEMANTIC_POSITION:
         case TGSI_SEMANTIC_COLOR:
      assert(sem_index <= 1);
      case TGSI_SEMANTIC_FOG:
         case TGSI_SEMANTIC_PSIZE:
         case TGSI_SEMANTIC_GENERIC:
         case TGSI_SEMANTIC_EDGEFLAG:
         case TGSI_SEMANTIC_TEXCOORD:
      assert(sem_index <= 7);
      default:
      assert(0);
         }
         /**
   * Helper function to convert tgsi semantic name to varying semantic name.
   */
   static gl_varying_slot
   svga_tgsi_to_gl_varying_semantic(unsigned sem_name,
         {
      switch (sem_name) {
   case TGSI_SEMANTIC_POSITION:
         case TGSI_SEMANTIC_COLOR:
      assert(sem_index <= 1);
      case TGSI_SEMANTIC_BCOLOR:
      assert(sem_index <= 1);
      case TGSI_SEMANTIC_FOG:
         case TGSI_SEMANTIC_PSIZE:
         case TGSI_SEMANTIC_GENERIC:
         case TGSI_SEMANTIC_FACE:
         case TGSI_SEMANTIC_EDGEFLAG:
         case TGSI_SEMANTIC_CLIPDIST:
      assert(sem_index <= 1);
      case TGSI_SEMANTIC_CLIPVERTEX:
         case TGSI_SEMANTIC_TEXCOORD:
      assert(sem_index <= 7);
      case TGSI_SEMANTIC_PCOORD:
         case TGSI_SEMANTIC_VIEWPORT_INDEX:
         case TGSI_SEMANTIC_LAYER:
         case TGSI_SEMANTIC_PATCH:
         case TGSI_SEMANTIC_TESSOUTER:
         case TGSI_SEMANTIC_TESSINNER:
         case TGSI_SEMANTIC_VIEWPORT_MASK:
         case TGSI_SEMANTIC_PRIMID:
         default:
      assert(0);
         }
         /**
   * Helper function to convert tgsi semantic name to fragment result
   * semantic name.
   */
   static gl_frag_result
   svga_tgsi_to_gl_frag_result_semantic(unsigned sem_name,
         {
      switch (sem_name) {
   case TGSI_SEMANTIC_POSITION:
         case TGSI_SEMANTIC_COLOR:
      assert(sem_index <= 7);
      case TGSI_SEMANTIC_STENCIL:
         case TGSI_SEMANTIC_SAMPLEMASK:
         default:
      assert(0);
         }
         /**
   * svga_tgsi_scan_shader is called to collect information of the
   * specified tgsi shader.
   */
   void
   svga_tgsi_scan_shader(struct svga_shader *shader)
   {
      struct tgsi_shader_info *tgsi_info = &shader->tgsi_info;
                     /* Save some common shader info in IR neutral format */
   info->num_inputs = tgsi_info->num_inputs;
   info->num_outputs = tgsi_info->num_outputs;
   info->writes_edgeflag = tgsi_info->writes_edgeflag;
   info->writes_layer = tgsi_info->writes_layer;
   info->writes_position = tgsi_info->writes_position;
   info->writes_psize = tgsi_info->writes_psize;
            info->uses_grid_size = tgsi_info->uses_grid_size;
   info->uses_const_buffers = tgsi_info->const_buffers_declared != 0;
   info->uses_hw_atomic = tgsi_info->hw_atomic_declared != 0;
   info->uses_images = tgsi_info->images_declared != 0;
   info->uses_image_size = tgsi_info->opcode_count[TGSI_OPCODE_RESQ] ? 1 : 0;
   info->uses_shader_buffers = tgsi_info->shader_buffers_declared != 0;
   info->uses_samplers = tgsi_info->samplers_declared != 0;
   info->const_buffers_declared = tgsi_info->const_buffers_declared;
            info->generic_inputs_mask = svga_get_generic_inputs_mask(tgsi_info);
            /* Convert TGSI inputs semantic.
   * Vertex shader does not have varying inputs but vertex attributes.
   */
   if (shader->stage == PIPE_SHADER_VERTEX) {
      for (unsigned i = 0; i < info->num_inputs; i++) {
      info->input_semantic_name[i] =
      svga_tgsi_to_gl_vert_attrib_semantic(
      tgsi_info->input_semantic_name[i],
         }
   else {
      for (unsigned i = 0; i < info->num_inputs; i++) {
      info->input_semantic_name[i] =
      svga_tgsi_to_gl_varying_semantic(
      tgsi_info->input_semantic_name[i],
                  /* Convert TGSI outputs semantic.
   * Fragment shader does not have varying outputs but fragment results.
   */
   if (shader->stage == PIPE_SHADER_FRAGMENT) {
      for (unsigned i = 0; i < info->num_outputs; i++) {
      info->output_semantic_name[i] =
      svga_tgsi_to_gl_frag_result_semantic(
      tgsi_info->output_semantic_name[i],
         }
   else {
      for (unsigned i = 0; i < info->num_outputs; i++) {
      info->output_semantic_name[i] =
      svga_tgsi_to_gl_varying_semantic(
      tgsi_info->output_semantic_name[i],
                           switch (tgsi_info->processor) {
   case PIPE_SHADER_FRAGMENT:
      info->fs.color0_writes_all_cbufs =
            case PIPE_SHADER_GEOMETRY:
      info->gs.out_prim = tgsi_info->properties[TGSI_PROPERTY_GS_OUTPUT_PRIM];
   info->gs.in_prim = tgsi_info->properties[TGSI_PROPERTY_GS_INPUT_PRIM];
      case PIPE_SHADER_TESS_CTRL:
      info->tcs.vertices_out =
            for (unsigned i = 0; i < info->num_outputs; i++) {
      switch (tgsi_info->output_semantic_name[i]) {
   case TGSI_SEMANTIC_TESSOUTER:
   case TGSI_SEMANTIC_TESSINNER:
      info->tcs.writes_tess_factor = true;
      default:
            }
      case PIPE_SHADER_TESS_EVAL:
      info->tes.prim_mode =
            for (unsigned i = 0; i < info->num_inputs; i++) {
      switch (tgsi_info->input_semantic_name[i]) {
   case TGSI_SEMANTIC_PATCH:
   case TGSI_SEMANTIC_TESSOUTER:
   case TGSI_SEMANTIC_TESSINNER:
         default:
            }
      default:
            }
         /**
   * Compile a TGSI shader
   */
   struct svga_shader_variant *
   svga_tgsi_compile_shader(struct svga_context *svga,
               {
      if (svga_have_vgpu10(svga)) {
         }
   else {
            }
