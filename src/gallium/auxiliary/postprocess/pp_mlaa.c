   /**
   * Copyright (C) 2010 Jorge Jimenez (jorge@iryoku.com)
   * Copyright (C) 2010 Belen Masia (bmasia@unizar.es)
   * Copyright (C) 2010 Jose I. Echevarria (joseignacioechevarria@gmail.com)
   * Copyright (C) 2010 Fernando Navarro (fernandn@microsoft.com)
   * Copyright (C) 2010 Diego Gutierrez (diegog@unizar.es)
   * Copyright (C) 2011 Lauri Kasanen (cand@gmx.com)
   * All rights reserved.
   *
   * Redistribution and use in source and binary forms, with or without
   * modification, are permitted provided that the following conditions are met:
   *
   *    1. Redistributions of source code must retain the above copyright notice,
   *       this list of conditions and the following disclaimer.
   *
   *    2. Redistributions in binary form must reproduce the following statement:
   *
   *       "Uses Jimenez's MLAA. Copyright (C) 2010 by Jorge Jimenez, Belen Masia,
   *        Jose I. Echevarria, Fernando Navarro and Diego Gutierrez."
   *
   *       Only for use in the Mesa project, this point 2 is filled by naming the
   *       technique Jimenez's MLAA in the Mesa config options.
   *
   * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
   * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
   * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
   * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   * POSSIBILITY OF SUCH DAMAGE.
   *
   * The views and conclusions contained in the software and documentation are
   * those of the authors and should not be interpreted as representing official
   * policies, either expressed or implied, of the copyright holders.
   */
      #include "util/compiler.h"
      #include "postprocess/postprocess.h"
   #include "postprocess/pp_mlaa.h"
   #include "postprocess/pp_filters.h"
   #include "postprocess/pp_private.h"
      #include "util/u_box.h"
   #include "util/u_sampler.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
   #include "pipe/p_screen.h"
      #define IMM_SPACE 80
      static float constants[] = { 1, 1, 0, 0 };
   static unsigned int dimensions[2] = { 0, 0 };
      /** Run function of the MLAA filter. */
   static void
   pp_jimenezmlaa_run(struct pp_queue_t *ppq, struct pipe_resource *in,
         {
                  struct pipe_depth_stencil_alpha_state mstencil;
            unsigned int w = 0;
                     /* Insufficient initialization checks. */
   assert(p);
   assert(ppq);
   assert(ppq->areamaptex);
   assert(ppq->inner_tmp);
            w = p->framebuffer.width;
                              /* Init the pixel size constant */
   if (dimensions[0] != p->framebuffer.width ||
      dimensions[1] != p->framebuffer.height) {
   constants[0] = 1.0f / p->framebuffer.width;
            dimensions[0] = p->framebuffer.width;
               struct pipe_constant_buffer cb;
   cb.buffer = NULL;
   cb.buffer_offset = 0;
   cb.buffer_size = sizeof(constants);
            struct pipe_context *pipe = ppq->p->pipe;
   pipe->set_constant_buffer(pipe, PIPE_SHADER_VERTEX, 0, false, &cb);
            mstencil.stencil[0].enabled = 1;
   mstencil.stencil[0].valuemask = mstencil.stencil[0].writemask = ~0;
   mstencil.stencil[0].func = PIPE_FUNC_ALWAYS;
   mstencil.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
   mstencil.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
                     /* First pass: depth edge detection */
   if (iscolor)
         else
                     pp_filter_set_fb(p);
   pp_filter_misc_state(p);
   cso_set_depth_stencil_alpha(p->cso, &mstencil);
   p->pipe->clear(p->pipe, PIPE_CLEAR_STENCIL | PIPE_CLEAR_COLOR0, NULL,
            {
      const struct pipe_sampler_state *samplers[] = {&p->sampler_point};
      }
            cso_set_vertex_shader_handle(p->cso, ppq->shaders[n][1]);    /* offsetvs */
            pp_filter_draw(p);
               /* Second pass: blend weights */
   /* Sampler order: areamap, edgesmap, edgesmapL (reversed, thx compiler) */
   mstencil.stencil[0].func = PIPE_FUNC_EQUAL;
   mstencil.stencil[0].zpass_op = PIPE_STENCIL_OP_KEEP;
            pp_filter_setup_in(p, ppq->areamaptex);
            u_sampler_view_default_template(&v_tmp, ppq->inner_tmp[0],
         arr[1] = arr[2] = p->pipe->create_sampler_view(p->pipe,
                     {
      const struct pipe_sampler_state *samplers[] =
                     arr[0] = p->view;
            cso_set_vertex_shader_handle(p->cso, ppq->shaders[n][0]);    /* passvs */
            pp_filter_draw(p);
   pp_filter_end_pass(p);
               /* Third pass: smoothed edges */
   /* Sampler order: colormap, blendmap (wtf compiler) */
   pp_filter_setup_in(p, ppq->inner_tmp[1]);
                     /* Blit the input to the output */
   pp_blit(p->pipe, in, 0, 0,
                  u_sampler_view_default_template(&v_tmp, in, in->format);
            {
      const struct pipe_sampler_state *samplers[] =
                     arr[1] = p->view;
            cso_set_vertex_shader_handle(p->cso, ppq->shaders[n][1]);    /* offsetvs */
            p->blend.rt[0].blend_enable = 1;
            pp_filter_draw(p);
   pp_filter_end_pass(p);
            p->blend.rt[0].blend_enable = 0;
      }
      /** The init function of the MLAA filter. */
   static bool
   pp_jimenezmlaa_init_run(struct pp_queue_t *ppq, unsigned int n,
         {
         struct pipe_box box;
   struct pipe_resource res;
            tmp_text = CALLOC(sizeof(blend2fs_1) + sizeof(blend2fs_2) +
            if (!tmp_text) {
      pp_debug("Failed to allocate shader space\n");
                        sprintf(tmp_text, "%s"
                           res.target = PIPE_TEXTURE_2D;
   res.format = PIPE_FORMAT_R8G8_UNORM;
   res.width0 = res.height0 = 165;
   res.bind = PIPE_BIND_SAMPLER_VIEW;
   res.usage = PIPE_USAGE_DEFAULT;
            if (!ppq->p->screen->is_format_supported(ppq->p->screen, res.format,
                  ppq->areamaptex = ppq->p->screen->resource_create(ppq->p->screen, &res);
      if (ppq->areamaptex == NULL) {
      pp_debug("Failed to allocate area map texture\n");
      }
               ppq->p->pipe->texture_subdata(ppq->p->pipe, ppq->areamaptex, 0,
                  ppq->shaders[n][1] = pp_tgsi_to_state(ppq->p->pipe, offsetvs, true,
         if (iscolor)
      ppq->shaders[n][2] = pp_tgsi_to_state(ppq->p->pipe, color1fs,
      else
      ppq->shaders[n][2] = pp_tgsi_to_state(ppq->p->pipe, depth1fs,
      ppq->shaders[n][3] = pp_tgsi_to_state(ppq->p->pipe, tmp_text, false,
         ppq->shaders[n][4] = pp_tgsi_to_state(ppq->p->pipe, neigh3fs, false,
                           fail:
                  /*
   * Call the common free function for destruction of partially initialized
   * resources.
   */
               }
      /** Short wrapper to init the depth version. */
   bool
   pp_jimenezmlaa_init(struct pp_queue_t *ppq, unsigned int n, unsigned int val)
   {
         }
      /** Short wrapper to init the color version. */
   bool
   pp_jimenezmlaa_init_color(struct pp_queue_t *ppq, unsigned int n,
         {
         }
      /** Short wrapper to run the depth version. */
   void
   pp_jimenezmlaa(struct pp_queue_t *ppq, struct pipe_resource *in,
         {
      if (!ppq->depth) {
         }
      }
      /** Short wrapper to run the color version. */
   void
   pp_jimenezmlaa_color(struct pp_queue_t *ppq, struct pipe_resource *in,
         {
         }
         /**
   * Short wrapper to free the mlaa filter resources. Shaders are freed in
   * the common code in pp_free.
   */
   void
   pp_jimenezmlaa_free(struct pp_queue_t *ppq, unsigned int n)
   {
         }
   