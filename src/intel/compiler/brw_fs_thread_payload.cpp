   /*
   * Copyright © 2006-2022 Intel Corporation
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
      #include "brw_fs.h"
      using namespace brw;
      vs_thread_payload::vs_thread_payload(const fs_visitor &v)
   {
               /* R0: Thread header. */
            /* R1: URB handles. */
   urb_handles = brw_ud8_grf(r, 0);
               }
      tcs_thread_payload::tcs_thread_payload(const fs_visitor &v)
   {
      struct brw_vue_prog_data *vue_prog_data = brw_vue_prog_data(v.prog_data);
   struct brw_tcs_prog_data *tcs_prog_data = brw_tcs_prog_data(v.prog_data);
            if (vue_prog_data->dispatch_mode == DISPATCH_MODE_TCS_SINGLE_PATCH) {
      patch_urb_output = brw_ud1_grf(0, 0);
            /* r1-r4 contain the ICP handles. */
               } else {
      assert(vue_prog_data->dispatch_mode == DISPATCH_MODE_TCS_MULTI_PATCH);
                              patch_urb_output = brw_ud8_grf(r, 0);
            if (tcs_prog_data->include_primitive_id) {
      primitive_id = brw_vec8_grf(r, 0);
               /* ICP handles occupy the next 1-32 registers. */
   icp_handle_start = brw_ud8_grf(r, 0);
                  }
      tes_thread_payload::tes_thread_payload(const fs_visitor &v)
   {
               /* R0: Thread Header. */
   patch_urb_input = retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_UD);
   primitive_id = brw_vec1_grf(0, 1);
            /* R1-3: gl_TessCoord.xyz. */
   for (unsigned i = 0; i < 3; i++) {
      coords[i] = brw_vec8_grf(r, 0);
               /* R4: URB output handles. */
   urb_output = brw_ud8_grf(r, 0);
               }
      gs_thread_payload::gs_thread_payload(const fs_visitor &v)
   {
      struct brw_vue_prog_data *vue_prog_data = brw_vue_prog_data(v.prog_data);
            /* R0: thread header. */
            /* R1: output URB handles. */
   urb_handles = v.bld.vgrf(BRW_REGISTER_TYPE_UD);
   v.bld.AND(urb_handles, brw_ud8_grf(r, 0),
                  if (gs_prog_data->include_primitive_id) {
      primitive_id = brw_ud8_grf(r, 0);
               /* Always enable VUE handles so we can safely use pull model if needed.
   *
   * The push model for a GS uses a ton of register space even for trivial
   * scenarios with just a few inputs, so just make things easier and a bit
   * safer by always having pull model available.
   */
            /* R3..RN: ICP Handles for each incoming vertex (when using pull model) */
   icp_handle_start = brw_ud8_grf(r, 0);
                     /* Use a maximum of 24 registers for push-model inputs. */
            /* If pushing our inputs would take too many registers, reduce the URB read
   * length (which is in HWords, or 8 registers), and resort to pulling.
   *
   * Note that the GS reads <URB Read Length> HWords for every vertex - so we
   * have to multiply by VerticesIn to obtain the total storage requirement.
   */
   if (8 * vue_prog_data->urb_read_length * v.nir->info.gs.vertices_in >
      max_push_components) {
   vue_prog_data->urb_read_length =
         }
      static inline void
   setup_fs_payload_gfx6(fs_thread_payload &payload,
               {
               const unsigned payload_width = MIN2(16, v.dispatch_width);
   assert(v.dispatch_width % payload_width == 0);
                     /* R0: PS thread payload header. */
            for (unsigned j = 0; j < v.dispatch_width / payload_width; j++) {
      /* R1: masks, pixel X/Y coordinates. */
               for (unsigned j = 0; j < v.dispatch_width / payload_width; j++) {
      /* R3-26: barycentric interpolation coordinates.  These appear in the
   * same order that they appear in the brw_barycentric_mode enum.  Each
   * set of coordinates occupies 2 registers if dispatch width == 8 and 4
   * registers if dispatch width == 16.  Coordinates only appear if they
   * were enabled using the "Barycentric Interpolation Mode" bits in
   * WM_STATE.
   */
   for (int i = 0; i < BRW_BARYCENTRIC_MODE_COUNT; ++i) {
      if (prog_data->barycentric_interp_modes & (1 << i)) {
      payload.barycentric_coord_reg[i][j] = payload.num_regs;
                  /* R27-28: interpolated depth if uses source depth */
   if (prog_data->uses_src_depth) {
      payload.source_depth_reg[j] = payload.num_regs;
               /* R29-30: interpolated W set if GFX6_WM_USES_SOURCE_W. */
   if (prog_data->uses_src_w) {
      payload.source_w_reg[j] = payload.num_regs;
               /* R31: MSAA position offsets. */
   if (prog_data->uses_pos_offset) {
      payload.sample_pos_reg[j] = payload.num_regs;
               /* R32-33: MSAA input coverage mask */
   if (prog_data->uses_sample_mask) {
      assert(v.devinfo->ver >= 7);
   payload.sample_mask_in_reg[j] = payload.num_regs;
               /* R66: Source Depth and/or W Attribute Vertex Deltas */
   if (prog_data->uses_depth_w_coefficients) {
      payload.depth_w_coef_reg[j] = payload.num_regs;
                  if (v.nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
            }
      #undef P                        /* prompted depth */
   #undef C                        /* computed */
   #undef N                        /* non-promoted? */
      #define P 0
   #define C 1
   #define N 2
      static const struct {
      GLuint mode:2;
   GLuint sd_present:1;
   GLuint sd_to_rt:1;
   GLuint dd_present:1;
      } wm_iz_table[BRW_WM_IZ_BIT_MAX] =
   {
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { N, 1, 1, 0, 0 },
   { N, 0, 1, 0, 0 },
   { N, 0, 1, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { C, 0, 1, 1, 0 },
   { C, 0, 1, 1, 0 },
   { P, 0, 0, 0, 0 },
   { N, 1, 1, 0, 0 },
   { C, 0, 1, 1, 0 },
   { C, 0, 1, 1, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { N, 1, 1, 0, 0 },
   { N, 0, 1, 0, 0 },
   { N, 0, 1, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { C, 0, 1, 1, 0 },
   { C, 0, 1, 1, 0 },
   { P, 0, 0, 0, 0 },
   { N, 1, 1, 0, 0 },
   { C, 0, 1, 1, 0 },
   { C, 0, 1, 1, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { N, 1, 1, 0, 1 },
   { N, 0, 1, 0, 1 },
   { N, 0, 1, 0, 1 },
   { P, 0, 0, 0, 0 },
   { P, 0, 0, 0, 0 },
   { C, 0, 1, 1, 1 },
   { C, 0, 1, 1, 1 },
   { P, 0, 0, 0, 0 },
   { N, 1, 1, 0, 1 },
   { C, 0, 1, 1, 1 },
   { C, 0, 1, 1, 1 },
   { P, 0, 0, 0, 0 },
   { C, 0, 0, 0, 1 },
   { P, 0, 0, 0, 0 },
   { C, 0, 1, 0, 1 },
   { P, 0, 0, 0, 0 },
   { C, 1, 1, 0, 1 },
   { C, 0, 1, 0, 1 },
   { C, 0, 1, 0, 1 },
   { P, 0, 0, 0, 0 },
   { C, 1, 1, 1, 1 },
   { C, 0, 1, 1, 1 },
   { C, 0, 1, 1, 1 },
   { P, 0, 0, 0, 0 },
   { C, 1, 1, 1, 1 },
   { C, 0, 1, 1, 1 },
   { C, 0, 1, 1, 1 }
   };
      /**
   * \param line_aa  BRW_NEVER, BRW_ALWAYS or BRW_SOMETIMES
   * \param lookup  bitmask of BRW_WM_IZ_* flags
   */
   static inline void
   setup_fs_payload_gfx4(fs_thread_payload &payload,
                     {
               struct brw_wm_prog_data *prog_data = brw_wm_prog_data(v.prog_data);
            GLuint reg = 1;
   bool kill_stats_promoted_workaround = false;
                     /* Crazy workaround in the windowizer, which we need to track in
   * our register allocation and render target writes.  See the "If
   * statistics are enabled..." paragraph of 11.5.3.2: Early Depth
   * Test Cases [Pre-DevGT] of the 3D Pipeline - Windower B-Spec.
   */
   if (key->stats_wm &&
      (lookup & BRW_WM_IZ_PS_KILL_ALPHATEST_BIT) &&
   wm_iz_table[lookup].mode == P) {
                        if (wm_iz_table[lookup].sd_present || prog_data->uses_src_depth ||
      kill_stats_promoted_workaround) {
   payload.source_depth_reg[0] = reg;
               if (wm_iz_table[lookup].sd_to_rt || kill_stats_promoted_workaround)
            if (wm_iz_table[lookup].ds_present || key->line_aa != BRW_NEVER) {
      payload.aa_dest_stencil_reg[0] = reg;
   runtime_check_aads_emit =
                     if (wm_iz_table[lookup].dd_present) {
      payload.dest_depth_reg[0] = reg;
                  }
      #undef P                        /* prompted depth */
   #undef C                        /* computed */
   #undef N                        /* non-promoted? */
      fs_thread_payload::fs_thread_payload(const fs_visitor &v,
               : subspan_coord_reg(),
      source_depth_reg(),
   source_w_reg(),
   aa_dest_stencil_reg(),
   dest_depth_reg(),
   sample_pos_reg(),
   sample_mask_in_reg(),
   depth_w_coef_reg(),
      {
      if (v.devinfo->ver >= 6)
         else
      setup_fs_payload_gfx4(*this, v, source_depth_to_render_target,
   }
      cs_thread_payload::cs_thread_payload(const fs_visitor &v)
   {
      /* See nir_setup_uniforms for subgroup_id in earlier versions. */
   if (v.devinfo->verx10 >= 125)
            /* TODO: Fill out uses_btd_stack_ids automatically */
   num_regs = (1 + brw_cs_prog_data(v.prog_data)->uses_btd_stack_ids) *
      }
      void
   cs_thread_payload::load_subgroup_id(const fs_builder &bld,
         {
      auto devinfo = bld.shader->devinfo;
            if (subgroup_id_.file != BAD_FILE) {
      assert(devinfo->verx10 >= 125);
      } else {
      assert(devinfo->verx10 < 125);
   assert(gl_shader_stage_is_compute(bld.shader->stage));
   int index = brw_get_subgroup_id_param_index(devinfo,
               }
      task_mesh_thread_payload::task_mesh_thread_payload(const fs_visitor &v)
         {
      /* Task and Mesh Shader Payloads (SIMD8 and SIMD16)
   *
   *  R0: Header
   *  R1: Local_ID.X[0-7 or 0-15]
   *  R2: Inline Parameter
   *
   * Task and Mesh Shader Payloads (SIMD32)
   *
   *  R0: Header
   *  R1: Local_ID.X[0-15]
   *  R2: Local_ID.X[16-31]
   *  R3: Inline Parameter
   *
   * Local_ID.X values are 16 bits.
   *
   * Inline parameter is optional but always present since we use it to pass
   * the address to descriptors.
            unsigned r = 0;
   assert(subgroup_id_.file != BAD_FILE);
            if (v.devinfo->ver >= 20) {
         } else {
      urb_output = v.bld.vgrf(BRW_REGISTER_TYPE_UD);
   /* In both mesh and task shader payload, lower 16 bits of g0.6 is
   * an offset within Slice's Local URB, which says where shader is
   * supposed to output its data.
   */
               if (v.stage == MESA_SHADER_MESH) {
      /* g0.7 is Task Shader URB Entry Offset, which contains both an offset
   * within Slice's Local USB (bits 0:15) and a slice selector
   * (bits 16:24). Slice selector can be non zero when mesh shader
   * is spawned on slice other than the one where task shader was run.
   * Bit 24 says that Slice ID is present and bits 16:23 is the Slice ID.
   */
      }
            local_index = brw_uw8_grf(r, 0);
   r += reg_unit(v.devinfo);
   if (v.devinfo->ver < 20 && v.dispatch_width == 32)
            inline_parameter = brw_ud1_grf(r, 0);
               }
      bs_thread_payload::bs_thread_payload(const fs_visitor &v)
   {
               /* R0: Thread header. */
            /* R1: Stack IDs. */
            /* R2: Inline Parameter.  Used for argument addresses. */
   global_arg_ptr = brw_ud1_grf(r, 0);
   local_arg_ptr = brw_ud1_grf(r, 2);
               }
      void
   bs_thread_payload::load_shader_type(const fs_builder &bld, fs_reg &dest) const
   {
      fs_reg ud_dest = retype(dest, BRW_REGISTER_TYPE_UD);
   bld.MOV(ud_dest, retype(brw_vec1_grf(0, 3), ud_dest.type));
      }
