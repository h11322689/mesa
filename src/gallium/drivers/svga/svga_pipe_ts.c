   /**********************************************************
   * Copyright 2018-2022 VMware, Inc.  All rights reserved.
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
      #include "nir/nir_to_tgsi.h"
   #include "pipe/p_context.h"
   #include "util/u_memory.h"
      #include "svga_context.h"
   #include "svga_shader.h"
      static void
   svga_set_tess_state(struct pipe_context *pipe,
               {
      struct svga_context *svga = svga_context(pipe);
            for (i = 0; i < 4; i++) {
         }
   for (i = 0; i < 2; i++) {
            }
         static void
   svga_set_patch_vertices(struct pipe_context *pipe, uint8_t patch_vertices)
   {
                  }
         static void *
   svga_create_tcs_state(struct pipe_context *pipe,
         {
      struct svga_context *svga = svga_context(pipe);
                     tcs = (struct svga_tcs_shader *)
               if (!tcs)
         done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
               }
         static void
   svga_bind_tcs_state(struct pipe_context *pipe, void *shader)
   {
      struct svga_tcs_shader *tcs = (struct svga_tcs_shader *) shader;
            if (tcs == svga->curr.tcs)
            svga->curr.tcs = tcs;
            /* Check if the shader uses samplers */
   svga_set_curr_shader_use_samplers_flag(svga, PIPE_SHADER_TESS_CTRL,
      }
         static void
   svga_delete_tcs_state(struct pipe_context *pipe, void *shader)
   {
      struct svga_context *svga = svga_context(pipe);
   struct svga_tcs_shader *tcs = (struct svga_tcs_shader *) shader;
   struct svga_tcs_shader *next_tcs;
                              while (tcs) {
      next_tcs = (struct svga_tcs_shader *)tcs->base.next;
   for (variant = tcs->base.variants; variant; variant = tmp) {
               /* Check if deleting currently bound shader */
   if (variant == svga->state.hw_draw.tcs) {
      SVGA_RETRY(svga, svga_set_shader(svga, SVGA3D_SHADERTYPE_HS, NULL));
                           FREE((void *)tcs->base.tokens);
   FREE(tcs);
         }
         void
   svga_cleanup_tcs_state(struct svga_context *svga)
   {
      if (svga->tcs.passthrough_tcs) {
            }
         static void *
   svga_create_tes_state(struct pipe_context *pipe,
         {
      struct svga_context *svga = svga_context(pipe);
                     tes = (struct svga_tes_shader *)
                  if (!tes)
         done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
               }
         static void
   svga_bind_tes_state(struct pipe_context *pipe, void *shader)
   {
      struct svga_tes_shader *tes = (struct svga_tes_shader *) shader;
            if (tes == svga->curr.tes)
            svga->curr.tes = tes;
            /* Check if the shader uses samplers */
   svga_set_curr_shader_use_samplers_flag(svga, PIPE_SHADER_TESS_EVAL,
      }
         static void
   svga_delete_tes_state(struct pipe_context *pipe, void *shader)
   {
      struct svga_context *svga = svga_context(pipe);
   struct svga_tes_shader *tes = (struct svga_tes_shader *) shader;
   struct svga_tes_shader *next_tes;
                              while (tes) {
      next_tes = (struct svga_tes_shader *)tes->base.next;
   for (variant = tes->base.variants; variant; variant = tmp) {
               /* Check if deleting currently bound shader */
   if (variant == svga->state.hw_draw.tes) {
      SVGA_RETRY(svga, svga_set_shader(svga, SVGA3D_SHADERTYPE_DS, NULL));
                           FREE((void *)tes->base.tokens);
   FREE(tes);
         }
         void
   svga_init_ts_functions(struct svga_context *svga)
   {
      svga->pipe.set_tess_state = svga_set_tess_state;
   svga->pipe.set_patch_vertices = svga_set_patch_vertices;
   svga->pipe.create_tcs_state = svga_create_tcs_state;
   svga->pipe.bind_tcs_state = svga_bind_tcs_state;
   svga->pipe.delete_tcs_state = svga_delete_tcs_state;
   svga->pipe.create_tes_state = svga_create_tes_state;
   svga->pipe.bind_tes_state = svga_bind_tes_state;
      }
