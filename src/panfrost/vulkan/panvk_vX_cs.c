   /*
   * Copyright (C) 2021 Collabora Ltd.
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
      #include "genxml/gen_macros.h"
      #include "compiler/shader_enums.h"
   #include "util/macros.h"
      #include "vk_util.h"
      #include "pan_cs.h"
   #include "pan_earlyzs.h"
   #include "pan_encoder.h"
   #include "pan_pool.h"
   #include "pan_shader.h"
      #include "panvk_cs.h"
   #include "panvk_private.h"
   #include "panvk_varyings.h"
      #include "vk_sampler.h"
      static enum mali_mipmap_mode
   panvk_translate_sampler_mipmap_mode(VkSamplerMipmapMode mode)
   {
      switch (mode) {
   case VK_SAMPLER_MIPMAP_MODE_NEAREST:
         case VK_SAMPLER_MIPMAP_MODE_LINEAR:
         default:
            }
      static unsigned
   panvk_translate_sampler_address_mode(VkSamplerAddressMode mode)
   {
      switch (mode) {
   case VK_SAMPLER_ADDRESS_MODE_REPEAT:
         case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
         case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
         case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
         case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
         default:
            }
      static mali_pixel_format
   panvk_varying_hw_format(const struct panvk_device *dev,
               {
      const struct panfrost_device *pdev = &dev->physical_device->pdev;
            switch (loc) {
   case VARYING_SLOT_PNTC:
      #if PAN_ARCH <= 6
         #else
         #endif
         #if PAN_ARCH <= 6
         #else
         #endif
      default:
      if (varyings->varying[loc].format != PIPE_FORMAT_NONE)
   #if PAN_ARCH >= 7
         #else
         #endif
         }
      static void
   panvk_emit_varying(const struct panvk_device *dev,
               {
               pan_pack(attrib, ATTRIBUTE, cfg) {
      cfg.buffer_index = varyings->varying[loc].buf;
   cfg.offset = varyings->varying[loc].offset;
         }
      void
   panvk_per_arch(emit_varyings)(const struct panvk_device *dev,
               {
               for (unsigned i = 0; i < varyings->stage[stage].count; i++)
      }
      static void
   panvk_emit_varying_buf(const struct panvk_varyings_info *varyings,
         {
               pan_pack(buf, ATTRIBUTE_BUFFER, cfg) {
               cfg.stride = varyings->buf[buf_idx].stride;
   cfg.size = varyings->buf[buf_idx].size + offset;
         }
      void
   panvk_per_arch(emit_varying_bufs)(const struct panvk_varyings_info *varyings,
         {
               for (unsigned i = 0; i < PANVK_VARY_BUF_MAX; i++) {
      if (varyings->buf_mask & (1 << i))
         }
      static void
   panvk_emit_attrib_buf(const struct panvk_attribs_info *info,
                     {
               assert(idx < buf_count);
   const struct panvk_attrib_buf *buf = &bufs[idx];
   mali_ptr addr = buf->address & ~63ULL;
   unsigned size = buf->size + (buf->address & 63);
            /* TODO: support instanced arrays */
   if (draw->instance_count <= 1) {
      pan_pack(desc, ATTRIBUTE_BUFFER, cfg) {
      cfg.type = MALI_ATTRIBUTE_TYPE_1D;
   cfg.stride = buf_info->per_instance ? 0 : buf_info->stride;
   cfg.pointer = addr;
         } else if (!buf_info->per_instance) {
      pan_pack(desc, ATTRIBUTE_BUFFER, cfg) {
      cfg.type = MALI_ATTRIBUTE_TYPE_1D_MODULUS;
   cfg.divisor = draw->padded_vertex_count;
   cfg.stride = buf_info->stride;
   cfg.pointer = addr;
         } else if (!divisor) {
      /* instance_divisor == 0 means all instances share the same value.
   * Make it a 1D array with a zero stride.
   */
   pan_pack(desc, ATTRIBUTE_BUFFER, cfg) {
      cfg.type = MALI_ATTRIBUTE_TYPE_1D;
   cfg.stride = 0;
   cfg.pointer = addr;
         } else if (util_is_power_of_two_or_zero(divisor)) {
      pan_pack(desc, ATTRIBUTE_BUFFER, cfg) {
      cfg.type = MALI_ATTRIBUTE_TYPE_1D_POT_DIVISOR;
   cfg.stride = buf_info->stride;
   cfg.pointer = addr;
   cfg.size = size;
         } else {
      unsigned divisor_r = 0, divisor_e = 0;
   unsigned divisor_num =
         pan_pack(desc, ATTRIBUTE_BUFFER, cfg) {
      cfg.type = MALI_ATTRIBUTE_TYPE_1D_NPOT_DIVISOR;
   cfg.stride = buf_info->stride;
   cfg.pointer = addr;
   cfg.size = size;
   cfg.divisor_r = divisor_r;
               desc += pan_size(ATTRIBUTE_BUFFER);
   pan_pack(desc, ATTRIBUTE_BUFFER_CONTINUATION_NPOT, cfg) {
      cfg.divisor_numerator = divisor_num;
            }
      void
   panvk_per_arch(emit_attrib_bufs)(const struct panvk_attribs_info *info,
                           {
               for (unsigned i = 0; i < info->buf_count; i++) {
      panvk_emit_attrib_buf(info, draw, bufs, buf_count, i, buf);
         }
      void
   panvk_per_arch(emit_sampler)(const VkSamplerCreateInfo *pCreateInfo, void *desc)
   {
      VkClearColorValue border_color =
            pan_pack(desc, SAMPLER, cfg) {
      cfg.magnify_nearest = pCreateInfo->magFilter == VK_FILTER_NEAREST;
   cfg.minify_nearest = pCreateInfo->minFilter == VK_FILTER_NEAREST;
   cfg.mipmap_mode =
                  cfg.lod_bias = pCreateInfo->mipLodBias;
   cfg.minimum_lod = pCreateInfo->minLod;
   cfg.maximum_lod = pCreateInfo->maxLod;
   cfg.wrap_mode_s =
         cfg.wrap_mode_t =
         cfg.wrap_mode_r =
         cfg.compare_function =
         cfg.border_color_r = border_color.uint32[0];
   cfg.border_color_g = border_color.uint32[1];
   cfg.border_color_b = border_color.uint32[2];
         }
      static void
   panvk_emit_attrib(const struct panvk_device *dev,
                     const struct panvk_draw_info *draw,
   {
      const struct panfrost_device *pdev = &dev->physical_device->pdev;
   unsigned buf_idx = attribs->attrib[idx].buf;
            pan_pack(attrib, ATTRIBUTE, cfg) {
      cfg.buffer_index = buf_idx * 2;
            if (buf_info->per_instance)
                  }
      void
   panvk_per_arch(emit_attribs)(const struct panvk_device *dev,
                           {
               for (unsigned i = 0; i < attribs->attrib_count; i++)
      }
      void
   panvk_per_arch(emit_ubo)(mali_ptr address, size_t size, void *desc)
   {
      pan_pack(desc, UNIFORM_BUFFER, cfg) {
      cfg.pointer = address;
         }
      void
   panvk_per_arch(emit_ubos)(const struct panvk_pipeline *pipeline,
               {
               panvk_per_arch(emit_ubo)(state->sysvals_ptr, sizeof(state->sysvals),
            if (pipeline->layout->push_constants.size) {
      panvk_per_arch(emit_ubo)(
      state->push_constants,
   ALIGN_POT(pipeline->layout->push_constants.size, 16),
   } else {
                  for (unsigned s = 0; s < pipeline->layout->vk.set_count; s++) {
      const struct panvk_descriptor_set_layout *set_layout =
                  unsigned ubo_start =
            if (!set) {
      unsigned all_ubos = set_layout->num_ubos + set_layout->num_dyn_ubos;
      } else {
                                    for (unsigned i = 0; i < set_layout->num_dyn_ubos; i++) {
                     mali_ptr address =
         size_t size =
         if (size) {
      panvk_per_arch(emit_ubo)(address, size,
      } else {
                     }
      void
   panvk_per_arch(emit_vertex_job)(const struct panvk_pipeline *pipeline,
         {
                        pan_section_pack(job, COMPUTE_JOB, PARAMETERS, cfg) {
                  pan_section_pack(job, COMPUTE_JOB, DRAW, cfg) {
      cfg.state = pipeline->rsds[MESA_SHADER_VERTEX];
   cfg.attributes = draw->stages[MESA_SHADER_VERTEX].attributes;
   cfg.attribute_buffers = draw->stages[MESA_SHADER_VERTEX].attribute_bufs;
   cfg.varyings = draw->stages[MESA_SHADER_VERTEX].varyings;
   cfg.varying_buffers = draw->varying_bufs;
   cfg.thread_storage = draw->tls;
   cfg.offset_start = draw->offset_start;
   cfg.instance_size =
         cfg.uniform_buffers = draw->ubos;
   cfg.push_uniforms = draw->stages[PIPE_SHADER_VERTEX].push_constants;
   cfg.textures = draw->textures;
         }
      void
   panvk_per_arch(emit_compute_job)(const struct panvk_pipeline *pipeline,
               {
      panfrost_pack_work_groups_compute(
      pan_section_ptr(job, COMPUTE_JOB, INVOCATION), dispatch->wg_count.x,
   dispatch->wg_count.y, dispatch->wg_count.z, pipeline->cs.local_size.x,
         pan_section_pack(job, COMPUTE_JOB, PARAMETERS, cfg) {
      cfg.job_task_split = util_logbase2_ceil(pipeline->cs.local_size.x + 1) +
                     pan_section_pack(job, COMPUTE_JOB, DRAW, cfg) {
      cfg.state = pipeline->rsds[MESA_SHADER_COMPUTE];
   cfg.attributes = dispatch->attributes;
   cfg.attribute_buffers = dispatch->attribute_bufs;
   cfg.thread_storage = dispatch->tsd;
   cfg.uniform_buffers = dispatch->ubos;
   cfg.push_uniforms = dispatch->push_uniforms;
   cfg.textures = dispatch->textures;
         }
      static void
   panvk_emit_tiler_primitive(const struct panvk_pipeline *pipeline,
         {
      pan_pack(prim, PRIMITIVE, cfg) {
      cfg.draw_mode = pipeline->ia.topology;
   if (pipeline->ia.writes_point_size)
            cfg.first_provoking_vertex = true;
   if (pipeline->ia.primitive_restart)
                  if (draw->index_size) {
      cfg.index_count = draw->index_count;
                  switch (draw->index_size) {
   case 32:
      cfg.index_type = MALI_INDEX_TYPE_UINT32;
      case 16:
      cfg.index_type = MALI_INDEX_TYPE_UINT16;
      case 8:
      cfg.index_type = MALI_INDEX_TYPE_UINT8;
      default:
            } else {
      cfg.index_count = draw->vertex_count;
            }
      static void
   panvk_emit_tiler_primitive_size(const struct panvk_pipeline *pipeline,
               {
      pan_pack(primsz, PRIMITIVE_SIZE, cfg) {
      if (pipeline->ia.writes_point_size) {
         } else {
               }
      static void
   panvk_emit_tiler_dcd(const struct panvk_pipeline *pipeline,
         {
      pan_pack(dcd, DRAW, cfg) {
      cfg.front_face_ccw = pipeline->rast.front_ccw;
   cfg.cull_front_face = pipeline->rast.cull_front_face;
   cfg.cull_back_face = pipeline->rast.cull_back_face;
   cfg.position = draw->position;
   cfg.state = draw->fs_rsd;
   cfg.attributes = draw->stages[MESA_SHADER_FRAGMENT].attributes;
   cfg.attribute_buffers = draw->stages[MESA_SHADER_FRAGMENT].attribute_bufs;
   cfg.viewport = draw->viewport;
   cfg.varyings = draw->stages[MESA_SHADER_FRAGMENT].varyings;
   cfg.varying_buffers = cfg.varyings ? draw->varying_bufs : 0;
            /* For all primitives but lines DRAW.flat_shading_vertex must
   * be set to 0 and the provoking vertex is selected with the
   * PRIMITIVE.first_provoking_vertex field.
   */
   if (pipeline->ia.topology == MALI_DRAW_MODE_LINES ||
      pipeline->ia.topology == MALI_DRAW_MODE_LINE_STRIP ||
   pipeline->ia.topology == MALI_DRAW_MODE_LINE_LOOP) {
               cfg.offset_start = draw->offset_start;
   cfg.instance_size =
         cfg.uniform_buffers = draw->ubos;
   cfg.push_uniforms = draw->stages[PIPE_SHADER_FRAGMENT].push_constants;
   cfg.textures = draw->textures;
                  }
      void
   panvk_per_arch(emit_tiler_job)(const struct panvk_pipeline *pipeline,
         {
               section = pan_section_ptr(job, TILER_JOB, INVOCATION);
            section = pan_section_ptr(job, TILER_JOB, PRIMITIVE);
            section = pan_section_ptr(job, TILER_JOB, PRIMITIVE_SIZE);
            section = pan_section_ptr(job, TILER_JOB, DRAW);
            pan_section_pack(job, TILER_JOB, TILER, cfg) {
         }
   pan_section_pack(job, TILER_JOB, PADDING, padding)
      }
      void
   panvk_per_arch(emit_viewport)(const VkViewport *viewport,
         {
      /* The spec says "width must be greater than 0.0" */
   assert(viewport->x >= 0);
   int minx = (int)viewport->x;
            /* Viewport height can be negative */
   int miny = MIN2((int)viewport->y, (int)(viewport->y + viewport->height));
            assert(scissor->offset.x >= 0 && scissor->offset.y >= 0);
   miny = MAX2(scissor->offset.x, minx);
   miny = MAX2(scissor->offset.y, miny);
   maxx = MIN2(scissor->offset.x + scissor->extent.width, maxx);
            /* Make sure we don't end up with a max < min when width/height is 0 */
   maxx = maxx > minx ? maxx - 1 : maxx;
            assert(viewport->minDepth >= 0.0f && viewport->minDepth <= 1.0f);
            pan_pack(vpd, VIEWPORT, cfg) {
      cfg.scissor_minimum_x = minx;
   cfg.scissor_minimum_y = miny;
   cfg.scissor_maximum_x = maxx;
   cfg.scissor_maximum_y = maxy;
   cfg.minimum_z = MIN2(viewport->minDepth, viewport->maxDepth);
         }
      static enum mali_register_file_format
   bifrost_blend_type_from_nir(nir_alu_type nir_type)
   {
      switch (nir_type) {
   case 0: /* Render target not in use */
         case nir_type_float16:
         case nir_type_float32:
         case nir_type_int32:
         case nir_type_uint32:
         case nir_type_int16:
         case nir_type_uint16:
         default:
            }
      void
   panvk_per_arch(emit_blend)(const struct panvk_device *dev,
               {
      const struct pan_blend_state *blend = &pipeline->blend.state;
   const struct pan_blend_rt_state *rts = &blend->rts[rt];
            pan_pack(bd, BLEND, cfg) {
      if (!blend->rt_count || !rts->equation.color_mask) {
      cfg.enable = false;
   cfg.internal.mode = MALI_BLEND_MODE_OFF;
               cfg.srgb = util_format_is_srgb(rts->format);
   cfg.load_destination = pan_blend_reads_dest(blend->rts[rt].equation);
            const struct panfrost_device *pdev = &dev->physical_device->pdev;
   const struct util_format_description *format_desc =
         unsigned chan_size = 0;
   for (unsigned i = 0; i < format_desc->nr_channels; i++)
            pan_blend_to_fixed_function_equation(blend->rts[rt].equation,
            /* Fixed point constant */
   float fconst = pan_blend_get_constant(
         u16 constant = fconst * ((1 << chan_size) - 1);
   constant <<= 16 - chan_size;
            if (pan_blend_is_opaque(blend->rts[rt].equation)) {
         } else {
               cfg.internal.fixed_function.alpha_zero_nop =
         cfg.internal.fixed_function.alpha_one_store =
               /* If we want the conversion to work properly,
   * num_comps must be set to 4
   */
   cfg.internal.fixed_function.num_comps = 4;
   cfg.internal.fixed_function.conversion.memory_format =
         cfg.internal.fixed_function.conversion.register_format =
               }
      void
   panvk_per_arch(emit_blend_constant)(const struct panvk_device *dev,
                     {
               pan_pack(bd, BLEND, cfg) {
      cfg.enable = false;
         }
      void
   panvk_per_arch(emit_dyn_fs_rsd)(const struct panvk_pipeline *pipeline,
         {
      pan_pack(rsd, RENDERER_STATE, cfg) {
      if (pipeline->dynamic_state_mask & (1 << VK_DYNAMIC_STATE_DEPTH_BIAS)) {
      cfg.depth_units = state->rast.depth_bias.constant_factor * 2.0f;
   cfg.depth_factor = state->rast.depth_bias.slope_factor;
               if (pipeline->dynamic_state_mask &
      (1 << VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK)) {
   cfg.stencil_front.mask = state->zs.s_front.compare_mask;
               if (pipeline->dynamic_state_mask &
      (1 << VK_DYNAMIC_STATE_STENCIL_WRITE_MASK)) {
   cfg.stencil_mask_misc.stencil_mask_front =
                     if (pipeline->dynamic_state_mask &
      (1 << VK_DYNAMIC_STATE_STENCIL_REFERENCE)) {
   cfg.stencil_front.reference_value = state->zs.s_front.ref;
            }
      void
   panvk_per_arch(emit_base_fs_rsd)(const struct panvk_device *dev,
               {
               pan_pack(rsd, RENDERER_STATE, cfg) {
      if (pipeline->fs.required) {
               uint8_t rt_written =
         uint8_t rt_mask = pipeline->fs.rt_mask;
   cfg.properties.allow_forward_pixel_to_kill =
                  bool writes_zs = pipeline->zs.z_write || pipeline->zs.s_test;
                  struct pan_earlyzs_state earlyzs =
                  cfg.properties.pixel_kill_operation = earlyzs.kill;
      } else {
      cfg.properties.depth_source = MALI_DEPTH_SOURCE_FIXED_FUNCTION;
   cfg.properties.allow_forward_pixel_to_kill = true;
   cfg.properties.allow_forward_pixel_to_be_killed = true;
               bool msaa = pipeline->ms.rast_samples > 1;
   cfg.multisample_misc.multisample_enable = msaa;
   cfg.multisample_misc.sample_mask =
            cfg.multisample_misc.depth_function =
            cfg.multisample_misc.depth_write_mask = pipeline->zs.z_write;
   cfg.multisample_misc.fixed_function_near_discard =
         cfg.multisample_misc.fixed_function_far_discard =
                  cfg.stencil_mask_misc.stencil_enable = pipeline->zs.s_test;
   cfg.stencil_mask_misc.alpha_to_coverage = pipeline->ms.alpha_to_coverage;
   cfg.stencil_mask_misc.alpha_test_compare_function = MALI_FUNC_ALWAYS;
   cfg.stencil_mask_misc.front_facing_depth_bias =
         cfg.stencil_mask_misc.back_facing_depth_bias =
         cfg.stencil_mask_misc.single_sampled_lines =
            if (!(pipeline->dynamic_state_mask &
            cfg.depth_units = pipeline->rast.depth_bias.constant_factor * 2.0f;
   cfg.depth_factor = pipeline->rast.depth_bias.slope_factor;
               if (!(pipeline->dynamic_state_mask &
            cfg.stencil_front.mask = pipeline->zs.s_front.compare_mask;
               if (!(pipeline->dynamic_state_mask &
            cfg.stencil_mask_misc.stencil_mask_front =
         cfg.stencil_mask_misc.stencil_mask_back =
               if (!(pipeline->dynamic_state_mask &
            cfg.stencil_front.reference_value = pipeline->zs.s_front.ref;
               cfg.stencil_front.compare_function = pipeline->zs.s_front.compare_func;
   cfg.stencil_front.stencil_fail = pipeline->zs.s_front.fail_op;
   cfg.stencil_front.depth_fail = pipeline->zs.s_front.z_fail_op;
   cfg.stencil_front.depth_pass = pipeline->zs.s_front.pass_op;
   cfg.stencil_back.compare_function = pipeline->zs.s_back.compare_func;
   cfg.stencil_back.stencil_fail = pipeline->zs.s_back.fail_op;
   cfg.stencil_back.depth_fail = pipeline->zs.s_back.z_fail_op;
         }
      void
   panvk_per_arch(emit_non_fs_rsd)(const struct panvk_device *dev,
               {
               pan_pack(rsd, RENDERER_STATE, cfg) {
            }
      void
   panvk_per_arch(emit_tiler_context)(const struct panvk_device *dev,
               {
               pan_pack(descs->cpu + pan_size(TILER_CONTEXT), TILER_HEAP, cfg) {
      cfg.size = pdev->tiler_heap->size;
   cfg.base = pdev->tiler_heap->ptr.gpu;
   cfg.bottom = pdev->tiler_heap->ptr.gpu;
               pan_pack(descs->cpu, TILER_CONTEXT, cfg) {
      cfg.hierarchy_mask = 0x28;
   cfg.fb_width = width;
   cfg.fb_height = height;
         }
