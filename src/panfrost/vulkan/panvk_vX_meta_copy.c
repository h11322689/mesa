   /*
   * Copyright © 2021 Collabora Ltd.
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
   */
      #include "gen_macros.h"
      #include "nir/nir_builder.h"
   #include "pan_encoder.h"
   #include "pan_shader.h"
      #include "panvk_private.h"
      static mali_ptr
   panvk_meta_copy_img_emit_texture(struct panfrost_device *pdev,
               {
      struct panfrost_ptr texture = pan_pool_alloc_desc(desc_pool, TEXTURE);
   size_t payload_size = GENX(panfrost_estimate_texture_payload_size)(view);
   struct panfrost_ptr surfaces = pan_pool_alloc_aligned(
                        }
      static mali_ptr
   panvk_meta_copy_img_emit_sampler(struct panfrost_device *pdev,
         {
               pan_pack(sampler.cpu, SAMPLER, cfg) {
      cfg.seamless_cube_map = false;
   cfg.normalized_coordinates = false;
   cfg.minify_nearest = true;
                  }
      static void
   panvk_meta_copy_emit_varying(struct pan_pool *pool, mali_ptr coordinates,
         {
      struct panfrost_ptr varying = pan_pool_alloc_desc(pool, ATTRIBUTE);
   struct panfrost_ptr varying_buffer =
            pan_pack(varying_buffer.cpu, ATTRIBUTE_BUFFER, cfg) {
      cfg.pointer = coordinates;
   cfg.stride = 4 * sizeof(uint32_t);
               /* Bifrost needs an empty desc to mark end of prefetching */
   pan_pack(varying_buffer.cpu + pan_size(ATTRIBUTE_BUFFER), ATTRIBUTE_BUFFER,
                  pan_pack(varying.cpu, ATTRIBUTE, cfg) {
      cfg.buffer_index = 0;
               *varyings = varying.gpu;
      }
      static void
   panvk_meta_copy_emit_dcd(struct pan_pool *pool, mali_ptr src_coords,
                     {
      pan_pack(out, DRAW, cfg) {
      cfg.thread_storage = tsd;
   cfg.state = rsd;
   cfg.push_uniforms = push_constants;
   cfg.position = dst_coords;
   if (src_coords) {
      panvk_meta_copy_emit_varying(pool, src_coords, &cfg.varying_buffers,
      }
   cfg.viewport = vpd;
   cfg.textures = texture;
         }
      static struct panfrost_ptr
   panvk_meta_copy_emit_tiler_job(struct pan_pool *desc_pool,
                                 {
               panvk_meta_copy_emit_dcd(desc_pool, src_coords, dst_coords, texture, sampler,
                  pan_section_pack(job.cpu, TILER_JOB, PRIMITIVE, cfg) {
      cfg.draw_mode = MALI_DRAW_MODE_TRIANGLE_STRIP;
   cfg.index_count = 4;
               pan_section_pack(job.cpu, TILER_JOB, PRIMITIVE_SIZE, cfg) {
                  void *invoc = pan_section_ptr(job.cpu, TILER_JOB, INVOCATION);
            pan_section_pack(job.cpu, TILER_JOB, PADDING, cfg)
         pan_section_pack(job.cpu, TILER_JOB, TILER, cfg) {
                  panfrost_add_job(desc_pool, scoreboard, MALI_JOB_TYPE_TILER, false, false, 0,
            }
      static struct panfrost_ptr
   panvk_meta_copy_emit_compute_job(struct pan_pool *desc_pool,
                                       {
               void *invoc = pan_section_ptr(job.cpu, COMPUTE_JOB, INVOCATION);
   panfrost_pack_work_groups_compute(invoc, num_wg->x, num_wg->y, num_wg->z,
                  pan_section_pack(job.cpu, COMPUTE_JOB, PARAMETERS, cfg) {
                  panvk_meta_copy_emit_dcd(desc_pool, 0, 0, texture, sampler, 0, tsd, rsd,
                  panfrost_add_job(desc_pool, scoreboard, MALI_JOB_TYPE_COMPUTE, false, false,
            }
      static uint32_t
   panvk_meta_copy_img_bifrost_raw_format(unsigned texelsize)
   {
      switch (texelsize) {
   case 6:
         case 8:
         case 12:
         case 16:
         default:
            }
      static mali_ptr
   panvk_meta_copy_to_img_emit_rsd(struct panfrost_device *pdev,
                           {
      struct panfrost_ptr rsd_ptr = pan_pool_alloc_desc_aggregate(
            bool raw = util_format_get_blocksize(fmt) > 4;
   unsigned fullmask = (1 << util_format_get_nr_components(fmt)) - 1;
   bool partialwrite = fullmask != wrmask && !raw;
            pan_pack(rsd_ptr.cpu, RENDERER_STATE, cfg) {
      pan_shader_prepare_rsd(shader_info, shader, &cfg);
   if (from_img) {
      cfg.shader.varying_count = 1;
   cfg.shader.texture_count = 1;
      }
   cfg.properties.depth_source = MALI_DEPTH_SOURCE_FIXED_FUNCTION;
   cfg.multisample_misc.sample_mask = UINT16_MAX;
   cfg.multisample_misc.depth_function = MALI_FUNC_ALWAYS;
   cfg.stencil_mask_misc.stencil_mask_front = 0xFF;
   cfg.stencil_mask_misc.stencil_mask_back = 0xFF;
   cfg.stencil_front.compare_function = MALI_FUNC_ALWAYS;
   cfg.stencil_front.stencil_fail = MALI_STENCIL_OP_REPLACE;
   cfg.stencil_front.depth_fail = MALI_STENCIL_OP_REPLACE;
   cfg.stencil_front.depth_pass = MALI_STENCIL_OP_REPLACE;
   cfg.stencil_front.mask = 0xFF;
            cfg.properties.allow_forward_pixel_to_be_killed = true;
   cfg.properties.allow_forward_pixel_to_kill = !partialwrite && !readstb;
   cfg.properties.zs_update_operation = MALI_PIXEL_KILL_STRONG_EARLY;
               pan_pack(rsd_ptr.cpu + pan_size(RENDERER_STATE), BLEND, cfg) {
      cfg.round_to_fb_precision = true;
   cfg.load_destination = partialwrite;
   cfg.equation.rgb.a = MALI_BLEND_OPERAND_A_SRC;
   cfg.equation.rgb.b = MALI_BLEND_OPERAND_B_SRC;
   cfg.equation.rgb.c = MALI_BLEND_OPERAND_C_ZERO;
   cfg.equation.alpha.a = MALI_BLEND_OPERAND_A_SRC;
   cfg.equation.alpha.b = MALI_BLEND_OPERAND_B_SRC;
   cfg.equation.alpha.c = MALI_BLEND_OPERAND_C_ZERO;
   cfg.internal.mode =
         cfg.equation.color_mask = partialwrite ? wrmask : 0xf;
   cfg.internal.fixed_function.num_comps = 4;
   if (!raw) {
      cfg.internal.fixed_function.conversion.memory_format =
         cfg.internal.fixed_function.conversion.register_format =
      } else {
               cfg.internal.fixed_function.conversion.memory_format =
         cfg.internal.fixed_function.conversion.register_format =
      (imgtexelsz & 2) ? MALI_REGISTER_FILE_FORMAT_U16
                  }
      static mali_ptr
   panvk_meta_copy_to_buf_emit_rsd(struct panfrost_device *pdev,
                     {
      struct panfrost_ptr rsd_ptr =
            pan_pack(rsd_ptr.cpu, RENDERER_STATE, cfg) {
      pan_shader_prepare_rsd(shader_info, shader, &cfg);
   if (from_img) {
      cfg.shader.texture_count = 1;
                     }
      static mali_ptr
   panvk_meta_copy_img2img_shader(struct panfrost_device *pdev,
                                 {
      nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_FRAGMENT, GENX(pan_shader_get_compiler_options)(),
   "panvk_meta_copy_img2img(srcfmt=%s,dstfmt=%s,%dD%s%s)",
   util_format_name(srcfmt), util_format_name(dstfmt), texdim,
         nir_variable *coord_var = nir_variable_create(
      b.shader, nir_var_shader_in,
      coord_var->data.location = VARYING_SLOT_VAR0;
            nir_tex_instr *tex = nir_tex_instr_create(b.shader, is_ms ? 2 : 1);
   tex->op = is_ms ? nir_texop_txf_ms : nir_texop_txf;
   tex->texture_index = 0;
   tex->is_array = texisarray;
   tex->dest_type =
            switch (texdim) {
   case 1:
      assert(!is_ms);
   tex->sampler_dim = GLSL_SAMPLER_DIM_1D;
      case 2:
      tex->sampler_dim = is_ms ? GLSL_SAMPLER_DIM_MS : GLSL_SAMPLER_DIM_2D;
      case 3:
      assert(!is_ms);
   tex->sampler_dim = GLSL_SAMPLER_DIM_3D;
      default:
                  tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_coord, coord);
            if (is_ms) {
      tex->src[1] =
               nir_def_init(&tex->instr, &tex->def, 4,
                           unsigned dstcompsz =
         unsigned ndstcomps = util_format_get_nr_components(dstfmt);
            if (srcfmt == PIPE_FORMAT_R5G6B5_UNORM && dstfmt == PIPE_FORMAT_R8G8_UNORM) {
      nir_def *rgb = nir_f2u32(
      &b, nir_fmul(&b, texel,
            nir_def *rg = nir_vec2(
      &b,
   nir_ior(&b, nir_channel(&b, rgb, 0),
         nir_ior(&b, nir_ushr_imm(&b, nir_channel(&b, rgb, 1), 3),
      rg = nir_iand_imm(&b, rg, 255);
   texel = nir_fmul_imm(&b, nir_u2f32(&b, rg), 1.0 / 255);
      } else if (srcfmt == PIPE_FORMAT_R8G8_UNORM &&
            nir_def *rg = nir_f2u32(&b, nir_fmul_imm(&b, texel, 255));
   nir_def *rgb = nir_vec3(
      &b, nir_channel(&b, rg, 0),
   nir_ior(&b, nir_ushr_imm(&b, nir_channel(&b, rg, 0), 5),
            rgb = nir_iand(&b, rgb,
               texel = nir_fmul(
      &b, nir_u2f32(&b, rgb),
   nir_vec3(&b, nir_imm_float(&b, 1.0 / 31), nir_imm_float(&b, 1.0 / 63),
         } else {
      assert(srcfmt == dstfmt);
   enum glsl_base_type basetype;
   if (util_format_is_unorm(dstfmt)) {
         } else if (dstcompsz == 16) {
         } else {
      assert(dstcompsz == 32);
               if (dstcompsz == 16)
            texel = nir_trim_vector(&b, texel, ndstcomps);
               nir_variable *out =
                  unsigned fullmask = (1 << ndstcomps) - 1;
   if (dstcompsz > 8 && dstmask != fullmask) {
      nir_def *oldtexel = nir_load_var(&b, out);
            for (unsigned i = 0; i < ndstcomps; i++) {
      if (dstmask & BITFIELD_BIT(i))
         else
                                    struct panfrost_compile_inputs inputs = {
      .gpu_id = pdev->gpu_id,
   .is_blit = true,
                        util_dynarray_init(&binary, NULL);
   pan_shader_preprocess(b.shader, inputs.gpu_id);
   NIR_PASS_V(b.shader, GENX(pan_inline_rt_conversion), pdev, &dstfmt);
                     mali_ptr shader =
            util_dynarray_fini(&binary);
               }
      static enum pipe_format
   panvk_meta_copy_img_format(enum pipe_format fmt)
   {
      /* We can't use a non-compressed format when handling a tiled/AFBC
   * compressed format because the tile size differ (4x4 blocks for
   * compressed formats and 16x16 texels for non-compressed ones).
   */
            /* Pick blendable formats when we can, otherwise pick the UINT variant
   * matching the texel size.
   */
   switch (util_format_get_blocksize(fmt)) {
   case 16:
         case 12:
         case 8:
         case 6:
         case 4:
         case 2:
      return (fmt == PIPE_FORMAT_R5G6B5_UNORM ||
         fmt == PIPE_FORMAT_B5G6R5_UNORM)
      case 1:
         default:
            }
      struct panvk_meta_copy_img2img_format_info {
      enum pipe_format srcfmt;
   enum pipe_format dstfmt;
      } PACKED;
      static const struct panvk_meta_copy_img2img_format_info
      panvk_meta_copy_img2img_fmts[] = {
      {PIPE_FORMAT_R8_UNORM, PIPE_FORMAT_R8_UNORM, 0x1},
   {PIPE_FORMAT_R5G6B5_UNORM, PIPE_FORMAT_R5G6B5_UNORM, 0x7},
   {PIPE_FORMAT_R5G6B5_UNORM, PIPE_FORMAT_R8G8_UNORM, 0x3},
   {PIPE_FORMAT_R8G8_UNORM, PIPE_FORMAT_R5G6B5_UNORM, 0x7},
   {PIPE_FORMAT_R8G8_UNORM, PIPE_FORMAT_R8G8_UNORM, 0x3},
   /* Z24S8(depth) */
   {PIPE_FORMAT_R8G8B8A8_UNORM, PIPE_FORMAT_R8G8B8A8_UNORM, 0x7},
   /* Z24S8(stencil) */
   {PIPE_FORMAT_R8G8B8A8_UNORM, PIPE_FORMAT_R8G8B8A8_UNORM, 0x8},
   {PIPE_FORMAT_R8G8B8A8_UNORM, PIPE_FORMAT_R8G8B8A8_UNORM, 0xf},
   {PIPE_FORMAT_R16G16B16_UINT, PIPE_FORMAT_R16G16B16_UINT, 0x7},
   {PIPE_FORMAT_R32G32_UINT, PIPE_FORMAT_R32G32_UINT, 0x3},
   /* Z32S8X24(depth) */
   {PIPE_FORMAT_R32G32_UINT, PIPE_FORMAT_R32G32_UINT, 0x1},
   /* Z32S8X24(stencil) */
   {PIPE_FORMAT_R32G32_UINT, PIPE_FORMAT_R32G32_UINT, 0x2},
   {PIPE_FORMAT_R32G32B32_UINT, PIPE_FORMAT_R32G32B32_UINT, 0x7},
   };
      static unsigned
   panvk_meta_copy_img2img_format_idx(
         {
      STATIC_ASSERT(ARRAY_SIZE(panvk_meta_copy_img2img_fmts) ==
            for (unsigned i = 0; i < ARRAY_SIZE(panvk_meta_copy_img2img_fmts); i++) {
      if (!memcmp(&key, &panvk_meta_copy_img2img_fmts[i], sizeof(key)))
                  }
      static unsigned
   panvk_meta_copy_img_mask(enum pipe_format imgfmt, VkImageAspectFlags aspectMask)
   {
      if (aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT &&
      aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT) {
                        switch (imgfmt) {
   case PIPE_FORMAT_S8_UINT:
         case PIPE_FORMAT_Z16_UNORM:
         case PIPE_FORMAT_Z16_UNORM_S8_UINT:
         case PIPE_FORMAT_Z24_UNORM_S8_UINT:
         case PIPE_FORMAT_Z24X8_UNORM:
      assert(aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT);
      case PIPE_FORMAT_Z32_FLOAT:
         case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
         default:
            }
      static void
   panvk_meta_copy_img2img(struct panvk_cmd_buffer *cmdbuf,
                     {
      struct panfrost_device *pdev = &cmdbuf->device->physical_device->pdev;
   struct pan_fb_info *fbinfo = &cmdbuf->state.fb.info;
   struct panvk_meta_copy_img2img_format_info key = {
      .srcfmt = panvk_meta_copy_img_format(src->pimage.layout.format),
   .dstfmt = panvk_meta_copy_img_format(dst->pimage.layout.format),
   .dstmask = panvk_meta_copy_img_mask(dst->pimage.layout.format,
                        unsigned texdimidx = panvk_meta_copy_tex_type(
         unsigned fmtidx = panvk_meta_copy_img2img_format_idx(key);
            mali_ptr rsd =
      cmdbuf->device->physical_device->meta.copy.img2img[ms][texdimidx][fmtidx]
         struct pan_image_view srcview = {
      .format = key.srcfmt,
   .dim = src->pimage.layout.dim == MALI_TEXTURE_DIMENSION_CUBE
               .planes[0] = &src->pimage,
   .nr_samples = src->pimage.layout.nr_samples,
   .first_level = region->srcSubresource.mipLevel,
   .last_level = region->srcSubresource.mipLevel,
   .first_layer = region->srcSubresource.baseArrayLayer,
   .last_layer = region->srcSubresource.baseArrayLayer +
         .swizzle = {PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y, PIPE_SWIZZLE_Z,
               struct pan_image_view dstview = {
      .format = key.dstfmt,
   .dim = MALI_TEXTURE_DIMENSION_2D,
   .planes[0] = &dst->pimage,
   .nr_samples = dst->pimage.layout.nr_samples,
   .first_level = region->dstSubresource.mipLevel,
   .last_level = region->dstSubresource.mipLevel,
   .swizzle = {PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y, PIPE_SWIZZLE_Z,
               unsigned minx = MAX2(region->dstOffset.x, 0);
   unsigned miny = MAX2(region->dstOffset.y, 0);
   unsigned maxx = MAX2(region->dstOffset.x + region->extent.width - 1, 0);
            mali_ptr vpd = panvk_per_arch(meta_emit_viewport)(&cmdbuf->desc_pool.base,
            float dst_rect[] = {
      minx, miny,     0.0, 1.0, maxx + 1, miny,     0.0, 1.0,
               mali_ptr dst_coords = pan_pool_upload_aligned(
                     unsigned width =
         unsigned height =
         cmdbuf->state.fb.crc_valid[0] = false;
   *fbinfo = (struct pan_fb_info){
      .width = width,
   .height = height,
   .extent.minx = minx & ~31,
   .extent.miny = miny & ~31,
   .extent.maxx = MIN2(ALIGN_POT(maxx + 1, 32), width) - 1,
   .extent.maxy = MIN2(ALIGN_POT(maxy + 1, 32), height) - 1,
   .nr_samples = dst->pimage.layout.nr_samples,
   .rt_count = 1,
   .rts[0].view = &dstview,
   .rts[0].preload = true,
               mali_ptr texture =
         mali_ptr sampler =
                     minx = MAX2(region->srcOffset.x, 0);
   miny = MAX2(region->srcOffset.y, 0);
   maxx = MAX2(region->srcOffset.x + region->extent.width - 1, 0);
   maxy = MAX2(region->srcOffset.y + region->extent.height - 1, 0);
            unsigned first_src_layer = MAX2(0, region->srcOffset.z);
   unsigned first_dst_layer =
         unsigned nlayers =
         for (unsigned l = 0; l < nlayers; l++) {
      unsigned src_l = l + first_src_layer;
   float src_rect[] = {
      minx, miny,     src_l, 1.0, maxx + 1, miny,     src_l, 1.0,
               mali_ptr src_coords = pan_pool_upload_aligned(
                     dstview.first_layer = dstview.last_layer = l + first_dst_layer;
   batch->blit.src = src->pimage.data.bo;
   batch->blit.dst = dst->pimage.data.bo;
   panvk_per_arch(cmd_alloc_tls_desc)(cmdbuf, true);
   panvk_per_arch(cmd_alloc_fb_desc)(cmdbuf);
                     tsd = batch->tls.gpu;
                     job = panvk_meta_copy_emit_tiler_job(
                  util_dynarray_append(&batch->jobs, void *, job.cpu);
         }
      static void
   panvk_meta_copy_img2img_init(struct panvk_physical_device *dev, bool is_ms)
   {
      STATIC_ASSERT(ARRAY_SIZE(panvk_meta_copy_img2img_fmts) ==
            for (unsigned i = 0; i < ARRAY_SIZE(panvk_meta_copy_img2img_fmts); i++) {
      for (unsigned texdim = 1; texdim <= 3; texdim++) {
                     /* No MSAA on 1D/3D textures */
                  struct pan_shader_info shader_info;
   mali_ptr shader = panvk_meta_copy_img2img_shader(
      &dev->pdev, &dev->meta.bin_pool.base,
   panvk_meta_copy_img2img_fmts[i].srcfmt,
   panvk_meta_copy_img2img_fmts[i].dstfmt,
   panvk_meta_copy_img2img_fmts[i].dstmask, texdim, false, is_ms,
      dev->meta.copy.img2img[is_ms][texdimidx][i].rsd =
      panvk_meta_copy_to_img_emit_rsd(
      &dev->pdev, &dev->meta.desc_pool.base, shader, &shader_info,
   panvk_meta_copy_img2img_fmts[i].dstfmt,
                  memset(&shader_info, 0, sizeof(shader_info));
   texdimidx = panvk_meta_copy_tex_type(texdim, true);
   assert(texdimidx < ARRAY_SIZE(dev->meta.copy.img2img[0]));
   shader = panvk_meta_copy_img2img_shader(
      &dev->pdev, &dev->meta.bin_pool.base,
   panvk_meta_copy_img2img_fmts[i].srcfmt,
   panvk_meta_copy_img2img_fmts[i].dstfmt,
   panvk_meta_copy_img2img_fmts[i].dstmask, texdim, true, is_ms,
      dev->meta.copy.img2img[is_ms][texdimidx][i].rsd =
      panvk_meta_copy_to_img_emit_rsd(
      &dev->pdev, &dev->meta.desc_pool.base, shader, &shader_info,
            }
      void
   panvk_per_arch(CmdCopyImage2)(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
   VK_FROM_HANDLE(panvk_image, dst, pCopyImageInfo->dstImage);
            for (unsigned i = 0; i < pCopyImageInfo->regionCount; i++) {
            }
      static unsigned
   panvk_meta_copy_buf_texelsize(enum pipe_format imgfmt, unsigned mask)
   {
      unsigned imgtexelsz = util_format_get_blocksize(imgfmt);
            if (nbufcomps == util_format_get_nr_components(imgfmt))
            /* Special case for Z24 buffers which are not tightly packed */
   if (mask == 7 && imgtexelsz == 4)
            /* Special case for S8 extraction from Z32_S8X24 */
   if (mask == 2 && imgtexelsz == 8)
            unsigned compsz =
                        }
      static enum pipe_format
   panvk_meta_copy_buf2img_format(enum pipe_format imgfmt)
   {
      /* Pick blendable formats when we can, and the FLOAT variant matching the
   * texelsize otherwise.
   */
   switch (util_format_get_blocksize(imgfmt)) {
   case 1:
         /* AFBC stores things differently for RGB565,
   * we can't simply map to R8G8 in that case */
   case 2:
      return (imgfmt == PIPE_FORMAT_R5G6B5_UNORM ||
         imgfmt == PIPE_FORMAT_B5G6R5_UNORM)
      case 4:
         case 6:
         case 8:
         case 12:
         case 16:
         default:
            }
      struct panvk_meta_copy_format_info {
      enum pipe_format imgfmt;
      } PACKED;
      static const struct panvk_meta_copy_format_info panvk_meta_copy_buf2img_fmts[] =
      {
      {PIPE_FORMAT_R8_UNORM, 0x1},
   {PIPE_FORMAT_R8G8_UNORM, 0x3},
   {PIPE_FORMAT_R5G6B5_UNORM, 0x7},
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0xf},
   {PIPE_FORMAT_R16G16B16_UINT, 0x7},
   {PIPE_FORMAT_R32G32_UINT, 0x3},
   {PIPE_FORMAT_R32G32B32_UINT, 0x7},
   {PIPE_FORMAT_R32G32B32A32_UINT, 0xf},
   /* S8 -> Z24S8 */
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x8},
   /* S8 -> Z32_S8X24 */
   {PIPE_FORMAT_R32G32_UINT, 0x2},
   /* Z24X8 -> Z24S8 */
   {PIPE_FORMAT_R8G8B8A8_UNORM, 0x7},
   /* Z32 -> Z32_S8X24 */
   };
      struct panvk_meta_copy_buf2img_info {
      struct {
      mali_ptr ptr;
   struct {
      unsigned line;
            } PACKED;
      #define panvk_meta_copy_buf2img_get_info_field(b, field)                       \
      nir_load_push_constant(                                                     \
      (b), 1, sizeof(((struct panvk_meta_copy_buf2img_info *)0)->field) * 8,   \
   nir_imm_int(b, 0),                                                       \
   .base = offsetof(struct panvk_meta_copy_buf2img_info, field),            \
      static mali_ptr
   panvk_meta_copy_buf2img_shader(struct panfrost_device *pdev,
                     {
      nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_FRAGMENT, GENX(pan_shader_get_compiler_options)(),
   "panvk_meta_copy_buf2img(imgfmt=%s,mask=%x)",
         nir_variable *coord_var =
      nir_variable_create(b.shader, nir_var_shader_in,
      coord_var->data.location = VARYING_SLOT_VAR0;
                     nir_def *bufptr = panvk_meta_copy_buf2img_get_info_field(&b, buf.ptr);
   nir_def *buflinestride =
         nir_def *bufsurfstride =
            unsigned imgtexelsz = util_format_get_blocksize(key.imgfmt);
   unsigned buftexelsz = panvk_meta_copy_buf_texelsize(key.imgfmt, key.mask);
            nir_def *offset =
         offset = nir_iadd(&b, offset,
         offset = nir_iadd(&b, offset,
                  unsigned imgcompsz =
      (imgtexelsz <= 4 && key.imgfmt != PIPE_FORMAT_R5G6B5_UNORM)
               unsigned nimgcomps = imgtexelsz / imgcompsz;
   unsigned bufcompsz = MIN2(buftexelsz, imgcompsz);
            assert(bufcompsz == 1 || bufcompsz == 2 || bufcompsz == 4);
            nir_def *texel =
            enum glsl_base_type basetype;
   if (key.imgfmt == PIPE_FORMAT_R5G6B5_UNORM) {
      texel = nir_vec3(
      &b, nir_iand_imm(&b, texel, BITFIELD_MASK(5)),
   nir_iand_imm(&b, nir_ushr_imm(&b, texel, 5), BITFIELD_MASK(6)),
      texel = nir_fmul(
      &b, nir_u2f32(&b, texel),
   nir_vec3(&b, nir_imm_float(&b, 1.0f / 31),
      nimgcomps = 3;
      } else if (imgcompsz == 1) {
      assert(bufcompsz == 1);
   /* Blendable formats are unorm and the fixed-function blend unit
   * takes float values.
   */
   texel = nir_fmul_imm(&b, nir_u2f32(&b, texel), 1.0f / 255);
      } else {
      texel = nir_u2uN(&b, texel, imgcompsz * 8);
               /* We always pass the texel using 32-bit regs for now */
   nir_variable *out =
      nir_variable_create(b.shader, nir_var_shader_out,
                                 if (fullmask != writemask) {
      unsigned first_written_comp = ffs(writemask) - 1;
   nir_def *oldtexel = NULL;
   if (imgcompsz > 1)
            nir_def *texel_comps[4];
   for (unsigned i = 0; i < nimgcomps; i++) {
      if (writemask & BITFIELD_BIT(i))
         else if (imgcompsz > 1)
         else
                                    struct panfrost_compile_inputs inputs = {
      .gpu_id = pdev->gpu_id,
   .is_blit = true,
                        util_dynarray_init(&binary, NULL);
            enum pipe_format rt_formats[8] = {key.imgfmt};
            GENX(pan_shader_compile)(b.shader, &inputs, &binary, shader_info);
   shader_info->push.count =
            mali_ptr shader =
            util_dynarray_fini(&binary);
               }
      static unsigned
   panvk_meta_copy_buf2img_format_idx(struct panvk_meta_copy_format_info key)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(panvk_meta_copy_buf2img_fmts); i++) {
      if (!memcmp(&key, &panvk_meta_copy_buf2img_fmts[i], sizeof(key)))
                  }
      static void
   panvk_meta_copy_buf2img(struct panvk_cmd_buffer *cmdbuf,
                     {
      struct pan_fb_info *fbinfo = &cmdbuf->state.fb.info;
   unsigned minx = MAX2(region->imageOffset.x, 0);
   unsigned miny = MAX2(region->imageOffset.y, 0);
   unsigned maxx =
         unsigned maxy =
            mali_ptr vpd = panvk_per_arch(meta_emit_viewport)(&cmdbuf->desc_pool.base,
            float dst_rect[] = {
      minx, miny,     0.0, 1.0, maxx + 1, miny,     0.0, 1.0,
      };
   mali_ptr dst_coords = pan_pool_upload_aligned(
            struct panvk_meta_copy_format_info key = {
      .imgfmt = panvk_meta_copy_buf2img_format(img->pimage.layout.format),
   .mask = panvk_meta_copy_img_mask(img->pimage.layout.format,
                        mali_ptr rsd =
            const struct vk_image_buffer_layout buflayout =
         struct panvk_meta_copy_buf2img_info info = {
      .buf.ptr = panvk_buffer_gpu_ptr(buf, region->bufferOffset),
   .buf.stride.line = buflayout.row_stride_B,
               mali_ptr pushconsts =
            struct pan_image_view view = {
      .format = key.imgfmt,
   .dim = MALI_TEXTURE_DIMENSION_2D,
   .planes[0] = &img->pimage,
   .nr_samples = img->pimage.layout.nr_samples,
   .first_level = region->imageSubresource.mipLevel,
   .last_level = region->imageSubresource.mipLevel,
   .swizzle = {PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y, PIPE_SWIZZLE_Z,
               /* TODO: don't force preloads of dst resources if unneeded */
   cmdbuf->state.fb.crc_valid[0] = false;
   *fbinfo = (struct pan_fb_info){
      .width =
         .height =
         .extent.minx = minx,
   .extent.maxx = maxx,
   .extent.miny = miny,
   .extent.maxy = maxy,
   .nr_samples = 1,
   .rt_count = 1,
   .rts[0].view = &view,
   .rts[0].preload = true,
                        assert(region->imageSubresource.layerCount == 1 ||
         assert(region->imageOffset.z >= 0);
   unsigned first_layer =
         unsigned nlayers =
         for (unsigned l = 0; l < nlayers; l++) {
      float src_rect[] = {
      0,
   0,
   l,
   1.0,
   region->imageExtent.width,
   0,
   l,
   1.0,
   0,
   region->imageExtent.height,
   l,
   1.0,
   region->imageExtent.width,
   region->imageExtent.height,
   l,
               mali_ptr src_coords = pan_pool_upload_aligned(
                     view.first_layer = view.last_layer = l + first_layer;
   batch->blit.src = buf->bo;
   batch->blit.dst = img->pimage.data.bo;
   panvk_per_arch(cmd_alloc_tls_desc)(cmdbuf, true);
   panvk_per_arch(cmd_alloc_fb_desc)(cmdbuf);
                     tsd = batch->tls.gpu;
                     job = panvk_meta_copy_emit_tiler_job(
                  util_dynarray_append(&batch->jobs, void *, job.cpu);
         }
      static void
   panvk_meta_copy_buf2img_init(struct panvk_physical_device *dev)
   {
      STATIC_ASSERT(ARRAY_SIZE(panvk_meta_copy_buf2img_fmts) ==
            for (unsigned i = 0; i < ARRAY_SIZE(panvk_meta_copy_buf2img_fmts); i++) {
      struct pan_shader_info shader_info;
   mali_ptr shader = panvk_meta_copy_buf2img_shader(
      &dev->pdev, &dev->meta.bin_pool.base, panvk_meta_copy_buf2img_fmts[i],
      dev->meta.copy.buf2img[i].rsd = panvk_meta_copy_to_img_emit_rsd(
      &dev->pdev, &dev->meta.desc_pool.base, shader, &shader_info,
   panvk_meta_copy_buf2img_fmts[i].imgfmt,
      }
      void
   panvk_per_arch(CmdCopyBufferToImage2)(
      VkCommandBuffer commandBuffer,
      {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
   VK_FROM_HANDLE(panvk_buffer, buf, pCopyBufferToImageInfo->srcBuffer);
            for (unsigned i = 0; i < pCopyBufferToImageInfo->regionCount; i++) {
      panvk_meta_copy_buf2img(cmdbuf, buf, img,
         }
      static const struct panvk_meta_copy_format_info panvk_meta_copy_img2buf_fmts[] =
      {
      {PIPE_FORMAT_R8_UINT, 0x1},
   {PIPE_FORMAT_R8G8_UINT, 0x3},
   {PIPE_FORMAT_R5G6B5_UNORM, 0x7},
   {PIPE_FORMAT_R8G8B8A8_UINT, 0xf},
   {PIPE_FORMAT_R16G16B16_UINT, 0x7},
   {PIPE_FORMAT_R32G32_UINT, 0x3},
   {PIPE_FORMAT_R32G32B32_UINT, 0x7},
   {PIPE_FORMAT_R32G32B32A32_UINT, 0xf},
   /* S8 -> Z24S8 */
   {PIPE_FORMAT_R8G8B8A8_UINT, 0x8},
   /* S8 -> Z32_S8X24 */
   {PIPE_FORMAT_R32G32_UINT, 0x2},
   /* Z24X8 -> Z24S8 */
   {PIPE_FORMAT_R8G8B8A8_UINT, 0x7},
   /* Z32 -> Z32_S8X24 */
   };
      static enum pipe_format
   panvk_meta_copy_img2buf_format(enum pipe_format imgfmt)
   {
      /* Pick blendable formats when we can, and the FLOAT variant matching the
   * texelsize otherwise.
   */
   switch (util_format_get_blocksize(imgfmt)) {
   case 1:
         /* AFBC stores things differently for RGB565,
   * we can't simply map to R8G8 in that case */
   case 2:
      return (imgfmt == PIPE_FORMAT_R5G6B5_UNORM ||
         imgfmt == PIPE_FORMAT_B5G6R5_UNORM)
      case 4:
         case 6:
         case 8:
         case 12:
         case 16:
         default:
            }
      struct panvk_meta_copy_img2buf_info {
      struct {
      mali_ptr ptr;
   struct {
      unsigned line;
         } buf;
   struct {
      struct {
         } offset;
   struct {
               } PACKED;
      #define panvk_meta_copy_img2buf_get_info_field(b, field)                       \
      nir_load_push_constant(                                                     \
      (b), 1, sizeof(((struct panvk_meta_copy_img2buf_info *)0)->field) * 8,   \
   nir_imm_int(b, 0),                                                       \
   .base = offsetof(struct panvk_meta_copy_img2buf_info, field),            \
      static mali_ptr
   panvk_meta_copy_img2buf_shader(struct panfrost_device *pdev,
                           {
      unsigned imgtexelsz = util_format_get_blocksize(key.imgfmt);
            /* FIXME: Won't work on compute queues, but we can't do that with
   * a compute shader if the destination is an AFBC surface.
   */
   nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_COMPUTE, GENX(pan_shader_get_compiler_options)(),
   "panvk_meta_copy_img2buf(dim=%dD%s,imgfmt=%s,mask=%x)", texdim,
         nir_def *coord = nir_load_global_invocation_id(&b, 32);
   nir_def *bufptr = panvk_meta_copy_img2buf_get_info_field(&b, buf.ptr);
   nir_def *buflinestride =
         nir_def *bufsurfstride =
            nir_def *imgminx =
         nir_def *imgminy =
         nir_def *imgmaxx =
         nir_def *imgmaxy =
                     switch (texdim + texisarray) {
   case 1:
      imgcoords =
      nir_iadd(&b, nir_channel(&b, coord, 0),
      inbounds =
      nir_iand(&b, nir_uge(&b, imgmaxx, nir_channel(&b, imgcoords, 0)),
         case 2:
      imgcoords = nir_vec2(
      &b,
   nir_iadd(&b, nir_channel(&b, coord, 0),
         nir_iadd(&b, nir_channel(&b, coord, 1),
      inbounds = nir_iand(
      &b,
   nir_iand(&b, nir_uge(&b, imgmaxx, nir_channel(&b, imgcoords, 0)),
         nir_iand(&b, nir_uge(&b, nir_channel(&b, imgcoords, 0), imgminx),
         case 3:
      imgcoords = nir_vec3(
      &b,
   nir_iadd(&b, nir_channel(&b, coord, 0),
         nir_iadd(&b, nir_channel(&b, coord, 1),
         nir_iadd(&b, nir_channel(&b, coord, 2),
      inbounds = nir_iand(
      &b,
   nir_iand(&b, nir_uge(&b, imgmaxx, nir_channel(&b, imgcoords, 0)),
         nir_iand(&b, nir_uge(&b, nir_channel(&b, imgcoords, 0), imgminx),
         default:
                           /* FIXME: doesn't work for tiled+compressed formats since blocks are 4x4
   * blocks instead of 16x16 texels in that case, and there's nothing we can
   * do to force the tile size to 4x4 in the render path.
   * This being said, compressed textures are not compatible with AFBC, so we
   * could use a compute shader arranging the blocks properly.
   */
   nir_def *offset =
         offset = nir_iadd(&b, offset,
         offset = nir_iadd(&b, offset,
                  unsigned imgcompsz =
         unsigned nimgcomps = imgtexelsz / imgcompsz;
            nir_tex_instr *tex = nir_tex_instr_create(b.shader, 1);
   tex->op = nir_texop_txf;
   tex->texture_index = 0;
   tex->is_array = texisarray;
   tex->dest_type =
            switch (texdim) {
   case 1:
      tex->sampler_dim = GLSL_SAMPLER_DIM_1D;
      case 2:
      tex->sampler_dim = GLSL_SAMPLER_DIM_2D;
      case 3:
      tex->sampler_dim = GLSL_SAMPLER_DIM_3D;
      default:
                  tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_coord, imgcoords);
   tex->coord_components = texdim + texisarray;
   nir_def_init(&tex->instr, &tex->def, 4,
                           unsigned fullmask = (1 << util_format_get_nr_components(key.imgfmt)) - 1;
   unsigned nbufcomps = util_bitcount(fullmask);
   if (key.mask != fullmask) {
      nir_def *bufcomps[4];
   nbufcomps = 0;
   for (unsigned i = 0; i < nimgcomps; i++) {
      if (key.mask & BITFIELD_BIT(i))
                                    if (key.imgfmt == PIPE_FORMAT_R5G6B5_UNORM) {
      texel = nir_fmul(&b, texel,
               texel = nir_f2u16(&b, texel);
   texel = nir_ior(
      &b, nir_channel(&b, texel, 0),
   nir_ior(&b,
            imgcompsz = 2;
   bufcompsz = 2;
   nbufcomps = 1;
      } else if (imgcompsz == 1) {
      nir_def *packed = nir_channel(&b, texel, 0);
   for (unsigned i = 1; i < nbufcomps; i++) {
      packed = nir_ior(
      &b, packed,
   nir_ishl(&b, nir_iand_imm(&b, nir_channel(&b, texel, i), 0xff),
   }
            bufcompsz = nbufcomps == 3 ? 4 : nbufcomps;
               assert(bufcompsz == 1 || bufcompsz == 2 || bufcompsz == 4);
   assert(nbufcomps <= 4 && nimgcomps <= 4);
            nir_store_global(&b, bufptr, bufcompsz, texel, (1 << nbufcomps) - 1);
            struct panfrost_compile_inputs inputs = {
      .gpu_id = pdev->gpu_id,
   .is_blit = true,
                        util_dynarray_init(&binary, NULL);
   pan_shader_preprocess(b.shader, inputs.gpu_id);
            shader_info->push.count =
            mali_ptr shader =
            util_dynarray_fini(&binary);
               }
      static unsigned
   panvk_meta_copy_img2buf_format_idx(struct panvk_meta_copy_format_info key)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(panvk_meta_copy_img2buf_fmts); i++) {
      if (!memcmp(&key, &panvk_meta_copy_img2buf_fmts[i], sizeof(key)))
                  }
      static void
   panvk_meta_copy_img2buf(struct panvk_cmd_buffer *cmdbuf,
                     {
      struct panfrost_device *pdev = &cmdbuf->device->physical_device->pdev;
   struct panvk_meta_copy_format_info key = {
      .imgfmt = panvk_meta_copy_img2buf_format(img->pimage.layout.format),
   .mask = panvk_meta_copy_img_mask(img->pimage.layout.format,
      };
   unsigned buftexelsz = panvk_meta_copy_buf_texelsize(key.imgfmt, key.mask);
   unsigned texdimidx = panvk_meta_copy_tex_type(
                  mali_ptr rsd =
            struct panvk_meta_copy_img2buf_info info = {
      .buf.ptr = panvk_buffer_gpu_ptr(buf, region->bufferOffset),
   .buf.stride.line =
         .img.offset.x = MAX2(region->imageOffset.x & ~15, 0),
   .img.extent.minx = MAX2(region->imageOffset.x, 0),
   .img.extent.maxx =
               if (img->pimage.layout.dim == MALI_TEXTURE_DIMENSION_1D) {
         } else {
      info.img.offset.y = MAX2(region->imageOffset.y & ~15, 0);
   info.img.offset.z = MAX2(region->imageOffset.z, 0);
   info.img.extent.miny = MAX2(region->imageOffset.y, 0);
   info.img.extent.maxy =
               info.buf.stride.surf =
      (region->bufferImageHeight ?: region->imageExtent.height) *
         mali_ptr pushconsts =
            struct pan_image_view view = {
      .format = key.imgfmt,
   .dim = img->pimage.layout.dim == MALI_TEXTURE_DIMENSION_CUBE
               .planes[0] = &img->pimage,
   .nr_samples = img->pimage.layout.nr_samples,
   .first_level = region->imageSubresource.mipLevel,
   .last_level = region->imageSubresource.mipLevel,
   .first_layer = region->imageSubresource.baseArrayLayer,
   .last_layer = region->imageSubresource.baseArrayLayer +
         .swizzle = {PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y, PIPE_SWIZZLE_Z,
               mali_ptr texture =
         mali_ptr sampler =
                                       batch->blit.src = img->pimage.data.bo;
   batch->blit.dst = buf->bo;
   batch->tls = pan_pool_alloc_desc(&cmdbuf->desc_pool.base, LOCAL_STORAGE);
                     struct pan_compute_dim wg_sz = {
      16,
   img->pimage.layout.dim == MALI_TEXTURE_DIMENSION_1D ? 1 : 16,
               struct pan_compute_dim num_wg = {
      (ALIGN_POT(info.img.extent.maxx + 1, 16) - info.img.offset.x) / 16,
   img->pimage.layout.dim == MALI_TEXTURE_DIMENSION_1D
      ? region->imageSubresource.layerCount
      img->pimage.layout.dim != MALI_TEXTURE_DIMENSION_1D
      ? MAX2(region->imageSubresource.layerCount, region->imageExtent.depth)
            struct panfrost_ptr job = panvk_meta_copy_emit_compute_job(
      &cmdbuf->desc_pool.base, &batch->scoreboard, &num_wg, &wg_sz, texture,
                     }
      static void
   panvk_meta_copy_img2buf_init(struct panvk_physical_device *dev)
   {
      STATIC_ASSERT(ARRAY_SIZE(panvk_meta_copy_img2buf_fmts) ==
            for (unsigned i = 0; i < ARRAY_SIZE(panvk_meta_copy_img2buf_fmts); i++) {
      for (unsigned texdim = 1; texdim <= 3; texdim++) {
                     struct pan_shader_info shader_info;
   mali_ptr shader = panvk_meta_copy_img2buf_shader(
      &dev->pdev, &dev->meta.bin_pool.base,
      dev->meta.copy.img2buf[texdimidx][i].rsd =
      panvk_meta_copy_to_buf_emit_rsd(&dev->pdev,
                              memset(&shader_info, 0, sizeof(shader_info));
   texdimidx = panvk_meta_copy_tex_type(texdim, true);
   assert(texdimidx < ARRAY_SIZE(dev->meta.copy.img2buf));
   shader = panvk_meta_copy_img2buf_shader(
      &dev->pdev, &dev->meta.bin_pool.base,
      dev->meta.copy.img2buf[texdimidx][i].rsd =
      panvk_meta_copy_to_buf_emit_rsd(&dev->pdev,
               }
      void
   panvk_per_arch(CmdCopyImageToBuffer2)(
      VkCommandBuffer commandBuffer,
      {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
   VK_FROM_HANDLE(panvk_buffer, buf, pCopyImageToBufferInfo->dstBuffer);
            for (unsigned i = 0; i < pCopyImageToBufferInfo->regionCount; i++) {
      panvk_meta_copy_img2buf(cmdbuf, buf, img,
         }
      struct panvk_meta_copy_buf2buf_info {
      mali_ptr src;
      } PACKED;
      #define panvk_meta_copy_buf2buf_get_info_field(b, field)                       \
      nir_load_push_constant(                                                     \
      (b), 1, sizeof(((struct panvk_meta_copy_buf2buf_info *)0)->field) * 8,   \
   nir_imm_int(b, 0),                                                       \
   .base = offsetof(struct panvk_meta_copy_buf2buf_info, field),            \
      static mali_ptr
   panvk_meta_copy_buf2buf_shader(struct panfrost_device *pdev,
               {
      /* FIXME: Won't work on compute queues, but we can't do that with
   * a compute shader if the destination is an AFBC surface.
   */
   nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_COMPUTE, GENX(pan_shader_get_compiler_options)(),
                  nir_def *offset = nir_u2u64(
         nir_def *srcptr =
         nir_def *dstptr =
            unsigned compsz = blksz < 4 ? blksz : 4;
   unsigned ncomps = blksz / compsz;
   nir_store_global(&b, dstptr, blksz,
                  struct panfrost_compile_inputs inputs = {
      .gpu_id = pdev->gpu_id,
   .is_blit = true,
                        util_dynarray_init(&binary, NULL);
   pan_shader_preprocess(b.shader, inputs.gpu_id);
            shader_info->push.count =
            mali_ptr shader =
            util_dynarray_fini(&binary);
               }
      static void
   panvk_meta_copy_buf2buf_init(struct panvk_physical_device *dev)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(dev->meta.copy.buf2buf); i++) {
      struct pan_shader_info shader_info;
   mali_ptr shader = panvk_meta_copy_buf2buf_shader(
         dev->meta.copy.buf2buf[i].rsd = panvk_meta_copy_to_buf_emit_rsd(
         }
      static void
   panvk_meta_copy_buf2buf(struct panvk_cmd_buffer *cmdbuf,
                     {
      struct panvk_meta_copy_buf2buf_info info = {
      .src = panvk_buffer_gpu_ptr(src, region->srcOffset),
               unsigned alignment = ffs((info.src | info.dst | region->size) & 15);
            assert(log2blksz <
         mali_ptr rsd =
            mali_ptr pushconsts =
                                                unsigned nblocks = region->size >> log2blksz;
   struct pan_compute_dim num_wg = {nblocks, 1, 1};
   struct pan_compute_dim wg_sz = {1, 1, 1};
   struct panfrost_ptr job = panvk_meta_copy_emit_compute_job(
      &cmdbuf->desc_pool.base, &batch->scoreboard, &num_wg, &wg_sz, 0, 0,
                  batch->blit.src = src->bo;
   batch->blit.dst = dst->bo;
      }
      void
   panvk_per_arch(CmdCopyBuffer2)(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
   VK_FROM_HANDLE(panvk_buffer, src, pCopyBufferInfo->srcBuffer);
            for (unsigned i = 0; i < pCopyBufferInfo->regionCount; i++) {
            }
      struct panvk_meta_fill_buf_info {
      mali_ptr start;
      } PACKED;
      #define panvk_meta_fill_buf_get_info_field(b, field)                           \
      nir_load_push_constant(                                                     \
      (b), 1, sizeof(((struct panvk_meta_fill_buf_info *)0)->field) * 8,       \
   nir_imm_int(b, 0),                                                       \
      static mali_ptr
   panvk_meta_fill_buf_shader(struct panfrost_device *pdev,
               {
      /* FIXME: Won't work on compute queues, but we can't do that with
   * a compute shader if the destination is an AFBC surface.
   */
   nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_COMPUTE, GENX(pan_shader_get_compiler_options)(),
                  nir_def *offset = nir_u2u64(&b, nir_imul(&b, nir_channel(&b, coord, 0),
         nir_def *ptr =
                           struct panfrost_compile_inputs inputs = {
      .gpu_id = pdev->gpu_id,
   .is_blit = true,
                        util_dynarray_init(&binary, NULL);
   pan_shader_preprocess(b.shader, inputs.gpu_id);
            shader_info->push.count =
            mali_ptr shader =
            util_dynarray_fini(&binary);
               }
      static mali_ptr
   panvk_meta_fill_buf_emit_rsd(struct panfrost_device *pdev,
               {
                        struct panfrost_ptr rsd_ptr =
            pan_pack(rsd_ptr.cpu, RENDERER_STATE, cfg) {
                     }
      static void
   panvk_meta_fill_buf_init(struct panvk_physical_device *dev)
   {
      dev->meta.copy.fillbuf.rsd = panvk_meta_fill_buf_emit_rsd(
      }
      static void
   panvk_meta_fill_buf(struct panvk_cmd_buffer *cmdbuf,
               {
      struct panvk_meta_fill_buf_info info = {
      .start = panvk_buffer_gpu_ptr(dst, offset),
      };
            /* From the Vulkan spec:
   *
   *    "size is the number of bytes to fill, and must be either a multiple
   *    of 4, or VK_WHOLE_SIZE to fill the range from offset to the end of
   *    the buffer. If VK_WHOLE_SIZE is used and the remaining size of the
   *    buffer is not a multiple of 4, then the nearest smaller multiple is
   *    used."
   */
                     unsigned nwords = size / sizeof(uint32_t);
            mali_ptr pushconsts =
                                                struct pan_compute_dim num_wg = {nwords, 1, 1};
   struct pan_compute_dim wg_sz = {1, 1, 1};
   struct panfrost_ptr job = panvk_meta_copy_emit_compute_job(
      &cmdbuf->desc_pool.base, &batch->scoreboard, &num_wg, &wg_sz, 0, 0,
                  batch->blit.dst = dst->bo;
      }
      void
   panvk_per_arch(CmdFillBuffer)(VkCommandBuffer commandBuffer, VkBuffer dstBuffer,
               {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
               }
      static void
   panvk_meta_update_buf(struct panvk_cmd_buffer *cmdbuf,
               {
      struct panvk_meta_copy_buf2buf_info info = {
      .src = pan_pool_upload_aligned(&cmdbuf->desc_pool.base, data, size, 4),
                        mali_ptr rsd =
            mali_ptr pushconsts =
                                                unsigned nblocks = size >> log2blksz;
   struct pan_compute_dim num_wg = {nblocks, 1, 1};
   struct pan_compute_dim wg_sz = {1, 1, 1};
   struct panfrost_ptr job = panvk_meta_copy_emit_compute_job(
      &cmdbuf->desc_pool.base, &batch->scoreboard, &num_wg, &wg_sz, 0, 0,
                  batch->blit.dst = dst->bo;
      }
      void
   panvk_per_arch(CmdUpdateBuffer)(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
               }
      void
   panvk_per_arch(meta_copy_init)(struct panvk_physical_device *dev)
   {
      panvk_meta_copy_img2img_init(dev, false);
   panvk_meta_copy_img2img_init(dev, true);
   panvk_meta_copy_buf2img_init(dev);
   panvk_meta_copy_img2buf_init(dev);
   panvk_meta_copy_buf2buf_init(dev);
      }
