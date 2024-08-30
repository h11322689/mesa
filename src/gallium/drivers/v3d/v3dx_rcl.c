   /*
   * Copyright © 2017 Broadcom
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "util/format/u_format.h"
   #include "util/macros.h"
   #include "v3d_context.h"
   #include "broadcom/common/v3d_macros.h"
   #include "broadcom/common/v3d_tiling.h"
   #include "broadcom/common/v3d_util.h"
   #include "broadcom/cle/v3dx_pack.h"
      #define PIPE_CLEAR_COLOR_BUFFERS (PIPE_CLEAR_COLOR0 |                   \
                        #define PIPE_FIRST_COLOR_BUFFER_BIT (ffs(PIPE_CLEAR_COLOR0) - 1)
      /* The HW queues up the load until the tile coordinates show up, but can only
   * track one at a time.  If we need to do more than one load, then we need to
   * flush out the previous load by emitting the tile coordinates and doing a
   * dummy store.
   */
   static void
   flush_last_load(struct v3d_cl *cl)
   {
         if (V3D_VERSION >= 40)
            cl_emit(cl, TILE_COORDINATES_IMPLICIT, coords);
   cl_emit(cl, STORE_TILE_BUFFER_GENERAL, store) {
         }
      static void
   load_general(struct v3d_cl *cl, struct pipe_surface *psurf, int buffer,
         {
         struct v3d_surface *surf = v3d_surface(psurf);
   bool separate_stencil = surf->separate_stencil && buffer == STENCIL;
   if (separate_stencil) {
                                 uint32_t layer_offset =
               cl_emit(cl, LOAD_TILE_BUFFER_GENERAL, load) {
            #if V3D_VERSION >= 40
                  load.memory_format = surf->tiling;
   if (separate_stencil)
         else
         load.r_b_swap = surf->swap_rb;
   load.force_alpha_1 = util_format_has_alpha1(psurf->format);
   if (surf->tiling == V3D_TILING_UIF_NO_XOR ||
      surf->tiling == V3D_TILING_UIF_XOR) {
            } else if (surf->tiling == V3D_TILING_RASTER) {
                              if (psurf->texture->nr_samples > 1)
         #else /* V3D_VERSION < 40 */
                  /* Can't do raw ZSTENCIL loads -- need to load/store them to
   * separate buffers for Z and stencil.
   */
   assert(buffer != ZSTENCIL);
      #endif /* V3D_VERSION < 40 */
                  *loads_pending &= ~pipe_bit;
   if (*loads_pending)
   }
      static void
   store_general(struct v3d_job *job,
               struct v3d_cl *cl, struct pipe_surface *psurf,
   int layer, int buffer, int pipe_bit,
   {
         struct v3d_surface *surf = v3d_surface(psurf);
   bool separate_stencil = surf->separate_stencil && buffer == STENCIL;
   if (separate_stencil) {
                        *stores_pending &= ~pipe_bit;
                              uint32_t layer_offset =
               cl_emit(cl, STORE_TILE_BUFFER_GENERAL, store) {
            #if V3D_VERSION >= 40
                           if (separate_stencil)
                                       if (surf->tiling == V3D_TILING_UIF_NO_XOR ||
      surf->tiling == V3D_TILING_UIF_XOR) {
            } else if (surf->tiling == V3D_TILING_RASTER) {
                              assert(!resolve_4x || job->bbuf);
   if (psurf->texture->nr_samples > 1)
         else if (resolve_4x && job->bbuf->texture->nr_samples > 1)
         #else /* V3D_VERSION < 40 */
                  /* Can't do raw ZSTENCIL stores -- need to load/store them to
   * separate buffers for Z and stencil.
   */
   assert(buffer != ZSTENCIL);
   store.raw_mode = true;
   if (!last_store) {
            store.disable_color_buffers_clear_on_write = true;
      } else {
            store.disable_color_buffers_clear_on_write =
         !(((pipe_bit & PIPE_CLEAR_COLOR_BUFFERS) &&
         store.disable_z_buffer_clear_on_write =
               #endif /* V3D_VERSION < 40 */
                  /* There must be a TILE_COORDINATES_IMPLICIT between each store. */
   if (V3D_VERSION < 40 && !last_store) {
         }
      static int
   zs_buffer_from_pipe_bits(int pipe_clear_bits)
   {
         switch (pipe_clear_bits & PIPE_CLEAR_DEPTHSTENCIL) {
   case PIPE_CLEAR_DEPTHSTENCIL:
         case PIPE_CLEAR_DEPTH:
         case PIPE_CLEAR_STENCIL:
         default:
         }
      static void
   v3d_rcl_emit_loads(struct v3d_job *job, struct v3d_cl *cl, int layer)
   {
         /* When blitting, no color or zs buffer is loaded; instead the blit
      * source buffer is loaded for the aspects that we are going to blit.
      assert(!job->bbuf || job->load == 0);
   assert(!job->bbuf || job->nr_cbufs <= 1);
                     for (int i = 0; i < job->nr_cbufs; i++) {
                                                if (!psurf || (V3D_VERSION < 40 &&
                                    if ((loads_pending & PIPE_CLEAR_DEPTHSTENCIL) &&
         (V3D_VERSION >= 40 ||
   (job->zsbuf && job->zsbuf->texture->nr_samples > 1))) {
                           if (rsc->separate_stencil &&
      (loads_pending & PIPE_CLEAR_STENCIL)) {
         load_general(cl, src,
                     if (loads_pending & PIPE_CLEAR_DEPTHSTENCIL) {
            load_general(cl, src,
                  #if V3D_VERSION < 40
         /* The initial reload will be queued until we get the
      * tile coordinates.
      if (loads_pending) {
            cl_emit(cl, RELOAD_TILE_COLOR_BUFFER, load) {
            load.disable_color_buffer_load =
         (~loads_pending &
         load.enable_z_load =
         #else /* V3D_VERSION >= 40 */
         assert(!loads_pending);
   #endif
   }
      static void
   v3d_rcl_emit_stores(struct v3d_job *job, struct v3d_cl *cl, int layer)
   {
   #if V3D_VERSION < 40
         UNUSED bool needs_color_clear = job->clear & PIPE_CLEAR_COLOR_BUFFERS;
   UNUSED bool needs_z_clear = job->clear & PIPE_CLEAR_DEPTH;
            /* For clearing color in a TLB general on V3D 3.3:
      *
   * - NONE buffer store clears all TLB color buffers.
   * - color buffer store clears just the TLB color buffer being stored.
   * - Z/S buffers store may not clear the TLB color buffer.
   *
   * And on V3D 4.1, we only have one flag for "clear the buffer being
   * stored" in the general packet, and a separate packet to clear all
   * color TLB buffers.
   *
   * As a result, we only bother flagging TLB color clears in a general
   * packet when we don't have to emit a separate packet to clear all
   * TLB color buffers.
      bool general_color_clear = (needs_color_clear &&
         #else
         #endif
                     /* For V3D 4.1, use general stores for all TLB stores.
      *
   * For V3D 3.3, we only use general stores to do raw stores for any
   * MSAA surfaces.  These output UIF tiled images where each 4x MSAA
   * pixel is a 2x2 quad, and the format will be that of the
   * internal_type/internal_bpp, rather than the format from GL's
   * perspective.  Non-MSAA surfaces will use
   * STORE_MULTI_SAMPLE_RESOLVED_TILE_COLOR_BUFFER_EXTENDED.
      assert(!job->bbuf || job->nr_cbufs <= 1);
   for (int i = 0; i < job->nr_cbufs; i++) {
                                 struct pipe_surface *psurf = job->cbufs[i];
   if (!psurf ||
                                    if (job->store & PIPE_CLEAR_DEPTHSTENCIL && job->zsbuf &&
         !(V3D_VERSION < 40 && job->zsbuf->texture->nr_samples <= 1)) {
      assert(!job->early_zs_clear);
   struct v3d_resource *rsc = v3d_resource(job->zsbuf->texture);
   if (rsc->separate_stencil) {
            if (job->store & PIPE_CLEAR_DEPTH) {
         store_general(job, cl, job->zsbuf, layer,
                              if (job->store & PIPE_CLEAR_STENCIL) {
         store_general(job, cl, job->zsbuf, layer,
                  } else {
            store_general(job, cl, job->zsbuf, layer,
                  #if V3D_VERSION < 40
         if (stores_pending) {
                              store.disable_color_buffer_write =
                              /* Note that when set this will clear all of the color
   * buffers.
   */
   store.disable_color_buffers_clear_on_write =
         store.disable_z_buffer_clear_on_write =
         } else if (needs_color_clear && !general_color_clear) {
            /* If we didn't do our color clears in the general packet,
   * then emit a packet to clear all the TLB color buffers now.
   */
   cl_emit(cl, STORE_TILE_BUFFER_GENERAL, store) {
      #else /* V3D_VERSION >= 40 */
         /* If we're emitting an RCL with GL_ARB_framebuffer_no_attachments,
      * we still need to emit some sort of store.
      if (!job->store) {
            cl_emit(cl, STORE_TILE_BUFFER_GENERAL, store) {
                        /* GFXH-1461/GFXH-1689: The per-buffer store command's clear
      * buffer bit is broken for depth/stencil.  In addition, the
   * clear packet's Z/S bit is broken, but the RTs bit ends up
   * clearing Z/S.
      #if V3D_VERSION <= 42
                  cl_emit(cl, CLEAR_TILE_BUFFERS, clear) {
      #endif
   #if V3D_VERSION >= 71
         #endif
            #endif /* V3D_VERSION >= 40 */
   }
      static void
   v3d_rcl_emit_generic_per_tile_list(struct v3d_job *job, int layer)
   {
         /* Emit the generic list in our indirect state -- the rcl will just
      * have pointers into it.
      struct v3d_cl *cl = &job->indirect;
   v3d_cl_ensure_space(cl, 200, 1);
            if (V3D_VERSION >= 40) {
            /* V3D 4.x only requires a single tile coordinates, and
   * END_OF_LOADS switches us between loading and rendering.
                        if (V3D_VERSION < 40) {
            /* Tile Coordinates triggers the last reload and sets where
   * the stores go. There must be one per store packet.
               /* The binner starts out writing tiles assuming that the initial mode
      * is triangles, so make sure that's the case.
      cl_emit(cl, PRIM_LIST_FORMAT, fmt) {
            #if V3D_VERSION >= 41
         /* PTB assumes that value to be 0, but hw will not set it. */
   cl_emit(cl, SET_INSTANCEID, set) {
         #endif
                        #if V3D_VERSION >= 40
         #endif
                     cl_emit(&job->rcl, START_ADDRESS_OF_GENERIC_TILE_LIST, branch) {
               }
      #if V3D_VERSION > 33
   /* Note that for v71, render target cfg packets has just one field that
   * combined the internal type and clamp mode. For simplicity we keep just one
   * helper.
   *
   * Note: rt_type is in fact a "enum V3DX(Internal_Type)".
   *
   */
   static uint32_t
   v3dX(clamp_for_format_and_type)(uint32_t rt_type,
         {
   #if V3D_VERSION >= 40 && V3D_VERSION <= 42
         if (util_format_is_srgb(format)) {
   #if V3D_VERSION >= 42
         } else if (util_format_is_pure_integer(format)) {
   #endif
         } else {
         #endif
   #if V3D_VERSION >= 71
         switch (rt_type) {
   case V3D_INTERNAL_TYPE_8I:
         case V3D_INTERNAL_TYPE_8UI:
         case V3D_INTERNAL_TYPE_8:
         case V3D_INTERNAL_TYPE_16I:
         case V3D_INTERNAL_TYPE_16UI:
         case V3D_INTERNAL_TYPE_16F:
            return util_format_is_srgb(format) ?
      case V3D_INTERNAL_TYPE_32I:
         case V3D_INTERNAL_TYPE_32UI:
         case V3D_INTERNAL_TYPE_32F:
         default:
         }
   #endif
         }
   #endif
      #if V3D_VERSION >= 71
   static void
   v3d_setup_render_target(struct v3d_job *job,
                     {
         if (!job->cbufs[cbuf])
            struct v3d_surface *surf = v3d_surface(job->cbufs[cbuf]);
   *rt_bpp = surf->internal_bpp;
   if (job->bbuf) {
      struct v3d_surface *bsurf = v3d_surface(job->bbuf);
      }
   *rt_type_clamp = v3dX(clamp_for_format_and_type)(surf->internal_type,
   }
   #endif
      #if V3D_VERSION >= 40 && V3D_VERSION <= 42
   static void
   v3d_setup_render_target(struct v3d_job *job,
                           {
         if (!job->cbufs[cbuf])
            struct v3d_surface *surf = v3d_surface(job->cbufs[cbuf]);
   *rt_bpp = surf->internal_bpp;
   if (job->bbuf) {
      struct v3d_surface *bsurf = v3d_surface(job->bbuf);
      }
   *rt_type = surf->internal_type;
   *rt_clamp = v3dX(clamp_for_format_and_type)(surf->internal_type,
   }
   #endif
      #if V3D_VERSION < 40
   static void
   v3d_emit_z_stencil_config(struct v3d_job *job, struct v3d_surface *surf,
         {
         cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_Z_STENCIL, zs) {
                     if (!is_separate_stencil) {
                                                               if (job->store & (is_separate_stencil ?
                     }
   #endif /* V3D_VERSION < 40 */
      static bool
   supertile_in_job_scissors(struct v3d_job *job,
         {
      if (job->scissor.disabled || job->scissor.count == 0)
            const uint32_t min_x = x * w;
   const uint32_t min_y = y * h;
   const uint32_t max_x = min_x + w - 1;
            for (uint32_t i = 0; i < job->scissor.count; i++) {
         const uint32_t min_s_x = job->scissor.rects[i].min_x;
   const uint32_t min_s_y = job->scissor.rects[i].min_y;
                  if (max_x < min_s_x || min_x > max_s_x ||
                                    }
      #if V3D_VERSION >= 40
   static inline bool
   do_double_initial_tile_clear(const struct v3d_job *job)
   {
         /* Our rendering code emits an initial clear per layer, unlike the
      * Vulkan driver, which only executes a single initial clear for all
   * layers. This is because in GL we don't use the
   * 'clear_buffer_being_stored' bit when storing tiles, so each layer
   * needs the iniital clear. This is also why this helper, unlike the
   * Vulkan version, doesn't check the layer count to decide if double
   * clear for double buffer mode is required.
      return job->double_buffer &&
   }
   #endif
      static void
   emit_render_layer(struct v3d_job *job, uint32_t layer)
   {
                  /* If doing multicore binning, we would need to initialize each
      * core's tile list here.
      uint32_t tile_alloc_offset =
         cl_emit(&job->rcl, MULTICORE_RENDERING_TILE_LIST_SET_BASE, list) {
                  cl_emit(&job->rcl, MULTICORE_RENDERING_SUPERTILE_CFG, config) {
                           /* Size up our supertiles until we get under the limit. */
   for (;;) {
            frame_w_in_supertiles = DIV_ROUND_UP(job->draw_tiles_x,
         frame_h_in_supertiles = DIV_ROUND_UP(job->draw_tiles_y,
                                    if (supertile_w < supertile_h)
                                                                     /* Start by clearing the tile buffer. */
   cl_emit(&job->rcl, TILE_COORDINATES, coords) {
                        /* Emit an initial clear of the tile buffers.  This is necessary
      * for any buffers that should be cleared (since clearing
   * normally happens at the *end* of the generic tile list), but
   * it's also nice to clear everything so the first tile doesn't
   * inherit any contents from some previous frame.
   *
   * Also, implement the GFXH-1742 workaround.  There's a race in
   * the HW between the RCL updating the TLB's internal type/size
   * and thespawning of the QPU instances using the TLB's current
   * internal type/size.  To make sure the QPUs get the right
   * state, we need 1 dummy store in between internal type/size
      #if V3D_VERSION < 40
         cl_emit(&job->rcl, STORE_TILE_BUFFER_GENERAL, store) {
         #endif
   #if V3D_VERSION >= 40
         for (int i = 0; i < 2; i++) {
            if (i > 0)
         cl_emit(&job->rcl, END_OF_LOADS, end);
               #if V3D_VERSION < 71
                           #else
         #endif
                     #endif
                           /* XXX perf: We should expose GL_MESA_tile_raster_order to
      * improve X11 performance, but we should use Morton order
   * otherwise to improve cache locality.
      uint32_t supertile_w_in_pixels = job->tile_width * supertile_w;
   uint32_t supertile_h_in_pixels = job->tile_height * supertile_h;
   uint32_t min_x_supertile = job->draw_min_x / supertile_w_in_pixels;
            uint32_t max_x_supertile = 0;
   uint32_t max_y_supertile = 0;
   if (job->draw_max_x != 0 && job->draw_max_y != 0) {
                        for (int y = min_y_supertile; y <= max_y_supertile; y++) {
            for (int x = min_x_supertile; x <= max_x_supertile; x++) {
            if (supertile_in_job_scissors(job, x, y,
                     cl_emit(&job->rcl, SUPERTILE_COORDINATES, coords) {
         }
      void
   v3dX(emit_rcl)(struct v3d_job *job)
   {
         /* The RCL list should be empty. */
            v3d_cl_ensure_space_with_branch(&job->rcl, 200 +
               job->submit.rcl_start = job->rcl.bo->offset;
            /* Common config must be the first TILE_RENDERING_MODE_CFG
      * and Z_STENCIL_CLEAR_VALUES must be last.  The ones in between are
   * optional updates to the previous HW state.
      #if V3D_VERSION < 40
               #else /* V3D_VERSION >= 40 */
                  if (job->zsbuf) {
      #endif /* V3D_VERSION >= 40 */
                     if (job->decided_global_ez_enable) {
            switch (job->first_ez_state) {
   case V3D_EZ_UNDECIDED:
   case V3D_EZ_LT_LE:
         config.early_z_disable = false;
   config.early_z_test_and_update_direction =
         case V3D_EZ_GT_GE:
         config.early_z_disable = false;
   config.early_z_test_and_update_direction =
         case V3D_EZ_DISABLED:
      } else {
         #if V3D_VERSION >= 40
                                       #endif /* V3D_VERSION >= 40 */
                                                   #if V3D_VERSION <= 42
         #endif
   #if V3D_VERSION >= 71
                                 /* FIXME: ideallly we would like next assert on the packet header (as is
   * general, so also applies to GL). We would need to expand
   * gen_pack_header for that.
      #endif
               #if V3D_VERSION >= 71
                  /* If we don't have any color RTs, we sill need to emit one and flag
      * it as not used using stride = 1
      if (job->nr_cbufs == 0) {
      cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_RENDER_TARGET_PART1, rt) {
            #endif
         for (int i = 0; i < job->nr_cbufs; i++) {
         #if V3D_VERSION >= 71
                           #endif
                                                               /* XXX: Set the pad for raster. */
   if (surf->tiling == V3D_TILING_UIF_NO_XOR ||
      surf->tiling == V3D_TILING_UIF_XOR) {
         int uif_block_height = v3d_utile_height(rsc->cpp) * 2;
   uint32_t implicit_padded_height = (align(job->draw_height, uif_block_height) /
         if (surf->padded_height_of_output_image_in_uif_blocks -
      implicit_padded_height < 15) {
      config_pad = (surf->padded_height_of_output_image_in_uif_blocks -
   } else {
      #if V3D_VERSION < 40
                  cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_COLOR, rt) {
            rt.address = cl_address(rsc->bo, surf->offset);
   rt.internal_type = surf->internal_type;
   rt.output_image_format = surf->format;
                        #endif /* V3D_VERSION < 40 */
      #if V3D_VERSION <= 42
                  cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_CLEAR_COLORS_PART1,
            clear) {
                     if (surf->internal_bpp >= V3D_INTERNAL_BPP_64) {
            cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_CLEAR_COLORS_PART2,
         clear) {
   clear.clear_color_mid_low_32_bits =
               clear.clear_color_mid_high_24_bits =
                     if (surf->internal_bpp >= V3D_INTERNAL_BPP_128 || clear_pad) {
            cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_CLEAR_COLORS_PART3,
         clear) {
   #endif
   #if V3D_VERSION >= 71
                  cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_RENDER_TARGET_PART1, rt) {
            rt.clear_color_low_bits = job->clear_color[i][0];
   v3d_setup_render_target(job, i, &rt.internal_bpp,
         rt.stride =
                                    if (surf->internal_bpp >= V3D_INTERNAL_BPP_64) {
            cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_RENDER_TARGET_PART2, rt) {
         rt.clear_color_mid_bits = /* 40 bits (32 + 8)  */
                     if (surf->internal_bpp >= V3D_INTERNAL_BPP_128) {
            cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_RENDER_TARGET_PART3, rt) {
         rt.clear_color_top_bits = /* 56 bits (24 + 32) */
   #endif
            #if V3D_VERSION >= 40 && V3D_VERSION <= 42
         cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_COLOR, rt) {
            v3d_setup_render_target(job, 0,
                     v3d_setup_render_target(job, 1,
                     v3d_setup_render_target(job, 2,
                     v3d_setup_render_target(job, 3,
            #endif
      #if V3D_VERSION < 40
         /* FIXME: Don't bother emitting if we don't load/clear Z/S. */
   if (job->zsbuf) {
                                          /* Emit the separate stencil packet if we have a resource for
   * it.  The HW will only load/store this buffer if the
   * Z/Stencil config doesn't have stencil in its format.
   */
   if (surf->separate_stencil) {
            v3d_emit_z_stencil_config(job,
   #endif /* V3D_VERSION < 40 */
            /* Ends rendering mode config. */
   cl_emit(&job->rcl, TILE_RENDERING_MODE_CFG_ZS_CLEAR_VALUES,
            clear) {
               /* Always set initial block size before the first branch, which needs
      * to match the value from binning mode config.
      cl_emit(&job->rcl, TILE_LIST_INITIAL_BLOCK_SIZE, init) {
            init.use_auto_chained_tile_lists = true;
               /* ARB_framebuffer_no_attachments allows rendering to happen even when
      * the framebuffer has no attachments, the idea being that fragment
   * shaders can still do image load/store, ssbo, etc without having to
   * write to actual attachments, so always run at least one iteration
   * of the loop.
      assert(job->num_layers > 0 || (job->load == 0 && job->store == 0));
   for (int layer = 0; layer < MAX2(1, job->num_layers); layer++)
            }
