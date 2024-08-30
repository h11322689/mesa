   /**********************************************************
   * Copyright 2014-2022 VMware, Inc.  All rights reserved.
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
      #include "draw/draw_context.h"
   #include "nir/nir_to_tgsi.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_bitmask.h"
      #include "svga_context.h"
   #include "svga_cmd.h"
   #include "svga_debug.h"
   #include "svga_shader.h"
   #include "svga_streamout.h"
      static void *
   svga_create_gs_state(struct pipe_context *pipe,
         {
      struct svga_context *svga = svga_context(pipe);
                     gs = (struct svga_geometry_shader *)
                  if (!gs)
            /* Original shader IR could have been deleted if it is converted from
   * NIR to TGSI. So need to explicitly set the shader state type to TGSI
   * before passing it to draw.
   */
   struct pipe_shader_state tmp = *templ;
   tmp.type = PIPE_SHADER_IR_TGSI;
            gs->base.get_dummy_shader = svga_get_compiled_dummy_geometry_shader;
         done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         static void
   svga_bind_gs_state(struct pipe_context *pipe, void *shader)
   {
      struct svga_geometry_shader *gs = (struct svga_geometry_shader *)shader;
            svga->curr.user_gs = gs;
            /* Check if the shader uses samplers */
   svga_set_curr_shader_use_samplers_flag(svga, PIPE_SHADER_GEOMETRY,
      }
         static void
   svga_delete_gs_state(struct pipe_context *pipe, void *shader)
   {
      struct svga_context *svga = svga_context(pipe);
   struct svga_geometry_shader *gs = (struct svga_geometry_shader *)shader;
   struct svga_geometry_shader *next_gs;
                     /* Start deletion from the original geometry shader state */
   if (gs->base.parent != NULL)
            /* Free the list of geometry shaders */
   while (gs) {
               if (gs->base.stream_output != NULL)
                     for (variant = gs->base.variants; variant; variant = tmp) {
               /* Check if deleting currently bound shader */
   if (variant == svga->state.hw_draw.gs) {
      SVGA_RETRY(svga, svga_set_shader(svga, SVGA3D_SHADERTYPE_GS, NULL));
                           FREE((void *)gs->base.tokens);
   FREE(gs);
         }
         void
   svga_init_gs_functions(struct svga_context *svga)
   {
      svga->pipe.create_gs_state = svga_create_gs_state;
   svga->pipe.bind_gs_state = svga_bind_gs_state;
      }
