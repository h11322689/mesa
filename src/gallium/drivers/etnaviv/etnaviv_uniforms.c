   /*
   * Copyright (c) 2016 Etnaviv Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include "etnaviv_uniforms.h"
      #include "etnaviv_compiler.h"
   #include "etnaviv_context.h"
   #include "etnaviv_util.h"
   #include "etnaviv_emit.h"
   #include "pipe/p_defines.h"
   #include "util/u_math.h"
      static unsigned
   get_const_idx(const struct etna_context *ctx, bool frag, unsigned samp_id)
   {
               if (frag)
               }
      static uint32_t
   get_texrect_scale(const struct etna_context *ctx, bool frag,
         {
      unsigned index = get_const_idx(ctx, frag, data);
   struct pipe_sampler_view *texture = ctx->sampler_view[index];
            if (contents == ETNA_UNIFORM_TEXRECT_SCALE_X)
         else
               }
      static inline bool
   is_array_texture(enum pipe_texture_target target)
   {
      switch (target) {
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
         default:
            }
      static uint32_t
   get_texture_size(const struct etna_context *ctx, bool frag,
         {
      unsigned index = get_const_idx(ctx, frag, data);
            switch (contents) {
   case ETNA_UNIFORM_TEXTURE_WIDTH:
      if (texture->target == PIPE_BUFFER) {
         } else {
            case ETNA_UNIFORM_TEXTURE_HEIGHT:
         case ETNA_UNIFORM_TEXTURE_DEPTH:
               if (is_array_texture(texture->target)) {
      if (texture->target != PIPE_TEXTURE_CUBE_ARRAY) {
         } else {
      assert(texture->texture->array_size % 6 == 0);
                     default:
            }
      void
   etna_uniforms_write(const struct etna_context *ctx,
               {
      struct etna_screen *screen = ctx->screen;
   struct etna_cmd_stream *stream = ctx->stream;
   const struct etna_shader_uniform_info *uinfo = &sobj->uniforms;
   bool frag = (sobj == ctx->shader.fs);
   uint32_t base = frag ? screen->specs.ps_uniforms_offset : screen->specs.vs_uniforms_offset;
            if (!uinfo->count)
            etna_cmd_stream_reserve(stream, align(uinfo->count + 1, 2));
            for (uint32_t i = 0; i < uinfo->count; i++) {
               switch (uinfo->contents[i]) {
   case ETNA_UNIFORM_CONSTANT:
                  case ETNA_UNIFORM_UNIFORM:
      assert(cb->user_buffer && val * 4 < cb->buffer_size);
               case ETNA_UNIFORM_TEXRECT_SCALE_X:
   case ETNA_UNIFORM_TEXRECT_SCALE_Y:
      etna_cmd_stream_emit(stream,
               case ETNA_UNIFORM_TEXTURE_WIDTH:
   case ETNA_UNIFORM_TEXTURE_HEIGHT:
   case ETNA_UNIFORM_TEXTURE_DEPTH:
      etna_cmd_stream_emit(stream,
               case ETNA_UNIFORM_UBO0_ADDR ... ETNA_UNIFORM_UBOMAX_ADDR:
      idx = uinfo->contents[i] - ETNA_UNIFORM_UBO0_ADDR;
   etna_cmd_stream_reloc(stream, &(struct etna_reloc) {
      .bo = etna_resource(cb[idx].buffer)->bo,
   .flags = ETNA_RELOC_READ,
                  case ETNA_UNIFORM_UNUSED:
      etna_cmd_stream_emit(stream, 0);
                  if ((uinfo->count % 2) == 0)
      }
      void
   etna_set_shader_uniforms_dirty_flags(struct etna_shader_variant *sobj)
   {
               for (uint32_t i = 0; i < sobj->uniforms.count; i++) {
      switch (sobj->uniforms.contents[i]) {
   default:
            case ETNA_UNIFORM_TEXRECT_SCALE_X:
   case ETNA_UNIFORM_TEXRECT_SCALE_Y:
      dirty |= ETNA_DIRTY_SAMPLER_VIEWS;
                     }
