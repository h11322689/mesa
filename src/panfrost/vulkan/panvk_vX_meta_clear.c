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
      #include "nir/nir_builder.h"
   #include "pan_blitter.h"
   #include "pan_encoder.h"
   #include "pan_shader.h"
      #include "panvk_private.h"
   #include "panvk_vX_meta.h"
      #include "vk_format.h"
      static mali_ptr
   panvk_meta_clear_color_attachment_shader(struct panfrost_device *pdev,
                     {
      nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_FRAGMENT, GENX(pan_shader_get_compiler_options)(),
         const struct glsl_type *out_type = glsl_vector_type(base_type, 4);
   nir_variable *out =
                  nir_def *clear_values =
                  struct panfrost_compile_inputs inputs = {
      .gpu_id = pdev->gpu_id,
   .is_blit = true,
                        util_dynarray_init(&binary, NULL);
   pan_shader_preprocess(b.shader, inputs.gpu_id);
                     mali_ptr shader =
            util_dynarray_fini(&binary);
               }
      static mali_ptr
   panvk_meta_clear_color_attachment_emit_rsd(struct panfrost_device *pdev,
                           {
      struct panfrost_ptr rsd_ptr = pan_pool_alloc_desc_aggregate(
            pan_pack(rsd_ptr.cpu, RENDERER_STATE, cfg) {
               cfg.properties.depth_source = MALI_DEPTH_SOURCE_FIXED_FUNCTION;
   cfg.multisample_misc.sample_mask = UINT16_MAX;
   cfg.multisample_misc.depth_function = MALI_FUNC_ALWAYS;
   cfg.properties.allow_forward_pixel_to_be_killed = true;
   cfg.properties.allow_forward_pixel_to_kill = true;
   cfg.properties.zs_update_operation = MALI_PIXEL_KILL_WEAK_EARLY;
                        pan_pack(bd, BLEND, cfg) {
      cfg.round_to_fb_precision = true;
   cfg.load_destination = false;
   cfg.equation.rgb.a = MALI_BLEND_OPERAND_A_SRC;
   cfg.equation.rgb.b = MALI_BLEND_OPERAND_B_SRC;
   cfg.equation.rgb.c = MALI_BLEND_OPERAND_C_ZERO;
   cfg.equation.alpha.a = MALI_BLEND_OPERAND_A_SRC;
   cfg.equation.alpha.b = MALI_BLEND_OPERAND_B_SRC;
   cfg.equation.alpha.c = MALI_BLEND_OPERAND_C_ZERO;
   cfg.internal.mode = MALI_BLEND_MODE_OPAQUE;
   cfg.equation.color_mask = 0xf;
   cfg.internal.fixed_function.num_comps = 4;
   cfg.internal.fixed_function.rt = rt;
   cfg.internal.fixed_function.conversion.memory_format =
         cfg.internal.fixed_function.conversion.register_format =
                  }
      static mali_ptr
   panvk_meta_clear_zs_attachment_emit_rsd(struct panfrost_device *pdev,
                     {
               pan_pack(rsd_ptr.cpu, RENDERER_STATE, cfg) {
      cfg.properties.depth_source = MALI_DEPTH_SOURCE_FIXED_FUNCTION;
            if (mask & VK_IMAGE_ASPECT_DEPTH_BIT) {
                     if (value.depth != 0.0) {
      cfg.stencil_mask_misc.front_facing_depth_bias = true;
   cfg.stencil_mask_misc.back_facing_depth_bias = true;
   cfg.depth_units = INFINITY;
                  if (mask & VK_IMAGE_ASPECT_STENCIL_BIT) {
      cfg.stencil_mask_misc.stencil_enable = true;
                  cfg.stencil_front.compare_function = (mask & VK_IMAGE_ASPECT_DEPTH_BIT)
                  cfg.stencil_front.stencil_fail = MALI_STENCIL_OP_KEEP;
   cfg.stencil_front.depth_fail = MALI_STENCIL_OP_REPLACE;
   cfg.stencil_front.depth_pass = MALI_STENCIL_OP_REPLACE;
   cfg.stencil_front.reference_value = value.stencil;
   cfg.stencil_front.mask = 0xFF;
               cfg.properties.allow_forward_pixel_to_be_killed = true;
   cfg.properties.zs_update_operation = MALI_PIXEL_KILL_WEAK_EARLY;
                  }
      static void
   panvk_meta_clear_attachment_emit_dcd(struct pan_pool *pool, mali_ptr coords,
               {
      pan_pack(out, DRAW, cfg) {
      cfg.thread_storage = tsd;
   cfg.state = rsd;
   cfg.push_uniforms = push_constants;
   cfg.position = coords;
         }
      static struct panfrost_ptr
   panvk_meta_clear_attachment_emit_tiler_job(struct pan_pool *desc_pool,
                                 {
               panvk_meta_clear_attachment_emit_dcd(
      desc_pool, coords, push_constants, vpd, tsd, rsd,
         pan_section_pack(job.cpu, TILER_JOB, PRIMITIVE, cfg) {
      cfg.draw_mode = MALI_DRAW_MODE_TRIANGLE_STRIP;
   cfg.index_count = 4;
               pan_section_pack(job.cpu, TILER_JOB, PRIMITIVE_SIZE, cfg) {
                  void *invoc = pan_section_ptr(job.cpu, TILER_JOB, INVOCATION);
            pan_section_pack(job.cpu, TILER_JOB, PADDING, cfg)
         pan_section_pack(job.cpu, TILER_JOB, TILER, cfg) {
                  panfrost_add_job(desc_pool, scoreboard, MALI_JOB_TYPE_TILER, false, false, 0,
            }
      static enum glsl_base_type
   panvk_meta_get_format_type(enum pipe_format format)
   {
      const struct util_format_description *desc = util_format_description(format);
            i = util_format_get_first_non_void_channel(format);
            if (desc->channel[i].normalized)
                     case UTIL_FORMAT_TYPE_UNSIGNED:
            case UTIL_FORMAT_TYPE_SIGNED:
            case UTIL_FORMAT_TYPE_FLOAT:
            default:
      unreachable("Unhandled format");
         }
      static void
   panvk_meta_clear_attachment(struct panvk_cmd_buffer *cmdbuf,
                           {
      struct panvk_physical_device *dev = cmdbuf->device->physical_device;
   struct panfrost_device *pdev = &dev->pdev;
   struct panvk_meta *meta = &cmdbuf->device->physical_device->meta;
   struct panvk_batch *batch = cmdbuf->state.batch;
   const struct panvk_render_pass *pass = cmdbuf->state.pass;
   const struct panvk_render_pass_attachment *att =
         unsigned minx = MAX2(clear_rect->rect.offset.x, 0);
   unsigned miny = MAX2(clear_rect->rect.offset.y, 0);
   unsigned maxx =
         unsigned maxy =
            panvk_per_arch(cmd_alloc_fb_desc)(cmdbuf);
   panvk_per_arch(cmd_alloc_tls_desc)(cmdbuf, true);
            mali_ptr vpd = panvk_per_arch(meta_emit_viewport)(&cmdbuf->desc_pool.base,
            float rect[] = {
      minx, miny,     0.0, 1.0, maxx + 1, miny,     0.0, 1.0,
      };
   mali_ptr coordinates =
                     mali_ptr tiler = batch->tiler.descs.gpu;
                     if (mask & VK_IMAGE_ASPECT_COLOR_BIT) {
      mali_ptr shader = meta->clear_attachment.color[base_type].shader;
   struct pan_shader_info *shader_info =
            pushconsts = pan_pool_upload_aligned(&cmdbuf->desc_pool.base, clear_value,
            rsd = panvk_meta_clear_color_attachment_emit_rsd(
      } else {
      rsd = panvk_meta_clear_zs_attachment_emit_rsd(
                        job = panvk_meta_clear_attachment_emit_tiler_job(
      &cmdbuf->desc_pool.base, &batch->scoreboard, coordinates, pushconsts, vpd,
            }
      static void
   panvk_meta_clear_color_img(struct panvk_cmd_buffer *cmdbuf,
                     {
      struct pan_fb_info *fbinfo = &cmdbuf->state.fb.info;
   struct pan_image_view view = {
      .format = img->pimage.layout.format,
   .dim = MALI_TEXTURE_DIMENSION_2D,
   .planes[0] = &img->pimage,
   .nr_samples = img->pimage.layout.nr_samples,
   .swizzle = {PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y, PIPE_SWIZZLE_Z,
               cmdbuf->state.fb.crc_valid[0] = false;
   *fbinfo = (struct pan_fb_info){
      .nr_samples = img->pimage.layout.nr_samples,
   .rt_count = 1,
   .rts[0].view = &view,
   .rts[0].clear = true,
               uint32_t clearval[4];
   pan_pack_color(clearval, (union pipe_color_union *)color,
         memcpy(fbinfo->rts[0].clear_value, clearval,
            unsigned level_count = vk_image_subresource_level_count(&img->vk, range);
            for (unsigned level = range->baseMipLevel;
      level < range->baseMipLevel + level_count; level++) {
   view.first_level = view.last_level = level;
   fbinfo->width = u_minify(img->pimage.layout.width, level);
   fbinfo->height = u_minify(img->pimage.layout.height, level);
   fbinfo->extent.maxx = fbinfo->width - 1;
            for (unsigned layer = range->baseArrayLayer;
      layer < range->baseArrayLayer + layer_count; layer++) {
   view.first_layer = view.last_layer = layer;
   panvk_cmd_open_batch(cmdbuf);
   panvk_per_arch(cmd_alloc_fb_desc)(cmdbuf);
            }
      void
   panvk_per_arch(CmdClearColorImage)(VkCommandBuffer commandBuffer, VkImage image,
                           {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
                     for (unsigned i = 0; i < rangeCount; i++)
      }
      static void
   panvk_meta_clear_zs_img(struct panvk_cmd_buffer *cmdbuf,
                     {
      struct pan_fb_info *fbinfo = &cmdbuf->state.fb.info;
   struct pan_image_view view = {
      .format = img->pimage.layout.format,
   .dim = MALI_TEXTURE_DIMENSION_2D,
   .planes[0] = &img->pimage,
   .nr_samples = img->pimage.layout.nr_samples,
   .swizzle = {PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y, PIPE_SWIZZLE_Z,
               cmdbuf->state.fb.crc_valid[0] = false;
   *fbinfo = (struct pan_fb_info){
      .nr_samples = img->pimage.layout.nr_samples,
   .rt_count = 1,
   .zs.clear_value.depth = value->depth,
   .zs.clear_value.stencil = value->stencil,
   .zs.clear.z = range->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT,
         const struct util_format_description *fdesc =
            if (util_format_has_depth(fdesc)) {
      fbinfo->zs.view.zs = &view;
   if (util_format_has_stencil(fdesc)) {
      fbinfo->zs.preload.z = !fbinfo->zs.clear.z;
         } else {
                  unsigned level_count = vk_image_subresource_level_count(&img->vk, range);
            for (unsigned level = range->baseMipLevel;
      level < range->baseMipLevel + level_count; level++) {
   view.first_level = view.last_level = level;
   fbinfo->width = u_minify(img->pimage.layout.width, level);
   fbinfo->height = u_minify(img->pimage.layout.height, level);
   fbinfo->extent.maxx = fbinfo->width - 1;
            for (unsigned layer = range->baseArrayLayer;
      layer < range->baseArrayLayer + layer_count; layer++) {
   view.first_layer = view.last_layer = layer;
   panvk_cmd_open_batch(cmdbuf);
   panvk_per_arch(cmd_alloc_fb_desc)(cmdbuf);
                     }
      void
   panvk_per_arch(CmdClearDepthStencilImage)(
      VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
   const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount,
      {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
                     for (unsigned i = 0; i < rangeCount; i++)
      }
      void
   panvk_per_arch(CmdClearAttachments)(VkCommandBuffer commandBuffer,
                           {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
            for (unsigned i = 0; i < attachmentCount; i++) {
                  uint32_t attachment, rt = 0;
   if (pAttachments[i].aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
      rt = pAttachments[i].colorAttachment;
      } else {
                                 panvk_meta_clear_attachment(cmdbuf, attachment, rt,
                  }
      static void
   panvk_meta_clear_attachment_init(struct panvk_physical_device *dev)
   {
      dev->meta.clear_attachment.color[GLSL_TYPE_UINT].shader =
      panvk_meta_clear_color_attachment_shader(
               dev->meta.clear_attachment.color[GLSL_TYPE_INT].shader =
      panvk_meta_clear_color_attachment_shader(
               dev->meta.clear_attachment.color[GLSL_TYPE_FLOAT].shader =
      panvk_meta_clear_color_attachment_shader(
         }
      void
   panvk_per_arch(meta_clear_init)(struct panvk_physical_device *dev)
   {
         }
