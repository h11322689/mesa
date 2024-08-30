   /**************************************************************************
   *
   * Copyright 2011 Lauri Kasanen
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
   * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "postprocess.h"
   #include "postprocess/pp_filters.h"
   #include "postprocess/pp_private.h"
      #include "frontend/api.h"
   #include "util/u_inlines.h"
   #include "util/u_sampler.h"
      #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_text.h"
         void
   pp_blit(struct pipe_context *pipe,
         struct pipe_resource *src_tex,
   int srcX0, int srcY0,
   int srcX1, int srcY1,
   int srcZ0,
   struct pipe_surface *dst,
   int dstX0, int dstY0,
   {
                        blit.src.resource = src_tex;
   blit.src.level = 0;
   blit.src.format = src_tex->format;
   blit.src.box.x = srcX0;
   blit.src.box.y = srcY0;
   blit.src.box.z = srcZ0;
   blit.src.box.width = srcX1 - srcX0;
   blit.src.box.height = srcY1 - srcY0;
            blit.dst.resource = dst->texture;
   blit.dst.level = dst->u.tex.level;
   blit.dst.format = dst->format;
   blit.dst.box.x = dstX0;
   blit.dst.box.y = dstY0;
   blit.dst.box.z = 0;
   blit.dst.box.width = dstX1 - dstX0;
   blit.dst.box.height = dstY1 - dstY0;
                        }
      /**
   *	Main run function of the PP queue. Called on swapbuffers/flush.
   *
   *	Runs all requested filters in order and handles shuffling the temp
   *	buffers in between.
   */
   void
   pp_run(struct pp_queue_t *ppq, struct pipe_resource *in,
         {
      struct pipe_resource *refin = NULL, *refout = NULL;
   unsigned int i;
            if (ppq->n_filters == 0)
            assert(ppq->pp_queue);
            if (in->width0 != ppq->p->framebuffer.width ||
      in->height0 != ppq->p->framebuffer.height) {
   pp_debug("Resizing the temp pp buffers\n");
   pp_free_fbos(ppq);
               if (in == out && ppq->n_filters == 1) {
      /* Make a copy of in to tmp[0] in this case. */
   unsigned int w = ppq->p->framebuffer.width;
               pp_blit(ppq->p->pipe, in, 0, 0,
                              /* save state (restored below) */
   cso_save_state(cso, (CSO_BIT_BLEND |
                        CSO_BIT_DEPTH_STENCIL_ALPHA |
   CSO_BIT_FRAGMENT_SHADER |
   CSO_BIT_FRAMEBUFFER |
   CSO_BIT_TESSCTRL_SHADER |
   CSO_BIT_TESSEVAL_SHADER |
   CSO_BIT_GEOMETRY_SHADER |
   CSO_BIT_RASTERIZER |
   CSO_BIT_SAMPLE_MASK |
   CSO_BIT_MIN_SAMPLES |
   CSO_BIT_FRAGMENT_SAMPLERS |
   CSO_BIT_STENCIL_REF |
   CSO_BIT_STREAM_OUTPUTS |
   CSO_BIT_VERTEX_ELEMENTS |
         /* set default state */
   cso_set_sample_mask(cso, ~0);
   cso_set_min_samples(cso, 1);
   cso_set_stream_outputs(cso, 0, NULL, NULL);
   cso_set_tessctrl_shader_handle(cso, NULL);
   cso_set_tesseval_shader_handle(cso, NULL);
   cso_set_geometry_shader_handle(cso, NULL);
            // Kept only for this frame.
   pipe_resource_reference(&ppq->depth, indepth);
   pipe_resource_reference(&refin, in);
            switch (ppq->n_filters) {
   case 0:
      /* Failsafe, but never reached. */
      case 1:                     /* No temp buf */
      ppq->pp_queue[0] (ppq, in, out, 0);
                  ppq->pp_queue[0] (ppq, in, ppq->tmp[0], 0);
               default:                    /* Two temp bufs */
      assert(ppq->tmp[1]);
            for (i = 1; i < (ppq->n_filters - 1); i++) {
                     else
               if (i % 2 == 0)
            else
                        /* restore state we changed */
   cso_restore_state(cso, CSO_UNBIND_FS_SAMPLERVIEWS |
                              /* restore states not restored by cso */
   if (ppq->p->st) {
      ppq->p->st_invalidate_state(ppq->p->st,
                                 pipe_resource_reference(&ppq->depth, NULL);
   pipe_resource_reference(&refin, NULL);
      }
         /* Utility functions for the filters. You're not forced to use these if */
   /* your filter is more complicated. */
      /** Setup this resource as the filter input. */
   void
   pp_filter_setup_in(struct pp_program *p, struct pipe_resource *in)
   {
      struct pipe_sampler_view v_tmp;
   u_sampler_view_default_template(&v_tmp, in, in->format);
      }
      /** Setup this resource as the filter output. */
   void
   pp_filter_setup_out(struct pp_program *p, struct pipe_resource *out)
   {
                  }
      /** Clean up the input and output set with the above. */
   void
   pp_filter_end_pass(struct pp_program *p)
   {
      pipe_surface_reference(&p->framebuffer.cbufs[0], NULL);
      }
      /**
   *	Convert the TGSI assembly to a runnable shader.
   *
   * We need not care about geometry shaders. All we have is screen quads.
   */
   void *
   pp_tgsi_to_state(struct pipe_context *pipe, const char *text, bool isvs,
         {
      struct pipe_shader_state state;
   struct tgsi_token *tokens = NULL;
            /*
   * Allocate temporary token storage. State creation will duplicate
   * tokens so we must free them on exit.
   */ 
            if (!tokens) {
      pp_debug("Failed to allocate temporary token storage.\n");
               if (tgsi_text_translate(text, tokens, PP_MAX_TOKENS) == false) {
      _debug_printf("pp: Failed to translate a shader for %s\n", name);
                        if (isvs) {
      ret_state = pipe->create_vs_state(pipe, &state);
      } else {
      ret_state = pipe->create_fs_state(pipe, &state);
                  }
      /** Setup misc state for the filter. */
   void
   pp_filter_misc_state(struct pp_program *p)
   {
      cso_set_blend(p->cso, &p->blend);
   cso_set_depth_stencil_alpha(p->cso, &p->depthstencil);
   cso_set_rasterizer(p->cso, &p->rasterizer);
               }
      /** Draw with the filter to the set output. */
   void
   pp_filter_draw(struct pp_program *p)
   {
      util_draw_vertex_buffer(p->pipe, p->cso, p->vbuf, 0,
      }
      /** Set the framebuffer as active. */
   void
   pp_filter_set_fb(struct pp_program *p)
   {
         }
      /** Set the framebuffer as active and clear it. */
   void
   pp_filter_set_clear_fb(struct pp_program *p)
   {
      cso_set_framebuffer(p->cso, &p->framebuffer);
      }
