   /*
   * Copyright © 2021 Raspberry Pi Ltd
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "v3dv_private.h"
   #include "broadcom/common/v3d_macros.h"
   #include "broadcom/cle/v3dx_pack.h"
   #include "broadcom/compiler/v3d_compiler.h"
      static uint8_t
   blend_factor(VkBlendFactor factor, bool dst_alpha_one, bool *needs_constants)
   {
      switch (factor) {
   case VK_BLEND_FACTOR_ZERO:
   case VK_BLEND_FACTOR_ONE:
   case VK_BLEND_FACTOR_SRC_COLOR:
   case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
   case VK_BLEND_FACTOR_DST_COLOR:
   case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
   case VK_BLEND_FACTOR_SRC_ALPHA:
   case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
   case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:
         case VK_BLEND_FACTOR_CONSTANT_COLOR:
   case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
   case VK_BLEND_FACTOR_CONSTANT_ALPHA:
   case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
      *needs_constants = true;
      case VK_BLEND_FACTOR_DST_ALPHA:
      return dst_alpha_one ? V3D_BLEND_FACTOR_ONE :
      case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
      return dst_alpha_one ? V3D_BLEND_FACTOR_ZERO :
      case VK_BLEND_FACTOR_SRC1_COLOR:
   case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
   case VK_BLEND_FACTOR_SRC1_ALPHA:
   case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:
         default:
            }
      static void
   pack_blend(struct v3dv_pipeline *pipeline,
         {
      /* By default, we are not enabling blending and all color channel writes are
   * enabled. Color write enables are independent of whether blending is
   * enabled or not.
   *
   * Vulkan specifies color write masks so that bits set correspond to
   * enabled channels. Our hardware does it the other way around.
   */
   pipeline->blend.enables = 0;
            if (!cb_info)
            assert(pipeline->subpass);
   if (pipeline->subpass->color_count == 0)
            assert(pipeline->subpass->color_count == cb_info->attachmentCount);
   pipeline->blend.needs_color_constants = false;
   uint32_t color_write_masks = 0;
   for (uint32_t i = 0; i < pipeline->subpass->color_count; i++) {
      const VkPipelineColorBlendAttachmentState *b_state =
            uint32_t attachment_idx =
         if (attachment_idx == VK_ATTACHMENT_UNUSED)
                     if (!b_state->blendEnable)
            VkAttachmentDescription2 *desc =
                  /* We only do blending with render pass attachments, so we should not have
   * multiplanar images here
   */
   assert(format->plane_count == 1);
            uint8_t rt_mask = 1 << i;
            v3dvx_pack(pipeline->blend.cfg[i], BLEND_CFG, config) {
               config.color_blend_mode = b_state->colorBlendOp;
   config.color_blend_dst_factor =
      blend_factor(b_state->dstColorBlendFactor, dst_alpha_one,
      config.color_blend_src_factor =
                  config.alpha_blend_mode = b_state->alphaBlendOp;
   config.alpha_blend_dst_factor =
      blend_factor(b_state->dstAlphaBlendFactor, dst_alpha_one,
      config.alpha_blend_src_factor =
      blend_factor(b_state->srcAlphaBlendFactor, dst_alpha_one,
                  }
      /* This requires that pack_blend() had been called before so we can set
   * the overall blend enable bit in the CFG_BITS packet.
   */
   static void
   pack_cfg_bits(struct v3dv_pipeline *pipeline,
               const VkPipelineDepthStencilStateCreateInfo *ds_info,
   const VkPipelineRasterizationStateCreateInfo *rs_info,
   const VkPipelineRasterizationProvokingVertexStateCreateInfoEXT *pv_info,
   {
               pipeline->msaa =
            v3dvx_pack(pipeline->cfg_bits, CFG_BITS, config) {
      config.enable_forward_facing_primitive =
            config.enable_reverse_facing_primitive =
            /* Seems like the hardware is backwards regarding this setting... */
   config.clockwise_primitives =
            /* Even if rs_info->depthBiasEnabled is true, we can decide to not
   * enable it, like if there isn't a depth/stencil attachment with the
   * pipeline.
   */
            /* This is required to pass line rasterization tests in CTS while
   * exposing, at least, a minimum of 4-bits of subpixel precision
   * (the minimum requirement).
   */
   if (ls_info &&
      ls_info->lineRasterizationMode == VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT)
      else
            if (rs_info && rs_info->polygonMode != VK_POLYGON_MODE_FILL) {
      config.direct3d_wireframe_triangles_mode = true;
   config.direct3d_point_fill_mode =
               /* diamond-exit rasterization does not support oversample */
   config.rasterizer_oversample_mode =
                  /* From the Vulkan spec:
   *
   *   "Provoking Vertex:
   *
   *       The vertex in a primitive from which flat shaded attribute
   *       values are taken. This is generally the “first” vertex in the
   *       primitive, and depends on the primitive topology."
   *
   * First vertex is the Direct3D style for provoking vertex. OpenGL uses
   * the last vertex by default.
   */
   if (pv_info) {
      config.direct3d_provoking_vertex =
      pv_info->provokingVertexMode ==
   } else {
                           /* Disable depth/stencil if we don't have a D/S attachment */
   bool has_ds_attachment =
            if (ds_info && ds_info->depthTestEnable && has_ds_attachment) {
      config.z_updates_enable = ds_info->depthWriteEnable;
      } else {
                  config.stencil_enable =
               #if V3D_VERSION >= 71
         /* From the Vulkan spec:
   *
   *    "depthClampEnable controls whether to clamp the fragment’s depth
   *     values as described in Depth Test. If the pipeline is not created
   *     with VkPipelineRasterizationDepthClipStateCreateInfoEXT present
   *     then enabling depth clamp will also disable clipping primitives to
   *     the z planes of the frustrum as described in Primitive Clipping.
   *     Otherwise depth clipping is controlled by the state set in
   *     VkPipelineRasterizationDepthClipStateCreateInfoEXT."
   *
   * Note: neither depth clamping nor VK_EXT_depth_clip_enable are actually
   * supported in the driver yet, so in practice we are always enabling Z
   * clipping for now.
   */
   bool z_clamp_enable = rs_info && rs_info->depthClampEnable;
   bool z_clip_enable = false;
   const VkPipelineRasterizationDepthClipStateCreateInfoEXT *clip_info =
      ds_info ? vk_find_struct_const(ds_info->pNext,
            if (clip_info)
         else if (!z_clamp_enable)
            if (z_clip_enable) {
      V3D_Z_CLIP_MODE_MIN_ONE_TO_ONE : V3D_Z_CLIP_MODE_ZERO_TO_ONE;
      } else {
                           config.depth_bounds_test_enable =
   #endif
         }
      static uint32_t
   translate_stencil_op(enum pipe_stencil_op op)
   {
      switch (op) {
   case VK_STENCIL_OP_KEEP:
         case VK_STENCIL_OP_ZERO:
         case VK_STENCIL_OP_REPLACE:
         case VK_STENCIL_OP_INCREMENT_AND_CLAMP:
         case VK_STENCIL_OP_DECREMENT_AND_CLAMP:
         case VK_STENCIL_OP_INVERT:
         case VK_STENCIL_OP_INCREMENT_AND_WRAP:
         case VK_STENCIL_OP_DECREMENT_AND_WRAP:
         default:
            }
      static void
   pack_single_stencil_cfg(struct v3dv_pipeline *pipeline,
                           {
      /* From the Vulkan spec:
   *
   *   "Reference is an integer reference value that is used in the unsigned
   *    stencil comparison. The reference value used by stencil comparison
   *    must be within the range [0,2^s-1] , where s is the number of bits in
   *    the stencil framebuffer attachment, otherwise the reference value is
   *    considered undefined."
   *
   * In our case, 's' is always 8, so we clamp to that to prevent our packing
   * functions to assert in debug mode if they see larger values.
   *
   * If we have dynamic state we need to make sure we set the corresponding
   * state bits to 0, since cl_emit_with_prepacked ORs the new value with
   * the old.
   */
   const uint8_t write_mask =
      pipeline->dynamic_state.mask & V3DV_DYNAMIC_STENCIL_WRITE_MASK ?
         const uint8_t compare_mask =
      pipeline->dynamic_state.mask & V3DV_DYNAMIC_STENCIL_COMPARE_MASK ?
         const uint8_t reference =
      pipeline->dynamic_state.mask & V3DV_DYNAMIC_STENCIL_COMPARE_MASK ?
         v3dvx_pack(stencil_cfg, STENCIL_CFG, config) {
      config.front_config = is_front;
   config.back_config = is_back;
   config.stencil_write_mask = write_mask;
   config.stencil_test_mask = compare_mask;
   config.stencil_test_function = stencil_state->compareOp;
   config.stencil_pass_op = translate_stencil_op(stencil_state->passOp);
   config.depth_test_fail_op = translate_stencil_op(stencil_state->depthFailOp);
   config.stencil_test_fail_op = translate_stencil_op(stencil_state->failOp);
         }
      static void
   pack_stencil_cfg(struct v3dv_pipeline *pipeline,
         {
               if (!ds_info || !ds_info->stencilTestEnable)
            if (pipeline->subpass->ds_attachment.attachment == VK_ATTACHMENT_UNUSED)
            const uint32_t dynamic_stencil_states = V3DV_DYNAMIC_STENCIL_COMPARE_MASK |
                     /* If front != back or we have dynamic stencil state we can't emit a single
   * packet for both faces.
   */
   bool needs_front_and_back = false;
   if ((pipeline->dynamic_state.mask & dynamic_stencil_states) ||
      memcmp(&ds_info->front, &ds_info->back, sizeof(ds_info->front)))
         /* If the front and back configurations are the same we can emit both with
   * a single packet.
   */
   pipeline->emit_stencil_cfg[0] = true;
   if (!needs_front_and_back) {
      pack_single_stencil_cfg(pipeline, pipeline->stencil_cfg[0],
      } else {
      pipeline->emit_stencil_cfg[1] = true;
   pack_single_stencil_cfg(pipeline, pipeline->stencil_cfg[0],
         pack_single_stencil_cfg(pipeline, pipeline->stencil_cfg[1],
         }
      void
   v3dX(pipeline_pack_state)(struct v3dv_pipeline *pipeline,
                           const VkPipelineColorBlendStateCreateInfo *cb_info,
   const VkPipelineDepthStencilStateCreateInfo *ds_info,
   {
      pack_blend(pipeline, cb_info);
   pack_cfg_bits(pipeline, ds_info, rs_info, pv_info, ls_info, ms_info);
      }
      static void
   pack_shader_state_record(struct v3dv_pipeline *pipeline)
   {
      assert(sizeof(pipeline->shader_state_record) >=
            struct v3d_fs_prog_data *prog_data_fs =
            struct v3d_vs_prog_data *prog_data_vs =
            struct v3d_vs_prog_data *prog_data_vs_bin =
               /* Note: we are not packing addresses, as we need the job (see
   * cl_pack_emit_reloc). Additionally uniforms can't be filled up at this
   * point as they depend on dynamic info that can be set after create the
   * pipeline (like viewport), . Would need to be filled later, so we are
   * doing a partial prepacking.
   */
   v3dvx_pack(pipeline->shader_state_record, GL_SHADER_STATE_RECORD, shader) {
               if (!pipeline->has_gs) {
      shader.point_size_in_shaded_vertex_data =
      } else {
      struct v3d_gs_prog_data *prog_data_gs =
                     /* Must be set if the shader modifies Z, discards, or modifies
   * the sample mask.  For any of these cases, the fragment
   * shader needs to write the Z value (even just discards).
   */
            /* Set if the EZ test must be disabled (due to shader side
   * effects and the early_z flag not being present in the
   * shader).
   */
            shader.fragment_shader_uses_real_pixel_centre_w_in_addition_to_centroid_w2 =
            /* The description for gl_SampleID states that if a fragment shader reads
   * it, then we should automatically activate per-sample shading. However,
   * the Vulkan spec also states that if a framebuffer has no attachments:
   *
   *    "The subpass continues to use the width, height, and layers of the
   *     framebuffer to define the dimensions of the rendering area, and the
   *     rasterizationSamples from each pipeline’s
   *     VkPipelineMultisampleStateCreateInfo to define the number of
   *     samples used in rasterization multisample rasterization."
   *
   * So in this scenario, if the pipeline doesn't enable multiple samples
   * but the fragment shader accesses gl_SampleID we would be requested
   * to do per-sample shading in single sample rasterization mode, which
   * is pointless, so just disable it in that case.
   */
   shader.enable_sample_rate_shading =
                           shader.do_scoreboard_wait_on_first_thread_switch =
         shader.disable_implicit_point_line_varyings =
            shader.number_of_varyings_in_fragment_shader =
            /* Note: see previous note about addresses */
   /* shader.coordinate_shader_code_address */
   /* shader.vertex_shader_code_address */
      #if V3D_VERSION == 42
         shader.coordinate_shader_propagate_nans = true;
   shader.vertex_shader_propagate_nans = true;
            /* FIXME: Use combined input/output size flag in the common case (also
   * on v3d, see v3dx_draw).
   */
   shader.coordinate_shader_has_separate_input_and_output_vpm_blocks =
         shader.vertex_shader_has_separate_input_and_output_vpm_blocks =
         shader.coordinate_shader_input_vpm_segment_size =
      prog_data_vs_bin->separate_segments ?
      shader.vertex_shader_input_vpm_segment_size =
         #endif
            /* On V3D 7.1 there isn't a specific flag to set if we are using
   * shared/separate segments or not. We just set the value of
   * vpm_input_size to 0, and set output to the max needed. That should be
   * already properly set on prog_data_vs_bin
   #if V3D_VERSION == 71
         shader.coordinate_shader_input_vpm_segment_size =
         shader.vertex_shader_input_vpm_segment_size =
   #endif
            shader.coordinate_shader_output_vpm_segment_size =
         shader.vertex_shader_output_vpm_segment_size =
            /* Note: see previous note about addresses */
   /* shader.coordinate_shader_uniforms_address */
   /* shader.vertex_shader_uniforms_address */
            shader.min_coord_shader_input_segments_required_in_play =
         shader.min_vertex_shader_input_segments_required_in_play =
            shader.min_coord_shader_output_segments_required_in_play_in_addition_to_vcm_cache_size =
         shader.min_vertex_shader_output_segments_required_in_play_in_addition_to_vcm_cache_size =
            shader.coordinate_shader_4_way_threadable =
         shader.vertex_shader_4_way_threadable =
         shader.fragment_shader_4_way_threadable =
            shader.coordinate_shader_start_in_final_thread_section =
         shader.vertex_shader_start_in_final_thread_section =
         shader.fragment_shader_start_in_final_thread_section =
            shader.vertex_id_read_by_coordinate_shader =
         shader.base_instance_id_read_by_coordinate_shader =
         shader.instance_id_read_by_coordinate_shader =
         shader.vertex_id_read_by_vertex_shader =
         shader.base_instance_id_read_by_vertex_shader =
         shader.instance_id_read_by_vertex_shader =
            /* Note: see previous note about addresses */
         }
      static void
   pack_vcm_cache_size(struct v3dv_pipeline *pipeline)
   {
      assert(sizeof(pipeline->vcm_cache_size) ==
            v3dvx_pack(pipeline->vcm_cache_size, VCM_CACHE_SIZE, vcm) {
      vcm.number_of_16_vertex_batches_for_binning = pipeline->vpm_cfg_bin.Vc;
         }
      /* As defined on the GL_SHADER_STATE_ATTRIBUTE_RECORD */
   static uint8_t
   get_attr_type(const struct util_format_description *desc)
   {
      uint32_t r_size = desc->channel[0].size;
            switch (desc->channel[0].type) {
   case UTIL_FORMAT_TYPE_FLOAT:
      if (r_size == 32) {
         } else {
      assert(r_size == 16);
      }
         case UTIL_FORMAT_TYPE_SIGNED:
   case UTIL_FORMAT_TYPE_UNSIGNED:
      switch (r_size) {
   case 32:
      attr_type = ATTRIBUTE_INT;
      case 16:
      attr_type = ATTRIBUTE_SHORT;
      case 10:
      attr_type = ATTRIBUTE_INT2_10_10_10;
      case 8:
      attr_type = ATTRIBUTE_BYTE;
      default:
      fprintf(stderr,
         "format %s unsupported\n",
   attr_type = ATTRIBUTE_BYTE;
      }
         default:
      fprintf(stderr,
         "format %s unsupported\n",
                  }
      static void
   pack_shader_state_attribute_record(struct v3dv_pipeline *pipeline,
               {
      const uint32_t packet_length =
            const struct util_format_description *desc =
                     v3dvx_pack(&pipeline->vertex_attrs[index * packet_length],
               /* vec_size == 0 means 4 */
   attr.vec_size = desc->nr_channels & 3;
   attr.signed_int_type = (desc->channel[0].type ==
         attr.normalized_int_type = desc->channel[0].normalized;
            attr.instance_divisor = MIN2(pipeline->vb[binding].instance_divisor,
         attr.stride = pipeline->vb[binding].stride;
         }
      void
   v3dX(pipeline_pack_compile_state)(struct v3dv_pipeline *pipeline,
               {
      pack_shader_state_record(pipeline);
            pipeline->vb_count = vi_info->vertexBindingDescriptionCount;
   for (uint32_t i = 0; i < vi_info->vertexBindingDescriptionCount; i++) {
      const VkVertexInputBindingDescription *desc =
            pipeline->vb[desc->binding].stride = desc->stride;
               if (vd_info) {
      for (uint32_t i = 0; i < vd_info->vertexBindingDivisorCount; i++) {
                                    pipeline->va_count = 0;
   struct v3d_vs_prog_data *prog_data_vs =
            for (uint32_t i = 0; i < vi_info->vertexAttributeDescriptionCount; i++) {
      const VkVertexInputAttributeDescription *desc =
                  /* We use a custom driver_location_map instead of
   * nir_find_variable_with_location because if we were able to get the
   * shader variant from the cache, we would not have the nir shader
   * available.
   */
   uint32_t driver_location =
            if (driver_location != -1) {
      assert(driver_location < MAX_VERTEX_ATTRIBS);
   pipeline->va[driver_location].offset = desc->offset;
                                    }
      #if V3D_VERSION == 42
   static bool
   pipeline_has_integer_vertex_attrib(struct v3dv_pipeline *pipeline)
   {
      for (uint8_t i = 0; i < pipeline->va_count; i++) {
      if (vk_format_is_int(pipeline->va[i].vk_format))
      }
      }
   #endif
      bool
   v3dX(pipeline_needs_default_attribute_values)(struct v3dv_pipeline *pipeline)
   {
   #if V3D_VERSION == 42
         #endif
            }
      /* @pipeline can be NULL. In that case we assume the most common case. For
   * example, for v42 we assume in that case that all the attributes have a
   * float format (we only create an all-float BO once and we reuse it with all
   * float pipelines), otherwise we look at the actual type of each attribute
   * used with the specific pipeline passed in.
   */
   struct v3dv_bo *
   v3dX(create_default_attribute_values)(struct v3dv_device *device,
         {
   #if V3D_VERSION >= 71
         #endif
         uint32_t size = MAX_VERTEX_ATTRIBS * sizeof(float) * 4;
                     if (!bo) {
      fprintf(stderr, "failed to allocate memory for the default "
                     bool ok = v3dv_bo_map(device, bo, size);
   if (!ok) {
      fprintf(stderr, "failed to map default attribute values buffer\n");
               uint32_t *attrs = bo->map;
   uint8_t va_count = pipeline != NULL ? pipeline->va_count : 0;
   for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++) {
      attrs[i * 4 + 0] = 0;
   attrs[i * 4 + 1] = 0;
   attrs[i * 4 + 2] = 0;
   VkFormat attr_format =
         if (i < va_count && vk_format_is_int(attr_format)) {
         } else {
                                 }
