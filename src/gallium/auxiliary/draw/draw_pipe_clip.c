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
   * \brief  Clipping stage
   *
   * \author  Keith Whitwell <keithw@vmware.com>
   */
         #include "util/u_bitcast.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
      #include "pipe/p_shader_tokens.h"
      #include "draw_vs.h"
   #include "draw_pipe.h"
   #include "draw_fs.h"
   #include "draw_gs.h"
         /** Set to 1 to enable printing of coords before/after clipping */
   #define DEBUG_CLIP 0
      #define MAX_CLIPPED_VERTICES ((2 * (6 + PIPE_MAX_CLIP_PLANES))+1)
         struct clip_stage {
               unsigned pos_attr;
   bool have_clipdist;
            /* List of the attributes to be constant interpolated. */
   unsigned num_const_attribs;
   uint8_t const_attribs[PIPE_MAX_SHADER_OUTPUTS];
   /* List of the attributes to be linear interpolated. */
   unsigned num_linear_attribs;
   uint8_t linear_attribs[PIPE_MAX_SHADER_OUTPUTS];
   /* List of the attributes to be perspective interpolated. */
   unsigned num_perspect_attribs;
               };
         /** Cast wrapper */
   static inline struct clip_stage *
   clip_stage(struct draw_stage *stage)
   {
         }
         static inline unsigned
   draw_viewport_index(struct draw_context *draw,
         {
      if (draw_current_shader_uses_viewport_index(draw)) {
      unsigned viewport_index_output =
         unsigned viewport_index =
            } else {
            }
         #define LINTERP(T, OUT, IN) ((OUT) + (T) * ((IN) - (OUT)))
         /* All attributes are float[4], so this is easy:
   */
   static void
   interp_attr(float dst[4],
               float t,
   {
      dst[0] = LINTERP(t, out[0], in[0]);
   dst[1] = LINTERP(t, out[1], in[1]);
   dst[2] = LINTERP(t, out[2], in[2]);
      }
         /**
   * Copy flat shaded attributes src vertex to dst vertex.
   */
   static void
   copy_flat(struct draw_stage *stage,
               {
      const struct clip_stage *clipper = clip_stage(stage);
   for (unsigned i = 0; i < clipper->num_const_attribs; i++) {
      const unsigned attr = clipper->const_attribs[i];
         }
         /* Interpolate between two vertices to produce a third.
   */
   static void
   interp(const struct clip_stage *clip,
         struct vertex_header *dst,
   float t,
   const struct vertex_header *out,
   const struct vertex_header *in,
   {
               /* Vertex header.
   */
   dst->clipmask = 0;
   dst->edgeflag = 0;        /* will get overwritten later */
   dst->pad = 0;
            /* Interpolate the clip-space coords.
   */
   if (clip->cv_attr >= 0) {
      interp_attr(dst->data[clip->cv_attr], t,
      }
   /* interpolate the clip-space position */
            /* Do the projective divide and viewport transformation to get
   * new window coordinates:
   */
   {
      const float *pos = dst->clip_pos;
   const float *scale =
         const float *trans =
                  dst->data[pos_attr][0] = pos[0] * oow * scale[0] + trans[0];
   dst->data[pos_attr][1] = pos[1] * oow * scale[1] + trans[1];
   dst->data[pos_attr][2] = pos[2] * oow * scale[2] + trans[2];
               /* interp perspective attribs */
   for (unsigned j = 0; j < clip->num_perspect_attribs; j++) {
      const unsigned attr = clip->perspect_attribs[j];
               /**
   * Compute the t in screen-space instead of 3d space to use
   * for noperspective interpolation.
   *
   * The points can be aligned with the X axis, so in that case try
   * the Y.  When both points are at the same screen position, we can
   * pick whatever value (the interpolated point won't be in front
   * anyway), so just use the 3d t.
   */
   if (clip->num_linear_attribs) {
      float t_nopersp = t;
   /* find either in.x != out.x or in.y != out.y */
   for (int k = 0; k < 2; k++) {
      if (in->clip_pos[k] != out->clip_pos[k]) {
      /* do divide by W, then compute linear interpolation factor */
   float in_coord = in->clip_pos[k] / in->clip_pos[3];
   float out_coord = out->clip_pos[k] / out->clip_pos[3];
   float dst_coord = dst->clip_pos[k] / dst->clip_pos[3];
   t_nopersp = (dst_coord - out_coord) / (in_coord - out_coord);
         }
   for (unsigned j = 0; j < clip->num_linear_attribs; j++) {
      const unsigned attr = clip->linear_attribs[j];
            }
         /**
   * Emit a post-clip polygon to the next pipeline stage.  The polygon
   * will be convex and the provoking vertex will always be vertex[0].
   */
   static void
   emit_poly(struct draw_stage *stage,
            struct vertex_header **inlist,
   const bool *edgeflags,
      {
      const struct clip_stage *clipper = clip_stage(stage);
            if (stage->draw->rasterizer->flatshade_first) {
      edge_first  = DRAW_PIPE_EDGE_FLAG_0;
   edge_middle = DRAW_PIPE_EDGE_FLAG_1;
      } else {
      edge_first  = DRAW_PIPE_EDGE_FLAG_2;
   edge_middle = DRAW_PIPE_EDGE_FLAG_0;
               if (!edgeflags[0])
            /* later stages may need the determinant, but only the sign matters */
   struct prim_header header;
   header.det = origPrim->det;
   header.flags = DRAW_PIPE_RESET_STIPPLE | edge_first | edge_middle;
            for (unsigned i = 2; i < n; i++, header.flags = edge_middle) {
      /* order the triangle verts to respect the provoking vertex mode */
   if (stage->draw->rasterizer->flatshade_first) {
      header.v[0] = inlist[0];  /* the provoking vertex */
   header.v[1] = inlist[i-1];
      } else {
      header.v[0] = inlist[i-1];
   header.v[1] = inlist[i];
               if (!edgeflags[i-1]) {
                  if (i == n - 1 && edgeflags[i])
            if (DEBUG_CLIP) {
      debug_printf("Clipped tri: (flat-shade-first = %d)\n",
         for (unsigned j = 0; j < 3; j++) {
      debug_printf("  Vert %d: clip pos: %f %f %f %f\n", j,
               header.v[j]->clip_pos[0],
   header.v[j]->clip_pos[1],
   if (clipper->cv_attr >= 0) {
      debug_printf("  Vert %d: cv: %f %f %f %f\n", j,
               header.v[j]->data[clipper->cv_attr][0],
      }
   for (unsigned k = 0; k < draw_num_shader_outputs(stage->draw); k++) {
      debug_printf("  Vert %d: Attr %d:  %f %f %f %f\n", j, k,
               header.v[j]->data[k][0],
            }
         }
         static inline float
   dot4(const float *a, const float *b)
   {
         }
      /*
   * this function extracts the clip distance for the current plane,
   * it first checks if the shader provided a clip distance, otherwise
   * it works out the value using the clipvertex
   */
   static inline float
   getclipdist(const struct clip_stage *clipper,
               {
      const float *plane;
            if (plane_idx < 6) {
      /* ordinary xyz view volume clipping uses pos output */
   plane = clipper->plane[plane_idx];
      }
   else if (clipper->have_clipdist) {
      /* pick the correct clipdistance element from the output vectors */
   int _idx = plane_idx - 6;
   int cdi = _idx >= 4;
   int vidx = cdi ? _idx - 4 : _idx;
      } else {
      /*
   * legacy user clip planes or gl_ClipVertex
   */
   plane = clipper->plane[plane_idx];
   if (clipper->cv_attr >= 0) {
         }
   else {
            }
      }
         /* Clip a triangle against the viewport and user clip planes.
   */
   static void
   do_clip_tri(struct draw_stage *stage,
               {
      struct clip_stage *clipper = clip_stage(stage);
   struct vertex_header *a[MAX_CLIPPED_VERTICES];
   struct vertex_header *b[MAX_CLIPPED_VERTICES];
   struct vertex_header **inlist = a;
   struct vertex_header **outlist = b;
   struct vertex_header *prov_vertex;
   unsigned tmpnr = 0;
   unsigned n = 3;
   bool aEdges[MAX_CLIPPED_VERTICES];
   bool bEdges[MAX_CLIPPED_VERTICES];
   bool *inEdges = aEdges;
   bool *outEdges = bEdges;
            inlist[0] = header->v[0];
   inlist[1] = header->v[1];
            /*
   * For d3d10, we need to take this from the leading (first) vertex.
   * For GL, we could do anything (as long as we advertize
   * GL_UNDEFINED_VERTEX for the VIEWPORT_INDEX_PROVOKING_VERTEX query),
   * but it needs to be consistent with what other parts (i.e. driver)
   * will do, and that seems easier with GL_PROVOKING_VERTEX logic.
   */
   if (stage->draw->rasterizer->flatshade_first) {
         } else {
         }
            if (DEBUG_CLIP) {
      const float *v0 = header->v[0]->clip_pos;
   const float *v1 = header->v[1]->clip_pos;
   const float *v2 = header->v[2]->clip_pos;
   debug_printf("Clip triangle pos:\n");
   debug_printf(" %f, %f, %f, %f\n", v0[0], v0[1], v0[2], v0[3]);
   debug_printf(" %f, %f, %f, %f\n", v1[0], v1[1], v1[2], v1[3]);
   debug_printf(" %f, %f, %f, %f\n", v2[0], v2[1], v2[2], v2[3]);
   if (clipper->cv_attr >= 0) {
      const float *v0 = header->v[0]->data[clipper->cv_attr];
   const float *v1 = header->v[1]->data[clipper->cv_attr];
   const float *v2 = header->v[2]->data[clipper->cv_attr];
   debug_printf("Clip triangle cv:\n");
   debug_printf(" %f, %f, %f, %f\n", v0[0], v0[1], v0[2], v0[3]);
   debug_printf(" %f, %f, %f, %f\n", v1[0], v1[1], v1[2], v1[3]);
                  /*
   * Note: at this point we can't just use the per-vertex edge flags.
   * We have to observe the edge flag bits set in header->flags which
   * were set during primitive decomposition.  Put those flags into
   * an edge flags array which parallels the vertex array.
   * Later, in the 'unfilled' pipeline stage we'll draw the edge if both
   * the header.flags bit is set AND the per-vertex edgeflag field is set.
   */
   inEdges[0] = !!(header->flags & DRAW_PIPE_EDGE_FLAG_0);
   inEdges[1] = !!(header->flags & DRAW_PIPE_EDGE_FLAG_1);
            while (clipmask && n >= 3) {
      const unsigned plane_idx = ffs(clipmask)-1;
   const bool is_user_clip_plane = plane_idx >= 6;
   struct vertex_header *vert_prev = inlist[0];
   bool *edge_prev = &inEdges[0];
   float dp_prev;
            dp_prev = getclipdist(clipper, vert_prev, plane_idx);
            if (util_is_inf_or_nan(dp_prev))
            assert(n < MAX_CLIPPED_VERTICES);
   if (n >= MAX_CLIPPED_VERTICES)
         inlist[n] = inlist[0]; /* prevent rotation of vertices */
            for (unsigned i = 1; i <= n; i++) {
      struct vertex_header *vert = inlist[i];
                                          if (dp_prev >= 0.0f) {
      assert(outcount < MAX_CLIPPED_VERTICES);
   if (outcount >= MAX_CLIPPED_VERTICES)
         outEdges[outcount] = *edge_prev;
   outlist[outcount++] = vert_prev;
      } else {
                  if (different_sign) {
                     assert(tmpnr < MAX_CLIPPED_VERTICES + 1);
   if (tmpnr >= MAX_CLIPPED_VERTICES + 1)
                  assert(outcount < MAX_CLIPPED_VERTICES);
                                 float denom = dp - dp_prev;
   if (dp < 0.0f) {
      /* Going out of bounds.  Avoid division by zero as we
   * know dp != dp_prev from different_sign, above.
   */
   if (-dp < dp_prev) {
      float t = dp / denom;
      } else {
                        /* Whether or not to set edge flag for the new vert depends
   * on whether it's a user-defined clipping plane.  We're
   * copying NVIDIA's behaviour here.
   */
   if (is_user_clip_plane) {
      /* we want to see an edge along the clip plane */
   *new_edge = true;
      }
   else {
      /* we don't want to see an edge along the frustum clip plane */
   *new_edge = *edge_prev;
         }
   else {
      /* Coming back in.
   */
   if (-dp_prev < dp) {
      float t = -dp_prev / denom;
      } else {
                        /* Copy starting vert's edgeflag:
   */
   new_vert->edgeflag = vert_prev->edgeflag;
                  vert_prev = vert;
   edge_prev = edge;
               /* swap in/out lists */
   {
      struct vertex_header **tmp = inlist;
   inlist = outlist;
   outlist = tmp;
      }
   {
      bool *tmp = inEdges;
   inEdges = outEdges;
                     /* If constant interpolated, copy provoking vertex attrib to polygon vertex[0]
   */
   if (n >= 3) {
      if (clipper->num_const_attribs) {
      if (stage->draw->rasterizer->flatshade_first) {
      if (inlist[0] != header->v[0]) {
      assert(tmpnr < MAX_CLIPPED_VERTICES + 1);
   if (tmpnr >= MAX_CLIPPED_VERTICES + 1)
         inlist[0] = dup_vert(stage, inlist[0], tmpnr++);
         }
   else {
      if (inlist[0] != header->v[2]) {
      assert(tmpnr < MAX_CLIPPED_VERTICES + 1);
   if (tmpnr >= MAX_CLIPPED_VERTICES + 1)
         inlist[0] = dup_vert(stage, inlist[0], tmpnr++);
                     /* Emit the polygon as triangles to the setup stage:
   */
         }
         /* Clip a line against the viewport and user clip planes.
   */
   static void
   do_clip_line(struct draw_stage *stage,
               {
      const struct clip_stage *clipper = clip_stage(stage);
   struct vertex_header *v0 = header->v[0];
   struct vertex_header *v1 = header->v[1];
   struct vertex_header *prov_vertex;
   float t0 = 0.0F;
   float t1 = 0.0F;
   struct prim_header newprim;
                     if (stage->draw->rasterizer->flatshade_first) {
         }
   else {
         }
            while (clipmask) {
      const unsigned plane_idx = ffs(clipmask)-1;
   const float dp0 = getclipdist(clipper, v0, plane_idx);
            if (util_is_inf_or_nan(dp0) || util_is_inf_or_nan(dp1))
            if (dp1 < 0.0F) {
      float t = dp1 / (dp1 - dp0);
               if (dp0 < 0.0F) {
      float t = dp0 / (dp0 - dp1);
               if (t0 + t1 >= 1.0F)
                        if (v0->clipmask) {
      interp(clipper, stage->tmp[0], t0, v0, v1, viewport_index);
   if (stage->draw->rasterizer->flatshade_first) {
         }
   else {
         }
      }
   else {
                  if (v1->clipmask) {
      interp(clipper, stage->tmp[1], t1, v1, v0, viewport_index);
   if (stage->draw->rasterizer->flatshade_first) {
         }
   else {
         }
      }
   else {
                     }
         static void
   clip_point(struct draw_stage *stage, struct prim_header *header)
   {
      if (header->v[0]->clipmask == 0)
      }
         /*
   * Clip points but ignore the first 4 (xy) clip planes.
   * (Because the generated clip mask is completely unaffacted by guard band,
   * we still need to manually evaluate the x/y planes if they are outside
   * the guard band and not just outside the vp.)
   */
   static void
   clip_point_guard_xy(struct draw_stage *stage, struct prim_header *header)
   {
      unsigned clipmask = header->v[0]->clipmask;
   if ((clipmask & 0xffffffff) == 0)
         else if ((clipmask & 0xfffffff0) == 0) {
      while (clipmask) {
      const unsigned plane_idx = ffs(clipmask)-1;
   clipmask &= ~(1 << plane_idx);  /* turn off this plane's bit */
   /* TODO: this should really do proper guardband clipping,
   * currently just throw out infs/nans.
   * Also note that vertices with negative w values MUST be tossed
   * out (not sure if proper guardband clipping would do this
   * automatically). These would usually be captured by depth clip
   * too but this can be disabled.
   */
   if (header->v[0]->clip_pos[3] <= 0.0f ||
      util_is_inf_or_nan(header->v[0]->clip_pos[0]) ||
   util_is_inf_or_nan(header->v[0]->clip_pos[1]))
   }
         }
         static void
   clip_first_point(struct draw_stage *stage, struct prim_header *header)
   {
      stage->point = stage->draw->guard_band_points_lines_xy ? clip_point_guard_xy : clip_point;
      }
         static void
   clip_line(struct draw_stage *stage, struct prim_header *header)
   {
      unsigned clipmask = (header->v[0]->clipmask |
            if (clipmask == 0) {
      /* no clipping needed */
      }
   else if ((header->v[0]->clipmask &
               }
      }
      static void
   clip_line_guard_xy(struct draw_stage *stage, struct prim_header *header)
   {
      unsigned clipmask = (header->v[0]->clipmask |
            if ((clipmask & 0xffffffff) == 0) {
         }
   else if ((clipmask & 0xfffffff0) == 0) {
      while (clipmask) {
      const unsigned plane_idx = ffs(clipmask)-1;
   clipmask &= ~(1 << plane_idx);  /* turn off this plane's bit */
   /* TODO: this should really do proper guardband clipping,
   * currently just throw out infs/nans.
   * Also note that vertices with negative w values MUST be tossed
   * out (not sure if proper guardband clipping would do this
   * automatically). These would usually be captured by depth clip
   * too but this can be disabled.
   */
   if ((header->v[0]->clip_pos[3] <= 0.0f &&
      header->v[1]->clip_pos[3] <= 0.0f) ||
   util_is_nan(header->v[0]->clip_pos[0]) ||
   util_is_nan(header->v[0]->clip_pos[1]) ||
   util_is_nan(header->v[1]->clip_pos[0]) ||
   util_is_nan(header->v[1]->clip_pos[1]))
   }
      } else if ((header->v[0]->clipmask &
                  }
      static void
   clip_tri(struct draw_stage *stage, struct prim_header *header)
   {
      unsigned clipmask = (header->v[0]->clipmask |
                  if (clipmask == 0) {
      /* no clipping needed */
      }
   else if ((header->v[0]->clipmask &
            header->v[1]->clipmask &
         }
         static enum tgsi_interpolate_mode
   find_interp(const struct draw_fragment_shader *fs,
               {
               /* If it's gl_{Front,Back}{,Secondary}Color, pick up the mode
   * from the array we've filled before. */
   if ((semantic_name == TGSI_SEMANTIC_COLOR ||
      semantic_name == TGSI_SEMANTIC_BCOLOR) &&
   semantic_index < 2) {
      } else if (semantic_name == TGSI_SEMANTIC_POSITION ||
            /* these inputs are handled specially always */
      } else {
      /* Otherwise, search in the FS inputs, with a decent default
   * if we don't find it.
   * This probably only matters for layer, vpindex, culldist, maybe
   * front_face.
   */
   unsigned j;
   if (semantic_name == TGSI_SEMANTIC_LAYER ||
      semantic_name == TGSI_SEMANTIC_VIEWPORT_INDEX) {
      }
   else {
         }
   if (fs) {
      for (j = 0; j < fs->info.num_inputs; j++) {
      if (semantic_name == fs->info.input_semantic_name[j] &&
      semantic_index == fs->info.input_semantic_index[j]) {
   interp = fs->info.input_interpolate[j];
               }
      }
         /* Update state.  Could further delay this until we hit the first
   * primitive that really requires clipping.
   */
   static void
   clip_init_state(struct draw_stage *stage)
   {
      struct clip_stage *clipper = clip_stage(stage);
   const struct draw_context *draw = stage->draw;
   const struct draw_fragment_shader *fs = draw->fs.fragment_shader;
            clipper->pos_attr = draw_current_shader_position_output(draw);
   clipper->have_clipdist = draw_current_shader_num_written_clipdistances(draw) > 0;
   if (draw_current_shader_clipvertex_output(draw) != clipper->pos_attr) {
         }
   else {
                  /* We need to know for each attribute what kind of interpolation is
   * done on it (flat, smooth or noperspective).  But the information
   * is not directly accessible for outputs, only for inputs.  So we
   * have to match semantic name and index between the VS (or GS/ES)
   * outputs and the FS inputs to get to the interpolation mode.
   *
   * The only hitch is with gl_FrontColor/gl_BackColor which map to
   * gl_Color, and their Secondary versions.  First there are (up to)
   * two outputs for one input, so we tuck the information in a
   * specific array.  Second if they don't have qualifiers, the
   * default value has to be picked from the global shade mode.
   *
   * Of course, if we don't have a fragment shader in the first
   * place, defaults should be used.
            /* First pick up the interpolation mode for
   * gl_Color/gl_SecondaryColor, with the correct default.
   */
   enum tgsi_interpolate_mode indexed_interp[2];
   indexed_interp[0] = indexed_interp[1] = draw->rasterizer->flatshade ?
            if (fs) {
      for (unsigned i = 0; i < fs->info.num_inputs; i++) {
      if (fs->info.input_semantic_name[i] == TGSI_SEMANTIC_COLOR &&
      fs->info.input_semantic_index[i] < 2) {
   if (fs->info.input_interpolate[i] != TGSI_INTERPOLATE_COLOR)
                              clipper->num_const_attribs = 0;
   clipper->num_linear_attribs = 0;
   clipper->num_perspect_attribs = 0;
   unsigned i;
   for (i = 0; i < info->num_outputs; i++) {
      /* Find the interpolation mode for a specific attribute */
   int interp = find_interp(fs, indexed_interp,
               switch (interp) {
   case TGSI_INTERPOLATE_CONSTANT:
      clipper->const_attribs[clipper->num_const_attribs] = i;
   clipper->num_const_attribs++;
      case TGSI_INTERPOLATE_LINEAR:
      clipper->linear_attribs[clipper->num_linear_attribs] = i;
   clipper->num_linear_attribs++;
      case TGSI_INTERPOLATE_PERSPECTIVE:
      clipper->perspect_attribs[clipper->num_perspect_attribs] = i;
   clipper->num_perspect_attribs++;
      case TGSI_INTERPOLATE_COLOR:
      if (draw->rasterizer->flatshade) {
      clipper->const_attribs[clipper->num_const_attribs] = i;
      } else {
      clipper->perspect_attribs[clipper->num_perspect_attribs] = i;
      }
      default:
      assert(interp == -1);
                  /* Search the extra vertex attributes */
   for (unsigned j = 0; j < draw->extra_shader_outputs.num; j++) {
      /* Find the interpolation mode for a specific attribute */
   enum tgsi_interpolate_mode interp =
      find_interp(fs, indexed_interp,
            switch (interp) {
   case TGSI_INTERPOLATE_CONSTANT:
      clipper->const_attribs[clipper->num_const_attribs] = i + j;
   clipper->num_const_attribs++;
      case TGSI_INTERPOLATE_LINEAR:
      clipper->linear_attribs[clipper->num_linear_attribs] = i + j;
   clipper->num_linear_attribs++;
      case TGSI_INTERPOLATE_PERSPECTIVE:
      clipper->perspect_attribs[clipper->num_perspect_attribs] = i + j;
   clipper->num_perspect_attribs++;
      default:
      assert(interp == -1);
                     }
         static void
   clip_first_tri(struct draw_stage *stage,
         {
      clip_init_state(stage);
      }
         static void
   clip_first_line(struct draw_stage *stage,
         {
      clip_init_state(stage);
   stage->line = stage->draw->guard_band_points_lines_xy ? clip_line_guard_xy : clip_line;
      }
         static void
   clip_flush(struct draw_stage *stage, unsigned flags)
   {
      stage->tri = clip_first_tri;
   stage->line = clip_first_line;
      }
         static void
   clip_reset_stipple_counter(struct draw_stage *stage)
   {
         }
         static void
   clip_destroy(struct draw_stage *stage)
   {
      draw_free_temp_verts(stage);
      }
         /**
   * Allocate a new clipper stage.
   * \return pointer to new stage object
   */
   struct draw_stage *
   draw_clip_stage(struct draw_context *draw)
   {
      struct clip_stage *clipper = CALLOC_STRUCT(clip_stage);
   if (!clipper)
            clipper->stage.draw = draw;
   clipper->stage.name = "clipper";
   clipper->stage.point = clip_first_point;
   clipper->stage.line = clip_first_line;
   clipper->stage.tri = clip_first_tri;
   clipper->stage.flush = clip_flush;
   clipper->stage.reset_stipple_counter = clip_reset_stipple_counter;
                     if (!draw_alloc_temp_verts(&clipper->stage, MAX_CLIPPED_VERTICES+1))
                  fail:
      if (clipper)
               }
