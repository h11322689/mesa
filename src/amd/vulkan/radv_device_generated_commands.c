   /*
   * Copyright © 2021 Google
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
      #include "meta/radv_meta.h"
   #include "radv_private.h"
      #include "ac_rgp.h"
      #include "nir_builder.h"
      #include "vk_common_entrypoints.h"
      static void
   radv_get_sequence_size_compute(const struct radv_indirect_command_layout *layout,
         {
      const struct radv_device *device = container_of(layout->base.device, struct radv_device, vk);
            /* dispatch */
            const struct radv_userdata_info *loc = radv_get_user_sgpr(cs, AC_UD_CS_GRID_SIZE);
   if (loc->sgpr_idx != -1) {
      if (device->load_grid_size_from_user_sgpr) {
      /* PKT3_SET_SH_REG for immediate values */
      } else {
      /* PKT3_SET_SH_REG for pointer */
                  if (device->sqtt.bo) {
      /* sqtt markers */
         }
      static void
   radv_get_sequence_size_graphics(const struct radv_indirect_command_layout *layout,
               {
      const struct radv_device *device = container_of(layout->base.device, struct radv_device, vk);
            if (layout->bind_vbo_mask) {
               /* One PKT3_SET_SH_REG for emitting VBO pointer (32-bit) */
               if (layout->binds_index_buffer) {
      /* Index type write (normal reg write) + index buffer base write (64-bits, but special packet
   * so only 1 word overhead) + index buffer size (again, special packet so only 1 word
   * overhead)
   */
               if (layout->indexed) {
      /* userdata writes + instance count + indexed draw */
      } else {
      /* userdata writes + instance count + non-indexed draw */
               if (device->sqtt.bo) {
      /* sqtt markers */
         }
      static void
   radv_get_sequence_size(const struct radv_indirect_command_layout *layout, struct radv_pipeline *pipeline,
         {
               *cmd_size = 0;
            if (layout->push_constant_mask) {
               for (unsigned i = 0; i < ARRAY_SIZE(pipeline->shaders); ++i) {
                     struct radv_userdata_locations *locs = &pipeline->shaders[i]->info.user_sgprs_locs;
   if (locs->shader_data[AC_UD_PUSH_CONSTANTS].sgpr_idx >= 0) {
      /* One PKT3_SET_SH_REG for emitting push constants pointer (32-bit) */
   *cmd_size += 3 * 4;
      }
   if (locs->shader_data[AC_UD_INLINE_PUSH_CONSTANTS].sgpr_idx >= 0)
      /* One PKT3_SET_SH_REG writing all inline push constants. */
   }
   if (need_copy)
               if (device->sqtt.bo) {
      /* THREAD_TRACE_MARKER */
               if (layout->pipeline_bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS) {
      struct radv_graphics_pipeline *graphics_pipeline = radv_pipeline_to_graphics(pipeline);
      } else {
      assert(layout->pipeline_bind_point == VK_PIPELINE_BIND_POINT_COMPUTE);
   struct radv_compute_pipeline *compute_pipeline = radv_pipeline_to_compute(pipeline);
         }
      static uint32_t
   radv_align_cmdbuf_size(const struct radv_device *device, uint32_t size, enum amd_ip_type ip_type)
   {
                  }
      static unsigned
   radv_dgc_preamble_cmdbuf_size(const struct radv_device *device)
   {
         }
      static bool
   radv_dgc_use_preamble(const VkGeneratedCommandsInfoNV *cmd_info)
   {
      /* Heuristic on when the overhead for the preamble (i.e. double jump) is worth it. Obviously
   * a bit of a guess as it depends on the actual count which we don't know. */
      }
      uint32_t
   radv_get_indirect_cmdbuf_size(const VkGeneratedCommandsInfoNV *cmd_info)
   {
      VK_FROM_HANDLE(radv_indirect_command_layout, layout, cmd_info->indirectCommandsLayout);
   VK_FROM_HANDLE(radv_pipeline, pipeline, cmd_info->pipeline);
            if (radv_dgc_use_preamble(cmd_info))
            uint32_t cmd_size, upload_size;
   radv_get_sequence_size(layout, pipeline, &cmd_size, &upload_size);
      }
      struct radv_dgc_params {
      uint32_t cmd_buf_stride;
   uint32_t cmd_buf_size;
   uint32_t upload_stride;
   uint32_t upload_addr;
   uint32_t sequence_count;
   uint32_t stream_stride;
            /* draw info */
   uint16_t draw_indexed;
   uint16_t draw_params_offset;
   uint16_t base_index_size;
   uint16_t vtx_base_sgpr;
            /* dispatch info */
   uint32_t dispatch_initiator;
   uint16_t dispatch_params_offset;
            /* bind index buffer info. Valid if base_index_size == 0 && draw_indexed */
                              /* Which VBOs are set in this indirect layout. */
            uint16_t vbo_reg;
                     uint32_t ibo_type_32;
                     uint8_t is_dispatch;
      };
      enum {
      DGC_USES_DRAWID = 1u << 14,
      };
      enum {
         };
      enum {
      DGC_DESC_STREAM,
   DGC_DESC_PREPARE,
   DGC_DESC_PARAMS,
   DGC_DESC_COUNT,
      };
      struct dgc_cmdbuf {
      nir_def *descriptor;
            enum amd_gfx_level gfx_level;
      };
      static void
   dgc_emit(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *value)
   {
      assert(value->bit_size >= 32);
   nir_def *offset = nir_load_var(b, cs->offset);
   nir_store_ssbo(b, value, cs->descriptor, offset, .access = ACCESS_NON_READABLE);
      }
      #define load_param32(b, field)                                                                                         \
            #define load_param16(b, field)                                                                                         \
      nir_ubfe_imm((b),                                                                                                   \
                     #define load_param8(b, field)                                                                                          \
      nir_ubfe_imm((b),                                                                                                   \
                     #define load_param64(b, field)                                                                                         \
      nir_pack_64_2x32((b), nir_load_push_constant((b), 2, 32, nir_imm_int((b), 0),                                       \
         static nir_def *
   nir_pkt3_base(nir_builder *b, unsigned op, nir_def *len, bool predicate)
   {
      len = nir_iand_imm(b, len, 0x3fff);
      }
      static nir_def *
   nir_pkt3(nir_builder *b, unsigned op, nir_def *len)
   {
         }
      static nir_def *
   dgc_get_nop_packet(nir_builder *b, const struct radv_device *device)
   {
      if (device->physical_device->rad_info.gfx_ib_pad_with_type2) {
         } else {
            }
      static void
   dgc_emit_userdata_vertex(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *vtx_base_sgpr, nir_def *first_vertex,
         {
      vtx_base_sgpr = nir_u2u32(b, vtx_base_sgpr);
   nir_def *has_drawid = nir_test_mask(b, vtx_base_sgpr, DGC_USES_DRAWID);
            nir_def *pkt_cnt = nir_imm_int(b, 1);
   pkt_cnt = nir_bcsel(b, has_drawid, nir_iadd_imm(b, pkt_cnt, 1), pkt_cnt);
            nir_def *values[5] = {
      nir_pkt3(b, PKT3_SET_SH_REG, pkt_cnt), nir_iand_imm(b, vtx_base_sgpr, 0x3FFF), first_vertex,
               values[3] = nir_bcsel(b, nir_ior(b, has_drawid, has_baseinstance), nir_bcsel(b, has_drawid, drawid, first_instance),
                     }
      static void
   dgc_emit_sqtt_userdata(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *data)
   {
      if (!cs->sqtt_enabled)
            nir_def *values[3] = {
      nir_pkt3_base(b, PKT3_SET_UCONFIG_REG, nir_imm_int(b, 1), cs->gfx_level >= GFX10),
   nir_imm_int(b, (R_030D08_SQ_THREAD_TRACE_USERDATA_2 - CIK_UCONFIG_REG_OFFSET) >> 2),
                  }
      static void
   dgc_emit_sqtt_thread_trace_marker(nir_builder *b, struct dgc_cmdbuf *cs)
   {
      if (!cs->sqtt_enabled)
            nir_def *values[2] = {
      nir_pkt3(b, PKT3_EVENT_WRITE, nir_imm_int(b, 0)),
                  }
      static void
   dgc_emit_sqtt_marker_event(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *sequence_id,
         {
               marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_EVENT;
            dgc_emit_sqtt_userdata(b, cs, nir_imm_int(b, marker.dword01));
   dgc_emit_sqtt_userdata(b, cs, nir_imm_int(b, marker.dword02));
      }
      static void
   dgc_emit_sqtt_marker_event_with_dims(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *sequence_id, nir_def *x,
         {
               marker.event.identifier = RGP_SQTT_MARKER_IDENTIFIER_EVENT;
   marker.event.api_type = event;
            dgc_emit_sqtt_userdata(b, cs, nir_imm_int(b, marker.event.dword01));
   dgc_emit_sqtt_userdata(b, cs, nir_imm_int(b, marker.event.dword02));
   dgc_emit_sqtt_userdata(b, cs, sequence_id);
   dgc_emit_sqtt_userdata(b, cs, x);
   dgc_emit_sqtt_userdata(b, cs, y);
      }
      static void
   dgc_emit_sqtt_begin_api_marker(nir_builder *b, struct dgc_cmdbuf *cs, enum rgp_sqtt_marker_general_api_type api_type)
   {
               marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_GENERAL_API;
               }
      static void
   dgc_emit_sqtt_end_api_marker(nir_builder *b, struct dgc_cmdbuf *cs, enum rgp_sqtt_marker_general_api_type api_type)
   {
               marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_GENERAL_API;
   marker.api_type = api_type;
               }
      static void
   dgc_emit_instance_count(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *instance_count)
   {
                  }
      static void
   dgc_emit_draw_index_offset_2(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *index_offset, nir_def *index_count,
         {
      nir_def *values[5] = {nir_imm_int(b, PKT3(PKT3_DRAW_INDEX_OFFSET_2, 3, false)), max_index_count, index_offset,
               }
      static void
   dgc_emit_draw_index_auto(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *vertex_count)
   {
      nir_def *values[3] = {nir_imm_int(b, PKT3(PKT3_DRAW_INDEX_AUTO, 1, false)), vertex_count,
               }
      static void
   dgc_emit_dispatch_direct(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *wg_x, nir_def *wg_y, nir_def *wg_z,
         {
      nir_def *values[5] = {nir_imm_int(b, PKT3(PKT3_DISPATCH_DIRECT, 3, false) | PKT3_SHADER_TYPE_S(1)), wg_x, wg_y, wg_z,
               }
      static void
   dgc_emit_grid_size_user_sgpr(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *grid_base_sgpr, nir_def *wg_x,
         {
      nir_def *values[5] = {
                     }
      static void
   dgc_emit_grid_size_pointer(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *grid_base_sgpr, nir_def *stream_offset)
   {
      nir_def *stream_addr = load_param64(b, stream_addr);
            nir_def *va_lo = nir_unpack_64_2x32_split_x(b, va);
                        }
      static nir_def *
   dgc_cmd_buf_size(nir_builder *b, nir_def *sequence_count, const struct radv_device *device)
   {
      nir_def *use_preamble = nir_ine_imm(b, load_param8(b, use_preamble), 0);
   nir_def *cmd_buf_size = load_param32(b, cmd_buf_size);
   nir_def *cmd_buf_stride = load_param32(b, cmd_buf_stride);
   nir_def *size = nir_imul(b, cmd_buf_stride, sequence_count);
                     /* Ensure we don't have to deal with a jump to an empty IB in the preamble. */
               }
      static nir_def *
   dgc_main_cmd_buf_offset(nir_builder *b, const struct radv_device *device)
   {
      nir_def *use_preamble = nir_ine_imm(b, load_param8(b, use_preamble), 0);
   nir_def *base_offset = nir_imm_int(b, radv_dgc_preamble_cmdbuf_size(device));
      }
      static void
   build_dgc_buffer_tail(nir_builder *b, nir_def *sequence_count, const struct radv_device *device)
   {
               nir_def *cmd_buf_stride = load_param32(b, cmd_buf_stride);
            nir_push_if(b, nir_ieq_imm(b, global_id, 0));
   {
      nir_def *base_offset = dgc_main_cmd_buf_offset(b, device);
            nir_variable *offset = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint_type(), "offset");
            nir_def *dst_buf = radv_meta_load_descriptor(b, 0, DGC_DESC_PREPARE);
   nir_push_loop(b);
   {
                     nir_push_if(b, nir_ieq(b, curr_offset, cmd_buf_size));
   {
                                 if (device->physical_device->rad_info.gfx_ib_pad_with_type2) {
      packet_size = nir_imm_int(b, 4);
      } else {
                     nir_def *len = nir_ushr_imm(b, packet_size, 2);
   len = nir_iadd_imm(b, len, -2);
               nir_store_ssbo(b, packet, dst_buf, nir_iadd(b, curr_offset, base_offset), .access = ACCESS_NON_READABLE);
      }
      }
      }
      static void
   build_dgc_buffer_preamble(nir_builder *b, nir_def *sequence_count, const struct radv_device *device)
   {
      nir_def *global_id = get_global_ids(b, 1);
            nir_push_if(b, nir_iand(b, nir_ieq_imm(b, global_id, 0), use_preamble));
   {
      unsigned preamble_size = radv_dgc_preamble_cmdbuf_size(device);
   nir_def *cmd_buf_size = dgc_cmd_buf_size(b, sequence_count, device);
                                       nir_def *nop_packets[] = {
      nop_packet,
   nop_packet,
   nop_packet,
               const unsigned jump_size = 16;
            /* Do vectorized store if possible */
   for (offset = 0; offset + 16 <= preamble_size - jump_size; offset += 16) {
                  for (; offset + 4 <= preamble_size - jump_size; offset += 4) {
                  nir_def *chain_packets[] = {
      nir_imm_int(b, PKT3(PKT3_INDIRECT_BUFFER, 2, 0)),
   addr,
   nir_imm_int(b, device->physical_device->rad_info.address32_hi),
               nir_store_ssbo(b, nir_vec(b, chain_packets, 4), dst_buf, nir_imm_int(b, preamble_size - jump_size),
      }
      }
      /**
   * Emit VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NV.
   */
   static void
   dgc_emit_draw(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *stream_buf, nir_def *stream_base,
         {
      nir_def *vtx_base_sgpr = load_param16(b, vtx_base_sgpr);
            nir_def *draw_data0 = nir_load_ssbo(b, 4, 32, stream_buf, stream_offset);
   nir_def *vertex_count = nir_channel(b, draw_data0, 0);
   nir_def *instance_count = nir_channel(b, draw_data0, 1);
   nir_def *vertex_offset = nir_channel(b, draw_data0, 2);
            nir_push_if(b, nir_iand(b, nir_ine_imm(b, vertex_count, 0), nir_ine_imm(b, instance_count, 0)));
   {
      dgc_emit_sqtt_begin_api_marker(b, cs, ApiCmdDraw);
            dgc_emit_userdata_vertex(b, cs, vtx_base_sgpr, vertex_offset, first_instance, sequence_id, device);
   dgc_emit_instance_count(b, cs, instance_count);
            dgc_emit_sqtt_thread_trace_marker(b, cs);
      }
      }
      /**
   * Emit VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV.
   */
   static void
   dgc_emit_draw_indexed(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *stream_buf, nir_def *stream_base,
               {
      nir_def *vtx_base_sgpr = load_param16(b, vtx_base_sgpr);
            nir_def *draw_data0 = nir_load_ssbo(b, 4, 32, stream_buf, stream_offset);
   nir_def *draw_data1 = nir_load_ssbo(b, 1, 32, stream_buf, nir_iadd_imm(b, stream_offset, 16));
   nir_def *index_count = nir_channel(b, draw_data0, 0);
   nir_def *instance_count = nir_channel(b, draw_data0, 1);
   nir_def *first_index = nir_channel(b, draw_data0, 2);
   nir_def *vertex_offset = nir_channel(b, draw_data0, 3);
            nir_push_if(b, nir_iand(b, nir_ine_imm(b, index_count, 0), nir_ine_imm(b, instance_count, 0)));
   {
      dgc_emit_sqtt_begin_api_marker(b, cs, ApiCmdDrawIndexed);
            dgc_emit_userdata_vertex(b, cs, vtx_base_sgpr, vertex_offset, first_instance, sequence_id, device);
   dgc_emit_instance_count(b, cs, instance_count);
            dgc_emit_sqtt_thread_trace_marker(b, cs);
      }
      }
      /**
   * Emit VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_NV.
   */
   static void
   dgc_emit_index_buffer(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *stream_buf, nir_def *stream_base,
               {
      nir_def *index_stream_offset = nir_iadd(b, index_buffer_offset, stream_base);
            nir_def *vk_index_type = nir_channel(b, data, 3);
   nir_def *index_type = nir_bcsel(b, nir_ieq(b, vk_index_type, ibo_type_32), nir_imm_int(b, V_028A7C_VGT_INDEX_32),
                  nir_def *index_size = nir_iand_imm(b, nir_ushr(b, nir_imm_int(b, 0x142), nir_imul_imm(b, index_type, 4)), 0xf);
            nir_def *max_index_count = nir_udiv(b, nir_channel(b, data, 2), index_size);
                     if (device->physical_device->rad_info.gfx_level >= GFX9) {
      unsigned opcode = PKT3_SET_UCONFIG_REG_INDEX;
   if (device->physical_device->rad_info.gfx_level < GFX9 ||
      (device->physical_device->rad_info.gfx_level == GFX9 && device->physical_device->rad_info.me_fw_version < 26))
      cmd_values[0] = nir_imm_int(b, PKT3(opcode, 1, 0));
   cmd_values[1] = nir_imm_int(b, (R_03090C_VGT_INDEX_TYPE - CIK_UCONFIG_REG_OFFSET) >> 2 | (2u << 28));
      } else {
      cmd_values[0] = nir_imm_int(b, PKT3(PKT3_INDEX_TYPE, 0, 0));
   cmd_values[1] = index_type;
               nir_def *addr_upper = nir_channel(b, data, 1);
            cmd_values[3] = nir_imm_int(b, PKT3(PKT3_INDEX_BASE, 1, 0));
   cmd_values[4] = nir_channel(b, data, 0);
   cmd_values[5] = addr_upper;
   cmd_values[6] = nir_imm_int(b, PKT3(PKT3_INDEX_BUFFER_SIZE, 0, 0));
               }
      /**
   * Emit VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NV.
   */
   static void
   dgc_emit_push_constant(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *stream_buf, nir_def *stream_base,
         {
      nir_def *vbo_cnt = load_param8(b, vbo_cnt);
   nir_def *const_copy = nir_ine_imm(b, load_param8(b, const_copy), 0);
   nir_def *const_copy_size = load_param16(b, const_copy_size);
   nir_def *const_copy_words = nir_ushr_imm(b, const_copy_size, 2);
            nir_variable *idx = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint_type(), "const_copy_idx");
            nir_def *param_buf = radv_meta_load_descriptor(b, 0, DGC_DESC_PARAMS);
   nir_def *param_offset = nir_imul_imm(b, vbo_cnt, 24);
   nir_def *param_offset_offset = nir_iadd_imm(b, param_offset, MESA_VULKAN_SHADER_STAGES * 12);
   nir_def *param_const_offset =
         nir_push_loop(b);
   {
      nir_def *cur_idx = nir_load_var(b, idx);
   nir_push_if(b, nir_uge(b, cur_idx, const_copy_words));
   {
         }
                     nir_def *update = nir_iand(b, push_const_mask, nir_ishl(b, nir_imm_int64(b, 1), cur_idx));
            nir_push_if(b, nir_ine_imm(b, update, 0));
   {
      nir_def *stream_offset =
         nir_def *new_data = nir_load_ssbo(b, 1, 32, stream_buf, nir_iadd(b, stream_base, stream_offset));
      }
   nir_push_else(b, NULL);
   {
      nir_store_var(b, data,
            }
            nir_store_ssbo(b, nir_load_var(b, data), cs->descriptor,
                     }
            nir_variable *shader_idx = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint_type(), "shader_idx");
   nir_store_var(b, shader_idx, nir_imm_int(b, 0), 0x1);
            nir_push_loop(b);
   {
      nir_def *cur_shader_idx = nir_load_var(b, shader_idx);
   nir_push_if(b, nir_uge(b, cur_shader_idx, shader_cnt));
   {
         }
            nir_def *reg_info =
         nir_def *upload_sgpr = nir_ubfe_imm(b, nir_channel(b, reg_info, 0), 0, 16);
   nir_def *inline_sgpr = nir_ubfe_imm(b, nir_channel(b, reg_info, 0), 16, 16);
            nir_push_if(b, nir_ine_imm(b, upload_sgpr, 0));
   {
                        }
            nir_push_if(b, nir_ine_imm(b, inline_sgpr, 0));
   {
                                       nir_push_loop(b);
   {
      nir_def *cur_idx = nir_load_var(b, idx);
   nir_push_if(b, nir_uge_imm(b, cur_idx, 64 /* bits in inline_mask */));
   {
                        nir_def *l = nir_ishl(b, nir_imm_int64(b, 1), cur_idx);
   nir_push_if(b, nir_ieq_imm(b, nir_iand(b, l, inline_mask), 0));
   {
      nir_store_var(b, idx, nir_iadd_imm(b, cur_idx, 1), 0x1);
                              nir_def *update = nir_iand(b, push_const_mask, nir_ishl(b, nir_imm_int64(b, 1), cur_idx));
                  nir_push_if(b, nir_ine_imm(b, update, 0));
   {
      nir_def *stream_offset =
         nir_def *new_data = nir_load_ssbo(b, 1, 32, stream_buf, nir_iadd(b, stream_base, stream_offset));
      }
   nir_push_else(b, NULL);
   {
      nir_store_var(
      b, data,
   nir_load_ssbo(b, 1, 32, param_buf, nir_iadd(b, param_const_offset, nir_ishl_imm(b, cur_idx, 2))),
                              }
      }
   nir_pop_if(b, NULL);
      }
      }
      /**
   * For emitting VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_NV.
   */
   static void
   dgc_emit_vertex_buffer(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *stream_buf, nir_def *stream_base,
         {
      nir_def *vbo_cnt = load_param8(b, vbo_cnt);
   nir_variable *vbo_idx = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint_type(), "vbo_idx");
            nir_push_loop(b);
   {
      nir_push_if(b, nir_uge(b, nir_load_var(b, vbo_idx), vbo_cnt));
   {
         }
            nir_def *vbo_offset = nir_imul_imm(b, nir_load_var(b, vbo_idx), 16);
            nir_def *param_buf = radv_meta_load_descriptor(b, 0, DGC_DESC_PARAMS);
            nir_def *vbo_override =
         nir_push_if(b, vbo_override);
   {
      nir_def *vbo_offset_offset =
         nir_def *vbo_over_data = nir_load_ssbo(b, 2, 32, param_buf, vbo_offset_offset);
                  nir_def *va = nir_pack_64_2x32(b, nir_trim_vector(b, stream_data, 2));
                  nir_def *dyn_stride = nir_test_mask(b, nir_channel(b, vbo_over_data, 0), DGC_DYNAMIC_STRIDE);
                  nir_def *use_per_attribute_vb_descs = nir_test_mask(b, nir_channel(b, vbo_over_data, 0), 1u << 31);
   nir_variable *num_records =
                  nir_push_if(b, use_per_attribute_vb_descs);
   {
                     nir_push_if(b, nir_ult(b, nir_load_var(b, num_records), attrib_end));
   {
         }
   nir_push_else(b, NULL);
   nir_push_if(b, nir_ieq_imm(b, stride, 0));
   {
         }
   nir_push_else(b, NULL);
   {
      nir_def *r = nir_iadd(
      b, nir_iadd_imm(b, nir_udiv(b, nir_isub(b, nir_load_var(b, num_records), attrib_end), stride), 1),
         }
                  nir_def *convert_cond = nir_ine_imm(b, nir_load_var(b, num_records), 0);
   if (device->physical_device->rad_info.gfx_level == GFX9)
                        nir_def *new_records =
         new_records = nir_bcsel(b, convert_cond, new_records, nir_load_var(b, num_records));
      }
   nir_push_else(b, NULL);
   {
      if (device->physical_device->rad_info.gfx_level != GFX8) {
      nir_push_if(b, nir_ine_imm(b, stride, 0));
   {
      nir_def *r = nir_iadd(b, nir_load_var(b, num_records), nir_iadd_imm(b, stride, -1));
      }
                        nir_def *rsrc_word3 = nir_channel(b, nir_load_var(b, vbo_data), 3);
   if (device->physical_device->rad_info.gfx_level >= GFX10) {
      nir_def *oob_select = nir_bcsel(b, nir_ieq_imm(b, stride, 0), nir_imm_int(b, V_008F0C_OOB_SELECT_RAW),
         rsrc_word3 = nir_iand_imm(b, rsrc_word3, C_008F0C_OOB_SELECT);
               nir_def *va_hi = nir_iand_imm(b, nir_unpack_64_2x32_split_y(b, va), 0xFFFF);
   stride = nir_iand_imm(b, stride, 0x3FFF);
   nir_def *new_vbo_data[4] = {nir_unpack_64_2x32_split_x(b, va), nir_ior(b, nir_ishl_imm(b, stride, 16), va_hi),
            }
            /* On GFX9, it seems bounds checking is disabled if both
   * num_records and stride are zero. This doesn't seem necessary on GFX8, GFX10 and
   * GFX10.3 but it doesn't hurt.
   */
   nir_def *num_records = nir_channel(b, nir_load_var(b, vbo_data), 2);
   nir_def *buf_va =
         nir_push_if(b, nir_ior(b, nir_ieq_imm(b, num_records, 0), nir_ieq_imm(b, buf_va, 0)));
   {
      nir_def *new_vbo_data[4] = {nir_imm_int(b, 0), nir_imm_int(b, 0), nir_imm_int(b, 0), nir_imm_int(b, 0)};
      }
            nir_def *upload_off = nir_iadd(b, nir_load_var(b, upload_offset), vbo_offset);
   nir_store_ssbo(b, nir_load_var(b, vbo_data), cs->descriptor, upload_off, .access = ACCESS_NON_READABLE);
      }
   nir_pop_loop(b, NULL);
   nir_def *packet[3] = {nir_imm_int(b, PKT3(PKT3_SET_SH_REG, 1, 0)), load_param16(b, vbo_reg),
                        }
      /**
   * For emitting VK_INDIRECT_COMMANDS_TOKEN_TYPE_DISPATCH_NV.
   */
   static void
   dgc_emit_dispatch(nir_builder *b, struct dgc_cmdbuf *cs, nir_def *stream_buf, nir_def *stream_base,
         {
               nir_def *dispatch_data = nir_load_ssbo(b, 3, 32, stream_buf, stream_offset);
   nir_def *wg_x = nir_channel(b, dispatch_data, 0);
   nir_def *wg_y = nir_channel(b, dispatch_data, 1);
            nir_def *grid_sgpr = load_param16(b, grid_base_sgpr);
   nir_push_if(b, nir_ine_imm(b, grid_sgpr, 0));
   {
      if (device->load_grid_size_from_user_sgpr) {
         } else {
            }
            nir_push_if(b, nir_iand(b, nir_ine_imm(b, wg_x, 0), nir_iand(b, nir_ine_imm(b, wg_y, 0), nir_ine_imm(b, wg_z, 0))));
   {
      dgc_emit_sqtt_begin_api_marker(b, cs, ApiCmdDispatch);
                     dgc_emit_sqtt_thread_trace_marker(b, cs);
      }
      }
      static nir_shader *
   build_dgc_prepare_shader(struct radv_device *dev)
   {
      nir_builder b = radv_meta_init_shader(dev, MESA_SHADER_COMPUTE, "meta_dgc_prepare");
                              nir_def *cmd_buf_stride = load_param32(&b, cmd_buf_stride);
   nir_def *sequence_count = load_param32(&b, sequence_count);
            nir_def *use_count = nir_iand_imm(&b, sequence_count, 1u << 31);
                     /* The effective number of draws is
   * min(sequencesCount, sequencesCountBuffer[sequencesCountOffset]) when
   * using sequencesCountBuffer. Otherwise it is sequencesCount. */
   nir_variable *count_var = nir_variable_create(b.shader, nir_var_shader_temp, glsl_uint_type(), "sequence_count");
            nir_push_if(&b, nir_ine_imm(&b, use_count, 0));
   {
      nir_def *count_buf = radv_meta_load_descriptor(&b, 0, DGC_DESC_COUNT);
   nir_def *cnt = nir_load_ssbo(&b, 1, 32, count_buf, nir_imm_int(&b, 0));
   /* Must clamp count against the API count explicitly.
   * The workgroup potentially contains more threads than maxSequencesCount from API,
   * and we have to ensure these threads write NOP packets to pad out the IB. */
   cnt = nir_umin(&b, cnt, sequence_count);
      }
                     nir_push_if(&b, nir_ult(&b, sequence_id, sequence_count));
   {
      struct dgc_cmdbuf cmd_buf = {
      .descriptor = radv_meta_load_descriptor(&b, 0, DGC_DESC_PREPARE),
   .offset = nir_variable_create(b.shader, nir_var_shader_temp, glsl_uint_type(), "cmd_buf_offset"),
   .gfx_level = dev->physical_device->rad_info.gfx_level,
      };
   nir_store_var(&b, cmd_buf.offset, nir_iadd(&b, nir_imul(&b, global_id, cmd_buf_stride), cmd_buf_base_offset), 1);
            nir_def *stream_buf = radv_meta_load_descriptor(&b, 0, DGC_DESC_STREAM);
            nir_variable *upload_offset =
         nir_def *upload_offset_init = nir_iadd(&b, nir_iadd(&b, load_param32(&b, cmd_buf_size), cmd_buf_base_offset),
                  nir_def *vbo_bind_mask = load_param32(&b, vbo_bind_mask);
   nir_push_if(&b, nir_ine_imm(&b, vbo_bind_mask, 0));
   {
         }
            nir_def *push_const_mask = load_param64(&b, push_constant_mask);
   nir_push_if(&b, nir_ine_imm(&b, push_const_mask, 0));
   {
         }
            nir_push_if(&b, nir_ieq_imm(&b, load_param8(&b, is_dispatch), 0));
   {
      nir_push_if(&b, nir_ieq_imm(&b, load_param16(&b, draw_indexed), 0));
   {
      dgc_emit_draw(&b, &cmd_buf, stream_buf, stream_base, load_param16(&b, draw_params_offset), sequence_id,
      }
   nir_push_else(&b, NULL);
   {
      nir_variable *index_size_var =
         nir_store_var(&b, index_size_var, load_param16(&b, base_index_size), 0x1);
   nir_variable *max_index_count_var =
                  nir_def *bind_index_buffer = nir_ieq_imm(&b, nir_load_var(&b, index_size_var), 0);
   nir_push_if(&b, bind_index_buffer);
   {
      dgc_emit_index_buffer(&b, &cmd_buf, stream_buf, stream_base, load_param16(&b, index_buffer_offset),
                                                         dgc_emit_draw_indexed(&b, &cmd_buf, stream_buf, stream_base, load_param16(&b, draw_params_offset),
      }
      }
   nir_push_else(&b, NULL);
   {
      dgc_emit_dispatch(&b, &cmd_buf, stream_buf, stream_base, load_param16(&b, dispatch_params_offset), sequence_id,
      }
            /* Pad the cmdbuffer if we did not use the whole stride */
   nir_push_if(&b, nir_ine(&b, nir_load_var(&b, cmd_buf.offset), cmd_buf_end));
   {
      if (dev->physical_device->rad_info.gfx_ib_pad_with_type2) {
      nir_push_loop(&b);
                     nir_push_if(&b, nir_ieq(&b, curr_offset, cmd_buf_end));
   {
                                    }
      } else {
      nir_def *cnt = nir_isub(&b, cmd_buf_end, nir_load_var(&b, cmd_buf.offset));
   cnt = nir_ushr_imm(&b, cnt, 2);
                        }
      }
            build_dgc_buffer_tail(&b, sequence_count, dev);
   build_dgc_buffer_preamble(&b, sequence_count, dev);
      }
      void
   radv_device_finish_dgc_prepare_state(struct radv_device *device)
   {
      radv_DestroyPipeline(radv_device_to_handle(device), device->meta_state.dgc_prepare.pipeline,
         radv_DestroyPipelineLayout(radv_device_to_handle(device), device->meta_state.dgc_prepare.p_layout,
         device->vk.dispatch_table.DestroyDescriptorSetLayout(
      }
      VkResult
   radv_device_init_dgc_prepare_state(struct radv_device *device)
   {
      VkResult result;
            VkDescriptorSetLayoutCreateInfo ds_create_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                   .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
   .bindingCount = DGC_NUM_DESCS,
   .pBindings = (VkDescriptorSetLayoutBinding[]){
      {.binding = DGC_DESC_STREAM,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      {.binding = DGC_DESC_PREPARE,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      {.binding = DGC_DESC_PARAMS,
               result = radv_CreateDescriptorSetLayout(radv_device_to_handle(device), &ds_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
            const VkPipelineLayoutCreateInfo leaf_pl_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &device->meta_state.dgc_prepare.ds_layout,
   .pushConstantRangeCount = 1,
               result = radv_CreatePipelineLayout(radv_device_to_handle(device), &leaf_pl_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
            VkPipelineShaderStageCreateInfo shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(cs),
   .pName = "main",
               VkComputePipelineCreateInfo pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = shader_stage,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &pipeline_info,
         if (result != VK_SUCCESS)
         cleanup:
      ralloc_free(cs);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateIndirectCommandsLayoutNV(VkDevice _device, const VkIndirectCommandsLayoutCreateInfoNV *pCreateInfo,
               {
      RADV_FROM_HANDLE(radv_device, device, _device);
                     layout = vk_zalloc2(&device->vk.alloc, pAllocator, size, alignof(struct radv_indirect_command_layout),
         if (!layout)
                     layout->pipeline_bind_point = pCreateInfo->pipelineBindPoint;
   layout->input_stride = pCreateInfo->pStreamStrides[0];
   layout->token_count = pCreateInfo->tokenCount;
            layout->ibo_type_32 = VK_INDEX_TYPE_UINT32;
            for (unsigned i = 0; i < pCreateInfo->tokenCount; ++i) {
      switch (pCreateInfo->pTokens[i].tokenType) {
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NV:
      layout->draw_params_offset = pCreateInfo->pTokens[i].offset;
      case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV:
      layout->indexed = true;
   layout->draw_params_offset = pCreateInfo->pTokens[i].offset;
      case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DISPATCH_NV:
      layout->dispatch_params_offset = pCreateInfo->pTokens[i].offset;
      case VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_NV:
      layout->binds_index_buffer = true;
   layout->index_buffer_offset = pCreateInfo->pTokens[i].offset;
   /* 16-bit is implied if we find no match. */
   for (unsigned j = 0; j < pCreateInfo->pTokens[i].indexTypeCount; j++) {
      if (pCreateInfo->pTokens[i].pIndexTypes[j] == VK_INDEX_TYPE_UINT32)
         else if (pCreateInfo->pTokens[i].pIndexTypes[j] == VK_INDEX_TYPE_UINT8_EXT)
      }
      case VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_NV:
      layout->bind_vbo_mask |= 1u << pCreateInfo->pTokens[i].vertexBindingUnit;
   layout->vbo_offsets[pCreateInfo->pTokens[i].vertexBindingUnit] = pCreateInfo->pTokens[i].offset;
   if (pCreateInfo->pTokens[i].vertexDynamicStride)
            case VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NV:
      for (unsigned j = pCreateInfo->pTokens[i].pushconstantOffset / 4, k = 0;
      k < pCreateInfo->pTokens[i].pushconstantSize / 4; ++j, ++k) {
   layout->push_constant_mask |= 1ull << j;
      }
      default:
            }
   if (!layout->indexed)
            *pIndirectCommandsLayout = radv_indirect_command_layout_to_handle(layout);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyIndirectCommandsLayoutNV(VkDevice _device, VkIndirectCommandsLayoutNV indirectCommandsLayout,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!layout)
            vk_object_base_finish(&layout->base);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetGeneratedCommandsMemoryRequirementsNV(VkDevice _device,
               {
      RADV_FROM_HANDLE(radv_device, device, _device);
   VK_FROM_HANDLE(radv_indirect_command_layout, layout, pInfo->indirectCommandsLayout);
            uint32_t cmd_stride, upload_stride;
            VkDeviceSize cmd_buf_size = radv_align_cmdbuf_size(device, cmd_stride * pInfo->maxSequencesCount, AMD_IP_GFX) +
                  pMemoryRequirements->memoryRequirements.memoryTypeBits = device->physical_device->memory_types_32bit;
   pMemoryRequirements->memoryRequirements.alignment =
      MAX2(device->physical_device->rad_info.ip[AMD_IP_GFX].ib_alignment,
      pMemoryRequirements->memoryRequirements.size =
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdPreprocessGeneratedCommandsNV(VkCommandBuffer commandBuffer,
         {
      /* Can't do anything here as we depend on some dynamic state in some cases that we only know
      }
      /* Always need to call this directly before draw due to dependence on bound state. */
   static void
   radv_prepare_dgc_graphics(struct radv_cmd_buffer *cmd_buffer, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo,
               {
      VK_FROM_HANDLE(radv_indirect_command_layout, layout, pGeneratedCommandsInfo->indirectCommandsLayout);
   VK_FROM_HANDLE(radv_pipeline, pipeline, pGeneratedCommandsInfo->pipeline);
   struct radv_graphics_pipeline *graphics_pipeline = radv_pipeline_to_graphics(pipeline);
   struct radv_shader *vs = radv_get_shader(graphics_pipeline->base.shaders, MESA_SHADER_VERTEX);
                     if (!radv_cmd_buffer_upload_alloc(cmd_buffer, *upload_size, upload_offset, upload_data)) {
      vk_command_buffer_set_error(&cmd_buffer->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
               uint16_t vtx_base_sgpr = (cmd_buffer->state.graphics_pipeline->vtx_base_sgpr - SI_SH_REG_OFFSET) >> 2;
   if (cmd_buffer->state.graphics_pipeline->uses_drawid)
         if (cmd_buffer->state.graphics_pipeline->uses_baseinstance)
            const struct radv_shader *vertex_shader = radv_get_shader(graphics_pipeline->base.shaders, MESA_SHADER_VERTEX);
   uint16_t vbo_sgpr =
      ((radv_get_user_sgpr(vertex_shader, AC_UD_VS_VERTEX_BUFFERS)->sgpr_idx * 4 + vertex_shader->info.user_data_0) -
   SI_SH_REG_OFFSET) >>
         params->draw_indexed = layout->indexed;
   params->draw_params_offset = layout->draw_params_offset;
   params->base_index_size = layout->binds_index_buffer ? 0 : radv_get_vgt_index_size(cmd_buffer->state.index_type);
   params->vtx_base_sgpr = vtx_base_sgpr;
   params->max_index_count = cmd_buffer->state.max_index_count;
   params->index_buffer_offset = layout->index_buffer_offset;
   params->vbo_reg = vbo_sgpr;
   params->ibo_type_32 = layout->ibo_type_32;
            if (layout->bind_vbo_mask) {
      uint32_t mask = vertex_shader->info.vs.vb_desc_usage_mask;
                              unsigned idx = 0;
   while (mask) {
      unsigned i = u_bit_scan(&mask);
   unsigned binding =
                  params->vbo_bind_mask |= ((layout->bind_vbo_mask >> binding) & 1u) << idx;
   vbo_info[2 * idx] =
         vbo_info[2 * idx + 1] = graphics_pipeline->attrib_index_offset[i] | (attrib_end << 16);
      }
   params->vbo_cnt = idx;
         }
      static void
   radv_prepare_dgc_compute(struct radv_cmd_buffer *cmd_buffer, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo,
               {
      VK_FROM_HANDLE(radv_indirect_command_layout, layout, pGeneratedCommandsInfo->indirectCommandsLayout);
   VK_FROM_HANDLE(radv_pipeline, pipeline, pGeneratedCommandsInfo->pipeline);
   VK_FROM_HANDLE(radv_buffer, stream_buffer, pGeneratedCommandsInfo->pStreams[0].buffer);
   struct radv_compute_pipeline *compute_pipeline = radv_pipeline_to_compute(pipeline);
                     if (!radv_cmd_buffer_upload_alloc(cmd_buffer, *upload_size, upload_offset, upload_data)) {
      vk_command_buffer_set_error(&cmd_buffer->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
               uint32_t dispatch_initiator = cmd_buffer->device->dispatch_initiator;
   dispatch_initiator |= S_00B800_FORCE_START_AT_000(1);
   if (cs->info.wave_size == 32) {
      assert(cmd_buffer->device->physical_device->rad_info.gfx_level >= GFX10);
               uint64_t stream_addr =
            params->dispatch_params_offset = layout->dispatch_params_offset;
   params->dispatch_initiator = dispatch_initiator;
   params->is_dispatch = 1;
            const struct radv_userdata_info *loc = radv_get_user_sgpr(cs, AC_UD_CS_GRID_SIZE);
   if (loc->sgpr_idx != -1) {
            }
      void
   radv_prepare_dgc(struct radv_cmd_buffer *cmd_buffer, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
   {
      VK_FROM_HANDLE(radv_indirect_command_layout, layout, pGeneratedCommandsInfo->indirectCommandsLayout);
   VK_FROM_HANDLE(radv_pipeline, pipeline, pGeneratedCommandsInfo->pipeline);
   VK_FROM_HANDLE(radv_buffer, prep_buffer, pGeneratedCommandsInfo->preprocessBuffer);
   struct radv_meta_saved_state saved_state;
   unsigned upload_offset, upload_size;
   struct radv_buffer token_buffer;
            uint32_t cmd_stride, upload_stride;
            unsigned cmd_buf_size =
            uint64_t upload_addr =
            struct radv_dgc_params params = {.cmd_buf_stride = cmd_stride,
                                          upload_size = pipeline->push_constant_size + 16 * pipeline->dynamic_offset_count +
         if (!layout->push_constant_mask)
            if (layout->pipeline_bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS) {
      radv_prepare_dgc_graphics(cmd_buffer, pGeneratedCommandsInfo, &upload_size, &upload_offset, &upload_data,
      } else {
      assert(layout->pipeline_bind_point == VK_PIPELINE_BIND_POINT_COMPUTE);
               if (layout->push_constant_mask) {
      uint32_t *desc = upload_data;
            unsigned idx = 0;
   for (unsigned i = 0; i < ARRAY_SIZE(pipeline->shaders); ++i) {
                     const struct radv_shader *shader = pipeline->shaders[i];
   const struct radv_userdata_locations *locs = &shader->info.user_sgprs_locs;
                  if (locs->shader_data[AC_UD_PUSH_CONSTANTS].sgpr_idx >= 0 ||
      locs->shader_data[AC_UD_INLINE_PUSH_CONSTANTS].sgpr_idx >= 0) {
                  if (locs->shader_data[AC_UD_PUSH_CONSTANTS].sgpr_idx >= 0) {
      upload_sgpr = (shader->info.user_data_0 + 4 * locs->shader_data[AC_UD_PUSH_CONSTANTS].sgpr_idx -
                     if (locs->shader_data[AC_UD_INLINE_PUSH_CONSTANTS].sgpr_idx >= 0) {
      inline_sgpr = (shader->info.user_data_0 + 4 * locs->shader_data[AC_UD_INLINE_PUSH_CONSTANTS].sgpr_idx -
               desc[idx * 3 + 1] = pipeline->shaders[i]->info.inline_push_constant_mask;
      }
   desc[idx * 3] = upload_sgpr | (inline_sgpr << 16);
                           params.const_copy_size = pipeline->push_constant_size + 16 * pipeline->dynamic_offset_count;
            memcpy(upload_data, layout->push_constant_offsets, sizeof(layout->push_constant_offsets));
            memcpy(upload_data, cmd_buffer->push_constants, pipeline->push_constant_size);
            struct radv_descriptor_state *descriptors_state =
         memcpy(upload_data, descriptors_state->dynamic_buffers, 16 * pipeline->dynamic_offset_count);
                        VkWriteDescriptorSet ds_writes[5];
   VkDescriptorBufferInfo buf_info[ARRAY_SIZE(ds_writes)];
   int ds_cnt = 0;
   buf_info[ds_cnt] =
         ds_writes[ds_cnt] = (VkWriteDescriptorSet){.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                          buf_info[ds_cnt] = (VkDescriptorBufferInfo){.buffer = pGeneratedCommandsInfo->preprocessBuffer,
               ds_writes[ds_cnt] = (VkWriteDescriptorSet){.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                          if (pGeneratedCommandsInfo->streamCount > 0) {
      buf_info[ds_cnt] = (VkDescriptorBufferInfo){.buffer = pGeneratedCommandsInfo->pStreams[0].buffer,
               ds_writes[ds_cnt] = (VkWriteDescriptorSet){.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                             if (pGeneratedCommandsInfo->sequencesCountBuffer != VK_NULL_HANDLE) {
      buf_info[ds_cnt] = (VkDescriptorBufferInfo){.buffer = pGeneratedCommandsInfo->sequencesCountBuffer,
               ds_writes[ds_cnt] = (VkWriteDescriptorSet){.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                 ++ds_cnt;
               radv_meta_save(&saved_state, cmd_buffer,
            radv_CmdBindPipeline(radv_cmd_buffer_to_handle(cmd_buffer), VK_PIPELINE_BIND_POINT_COMPUTE,
            radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), cmd_buffer->device->meta_state.dgc_prepare.p_layout,
            radv_meta_push_descriptor_set(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            unsigned block_count = MAX2(1, DIV_ROUND_UP(pGeneratedCommandsInfo->sequencesCount, 64));
            radv_buffer_finish(&token_buffer);
               }
      /* VK_NV_device_generated_commands_compute */
   VKAPI_ATTR void VKAPI_CALL
   radv_GetPipelineIndirectMemoryRequirementsNV(VkDevice device, const VkComputePipelineCreateInfo *pCreateInfo,
         {
         }
      VKAPI_ATTR VkDeviceAddress VKAPI_CALL
   radv_GetPipelineIndirectDeviceAddressNV(VkDevice device, const VkPipelineIndirectDeviceAddressInfoNV *pInfo)
   {
         }
