   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   * Copyright 2008 VMware, Inc.  All rights reserved.
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
      /* Vertices are just an array of floats, with all the attributes
   * packed.  We currently assume a layout like:
   *
   * attr[0][0..3] - window position
   * attr[1..n][0..3] - remaining attributes.
   *
   * Attributes are assumed to be 4 floats wide but are packed so that
   * all the enabled attributes run contiguously.
   */
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_shader_tokens.h"
      #include "sp_context.h"
   #include "sp_state.h"
   #include "sp_quad.h"
   #include "sp_quad_pipe.h"
         struct quad_shade_stage
   {
                  };
         /**
   * Execute fragment shader for the four fragments in the quad.
   * \return TRUE if quad is alive, FALSE if all four pixels are killed
   */
   static inline bool
   shade_quad(struct quad_stage *qs, struct quad_header *quad)
   {
      struct softpipe_context *softpipe = qs->softpipe;
            if (softpipe->active_statistics_queries) {
      softpipe->pipeline_statistics.ps_invocations +=
               /* run shader */
   machine->flatshade_color = softpipe->rasterizer->flatshade ? true : false;
      }
            static void
   coverage_quad(struct quad_stage *qs, struct quad_header *quad)
   {
      struct softpipe_context *softpipe = qs->softpipe;
            /* loop over colorbuffer outputs */
   for (cbuf = 0; cbuf < softpipe->framebuffer.nr_cbufs; cbuf++) {
      float (*quadColor)[4] = quad->output.color[cbuf];
   unsigned j;
   for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      assert(quad->input.coverage[j] >= 0.0);
   assert(quad->input.coverage[j] <= 1.0);
            }
         /**
   * Shade/write an array of quads
   * Called via quad_stage::run()
   */
   static void
   shade_quads(struct quad_stage *qs, 
               {
      struct softpipe_context *softpipe = qs->softpipe;
   struct tgsi_exec_machine *machine = softpipe->fs_machine;
            tgsi_exec_set_constant_buffers(machine, PIPE_MAX_CONSTANT_BUFFERS,
                     for (i = 0; i < nr; i++) {
      /* Only omit this quad from the output list if all the fragments
   * are killed _AND_ it's not the first quad in the list.
   * The first quad is special in the (optimized) depth-testing code:
   * the quads' Z coordinates are step-wise interpolated with respect
   * to the first quad in the list.
   * For multi-pass algorithms we need to produce exactly the same
   * Z values in each pass.  If interpolation starts with different quads
   * we can get different Z values for the same (x,y).
   */
   if (!shade_quad(qs, quads[i]) && i > 0)
            if (/*do_coverage*/ 0)
               }
      if (nr_quads)
      }
            /**
   * Per-primitive (or per-begin?) setup
   */
   static void
   shade_begin(struct quad_stage *qs)
   {
         }
         static void
   shade_destroy(struct quad_stage *qs)
   {
         }
         struct quad_stage *
   sp_quad_shade_stage( struct softpipe_context *softpipe )
   {
      struct quad_shade_stage *qss = CALLOC_STRUCT(quad_shade_stage);
   if (!qss)
            qss->stage.softpipe = softpipe;
   qss->stage.begin = shade_begin;
   qss->stage.run = shade_quads;
                  fail:
      FREE(qss);
      }
