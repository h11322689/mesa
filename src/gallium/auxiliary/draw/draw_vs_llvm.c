   /**************************************************************************
   *
   * Copyright 2010 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "pipe/p_shader_tokens.h"
   #include "pipe/p_screen.h"
      #include "draw_private.h"
   #include "draw_context.h"
   #include "draw_vs.h"
   #include "draw_llvm.h"
      #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_scan.h"
   #include "nir/nir_to_tgsi_info.h"
   #include "nir.h"
         static void
   vs_llvm_prepare(struct draw_vertex_shader *shader,
         {
         }
         static void
   vs_llvm_run_linear(struct draw_vertex_shader *shader,
                     const float (*input)[4],
   float (*output)[4],
   const struct draw_buffer_info *constants,
   unsigned count,
   {
      /* we should never get here since the entire pipeline is
   * generated in draw_pt_fetch_shade_pipeline_llvm.c */
      }
         static void
   vs_llvm_delete(struct draw_vertex_shader *dvs)
   {
      struct llvm_vertex_shader *shader = llvm_vertex_shader(dvs);
            LIST_FOR_EACH_ENTRY_SAFE(li, next, &shader->variants.list, list) {
                  assert(shader->variants_cached == 0);
   if (dvs->state.ir.nir)
         FREE((void*) dvs->state.tokens);
      }
         struct draw_vertex_shader *
   draw_create_vs_llvm(struct draw_context *draw,
         {
               if (!vs)
            if (state->type == PIPE_SHADER_IR_NIR) {
      vs->base.state.ir.nir = state->ir.nir;
   nir_shader *nir = (nir_shader *)state->ir.nir;
   if (!nir->options->lower_uniforms_to_ubo)
            } else {
      /* we make a private copy of the tokens */
   vs->base.state.tokens = tgsi_dup_tokens(state->tokens);
   if (!vs->base.state.tokens) {
      FREE(vs);
                           vs->variant_key_size =
      draw_llvm_variant_key_size(
      vs->base.info.file_max[TGSI_FILE_INPUT]+1,
   vs->base.info.file_max[TGSI_FILE_SAMPLER]+1,
            vs->base.state.type = state->type;
   vs->base.state.stream_output = state->stream_output;
   vs->base.draw = draw;
   vs->base.prepare = vs_llvm_prepare;
   vs->base.run_linear = vs_llvm_run_linear;
   vs->base.delete = vs_llvm_delete;
                        }
