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
      /**
   * Execute fragment shader using the TGSI interpreter.
   */
      #include "sp_context.h"
   #include "sp_state.h"
   #include "sp_fs.h"
   #include "sp_quad.h"
      #include "pipe/p_state.h"
   #include "pipe/p_defines.h"
   #include "util/u_memory.h"
   #include "tgsi/tgsi_exec.h"
         /**
   * Subclass of sp_fragment_shader_variant
   */
   struct sp_exec_fragment_shader
   {
      struct sp_fragment_shader_variant base;
      };
         static void
   exec_prepare( const struct sp_fragment_shader_variant *var,
               struct tgsi_exec_machine *machine,
   struct tgsi_sampler *sampler,
   {
      /*
   * Bind tokens/shader to the interpreter's machine state.
   */
   tgsi_exec_machine_bind_shader(machine,
            }
            /**
   * Compute quad X,Y,Z,W for the four fragments in a quad.
   *
   * This should really be part of the compiled shader.
   */
   static void
   setup_pos_vector(const struct tgsi_interp_coef *coef,
               {
      uint chan;
   /* do X */
   quadpos->xyzw[0].f[0] = x;
   quadpos->xyzw[0].f[1] = x + 1;
   quadpos->xyzw[0].f[2] = x;
            /* do Y */
   quadpos->xyzw[1].f[0] = y;
   quadpos->xyzw[1].f[1] = y;
   quadpos->xyzw[1].f[2] = y + 1;
            /* do Z and W for all fragments in the quad */
   for (chan = 2; chan < 4; chan++) {
      const float dadx = coef->dadx[chan];
   const float dady = coef->dady[chan];
   const float a0 = coef->a0[chan] + dadx * x + dady * y;
   quadpos->xyzw[chan].f[0] = a0;
   quadpos->xyzw[chan].f[1] = a0 + dadx;
   quadpos->xyzw[chan].f[2] = a0 + dady;
         }
         /* TODO: hide the machine struct in here somewhere, remove from this
   * interface:
   */
   static unsigned 
   exec_run( const struct sp_fragment_shader_variant *var,
      struct tgsi_exec_machine *machine,
   struct quad_header *quad,
      {
      /* Compute X, Y, Z, W vals for this quad */
   setup_pos_vector(quad->posCoef, 
                  /* convert 0 to 1.0 and 1 to -1.0 */
            machine->NonHelperMask = quad->inout.mask;
   quad->inout.mask &= tgsi_exec_machine_run( machine, 0 );
   if (quad->inout.mask == 0)
            /* store outputs */
   {
      const uint8_t *sem_name = var->info.output_semantic_name;
   const uint8_t *sem_index = var->info.output_semantic_index;
   const uint n = var->info.num_outputs;
   uint i;
   for (i = 0; i < n; i++) {
      switch (sem_name[i]) {
   case TGSI_SEMANTIC_COLOR:
                                       /* copy float[4][4] result */
   memcpy(quad->output.color[cbuf],
            }
      case TGSI_SEMANTIC_POSITION:
                        if (!early_depth_test) {
      for (j = 0; j < 4; j++)
         }
      case TGSI_SEMANTIC_STENCIL:
      {
      uint j;
   if (!early_depth_test) {
      for (j = 0; j < 4; j++)
         }
                        }
         static void 
   exec_delete(struct sp_fragment_shader_variant *var,
         {
      if (machine->Tokens == var->tokens) {
                  FREE( (void *) var->tokens );
      }
         struct sp_fragment_shader_variant *
   softpipe_create_fs_variant_exec(struct softpipe_context *softpipe)
   {
               shader = CALLOC_STRUCT(sp_exec_fragment_shader);
   if (!shader)
            shader->base.prepare = exec_prepare;
   shader->base.run = exec_run;
               }
