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
   * Tiling engine.
   *
   * Builds per-tile display lists and executes them on calls to
   * lp_setup_flush().
   */
      #include <limits.h>
      #include "pipe/p_defines.h"
   #include "util/u_framebuffer.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_pack_color.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_viewport.h"
   #include "draw/draw_pipe.h"
   #include "util/os_time.h"
   #include "lp_context.h"
   #include "lp_memory.h"
   #include "lp_scene.h"
   #include "lp_texture.h"
   #include "lp_debug.h"
   #include "lp_fence.h"
   #include "lp_query.h"
   #include "lp_rast.h"
   #include "lp_setup_context.h"
   #include "lp_screen.h"
   #include "lp_state.h"
   #include "lp_jit.h"
   #include "frontend/sw_winsys.h"
      #include "draw/draw_context.h"
   #include "draw/draw_vbuf.h"
         static bool
   try_update_scene_state(struct lp_setup_context *setup);
         static unsigned
   lp_setup_wait_empty_scene(struct lp_setup_context *setup)
   {
      /* just use the first scene if we run out */
   if (setup->scenes[0]->fence) {
      lp_fence_wait(setup->scenes[0]->fence);
      }
      }
         static void
   lp_setup_get_empty_scene(struct lp_setup_context *setup)
   {
      assert(setup->scene == NULL);
            /* try and find a scene that isn't being used */
   for (i = 0; i < setup->num_active_scenes; i++) {
      if (setup->scenes[i]->fence) {
      if (lp_fence_signalled(setup->scenes[i]->fence)) {
      lp_scene_end_rasterization(setup->scenes[i]);
         } else {
                     if (setup->num_active_scenes + 1 > MAX_SCENES) {
         } else if (i == setup->num_active_scenes) {
      /* allocate a new scene */
   struct lp_scene *scene = lp_scene_create(setup);
   if (!scene) {
      /* block and reuse scenes */
      } else {
      LP_DBG(DEBUG_SETUP, "allocated scene: %d\n", setup->num_active_scenes);
   setup->scenes[setup->num_active_scenes] = scene;
   i = setup->num_active_scenes;
                  setup->scene = setup->scenes[i];
   setup->scene->permit_linear_rasterizer = setup->permit_linear_rasterizer;
      }
         static void
   first_triangle(struct lp_setup_context *setup,
                     {
      assert(setup->state == SETUP_ACTIVE);
   lp_setup_choose_triangle(setup);
      }
         static bool
   first_rectangle(struct lp_setup_context *setup,
                  const float (*v0)[4],
   const float (*v1)[4],
   const float (*v2)[4],
      {
      assert(setup->state == SETUP_ACTIVE);
   lp_setup_choose_rect(setup);
      }
         static void
   first_line(struct lp_setup_context *setup,
               {
      assert(setup->state == SETUP_ACTIVE);
   lp_setup_choose_line(setup);
      }
         static void
   first_point(struct lp_setup_context *setup,
         {
      assert(setup->state == SETUP_ACTIVE);
   lp_setup_choose_point(setup);
      }
         void
   lp_setup_reset(struct lp_setup_context *setup)
   {
               /* Reset derived state */
   for (unsigned i = 0; i < ARRAY_SIZE(setup->constants); ++i) {
      setup->constants[i].stored_size = 0;
               setup->fs.stored = NULL;
            /* no current bin */
            /* Reset some state:
   */
            /* Have an explicit "start-binning" call and get rid of this
   * pointer twiddling?
   */
   setup->line = first_line;
   setup->point = first_point;
   setup->triangle = first_triangle;
      }
         /** Rasterize all scene's bins */
   static void
   lp_setup_rasterize_scene(struct lp_setup_context *setup)
   {
      struct lp_scene *scene = setup->scene;
            scene->num_active_queries = setup->active_binned_queries;
   memcpy(scene->active_queries, setup->active_queries,
                     mtx_lock(&screen->rast_mutex);
   lp_rast_queue_scene(screen->rast, scene);
                        }
         static bool
   begin_binning(struct lp_setup_context *setup)
   {
               assert(scene);
            /* Always create a fence:
   */
   scene->fence = lp_fence_create(MAX2(1, setup->num_threads));
   if (!scene->fence)
            if (!try_update_scene_state(setup)) {
                  bool need_zsload = false;
   if (setup->fb.zsbuf &&
      ((setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL) != PIPE_CLEAR_DEPTHSTENCIL) &&
   util_format_is_depth_and_stencil(setup->fb.zsbuf->format)) {
               LP_DBG(DEBUG_SETUP, "%s color clear bufs: %x depth: %s\n", __func__,
                  if (setup->clear.flags & PIPE_CLEAR_COLOR) {
      for (unsigned cbuf = 0; cbuf < setup->fb.nr_cbufs; cbuf++) {
      assert(PIPE_CLEAR_COLOR0 == 1 << 2);
   if (setup->clear.flags & (1 << (2 + cbuf))) {
      union lp_rast_cmd_arg clearrb_arg;
   struct lp_rast_clear_rb *cc_scene =
                  if (!cc_scene) {
                  cc_scene->cbuf = cbuf;
                  if (!lp_scene_bin_everywhere(scene,
                                       if (setup->fb.zsbuf) {
      if (setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL) {
      if (!lp_scene_bin_everywhere(scene,
                                                setup->clear.flags = 0;
   setup->clear.zsmask = 0;
                     LP_DBG(DEBUG_SETUP, "%s done\n", __func__);
      }
         /* This basically bins and then flushes any outstanding full-screen
   * clears.
   *
   * TODO: fast path for fullscreen clears and no triangles.
   */
   static bool
   execute_clears(struct lp_setup_context *setup)
   {
                  }
         static const char *states[] = {
      "FLUSHED",
   "CLEARED",
      };
         static bool
   set_scene_state(struct lp_setup_context *setup,
               {
               if (old_state == new_state)
            if (LP_DEBUG & DEBUG_SCENE) {
      debug_printf("%s old %s new %s%s%s\n",
               __func__,
   states[old_state],
            if (new_state == SETUP_FLUSHED && setup->scene)
               /* wait for a free/empty scene
   */
   if (old_state == SETUP_FLUSHED)
            switch (new_state) {
   case SETUP_CLEARED:
         case SETUP_ACTIVE:
      if (!begin_binning(setup))
            case SETUP_FLUSHED:
      if (old_state == SETUP_CLEARED)
      if (!execute_clears(setup))
      lp_setup_rasterize_scene(setup);
   assert(setup->scene == NULL);
      default:
      assert(0 && "invalid setup state mode");
               setup->state = new_state;
         fail:
      if (setup->scene) {
      lp_scene_end_rasterization(setup->scene);
               setup->state = SETUP_FLUSHED;
   lp_setup_reset(setup);
      }
         void
   lp_setup_flush(struct lp_setup_context *setup,
         {
         }
         void
   lp_setup_bind_framebuffer(struct lp_setup_context *setup,
         {
               /* Flush any old scene.
   */
            /*
   * Ensure the old scene is not reused.
   */
            /* Set new state.  This will be picked up later when we next need a
   * scene.
   */
   util_copy_framebuffer_state(&setup->fb, fb);
   setup->framebuffer.x0 = 0;
   setup->framebuffer.y0 = 0;
   setup->framebuffer.x1 = fb->width-1;
   setup->framebuffer.y1 = fb->height-1;
   setup->viewport_index_slot = -1;
      }
         /*
   * Try to clear one color buffer of the attached fb, either by binning a clear
   * command or queuing up the clear for later (when binning is started).
   */
   static bool
   lp_setup_try_clear_color_buffer(struct lp_setup_context *setup,
               {
      union lp_rast_cmd_arg clearrb_arg;
   union util_color uc;
                              if (setup->state == SETUP_ACTIVE) {
               /* Add the clear to existing scene.  In the unusual case where
   * both color and depth-stencil are being cleared when there's
   * already been some rendering, we could discard the currently
   * binned scene and start again, but I don't see that as being
   * a common usage.
   */
   struct lp_rast_clear_rb *cc_scene =
                  if (!cc_scene) {
                  cc_scene->cbuf = cbuf;
   cc_scene->color_val = uc;
            if (!lp_scene_bin_everywhere(scene,
                        } else {
      /* Put ourselves into the 'pre-clear' state, specifically to try
   * and accumulate multiple clears to color and depth_stencil
   * buffers which the app or gallium frontend might issue
   * separately.
   */
            assert(PIPE_CLEAR_COLOR0 == (1 << 2));
   setup->clear.flags |= 1 << (cbuf + 2);
                  }
         static bool
   lp_setup_try_clear_zs(struct lp_setup_context *setup,
                     {
                        const uint32_t zmask32 = (flags & PIPE_CLEAR_DEPTH) ? ~0 : 0;
            uint64_t zsvalue = util_pack64_z_stencil(format, depth, stencil);
                     if (format == PIPE_FORMAT_Z24X8_UNORM ||
      format == PIPE_FORMAT_X8Z24_UNORM) {
   /*
   * Make full mask if there's "X" bits so we can do full
   * clear (without rmw).
   */
   uint32_t zsmask_full = util_pack_mask_z_stencil(format, ~0, ~0);
               if (setup->state == SETUP_ACTIVE) {
               /* Add the clear to existing scene.  In the unusual case where
   * both color and depth-stencil are being cleared when there's
   * already been some rendering, we could discard the currently
   * binned scene and start again, but I don't see that as being
   * a common usage.
   */
   if (!lp_scene_bin_everywhere(scene,
                  } else {
      /* Put ourselves into the 'pre-clear' state, specifically to try
   * and accumulate multiple clears to color and depth_stencil
   * buffers which the app or gallium frontend might issue
   * separately.
   */
                     setup->clear.zsmask |= zsmask;
   setup->clear.zsvalue =
                  }
         void
   lp_setup_clear(struct lp_setup_context *setup,
                  const union pipe_color_union *color,
      {
      /*
   * Note any of these (max 9) clears could fail (but at most there should
   * be just one failure!). This avoids doing the previous succeeded
   * clears again (we still clear tiles twice if a clear command succeeded
   * partially for one buffer).
   */
   if (flags & PIPE_CLEAR_DEPTHSTENCIL) {
      unsigned flagszs = flags & PIPE_CLEAR_DEPTHSTENCIL;
   if (!lp_setup_try_clear_zs(setup, depth, stencil, flagszs)) {
               if (!lp_setup_try_clear_zs(setup, depth, stencil, flagszs))
                  if (flags & PIPE_CLEAR_COLOR) {
      assert(PIPE_CLEAR_COLOR0 == (1 << 2));
   for (unsigned i = 0; i < setup->fb.nr_cbufs; i++) {
      if ((flags & (1 << (2 + i))) && setup->fb.cbufs[i]) {
                        if (!lp_setup_try_clear_color_buffer(setup, color, i))
                  }
         void
   lp_setup_bind_rasterizer(struct lp_setup_context *setup,
         {
               setup->ccw_is_frontface = rast->front_ccw;
   setup->cullmode = rast->cull_face;
   setup->triangle = first_triangle;
   setup->rect = first_rectangle;
   setup->multisample = rast->multisample;
   setup->pixel_offset = rast->half_pixel_center ? 0.5f : 0.0f;
            if (setup->scissor_test != rast->scissor) {
      setup->dirty |= LP_SETUP_NEW_SCISSOR;
               setup->flatshade_first = rast->flatshade_first;
   setup->line_width = rast->line_width;
            setup->point_size = rast->point_size;
   setup->sprite_coord_enable = rast->sprite_coord_enable;
   setup->sprite_coord_origin = rast->sprite_coord_mode;
   setup->point_line_tri_clip = rast->point_line_tri_clip;
   setup->point_size_per_vertex = rast->point_size_per_vertex;
      }
         void
   lp_setup_set_setup_variant(struct lp_setup_context *setup,
         {
                  }
         void
   lp_setup_set_fs_variant(struct lp_setup_context *setup,
         {
               setup->fs.current.variant = variant;
      }
         void
   lp_setup_set_fs_constants(struct lp_setup_context *setup,
               {
                        unsigned i;
   for (i = 0; i < num; ++i) {
      util_copy_constant_buffer(&setup->constants[i].current,
      }
   for (; i < ARRAY_SIZE(setup->constants); i++) {
         }
      }
         void
   lp_setup_set_fs_ssbos(struct lp_setup_context *setup,
                     {
                        unsigned i;
   for (i = 0; i < num; ++i) {
         }
   for (; i < ARRAY_SIZE(setup->ssbos); i++) {
         }
   setup->ssbo_write_mask = ssbo_write_mask;
      }
         void
   lp_setup_set_fs_images(struct lp_setup_context *setup,
               {
                                 for (i = 0; i < num; ++i) {
      const struct pipe_image_view *image = &images[i];
            struct pipe_resource *res = image->resource;
            if (!res)
               }
   for (; i < ARRAY_SIZE(setup->images); i++) {
         }
      }
         void
   lp_setup_set_alpha_ref_value(struct lp_setup_context *setup,
         {
               if (setup->fs.current.jit_context.alpha_ref_value != alpha_ref_value) {
      setup->fs.current.jit_context.alpha_ref_value = alpha_ref_value;
         }
         void
   lp_setup_set_stencil_ref_values(struct lp_setup_context *setup,
         {
               if (setup->fs.current.jit_context.stencil_ref_front != refs[0] ||
      setup->fs.current.jit_context.stencil_ref_back != refs[1]) {
   setup->fs.current.jit_context.stencil_ref_front = refs[0];
   setup->fs.current.jit_context.stencil_ref_back = refs[1];
         }
         void
   lp_setup_set_blend_color(struct lp_setup_context *setup,
         {
                        if (memcmp(&setup->blend_color.current,
            memcpy(&setup->blend_color.current, blend_color, sizeof *blend_color);
         }
         void
   lp_setup_set_scissors(struct lp_setup_context *setup,
         {
                        for (unsigned i = 0; i < PIPE_MAX_VIEWPORTS; ++i) {
      setup->scissors[i].x0 = scissors[i].minx;
   setup->scissors[i].x1 = scissors[i].maxx-1;
   setup->scissors[i].y0 = scissors[i].miny;
      }
      }
         void
   lp_setup_set_sample_mask(struct lp_setup_context *setup,
         {
      if (setup->fs.current.jit_context.sample_mask != sample_mask) {
      setup->fs.current.jit_context.sample_mask = sample_mask;
         }
         void
   lp_setup_set_rasterizer_discard(struct lp_setup_context *setup,
         {
      if (setup->rasterizer_discard != rasterizer_discard) {
      setup->rasterizer_discard = rasterizer_discard;
   setup->line = first_line;
   setup->point = first_point;
   setup->triangle = first_triangle;
         }
         void
   lp_setup_set_vertex_info(struct lp_setup_context *setup,
         {
      /* XXX: just silently holding onto the pointer:
   */
      }
         void
   lp_setup_set_linear_mode(struct lp_setup_context *setup,
         {
      /* The linear rasterizer requires sse2 both at compile and runtime,
   * in particular for the code in lp_rast_linear_fallback.c.  This
   * is more than ten-year-old technology, so it's a reasonable
   * baseline.
      #if DETECT_ARCH_SSE
      setup->permit_linear_rasterizer = (mode &&
      #else
         #endif
   }
         /**
   * Called during state validation when LP_NEW_VIEWPORT is set.
   */
   void
   lp_setup_set_viewports(struct lp_setup_context *setup,
               {
                        assert(num_viewports <= PIPE_MAX_VIEWPORTS);
            /*
   * Linear rasterizer path for scissor/viewport intersection.
   *
   * Calculate "scissor" rect from the (first) viewport.
   * Just like stored scissor rects need inclusive coords.
   * For rounding, assume half pixel center (d3d9 should not end up
   * with fractional viewports) - quite obviously for msaa we'd need
   * fractional values here (and elsewhere for the point bounding box).
   *
   * See: lp_setup.c::try_update_scene_state
   */
   const float half_height = fabsf(viewports[0].scale[1]);
   const float x0 = viewports[0].translate[0] - viewports[0].scale[0];
            setup->vpwh.x0 = (int)(x0 + 0.499f);
   setup->vpwh.x1 = (int)(viewports[0].scale[0] * 2.0f + x0 - 0.501f);
   setup->vpwh.y0 = (int)(y0 + 0.499f);
   setup->vpwh.y1 = (int)(half_height * 2.0f + y0 - 0.501f);
            /*
   * For use in lp_state_fs.c, propagate the viewport values for all viewports.
   */
   for (unsigned i = 0; i < num_viewports; i++) {
      float min_depth, max_depth;
   util_viewport_zmin_zmax(&viewports[i], lp->rasterizer->clip_halfz,
            if (setup->viewports[i].min_depth != min_depth ||
      setup->viewports[i].max_depth != max_depth) {
   setup->viewports[i].min_depth = min_depth;
   setup->viewports[i].max_depth = max_depth;
            }
         /**
   * Called directly by llvmpipe_set_sampler_views
   */
   void
   lp_setup_set_fragment_sampler_views(struct lp_setup_context *setup,
               {
                                 for (unsigned i = 0; i < max_tex_num; i++) {
               /* We are going to overwrite/unref the current texture further below. If
   * set, make sure to unmap its resource to avoid leaking previous
   * mapping.  */
   if (setup->fs.current_tex[i])
            if (view) {
      struct pipe_resource *res = view->texture;
                  /* We're referencing the texture's internal data, so save a
   * reference to it.
                     } else {
            }
               }
         /**
   * Called during state validation when LP_NEW_SAMPLER is set.
   */
   void
   lp_setup_set_fragment_sampler_state(struct lp_setup_context *setup,
               {
                        for (unsigned i = 0; i < PIPE_MAX_SAMPLERS; i++) {
               if (sampler) {
                                       }
         /**
   * Is the given texture referenced by any scene?
   * Note: we have to check all scenes including any scenes currently
   * being rendered and the current scene being built.
   */
   unsigned
   lp_setup_is_resource_referenced(const struct lp_setup_context *setup,
         {
      /* check the render targets */
   for (unsigned i = 0; i < setup->fb.nr_cbufs; i++) {
      if (setup->fb.cbufs[i] && setup->fb.cbufs[i]->texture == texture)
      }
   if (setup->fb.zsbuf && setup->fb.zsbuf->texture == texture) {
                  /* check resources referenced by active scenes */
   for (unsigned i = 0; i < setup->num_active_scenes; i++) {
               mtx_lock(&scene->mutex);
   unsigned ref = lp_scene_is_resource_referenced(scene, texture);
   mtx_unlock(&scene->mutex);
   if (ref)
                  }
         /**
   * Called by vbuf code when we're about to draw something.
   *
   * This function stores all dirty state in the current scene's display list
   * memory, via lp_scene_alloc().  We can not pass pointers of mutable state to
   * the JIT functions, as the JIT functions will be called later on, most likely
   * on a different thread.
   *
   * When processing dirty state it is imperative that we don't refer to any
   * pointers previously allocated with lp_scene_alloc() in this function (or any
   * function) as they may belong to a scene freed since then.
   */
   static bool
   try_update_scene_state(struct lp_setup_context *setup)
   {
      bool new_scene = (setup->fs.stored == NULL);
                     if (setup->dirty & LP_SETUP_NEW_VIEWPORTS) {
      /*
   * Record new depth range state for changes due to viewport updates.
   *
   * TODO: Collapse the existing viewport and depth range information
   *       into one structure, for access by JIT.
   */
            stored = (struct lp_jit_viewport *)
            if (!stored) {
      assert(!new_scene);
                        setup->fs.current.jit_context.viewports = stored;
               if (setup->dirty & LP_SETUP_NEW_BLEND_COLOR) {
      /* Alloc u8_blend_color (16 x i8) and f_blend_color (4 or 8 x f32) */
   const unsigned size = 4 * 16 * sizeof(uint8_t)
            uint8_t *stored =
            if (!stored) {
      assert(!new_scene);
               /* Store floating point colour (after ubyte colors (see below)) */
   float *fstored = (float *) (stored + 4 * 16);
   for (unsigned i = 0; i < (LP_MAX_VECTOR_LENGTH / 4); ++i) {
                  /* smear each blend color component across 16 ubyte elements */
   for (unsigned i = 0; i < 4; ++i) {
      uint8_t c = float_to_ubyte(setup->blend_color.current.color[i]);
   for (unsigned j = 0; j < 16; ++j) {
                     setup->blend_color.stored = stored;
   setup->fs.current.jit_context.u8_blend_color = stored;
   setup->fs.current.jit_context.f_blend_color = fstored;
               struct llvmpipe_context *llvmpipe = llvmpipe_context(setup->pipe);
   if (llvmpipe->dirty & LP_NEW_FS_CONSTANTS)
      lp_setup_set_fs_constants(llvmpipe->setup,
               if (setup->dirty & LP_SETUP_NEW_CONSTANTS) {
      for (unsigned i = 0; i < ARRAY_SIZE(setup->constants); ++i) {
      lp_jit_buffer_from_pipe_const(&setup->fs.current.jit_resources.constants[i],
         if (setup->constants[i].current.buffer &&
      !lp_scene_add_resource_reference(scene,
               assert(!new_scene);
      }
                  if (setup->dirty & LP_SETUP_NEW_SSBOS) {
      for (unsigned i = 0; i < ARRAY_SIZE(setup->ssbos); ++i) {
      lp_jit_buffer_from_pipe(&setup->fs.current.jit_resources.ssbos[i],
                        if (setup->dirty & LP_SETUP_NEW_FS) {
      if (!setup->fs.stored ||
      memcmp(setup->fs.stored,
         &setup->fs.current,
   /* The fs state that's been stored in the scene is different from
   * the new, current state.  So allocate a new lp_rast_state object
   * and append it to the bin's setup data buffer.
   */
   struct lp_rast_state *stored =
         if (!stored) {
      assert(!new_scene);
               memcpy(&stored->jit_context,
         &setup->fs.current.jit_context,
   memcpy(&stored->jit_resources,
                  stored->jit_resources.aniso_filter_table =
                  if (!lp_scene_add_frag_shader_reference(scene,
                                 /* The scene now references the textures in the rasterization
   * state record.  Note that now.
   */
   for (unsigned i = 0; i < ARRAY_SIZE(setup->fs.current_tex); i++) {
      if (setup->fs.current_tex[i]) {
      if (!lp_scene_add_resource_reference(scene,
                  assert(!new_scene);
                     for (unsigned i = 0; i < ARRAY_SIZE(setup->ssbos); i++) {
      if (setup->ssbos[i].current.buffer) {
      if (!lp_scene_add_resource_reference(scene,
                  assert(!new_scene);
                     for (unsigned i = 0; i < ARRAY_SIZE(setup->images); i++) {
      if (setup->images[i].current.resource) {
      if (!lp_scene_add_resource_reference(scene,
                        assert(!new_scene);
                           if (setup->dirty & LP_SETUP_NEW_SCISSOR) {
      for (unsigned i = 0; i < PIPE_MAX_VIEWPORTS; ++i) {
      setup->draw_regions[i] = setup->framebuffer;
   if (setup->scissor_test) {
      u_rect_possible_intersection(&setup->scissors[i],
         }
   if (setup->permit_linear_rasterizer) {
      /* NOTE: this only takes first vp into account. */
   bool need_vp_scissoring =
                  assert(setup->viewport_index_slot < 0);
   if (need_vp_scissoring) {
      u_rect_possible_intersection(&setup->vpwh,
         } else if (setup->point_line_tri_clip) {
      /*
   * for d3d-style point clipping, we're going to need
   * the fake vp scissor too. Hence do the intersection with vp,
   * but don't indicate this. As above this will only work for first vp
   * which should be ok because we instruct draw to only skip point
   * clipping when there's only one viewport (this works because d3d10
   * points are always single pixel).
   * (Also note that if we have permit_linear_rasterizer this will
   * cause large points to always get vp scissored, regardless the
   * point_line_tri_clip setting.)
   */
   bool need_vp_scissoring =
      !!memcmp(&setup->vpwh, &setup->framebuffer,
      if (need_vp_scissoring) {
      u_rect_possible_intersection(&setup->vpwh,
                              assert(setup->fs.stored);
      }
         bool
   lp_setup_update_state(struct lp_setup_context *setup,
         {
      /* Some of the 'draw' pipeline stages may have changed some driver state.
   * Make sure we've processed those state changes before anything else.
   *
   * XXX this is the only place where llvmpipe_context is used in the
   * setup code.  This may get refactored/changed...
   */
   {
      struct llvmpipe_context *lp = llvmpipe_context(setup->pipe);
   if (lp->dirty) {
                  if (lp->setup->dirty) {
                           /* Will probably need to move this somewhere else, just need
   * to know about vertex shader point size attribute.
   */
   setup->psize_slot = lp->psize_slot;
   setup->viewport_index_slot = lp->viewport_index_slot;
   setup->layer_slot = lp->layer_slot;
                     assert(lp->setup_variant.key.size ==
            assert(memcmp(&lp->setup_variant.key,
                     if (update_scene && setup->state != SETUP_ACTIVE) {
      if (!set_scene_state(setup, SETUP_ACTIVE, __func__))
               /* Only call into update_scene_state() if we already have a
   * scene:
   */
   if (update_scene && setup->scene) {
               if (try_update_scene_state(setup))
            /* Update failed, try to restart the scene.
   *
   * Cannot call lp_setup_flush_and_restart() directly here
   * because of potential recursion.
   */
   if (!set_scene_state(setup, SETUP_FLUSHED, __func__))
            if (!set_scene_state(setup, SETUP_ACTIVE, __func__))
            if (!setup->scene)
                           }
            /* Only caller is lp_setup_vbuf_destroy()
   */
   void
   lp_setup_destroy(struct lp_setup_context *setup)
   {
                        for (unsigned i = 0; i < ARRAY_SIZE(setup->fs.current_tex); i++) {
      struct pipe_resource **res_ptr = &setup->fs.current_tex[i];
   if (*res_ptr)
                     for (unsigned i = 0; i < ARRAY_SIZE(setup->constants); i++) {
                  for (unsigned i = 0; i < ARRAY_SIZE(setup->ssbos); i++) {
                  for (unsigned i = 0; i < ARRAY_SIZE(setup->images); i++) {
                  /* free the scenes in the 'empty' queue */
   for (unsigned i = 0; i < setup->num_active_scenes; i++) {
               if (scene->fence)
                        LP_DBG(DEBUG_SETUP, "number of scenes used: %d\n", setup->num_active_scenes);
               }
         /**
   * Create a new primitive tiling engine.  Plug it into the backend of
   * the draw module.  Currently also creates a rasterizer to use with
   * it.
   */
   struct lp_setup_context *
   lp_setup_create(struct pipe_context *pipe,
         {
      struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   struct lp_setup_context *setup = CALLOC_STRUCT(lp_setup_context);
   if (!setup) {
                           setup->psize_slot = -1;
   setup->viewport_index_slot = -1;
   setup->layer_slot = -1;
            /* Used only in update_state():
   */
            setup->num_threads = screen->num_threads;
   setup->vbuf = draw_vbuf_stage(draw, &setup->base);
   if (!setup->vbuf) {
                  draw_set_rasterize_stage(draw, setup->vbuf);
            slab_create(&setup->scene_slab,
               /* create just one scene for starting point */
   setup->scenes[0] = lp_scene_create(setup);
   if (!setup->scenes[0]) {
         }
            setup->triangle = first_triangle;
   setup->line     = first_line;
                     /* Initialize empty default fb correctly, so the rect is empty */
   setup->framebuffer.x1 = -1;
                  no_scenes:
      for (unsigned i = 0; i < MAX_SCENES; i++) {
      if (setup->scenes[i]) {
                        no_vbuf:
         no_setup:
         }
         /**
   * Put a BeginQuery command into all bins.
   */
   void
   lp_setup_begin_query(struct lp_setup_context *setup,
         {
               if (!(pq->type == PIPE_QUERY_OCCLUSION_COUNTER ||
         pq->type == PIPE_QUERY_OCCLUSION_PREDICATE ||
   pq->type == PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE ||
   pq->type == PIPE_QUERY_PIPELINE_STATISTICS ||
            /* init the query to its beginning state */
   assert(setup->active_binned_queries < LP_MAX_ACTIVE_BINNED_QUERIES);
   /* exceeding list size so just ignore the query */
   if (setup->active_binned_queries >= LP_MAX_ACTIVE_BINNED_QUERIES) {
         }
   assert(setup->active_queries[setup->active_binned_queries] == NULL);
   setup->active_queries[setup->active_binned_queries] = pq;
            assert(setup->scene);
   if (setup->scene) {
      if (!lp_scene_bin_everywhere(setup->scene,
                                    if (!lp_scene_bin_everywhere(setup->scene,
                        }
         }
         /**
   * Put an EndQuery command into all bins.
   */
   void
   lp_setup_end_query(struct lp_setup_context *setup, struct llvmpipe_query *pq)
   {
               assert(setup->scene);
   if (setup->scene) {
      /* pq->fence should be the fence of the *last* scene which
   * contributed to the query result.
   */
            if (pq->type == PIPE_QUERY_OCCLUSION_COUNTER ||
      pq->type == PIPE_QUERY_OCCLUSION_PREDICATE ||
   pq->type == PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE ||
   pq->type == PIPE_QUERY_PIPELINE_STATISTICS ||
   pq->type == PIPE_QUERY_TIMESTAMP ||
   pq->type == PIPE_QUERY_TIME_ELAPSED) {
   if (pq->type == PIPE_QUERY_TIMESTAMP &&
            /*
   * If there's a zero width/height framebuffer, there's no bins and
   * hence no rast task is ever run. So fill in something here instead.
   */
               if (!lp_scene_bin_everywhere(setup->scene,
                                 if (!lp_scene_bin_everywhere(setup->scene,
                        }
         } else {
      struct llvmpipe_screen *screen = llvmpipe_screen(setup->pipe->screen);
   mtx_lock(&screen->rast_mutex);
   lp_rast_fence(screen->rast, &pq->fence);
            fail:
      /* Need to do this now not earlier since it still needs to be marked as
   * active when binning it would cause a flush.
   */
   if (pq->type == PIPE_QUERY_OCCLUSION_COUNTER ||
      pq->type == PIPE_QUERY_OCCLUSION_PREDICATE ||
   pq->type == PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE ||
   pq->type == PIPE_QUERY_PIPELINE_STATISTICS ||
   pq->type == PIPE_QUERY_TIME_ELAPSED) {
            /* remove from active binned query list */
   for (i = 0; i < setup->active_binned_queries; i++) {
      if (setup->active_queries[i] == pq)
      }
   assert(i < setup->active_binned_queries);
   if (i == setup->active_binned_queries)
         setup->active_binned_queries--;
   setup->active_queries[i] = setup->active_queries[setup->active_binned_queries];
         }
         bool
   lp_setup_flush_and_restart(struct lp_setup_context *setup)
   {
                        if (!set_scene_state(setup, SETUP_FLUSHED, __func__))
            if (!lp_setup_update_state(setup, true))
               }
         void
   lp_setup_add_scissor_planes(const struct u_rect *scissor,
               {
      /*
   * When rasterizing scissored tris, use the intersection of the
   * triangle bounding box and the scissor rect to generate the
   * scissor planes.
   *
   * This permits us to cut off the triangle "tails" that are present
   * in the intermediate recursive levels caused when two of the
   * triangles edges don't diverge quickly enough to trivially reject
   * exterior blocks from the triangle.
   *
   * It's not really clear if it's worth worrying about these tails,
   * but since we generate the planes for each scissored tri, it's
   * free to trim them in this case.
   *
   * Note that otherwise, the scissor planes only vary in 'C' value,
   * and even then only on state-changes.  Could alternatively store
   * these planes elsewhere.
   * (Or only store the c value together with a bit indicating which
   * scissor edge this is, so rasterization would treat them differently
   * (easier to evaluate) to ordinary planes.)
   */
   int adj = multisample ? 127 : 0;
   if (s_planes[0]) {
      int x0 = scissor->x0 - 1;
   plane_s->dcdx = ~0U << 8;
   plane_s->dcdy = 0;
   plane_s->c = x0 << 8;
   plane_s->c += adj;
   plane_s->c = -plane_s->c; /* flip sign */
   plane_s->eo = 1 << 8;
      }
   if (s_planes[1]) {
      int x1 = scissor->x1;
   plane_s->dcdx = 1 << 8;
   plane_s->dcdy = 0;
   plane_s->c = x1 << 8;
   plane_s->c += 127 + adj;
   plane_s->eo = 0 << 8;
      }
   if (s_planes[2]) {
      int y0 = scissor->y0 - 1;
   plane_s->dcdx = 0;
   plane_s->dcdy = 1 << 8;
   plane_s->c = y0 << 8;
   plane_s->c += adj;
   plane_s->c = -plane_s->c; /* flip sign */
   plane_s->eo = 1 << 8;
      }
   if (s_planes[3]) {
      int y1 = scissor->y1;
   plane_s->dcdx = 0;
   plane_s->dcdy = ~0U << 8;
   plane_s->c = y1 << 8;
   plane_s->c += 127 + adj;
   plane_s->eo = 0;
         }
