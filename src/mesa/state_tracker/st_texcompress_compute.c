   /**************************************************************************
   *
   * Copyright © 2022 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "compiler/glsl/astc_glsl.h"
   #include "compiler/glsl/bc1_glsl.h"
   #include "compiler/glsl/bc4_glsl.h"
   #include "compiler/glsl/cross_platform_settings_piece_all.h"
   #include "compiler/glsl/etc2_rgba_stitch_glsl.h"
      #include "main/context.h"
   #include "main/shaderapi.h"
   #include "main/shaderobj.h"
   #include "main/texcompress_astc.h"
   #include "util/texcompress_astc_luts_wrap.h"
   #include "main/uniforms.h"
      #include "state_tracker/st_atom_constbuf.h"
   #include "state_tracker/st_bc1_tables.h"
   #include "state_tracker/st_context.h"
   #include "state_tracker/st_program.h"
   #include "state_tracker/st_texcompress_compute.h"
   #include "state_tracker/st_texture.h"
      #include "util/u_hash_table.h"
   #include "util/u_string.h"
      enum compute_program_id {
      COMPUTE_PROGRAM_BC1,
   COMPUTE_PROGRAM_BC4,
   COMPUTE_PROGRAM_STITCH,
   COMPUTE_PROGRAM_ASTC_4x4,
   COMPUTE_PROGRAM_ASTC_5x4,
   COMPUTE_PROGRAM_ASTC_5x5,
   COMPUTE_PROGRAM_ASTC_6x5,
   COMPUTE_PROGRAM_ASTC_6x6,
   COMPUTE_PROGRAM_ASTC_8x5,
   COMPUTE_PROGRAM_ASTC_8x6,
   COMPUTE_PROGRAM_ASTC_8x8,
   COMPUTE_PROGRAM_ASTC_10x5,
   COMPUTE_PROGRAM_ASTC_10x6,
   COMPUTE_PROGRAM_ASTC_10x8,
   COMPUTE_PROGRAM_ASTC_10x10,
   COMPUTE_PROGRAM_ASTC_12x10,
   COMPUTE_PROGRAM_ASTC_12x12,
      };
      static struct gl_program * PRINTFLIKE(3, 4)
   get_compute_program(struct st_context *st,
               {
      /* Try to get the program from the cache. */
   assert(prog_id < COMPUTE_PROGRAM_COUNT);
   if (st->texcompress_compute.progs[prog_id])
            /* Cache miss. Create the final source string. */
   char *source_str;
   va_list ap;
   va_start(ap, source_fmt);
   int num_printed_bytes = vasprintf(&source_str, source_fmt, ap);
   va_end(ap);
   if (num_printed_bytes == -1)
            /* Compile and link the shader. Then, destroy the shader string. */
   const char *strings[] = { source_str };
   GLuint program =
                  struct gl_shader_program *shProg =
         if (!shProg)
            if (shProg->data->LinkStatus == LINKING_FAILURE) {
      fprintf(stderr, "Linking failed:\n%s\n", shProg->data->InfoLog);
   _mesa_reference_shader_program(st->ctx, &shProg, NULL);
               /* Cache the program and return it. */
   return st->texcompress_compute.progs[prog_id] =
      }
      static struct pipe_resource *
   create_bc1_endpoint_ssbo(struct pipe_context *pipe)
   {
      struct pipe_resource *buffer =
      pipe_buffer_create(pipe->screen, PIPE_BIND_SHADER_BUFFER,
               if (!buffer)
            struct pipe_transfer *transfer;
   float (*buffer_map)[2] = pipe_buffer_map(pipe, buffer,
                     if (!buffer_map) {
      pipe_resource_reference(&buffer, NULL);
               for (int i = 0; i < 256; i++) {
      for (int j = 0; j < 2; j++) {
      buffer_map[i][j] = (float) stb__OMatch5[i][j];
                              }
      static void
   bind_compute_state(struct st_context *st,
                     struct gl_program *prog,
   struct pipe_sampler_view **sampler_views,
   const struct pipe_shader_buffer *shader_buffers,
   {
                        assert(prog->affected_states & ST_NEW_CS_STATE);
   assert(st->shader_has_one_variant[PIPE_SHADER_COMPUTE]);
   cso_set_compute_shader_handle(st->cso_context,
                  if (prog->affected_states & ST_NEW_CS_SAMPLER_VIEWS) {
      st->pipe->set_sampler_views(st->pipe, prog->info.stage, 0,
                     if (prog->affected_states & ST_NEW_CS_SAMPLERS) {
      /* Programs seem to set this bit more often than needed. For example, if
   * a program only uses texelFetch, this shouldn't be needed. Section
   * "11.1.3.2 Texel Fetches", of the GL 4.6 spec says:
   *
   *    Texel fetch proceeds similarly to the steps described for texture
   *    access in section 11.1.3.5, with the exception that none of the
   *    operations controlled by sampler object state are performed,
   *
   * We assume that the program is using texelFetch or doesn't care about
   * this state for a similar reason.
   *
   * See https://gitlab.freedesktop.org/mesa/mesa/-/issues/8014.
               if (prog->affected_states & ST_NEW_CS_CONSTANTS) {
      st_upload_constants(st, constbuf0_from_prog ? prog : NULL,
               if (prog->affected_states & ST_NEW_CS_UBOS) {
                  if (prog->affected_states & ST_NEW_CS_ATOMICS) {
                  if (prog->affected_states & ST_NEW_CS_SSBOS) {
      st->pipe->set_shader_buffers(st->pipe, prog->info.stage, 0,
                     if (prog->affected_states & ST_NEW_CS_IMAGES) {
      st->pipe->set_shader_images(st->pipe, prog->info.stage, 0,
         }
      static void
   dispatch_compute_state(struct st_context *st,
                        struct gl_program *prog,
   struct pipe_sampler_view **sampler_views,
   const struct pipe_shader_buffer *shader_buffers,
      {
               /* Bind the state */
   bind_compute_state(st, prog, sampler_views, shader_buffers, image_views,
            /* Launch the grid */
   const struct pipe_grid_info info = {
      .block[0] = prog->info.workgroup_size[0],
   .block[1] = prog->info.workgroup_size[1],
   .block[2] = prog->info.workgroup_size[2],
   .grid[0] = num_workgroups_x,
   .grid[1] = num_workgroups_y,
                        /* Unbind the state */
            /* If the previously used compute program was relying on any state that was
   * trampled on by these state changes, dirty the relevant flags.
   */
   if (st->cp) {
      st->ctx->NewDriverState |=
         }
      static struct pipe_resource *
   cs_encode_bc1(struct st_context *st,
         {
      /* Create the required compute state */
   struct gl_program *prog =
      get_compute_program(st, COMPUTE_PROGRAM_BC1, bc1_source,
      if (!prog)
            /* ... complete the program setup by defining the number of refinements to
   * do on the created blocks. The program will attempt to create a more
   * accurate encoding on each iteration. Doing at least one refinement
   * provides a significant improvement in quality and is needed to give a
   * result comparable to the CPU encoder (according to piglit tests).
   * Additional refinements don't help as much.
   */
   const unsigned num_refinements = 1;
   _mesa_uniform(0, 1, &num_refinements, st->ctx, prog->shader_program,
            const struct pipe_sampler_view templ = {
      .target = PIPE_TEXTURE_2D,
   .format = PIPE_FORMAT_R8G8B8A8_UNORM,
   .swizzle_r = PIPE_SWIZZLE_X,
   .swizzle_g = PIPE_SWIZZLE_Y,
   .swizzle_b = PIPE_SWIZZLE_Z,
      };
   struct pipe_sampler_view *rgba8_view =
         if (!rgba8_view)
            const struct pipe_shader_buffer ssbo = {
      .buffer = st->texcompress_compute.bc1_endpoint_buf,
               struct pipe_resource *bc1_tex =
      st_texture_create(st, PIPE_TEXTURE_2D, PIPE_FORMAT_R32G32_UINT, 0,
                        if (!bc1_tex)
            const struct pipe_image_view image = {
      .resource = bc1_tex,
   .format = PIPE_FORMAT_R16G16B16A16_UINT,
   .access = PIPE_IMAGE_ACCESS_WRITE,
               /* Dispatch the compute state */
   dispatch_compute_state(st, prog, &rgba8_view, &ssbo, &image,
               release_sampler_views:
                  }
      static struct pipe_resource *
   cs_encode_bc4(struct st_context *st,
               {
      /* Create the required compute state */
   struct gl_program *prog =
      get_compute_program(st, COMPUTE_PROGRAM_BC4, bc4_source,
      if (!prog)
            /* ... complete the program setup by picking the channel to encode and
   * whether to encode it as snorm. The shader doesn't actually support
   * channel index 2. So, pick index 0 and rely on swizzling instead.
   */
   const unsigned params[] = { 0, use_snorm };
   _mesa_uniform(0, 1, params, st->ctx, prog->shader_program,
            const struct pipe_sampler_view templ = {
      .target = PIPE_TEXTURE_2D,
   .format = PIPE_FORMAT_R8G8B8A8_UNORM,
   .swizzle_r = component,
   .swizzle_g = PIPE_SWIZZLE_0,
   .swizzle_b = PIPE_SWIZZLE_0,
      };
   struct pipe_sampler_view *rgba8_view =
         if (!rgba8_view)
            struct pipe_resource *bc4_tex =
      st_texture_create(st, PIPE_TEXTURE_2D, PIPE_FORMAT_R32G32_UINT, 0,
                        if (!bc4_tex)
            const struct pipe_image_view image = {
      .resource = bc4_tex,
   .format = PIPE_FORMAT_R16G16B16A16_UINT,
   .access = PIPE_IMAGE_ACCESS_WRITE,
               /* Dispatch the compute state */
   dispatch_compute_state(st, prog, &rgba8_view, NULL, &image, 1,
               release_sampler_views:
                  }
      static struct pipe_resource *
   cs_stitch_64bpb_textures(struct st_context *st,
               {
      assert(util_format_get_blocksizebits(tex_hi->format) == 64);
   assert(util_format_get_blocksizebits(tex_lo->format) == 64);
   assert(tex_hi->width0 == tex_lo->width0);
                     /* Create the required compute state */
   struct gl_program *prog =
      get_compute_program(st, COMPUTE_PROGRAM_STITCH, etc2_rgba_stitch_source,
      if (!prog)
            const struct pipe_sampler_view templ = {
      .target = PIPE_TEXTURE_2D,
   .format = PIPE_FORMAT_R32G32_UINT,
   .swizzle_r = PIPE_SWIZZLE_X,
   .swizzle_g = PIPE_SWIZZLE_Y,
   .swizzle_b = PIPE_SWIZZLE_0,
      };
   struct pipe_sampler_view *rg32_views[2] = {
      [0] = st->pipe->create_sampler_view(st->pipe, tex_hi, &templ),
      };
   if (!rg32_views[0] || !rg32_views[1])
            stitched_tex =
      st_texture_create(st, PIPE_TEXTURE_2D, PIPE_FORMAT_R32G32B32A32_UINT, 0,
                        if (!stitched_tex)
            const struct pipe_image_view image = {
      .resource = stitched_tex,
   .format = PIPE_FORMAT_R32G32B32A32_UINT,
   .access = PIPE_IMAGE_ACCESS_WRITE,
               /* Dispatch the compute state */
   dispatch_compute_state(st, prog, rg32_views, NULL, &image,
               release_sampler_views:
      pipe_sampler_view_reference(&rg32_views[0], NULL);
               }
      static struct pipe_resource *
   cs_encode_bc3(struct st_context *st,
         {
               /* Encode RGB channels as BC1. */
   struct pipe_resource *bc1_tex = cs_encode_bc1(st, rgba8_tex);
   if (!bc1_tex)
            /* Encode alpha channels as BC4. */
   struct pipe_resource *bc4_tex =
         if (!bc4_tex)
                     /* Combine BC1 and BC4 to create BC3. */
   bc3_tex = cs_stitch_64bpb_textures(st, bc1_tex, bc4_tex);
   if (!bc3_tex)
         release_textures:
      pipe_resource_reference(&bc1_tex, NULL);
               }
      static struct pipe_resource *
   sw_decode_astc(struct st_context *st,
                  uint8_t *astc_data,
      {
      /* Create the destination */
   struct pipe_resource *rgba8_tex =
      st_texture_create(st, PIPE_TEXTURE_2D, PIPE_FORMAT_R8G8B8A8_UNORM, 0,
            if (!rgba8_tex)
            /* Temporarily map the destination and decode into the returned pointer */
   struct pipe_transfer *rgba8_xfer;
   void *rgba8_map = pipe_texture_map(st->pipe, rgba8_tex, 0, 0,
               if (!rgba8_map) {
      pipe_resource_reference(&rgba8_tex, NULL);
               _mesa_unpack_astc_2d_ldr(rgba8_map, rgba8_xfer->stride,
                              }
      static struct pipe_sampler_view *
   create_astc_cs_payload_view(struct st_context *st,
               {
      const struct pipe_resource src_templ = {
      .target = PIPE_TEXTURE_2D,
   .format = PIPE_FORMAT_R32G32B32A32_UINT,
   .bind = PIPE_BIND_SAMPLER_VIEW,
   .usage = PIPE_USAGE_STAGING,
   .width0 = width_el,
   .height0 = height_el,
   .depth0 = 1,
               struct pipe_resource *payload_res =
            if (!payload_res)
            struct pipe_box box;
            st->pipe->texture_subdata(st->pipe, payload_res, 0, 0,
                              const struct pipe_sampler_view view_templ = {
      .target = PIPE_TEXTURE_2D,
   .format = payload_res->format,
   .swizzle_r = PIPE_SWIZZLE_X,
   .swizzle_g = PIPE_SWIZZLE_Y,
   .swizzle_b = PIPE_SWIZZLE_Z,
               struct pipe_sampler_view *view =
                        }
      static struct pipe_sampler_view *
   get_astc_partition_table_view(struct st_context *st,
               {
      unsigned lut_width;
   unsigned lut_height;
   struct pipe_box ptable_box;
   void *ptable_data =
                  struct pipe_sampler_view *view =
      util_hash_table_get(st->texcompress_compute.astc_partition_tables,
         if (view)
            struct pipe_resource *res =
      st_texture_create(st, PIPE_TEXTURE_2D, PIPE_FORMAT_R8_UINT, 0,
                  if (!res)
            st->pipe->texture_subdata(st->pipe, res, 0, 0,
                              const struct pipe_sampler_view templ = {
      .target = PIPE_TEXTURE_2D,
   .format = res->format,
   .swizzle_r = PIPE_SWIZZLE_X,
   .swizzle_g = PIPE_SWIZZLE_Y,
   .swizzle_b = PIPE_SWIZZLE_Z,
                                 if (view) {
      _mesa_hash_table_insert(st->texcompress_compute.astc_partition_tables,
         ASSERTED const unsigned max_entries =
         assert(_mesa_hash_table_num_entries(
                  }
      static struct pipe_resource *
   cs_decode_astc(struct st_context *st,
                  uint8_t *astc_data,
      {
      const enum compute_program_id astc_id = COMPUTE_PROGRAM_ASTC_4x4 +
            unsigned block_w, block_h;
            struct gl_program *prog =
            if (!prog)
            struct pipe_sampler_view *ptable_view =
            if (!ptable_view)
            struct pipe_sampler_view *payload_view =
      create_astc_cs_payload_view(st, astc_data, astc_stride,
               if (!payload_view)
            /* Create the destination */
   struct pipe_resource *rgba8_tex =
      st_texture_create(st, PIPE_TEXTURE_2D, PIPE_FORMAT_R8G8B8A8_UNORM, 0,
               if (!rgba8_tex)
            const struct pipe_image_view image = {
      .resource = rgba8_tex,
   .format = PIPE_FORMAT_R8G8B8A8_UINT,
   .access = PIPE_IMAGE_ACCESS_WRITE,
               struct pipe_sampler_view *sampler_views[] = {
      st->texcompress_compute.astc_luts[0],
   st->texcompress_compute.astc_luts[1],
   st->texcompress_compute.astc_luts[2],
   st->texcompress_compute.astc_luts[3],
   st->texcompress_compute.astc_luts[4],
   ptable_view,
               dispatch_compute_state(st, prog, sampler_views, NULL, &image,
                     release_payload_view:
                  }
      static struct pipe_sampler_view *
   get_sampler_view_for_lut(struct pipe_context *pipe,
         {
      struct pipe_resource *res =
      pipe_buffer_create_with_data(pipe,
                        if (!res)
            const struct pipe_sampler_view templ = {
      .format = lut->format,
   .target = PIPE_BUFFER,
   .swizzle_r = PIPE_SWIZZLE_X,
   .swizzle_g = PIPE_SWIZZLE_Y,
   .swizzle_b = PIPE_SWIZZLE_Z,
   .swizzle_a = PIPE_SWIZZLE_W,
   .u.buf.offset = 0,
               struct pipe_sampler_view *view =
                        }
      /* Initializes required resources for Granite ASTC GPU decode.
   *
   * There are 5 texture buffer objects and one additional texture required.
   * We initialize 5 tbo's here and a single texture later during runtime.
   */
   static bool
   initialize_astc_decoder(struct st_context *st)
   {
      astc_decoder_lut_holder astc_lut_holder;
            const astc_decoder_lut *luts[] = {
      &astc_lut_holder.color_endpoint,
   &astc_lut_holder.color_endpoint_unquant,
   &astc_lut_holder.weights,
   &astc_lut_holder.weights_unquant,
               for (unsigned i = 0; i < ARRAY_SIZE(luts); i++) {
      st->texcompress_compute.astc_luts[i] =
         if (!st->texcompress_compute.astc_luts[i])
               st->texcompress_compute.astc_partition_tables =
            if (!st->texcompress_compute.astc_partition_tables)
               }
      bool
   st_init_texcompress_compute(struct st_context *st)
   {
      st->texcompress_compute.progs =
         if (!st->texcompress_compute.progs)
            st->texcompress_compute.bc1_endpoint_buf =
         if (!st->texcompress_compute.bc1_endpoint_buf)
            if (!initialize_astc_decoder(st))
               }
      static void
   destroy_astc_decoder(struct st_context *st)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(st->texcompress_compute.astc_luts); i++)
            if (st->texcompress_compute.astc_partition_tables) {
      hash_table_foreach(st->texcompress_compute.astc_partition_tables,
            pipe_sampler_view_reference(
                  _mesa_hash_table_destroy(st->texcompress_compute.astc_partition_tables,
      }
      void
   st_destroy_texcompress_compute(struct st_context *st)
   {
      /* The programs in the array are part of the gl_context (in st->ctx).They
   * are automatically destroyed when the context is destroyed (via
   * _mesa_free_context_data -> ... -> free_shader_program_data_cb).
   */
            /* Destroy the SSBO used by the BC1 shader program. */
               }
      /* See st_texcompress_compute.h for more information. */
   bool
   st_compute_transcode_astc_to_dxt5(struct st_context *st,
                                       {
      assert(_mesa_has_compute_shaders(st->ctx));
   assert(_mesa_is_format_astc_2d(astc_format));
   assert(dxt5_tex->format == PIPE_FORMAT_DXT5_RGBA ||
         assert(dxt5_level <= dxt5_tex->last_level);
                     /* Decode ASTC to RGBA8. */
   struct pipe_resource *rgba8_tex =
      cs_decode_astc(st, astc_data, astc_stride, astc_format,
            if (!rgba8_tex)
                     /* Encode RGBA8 to BC3. */
   struct pipe_resource *bc3_tex = cs_encode_bc3(st, rgba8_tex);
   if (!bc3_tex)
            /* Upload the result. */
   struct pipe_box src_box;
   u_box_origin_2d(bc3_tex->width0, bc3_tex->height0, &src_box);
   st->pipe->resource_copy_region(st->pipe, dxt5_tex, dxt5_level,
                  release_textures:
      pipe_resource_reference(&rgba8_tex, NULL);
               }
