   /**************************************************************************
   * 
   * Copyright 2003 VMware, Inc.
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
      #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "pipe/p_shader_tokens.h"
   #include "draw/draw_context.h"
   #include "draw/draw_vertex.h"
   #include "sp_context.h"
   #include "sp_screen.h"
   #include "sp_state.h"
   #include "sp_texture.h"
   #include "sp_tex_sample.h"
   #include "sp_tex_tile_cache.h"
         /**
   * Mark the current vertex layout as "invalid".
   * We'll validate the vertex layout later, when we start to actually
   * render a point or line or tri.
   */
   static void
   invalidate_vertex_layout(struct softpipe_context *softpipe)
   {
         }
         /**
   * The vertex info describes how to convert the post-transformed vertices
   * (simple float[][4]) used by the 'draw' module into vertices for
   * rasterization.
   *
   * This function validates the vertex layout.
   */
   static void
   softpipe_compute_vertex_info(struct softpipe_context *softpipe)
   {
               if (sinfo->valid == 0) {
      const struct tgsi_shader_info *fsInfo = &softpipe->fs_variant->info;
   struct vertex_info *vinfo = &softpipe->vertex_info;
   uint i;
   int vs_index;
   /*
   * This doesn't quite work right (wrt face injection, prim id,
   * wide points) - hit a couple assertions, misrenderings plus
   * memory corruption. Albeit could fix (the former two) by calling
   * this "more often" (rasterizer changes etc.). (The latter would
   * need to be included in draw_prepare_shader_outputs, but it looks
   * like that would potentially allocate quite some unused additional
   * vertex outputs.)
   * draw_prepare_shader_outputs(softpipe->draw);
            /*
   * Those can't actually be 0 (because pos is always at 0).
   * But use ints anyway to avoid confusion (in vs outputs, they
   * can very well be at pos 0).
   */
   softpipe->viewport_index_slot = -1;
   softpipe->layer_slot = -1;
                     /*
   * Put position always first (setup needs it there).
   */
   vs_index = draw_find_shader_output(softpipe->draw,
                     /*
   * Match FS inputs against VS outputs, emitting the necessary
   * attributes.
   */
   for (i = 0; i < fsInfo->num_inputs; i++) {
               switch (fsInfo->input_interpolate[i]) {
   case TGSI_INTERPOLATE_CONSTANT:
      interp = SP_INTERP_CONSTANT;
      case TGSI_INTERPOLATE_LINEAR:
      interp = SP_INTERP_LINEAR;
      case TGSI_INTERPOLATE_PERSPECTIVE:
      interp = SP_INTERP_PERSPECTIVE;
      case TGSI_INTERPOLATE_COLOR:
      assert(fsInfo->input_semantic_name[i] == TGSI_SEMANTIC_COLOR);
      default:
                  switch (fsInfo->input_semantic_name[i]) {
   case TGSI_SEMANTIC_POSITION:
                  case TGSI_SEMANTIC_COLOR:
      if (fsInfo->input_interpolate[i] == TGSI_INTERPOLATE_COLOR) {
      if (softpipe->rasterizer->flatshade)
         else
      }
               /*
   * Search for each input in current vs output:
   */
   vs_index = draw_find_shader_output(softpipe->draw,
                  if (fsInfo->input_semantic_name[i] == TGSI_SEMANTIC_COLOR &&
      vs_index == -1) {
   /*
   * try and find a bcolor.
   * Note that if there's both front and back color, draw will
   * have copied back to front color already.
   */
   vs_index = draw_find_shader_output(softpipe->draw,
                     sinfo->attrib[i].interp = interp;
   /* extremely pointless index map */
   sinfo->attrib[i].src_index = i + 1;
   /*
   * For vp index and layer, if the fs requires them but the vs doesn't
   * provide them, draw (vbuf) will give us the required 0 (slot -1).
   * (This means in this case we'll also use those slots in setup, which
   * isn't necessary but they'll contain the correct (0) value.)
   */
   if (fsInfo->input_semantic_name[i] ==
            softpipe->viewport_index_slot = (int)vinfo->num_attribs;
      } else if (fsInfo->input_semantic_name[i] == TGSI_SEMANTIC_LAYER) {
      softpipe->layer_slot = (int)vinfo->num_attribs;
   draw_emit_vertex_attr(vinfo, EMIT_4F, vs_index);
   /*
   * Note that we'd actually want to skip position (as we won't use
   * the attribute in the fs) but can't. The reason is that we don't
   * actually have an input/output map for setup (even though it looks
   * like we do...). Could adjust for this though even without a map.
      } else {
      /*
   * Note that we'd actually want to skip position (as we won't use
   * the attribute in the fs) but can't. The reason is that we don't
   * actually have an input/output map for setup (even though it looks
   * like we do...). Could adjust for this though even without a map.
   */
                  /* Figure out if we need pointsize as well.
   */
   vs_index = draw_find_shader_output(softpipe->draw,
            if (vs_index >= 0) {
      softpipe->psize_slot = (int)vinfo->num_attribs;
               /* Figure out if we need viewport index (if it wasn't already in fs input) */
   if (softpipe->viewport_index_slot < 0) {
      vs_index = draw_find_shader_output(softpipe->draw,
               if (vs_index >= 0) {
      softpipe->viewport_index_slot =(int)vinfo->num_attribs;
                  /* Figure out if we need layer (if it wasn't already in fs input) */
   if (softpipe->layer_slot < 0) {
      vs_index = draw_find_shader_output(softpipe->draw,
               if (vs_index >= 0) {
      softpipe->layer_slot = (int)vinfo->num_attribs;
                  draw_compute_vertex_size(vinfo);
      }
      }
         /**
   * Called from vbuf module.
   *
   * This will trigger validation of the vertex layout (and also compute
   * the required information for setup).
   */
   struct vertex_info *
   softpipe_get_vbuf_vertex_info(struct softpipe_context *softpipe)
   {
      softpipe_compute_vertex_info(softpipe);
      }
         /**
   * Recompute cliprect from scissor bounds, scissor enable and surface size.
   */
   static void
   compute_cliprect(struct softpipe_context *sp)
   {
      unsigned i;
   /* SP_NEW_FRAMEBUFFER
   */
   uint surfWidth = sp->framebuffer.width;
            for (i = 0; i < PIPE_MAX_VIEWPORTS; i++) {
      /* SP_NEW_RASTERIZER
   */
               /* SP_NEW_SCISSOR
   *
   * clip to scissor rect:
   */
   sp->cliprect[i].minx = MAX2(sp->scissors[i].minx, 0);
   sp->cliprect[i].miny = MAX2(sp->scissors[i].miny, 0);
   sp->cliprect[i].maxx = MIN2(sp->scissors[i].maxx, surfWidth);
      }
   else {
      /* clip to surface bounds */
   sp->cliprect[i].minx = 0;
   sp->cliprect[i].miny = 0;
   sp->cliprect[i].maxx = surfWidth;
            }
         static void
   set_shader_sampler(struct softpipe_context *softpipe,
               {
      int i;
   for (i = 0; i <= max_sampler; i++) {
      softpipe->tgsi.sampler[shader]->sp_sampler[i] =
         }
      void
   softpipe_update_compute_samplers(struct softpipe_context *softpipe)
   {
         }
      static void
   update_tgsi_samplers( struct softpipe_context *softpipe )
   {
               set_shader_sampler(softpipe, PIPE_SHADER_VERTEX,
         set_shader_sampler(softpipe, PIPE_SHADER_FRAGMENT,
         if (softpipe->gs) {
      set_shader_sampler(softpipe, PIPE_SHADER_GEOMETRY,
               /* XXX is this really necessary here??? */
   for (sh = 0; sh < ARRAY_SIZE(softpipe->tex_cache); sh++) {
      for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct softpipe_tex_tile_cache *tc = softpipe->tex_cache[sh][i];
   if (tc && tc->texture) {
      struct softpipe_resource *spt = softpipe_resource(tc->texture);
   if (spt->timestamp != tc->timestamp) {
      sp_tex_tile_cache_validate_texture( tc );
   /*
   _debug_printf("INV %d %d\n", tc->timestamp, spt->timestamp);
   */
                  }
         static void
   update_fragment_shader(struct softpipe_context *softpipe, unsigned prim)
   {
                        if (softpipe->fs) {
      softpipe->fs_variant = softpipe_find_fs_variant(softpipe,
            /* prepare the TGSI interpreter for FS execution */
   softpipe->fs_variant->prepare(softpipe->fs_variant, 
                              }
   else {
                  /* This would be the logical place to pass the fragment shader
   * to the draw module.  However, doing this here, during state
   * validation, causes problems with the 'draw' module helpers for
   * wide/AA/stippled lines.
   * In principle, the draw's fragment shader should be per-variant
   * but that doesn't work.  So we use a single draw fragment shader
   * per fragment shader, not per variant.
      #if 0
      if (softpipe->fs_variant) {
      draw_bind_fragment_shader(softpipe->draw,
      }
   else {
            #endif
   }
         /* Hopefully this will remain quite simple, otherwise need to pull in
   * something like the gallium frontend mechanism.
   */
   void
   softpipe_update_derived(struct softpipe_context *softpipe, unsigned prim)
   {
               /* Check for updated textures.
   */
   if (softpipe->tex_timestamp != sp_screen->timestamp) {
      softpipe->tex_timestamp = sp_screen->timestamp;
               if (softpipe->dirty & (SP_NEW_RASTERIZER |
                  /* TODO: this looks suboptimal */
   if (softpipe->dirty & (SP_NEW_SAMPLER |
                              if (softpipe->dirty & (SP_NEW_RASTERIZER |
                        if (softpipe->dirty & (SP_NEW_SCISSOR |
                        if (softpipe->dirty & (SP_NEW_BLEND |
                                 }
