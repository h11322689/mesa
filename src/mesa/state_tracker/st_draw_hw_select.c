   /*
   * Copyright 2022 Advanced Micro Devices, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "main/enums.h"
   #include "main/context.h"
      #include "st_context.h"
   #include "st_nir.h"
   #include "st_draw.h"
      #include "nir.h"
   #include "nir_builtin_builder.h"
      #include "util/u_memory.h"
      union state_key {
      struct {
      unsigned num_user_clip_planes:4;
   unsigned face_culling_enabled:1;
   unsigned result_offset_from_attribute:1;
      };
      };
      enum primitive_state {
      HW_SELECT_PRIM_NONE,
   HW_SELECT_PRIM_POINTS,
   HW_SELECT_PRIM_LINES,
   HW_SELECT_PRIM_TRIANGLES,
      };
      struct geometry_constant {
      float depth_scale;
   float depth_transport;
   uint32_t culling_config;
   uint32_t result_offset;
      };
      #define set_uniform_location(var, field, packed)                 \
      do {                                                          \
      unsigned offset = offsetof(struct geometry_constant, field); \
   var->data.driver_location = offset >> (packed ? 2 : 4);    \
            static nir_def *
   has_nan_or_inf(nir_builder *b, nir_def *v)
   {
                           }
      static void
   return_if_true(nir_builder *b, nir_def *cond)
   {
      nir_if *if_cond = nir_push_if(b, cond);
   nir_jump(b, nir_jump_return);
      }
      static void
   get_input_vertices(nir_builder *b, nir_def **v)
   {
               nir_variable *in_pos = nir_variable_create(
      b->shader, nir_var_shader_in, glsl_array_type(glsl_vec4_type(), num_in_vert, 0),
               nir_def *is_nan_or_inf = NULL;
   for (int i = 0; i < num_in_vert; i++) {
      v[i] = nir_load_array_var_imm(b, in_pos, i);
   nir_def *r = has_nan_or_inf(b, v[i]);
      }
      }
      static void
   face_culling(nir_builder *b, nir_def **v, bool packed)
   {
      /* use the z value of the face normal to determine if the face points to us:
   *   Nz = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0)
   *
   * it should be in NDC (Normalized Device Coordinate), but now we are in clip
   * space (Vd = Vc / Vc.w), so multiply Nz with w0*w1*w2 to get the clip space
   * value:
   *   det = x0 * (y1 * w2 - y2 * w1) +
   *         x1 * (y2 * w0 - y0 * w2) +
   *         x2 * (y0 * w1 - y1 * w0)
   *
   * we only care about the sign of the det, but also need to count the sign of
   * w0/w1/w2 as a negtive w would change the direction of Nz < 0
   */
   nir_def *y1w2 = nir_fmul(b, nir_channel(b, v[1], 1), nir_channel(b, v[2], 3));
   nir_def *y2w1 = nir_fmul(b, nir_channel(b, v[2], 1), nir_channel(b, v[1], 3));
   nir_def *y2w0 = nir_fmul(b, nir_channel(b, v[2], 1), nir_channel(b, v[0], 3));
   nir_def *y0w2 = nir_fmul(b, nir_channel(b, v[0], 1), nir_channel(b, v[2], 3));
   nir_def *y0w1 = nir_fmul(b, nir_channel(b, v[0], 1), nir_channel(b, v[1], 3));
   nir_def *y1w0 = nir_fmul(b, nir_channel(b, v[1], 1), nir_channel(b, v[0], 3));
   nir_def *t0 = nir_fmul(b, nir_channel(b, v[0], 0), nir_fsub(b, y1w2, y2w1));
   nir_def *t1 = nir_fmul(b, nir_channel(b, v[1], 0), nir_fsub(b, y2w0, y0w2));
   nir_def *t2 = nir_fmul(b, nir_channel(b, v[2], 0), nir_fsub(b, y0w1, y1w0));
            /* invert det sign once any vertex w < 0 */
   nir_def *n0 = nir_flt_imm(b, nir_channel(b, v[0], 3), 0);
   nir_def *n1 = nir_flt_imm(b, nir_channel(b, v[1], 3), 0);
   nir_def *n2 = nir_flt_imm(b, nir_channel(b, v[2], 3), 0);
   nir_def *cond = nir_ixor(b, nir_ixor(b, n0, n1), n2);
            nir_variable *culling_config = nir_variable_create(
         set_uniform_location(culling_config, culling_config, packed);
            /* det < 0 then z points to camera */
   nir_def *zero = nir_imm_zero(b, 1, det->bit_size);
   nir_def *is_zero = nir_feq(b, det, zero);
   nir_def *is_neg = nir_flt(b, det, zero);
   nir_def *cull = nir_ixor(b, is_neg, config);
      }
      static void
   fast_frustum_culling(nir_builder *b, nir_def **v)
   {
               /* there are six culling planes for the visible volume:
   *   1.  x + w = 0
   *   2. -x + w = 0
   *   3.  y + w = 0
   *   4. -y + w = 0
   *   5.  z + w = 0
   *   6. -z + w = 0
   *
   * if all vertices of the primitive are outside (plane equation <0) of
   * any plane, the primitive must be invisible.
   */
   for (int i = 0; i < 6; i++) {
               for (int j = 0; j < b->shader->info.gs.vertices_in; j++) {
      nir_def *c = nir_channel(b, v[j], i >> 1);
                  nir_def *r = nir_flt(b, nir_channel(b, v[j], 3), c);
                              }
      static nir_def *
   get_intersection(nir_builder *b, nir_def *v1, nir_def *v2,
         {
      nir_def *factor = nir_fdiv(b, d1, nir_fsub(b, d1, d2));
      }
      #define begin_for_loop(name, max)                                       \
      nir_variable *name##_index =                                         \
         nir_store_var(b, name##_index, nir_imm_int(b, 0), 1);                \
         nir_loop *name = nir_push_loop(b);                                   \
   {                                                                    \
      nir_def *idx = nir_load_var(b, name##_index);                 \
      #define end_for_loop(name)                                              \
               nir_push_else(b, if_in_loop);                                     \
            }                                                                    \
         static void
   clip_with_plane(nir_builder *b, nir_variable *vert, nir_variable *num_vert,
         {
      nir_variable *all_clipped = nir_local_variable_create(
                  nir_variable *dist = nir_local_variable_create(
            nir_def *num = nir_load_var(b, num_vert);
   begin_for_loop(dist_loop, num)
   {
      nir_def *v = nir_load_array_var(b, vert, idx);
   nir_def *d = nir_fdot(b, v, plane);
            nir_def *clipped = nir_flt_imm(b, d, 0);
   nir_store_var(b, all_clipped,
      }
                     /* Use +/0/- to denote the dist[i] sign, which means:
   * +: inside plane
   * -: outside plane
   * 0: just on the plane
   *
   * Some example:
   * ++++: all vertex not clipped
   * ----: all vertex clipped
   * +-++: one vertex clipped, need to insert two vertex at '-', array grow
   * +--+: two vertex clipped, need to insert two vertex at '--', array same
   * +---: three vertex clipped, need to insert two vertex at '---', array trim
   * +-0+: one vertex clipped, need to insert one vertex at '-', array same
   *
   * Plane clip only produce convex polygon, so '-' must be contigous, there's
   * no '+-+-', so one clip plane can only grow array by 1.
            /* when array grow or '-' has been replaced with inserted vertex, save the
   * original vert to be used by following calculation.
   */
   nir_variable *saved =
            nir_variable *vert_index =
                  begin_for_loop(vert_loop, num)
   {
      nir_def *di = nir_load_array_var(b, dist, idx);
   nir_if *if_clipped = nir_push_if(b, nir_flt_imm(b, di, 0));
   {
               nir_def *prev = nir_bcsel(b, nir_ieq_imm(b, idx, 0),
               nir_def *dp = nir_load_array_var(b, dist, prev);
   nir_if *prev_if = nir_push_if(b, nir_fgt_imm(b, dp, 0));
   {
      /* +- case, replace - with inserted vertex
   * assert(vert_index <= idx), array is sure to not grow here
                                 nir_def *vp = nir_load_array_var(b, vert, prev);
   nir_def *iv = get_intersection(b, vp, vi, dp, di);
                                    nir_def *next = nir_bcsel(b, nir_ieq(b, idx, nir_iadd_imm(b, num, -1)),
         nir_def *dn = nir_load_array_var(b, dist, next);
   nir_if *next_if = nir_push_if(b, nir_fgt_imm(b, dn, 0));
   {
      /* -+ case, may grow array:
   *   vert_index > idx: +-+ case, grow array, current vertex in 'saved',
   *     save next + to 'saved', will replace it with inserted vertex.
   *   vert_index <= idx: --+ case, will replace last - with inserted vertex,
                  nir_def *index = nir_load_var(b, vert_index);
   nir_def *vi = nir_bcsel(b, nir_flt(b, idx, index),
                                                }
      }
   nir_push_else(b, if_clipped);
   {
      /* +/0 case, just keep the vert
   *   vert_index > idx: array grew case, vert[idx] is inserted vertex or prev
   *     +/0 vertex, current vertex is in 'saved', need to save next vertex
                  nir_def *index = nir_load_var(b, vert_index);
   nir_def *vi = nir_bcsel(b, nir_flt(b, idx, index),
                                    }
      }
               }
      static nir_def *
   get_user_clip_plane(nir_builder *b, int index, bool packed)
   {
      char name[16];
   snprintf(name, sizeof(name), "gl_ClipPlane%d", index);
   nir_variable *plane = nir_variable_create(
                        }
      static void
   get_depth_range_transform(nir_builder *b, bool packed, nir_def **trans)
   {
      nir_variable *depth_scale = nir_variable_create(
                  nir_variable *depth_transport = nir_variable_create(
                  trans[0] = nir_load_var(b, depth_scale);
      }
      static nir_def *
   get_window_space_depth(nir_builder *b, nir_def *v, nir_def **trans)
   {
      nir_def *z = nir_channel(b, v, 2);
            /* do perspective division, if w==0, xyz must be 0 too (otherwise can't pass
   * the clip test), 0/0=NaN, but we want it to be the nearest point.
   */
   nir_def *c = nir_feq_imm(b, w, 0);
            /* map [-1, 1] to [near, far] set by glDepthRange(near, far) */
      }
      static void
   update_result_buffer(nir_builder *b, nir_def *dmin, nir_def *dmax,
         {
      nir_def *offset;
   if (offset_from_attribute) {
      nir_variable *in_offset = nir_variable_create(
      b->shader, nir_var_shader_in,
   glsl_array_type(glsl_uint_type(), b->shader->info.gs.vertices_in, 0),
      in_offset->data.location = VARYING_SLOT_VAR0;
      } else {
      nir_variable *uni_offset = nir_variable_create(
         set_uniform_location(uni_offset, result_offset, packed);
               nir_variable_create(b->shader, nir_var_mem_ssbo,
                  nir_def *ssbo = nir_imm_int(b, 0);
   nir_ssbo_atomic(b, 32, ssbo, offset, nir_imm_int(b, 1),
         nir_ssbo_atomic(b, 32, ssbo, nir_iadd_imm(b, offset, 4), dmin,
         nir_ssbo_atomic(b, 32, ssbo, nir_iadd_imm(b, offset, 8), dmax,
      }
      static void
   build_point_nir_shader(nir_builder *b, union state_key state, bool packed)
   {
               nir_def *v;
                     nir_def *outside = NULL;
   for (int i = 0; i < state.num_user_clip_planes; i++) {
      nir_def *p = get_user_clip_plane(b, i, packed);
   nir_def *d = nir_fdot(b, v, p);
   nir_def *r = nir_flt_imm(b, d, 0);
      }
   if (outside)
            nir_def *trans[2];
            nir_def *depth = get_window_space_depth(b, v, trans);
   nir_def *fdepth = nir_fmul_imm(b, depth, 4294967295.0);
               }
      static nir_variable *
   create_clip_planes(nir_builder *b, int num_clip_planes, bool packed)
   {
      nir_variable *clip_planes = nir_local_variable_create(
            nir_def *unit_clip_planes[6] = {
      nir_imm_vec4(b,  1,  0,  0,  1),
   nir_imm_vec4(b, -1,  0,  0,  1),
   nir_imm_vec4(b,  0,  1,  0,  1),
   nir_imm_vec4(b,  0, -1,  0,  1),
   nir_imm_vec4(b,  0,  0,  1,  1),
      };
   for (int i = 0; i < 6; i++)
            for (int i = 6; i < num_clip_planes; i++) {
      nir_def *p = get_user_clip_plane(b, i - 6, packed);
                  }
      static void
   build_line_nir_shader(nir_builder *b, union state_key state, bool packed)
   {
               nir_def *v[2];
                     nir_variable *vert0 = nir_local_variable_create(b->impl, glsl_vec4_type(), "vert0");
            nir_variable *vert1 = nir_local_variable_create(b->impl, glsl_vec4_type(), "vert1");
            const int num_clip_planes = 6 + state.num_user_clip_planes;
            begin_for_loop(clip_loop, nir_imm_int(b, num_clip_planes))
   {
      nir_def *plane = nir_load_array_var(b, clip_planes, idx);
   nir_def *v0 = nir_load_var(b, vert0);
   nir_def *v1 = nir_load_var(b, vert1);
   nir_def *d0 = nir_fdot(b, v0, plane);
   nir_def *d1 = nir_fdot(b, v1, plane);
   nir_def *n0 = nir_flt_imm(b, d0, 0);
                     nir_if *clip_if = nir_push_if(b, nir_ior(b, n0, n1));
   {
      nir_def *iv = get_intersection(b, v0, v1, d0, d1);
   nir_store_var(b, vert0, nir_bcsel(b, n0, iv, v0), 0xf);
      }
      }
            nir_def *trans[2];
            nir_def *d0 = get_window_space_depth(b, nir_load_var(b, vert0), trans);
            nir_def *dmin = nir_fmin(b, d0, d1);
            nir_def *fdmin = nir_fmul_imm(b, dmin, 4294967295.0);
            nir_def *fdmax = nir_fmul_imm(b, dmax, 4294967295.0);
               }
      static void
   build_planar_primitive_nir_shader(nir_builder *b, union state_key state, bool packed)
   {
      const int num_in_vert = b->shader->info.gs.vertices_in;
            nir_def *v[4];
            if (state.face_culling_enabled)
            /* fast frustum culling, this should filter out most primitives */
            const int num_clip_planes = 6 + state.num_user_clip_planes;
            /* TODO: could use shared memory (ie. AMD GPU LDS) for this array
   * to reduce register usage.
   */
   nir_variable *vert = nir_local_variable_create(
         for (int i = 0; i < num_in_vert; i++)
            nir_variable *num_vert =
                           /* accurate clipping with all clip planes */
   begin_for_loop(clip_loop, nir_imm_int(b, num_clip_planes))
   {
      nir_def *plane = nir_load_array_var(b, clip_planes, idx);
      }
            nir_def *trans[2];
            nir_variable *dmin =
                  nir_variable *dmax =
                  begin_for_loop(depth_loop, nir_load_var(b, num_vert))
   {
      nir_def *vtx = nir_load_array_var(b, vert, idx);
   nir_def *depth = get_window_space_depth(b, vtx, trans);
   nir_store_var(b, dmin, nir_fmin(b, nir_load_var(b, dmin), depth), 1);
      }
            nir_def *fdmin = nir_fmul_imm(b, nir_load_var(b, dmin), 4294967295.0);
            nir_def *fdmax = nir_fmul_imm(b, nir_load_var(b, dmax), 4294967295.0);
               }
      static void *
   hw_select_create_gs(struct st_context *st, union state_key state)
   {
      const nir_shader_compiler_options *options =
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_GEOMETRY, options,
            nir_shader *nir = b.shader;
   nir->info.inputs_read = VARYING_BIT_POS;
   nir->num_uniforms = DIV_ROUND_UP(sizeof(struct geometry_constant), (4 * sizeof(float)));
   nir->info.num_ssbos = 1;
   nir->info.gs.output_primitive = MESA_PRIM_POINTS;
   nir->info.gs.vertices_out = 1;
   nir->info.gs.invocations = 1;
            if (state.result_offset_from_attribute)
                     switch (state.primitive) {
   case HW_SELECT_PRIM_POINTS:
      nir->info.gs.input_primitive = MESA_PRIM_POINTS;
   nir->info.gs.vertices_in = 1;
   build_point_nir_shader(&b, state, packed);
      case HW_SELECT_PRIM_LINES:
      nir->info.gs.input_primitive = MESA_PRIM_LINES;
   nir->info.gs.vertices_in = 2;
   build_line_nir_shader(&b, state, packed);
      case HW_SELECT_PRIM_TRIANGLES:
      nir->info.gs.input_primitive = MESA_PRIM_TRIANGLES;
   nir->info.gs.vertices_in = 3;
   build_planar_primitive_nir_shader(&b, state, packed);
      case HW_SELECT_PRIM_QUADS:
      /* geometry shader has no quad primitive, use lines_adjacency instead */
   nir->info.gs.input_primitive = MESA_PRIM_LINES_ADJACENCY;
   nir->info.gs.vertices_in = 4;
   build_planar_primitive_nir_shader(&b, state, packed);
      default:
                              }
      bool
   st_draw_hw_select_prepare_common(struct gl_context *ctx)
   {
      struct st_context *st = st_context(ctx);
   if (ctx->GeometryProgram._Current ||
      ctx->TessCtrlProgram._Current ||
   ctx->TessEvalProgram._Current) {
   fprintf(stderr, "HW GL_SELECT does not support user geometry/tessellation shader\n");
                        float n = ctx->ViewportArray[0].Near;
   float f = ctx->ViewportArray[0].Far;
   consts.depth_scale = (f - n) / 2;
            /* this field is not used when face culling disabled */
   consts.culling_config =
      (ctx->Polygon.CullFaceMode == GL_BACK) ^
         /* this field is not used when passing result offset by attribute */
            int num_planes = 0;
   u_foreach_bit(i, ctx->Transform.ClipPlanesEnabled) {
      COPY_4V(consts.clip_planes[num_planes], ctx->Transform._ClipUserPlane[i]);
               struct pipe_constant_buffer cb;
   cb.buffer = NULL;
   cb.user_buffer = &consts;
   cb.buffer_offset = 0;
            struct pipe_context *pipe = st->pipe;
            struct pipe_shader_buffer buffer;
   memset(&buffer, 0, sizeof(buffer));
   buffer.buffer = ctx->Select.Result->buffer;
                        }
      static union state_key
   make_state_key(struct gl_context *ctx, int mode)
   {
               switch (mode) {
   case GL_POINTS:
      state.primitive = HW_SELECT_PRIM_POINTS;
      case GL_LINES:
   case GL_LINE_STRIP:
   case GL_LINE_LOOP:
      state.primitive = HW_SELECT_PRIM_LINES;
      case GL_QUADS:
      state.primitive = HW_SELECT_PRIM_QUADS;
      case GL_TRIANGLES:
   case GL_TRIANGLE_STRIP:
   case GL_TRIANGLE_FAN:
         case GL_QUAD_STRIP:
   case GL_POLYGON:
      state.primitive = HW_SELECT_PRIM_TRIANGLES;
      default:
      fprintf(stderr, "HW GL_SELECT does not support draw mode %s\n",
                     /* TODO: support gl_ClipDistance/gl_CullDistance, but it costs more regs */
   struct gl_program *vp = ctx->VertexProgram._Current;
   if (vp->info.clip_distance_array_size || vp->info.cull_distance_array_size) {
      fprintf(stderr, "HW GL_SELECT does not support gl_ClipDistance/gl_CullDistance\n");
                        /* face culling only apply to 2D primitives */
   if (state.primitive == HW_SELECT_PRIM_QUADS ||
      state.primitive == HW_SELECT_PRIM_TRIANGLES)
         state.result_offset_from_attribute =
      ctx->VertexProgram._VPMode == VP_MODE_FF &&
            }
      bool
   st_draw_hw_select_prepare_mode(struct gl_context *ctx, struct pipe_draw_info *info)
   {
      union state_key key = make_state_key(ctx, info->mode);
   if (!key.u32)
            struct st_context *st = st_context(ctx);
   if (!st->hw_select_shaders)
            struct hash_entry *he = _mesa_hash_table_search(st->hw_select_shaders,
         void *gs;
   if (!he) {
      gs = hw_select_create_gs(st, key);
   if (!gs)
               } else
            struct cso_context *cso = st->cso_context;
            /* Replace draw mode with equivalent one which geometry shader support.
   *
   * New mode consume same vertex buffer structure and produce primitive with
   * same vertices (no need to be same type of primitive, because geometry shader
   * operate on vertives and emit nothing).
   *
   * We can break QUAD and POLYGON to triangles with same shape. But we can't futher
   * break them into single line or point because new primitive need to contain >=3
   * vertices so that it's still handled in 2D (planar) way instead of 1D (line) or
   * 0D (point) way which have different algorithm.
   */
   switch (info->mode) {
   case GL_QUADS:
      info->mode = GL_LINES_ADJACENCY;
      case GL_QUAD_STRIP:
      info->mode = GL_TRIANGLE_STRIP;
      case GL_POLYGON:
      info->mode = GL_TRIANGLE_FAN;
      default:
                  /* Only normal glBegin/End draws pass result offset by attribute to avoid flush
   * vertices when change name stack, so multiple glBegin/End sections before/after
   * name stack calls can be merged to a single draw call. To achieve this We mark
   * name stack result buffer used in glEnd instead of the last draw call.
   *
   * Other case like glDrawArrays and display list replay won't merge draws cross
   * name stack calls, so we just mark name stack result buffer used here.
   */
   if (!key.result_offset_from_attribute)
               }
