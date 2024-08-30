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
      #include "nir/nir_to_tgsi.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_bitmask.h"
   #include "draw/draw_context.h"
      #include "svga_context.h"
   #include "svga_hw_reg.h"
   #include "svga_cmd.h"
   #include "svga_debug.h"
   #include "svga_shader.h"
         void *
   svga_create_fs_state(struct pipe_context *pipe,
         {
      struct svga_context *svga = svga_context(pipe);
                     fs = (struct svga_fragment_shader *)
               if (!fs)
            /* Original shader IR could have been deleted if it is converted from
   * NIR to TGSI. So need to explicitly set the shader state type to TGSI
   * before passing it to draw.
   */
   struct pipe_shader_state tmp = *templ;
   tmp.type = PIPE_SHADER_IR_TGSI;
                              svga_remap_generics(fs->base.info.generic_inputs_mask,
                  done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         void
   svga_bind_fs_state(struct pipe_context *pipe, void *shader)
   {
      struct svga_fragment_shader *fs = (struct svga_fragment_shader *) shader;
            svga->curr.fs = fs;
            /* Check if shader uses samplers */
   svga_set_curr_shader_use_samplers_flag(svga, PIPE_SHADER_FRAGMENT,
      }
         static void
   svga_delete_fs_state(struct pipe_context *pipe, void *shader)
   {
      struct svga_context *svga = svga_context(pipe);
   struct svga_fragment_shader *fs = (struct svga_fragment_shader *) shader;
   struct svga_fragment_shader *next_fs;
                              while (fs) {
                        for (variant = fs->base.variants; variant; variant = tmp) {
               /* Check if deleting currently bound shader */
   if (variant == svga->state.hw_draw.fs) {
      SVGA_RETRY(svga, svga_set_shader(svga, SVGA3D_SHADERTYPE_PS, NULL));
                           FREE((void *)fs->base.tokens);
   FREE(fs);
         }
         void
   svga_init_fs_functions(struct svga_context *svga)
   {
      svga->pipe.create_fs_state = svga_create_fs_state;
   svga->pipe.bind_fs_state = svga_bind_fs_state;
      }
