   /**************************************************************************
   *
   * Copyright 2007-2018 VMware, Inc.
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
   * AA line stage:  AA lines are converted triangles (with extra generic)
   *
   * Authors:  Brian Paul
   */
         #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_shader_tokens.h"
   #include "util/u_inlines.h"
      #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "tgsi/tgsi_transform.h"
   #include "tgsi/tgsi_dump.h"
      #include "draw_context.h"
   #include "draw_private.h"
   #include "draw_pipe.h"
      #include "nir.h"
   #include "nir/nir_draw_helpers.h"
      /** Approx number of new tokens for instructions in aa_transform_inst() */
   #define NUM_NEW_TOKENS 53
         /**
   * Subclass of pipe_shader_state to carry extra fragment shader info.
   */
   struct aaline_fragment_shader
   {
      struct pipe_shader_state state;
   void *driver_fs;
   void *aaline_fs;
      };
         /**
   * Subclass of draw_stage
   */
   struct aaline_stage
   {
                        /** For AA lines, this is the vertex attrib slot for new generic */
   unsigned coord_slot;
   /** position, not necessarily output zero */
               /*
   * Currently bound state
   */
            /*
   * Driver interface/override functions
   */
   void * (*driver_create_fs_state)(struct pipe_context *,
         void (*driver_bind_fs_state)(struct pipe_context *, void *);
      };
            /**
   * Subclass of tgsi_transform_context, used for transforming the
   * user's fragment shader to add the special AA instructions.
   */
   struct aa_transform_context {
      struct tgsi_transform_context base;
   uint64_t tempsUsed;  /**< bitmask */
   int colorOutput; /**< which output is the primary color */
   int maxInput, maxGeneric;  /**< max input index found */
   int numImm; /**< number of immediate regsters */
      };
      /**
   * TGSI declaration transform callback.
   * Look for a free input attrib, and two free temp regs.
   */
   static void
   aa_transform_decl(struct tgsi_transform_context *ctx,
         {
               if (decl->Declaration.File == TGSI_FILE_OUTPUT &&
      decl->Semantic.Name == TGSI_SEMANTIC_COLOR &&
   decl->Semantic.Index == 0) {
      }
   else if (decl->Declaration.File == TGSI_FILE_INPUT) {
      if ((int) decl->Range.Last > aactx->maxInput)
         if (decl->Semantic.Name == TGSI_SEMANTIC_GENERIC &&
      (int) decl->Semantic.Index > aactx->maxGeneric) {
         }
   else if (decl->Declaration.File == TGSI_FILE_TEMPORARY) {
      unsigned i;
   for (i = decl->Range.First;
      i <= decl->Range.Last; i++) {
   /*
   * XXX this bitfield doesn't really cut it...
   */
                     }
      /**
   * TGSI immediate declaration transform callback.
   */
   static void
   aa_immediate(struct tgsi_transform_context *ctx,
         {
               ctx->emit_immediate(ctx, imm);
      }
      /**
   * Find the lowest zero bit, or -1 if bitfield is all ones.
   */
   static int
   free_bit(uint64_t bitfield)
   {
         }
         /**
   * TGSI transform prolog callback.
   */
   static void
   aa_transform_prolog(struct tgsi_transform_context *ctx)
   {
      struct aa_transform_context *aactx = (struct aa_transform_context *) ctx;
            /* find two free temp regs */
   aactx->colorTemp = free_bit(usedTemps);
   usedTemps |= UINT64_C(1) << aactx->colorTemp;
   aactx->aaTemp = free_bit(usedTemps);
   assert(aactx->colorTemp >= 0);
            /* declare new generic input/texcoord */
   tgsi_transform_input_decl(ctx, aactx->maxInput + 1,
                  /* declare new temp regs */
   tgsi_transform_temp_decl(ctx, aactx->aaTemp);
            /* declare new immediate reg */
      }
         /**
   * TGSI transform epilog callback.
   */
   static void
   aa_transform_epilog(struct tgsi_transform_context *ctx)
   {
               if (aactx->colorOutput != -1) {
      struct tgsi_full_instruction inst;
            /* saturate(linewidth - fabs(interpx), linelength - fabs(interpz) */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Saturate = true;
   inst.Instruction.Opcode = TGSI_OPCODE_ADD;
   inst.Instruction.NumDstRegs = 1;
   tgsi_transform_dst_reg(&inst.Dst[0], TGSI_FILE_TEMPORARY,
         inst.Instruction.NumSrcRegs = 2;
   tgsi_transform_src_reg(&inst.Src[1], TGSI_FILE_INPUT, aactx->maxInput + 1,
               tgsi_transform_src_reg(&inst.Src[0], TGSI_FILE_INPUT, aactx->maxInput + 1,
               inst.Src[1].Register.Absolute = true;
   inst.Src[1].Register.Negate = true;
            /* linelength * 2 - 1 */
   tgsi_transform_op3_swz_inst(ctx, TGSI_OPCODE_MAD,
                              TGSI_FILE_TEMPORARY, aactx->aaTemp,
   TGSI_WRITEMASK_Y,
               /* MIN height alpha */
   tgsi_transform_op2_swz_inst(ctx, TGSI_OPCODE_MIN,
                                          /* MUL width / height alpha */
   tgsi_transform_op2_swz_inst(ctx, TGSI_OPCODE_MUL,
                                          /* MOV rgb */
   tgsi_transform_op1_inst(ctx, TGSI_OPCODE_MOV,
                        /* MUL alpha */
   tgsi_transform_op2_inst(ctx, TGSI_OPCODE_MUL,
                           }
         /**
   * TGSI instruction transform callback.
   * Replace writes to result.color w/ a temp reg.
   */
   static void
   aa_transform_inst(struct tgsi_transform_context *ctx,
         {
      struct aa_transform_context *aactx = (struct aa_transform_context *) ctx;
            /*
   * Look for writes to result.color and replace with colorTemp reg.
   */
   for (i = 0; i < inst->Instruction.NumDstRegs; i++) {
      struct tgsi_full_dst_register *dst = &inst->Dst[i];
   if (dst->Register.File == TGSI_FILE_OUTPUT &&
      dst->Register.Index == aactx->colorOutput) {
   dst->Register.File = TGSI_FILE_TEMPORARY;
                     }
         /**
   * Generate the frag shader we'll use for drawing AA lines.
   * This will be the user's shader plus some arithmetic instructions.
   */
   static bool
   generate_aaline_fs(struct aaline_stage *aaline)
   {
      struct pipe_context *pipe = aaline->stage.draw->pipe;
   const struct pipe_shader_state *orig_fs = &aaline->fs->state;
   struct pipe_shader_state aaline_fs;
   struct aa_transform_context transform;
                     memset(&transform, 0, sizeof(transform));
   transform.colorOutput = -1;
   transform.maxInput = -1;
   transform.maxGeneric = -1;
   transform.colorTemp = -1;
   transform.aaTemp = -1;
   transform.base.prolog = aa_transform_prolog;
   transform.base.epilog = aa_transform_epilog;
   transform.base.transform_instruction = aa_transform_inst;
   transform.base.transform_declaration = aa_transform_decl;
            aaline_fs.tokens = tgsi_transform_shader(orig_fs->tokens, newLen, &transform.base);
   if (!aaline_fs.tokens)
         #if 0 /* DEBUG */
      debug_printf("draw_aaline, orig shader:\n");
   tgsi_dump(orig_fs->tokens, 0);
   debug_printf("draw_aaline, new shader:\n");
      #endif
         aaline->fs->aaline_fs = aaline->driver_create_fs_state(pipe, &aaline_fs);
   if (aaline->fs->aaline_fs != NULL)
            FREE((void *)aaline_fs.tokens);
      }
      static bool
   generate_aaline_fs_nir(struct aaline_stage *aaline)
   {
      struct pipe_context *pipe = aaline->stage.draw->pipe;
   const struct pipe_shader_state *orig_fs = &aaline->fs->state;
            aaline_fs = *orig_fs; /* copy to init */
   aaline_fs.ir.nir = nir_shader_clone(NULL, orig_fs->ir.nir);
   if (!aaline_fs.ir.nir)
            nir_lower_aaline_fs(aaline_fs.ir.nir, &aaline->fs->generic_attrib, NULL, NULL);
   aaline->fs->aaline_fs = aaline->driver_create_fs_state(pipe, &aaline_fs);
   if (aaline->fs->aaline_fs == NULL)
               }
      /**
   * When we're about to draw our first AA line in a batch, this function is
   * called to tell the driver to bind our modified fragment shader.
   */
   static bool
   bind_aaline_fragment_shader(struct aaline_stage *aaline)
   {
      struct draw_context *draw = aaline->stage.draw;
            if (!aaline->fs->aaline_fs) {
      if (aaline->fs->state.type == PIPE_SHADER_IR_NIR) {
      if (!generate_aaline_fs_nir(aaline))
      } else
      if (!generate_aaline_fs(aaline))
            draw->suspend_flushing = true;
   aaline->driver_bind_fs_state(pipe, aaline->fs->aaline_fs);
               }
            static inline struct aaline_stage *
   aaline_stage(struct draw_stage *stage)
   {
         }
         /**
   * Draw a wide line by drawing a quad, using geometry which will
   * fullfill GL's antialiased line requirements.
   */
   static void
   aaline_line(struct draw_stage *stage, struct prim_header *header)
   {
      const struct aaline_stage *aaline = aaline_stage(stage);
   const float half_width = aaline->half_line_width;
   struct prim_header tri;
   struct vertex_header *v[8];
   unsigned coordPos = aaline->coord_slot;
   unsigned posPos = aaline->pos_slot;
   float *pos, *tex;
   float dx = header->v[1]->data[posPos][0] - header->v[0]->data[posPos][0];
   float dy = header->v[1]->data[posPos][1] - header->v[0]->data[posPos][1];
   float length = sqrtf(dx * dx + dy * dy);
   float c_a = dx / length, s_a = dy / length;
   float half_length = 0.5 * length;
   float t_l, t_w;
                     t_w = half_width;
            /* allocate/dup new verts */
   for (i = 0; i < 4; i++) {
                  /*
   * Quad strip for line from v0 to v1 (*=endpoints):
   *
   *  1                             3
   *  +-----------------------------+
   *  |                             |
   *  | *v0                     v1* |
   *  |                             |
   *  +-----------------------------+
   *  0                             2
            /*
   * We increase line length by 0.5 pixels (at each endpoint),
   * and calculate the tri endpoints by moving them half-width
   * distance away perpendicular to the line.
   * XXX: since we change line endpoints (by 0.5 pixel), should
   * actually re-interpolate all other values?
            /* new verts */
   pos = v[0]->data[posPos];
   pos[0] += (-t_l * c_a -  t_w * s_a);
            pos = v[1]->data[posPos];
   pos[0] += (-t_l * c_a - -t_w * s_a);
            pos = v[2]->data[posPos];
   pos[0] += (t_l * c_a -  t_w * s_a);
            pos = v[3]->data[posPos];
   pos[0] += (t_l * c_a - -t_w * s_a);
            /* new texcoords */
   tex = v[0]->data[coordPos];
            tex = v[1]->data[coordPos];
            tex = v[2]->data[coordPos];
            tex = v[3]->data[coordPos];
            tri.v[0] = v[2];  tri.v[1] = v[1];  tri.v[2] = v[0];
            tri.v[0] = v[3];  tri.v[1] = v[1];  tri.v[2] = v[2];
      }
         static void
   aaline_first_line(struct draw_stage *stage, struct prim_header *header)
   {
      auto struct aaline_stage *aaline = aaline_stage(stage);
   struct draw_context *draw = stage->draw;
   struct pipe_context *pipe = draw->pipe;
   const struct pipe_rasterizer_state *rast = draw->rasterizer;
                     if (draw->rasterizer->line_width <= 1.0)
         else
            if (!draw->rasterizer->half_pixel_center)
      /*
   * The tex coords probably would need adjustments?
   */
         /*
   * Bind (generate) our fragprog
   */
   if (!bind_aaline_fragment_shader(aaline)) {
      stage->line = draw_pipe_passthrough_line;
   stage->line(stage, header);
                                 /* Disable triangle culling, stippling, unfilled mode etc. */
   r = draw_get_rasterizer_no_cull(draw, rast);
                     /* now really draw first line */
   stage->line = aaline_line;
      }
         static void
   aaline_flush(struct draw_stage *stage, unsigned flags)
   {
      struct draw_context *draw = stage->draw;
   struct aaline_stage *aaline = aaline_stage(stage);
            stage->line = aaline_first_line;
            /* restore original frag shader */
   draw->suspend_flushing = true;
            /* restore original rasterizer state */
   if (draw->rast_handle) {
                              }
         static void
   aaline_reset_stipple_counter(struct draw_stage *stage)
   {
         }
         static void
   aaline_destroy(struct draw_stage *stage)
   {
      struct aaline_stage *aaline = aaline_stage(stage);
                     /* restore the old entry points */
   pipe->create_fs_state = aaline->driver_create_fs_state;
   pipe->bind_fs_state = aaline->driver_bind_fs_state;
               }
         static struct aaline_stage *
   draw_aaline_stage(struct draw_context *draw)
   {
      struct aaline_stage *aaline = CALLOC_STRUCT(aaline_stage);
   if (!aaline)
            aaline->stage.draw = draw;
   aaline->stage.name = "aaline";
   aaline->stage.next = NULL;
   aaline->stage.point = draw_pipe_passthrough_point;
   aaline->stage.line = aaline_first_line;
   aaline->stage.tri = draw_pipe_passthrough_tri;
   aaline->stage.flush = aaline_flush;
   aaline->stage.reset_stipple_counter = aaline_reset_stipple_counter;
            if (!draw_alloc_temp_verts(&aaline->stage, 8)) {
      aaline->stage.destroy(&aaline->stage);
                  }
         static struct aaline_stage *
   aaline_stage_from_pipe(struct pipe_context *pipe)
   {
               if (draw) {
         } else {
            }
         /**
   * This function overrides the driver's create_fs_state() function and
   * will typically be called by the gallium frontend.
   */
   static void *
   aaline_create_fs_state(struct pipe_context *pipe,
         {
      struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
            if (!aaline)
                     if (!aafs)
            aafs->state.type = fs->type;
   if (fs->type == PIPE_SHADER_IR_TGSI)
         else
            /* pass-through */
               }
         static void
   aaline_bind_fs_state(struct pipe_context *pipe, void *fs)
   {
      struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
            if (!aaline) {
                  /* save current */
   aaline->fs = aafs;
   /* pass-through */
      }
         static void
   aaline_delete_fs_state(struct pipe_context *pipe, void *fs)
   {
      struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
            if (!aafs) {
                  if (aaline) {
      /* pass-through */
            if (aafs->aaline_fs)
               if (aafs->state.type == PIPE_SHADER_IR_TGSI)
         else
            }
         void
   draw_aaline_prepare_outputs(struct draw_context *draw,
         {
      struct aaline_stage *aaline = aaline_stage(stage);
            /* update vertex attrib info */
            if (!rast->line_smooth || rast->multisample)
            /* allocate the extra post-transformed vertex attribute */
   if (aaline->fs && aaline->fs->aaline_fs)
      aaline->coord_slot = draw_alloc_extra_vertex_attrib(draw,
            else
      }
      /**
   * Called by drivers that want to install this AA line prim stage
   * into the draw module's pipeline.  This will not be used if the
   * hardware has native support for AA lines.
   */
   bool
   draw_install_aaline_stage(struct draw_context *draw, struct pipe_context *pipe)
   {
                        /*
   * Create / install AA line drawing / prim stage
   */
   aaline = draw_aaline_stage(draw);
   if (!aaline)
            /* save original driver functions */
   aaline->driver_create_fs_state = pipe->create_fs_state;
   aaline->driver_bind_fs_state = pipe->bind_fs_state;
            /* override the driver's functions */
   pipe->create_fs_state = aaline_create_fs_state;
   pipe->bind_fs_state = aaline_bind_fs_state;
            /* Install once everything is known to be OK:
   */
               }
