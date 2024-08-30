   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   * Copyright 2008-2010 VMware, Inc.  All rights reserved.
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
   * Texture sampling
   *
   * Authors:
   *   Brian Paul
   *   Keith Whitwell
   */
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_shader_tokens.h"
   #include "util/u_math.h"
   #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "sp_quad.h"   /* only for #define QUAD_* tokens */
   #include "sp_tex_sample.h"
   #include "sp_texture.h"
   #include "sp_tex_tile_cache.h"
         /** Set to one to help debug texture sampling */
   #define DEBUG_TEX 0
         /*
   * Return fractional part of 'f'.  Used for computing interpolation weights.
   * Need to be careful with negative values.
   * Note, if this function isn't perfect you'll sometimes see 1-pixel bands
   * of improperly weighted linear-filtered textures.
   * The tests/texwrap.c demo is a good test.
   */
   static inline float
   frac(float f)
   {
         }
            /**
   * Linear interpolation macro
   */
   static inline float
   lerp(float a, float v0, float v1)
   {
         }
         /**
   * Do 2D/bilinear interpolation of float values.
   * v00, v10, v01 and v11 are typically four texture samples in a square/box.
   * a and b are the horizontal and vertical interpolants.
   * It's important that this function is inlined when compiled with
   * optimization!  If we find that's not true on some systems, convert
   * to a macro.
   */
   static inline float
   lerp_2d(float a, float b,
         {
      const float temp0 = lerp(a, v00, v10);
   const float temp1 = lerp(a, v01, v11);
      }
         /**
   * As above, but 3D interpolation of 8 values.
   */
   static inline float
   lerp_3d(float a, float b, float c,
         float v000, float v100, float v010, float v110,
   {
      const float temp0 = lerp_2d(a, b, v000, v100, v010, v110);
   const float temp1 = lerp_2d(a, b, v001, v101, v011, v111);
      }
            /**
   * Compute coord % size for repeat wrap modes.
   * Note that if coord is negative, coord % size doesn't give the right
   * value.  To avoid that problem we add a large multiple of the size
   * (rather than using a conditional).
   */
   static inline int
   repeat(int coord, unsigned size)
   {
         }
         /**
   * Apply texture coord wrapping mode and return integer texture indexes
   * for a vector of four texcoords (S or T or P).
   * \param wrapMode  PIPE_TEX_WRAP_x
   * \param s  the incoming texcoords
   * \param size  the texture image size
   * \param icoord  returns the integer texcoords
   */
   static void
   wrap_nearest_repeat(float s, unsigned size, int offset, int *icoord)
   {
      /* s limited to [0,1) */
   /* i limited to [0,size-1] */
   const int i = util_ifloor(s * size);
      }
         static void
   wrap_nearest_clamp(float s, unsigned size, int offset, int *icoord)
   {
      /* s limited to [0,1] */
   /* i limited to [0,size-1] */
   s *= size;
   s += offset;
   if (s <= 0.0F)
         else if (s >= size)
         else
      }
         static void
   wrap_nearest_clamp_to_edge(float s, unsigned size, int offset, int *icoord)
   {
      /* s limited to [min,max] */
   /* i limited to [0, size-1] */
   const float min = 0.5F;
            s *= size;
            if (s < min)
         else if (s > max)
         else
      }
         static void
   wrap_nearest_clamp_to_border(float s, unsigned size, int offset, int *icoord)
   {
      /* s limited to [min,max] */
   /* i limited to [-1, size] */
   const float min = -0.5F;
            s *= size;
   s += offset;
   if (s <= min)
         else if (s >= max)
         else
      }
      static void
   wrap_nearest_mirror_repeat(float s, unsigned size, int offset, int *icoord)
   {
      const float min = 1.0F / (2.0F * size);
   const float max = 1.0F - min;
   int flr;
            s += (float)offset / size;
   flr = util_ifloor(s);
   u = frac(s);
   if (flr & 1)
         if (u < min)
         else if (u > max)
         else
      }
         static void
   wrap_nearest_mirror_clamp(float s, unsigned size, int offset, int *icoord)
   {
      /* s limited to [0,1] */
   /* i limited to [0,size-1] */
   const float u = fabsf(s * size + offset);
   if (u <= 0.0F)
         else if (u >= size)
         else
      }
         static void
   wrap_nearest_mirror_clamp_to_edge(float s, unsigned size, int offset, int *icoord)
   {
      /* s limited to [min,max] */
   /* i limited to [0, size-1] */
   const float min = 0.5F;
   const float max = (float)size - 0.5F;
            if (u < min)
         else if (u > max)
         else
      }
         static void
   wrap_nearest_mirror_clamp_to_border(float s, unsigned size, int offset, int *icoord)
   {
      /* u limited to [-0.5, size-0.5] */
   const float min = -0.5F;
   const float max = (float)size + 0.5F;
            if (u < min)
         else if (u > max)
         else
      }
         /**
   * Used to compute texel locations for linear sampling
   * \param wrapMode  PIPE_TEX_WRAP_x
   * \param s  the texcoord
   * \param size  the texture image size
   * \param icoord0  returns first texture index
   * \param icoord1  returns second texture index (usually icoord0 + 1)
   * \param w  returns blend factor/weight between texture indices
   * \param icoord  returns the computed integer texture coord
   */
   static void
   wrap_linear_repeat(float s, unsigned size, int offset,
         {
      const float u = s * size - 0.5F;
   *icoord0 = repeat(util_ifloor(u) + offset, size);
   *icoord1 = repeat(*icoord0 + 1, size);
      }
         static void
   wrap_linear_clamp(float s, unsigned size, int offset,
         {
               *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
      }
         static void
   wrap_linear_clamp_to_edge(float s, unsigned size, int offset,
         {
      const float u = CLAMP(s * size + offset, 0.0F, (float)size) - 0.5f;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   if (*icoord0 < 0)
         if (*icoord1 >= (int) size)
            }
         static void
   wrap_linear_clamp_to_border(float s, unsigned size, int offset,
         {
      const float min = -1.0F;
   const float max = (float)size + 0.5F;
   const float u = CLAMP(s * size + offset, min, max) - 0.5f;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
      }
         static void
   wrap_linear_mirror_repeat(float s, unsigned size, int offset,
         {
      int flr;
   float u;
            s += (float)offset / size;
   flr = util_ifloor(s);
            u = frac(s);
   if (no_mirror) {
         } else {
      u = 1.0F - u;
               *icoord0 = util_ifloor(u);
            if (*icoord0 < 0)
         if (*icoord0 >= (int) size)
            if (*icoord1 >= (int) size)
         if (*icoord1 < 0)
               }
         static void
   wrap_linear_mirror_clamp(float s, unsigned size, int offset,
         {
      float u = fabsf(s * size + offset);
   if (u >= size)
         u -= 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
      }
         static void
   wrap_linear_mirror_clamp_to_edge(float s, unsigned size, int offset,
         {
      float u = fabsf(s * size + offset);
   if (u >= size)
         u -= 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   if (*icoord0 < 0)
         if (*icoord1 >= (int) size)
            }
         static void
   wrap_linear_mirror_clamp_to_border(float s, unsigned size, int offset,
         {
      const float min = -0.5F;
   const float max = size + 0.5F;
   const float t = fabsf(s * size + offset);
   const float u = CLAMP(t, min, max) - 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
      }
         /**
   * PIPE_TEX_WRAP_CLAMP for nearest sampling, unnormalized coords.
   */
   static void
   wrap_nearest_unorm_clamp(float s, unsigned size, int offset, int *icoord)
   {
      const int i = util_ifloor(s);
      }
         /**
   * PIPE_TEX_WRAP_CLAMP_TO_BORDER for nearest sampling, unnormalized coords.
   */
   static void
   wrap_nearest_unorm_clamp_to_border(float s, unsigned size, int offset, int *icoord)
   {
         }
         /**
   * PIPE_TEX_WRAP_CLAMP_TO_EDGE for nearest sampling, unnormalized coords.
   */
   static void
   wrap_nearest_unorm_clamp_to_edge(float s, unsigned size, int offset, int *icoord)
   {
         }
         /**
   * PIPE_TEX_WRAP_CLAMP for linear sampling, unnormalized coords.
   */
   static void
   wrap_linear_unorm_clamp(float s, unsigned size, int offset,
         {
      /* Not exactly what the spec says, but it matches NVIDIA output */
   const float u = CLAMP(s + offset - 0.5F, 0.0f, (float) size - 1.0f);
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
      }
         /**
   * PIPE_TEX_WRAP_CLAMP_TO_BORDER for linear sampling, unnormalized coords.
   */
   static void
   wrap_linear_unorm_clamp_to_border(float s, unsigned size, int offset,
         {
      const float u = CLAMP(s + offset, -0.5F, (float) size + 0.5F) - 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   if (*icoord1 > (int) size - 1)
            }
         /**
   * PIPE_TEX_WRAP_CLAMP_TO_EDGE for linear sampling, unnormalized coords.
   */
   static void
   wrap_linear_unorm_clamp_to_edge(float s, unsigned size, int offset,
         {
      const float u = CLAMP(s + offset, +0.5F, (float) size - 0.5F) - 0.5F;
   *icoord0 = util_ifloor(u);
   *icoord1 = *icoord0 + 1;
   if (*icoord1 > (int) size - 1)
            }
         /**
   * Do coordinate to array index conversion.  For array textures.
   */
   static inline int
   coord_to_layer(float coord, unsigned first_layer, unsigned last_layer)
   {
      const int c = util_ifloor(coord + 0.5F);
      }
      static void
   compute_gradient_1d(const float s[TGSI_QUAD_SIZE],
                     {
      memset(derivs, 0, 6 * TGSI_QUAD_SIZE * sizeof(float));
   derivs[0][0][0] = s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT];
      }
      static float
   compute_lambda_1d_explicit_gradients(const struct sp_sampler_view *sview,
               {
      const struct pipe_resource *texture = sview->base.texture;
   const float dsdx = fabsf(derivs[0][0][quad]);
   const float dsdy = fabsf(derivs[0][1][quad]);
   const float rho = MAX2(dsdx, dsdy) * u_minify(texture->width0, sview->base.u.tex.first_level);
      }
         /**
   * Examine the quad's texture coordinates to compute the partial
   * derivatives w.r.t X and Y, then compute lambda (level of detail).
   */
   static float
   compute_lambda_1d(const struct sp_sampler_view *sview,
                     {
      float derivs[3][2][TGSI_QUAD_SIZE];
   compute_gradient_1d(s, t, p, derivs);
      }
         static void
   compute_gradient_2d(const float s[TGSI_QUAD_SIZE],
                     {
      memset(derivs, 0, 6 * TGSI_QUAD_SIZE * sizeof(float));
   derivs[0][0][0] = s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT];
   derivs[0][1][0] = s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT];
   derivs[1][0][0] = t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT];
      }
      static float
   compute_lambda_2d_explicit_gradients(const struct sp_sampler_view *sview,
               {
      const struct pipe_resource *texture = sview->base.texture;
   const float dsdx = fabsf(derivs[0][0][quad]);
   const float dsdy = fabsf(derivs[0][1][quad]);
   const float dtdx = fabsf(derivs[1][0][quad]);
   const float dtdy = fabsf(derivs[1][1][quad]);
   const float maxx = MAX2(dsdx, dsdy) * u_minify(texture->width0, sview->base.u.tex.first_level);
   const float maxy = MAX2(dtdx, dtdy) * u_minify(texture->height0, sview->base.u.tex.first_level);
   const float rho  = MAX2(maxx, maxy);
      }
         static float
   compute_lambda_2d(const struct sp_sampler_view *sview,
                     {
      float derivs[3][2][TGSI_QUAD_SIZE];
   compute_gradient_2d(s, t, p, derivs);
      }
         static void
   compute_gradient_3d(const float s[TGSI_QUAD_SIZE],
                     {
      memset(derivs, 0, 6 * TGSI_QUAD_SIZE * sizeof(float));
   derivs[0][0][0] = fabsf(s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT]);
   derivs[0][1][0] = fabsf(s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT]);
   derivs[1][0][0] = fabsf(t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT]);
   derivs[1][1][0] = fabsf(t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT]);
   derivs[2][0][0] = fabsf(p[QUAD_BOTTOM_RIGHT] - p[QUAD_BOTTOM_LEFT]);
      }
      static float
   compute_lambda_3d_explicit_gradients(const struct sp_sampler_view *sview,
               {
      const struct pipe_resource *texture = sview->base.texture;
   const float dsdx = fabsf(derivs[0][0][quad]);
   const float dsdy = fabsf(derivs[0][1][quad]);
   const float dtdx = fabsf(derivs[1][0][quad]);
   const float dtdy = fabsf(derivs[1][1][quad]);
   const float dpdx = fabsf(derivs[2][0][quad]);
   const float dpdy = fabsf(derivs[2][1][quad]);
   const float maxx = MAX2(dsdx, dsdy) * u_minify(texture->width0, sview->base.u.tex.first_level);
   const float maxy = MAX2(dtdx, dtdy) * u_minify(texture->height0, sview->base.u.tex.first_level);
   const float maxz = MAX2(dpdx, dpdy) * u_minify(texture->depth0, sview->base.u.tex.first_level);
               }
         static float
   compute_lambda_3d(const struct sp_sampler_view *sview,
                     {
      float derivs[3][2][TGSI_QUAD_SIZE];
   compute_gradient_3d(s, t, p, derivs);
      }
         static float
   compute_lambda_cube_explicit_gradients(const struct sp_sampler_view *sview,
               {
      const struct pipe_resource *texture = sview->base.texture;
   const float dsdx = fabsf(derivs[0][0][quad]);
   const float dsdy = fabsf(derivs[0][1][quad]);
   const float dtdx = fabsf(derivs[1][0][quad]);
   const float dtdy = fabsf(derivs[1][1][quad]);
   const float dpdx = fabsf(derivs[2][0][quad]);
   const float dpdy = fabsf(derivs[2][1][quad]);
   const float maxx = MAX2(dsdx, dsdy);
   const float maxy = MAX2(dtdx, dtdy);
   const float maxz = MAX2(dpdx, dpdy);
               }
      static float
   compute_lambda_cube(const struct sp_sampler_view *sview,
                     {
      float derivs[3][2][TGSI_QUAD_SIZE];
   compute_gradient_3d(s, t, p, derivs);
      }
      /**
   * Compute lambda for a vertex texture sampler.
   * Since there aren't derivatives to use, just return 0.
   */
   static float
   compute_lambda_vert(const struct sp_sampler_view *sview,
                     {
         }
         compute_lambda_from_grad_func
   softpipe_get_lambda_from_grad_func(const struct pipe_sampler_view *view,
         {
      switch (view->target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
         case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_RECT:
         case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
         case PIPE_TEXTURE_3D:
         default:
      assert(0);
         }
         /**
   * Get a texel from a texture, using the texture tile cache.
   *
   * \param addr  the template tex address containing cube, z, face info.
   * \param x  the x coord of texel within 2D image
   * \param y  the y coord of texel within 2D image
   * \param rgba  the quad to put the texel/color into
   *
   * XXX maybe move this into sp_tex_tile_cache.c and merge with the
   * sp_get_cached_tile_tex() function.
   */
            static inline const float *
   get_texel_buffer_no_border(const struct sp_sampler_view *sp_sview,
         {
      const struct softpipe_tex_cached_tile *tile;
   addr.bits.x = x * elmsize / TEX_TILE_SIZE;
                                 }
         static inline const float *
   get_texel_2d_no_border(const struct sp_sampler_view *sp_sview,
         {
      const struct softpipe_tex_cached_tile *tile;
   addr.bits.x = x / TEX_TILE_SIZE;
   addr.bits.y = y / TEX_TILE_SIZE;
   y %= TEX_TILE_SIZE;
                        }
         static inline const float *
   get_texel_2d(const struct sp_sampler_view *sp_sview,
               {
      const struct pipe_resource *texture = sp_sview->base.texture;
            if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
      y < 0 || y >= (int) u_minify(texture->height0, level)) {
      }
   else {
            }
         /*
   * Here's the complete logic (HOLY CRAP) for finding next face and doing the
   * corresponding coord wrapping, implemented by get_next_face,
   * get_next_xcoord, get_next_ycoord.
   * Read like that (first line):
   * If face is +x and s coord is below zero, then
   * new face is +z, new s is max , new t is old t
   * (max is always cube size - 1).
   *
   * +x s- -> +z: s = max,   t = t
   * +x s+ -> -z: s = 0,     t = t
   * +x t- -> +y: s = max,   t = max-s
   * +x t+ -> -y: s = max,   t = s
   *
   * -x s- -> -z: s = max,   t = t
   * -x s+ -> +z: s = 0,     t = t
   * -x t- -> +y: s = 0,     t = s
   * -x t+ -> -y: s = 0,     t = max-s
   *
   * +y s- -> -x: s = t,     t = 0
   * +y s+ -> +x: s = max-t, t = 0
   * +y t- -> -z: s = max-s, t = 0
   * +y t+ -> +z: s = s,     t = 0
   *
   * -y s- -> -x: s = max-t, t = max
   * -y s+ -> +x: s = t,     t = max
   * -y t- -> +z: s = s,     t = max
   * -y t+ -> -z: s = max-s, t = max
      * +z s- -> -x: s = max,   t = t
   * +z s+ -> +x: s = 0,     t = t
   * +z t- -> +y: s = s,     t = max
   * +z t+ -> -y: s = s,     t = 0
      * -z s- -> +x: s = max,   t = t
   * -z s+ -> -x: s = 0,     t = t
   * -z t- -> +y: s = max-s, t = 0
   * -z t+ -> -y: s = max-s, t = max
   */
         /*
   * seamless cubemap neighbour array.
   * this array is used to find the adjacent face in each of 4 directions,
   * left, right, up, down. (or -x, +x, -y, +y).
   */
   static const unsigned face_array[PIPE_TEX_FACE_MAX][4] = {
      /* pos X first then neg X is Z different, Y the same */
   /* PIPE_TEX_FACE_POS_X,*/
   { PIPE_TEX_FACE_POS_Z, PIPE_TEX_FACE_NEG_Z,
   PIPE_TEX_FACE_POS_Y, PIPE_TEX_FACE_NEG_Y },
   /* PIPE_TEX_FACE_NEG_X */
   { PIPE_TEX_FACE_NEG_Z, PIPE_TEX_FACE_POS_Z,
            /* pos Y first then neg Y is X different, X the same */
   /* PIPE_TEX_FACE_POS_Y */
   { PIPE_TEX_FACE_NEG_X, PIPE_TEX_FACE_POS_X,
            /* PIPE_TEX_FACE_NEG_Y */
   { PIPE_TEX_FACE_NEG_X, PIPE_TEX_FACE_POS_X,
            /* pos Z first then neg Y is X different, X the same */
   /* PIPE_TEX_FACE_POS_Z */
   { PIPE_TEX_FACE_NEG_X, PIPE_TEX_FACE_POS_X,
            /* PIPE_TEX_FACE_NEG_Z */
   { PIPE_TEX_FACE_POS_X, PIPE_TEX_FACE_NEG_X,
      };
      static inline unsigned
   get_next_face(unsigned face, int idx)
   {
         }
      /*
   * return a new xcoord based on old face, old coords, cube size
   * and fall_off_index (0 for x-, 1 for x+, 2 for y-, 3 for y+)
   */
   static inline int
   get_next_xcoord(unsigned face, unsigned fall_off_index, int max, int xc, int yc)
   {
      if ((face == 0 && fall_off_index != 1) ||
      (face == 1 && fall_off_index == 0) ||
   (face == 4 && fall_off_index == 0) ||
   (face == 5 && fall_off_index == 0)) {
      }
   if ((face == 1 && fall_off_index != 0) ||
      (face == 0 && fall_off_index == 1) ||
   (face == 4 && fall_off_index == 1) ||
   (face == 5 && fall_off_index == 1)) {
      }
   if ((face == 4 && fall_off_index >= 2) ||
      (face == 2 && fall_off_index == 3) ||
   (face == 3 && fall_off_index == 2)) {
      }
   if ((face == 5 && fall_off_index >= 2) ||
      (face == 2 && fall_off_index == 2) ||
   (face == 3 && fall_off_index == 3)) {
      }
   if ((face == 2 && fall_off_index == 0) ||
      (face == 3 && fall_off_index == 1)) {
      }
   /* (face == 2 && fall_off_index == 1) ||
            }
      /*
   * return a new ycoord based on old face, old coords, cube size
   * and fall_off_index (0 for x-, 1 for x+, 2 for y-, 3 for y+)
   */
   static inline int
   get_next_ycoord(unsigned face, unsigned fall_off_index, int max, int xc, int yc)
   {
      if ((fall_off_index <= 1) && (face <= 1 || face >= 4)) {
         }
   if (face == 2 ||
      (face == 4 && fall_off_index == 3) ||
   (face == 5 && fall_off_index == 2)) {
      }
   if (face == 3 ||
      (face == 4 && fall_off_index == 2) ||
   (face == 5 && fall_off_index == 3)) {
      }
   if ((face == 0 && fall_off_index == 3) ||
      (face == 1 && fall_off_index == 2)) {
      }
   /* (face == 0 && fall_off_index == 2) ||
            }
         /* Gather a quad of adjacent texels within a tile:
   */
   static inline void
   get_texel_quad_2d_no_border_single_tile(const struct sp_sampler_view *sp_sview,
                     {
               addr.bits.x = x / TEX_TILE_SIZE;
   addr.bits.y = y / TEX_TILE_SIZE;
   y %= TEX_TILE_SIZE;
            tile = sp_get_cached_tile_tex(sp_sview->cache, addr);
         out[0] = &tile->data.color[y  ][x  ][0];
   out[1] = &tile->data.color[y  ][x+1][0];
   out[2] = &tile->data.color[y+1][x  ][0];
      }
         /* Gather a quad of potentially non-adjacent texels:
   */
   static inline void
   get_texel_quad_2d_no_border(const struct sp_sampler_view *sp_sview,
                           {
      out[0] = get_texel_2d_no_border( sp_sview, addr, x0, y0 );
   out[1] = get_texel_2d_no_border( sp_sview, addr, x1, y0 );
   out[2] = get_texel_2d_no_border( sp_sview, addr, x0, y1 );
      }
         /* 3d variants:
   */
   static inline const float *
   get_texel_3d_no_border(const struct sp_sampler_view *sp_sview,
         {
               addr.bits.x = x / TEX_TILE_SIZE;
   addr.bits.y = y / TEX_TILE_SIZE;
   addr.bits.z = z;
   y %= TEX_TILE_SIZE;
                        }
         static inline const float *
   get_texel_3d(const struct sp_sampler_view *sp_sview,
               {
      const struct pipe_resource *texture = sp_sview->base.texture;
            if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
      y < 0 || y >= (int) u_minify(texture->height0, level) ||
   z < 0 || z >= (int) u_minify(texture->depth0, level)) {
      }
   else {
            }
         /* Get texel pointer for 1D array texture */
   static inline const float *
   get_texel_1d_array(const struct sp_sampler_view *sp_sview,
               {
      const struct pipe_resource *texture = sp_sview->base.texture;
            if (x < 0 || x >= (int) u_minify(texture->width0, level)) {
         }
   else {
            }
         /* Get texel pointer for 2D array texture */
   static inline const float *
   get_texel_2d_array(const struct sp_sampler_view *sp_sview,
               {
      const struct pipe_resource *texture = sp_sview->base.texture;
            assert(layer < (int) texture->array_size);
            if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
      y < 0 || y >= (int) u_minify(texture->height0, level)) {
      }
   else {
            }
         static inline const float *
   get_texel_cube_seamless(const struct sp_sampler_view *sp_sview,
               {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const unsigned level = addr.bits.level;
                     assert(texture->width0 == texture->height0);
   new_x = x;
            /* change the face */
   if (x < 0) {
      /*
   * Cheat with corners. They are difficult and I believe because we don't get
   * per-pixel faces we can actually have multiple corner texels per pixel,
   * which screws things up majorly in any case (as the per spec behavior is
   * to average the 3 remaining texels, which we might not have).
   * Hence just make sure that the 2nd coord is clamped, will simply pick the
   * sample which would have fallen off the x coord, but not y coord.
   * So the filter weight of the samples will be wrong, but at least this
   * ensures that only valid texels near the corner are used.
   */
   if (y < 0 || y >= max_x) {
         }
   new_x = get_next_xcoord(face, 0, max_x -1, x, y);
   new_y = get_next_ycoord(face, 0, max_x -1, x, y);
      } else if (x >= max_x) {
      if (y < 0 || y >= max_x) {
         }
   new_x = get_next_xcoord(face, 1, max_x -1, x, y);
   new_y = get_next_ycoord(face, 1, max_x -1, x, y);
      } else if (y < 0) {
      new_x = get_next_xcoord(face, 2, max_x -1, x, y);
   new_y = get_next_ycoord(face, 2, max_x -1, x, y);
      } else if (y >= max_x) {
      new_x = get_next_xcoord(face, 3, max_x -1, x, y);
   new_y = get_next_ycoord(face, 3, max_x -1, x, y);
                  }
         /* Get texel pointer for cube array texture */
   static inline const float *
   get_texel_cube_array(const struct sp_sampler_view *sp_sview,
               {
      const struct pipe_resource *texture = sp_sview->base.texture;
            assert(layer < (int) texture->array_size);
            if (x < 0 || x >= (int) u_minify(texture->width0, level) ||
      y < 0 || y >= (int) u_minify(texture->height0, level)) {
      }
   else {
            }
   /**
   * Given the logbase2 of a mipmap's base level size and a mipmap level,
   * return the size (in texels) of that mipmap level.
   * For example, if level[0].width = 256 then base_pot will be 8.
   * If level = 2, then we'll return 64 (the width at level=2).
   * Return 1 if level > base_pot.
   */
   static inline unsigned
   pot_level_size(unsigned base_pot, unsigned level)
   {
         }
         static void
   print_sample(const char *function, const float *rgba)
   {
      debug_printf("%s %g %g %g %g\n",
            }
         static void
   print_sample_4(const char *function, float rgba[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE])
   {
      debug_printf("%s %g %g %g %g, %g %g %g %g, %g %g %g %g, %g %g %g %g\n",
               function,
   rgba[0][0], rgba[1][0], rgba[2][0], rgba[3][0],
      }
         /* Some image-filter fastpaths:
   */
   static inline void
   img_filter_2d_linear_repeat_POT(const struct sp_sampler_view *sp_sview,
                     {
      const unsigned xpot = pot_level_size(sp_sview->xpot, args->level);
   const unsigned ypot = pot_level_size(sp_sview->ypot, args->level);
   const int xmax = (xpot - 1) & (TEX_TILE_SIZE - 1); /* MIN2(TEX_TILE_SIZE, xpot) - 1; */
   const int ymax = (ypot - 1) & (TEX_TILE_SIZE - 1); /* MIN2(TEX_TILE_SIZE, ypot) - 1; */
   union tex_tile_address addr;
            const float u = (args->s * xpot - 0.5F) + args->offset[0];
            const int uflr = util_ifloor(u);
            const float xw = u - (float)uflr;
            const int x0 = uflr & (xpot - 1);
            const float *tx[4];
         addr.value = 0;
   addr.bits.level = args->level;
            /* Can we fetch all four at once:
   */
   if (x0 < xmax && y0 < ymax) {
         }
   else {
      const unsigned x1 = (x0 + 1) & (xpot - 1);
   const unsigned y1 = (y0 + 1) & (ypot - 1);
               /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_NUM_CHANNELS; c++) {
      rgba[TGSI_NUM_CHANNELS*c] = lerp_2d(xw, yw, 
                     if (DEBUG_TEX) {
            }
         static inline void
   img_filter_2d_nearest_repeat_POT(const struct sp_sampler_view *sp_sview,
                     {
      const unsigned xpot = pot_level_size(sp_sview->xpot, args->level);
   const unsigned ypot = pot_level_size(sp_sview->ypot, args->level);
   const float *out;
   union tex_tile_address addr;
            const float u = args->s * xpot + args->offset[0];
            const int uflr = util_ifloor(u);
            const int x0 = uflr & (xpot - 1);
            addr.value = 0;
   addr.bits.level = args->level;
            out = get_texel_2d_no_border(sp_sview, addr, x0, y0);
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
            if (DEBUG_TEX) {
            }
         static inline void
   img_filter_2d_nearest_clamp_POT(const struct sp_sampler_view *sp_sview,
                     {
      const unsigned xpot = pot_level_size(sp_sview->xpot, args->level);
   const unsigned ypot = pot_level_size(sp_sview->ypot, args->level);
   union tex_tile_address addr;
            const float u = args->s * xpot + args->offset[0];
            int x0, y0;
            addr.value = 0;
   addr.bits.level = args->level;
            x0 = util_ifloor(u);
   if (x0 < 0) 
         else if (x0 > (int) xpot - 1)
            y0 = util_ifloor(v);
   if (y0 < 0) 
         else if (y0 > (int) ypot - 1)
            out = get_texel_2d_no_border(sp_sview, addr, x0, y0);
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
            if (DEBUG_TEX) {
            }
         static void
   img_filter_1d_nearest(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   int x;
   union tex_tile_address addr;
   const float *out;
                     addr.value = 0;
                     out = get_texel_1d_array(sp_sview, sp_samp, addr, x,
         for (c = 0; c < TGSI_NUM_CHANNELS; c++)
            if (DEBUG_TEX) {
            }
         static void
   img_filter_1d_array_nearest(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int layer = coord_to_layer(args->t, sp_sview->base.u.tex.first_layer,
         int x;
   union tex_tile_address addr;
   const float *out;
                     addr.value = 0;
                     out = get_texel_1d_array(sp_sview, sp_samp, addr, x, layer);
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
            if (DEBUG_TEX) {
            }
         static void
   img_filter_2d_nearest(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int height = u_minify(texture->height0, args->level);
   int x, y;
   union tex_tile_address addr;
   const float *out;
            assert(width > 0);
            addr.value = 0;
   addr.bits.level = args->level;
            sp_samp->nearest_texcoord_s(args->s, width, args->offset[0], &x);
            out = get_texel_2d(sp_sview, sp_samp, addr, x, y);
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
            if (DEBUG_TEX) {
            }
         static void
   img_filter_2d_array_nearest(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int height = u_minify(texture->height0, args->level);
   const int layer = coord_to_layer(args->p, sp_sview->base.u.tex.first_layer,
         int x, y;
   union tex_tile_address addr;
   const float *out;
            assert(width > 0);
            addr.value = 0;
            sp_samp->nearest_texcoord_s(args->s, width, args->offset[0], &x);
            out = get_texel_2d_array(sp_sview, sp_samp, addr, x, y, layer);
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
            if (DEBUG_TEX) {
            }
         static void
   img_filter_cube_nearest(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int height = u_minify(texture->height0, args->level);
   const int layerface = args->face_id + sp_sview->base.u.tex.first_layer;
   int x, y;
   union tex_tile_address addr;
   const float *out;
            assert(width > 0);
            addr.value = 0;
            /*
   * If NEAREST filtering is done within a miplevel, always apply wrap
   * mode CLAMP_TO_EDGE.
   */
   if (sp_samp->base.seamless_cube_map) {
      wrap_nearest_clamp_to_edge(args->s, width, args->offset[0], &x);
      } else {
      /* Would probably make sense to ignore mode and just do edge clamp */
   sp_samp->nearest_texcoord_s(args->s, width, args->offset[0], &x);
               out = get_texel_cube_array(sp_sview, sp_samp, addr, x, y, layerface);
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
            if (DEBUG_TEX) {
            }
      static void
   img_filter_cube_array_nearest(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int height = u_minify(texture->height0, args->level);
   const int layerface = CLAMP(6 * util_ifloor(args->p + 0.5f) + sp_sview->base.u.tex.first_layer,
               int x, y;
   union tex_tile_address addr;
   const float *out;
            assert(width > 0);
            addr.value = 0;
            sp_samp->nearest_texcoord_s(args->s, width, args->offset[0], &x);
            out = get_texel_cube_array(sp_sview, sp_samp, addr, x, y, layerface);
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
            if (DEBUG_TEX) {
            }
      static void
   img_filter_3d_nearest(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int height = u_minify(texture->height0, args->level);
   const int depth = u_minify(texture->depth0, args->level);
   int x, y, z;
   union tex_tile_address addr;
   const float *out;
            assert(width > 0);
   assert(height > 0);
            sp_samp->nearest_texcoord_s(args->s, width,  args->offset[0], &x);
   sp_samp->nearest_texcoord_t(args->t, height, args->offset[1], &y);
            addr.value = 0;
            out = get_texel_3d(sp_sview, sp_samp, addr, x, y, z);
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      }
         static void
   img_filter_1d_linear(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   int x0, x1;
   float xw; /* weights */
   union tex_tile_address addr;
   const float *tx0, *tx1;
                     addr.value = 0;
                     tx0 = get_texel_1d_array(sp_sview, sp_samp, addr, x0,
         tx1 = get_texel_1d_array(sp_sview, sp_samp, addr, x1,
            /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      }
         static void
   img_filter_1d_array_linear(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int layer = coord_to_layer(args->t, sp_sview->base.u.tex.first_layer,
         int x0, x1;
   float xw; /* weights */
   union tex_tile_address addr;
   const float *tx0, *tx1;
                     addr.value = 0;
                     tx0 = get_texel_1d_array(sp_sview, sp_samp, addr, x0, layer);
            /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      }
      /*
   * Retrieve the gathered value, need to convert to the
   * TGSI expected interface, and take component select
   * and swizzling into account.
   */
   static float
   get_gather_value(const struct sp_sampler_view *sp_sview,
               {
      int chan;
            /*
   * softpipe samples in a different order
   * to TGSI expects, so we need to swizzle,
   * the samples into the correct slots.
   */
   switch (chan_in) {
   case 0:
      chan = 2;
      case 1:
      chan = 3;
      case 2:
      chan = 1;
      case 3:
      chan = 0;
      default:
      assert(0);
               /* pick which component to use for the swizzle */
   switch (comp_sel) {
   case 0:
      swizzle = sp_sview->base.swizzle_r;
      case 1:
      swizzle = sp_sview->base.swizzle_g;
      case 2:
      swizzle = sp_sview->base.swizzle_b;
      case 3:
      swizzle = sp_sview->base.swizzle_a;
      default:
      assert(0);
               /* get correct result using the channel and swizzle */
   switch (swizzle) {
   case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
         default:
            }
         static void
   img_filter_2d_linear(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int height = u_minify(texture->height0, args->level);
   int x0, y0, x1, y1;
   float xw, yw; /* weights */
   union tex_tile_address addr;
   const float *tx[4];
            assert(width > 0);
            addr.value = 0;
   addr.bits.level = args->level;
            sp_samp->linear_texcoord_s(args->s, width,  args->offset[0], &x0, &x1, &xw);
            tx[0] = get_texel_2d(sp_sview, sp_samp, addr, x0, y0);
   tx[1] = get_texel_2d(sp_sview, sp_samp, addr, x1, y0);
   tx[2] = get_texel_2d(sp_sview, sp_samp, addr, x0, y1);
            if (args->gather_only) {
      for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      rgba[TGSI_NUM_CHANNELS*c] = get_gather_value(sp_sview, c,
         } else {
      /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      rgba[TGSI_NUM_CHANNELS*c] = lerp_2d(xw, yw,
            }
         static void
   img_filter_2d_array_linear(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int height = u_minify(texture->height0, args->level);
   const int layer = coord_to_layer(args->p, sp_sview->base.u.tex.first_layer,
         int x0, y0, x1, y1;
   float xw, yw; /* weights */
   union tex_tile_address addr;
   const float *tx[4];
            assert(width > 0);
            addr.value = 0;
            sp_samp->linear_texcoord_s(args->s, width,  args->offset[0], &x0, &x1, &xw);
            tx[0] = get_texel_2d_array(sp_sview, sp_samp, addr, x0, y0, layer);
   tx[1] = get_texel_2d_array(sp_sview, sp_samp, addr, x1, y0, layer);
   tx[2] = get_texel_2d_array(sp_sview, sp_samp, addr, x0, y1, layer);
            if (args->gather_only) {
      for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      rgba[TGSI_NUM_CHANNELS*c] = get_gather_value(sp_sview, c,
         } else {
      /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      rgba[TGSI_NUM_CHANNELS*c] = lerp_2d(xw, yw,
            }
         static void
   img_filter_cube_linear(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int height = u_minify(texture->height0, args->level);
   const int layer = sp_sview->base.u.tex.first_layer;
   int x0, y0, x1, y1;
   float xw, yw; /* weights */
   union tex_tile_address addr;
   const float *tx[4];
   float corner0[TGSI_QUAD_SIZE], corner1[TGSI_QUAD_SIZE],
                  assert(width > 0);
            addr.value = 0;
            /*
   * For seamless if LINEAR filtering is done within a miplevel,
   * always apply wrap mode CLAMP_TO_BORDER.
   */
   if (sp_samp->base.seamless_cube_map) {
      /* Note this is a bit overkill, actual clamping is not required */
   wrap_linear_clamp_to_border(args->s, width, args->offset[0], &x0, &x1, &xw);
      } else {
      /* Would probably make sense to ignore mode and just do edge clamp */
   sp_samp->linear_texcoord_s(args->s, width,  args->offset[0], &x0, &x1, &xw);
               if (sp_samp->base.seamless_cube_map) {
      tx[0] = get_texel_cube_seamless(sp_sview, addr, x0, y0, corner0, layer, args->face_id);
   tx[1] = get_texel_cube_seamless(sp_sview, addr, x1, y0, corner1, layer, args->face_id);
   tx[2] = get_texel_cube_seamless(sp_sview, addr, x0, y1, corner2, layer, args->face_id);
      } else {
      tx[0] = get_texel_cube_array(sp_sview, sp_samp, addr, x0, y0, layer + args->face_id);
   tx[1] = get_texel_cube_array(sp_sview, sp_samp, addr, x1, y0, layer + args->face_id);
   tx[2] = get_texel_cube_array(sp_sview, sp_samp, addr, x0, y1, layer + args->face_id);
               if (args->gather_only) {
      for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      rgba[TGSI_NUM_CHANNELS*c] = get_gather_value(sp_sview, c,
         } else {
      /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      rgba[TGSI_NUM_CHANNELS*c] = lerp_2d(xw, yw,
            }
         static void
   img_filter_cube_array_linear(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
            const int layer = CLAMP(6 * util_ifloor(args->p + 0.5f) + sp_sview->base.u.tex.first_layer,
                  int x0, y0, x1, y1;
   float xw, yw; /* weights */
   union tex_tile_address addr;
   const float *tx[4];
   float corner0[TGSI_QUAD_SIZE], corner1[TGSI_QUAD_SIZE],
                  assert(width > 0);
            addr.value = 0;
            /*
   * For seamless if LINEAR filtering is done within a miplevel,
   * always apply wrap mode CLAMP_TO_BORDER.
   */
   if (sp_samp->base.seamless_cube_map) {
      /* Note this is a bit overkill, actual clamping is not required */
   wrap_linear_clamp_to_border(args->s, width, args->offset[0], &x0, &x1, &xw);
      } else {
      /* Would probably make sense to ignore mode and just do edge clamp */
   sp_samp->linear_texcoord_s(args->s, width,  args->offset[0], &x0, &x1, &xw);
               if (sp_samp->base.seamless_cube_map) {
      tx[0] = get_texel_cube_seamless(sp_sview, addr, x0, y0, corner0, layer, args->face_id);
   tx[1] = get_texel_cube_seamless(sp_sview, addr, x1, y0, corner1, layer, args->face_id);
   tx[2] = get_texel_cube_seamless(sp_sview, addr, x0, y1, corner2, layer, args->face_id);
      } else {
      tx[0] = get_texel_cube_array(sp_sview, sp_samp, addr, x0, y0, layer + args->face_id);
   tx[1] = get_texel_cube_array(sp_sview, sp_samp, addr, x1, y0, layer + args->face_id);
   tx[2] = get_texel_cube_array(sp_sview, sp_samp, addr, x0, y1, layer + args->face_id);
               if (args->gather_only) {
      for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      rgba[TGSI_NUM_CHANNELS*c] = get_gather_value(sp_sview, c,
         } else {
      /* interpolate R, G, B, A */
   for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      rgba[TGSI_NUM_CHANNELS*c] = lerp_2d(xw, yw,
            }
      static void
   img_filter_3d_linear(const struct sp_sampler_view *sp_sview,
                     {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const int width = u_minify(texture->width0, args->level);
   const int height = u_minify(texture->height0, args->level);
   const int depth = u_minify(texture->depth0, args->level);
   int x0, x1, y0, y1, z0, z1;
   float xw, yw, zw; /* interpolation weights */
   union tex_tile_address addr;
   const float *tx00, *tx01, *tx02, *tx03, *tx10, *tx11, *tx12, *tx13;
            addr.value = 0;
            assert(width > 0);
   assert(height > 0);
            sp_samp->linear_texcoord_s(args->s, width,  args->offset[0], &x0, &x1, &xw);
   sp_samp->linear_texcoord_t(args->t, height, args->offset[1], &y0, &y1, &yw);
            tx00 = get_texel_3d(sp_sview, sp_samp, addr, x0, y0, z0);
   tx01 = get_texel_3d(sp_sview, sp_samp, addr, x1, y0, z0);
   tx02 = get_texel_3d(sp_sview, sp_samp, addr, x0, y1, z0);
   tx03 = get_texel_3d(sp_sview, sp_samp, addr, x1, y1, z0);
         tx10 = get_texel_3d(sp_sview, sp_samp, addr, x0, y0, z1);
   tx11 = get_texel_3d(sp_sview, sp_samp, addr, x1, y0, z1);
   tx12 = get_texel_3d(sp_sview, sp_samp, addr, x0, y1, z1);
   tx13 = get_texel_3d(sp_sview, sp_samp, addr, x1, y1, z1);
            for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      rgba[TGSI_NUM_CHANNELS*c] =  lerp_3d(xw, yw, zw,
                     }
         /* Calculate level of detail for every fragment,
   * with lambda already computed.
   * Note that lambda has already been biased by global LOD bias.
   * \param biased_lambda per-quad lambda.
   * \param lod_in per-fragment lod_bias or explicit_lod.
   * \param lod returns the per-fragment lod.
   */
   static inline void
   compute_lod(const struct pipe_sampler_state *sampler,
               enum tgsi_sampler_control control,
   const float biased_lambda,
   {
      const float min_lod = sampler->min_lod;
   const float max_lod = sampler->max_lod;
            switch (control) {
   case TGSI_SAMPLER_LOD_NONE:
   case TGSI_SAMPLER_LOD_ZERO:
      lod[0] = lod[1] = lod[2] = lod[3] = CLAMP(biased_lambda, min_lod, max_lod);
      case TGSI_SAMPLER_DERIVS_EXPLICIT:
      for (i = 0; i < TGSI_QUAD_SIZE; i++)
            case TGSI_SAMPLER_LOD_BIAS:
      for (i = 0; i < TGSI_QUAD_SIZE; i++) {
      lod[i] = biased_lambda + lod_in[i];
      }
      case TGSI_SAMPLER_LOD_EXPLICIT:
      for (i = 0; i < TGSI_QUAD_SIZE; i++) {
         }
      default:
      assert(0);
         }
         /* Calculate level of detail for every fragment. The computed value is not
   * clamped to lod_min and lod_max.
   * \param lod_in per-fragment lod_bias or explicit_lod.
   * \param lod results per-fragment lod.
   */
   static inline void
   compute_lambda_lod_unclamped(const struct sp_sampler_view *sp_sview,
                              const struct sp_sampler *sp_samp,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
      {
      const struct pipe_sampler_state *sampler = &sp_samp->base;
   const float lod_bias = sampler->lod_bias;
   float lambda;
            switch (control) {
   case TGSI_SAMPLER_LOD_NONE:
      lambda = sp_sview->compute_lambda(sp_sview, s, t, p) + lod_bias;
   lod[0] = lod[1] = lod[2] = lod[3] = lambda;
      case TGSI_SAMPLER_DERIVS_EXPLICIT:
      for (i = 0; i < TGSI_QUAD_SIZE; i++)
            case TGSI_SAMPLER_LOD_BIAS:
      lambda = sp_sview->compute_lambda(sp_sview, s, t, p) + lod_bias;
   for (i = 0; i < TGSI_QUAD_SIZE; i++) {
         }
      case TGSI_SAMPLER_LOD_EXPLICIT:
      for (i = 0; i < TGSI_QUAD_SIZE; i++) {
         }
      case TGSI_SAMPLER_LOD_ZERO:
   case TGSI_SAMPLER_GATHER:
      lod[0] = lod[1] = lod[2] = lod[3] = lod_bias;
      default:
      assert(0);
         }
      /* Calculate level of detail for every fragment.
   * \param lod_in per-fragment lod_bias or explicit_lod.
   * \param lod results per-fragment lod.
   */
   static inline void
   compute_lambda_lod(const struct sp_sampler_view *sp_sview,
                     const struct sp_sampler *sp_samp,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   float derivs[3][2][TGSI_QUAD_SIZE],
   {
      const struct pipe_sampler_state *sampler = &sp_samp->base;
   const float min_lod = sampler->min_lod;
   const float max_lod = sampler->max_lod;
            compute_lambda_lod_unclamped(sp_sview, sp_samp,
         for (i = 0; i < TGSI_QUAD_SIZE; i++) {
            }
      static inline unsigned
   get_gather_component(const float lod_in[TGSI_QUAD_SIZE])
   {
      /* gather component is stored in lod_in slot as unsigned */
      }
      /**
   * Clamps given lod to both lod limits and mip level limits. Clamping to the
   * latter limits is done so that lod is relative to the first (base) level.
   */
   static void
   clamp_lod(const struct sp_sampler_view *sp_sview,
            const struct sp_sampler *sp_samp,
      {
      const float min_lod = sp_samp->base.min_lod;
   const float max_lod = sp_samp->base.max_lod;
   const float min_level = sp_sview->base.u.tex.first_level;
   const float max_level = sp_sview->base.u.tex.last_level;
            for (i = 0; i < TGSI_QUAD_SIZE; i++) {
               cl = CLAMP(cl, min_lod, max_lod);
   cl = CLAMP(cl, 0, max_level - min_level);
         }
      /**
   * Get mip level relative to base level for linear mip filter
   */
   static void
   mip_rel_level_linear(const struct sp_sampler_view *sp_sview,
                     {
         }
      static void
   mip_filter_linear(const struct sp_sampler_view *sp_sview,
                     const struct sp_sampler *sp_samp,
   img_filter_func min_filter,
   img_filter_func mag_filter,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   int gather_comp,
   {
      const struct pipe_sampler_view *psview = &sp_sview->base;
   int j;
            args.offset = filt_args->offset;
   args.gather_only = filt_args->control == TGSI_SAMPLER_GATHER;
            for (j = 0; j < TGSI_QUAD_SIZE; j++) {
               args.s = s[j];
   args.t = t[j];
   args.p = p[j];
            if (lod[j] <= 0.0 && !args.gather_only) {
      args.level = psview->u.tex.first_level;
      }
   else if (level0 >= (int) psview->u.tex.last_level) {
      args.level = psview->u.tex.last_level;
      }
   else {
      float levelBlend = frac(lod[j]);
                  args.level = level0;
   min_filter(sp_sview, sp_samp, &args, &rgbax[0][0]);
                  for (c = 0; c < 4; c++) {
                        if (DEBUG_TEX) {
            }
         /**
   * Get mip level relative to base level for nearest mip filter
   */
   static void
   mip_rel_level_nearest(const struct sp_sampler_view *sp_sview,
                     {
               clamp_lod(sp_sview, sp_samp, lod, level);
   for (j = 0; j < TGSI_QUAD_SIZE; j++)
      /* TODO: It should rather be:
   * level[j] = ceil(level[j] + 0.5F) - 1.0F;
   */
   }
      /**
   * Compute nearest mipmap level from texcoords.
   * Then sample the texture level for four elements of a quad.
   * \param c0  the LOD bias factors, or absolute LODs (depending on control)
   */
   static void
   mip_filter_nearest(const struct sp_sampler_view *sp_sview,
                     const struct sp_sampler *sp_samp,
   img_filter_func min_filter,
   img_filter_func mag_filter,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   int gather_component,
   {
      const struct pipe_sampler_view *psview = &sp_sview->base;
   int j;
            args.offset = filt_args->offset;
   args.gather_only = filt_args->control == TGSI_SAMPLER_GATHER;
            for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      args.s = s[j];
   args.t = t[j];
   args.p = p[j];
            if (lod[j] <= 0.0f && !args.gather_only) {
      args.level = psview->u.tex.first_level;
      } else {
      const int level = psview->u.tex.first_level + (int)(lod[j] + 0.5F);
   args.level = MIN2(level, (int)psview->u.tex.last_level);
                  if (DEBUG_TEX) {
            }
         /**
   * Get mip level relative to base level for none mip filter
   */
   static void
   mip_rel_level_none(const struct sp_sampler_view *sp_sview,
                     {
               for (j = 0; j < TGSI_QUAD_SIZE; j++) {
            }
      static void
   mip_filter_none(const struct sp_sampler_view *sp_sview,
                  const struct sp_sampler *sp_samp,
   img_filter_func min_filter,
   img_filter_func mag_filter,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   int gather_component,
      {
      int j;
            args.level = sp_sview->base.u.tex.first_level;
   args.offset = filt_args->offset;
   args.gather_only = filt_args->control == TGSI_SAMPLER_GATHER;
            for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      args.s = s[j];
   args.t = t[j];
   args.p = p[j];
   args.face_id = filt_args->faces[j];
   if (lod[j] <= 0.0f && !args.gather_only) {
         }
   else {
               }
         /**
   * Get mip level relative to base level for none mip filter
   */
   static void
   mip_rel_level_none_no_filter_select(const struct sp_sampler_view *sp_sview,
                     {
         }
      static void
   mip_filter_none_no_filter_select(const struct sp_sampler_view *sp_sview,
                                    const struct sp_sampler *sp_samp,
   img_filter_func min_filter,
   img_filter_func mag_filter,
   const float s[TGSI_QUAD_SIZE],
      {
      int j;
   struct img_filter_args args;
   args.level = sp_sview->base.u.tex.first_level;
   args.offset = filt_args->offset;
   args.gather_only = filt_args->control == TGSI_SAMPLER_GATHER;
   args.gather_comp = gather_comp;
   for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      args.s = s[j];
   args.t = t[j];
   args.p = p[j];
   args.face_id = filt_args->faces[j];
         }
         /* For anisotropic filtering */
   #define WEIGHT_LUT_SIZE 1024
      static const float *weightLut = NULL;
      /**
   * Creates the look-up table used to speed-up EWA sampling
   */
   static void
   create_filter_table(void)
   {
      unsigned i;
   if (!weightLut) {
               for (i = 0; i < WEIGHT_LUT_SIZE; ++i) {
      const float alpha = 2;
   const float r2 = (float) i / (float) (WEIGHT_LUT_SIZE - 1);
   const float weight = (float) expf(-alpha * r2);
      }
         }
         /**
   * Elliptical weighted average (EWA) filter for producing high quality
   * anisotropic filtered results.
   * Based on the Higher Quality Elliptical Weighted Average Filter
   * published by Paul S. Heckbert in his Master's Thesis
   * "Fundamentals of Texture Mapping and Image Warping" (1989)
   */
   static void
   img_filter_2d_ewa(const struct sp_sampler_view *sp_sview,
                     const struct sp_sampler *sp_samp,
   img_filter_func min_filter,
   img_filter_func mag_filter,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   const uint faces[TGSI_QUAD_SIZE],
   const int8_t *offset,
   unsigned level,
   {
               // ??? Won't the image filters blow up if level is negative?
   const unsigned level0 = level > 0 ? level : 0;
   const float scaling = 1.0f / (1 << level0);
   const int width = u_minify(texture->width0, level0);
   const int height = u_minify(texture->height0, level0);
   struct img_filter_args args;
   const float ux = dudx * scaling;
   const float vx = dvdx * scaling;
   const float uy = dudy * scaling;
            /* compute ellipse coefficients to bound the region: 
   * A*x*x + B*x*y + C*y*y = F.
   */
   float A = vx*vx+vy*vy+1;
   float B = -2*(ux*vx+uy*vy);
   float C = ux*ux+uy*uy+1;
            /* check if it is an ellipse */
            /* Compute the ellipse's (u,v) bounding box in texture space */
   const float d = -B*B+4.0f*C*A;
   const float box_u = 2.0f / d * sqrtf(d*C*F); /* box_u -> half of bbox with   */
            float rgba_temp[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE];
   float s_buffer[TGSI_QUAD_SIZE];
   float t_buffer[TGSI_QUAD_SIZE];
   float weight_buffer[TGSI_QUAD_SIZE];
            /* Scale ellipse formula to directly index the Filter Lookup Table.
   * i.e. scale so that F = WEIGHT_LUT_SIZE-1
   */
   const double formScale = (double) (WEIGHT_LUT_SIZE - 1) / F;
   A *= formScale;
   B *= formScale;
   C *= formScale;
            /* For each quad, the du and dx values are the same and so the ellipse is
   * also the same. Note that texel/image access can only be performed using
   * a quad, i.e. it is not possible to get the pixel value for a single
   * tex coord. In order to have a better performance, the access is buffered
   * using the s_buffer/t_buffer and weight_buffer. Only when the buffer is
   * full, then the pixel values are read from the image.
   */
            args.level = level;
            for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      /* Heckbert MS thesis, p. 59; scan over the bounding box of the ellipse
   * and incrementally update the value of Ax^2+Bxy*Cy^2; when this
   * value, q, is less than F, we're inside the ellipse
   */
   const float tex_u = -0.5F + s[j] * texture->width0 * scaling;
            const int u0 = (int) floorf(tex_u - box_u);
   const int u1 = (int) ceilf(tex_u + box_u);
   const int v0 = (int) floorf(tex_v - box_v);
   const int v1 = (int) ceilf(tex_v + box_v);
            float num[4] = {0.0F, 0.0F, 0.0F, 0.0F};
   unsigned buffer_next = 0;
   float den = 0;
   int v;
            for (v = v0; v <= v1; ++v) {
      const float V = v - tex_v;
                  int u;
   for (u = u0; u <= u1; ++u) {
      /* Note that the ellipse has been pre-scaled so F =
   * WEIGHT_LUT_SIZE - 1
   */
   if (q < WEIGHT_LUT_SIZE) {
      /* as a LUT is used, q must never be negative;
   * should not happen, though
                        weight_buffer[buffer_next] = weight;
   s_buffer[buffer_next] = u / ((float) width);
            buffer_next++;
   if (buffer_next == TGSI_QUAD_SIZE) {
      /* 4 texel coords are in the buffer -> read it now */
   unsigned jj;
   /* it is assumed that samp->min_img_filter is set to
   * img_filter_2d_nearest or one of the
   * accelerated img_filter_2d_nearest_XXX functions.
   */
   for (jj = 0; jj < buffer_next; jj++) {
      args.s = s_buffer[jj];
   args.t = t_buffer[jj];
   args.p = p[jj];
   min_filter(sp_sview, sp_samp, &args, &rgba_temp[0][jj]);
   num[0] += weight_buffer[jj] * rgba_temp[0][jj];
                                          }
   q += dq;
                  /* if the tex coord buffer contains unread values, we will read
   * them now.
   */
   if (buffer_next > 0) {
      unsigned jj;
   /* it is assumed that samp->min_img_filter is set to
   * img_filter_2d_nearest or one of the
   * accelerated img_filter_2d_nearest_XXX functions.
   */
   for (jj = 0; jj < buffer_next; jj++) {
      args.s = s_buffer[jj];
   args.t = t_buffer[jj];
   args.p = p[jj];
   min_filter(sp_sview, sp_samp, &args, &rgba_temp[0][jj]);
   num[0] += weight_buffer[jj] * rgba_temp[0][jj];
   num[1] += weight_buffer[jj] * rgba_temp[1][jj];
   num[2] += weight_buffer[jj] * rgba_temp[2][jj];
                  if (den <= 0.0F) {
      /* Reaching this place would mean that no pixels intersected
   * the ellipse.  This should never happen because the filter
                  /*rgba[0]=0;
   rgba[1]=0;
   rgba[2]=0;
   rgba[3]=0;*/
   /* not enough pixels in resampling, resort to direct interpolation */
   args.s = s[j];
   args.t = t[j];
   args.p = p[j];
   min_filter(sp_sview, sp_samp, &args, &rgba_temp[0][j]);
   den = 1;
   num[0] = rgba_temp[0][j];
   num[1] = rgba_temp[1][j];
   num[2] = rgba_temp[2][j];
               rgba[0][j] = num[0] / den;
   rgba[1][j] = num[1] / den;
   rgba[2][j] = num[2] / den;
         }
         /**
   * Get mip level relative to base level for linear mip filter
   */
   static void
   mip_rel_level_linear_aniso(const struct sp_sampler_view *sp_sview,
                     {
         }
      /**
   * Sample 2D texture using an anisotropic filter.
   */
   static void
   mip_filter_linear_aniso(const struct sp_sampler_view *sp_sview,
                           const struct sp_sampler *sp_samp,
   img_filter_func min_filter,
   img_filter_func mag_filter,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   {
      const struct pipe_resource *texture = sp_sview->base.texture;
   const struct pipe_sampler_view *psview = &sp_sview->base;
   int level0;
   float lambda;
            const float s_to_u = u_minify(texture->width0, psview->u.tex.first_level);
   const float t_to_v = u_minify(texture->height0, psview->u.tex.first_level);
   const float dudx = (s[QUAD_BOTTOM_RIGHT] - s[QUAD_BOTTOM_LEFT]) * s_to_u;
   const float dudy = (s[QUAD_TOP_LEFT]     - s[QUAD_BOTTOM_LEFT]) * s_to_u;
   const float dvdx = (t[QUAD_BOTTOM_RIGHT] - t[QUAD_BOTTOM_LEFT]) * t_to_v;
   const float dvdy = (t[QUAD_TOP_LEFT]     - t[QUAD_BOTTOM_LEFT]) * t_to_v;
                     if (filt_args->control == TGSI_SAMPLER_LOD_BIAS ||
      filt_args->control == TGSI_SAMPLER_LOD_NONE ||
   /* XXX FIXME */
   filt_args->control == TGSI_SAMPLER_DERIVS_EXPLICIT) {
   /* note: instead of working with Px and Py, we will use the 
   * squared length instead, to avoid sqrt.
   */
   const float Px2 = dudx * dudx + dvdx * dvdx;
            float Pmax2;
   float Pmin2;
   float e;
   const float maxEccentricity = sp_samp->base.max_anisotropy * sp_samp->base.max_anisotropy;
      if (Px2 < Py2) {
      Pmax2 = Py2;
      }
   else {
      Pmax2 = Px2;
      }
      /* if the eccentricity of the ellipse is too big, scale up the shorter
   * of the two vectors to limit the maximum amount of work per pixel
   */
   e = Pmax2 / Pmin2;
   if (e > maxEccentricity) {
      /* float s=e / maxEccentricity;
      minor[0] *= s;
   minor[1] *= s;
         }
      /* note: we need to have Pmin=sqrt(Pmin2) here, but we can avoid
   * this since 0.5*log(x) = log(sqrt(x))
   */
   lambda = 0.5F * util_fast_log2(Pmin2) + sp_samp->base.lod_bias;
      }
   else {
      assert(filt_args->control == TGSI_SAMPLER_LOD_EXPLICIT ||
            }
      /* XXX: Take into account all lod values.
   */
   lambda = lod[0];
            /* If the ellipse covers the whole image, we can
   * simply return the average of the whole image.
   */
   if (level0 >= (int) psview->u.tex.last_level) {
      int j;
   for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      args.s = s[j];
   args.t = t[j];
   args.p = p[j];
   args.level = psview->u.tex.last_level;
   args.face_id = filt_args->faces[j];
   /*
   * XXX: we overwrote any linear filter with nearest, so this
   * isn't right (albeit if last level is 1x1 and no border it
   * will work just the same).
   */
         }
   else {
      /* don't bother interpolating between multiple LODs; it doesn't
   * seem to be worth the extra running time.
   */
   img_filter_2d_ewa(sp_sview, sp_samp, min_filter, mag_filter,
                     if (DEBUG_TEX) {
            }
      /**
   * Get mip level relative to base level for linear mip filter
   */
   static void
   mip_rel_level_linear_2d_linear_repeat_POT(
      const struct sp_sampler_view *sp_sview,
   const struct sp_sampler *sp_samp,
   const float lod[TGSI_QUAD_SIZE],
      {
         }
      /**
   * Specialized version of mip_filter_linear with hard-wired calls to
   * 2d lambda calculation and 2d_linear_repeat_POT img filters.
   */
   static void
   mip_filter_linear_2d_linear_repeat_POT(
      const struct sp_sampler_view *sp_sview,
   const struct sp_sampler *sp_samp,
   img_filter_func min_filter,
   img_filter_func mag_filter,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   int gather_comp,
   const float lod[TGSI_QUAD_SIZE],
   const struct filter_args *filt_args,
      {
      const struct pipe_sampler_view *psview = &sp_sview->base;
            for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      const int level0 = psview->u.tex.first_level + (int)lod[j];
   struct img_filter_args args;
   /* Catches both negative and large values of level0:
   */
   args.s = s[j];
   args.t = t[j];
   args.p = p[j];
   args.face_id = filt_args->faces[j];
   args.offset = filt_args->offset;
   args.gather_only = filt_args->control == TGSI_SAMPLER_GATHER;
   args.gather_comp = gather_comp;
   if ((unsigned)level0 >= psview->u.tex.last_level) {
      if (level0 < 0)
         else
                     }
   else {
      const float levelBlend = frac(lod[j]);
                  args.level = level0;
   img_filter_2d_linear_repeat_POT(sp_sview, sp_samp, &args, &rgbax[0][0]);
                  for (c = 0; c < TGSI_NUM_CHANNELS; c++)
                  if (DEBUG_TEX) {
            }
      static const struct sp_filter_funcs funcs_linear = {
      mip_rel_level_linear,
      };
      static const struct sp_filter_funcs funcs_nearest = {
      mip_rel_level_nearest,
      };
      static const struct sp_filter_funcs funcs_none = {
      mip_rel_level_none,
      };
      static const struct sp_filter_funcs funcs_none_no_filter_select = {
      mip_rel_level_none_no_filter_select,
      };
      static const struct sp_filter_funcs funcs_linear_aniso = {
      mip_rel_level_linear_aniso,
      };
      static const struct sp_filter_funcs funcs_linear_2d_linear_repeat_POT = {
      mip_rel_level_linear_2d_linear_repeat_POT,
      };
      /**
   * Do shadow/depth comparisons.
   */
   static void
   sample_compare(const struct sp_sampler_view *sp_sview,
                  const struct sp_sampler *sp_samp,
      {
      const struct pipe_sampler_state *sampler = &sp_samp->base;
   int j, v;
   int k[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE];
   float pc[4];
   const struct util_format_description *format_desc =
         /* not entirely sure we couldn't end up with non-valid swizzle here */
   const unsigned chan_type =
      format_desc->swizzle[0] <= PIPE_SWIZZLE_W ?
   format_desc->channel[format_desc->swizzle[0]].type :
               /**
   * Compare texcoord 'p' (aka R) against texture value 'rgba[0]'
   * for 2D Array texture we need to use the 'c0' (aka Q).
   * When we sampled the depth texture, the depth value was put into all
   * RGBA channels.  We look at the red channel here.
                  if (chan_type != UTIL_FORMAT_TYPE_FLOAT) {
      /*
   * clamping is a result of conversion to texture format, hence
   * doesn't happen with floats. Technically also should do comparison
   * in texture format (quantization!).
   */
   pc[0] = CLAMP(c0[0], 0.0F, 1.0F);
   pc[1] = CLAMP(c0[1], 0.0F, 1.0F);
   pc[2] = CLAMP(c0[2], 0.0F, 1.0F);
      } else {
      pc[0] = c0[0];
   pc[1] = c0[1];
   pc[2] = c0[2];
               for (v = 0; v < (is_gather ? TGSI_NUM_CHANNELS : 1); v++) {
      /* compare four texcoords vs. four texture samples */
   switch (sampler->compare_func) {
   case PIPE_FUNC_LESS:
      k[v][0] = pc[0] < rgba[v][0];
   k[v][1] = pc[1] < rgba[v][1];
   k[v][2] = pc[2] < rgba[v][2];
   k[v][3] = pc[3] < rgba[v][3];
      case PIPE_FUNC_LEQUAL:
      k[v][0] = pc[0] <= rgba[v][0];
   k[v][1] = pc[1] <= rgba[v][1];
   k[v][2] = pc[2] <= rgba[v][2];
   k[v][3] = pc[3] <= rgba[v][3];
      case PIPE_FUNC_GREATER:
      k[v][0] = pc[0] > rgba[v][0];
   k[v][1] = pc[1] > rgba[v][1];
   k[v][2] = pc[2] > rgba[v][2];
   k[v][3] = pc[3] > rgba[v][3];
      case PIPE_FUNC_GEQUAL:
      k[v][0] = pc[0] >= rgba[v][0];
   k[v][1] = pc[1] >= rgba[v][1];
   k[v][2] = pc[2] >= rgba[v][2];
   k[v][3] = pc[3] >= rgba[v][3];
      case PIPE_FUNC_EQUAL:
      k[v][0] = pc[0] == rgba[v][0];
   k[v][1] = pc[1] == rgba[v][1];
   k[v][2] = pc[2] == rgba[v][2];
   k[v][3] = pc[3] == rgba[v][3];
      case PIPE_FUNC_NOTEQUAL:
      k[v][0] = pc[0] != rgba[v][0];
   k[v][1] = pc[1] != rgba[v][1];
   k[v][2] = pc[2] != rgba[v][2];
   k[v][3] = pc[3] != rgba[v][3];
      case PIPE_FUNC_ALWAYS:
      k[v][0] = k[v][1] = k[v][2] = k[v][3] = 1;
      case PIPE_FUNC_NEVER:
      k[v][0] = k[v][1] = k[v][2] = k[v][3] = 0;
      default:
      k[v][0] = k[v][1] = k[v][2] = k[v][3] = 0;
   assert(0);
                  if (is_gather) {
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      for (v = 0; v < TGSI_NUM_CHANNELS; v++) {
               } else {
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      rgba[0][j] = k[0][j];
   rgba[1][j] = k[0][j];
   rgba[2][j] = k[0][j];
            }
      static void
   do_swizzling(const struct pipe_sampler_view *sview,
               {
      struct sp_sampler_view *sp_sview = (struct sp_sampler_view *)sview;
   int j;
   const unsigned swizzle_r = sview->swizzle_r;
   const unsigned swizzle_g = sview->swizzle_g;
   const unsigned swizzle_b = sview->swizzle_b;
            switch (swizzle_r) {
   case PIPE_SWIZZLE_0:
      for (j = 0; j < 4; j++)
            case PIPE_SWIZZLE_1:
      for (j = 0; j < 4; j++)
            default:
      assert(swizzle_r < 4);
   for (j = 0; j < 4; j++)
               switch (swizzle_g) {
   case PIPE_SWIZZLE_0:
      for (j = 0; j < 4; j++)
            case PIPE_SWIZZLE_1:
      for (j = 0; j < 4; j++)
            default:
      assert(swizzle_g < 4);
   for (j = 0; j < 4; j++)
               switch (swizzle_b) {
   case PIPE_SWIZZLE_0:
      for (j = 0; j < 4; j++)
            case PIPE_SWIZZLE_1:
      for (j = 0; j < 4; j++)
            default:
      assert(swizzle_b < 4);
   for (j = 0; j < 4; j++)
               switch (swizzle_a) {
   case PIPE_SWIZZLE_0:
      for (j = 0; j < 4; j++)
            case PIPE_SWIZZLE_1:
      for (j = 0; j < 4; j++)
            default:
      assert(swizzle_a < 4);
   for (j = 0; j < 4; j++)
         }
         static wrap_nearest_func
   get_nearest_unorm_wrap(unsigned mode)
   {
      switch (mode) {
   case PIPE_TEX_WRAP_CLAMP:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         default:
      debug_printf("illegal wrap mode %d with non-normalized coords\n", mode);
         }
         static wrap_nearest_func
   get_nearest_wrap(unsigned mode)
   {
      switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_MIRROR_CLAMP:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
         default:
      assert(0);
         }
         static wrap_linear_func
   get_linear_unorm_wrap(unsigned mode)
   {
      switch (mode) {
   case PIPE_TEX_WRAP_CLAMP:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         default:
      debug_printf("illegal wrap mode %d with non-normalized coords\n", mode);
         }
         static wrap_linear_func
   get_linear_wrap(unsigned mode)
   {
      switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_MIRROR_CLAMP:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
         default:
      assert(0);
         }
         /**
   * Is swizzling needed for the given state key?
   */
   static inline bool
   any_swizzle(const struct pipe_sampler_view *view)
   {
      return (view->swizzle_r != PIPE_SWIZZLE_X ||
         view->swizzle_g != PIPE_SWIZZLE_Y ||
      }
         static img_filter_func
   get_img_filter(const struct sp_sampler_view *sp_sview,
               {
      switch (sp_sview->base.target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         else
            case PIPE_TEXTURE_1D_ARRAY:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         else
            case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      /* Try for fast path:
   */
   if (!gather && sp_sview->pot2d &&
      sampler->wrap_s == sampler->wrap_t &&
      {
      switch (sampler->wrap_s) {
   case PIPE_TEX_WRAP_REPEAT:
      switch (filter) {
   case PIPE_TEX_FILTER_NEAREST:
         case PIPE_TEX_FILTER_LINEAR:
         default:
         }
      case PIPE_TEX_WRAP_CLAMP:
      switch (filter) {
   case PIPE_TEX_FILTER_NEAREST:
         default:
               }
   /* Otherwise use default versions:
   */
   if (filter == PIPE_TEX_FILTER_NEAREST) 
         else
            case PIPE_TEXTURE_2D_ARRAY:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         else
            case PIPE_TEXTURE_CUBE:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         else
            case PIPE_TEXTURE_CUBE_ARRAY:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         else
            case PIPE_TEXTURE_3D:
      if (filter == PIPE_TEX_FILTER_NEAREST) 
         else
            default:
      assert(0);
         }
      /**
   * Get mip filter funcs, and optionally both img min filter and img mag
   * filter. Note that both img filter function pointers must be either non-NULL
   * or NULL.
   */
   static void
   get_filters(const struct sp_sampler_view *sp_sview,
               const struct sp_sampler *sp_samp,
   const enum tgsi_sampler_control control,
   const struct sp_filter_funcs **funcs,
   {
      assert(funcs);
   if (control == TGSI_SAMPLER_GATHER) {
      *funcs = &funcs_nearest;
   if (min) {
      *min = get_img_filter(sp_sview, &sp_samp->base,
         } else if (sp_sview->pot2d & sp_samp->min_mag_equal_repeat_linear) {
         } else {
      *funcs = sp_samp->filter_funcs;
   if (min) {
      assert(mag);
   *min = get_img_filter(sp_sview, &sp_samp->base,
         if (sp_samp->min_mag_equal) {
         } else {
      *mag = get_img_filter(sp_sview, &sp_samp->base,
               }
      static void
   sample_mip(const struct sp_sampler_view *sp_sview,
            const struct sp_sampler *sp_samp,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   const float c0[TGSI_QUAD_SIZE],
   int gather_comp,
   const float lod[TGSI_QUAD_SIZE],
      {
      const struct sp_filter_funcs *funcs = NULL;
   img_filter_func min_img_filter = NULL;
            get_filters(sp_sview, sp_samp, filt_args->control,
            funcs->filter(sp_sview, sp_samp, min_img_filter, mag_img_filter,
            if (sp_samp->base.compare_mode != PIPE_TEX_COMPARE_NONE) {
                  if (sp_sview->need_swizzle && filt_args->control != TGSI_SAMPLER_GATHER) {
      float rgba_temp[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE];
   memcpy(rgba_temp, rgba, sizeof(rgba_temp));
            }
         /**
   * This function uses cube texture coordinates to choose a face of a cube and
   * computes the 2D cube face coordinates. Puts face info into the sampler
   * faces[] array.
   */
   static void
   convert_cube(const struct sp_sampler_view *sp_sview,
               const struct sp_sampler *sp_samp,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   const float c0[TGSI_QUAD_SIZE],
   float ssss[TGSI_QUAD_SIZE],
   float tttt[TGSI_QUAD_SIZE],
   {
               pppp[0] = c0[0];
   pppp[1] = c0[1];
   pppp[2] = c0[2];
   pppp[3] = c0[3];
   /*
   major axis
   direction    target                             sc     tc    ma
   ----------   -------------------------------    ---    ---   ---
   +rx          TEXTURE_CUBE_MAP_POSITIVE_X_EXT    -rz    -ry   rx
   -rx          TEXTURE_CUBE_MAP_NEGATIVE_X_EXT    +rz    -ry   rx
   +ry          TEXTURE_CUBE_MAP_POSITIVE_Y_EXT    +rx    +rz   ry
   -ry          TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT    +rx    -rz   ry
   +rz          TEXTURE_CUBE_MAP_POSITIVE_Z_EXT    +rx    -ry   rz
   -rz          TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT    -rx    -ry   rz
            /* Choose the cube face and compute new s/t coords for the 2D face.
   *
   * Use the same cube face for all four pixels in the quad.
   *
   * This isn't ideal, but if we want to use a different cube face
   * per pixel in the quad, we'd have to also compute the per-face
   * LOD here too.  That's because the four post-face-selection
   * texcoords are no longer related to each other (they're
   * per-face!)  so we can't use subtraction to compute the partial
   * deriviates to compute the LOD.  Doing so (near cube edges
   * anyway) gives us pretty much random values.
   */
   for (j = 0; j < TGSI_QUAD_SIZE; j++)  {
      const float rx = s[j], ry = t[j], rz = p[j];
            if (arx >= ary && arx >= arz) {
      const float sign = (rx >= 0.0F) ? 1.0F : -1.0F;
   const uint face = (rx >= 0.0F) ?
         const float ima = -0.5F / fabsf(s[j]);
   ssss[j] = sign *  p[j] * ima + 0.5F;
   tttt[j] =         t[j] * ima + 0.5F;
      }
   else if (ary >= arx && ary >= arz) {
      const float sign = (ry >= 0.0F) ? 1.0F : -1.0F;
   const uint face = (ry >= 0.0F) ?
         const float ima = -0.5F / fabsf(t[j]);
   ssss[j] =        -s[j] * ima + 0.5F;
   tttt[j] = sign * -p[j] * ima + 0.5F;
      }
   else {
      const float sign = (rz >= 0.0F) ? 1.0F : -1.0F;
   const uint face = (rz >= 0.0F) ?
         const float ima = -0.5F / fabsf(p[j]);
   ssss[j] = sign * -s[j] * ima + 0.5F;
   tttt[j] =         t[j] * ima + 0.5F;
            }
         static void
   sp_get_dims(const struct sp_sampler_view *sp_sview,
               {
      const struct pipe_sampler_view *view = &sp_sview->base;
            if (view->target == PIPE_BUFFER) {
      dims[0] = view->u.buf.size / util_format_get_blocksize(view->format);
   /* the other values are undefined, but let's avoid potential valgrind
   * warnings.
   */
   dims[1] = dims[2] = dims[3] = 0;
               /* undefined according to EXT_gpu_program */
   level += view->u.tex.first_level;
   if (level > view->u.tex.last_level)
            dims[3] = view->u.tex.last_level - view->u.tex.first_level + 1;
            switch (view->target) {
   case PIPE_TEXTURE_1D_ARRAY:
      dims[1] = view->u.tex.last_layer - view->u.tex.first_layer + 1;
      case PIPE_TEXTURE_1D:
         case PIPE_TEXTURE_2D_ARRAY:
      dims[2] = view->u.tex.last_layer - view->u.tex.first_layer + 1;
      case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_RECT:
      dims[1] = u_minify(texture->height0, level);
      case PIPE_TEXTURE_3D:
      dims[1] = u_minify(texture->height0, level);
   dims[2] = u_minify(texture->depth0, level);
      case PIPE_TEXTURE_CUBE_ARRAY:
      dims[1] = u_minify(texture->height0, level);
   dims[2] = (view->u.tex.last_layer - view->u.tex.first_layer + 1) / 6;
      default:
      assert(!"unexpected texture target in sp_get_dims()");
         }
      /**
   * This function is only used for getting unfiltered texels via the
   * TXF opcode.  The GL spec says that out-of-bounds texel fetches
   * produce undefined results.  Instead of crashing, lets just clamp
   * coords to the texture image size.
   */
   static void
   sp_get_texels(const struct sp_sampler_view *sp_sview,
               const int v_i[TGSI_QUAD_SIZE],
   const int v_j[TGSI_QUAD_SIZE],
   const int v_k[TGSI_QUAD_SIZE],
   const int lod[TGSI_QUAD_SIZE],
   {
      union tex_tile_address addr;
   const struct pipe_resource *texture = sp_sview->base.texture;
   int j, c;
   const float *tx;
   /* TODO write a better test for LOD */
   const unsigned level =
      sp_sview->base.target == PIPE_BUFFER ? 0 :
   CLAMP(lod[0] + sp_sview->base.u.tex.first_level,
            const int width = u_minify(texture->width0, level);
   const int height = u_minify(texture->height0, level);
   const int depth = u_minify(texture->depth0, level);
            addr.value = 0;
            switch (sp_sview->base.target) {
   case PIPE_BUFFER:
      elem_size = util_format_get_blocksize(sp_sview->base.format);
   first_element = sp_sview->base.u.buf.offset / elem_size;
   last_element = (sp_sview->base.u.buf.offset +
         for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      const int x = CLAMP(v_i[j] + offset[0] +
                     tx = get_texel_buffer_no_border(sp_sview, addr, x, elem_size);
   for (c = 0; c < 4; c++) {
            }
      case PIPE_TEXTURE_1D:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      const int x = CLAMP(v_i[j] + offset[0], 0, width - 1);
   tx = get_texel_2d_no_border(sp_sview, addr, x,
         for (c = 0; c < 4; c++) {
            }
      case PIPE_TEXTURE_1D_ARRAY:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      const int x = CLAMP(v_i[j] + offset[0], 0, width - 1);
   const int y = CLAMP(v_j[j], sp_sview->base.u.tex.first_layer,
         tx = get_texel_2d_no_border(sp_sview, addr, x, y);
   for (c = 0; c < 4; c++) {
            }
      case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      const int x = CLAMP(v_i[j] + offset[0], 0, width - 1);
   const int y = CLAMP(v_j[j] + offset[1], 0, height - 1);
   tx = get_texel_3d_no_border(sp_sview, addr, x, y,
         for (c = 0; c < 4; c++) {
            }
      case PIPE_TEXTURE_2D_ARRAY:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      const int x = CLAMP(v_i[j] + offset[0], 0, width - 1);
   const int y = CLAMP(v_j[j] + offset[1], 0, height - 1);
   const int layer = CLAMP(v_k[j], sp_sview->base.u.tex.first_layer,
         tx = get_texel_3d_no_border(sp_sview, addr, x, y, layer);
   for (c = 0; c < 4; c++) {
            }
      case PIPE_TEXTURE_3D:
      for (j = 0; j < TGSI_QUAD_SIZE; j++) {
      int x = CLAMP(v_i[j] + offset[0], 0, width - 1);
   int y = CLAMP(v_j[j] + offset[1], 0, height - 1);
   int z = CLAMP(v_k[j] + offset[2], 0, depth - 1);
   tx = get_texel_3d_no_border(sp_sview, addr, x, y, z);
   for (c = 0; c < 4; c++) {
            }
      case PIPE_TEXTURE_CUBE: /* TXF can't work on CUBE according to spec */
   case PIPE_TEXTURE_CUBE_ARRAY:
   default:
      assert(!"Unknown or CUBE texture type in TXF processing\n");
               if (sp_sview->need_swizzle) {
      float rgba_temp[TGSI_NUM_CHANNELS][TGSI_QUAD_SIZE];
   memcpy(rgba_temp, rgba, sizeof(rgba_temp));
         }
         void *
   softpipe_create_sampler_state(struct pipe_context *pipe,
         {
                        /* Note that (for instance) linear_texcoord_s and
   * nearest_texcoord_s may be active at the same time, if the
   * sampler min_img_filter differs from its mag_img_filter.
   */
   if (!sampler->unnormalized_coords) {
      samp->linear_texcoord_s = get_linear_wrap( sampler->wrap_s );
   samp->linear_texcoord_t = get_linear_wrap( sampler->wrap_t );
            samp->nearest_texcoord_s = get_nearest_wrap( sampler->wrap_s );
   samp->nearest_texcoord_t = get_nearest_wrap( sampler->wrap_t );
      }
   else {
      samp->linear_texcoord_s = get_linear_unorm_wrap( sampler->wrap_s );
   samp->linear_texcoord_t = get_linear_unorm_wrap( sampler->wrap_t );
            samp->nearest_texcoord_s = get_nearest_unorm_wrap( sampler->wrap_s );
   samp->nearest_texcoord_t = get_nearest_unorm_wrap( sampler->wrap_t );
                        switch (sampler->min_mip_filter) {
   case PIPE_TEX_MIPFILTER_NONE:
      if (sampler->min_img_filter == sampler->mag_img_filter)
         else
               case PIPE_TEX_MIPFILTER_NEAREST:
      samp->filter_funcs = &funcs_nearest;
         case PIPE_TEX_MIPFILTER_LINEAR:
      if (sampler->min_img_filter == sampler->mag_img_filter &&
      !sampler->unnormalized_coords &&
   sampler->wrap_s == PIPE_TEX_WRAP_REPEAT &&
   sampler->wrap_t == PIPE_TEX_WRAP_REPEAT &&
   sampler->min_img_filter == PIPE_TEX_FILTER_LINEAR &&
   sampler->max_anisotropy <= 1) {
      }
            /* Anisotropic filtering extension. */
   if (sampler->max_anisotropy > 1) {
               /* Override min_img_filter:
   * min_img_filter needs to be set to NEAREST since we need to access
   * each texture pixel as it is and weight it later; using linear
   * filters will have incorrect results.
   * By setting the filter to NEAREST here, we can avoid calling the
   * generic img_filter_2d_nearest in the anisotropic filter function,
   * making it possible to use one of the accelerated implementations
                     if (!weightLut) {
         }
   }
      }
   if (samp->min_img_filter == sampler->mag_img_filter) {
                     }
         compute_lambda_func
   softpipe_get_lambda_func(const struct pipe_sampler_view *view,
         {
      if (shader != PIPE_SHADER_FRAGMENT)
            switch (view->target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
         case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_RECT:
         case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
         case PIPE_TEXTURE_3D:
         default:
      assert(0);
         }
         struct pipe_sampler_view *
   softpipe_create_sampler_view(struct pipe_context *pipe,
               {
      struct sp_sampler_view *sview = CALLOC_STRUCT(sp_sampler_view);
            if (sview) {
      struct pipe_sampler_view *view = &sview->base;
   *view = *templ;
   view->reference.count = 1;
   view->texture = NULL;
   pipe_resource_reference(&view->texture, resource);
      #ifdef DEBUG
      /*
      * This is possibly too lenient, but the primary reason is just
   * to catch gallium frontends which forget to initialize this, so
   * it only catches clearly impossible view targets.
   */
   if (view->target != resource->target) {
      if (view->target == PIPE_TEXTURE_1D)
         else if (view->target == PIPE_TEXTURE_1D_ARRAY)
         else if (view->target == PIPE_TEXTURE_2D)
      assert(resource->target == PIPE_TEXTURE_2D_ARRAY ||
            else if (view->target == PIPE_TEXTURE_2D_ARRAY)
      assert(resource->target == PIPE_TEXTURE_2D ||
            else if (view->target == PIPE_TEXTURE_CUBE)
      assert(resource->target == PIPE_TEXTURE_CUBE_ARRAY ||
      else if (view->target == PIPE_TEXTURE_CUBE_ARRAY)
      assert(resource->target == PIPE_TEXTURE_CUBE ||
      else
      #endif
            if (any_swizzle(view)) {
                  sview->need_cube_convert = (view->target == PIPE_TEXTURE_CUBE ||
         sview->pot2d = spr->pot &&
                  sview->xpot = util_logbase2( resource->width0 );
                           }
         static inline const struct sp_tgsi_sampler *
   sp_tgsi_sampler_cast_c(const struct tgsi_sampler *sampler)
   {
         }
         static void
   sp_tgsi_get_dims(struct tgsi_sampler *tgsi_sampler,
               {
      const struct sp_tgsi_sampler *sp_samp =
            assert(sview_index < PIPE_MAX_SHADER_SAMPLER_VIEWS);
   /* always have a view here but texture is NULL if no sampler view was set. */
   if (!sp_samp->sp_sview[sview_index].base.texture) {
      dims[0] = dims[1] = dims[2] = dims[3] = 0;
      }
      }
         static void prepare_compare_values(enum pipe_texture_target target,
                           {
      if (target == PIPE_TEXTURE_2D_ARRAY ||
      target == PIPE_TEXTURE_CUBE) {
   pc[0] = c0[0];
   pc[1] = c0[1];
   pc[2] = c0[2];
      } else if (target == PIPE_TEXTURE_CUBE_ARRAY) {
      pc[0] = c1[0];
   pc[1] = c1[1];
   pc[2] = c1[2];
      } else {
      pc[0] = p[0];
   pc[1] = p[1];
   pc[2] = p[2];
         }
      static void
   sp_tgsi_get_samples(struct tgsi_sampler *tgsi_sampler,
                     const unsigned sview_index,
   const unsigned sampler_index,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   const float c0[TGSI_QUAD_SIZE],
   const float lod_in[TGSI_QUAD_SIZE],
   float derivs[3][2][TGSI_QUAD_SIZE],
   {
      const struct sp_tgsi_sampler *sp_tgsi_samp =
         struct sp_sampler_view sp_sview;
   const struct sp_sampler *sp_samp;
   struct filter_args filt_args;
   float compare_values[TGSI_QUAD_SIZE];
   float lod[TGSI_QUAD_SIZE];
            assert(sview_index < PIPE_MAX_SHADER_SAMPLER_VIEWS);
   assert(sampler_index < PIPE_MAX_SAMPLERS);
            memcpy(&sp_sview, &sp_tgsi_samp->sp_sview[sview_index],
                  if (util_format_is_unorm(sp_sview.base.format)) {
      for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      sp_sview.border_color.f[c] = CLAMP(sp_samp->base.border_color.f[c],
   } else if (util_format_is_snorm(sp_sview.base.format)) {
      for (c = 0; c < TGSI_NUM_CHANNELS; c++)
      sp_sview.border_color.f[c] = CLAMP(sp_samp->base.border_color.f[c],
   } else {
      memcpy(sp_sview.border_color.f, sp_samp->base.border_color.f,
               /* always have a view here but texture is NULL if no sampler view was set. */
   if (!sp_sview.base.texture) {
      int i, j;
   for (j = 0; j < TGSI_NUM_CHANNELS; j++) {
      for (i = 0; i < TGSI_QUAD_SIZE; i++) {
            }
               if (sp_samp->base.compare_mode != PIPE_TEX_COMPARE_NONE)
            filt_args.control = control;
   filt_args.offset = offset;
                     if (sp_sview.need_cube_convert) {
      float cs[TGSI_QUAD_SIZE];
   float ct[TGSI_QUAD_SIZE];
   float cp[TGSI_QUAD_SIZE];
                     filt_args.faces = faces;
      } else {
               filt_args.faces = zero_faces;
         }
      static void
   sp_tgsi_query_lod(const struct tgsi_sampler *tgsi_sampler,
                     const unsigned sview_index,
   const unsigned sampler_index,
   const float s[TGSI_QUAD_SIZE],
   const float t[TGSI_QUAD_SIZE],
   const float p[TGSI_QUAD_SIZE],
   const float c0[TGSI_QUAD_SIZE],
   {
      static const float lod_in[TGSI_QUAD_SIZE] = { 0.0, 0.0, 0.0, 0.0 };
            const struct sp_tgsi_sampler *sp_tgsi_samp =
         const struct sp_sampler_view *sp_sview;
   const struct sp_sampler *sp_samp;
   const struct sp_filter_funcs *funcs;
            assert(sview_index < PIPE_MAX_SHADER_SAMPLER_VIEWS);
   assert(sampler_index < PIPE_MAX_SAMPLERS);
            sp_sview = &sp_tgsi_samp->sp_sview[sview_index];
   sp_samp = sp_tgsi_samp->sp_sampler[sampler_index];
   /* always have a view here but texture is NULL if no sampler view was
   * set. */
   if (!sp_sview->base.texture) {
      for (i = 0; i < TGSI_QUAD_SIZE; i++) {
      mipmap[i] = 0.0f;
      }
      }
   compute_lambda_lod_unclamped(sp_sview, sp_samp,
            get_filters(sp_sview, sp_samp, control, &funcs, NULL, NULL);
      }
      static void
   sp_tgsi_get_texel(struct tgsi_sampler *tgsi_sampler,
                     const unsigned sview_index,
   const int i[TGSI_QUAD_SIZE],
   {
      const struct sp_tgsi_sampler *sp_samp =
            assert(sview_index < PIPE_MAX_SHADER_SAMPLER_VIEWS);
   /* always have a view here but texture is NULL if no sampler view was set. */
   if (!sp_samp->sp_sview[sview_index].base.texture) {
      int i, j;
   for (j = 0; j < TGSI_NUM_CHANNELS; j++) {
      for (i = 0; i < TGSI_QUAD_SIZE; i++) {
            }
      }
      }
         struct sp_tgsi_sampler *
   sp_create_tgsi_sampler(void)
   {
      struct sp_tgsi_sampler *samp = CALLOC_STRUCT(sp_tgsi_sampler);
   if (!samp)
            samp->base.get_dims = sp_tgsi_get_dims;
   samp->base.get_samples = sp_tgsi_get_samples;
   samp->base.get_texel = sp_tgsi_get_texel;
               }
