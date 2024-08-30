   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_buffer.h"
   #include "nvk_entrypoints.h"
   #include "nvk_cmd_buffer.h"
   #include "nvk_device.h"
   #include "nvk_format.h"
   #include "nvk_image.h"
   #include "nvk_image_view.h"
   #include "nvk_mme.h"
   #include "nvk_physical_device.h"
   #include "nvk_pipeline.h"
      #include "nil_format.h"
   #include "util/bitpack_helpers.h"
   #include "vulkan/runtime/vk_render_pass.h"
   #include "vulkan/runtime/vk_standard_sample_locations.h"
   #include "vulkan/util/vk_format.h"
      #include "nouveau_context.h"
      #include "nvk_cl902d.h"
   #include "nvk_cl9039.h"
   #include "nvk_cl906f.h"
   #include "nvk_cl90b5.h"
   #include "nvk_cl90c0.h"
   #include "nvk_clb0c0.h"
      #include "nvk_cl9097.h"
   #include "nvk_cla097.h"
   #include "nvk_clb097.h"
   #include "nvk_clb197.h"
   #include "nvk_clc397.h"
   #include "nvk_clc597.h"
   #include "drf.h"
      static inline uint16_t
   nvk_cmd_buffer_3d_cls(struct nvk_cmd_buffer *cmd)
   {
         }
      void
   nvk_mme_set_priv_reg(struct mme_builder *b)
   {
      mme_mthd(b, NV9097_WAIT_FOR_IDLE);
            mme_mthd(b, NV9097_SET_MME_SHADOW_SCRATCH(0));
   mme_emit(b, mme_zero());
   mme_emit(b, mme_load(b));
            /* Not sure if this has to strictly go before SET_FALCON04, but it might.
   * We also don't really know what that value indicates and when and how it's
   * set.
   */
   struct mme_value s26 = mme_state(b, NV9097_SET_MME_SHADOW_SCRATCH(26));
            mme_mthd(b, NV9097_SET_FALCON04);
            mme_if(b, ieq, s26, mme_imm(2)) {
      struct mme_value loop_cond = mme_mov(b, mme_zero());
   mme_while(b, ine, loop_cond, mme_imm(1)) {
      mme_state_to(b, loop_cond, NV9097_SET_MME_SHADOW_SCRATCH(0));
   mme_mthd(b, NV9097_NO_OPERATION);
                  mme_if(b, ine, s26, mme_imm(2)) {
      mme_loop(b, mme_imm(10)) {
      mme_mthd(b, NV9097_NO_OPERATION);
            }
      VkResult
   nvk_queue_init_context_draw_state(struct nvk_queue *queue)
   {
               uint32_t push_data[2048];
   struct nv_push push;
   nv_push_init(&push, push_data, ARRAY_SIZE(push_data));
            /* M2MF state */
   if (dev->pdev->info.cls_m2mf <= FERMI_MEMORY_TO_MEMORY_FORMAT_A) {
      /* we absolutely do not support Fermi, but if somebody wants to toy
   * around with it, this is a must
   */
   P_MTHD(p, NV9039, SET_OBJECT);
   P_NV9039_SET_OBJECT(p, {
      .class_id = dev->pdev->info.cls_m2mf,
                  /* 2D state */
   P_MTHD(p, NV902D, SET_OBJECT);
   P_NV902D_SET_OBJECT(p, {
      .class_id = dev->pdev->info.cls_eng2d,
               /* 3D state */
   P_MTHD(p, NV9097, SET_OBJECT);
   P_NV9097_SET_OBJECT(p, {
      .class_id = dev->pdev->info.cls_eng3d,
               for (uint32_t mme = 0, mme_pos = 0; mme < NVK_MME_COUNT; mme++) {
      size_t size;
   uint32_t *dw = nvk_build_mme(&nvk_device_physical(dev)->info, mme, &size);
   if (dw == NULL)
            assert(size % sizeof(uint32_t) == 0);
            P_MTHD(p, NV9097, LOAD_MME_START_ADDRESS_RAM_POINTER);
   P_NV9097_LOAD_MME_START_ADDRESS_RAM_POINTER(p, mme);
            P_1INC(p, NV9097, LOAD_MME_INSTRUCTION_RAM_POINTER);
   P_NV9097_LOAD_MME_INSTRUCTION_RAM_POINTER(p, mme_pos);
                                 /* Enable FP hepler invocation memory loads
   *
   * For generations with firmware support for our `SET_PRIV_REG` mme method
   * we simply use that. On older generations we'll let the kernel do it.
   * Starting with GSP we have to do it via the firmware anyway.
   */
   if (dev->pdev->info.cls_eng3d >= MAXWELL_B) {
      unsigned reg = dev->pdev->info.cls_eng3d >= VOLTA_A ? 0x419ba4 : 0x419f78;
   P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_SET_PRIV_REG));
   P_INLINE_DATA(p, 0);
   P_INLINE_DATA(p, BITFIELD_BIT(3));
                        P_IMMD(p, NV9097, SET_Z_COMPRESSION, ENABLE_TRUE);
   P_MTHD(p, NV9097, SET_COLOR_COMPRESSION(0));
   for (unsigned i = 0; i < 8; i++)
                  //   P_MTHD(cmd->push, NVC0_3D, CSAA_ENABLE);
   //   P_INLINE_DATA(cmd->push, 0);
                           P_IMMD(p, NV9097, SET_BLEND_SEPARATE_FOR_ALPHA, ENABLE_TRUE);
   P_IMMD(p, NV9097, SET_SINGLE_CT_WRITE_CONTROL, ENABLE_TRUE);
   P_IMMD(p, NV9097, SET_SINGLE_ROP_CONTROL, ENABLE_FALSE);
                                       P_IMMD(p, NV9097, SET_L1_CONFIGURATION,
            P_IMMD(p, NV9097, SET_REDUCE_COLOR_THRESHOLDS_ENABLE, V_FALSE);
   P_IMMD(p, NV9097, SET_REDUCE_COLOR_THRESHOLDS_UNORM8, {
         });
   P_MTHD(p, NV9097, SET_REDUCE_COLOR_THRESHOLDS_UNORM10);
   P_NV9097_SET_REDUCE_COLOR_THRESHOLDS_UNORM10(p, {
         });
   P_NV9097_SET_REDUCE_COLOR_THRESHOLDS_UNORM16(p, {
         });
   P_NV9097_SET_REDUCE_COLOR_THRESHOLDS_FP11(p, {
         });
   P_NV9097_SET_REDUCE_COLOR_THRESHOLDS_FP16(p, {
         });
   P_NV9097_SET_REDUCE_COLOR_THRESHOLDS_SRGB8(p, {
                  if (dev->pdev->info.cls_eng3d < VOLTA_A)
            P_IMMD(p, NV9097, CHECK_SPH_VERSION, {
      .current = 3,
      });
   P_IMMD(p, NV9097, CHECK_AAM_VERSION, {
      .current = 2,
               if (dev->pdev->info.cls_eng3d < MAXWELL_A)
            P_IMMD(p, NV9097, SET_L2_CACHE_CONTROL_FOR_ROP_PREFETCH_READ_REQUESTS,
         P_IMMD(p, NV9097, SET_L2_CACHE_CONTROL_FOR_ROP_NONINTERLOCKED_READ_REQUESTS,
         P_IMMD(p, NV9097, SET_L2_CACHE_CONTROL_FOR_ROP_INTERLOCKED_READ_REQUESTS,
         P_IMMD(p, NV9097, SET_L2_CACHE_CONTROL_FOR_ROP_NONINTERLOCKED_WRITE_REQUESTS,
         P_IMMD(p, NV9097, SET_L2_CACHE_CONTROL_FOR_ROP_INTERLOCKED_WRITE_REQUESTS,
                     P_IMMD(p, NV9097, SET_ATTRIBUTE_DEFAULT, {
      .color_front_diffuse    = COLOR_FRONT_DIFFUSE_VECTOR_0001,
   .color_front_specular   = COLOR_FRONT_SPECULAR_VECTOR_0001,
   .generic_vector         = GENERIC_VECTOR_VECTOR_0001,
   .fixed_fnc_texture      = FIXED_FNC_TEXTURE_VECTOR_0001,
   .dx9_color0             = DX9_COLOR0_VECTOR_0001,
                        P_IMMD(p, NV9097, SET_RENDER_ENABLE_CONTROL,
            P_IMMD(p, NV9097, SET_PS_OUTPUT_SAMPLE_MASK_USAGE, {
      .enable                       = ENABLE_TRUE,
               if (dev->pdev->info.cls_eng3d < VOLTA_A)
            P_IMMD(p, NV9097, SET_BLEND_OPT_CONTROL, ALLOW_FLOAT_PIXEL_KILLS_TRUE);
            if (dev->pdev->info.cls_eng3d < MAXWELL_A)
            if (dev->pdev->info.cls_eng3d >= KEPLER_A &&
      dev->pdev->info.cls_eng3d < MAXWELL_A) {
   P_IMMD(p, NVA097, SET_TEXTURE_INSTRUCTION_OPERAND,
               P_IMMD(p, NV9097, SET_ALPHA_TEST, ENABLE_FALSE);
   P_IMMD(p, NV9097, SET_TWO_SIDED_LIGHT, ENABLE_FALSE);
   P_IMMD(p, NV9097, SET_COLOR_CLAMP, ENABLE_TRUE);
   P_IMMD(p, NV9097, SET_PS_SATURATE, {
      .output0 = OUTPUT0_FALSE,
   .output1 = OUTPUT1_FALSE,
   .output2 = OUTPUT2_FALSE,
   .output3 = OUTPUT3_FALSE,
   .output4 = OUTPUT4_FALSE,
   .output5 = OUTPUT5_FALSE,
   .output6 = OUTPUT6_FALSE,
               /* vulkan allows setting point sizes only within shaders */
   P_IMMD(p, NV9097, SET_ATTRIBUTE_POINT_SIZE, {
      .enable  = ENABLE_TRUE,
      });
               /* From vulkan spec's point rasterization:
   * "Point rasterization produces a fragment for each fragment area group of
   * framebuffer pixels with one or more sample points that intersect a region
   * centered at the point’s (xf,yf).
   * This region is a square with side equal to the current point size.
   * ... (xf,yf) is the exact, unrounded framebuffer coordinate of the vertex
   * for the point"
   *
   * So it seems we always need square points with PointCoords like OpenGL
   * point sprites.
   *
   * From OpenGL compatibility spec:
   * Basic point rasterization:
   * "If point sprites are enabled, then point rasterization produces a
   * fragment for each framebuffer pixel whose center lies inside a square
   * centered at the point’s (xw, yw), with side length equal to the current
   * point size.
   * ... and xw and yw are the exact, unrounded window coordinates of the
   * vertex for the point"
   *
   * And Point multisample rasterization:
   * "This region is a circle having diameter equal to the current point width
   * if POINT_SPRITE is disabled, or a square with side equal to the current
   * point width if POINT_SPRITE is enabled."
   */
   P_IMMD(p, NV9097, SET_POINT_SPRITE, ENABLE_TRUE);
   P_IMMD(p, NV9097, SET_POINT_SPRITE_SELECT, {
      .rmode      = RMODE_ZERO,
   .origin     = ORIGIN_TOP,
   .texture0   = TEXTURE0_PASSTHROUGH,
   .texture1   = TEXTURE1_PASSTHROUGH,
   .texture2   = TEXTURE2_PASSTHROUGH,
   .texture3   = TEXTURE3_PASSTHROUGH,
   .texture4   = TEXTURE4_PASSTHROUGH,
   .texture5   = TEXTURE5_PASSTHROUGH,
   .texture6   = TEXTURE6_PASSTHROUGH,
   .texture7   = TEXTURE7_PASSTHROUGH,
   .texture8   = TEXTURE8_PASSTHROUGH,
               /* OpenGL's GL_POINT_SMOOTH */
            if (dev->pdev->info.cls_eng3d >= MAXWELL_B)
                              P_IMMD(p, NV9097, SET_HYBRID_ANTI_ALIAS_CONTROL, {
      .passes     = 1,
               /* Enable multisample rasterization even for one sample rasterization,
   * this way we get strict lines and rectangular line support.
   * More info at: DirectX rasterization rules
   */
            if (dev->pdev->info.cls_eng3d >= MAXWELL_B) {
      P_IMMD(p, NVB197, SET_OFFSET_RENDER_TARGET_INDEX,
                        P_IMMD(p, NV9097, SET_WINDOW_ORIGIN, {
      .mode    = MODE_UPPER_LEFT,
               P_MTHD(p, NV9097, SET_WINDOW_OFFSET_X);
   P_NV9097_SET_WINDOW_OFFSET_X(p, 0);
            P_IMMD(p, NV9097, SET_ACTIVE_ZCULL_REGION, 0x3f);
   P_IMMD(p, NV9097, SET_WINDOW_CLIP_ENABLE, V_FALSE);
         //   P_IMMD(p, NV9097, X_X_X_SET_CLEAR_CONTROL, {
   //      .respect_stencil_mask   = RESPECT_STENCIL_MASK_FALSE,
   //      .use_clear_rect         = USE_CLEAR_RECT_FALSE,
   //   });
                  P_IMMD(p, NV9097, SET_VIEWPORT_CLIP_CONTROL, {
      .min_z_zero_max_z_one      = MIN_Z_ZERO_MAX_Z_ONE_FALSE,
   .pixel_min_z               = PIXEL_MIN_Z_CLAMP,
   .pixel_max_z               = PIXEL_MAX_Z_CLAMP,
   .geometry_guardband        = GEOMETRY_GUARDBAND_SCALE_256,
   .line_point_cull_guardband = LINE_POINT_CULL_GUARDBAND_SCALE_256,
   .geometry_clip             = GEOMETRY_CLIP_WZERO_CLIP,
               for (unsigned i = 0; i < 16; i++)
                     for (uint32_t i = 0; i < 6; i++) {
      P_IMMD(p, NV9097, SET_PIPELINE_SHADER(i), {
      .enable  = ENABLE_FALSE,
               //   P_MTHD(cmd->push, NVC0_3D, MACRO_GP_SELECT);
   //   P_INLINE_DATA(cmd->push, 0x40);
      P_IMMD(p, NV9097, SET_RT_LAYER, {
      .v = 0,
         //   P_MTHD(cmd->push, NVC0_3D, MACRO_TEP_SELECT;
   //   P_INLINE_DATA(cmd->push, 0x30);
         P_IMMD(p, NV9097, SET_POINT_CENTER_MODE, V_OGL);
   P_IMMD(p, NV9097, SET_EDGE_FLAG, V_TRUE);
            uint64_t zero_addr = dev->zero_page->offset;
   P_MTHD(p, NV9097, SET_VERTEX_STREAM_SUBSTITUTE_A);
   P_NV9097_SET_VERTEX_STREAM_SUBSTITUTE_A(p, zero_addr >> 32);
            if (dev->pdev->info.cls_eng3d >= FERMI_A &&
      dev->pdev->info.cls_eng3d < MAXWELL_A) {
   assert(dev->vab_memory);
   uint64_t vab_addr = dev->vab_memory->offset;
   P_MTHD(p, NV9097, SET_VAB_MEMORY_AREA_A);
   P_NV9097_SET_VAB_MEMORY_AREA_A(p, vab_addr >> 32);
   P_NV9097_SET_VAB_MEMORY_AREA_B(p, vab_addr);
               if (dev->pdev->info.cls_eng3d == MAXWELL_A)
            /* Compute state */
   P_MTHD(p, NV90C0, SET_OBJECT);
   P_NV90C0_SET_OBJECT(p, {
      .class_id = dev->pdev->info.cls_compute,
               if (dev->pdev->info.cls_compute == MAXWELL_COMPUTE_A)
            return nvk_queue_submit_simple(queue, nv_push_dw_count(&push), push_data,
      }
      static void
   nvk_cmd_buffer_dirty_render_pass(struct nvk_cmd_buffer *cmd)
   {
               BITSET_SET(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_TEST_ENABLE);
   BITSET_SET(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_WRITE_ENABLE);
   BITSET_SET(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_BOUNDS_TEST_ENABLE);
      }
      void
   nvk_cmd_buffer_begin_graphics(struct nvk_cmd_buffer *cmd,
         {
      if (cmd->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      struct nv_push *p = nvk_cmd_buffer_push(cmd, 3);
   P_MTHD(p, NV9097, INVALIDATE_SAMPLER_CACHE_NO_WFI);
   P_NV9097_INVALIDATE_SAMPLER_CACHE_NO_WFI(p, {
         });
   P_NV9097_INVALIDATE_TEXTURE_HEADER_CACHE_NO_WFI(p, {
                     if (cmd->vk.level != VK_COMMAND_BUFFER_LEVEL_PRIMARY &&
      (pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)) {
   char gcbiar_data[VK_GCBIARR_DATA_SIZE(NVK_MAX_RTS)];
   const VkRenderingInfo *resume_info =
      vk_get_command_buffer_inheritance_as_rendering_resume(cmd->vk.level,
            if (resume_info) {
         } else {
      const VkCommandBufferInheritanceRenderingInfo *inheritance_info =
      vk_get_command_buffer_inheritance_rendering_info(cmd->vk.level,
               struct nvk_rendering_state *render = &cmd->state.gfx.render;
   render->flags = inheritance_info->flags;
   render->area = (VkRect2D) { };
                  render->color_att_count = inheritance_info->colorAttachmentCount;
   for (uint32_t i = 0; i < render->color_att_count; i++) {
      render->color_att[i].vk_format =
      }
   render->depth_att.vk_format =
                                 }
      static void
   nvk_attachment_init(struct nvk_attachment *att,
         {
      if (info == NULL || info->imageView == VK_NULL_HANDLE) {
      *att = (struct nvk_attachment) { .iview = NULL, };
               VK_FROM_HANDLE(nvk_image_view, iview, info->imageView);
   *att = (struct nvk_attachment) {
      .vk_format = iview->vk.format,
               if (info->resolveMode != VK_RESOLVE_MODE_NONE) {
      VK_FROM_HANDLE(nvk_image_view, res_iview, info->resolveImageView);
   att->resolve_mode = info->resolveMode;
         }
      static uint32_t
   nil_to_nv9097_samples_mode(enum nil_sample_layout sample_layout)
   {
   #define MODE(S) [NIL_SAMPLE_LAYOUT_##S] = NV9097_SET_ANTI_ALIAS_SAMPLES_MODE_##S
      uint16_t nil_to_nv9097[] = {
      MODE(1X1),
   MODE(2X1),
   MODE(2X2),
   MODE(4X2),
         #undef MODE
                  }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetRenderingAreaGranularityKHR(
      VkDevice device,
   const VkRenderingAreaInfoKHR *pRenderingAreaInfo,
      {
         }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdBeginRendering(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
                     render->flags = pRenderingInfo->flags;
   render->area = pRenderingInfo->renderArea;
   render->view_mask = pRenderingInfo->viewMask;
            const uint32_t layer_count =
      render->view_mask ? util_last_bit(render->view_mask) :
         render->color_att_count = pRenderingInfo->colorAttachmentCount;
   for (uint32_t i = 0; i < render->color_att_count; i++) {
      nvk_attachment_init(&render->color_att[i],
               nvk_attachment_init(&render->depth_att,
         nvk_attachment_init(&render->stencil_att,
                     /* Always emit at least one color attachment, even if it's just a dummy. */
   uint32_t color_att_count = MAX2(1, render->color_att_count);
            P_IMMD(p, NV9097, SET_MME_SHADOW_SCRATCH(NVK_MME_SCRATCH_VIEW_MASK),
            P_MTHD(p, NV9097, SET_SURFACE_CLIP_HORIZONTAL);
   P_NV9097_SET_SURFACE_CLIP_HORIZONTAL(p, {
      .x       = render->area.offset.x,
      });
   P_NV9097_SET_SURFACE_CLIP_VERTICAL(p, {
      .y       = render->area.offset.y,
               enum nil_sample_layout sample_layout = NIL_SAMPLE_LAYOUT_INVALID;
   for (uint32_t i = 0; i < color_att_count; i++) {
      if (render->color_att[i].iview) {
      const struct nvk_image_view *iview = render->color_att[i].iview;
   const struct nvk_image *image = (struct nvk_image *)iview->vk.image;
   /* Rendering to multi-planar images is valid for a specific single plane
   * only, so assert that what we have is a single-plane, obtain its index,
   * and begin rendering
   */
                  const struct nil_image_level *level =
                        assert(sample_layout == NIL_SAMPLE_LAYOUT_INVALID ||
                           P_MTHD(p, NV9097, SET_COLOR_TARGET_A(i));
   P_NV9097_SET_COLOR_TARGET_A(p, i, addr >> 32);
   P_NV9097_SET_COLOR_TARGET_B(p, i, addr);
   assert(level->tiling.is_tiled);
   P_NV9097_SET_COLOR_TARGET_WIDTH(p, i, level_extent_sa.w);
   P_NV9097_SET_COLOR_TARGET_HEIGHT(p, i, level_extent_sa.h);
   const enum pipe_format p_format =
         const uint8_t ct_format = nil_format_to_color_target(p_format);
   P_NV9097_SET_COLOR_TARGET_FORMAT(p, i, ct_format);
   P_NV9097_SET_COLOR_TARGET_MEMORY(p, i, {
      .block_width   = BLOCK_WIDTH_ONE_GOB,
   .block_height  = level->tiling.y_log2,
   .block_depth   = level->tiling.z_log2,
   .layout        = LAYOUT_BLOCKLINEAR,
   .third_dimension_control =
      (image->planes[ip].nil.dim == NIL_IMAGE_DIM_3D) ?
   THIRD_DIMENSION_CONTROL_THIRD_DIMENSION_DEFINES_DEPTH_SIZE :
   });
   P_NV9097_SET_COLOR_TARGET_THIRD_DIMENSION(p, i,
         P_NV9097_SET_COLOR_TARGET_ARRAY_PITCH(p, i,
            } else {
      P_MTHD(p, NV9097, SET_COLOR_TARGET_A(i));
   P_NV9097_SET_COLOR_TARGET_A(p, i, 0);
   P_NV9097_SET_COLOR_TARGET_B(p, i, 0);
   P_NV9097_SET_COLOR_TARGET_WIDTH(p, i, 64);
   P_NV9097_SET_COLOR_TARGET_HEIGHT(p, i, 0);
   P_NV9097_SET_COLOR_TARGET_FORMAT(p, i, V_DISABLED);
   P_NV9097_SET_COLOR_TARGET_MEMORY(p, i, {
         });
   P_NV9097_SET_COLOR_TARGET_THIRD_DIMENSION(p, i, layer_count);
   P_NV9097_SET_COLOR_TARGET_ARRAY_PITCH(p, i, 0);
                  P_IMMD(p, NV9097, SET_CT_SELECT, {
      .target_count = color_att_count,
   .target0 = 0,
   .target1 = 1,
   .target2 = 2,
   .target3 = 3,
   .target4 = 4,
   .target5 = 5,
   .target6 = 6,
               if (render->depth_att.iview || render->stencil_att.iview) {
      struct nvk_image_view *iview = render->depth_att.iview ?
               const struct nvk_image *image = (struct nvk_image *)iview->vk.image;
   /* Depth/stencil are always single-plane */
   assert(iview->plane_count == 1);
   const uint8_t ip = iview->planes[0].image_plane;
            uint64_t addr = nvk_image_base_address(image, ip);
   uint32_t mip_level = iview->vk.base_mip_level;
   uint32_t base_array_layer = iview->vk.base_array_layer;
            if (nil_image.dim == NIL_IMAGE_DIM_3D) {
      uint64_t level_offset_B;
   nil_image_3d_level_as_2d_array(&nil_image, mip_level,
         addr += level_offset_B;
   mip_level = 0;
   base_array_layer = 0;
               const struct nil_image_level *level = &nil_image.levels[mip_level];
            assert(sample_layout == NIL_SAMPLE_LAYOUT_INVALID ||
                  P_MTHD(p, NV9097, SET_ZT_A);
   P_NV9097_SET_ZT_A(p, addr >> 32);
   P_NV9097_SET_ZT_B(p, addr);
   const enum pipe_format p_format =
         const uint8_t zs_format = nil_format_to_depth_stencil(p_format);
   P_NV9097_SET_ZT_FORMAT(p, zs_format);
   assert(level->tiling.z_log2 == 0);
   P_NV9097_SET_ZT_BLOCK_SIZE(p, {
      .width = WIDTH_ONE_GOB,
   .height = level->tiling.y_log2,
      });
                     struct nil_extent4d level_extent_sa =
            P_MTHD(p, NV9097, SET_ZT_SIZE_A);
   P_NV9097_SET_ZT_SIZE_A(p, level_extent_sa.w);
   P_NV9097_SET_ZT_SIZE_B(p, level_extent_sa.h);
   P_NV9097_SET_ZT_SIZE_C(p, {
      .third_dimension  = base_array_layer + layer_count,
                        if (nvk_cmd_buffer_3d_cls(cmd) >= MAXWELL_B) {
      P_IMMD(p, NVC597, SET_ZT_SPARSE, {
               } else {
                  if (sample_layout == NIL_SAMPLE_LAYOUT_INVALID)
                     if (render->flags & VK_RENDERING_RESUMING_BIT)
            uint32_t clear_count = 0;
   VkClearAttachment clear_att[NVK_MAX_RTS + 1];
   for (uint32_t i = 0; i < pRenderingInfo->colorAttachmentCount; i++) {
      const VkRenderingAttachmentInfo *att_info =
         if (att_info->imageView == VK_NULL_HANDLE ||
                  clear_att[clear_count++] = (VkClearAttachment) {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .colorAttachment = i,
                  clear_att[clear_count] = (VkClearAttachment) { .aspectMask = 0, };
   if (pRenderingInfo->pDepthAttachment != NULL &&
      pRenderingInfo->pDepthAttachment->imageView != VK_NULL_HANDLE &&
   pRenderingInfo->pDepthAttachment->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
   clear_att[clear_count].aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
   clear_att[clear_count].clearValue.depthStencil.depth =
      }
   if (pRenderingInfo->pStencilAttachment != NULL &&
      pRenderingInfo->pStencilAttachment->imageView != VK_NULL_HANDLE &&
   pRenderingInfo->pStencilAttachment->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
   clear_att[clear_count].aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
   clear_att[clear_count].clearValue.depthStencil.stencil =
      }
   if (clear_att[clear_count].aspectMask != 0)
            if (clear_count > 0) {
      const VkClearRect clear_rect = {
      .rect = render->area,
   .baseArrayLayer = 0,
               P_MTHD(p, NV9097, SET_RENDER_ENABLE_OVERRIDE);
            nvk_CmdClearAttachments(nvk_cmd_buffer_to_handle(cmd),
         p = nvk_cmd_buffer_push(cmd, 2);
   P_MTHD(p, NV9097, SET_RENDER_ENABLE_OVERRIDE);
                  }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdEndRendering(VkCommandBuffer commandBuffer)
   {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
                     /* Translate render state back to VK for meta */
   VkRenderingAttachmentInfo vk_color_att[NVK_MAX_RTS];
   for (uint32_t i = 0; i < render->color_att_count; i++) {
      if (render->color_att[i].resolve_mode != VK_RESOLVE_MODE_NONE)
            vk_color_att[i] = (VkRenderingAttachmentInfo) {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = nvk_image_view_to_handle(render->color_att[i].iview),
   .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   .resolveMode = render->color_att[i].resolve_mode,
   .resolveImageView =
                        const VkRenderingAttachmentInfo vk_depth_att = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = nvk_image_view_to_handle(render->depth_att.iview),
   .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   .resolveMode = render->depth_att.resolve_mode,
   .resolveImageView =
            };
   if (render->depth_att.resolve_mode != VK_RESOLVE_MODE_NONE)
            const VkRenderingAttachmentInfo vk_stencil_att = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = nvk_image_view_to_handle(render->stencil_att.iview),
   .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   .resolveMode = render->stencil_att.resolve_mode,
   .resolveImageView =
            };
   if (render->stencil_att.resolve_mode != VK_RESOLVE_MODE_NONE)
            const VkRenderingInfo vk_render = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea = render->area,
   .layerCount = render->layer_count,
   .viewMask = render->view_mask,
   .colorAttachmentCount = render->color_att_count,
   .pColorAttachments = vk_color_att,
   .pDepthAttachment = &vk_depth_att,
               if (render->flags & VK_RENDERING_SUSPENDING_BIT)
                     if (need_resolve) {
      struct nv_push *p = nvk_cmd_buffer_push(cmd, 2);
                  }
      void
   nvk_cmd_bind_graphics_pipeline(struct nvk_cmd_buffer *cmd,
         {
      cmd->state.gfx.pipeline = pipeline;
            struct nv_push *p = nvk_cmd_buffer_push(cmd, pipeline->push_dw_count);
      }
      static void
   nvk_flush_vi_state(struct nvk_cmd_buffer *cmd)
   {
      struct nvk_device *dev = nvk_cmd_buffer_device(cmd);
   struct nvk_physical_device *pdev = nvk_device_physical(dev);
   const struct vk_dynamic_graphics_state *dyn =
                     if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VI) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VI_BINDINGS_VALID)) {
   u_foreach_bit(a, dyn->vi->attributes_valid) {
                     P_IMMD(p, NV9097, SET_VERTEX_ATTRIBUTE_A(a), {
      .stream                 = dyn->vi->attributes[a].binding,
   .offset                 = dyn->vi->attributes[a].offset,
   .component_bit_widths   = fmt->bit_widths,
   .numerical_type         = fmt->type,
                  u_foreach_bit(b, dyn->vi->bindings_valid) {
      const bool instanced = dyn->vi->bindings[b].input_rate ==
         P_IMMD(p, NV9097, SET_VERTEX_STREAM_INSTANCE_A(b), instanced);
   P_IMMD(p, NV9097, SET_VERTEX_STREAM_A_FREQUENCY(b),
                  if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VI_BINDINGS_VALID) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VI_BINDING_STRIDES)) {
   for (uint32_t b = 0; b < 32; b++) {
      P_IMMD(p, NV9097, SET_VERTEX_STREAM_A_FORMAT(b), {
      .stride = dyn->vi_binding_strides[b],
               }
      static void
   nvk_flush_ia_state(struct nvk_cmd_buffer *cmd)
   {
      const struct vk_dynamic_graphics_state *dyn =
                     if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_IA_PRIMITIVE_RESTART_ENABLE)) {
      struct nv_push *p = nvk_cmd_buffer_push(cmd, 2);
   P_IMMD(p, NV9097, SET_DA_PRIMITIVE_RESTART,
         }
      static void
   nvk_flush_ts_state(struct nvk_cmd_buffer *cmd)
   {
      const struct vk_dynamic_graphics_state *dyn =
            if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_TS_PATCH_CONTROL_POINTS)) {
      struct nv_push *p = nvk_cmd_buffer_push(cmd, 2);
         }
      static void
   nvk_flush_vp_state(struct nvk_cmd_buffer *cmd)
   {
      const struct vk_dynamic_graphics_state *dyn =
            struct nv_push *p =
                     if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_VIEWPORTS) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_DEPTH_CLIP_NEGATIVE_ONE_TO_ONE)) {
   for (uint32_t i = 0; i < dyn->vp.viewport_count; i++) {
               /* These exactly match the spec values.  Nvidia hardware oddities
   * are accounted for later.
   */
   const float o_x = vp->x + 0.5f * vp->width;
   const float o_y = vp->y + 0.5f * vp->height;
   const float o_z = !dyn->vp.depth_clip_negative_one_to_one ?
                  const float p_x = vp->width;
   const float p_y = vp->height;
   const float p_z = !dyn->vp.depth_clip_negative_one_to_one ?
                  P_MTHD(p, NV9097, SET_VIEWPORT_SCALE_X(i));
   P_NV9097_SET_VIEWPORT_SCALE_X(p, i, fui(0.5f * p_x));
                  P_NV9097_SET_VIEWPORT_OFFSET_X(p, i, fui(o_x));
                  float xmin = vp->x;
   float xmax = vp->x + vp->width;
   float ymin = MIN2(vp->y, vp->y + vp->height);
   float ymax = MAX2(vp->y, vp->y + vp->height);
   float zmin = MIN2(vp->minDepth, vp->maxDepth);
                  const float max_dim = (float)0xffff;
   xmin = CLAMP(xmin, 0, max_dim);
   xmax = CLAMP(xmax, 0, max_dim);
                  P_MTHD(p, NV9097, SET_VIEWPORT_CLIP_HORIZONTAL(i));
   P_NV9097_SET_VIEWPORT_CLIP_HORIZONTAL(p, i, {
      .x0      = xmin,
      });
   P_NV9097_SET_VIEWPORT_CLIP_VERTICAL(p, i, {
      .y0      = ymin,
      });
                  if (nvk_cmd_buffer_3d_cls(cmd) >= MAXWELL_B) {
      P_IMMD(p, NVB197, SET_VIEWPORT_COORDINATE_SWIZZLE(i), {
      .x = X_POS_X,
   .y = Y_POS_Y,
   .z = Z_POS_Z,
                        if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_DEPTH_CLIP_NEGATIVE_ONE_TO_ONE)) {
      P_IMMD(p, NV9097, SET_VIEWPORT_Z_CLIP,
         dyn->vp.depth_clip_negative_one_to_one ?
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_SCISSOR_COUNT)) {
      for (unsigned i = dyn->vp.scissor_count; i < NVK_MAX_VIEWPORTS; i++)
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_SCISSORS)) {
      for (unsigned i = 0; i < dyn->vp.scissor_count; i++) {
               const uint32_t xmin = MIN2(16384, s->offset.x);
   const uint32_t xmax = MIN2(16384, s->offset.x + s->extent.width);
                  P_MTHD(p, NV9097, SET_SCISSOR_ENABLE(i));
   P_NV9097_SET_SCISSOR_ENABLE(p, i, V_TRUE);
   P_NV9097_SET_SCISSOR_HORIZONTAL(p, i, {
      .xmin = xmin,
      });
   P_NV9097_SET_SCISSOR_VERTICAL(p, i, {
      .ymin = ymin,
               }
      static uint32_t
   vk_to_nv9097_polygon_mode(VkPolygonMode vk_mode)
   {
      ASSERTED uint16_t vk_to_nv9097[] = {
      [VK_POLYGON_MODE_FILL]  = NV9097_SET_FRONT_POLYGON_MODE_V_FILL,
   [VK_POLYGON_MODE_LINE]  = NV9097_SET_FRONT_POLYGON_MODE_V_LINE,
      };
            uint32_t nv9097_mode = 0x1b00 | (2 - vk_mode);
   assert(nv9097_mode == vk_to_nv9097[vk_mode]);
      }
      static uint32_t
   vk_to_nv9097_cull_mode(VkCullModeFlags vk_cull_mode)
   {
      static const uint16_t vk_to_nv9097[] = {
      [VK_CULL_MODE_FRONT_BIT]      = NV9097_OGL_SET_CULL_FACE_V_FRONT,
   [VK_CULL_MODE_BACK_BIT]       = NV9097_OGL_SET_CULL_FACE_V_BACK,
      };
   assert(vk_cull_mode < ARRAY_SIZE(vk_to_nv9097));
      }
      static uint32_t
   vk_to_nv9097_front_face(VkFrontFace vk_face)
   {
      /* Vulkan and OpenGL are backwards here because Vulkan assumes the D3D
   * convention in which framebuffer coordinates always start in the upper
   * left while OpenGL has framebuffer coordinates starting in the lower
   * left.  Therefore, we want the reverse of the hardware enum name.
   */
   ASSERTED static const uint16_t vk_to_nv9097[] = {
      [VK_FRONT_FACE_COUNTER_CLOCKWISE]   = NV9097_OGL_SET_FRONT_FACE_V_CCW,
      };
            uint32_t nv9097_face = 0x900 | (1 - vk_face);
   assert(nv9097_face == vk_to_nv9097[vk_face]);
      }
      static uint32_t
   vk_to_nv9097_provoking_vertex(VkProvokingVertexModeEXT vk_mode)
   {
      STATIC_ASSERT(VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT ==
         STATIC_ASSERT(VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT ==
            }
      static void
   nvk_flush_rs_state(struct nvk_cmd_buffer *cmd)
   {
               const struct vk_dynamic_graphics_state *dyn =
            if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_RASTERIZER_DISCARD_ENABLE))
            if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_CLIP_ENABLE) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_CLAMP_ENABLE)) {
   const bool z_clamp = dyn->rs.depth_clamp_enable;
   const bool z_clip = vk_rasterization_state_depth_clip_enable(&dyn->rs);
   P_IMMD(p, NVC397, SET_VIEWPORT_CLIP_CONTROL, {
      /* TODO: Fix pre-Volta
   *
   * This probably involves a few macros, one which stases viewport
   * min/maxDepth in scratch states and one which goes here and
   * emits either min/maxDepth or -/+INF as needed.
   */
   .min_z_zero_max_z_one = MIN_Z_ZERO_MAX_Z_ONE_FALSE,
   .z_clip_range = nvk_cmd_buffer_3d_cls(cmd) >= VOLTA_A
                                             .geometry_guardband = GEOMETRY_GUARDBAND_SCALE_256,
   .line_point_cull_guardband = LINE_POINT_CULL_GUARDBAND_SCALE_256,
                  /* We clip depth with the geometry clipper to ensure that it gets
   * clipped before depth bias is applied.  If we leave it up to the
   * raserizer clipper (pixel_min/max_z = CLIP), it will clip according
   * to the post-bias Z value which is wrong.  In order to always get
   * the geometry clipper, we need to set a tignt guardband
   * (geometry_guardband_z = SCALE_1).
   */
   .geometry_guardband_z = z_clip ? GEOMETRY_GUARDBAND_Z_SCALE_1
                  if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_POLYGON_MODE)) {
      uint32_t polygon_mode = vk_to_nv9097_polygon_mode(dyn->rs.polygon_mode);
   P_MTHD(p, NV9097, SET_FRONT_POLYGON_MODE);
   P_NV9097_SET_FRONT_POLYGON_MODE(p, polygon_mode);
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_CULL_MODE)) {
               if (dyn->rs.cull_mode != VK_CULL_MODE_NONE) {
      uint32_t face = vk_to_nv9097_cull_mode(dyn->rs.cull_mode);
                  if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_FRONT_FACE)) {
      P_IMMD(p, NV9097, OGL_SET_FRONT_FACE,
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_PROVOKING_VERTEX)) {
      P_IMMD(p, NV9097, SET_PROVOKING_VERTEX,
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_ENABLE)) {
      P_MTHD(p, NV9097, SET_POLY_OFFSET_POINT);
   P_NV9097_SET_POLY_OFFSET_POINT(p, dyn->rs.depth_bias.enable);
   P_NV9097_SET_POLY_OFFSET_LINE(p, dyn->rs.depth_bias.enable);
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_FACTORS)) {
      switch (dyn->rs.depth_bias.representation) {
   case VK_DEPTH_BIAS_REPRESENTATION_LEAST_REPRESENTABLE_VALUE_FORMAT_EXT:
      P_IMMD(p, NV9097, SET_DEPTH_BIAS_CONTROL,
            case VK_DEPTH_BIAS_REPRESENTATION_LEAST_REPRESENTABLE_VALUE_FORCE_UNORM_EXT:
      P_IMMD(p, NV9097, SET_DEPTH_BIAS_CONTROL,
            case VK_DEPTH_BIAS_REPRESENTATION_FLOAT_EXT:
   default:
         }
   /* TODO: The blob multiplies by 2 for some reason. We don't. */
   P_IMMD(p, NV9097, SET_DEPTH_BIAS, fui(dyn->rs.depth_bias.constant));
   P_IMMD(p, NV9097, SET_SLOPE_SCALE_DEPTH_BIAS, fui(dyn->rs.depth_bias.slope));
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_WIDTH)) {
      P_MTHD(p, NV9097, SET_LINE_WIDTH_FLOAT);
   P_NV9097_SET_LINE_WIDTH_FLOAT(p, fui(dyn->rs.line.width));
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_MODE)) {
      switch (dyn->rs.line.mode) {
   case VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT:
   case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT:
      P_IMMD(p, NV9097, SET_LINE_MULTISAMPLE_OVERRIDE, ENABLE_FALSE);
               case VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT:
      P_IMMD(p, NV9097, SET_LINE_MULTISAMPLE_OVERRIDE, ENABLE_TRUE);
               case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT:
      P_IMMD(p, NV9097, SET_LINE_MULTISAMPLE_OVERRIDE, ENABLE_TRUE);
               default:
                     if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_STIPPLE_ENABLE))
            if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_STIPPLE)) {
      /* map factor from [1,256] to [0, 255] */
   uint32_t stipple_factor = CLAMP(dyn->rs.line.stipple.factor, 1, 256) - 1;
   P_IMMD(p, NV9097, SET_LINE_STIPPLE_PARAMETERS, {
      .factor  = stipple_factor,
            }
      static VkSampleLocationEXT
   vk_sample_location(const struct vk_sample_locations_state *sl,
         {
      x = x % sl->grid_size.width;
               }
      static struct nvk_sample_location
   vk_to_nvk_sample_location(VkSampleLocationEXT loc)
   {
      return (struct nvk_sample_location) {
      .x_u4 = util_bitpack_ufixed_clamp(loc.x, 0, 3, 4),
         }
      static void
   nvk_flush_ms_state(struct nvk_cmd_buffer *cmd)
   {
      struct nvk_descriptor_state *desc = &cmd->state.gfx.descriptors;
   const struct vk_dynamic_graphics_state *dyn =
            if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_MS_SAMPLE_LOCATIONS) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_MS_SAMPLE_LOCATIONS_ENABLE)) {
   const struct vk_sample_locations_state *sl;
   if (dyn->ms.sample_locations_enable) {
         } else {
                  for (uint32_t i = 0; i < sl->per_pixel; i++) {
      desc->root.draw.sample_locations[i] =
               if (nvk_cmd_buffer_3d_cls(cmd) >= MAXWELL_B) {
      struct nvk_sample_location loc[16];
   for (uint32_t n = 0; n < ARRAY_SIZE(loc); n++) {
      const uint32_t s = n % sl->per_pixel;
   const uint32_t px = n / sl->per_pixel;
                                       P_MTHD(p, NVB197, SET_ANTI_ALIAS_SAMPLE_POSITIONS(0));
   for (uint32_t i = 0; i < 4; i++) {
      P_NVB197_SET_ANTI_ALIAS_SAMPLE_POSITIONS(p, i, {
      .x0 = loc[i * 4 + 0].x_u4,
   .y0 = loc[i * 4 + 0].y_u4,
   .x1 = loc[i * 4 + 1].x_u4,
   .y1 = loc[i * 4 + 1].y_u4,
   .x2 = loc[i * 4 + 2].x_u4,
   .y2 = loc[i * 4 + 2].y_u4,
   .x3 = loc[i * 4 + 3].x_u4,
                  }
      static uint32_t
   vk_to_nv9097_compare_op(VkCompareOp vk_op)
   {
      ASSERTED static const uint16_t vk_to_nv9097[] = {
      [VK_COMPARE_OP_NEVER]            = NV9097_SET_DEPTH_FUNC_V_OGL_NEVER,
   [VK_COMPARE_OP_LESS]             = NV9097_SET_DEPTH_FUNC_V_OGL_LESS,
   [VK_COMPARE_OP_EQUAL]            = NV9097_SET_DEPTH_FUNC_V_OGL_EQUAL,
   [VK_COMPARE_OP_LESS_OR_EQUAL]    = NV9097_SET_DEPTH_FUNC_V_OGL_LEQUAL,
   [VK_COMPARE_OP_GREATER]          = NV9097_SET_DEPTH_FUNC_V_OGL_GREATER,
   [VK_COMPARE_OP_NOT_EQUAL]        = NV9097_SET_DEPTH_FUNC_V_OGL_NOTEQUAL,
   [VK_COMPARE_OP_GREATER_OR_EQUAL] = NV9097_SET_DEPTH_FUNC_V_OGL_GEQUAL,
      };
            uint32_t nv9097_op = 0x200 | vk_op;
   assert(nv9097_op == vk_to_nv9097[vk_op]);
      }
      static uint32_t
   vk_to_nv9097_stencil_op(VkStencilOp vk_op)
   {
   #define OP(vk, nv) [VK_STENCIL_OP_##vk] = NV9097_SET_STENCIL_OP_FAIL_V_##nv
      ASSERTED static const uint16_t vk_to_nv9097[] = {
      OP(KEEP,                D3D_KEEP),
   OP(ZERO,                D3D_ZERO),
   OP(REPLACE,             D3D_REPLACE),
   OP(INCREMENT_AND_CLAMP, D3D_INCRSAT),
   OP(DECREMENT_AND_CLAMP, D3D_DECRSAT),
   OP(INVERT,              D3D_INVERT),
   OP(INCREMENT_AND_WRAP,  D3D_INCR),
      };
      #undef OP
         uint32_t nv9097_op = vk_op + 1;
   assert(nv9097_op == vk_to_nv9097[vk_op]);
      }
      static void
   nvk_flush_ds_state(struct nvk_cmd_buffer *cmd)
   {
               const struct nvk_rendering_state *render = &cmd->state.gfx.render;
   const struct vk_dynamic_graphics_state *dyn =
            if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_TEST_ENABLE)) {
      bool enable = dyn->ds.depth.test_enable &&
                     if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_WRITE_ENABLE)) {
      bool enable = dyn->ds.depth.write_enable &&
                     if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_COMPARE_OP)) {
      const uint32_t func = vk_to_nv9097_compare_op(dyn->ds.depth.compare_op);
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_BOUNDS_TEST_ENABLE)) {
      bool enable = dyn->ds.depth.bounds_test.enable &&
                     if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_BOUNDS_TEST_BOUNDS)) {
      P_MTHD(p, NV9097, SET_DEPTH_BOUNDS_MIN);
   P_NV9097_SET_DEPTH_BOUNDS_MIN(p, fui(dyn->ds.depth.bounds_test.min));
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_TEST_ENABLE)) {
      bool enable = dyn->ds.stencil.test_enable &&
                     const struct vk_stencil_test_face_state *front = &dyn->ds.stencil.front;
   const struct vk_stencil_test_face_state *back = &dyn->ds.stencil.back;
   if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_OP)) {
      P_MTHD(p, NV9097, SET_STENCIL_OP_FAIL);
   P_NV9097_SET_STENCIL_OP_FAIL(p, vk_to_nv9097_stencil_op(front->op.fail));
   P_NV9097_SET_STENCIL_OP_ZFAIL(p, vk_to_nv9097_stencil_op(front->op.depth_fail));
   P_NV9097_SET_STENCIL_OP_ZPASS(p, vk_to_nv9097_stencil_op(front->op.pass));
            P_MTHD(p, NV9097, SET_BACK_STENCIL_OP_FAIL);
   P_NV9097_SET_BACK_STENCIL_OP_FAIL(p, vk_to_nv9097_stencil_op(back->op.fail));
   P_NV9097_SET_BACK_STENCIL_OP_ZFAIL(p, vk_to_nv9097_stencil_op(back->op.depth_fail));
   P_NV9097_SET_BACK_STENCIL_OP_ZPASS(p, vk_to_nv9097_stencil_op(back->op.pass));
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_COMPARE_MASK)) {
      P_IMMD(p, NV9097, SET_STENCIL_FUNC_MASK, front->compare_mask);
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_WRITE_MASK)) {
      P_IMMD(p, NV9097, SET_STENCIL_MASK, front->write_mask);
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_REFERENCE)) {
      P_IMMD(p, NV9097, SET_STENCIL_FUNC_REF, front->reference);
         }
      static uint32_t
   vk_to_nv9097_logic_op(VkLogicOp vk_op)
   {
      ASSERTED uint16_t vk_to_nv9097[] = {
      [VK_LOGIC_OP_CLEAR]           = NV9097_SET_LOGIC_OP_FUNC_V_CLEAR,
   [VK_LOGIC_OP_AND]             = NV9097_SET_LOGIC_OP_FUNC_V_AND,
   [VK_LOGIC_OP_AND_REVERSE]     = NV9097_SET_LOGIC_OP_FUNC_V_AND_REVERSE,
   [VK_LOGIC_OP_COPY]            = NV9097_SET_LOGIC_OP_FUNC_V_COPY,
   [VK_LOGIC_OP_AND_INVERTED]    = NV9097_SET_LOGIC_OP_FUNC_V_AND_INVERTED,
   [VK_LOGIC_OP_NO_OP]           = NV9097_SET_LOGIC_OP_FUNC_V_NOOP,
   [VK_LOGIC_OP_XOR]             = NV9097_SET_LOGIC_OP_FUNC_V_XOR,
   [VK_LOGIC_OP_OR]              = NV9097_SET_LOGIC_OP_FUNC_V_OR,
   [VK_LOGIC_OP_NOR]             = NV9097_SET_LOGIC_OP_FUNC_V_NOR,
   [VK_LOGIC_OP_EQUIVALENT]      = NV9097_SET_LOGIC_OP_FUNC_V_EQUIV,
   [VK_LOGIC_OP_INVERT]          = NV9097_SET_LOGIC_OP_FUNC_V_INVERT,
   [VK_LOGIC_OP_OR_REVERSE]      = NV9097_SET_LOGIC_OP_FUNC_V_OR_REVERSE,
   [VK_LOGIC_OP_COPY_INVERTED]   = NV9097_SET_LOGIC_OP_FUNC_V_COPY_INVERTED,
   [VK_LOGIC_OP_OR_INVERTED]     = NV9097_SET_LOGIC_OP_FUNC_V_OR_INVERTED,
   [VK_LOGIC_OP_NAND]            = NV9097_SET_LOGIC_OP_FUNC_V_NAND,
      };
            uint32_t nv9097_op = 0x1500 | vk_op;
   assert(nv9097_op == vk_to_nv9097[vk_op]);
      }
      static void
   nvk_flush_cb_state(struct nvk_cmd_buffer *cmd)
   {
               const struct vk_dynamic_graphics_state *dyn =
            if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_LOGIC_OP_ENABLE))
            if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_LOGIC_OP)) {
      const uint32_t func = vk_to_nv9097_logic_op(dyn->cb.logic_op);
                        if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_BLEND_CONSTANTS)) {
      P_MTHD(p, NV9097, SET_BLEND_CONST_RED);
   P_NV9097_SET_BLEND_CONST_RED(p,     fui(dyn->cb.blend_constants[0]));
   P_NV9097_SET_BLEND_CONST_GREEN(p,   fui(dyn->cb.blend_constants[1]));
   P_NV9097_SET_BLEND_CONST_BLUE(p,    fui(dyn->cb.blend_constants[2]));
         }
      static void
   nvk_flush_dynamic_state(struct nvk_cmd_buffer *cmd)
   {
      struct vk_dynamic_graphics_state *dyn =
            if (!vk_dynamic_graphics_state_any_dirty(dyn))
            nvk_flush_vi_state(cmd);
   nvk_flush_ia_state(cmd);
   nvk_flush_ts_state(cmd);
   nvk_flush_vp_state(cmd);
                     nvk_flush_ms_state(cmd);
   nvk_flush_ds_state(cmd);
               }
      static void
   nvk_flush_descriptors(struct nvk_cmd_buffer *cmd)
   {
      struct nvk_descriptor_state *desc = &cmd->state.gfx.descriptors;
                     /* pre Pascal the constant buffer sizes need to be 0x100 aligned. As we
   * simply allocated a buffer and upload data to it, make sure its size is
   * 0x100 aligned.
   */
            void *root_desc_map;
   uint64_t root_desc_addr;
   result = nvk_cmd_buffer_upload_alloc(cmd, sizeof(desc->root), 0x100,
         if (unlikely(result != VK_SUCCESS)) {
      vk_command_buffer_set_error(&cmd->vk, result);
               desc->root.root_desc_addr = root_desc_addr;
                     P_MTHD(p, NV9097, SET_CONSTANT_BUFFER_SELECTOR_A);
   P_NV9097_SET_CONSTANT_BUFFER_SELECTOR_A(p, sizeof(desc->root));
   P_NV9097_SET_CONSTANT_BUFFER_SELECTOR_B(p, root_desc_addr >> 32);
            for (uint32_t i = 0; i < 5; i++) {
      P_IMMD(p, NV9097, BIND_GROUP_CONSTANT_BUFFER(i), {
      .valid = VALID_TRUE,
      });
   P_IMMD(p, NV9097, BIND_GROUP_CONSTANT_BUFFER(i), {
      .valid = VALID_TRUE,
                  assert(nvk_cmd_buffer_3d_cls(cmd) >= KEPLER_A);
   P_IMMD(p, NVA097, INVALIDATE_SHADER_CACHES_NO_WFI, {
            }
      static void
   nvk_flush_gfx_state(struct nvk_cmd_buffer *cmd)
   {
      nvk_flush_dynamic_state(cmd);
      }
      static uint32_t
   vk_to_nv_index_format(VkIndexType type)
   {
      switch (type) {
   case VK_INDEX_TYPE_UINT16:
         case VK_INDEX_TYPE_UINT32:
         case VK_INDEX_TYPE_UINT8_EXT:
         default:
            }
      static uint32_t
   vk_index_to_restart(VkIndexType index_type)
   {
      switch (index_type) {
   case VK_INDEX_TYPE_UINT16:
         case VK_INDEX_TYPE_UINT32:
         case VK_INDEX_TYPE_UINT8_EXT:
         default:
            }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdBindIndexBuffer2KHR(VkCommandBuffer commandBuffer,
                           {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
                     uint64_t addr, range;
   if (buffer != NULL && size > 0) {
      addr = nvk_buffer_address(buffer, offset);
      } else {
                  P_IMMD(p, NV9097, SET_DA_PRIMITIVE_RESTART_INDEX,
            P_MTHD(p, NV9097, SET_INDEX_BUFFER_A);
   P_NV9097_SET_INDEX_BUFFER_A(p, addr >> 32);
            if (nvk_cmd_buffer_3d_cls(cmd) >= TURING_A) {
      P_MTHD(p, NVC597, SET_INDEX_BUFFER_SIZE_A);
   P_NVC597_SET_INDEX_BUFFER_SIZE_A(p, range >> 32);
      } else {
      /* TODO: What about robust zero-size buffers? */
   const uint64_t limit = range > 0 ? addr + range - 1 : 0;
   P_MTHD(p, NV9097, SET_INDEX_BUFFER_C);
   P_NV9097_SET_INDEX_BUFFER_C(p, limit >> 32);
                  }
      void
   nvk_cmd_bind_vertex_buffer(struct nvk_cmd_buffer *cmd, uint32_t vb_idx,
         {
      /* Used for meta save/restore */
   if (vb_idx == 0)
                     P_MTHD(p, NV9097, SET_VERTEX_STREAM_A_LOCATION_A(vb_idx));
   P_NV9097_SET_VERTEX_STREAM_A_LOCATION_A(p, vb_idx, addr_range.addr >> 32);
            if (nvk_cmd_buffer_3d_cls(cmd) >= TURING_A) {
      P_MTHD(p, NVC597, SET_VERTEX_STREAM_SIZE_A(vb_idx));
   P_NVC597_SET_VERTEX_STREAM_SIZE_A(p, vb_idx, addr_range.range >> 32);
      } else {
      /* TODO: What about robust zero-size buffers? */
   const uint64_t limit = addr_range.range > 0 ?
         P_MTHD(p, NV9097, SET_VERTEX_STREAM_LIMIT_A_A(vb_idx));
   P_NV9097_SET_VERTEX_STREAM_LIMIT_A_A(p, vb_idx, limit >> 32);
         }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdBindVertexBuffers2(VkCommandBuffer commandBuffer,
                           uint32_t firstBinding,
   uint32_t bindingCount,
   {
               if (pStrides) {
      vk_cmd_set_vertex_binding_strides(&cmd->vk, firstBinding,
               for (uint32_t i = 0; i < bindingCount; i++) {
      VK_FROM_HANDLE(nvk_buffer, buffer, pBuffers[i]);
            uint64_t size = pSizes ? pSizes[i] : VK_WHOLE_SIZE;
   const struct nvk_addr_range addr_range =
                  }
      static uint32_t
   vk_to_nv9097_primitive_topology(VkPrimitiveTopology prim)
   {
      switch (prim) {
   case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
         case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
         case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
            #pragma GCC diagnostic push
   #pragma GCC diagnostic ignored "-Wswitch"
         #pragma GCC diagnostic pop
            case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
         case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
         case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
         case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
         default:
            }
      struct mme_draw_params {
      struct mme_value base_vertex;
   struct mme_value first_vertex;
   struct mme_value first_instance;
      };
      static void
   nvk_mme_build_set_draw_params(struct mme_builder *b,
         {
      const uint32_t draw_params_offset = nvk_root_descriptor_offset(draw);
   mme_mthd(b, NV9097_LOAD_CONSTANT_BUFFER_OFFSET);
   mme_emit(b, mme_imm(draw_params_offset));
   mme_mthd(b, NV9097_LOAD_CONSTANT_BUFFER(0));
   mme_emit(b, p->first_vertex);
   mme_emit(b, p->first_instance);
   mme_emit(b, p->draw_idx);
            mme_mthd(b, NV9097_SET_GLOBAL_BASE_VERTEX_INDEX);
   mme_emit(b, p->base_vertex);
   mme_mthd(b, NV9097_SET_VERTEX_ID_BASE);
            mme_mthd(b, NV9097_SET_GLOBAL_BASE_INSTANCE_INDEX);
      }
      static void
   nvk_mme_emit_view_index(struct mme_builder *b, struct mme_value view_index)
   {
      /* Set the push constant */
   mme_mthd(b, NV9097_LOAD_CONSTANT_BUFFER_OFFSET);
   mme_emit(b, mme_imm(nvk_root_descriptor_offset(draw.view_index)));
   mme_mthd(b, NV9097_LOAD_CONSTANT_BUFFER(0));
            /* Set the layer to the view index */
   STATIC_ASSERT(DRF_LO(NV9097_SET_RT_LAYER_V) == 0);
   STATIC_ASSERT(NV9097_SET_RT_LAYER_CONTROL_V_SELECTS_LAYER == 0);
   mme_mthd(b, NV9097_SET_RT_LAYER);
      }
      static void
   nvk_mme_build_draw_loop(struct mme_builder *b,
                     {
               mme_loop(b, instance_count) {
      mme_mthd(b, NV9097_BEGIN);
            mme_mthd(b, NV9097_SET_VERTEX_ARRAY_START);
   mme_emit(b, first_vertex);
            mme_mthd(b, NV9097_END);
                           }
      static void
   nvk_mme_build_draw(struct mme_builder *b,
         {
      /* These are in VkDrawIndirectCommand order */
   struct mme_value vertex_count = mme_load(b);
   struct mme_value instance_count = mme_load(b);
   struct mme_value first_vertex = mme_load(b);
            struct mme_draw_params params = {
      .first_vertex = first_vertex,
   .first_instance = first_instance,
      };
                     if (b->devinfo->cls_eng3d < TURING_A)
            struct mme_value view_mask = nvk_mme_load_scratch(b, VIEW_MASK);
   mme_if(b, ieq, view_mask, mme_zero()) {
               nvk_mme_build_draw_loop(b, instance_count,
               view_mask = nvk_mme_load_scratch(b, VIEW_MASK);
   mme_if(b, ine, view_mask, mme_zero()) {
               struct mme_value view = mme_mov(b, mme_zero());
   mme_while(b, ine, view, mme_imm(32)) {
      view_mask = nvk_mme_load_scratch(b, VIEW_MASK);
   struct mme_value has_view = mme_bfe(b, view_mask, view, 1);
   mme_free_reg(b, view_mask);
   mme_if(b, ine, has_view, mme_zero()) {
      mme_free_reg(b, has_view);
   nvk_mme_emit_view_index(b, view);
   nvk_mme_build_draw_loop(b, instance_count,
                  }
               mme_free_reg(b, instance_count);
   mme_free_reg(b, first_vertex);
            if (b->devinfo->cls_eng3d < TURING_A)
      }
      void
   nvk_mme_draw(struct mme_builder *b)
   {
                  }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdDraw(VkCommandBuffer commandBuffer,
               uint32_t vertexCount,
   uint32_t instanceCount,
   {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
   const struct vk_dynamic_graphics_state *dyn =
                     uint32_t begin;
   V_NV9097_BEGIN(begin, {
      .op = vk_to_nv9097_primitive_topology(dyn->ia.primitive_topology),
   .primitive_id = NV9097_BEGIN_PRIMITIVE_ID_FIRST,
   .instance_id = NV9097_BEGIN_INSTANCE_ID_FIRST,
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 6);
   P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_DRAW));
   P_INLINE_DATA(p, begin);
   P_INLINE_DATA(p, vertexCount);
   P_INLINE_DATA(p, instanceCount);
   P_INLINE_DATA(p, firstVertex);
      }
      static void
   nvk_mme_build_draw_indexed_loop(struct mme_builder *b,
                     {
               mme_loop(b, instance_count) {
      mme_mthd(b, NV9097_BEGIN);
            mme_mthd(b, NV9097_SET_INDEX_BUFFER_F);
   mme_emit(b, first_index);
            mme_mthd(b, NV9097_END);
                           }
      static void
   nvk_mme_build_draw_indexed(struct mme_builder *b,
         {
      /* These are in VkDrawIndexedIndirectCommand order */
   struct mme_value index_count = mme_load(b);
   struct mme_value instance_count = mme_load(b);
   struct mme_value first_index = mme_load(b);
   struct mme_value vertex_offset = mme_load(b);
            struct mme_draw_params params = {
      .base_vertex = vertex_offset,
   .first_vertex = vertex_offset,
   .first_instance = first_instance,
      };
            mme_free_reg(b, vertex_offset);
            if (b->devinfo->cls_eng3d < TURING_A)
            struct mme_value view_mask = nvk_mme_load_scratch(b, VIEW_MASK);
   mme_if(b, ieq, view_mask, mme_zero()) {
               nvk_mme_build_draw_indexed_loop(b, instance_count,
               view_mask = nvk_mme_load_scratch(b, VIEW_MASK);
   mme_if(b, ine, view_mask, mme_zero()) {
               struct mme_value view = mme_mov(b, mme_zero());
   mme_while(b, ine, view, mme_imm(32)) {
      view_mask = nvk_mme_load_scratch(b, VIEW_MASK);
   struct mme_value has_view = mme_bfe(b, view_mask, view, 1);
   mme_free_reg(b, view_mask);
   mme_if(b, ine, has_view, mme_zero()) {
      mme_free_reg(b, has_view);
   nvk_mme_emit_view_index(b, view);
   nvk_mme_build_draw_indexed_loop(b, instance_count,
                  }
               mme_free_reg(b, instance_count);
   mme_free_reg(b, first_index);
            if (b->devinfo->cls_eng3d < TURING_A)
      }
      void
   nvk_mme_draw_indexed(struct mme_builder *b)
   {
                  }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdDrawIndexed(VkCommandBuffer commandBuffer,
                     uint32_t indexCount,
   uint32_t instanceCount,
   {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
   const struct vk_dynamic_graphics_state *dyn =
                     uint32_t begin;
   V_NV9097_BEGIN(begin, {
      .op = vk_to_nv9097_primitive_topology(dyn->ia.primitive_topology),
   .primitive_id = NV9097_BEGIN_PRIMITIVE_ID_FIRST,
   .instance_id = NV9097_BEGIN_INSTANCE_ID_FIRST,
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 7);
   P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_DRAW_INDEXED));
   P_INLINE_DATA(p, begin);
   P_INLINE_DATA(p, indexCount);
   P_INLINE_DATA(p, instanceCount);
   P_INLINE_DATA(p, firstIndex);
   P_INLINE_DATA(p, vertexOffset);
      }
      void
   nvk_mme_draw_indirect(struct mme_builder *b)
   {
               if (b->devinfo->cls_eng3d >= TURING_A) {
      struct mme_value64 draw_addr = mme_load_addr64(b);
   struct mme_value draw_count = mme_load(b);
            struct mme_value draw = mme_mov(b, mme_zero());
   mme_while(b, ult, draw, draw_count) {
                        mme_add_to(b, draw, draw, mme_imm(1));
         } else {
      struct mme_value draw_count = mme_load(b);
            struct mme_value draw = mme_mov(b, mme_zero());
   mme_while(b, ine, draw, draw_count) {
                              struct mme_value pad_dw = nvk_mme_load_scratch(b, DRAW_PAD_DW);
   mme_loop(b, pad_dw) {
                                 }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdDrawIndirect(VkCommandBuffer commandBuffer,
                     VkBuffer _buffer,
   {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
   VK_FROM_HANDLE(nvk_buffer, buffer, _buffer);
   const struct vk_dynamic_graphics_state *dyn =
            /* From the Vulkan 1.3.238 spec:
   *
   *    VUID-vkCmdDrawIndirect-drawCount-00476
   *
   *    "If drawCount is greater than 1, stride must be a multiple of 4 and
   *    must be greater than or equal to sizeof(VkDrawIndirectCommand)"
   *
   * and
   *
   *    "If drawCount is less than or equal to one, stride is ignored."
   */
   if (drawCount > 1) {
      assert(stride % 4 == 0);
      } else {
                           uint32_t begin;
   V_NV9097_BEGIN(begin, {
      .op = vk_to_nv9097_primitive_topology(dyn->ia.primitive_topology),
   .primitive_id = NV9097_BEGIN_PRIMITIVE_ID_FIRST,
   .instance_id = NV9097_BEGIN_INSTANCE_ID_FIRST,
               if (nvk_cmd_buffer_3d_cls(cmd) >= TURING_A) {
      struct nv_push *p = nvk_cmd_buffer_push(cmd, 10);
   P_IMMD(p, NVC597, MME_DMA_SYSMEMBAR, 0);
   P_IMMD(p, NVC597, SET_MME_DATA_FIFO_CONFIG, FIFO_SIZE_SIZE_4KB);
   P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_DRAW_INDIRECT));
   P_INLINE_DATA(p, begin);
   uint64_t draw_addr = nvk_buffer_address(buffer, offset);
   P_INLINE_DATA(p, draw_addr >> 32);
   P_INLINE_DATA(p, draw_addr);
   P_INLINE_DATA(p, drawCount);
      } else {
      /* Stall the command streamer */
   struct nv_push *p = nvk_cmd_buffer_push(cmd, 2);
            const uint32_t max_draws_per_push =
         while (drawCount) {
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 4);
   P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_DRAW_INDIRECT));
   P_INLINE_DATA(p, begin);
                  uint64_t range = count * (uint64_t)stride;
                  offset += range;
            }
      void
   nvk_mme_draw_indexed_indirect(struct mme_builder *b)
   {
               if (b->devinfo->cls_eng3d >= TURING_A) {
      struct mme_value64 draw_addr = mme_load_addr64(b);
   struct mme_value draw_count = mme_load(b);
            struct mme_value draw = mme_mov(b, mme_zero());
   mme_while(b, ult, draw, draw_count) {
                        mme_add_to(b, draw, draw, mme_imm(1));
         } else {
      struct mme_value draw_count = mme_load(b);
            struct mme_value draw = mme_mov(b, mme_zero());
   mme_while(b, ine, draw, draw_count) {
                              struct mme_value pad_dw = nvk_mme_load_scratch(b, DRAW_PAD_DW);
   mme_loop(b, pad_dw) {
                                 }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer,
                           {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
   VK_FROM_HANDLE(nvk_buffer, buffer, _buffer);
   const struct vk_dynamic_graphics_state *dyn =
            /* From the Vulkan 1.3.238 spec:
   *
   *    VUID-vkCmdDrawIndexedIndirect-drawCount-00528
   *
   *    "If drawCount is greater than 1, stride must be a multiple of 4 and
   *    must be greater than or equal to sizeof(VkDrawIndexedIndirectCommand)"
   *
   * and
   *
   *    "If drawCount is less than or equal to one, stride is ignored."
   */
   if (drawCount > 1) {
      assert(stride % 4 == 0);
      } else {
                           uint32_t begin;
   V_NV9097_BEGIN(begin, {
      .op = vk_to_nv9097_primitive_topology(dyn->ia.primitive_topology),
   .primitive_id = NV9097_BEGIN_PRIMITIVE_ID_FIRST,
   .instance_id = NV9097_BEGIN_INSTANCE_ID_FIRST,
               if (nvk_cmd_buffer_3d_cls(cmd) >= TURING_A) {
      struct nv_push *p = nvk_cmd_buffer_push(cmd, 10);
   P_IMMD(p, NVC597, MME_DMA_SYSMEMBAR, 0);
   P_IMMD(p, NVC597, SET_MME_DATA_FIFO_CONFIG, FIFO_SIZE_SIZE_4KB);
   P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_DRAW_INDEXED_INDIRECT));
   P_INLINE_DATA(p, begin);
   uint64_t draw_addr = nvk_buffer_address(buffer, offset);
   P_INLINE_DATA(p, draw_addr >> 32);
   P_INLINE_DATA(p, draw_addr);
   P_INLINE_DATA(p, drawCount);
      } else {
      /* Stall the command streamer */
   struct nv_push *p = nvk_cmd_buffer_push(cmd, 2);
            const uint32_t max_draws_per_push =
         while (drawCount) {
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 4);
   P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_DRAW_INDEXED_INDIRECT));
   P_INLINE_DATA(p, begin);
                  uint64_t range = count * (uint64_t)stride;
                  offset += range;
            }
      void
   nvk_mme_draw_indirect_count(struct mme_builder *b)
   {
      if (b->devinfo->cls_eng3d < TURING_A)
                     struct mme_value64 draw_addr = mme_load_addr64(b);
   struct mme_value64 draw_count_addr = mme_load_addr64(b);
   struct mme_value draw_max = mme_load(b);
            mme_tu104_read_fifoed(b, draw_count_addr, mme_imm(1));
   mme_free_reg64(b, draw_count_addr);
            mme_if(b, ule, draw_count_buf, draw_max) {
         }
            struct mme_value draw = mme_mov(b, mme_zero());
   mme_while(b, ult, draw, draw_max) {
                        mme_add_to(b, draw, draw, mme_imm(1));
         }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdDrawIndirectCount(VkCommandBuffer commandBuffer,
                           VkBuffer _buffer,
   VkDeviceSize offset,
   {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
   VK_FROM_HANDLE(nvk_buffer, buffer, _buffer);
            const struct vk_dynamic_graphics_state *dyn =
            /* TODO: Indirect count draw pre-Turing */
                     uint32_t begin;
   V_NV9097_BEGIN(begin, {
      .op = vk_to_nv9097_primitive_topology(dyn->ia.primitive_topology),
   .primitive_id = NV9097_BEGIN_PRIMITIVE_ID_FIRST,
   .instance_id = NV9097_BEGIN_INSTANCE_ID_FIRST,
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 12);
   P_IMMD(p, NVC597, MME_DMA_SYSMEMBAR, 0);
   P_IMMD(p, NVC597, SET_MME_DATA_FIFO_CONFIG, FIFO_SIZE_SIZE_4KB);
   P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_DRAW_INDIRECT_COUNT));
   P_INLINE_DATA(p, begin);
   uint64_t draw_addr = nvk_buffer_address(buffer, offset);
   P_INLINE_DATA(p, draw_addr >> 32);
   P_INLINE_DATA(p, draw_addr);
   uint64_t draw_count_addr = nvk_buffer_address(count_buffer,
         P_INLINE_DATA(p, draw_count_addr >> 32);
   P_INLINE_DATA(p, draw_count_addr);
   P_INLINE_DATA(p, maxDrawCount);
      }
      void
   nvk_mme_draw_indexed_indirect_count(struct mme_builder *b)
   {
      if (b->devinfo->cls_eng3d < TURING_A)
                     struct mme_value64 draw_addr = mme_load_addr64(b);
   struct mme_value64 draw_count_addr = mme_load_addr64(b);
   struct mme_value draw_max = mme_load(b);
            mme_tu104_read_fifoed(b, draw_count_addr, mme_imm(1));
   mme_free_reg64(b, draw_count_addr);
            mme_if(b, ule, draw_count_buf, draw_max) {
         }
            struct mme_value draw = mme_mov(b, mme_zero());
   mme_while(b, ult, draw, draw_max) {
                        mme_add_to(b, draw, draw, mme_imm(1));
         }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer,
                                 VkBuffer _buffer,
   {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
   VK_FROM_HANDLE(nvk_buffer, buffer, _buffer);
            const struct vk_dynamic_graphics_state *dyn =
            /* TODO: Indexed indirect count draw pre-Turing */
                     uint32_t begin;
   V_NV9097_BEGIN(begin, {
      .op = vk_to_nv9097_primitive_topology(dyn->ia.primitive_topology),
   .primitive_id = NV9097_BEGIN_PRIMITIVE_ID_FIRST,
   .instance_id = NV9097_BEGIN_INSTANCE_ID_FIRST,
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 12);
   P_IMMD(p, NVC597, MME_DMA_SYSMEMBAR, 0);
   P_IMMD(p, NVC597, SET_MME_DATA_FIFO_CONFIG, FIFO_SIZE_SIZE_4KB);
   P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_DRAW_INDEXED_INDIRECT_COUNT));
   P_INLINE_DATA(p, begin);
   uint64_t draw_addr = nvk_buffer_address(buffer, offset);
   P_INLINE_DATA(p, draw_addr >> 32);
   P_INLINE_DATA(p, draw_addr);
   uint64_t draw_count_addr = nvk_buffer_address(count_buffer,
         P_INLINE_DATA(p, draw_count_addr >> 32);
   P_INLINE_DATA(p, draw_count_addr);
   P_INLINE_DATA(p, maxDrawCount);
      }
      static void
   nvk_mme_xfb_draw_indirect_loop(struct mme_builder *b,
               {
               mme_loop(b, instance_count) {
      mme_mthd(b, NV9097_BEGIN);
            mme_mthd(b, NV9097_DRAW_AUTO);
            mme_mthd(b, NV9097_END);
                           }
      void
   nvk_mme_xfb_draw_indirect(struct mme_builder *b)
   {
               struct mme_value instance_count = mme_load(b);
            if (b->devinfo->cls_eng3d >= TURING_A) {
      struct mme_value64 counter_addr = mme_load_addr64(b);
   mme_tu104_read_fifoed(b, counter_addr, mme_imm(1));
   mme_free_reg(b, counter_addr.lo);
      }
            struct mme_draw_params params = {
         };
                     struct mme_value view_mask = nvk_mme_load_scratch(b, VIEW_MASK);
   mme_if(b, ieq, view_mask, mme_zero()) {
                           view_mask = nvk_mme_load_scratch(b, VIEW_MASK);
   mme_if(b, ine, view_mask, mme_zero()) {
               struct mme_value view = mme_mov(b, mme_zero());
   mme_while(b, ine, view, mme_imm(32)) {
      view_mask = nvk_mme_load_scratch(b, VIEW_MASK);
   struct mme_value has_view = mme_bfe(b, view_mask, view, 1);
   mme_free_reg(b, view_mask);
   mme_if(b, ine, has_view, mme_zero()) {
      mme_free_reg(b, has_view);
   nvk_mme_emit_view_index(b, view);
                              mme_free_reg(b, instance_count);
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdDrawIndirectByteCountEXT(VkCommandBuffer commandBuffer,
                                 uint32_t instanceCount,
   {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
   VK_FROM_HANDLE(nvk_buffer, counter_buffer, counterBuffer);
   const struct vk_dynamic_graphics_state *dyn =
                     uint32_t begin;
   V_NV9097_BEGIN(begin, {
      .op = vk_to_nv9097_primitive_topology(dyn->ia.primitive_topology),
   .primitive_id = NV9097_BEGIN_PRIMITIVE_ID_FIRST,
   .instance_id = NV9097_BEGIN_INSTANCE_ID_FIRST,
               if (nvk_cmd_buffer_3d_cls(cmd) >= TURING_A) {
      struct nv_push *p = nvk_cmd_buffer_push(cmd, 14);
   P_IMMD(p, NV9097, SET_DRAW_AUTO_START, counterOffset);
   P_IMMD(p, NV9097, SET_DRAW_AUTO_STRIDE, vertexStride);
   P_IMMD(p, NVC597, MME_DMA_SYSMEMBAR, 0);
            P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_XFB_DRAW_INDIRECT));
   P_INLINE_DATA(p, begin);
   P_INLINE_DATA(p, instanceCount);
   P_INLINE_DATA(p, firstInstance);
   uint64_t counter_addr = nvk_buffer_address(counter_buffer,
         P_INLINE_DATA(p, counter_addr >> 32);
      } else {
      struct nv_push *p = nvk_cmd_buffer_push(cmd, 11);
   /* Stall the command streamer */
   __push_immd(p, SUBC_NV9097, NV906F_SET_REFERENCE, 0);
   P_IMMD(p, NV9097, SET_DRAW_AUTO_START, counterOffset);
            P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_XFB_DRAW_INDIRECT));
   P_INLINE_DATA(p, begin);
   P_INLINE_DATA(p, instanceCount);
   P_INLINE_DATA(p, firstInstance);
   nv_push_update_count(p, 1);
   nvk_cmd_buffer_push_indirect_buffer(cmd, counter_buffer,
         }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdBeginConditionalRenderingEXT(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
            uint64_t addr = nvk_buffer_address(buffer, pConditionalRenderingBegin->offset);
   bool inverted = pConditionalRenderingBegin->flags &
                     if (addr & 0x3f || buffer->is_local) {
      uint64_t tmp_addr;
   VkResult result = nvk_cmd_buffer_cond_render_alloc(cmd, &tmp_addr);
   if (result != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd->vk, result);
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 12);
   P_MTHD(p, NV90B5, OFFSET_IN_UPPER);
   P_NV90B5_OFFSET_IN_UPPER(p, addr >> 32);
   P_NV90B5_OFFSET_IN_LOWER(p, addr & 0xffffffff);
   P_NV90B5_OFFSET_OUT_UPPER(p, tmp_addr >> 32);
   P_NV90B5_OFFSET_OUT_LOWER(p, tmp_addr & 0xffffffff);
   P_NV90B5_PITCH_IN(p, 4);
   P_NV90B5_PITCH_OUT(p, 4);
   P_NV90B5_LINE_LENGTH_IN(p, 4);
            P_IMMD(p, NV90B5, LAUNCH_DMA, {
         .data_transfer_type = DATA_TRANSFER_TYPE_PIPELINED,
   .multi_line_enable = MULTI_LINE_ENABLE_TRUE,
   .flush_enable = FLUSH_ENABLE_TRUE,
   .src_memory_layout = SRC_MEMORY_LAYOUT_PITCH,
                     struct nv_push *p = nvk_cmd_buffer_push(cmd, 12);
   P_MTHD(p, NV9097, SET_RENDER_ENABLE_A);
   P_NV9097_SET_RENDER_ENABLE_A(p, addr >> 32);
   P_NV9097_SET_RENDER_ENABLE_B(p, addr & 0xfffffff0);
            P_MTHD(p, NV90C0, SET_RENDER_ENABLE_A);
   P_NV90C0_SET_RENDER_ENABLE_A(p, addr >> 32);
   P_NV90C0_SET_RENDER_ENABLE_B(p, addr & 0xfffffff0);
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdEndConditionalRenderingEXT(VkCommandBuffer commandBuffer)
   {
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 12);
   P_MTHD(p, NV9097, SET_RENDER_ENABLE_A);
   P_NV9097_SET_RENDER_ENABLE_A(p, 0);
   P_NV9097_SET_RENDER_ENABLE_B(p, 0);
            P_MTHD(p, NV90C0, SET_RENDER_ENABLE_A);
   P_NV90C0_SET_RENDER_ENABLE_A(p, 0);
   P_NV90C0_SET_RENDER_ENABLE_B(p, 0);
      }
