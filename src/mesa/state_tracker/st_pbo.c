   /*
   * Copyright 2007 VMware, Inc.
   * Copyright 2016 Advanced Micro Devices, Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file
   *
   * Common helper functions for PBO up- and downloads.
   */
      #include "state_tracker/st_context.h"
   #include "state_tracker/st_nir.h"
   #include "state_tracker/st_pbo.h"
      #include "main/context.h"
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "cso_cache/cso_context.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_upload_mgr.h"
      #include "compiler/nir/nir_builder.h"
      /* Final setup of buffer addressing information.
   *
   * buf_offset is in pixels.
   *
   * Returns false if something (e.g. alignment) prevents PBO upload/download.
   */
   bool
   st_pbo_addresses_setup(struct st_context *st,
               {
               /* Check alignment against texture buffer requirements. */
   {
      unsigned ofs = (buf_offset * addr->bytes_per_pixel) % st->ctx->Const.TextureBufferOffsetAlignment;
   if (ofs != 0) {
                     skip_pixels = ofs / addr->bytes_per_pixel;
      } else {
                              addr->buffer = buf;
   addr->first_element = buf_offset;
   addr->last_element = buf_offset + skip_pixels + addr->width - 1
            if (addr->last_element - addr->first_element > st->ctx->Const.MaxTextureBufferSize - 1)
            /* This should be ensured by Mesa before calling our callbacks */
            addr->constants.xoffset = -addr->xoffset + skip_pixels;
   addr->constants.yoffset = -addr->yoffset;
   addr->constants.stride = addr->pixels_per_row;
   addr->constants.image_size = addr->pixels_per_row * addr->image_height;
               }
      /* Validate and fill buffer addressing information based on GL pixelstore
   * attributes.
   *
   * Returns false if some aspect of the addressing (e.g. alignment) prevents
   * PBO upload/download.
   */
   bool
   st_pbo_addresses_pixelstore(struct st_context *st,
                           {
      struct pipe_resource *buf = store->BufferObj->buffer;
            if (buf_offset % addr->bytes_per_pixel)
            /* Convert to texels */
            /* Determine image height */
   if (gl_target == GL_TEXTURE_1D_ARRAY) {
         } else {
                  /* Compute the stride, taking store->Alignment into account */
   {
      unsigned pixels_per_row = store->RowLength > 0 ?
         unsigned bytes_per_row = pixels_per_row * addr->bytes_per_pixel;
   unsigned remainder = bytes_per_row % store->Alignment;
            if (remainder > 0)
            if (bytes_per_row % addr->bytes_per_pixel)
                     offset_rows = store->SkipRows;
   if (skip_images)
                        if (!st_pbo_addresses_setup(st, buf, buf_offset, addr))
            /* Support GL_PACK_INVERT_MESA */
   if (store->Invert) {
      addr->constants.xoffset += (addr->height - 1) * addr->constants.stride;
                  }
      /* For download from a framebuffer, we may have to invert the Y axis. The
   * setup is as follows:
   * - set viewport to inverted, so that the position sysval is correct for
   *   texel fetches
   * - this function adjusts the fragment shader's constant buffer to compute
   *   the correct destination addresses.
   */
   void
   st_pbo_addresses_invert_y(struct st_pbo_addresses *addr,
         {
      addr->constants.xoffset +=
            }
      /* Setup all vertex pipeline state, rasterizer state, and fragment shader
   * constants, and issue the draw call for PBO upload/download.
   *
   * The caller is responsible for saving and restoring state, as well as for
   * setting other fragment shader state (fragment shader, samplers), and
   * framebuffer/viewport/DSA/blend state.
   */
   bool
   st_pbo_draw(struct st_context *st, const struct st_pbo_addresses *addr,
         {
      struct cso_context *cso = st->cso_context;
            /* Setup vertex and geometry shaders */
   if (!st->pbo.vs) {
      st->pbo.vs = st_pbo_create_vs(st);
   if (!st->pbo.vs)
               if (addr->depth != 1 && st->pbo.use_gs && !st->pbo.gs) {
      st->pbo.gs = st_pbo_create_gs(st);
   if (!st->pbo.gs)
                                                   /* Upload vertices */
   {
      struct pipe_vertex_buffer vbo = {0};
            float x0 = (float) addr->xoffset / surface_width * 2.0f - 1.0f;
   float y0 = (float) addr->yoffset / surface_height * 2.0f - 1.0f;
   float x1 = (float) (addr->xoffset + addr->width) / surface_width * 2.0f - 1.0f;
                     u_upload_alloc(st->pipe->stream_uploader, 0, 8 * sizeof(float), 4,
         if (!verts)
            verts[0] = x0;
   verts[1] = y0;
   verts[2] = x0;
   verts[3] = y1;
   verts[4] = x1;
   verts[5] = y0;
   verts[6] = x1;
                     velem.count = 1;
   velem.velems[0].src_offset = 0;
   velem.velems[0].src_stride = 2 * sizeof(float);
   velem.velems[0].instance_divisor = 0;
   velem.velems[0].vertex_buffer_index = 0;
   velem.velems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;
                     cso_set_vertex_buffers(cso, 1, 0, false, &vbo);
                        /* Upload constants */
   {
               cb.buffer = NULL;
   cb.user_buffer = &addr->constants;
   cb.buffer_offset = 0;
                                 /* Rasterizer state */
            /* Disable stream output */
            if (addr->depth == 1) {
         } else {
      cso_draw_arrays_instanced(cso, MESA_PRIM_TRIANGLE_STRIP,
                  }
      void *
   st_pbo_create_vs(struct st_context *st)
   {
      const struct glsl_type *vec4 = glsl_vec4_type();
   const nir_shader_compiler_options *options =
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_VERTEX, options,
            nir_variable *in_pos = nir_create_variable_with_location(b.shader, nir_var_shader_in,
            nir_variable *out_pos = nir_create_variable_with_location(b.shader, nir_var_shader_out,
            if (!st->pbo.use_gs)
            if (st->pbo.layers) {
      nir_variable *instance_id = nir_create_variable_with_location(b.shader, nir_var_system_value,
            if (st->pbo.use_gs) {
      nir_store_var(&b, out_pos,
                  } else {
      nir_variable *out_layer = nir_create_variable_with_location(b.shader, nir_var_shader_out,
         out_layer->data.interpolation = INTERP_MODE_NONE;
                     }
      void *
   st_pbo_create_gs(struct st_context *st)
   {
      const nir_shader_compiler_options *options =
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_GEOMETRY, options,
            b.shader->info.gs.input_primitive = MESA_PRIM_TRIANGLES;
   b.shader->info.gs.output_primitive = MESA_PRIM_TRIANGLE_STRIP;
   b.shader->info.gs.vertices_in = 3;
   b.shader->info.gs.vertices_out = 3;
   b.shader->info.gs.invocations = 1;
            const struct glsl_type *in_type = glsl_array_type(glsl_vec4_type(), 3, 0);
   nir_variable *in_pos = nir_variable_create(b.shader, nir_var_shader_in, in_type, "in_pos");
   in_pos->data.location = VARYING_SLOT_POS;
            nir_variable *out_pos = nir_create_variable_with_location(b.shader, nir_var_shader_out,
                     nir_variable *out_layer = nir_create_variable_with_location(b.shader, nir_var_shader_out,
         out_layer->data.interpolation = INTERP_MODE_NONE;
            for (int i = 0; i < 3; ++i) {
               nir_store_var(&b, out_pos, nir_vector_insert_imm(&b, pos, nir_imm_float(&b, 0.0), 2), 0xf);
   /* out_layer.x = f2i(in_pos[i].z) */
                           }
      const struct glsl_type *
   st_pbo_sampler_type_for_target(enum pipe_texture_target target,
         {
      bool is_array = target >= PIPE_TEXTURE_1D_ARRAY;
   static const enum glsl_sampler_dim dim[] = {
      [PIPE_BUFFER]             = GLSL_SAMPLER_DIM_BUF,
   [PIPE_TEXTURE_1D]         = GLSL_SAMPLER_DIM_1D,
   [PIPE_TEXTURE_2D]         = GLSL_SAMPLER_DIM_2D,
   [PIPE_TEXTURE_3D]         = GLSL_SAMPLER_DIM_3D,
   [PIPE_TEXTURE_CUBE]       = GLSL_SAMPLER_DIM_CUBE,
   [PIPE_TEXTURE_RECT]       = GLSL_SAMPLER_DIM_RECT,
   [PIPE_TEXTURE_1D_ARRAY]   = GLSL_SAMPLER_DIM_1D,
   [PIPE_TEXTURE_2D_ARRAY]   = GLSL_SAMPLER_DIM_2D,
               static const enum glsl_base_type type[] = {
      [ST_PBO_CONVERT_FLOAT] = GLSL_TYPE_FLOAT,
   [ST_PBO_CONVERT_UINT] = GLSL_TYPE_UINT,
   [ST_PBO_CONVERT_UINT_TO_SINT] = GLSL_TYPE_UINT,
   [ST_PBO_CONVERT_SINT] = GLSL_TYPE_INT,
                  }
         static void *
   create_fs(struct st_context *st, bool download,
            enum pipe_texture_target target,
   enum st_pbo_conversion conversion,
      {
      struct pipe_screen *screen = st->screen;
   const nir_shader_compiler_options *options =
         bool pos_is_sysval =
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT, options,
                                 /* param = [ -xoffset + skip_pixels, -yoffset, stride, image_height ] */
   nir_variable *param_var =
         b.shader->num_uniforms += 4;
            nir_variable *fragcoord;
   if (pos_is_sysval)
      fragcoord = nir_create_variable_with_location(b.shader, nir_var_system_value,
      else
      fragcoord = nir_create_variable_with_location(b.shader, nir_var_shader_in,
               /* When st->pbo.layers == false, it is guaranteed we only have a single
   * layer. But we still need the "layer" variable to add the "array"
   * coordinate to the texture. Hence we set layer to zero when array texture
   * is used in case only a single layer is required.
   */
   nir_def *layer = NULL;
   if (!download || target == PIPE_TEXTURE_1D_ARRAY ||
                  target == PIPE_TEXTURE_2D_ARRAY ||
   target == PIPE_TEXTURE_3D ||
   if (need_layer) {
      assert(st->pbo.layers);
   nir_variable *var = nir_create_variable_with_location(b.shader, nir_var_shader_in,
         var->data.interpolation = INTERP_MODE_FLAT;
      }
   else {
                     /* offset_pos = param.xy + f2i(coord.xy) */
   nir_def *offset_pos =
      nir_iadd(&b, nir_channels(&b, param, TGSI_WRITEMASK_XY),
         /* addr = offset_pos.x + offset_pos.y * stride */
   nir_def *pbo_addr =
      nir_iadd(&b, nir_channel(&b, offset_pos, 0),
            if (layer && layer != zero) {
      /* pbo_addr += image_height * layer */
   pbo_addr = nir_iadd(&b, pbo_addr,
               nir_def *texcoord;
   if (download) {
               if (target == PIPE_TEXTURE_1D) {
      unsigned sw = 0;
               if (layer) {
               if (target == PIPE_TEXTURE_3D) {
      nir_variable *layer_offset_var =
      nir_variable_create(b.shader, nir_var_uniform,
      b.shader->num_uniforms += 1;
                              if (target == PIPE_TEXTURE_1D_ARRAY) {
      texcoord = nir_vec2(&b, nir_channel(&b, texcoord, 0),
      } else {
      texcoord = nir_vec3(&b, nir_channel(&b, texcoord, 0),
                  } else {
                  nir_variable *tex_var =
      nir_variable_create(b.shader, nir_var_uniform,
            tex_var->data.explicit_binding = true;
                     nir_tex_instr *tex = nir_tex_instr_create(b.shader, 3);
   tex->op = nir_texop_txf;
   tex->sampler_dim = glsl_get_sampler_dim(tex_var->type);
   tex->coord_components =
                  tex->dest_type = nir_get_nir_type_for_glsl_base_type(glsl_get_sampler_result_type(tex_var->type));
   tex->src[0].src_type = nir_tex_src_texture_deref;
   tex->src[0].src = nir_src_for_ssa(&tex_deref->def);
   tex->src[1].src_type = nir_tex_src_sampler_deref;
   tex->src[1].src = nir_src_for_ssa(&tex_deref->def);
   tex->src[2].src_type = nir_tex_src_coord;
   tex->src[2].src = nir_src_for_ssa(texcoord);
   nir_def_init(&tex->instr, &tex->def, 4, 32);
   nir_builder_instr_insert(&b, &tex->instr);
            if (conversion == ST_PBO_CONVERT_SINT_TO_UINT)
         else if (conversion == ST_PBO_CONVERT_UINT_TO_SINT)
            if (download) {
      static const enum glsl_base_type type[] = {
      [ST_PBO_CONVERT_FLOAT] = GLSL_TYPE_FLOAT,
   [ST_PBO_CONVERT_UINT] = GLSL_TYPE_UINT,
   [ST_PBO_CONVERT_UINT_TO_SINT] = GLSL_TYPE_INT,
   [ST_PBO_CONVERT_SINT] = GLSL_TYPE_INT,
      };
   nir_variable *img_var =
      nir_variable_create(b.shader, nir_var_image,
            img_var->data.access = ACCESS_NON_READABLE;
   img_var->data.explicit_binding = true;
   img_var->data.binding = 0;
   img_var->data.image.format = format;
            nir_image_deref_store(&b, &img_deref->def,
                        nir_vec4(&b, pbo_addr, zero, zero, zero),
   } else {
      nir_variable *color =
                                 }
      static enum st_pbo_conversion
   get_pbo_conversion(enum pipe_format src_format, enum pipe_format dst_format)
   {
      if (util_format_is_pure_uint(src_format)) {
      if (util_format_is_pure_uint(dst_format))
         if (util_format_is_pure_sint(dst_format))
      } else if (util_format_is_pure_sint(src_format)) {
      if (util_format_is_pure_sint(dst_format))
         if (util_format_is_pure_uint(dst_format))
                  }
      void *
   st_pbo_get_upload_fs(struct st_context *st,
                     {
                        if (!st->pbo.upload_fs[conversion][need_layer])
               }
      void *
   st_pbo_get_download_fs(struct st_context *st, enum pipe_texture_target target,
                     {
      STATIC_ASSERT(ARRAY_SIZE(st->pbo.download_fs) == ST_NUM_PBO_CONVERSIONS);
            struct pipe_screen *screen = st->screen;
   enum st_pbo_conversion conversion = get_pbo_conversion(src_format, dst_format);
            /* For drivers not supporting formatless storing, download FS is stored in an
   * indirect dynamically allocated array of storing formats.
   */
   if (!formatless_store && !st->pbo.download_fs[conversion][target][need_layer])
            if (formatless_store) {
      if (!st->pbo.download_fs[conversion][target][need_layer])
            } else {
      void **fs_array = (void **)st->pbo.download_fs[conversion][target][need_layer];
   if (!fs_array[dst_format])
               }
      void
   st_init_pbo_helpers(struct st_context *st)
   {
               st->pbo.upload_enabled =
      screen->get_param(screen, PIPE_CAP_TEXTURE_BUFFER_OBJECTS) &&
   screen->get_param(screen, PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT) >= 1 &&
      if (!st->pbo.upload_enabled)
            st->pbo.download_enabled =
      st->pbo.upload_enabled &&
   screen->get_param(screen, PIPE_CAP_SAMPLER_VIEW_TARGET) &&
   screen->get_param(screen, PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT) &&
   screen->get_shader_param(screen, PIPE_SHADER_FRAGMENT,
         st->pbo.rgba_only =
            if (screen->get_param(screen, PIPE_CAP_VS_INSTANCEID)) {
      if (screen->get_param(screen, PIPE_CAP_VS_LAYER_VIEWPORT)) {
         } else if (screen->get_param(screen, PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES) >= 3) {
      st->pbo.layers = true;
                  /* Blend state */
   memset(&st->pbo.upload_blend, 0, sizeof(struct pipe_blend_state));
            /* Rasterizer state */
   memset(&st->pbo.raster, 0, sizeof(struct pipe_rasterizer_state));
            const char *pbo = debug_get_option("MESA_COMPUTE_PBO", NULL);
   if (pbo) {
      st->force_compute_based_texture_transfer = true;
               if (st->allow_compute_based_texture_transfer || st->force_compute_based_texture_transfer)
      }
      void
   st_destroy_pbo_helpers(struct st_context *st)
   {
      struct pipe_screen *screen = st->screen;
   bool formatless_store = screen->get_param(screen, PIPE_CAP_IMAGE_STORE_FORMATTED);
            for (i = 0; i < ARRAY_SIZE(st->pbo.upload_fs); ++i) {
      for (unsigned j = 0; j < ARRAY_SIZE(st->pbo.upload_fs[0]); j++) {
      if (st->pbo.upload_fs[i][j]) {
      st->pipe->delete_fs_state(st->pipe, st->pbo.upload_fs[i][j]);
                     for (i = 0; i < ARRAY_SIZE(st->pbo.download_fs); ++i) {
      for (unsigned j = 0; j < ARRAY_SIZE(st->pbo.download_fs[0]); ++j) {
      for (unsigned k = 0; k < ARRAY_SIZE(st->pbo.download_fs[0][0]); k++) {
      if (st->pbo.download_fs[i][j][k]) {
      if (formatless_store) {
         } else {
      void **fs_array = (void **)st->pbo.download_fs[i][j][k];
   for (unsigned l = 0; l < PIPE_FORMAT_COUNT; l++)
      if (fs_array[l])
         }
                        if (st->pbo.gs) {
      st->pipe->delete_gs_state(st->pipe, st->pbo.gs);
               if (st->pbo.vs) {
      st->pipe->delete_vs_state(st->pipe, st->pbo.vs);
                  }
