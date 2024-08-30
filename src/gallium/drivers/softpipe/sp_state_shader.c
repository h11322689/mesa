   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc.
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
      #include "sp_context.h"
   #include "sp_screen.h"
   #include "sp_state.h"
   #include "sp_fs.h"
   #include "sp_texture.h"
      #include "nir.h"
   #include "nir/nir_to_tgsi.h"
   #include "pipe/p_defines.h"
   #include "util/ralloc.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "draw/draw_context.h"
   #include "draw/draw_vs.h"
   #include "draw/draw_gs.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_from_mesa.h"
   #include "tgsi/tgsi_scan.h"
   #include "tgsi/tgsi_parse.h"
   #include "compiler/shader_enums.h"
         /**
   * Create a new fragment shader variant.
   */
   static struct sp_fragment_shader_variant *
   create_fs_variant(struct softpipe_context *softpipe,
               {
      struct sp_fragment_shader_variant *var;
            /* codegen, create variant object */
            if (var) {
                                 #if 0
         /* draw's fs state */
   var->draw_shader = draw_create_fragment_shader(softpipe->draw,
         if (!var->draw_shader) {
      var->delete(var);
   FREE((void *) var->tokens);
      #endif
            /* insert variant into linked list */
   var->next = fs->variants;
                  }
         struct sp_fragment_shader_variant *
   softpipe_find_fs_variant(struct softpipe_context *sp,
               {
               for (var = fs->variants; var; var = var->next) {
      if (memcmp(&var->key, key, sizeof(*key)) == 0) {
      /* found it */
                     }
      static void
   softpipe_shader_db(struct pipe_context *pipe, const struct tgsi_token *tokens)
   {
      struct tgsi_shader_info info;
   tgsi_scan_shader(tokens, &info);
   util_debug_message(&pipe->debug, SHADER_INFO, "%s shader: %d inst, %d loops, %d temps, %d const, %d imm",
                     _mesa_shader_stage_to_abbrev(tgsi_processor_to_shader_stage(info.processor)),
   info.num_instructions,
      }
      static void
   softpipe_create_shader_state(struct pipe_context *pipe,
                     {
      if (templ->type == PIPE_SHADER_IR_NIR) {
      if (debug)
               } else {
      assert(templ->type == PIPE_SHADER_IR_TGSI);
   /* we need to keep a local copy of the tokens */
                                 if (debug)
               }
      static void *
   softpipe_create_fs_state(struct pipe_context *pipe,
         {
      struct softpipe_context *softpipe = softpipe_context(pipe);
            softpipe_create_shader_state(pipe, &state->shader, templ,
            /* draw's fs state */
   state->draw_shader = draw_create_fragment_shader(softpipe->draw,
         if (!state->draw_shader) {
      tgsi_free_tokens(state->shader.tokens);
   FREE(state);
                  }
         static void
   softpipe_bind_fs_state(struct pipe_context *pipe, void *fs)
   {
      struct softpipe_context *softpipe = softpipe_context(pipe);
            if (softpipe->fs == fs)
                              /* This depends on the current fragment shader and must always be
   * re-validated before use.
   */
            if (state)
      draw_bind_fragment_shader(softpipe->draw,
      else
               }
         static void
   softpipe_delete_fs_state(struct pipe_context *pipe, void *fs)
   {
      struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_fragment_shader *state = fs;
                     /* delete variants */
   for (var = state->variants; var; var = next_var) {
                        #if 0
         #endif
                                 tgsi_free_tokens(state->shader.tokens);
      }
         static void *
   softpipe_create_vs_state(struct pipe_context *pipe,
         {
      struct softpipe_context *softpipe = softpipe_context(pipe);
            state = CALLOC_STRUCT(sp_vertex_shader);
   if (!state)
            softpipe_create_shader_state(pipe, &state->shader, templ,
         if (!state->shader.tokens)
            state->draw_data = draw_create_vertex_shader(softpipe->draw, &state->shader);
   if (state->draw_data == NULL) 
                           fail:
      if (state) {
      tgsi_free_tokens(state->shader.tokens);
   FREE( state->draw_data );
      }
      }
         static void
   softpipe_bind_vs_state(struct pipe_context *pipe, void *vs)
   {
                        draw_bind_vertex_shader(softpipe->draw,
               }
         static void
   softpipe_delete_vs_state(struct pipe_context *pipe, void *vs)
   {
                        draw_delete_vertex_shader(softpipe->draw, state->draw_data);
   tgsi_free_tokens(state->shader.tokens);
      }
         static void *
   softpipe_create_gs_state(struct pipe_context *pipe,
         {
      struct softpipe_context *softpipe = softpipe_context(pipe);
            state = CALLOC_STRUCT(sp_geometry_shader);
   if (!state)
            softpipe_create_shader_state(pipe, &state->shader, templ,
            if (state->shader.tokens) {
      state->draw_data = draw_create_geometry_shader(softpipe->draw,
         if (state->draw_data == NULL)
                              fail:
      if (state) {
      tgsi_free_tokens(state->shader.tokens);
   FREE( state->draw_data );
      }
      }
         static void
   softpipe_bind_gs_state(struct pipe_context *pipe, void *gs)
   {
                        draw_bind_geometry_shader(softpipe->draw,
               }
         static void
   softpipe_delete_gs_state(struct pipe_context *pipe, void *gs)
   {
               struct sp_geometry_shader *state =
            draw_delete_geometry_shader(softpipe->draw,
            tgsi_free_tokens(state->shader.tokens);
      }
         static void
   softpipe_set_constant_buffer(struct pipe_context *pipe,
                     {
      struct softpipe_context *softpipe = softpipe_context(pipe);
   struct pipe_resource *constants = cb ? cb->buffer : NULL;
   unsigned size;
                     if (cb && cb->user_buffer) {
      constants = softpipe_user_buffer_create(pipe->screen,
                           size = cb ? cb->buffer_size : 0;
   data = constants ? softpipe_resource_data(constants) : NULL;
   if (data)
                     /* note: reference counting */
   if (take_ownership) {
      pipe_resource_reference(&softpipe->constants[shader][index], NULL);
      } else {
                  if (shader == PIPE_SHADER_VERTEX || shader == PIPE_SHADER_GEOMETRY) {
                  softpipe->mapped_constants[shader][index].ptr = data;
                     if (cb && cb->user_buffer) {
            }
      static void *
   softpipe_create_compute_state(struct pipe_context *pipe,
         {
                        if (templ->ir_type == PIPE_SHADER_IR_NIR) {
               if (sp_debug & SP_DBG_CS)
               } else {
      assert(templ->ir_type == PIPE_SHADER_IR_TGSI);
   /* we need to keep a local copy of the tokens */
               if (sp_debug & SP_DBG_CS)
                                          }
      static void
   softpipe_bind_compute_state(struct pipe_context *pipe,
         {
      struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_compute_shader *state = (struct sp_compute_shader *)cs;
   if (softpipe->cs == state)
               }
      static void
   softpipe_delete_compute_state(struct pipe_context *pipe,
         {
      ASSERTED struct softpipe_context *softpipe = softpipe_context(pipe);
            assert(softpipe->cs != state);
   tgsi_free_tokens(state->tokens);
      }
      void
   softpipe_init_shader_funcs(struct pipe_context *pipe)
   {
      pipe->create_fs_state = softpipe_create_fs_state;
   pipe->bind_fs_state   = softpipe_bind_fs_state;
            pipe->create_vs_state = softpipe_create_vs_state;
   pipe->bind_vs_state   = softpipe_bind_vs_state;
            pipe->create_gs_state = softpipe_create_gs_state;
   pipe->bind_gs_state   = softpipe_bind_gs_state;
                     pipe->create_compute_state = softpipe_create_compute_state;
   pipe->bind_compute_state = softpipe_bind_compute_state;
      }
