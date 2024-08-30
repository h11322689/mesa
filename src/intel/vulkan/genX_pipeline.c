   /*
   * Copyright © 2015 Intel Corporation
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
      #include "anv_private.h"
      #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
   #include "genxml/genX_rt_pack.h"
      #include "common/intel_genX_state.h"
   #include "common/intel_l3_config.h"
   #include "common/intel_sample_positions.h"
   #include "nir/nir_xfb_info.h"
   #include "vk_util.h"
   #include "vk_format.h"
   #include "vk_log.h"
   #include "vk_render_pass.h"
      static inline struct anv_batch *
   anv_gfx_pipeline_add(struct anv_graphics_pipeline *pipeline,
               {
               assert(ptr->len == 0 ||
         if (ptr->len == 0)
                     }
      #define anv_pipeline_emit(pipeline, state, cmd, name)                   \
      for (struct cmd name = { __anv_cmd_header(cmd) },                    \
         *_dst = anv_batch_emit_dwords(                               \
      anv_gfx_pipeline_add(pipeline,                            \
               __builtin_expect(_dst != NULL, 1);                              \
   ({ __anv_cmd_pack(cmd)(&(pipeline)->base.base.batch,            \
            VG(VALGRIND_CHECK_MEM_IS_DEFINED(_dst, __anv_cmd_length(cmd) * 4)); \
         #define anv_pipeline_emitn(pipeline, state, n, cmd, ...) ({             \
      void *__dst = anv_batch_emit_dwords(                                 \
         if (__dst) {                                                         \
      struct cmd __template = {                                         \
      __anv_cmd_header(cmd),                                         \
   .DWordLength = n - __anv_cmd_length_bias(cmd),                 \
      };                                                                \
   __anv_cmd_pack(cmd)(&pipeline->base.base.batch,                   \
      }                                                                    \
   __dst;                                                               \
            static uint32_t
   vertex_element_comp_control(enum isl_format format, unsigned comp)
   {
      uint8_t bits;
   switch (comp) {
   case 0: bits = isl_format_layouts[format].channels.r.bits; break;
   case 1: bits = isl_format_layouts[format].channels.g.bits; break;
   case 2: bits = isl_format_layouts[format].channels.b.bits; break;
   case 3: bits = isl_format_layouts[format].channels.a.bits; break;
   default: unreachable("Invalid component");
            /*
   * Take in account hardware restrictions when dealing with 64-bit floats.
   *
   * From Broadwell spec, command reference structures, page 586:
   *  "When SourceElementFormat is set to one of the *64*_PASSTHRU formats,
   *   64-bit components are stored * in the URB without any conversion. In
   *   this case, vertex elements must be written as 128 or 256 bits, with
   *   VFCOMP_STORE_0 being used to pad the output as required. E.g., if
   *   R64_PASSTHRU is used to copy a 64-bit Red component into the URB,
   *   Component 1 must be specified as VFCOMP_STORE_0 (with Components 2,3
   *   set to VFCOMP_NOSTORE) in order to output a 128-bit vertex element, or
   *   Components 1-3 must be specified as VFCOMP_STORE_0 in order to output
   *   a 256-bit vertex element. Likewise, use of R64G64B64_PASSTHRU requires
   *   Component 3 to be specified as VFCOMP_STORE_0 in order to output a
   *   256-bit vertex element."
   */
   if (bits) {
         } else if (comp >= 2 &&
            !isl_format_layouts[format].channels.b.bits &&
   /* When emitting 64-bit attributes, we need to write either 128 or 256
   * bit chunks, using VFCOMP_NOSTORE when not writing the chunk, and
   * VFCOMP_STORE_0 to pad the written chunk */
      } else if (comp < 3 ||
            /* Note we need to pad with value 0, not 1, due hardware restrictions
   * (see comment above) */
      } else if (isl_format_layouts[format].channels.r.type == ISL_UINT ||
            assert(comp == 3);
      } else {
      assert(comp == 3);
         }
      void
   genX(emit_vertex_input)(struct anv_batch *batch,
                           {
      const struct anv_device *device = pipeline->base.base.device;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);
   const uint64_t inputs_read = vs_prog_data->inputs_read;
   const uint64_t double_inputs_read =
         assert((inputs_read & ((1 << VERT_ATTRIB_GENERIC0) - 1)) == 0);
   const uint32_t elements = inputs_read >> VERT_ATTRIB_GENERIC0;
            for (uint32_t i = 0; i < pipeline->vs_input_elements; i++) {
      /* The SKL docs for VERTEX_ELEMENT_STATE say:
   *
   *    "All elements must be valid from Element[0] to the last valid
   *    element. (I.e. if Element[2] is valid then Element[1] and
   *    Element[0] must also be valid)."
   *
   * The SKL docs for 3D_Vertex_Component_Control say:
   *
   *    "Don't store this component. (Not valid for Component 0, but can
   *    be used for Component 1-3)."
   *
   * So we can't just leave a vertex element blank and hope for the best.
   * We have to tell the VF hardware to put something in it; so we just
   * store a bunch of zero.
   *
   * TODO: Compact vertex elements so we never end up with holes.
   */
   struct GENX(VERTEX_ELEMENT_STATE) element = {
      .Valid = true,
   .Component0Control = VFCOMP_STORE_0,
   .Component1Control = VFCOMP_STORE_0,
   .Component2Control = VFCOMP_STORE_0,
      };
   GENX(VERTEX_ELEMENT_STATE_pack)(NULL,
                     u_foreach_bit(a, vi->attributes_valid) {
      enum isl_format format = anv_get_isl_format(device->info,
                        uint32_t binding = vi->attributes[a].binding;
            if ((elements & (1 << a)) == 0)
            uint32_t slot =
      __builtin_popcount(elements & ((1 << a) - 1)) -
               struct GENX(VERTEX_ELEMENT_STATE) element = {
      .VertexBufferIndex = vi->attributes[a].binding,
   .Valid = true,
   .SourceElementFormat = format,
   .EdgeFlagEnable = false,
   .SourceElementOffset = vi->attributes[a].offset,
   .Component0Control = vertex_element_comp_control(format, 0),
   .Component1Control = vertex_element_comp_control(format, 1),
   .Component2Control = vertex_element_comp_control(format, 2),
      };
   GENX(VERTEX_ELEMENT_STATE_pack)(NULL,
                  /* On Broadwell and later, we have a separate VF_INSTANCING packet
   * that controls instancing.  On Haswell and prior, that's part of
   * VERTEX_BUFFER_STATE which we emit later.
   */
   if (emit_in_pipeline) {
      anv_pipeline_emit(pipeline, final.vf_instancing, GENX(3DSTATE_VF_INSTANCING), vfi) {
      bool per_instance = vi->bindings[binding].input_rate ==
                        vfi.InstancingEnable = per_instance;
   vfi.VertexElementIndex = slot;
         } else {
      anv_batch_emit(batch, GENX(3DSTATE_VF_INSTANCING), vfi) {
      bool per_instance = vi->bindings[binding].input_rate ==
                        vfi.InstancingEnable = per_instance;
   vfi.VertexElementIndex = slot;
               }
      static void
   emit_vertex_input(struct anv_graphics_pipeline *pipeline,
               {
      /* Only pack the VERTEX_ELEMENT_STATE if not dynamic so we can just memcpy
   * everything in gfx8_cmd_buffer.c
   */
   if (!BITSET_TEST(state->dynamic, MESA_VK_DYNAMIC_VI)) {
      genX(emit_vertex_input)(NULL,
                     const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);
   const bool needs_svgs_elem = pipeline->svgs_count > 1 ||
         const uint32_t id_slot = pipeline->vs_input_elements;
   const uint32_t drawid_slot = id_slot + needs_svgs_elem;
   if (pipeline->svgs_count > 0) {
      assert(pipeline->vertex_input_elems >= pipeline->svgs_count);
   uint32_t slot_offset =
            #if GFX_VER < 11
            /* From the Broadwell PRM for the 3D_Vertex_Component_Control enum:
   *    "Within a VERTEX_ELEMENT_STATE structure, if a Component
   *    Control field is set to something other than VFCOMP_STORE_SRC,
   *    no higher-numbered Component Control fields may be set to
   *    VFCOMP_STORE_SRC"
   *
   * This means, that if we have BaseInstance, we need BaseVertex as
   * well.  Just do all or nothing.
   */
   uint32_t base_ctrl = (vs_prog_data->uses_firstvertex ||
      #endif
               struct GENX(VERTEX_ELEMENT_STATE) element = {
      .VertexBufferIndex = ANV_SVGS_VB_INDEX,
   #if GFX_VER >= 11
               /* On gen11, these are taken care of by extra parameter slots */
   #else
               #endif
               .Component2Control = VFCOMP_STORE_0,
      };
   GENX(VERTEX_ELEMENT_STATE_pack)(NULL,
                        anv_pipeline_emit(pipeline, final.vf_sgvs_instancing,
                           if (vs_prog_data->uses_drawid) {
      struct GENX(VERTEX_ELEMENT_STATE) element = {
      .VertexBufferIndex = ANV_DRAWID_VB_INDEX,
   #if GFX_VER >= 11
               #else
         #endif
               .Component1Control = VFCOMP_STORE_0,
   .Component2Control = VFCOMP_STORE_0,
      };
   GENX(VERTEX_ELEMENT_STATE_pack)(NULL,
                        anv_pipeline_emit(pipeline, final.vf_sgvs_instancing,
                              anv_pipeline_emit(pipeline, final.vf_sgvs, GENX(3DSTATE_VF_SGVS), sgvs) {
      sgvs.VertexIDEnable              = vs_prog_data->uses_vertexid;
   sgvs.VertexIDComponentNumber     = 2;
   sgvs.VertexIDElementOffset       = id_slot;
   sgvs.InstanceIDEnable            = vs_prog_data->uses_instanceid;
   sgvs.InstanceIDComponentNumber   = 3;
            #if GFX_VER >= 11
      anv_pipeline_emit(pipeline, final.vf_sgvs_2, GENX(3DSTATE_VF_SGVS_2), sgvs) {
      /* gl_BaseVertex */
   sgvs.XP0Enable                   = vs_prog_data->uses_firstvertex;
   sgvs.XP0SourceSelect             = XP0_PARAMETER;
   sgvs.XP0ComponentNumber          = 0;
            /* gl_BaseInstance */
   sgvs.XP1Enable                   = vs_prog_data->uses_baseinstance;
   sgvs.XP1SourceSelect             = StartingInstanceLocation;
   sgvs.XP1ComponentNumber          = 1;
            /* gl_DrawID */
   sgvs.XP2Enable                   = vs_prog_data->uses_drawid;
   sgvs.XP2ComponentNumber          = 0;
         #endif
   }
      void
   genX(emit_urb_setup)(struct anv_device *device, struct anv_batch *batch,
                           {
               unsigned entries[4];
   unsigned start[4];
   bool constrained;
   intel_get_urb_config(devinfo, l3_config,
                        active_stages &
         for (int i = 0; i <= MESA_SHADER_GEOMETRY; i++) {
      anv_batch_emit(batch, GENX(3DSTATE_URB_VS), urb) {
      urb._3DCommandSubOpcode      += i;
   urb.VSURBStartingAddress      = start[i];
   urb.VSURBEntryAllocationSize  = entry_size[i] - 1;
            #if GFX_VERx10 >= 125
      if (device->vk.enabled_extensions.EXT_mesh_shader) {
      anv_batch_emit(batch, GENX(3DSTATE_URB_ALLOC_MESH), zero);
         #endif
   }
      #if GFX_VERx10 >= 125
   static void
   emit_urb_setup_mesh(struct anv_graphics_pipeline *pipeline,
         {
               const struct brw_task_prog_data *task_prog_data =
      anv_pipeline_has_stage(pipeline, MESA_SHADER_TASK) ?
               const struct intel_mesh_urb_allocation alloc =
      intel_get_mesh_urb_config(devinfo, pipeline->base.base.l3_config,
               /* Zero out the primitive pipeline URB allocations. */
   for (int i = 0; i <= MESA_SHADER_GEOMETRY; i++) {
      anv_pipeline_emit(pipeline, final.urb, GENX(3DSTATE_URB_VS), urb) {
                     anv_pipeline_emit(pipeline, final.urb, GENX(3DSTATE_URB_ALLOC_TASK), urb) {
      if (task_prog_data) {
      urb.TASKURBEntryAllocationSize   = alloc.task_entry_size_64b - 1;
   urb.TASKNumberofURBEntriesSlice0 = alloc.task_entries;
   urb.TASKNumberofURBEntriesSliceN = alloc.task_entries;
   urb.TASKURBStartingAddressSlice0 = alloc.task_starting_address_8kb;
                  anv_pipeline_emit(pipeline, final.urb, GENX(3DSTATE_URB_ALLOC_MESH), urb) {
      urb.MESHURBEntryAllocationSize   = alloc.mesh_entry_size_64b - 1;
   urb.MESHNumberofURBEntriesSlice0 = alloc.mesh_entries;
   urb.MESHNumberofURBEntriesSliceN = alloc.mesh_entries;
   urb.MESHURBStartingAddressSlice0 = alloc.mesh_starting_address_8kb;
                  }
   #endif
      static void
   emit_urb_setup(struct anv_graphics_pipeline *pipeline,
         {
   #if GFX_VERx10 >= 125
      if (anv_pipeline_is_mesh(pipeline)) {
      emit_urb_setup_mesh(pipeline, deref_block_size);
         #endif
         unsigned entry_size[4];
   for (int i = MESA_SHADER_VERTEX; i <= MESA_SHADER_GEOMETRY; i++) {
      const struct brw_vue_prog_data *prog_data =
                              struct anv_device *device = pipeline->base.base.device;
            unsigned entries[4];
   unsigned start[4];
   bool constrained;
   intel_get_urb_config(devinfo,
                        pipeline->base.base.l3_config,
   pipeline->base.base.active_stages &
               for (int i = 0; i <= MESA_SHADER_GEOMETRY; i++) {
      anv_pipeline_emit(pipeline, final.urb, GENX(3DSTATE_URB_VS), urb) {
      urb._3DCommandSubOpcode      += i;
   urb.VSURBStartingAddress      = start[i];
   urb.VSURBEntryAllocationSize  = entry_size[i] - 1;
            #if GFX_VERx10 >= 125
      if (device->vk.enabled_extensions.EXT_mesh_shader) {
      anv_pipeline_emit(pipeline, final.urb, GENX(3DSTATE_URB_ALLOC_TASK), zero);
         #endif
      }
      static void
   emit_3dstate_sbe(struct anv_graphics_pipeline *pipeline)
   {
               if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      anv_pipeline_emit(pipeline, final.sbe, GENX(3DSTATE_SBE), sbe);
   #if GFX_VERx10 >= 125
         if (anv_pipeline_is_mesh(pipeline))
   #endif
                     anv_pipeline_emit(pipeline, final.sbe, GENX(3DSTATE_SBE), sbe) {
               /* TODO(mesh): Figure out cases where we need attribute swizzling.  See also
   * calculate_urb_setup() and related functions.
   */
   sbe.AttributeSwizzleEnable = anv_pipeline_is_primitive(pipeline);
   sbe.PointSpriteTextureCoordinateOrigin = UPPERLEFT;
   sbe.NumberofSFOutputAttributes = wm_prog_data->num_varying_inputs;
            for (unsigned i = 0; i < 32; i++)
            if (anv_pipeline_is_primitive(pipeline)) {
                     int first_slot =
      brw_compute_first_urb_slot_required(wm_prog_data->inputs,
      assert(first_slot % 2 == 0);
   unsigned urb_entry_read_offset = first_slot / 2;
   int max_source_attr = 0;
   for (uint8_t idx = 0; idx < wm_prog_data->urb_setup_attribs_count; idx++) {
                              /* gl_Viewport, gl_Layer and FragmentShadingRateKHR are stored in the
   * VUE header
   */
   if (attr == VARYING_SLOT_VIEWPORT ||
      attr == VARYING_SLOT_LAYER ||
                     if (attr == VARYING_SLOT_PNTC) {
                                 if (slot == -1) {
      /* This attribute does not exist in the VUE--that means that
   * the vertex shader did not write to it. It could be that it's
   * a regular varying read by the fragment shader but not
   * written by the vertex shader or it's gl_PrimitiveID. In the
   * first case the value is undefined, in the second it needs to
   * be gl_PrimitiveID.
   */
   swiz.Attribute[input_index].ConstantSource = PRIM_ID;
   swiz.Attribute[input_index].ComponentOverrideX = true;
   swiz.Attribute[input_index].ComponentOverrideY = true;
   swiz.Attribute[input_index].ComponentOverrideZ = true;
                     /* We have to subtract two slots to account for the URB entry
   * output read offset in the VS and GS stages.
   */
   const int source_attr = slot - 2 * urb_entry_read_offset;
   assert(source_attr >= 0 && source_attr < 32);
   max_source_attr = MAX2(max_source_attr, source_attr);
   /* The hardware can only do overrides on 16 overrides at a time,
   * and the other up to 16 have to be lined up so that the input
   * index = the output index. We'll need to do some tweaking to
   * make sure that's the case.
   */
   if (input_index < 16)
         else
               sbe.VertexURBEntryReadOffset = urb_entry_read_offset;
   sbe.VertexURBEntryReadLength = DIV_ROUND_UP(max_source_attr + 1, 2);
                  /* Ask the hardware to supply PrimitiveID if the fragment shader
   * reads it but a previous stage didn't write one.
   */
   if ((wm_prog_data->inputs & VARYING_BIT_PRIMITIVE_ID) &&
      fs_input_map->varying_to_slot[VARYING_SLOT_PRIMITIVE_ID] == -1) {
   sbe.PrimitiveIDOverrideAttributeSelect =
         sbe.PrimitiveIDOverrideComponentX = true;
   sbe.PrimitiveIDOverrideComponentY = true;
   sbe.PrimitiveIDOverrideComponentZ = true;
   sbe.PrimitiveIDOverrideComponentW = true;
         } else {
   #if GFX_VERx10 >= 125
            const struct brw_mesh_prog_data *mesh_prog_data = get_mesh_prog_data(pipeline);
   anv_pipeline_emit(pipeline, final.sbe_mesh,
                     assert(mue->per_vertex_header_size_dw % 8 == 0);
                  /* Clip distance array is passed in the per-vertex header so that
   * it can be consumed by the HW. If user wants to read it in the
   * FS, adjust the offset and length to cover it. Conveniently it
   * is at the end of the per-vertex header, right before per-vertex
   * attributes.
   *
   * Note that FS attribute reading must be aware that the clip
   * distances have fixed position.
   */
   if (mue->per_vertex_header_size_dw > 8 &&
      (wm_prog_data->urb_setup[VARYING_SLOT_CLIP_DIST0] >= 0 ||
   wm_prog_data->urb_setup[VARYING_SLOT_CLIP_DIST1] >= 0)) {
                     if (mue->user_data_in_vertex_header) {
                        assert(mue->per_primitive_header_size_dw % 8 == 0);
   sbe_mesh.PerPrimitiveURBEntryOutputReadOffset =
                        /* Just like with clip distances, if Primitive Shading Rate,
   * Viewport Index or Layer is read back in the FS, adjust the
   * offset and length to cover the Primitive Header, where PSR,
   * Viewport Index & Layer are stored.
   */
   if (wm_prog_data->urb_setup[VARYING_SLOT_VIEWPORT] >= 0 ||
      wm_prog_data->urb_setup[VARYING_SLOT_PRIMITIVE_SHADING_RATE] >= 0 ||
   wm_prog_data->urb_setup[VARYING_SLOT_LAYER] >= 0 ||
   mue->user_data_in_primitive_header) {
   assert(sbe_mesh.PerPrimitiveURBEntryOutputReadOffset > 0);
   sbe_mesh.PerPrimitiveURBEntryOutputReadOffset -= 1;
      #endif
            }
      }
      /** Returns the final polygon mode for rasterization
   *
   * This function takes into account polygon mode, primitive topology and the
   * different shader stages which might generate their own type of primitives.
   */
   VkPolygonMode
   genX(raster_polygon_mode)(struct anv_graphics_pipeline *pipeline,
               {
      if (anv_pipeline_is_mesh(pipeline)) {
      switch (get_mesh_prog_data(pipeline)->primitive_type) {
   case MESA_PRIM_POINTS:
         case MESA_PRIM_LINES:
         case MESA_PRIM_TRIANGLES:
         default:
            } else if (anv_pipeline_has_stage(pipeline, MESA_SHADER_GEOMETRY)) {
      switch (get_gs_prog_data(pipeline)->output_topology) {
   case _3DPRIM_POINTLIST:
            case _3DPRIM_LINELIST:
   case _3DPRIM_LINESTRIP:
   case _3DPRIM_LINELOOP:
            case _3DPRIM_TRILIST:
   case _3DPRIM_TRIFAN:
   case _3DPRIM_TRISTRIP:
   case _3DPRIM_RECTLIST:
   case _3DPRIM_QUADLIST:
   case _3DPRIM_QUADSTRIP:
   case _3DPRIM_POLYGON:
         }
      } else if (anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL)) {
      switch (get_tes_prog_data(pipeline)->output_topology) {
   case BRW_TESS_OUTPUT_TOPOLOGY_POINT:
            case BRW_TESS_OUTPUT_TOPOLOGY_LINE:
            case BRW_TESS_OUTPUT_TOPOLOGY_TRI_CW:
   case BRW_TESS_OUTPUT_TOPOLOGY_TRI_CCW:
         }
      } else {
      switch (primitive_topology) {
   case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
            case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
   case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
   case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
   case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
            case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
            default:
               }
      const uint32_t genX(vk_to_intel_cullmode)[] = {
      [VK_CULL_MODE_NONE]                       = CULLMODE_NONE,
   [VK_CULL_MODE_FRONT_BIT]                  = CULLMODE_FRONT,
   [VK_CULL_MODE_BACK_BIT]                   = CULLMODE_BACK,
      };
      const uint32_t genX(vk_to_intel_fillmode)[] = {
      [VK_POLYGON_MODE_FILL]                    = FILL_MODE_SOLID,
   [VK_POLYGON_MODE_LINE]                    = FILL_MODE_WIREFRAME,
      };
      const uint32_t genX(vk_to_intel_front_face)[] = {
      [VK_FRONT_FACE_COUNTER_CLOCKWISE]         = 1,
      };
      static void
   emit_rs_state(struct anv_graphics_pipeline *pipeline,
               const struct vk_input_assembly_state *ia,
   const struct vk_rasterization_state *rs,
   const struct vk_multisample_state *ms,
   {
      anv_pipeline_emit(pipeline, partial.sf, GENX(3DSTATE_SF), sf) {
      sf.ViewportTransformEnable = true;
   sf.StatisticsEnable = true;
   sf.VertexSubPixelPrecisionSelect = _8Bit;
      #if GFX_VER >= 12
         #endif
            bool point_from_shader;
   if (anv_pipeline_is_primitive(pipeline)) {
      const struct brw_vue_prog_data *last_vue_prog_data =
            } else {
      assert(anv_pipeline_is_mesh(pipeline));
   const struct brw_mesh_prog_data *mesh_prog_data = get_mesh_prog_data(pipeline);
               if (point_from_shader) {
         } else {
      sf.PointWidthSource = State;
                  anv_pipeline_emit(pipeline, partial.raster, GENX(3DSTATE_RASTER), raster) {
      /* For details on 3DSTATE_RASTER multisample state, see the BSpec table
   * "Multisample Modes State".
   */
   /* NOTE: 3DSTATE_RASTER::ForcedSampleCount affects the BDW and SKL PMA fix
   * computations.  If we ever set this bit to a different value, they will
   * need to be updated accordingly.
   */
   raster.ForcedSampleCount = FSC_NUMRASTSAMPLES_0;
                  }
      static void
   emit_ms_state(struct anv_graphics_pipeline *pipeline,
         {
      anv_pipeline_emit(pipeline, final.ms, GENX(3DSTATE_MULTISAMPLE), ms) {
                        /* The PRM says that this bit is valid only for DX9:
   *
   *    SW can choose to set this bit only for DX9 API. DX10/OGL API's
   *    should not have any effect by setting or not setting this bit.
   */
         }
      const uint32_t genX(vk_to_intel_logic_op)[] = {
      [VK_LOGIC_OP_COPY]                        = LOGICOP_COPY,
   [VK_LOGIC_OP_CLEAR]                       = LOGICOP_CLEAR,
   [VK_LOGIC_OP_AND]                         = LOGICOP_AND,
   [VK_LOGIC_OP_AND_REVERSE]                 = LOGICOP_AND_REVERSE,
   [VK_LOGIC_OP_AND_INVERTED]                = LOGICOP_AND_INVERTED,
   [VK_LOGIC_OP_NO_OP]                       = LOGICOP_NOOP,
   [VK_LOGIC_OP_XOR]                         = LOGICOP_XOR,
   [VK_LOGIC_OP_OR]                          = LOGICOP_OR,
   [VK_LOGIC_OP_NOR]                         = LOGICOP_NOR,
   [VK_LOGIC_OP_EQUIVALENT]                  = LOGICOP_EQUIV,
   [VK_LOGIC_OP_INVERT]                      = LOGICOP_INVERT,
   [VK_LOGIC_OP_OR_REVERSE]                  = LOGICOP_OR_REVERSE,
   [VK_LOGIC_OP_COPY_INVERTED]               = LOGICOP_COPY_INVERTED,
   [VK_LOGIC_OP_OR_INVERTED]                 = LOGICOP_OR_INVERTED,
   [VK_LOGIC_OP_NAND]                        = LOGICOP_NAND,
      };
      const uint32_t genX(vk_to_intel_compare_op)[] = {
      [VK_COMPARE_OP_NEVER]                        = PREFILTEROP_NEVER,
   [VK_COMPARE_OP_LESS]                         = PREFILTEROP_LESS,
   [VK_COMPARE_OP_EQUAL]                        = PREFILTEROP_EQUAL,
   [VK_COMPARE_OP_LESS_OR_EQUAL]                = PREFILTEROP_LEQUAL,
   [VK_COMPARE_OP_GREATER]                      = PREFILTEROP_GREATER,
   [VK_COMPARE_OP_NOT_EQUAL]                    = PREFILTEROP_NOTEQUAL,
   [VK_COMPARE_OP_GREATER_OR_EQUAL]             = PREFILTEROP_GEQUAL,
      };
      const uint32_t genX(vk_to_intel_stencil_op)[] = {
      [VK_STENCIL_OP_KEEP]                         = STENCILOP_KEEP,
   [VK_STENCIL_OP_ZERO]                         = STENCILOP_ZERO,
   [VK_STENCIL_OP_REPLACE]                      = STENCILOP_REPLACE,
   [VK_STENCIL_OP_INCREMENT_AND_CLAMP]          = STENCILOP_INCRSAT,
   [VK_STENCIL_OP_DECREMENT_AND_CLAMP]          = STENCILOP_DECRSAT,
   [VK_STENCIL_OP_INVERT]                       = STENCILOP_INVERT,
   [VK_STENCIL_OP_INCREMENT_AND_WRAP]           = STENCILOP_INCR,
      };
      const uint32_t genX(vk_to_intel_primitive_type)[] = {
      [VK_PRIMITIVE_TOPOLOGY_POINT_LIST]                    = _3DPRIM_POINTLIST,
   [VK_PRIMITIVE_TOPOLOGY_LINE_LIST]                     = _3DPRIM_LINELIST,
   [VK_PRIMITIVE_TOPOLOGY_LINE_STRIP]                    = _3DPRIM_LINESTRIP,
   [VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST]                 = _3DPRIM_TRILIST,
   [VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP]                = _3DPRIM_TRISTRIP,
   [VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN]                  = _3DPRIM_TRIFAN,
   [VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY]      = _3DPRIM_LINELIST_ADJ,
   [VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY]     = _3DPRIM_LINESTRIP_ADJ,
   [VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY]  = _3DPRIM_TRILIST_ADJ,
      };
      static void
   emit_3dstate_clip(struct anv_graphics_pipeline *pipeline,
                     {
      const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);
            anv_pipeline_emit(pipeline, partial.clip, GENX(3DSTATE_CLIP), clip) {
      clip.ClipEnable               = true;
   clip.StatisticsEnable         = true;
   clip.EarlyCullEnable          = true;
            clip.VertexSubPixelPrecisionSelect = _8Bit;
            clip.MinimumPointWidth = 0.125;
            /* TODO(mesh): Multiview. */
   if (anv_pipeline_is_primitive(pipeline)) {
                     /* From the Vulkan 1.0.45 spec:
   *
   *    "If the last active vertex processing stage shader entry
   *    point's interface does not include a variable decorated with
   *    ViewportIndex, then the first viewport is used."
   */
   if (vp && (last->vue_map.slots_valid & VARYING_BIT_VIEWPORT)) {
      clip.MaximumVPIndex = vp->viewport_count > 0 ?
      } else {
                  /* From the Vulkan 1.0.45 spec:
   *
   *    "If the last active vertex processing stage shader entry point's
   *    interface does not include a variable decorated with Layer, then
   *    the first layer is used."
   */
               } else if (anv_pipeline_is_mesh(pipeline)) {
      const struct brw_mesh_prog_data *mesh_prog_data = get_mesh_prog_data(pipeline);
   if (vp && vp->viewport_count > 0 &&
      mesh_prog_data->map.start_dw[VARYING_SLOT_VIEWPORT] >= 0) {
      } else {
                  clip.ForceZeroRTAIndexEnable =
               clip.NonPerspectiveBarycentricEnable = wm_prog_data ?
            #if GFX_VERx10 >= 125
      if (anv_pipeline_is_mesh(pipeline)) {
      const struct brw_mesh_prog_data *mesh_prog_data = get_mesh_prog_data(pipeline);
   anv_pipeline_emit(pipeline, final.clip_mesh,
            clip_mesh.PrimitiveHeaderEnable = mesh_prog_data->map.per_primitive_header_size_dw > 0;
   clip_mesh.UserClipDistanceClipTestEnableBitmask = mesh_prog_data->clip_distance_mask;
            #endif
   }
      static void
   emit_3dstate_streamout(struct anv_graphics_pipeline *pipeline,
         {
      const struct brw_vue_prog_data *prog_data =
                  nir_xfb_info *xfb_info;
   if (anv_pipeline_has_stage(pipeline, MESA_SHADER_GEOMETRY))
         else if (anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL))
         else
            if (xfb_info) {
      struct GENX(SO_DECL) so_decl[MAX_XFB_STREAMS][128];
   int next_offset[MAX_XFB_BUFFERS] = {0, 0, 0, 0};
                     for (unsigned i = 0; i < xfb_info->output_count; i++) {
      const nir_xfb_output_info *output = &xfb_info->outputs[i];
                  /* Our hardware is unusual in that it requires us to program SO_DECLs
   * for fake "hole" components, rather than simply taking the offset
   * for each real varying.  Each hole can have size 1, 2, 3, or 4; we
   * program as many size = 4 holes as we can, then a final hole to
   * accommodate the final 1, 2, or 3 remaining.
   */
   int hole_dwords = (output->offset - next_offset[buffer]) / 4;
   while (hole_dwords > 0) {
      so_decl[stream][decls[stream]++] = (struct GENX(SO_DECL)) {
      .HoleFlag = 1,
   .OutputBufferSlot = buffer,
      };
               int varying = output->location;
   uint8_t component_mask = output->component_mask;
   /* VARYING_SLOT_PSIZ contains four scalar fields packed together:
   * - VARYING_SLOT_PRIMITIVE_SHADING_RATE in VARYING_SLOT_PSIZ.x
   * - VARYING_SLOT_LAYER                  in VARYING_SLOT_PSIZ.y
   * - VARYING_SLOT_VIEWPORT               in VARYING_SLOT_PSIZ.z
   * - VARYING_SLOT_PSIZ                   in VARYING_SLOT_PSIZ.w
   */
   if (varying == VARYING_SLOT_PRIMITIVE_SHADING_RATE) {
      varying = VARYING_SLOT_PSIZ;
      } else if (varying == VARYING_SLOT_LAYER) {
      varying = VARYING_SLOT_PSIZ;
      } else if (varying == VARYING_SLOT_VIEWPORT) {
      varying = VARYING_SLOT_PSIZ;
      } else if (varying == VARYING_SLOT_PSIZ) {
                                 const int slot = vue_map->varying_to_slot[varying];
   if (slot < 0) {
      /* This can happen if the shader never writes to the varying.
   * Insert a hole instead of actual varying data.
   */
   so_decl[stream][decls[stream]++] = (struct GENX(SO_DECL)) {
      .HoleFlag = true,
   .OutputBufferSlot = buffer,
         } else {
      so_decl[stream][decls[stream]++] = (struct GENX(SO_DECL)) {
      .OutputBufferSlot = buffer,
   .RegisterIndex = slot,
                     int max_decls = 0;
   for (unsigned s = 0; s < MAX_XFB_STREAMS; s++)
            uint8_t sbs[MAX_XFB_STREAMS] = { };
   for (unsigned b = 0; b < MAX_XFB_BUFFERS; b++) {
      if (xfb_info->buffers_written & (1 << b))
               uint32_t *dw = anv_pipeline_emitn(pipeline, final.so_decl_list,
                                    3 + 2 * max_decls,
   GENX(3DSTATE_SO_DECL_LIST),
   .StreamtoBufferSelects0 = sbs[0],
               for (int i = 0; i < max_decls; i++) {
      GENX(SO_DECL_ENTRY_pack)(NULL, dw + 3 + i * 2,
      &(struct GENX(SO_DECL_ENTRY)) {
      .Stream0Decl = so_decl[0][i],
   .Stream1Decl = so_decl[1][i],
   .Stream2Decl = so_decl[2][i],
                  anv_pipeline_emit(pipeline, partial.so, GENX(3DSTATE_STREAMOUT), so) {
      if (xfb_info) {
                              so.Buffer0SurfacePitch = xfb_info->buffers[0].stride;
   so.Buffer1SurfacePitch = xfb_info->buffers[1].stride;
                  int urb_entry_read_offset = 0;
                  /* We always read the whole vertex. This could be reduced at some
   * point by reading less and offsetting the register index in the
   * SO_DECLs.
   */
   so.Stream0VertexReadOffset = urb_entry_read_offset;
   so.Stream0VertexReadLength = urb_entry_read_length - 1;
   so.Stream1VertexReadOffset = urb_entry_read_offset;
   so.Stream1VertexReadLength = urb_entry_read_length - 1;
   so.Stream2VertexReadOffset = urb_entry_read_offset;
   so.Stream2VertexReadLength = urb_entry_read_length - 1;
   so.Stream3VertexReadOffset = urb_entry_read_offset;
            }
      static uint32_t
   get_sampler_count(const struct anv_shader_bin *bin)
   {
               /* We can potentially have way more than 32 samplers and that's ok.
   * However, the 3DSTATE_XS packets only have 3 bits to specify how
   * many to pre-fetch and all values above 4 are marked reserved.
   */
      }
      static UNUSED struct anv_address
   get_scratch_address(struct anv_pipeline *pipeline,
               {
      return (struct anv_address) {
      .bo = anv_scratch_pool_alloc(pipeline->device,
                     }
      static UNUSED uint32_t
   get_scratch_space(const struct anv_shader_bin *bin)
   {
         }
      static UNUSED uint32_t
   get_scratch_surf(struct anv_pipeline *pipeline,
               {
      if (bin->prog_data->total_scratch == 0)
            struct anv_bo *bo =
      anv_scratch_pool_alloc(pipeline->device,
            anv_reloc_list_add_bo(pipeline->batch.relocs, bo);
   return anv_scratch_pool_get_surf(pipeline->device,
            }
      static void
   emit_3dstate_vs(struct anv_graphics_pipeline *pipeline)
   {
      const struct intel_device_info *devinfo = pipeline->base.base.device->info;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);
   const struct anv_shader_bin *vs_bin =
                     anv_pipeline_emit(pipeline, final.vs, GENX(3DSTATE_VS), vs) {
      vs.Enable               = true;
   vs.StatisticsEnable     = true;
   vs.KernelStartPointer   = vs_bin->kernel.offset;
   vs.SIMD8DispatchEnable  =
            #if GFX_VER < 11
         #endif
         vs.VectorMaskEnable           = false;
   /* Wa_1606682166:
   * Incorrect TDL's SSP address shift in SARB for 16:6 & 18:8 modes.
   * Disable the Sampler state prefetch functionality in the SARB by
   * programming 0xB000[30] to '1'.
   */
   vs.SamplerCount               = GFX_VER == 11 ? 0 : get_sampler_count(vs_bin);
   vs.BindingTableEntryCount     = vs_bin->bind_map.surface_count;
   vs.FloatingPointMode          = IEEE754;
   vs.IllegalOpcodeExceptionEnable = false;
   vs.SoftwareExceptionEnable    = false;
            if (GFX_VER == 9 && devinfo->gt == 4 &&
      anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL)) {
   /* On Sky Lake GT4, we have experienced some hangs related to the VS
   * cache and tessellation.  It is unknown exactly what is happening
   * but the Haswell docs for the "VS Reference Count Full Force Miss
   * Enable" field of the "Thread Mode" register refer to a HSW bug in
   * which the VUE handle reference count would overflow resulting in
   * internal reference counting bugs.  My (Faith's) best guess is that
   * this bug cropped back up on SKL GT4 when we suddenly had more
   * threads in play than any previous gfx9 hardware.
   *
   * What we do know for sure is that setting this bit when
   * tessellation shaders are in use fixes a GPU hang in Batman: Arkham
   * City when playing with DXVK (https://bugs.freedesktop.org/107280).
   * Disabling the vertex cache with tessellation shaders should only
   * have a minor performance impact as the tessellation shaders are
   * likely generating and processing far more geometry than the vertex
   * stage.
   */
               vs.VertexURBEntryReadLength      = vs_prog_data->base.urb_read_length;
   vs.VertexURBEntryReadOffset      = 0;
   vs.DispatchGRFStartRegisterForURBData =
            vs.UserClipDistanceClipTestEnableBitmask =
         vs.UserClipDistanceCullTestEnableBitmask =
      #if GFX_VERx10 >= 125
         vs.ScratchSpaceBuffer =
   #else
         vs.PerThreadScratchSpace   = get_scratch_space(vs_bin);
   vs.ScratchSpaceBasePointer =
   #endif
         }
      static void
   emit_3dstate_hs_ds(struct anv_graphics_pipeline *pipeline,
         {
      if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL)) {
      anv_pipeline_emit(pipeline, final.hs, GENX(3DSTATE_HS), hs);
   anv_pipeline_emit(pipeline, final.ds, GENX(3DSTATE_DS), ds);
               const struct intel_device_info *devinfo = pipeline->base.base.device->info;
   const struct anv_shader_bin *tcs_bin =
         const struct anv_shader_bin *tes_bin =
            const struct brw_tcs_prog_data *tcs_prog_data = get_tcs_prog_data(pipeline);
            anv_pipeline_emit(pipeline, final.hs, GENX(3DSTATE_HS), hs) {
      hs.Enable = true;
   hs.StatisticsEnable = true;
   hs.KernelStartPointer = tcs_bin->kernel.offset;
   /* Wa_1606682166 */
   hs.SamplerCount = GFX_VER == 11 ? 0 : get_sampler_count(tcs_bin);
      #if GFX_VER >= 12
         /* Wa_1604578095:
   *
   *    Hang occurs when the number of max threads is less than 2 times
   *    the number of instance count. The number of max threads must be
   *    more than 2 times the number of instance count.
   */
   #endif
            hs.MaximumNumberofThreads = devinfo->max_tcs_threads - 1;
   hs.IncludeVertexHandles = true;
            hs.VertexURBEntryReadLength = 0;
   hs.VertexURBEntryReadOffset = 0;
   hs.DispatchGRFStartRegisterForURBData =
   #if GFX_VER >= 12
         hs.DispatchGRFStartRegisterForURBData5 =
   #endif
      #if GFX_VERx10 >= 125
         hs.ScratchSpaceBuffer =
   #else
         hs.PerThreadScratchSpace = get_scratch_space(tcs_bin);
   hs.ScratchSpaceBasePointer =
   #endif
      #if GFX_VER == 12
         /*  Patch Count threshold specifies the maximum number of patches that
   *  will be accumulated before a thread dispatch is forced.
   */
   #endif
            hs.DispatchMode = tcs_prog_data->base.dispatch_mode;
               anv_pipeline_emit(pipeline, final.ds, GENX(3DSTATE_DS), ds) {
      ds.Enable = true;
   ds.StatisticsEnable = true;
   ds.KernelStartPointer = tes_bin->kernel.offset;
   /* Wa_1606682166 */
   ds.SamplerCount = GFX_VER == 11 ? 0 : get_sampler_count(tes_bin);
   ds.BindingTableEntryCount = tes_bin->bind_map.surface_count;
            ds.ComputeWCoordinateEnable =
            ds.PatchURBEntryReadLength = tes_prog_data->base.urb_read_length;
   ds.PatchURBEntryReadOffset = 0;
   ds.DispatchGRFStartRegisterForURBData =
      #if GFX_VER < 11
         ds.DispatchMode =
      tes_prog_data->base.dispatch_mode == DISPATCH_MODE_SIMD8 ?
      #else
         assert(tes_prog_data->base.dispatch_mode == DISPATCH_MODE_SIMD8);
   #endif
            ds.UserClipDistanceClipTestEnableBitmask =
         ds.UserClipDistanceCullTestEnableBitmask =
      #if GFX_VER >= 12
         #endif
   #if GFX_VERx10 >= 125
         ds.ScratchSpaceBuffer =
   #else
         ds.PerThreadScratchSpace = get_scratch_space(tes_bin);
   ds.ScratchSpaceBasePointer =
   #endif
         }
      static UNUSED bool
   geom_or_tess_prim_id_used(struct anv_graphics_pipeline *pipeline)
   {
      const struct brw_tcs_prog_data *tcs_prog_data =
      anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_CTRL) ?
      const struct brw_tes_prog_data *tes_prog_data =
      anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL) ?
      const struct brw_gs_prog_data *gs_prog_data =
      anv_pipeline_has_stage(pipeline, MESA_SHADER_GEOMETRY) ?
         return (tcs_prog_data && tcs_prog_data->include_primitive_id) ||
            }
      static void
   emit_3dstate_te(struct anv_graphics_pipeline *pipeline)
   {
      anv_pipeline_emit(pipeline, partial.te, GENX(3DSTATE_TE), te) {
      if (anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL)) {
                     te.Partitioning = tes_prog_data->partitioning;
   te.TEDomain = tes_prog_data->domain;
   te.TEEnable = true;
      #if GFX_VERx10 >= 125
            const struct anv_device *device = pipeline->base.base.device;
   if (intel_needs_workaround(device->info, 22012699309))
                        if (intel_needs_workaround(device->info, 14015055625)) {
      /* Wa_14015055625:
   *
   * Disable Tessellation Distribution when primitive Id is enabled.
   */
   if (pipeline->primitive_id_override ||
                     te.TessellationDistributionLevel = TEDLEVEL_PATCH;
   /* 64_TRIANGLES */
   te.SmallPatchThreshold = 3;
   /* 1K_TRIANGLES */
   te.TargetBlockSize = 8;
      #endif
               }
      static void
   emit_3dstate_gs(struct anv_graphics_pipeline *pipeline)
   {
      if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_GEOMETRY)) {
      anv_pipeline_emit(pipeline, partial.gs, GENX(3DSTATE_GS), gs);
               const struct intel_device_info *devinfo = pipeline->base.base.device->info;
   const struct anv_shader_bin *gs_bin =
                  anv_pipeline_emit(pipeline, partial.gs, GENX(3DSTATE_GS), gs) {
      gs.Enable                  = true;
   gs.StatisticsEnable        = true;
   gs.KernelStartPointer      = gs_bin->kernel.offset;
            gs.SingleProgramFlow       = false;
   gs.VectorMaskEnable        = false;
   /* Wa_1606682166 */
   gs.SamplerCount            = GFX_VER == 11 ? 0 : get_sampler_count(gs_bin);
   gs.BindingTableEntryCount  = gs_bin->bind_map.surface_count;
   gs.IncludeVertexHandles    = gs_prog_data->base.include_vue_handles;
                     gs.OutputVertexSize        = gs_prog_data->output_vertex_size_hwords * 2 - 1;
   gs.OutputTopology          = gs_prog_data->output_topology;
   gs.ControlDataFormat       = gs_prog_data->control_data_format;
   gs.ControlDataHeaderSize   = gs_prog_data->control_data_header_size_hwords;
            gs.ExpectedVertexCount     = gs_prog_data->vertices_in;
   gs.StaticOutput            = gs_prog_data->static_vertex_count >= 0;
   gs.StaticOutputVertexCount = gs_prog_data->static_vertex_count >= 0 ?
            gs.VertexURBEntryReadOffset = 0;
   gs.VertexURBEntryReadLength = gs_prog_data->base.urb_read_length;
   gs.DispatchGRFStartRegisterForURBData =
            gs.UserClipDistanceClipTestEnableBitmask =
         gs.UserClipDistanceCullTestEnableBitmask =
      #if GFX_VERx10 >= 125
         gs.ScratchSpaceBuffer =
   #else
         gs.PerThreadScratchSpace   = get_scratch_space(gs_bin);
   gs.ScratchSpaceBasePointer =
   #endif
         }
      static bool
   rp_has_ds_self_dep(const struct vk_render_pass_state *rp)
   {
      return rp->pipeline_flags &
      }
      static void
   emit_3dstate_wm(struct anv_graphics_pipeline *pipeline,
                  const struct vk_input_assembly_state *ia,
   const struct vk_rasterization_state *rs,
      {
               anv_pipeline_emit(pipeline, partial.wm, GENX(3DSTATE_WM), wm) {
      wm.StatisticsEnable                    = true;
   wm.LineEndCapAntialiasingRegionWidth   = _05pixels;
   wm.LineAntialiasingRegionWidth         = _10pixels;
            if (anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      if (wm_prog_data->early_fragment_tests) {
         } else if (wm_prog_data->has_side_effects) {
         } else {
                  /* Gen8 hardware tries to compute ThreadDispatchEnable for us but
   * doesn't take into account KillPixels when no depth or stencil
   * writes are enabled. In order for occlusion queries to work
   * correctly with no attachments, we need to force-enable PS thread
   * dispatch.
   *
   * The BDW docs are pretty clear that that this bit isn't validated
   * and probably shouldn't be used in production:
   *
   *    "This must always be set to Normal. This field should not be
   *     tested for functional validation."
   *
   * Unfortunately, however, the other mechanism we have for doing this
   * is 3DSTATE_PS_EXTRA::PixelShaderHasUAV which causes hangs on BDW.
   * Given two bad options, we choose the one which works.
   */
   pipeline->force_fragment_thread_dispatch =
                  wm.BarycentricInterpolationMode =
      wm_prog_data_barycentric_modes(wm_prog_data,
         }
      static void
   emit_3dstate_ps(struct anv_graphics_pipeline *pipeline,
               {
      UNUSED const struct intel_device_info *devinfo =
         const struct anv_shader_bin *fs_bin =
            if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      anv_pipeline_emit(pipeline, final.ps, GENX(3DSTATE_PS), ps);
                        anv_pipeline_emit(pipeline, final.ps, GENX(3DSTATE_PS), ps) {
      intel_set_ps_dispatch_state(&ps, devinfo, wm_prog_data,
                  const bool persample =
            ps.KernelStartPointer0 = fs_bin->kernel.offset +
         ps.KernelStartPointer1 = fs_bin->kernel.offset +
         ps.KernelStartPointer2 = fs_bin->kernel.offset +
            ps.SingleProgramFlow          = false;
   ps.VectorMaskEnable           = wm_prog_data->uses_vmask;
   /* Wa_1606682166 */
   ps.SamplerCount               = GFX_VER == 11 ? 0 : get_sampler_count(fs_bin);
   ps.BindingTableEntryCount     = fs_bin->bind_map.surface_count;
   ps.PushConstantEnable         = wm_prog_data->base.nr_params > 0 ||
         ps.PositionXYOffsetSelect     =
                           ps.DispatchGRFStartRegisterForConstantSetupData0 =
         ps.DispatchGRFStartRegisterForConstantSetupData1 =
         ps.DispatchGRFStartRegisterForConstantSetupData2 =
      #if GFX_VERx10 >= 125
         ps.ScratchSpaceBuffer =
   #else
         ps.PerThreadScratchSpace   = get_scratch_space(fs_bin);
   ps.ScratchSpaceBasePointer =
   #endif
         }
      static void
   emit_3dstate_ps_extra(struct anv_graphics_pipeline *pipeline,
               {
               if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      anv_pipeline_emit(pipeline, final.ps_extra, GENX(3DSTATE_PS_EXTRA), ps);
               anv_pipeline_emit(pipeline, final.ps_extra, GENX(3DSTATE_PS_EXTRA), ps) {
      ps.PixelShaderValid              = true;
   ps.AttributeEnable               = wm_prog_data->num_varying_inputs > 0;
   ps.oMaskPresenttoRenderTarget    = wm_prog_data->uses_omask;
   ps.PixelShaderIsPerSample        =
         ps.PixelShaderComputedDepthMode  = wm_prog_data->computed_depth_mode;
   ps.PixelShaderUsesSourceDepth    = wm_prog_data->uses_src_depth;
            /* If the subpass has a depth or stencil self-dependency, then we need
   * to force the hardware to do the depth/stencil write *after* fragment
   * shader execution.  Otherwise, the writes may hit memory before we get
   * around to fetching from the input attachment and we may get the depth
   * or stencil value from the current draw rather than the previous one.
   */
   ps.PixelShaderKillsPixel         = rp_has_ds_self_dep(rp) ||
            ps.PixelShaderComputesStencil = wm_prog_data->computed_stencil;
            ps.InputCoverageMaskState = ICMS_NONE;
   assert(!wm_prog_data->inner_coverage); /* Not available in SPIR-V */
   if (!wm_prog_data->uses_sample_mask)
         else if (brw_wm_prog_data_is_coarse(wm_prog_data, 0))
         else if (wm_prog_data->post_depth_coverage)
         else
      #if GFX_VER >= 11
         ps.PixelShaderRequiresSourceDepthandorWPlaneCoefficients =
         ps.PixelShaderIsPerCoarsePixel =
   #endif
   #if GFX_VERx10 >= 125
         /* TODO: We should only require this when the last geometry shader uses
   *       a fragment shading rate that is not constant.
   */
   ps.EnablePSDependencyOnCPsizeChange =
   #endif
         }
      static void
   emit_3dstate_vf_statistics(struct anv_graphics_pipeline *pipeline)
   {
      anv_pipeline_emit(pipeline, final.vf_statistics,
                  }
      static void
   compute_kill_pixel(struct anv_graphics_pipeline *pipeline,
               {
      if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      pipeline->kill_pixel = false;
                        /* This computes the KillPixel portion of the computation for whether or
   * not we want to enable the PMA fix on gfx8 or gfx9.  It's given by this
   * chunk of the giant formula:
   *
   *    (3DSTATE_PS_EXTRA::PixelShaderKillsPixels ||
   *     3DSTATE_PS_EXTRA::oMask Present to RenderTarget ||
   *     3DSTATE_PS_BLEND::AlphaToCoverageEnable ||
   *     3DSTATE_PS_BLEND::AlphaTestEnable ||
   *     3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable)
   *
   * 3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable is always false and so is
   * 3DSTATE_PS_BLEND::AlphaTestEnable since Vulkan doesn't have a concept
   * of an alpha test.
   */
   pipeline->kill_pixel =
      rp_has_ds_self_dep(rp) ||
   wm_prog_data->uses_kill ||
   wm_prog_data->uses_omask ||
   }
      #if GFX_VER >= 12
   static void
   emit_3dstate_primitive_replication(struct anv_graphics_pipeline *pipeline,
         {
      if (anv_pipeline_is_mesh(pipeline)) {
      anv_pipeline_emit(pipeline, final.primitive_replication,
                     const int replication_count =
            assert(replication_count >= 1);
   if (replication_count == 1) {
      anv_pipeline_emit(pipeline, final.primitive_replication,
                     assert(replication_count == util_bitcount(rp->view_mask));
            anv_pipeline_emit(pipeline, final.primitive_replication,
            pr.ReplicaMask = (1 << replication_count) - 1;
            int i = 0;
   u_foreach_bit(view_index, rp->view_mask) {
      pr.RTAIOffset[i] = view_index;
            }
   #endif
      #if GFX_VERx10 >= 125
   static void
   emit_task_state(struct anv_graphics_pipeline *pipeline)
   {
               if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_TASK)) {
      anv_pipeline_emit(pipeline, final.task_control,
         anv_pipeline_emit(pipeline, final.task_shader,
         anv_pipeline_emit(pipeline, final.task_redistrib,
                     const struct anv_shader_bin *task_bin =
            anv_pipeline_emit(pipeline, final.task_control,
            tc.TaskShaderEnable = true;
   tc.ScratchSpaceBuffer =
                     const struct intel_device_info *devinfo = pipeline->base.base.device->info;
   const struct brw_task_prog_data *task_prog_data = get_task_prog_data(pipeline);
   const struct brw_cs_dispatch_info task_dispatch =
            anv_pipeline_emit(pipeline, final.task_shader,
            task.KernelStartPointer                = task_bin->kernel.offset;
   task.SIMDSize                          = task_dispatch.simd_size / 16;
   task.MessageSIMD                       = task.SIMDSize;
   task.NumberofThreadsinGPGPUThreadGroup = task_dispatch.threads;
   task.ExecutionMask                     = task_dispatch.right_mask;
   task.LocalXMaximum                     = task_dispatch.group_size - 1;
            task.NumberofBarriers                  = task_prog_data->base.uses_barrier;
   task.SharedLocalMemorySize             =
         task.PreferredSLMAllocationSize        =
            /*
   * 3DSTATE_TASK_SHADER_DATA.InlineData[0:1] will be used for an address
   * of a buffer with push constants and descriptor set table and
   * InlineData[2:7] will be used for first few push constants.
   */
                        /* Recommended values from "Task and Mesh Distribution Programming". */
   anv_pipeline_emit(pipeline, final.task_redistrib,
            redistrib.LocalBOTAccumulatorThreshold = MULTIPLIER_1;
   redistrib.SmallTaskThreshold = 1; /* 2^N */
   redistrib.TargetMeshBatchSize = devinfo->num_slices > 2 ? 3 : 5; /* 2^N */
   redistrib.TaskRedistributionLevel = TASKREDISTRIB_BOM;
         }
      static void
   emit_mesh_state(struct anv_graphics_pipeline *pipeline)
   {
                        anv_pipeline_emit(pipeline, final.mesh_control,
            mc.MeshShaderEnable = true;
   mc.ScratchSpaceBuffer =
                     const struct intel_device_info *devinfo = pipeline->base.base.device->info;
   const struct brw_mesh_prog_data *mesh_prog_data = get_mesh_prog_data(pipeline);
   const struct brw_cs_dispatch_info mesh_dispatch =
            const unsigned output_topology =
      mesh_prog_data->primitive_type == MESA_PRIM_POINTS ? OUTPUT_POINT :
   mesh_prog_data->primitive_type == MESA_PRIM_LINES  ? OUTPUT_LINE :
         uint32_t index_format;
   switch (mesh_prog_data->index_format) {
   case BRW_INDEX_FORMAT_U32:
      index_format = INDEX_U32;
      case BRW_INDEX_FORMAT_U888X:
      index_format = INDEX_U888X;
      default:
                  anv_pipeline_emit(pipeline, final.mesh_shader,
            mesh.KernelStartPointer                = mesh_bin->kernel.offset;
   mesh.SIMDSize                          = mesh_dispatch.simd_size / 16;
   mesh.MessageSIMD                       = mesh.SIMDSize;
   mesh.NumberofThreadsinGPGPUThreadGroup = mesh_dispatch.threads;
   mesh.ExecutionMask                     = mesh_dispatch.right_mask;
   mesh.LocalXMaximum                     = mesh_dispatch.group_size - 1;
            mesh.MaximumPrimitiveCount             = MAX2(mesh_prog_data->map.max_primitives, 1) - 1;
   mesh.OutputTopology                    = output_topology;
   mesh.PerVertexDataPitch                = mesh_prog_data->map.per_vertex_pitch_dw / 8;
   mesh.PerPrimitiveDataPresent           = mesh_prog_data->map.per_primitive_pitch_dw > 0;
   mesh.PerPrimitiveDataPitch             = mesh_prog_data->map.per_primitive_pitch_dw / 8;
            mesh.NumberofBarriers                  = mesh_prog_data->base.uses_barrier;
   mesh.SharedLocalMemorySize             =
         mesh.PreferredSLMAllocationSize        =
            /*
   * 3DSTATE_MESH_SHADER_DATA.InlineData[0:1] will be used for an address
   * of a buffer with push constants and descriptor set table and
   * InlineData[2:7] will be used for first few push constants.
   */
                        /* Recommended values from "Task and Mesh Distribution Programming". */
   anv_pipeline_emit(pipeline, final.mesh_distrib,
            distrib.DistributionMode = MESH_RR_FREE;
   distrib.TaskDistributionBatchSize = devinfo->num_slices > 2 ? 4 : 9; /* 2^N thread groups */
         }
   #endif
      void
   genX(graphics_pipeline_emit)(struct anv_graphics_pipeline *pipeline,
         {
      enum intel_urb_deref_block_size urb_deref_block_size;
            emit_rs_state(pipeline, state->ia, state->rs, state->ms, state->rp,
         emit_ms_state(pipeline, state->ms);
                  #if GFX_VER >= 12
         #endif
      #if GFX_VERx10 >= 125
      struct anv_device *device = pipeline->base.base.device;
   anv_pipeline_emit(pipeline, partial.vfg, GENX(3DSTATE_VFG), vfg) {
      /* If 3DSTATE_TE: TE Enable == 1 then RR_STRICT else RR_FREE*/
   vfg.DistributionMode =
      anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL) ? RR_STRICT :
      vfg.DistributionGranularity = BatchLevelGranularity;
   /* Wa_14014890652 */
   if (intel_device_info_is_dg2(device->info))
         /* 192 vertices for TRILIST_ADJ */
   vfg.ListNBatchSizeScale = 0;
   /* Batch size of 384 vertices */
   vfg.List3BatchSizeScale = 2;
   /* Batch size of 128 vertices */
   vfg.List2BatchSizeScale = 1;
   /* Batch size of 128 vertices */
   vfg.List1BatchSizeScale = 2;
   /* Batch size of 256 vertices for STRIP topologies */
   vfg.StripBatchSizeScale = 3;
   /* 192 control points for PATCHLIST_3 */
   vfg.PatchBatchSizeScale = 1;
   /* 192 control points for PATCHLIST_3 */
         #endif
                  if (anv_pipeline_is_primitive(pipeline)) {
               emit_3dstate_vs(pipeline);
   emit_3dstate_hs_ds(pipeline, state->ts);
   emit_3dstate_te(pipeline);
               #if GFX_VERx10 >= 125
         const struct anv_device *device = pipeline->base.base.device;
   /* Disable Mesh. */
   if (device->vk.enabled_extensions.EXT_mesh_shader) {
      anv_pipeline_emit(pipeline, final.mesh_control,
         anv_pipeline_emit(pipeline, final.mesh_shader,
         anv_pipeline_emit(pipeline, final.mesh_distrib,
         anv_pipeline_emit(pipeline, final.clip_mesh,
         anv_pipeline_emit(pipeline, final.sbe_mesh,
         anv_pipeline_emit(pipeline, final.task_control,
         anv_pipeline_emit(pipeline, final.task_shader,
         anv_pipeline_emit(pipeline, final.task_redistrib,
      #endif
      } else {
               #if GFX_VER >= 11
         #endif
         anv_pipeline_emit(pipeline, final.vs, GENX(3DSTATE_VS), vs);
   anv_pipeline_emit(pipeline, final.hs, GENX(3DSTATE_HS), hs);
   anv_pipeline_emit(pipeline, final.ds, GENX(3DSTATE_DS), ds);
   anv_pipeline_emit(pipeline, partial.te, GENX(3DSTATE_TE), te);
            /* BSpec 46303 forbids both 3DSTATE_MESH_CONTROL.MeshShaderEnable
   * and 3DSTATE_STREAMOUT.SOFunctionEnable to be 1.
   */
      #if GFX_VERx10 >= 125
         emit_task_state(pipeline);
   #endif
               emit_3dstate_sbe(pipeline);
   emit_3dstate_wm(pipeline, state->ia, state->rs,
         emit_3dstate_ps(pipeline, state->ms, state->cb);
      }
      #if GFX_VERx10 >= 125
      void
   genX(compute_pipeline_emit)(struct anv_compute_pipeline *pipeline)
   {
      const struct brw_cs_prog_data *cs_prog_data = get_cs_prog_data(pipeline);
      }
      #else /* #if GFX_VERx10 >= 125 */
      void
   genX(compute_pipeline_emit)(struct anv_compute_pipeline *pipeline)
   {
      struct anv_device *device = pipeline->base.device;
   const struct intel_device_info *devinfo = device->info;
                     const struct brw_cs_dispatch_info dispatch =
         const uint32_t vfe_curbe_allocation =
      ALIGN(cs_prog_data->push.per_thread.regs * dispatch.threads +
                  anv_batch_emit(&pipeline->base.batch, GENX(MEDIA_VFE_STATE), vfe) {
      vfe.StackSize              = 0;
   vfe.MaximumNumberofThreads =
         #if GFX_VER < 11
         #endif
         vfe.URBEntryAllocationSize = 2;
            if (cs_bin->prog_data->total_scratch) {
      /* Broadwell's Per Thread Scratch Space is in the range [0, 11]
   * where 0 = 1k, 1 = 2k, 2 = 4k, ..., 11 = 2M.
   */
   vfe.PerThreadScratchSpace =
         vfe.ScratchSpaceBasePointer =
                  struct GENX(INTERFACE_DESCRIPTOR_DATA) desc = {
      .KernelStartPointer     =
                  /* Wa_1606682166 */
   .SamplerCount           = GFX_VER == 11 ? 0 : get_sampler_count(cs_bin),
   /* We add 1 because the CS indirect parameters buffer isn't accounted
   * for in bind_map.surface_count.
   *
   * Typically set to 0 to avoid prefetching on every thread dispatch.
   */
   .BindingTableEntryCount = devinfo->verx10 == 125 ?
         .BarrierEnable          = cs_prog_data->uses_barrier,
   .SharedLocalMemorySize  =
            .ConstantURBEntryReadOffset = 0,
   .ConstantURBEntryReadLength = cs_prog_data->push.per_thread.regs,
   .CrossThreadConstantDataReadLength =
   #if GFX_VER >= 12
         /* TODO: Check if we are missing workarounds and enable mid-thread
   * preemption.
   *
   * We still have issues with mid-thread preemption (it was already
   * disabled by the kernel on gfx11, due to missing workarounds). It's
   * possible that we are just missing some workarounds, and could enable
   * it later, but for now let's disable it to fix a GPU in compute in Car
   * Chase (and possibly more).
   */
   #endif
               };
   GENX(INTERFACE_DESCRIPTOR_DATA_pack)(NULL,
            }
      #endif /* #if GFX_VERx10 >= 125 */
      #if GFX_VERx10 >= 125
      void
   genX(ray_tracing_pipeline_emit)(struct anv_ray_tracing_pipeline *pipeline)
   {
      for (uint32_t i = 0; i < pipeline->group_count; i++) {
               switch (group->type) {
   case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR: {
      struct GENX(RT_GENERAL_SBT_HANDLE) sh = {};
   sh.General = anv_shader_bin_get_bsr(group->general, 32);
   GENX(RT_GENERAL_SBT_HANDLE_pack)(NULL, group->handle, &sh);
               case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR: {
      struct GENX(RT_TRIANGLES_SBT_HANDLE) sh = {};
   if (group->closest_hit)
         if (group->any_hit)
         GENX(RT_TRIANGLES_SBT_HANDLE_pack)(NULL, group->handle, &sh);
               case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR: {
      struct GENX(RT_PROCEDURAL_SBT_HANDLE) sh = {};
   if (group->closest_hit)
         sh.Intersection = anv_shader_bin_get_bsr(group->intersection, 24);
   GENX(RT_PROCEDURAL_SBT_HANDLE_pack)(NULL, group->handle, &sh);
               default:
               }
      #else
      void
   genX(ray_tracing_pipeline_emit)(struct anv_ray_tracing_pipeline *pipeline)
   {
         }
      #endif /* GFX_VERx10 >= 125 */
