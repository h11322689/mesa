   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * All Rights Reserved.
   * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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
   * @file
   * Simple vertex/fragment shader generators.
   *  
   * @author Brian Paul
         */
         #include "pipe/p_context.h"
   #include "pipe/p_shader_tokens.h"
   #include "pipe/p_state.h"
   #include "util/u_simple_shaders.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_strings.h"
   #include "tgsi/tgsi_ureg.h"
   #include "tgsi/tgsi_text.h"
   #include <stdio.h> /* include last */
            /**
   * Make simple vertex pass-through shader.
   * \param num_attribs  number of attributes to pass through
   * \param semantic_names  array of semantic names for each attribute
   * \param semantic_indexes  array of semantic indexes for each attribute
   */
   void *
   util_make_vertex_passthrough_shader(struct pipe_context *pipe,
                           {
      return util_make_vertex_passthrough_shader_with_so(pipe, num_attribs,
                  }
      void *
   util_make_vertex_passthrough_shader_with_so(struct pipe_context *pipe,
                                 {
      struct ureg_program *ureg;
            ureg = ureg_create( PIPE_SHADER_VERTEX );
   if (!ureg)
            if (window_space)
            for (i = 0; i < num_attribs; i++) {
      struct ureg_src src;
            src = ureg_DECL_vs_input( ureg, i );
      dst = ureg_DECL_output( ureg,
                              if (layered) {
      struct ureg_src instance_id =
                  ureg_MOV(ureg, ureg_writemask(layer, TGSI_WRITEMASK_X),
                           }
         void *util_make_layered_clear_vertex_shader(struct pipe_context *pipe)
   {
      const enum tgsi_semantic semantic_names[] = {TGSI_SEMANTIC_POSITION,
                  return util_make_vertex_passthrough_shader_with_so(pipe, 2, semantic_names,
            }
      /**
   * Takes position and color, and outputs position, color, and instance id.
   */
   void *util_make_layered_clear_helper_vertex_shader(struct pipe_context *pipe)
   {
      static const char text[] =
         "VERT\n"
   "DCL IN[0]\n"
   "DCL IN[1]\n"
   "DCL SV[0], INSTANCEID\n"
   "DCL OUT[0], POSITION\n"
                  "MOV OUT[0], IN[0]\n"
   "MOV OUT[1], IN[1]\n"
   "MOV OUT[2].x, SV[0].xxxx\n"
   struct tgsi_token tokens[1000];
            if (!tgsi_text_translate(text, tokens, ARRAY_SIZE(tokens))) {
      assert(0);
      }
   pipe_shader_state_from_tgsi(&state, tokens);
      }
      /**
   * Takes position, color, and target layer, and emits vertices on that target
   * layer, with the specified color.
   */
   void *util_make_layered_clear_geometry_shader(struct pipe_context *pipe)
   {
      static const char text[] =
      "GEOM\n"
   "PROPERTY GS_INPUT_PRIMITIVE TRIANGLES\n"
   "PROPERTY GS_OUTPUT_PRIMITIVE TRIANGLE_STRIP\n"
   "PROPERTY GS_MAX_OUTPUT_VERTICES 3\n"
   "PROPERTY GS_INVOCATIONS 1\n"
   "DCL IN[][0], POSITION\n" /* position */
   "DCL IN[][1], GENERIC[0]\n" /* color */
   "DCL IN[][2], GENERIC[1]\n" /* vs invocation */
   "DCL OUT[0], POSITION\n"
   "DCL OUT[1], GENERIC[0]\n"
   "DCL OUT[2], LAYER\n"
            "MOV OUT[0], IN[0][0]\n"
   "MOV OUT[1], IN[0][1]\n"
   "MOV OUT[2].x, IN[0][2].xxxx\n"
   "EMIT IMM[0].xxxx\n"
   "MOV OUT[0], IN[1][0]\n"
   "MOV OUT[1], IN[1][1]\n"
   "MOV OUT[2].x, IN[1][2].xxxx\n"
   "EMIT IMM[0].xxxx\n"
   "MOV OUT[0], IN[2][0]\n"
   "MOV OUT[1], IN[2][1]\n"
   "MOV OUT[2].x, IN[2][2].xxxx\n"
   "EMIT IMM[0].xxxx\n"
      struct tgsi_token tokens[1000];
            if (!tgsi_text_translate(text, tokens, ARRAY_SIZE(tokens))) {
      assert(0);
      }
   pipe_shader_state_from_tgsi(&state, tokens);
      }
      static void
   ureg_load_tex(struct ureg_program *ureg, struct ureg_dst out,
               struct ureg_src coord, struct ureg_src sampler,
   {
      if (use_txf) {
               /* Nearest filtering floors and then converts to integer, and then
   * applies clamp to edge as clamp(coord, 0, dim - 1).
   * u_blitter only uses this when the coordinates are in bounds,
   * so no clamping is needed and we can use trunc instead of floor. trunc
   * with f2i will get optimized out in NIR where f2i has round-to-zero
   * behaviour already.
   */
   unsigned wrmask = tex_target == TGSI_TEXTURE_1D ||
                        ureg_MOV(ureg, temp, coord);
   ureg_TRUNC(ureg, ureg_writemask(temp, wrmask), ureg_src(temp));
            if (load_level_zero)
         else
      } else {
      if (load_level_zero)
         else
         }
      /**
   * Make simple fragment texture shader:
   *  TEX TEMP[0], IN[0], SAMP[0], 2D;
   *   .. optional SINT <-> UINT clamping ..
   *  MOV OUT[0], TEMP[0]
   *  END;
   *
   * \param tex_target  one of TGSI_TEXTURE_x
   */
   void *
   util_make_fragment_tex_shader(struct pipe_context *pipe,
                                 {
      struct ureg_program *ureg;
   struct ureg_src sampler;
   struct ureg_src tex;
   struct ureg_dst temp;
                     ureg = ureg_create( PIPE_SHADER_FRAGMENT );
   if (!ureg)
                              tex = ureg_DECL_fs_input( ureg, 
                  out = ureg_DECL_output( ureg, 
                           if (tex_target == TGSI_TEXTURE_BUFFER)
      ureg_TXF(ureg,
            else
      ureg_load_tex(ureg, ureg_writemask(temp, TGSI_WRITEMASK_XYZW), tex, sampler,
         if (stype != dtype) {
      if (stype == TGSI_RETURN_TYPE_SINT) {
                  } else {
                                                         }
      /**
   * Make a simple fragment texture shader which reads the texture unit 0 and 1
   * and writes it as depth and stencil, respectively.
   */
   void *
   util_make_fs_blit_zs(struct pipe_context *pipe, unsigned zs_mask,
               {
      struct ureg_program *ureg;
   struct ureg_src depth_sampler, stencil_sampler, coord;
            ureg = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!ureg)
            coord = ureg_DECL_fs_input(ureg, TGSI_SEMANTIC_GENERIC, 0,
                  if (zs_mask & PIPE_MASK_Z) {
      depth_sampler = ureg_DECL_sampler(ureg, 0);
   ureg_DECL_sampler_view(ureg, 0, tex_target,
                              ureg_load_tex(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), coord,
            depth = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0);
   ureg_MOV(ureg, ureg_writemask(depth, TGSI_WRITEMASK_Z),
               if (zs_mask & PIPE_MASK_S) {
      stencil_sampler = ureg_DECL_sampler(ureg, zs_mask & PIPE_MASK_Z ? 1 : 0);
   ureg_DECL_sampler_view(ureg, zs_mask & PIPE_MASK_Z ? 1 : 0, tex_target,
                              ureg_load_tex(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), coord,
            stencil = ureg_DECL_output(ureg, TGSI_SEMANTIC_STENCIL, 0);
   ureg_MOV(ureg, ureg_writemask(stencil, TGSI_WRITEMASK_Y),
                           }
         /**
   * Make simple fragment color pass-through shader that replicates OUT[0]
   * to all bound colorbuffers.
   */
   void *
   util_make_fragment_passthrough_shader(struct pipe_context *pipe,
                     {
      static const char shader_templ[] =
         "FRAG\n"
   "%s"
                           char text[sizeof(shader_templ)+100];
   struct tgsi_token tokens[1000];
            sprintf(text, shader_templ,
         write_all_cbufs ? "PROPERTY FS_COLOR0_WRITES_ALL_CBUFS 1\n" : "",
            if (!tgsi_text_translate(text, tokens, ARRAY_SIZE(tokens))) {
      assert(0);
      }
      #if 0
         #endif
            }
         void *
   util_make_empty_fragment_shader(struct pipe_context *pipe)
   {
      struct ureg_program *ureg = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!ureg)
            ureg_END(ureg);
      }
         /**
   * Make a fragment shader that copies the input color to N output colors.
   */
   void *
   util_make_fragment_cloneinput_shader(struct pipe_context *pipe, int num_cbufs,
               {
      struct ureg_program *ureg;
   struct ureg_src src;
   struct ureg_dst dst[PIPE_MAX_COLOR_BUFS];
                     ureg = ureg_create( PIPE_SHADER_FRAGMENT );
   if (!ureg)
            src = ureg_DECL_fs_input( ureg, input_semantic, 0,
            for (i = 0; i < num_cbufs; i++)
            for (i = 0; i < num_cbufs; i++)
                        }
         static void *
   util_make_fs_blit_msaa_gen(struct pipe_context *pipe,
                              enum tgsi_texture_type tgsi_tex,
      {
      char text[1000];
   struct tgsi_token tokens[1000];
            if (has_txq) {
      static const char shader_templ[] =
         "FRAG\n"
   "DCL IN[0], GENERIC[0], LINEAR\n"
   "DCL SAMP[0]\n"
   "DCL SVIEW[0], %s, %s\n"
   "DCL OUT[0], %s\n"
   "DCL TEMP[0..1]\n"
                  /* Nearest filtering floors and then converts to integer, and then
   * applies clamp to edge as clamp(coord, 0, dim - 1).
   */
   "MOV TEMP[0], IN[0]\n"
   "FLR TEMP[0].xy, TEMP[0]\n"
   "F2I TEMP[0], TEMP[0]\n"
   "IMAX TEMP[0].xy, TEMP[0], IMM[0].xxxx\n"
   /* Clamp to edge for the upper bound. */
   "TXQ TEMP[1].xy, IMM[0].xxxx, SAMP[0], %s\n"
   "UADD TEMP[1].xy, TEMP[1], IMM[0].yyyy\n" /* width - 1, height - 1 */
   "IMIN TEMP[0].xy, TEMP[0], TEMP[1]\n"
   /* Texel fetch. */
   "%s"
   "TXF TEMP[0], TEMP[0], SAMP[0], %s\n"
   "%s"
                     assert(tgsi_tex == TGSI_TEXTURE_2D_MSAA ||
            snprintf(text, sizeof(text), shader_templ, type, samp_type,
            output_semantic, sample_shading ? "DCL SV[0], SAMPLEID\n" : "",
   } else {
      static const char shader_templ[] =
         "FRAG\n"
   "DCL IN[0], GENERIC[0], LINEAR\n"
   "DCL SAMP[0]\n"
   "DCL SVIEW[0], %s, %s\n"
   "DCL OUT[0], %s\n"
   "DCL TEMP[0..1]\n"
                  /* Nearest filtering floors and then converts to integer, and then
   * applies clamp to edge as clamp(coord, 0, dim - 1). Don't clamp
   * to dim - 1 because TXQ is unsupported.
   */
   "MOV TEMP[0], IN[0]\n"
   "FLR TEMP[0].xy, TEMP[0]\n"
   "F2I TEMP[0], TEMP[0]\n"
   "IMAX TEMP[0].xy, TEMP[0], IMM[0].xxxx\n"
   /* Texel fetch. */
   "%s"
   "TXF TEMP[0], TEMP[0], SAMP[0], %s\n"
   "%s"
                     assert(tgsi_tex == TGSI_TEXTURE_2D_MSAA ||
            snprintf(text, sizeof(text), shader_templ, type, samp_type,
            output_semantic, sample_shading ? "DCL SV[0], SAMPLEID\n" : "",
            if (!tgsi_text_translate(text, tokens, ARRAY_SIZE(tokens))) {
      puts(text);
   assert(0);
      }
      #if 0
         #endif
            }
         /**
   * Make a fragment shader that sets the output color to a color
   * fetched from a multisample texture.
   * \param tex_target  one of PIPE_TEXTURE_x
   */
   void *
   util_make_fs_blit_msaa_color(struct pipe_context *pipe,
                           {
      const char *samp_type;
            if (stype == TGSI_RETURN_TYPE_UINT) {
               if (dtype == TGSI_RETURN_TYPE_SINT) {
            } else if (stype == TGSI_RETURN_TYPE_SINT) {
               if (dtype == TGSI_RETURN_TYPE_UINT) {
            } else {
      assert(dtype == TGSI_RETURN_TYPE_FLOAT);
               return util_make_fs_blit_msaa_gen(pipe, tgsi_tex, sample_shading, has_txq,
      }
         /**
   * Make a fragment shader that sets the output depth to a depth value
   * fetched from a multisample texture.
   * \param tex_target  one of PIPE_TEXTURE_x
   */
   void *
   util_make_fs_blit_msaa_depth(struct pipe_context *pipe,
               {
      return util_make_fs_blit_msaa_gen(pipe, tgsi_tex, sample_shading, has_txq,
            }
         /**
   * Make a fragment shader that sets the output stencil to a stencil value
   * fetched from a multisample texture.
   * \param tex_target  one of PIPE_TEXTURE_x
   */
   void *
   util_make_fs_blit_msaa_stencil(struct pipe_context *pipe,
               {
      return util_make_fs_blit_msaa_gen(pipe, tgsi_tex, sample_shading, has_txq,
            }
         /**
   * Make a fragment shader that sets the output depth and stencil to depth
   * and stencil values fetched from two multisample textures / samplers.
   * The sizes of both textures should match (it should be one depth-stencil
   * texture).
   * \param tex_target  one of PIPE_TEXTURE_x
   */
   void *
   util_make_fs_blit_msaa_depthstencil(struct pipe_context *pipe,
               {
      const char *type = tgsi_texture_names[tgsi_tex];
   char text[1000];
   struct tgsi_token tokens[1000];
            assert(tgsi_tex == TGSI_TEXTURE_2D_MSAA ||
            if (has_txq) {
      static const char shader_templ[] =
         "FRAG\n"
   "DCL IN[0], GENERIC[0], LINEAR\n"
   "DCL SAMP[0..1]\n"
   "DCL SVIEW[0], %s, FLOAT\n"
   "DCL SVIEW[1], %s, UINT\n"
   "DCL OUT[0], POSITION\n"
   "DCL OUT[1], STENCIL\n"
   "DCL TEMP[0..1]\n"
                  /* Nearest filtering floors and then converts to integer, and then
   * applies clamp to edge as clamp(coord, 0, dim - 1).
   */
   "MOV TEMP[0], IN[0]\n"
   "FLR TEMP[0].xy, TEMP[0]\n"
   "F2I TEMP[0], TEMP[0]\n"
   "IMAX TEMP[0].xy, TEMP[0], IMM[0].xxxx\n"
   /* Clamp to edge for the upper bound. */
   "TXQ TEMP[1].xy, IMM[0].xxxx, SAMP[0], %s\n"
   "UADD TEMP[1].xy, TEMP[1], IMM[0].yyyy\n" /* width - 1, height - 1 */
   "IMIN TEMP[0].xy, TEMP[0], TEMP[1]\n"
   /* Texel fetch. */
   "%s"
   "TXF OUT[0].z, TEMP[0], SAMP[0], %s\n"
            sprintf(text, shader_templ, type, type,
         sample_shading ? "DCL SV[0], SAMPLEID\n" : "", type,
      } else {
      static const char shader_templ[] =
         "FRAG\n"
   "DCL IN[0], GENERIC[0], LINEAR\n"
   "DCL SAMP[0..1]\n"
   "DCL SVIEW[0], %s, FLOAT\n"
   "DCL SVIEW[1], %s, UINT\n"
   "DCL OUT[0], POSITION\n"
   "DCL OUT[1], STENCIL\n"
   "DCL TEMP[0..1]\n"
                  /* Nearest filtering floors and then converts to integer, and then
   * applies clamp to edge as clamp(coord, 0, dim - 1). Don't clamp
   * to dim - 1 because TXQ is unsupported.
   */
   "MOV TEMP[0], IN[0]\n"
   "FLR TEMP[0].xy, TEMP[0]\n"
   "F2I TEMP[0], TEMP[0]\n"
   "IMAX TEMP[0].xy, TEMP[0], IMM[0].xxxx\n"
   /* Texel fetch. */
   "%s"
   "TXF OUT[0].z, TEMP[0], SAMP[0], %s\n"
            sprintf(text, shader_templ, type, type,
         sample_shading ? "DCL SV[0], SAMPLEID\n" : "",
               if (!tgsi_text_translate(text, tokens, ARRAY_SIZE(tokens))) {
      assert(0);
      }
      #if 0
         #endif
            }
         void *
   util_make_fs_msaa_resolve(struct pipe_context *pipe,
               {
      struct ureg_program *ureg;
   struct ureg_src sampler, coord;
   struct ureg_dst out, tmp_sum, tmp_coord, tmp;
            ureg = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!ureg)
            /* Declarations. */
   sampler = ureg_DECL_sampler(ureg, 0);
   ureg_DECL_sampler_view(ureg, 0, tgsi_tex,
               coord = ureg_DECL_fs_input(ureg, TGSI_SEMANTIC_GENERIC, 0,
         out = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);
   tmp_sum = ureg_DECL_temporary(ureg);
   tmp_coord = ureg_DECL_temporary(ureg);
            /* Instructions. */
            /* Nearest filtering floors and then converts to integer, and then
   * applies clamp to edge as clamp(coord, 0, dim - 1).
   */
   ureg_MOV(ureg, tmp_coord, coord);
   ureg_FLR(ureg, ureg_writemask(tmp_coord, TGSI_WRITEMASK_XY),
         ureg_F2I(ureg, tmp_coord, ureg_src(tmp_coord));
            /* Clamp to edge for the upper bound. */
   if (has_txq) {
      ureg_TXQ(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_XY), tgsi_tex,
         ureg_UADD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_XY), ureg_src(tmp),
         ureg_IMIN(ureg,  ureg_writemask(tmp_coord, TGSI_WRITEMASK_XY),
               for (i = 0; i < nr_samples; i++) {
      /* Read one sample. */
   ureg_MOV(ureg, ureg_writemask(tmp_coord, TGSI_WRITEMASK_W),
                  /* Add it to the sum.*/
               /* Calculate the average and return. */
   ureg_MUL(ureg, out, ureg_src(tmp_sum),
                     }
         void *
   util_make_fs_msaa_resolve_bilinear(struct pipe_context *pipe,
               {
      struct ureg_program *ureg;
   struct ureg_src sampler, coord;
   struct ureg_dst out, tmp, top, bottom;
   struct ureg_dst tmp_coord[4], tmp_sum[4], weights;
            ureg = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!ureg)
            /* Declarations. */
   sampler = ureg_DECL_sampler(ureg, 0);
   ureg_DECL_sampler_view(ureg, 0, tgsi_tex,
               coord = ureg_DECL_fs_input(ureg, TGSI_SEMANTIC_GENERIC, 0,
         out = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);
   for (c = 0; c < 4; c++)
         for (c = 0; c < 4; c++)
         tmp = ureg_DECL_temporary(ureg);
   top = ureg_DECL_temporary(ureg);
   weights = ureg_DECL_temporary(ureg);
            /* Instructions. */
   for (c = 0; c < 4; c++)
            /* Bilinear filtering starts with subtracting 0.5 from unnormalized
   * coordinates.
   */
   ureg_MOV(ureg, ureg_writemask(tmp_coord[0], TGSI_WRITEMASK_ZW), coord);
   ureg_ADD(ureg, ureg_writemask(tmp_coord[0], TGSI_WRITEMASK_XY), coord,
            /* Get the filter weights. */
   ureg_FRC(ureg, ureg_writemask(weights, TGSI_WRITEMASK_XY),
            /* Convert to integer by flooring to get the top-left coordinates. */
   ureg_FLR(ureg, ureg_writemask(tmp_coord[0], TGSI_WRITEMASK_XY),
                  /* Get the bottom-right coordinates. */
   ureg_UADD(ureg, tmp_coord[3], ureg_src(tmp_coord[0]),
            /* Clamp to edge. */
   if (has_txq) {
      ureg_TXQ(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_XY), tgsi_tex,
         ureg_UADD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_XY), ureg_src(tmp),
            ureg_IMIN(ureg, ureg_writemask(tmp_coord[0], TGSI_WRITEMASK_XY),
         ureg_IMIN(ureg, ureg_writemask(tmp_coord[3], TGSI_WRITEMASK_XY),
               ureg_IMAX(ureg, ureg_writemask(tmp_coord[0], TGSI_WRITEMASK_XY),
         ureg_IMAX(ureg, ureg_writemask(tmp_coord[3], TGSI_WRITEMASK_XY),
            /* Get the remaining top-right and bottom-left coordinates. */
   ureg_MOV(ureg, ureg_writemask(tmp_coord[1], TGSI_WRITEMASK_X),
         ureg_MOV(ureg, ureg_writemask(tmp_coord[1], TGSI_WRITEMASK_YZW),
            ureg_MOV(ureg, ureg_writemask(tmp_coord[2], TGSI_WRITEMASK_Y),
         ureg_MOV(ureg, ureg_writemask(tmp_coord[2], TGSI_WRITEMASK_XZW),
            for (i = 0; i < nr_samples; i++) {
      for (c = 0; c < 4; c++) {
      /* Read one sample. */
   ureg_MOV(ureg, ureg_writemask(tmp_coord[c], TGSI_WRITEMASK_W),
                  /* Add it to the sum.*/
                  /* Calculate the average. */
   for (c = 0; c < 4; c++)
      ureg_MUL(ureg, tmp_sum[c], ureg_src(tmp_sum[c]),
         /* Take the 4 average values and apply a standard bilinear filter. */
   ureg_LRP(ureg, top,
            ureg_scalar(ureg_src(weights), 0),
         ureg_LRP(ureg, bottom,
            ureg_scalar(ureg_src(weights), 0),
         ureg_LRP(ureg, out,
            ureg_scalar(ureg_src(weights), 1),
                  }
      void *
   util_make_geometry_passthrough_shader(struct pipe_context *pipe,
                     {
               struct ureg_program *ureg;
   struct ureg_dst dst[PIPE_MAX_SHADER_OUTPUTS];
   struct ureg_src src[PIPE_MAX_SHADER_INPUTS];
                     ureg = ureg_create(PIPE_SHADER_GEOMETRY);
   if (!ureg)
            ureg_property(ureg, TGSI_PROPERTY_GS_INPUT_PRIM, MESA_PRIM_POINTS);
   ureg_property(ureg, TGSI_PROPERTY_GS_OUTPUT_PRIM, MESA_PRIM_POINTS);
   ureg_property(ureg, TGSI_PROPERTY_GS_MAX_OUTPUT_VERTICES, 1);
   ureg_property(ureg, TGSI_PROPERTY_GS_INVOCATIONS, 1);
            /**
   * Loop over all the attribs and declare the corresponding
   * declarations in the geometry shader
   */
   for (i = 0; i < num_attribs; i++) {
      src[i] = ureg_DECL_input(ureg, semantic_names[i],
         src[i] = ureg_src_dimension(src[i], 0);
               /* MOV dst[i] src[i] */
   for (i = 0; i < num_attribs; i++) {
                  /* EMIT IMM[0] */
            /* END */
               }
         /**
   * Blit from color to ZS or from ZS to color in a manner that is equivalent
   * to memcpy.
   *
   * Color is either R32_UINT (for Z24S8 / S8Z24) or R32G32_UINT (Z32_S8X24).
   *
   * Depth and stencil samplers are used to load depth and stencil,
   * and they are packed and the result is written to a color output.
   *   OR
   * A color sampler is used to load a color value, which is unpacked and
   * written to depth and stencil shader outputs.
   */
   void *
   util_make_fs_pack_color_zs(struct pipe_context *pipe,
                     {
      struct ureg_program *ureg;
   struct ureg_src depth_sampler, stencil_sampler, color_sampler, coord;
            assert(zs_format == PIPE_FORMAT_Z24_UNORM_S8_UINT || /* color is R32_UINT */
         zs_format == PIPE_FORMAT_S8_UINT_Z24_UNORM || /* color is R32_UINT */
   zs_format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT || /* color is R32G32_UINT */
            bool has_stencil = zs_format != PIPE_FORMAT_Z24X8_UNORM &&
         bool is_z24 = zs_format != PIPE_FORMAT_Z32_FLOAT_S8X24_UINT;
   bool z24_is_high = zs_format == PIPE_FORMAT_S8_UINT_Z24_UNORM ||
            ureg = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!ureg)
            coord = ureg_DECL_fs_input(ureg, TGSI_SEMANTIC_GENERIC, 0,
            if (dst_is_color) {
      /* Load depth. */
   depth_sampler = ureg_DECL_sampler(ureg, 0);
   ureg_DECL_sampler_view(ureg, 0, tex_target,
                              depth = ureg_DECL_temporary(ureg);
   depth_x = ureg_writemask(depth, TGSI_WRITEMASK_X);
            /* Pack to Z24. */
   if (is_z24) {
      double imm = 0xffffff;
   struct ureg_src imm_f64 = ureg_DECL_immediate_f64(ureg, &imm, 2);
                  ureg_F2D(ureg, tmp_xy, ureg_src(depth));
                  if (z24_is_high)
         else
               if (has_stencil) {
      /* Load stencil. */
   stencil_sampler = ureg_DECL_sampler(ureg, 1);
   ureg_DECL_sampler_view(ureg, 0, tex_target,
                              stencil = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_X);
                  /* Pack stencil into depth. */
   if (is_z24) {
                                             if (is_z24) {
         } else {
      /* Z32_S8X24 */
   ureg_MOV(ureg, ureg_writemask(depth, TGSI_WRITEMASK_Y),
               } else {
      color_sampler = ureg_DECL_sampler(ureg, 0);
   ureg_DECL_sampler_view(ureg, 0, tex_target,
                              color = ureg_DECL_temporary(ureg);
            depth = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_X);
            if (is_z24) {
      double imm = 1.0 / 0xffffff;
   struct ureg_src imm_f64 = ureg_DECL_immediate_f64(ureg, &imm, 2);
                  ureg_UBFE(ureg, depth, ureg_src(color),
               ureg_U2D(ureg, tmp_xy, ureg_src(depth));
   ureg_DMUL(ureg, tmp_xy, ureg_src(tmp_xy), imm_f64);
      } else {
      /* depth = color.x; (Z32_S8X24) */
               out_depth = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0);
   ureg_MOV(ureg, ureg_writemask(out_depth, TGSI_WRITEMASK_Z),
            if (has_stencil) {
      if (is_z24) {
      ureg_UBFE(ureg, stencil, ureg_src(color),
            } else {
      /* stencil = color.y[0:7]; (Z32_S8X24) */
   ureg_UBFE(ureg, stencil,
                           out_stencil = ureg_DECL_output(ureg, TGSI_SEMANTIC_STENCIL, 0);
   ureg_MOV(ureg, ureg_writemask(out_stencil, TGSI_WRITEMASK_Y),
                              }
         /**
   * Create passthrough tessellation control shader.
   * Passthrough tessellation control shader has output of vertex shader
   * as input and input of tessellation eval shader as output.
   */
   void *
   util_make_tess_ctrl_passthrough_shader(struct pipe_context *pipe,
                                             {
      unsigned i, j;
            struct ureg_program *ureg;
   struct ureg_dst temp, addr;
   struct ureg_src invocationID;
   struct ureg_dst dst[PIPE_MAX_SHADER_OUTPUTS];
                     if (!ureg)
                              for (i = 0; i < num_tes_inputs; i++) {
      switch (tes_semantic_names[i]) {
   case TGSI_SEMANTIC_POSITION:
   case TGSI_SEMANTIC_PSIZE:
   case TGSI_SEMANTIC_COLOR:
   case TGSI_SEMANTIC_BCOLOR:
   case TGSI_SEMANTIC_CLIPDIST:
   case TGSI_SEMANTIC_CLIPVERTEX:
   case TGSI_SEMANTIC_TEXCOORD:
   case TGSI_SEMANTIC_FOG:
   case TGSI_SEMANTIC_GENERIC:
      for (j = 0; j < num_vs_outputs; j++) {
                        dst[num_regs] = ureg_DECL_output(ureg,
                                    if (tes_semantic_names[i] == TGSI_SEMANTIC_GENERIC ||
      tes_semantic_names[i] == TGSI_SEMANTIC_POSITION) {
                     num_regs++;
         }
      default:
                     dst[num_regs] = ureg_DECL_output(ureg, TGSI_SEMANTIC_TESSOUTER,
         src[num_regs] = ureg_DECL_constant(ureg, 0);
   num_regs++;
   dst[num_regs] = ureg_DECL_output(ureg, TGSI_SEMANTIC_TESSINNER,
         src[num_regs] = ureg_DECL_constant(ureg, 1);
            if (vertices_per_patch > 1) {
      invocationID = ureg_DECL_system_value(ureg,
         temp = ureg_DECL_local_temporary(ureg);
   addr = ureg_DECL_address(ureg);
   ureg_UARL(ureg, ureg_writemask(addr, TGSI_WRITEMASK_X),
               for (i = 0; i < num_regs; i++) {
      if (dst[i].Dimension && vertices_per_patch > 1) {
      struct ureg_src addr_x = ureg_scalar(ureg_src(addr), TGSI_SWIZZLE_X);
   ureg_MOV(ureg, temp, ureg_src_dimension_indirect(src[i],
         ureg_MOV(ureg, ureg_dst_dimension_indirect(dst[i],
      }
   else
                           }
      void *
   util_make_fs_stencil_blit(struct pipe_context *pipe, bool msaa_src, bool has_txq)
   {
      char text[1000];
   struct tgsi_token tokens[1000];
   struct pipe_shader_state state = { 0 };
   enum tgsi_texture_type tgsi_tex = msaa_src ? TGSI_TEXTURE_2D_MSAA :
            if (has_txq) {
      static const char shader_templ[] =
      "FRAG\n"
   "DCL IN[0], GENERIC[0], LINEAR\n"
   "DCL SAMP[0]\n"
   "DCL SVIEW[0], %s, UINT\n"
   "DCL CONST[0][0]\n"
                  /* Nearest filtering floors and then converts to integer, and then
   * applies clamp to edge as clamp(coord, 0, dim - 1).
   */
   "MOV TEMP[0], IN[0]\n"
   "FLR TEMP[0].xy, TEMP[0]\n"
   "F2I TEMP[0], TEMP[0]\n"
   "IMAX TEMP[0].xy, TEMP[0], IMM[0].xxxx\n"
   /* Clamp to edge for the upper bound. */
   "TXQ TEMP[1].xy, IMM[0].xxxx, SAMP[0], %s\n"
   "UADD TEMP[1].xy, TEMP[1], IMM[0].yyyy\n" /* width - 1, height - 1 */
   "IMIN TEMP[0].xy, TEMP[0], TEMP[1]\n"
   /* Texel fetch. */
   "TXF_LZ TEMP[0].x, TEMP[0], SAMP[0], %s\n"
   "AND TEMP[0].x, TEMP[0], CONST[0][0]\n"
   "USNE TEMP[0].x, TEMP[0], CONST[0][0]\n"
   "U2F TEMP[0].x, TEMP[0]\n"
               sprintf(text, shader_templ, tgsi_texture_names[tgsi_tex],
      } else {
      static const char shader_templ[] =
      "FRAG\n"
   "DCL IN[0], GENERIC[0], LINEAR\n"
   "DCL SAMP[0]\n"
   "DCL SVIEW[0], %s, UINT\n"
   "DCL CONST[0][0]\n"
                  /* Nearest filtering floors and then converts to integer, and then
   * applies clamp to edge as clamp(coord, 0, dim - 1).
   */
   "MOV TEMP[0], IN[0]\n"
   "FLR TEMP[0].xy, TEMP[0]\n"
   "F2I TEMP[0], TEMP[0]\n"
   "IMAX TEMP[0].xy, TEMP[0], IMM[0].xxxx\n"
   /* Texel fetch. */
   "TXF_LZ TEMP[0].x, TEMP[0], SAMP[0], %s\n"
   "AND TEMP[0].x, TEMP[0], CONST[0][0]\n"
   "USNE TEMP[0].x, TEMP[0], CONST[0][0]\n"
   "U2F TEMP[0].x, TEMP[0]\n"
               sprintf(text, shader_templ, tgsi_texture_names[tgsi_tex],
               if (!tgsi_text_translate(text, tokens, ARRAY_SIZE(tokens))) {
      assert(0);
                           }
      void *
   util_make_fs_clear_all_cbufs(struct pipe_context *pipe)
   {
      static const char text[] =
      "FRAG\n"
   "PROPERTY FS_COLOR0_WRITES_ALL_CBUFS 1\n"
   "DCL OUT[0], COLOR[0]\n"
            "MOV OUT[0], CONST[0][0]\n"
         struct tgsi_token tokens[1000];
            if (!tgsi_text_translate(text, tokens, ARRAY_SIZE(tokens))) {
      assert(0);
                           }
