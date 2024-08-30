   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <inttypes.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <vulkan/vulkan.h>
      #include "pvr_bo.h"
   #include "pvr_csb.h"
   #include "pvr_csb_enum_helpers.h"
   #include "pvr_device_info.h"
   #include "pvr_dump.h"
   #include "pvr_dump_bo.h"
   #include "pvr_private.h"
   #include "pvr_util.h"
   #include "util/list.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "vulkan/util/vk_enum_to_str.h"
      /*****************************************************************************
         ******************************************************************************/
      #define PVR_DUMP_CSB_WORD_SIZE ((unsigned)sizeof(uint32_t))
      enum buffer_type {
      BUFFER_TYPE_NONE = 0,
   BUFFER_TYPE_CDMCTRL,
   BUFFER_TYPE_VDMCTRL,
   BUFFER_TYPE_PPP,
      };
      struct pvr_dump_csb_ctx {
               /* User-modifiable values */
      };
      static inline bool
   pvr_dump_csb_ctx_push(struct pvr_dump_csb_ctx *const ctx,
         {
      if (!pvr_dump_buffer_ctx_push(&ctx->base,
                                                }
      static inline struct pvr_dump_buffer_ctx *
   pvr_dump_csb_ctx_pop(struct pvr_dump_csb_ctx *const ctx, bool advance_parent)
   {
      struct pvr_dump_buffer_ctx *parent;
   struct pvr_dump_ctx *parent_base;
   const uint64_t unused_words =
            if (unused_words) {
      pvr_dump_buffer_print_header_line(&ctx->base,
                                    pvr_dump_buffer_advance(&ctx->base,
                        parent_base = pvr_dump_buffer_ctx_pop(&ctx->base);
   if (!parent_base)
                     if (advance_parent)
               }
      struct pvr_dump_csb_block_ctx {
         };
      #define pvr_dump_csb_block_ctx_push(ctx,                               \
                        ({                                                                  \
      struct pvr_dump_csb_ctx *const _csb_ctx = (parent_ctx);          \
   pvr_dump_buffer_print_header_line(&_csb_ctx->base,               \
                              static inline bool
   __pvr_dump_csb_block_ctx_push(struct pvr_dump_csb_block_ctx *const ctx,
         {
               if (!pvr_dump_buffer_ctx_push(&ctx->base,
                                                }
      static inline struct pvr_dump_csb_ctx *
   pvr_dump_csb_block_ctx_pop(struct pvr_dump_csb_block_ctx *const ctx)
   {
      const uint64_t used_size = ctx->base.capacity - ctx->base.remaining_size;
   struct pvr_dump_csb_ctx *parent_ctx;
            parent_base = pvr_dump_buffer_ctx_pop(&ctx->base);
   if (!parent_base)
                     /* No need to check this since it can never fail. */
                        }
      static inline const uint32_t *
   pvr_dump_csb_block_take(struct pvr_dump_csb_block_ctx *const restrict ctx,
         {
         }
      #define pvr_dump_csb_block_take_packed(ctx, cmd, dest)             \
      ({                                                              \
      struct pvr_dump_csb_block_ctx *const _block_ctx = (ctx);     \
   struct PVRX(cmd) *const _dest = (dest);                      \
   const void *const _ptr =                                     \
         if (_ptr) {                                                  \
         } else {                                                     \
      pvr_dump_field_error(&_block_ctx->base.base,              \
      }                                                            \
            /*****************************************************************************
         ******************************************************************************/
      static inline void
   __pvr_dump_field_needs_feature(struct pvr_dump_ctx *const ctx,
               {
         }
      #define pvr_dump_field_needs_feature(ctx, name, feature)              \
      do {                                                               \
      (void)PVR_HAS_FEATURE((struct pvr_device_info *)NULL, feature); \
            #define pvr_dump_field_member_needs_feature(ctx, compound, member, feature) \
      do {                                                                     \
      (void)&(compound)->member;                                            \
            /******************************************************************************
         ******************************************************************************/
      static bool print_sub_buffer(struct pvr_dump_ctx *ctx,
                                    /******************************************************************************
         ******************************************************************************/
      static uint32_t
   print_block_cdmctrl_kernel(struct pvr_dump_csb_ctx *const csb_ctx,
         {
               struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(CDMCTRL_KERNEL0) kernel0 = { 0 };
   struct PVRX(CDMCTRL_KERNEL1) kernel1 = { 0 };
   struct PVRX(CDMCTRL_KERNEL2) kernel2 = { 0 };
   struct PVRX(CDMCTRL_KERNEL3) kernel3 = { 0 };
   struct PVRX(CDMCTRL_KERNEL4) kernel4 = { 0 };
   struct PVRX(CDMCTRL_KERNEL5) kernel5 = { 0 };
   struct PVRX(CDMCTRL_KERNEL6) kernel6 = { 0 };
   struct PVRX(CDMCTRL_KERNEL7) kernel7 = { 0 };
   struct PVRX(CDMCTRL_KERNEL8) kernel8 = { 0 };
   struct PVRX(CDMCTRL_KERNEL9) kernel9 = { 0 };
   struct PVRX(CDMCTRL_KERNEL10) kernel10 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "KERNEL"))
            if (!pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL0, &kernel0) ||
      !pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL1, &kernel1) ||
   !pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL2, &kernel2)) {
      }
            if (!kernel0.indirect_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL3, &kernel3) ||
      !pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL4, &kernel4) ||
   !pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL5, &kernel5)) {
      }
      } else {
      if (!pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL6, &kernel6) ||
      !pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL7, &kernel7)) {
      }
               if (!pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL8, &kernel8))
                  if (!pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL9, &kernel9) ||
      !pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL10, &kernel10) ||
   !pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_KERNEL11, &kernel11)) {
      }
            pvr_dump_field_member_bool(base_ctx, &kernel0, indirect_present);
   pvr_dump_field_member_bool(base_ctx, &kernel0, global_offsets_present);
   pvr_dump_field_member_bool(base_ctx, &kernel0, event_object_present);
   pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &kernel0,
   usc_common_size,
   PVRX(CDMCTRL_KERNEL0_USC_COMMON_SIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &kernel0,
   usc_unified_size,
   PVRX(CDMCTRL_KERNEL0_USC_UNIFIED_SIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &kernel0,
   pds_temp_size,
   PVRX(CDMCTRL_KERNEL0_PDS_TEMP_SIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &kernel0,
   pds_data_size,
   PVRX(CDMCTRL_KERNEL0_PDS_DATA_SIZE_UNIT_SIZE),
      pvr_dump_field_member_enum(base_ctx,
                              pvr_dump_field_member_addr_offset(base_ctx,
                     ret = print_sub_buffer(
      base_ctx,
   device,
   BUFFER_TYPE_NONE,
   PVR_DEV_ADDR_OFFSET(pds_heap_base, kernel1.data_addr.addr),
   kernel0.pds_data_size * PVRX(CDMCTRL_KERNEL0_PDS_DATA_SIZE_UNIT_SIZE),
      if (!ret)
            pvr_dump_field_member_enum(base_ctx,
                              pvr_dump_field_member_addr_offset(base_ctx,
                     /* FIXME: Determine the exact size of the PDS code section once disassembly
   * is implemented.
   */
   ret = print_sub_buffer(base_ctx,
                        device,
   BUFFER_TYPE_NONE,
      if (!ret)
                     if (!kernel0.indirect_present) {
      pvr_dump_field_member_u32_offset(base_ctx, &kernel3, workgroup_x, 1);
   pvr_dump_field_member_u32_offset(base_ctx, &kernel4, workgroup_y, 1);
               } else {
      pvr_dump_field_member_not_present(base_ctx, &kernel3, workgroup_x);
   pvr_dump_field_member_not_present(base_ctx, &kernel4, workgroup_y);
            pvr_dump_field_addr_split(base_ctx,
                           pvr_dump_field_member_u32_zero(base_ctx, &kernel8, max_instances, 32);
   pvr_dump_field_member_u32_offset(base_ctx, &kernel8, workgroup_size_x, 1);
   pvr_dump_field_member_u32_offset(base_ctx, &kernel8, workgroup_size_y, 1);
            if (kernel0.event_object_present) {
      pvr_dump_field_member_u32(base_ctx, &kernel9, global_offset_x);
   pvr_dump_field_member_u32(base_ctx, &kernel10, global_offset_y);
      } else {
      pvr_dump_field_member_not_present(base_ctx, &kernel9, global_offset_x);
   pvr_dump_field_member_not_present(base_ctx, &kernel10, global_offset_y);
                     end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_cdmctrl_stream_link(struct pvr_dump_csb_ctx *const csb_ctx)
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(CDMCTRL_STREAM_LINK0) link0 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "STREAM_LINK"))
            if (!pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_STREAM_LINK0, &link0) ||
      !pvr_dump_csb_block_take_packed(&ctx, CDMCTRL_STREAM_LINK1, &link1)) {
      }
            pvr_dump_field_addr_split(base_ctx,
                              end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_cdmctrl_stream_terminate(struct pvr_dump_csb_ctx *const csb_ctx)
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
                     if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "TERMINATE"))
            if (!pvr_dump_csb_block_take_packed(&ctx,
                     }
                           end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_vdmctrl_ppp_state_update(struct pvr_dump_csb_ctx *const csb_ctx,
         {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(VDMCTRL_PPP_STATE0) state0 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "PPP_STATE_UPDATE"))
            if (!pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_PPP_STATE0, &state0) ||
      !pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_PPP_STATE1, &state1)) {
      }
            pvr_dump_field_member_u32_zero(base_ctx, &state0, word_count, 256);
   pvr_dump_field_addr_split(base_ctx, "addr", state0.addrmsb, state1.addrlsb);
   ret = print_sub_buffer(
      base_ctx,
   device,
   BUFFER_TYPE_PPP,
   PVR_DEV_ADDR(state0.addrmsb.addr | state1.addrlsb.addr),
   (state0.word_count ? state0.word_count : 256) * PVR_DUMP_CSB_WORD_SIZE,
      if (!ret)
                  end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_vdmctrl_pds_state_update(struct pvr_dump_csb_ctx *const csb_ctx,
         {
               struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(VDMCTRL_PDS_STATE0) state0 = { 0 };
   struct PVRX(VDMCTRL_PDS_STATE1) state1 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "PDS_STATE_UPDATE"))
            if (!pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_PDS_STATE0, &state0) ||
      !pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_PDS_STATE1, &state1) ||
   !pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_PDS_STATE2, &state2)) {
      }
            pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &state0,
   usc_common_size,
   PVRX(VDMCTRL_PDS_STATE0_USC_COMMON_SIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &state0,
   usc_unified_size,
   PVRX(VDMCTRL_PDS_STATE0_USC_UNIFIED_SIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &state0,
   pds_temp_size,
   PVRX(VDMCTRL_PDS_STATE0_PDS_TEMP_SIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &state0,
   pds_data_size,
   PVRX(VDMCTRL_PDS_STATE0_PDS_DATA_SIZE_UNIT_SIZE),
         pvr_dump_field_member_addr_offset(base_ctx,
                     ret = print_sub_buffer(
      base_ctx,
   device,
   BUFFER_TYPE_NONE,
   PVR_DEV_ADDR_OFFSET(pds_heap_base, state1.pds_data_addr.addr),
   state0.pds_data_size * PVRX(VDMCTRL_PDS_STATE0_PDS_DATA_SIZE_UNIT_SIZE),
      if (!ret)
            pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_enum(base_ctx,
                        pvr_dump_field_member_addr_offset(base_ctx,
                     /* FIXME: Determine the exact size of the PDS code section once disassembly
   * is implemented.
   */
   ret = print_sub_buffer(base_ctx,
                        device,
   BUFFER_TYPE_NONE,
      if (!ret)
                  end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_vdmctrl_vdm_state_update(struct pvr_dump_csb_ctx *const csb_ctx,
         {
               struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(VDMCTRL_VDM_STATE0) state0 = { 0 };
   struct PVRX(VDMCTRL_VDM_STATE1) state1 = { 0 };
   struct PVRX(VDMCTRL_VDM_STATE2) state2 = { 0 };
   struct PVRX(VDMCTRL_VDM_STATE3) state3 = { 0 };
   struct PVRX(VDMCTRL_VDM_STATE4) state4 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "VDM_STATE_UPDATE"))
            if (!pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_VDM_STATE0, &state0))
                  if (state0.cut_index_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_VDM_STATE1, &state1))
                     if (state0.vs_data_addr_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_VDM_STATE2, &state2))
                     if (state0.vs_other_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_VDM_STATE3, &state3) ||
      !pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_VDM_STATE4, &state4) ||
   !pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_VDM_STATE5, &state5)) {
      }
               if (state0.cut_index_present) {
         } else {
                  if (state0.vs_data_addr_present) {
      pvr_dump_field_member_addr_offset(base_ctx,
                     if (state0.vs_other_present) {
      ret = print_sub_buffer(
      base_ctx,
   device,
   BUFFER_TYPE_NONE,
   PVR_DEV_ADDR_OFFSET(pds_heap_base,
         state5.vs_pds_data_size *
         } else {
      /* FIXME: Determine the exact size of the PDS data section when no
   * code section is present once disassembly is implemented.
   */
   ret = print_sub_buffer(
      base_ctx,
   device,
   BUFFER_TYPE_NONE,
   PVR_DEV_ADDR_OFFSET(pds_heap_base,
         0,
   }
   if (!ret)
      } else {
      pvr_dump_field_member_not_present(base_ctx,
                     if (state0.vs_other_present) {
      pvr_dump_field_member_addr_offset(base_ctx,
                     /* FIXME: Determine the exact size of the PDS code section once
   * disassembly is implemented.
   */
   ret = print_sub_buffer(
      base_ctx,
   device,
   BUFFER_TYPE_NONE,
   PVR_DEV_ADDR_OFFSET(pds_heap_base, state3.vs_pds_code_base_addr.addr),
   0,
      if (!ret)
            pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &state4,
   vs_output_size,
               pvr_dump_field_member_u32_zero(base_ctx, &state5, vs_max_instances, 32);
   pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &state5,
   vs_usc_common_size,
   PVRX(VDMCTRL_VDM_STATE5_VS_USC_COMMON_SIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &state5,
   vs_usc_unified_size,
   PVRX(VDMCTRL_VDM_STATE5_VS_USC_UNIFIED_SIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &state5,
   vs_pds_temp_size,
   PVRX(VDMCTRL_VDM_STATE5_VS_PDS_TEMP_SIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &state5,
   vs_pds_data_size,
   PVRX(VDMCTRL_VDM_STATE5_VS_PDS_DATA_SIZE_UNIT_SIZE),
   } else {
      pvr_dump_field_member_not_present(base_ctx,
               pvr_dump_field_member_not_present(base_ctx, &state4, vs_output_size);
   pvr_dump_field_member_not_present(base_ctx, &state5, vs_max_instances);
   pvr_dump_field_member_not_present(base_ctx, &state5, vs_usc_common_size);
   pvr_dump_field_member_not_present(base_ctx, &state5, vs_usc_unified_size);
   pvr_dump_field_member_not_present(base_ctx, &state5, vs_pds_temp_size);
               pvr_dump_field_member_bool(base_ctx, &state0, ds_present);
   pvr_dump_field_member_bool(base_ctx, &state0, gs_present);
   pvr_dump_field_member_bool(base_ctx, &state0, hs_present);
   pvr_dump_field_member_u32_offset(base_ctx, &state0, cam_size, 1);
   pvr_dump_field_member_enum(
      base_ctx,
   &state0,
   uvs_scratch_size_select,
      pvr_dump_field_member_bool(base_ctx, &state0, cut_index_enable);
   pvr_dump_field_member_bool(base_ctx, &state0, tess_enable);
   pvr_dump_field_member_bool(base_ctx, &state0, gs_enable);
   pvr_dump_field_member_enum(base_ctx,
                                    end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_vdmctrl_index_list(struct pvr_dump_csb_ctx *const csb_ctx,
         {
               struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(VDMCTRL_INDEX_LIST0) index_list0 = { 0 };
   struct PVRX(VDMCTRL_INDEX_LIST1) index_list1 = { 0 };
   struct PVRX(VDMCTRL_INDEX_LIST2) index_list2 = { 0 };
   struct PVRX(VDMCTRL_INDEX_LIST3) index_list3 = { 0 };
   struct PVRX(VDMCTRL_INDEX_LIST4) index_list4 = { 0 };
   struct PVRX(VDMCTRL_INDEX_LIST5) index_list5 = { 0 };
   struct PVRX(VDMCTRL_INDEX_LIST6) index_list6 = { 0 };
   struct PVRX(VDMCTRL_INDEX_LIST7) index_list7 = { 0 };
   struct PVRX(VDMCTRL_INDEX_LIST8) index_list8 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "INDEX_LIST"))
            if (!pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_INDEX_LIST0, &index_list0))
                  if (index_list0.index_addr_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                     }
               if (index_list0.index_count_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                     }
               if (index_list0.index_instance_count_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                     }
               if (index_list0.index_offset_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                     }
               if (index_list0.start_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                  !pvr_dump_csb_block_take_packed(&ctx,
                  }
               if (index_list0.indirect_addr_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                  !pvr_dump_csb_block_take_packed(&ctx,
                  }
               if (index_list0.split_count_present) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                                 if (PVR_HAS_FEATURE(dev_info, vdm_degenerate_culling)) {
         } else {
      pvr_dump_field_member_needs_feature(base_ctx,
                           pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_u32_offset(base_ctx, &index_list0, patch_count, 1);
   pvr_dump_field_member_enum(base_ctx,
                        if (index_list0.index_addr_present) {
      pvr_dump_field_addr_split(base_ctx,
                     const uint32_t index_size =
            if (!index_list0.index_count_present) {
      ret = pvr_dump_error(base_ctx, "index_addr requires index_count");
               ret = print_sub_buffer(base_ctx,
                        device,
   BUFFER_TYPE_NONE,
      if (!ret)
      } else {
                  if (index_list0.index_count_present) {
         } else {
                  if (index_list0.index_instance_count_present) {
      pvr_dump_field_member_u32_offset(base_ctx,
                  } else {
                  if (index_list0.index_offset_present) {
         } else {
                  if (index_list0.start_present) {
      pvr_dump_field_member_u32(base_ctx, &index_list5, start_index);
      } else {
      pvr_dump_field_member_not_present(base_ctx, &index_list5, start_index);
               if (index_list0.indirect_addr_present) {
      pvr_dump_field_addr_split(base_ctx,
                     ret =
      print_sub_buffer(base_ctx,
                  device,
   BUFFER_TYPE_NONE,
   PVR_DEV_ADDR(index_list7.indirect_base_addrmsb.addr |
   if (!ret)
      } else {
                  if (index_list0.split_count_present) {
         } else {
                        end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_vdmctrl_stream_link(struct pvr_dump_csb_ctx *const csb_ctx)
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(VDMCTRL_STREAM_LINK0) link0 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "STREAM_LINK"))
            if (!pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_STREAM_LINK0, &link0) ||
      !pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_STREAM_LINK1, &link1)) {
      }
                     if (link0.compare_present) {
      pvr_dump_field_member_u32(base_ctx, &link0, compare_mode);
      } else {
      pvr_dump_field_member_not_present(base_ctx, &link0, compare_mode);
               pvr_dump_field_addr_split(base_ctx,
                              end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_vdmctrl_stream_return(struct pvr_dump_csb_ctx *const csb_ctx)
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
                     if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "STREAM_RETURN"))
            if (!pvr_dump_csb_block_take_packed(&ctx, VDMCTRL_STREAM_RETURN, &return_))
                                 end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_vdmctrl_stream_terminate(struct pvr_dump_csb_ctx *const csb_ctx)
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
                     if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "TERMINATE"))
            if (!pvr_dump_csb_block_take_packed(&ctx,
                     }
                           end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_ppp_state_header(struct pvr_dump_csb_ctx *const csb_ctx,
         {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
                     if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "STATE_HEADER"))
            if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_HEADER, &header))
                  pvr_dump_field_member_bool(base_ctx, &header, pres_ispctl);
   pvr_dump_field_member_bool(base_ctx, &header, pres_ispctl_fa);
   pvr_dump_field_member_bool(base_ctx, &header, pres_ispctl_fb);
   pvr_dump_field_member_bool(base_ctx, &header, pres_ispctl_ba);
   pvr_dump_field_member_bool(base_ctx, &header, pres_ispctl_bb);
   pvr_dump_field_member_bool(base_ctx, &header, pres_ispctl_dbsc);
   pvr_dump_field_member_bool(base_ctx, &header, pres_pds_state_ptr0);
   pvr_dump_field_member_bool(base_ctx, &header, pres_pds_state_ptr1);
   pvr_dump_field_member_bool(base_ctx, &header, pres_pds_state_ptr2);
   pvr_dump_field_member_bool(base_ctx, &header, pres_pds_state_ptr3);
   pvr_dump_field_member_bool(base_ctx, &header, pres_region_clip);
   pvr_dump_field_member_bool(base_ctx, &header, pres_viewport);
   pvr_dump_field_member_u32_offset(base_ctx, &header, view_port_count, 1);
   pvr_dump_field_member_bool(base_ctx, &header, pres_wclamp);
   pvr_dump_field_member_bool(base_ctx, &header, pres_outselects);
   pvr_dump_field_member_bool(base_ctx, &header, pres_varying_word0);
   pvr_dump_field_member_bool(base_ctx, &header, pres_varying_word1);
   pvr_dump_field_member_bool(base_ctx, &header, pres_varying_word2);
   pvr_dump_field_member_bool(base_ctx, &header, pres_ppp_ctrl);
   pvr_dump_field_member_bool(base_ctx, &header, pres_stream_out_size);
   pvr_dump_field_member_bool(base_ctx, &header, pres_stream_out_program);
   pvr_dump_field_member_bool(base_ctx, &header, context_switch);
   pvr_dump_field_member_bool(base_ctx, &header, pres_terminate);
            if (header_out)
                  end_pop_ctx:
            end_out:
         }
      static void print_block_ppp_state_isp_one_side(
      struct pvr_dump_csb_block_ctx *const ctx,
   const struct PVRX(TA_STATE_ISPA) *const isp_a,
   const struct PVRX(TA_STATE_ISPB) *const isp_b,
      {
                        pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_bool(base_ctx, isp_a, ovgvispassmaskop);
   pvr_dump_field_member_bool(base_ctx, isp_a, maskval);
   pvr_dump_field_member_bool(base_ctx, isp_a, dwritedisable);
   pvr_dump_field_member_bool(base_ctx, isp_a, dfbztestenable);
   pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_bool(base_ctx, isp_a, linefilllastpixel);
   pvr_dump_field_member_uq4_4_offset(base_ctx, isp_a, pointlinewidth, 0x01);
            if (has_b) {
      pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_x32(base_ctx, isp_b, scmpmask, 2);
      } else {
      pvr_dump_field_member_not_present(base_ctx, isp_b, scmpmode);
   pvr_dump_field_member_not_present(base_ctx, isp_b, sop1);
   pvr_dump_field_member_not_present(base_ctx, isp_b, sop2);
   pvr_dump_field_member_not_present(base_ctx, isp_b, sop3);
   pvr_dump_field_member_not_present(base_ctx, isp_b, scmpmask);
                  }
      static uint32_t
   print_block_ppp_state_isp(struct pvr_dump_csb_ctx *const csb_ctx,
                           const bool has_fa,
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(TA_STATE_ISPCTL) isp_ctl = { 0 };
   struct PVRX(TA_STATE_ISPA) isp_fa = { 0 };
   struct PVRX(TA_STATE_ISPB) isp_fb = { 0 };
   struct PVRX(TA_STATE_ISPA) isp_ba = { 0 };
   struct PVRX(TA_STATE_ISPB) isp_bb = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "STATE_ISP"))
            if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_ISPCTL, &isp_ctl))
                  /* In most blocks, we try to read all words before printing anything. In
   * this case, there can be ambiguity in which words to parse (which results
   * in an error from the conditional below). To aid in debugging when this
   * ambiguity is present, print the control word's contents before continuing
   * so the fields which create the ambiguity are dumped even when the rest of
   * the block isn't.
   */
   pvr_dump_field_member_u32(base_ctx, &isp_ctl, visreg);
   pvr_dump_field_member_bool(base_ctx, &isp_ctl, visbool);
   pvr_dump_field_member_bool(base_ctx, &isp_ctl, vistest);
   pvr_dump_field_member_bool(base_ctx, &isp_ctl, scenable);
   pvr_dump_field_member_bool(base_ctx, &isp_ctl, dbenable);
   pvr_dump_field_member_bool(base_ctx, &isp_ctl, bpres);
   pvr_dump_field_member_bool(base_ctx, &isp_ctl, two_sided);
   pvr_dump_field_member_bool(base_ctx, &isp_ctl, ovgmtestdisable);
   pvr_dump_field_member_bool(base_ctx, &isp_ctl, tagwritedisable);
   pvr_dump_field_member_u32(base_ctx, &isp_ctl, upass);
            if (!has_fa || has_fb != isp_ctl.bpres || has_ba != isp_ctl.two_sided ||
      has_bb != (isp_ctl.bpres && isp_ctl.two_sided)) {
   pvr_dump_error(
      base_ctx,
                  if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_ISPA, &isp_fa))
                  if (has_fb) {
      if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_ISPB, &isp_fb))
                     if (has_ba) {
      if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_ISPA, &isp_ba))
                     if (has_bb) {
      if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_ISPB, &isp_bb))
                     if (has_dbsc) {
      if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_ISPDBSC, &isp_dbsc))
                     pvr_dump_println(base_ctx, "front");
            if (isp_ctl.two_sided) {
      pvr_dump_println(base_ctx, "back");
      } else {
                  if (has_dbsc) {
      pvr_dump_field_member_u32(base_ctx, &isp_dbsc, dbindex);
      } else {
      pvr_dump_field_member_not_present(base_ctx, &isp_dbsc, dbindex);
                     end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_ppp_state_pds(struct pvr_dump_csb_ctx *const csb_ctx,
                           struct pvr_device *const device,
   {
               struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(TA_STATE_PDS_SHADERBASE) shader_base = { 0 };
   struct PVRX(TA_STATE_PDS_TEXUNICODEBASE) tex_unicode_base = { 0 };
   struct PVRX(TA_STATE_PDS_SIZEINFO1) size_info1 = { 0 };
   struct PVRX(TA_STATE_PDS_SIZEINFO2) size_info2 = { 0 };
   struct PVRX(TA_STATE_PDS_VARYINGBASE) varying_base = { 0 };
   struct PVRX(TA_STATE_PDS_TEXTUREDATABASE) texture_data_base = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "STATE_PDS"))
            if (has_initial_words) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                  !pvr_dump_csb_block_take_packed(&ctx,
               !pvr_dump_csb_block_take_packed(&ctx,
               !pvr_dump_csb_block_take_packed(&ctx,
                  }
               if (has_varying) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                     }
               if (has_texturedata) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                     }
               if (has_uniformdata) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                     }
               if (has_initial_words) {
      pvr_dump_field_addr_offset(base_ctx,
                     pvr_dump_field_addr_offset(base_ctx,
                        pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &size_info1,
   pds_uniformsize,
   PVRX(TA_STATE_PDS_SIZEINFO1_PDS_UNIFORMSIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &size_info1,
   pds_texturestatesize,
   PVRX(TA_STATE_PDS_SIZEINFO1_PDS_TEXTURESTATESIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &size_info1,
   pds_varyingsize,
   PVRX(TA_STATE_PDS_SIZEINFO1_PDS_VARYINGSIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &size_info1,
   usc_varyingsize,
   PVRX(TA_STATE_PDS_SIZEINFO1_USC_VARYINGSIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &size_info1,
   pds_tempsize,
               pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &size_info2,
   usc_sharedsize,
   PVRX(TA_STATE_PDS_SIZEINFO2_USC_SHAREDSIZE_UNIT_SIZE),
      pvr_dump_field_member_bool(base_ctx, &size_info2, pds_tri_merge_disable);
      } else {
      pvr_dump_field_not_present(base_ctx, "shaderbase");
   pvr_dump_field_not_present(base_ctx, "texunicodebase");
   pvr_dump_field_member_not_present(base_ctx, &size_info1, pds_uniformsize);
   pvr_dump_field_member_not_present(base_ctx,
               pvr_dump_field_member_not_present(base_ctx, &size_info1, pds_varyingsize);
   pvr_dump_field_member_not_present(base_ctx, &size_info1, usc_varyingsize);
   pvr_dump_field_member_not_present(base_ctx, &size_info1, pds_tempsize);
   pvr_dump_field_member_not_present(base_ctx, &size_info2, usc_sharedsize);
   pvr_dump_field_member_not_present(base_ctx,
                           if (has_varying) {
      pvr_dump_field_addr_offset(base_ctx,
                  } else {
                  if (has_texturedata) {
      pvr_dump_field_addr_offset(base_ctx,
                  } else {
                  if (has_uniformdata) {
      pvr_dump_field_addr_offset(base_ctx,
                  } else {
                        end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_ppp_region_clip(struct pvr_dump_csb_ctx *const csb_ctx)
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(TA_REGION_CLIP0) clip0 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "REGION_CLIP"))
            if (!pvr_dump_csb_block_take_packed(&ctx, TA_REGION_CLIP0, &clip0) ||
      !pvr_dump_csb_block_take_packed(&ctx, TA_REGION_CLIP1, &clip1)) {
      }
            pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_u32_scaled_units(base_ctx, &clip0, left, 32, "pixels");
            pvr_dump_field_member_u32_scaled_units(base_ctx, &clip1, top, 32, "pixels");
   pvr_dump_field_member_u32_scaled_units(base_ctx,
                                    end_pop_ctx:
            end_out:
         }
      static uint32_t print_block_ppp_viewport(struct pvr_dump_csb_ctx *const csb_ctx,
         {
      static char const *const field_names[] = {
                  struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
                     if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "VIEWPORT %" PRIu32, idx))
            for (uint32_t i = 0; i < ARRAY_SIZE(field_names); i++) {
      const uint32_t *const value = pvr_dump_csb_block_take(&ctx, 1);
   if (!value)
                                    end_pop_ctx:
            end_out:
         }
      static uint32_t print_block_ppp_wclamp(struct pvr_dump_csb_ctx *const csb_ctx)
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
                     if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "WCLAMP"))
            const uint32_t *const value = pvr_dump_csb_block_take(&ctx, 1);
   if (!value)
                                 end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_ppp_output_sel(struct pvr_dump_csb_ctx *const csb_ctx)
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
                     if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "OUTPUT_SEL"))
            if (!pvr_dump_csb_block_take_packed(&ctx, TA_OUTPUT_SEL, &output_sel))
                  pvr_dump_field_member_bool(base_ctx, &output_sel, plane0);
   pvr_dump_field_member_bool(base_ctx, &output_sel, plane1);
   pvr_dump_field_member_bool(base_ctx, &output_sel, plane2);
   pvr_dump_field_member_bool(base_ctx, &output_sel, plane3);
   pvr_dump_field_member_bool(base_ctx, &output_sel, plane4);
   pvr_dump_field_member_bool(base_ctx, &output_sel, plane5);
   pvr_dump_field_member_bool(base_ctx, &output_sel, plane6);
   pvr_dump_field_member_bool(base_ctx, &output_sel, plane7);
   pvr_dump_field_member_bool(base_ctx, &output_sel, cullplane0);
   pvr_dump_field_member_bool(base_ctx, &output_sel, cullplane1);
   pvr_dump_field_member_bool(base_ctx, &output_sel, cullplane2);
   pvr_dump_field_member_bool(base_ctx, &output_sel, cullplane3);
   pvr_dump_field_member_bool(base_ctx, &output_sel, cullplane4);
   pvr_dump_field_member_bool(base_ctx, &output_sel, cullplane5);
   pvr_dump_field_member_bool(base_ctx, &output_sel, cullplane6);
   pvr_dump_field_member_bool(base_ctx, &output_sel, cullplane7);
   pvr_dump_field_member_bool(base_ctx, &output_sel, rhw_pres);
   pvr_dump_field_member_bool(base_ctx,
               pvr_dump_field_member_bool(base_ctx, &output_sel, psprite_size_pres);
   pvr_dump_field_member_bool(base_ctx, &output_sel, vpt_tgt_pres);
   pvr_dump_field_member_bool(base_ctx, &output_sel, render_tgt_pres);
   pvr_dump_field_member_bool(base_ctx, &output_sel, tsp_unclamped_z_pres);
                  end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_ppp_state_varying(struct pvr_dump_csb_ctx *const csb_ctx,
                     {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(TA_STATE_VARYING0) varying0 = { 0 };
   struct PVRX(TA_STATE_VARYING1) varying1 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "STATE_VARYING"))
            if (has_word0) {
      if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_VARYING0, &varying0))
                     if (has_word1) {
      if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_VARYING1, &varying1))
                     if (has_word2) {
      if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_VARYING2, &varying2))
                     if (has_word0) {
      pvr_dump_field_member_u32(base_ctx, &varying0, f32_linear);
   pvr_dump_field_member_u32(base_ctx, &varying0, f32_flat);
      } else {
      pvr_dump_field_member_not_present(base_ctx, &varying0, f32_linear);
   pvr_dump_field_member_not_present(base_ctx, &varying0, f32_flat);
               if (has_word1) {
      pvr_dump_field_member_u32(base_ctx, &varying1, f16_linear);
   pvr_dump_field_member_u32(base_ctx, &varying1, f16_flat);
      } else {
      pvr_dump_field_member_not_present(base_ctx, &varying1, f16_linear);
   pvr_dump_field_member_not_present(base_ctx, &varying1, f16_flat);
               if (has_word2) {
         } else {
      pvr_dump_field_member_not_present(base_ctx,
                           end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_ppp_state_ppp_ctrl(struct pvr_dump_csb_ctx *const csb_ctx)
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
                     if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "STATE_PPP_CTRL"))
            if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_PPP_CTRL, &ppp_ctrl))
                  pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_bool(base_ctx, &ppp_ctrl, updatebbox);
   pvr_dump_field_member_bool(base_ctx, &ppp_ctrl, resetbbox);
   pvr_dump_field_member_bool(base_ctx, &ppp_ctrl, wbuffen);
   pvr_dump_field_member_bool(base_ctx, &ppp_ctrl, wclampen);
   pvr_dump_field_member_bool(base_ctx, &ppp_ctrl, pretransform);
   pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_bool(base_ctx, &ppp_ctrl, drawclippededges);
   pvr_dump_field_member_enum(base_ctx,
                     pvr_dump_field_member_bool(base_ctx, &ppp_ctrl, pres_prim_id);
   pvr_dump_field_member_enum(base_ctx,
                                    end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_ppp_state_stream_out(struct pvr_dump_csb_ctx *const csb_ctx,
                     {
               struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(TA_STATE_STREAM_OUT0) stream_out0 = { 0 };
   struct PVRX(TA_STATE_STREAM_OUT1) stream_out1 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "STATE_STREAM_OUT"))
            if (has_word0) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                     }
               if (has_words12) {
      if (!pvr_dump_csb_block_take_packed(&ctx,
                  !pvr_dump_csb_block_take_packed(&ctx,
                  }
               if (has_word0) {
      pvr_dump_field_member_bool(base_ctx, &stream_out0, stream0_ta_output);
   pvr_dump_field_member_bool(base_ctx, &stream_out0, stream0_mem_output);
   pvr_dump_field_member_u32_units(base_ctx,
                     pvr_dump_field_member_u32_units(base_ctx,
                     pvr_dump_field_member_u32_units(base_ctx,
                  } else {
      pvr_dump_field_member_not_present(base_ctx,
               pvr_dump_field_member_not_present(base_ctx,
               pvr_dump_field_member_not_present(base_ctx, &stream_out0, stream1_size);
   pvr_dump_field_member_not_present(base_ctx, &stream_out0, stream2_size);
               if (has_words12) {
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &stream_out1,
   pds_temp_size,
   PVRX(TA_STATE_STREAM_OUT1_PDS_TEMP_SIZE_UNIT_SIZE),
      pvr_dump_field_member_u32_scaled_units(
      base_ctx,
   &stream_out1,
   pds_data_size,
   PVRX(TA_STATE_STREAM_OUT1_PDS_DATA_SIZE_UNIT_SIZE),
      pvr_dump_field_member_bool(base_ctx, &stream_out1, sync);
   pvr_dump_field_member_addr_offset(base_ctx,
                     ret = print_sub_buffer(
      base_ctx,
   device,
   BUFFER_TYPE_NONE,
   PVR_DEV_ADDR_OFFSET(pds_heap_base, stream_out2.pds_data_addr.addr),
   stream_out1.pds_data_size,
      if (!ret)
      } else {
      pvr_dump_field_member_not_present(base_ctx, &stream_out1, pds_temp_size);
   pvr_dump_field_member_not_present(base_ctx, &stream_out1, pds_data_size);
   pvr_dump_field_member_not_present(base_ctx, &stream_out1, sync);
                     end_pop_ctx:
            end_out:
         }
      static uint32_t
   print_block_ppp_state_terminate(struct pvr_dump_csb_ctx *const csb_ctx)
   {
      struct pvr_dump_csb_block_ctx ctx;
   struct pvr_dump_ctx *const base_ctx = &ctx.base.base;
   uint32_t words_read = 0;
            struct PVRX(TA_STATE_TERMINATE0) terminate0 = { 0 };
            if (!pvr_dump_csb_block_ctx_push(&ctx, csb_ctx, "STATE_TERMINATE"))
            if (!pvr_dump_csb_block_take_packed(&ctx, TA_STATE_TERMINATE0, &terminate0) ||
      !pvr_dump_csb_block_take_packed(&ctx, TA_STATE_TERMINATE1, &terminate1)) {
      }
            pvr_dump_field_member_u32_scaled_units(base_ctx,
                           pvr_dump_field_member_u32_scaled_units(base_ctx,
                           pvr_dump_field_member_u32_scaled_units(base_ctx,
                           pvr_dump_field_member_u32_scaled_units(base_ctx,
                                          end_pop_ctx:
            end_out:
         }
      /******************************************************************************
         ******************************************************************************/
      static bool print_block_hex(struct pvr_dump_buffer_ctx *const ctx,
         {
               if (!nr_words)
                              pvr_dump_indent(&ctx->base);
   pvr_dump_buffer_rewind(ctx, nr_bytes);
   pvr_dump_buffer_hex(ctx, nr_bytes);
                        }
      static bool print_cdmctrl_buffer(struct pvr_dump_buffer_ctx *const parent_ctx,
         {
      struct pvr_dump_csb_ctx ctx;
            /* All blocks contain a block_type member in the first word at the same
   * position. We could unpack any block to pick out this discriminant field,
   * but this one has been chosen because it's only one word long.
   */
            if (!pvr_dump_csb_ctx_push(&ctx, parent_ctx))
            do {
      enum PVRX(CDMCTRL_BLOCK_TYPE) block_type;
   const uint32_t *next_word;
            next_word = pvr_dump_buffer_peek(&ctx.base, sizeof(*next_word));
   if (!next_word) {
      ret = false;
               block_type =
         switch (block_type) {
   case PVRX(CDMCTRL_BLOCK_TYPE_COMPUTE_KERNEL):
                  case PVRX(CDMCTRL_BLOCK_TYPE_STREAM_LINK):
                  case PVRX(CDMCTRL_BLOCK_TYPE_STREAM_TERMINATE):
                  default:
      pvr_dump_buffer_print_header_line(
      &ctx.base,
   "<could not decode CDMCTRL block (%u)>",
                  if (!print_block_hex(&ctx.base, words_read))
            if (block_type == PVRX(CDMCTRL_BLOCK_TYPE_STREAM_TERMINATE))
            end_pop_ctx:
                  }
      static bool print_vdmctrl_buffer(struct pvr_dump_buffer_ctx *const parent_ctx,
         {
      struct pvr_dump_csb_ctx ctx;
            /* All blocks contain a block_type member in the first word at the same
   * position. We could unpack any block to pick out this discriminant field,
   * but this one has been chosen because it's only one word long.
   */
            if (!pvr_dump_csb_ctx_push(&ctx, parent_ctx))
            do {
      enum PVRX(VDMCTRL_BLOCK_TYPE) block_type;
   const uint32_t *next_word;
            next_word = pvr_dump_buffer_peek(&ctx.base, sizeof(*next_word));
   if (!next_word) {
      ret = false;
               block_type = pvr_csb_unpack(next_word, VDMCTRL_STREAM_RETURN).block_type;
   switch (block_type) {
   case PVRX(VDMCTRL_BLOCK_TYPE_PPP_STATE_UPDATE):
                  case PVRX(VDMCTRL_BLOCK_TYPE_PDS_STATE_UPDATE):
                  case PVRX(VDMCTRL_BLOCK_TYPE_VDM_STATE_UPDATE):
                  case PVRX(VDMCTRL_BLOCK_TYPE_INDEX_LIST):
                  case PVRX(VDMCTRL_BLOCK_TYPE_STREAM_LINK):
                  case PVRX(VDMCTRL_BLOCK_TYPE_STREAM_RETURN):
                  case PVRX(VDMCTRL_BLOCK_TYPE_STREAM_TERMINATE):
                  default:
      pvr_dump_buffer_print_header_line(
      &ctx.base,
   "<could not decode VDMCTRL block (%u)>",
                  if (!print_block_hex(&ctx.base, words_read))
            if (block_type == PVRX(VDMCTRL_BLOCK_TYPE_STREAM_TERMINATE))
            end_pop_ctx:
                  }
      static bool print_ppp_buffer(struct pvr_dump_buffer_ctx *const parent_ctx,
         {
      struct pvr_dump_csb_ctx ctx;
   uint32_t words_read;
                     if (!pvr_dump_csb_ctx_push(&ctx, parent_ctx))
            words_read = print_block_ppp_state_header(&ctx, &header);
   if (!print_block_hex(&ctx.base, words_read))
            if (header.pres_ispctl_fa || header.pres_ispctl_fb ||
      header.pres_ispctl_ba || header.pres_ispctl_bb ||
   header.pres_ispctl_dbsc) {
   if (!header.pres_ispctl) {
      ret =
                     words_read = print_block_ppp_state_isp(&ctx,
                                 if (!print_block_hex(&ctx.base, words_read))
               if (header.pres_pds_state_ptr0 || header.pres_pds_state_ptr1 ||
      header.pres_pds_state_ptr2 || header.pres_pds_state_ptr3) {
   words_read = print_block_ppp_state_pds(&ctx,
                                 if (!print_block_hex(&ctx.base, words_read))
               if (header.pres_region_clip) {
      words_read = print_block_ppp_region_clip(&ctx);
   if (!print_block_hex(&ctx.base, words_read))
               if (header.pres_viewport) {
      for (uint32_t i = 0; i < header.view_port_count + 1; i++) {
      words_read = print_block_ppp_viewport(&ctx, i);
   if (!print_block_hex(&ctx.base, words_read))
                  if (header.pres_wclamp) {
      words_read = print_block_ppp_wclamp(&ctx);
   if (!print_block_hex(&ctx.base, words_read))
               if (header.pres_outselects) {
      words_read = print_block_ppp_output_sel(&ctx);
   if (!print_block_hex(&ctx.base, words_read))
               if (header.pres_varying_word0 || header.pres_varying_word1 ||
      header.pres_varying_word2) {
   words_read = print_block_ppp_state_varying(&ctx,
                     if (!print_block_hex(&ctx.base, words_read))
               if (header.pres_ppp_ctrl) {
      words_read = print_block_ppp_state_ppp_ctrl(&ctx);
   if (!print_block_hex(&ctx.base, words_read))
               if (header.pres_stream_out_size || header.pres_stream_out_program) {
      words_read =
      print_block_ppp_state_stream_out(&ctx,
                  if (!print_block_hex(&ctx.base, words_read))
               if (header.pres_terminate) {
      words_read = print_block_ppp_state_terminate(&ctx);
   if (!print_block_hex(&ctx.base, words_read))
                     end_pop_ctx:
            end_out:
         }
      /******************************************************************************
         ******************************************************************************/
      static bool print_sub_buffer(struct pvr_dump_ctx *const ctx,
                                 {
      struct pvr_dump_bo_ctx sub_ctx;
   struct pvr_dump_ctx *base_ctx;
   struct pvr_bo *bo;
   uint64_t real_size;
   uint64_t offset;
                     bo = pvr_bo_store_lookup(device, addr);
   if (!bo) {
      if (expected_size) {
      pvr_dump_field(ctx,
                  "<buffer size>",
   } else {
                  /* FIXME: Trace pvr_buffer allocations with pvr_bo_store. */
            /* Not a fatal error; don't let a single bad address halt the dump. */
   ret = true;
                        if (!pvr_dump_bo_ctx_push(&sub_ctx, ctx, device, bo)) {
      pvr_dump_println(&sub_ctx.base.base, "<unable to read buffer>");
                        if (!pvr_dump_buffer_advance(&sub_ctx.base, offset))
                     if (!expected_size) {
      pvr_dump_field(base_ctx,
                  } else if (expected_size > real_size) {
      pvr_dump_field(base_ctx,
                  "<buffer size>",
   "%" PRIu64 " bytes mapped, expected %" PRIu64
   " bytes (from %s)",
   } else {
      pvr_dump_field(base_ctx,
                  "<buffer size>",
                  if (sub_ctx.bo_mapped_in_ctx)
         else
            switch (type) {
   case BUFFER_TYPE_NONE:
      pvr_dump_field(base_ctx, "<content>", "<not decoded>");
   ret = true;
         case BUFFER_TYPE_PPP:
      pvr_dump_field(base_ctx, "<content>", "<decoded as PPP>");
   ret = print_ppp_buffer(&sub_ctx.base, device);
         default:
      pvr_dump_field(base_ctx, "<content>", "<unsupported format>");
               pvr_dump_field_u32_units(&sub_ctx.base.base,
                        pvr_dump_indent(&sub_ctx.base.base);
   pvr_dump_buffer_restart(&sub_ctx.base);
   pvr_dump_buffer_hex(&sub_ctx.base, 0);
         end_pop_ctx:
            end_out:
                  }
      /******************************************************************************
         ******************************************************************************/
      static bool dump_first_buffer(struct pvr_dump_buffer_ctx *const ctx,
               {
               pvr_dump_mark_section(&ctx->base, "First buffer content");
   switch (stream_type) {
   case PVR_CMD_STREAM_TYPE_GRAPHICS:
      ret = print_vdmctrl_buffer(ctx, device);
         case PVR_CMD_STREAM_TYPE_COMPUTE:
      ret = print_cdmctrl_buffer(ctx, device);
         default:
                  if (!ret)
      pvr_dump_println(&ctx->base,
               pvr_dump_buffer_restart(ctx);
   pvr_dump_mark_section(&ctx->base, "First buffer hexdump");
      }
      /******************************************************************************
         ******************************************************************************/
      void pvr_csb_dump(const struct pvr_csb *const csb,
               {
      const uint32_t nr_bos = list_length(&csb->pvr_bo_list);
            struct pvr_dump_bo_ctx first_bo_ctx;
                              pvr_dump_field_u32(&root_ctx, "Frame num", frame_num);
   pvr_dump_field_u32(&root_ctx, "Job num", job_num);
   pvr_dump_field_enum(&root_ctx, "Status", csb->status, vk_Result_to_str);
   pvr_dump_field_enum(&root_ctx,
                        if (nr_bos <= 1) {
         } else {
      /* TODO: Implement multi-buffer dumping. */
   pvr_dump_field_computed(&root_ctx,
                                 if (nr_bos == 0)
            pvr_dump_mark_section(&root_ctx, "Buffer objects");
            if (!pvr_dump_bo_ctx_push(
         &first_bo_ctx,
   &root_ctx,
   device,
      pvr_dump_mark_section(&root_ctx, "First buffer");
   pvr_dump_println(&root_ctx, "<unable to read buffer>");
                              end_dump:
         }
