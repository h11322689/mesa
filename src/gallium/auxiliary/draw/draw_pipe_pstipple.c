   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
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
   * Polygon stipple stage:  implement polygon stipple with texture map and
   * fragment program.  The fragment program samples the texture using the
   * fragment window coordinate register and does a fragment kill for the
   * stipple-failing fragments.
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
   #include "util/u_pstipple.h"
   #include "util/u_sampler.h"
      #include "tgsi/tgsi_parse.h"
      #include "draw_context.h"
   #include "draw_pipe.h"
      #include "nir.h"
   #include "nir/nir_draw_helpers.h"
      /** Approx number of new tokens for instructions in pstip_transform_inst() */
   #define NUM_NEW_TOKENS 53
         /**
   * Subclass of pipe_shader_state to carry extra fragment shader info.
   */
   struct pstip_fragment_shader
   {
      struct pipe_shader_state state;
   void *driver_fs;
   void *pstip_fs;
      };
         /**
   * Subclass of draw_stage
   */
   struct pstip_stage
   {
               void *sampler_cso;
   struct pipe_resource *texture;
   struct pipe_sampler_view *sampler_view;
   unsigned num_samplers;
            /*
   * Currently bound state
   */
   struct pstip_fragment_shader *fs;
   struct {
      void *samplers[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_view *sampler_views[PIPE_MAX_SHADER_SAMPLER_VIEWS];
               /*
   * Driver interface/override functions
   */
   void * (*driver_create_fs_state)(struct pipe_context *,
         void (*driver_bind_fs_state)(struct pipe_context *, void *);
            void (*driver_bind_sampler_states)(struct pipe_context *,
                  void (*driver_set_sampler_views)(struct pipe_context *,
                                    void (*driver_set_polygon_stipple)(struct pipe_context *,
               };
         /**
   * Generate the frag shader we'll use for doing polygon stipple.
   * This will be the user's shader prefixed with a TEX and KIL instruction.
   */
   static bool
   generate_pstip_fs(struct pstip_stage *pstip)
   {
      struct pipe_context *pipe = pstip->pipe;
   struct pipe_screen *screen = pipe->screen;
   const struct pipe_shader_state *orig_fs = &pstip->fs->state;
   /*struct draw_context *draw = pstip->stage.draw;*/
   struct pipe_shader_state pstip_fs;
            wincoord_file = screen->get_param(screen, PIPE_CAP_FS_POSITION_IS_SYSVAL) ?
            pstip_fs = *orig_fs; /* copy to init */
   if (orig_fs->type == PIPE_SHADER_IR_TGSI) {
      pstip_fs.tokens = util_pstipple_create_fragment_shader(orig_fs->tokens,
                     if (pstip_fs.tokens == NULL)
      } else {
      pstip_fs.ir.nir = nir_shader_clone(NULL, orig_fs->ir.nir);
   nir_lower_pstipple_fs(pstip_fs.ir.nir,
                                                if (!pstip->fs->pstip_fs)
               }
         /**
   * When we're about to draw our first stipple polygon in a batch, this function
   * is called to tell the driver to bind our modified fragment shader.
   */
   static bool
   bind_pstip_fragment_shader(struct pstip_stage *pstip)
   {
      struct draw_context *draw = pstip->stage.draw;
   if (!pstip->fs->pstip_fs &&
      !generate_pstip_fs(pstip))
         draw->suspend_flushing = true;
   pstip->driver_bind_fs_state(pstip->pipe, pstip->fs->pstip_fs);
   draw->suspend_flushing = false;
      }
         static inline struct pstip_stage *
   pstip_stage(struct draw_stage *stage)
   {
         }
         static void
   pstip_first_tri(struct draw_stage *stage, struct prim_header *header)
   {
      struct pstip_stage *pstip = pstip_stage(stage);
   struct pipe_context *pipe = pstip->pipe;
   struct draw_context *draw = stage->draw;
   unsigned num_samplers;
                     /* bind our fragprog */
   if (!bind_pstip_fragment_shader(pstip)) {
      stage->tri = draw_pipe_passthrough_tri;
   stage->tri(stage, header);
               /* how many samplers? */
   /* we'll use sampler/texture[pstip->sampler_unit] for the stipple */
   num_samplers = MAX2(pstip->num_samplers, pstip->fs->sampler_unit + 1);
            /* plug in our sampler, texture */
   pstip->state.samplers[pstip->fs->sampler_unit] = pstip->sampler_cso;
   pipe_sampler_view_reference(&pstip->state.sampler_views[pstip->fs->sampler_unit],
                              pstip->driver_bind_sampler_states(pipe, PIPE_SHADER_FRAGMENT, 0,
            pstip->driver_set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0,
                           /* now really draw first triangle */
   stage->tri = draw_pipe_passthrough_tri;
      }
         static void
   pstip_flush(struct draw_stage *stage, unsigned flags)
   {
      struct draw_context *draw = stage->draw;
   struct pstip_stage *pstip = pstip_stage(stage);
            stage->tri = pstip_first_tri;
            /* restore original frag shader, texture, sampler state */
   draw->suspend_flushing = true;
            pstip->driver_bind_sampler_states(pipe, PIPE_SHADER_FRAGMENT, 0,
                  pstip->driver_set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0,
                     }
         static void
   pstip_reset_stipple_counter(struct draw_stage *stage)
   {
         }
         static void
   pstip_destroy(struct draw_stage *stage)
   {
               for (unsigned i = 0; i < PIPE_MAX_SHADER_SAMPLER_VIEWS; i++) {
                                    if (pstip->sampler_view) {
                  draw_free_temp_verts(stage);
      }
         /** Create a new polygon stipple drawing stage object */
   static struct pstip_stage *
   draw_pstip_stage(struct draw_context *draw, struct pipe_context *pipe)
   {
      struct pstip_stage *pstip = CALLOC_STRUCT(pstip_stage);
   if (!pstip)
                     pstip->stage.draw = draw;
   pstip->stage.name = "pstip";
   pstip->stage.next = NULL;
   pstip->stage.point = draw_pipe_passthrough_point;
   pstip->stage.line = draw_pipe_passthrough_line;
   pstip->stage.tri = pstip_first_tri;
   pstip->stage.flush = pstip_flush;
   pstip->stage.reset_stipple_counter = pstip_reset_stipple_counter;
            if (!draw_alloc_temp_verts(&pstip->stage, 8))
                  fail:
      if (pstip)
               }
         static struct pstip_stage *
   pstip_stage_from_pipe(struct pipe_context *pipe)
   {
      struct draw_context *draw = (struct draw_context *) pipe->draw;
      }
         /**
   * This function overrides the driver's create_fs_state() function and
   * will typically be called by the gallium frontend.
   */
   static void *
   pstip_create_fs_state(struct pipe_context *pipe,
         {
      struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
            if (pstipfs) {
      pstipfs->state.type = fs->type;
   if (fs->type == PIPE_SHADER_IR_TGSI)
         else
            /* pass-through */
                  }
         static void
   pstip_bind_fs_state(struct pipe_context *pipe, void *fs)
   {
      struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   struct pstip_fragment_shader *pstipfs = (struct pstip_fragment_shader *) fs;
   /* save current */
   pstip->fs = pstipfs;
   /* pass-through */
   pstip->driver_bind_fs_state(pstip->pipe,
      }
         static void
   pstip_delete_fs_state(struct pipe_context *pipe, void *fs)
   {
      struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   struct pstip_fragment_shader *pstipfs = (struct pstip_fragment_shader *) fs;
   /* pass-through */
            if (pstipfs->pstip_fs)
            if (pstipfs->state.type == PIPE_SHADER_IR_TGSI)
         else
            }
         static void
   pstip_bind_sampler_states(struct pipe_context *pipe,
               {
                        if (shader == PIPE_SHADER_FRAGMENT) {
      /* save current */
   memcpy(pstip->state.samplers, sampler, num * sizeof(void *));
   for (unsigned i = num; i < PIPE_MAX_SAMPLERS; i++) {
         }
               /* pass-through */
      }
         static void
   pstip_set_sampler_views(struct pipe_context *pipe,
                           enum pipe_shader_type shader,
   {
               if (shader == PIPE_SHADER_FRAGMENT) {
      /* save current */
   unsigned i;
   for (i = 0; i < num; i++) {
      pipe_sampler_view_reference(&pstip->state.sampler_views[start + i],
      }
   for (; i < num + unbind_num_trailing_slots; i++) {
      pipe_sampler_view_reference(&pstip->state.sampler_views[start + i],
      }
               /* pass-through */
   pstip->driver_set_sampler_views(pstip->pipe, shader, start, num,
      }
         static void
   pstip_set_polygon_stipple(struct pipe_context *pipe,
         {
               /* save current */
            /* pass-through */
            util_pstipple_update_stipple_texture(pstip->pipe, pstip->texture,
      }
         /**
   * Called by drivers that want to install this polygon stipple stage
   * into the draw module's pipeline.  This will not be used if the
   * hardware has native support for polygon stipple.
   */
   bool
   draw_install_pstipple_stage(struct draw_context *draw,
         {
               /*
   * Create / install pgon stipple drawing / prim stage
   */
   struct pstip_stage *pstip = draw_pstip_stage(draw, pipe);
   if (!pstip)
                     /* save original driver functions */
   pstip->driver_create_fs_state = pipe->create_fs_state;
   pstip->driver_bind_fs_state = pipe->bind_fs_state;
            pstip->driver_bind_sampler_states = pipe->bind_sampler_states;
   pstip->driver_set_sampler_views = pipe->set_sampler_views;
            /* create special texture, sampler state */
   pstip->texture = util_pstipple_create_stipple_texture(pipe, NULL);
   if (!pstip->texture)
            pstip->sampler_view = util_pstipple_create_sampler_view(pipe,
         if (!pstip->sampler_view)
            pstip->sampler_cso = util_pstipple_create_sampler(pipe);
   if (!pstip->sampler_cso)
            /* override the driver's functions */
   pipe->create_fs_state = pstip_create_fs_state;
   pipe->bind_fs_state = pstip_bind_fs_state;
            pipe->bind_sampler_states = pstip_bind_sampler_states;
   pipe->set_sampler_views = pstip_set_sampler_views;
                  fail:
      if (pstip)
               }
