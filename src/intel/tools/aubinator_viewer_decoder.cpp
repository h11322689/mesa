   /*
   * Copyright © 2017 Intel Corporation
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
      #include <string.h>
      #include "util/macros.h"
      #include "aubinator_viewer.h"
      void
   aub_viewer_decode_ctx_init(struct aub_viewer_decode_ctx *ctx,
                              struct aub_viewer_cfg *cfg,
   struct aub_viewer_decode_cfg *decode_cfg,
      {
               ctx->get_bo = get_bo;
   ctx->get_state_size = get_state_size;
   ctx->user_data = user_data;
   ctx->devinfo = devinfo;
            ctx->cfg = cfg;
   ctx->decode_cfg = decode_cfg;
      }
      static void
   aub_viewer_print_group(struct aub_viewer_decode_ctx *ctx,
               {
      struct intel_field_iterator iter;
   int last_dword = -1;
            intel_field_iterator_init(&iter, group, p, 0, false);
   while (intel_field_iterator_next(&iter)) {
      if (ctx->decode_cfg->show_dwords) {
      int iter_dword = iter.end_bit / 32;
   if (last_dword != iter_dword) {
      for (int i = last_dword + 1; i <= iter_dword; i++) {
      ImGui::TextColored(ctx->cfg->dwords_color,
            }
         }
   if (!intel_field_is_header(iter.field)) {
      if (ctx->decode_cfg->field_filter.PassFilter(iter.name)) {
      if (iter.field->type.kind == intel_type::INTEL_TYPE_BOOL && iter.raw_value) {
      ImGui::Text("%s: ", iter.name); ImGui::SameLine();
      } else {
         }
   if (iter.struct_desc) {
      int struct_dword = iter.start_bit / 32;
   uint64_t struct_address = address + 4 * struct_dword;
   aub_viewer_print_group(ctx, iter.struct_desc, struct_address,
                  }
      static struct intel_batch_decode_bo
   ctx_get_bo(struct aub_viewer_decode_ctx *ctx, bool ppgtt, uint64_t addr)
   {
      if (intel_spec_get_gen(ctx->spec) >= intel_make_gen(8,0)) {
      /* On Broadwell and above, we have 48-bit addresses which consume two
   * dwords.  Some packets require that these get stored in a "canonical
   * form" which means that bit 47 is sign-extended through the upper
   * bits. In order to correctly handle those aub dumps, we need to mask
   * off the top 16 bits.
   */
                        if (intel_spec_get_gen(ctx->spec) >= intel_make_gen(8,0))
            /* We may actually have an offset into the bo */
   if (bo.map != NULL) {
      assert(bo.addr <= addr);
   uint64_t offset = addr - bo.addr;
   bo.map = (const uint8_t *)bo.map + offset;
   bo.addr += offset;
                  }
      static int
   update_count(struct aub_viewer_decode_ctx *ctx,
               uint32_t offset_from_dsba,
   {
               if (ctx->get_state_size)
            if (size > 0)
            /* In the absence of any information, just guess arbitrarily. */
      }
      static void
   ctx_disassemble_program(struct aub_viewer_decode_ctx *ctx,
         {
      uint64_t addr = ctx->instruction_base + ksp;
   struct intel_batch_decode_bo bo = ctx_get_bo(ctx, true, addr);
   if (!bo.map) {
      ImGui::TextColored(ctx->cfg->missing_color,
                     ImGui::PushID(addr);
   if (ImGui::Button(type) && ctx->display_shader)
            }
      static void
   handle_state_base_address(struct aub_viewer_decode_ctx *ctx,
               {
      struct intel_field_iterator iter;
            uint64_t surface_base = 0, dynamic_base = 0, instruction_base = 0;
            while (intel_field_iterator_next(&iter)) {
      if (strcmp(iter.name, "Surface State Base Address") == 0) {
         } else if (strcmp(iter.name, "Dynamic State Base Address") == 0) {
         } else if (strcmp(iter.name, "Instruction Base Address") == 0) {
         } else if (strcmp(iter.name, "Surface State Base Address Modify Enable") == 0) {
         } else if (strcmp(iter.name, "Dynamic State Base Address Modify Enable") == 0) {
         } else if (strcmp(iter.name, "Instruction Base Address Modify Enable") == 0) {
                     if (dynamic_modify)
            if (surface_modify)
            if (instruction_modify)
      }
      static void
   dump_binding_table(struct aub_viewer_decode_ctx *ctx, uint32_t offset, int count)
   {
      struct intel_group *strct =
         if (strct == NULL) {
      ImGui::TextColored(ctx->cfg->missing_color, "did not find RENDER_SURFACE_STATE info");
               if (count < 0)
            if (offset % 32 != 0 || offset >= UINT16_MAX) {
      ImGui::TextColored(ctx->cfg->missing_color, "invalid binding table pointer");
               struct intel_batch_decode_bo bind_bo =
            if (bind_bo.map == NULL) {
      ImGui::TextColored(ctx->cfg->missing_color,
                           const uint32_t *pointers = (const uint32_t *) bind_bo.map;
   for (int i = 0; i < count; i++) {
      if (pointers[i] == 0)
            uint64_t addr = ctx->surface_base + pointers[i];
   struct intel_batch_decode_bo bo = ctx_get_bo(ctx, true, addr);
            if (pointers[i] % 32 != 0 ||
      addr < bo.addr || addr + size >= bo.addr + bo.size) {
   ImGui::TextColored(ctx->cfg->missing_color,
                     const uint8_t *state = (const uint8_t *) bo.map + (addr - bo.addr);
   if (ImGui::TreeNodeEx(&pointers[i], ImGuiTreeNodeFlags_Framed,
            aub_viewer_print_group(ctx, strct, addr, state);
            }
      static void
   dump_samplers(struct aub_viewer_decode_ctx *ctx, uint32_t offset, int count)
   {
               uint64_t state_addr = ctx->dynamic_base + offset;
   struct intel_batch_decode_bo bo = ctx_get_bo(ctx, true, state_addr);
            if (state_map == NULL) {
      ImGui::TextColored(ctx->cfg->missing_color,
                     if (offset % 32 != 0) {
      ImGui::TextColored(ctx->cfg->missing_color, "invalid sampler state pointer");
                        if (count * sampler_state_size >= bo.size) {
      ImGui::TextColored(ctx->cfg->missing_color, "sampler state ends after bo ends");
               for (int i = 0; i < count; i++) {
      if (ImGui::TreeNodeEx(state_map, ImGuiTreeNodeFlags_Framed,
            aub_viewer_print_group(ctx, strct, state_addr, state_map);
      }
   state_addr += sampler_state_size;
         }
      static void
   handle_media_interface_descriptor_load(struct aub_viewer_decode_ctx *ctx,
               {
      struct intel_group *desc =
            struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, inst, p, 0, false);
   uint32_t descriptor_offset = 0;
   int descriptor_count = 0;
   while (intel_field_iterator_next(&iter)) {
      if (strcmp(iter.name, "Interface Descriptor Data Start Address") == 0) {
         } else if (strcmp(iter.name, "Interface Descriptor Total Length") == 0) {
      descriptor_count =
                  uint64_t desc_addr = ctx->dynamic_base + descriptor_offset;
   struct intel_batch_decode_bo bo = ctx_get_bo(ctx, true, desc_addr);
            if (desc_map == NULL) {
      ImGui::TextColored(ctx->cfg->missing_color,
                     for (int i = 0; i < descriptor_count; i++) {
                        intel_field_iterator_init(&iter, desc, desc_map, 0, false);
   uint64_t ksp = 0;
   uint32_t sampler_offset = 0, sampler_count = 0;
   uint32_t binding_table_offset = 0, binding_entry_count = 0;
   while (intel_field_iterator_next(&iter)) {
      if (strcmp(iter.name, "Kernel Start Pointer") == 0) {
         } else if (strcmp(iter.name, "Sampler State Pointer") == 0) {
         } else if (strcmp(iter.name, "Sampler Count") == 0) {
         } else if (strcmp(iter.name, "Binding Table Pointer") == 0) {
         } else if (strcmp(iter.name, "Binding Table Entry Count") == 0) {
                              dump_samplers(ctx, sampler_offset, sampler_count);
            desc_map += desc->dw_length;
         }
      static void
   handle_3dstate_vertex_buffers(struct aub_viewer_decode_ctx *ctx,
               {
               struct intel_batch_decode_bo vb = {};
   uint32_t vb_size = 0;
   int index = -1;
   int pitch = -1;
            struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, inst, p, 0, false);
   while (intel_field_iterator_next(&iter)) {
      if (iter.struct_desc != vbs)
                     struct intel_field_iterator vbs_iter;
   intel_field_iterator_init(&vbs_iter, vbs, &iter.p[iter.start_bit / 32], 0, false);
   while (intel_field_iterator_next(&vbs_iter)) {
      if (strcmp(vbs_iter.name, "Vertex Buffer Index") == 0) {
         } else if (strcmp(vbs_iter.name, "Buffer Pitch") == 0) {
         } else if (strcmp(vbs_iter.name, "Buffer Starting Address") == 0) {
      buffer_addr = vbs_iter.raw_value;
      } else if (strcmp(vbs_iter.name, "Buffer Size") == 0) {
      vb_size = vbs_iter.raw_value;
      } else if (strcmp(vbs_iter.name, "End Address") == 0) {
      if (vb.map && vbs_iter.raw_value >= vb.addr)
         else
                                             if (vb.map == NULL) {
      ImGui::TextColored(ctx->cfg->missing_color,
                                    vb.map = NULL;
   vb_size = 0;
   index = -1;
   pitch = -1;
            }
      static void
   handle_3dstate_index_buffer(struct aub_viewer_decode_ctx *ctx,
               {
      struct intel_batch_decode_bo ib = {};
   uint64_t buffer_addr = 0;
   uint32_t ib_size = 0;
            struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, inst, p, 0, false);
   while (intel_field_iterator_next(&iter)) {
      if (strcmp(iter.name, "Index Format") == 0) {
         } else if (strcmp(iter.name, "Buffer Starting Address") == 0) {
      buffer_addr = iter.raw_value;
      } else if (strcmp(iter.name, "Buffer Size") == 0) {
                     if (ib.map == NULL) {
      ImGui::TextColored(ctx->cfg->missing_color,
                           const uint8_t *m = (const uint8_t *) ib.map;
   const uint8_t *ib_end = m + MIN2(ib.size, ib_size);
   for (int i = 0; m < ib_end && i < 10; i++) {
      switch (format) {
   case 0:
      m += 1;
      case 1:
      m += 2;
      case 2:
      m += 4;
            }
      static void
   decode_single_ksp(struct aub_viewer_decode_ctx *ctx,
               {
      uint64_t ksp = 0;
   bool is_simd8 = false; /* vertex shaders on Gfx8+ only */
            struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, inst, p, 0, false);
   while (intel_field_iterator_next(&iter)) {
      if (strcmp(iter.name, "Kernel Start Pointer") == 0) {
         } else if (strcmp(iter.name, "SIMD8 Dispatch Enable") == 0) {
         } else if (strcmp(iter.name, "Dispatch Mode") == 0) {
         } else if (strcmp(iter.name, "Dispatch Enable") == 0) {
         } else if (strcmp(iter.name, "Enable") == 0) {
                     const char *type =
      strcmp(inst->name,   "VS_STATE") == 0 ? "vertex shader" :
   strcmp(inst->name,   "GS_STATE") == 0 ? "geometry shader" :
   strcmp(inst->name,   "SF_STATE") == 0 ? "strips and fans shader" :
   strcmp(inst->name, "CLIP_STATE") == 0 ? "clip shader" :
   strcmp(inst->name, "3DSTATE_DS") == 0 ? "tessellation evaluation shader" :
   strcmp(inst->name, "3DSTATE_HS") == 0 ? "tessellation control shader" :
   strcmp(inst->name, "3DSTATE_VS") == 0 ? (is_simd8 ? "SIMD8 vertex shader" : "vec4 vertex shader") :
   strcmp(inst->name, "3DSTATE_GS") == 0 ? (is_simd8 ? "SIMD8 geometry shader" : "vec4 geometry shader") :
         if (is_enabled)
      }
      static void
   decode_ps_kernels(struct aub_viewer_decode_ctx *ctx,
               {
      uint64_t ksp[3] = {0, 0, 0};
            struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, inst, p, 0, false);
   while (intel_field_iterator_next(&iter)) {
      if (strncmp(iter.name, "Kernel Start Pointer ",
            int idx = iter.name[strlen("Kernel Start Pointer ")] - '0';
      } else if (strcmp(iter.name, "8 Pixel Dispatch Enable") == 0) {
         } else if (strcmp(iter.name, "16 Pixel Dispatch Enable") == 0) {
         } else if (strcmp(iter.name, "32 Pixel Dispatch Enable") == 0) {
                     /* Reorder KSPs to be [8, 16, 32] instead of the hardware order. */
   if (enabled[0] + enabled[1] + enabled[2] == 1) {
      if (enabled[1]) {
      ksp[1] = ksp[0];
      } else if (enabled[2]) {
      ksp[2] = ksp[0];
         } else {
      uint64_t tmp = ksp[1];
   ksp[1] = ksp[2];
               if (enabled[0])
         if (enabled[1])
         if (enabled[2])
      }
      static void
   decode_3dstate_constant(struct aub_viewer_decode_ctx *ctx,
               {
      struct intel_group *body =
            uint32_t read_length[4] = {0};
            struct intel_field_iterator outer;
   intel_field_iterator_init(&outer, inst, p, 0, false);
   while (intel_field_iterator_next(&outer)) {
      if (outer.struct_desc != body)
            struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, body, &outer.p[outer.start_bit / 32],
            while (intel_field_iterator_next(&iter)) {
      int idx;
   if (sscanf(iter.name, "Read Length[%d]", &idx) == 1) {
         } else if (sscanf(iter.name, "Buffer[%d]", &idx) == 1) {
                     for (int i = 0; i < 4; i++) {
                     struct intel_batch_decode_bo buffer = ctx_get_bo(ctx, true, read_addr[i]);
   if (!buffer.map) {
      ImGui::TextColored(ctx->cfg->missing_color,
                                          if (ctx->edit_address) {
      if (ImGui::Button("Show/Edit buffer"))
               }
      static void
   decode_3dstate_binding_table_pointers(struct aub_viewer_decode_ctx *ctx,
               {
         }
      static void
   decode_3dstate_sampler_state_pointers(struct aub_viewer_decode_ctx *ctx,
               {
         }
      static void
   decode_3dstate_sampler_state_pointers_gfx6(struct aub_viewer_decode_ctx *ctx,
               {
      dump_samplers(ctx, p[1], 1);
   dump_samplers(ctx, p[2], 1);
      }
      static bool
   str_ends_with(const char *str, const char *end)
   {
      int offset = strlen(str) - strlen(end);
   if (offset < 0)
               }
      static void
   decode_dynamic_state_pointers(struct aub_viewer_decode_ctx *ctx,
               {
               struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, inst, p, 0, false);
   while (intel_field_iterator_next(&iter)) {
      if (str_ends_with(iter.name, "Pointer")) {
      state_offset = iter.raw_value;
                  uint64_t state_addr = ctx->dynamic_base + state_offset;
   struct intel_batch_decode_bo bo = ctx_get_bo(ctx, true, state_addr);
            if (state_map == NULL) {
      ImGui::TextColored(ctx->cfg->missing_color,
                           struct intel_group *state = intel_spec_find_struct(ctx->spec, struct_type);
   if (strcmp(struct_type, "BLEND_STATE") == 0) {
      /* Blend states are different from the others because they have a header
   * struct called BLEND_STATE which is followed by a variable number of
   * BLEND_STATE_ENTRY structs.
   */
   ImGui::Text("%s", struct_type);
            state_addr += state->dw_length * 4;
            struct_type = "BLEND_STATE_ENTRY";
               for (int i = 0; i < count; i++) {
      ImGui::Text("%s %d", struct_type, i);
            state_addr += state->dw_length * 4;
         }
      static void
   decode_3dstate_viewport_state_pointers_cc(struct aub_viewer_decode_ctx *ctx,
               {
         }
      static void
   decode_3dstate_viewport_state_pointers_sf_clip(struct aub_viewer_decode_ctx *ctx,
               {
         }
      static void
   decode_3dstate_blend_state_pointers(struct aub_viewer_decode_ctx *ctx,
               {
         }
      static void
   decode_3dstate_cc_state_pointers(struct aub_viewer_decode_ctx *ctx,
               {
         }
      static void
   decode_3dstate_scissor_state_pointers(struct aub_viewer_decode_ctx *ctx,
               {
         }
      static void
   decode_load_register_imm(struct aub_viewer_decode_ctx *ctx,
               {
               if (reg != NULL &&
      ImGui::TreeNodeEx(&p[1], ImGuiTreeNodeFlags_Framed,
               aub_viewer_print_group(ctx, reg, reg->register_offset, &p[2]);
         }
      static void
   decode_3dprimitive(struct aub_viewer_decode_ctx *ctx,
               {
      if (ctx->display_urb) {
      if (ImGui::Button("Show URB"))
         }
      static void
   handle_urb(struct aub_viewer_decode_ctx *ctx,
               {
      struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, inst, p, 0, false);
   while (intel_field_iterator_next(&iter)) {
      if (strstr(iter.name, "URB Starting Address")) {
         } else if (strstr(iter.name, "URB Entry Allocation Size")) {
         } else if (strstr(iter.name, "Number of URB Entries")) {
                     ctx->end_urb_offset = MAX2(ctx->urb_stages[ctx->stage].start +
                  }
      static void
   handle_urb_read(struct aub_viewer_decode_ctx *ctx,
               {
      struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, inst, p, 0, false);
   while (intel_field_iterator_next(&iter)) {
      /* Workaround the "Force * URB Entry Read Length" fields */
   if (iter.end_bit - iter.start_bit < 2)
            if (strstr(iter.name, "URB Entry Read Offset")) {
         } else if (strstr(iter.name, "URB Entry Read Length")) {
         } else if (strstr(iter.name, "URB Entry Output Read Offset")) {
         } else if (strstr(iter.name, "URB Entry Output Length")) {
               }
      static void
   handle_urb_constant(struct aub_viewer_decode_ctx *ctx,
               {
      struct intel_group *body =
            struct intel_field_iterator outer;
   intel_field_iterator_init(&outer, inst, p, 0, false);
   while (intel_field_iterator_next(&outer)) {
      if (outer.struct_desc != body)
            struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, body, &outer.p[outer.start_bit / 32],
            ctx->urb_stages[ctx->stage].const_rd_length = 0;
   while (intel_field_iterator_next(&iter)) {
      int idx;
   if (sscanf(iter.name, "Read Length[%d]", &idx) == 1) {
                  }
      struct custom_decoder {
      const char *cmd_name;
   void (*decode)(struct aub_viewer_decode_ctx *ctx,
                  } display_decoders[] = {
      { "STATE_BASE_ADDRESS", handle_state_base_address },
   { "MEDIA_INTERFACE_DESCRIPTOR_LOAD", handle_media_interface_descriptor_load },
   { "3DSTATE_VERTEX_BUFFERS", handle_3dstate_vertex_buffers },
   { "3DSTATE_INDEX_BUFFER", handle_3dstate_index_buffer },
   { "3DSTATE_VS", decode_single_ksp, AUB_DECODE_STAGE_VS, },
   { "3DSTATE_GS", decode_single_ksp, AUB_DECODE_STAGE_GS, },
   { "3DSTATE_DS", decode_single_ksp, AUB_DECODE_STAGE_DS, },
   { "3DSTATE_HS", decode_single_ksp, AUB_DECODE_STAGE_HS, },
   { "3DSTATE_PS", decode_ps_kernels, AUB_DECODE_STAGE_PS, },
   { "3DSTATE_CONSTANT_VS", decode_3dstate_constant, AUB_DECODE_STAGE_VS, },
   { "3DSTATE_CONSTANT_GS", decode_3dstate_constant, AUB_DECODE_STAGE_GS, },
   { "3DSTATE_CONSTANT_DS", decode_3dstate_constant, AUB_DECODE_STAGE_DS, },
   { "3DSTATE_CONSTANT_HS", decode_3dstate_constant, AUB_DECODE_STAGE_HS, },
            { "3DSTATE_BINDING_TABLE_POINTERS_VS", decode_3dstate_binding_table_pointers, AUB_DECODE_STAGE_VS, },
   { "3DSTATE_BINDING_TABLE_POINTERS_GS", decode_3dstate_binding_table_pointers, AUB_DECODE_STAGE_GS, },
   { "3DSTATE_BINDING_TABLE_POINTERS_HS", decode_3dstate_binding_table_pointers, AUB_DECODE_STAGE_HS, },
   { "3DSTATE_BINDING_TABLE_POINTERS_DS", decode_3dstate_binding_table_pointers, AUB_DECODE_STAGE_DS, },
            { "3DSTATE_SAMPLER_STATE_POINTERS_VS", decode_3dstate_sampler_state_pointers, AUB_DECODE_STAGE_VS, },
   { "3DSTATE_SAMPLER_STATE_POINTERS_GS", decode_3dstate_sampler_state_pointers, AUB_DECODE_STAGE_GS, },
   { "3DSTATE_SAMPLER_STATE_POINTERS_DS", decode_3dstate_sampler_state_pointers, AUB_DECODE_STAGE_DS, },
   { "3DSTATE_SAMPLER_STATE_POINTERS_HS", decode_3dstate_sampler_state_pointers, AUB_DECODE_STAGE_HS, },
   { "3DSTATE_SAMPLER_STATE_POINTERS_PS", decode_3dstate_sampler_state_pointers, AUB_DECODE_STAGE_PS, },
            { "3DSTATE_VIEWPORT_STATE_POINTERS_CC", decode_3dstate_viewport_state_pointers_cc },
   { "3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP", decode_3dstate_viewport_state_pointers_sf_clip },
   { "3DSTATE_BLEND_STATE_POINTERS", decode_3dstate_blend_state_pointers },
   { "3DSTATE_CC_STATE_POINTERS", decode_3dstate_cc_state_pointers },
   { "3DSTATE_SCISSOR_STATE_POINTERS", decode_3dstate_scissor_state_pointers },
   { "MI_LOAD_REGISTER_IMM", decode_load_register_imm },
      };
      struct custom_decoder info_decoders[] = {
      { "STATE_BASE_ADDRESS", handle_state_base_address },
   { "3DSTATE_URB_VS", handle_urb, AUB_DECODE_STAGE_VS, },
   { "3DSTATE_URB_GS", handle_urb, AUB_DECODE_STAGE_GS, },
   { "3DSTATE_URB_DS", handle_urb, AUB_DECODE_STAGE_DS, },
   { "3DSTATE_URB_HS", handle_urb, AUB_DECODE_STAGE_HS, },
   { "3DSTATE_VS", handle_urb_read, AUB_DECODE_STAGE_VS, },
   { "3DSTATE_GS", handle_urb_read, AUB_DECODE_STAGE_GS, },
   { "3DSTATE_DS", handle_urb_read, AUB_DECODE_STAGE_DS, },
   { "3DSTATE_HS", handle_urb_read, AUB_DECODE_STAGE_HS, },
   { "3DSTATE_PS", handle_urb_read, AUB_DECODE_STAGE_PS, },
   { "3DSTATE_CONSTANT_VS", handle_urb_constant, AUB_DECODE_STAGE_VS, },
   { "3DSTATE_CONSTANT_GS", handle_urb_constant, AUB_DECODE_STAGE_GS, },
   { "3DSTATE_CONSTANT_DS", handle_urb_constant, AUB_DECODE_STAGE_DS, },
   { "3DSTATE_CONSTANT_HS", handle_urb_constant, AUB_DECODE_STAGE_HS, },
      };
      void
   aub_viewer_render_batch(struct aub_viewer_decode_ctx *ctx,
               {
      struct intel_group *inst;
   const uint32_t *p, *batch = (const uint32_t *) _batch, *end = batch + batch_size / sizeof(uint32_t);
            if (ctx->n_batch_buffer_start >= 100) {
      ImGui::TextColored(ctx->cfg->error_color,
                              for (p = batch; p < end; p += length) {
      inst = intel_spec_find_instruction(ctx->spec, ctx->engine, p);
   length = intel_group_get_length(inst, p);
   assert(inst == NULL || length > 0);
                     if (inst == NULL) {
      ImGui::TextColored(ctx->cfg->error_color,
                                    for (unsigned i = 0; i < ARRAY_SIZE(info_decoders); i++) {
      if (strcmp(inst_name, info_decoders[i].cmd_name) == 0) {
      ctx->stage = info_decoders[i].stage;
   info_decoders[i].decode(ctx, inst, p);
                  if (ctx->decode_cfg->command_filter.PassFilter(inst->name) &&
      ImGui::TreeNodeEx(p,
                              for (unsigned i = 0; i < ARRAY_SIZE(display_decoders); i++) {
      if (strcmp(inst_name, display_decoders[i].cmd_name) == 0) {
      ctx->stage = display_decoders[i].stage;
   display_decoders[i].decode(ctx, inst, p);
                  if (ctx->edit_address) {
      if (ImGui::Button("Edit instruction"))
                           if (strcmp(inst_name, "MI_BATCH_BUFFER_START") == 0) {
      uint64_t next_batch_addr = 0xd0d0d0d0;
   bool ppgtt = false;
   bool second_level = false;
   struct intel_field_iterator iter;
   intel_field_iterator_init(&iter, inst, p, 0, false);
   while (intel_field_iterator_next(&iter)) {
      if (strcmp(iter.name, "Batch Buffer Start Address") == 0) {
         } else if (strcmp(iter.name, "Second Level Batch Buffer") == 0) {
         } else if (strcmp(iter.name, "Address Space Indicator") == 0) {
                              if (next_batch.map == NULL) {
      ImGui::TextColored(ctx->cfg->missing_color,
            } else {
      aub_viewer_render_batch(ctx, next_batch.map, next_batch.size,
      }
   if (second_level) {
      /* MI_BATCH_BUFFER_START with "2nd Level Batch Buffer" set acts
   * like a subroutine call.  Commands that come afterwards get
   * processed once the 2nd level batch buffer returns with
   * MI_BATCH_BUFFER_END.
   */
      } else if (!from_ring) {
      /* MI_BATCH_BUFFER_START with "2nd Level Batch Buffer" unset acts
   * like a goto.  Nothing after it will ever get processed.  In
   * order to prevent the recursion from growing, we just reset the
   * loop and continue;
   */
         } else if (strcmp(inst_name, "MI_BATCH_BUFFER_END") == 0) {
                        }
