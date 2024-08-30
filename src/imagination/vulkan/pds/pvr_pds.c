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
      #include <assert.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <string.h>
      #include "pvr_device_info.h"
   #include "pvr_pds.h"
   #include "pvr_rogue_pds_defs.h"
   #include "pvr_rogue_pds_disasm.h"
   #include "pvr_rogue_pds_encode.h"
   #include "util/log.h"
   #include "util/macros.h"
      #define H32(X) (uint32_t)((((X) >> 32U) & 0xFFFFFFFFUL))
   #define L32(X) (uint32_t)(((X)&0xFFFFFFFFUL))
      /*****************************************************************************
   Macro definitions
   *****************************************************************************/
      #define PVR_PDS_DWORD_SHIFT 2
      #define PVR_PDS_CONSTANTS_BLOCK_BASE 0
   #define PVR_PDS_CONSTANTS_BLOCK_SIZE 128
   #define PVR_PDS_TEMPS_BLOCK_BASE 128
   #define PVR_PDS_TEMPS_BLOCK_SIZE 32
      #define PVR_ROGUE_PDSINST_ST_COUNT4_MAX_SIZE PVR_ROGUE_PDSINST_ST_COUNT4_MASK
   #define PVR_ROGUE_PDSINST_LD_COUNT8_MAX_SIZE PVR_ROGUE_PDSINST_LD_COUNT8_MASK
      /* Map PDS temp registers to the CDM values they contain Work-group IDs are only
   * available in the coefficient sync task.
   */
   #define PVR_PDS_CDM_WORK_GROUP_ID_X 0
   #define PVR_PDS_CDM_WORK_GROUP_ID_Y 1
   #define PVR_PDS_CDM_WORK_GROUP_ID_Z 2
   /* Local IDs are available in every task. */
   #define PVR_PDS_CDM_LOCAL_ID_X 0
   #define PVR_PDS_CDM_LOCAL_ID_YZ 1
      #define PVR_PDS_DOUTW_LOWER32 0x0
   #define PVR_PDS_DOUTW_UPPER32 0x1
   #define PVR_PDS_DOUTW_LOWER64 0x2
   #define PVR_PDS_DOUTW_LOWER128 0x3
   #define PVR_PDS_DOUTW_MAXMASK 0x4
      #define ROGUE_PDS_FIXED_PIXEL_SHADER_DATA_SIZE 8U
   #define PDS_ROGUE_TA_STATE_PDS_ADDR_ALIGNSIZE (16U)
      /*****************************************************************************
   Static variables
   *****************************************************************************/
      static const uint32_t dword_mask_const[PVR_PDS_DOUTW_MAXMASK] = {
      PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_BSIZE_LOWER,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_BSIZE_UPPER,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_BSIZE_ALL64,
      };
      /* If has_slc_mcu_cache_control is enabled use cache_control_const[0], else use
   * cache_control_const[1].
   */
   static const uint32_t cache_control_const[2][2] = {
      { PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_CMODE_BYPASS,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_CMODE_CACHED },
      };
      /*****************************************************************************
   Function definitions
   *****************************************************************************/
      uint64_t pvr_pds_encode_ld_src0(uint64_t dest,
                           {
               if (PVR_HAS_FEATURE(dev_info, slc_mcu_cache_controls)) {
      encoded |= (cached ? PVR_ROGUE_PDSINST_LD_LD_SRC0_SLCMODE_CACHED
               encoded |= ((src_add & PVR_ROGUE_PDSINST_LD_SRCADD_MASK)
         encoded |= ((count8 & PVR_ROGUE_PDSINST_LD_COUNT8_MASK)
         encoded |= (cached ? PVR_ROGUE_PDSINST_LD_LD_SRC0_CMODE_CACHED
         encoded |= ((dest & PVR_ROGUE_PDSINST_REGS64TP_MASK)
               }
      uint64_t pvr_pds_encode_st_src0(uint64_t src,
                           {
               if (device_info->features.has_slc_mcu_cache_controls) {
      encoded |= (write_through
                     encoded |= ((dst_add & PVR_ROGUE_PDSINST_ST_SRCADD_MASK)
         encoded |= ((count4 & PVR_ROGUE_PDSINST_ST_COUNT4_MASK)
         encoded |= (write_through ? PVR_ROGUE_PDSINST_ST_ST_SRC0_CMODE_WRITE_THROUGH
         encoded |= ((src & PVR_ROGUE_PDSINST_REGS32TP_MASK)
               }
      static ALWAYS_INLINE uint32_t
   pvr_pds_encode_doutw_src1(uint32_t dest,
                           {
      assert(((dword_mask > PVR_PDS_DOUTW_LOWER64) && ((dest & 3) == 0)) ||
                  uint32_t encoded =
                              encoded |=
      cache_control_const[PVR_HAS_FEATURE(dev_info, slc_mcu_cache_controls) ? 0
               }
      static ALWAYS_INLINE uint32_t pvr_pds_encode_doutw64(uint32_t cc,
                     {
      return pvr_pds_inst_encode_dout(cc,
                        }
      static ALWAYS_INLINE uint32_t pvr_pds_encode_doutu(uint32_t cc,
               {
      return pvr_pds_inst_encode_dout(cc,
                        }
      static ALWAYS_INLINE uint32_t pvr_pds_inst_encode_doutc(uint32_t cc,
         {
      return pvr_pds_inst_encode_dout(cc,
                        }
      static ALWAYS_INLINE uint32_t pvr_pds_encode_doutd(uint32_t cc,
                     {
      return pvr_pds_inst_encode_dout(cc,
                        }
      static ALWAYS_INLINE uint32_t pvr_pds_encode_douti(uint32_t cc,
               {
      return pvr_pds_inst_encode_dout(cc,
                        }
      static ALWAYS_INLINE uint32_t pvr_pds_encode_bra(uint32_t srcc,
                     {
      /* Address should be signed but API only allows unsigned value. */
      }
      /**
   * Gets the next constant address and moves the next constant pointer along.
   *
   * \param next_constant Pointer to the next constant address.
   * \param num_constants The number of constants required.
   * \param count The number of constants allocated.
   * \return The address of the next constant.
   */
   static uint32_t pvr_pds_get_constants(uint32_t *next_constant,
               {
               /* Work out starting constant number. For even number of constants, start on
   * a 64-bit boundary.
   */
   if (num_constants & 1)
         else
            /* Update the count with the number of constants actually allocated. */
            /* Move the next constant pointer. */
                        }
      /**
   * Gets the next temp address and moves the next temp pointer along.
   *
   * \param next_temp Pointer to the next temp address.
   * \param num_temps The number of temps required.
   * \param count The number of temps allocated.
   * \return The address of the next temp.
   */
   static uint32_t
   pvr_pds_get_temps(uint32_t *next_temp, uint32_t num_temps, uint32_t *count)
   {
               /* Work out starting temp number. For even number of temps, start on a
   * 64-bit boundary.
   */
   if (num_temps & 1)
         else
            /* Update the count with the number of temps actually allocated. */
            /* Move the next temp pointer. */
            assert((temp + num_temps) <=
               }
      /**
   * Write a 32-bit constant indexed by the long range.
   *
   * \param data_block Pointer to data block to write to.
   * \param index Index within the data to write to.
   * \param dword The 32-bit constant to write.
   */
   static void
   pvr_pds_write_constant32(uint32_t *data_block, uint32_t index, uint32_t dword0)
   {
      /* Check range. */
   assert(index <= (PVR_ROGUE_PDSINST_REGS32_CONST32_UPPER -
                        }
      /**
   * Write a 64-bit constant indexed by the long range.
   *
   * \param data_block Pointer to data block to write to.
   * \param index Index within the data to write to.
   * \param dword0 Lower half of the 64 bit constant.
   * \param dword1 Upper half of the 64 bit constant.
   */
   static void pvr_pds_write_constant64(uint32_t *data_block,
                     {
      /* Has to be on 64 bit boundary. */
            /* Check range. */
   assert((index >> 1) <= (PVR_ROGUE_PDSINST_REGS64_CONST64_UPPER -
            data_block[index + 0] = dword0;
            PVR_PDS_PRINT_DATA("WriteConstant64",
            }
      /**
   * Write a 64-bit constant from a single wide word indexed by the long-range
   * number.
   *
   * \param data_block Pointer to data block to write to.
   * \param index Index within the data to write to.
   * \param word The 64-bit constant to write.
   */
      static void
   pvr_pds_write_wide_constant(uint32_t *data_block, uint32_t index, uint64_t word)
   {
      /* Has to be on 64 bit boundary. */
            /* Check range. */
   assert((index >> 1) <= (PVR_ROGUE_PDSINST_REGS64_CONST64_UPPER -
            data_block[index + 0] = L32(word);
               }
      static void pvr_pds_write_dma_address(uint32_t *data_block,
                           {
      /* Has to be on 64 bit boundary. */
            if (PVR_HAS_FEATURE(dev_info, slc_mcu_cache_controls))
            /* Check range. */
   assert((index >> 1) <= (PVR_ROGUE_PDSINST_REGS64_CONST64_UPPER -
            data_block[index + 0] = L32(address);
               }
      /**
   * External API to append a 64-bit constant to an existing data segment
   * allocation.
   *
   * \param constants Pointer to start of data segment.
   * \param constant_value Value to write to constant.
   * \param data_size The number of constants allocated.
   * \returns The address of the next constant.
   */
   uint32_t pvr_pds_append_constant64(uint32_t *constants,
               {
      /* Calculate next constant from current data size. */
   uint32_t next_constant = *data_size;
            /* Set the value. */
               }
      void pvr_pds_pixel_shader_sa_initialize(
         {
         }
      /**
   * Encode a DMA burst.
   *
   * \param dma_control DMA control words.
   * \param dma_address DMA address.
   * \param dest_offset Destination offset in the attribute.
   * \param dma_size The size of the DMA in words.
   * \param src_address Source address for the burst.
   * \param last Last DMA in program.
   * \param dev_info PVR device info structure.
   * \returns The number of DMA transfers required.
   */
   uint32_t pvr_pds_encode_dma_burst(uint32_t *dma_control,
                                       {
      dma_control[0] = dma_size
         dma_control[0] |= dest_offset
            dma_control[0] |= PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTD_SRC1_CMODE_CACHED |
            if (last)
            dma_address[0] = src_address;
   if (PVR_HAS_FEATURE(dev_info, slc_mcu_cache_controls))
            /* Force to 1 DMA. */
      }
      /* FIXME: use the csbgen interface and pvr_csb_pack.
   * FIXME: use bool for phase_rate_change.
   */
   /**
   * Sets up the USC control words for a DOUTU.
   *
   * \param usc_task_control USC task control structure to be setup.
   * \param execution_address USC execution virtual address.
   * \param usc_temps Number of USC temps.
   * \param sample_rate Sample rate for the DOUTU.
   * \param phase_rate_change Phase rate change for the DOUTU.
   */
   void pvr_pds_setup_doutu(struct pvr_pds_usc_task_control *usc_task_control,
                           {
               /* Set the execution address. */
   pvr_set_usc_execution_address64(&(usc_task_control->src0),
            if (usc_temps > 0) {
      /* Temps are allocated in blocks of 4 dwords. */
   usc_temps =
                  /* Check for losing temps due to too many requested. */
   assert((usc_temps & PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTU_TEMPS_MASK) ==
            usc_task_control->src0 |=
      ((uint64_t)(usc_temps &
                  if (sample_rate > 0) {
      usc_task_control->src0 |=
      ((uint64_t)sample_rate)
            if (phase_rate_change) {
      usc_task_control->src0 |=
         }
      /**
   * Generates the PDS pixel event program.
   *
   * \param program Pointer to the PDS pixel event program.
   * \param buffer Pointer to the buffer for the program.
   * \param gen_mode Generate either a data segment or code segment.
   * \param dev_info PVR device info structure.
   * \returns Pointer to just beyond the buffer for the program.
   */
   uint32_t *
   pvr_pds_generate_pixel_event(struct pvr_pds_event_program *restrict program,
                     {
      uint32_t next_constant = PVR_PDS_CONSTANTS_BLOCK_BASE;
                     /* Copy the DMA control words and USC task control words to constants, then
   * arrange them so that the 64-bit words are together followed by the 32-bit
   * words.
   */
   uint32_t control_constant =
         uint32_t emit_constant =
      pvr_pds_get_constants(&next_constant,
               uint32_t control_word_constant =
      pvr_pds_get_constants(&next_constant,
               if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      /* Src0 for DOUTU. */
   pvr_pds_write_wide_constant(buffer,
                        /* Emit words for end of tile program. */
   for (uint32_t i = 0; i < program->num_emit_word_pairs; i++) {
      pvr_pds_write_constant64(constants,
                           /* Control words. */
   for (uint32_t i = 0; i < program->num_emit_word_pairs; i++) {
      uint32_t doutw = pvr_pds_encode_doutw_src1(
      (2 * i),
   PVR_PDS_DOUTW_LOWER64,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_COMMON_STORE,
                                             else if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* DOUTW the state into the shared register. */
   for (uint32_t i = 0; i < program->num_emit_word_pairs; i++) {
      *buffer++ = pvr_pds_encode_doutw64(
      /* cc */ 0,
   /* END */ 0,
   /* SRC1 */ (control_word_constant + i), /* DOUTW 32-bit Src1 */
   /* SRC0 */ (emit_constant + (2 * i)) >> 1); /* DOUTW 64-bit Src0
            /* Kick the USC. */
   *buffer++ = pvr_pds_encode_doutu(
      /* cc */ 0,
   /* END */ 1,
                     /* Save the data segment Pointer and size. */
   program->data_segment = constants;
   program->data_size = data_size;
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT)
            if (gen_mode == PDS_GENERATE_CODE_SEGMENT)
               }
      /**
   * Checks if any of the vertex streams contains instance data.
   *
   * \param streams Streams contained in the vertex shader.
   * \param num_streams Number of vertex streams.
   * \returns true if one or more of the given vertex streams contains
   *          instance data, otherwise false.
   */
   static bool pvr_pds_vertex_streams_contains_instance_data(
      const struct pvr_pds_vertex_stream *streams,
      {
      for (uint32_t i = 0; i < num_streams; i++) {
      const struct pvr_pds_vertex_stream *vertex_stream = &streams[i];
   if (vertex_stream->instance_data)
                  }
      static uint32_t pvr_pds_get_bank_based_constants(uint32_t num_backs,
                     {
      /* Allocate constant for PDS vertex shader where constant is divided into
   * banks.
   */
                     if (*next_constant >= (num_backs << 3))
            if ((*next_constant % 8) == 0) {
               if (num_constants == 1)
         else
      } else if (num_constants == 1) {
      constant = *next_constant;
      } else {
      *next_constant += 7;
            if (*next_constant >= (num_backs << 3)) {
      *next_constant += 2;
      } else {
            }
      }
      /**
   * Generates a PDS program to load USC vertex inputs based from one or more
   * vertex buffers, each containing potentially multiple elements, and then a
   * DOUTU to execute the USC.
   *
   * \param program Pointer to the description of the program which should be
   *                generated.
   * \param buffer Pointer to buffer that receives the output of this function.
   *               Will either be the data segment or code segment depending on
   *               gen_mode.
   * \param gen_mode Which part to generate, either data segment or
   *                 code segment. If PDS_GENERATE_SIZES is specified, nothing is
   *                 written, but size information in program is updated.
   * \param dev_info PVR device info structure.
   * \returns Pointer to just beyond the buffer for the data - i.e the value
   *          of the buffer after writing its contents.
   */
   /* FIXME: Implement PDS_GENERATE_CODEDATA_SEGMENTS? */
   uint32_t *
   pvr_pds_vertex_shader(struct pvr_pds_vertex_shader_program *restrict program,
                     {
      uint32_t next_constant = PVR_PDS_CONSTANTS_BLOCK_BASE;
   uint32_t next_stream_constant;
   uint32_t next_temp;
   uint32_t usc_control_constant64;
   uint32_t stride_constant32 = 0;
   uint32_t dma_address_constant64 = 0;
   uint32_t dma_control_constant64;
   uint32_t multiplier_constant32 = 0;
            uint32_t temp = 0;
   uint32_t index_temp64 = 0;
   uint32_t num_vertices_temp64 = 0;
   uint32_t pre_index_temp = (uint32_t)(-1);
   bool first_ddmadt = true;
   uint32_t input_register0;
   uint32_t input_register1;
            struct pvr_pds_vertex_stream *vertex_stream;
   struct pvr_pds_vertex_element *vertex_element;
            uint32_t data_size = 0;
   uint32_t code_size = 0;
                     uint32_t consts_size = 0;
   uint32_t vertex_id_control_word_const32 = 0;
   uint32_t instance_id_control_word_const32 = 0;
   uint32_t instance_id_modifier_word_const32 = 0;
   uint32_t geometry_id_control_word_const64 = 0;
            bool any_instanced_stream =
      pvr_pds_vertex_streams_contains_instance_data(program->streams,
         uint32_t base_instance_register = 0;
            bool issue_empty_ddmad = false;
   uint32_t last_stream_index = program->num_streams - 1;
   bool current_p0 = false;
                  #if defined(DEBUG)
      if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      for (uint32_t i = 0; i < program->data_size; i++)
         #endif
         /* Generate the PDS vertex shader program */
   next_temp = PVR_PDS_TEMPS_BLOCK_BASE;
   /* IR0 is in first 32-bit temp, temp[0].32, vertex_Index. */
   input_register0 = pvr_pds_get_temps(&next_temp, 1, &temps_used);
   /* IR1 is in second 32-bit temp, temp[1].32, instance_ID. */
            if (program->iterate_remap_id)
         else
            /* Generate the PDS vertex shader code. The constants in the data block are
   * arranged as follows:
   *
   * 64 bit bank 0        64 bit bank 1          64 bit bank 2    64 bit bank
   * 3 Not used (tmps)    Stride | Multiplier    Address          Control
            /* Find out how many constants are needed by streams. */
   for (uint32_t stream = 0; stream < program->num_streams; stream++) {
      pvr_pds_get_constants(&next_constant,
                     /* If there are no vertex streams allocate the first bank for USC Code
   * Address.
   */
   if (consts_size == 0)
         else
            direct_writes_needed = program->iterate_instance_id ||
            if (!PVR_HAS_FEATURE(dev_info, pds_ddmadt)) {
      /* Evaluate what config of DDMAD should be used for each stream. */
   for (uint32_t stream = 0; stream < program->num_streams; stream++) {
                                 /* The condition for index value is:
   * index * stride + size <= bufferSize (all in unit of byte)
   */
   if (vertex_stream->stride == 0) {
      if (vertex_stream->elements[0].size <=
      vertex_stream->buffer_size_in_bytes) {
   /* index can be any value -> no need to use DDMADT. */
      } else {
      /* No index works -> no need to issue DDMAD instruction.
   */
         } else {
      /* index * stride + size <= bufferSize
   *
   * can be converted to:
   * index <= (bufferSize - size) / stride
   *
   * where maximum index is:
   * integer((bufferSize - size) / stride).
   */
   if (vertex_stream->buffer_size_in_bytes <
      vertex_stream->elements[0].size) {
   /* No index works -> no need to issue DDMAD instruction.
   */
      } else {
      uint32_t max_index = (vertex_stream->buffer_size_in_bytes -
               if (max_index == 0xFFFFFFFFu) {
      /* No need to use DDMADT as all possible indices can
   * pass the test.
   */
      } else {
      /* In this case, test condition can be changed to
   * index < max_index + 1.
   */
   program->streams[stream].num_vertices =
                              if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      pvr_pds_write_constant32(
      buffer,
   program->streams[stream].num_vertices,
                        if ((skip_stream_flag & (1 << stream)) == 0) {
      issue_empty_ddmad = (ddmadt_enables & (1 << stream)) != 0;
            } else {
      if (program->num_streams > 0 &&
      program->streams[program->num_streams - 1].use_ddmadt) {
                  if (direct_writes_needed)
            if (issue_empty_ddmad) {
      /* An empty DMA control const (DMA size = 0) is required in case the
   * last DDMADD is predicated out and last flag does not have any usage.
   */
   empty_dma_control_constant64 =
      pvr_pds_get_bank_based_constants(program->num_streams,
                        /* Assign constants for non stream or base instance if there is any
   * instanced stream.
   */
   if (direct_writes_needed || any_instanced_stream ||
      program->instance_id_modifier) {
   if (program->iterate_vtx_id) {
      vertex_id_control_word_const32 =
      pvr_pds_get_bank_based_constants(program->num_streams,
                        if (program->iterate_instance_id || program->instance_id_modifier) {
      if (program->instance_id_modifier == 0) {
      instance_id_control_word_const32 =
      pvr_pds_get_bank_based_constants(program->num_streams,
               } else {
      instance_id_modifier_word_const32 =
      pvr_pds_get_bank_based_constants(program->num_streams,
                  if ((instance_id_modifier_word_const32 % 2) == 0) {
      instance_id_control_word_const32 =
      pvr_pds_get_bank_based_constants(program->num_streams,
               } else {
      instance_id_control_word_const32 =
         instance_id_modifier_word_const32 =
      pvr_pds_get_bank_based_constants(program->num_streams,
                              if (program->base_instance != 0) {
      base_instance_const32 =
      pvr_pds_get_bank_based_constants(program->num_streams,
                        if (program->iterate_remap_id) {
      geometry_id_control_word_const64 =
      pvr_pds_get_bank_based_constants(program->num_streams,
                           if (program->instance_id_modifier != 0) {
      /* This instanceID modifier is used when a draw array instanced call
   * sourcing from client data cannot fit into vertex buffer and needs to
   * be broken down into several draw calls.
                     if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      pvr_pds_write_constant32(buffer,
            } else if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      *buffer++ = pvr_pds_inst_encode_add32(
      /* cc */ 0x0,
   /* ALUM */ 0, /* Unsigned */
   /* SNA */ 0, /* Add */
   /* SRC0 32b */ instance_id_modifier_word_const32,
   /* SRC1 32b */ input_register1,
               /* Adjust instanceID if necessary. */
   if (any_instanced_stream || program->iterate_instance_id) {
      if (program->base_instance != 0) {
               if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      pvr_pds_write_constant32(buffer,
                                 if (program->draw_indirect) {
                                    next_constant = next_stream_constant = PVR_PDS_CONSTANTS_BLOCK_BASE;
   usc_control_constant64 =
            for (uint32_t stream = 0; stream < program->num_streams; stream++) {
               if ((!PVR_HAS_FEATURE(dev_info, pds_ddmadt)) &&
      ((skip_stream_flag & (1 << stream)) != 0)) {
                        instance_data_with_base_instance =
                  /* Get all 8 32-bit constants at once, only 6 for first stream due to
   * USC constants.
   */
   if (stream == 0) {
      stride_constant32 =
      } else {
                     /* Skip bank 0. */
                        if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      pvr_pds_write_constant32(buffer,
                  /* Vertex stream frequency multiplier. */
   if (vertex_stream->multiplier)
      pvr_pds_write_constant32(buffer,
                  /* Update the code size count and temps count for the above code
   * segment.
   */
   if (vertex_stream->current_state) {
      code_size += 1;
      } else {
               if (vertex_stream->multiplier) {
                                       if ((int32_t)vertex_stream->shift > 0)
         } else if (vertex_stream->shift) {
      code_size += 1;
      } else if (instance_data_with_base_instance) {
                  if (num_temps_required != 0) {
      temp = pvr_pds_get_temps(&next_temp,
            } else {
      temp = vertex_stream->instance_data ? input_register1
               if (instance_data_with_base_instance)
               /* The real code segment. */
   if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* If it's current state stream, then index = 0 always. */
   if (vertex_stream->current_state) {
      /* Put zero in temp. */
      } else if (vertex_stream->multiplier) {
      /* old: Iout = (Iin * (Multiplier+2^24)) >> (Shift+24)
                  /* Put zero in temp. Need zero for add part of the following
   * MAD. MAD source is 64 bit, so need two LIMMs.
   */
   *buffer++ = pvr_pds_inst_encode_limm(0, temp, 0, 0);
   /* Put zero in temp. Need zero for add part of the following
   * MAD.
                  /* old: (Iin * (Multiplier+2^24))
   * new: (Iin * Multiplier)
   */
   *buffer++ = pvr_rogue_inst_encode_mad(
      0, /* Sign of add is positive. */
   0, /* Unsigned ALU mode */
   0, /* Unconditional */
   multiplier_constant32,
                                                            if (shift < -31) {
      /* >> (31) */
   shift_2s_comp = 0xFFFE1;
   *buffer++ = pvr_pds_inst_encode_sftlp64(
      /* cc */ 0,
   /* LOP */ PVR_ROGUE_PDSINST_LOP_NONE,
   /* IM */ 1, /*  enable immediate */
   /* SRC0 */ temp / 2,
   /* SRC1 */ input_register0, /* This won't be used in
                                    /* old: >> (Shift+24)
   * new: >> (shift + 31)
   */
   shift_2s_comp = *((uint32_t *)&shift);
   *buffer++ = pvr_pds_inst_encode_sftlp64(
      /* cc */ 0,
   /* LOP */ PVR_ROGUE_PDSINST_LOP_NONE,
   /* IM */ 1, /*enable immediate */
   /* SRC0 */ temp / 2,
   /* SRC1 */ input_register0, /* This won't be used in
                              if (instance_data_with_base_instance) {
      *buffer++ =
      pvr_pds_inst_encode_add32(0, /* cc */
                           0, /* ALNUM */
   0, /* SNA */
      } else { /* NOT vertex_stream->multiplier */
      if (vertex_stream->shift) {
                                          *buffer++ = pvr_pds_inst_encode_sftlp32(
      /* IM */ 1, /*  enable immediate. */
   /* cc */ 0,
   /* LOP */ PVR_ROGUE_PDSINST_LOP_NONE,
   /* SRC0 */ vertex_stream->instance_data ? input_register1
         /* SRC1 */ input_register0, /* This won't be used in
                           if (instance_data_with_base_instance) {
      *buffer++ =
      pvr_pds_inst_encode_add32(0, /* cc */
                           0, /* ALNUM */
   0, /* SNA */
      } else {
      if (instance_data_with_base_instance) {
      *buffer++ =
      pvr_pds_inst_encode_add32(0, /* cc */
                           0, /* ALNUM */
   0, /* SNA */
   } else {
      /* If the shift instruction doesn't happen, use the IR
   * directly into the following MAD.
   */
   temp = vertex_stream->instance_data ? input_register1
                        if (PVR_HAS_FEATURE(dev_info, pds_ddmadt)) {
      if (vertex_stream->use_ddmadt)
      } else {
      if ((ddmadt_enables & (1 << stream)) != 0) {
      /* Emulate what DDMADT does for range checking. */
   if (first_ddmadt) {
      /* Get an 64 bits temp such that cmp current index with
   * allowed vertex number can work.
   */
   index_temp64 =
      pvr_pds_get_temps(&next_temp, 2, &temps_used); /* 64-bit
                                                                     if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      if (first_ddmadt) {
      /* Set predicate to be P0. */
   *buffer++ = pvr_pds_encode_bra(
      PVR_ROGUE_PDSINST_PREDICATE_KEEP, /* SRCCC
                                 *buffer++ =
                           if (temp != pre_index_temp) {
      *buffer++ = pvr_pds_inst_encode_sftlp32(
      /* IM */ 1, /*  enable immediate. */
   /* cc */ 0,
   /* LOP */ PVR_ROGUE_PDSINST_LOP_NONE,
   /* SRC0 */ temp - PVR_ROGUE_PDSINST_REGS32_TEMP32_LOWER,
                        *buffer++ = pvr_pds_inst_encode_sftlp32(
      /* IM */ 1, /*  enable immediate. */
   /* cc */ 0,
   /* LOP */ PVR_ROGUE_PDSINST_LOP_OR,
   /* SRC0 */ num_vertices_temp64 + 1,
   /* SRC1 */ vertex_stream->num_vertices,
                                          /* Process the elements in the stream. */
   for (uint32_t element = 0; element < vertex_stream->num_elements;
                     vertex_element = &vertex_stream->elements[element];
   /* Check if last DDMAD needs terminate or not. */
   if ((element == (vertex_stream->num_elements - 1)) &&
      (stream == last_stream_index)) {
               /* Get a new set of constants for this element. */
   if (element) {
      /* Get all 8 32 bit constants at once. */
   next_constant =
                              if (vertex_element->component_size == 0) {
      /* Standard DMA.
   *
   * Write the DMA transfer control words into the PDS data
   * section.
   *
                  if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
                           /* Write the address to the constant. */
   pvr_pds_write_dma_address(buffer,
                           dma_address_constant64,
   {
      if (program->stream_patch_offsets) {
      program
                        /* Size is in bytes - round up to nearest 32 bit word. */
                                                                                                if (PVR_HAS_FEATURE(dev_info, pds_ddmadt)) {
      if ((ddmadt_enables & (1 << stream)) != 0) {
      assert(
      ((((uint64_t)vertex_stream->buffer_size_in_bytes
         ~PVR_ROGUE_PDSINST_DDMAD_FIELDS_SRC3_MSIZE_CLRMSK) >>
   PVR_ROGUE_PDSINST_DDMAD_FIELDS_SRC3_MSIZE_SHIFT) ==
      dma_control_word64 =
      (PVR_ROGUE_PDSINST_DDMAD_FIELDS_SRC3_TEST_EN |
   (((uint64_t)vertex_stream->buffer_size_in_bytes
            }
   /* If this is the last dma then also set the last flag. */
   if (terminate) {
                        /* Write the 32-Bit SRC3 word to a 64-bit constant as per
   * spec.
   */
   pvr_pds_write_wide_constant(buffer,
                           if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      if (!PVR_HAS_FEATURE(dev_info, pds_ddmadt)) {
      if ((ddmadt_enables & (1 << stream)) != 0) {
      *buffer++ = pvr_pds_inst_encode_cmp(
      0, /* cc enable */
   PVR_ROGUE_PDSINST_COP_LT, /* Operation */
   index_temp64 >> 1, /* SRC0 (REGS64TP) */
   (num_vertices_temp64 >> 1) +
      PVR_ROGUE_PDSINST_REGS64_TEMP64_LOWER); /* SRC1
         }
   /* Multiply by the vertex stream stride and add the base
   * followed by a DOUTD.
   *
   * dmad32 (C0 * T0) + C1, C2
                        uint32_t cc;
   if (PVR_HAS_FEATURE(dev_info, pds_ddmadt))
                        *buffer++ = pvr_pds_inst_encode_ddmad(
      /* cc */ cc,
   /* END */ 0,
   /* SRC0 */ stride_constant32, /* Stride 32-bit*/
   /* SRC1 */ temp, /* Index 32-bit*/
   /* SRC2 64-bit */ dma_address_constant64 >> 1, /* Stream
                           /* SRC3 64-bit */ dma_control_constant64 >> 1 /* DMA
                                    if ((!PVR_HAS_FEATURE(dev_info, pds_ddmadt)) &&
      ((ddmadt_enables & (1 << stream)) != 0)) {
      }
      } else {
      /* Repeat DMA.
   *
   * Write the DMA transfer control words into the PDS data
   * section.
   *
                                    /* Write the address to the constant. */
   pvr_pds_write_dma_address(buffer,
                                    /* Set up the DMA transfer control word. */
                                             switch (vertex_element->component_size) {
   case 4: {
      dma_control_word |=
            }
   case 3: {
      dma_control_word |=
            }
   case 2: {
      dma_control_word |=
            }
   default: {
      dma_control_word |=
                                                               /* If this is the last dma then also set the last flag. */
   if (terminate) {
                        /* Write the 32-Bit SRC3 word to a 64-bit constant as per
   * spec.
   */
   pvr_pds_write_wide_constant(buffer,
                     if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* Multiply by the vertex stream stride and add the base
   * followed by a DOUTD.
   *
   * dmad32 (C0 * T0) + C1, C2
   * src0 = stride  src1 = index  src2 = baseaddr src3 =
   * doutd part
   */
   *buffer++ = pvr_pds_inst_encode_ddmad(
      /* cc */ 0,
   /* END */ 0,
   /* SRC0 */ stride_constant32, /* Stride 32-bit*/
   /* SRC1 */ temp, /* Index 32-bit*/
   /* SRC2 64-bit */ dma_address_constant64 >> 1, /* Stream
                           /* SRC3 64-bit */ dma_control_constant64 >> 1 /* DMA
                                                      if (issue_empty_ddmad) {
      /* Issue an empty last DDMAD, always executed. */
   if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      pvr_pds_write_wide_constant(
      buffer,
   empty_dma_control_constant64,
                     if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      *buffer++ = pvr_pds_inst_encode_ddmad(
      /* cc */ 0,
   /* END */ 0,
   /* SRC0 */ stride_constant32, /* Stride 32-bit*/
   /* SRC1 */ temp, /* Index 32-bit*/
   /* SRC2 64-bit */ dma_address_constant64 >> 1, /* Stream
                     /* SRC3 64-bit */ empty_dma_control_constant64 >> 1 /* DMA
                                       if (!PVR_HAS_FEATURE(dev_info, pds_ddmadt)) {
      if (current_p0) {
               if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* Revert predicate back to IF0 which is required by DOUTU. */
   *buffer++ =
      pvr_pds_encode_bra(PVR_ROGUE_PDSINST_PREDICATE_KEEP, /* SRCCC
                                 }
   /* Send VertexID if requested. */
   if (program->iterate_vtx_id) {
      if (program->draw_indirect) {
      if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      *buffer++ = pvr_pds_inst_encode_add32(
      /* cc */ 0x0,
   /* ALUM */ 0, /* Unsigned */
   /* SNA */ 1, /* Minus */
   /* SRC0 32b */ input_register0, /* vertexID */
   /* SRC1 32b */ PVR_ROGUE_PDSINST_REGS32_PTEMP32_LOWER, /* base
                                    if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      uint32_t doutw = pvr_pds_encode_doutw_src1(
      program->vtx_id_register,
   PVR_PDS_DOUTW_LOWER32,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_UNIFIED_STORE,
                              pvr_pds_write_constant32(buffer,
            } else if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      *buffer++ = pvr_pds_encode_doutw64(
      /* cc */ 0,
   /* END */ 0,
   /* SRC1 */ vertex_id_control_word_const32, /* DOUTW 32-bit Src1
                              /* Send InstanceID if requested. */
   if (program->iterate_instance_id) {
      if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      uint32_t doutw = pvr_pds_encode_doutw_src1(
      program->instance_id_register,
   PVR_PDS_DOUTW_UPPER32,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_UNIFIED_STORE,
                              pvr_pds_write_constant32(buffer,
            } else if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      *buffer++ = pvr_pds_encode_doutw64(
      /* cc */ 0,
   /* END */ 0,
   /* SRC1 */ instance_id_control_word_const32, /* DOUTW 32-bit Src1 */
                        /* Send remapped index number to vi0. */
   if (program->iterate_remap_id) {
      if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      uint32_t doutw = pvr_pds_encode_doutw_src1(
      0 /* vi0 */,
   PVR_PDS_DOUTW_LOWER32,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_UNIFIED_STORE |
                     pvr_pds_write_constant64(buffer,
                  } else if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      *buffer++ = pvr_pds_encode_doutw64(
      /* cc */ 0,
   /* END */ 0,
   /* SRC1 */ geometry_id_control_word_const64, /* DOUTW 32-bit
                                    /* Copy the USC task control words to constants. */
   if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      pvr_pds_write_wide_constant(buffer,
                           if (program->stream_patch_offsets) {
      /* USC TaskControl is always the first patch. */
                  if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* Conditionally (if last in task) issue the task to the USC
   * (if0) DOUTU src1=USC Code Base address, src2=DOUTU word 2.
            *buffer++ = pvr_pds_encode_doutu(
      /* cc */ 1,
               /* End the program if the Dout did not already end it. */
                        if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      /* Set the data segment pointer and ensure we return 1 past the buffer
   * ptr.
   */
                        program->temps_used = temps_used;
   program->data_size = consts_size;
   program->code_size = code_size;
   program->ddmadt_enables = ddmadt_enables;
   if (!PVR_HAS_FEATURE(dev_info, pds_ddmadt))
               }
      /**
   * Generates a PDS program to load USC compute shader global/local/workgroup
   * sizes/ids and then a DOUTU to execute the USC.
   *
   * \param program Pointer to description of the program that should be
   *                generated.
   * \param buffer Pointer to buffer that receives the output of this function.
   *               This will be either the data segment, or the code depending on
   *               gen_mode.
   * \param gen_mode Which part to generate, either data segment or code segment.
   *                 If PDS_GENERATE_SIZES is specified, nothing is written, but
   *                 size information in program is updated.
   * \param dev_info PVR device info struct.
   * \returns Pointer to just beyond the buffer for the data - i.e. the value of
   *          the buffer after writing its contents.
   */
   uint32_t *
   pvr_pds_compute_shader(struct pvr_pds_compute_shader_program *restrict program,
                     {
      uint32_t usc_control_constant64;
   uint32_t usc_control_constant64_coeff_update = 0;
            uint32_t data_size = 0;
   uint32_t code_size = 0;
   uint32_t temps_used = 0;
            uint32_t barrier_ctrl_word = 0;
            /* Even though there are 3 IDs for local and global we only need max one
   * DOUTW for local, and two for global.
   */
   uint32_t work_group_id_ctrl_words[2] = { 0 };
   uint32_t local_id_ctrl_word = 0;
            /* For the constant value to load into ptemp (SW fence). */
   uint64_t predicate_ld_src0_constant = 0;
            uint32_t cond_render_pred_temp;
            /* 2x 64 bit registers that will mask out the Predicate load. */
         #if defined(DEBUG)
      if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      for (uint32_t j = 0; j < program->data_size; j++)
         #endif
         /* All the compute input registers are in temps. */
                              if (program->kick_usc) {
      /* Copy the USC task control words to constants. */
   usc_control_constant64 =
               if (program->has_coefficient_update_task) {
      usc_control_constant64_coeff_update =
               if (program->conditional_render) {
      predicate_ld_src0_constant =
         cond_render_negate_constant =
         cond_render_pred_mask_constant =
            /* LD will load a 64 bit value. */
   cond_render_pred_temp = pvr_pds_get_temps(&next_temp, 4, &temps_used);
            program->cond_render_const_offset_in_dwords = predicate_ld_src0_constant;
               if ((program->barrier_coefficient != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) ||
      (program->clear_pds_barrier) ||
   (program->kick_usc && program->conditional_render)) {
               if (program->barrier_coefficient != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
      barrier_ctrl_word = pvr_pds_get_constants(&next_constant, 1, &data_size);
   if (PVR_HAS_QUIRK(dev_info, 51210)) {
      barrier_ctrl_word2 =
                  if (program->work_group_input_regs[0] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED ||
      program->work_group_input_regs[1] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
   work_group_id_ctrl_words[0] =
               if (program->work_group_input_regs[2] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
      work_group_id_ctrl_words[1] =
               if ((program->local_input_regs[0] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) ||
      (program->local_input_regs[1] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) ||
   (program->local_input_regs[2] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED)) {
               if (program->add_base_workgroup) {
      for (uint32_t workgroup_component = 0; workgroup_component < 3;
      workgroup_component++) {
   if (program->work_group_input_regs[workgroup_component] !=
      PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
   program
      ->base_workgroup_constant_offset_in_dwords[workgroup_component] =
                  if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      if (program->kick_usc) {
      /* Src0 for DOUTU */
   pvr_pds_write_wide_constant(buffer,
                                 if (program->has_coefficient_update_task) {
      /* Src0 for DOUTU. */
   pvr_pds_write_wide_constant(
      buffer,
   usc_control_constant64_coeff_update,
            if ((program->barrier_coefficient != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) ||
      (program->clear_pds_barrier) ||
   (program->kick_usc && program->conditional_render)) {
   pvr_pds_write_wide_constant(buffer, zero_constant64, 0); /* 64-bit
                     if (program->barrier_coefficient != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
      if (PVR_HAS_QUIRK(dev_info, 51210)) {
      /* Write the constant for the coefficient register write. */
   doutw = pvr_pds_encode_doutw_src1(
      program->barrier_coefficient + 4,
   PVR_PDS_DOUTW_LOWER64,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_COMMON_STORE,
   true,
         }
   /* Write the constant for the coefficient register write. */
   doutw = pvr_pds_encode_doutw_src1(
      program->barrier_coefficient,
   PVR_PDS_DOUTW_LOWER64,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_COMMON_STORE,
               /* Check whether the barrier is going to be the last DOUTW done by
   * the coefficient sync task.
   */
   if ((program->work_group_input_regs[0] ==
      PVR_PDS_COMPUTE_INPUT_REG_UNUSED) &&
   (program->work_group_input_regs[1] ==
   PVR_PDS_COMPUTE_INPUT_REG_UNUSED) &&
   (program->work_group_input_regs[2] ==
   PVR_PDS_COMPUTE_INPUT_REG_UNUSED)) {
                           /* If we want work-group id X, see if we also want work-group id Y. */
   if (program->work_group_input_regs[0] !=
            program->work_group_input_regs[1] !=
         /* Make sure we are going to DOUTW them into adjacent registers
   * otherwise we can't do it in one.
   */
                  doutw = pvr_pds_encode_doutw_src1(
      program->work_group_input_regs[0],
   PVR_PDS_DOUTW_LOWER64,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_COMMON_STORE,
               /* If we don't want the Z work-group id then this is the last one.
   */
   if (program->work_group_input_regs[2] ==
      PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
                  }
   /* If we only want one of X or Y then handle them separately. */
   else {
      if (program->work_group_input_regs[0] !=
      PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
   doutw = pvr_pds_encode_doutw_src1(
      program->work_group_input_regs[0],
   PVR_PDS_DOUTW_LOWER32,
                     /* If we don't want the Z work-group id then this is the last
   * one.
   */
   if (program->work_group_input_regs[2] ==
                        pvr_pds_write_constant32(buffer,
            } else if (program->work_group_input_regs[1] !=
            doutw = pvr_pds_encode_doutw_src1(
      program->work_group_input_regs[1],
   PVR_PDS_DOUTW_UPPER32,
                     /* If we don't want the Z work-group id then this is the last
   * one.
   */
   if (program->work_group_input_regs[2] ==
                        pvr_pds_write_constant32(buffer,
                        /* Handle work-group id Z. */
   if (program->work_group_input_regs[2] !=
      PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
   doutw = pvr_pds_encode_doutw_src1(
      program->work_group_input_regs[2],
   PVR_PDS_DOUTW_UPPER32,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_COMMON_STORE |
                                 /* Handle the local IDs. */
   if ((program->local_input_regs[1] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) ||
                     /* If we want local id Y and Z make sure the compiler wants them in
   * the same register.
   */
   if (!program->flattened_work_groups) {
      if ((program->local_input_regs[1] !=
      PVR_PDS_COMPUTE_INPUT_REG_UNUSED) &&
   (program->local_input_regs[2] !=
   PVR_PDS_COMPUTE_INPUT_REG_UNUSED)) {
   assert(program->local_input_regs[1] ==
                  if (program->local_input_regs[1] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED)
                        /* If we want local id X and (Y or Z) then we can do that in a
   * single 64-bit DOUTW.
   */
                     doutw = pvr_pds_encode_doutw_src1(
      program->local_input_regs[0],
   PVR_PDS_DOUTW_LOWER64,
                                 }
   /* Otherwise just DMA in Y and Z together in a single 32-bit DOUTW.
   */
   else {
      doutw = pvr_pds_encode_doutw_src1(
      dest_reg,
   PVR_PDS_DOUTW_UPPER32,
                                    }
   /* If we don't want Y or Z then just DMA in X in a single 32-bit DOUTW.
   */
   else if (program->local_input_regs[0] !=
            doutw = pvr_pds_encode_doutw_src1(
      program->local_input_regs[0],
   PVR_PDS_DOUTW_LOWER32,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_UNIFIED_STORE |
                                    if (gen_mode == PDS_GENERATE_CODE_SEGMENT ||
      gen_mode == PDS_GENERATE_SIZES) {
   #define APPEND(X)                    \
      if (encode) {                     \
      *buffer = X;                   \
      } else {                          \
                     /* Assert that coeff_update_task_branch_size is > 0 because if it is 0
   * then we will be doing an infinite loop.
   */
   if (gen_mode == PDS_GENERATE_CODE_SEGMENT)
            /* Test whether this is the coefficient update task or not. */
   APPEND(
      pvr_pds_encode_bra(PVR_ROGUE_PDSINST_PREDICATE_IF1, /* SRCC */
                     /* Do we need to initialize the barrier coefficient? */
   if (program->barrier_coefficient != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
      if (PVR_HAS_QUIRK(dev_info, 51210)) {
      /* Initialize the second barrier coefficient registers to zero.
   */
   APPEND(pvr_pds_encode_doutw64(0, /* cc */
                  }
   /* Initialize the coefficient register to zero. */
   APPEND(pvr_pds_encode_doutw64(0, /* cc */
                           if (program->add_base_workgroup) {
      const uint32_t temp_values[3] = { 0, 1, 3 };
   for (uint32_t workgroup_component = 0; workgroup_component < 3;
      workgroup_component++) {
   if (program->work_group_input_regs[workgroup_component] ==
                  APPEND(pvr_pds_inst_encode_add32(
      /* cc */ 0x0,
   /* ALUM */ 0,
   /* SNA */ 0,
   /* SRC0 (R32)*/ PVR_ROGUE_PDSINST_REGS32_CONST32_LOWER +
      program->base_workgroup_constant_offset_in_dwords
      /* SRC1 (R32)*/ PVR_ROGUE_PDSINST_REGS32_TEMP32_LOWER +
      PVR_PDS_CDM_WORK_GROUP_ID_X +
      /* DST  (R32TP)*/ PVR_ROGUE_PDSINST_REGS32TP_TEMP32_LOWER +
                     /* If we are going to put the work-group IDs in coefficients then we
   * just need to do the DOUTWs.
   */
   if ((program->work_group_input_regs[0] !=
      PVR_PDS_COMPUTE_INPUT_REG_UNUSED) ||
   (program->work_group_input_regs[1] !=
                  if (program->work_group_input_regs[0] !=
      PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
      } else {
                  APPEND(pvr_pds_encode_doutw64(0, /* cc */
                                 if (program->work_group_input_regs[2] !=
      PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
   APPEND(pvr_pds_encode_doutw64(
      0, /* cc */
   0, /* END */
   work_group_id_ctrl_words[1], /* SRC1 */
   (PVR_PDS_TEMPS_BLOCK_BASE + PVR_PDS_CDM_WORK_GROUP_ID_Z) >>
            /* Issue the task to the USC. */
   if (program->kick_usc && program->has_coefficient_update_task) {
      APPEND(pvr_pds_encode_doutu(0, /* cc */
                           /* Encode a HALT */
            /* Set the branch size used to skip the coefficient sync task. */
                     /* If we want X and Y or Z, we only need one DOUTW. */
   if ((program->local_input_regs[0] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) &&
      ((program->local_input_regs[1] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) ||
   (program->local_input_regs[2] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED))) {
   local_input_register =
      } else {
      /* If we just want X. */
   if (program->local_input_regs[0] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) {
      local_input_register =
      }
   /* If we just want Y or Z. */
   else if (program->local_input_regs[1] !=
                  program->local_input_regs[2] !=
   local_input_register =
                  if ((program->local_input_regs[0] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) ||
      (program->local_input_regs[1] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED) ||
   (program->local_input_regs[2] != PVR_PDS_COMPUTE_INPUT_REG_UNUSED)) {
   APPEND(pvr_pds_encode_doutw64(0, /* cc */
                                 if (program->clear_pds_barrier) {
      /* Zero the persistent temp (SW fence for context switch). */
   APPEND(pvr_pds_inst_encode_add64(
      0, /* cc */
   PVR_ROGUE_PDSINST_ALUM_UNSIGNED,
   PVR_ROGUE_PDSINST_MAD_SNA_ADD,
   PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER +
         PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER +
         PVR_ROGUE_PDSINST_REGS64TP_PTEMP64_LOWER + 0)); /* dest =
                  /* If this is a fence, issue the DOUTC. */
   if (program->fence) {
      APPEND(pvr_pds_inst_encode_doutc(0, /* cc */
               if (program->kick_usc) {
      if (program->conditional_render) {
      /* Skip if coefficient update task. */
   APPEND(pvr_pds_inst_encode_bra(PVR_ROGUE_PDSINST_PREDICATE_IF1,
                                       /* Load negate constant into temp for CMP. */
   APPEND(pvr_pds_inst_encode_add64(
      0, /* cc */
   PVR_ROGUE_PDSINST_ALUM_UNSIGNED,
   PVR_ROGUE_PDSINST_MAD_SNA_ADD,
   PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER +
         PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER +
                                    for (uint32_t i = 0; i < 4; i++) {
      APPEND(pvr_pds_inst_encode_sftlp32(
      1, /* enable immediate */
   0, /* cc */
   PVR_ROGUE_PDSINST_LOP_AND, /* LOP */
   cond_render_pred_temp + i, /* SRC0 */
                     APPEND(
      pvr_pds_inst_encode_sftlp32(1, /* enable immediate */
                              0, /* cc */
                     APPEND(pvr_pds_inst_encode_limm(0, /* cc */
                              APPEND(pvr_pds_inst_encode_sftlp32(1, /* enable immediate */
                                    0, /* cc */
                     /* Check that the predicate is 0. */
   APPEND(pvr_pds_inst_encode_cmpi(
      0, /* cc */
   PVR_ROGUE_PDSINST_COP_EQ, /* LOP */
                     /* If predicate is 0, skip DOUTU. */
   APPEND(pvr_pds_inst_encode_bra(
      PVR_ROGUE_PDSINST_PREDICATE_P0, /* SRCC:
         0, /* NEG */
   PVR_ROGUE_PDSINST_PREDICATE_KEEP, /* SETC:
                        /* Issue the task to the USC.
   * DoutU src1=USC Code Base address, src2=doutu word 2.
   */
   APPEND(pvr_pds_encode_doutu(1, /* cc */
                              1, /* END */
            /* End the program if the Dout did not already end it. */
   #undef APPEND
               if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      /* Set the data segment pointer and ensure we return 1 past the buffer
   * ptr.
   */
                        /* Require at least one DWORD of PDS data so the program runs. */
            program->temps_used = temps_used;
   program->highest_temp = temps_used;
   program->data_size = data_size;
   if (gen_mode == PDS_GENERATE_SIZES)
               }
      /**
   * Generates the PDS vertex shader data or code block. This program will do a
   * DMA into USC Constants followed by a DOUTU.
   *
   * \param program Pointer to the PDS vertex shader program.
   * \param buffer Pointer to the buffer for the program.
   * \param gen_mode Generate code or data.
   * \param dev_info PVR device information struct.
   * \returns Pointer to just beyond the code/data.
   */
   uint32_t *pvr_pds_vertex_shader_sa(
      struct pvr_pds_vertex_shader_sa_program *restrict program,
   uint32_t *restrict buffer,
   enum pvr_pds_generate_mode gen_mode,
      {
      uint32_t next_constant;
   uint32_t data_size = 0;
            uint32_t usc_control_constant64 = 0;
   uint32_t dma_address_constant64 = 0;
   uint32_t dma_control_constant32 = 0;
   uint32_t doutw_value_constant64 = 0;
   uint32_t doutw_control_constant32 = 0;
   uint32_t fence_constant_word = 0;
   uint32_t *buffer_base;
            uint32_t total_num_doutw =
         uint32_t total_size_dma =
                     /* Copy the DMA control words and USC task control words to constants.
   *
   * Arrange them so that the 64-bit words are together followed by the 32-bit
   * words.
   */
   if (program->kick_usc) {
      usc_control_constant64 =
               if (program->clear_pds_barrier) {
      fence_constant_word =
      }
   dma_address_constant64 = pvr_pds_get_constants(&next_constant,
                  /* Assign all unaligned constants together to avoid alignment issues caused
   * by pvr_pds_get_constants with even allocation sizes.
   */
   doutw_value_constant64 = pvr_pds_get_constants(
      &next_constant,
   total_size_dma + total_num_doutw + program->num_dma_kicks,
      doutw_control_constant32 = doutw_value_constant64 + total_size_dma;
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
               if (program->kick_usc) {
      /* Src0 for DOUTU. */
   pvr_pds_write_wide_constant(buffer_base,
                                             if (program->clear_pds_barrier) {
      /* Encode the fence constant src0. Fence barrier is initialized to
   * zero.
   */
   pvr_pds_write_wide_constant(buffer_base, fence_constant_word, 0);
               if (total_num_doutw > 0) {
      for (uint32_t i = 0; i < program->num_q_word_doutw; i++) {
      /* Write the constant for the coefficient register write. */
   pvr_pds_write_constant64(buffer_base,
                     pvr_pds_write_constant32(
      buffer_base,
   doutw_control_constant32,
   program->q_word_doutw_control[i] |
                     doutw_value_constant64 += 2;
               for (uint32_t i = 0; i < program->num_dword_doutw; i++) {
      /* Write the constant for the coefficient register write. */
   pvr_pds_write_constant32(buffer_base,
               pvr_pds_write_constant32(
      buffer_base,
   doutw_control_constant32,
   program->dword_doutw_control[i] |
                     doutw_value_constant64 += 1;
                           if (program->num_dma_kicks == 1) /* Most-common case. */
   {
      /* Src0 for DOUTD - Address. */
   pvr_pds_write_dma_address(buffer_base,
                              /* Src1 for DOUTD - Control Word. */
   pvr_pds_write_constant32(
      buffer_base,
   dma_control_constant32,
               /* Move the buffer ptr along as we will return 1 past the buffer. */
      } else if (program->num_dma_kicks > 1) {
      for (kick_index = 0; kick_index < program->num_dma_kicks - 1;
      kick_index++) {
   /* Src0 for DOUTD - Address. */
   pvr_pds_write_dma_address(buffer_base,
                              /* Src1 for DOUTD - Control Word. */
   pvr_pds_write_constant32(buffer_base,
               dma_address_constant64 += 2;
               /* Src0 for DOUTD - Address. */
   pvr_pds_write_dma_address(buffer_base,
                              /* Src1 for DOUTD - Control Word. */
   pvr_pds_write_constant32(
      buffer_base,
   dma_control_constant32,
                     } else if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      if (program->clear_pds_barrier) {
      /* Zero the persistent temp (SW fence for context switch). */
   *buffer++ = pvr_pds_inst_encode_add64(
      0, /* cc */
   PVR_ROGUE_PDSINST_ALUM_UNSIGNED,
   PVR_ROGUE_PDSINST_MAD_SNA_ADD,
   PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER +
         PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER +
         PVR_ROGUE_PDSINST_REGS64TP_PTEMP64_LOWER + 0); /* dest =
                  if (total_num_doutw > 0) {
      for (uint32_t i = 0; i < program->num_q_word_doutw; i++) {
      /* Set the coefficient register to data value. */
   *buffer++ = pvr_pds_encode_doutw64(
      /* cc */ 0,
   /* END */ !program->num_dma_kicks && !program->kick_usc &&
                     doutw_value_constant64 += 2;
               for (uint32_t i = 0; i < program->num_dword_doutw; i++) {
      /* Set the coefficient register to data value. */
   *buffer++ = pvr_pds_encode_doutw64(
      /* cc */ 0,
   /* END */ !program->num_dma_kicks && !program->kick_usc &&
                     doutw_value_constant64 += 1;
                  if (program->num_dma_kicks != 0) {
               if (program->num_dma_kicks == 1) /* Most-common case. */
   {
      *buffer++ = pvr_pds_encode_doutd(
      /* cc */ 0,
   /* END */ !program->kick_usc,
   /* SRC1 */ dma_control_constant32, /* DOUTD 32-bit Src1 */
   /* SRC0 */ dma_address_constant64 >> 1); /* DOUTD 64-bit
         } else {
      for (kick_index = 0; kick_index < program->num_dma_kicks;
      kick_index++) {
   *buffer++ = pvr_pds_encode_doutd(
      /* cc */ 0,
   /* END */ (!program->kick_usc) &&
         /* SRC1 */ dma_control_constant32, /* DOUTD 32-bit
               /* SRC0 */ dma_address_constant64 >> 1); /* DOUTD
                  dma_address_constant64 += 2;
                     if (program->kick_usc) {
      /* Kick the USC. */
   *buffer++ = pvr_pds_encode_doutu(
      /* cc */ 0,
   /* END */ 1,
   /* SRC0 */ usc_control_constant64 >> 1); /* DOUTU 64-bit Src0.
            if (!program->kick_usc && program->num_dma_kicks == 0 &&
      total_num_doutw == 0) {
                  code_size = program->num_dma_kicks + total_num_doutw;
   if (program->clear_pds_barrier)
            if (program->kick_usc)
            /* If there are no DMAs and no USC kick then code is HALT only. */
   if (code_size == 0)
            program->data_size = data_size;
               }
      /**
   * Writes the Uniform Data block for the PDS pixel shader secondary attributes
   * program.
   *
   * \param program Pointer to the PDS pixel shader secondary attributes program.
   * \param buffer Pointer to the buffer for the code/data.
   * \param gen_mode Either code or data can be generated or sizes only updated.
   * \returns Pointer to just beyond the buffer for the program/data.
   */
   uint32_t *pvr_pds_pixel_shader_uniform_texture_code(
      struct pvr_pds_pixel_shader_sa_program *restrict program,
   uint32_t *restrict buffer,
      {
      uint32_t *instruction;
   uint32_t code_size = 0;
   uint32_t data_size = 0;
   uint32_t temps_used = 0;
            assert((((uintptr_t)buffer) & (PDS_ROGUE_TA_STATE_PDS_ADDR_ALIGNSIZE - 1)) ==
            assert((gen_mode == PDS_GENERATE_CODE_SEGMENT && buffer) ||
            /* clang-format off */
   /* Shape of code segment (note: clear is different)
   *
   *      Code
   *    +------------+
   *    | BRA if0    |
   *    | DOUTD      |
   *    |  ...       |
   *    | DOUTD.halt |
   *    | uniform    |
   *    | DOUTD      |
   *    |  ...       |
   *    |  ...       |
   *    | DOUTW      |
   *    |  ...       |
   *    |  ...       |
   *    | DOUTU.halt |
   *    | HALT       |
   *    +------------+
   */
   /* clang-format on */
                     /* The clear color can arrive packed in the right form in the first (or
   * first 2) dwords of the shared registers and the program will issue a
   * single doutw for this.
   */
   if (program->clear && program->packed_clear) {
      uint32_t color_constant1 =
            uint32_t control_word_constant1 =
            if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* DOUTW the clear color to the USC constants. Predicate with
   * uniform loading flag (IF0).
   */
   *instruction++ = pvr_pds_encode_doutw64(
      /* cc */ 1, /* Only for uniform loading program. */
   /* END */ program->kick_usc ? 0 : 1, /* Last
                                       } else if (program->clear) {
               if (program->clear_color_dest_reg & 0x1) {
                     color_constant1 = pvr_pds_get_constants(&next_constant, 1, &data_size);
                  control_word_constant1 =
         control_word_constant2 =
                  if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* DOUTW the clear color to the USSE constants. Predicate with
   * uniform loading flag (IF0).
   */
   *instruction++ = pvr_pds_encode_doutw64(
      /* cc */ 1, /* Only for Uniform Loading program */
                     *instruction++ = pvr_pds_encode_doutw64(
      /* cc */ 1, /* Only for Uniform Loading program */
                     *instruction++ = pvr_pds_encode_doutw64(
      /* cc */ 1, /* Only for uniform loading program */
   /* END */ program->kick_usc ? 0 : 1, /* Last
                                       } else {
               /* Put the clear color and control words into the first 8
   * constants.
   */
   color_constant1 = pvr_pds_get_constants(&next_constant, 2, &data_size);
   color_constant2 = pvr_pds_get_constants(&next_constant, 2, &data_size);
   control_word_constant =
                        if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* DOUTW the clear color to the USSE constants. Predicate with
   * uniform loading flag (IF0).
   */
   *instruction++ = pvr_pds_encode_doutw64(
      /* cc */ 1, /* Only for Uniform Loading program */
                     *instruction++ = pvr_pds_encode_doutw64(
      /* cc */ 1, /* Only for uniform loading program */
   /* END */ program->kick_usc ? 0 : 1, /* Last
                     /* SRC1 */ control_word_last_constant, /* DOUTW 32-bit Src1
                              if (program->kick_usc) {
                              if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* Issue the task to the USC.
   *
   * dout ds1[constant_use], ds0[constant_use],
   * ds1[constant_use], emit
   */
   *instruction++ = pvr_pds_encode_doutu(
      /* cc */ 0,
   /* END */ 1,
                              if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* End the program. */
      }
      } else {
      uint32_t total_num_doutw =
         bool both_textures_and_uniforms =
      ((program->num_texture_dma_kicks > 0) &&
   ((program->num_uniform_dma_kicks > 0 || total_num_doutw > 0) ||
               if (both_textures_and_uniforms) {
      /* If the size of a PDS data section is 0, the hardware won't run
   * it. We therefore don't need to branch when there is only a
   * texture OR a uniform update program.
   */
   if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
                     /* Use If0 to BRAnch to uniform code. */
   *instruction++ = pvr_pds_encode_bra(
      /* SRCC */ PVR_ROGUE_PDSINST_PREDICATE_IF0,
   /* NEG */ PVR_ROGUE_PDSINST_NEG_DISABLE,
                              if (program->num_texture_dma_kicks > 0) {
      uint32_t dma_address_constant64;
   uint32_t dma_control_constant32;
   /* Allocate 3 constant spaces for each kick. The 64-bit constants
   * come first followed by the 32-bit constants.
   */
   dma_address_constant64 = PVR_PDS_CONSTANTS_BLOCK_BASE;
                  for (uint32_t dma = 0; dma < program->num_texture_dma_kicks; dma++) {
      code_size += 1;
                  /* DMA the state into the secondary attributes. */
   *instruction++ = pvr_pds_encode_doutd(
      /* cc */ 0,
   /* END */ dma == (program->num_texture_dma_kicks - 1),
   /* SRC1 */ dma_control_constant32, /* DOUT 32-bit Src1 */
   /* SRC0 */ dma_address_constant64 >> 1); /* DOUT
                  dma_address_constant64 += 2;
         } else if (both_textures_and_uniforms) {
      if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* End the program. */
                           /* Reserve space at the beginning of the data segment for the DOUTU Task
   * Control if one is needed.
   */
   if (program->kick_usc) {
      doutu_constant64 =
               /* Allocate 3 constant spaces for each DMA and 2 for a USC kick. The
   * 64-bit constants come first followed by the 32-bit constants.
   */
   uint32_t total_size_dma =
            uint32_t dma_address_constant64 = pvr_pds_get_constants(
      &next_constant,
   program->num_uniform_dma_kicks * 3 + total_size_dma + total_num_doutw,
      uint32_t doutw_value_constant64 =
         uint32_t dma_control_constant32 = doutw_value_constant64 + total_size_dma;
   uint32_t doutw_control_constant32 =
            if (total_num_doutw > 0) {
               if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      for (uint32_t i = 0; i < program->num_q_word_doutw; i++) {
      /* Set the coefficient register to data value. */
   *instruction++ = pvr_pds_encode_doutw64(
      /* cc */ 0,
   /* END */ !program->num_uniform_dma_kicks &&
                                       for (uint32_t i = 0; i < program->num_dword_doutw; i++) {
      /* Set the coefficient register to data value. */
   *instruction++ = pvr_pds_encode_doutw64(
      /* cc */ 0,
   /* END */ !program->num_uniform_dma_kicks &&
                     doutw_value_constant64 += 1;
         }
               if (program->num_uniform_dma_kicks > 0) {
                                       bool last_instruction = false;
   if (!program->kick_usc &&
      (dma == program->num_uniform_dma_kicks - 1)) {
      }
   /* DMA the state into the secondary attributes. */
   *instruction++ = pvr_pds_encode_doutd(
      /* cc */ 0,
   /* END */ last_instruction,
   /* SRC1 */ dma_control_constant32, /* DOUT 32-bit Src1
         /* SRC0 */ dma_address_constant64 >> 1); /* DOUT
                  dma_address_constant64 += 2;
                  if (program->kick_usc) {
      if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* Issue the task to the USC.
   *
   * dout ds1[constant_use], ds0[constant_use],
                  *instruction++ = pvr_pds_encode_doutu(
      /* cc */ 0,
                     } else if (program->num_uniform_dma_kicks == 0 && total_num_doutw == 0) {
      if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* End the program. */
                              /* Minimum temp count is 1. */
   program->temps_used = MAX2(temps_used, 1);
            if (gen_mode == PDS_GENERATE_CODE_SEGMENT)
         else
      }
      /**
   * Writes the Uniform Data block for the PDS pixel shader secondary attributes
   * program.
   *
   * \param program Pointer to the PDS pixel shader secondary attributes program.
   * \param buffer Pointer to the buffer for the code/data.
   * \param gen_mode Either code or data can be generated or sizes only updated.
   * \param dev_info PVR device information struct.
   * \returns Pointer to just beyond the buffer for the program/data.
   */
   uint32_t *pvr_pds_pixel_shader_uniform_texture_data(
      struct pvr_pds_pixel_shader_sa_program *restrict program,
   uint32_t *restrict buffer,
   enum pvr_pds_generate_mode gen_mode,
   bool uniform,
      {
      uint32_t *constants = buffer;
   uint32_t next_constant = PVR_PDS_CONSTANTS_BLOCK_BASE;
   uint32_t temps_used = 0;
            assert((((uintptr_t)buffer) & (PDS_ROGUE_TA_STATE_PDS_ADDR_ALIGNSIZE - 1)) ==
                     /* Shape of data segment (note: clear is different).
   *
   *        Uniform            Texture
   *    +--------------+   +-------------+
   *    | USC Task   L |   | USC Task  L |
   *    |            H |   |           H |
   *    | DMA1 Src0  L |   | DMA1 Src0 L |
   *    |            H |   |           H |
   *    | DMA2 Src0  L |   |             |
   *    |            H |   |             |
   *    | DMA1 Src1    |   | DMA1 Src1   |
   *    | DMA2 Src1    |   |             |
   *    | DOUTW0 Src1  |   |             |
   *    | DOUTW1 Src1  |   |             |
   *    |   ...        |   |             |
   *    | DOUTWn Srcn  |   |             |
   *    | other data   |   |             |
   *    +--------------+   +-------------+
            /* Generate the PDS pixel shader secondary attributes data.
   *
   * Packed Clear
   * The clear color can arrive packed in the right form in the first (or
   * first 2) dwords of the shared registers and the program will issue a
   * single DOUTW for this.
   */
   if (program->clear && uniform && program->packed_clear) {
      uint32_t color_constant1 =
            uint32_t control_word_constant1 =
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
               pvr_pds_write_constant64(constants,
                        /* Load into first constant in common store. */
   doutw = pvr_pds_encode_doutw_src1(
      program->clear_color_dest_reg,
   PVR_PDS_DOUTW_LOWER64,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_COMMON_STORE,
               /* Set the last flag. */
   doutw |= PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_LAST_EN;
         } else if (program->clear && uniform) {
               if (program->clear_color_dest_reg & 0x1) {
                     color_constant1 = pvr_pds_get_constants(&next_constant, 1, &data_size);
                  control_word_constant1 =
         control_word_constant2 =
                                    pvr_pds_write_constant32(constants,
                  pvr_pds_write_constant64(constants,
                        pvr_pds_write_constant32(constants,
                  /* Load into first constant in common store. */
   doutw = pvr_pds_encode_doutw_src1(
      program->clear_color_dest_reg,
   PVR_PDS_DOUTW_LOWER32,
                     pvr_pds_write_constant64(constants,
                        /* Move the destination register along. */
   doutw = pvr_pds_encode_doutw_src1(
      program->clear_color_dest_reg + 1,
   PVR_PDS_DOUTW_LOWER64,
                     pvr_pds_write_constant64(constants,
                        /* Move the destination register along. */
   doutw = pvr_pds_encode_doutw_src1(
      program->clear_color_dest_reg + 3,
   PVR_PDS_DOUTW_LOWER32,
                     /* Set the last flag. */
   doutw |= PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_LAST_EN;
         } else {
               /* Put the clear color and control words into the first 8
   * constants.
   */
   color_constant1 = pvr_pds_get_constants(&next_constant, 2, &data_size);
   color_constant2 = pvr_pds_get_constants(&next_constant, 2, &data_size);
   control_word_constant =
                        if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      uint32_t doutw;
   pvr_pds_write_constant64(constants,
                        pvr_pds_write_constant64(constants,
                        /* Load into first constant in common store. */
   doutw = pvr_pds_encode_doutw_src1(
      program->clear_color_dest_reg,
   PVR_PDS_DOUTW_LOWER64,
                              /* Move the destination register along. */
   doutw &= PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_AO_CLRMSK;
                  /* Set the last flag. */
   doutw |= PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_LAST_EN;
   pvr_pds_write_constant64(constants,
                              /* Constants for the DOUTU Task Control, if needed. */
   if (program->kick_usc) {
                     if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      pvr_pds_write_wide_constant(
      constants,
   doutu_constant64,
   program->usc_task_control.src0); /* 64-bit
               } else {
      if (uniform) {
      /* Reserve space at the beginning of the data segment for the DOUTU
   * Task Control if one is needed.
   */
   if (program->kick_usc) {
                     if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      pvr_pds_write_wide_constant(
      constants,
   doutu_constant64,
               uint32_t total_num_doutw =
                        /* Allocate 3 constant spaces for each kick. The 64-bit constants
   * come first followed by the 32-bit constants.
   */
   uint32_t dma_address_constant64 =
      pvr_pds_get_constants(&next_constant,
                  uint32_t doutw_value_constant64 =
         uint32_t dma_control_constant32 =
                        if (total_num_doutw > 0) {
      if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      for (uint32_t i = 0; i < program->num_q_word_doutw; i++) {
      pvr_pds_write_constant64(
      constants,
   doutw_value_constant64,
   program->q_word_doutw_value[2 * i],
      pvr_pds_write_constant32(
      constants,
   doutw_control_constant32,
   program->q_word_doutw_control[i] |
                                             for (uint32_t i = 0; i < program->num_dword_doutw; i++) {
      pvr_pds_write_constant32(constants,
               pvr_pds_write_constant32(
      constants,
   doutw_control_constant32,
   program->dword_doutw_control[i] |
                           doutw_value_constant64 += 1;
                                       if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      for (kick = 0; kick < program->num_uniform_dma_kicks - 1;
      kick++) {
   /* Copy the dma control words to constants. */
   pvr_pds_write_dma_address(constants,
                                                                  pvr_pds_write_dma_address(constants,
                           pvr_pds_write_constant32(
      constants,
   dma_control_constant32,
   program->uniform_dma_control[kick] |
            } else if (program->num_texture_dma_kicks > 0) {
      /* Allocate 3 constant spaces for each kick. The 64-bit constants
   * come first followed by the 32-bit constants.
   */
   uint32_t dma_address_constant64 =
      pvr_pds_get_constants(&next_constant,
                           if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      uint32_t kick;
   for (kick = 0; kick < program->num_texture_dma_kicks - 1; kick++) {
      /* Copy the DMA control words to constants. */
   pvr_pds_write_dma_address(constants,
                                                                     pvr_pds_write_dma_address(constants,
                              pvr_pds_write_constant32(
      constants,
   dma_control_constant32,
   program->texture_dma_control[kick] |
                  /* Save the data segment pointer and size. */
            /* Minimum temp count is 1. */
   program->temps_used = MAX2(temps_used, 1);
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT)
         else
      }
      /**
   * Generates generic DOUTC PDS program.
   *
   * \param program Pointer to the PDS kick USC.
   * \param buffer Pointer to the buffer for the program.
   * \param gen_mode Either code and data can be generated, or sizes only updated.
   * \returns Pointer to just beyond the buffer for the code or program segment.
   */
   uint32_t *pvr_pds_generate_doutc(struct pvr_pds_fence_program *restrict program,
               {
               /* Automatically get a data size of 1x 128bit chunks. */
            /* Setup the data part. */
   uint32_t *constants = buffer; /* Constants placed at front of buffer. */
   uint32_t *instruction = buffer;
   uint32_t next_constant = PVR_PDS_CONSTANTS_BLOCK_BASE; /* Constants count in
                  /* Update the program sizes. */
   program->data_size = data_size;
   program->code_size = code_size;
            if (gen_mode == PDS_GENERATE_SIZES)
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
               constant = pvr_pds_get_constants(&next_constant, 2, &data_size);
   pvr_pds_write_wide_constant(constants, constant + 0, 0); /* 64-bit
                  uint32_t control_word_constant =
         pvr_pds_write_constant64(constants, control_word_constant, 0, 0); /* 32-bit
                  program->data_size = data_size;
               } else if (gen_mode == PDS_GENERATE_CODE_SEGMENT && instruction) {
      *instruction++ = pvr_pds_inst_encode_doutc(
                           /* End the program. */
   *instruction++ = pvr_pds_inst_encode_halt(0);
                           }
      /**
   * Generates generic kick DOUTU PDS program in a single data+code block.
   *
   * \param control Pointer to the PDS kick USC.
   * \param buffer Pointer to the buffer for the program.
   * \param gen_mode Either code and data can be generated or sizes only updated.
   * \param dev_info PVR device information structure.
   * \returns Pointer to just beyond the buffer for the code or program segment.
   */
   uint32_t *pvr_pds_generate_doutw(struct pvr_pds_doutw_control *restrict control,
                     {
      uint32_t next_constant = PVR_PDS_CONSTANTS_BLOCK_BASE;
   uint32_t doutw;
   uint32_t data_size = 0, code_size = 0;
   uint32_t constant[PVR_PDS_MAX_NUM_DOUTW_CONSTANTS];
            /* Assert if buffer is exceeded. */
            uint32_t *constants = buffer;
            /* Put the constants and control words interleaved in the data region. */
   for (uint32_t const_pair = 0; const_pair < control->num_const64;
      const_pair++) {
   constant[const_pair] =
         control_word_constant[const_pair] =
               if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      /* Data segment points to start of constants. */
            for (uint32_t const_pair = 0; const_pair < control->num_const64;
      const_pair++) {
   pvr_pds_write_constant64(constants,
                        /* Start loading at offset 0. */
   if (control->dest_store == PDS_COMMON_STORE) {
      doutw = pvr_pds_encode_doutw_src1(
      (2 * const_pair),
   PVR_PDS_DOUTW_LOWER64,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_COMMON_STORE,
   false,
   } else {
      doutw = pvr_pds_encode_doutw_src1(
      (2 * const_pair),
   PVR_PDS_DOUTW_LOWER64,
   PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTW_SRC1_DEST_UNIFIED_STORE,
                  if (const_pair + 1 == control->num_const64) {
      /* Set the last flag for the MCU (assume there are no following
   * DOUTD's).
   */
      }
   pvr_pds_write_constant64(constants,
                              } else if (gen_mode == PDS_GENERATE_CODE_SEGMENT && instruction) {
               for (uint32_t const_pair = 0; const_pair < control->num_const64;
      const_pair++) {
   /* DOUTW the PDS data to the USC constants. */
   *instruction++ = pvr_pds_encode_doutw64(
      /* cc */ 0,
   /* END */ control->last_instruction &&
         /* SRC1 */ control_word_constant[const_pair], /* DOUTW 32-bit
                                 if (control->last_instruction) {
      /* End the program. */
   *instruction++ = pvr_pds_inst_encode_halt(0);
                           if (gen_mode == PDS_GENERATE_DATA_SEGMENT)
         else
      }
      /**
   * Generates generic kick DOUTU PDS program in a single data+code block.
   *
   * \param program Pointer to the PDS kick USC.
   * \param buffer Pointer to the buffer for the program.
   * \param start_next_constant Next constant in data segment. Non-zero if another
   *                            instruction precedes the DOUTU.
   * \param cc_enabled If true then the DOUTU is predicated (cc set).
   * \param gen_mode Either code and data can be generated or sizes only updated.
   * \returns Pointer to just beyond the buffer for the code or program segment.
   */
   uint32_t *pvr_pds_kick_usc(struct pvr_pds_kickusc_program *restrict program,
                           {
               /* Automatically get a data size of 2 128bit chunks. */
   uint32_t data_size = ROGUE_PDS_FIXED_PIXEL_SHADER_DATA_SIZE;
   uint32_t code_size = 1; /* Single doutu */
            /* Setup the data part. */
   uint32_t *constants = buffer; /* Constants placed at front of buffer. */
   uint32_t next_constant = PVR_PDS_CONSTANTS_BLOCK_BASE; /* Constants count in
                  /* Update the program sizes. */
   program->data_size = data_size;
   program->code_size = code_size;
            if (gen_mode == PDS_GENERATE_SIZES)
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT ||
      gen_mode == PDS_GENERATE_CODEDATA_SEGMENTS) {
                     pvr_pds_write_wide_constant(constants,
                                    if (gen_mode == PDS_GENERATE_DATA_SEGMENT)
               if (gen_mode == PDS_GENERATE_CODE_SEGMENT ||
      gen_mode == PDS_GENERATE_CODEDATA_SEGMENTS) {
            /* Setup the instruction pointer. */
            /* Issue the task to the USC.
   *
   * dout ds1[constant_use], ds0[constant_use], ds1[constant_use], emit ;
   * halt halt
            *instruction++ = pvr_pds_encode_doutu(
      /* cc */ cc_enabled,
   /* END */ 1,
   /* SRC0 */ (constant + start_next_constant) >> 1); /* DOUTU
               /* Return pointer to just after last instruction. */
               /* Execution should never reach here; keep compiler happy. */
      }
      uint32_t *pvr_pds_generate_compute_barrier_conditional(
      uint32_t *buffer,
      {
               if (gen_mode == PDS_GENERATE_DATA_SEGMENT)
            if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* Test whether this is the coefficient update task or not. */
   *buffer++ = pvr_pds_encode_bra(PVR_ROGUE_PDSINST_PREDICATE_IF0, /* SRCC
                                          /* Encode a HALT. */
            /* Reset the default predicate to IF0. */
   *buffer++ = pvr_pds_encode_bra(PVR_ROGUE_PDSINST_PREDICATE_IF0, /* SRCC
                                                }
      /**
   * Generates program to kick the USC task to store shared.
   *
   * \param program Pointer to the PDS shared register.
   * \param buffer Pointer to the buffer for the program.
   * \param gen_mode Either code and data can be generated or sizes only updated.
   * \param dev_info PVR device information structure.
   * \returns Pointer to just beyond the buffer for the program.
   */
   uint32_t *pvr_pds_generate_shared_storing_program(
      struct pvr_pds_shared_storing_program *restrict program,
   uint32_t *restrict buffer,
   enum pvr_pds_generate_mode gen_mode,
      {
      struct pvr_pds_kickusc_program *kick_usc_program = &program->usc_task;
            if (gen_mode == PDS_GENERATE_SIZES)
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
               constants =
                  constants = pvr_pds_kick_usc(kick_usc_program,
                                                if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* Generate PDS code segment. */
            /* doutw	vi1, vi0
   * doutu	ds1[constant_use], ds0[constant_use], ds1[constant_use],
   * emit
   */
   instruction =
                  /* Offset into data segment follows on from doutw data segment. */
   instruction = pvr_pds_kick_usc(kick_usc_program,
                                                /* Execution should never reach here. */
      }
      uint32_t *pvr_pds_generate_fence_terminate_program(
      struct pvr_pds_fence_program *restrict program,
   uint32_t *restrict buffer,
   enum pvr_pds_generate_mode gen_mode,
      {
      uint32_t data_size = 0;
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      /* Data segment. */
                     /* DOUTC sources are not used, but they must be valid. */
   pvr_pds_generate_doutc(program, constants, PDS_GENERATE_DATA_SEGMENT);
            if (PVR_NEED_SW_COMPUTE_PDS_BARRIER(dev_info)) {
      /* Append a 64-bit constant with value 1. Used to increment ptemp.
   * Return the offset into the data segment.
   */
   program->fence_constant_word =
               program->data_size = data_size;
               if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* Code segment. */
            instruction = pvr_pds_generate_compute_barrier_conditional(
      instruction,
               if (PVR_NEED_SW_COMPUTE_PDS_BARRIER(dev_info)) {
                     /* add64	pt[0], pt[0], #1 */
   *instruction++ = pvr_pds_inst_encode_add64(
      0, /* cc */
   PVR_ROGUE_PDSINST_ALUM_UNSIGNED,
   PVR_ROGUE_PDSINST_MAD_SNA_ADD,
   PVR_ROGUE_PDSINST_REGS64_PTEMP64_LOWER + 0, /* src0 = ptemp[0]
         PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER +
         PVR_ROGUE_PDSINST_REGS64TP_PTEMP64_LOWER + 0); /* dest =
                              /* cmp		pt[0] EQ 0x4 == Number of USC clusters per phantom */
   *instruction++ = pvr_pds_inst_encode_cmpi(
      0, /* cc */
   PVR_ROGUE_PDSINST_COP_EQ,
   PVR_ROGUE_PDSINST_REGS64TP_PTEMP64_LOWER + 0, /* src0
                     /* bra		-1 */
   *instruction++ =
      pvr_pds_encode_bra(0, /* cc */
                     1, /* PVR_ROGUE_PDSINST_BRA_NEG_ENABLE
                  /* DOUTC */
   instruction = pvr_pds_generate_doutc(program,
                        program->code_size = code_size;
               /* Execution should never reach here. */
      }
      /**
   * Generates program to kick the USC task to load shared registers from memory.
   *
   * \param program Pointer to the PDS shared register.
   * \param buffer Pointer to the buffer for the program.
   * \param gen_mode Either code and data can be generated or sizes only updated.
   * \param dev_info PVR device information struct.
   * \returns Pointer to just beyond the buffer for the program.
   */
   uint32_t *pvr_pds_generate_compute_shared_loading_program(
      struct pvr_pds_shared_storing_program *restrict program,
   uint32_t *restrict buffer,
   enum pvr_pds_generate_mode gen_mode,
      {
      struct pvr_pds_kickusc_program *kick_usc_program = &program->usc_task;
            uint32_t next_constant;
   uint32_t data_size = 0;
            /* This needs to persist to the CODE_SEGMENT call. */
   static uint32_t fence_constant_word = 0;
            if (gen_mode == PDS_GENERATE_SIZES)
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
               constants = pvr_pds_generate_doutw(doutw_control,
                              constants = pvr_pds_kick_usc(kick_usc_program,
                                    /* Copy the fence constant value (64-bit). */
   next_constant = data_size; /* Assumes data words fully packed. */
   fence_constant_word =
            /* Encode the fence constant src0 (offset measured from start of data
   * buffer). Fence barrier is initialized to zero.
   */
   pvr_pds_write_wide_constant(buffer, fence_constant_word, zero_constant64);
   /* Update the const size. */
   data_size += 2;
            program->data_size = data_size;
               if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* Generate PDS code segment. */
            /* add64	pt0, c0, c0
   * IF [2x Phantoms]
   * add64	pt1, c0, c0
   * st		[constant_mem_addr], pt0, 4
   * ENDIF
   * doutw	vi1, vi0
   * doutu	ds1[constant_use], ds0[constant_use], ds1[constant_use],
   * emit
   *
   * Zero the persistent temp (SW fence for context switch).
   */
   *instruction++ = pvr_pds_inst_encode_add64(
      0, /* cc */
   PVR_ROGUE_PDSINST_ALUM_UNSIGNED,
   PVR_ROGUE_PDSINST_MAD_SNA_ADD,
   PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER +
      (fence_constant_word >> 1), /* src0
            PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER +
      (fence_constant_word >> 1), /* src1
            PVR_ROGUE_PDSINST_REGS64TP_PTEMP64_LOWER + 0); /* dest = ptemp64[0]
               instruction = pvr_pds_generate_doutw(doutw_control,
                              /* Offset into data segment follows on from doutw data segment. */
   instruction = pvr_pds_kick_usc(kick_usc_program,
                                    program->code_size = code_size;
               /* Execution should never reach here. */
      }
      /**
   * Generates both code and data when gen_mode is not PDS_GENERATE_SIZES.
   * Relies on num_fpu_iterators being initialized for size calculation.
   * Relies on num_fpu_iterators, destination[], and FPU_iterators[] being
   * initialized for program generation.
   *
   * \param program Pointer to the PDS pixel shader program.
   * \param buffer Pointer to the buffer for the program.
   * \param gen_mode Either code and data can be generated or sizes only updated.
   * \returns Pointer to just beyond the buffer for the program.
   */
   uint32_t *pvr_pds_coefficient_loading(
      struct pvr_pds_coeff_loading_program *restrict program,
   uint32_t *restrict buffer,
      {
      uint32_t constant;
   uint32_t *instruction;
            /* Place constants at the front of the buffer. */
   uint32_t *constants = buffer;
   /* Start counting constants from 0. */
            /* Save the data segment pointer and size. */
            total_data_size = 0;
            total_data_size += 2 * program->num_fpu_iterators;
            /* Instructions start where constants finished, but we must take note of
   * alignment.
   *
   * 128-bit boundary = 4 dwords.
   */
   total_data_size = ALIGN_POT(total_data_size, 4);
   if (gen_mode != PDS_GENERATE_SIZES) {
      uint32_t data_size = 0;
                     while (iterator < program->num_fpu_iterators) {
                              /* Write the first iterator. */
   iterator_word =
                  /* Write the destination. */
   iterator_word |=
                  /* If this is the last DOUTI word the "Last Issue" bit should be
   * set.
   */
   if (iterator >= program->num_fpu_iterators) {
                  /* Write the word to the buffer. */
   pvr_pds_write_wide_constant(constants,
                              /* Write the DOUT instruction. */
   *instruction++ = pvr_pds_encode_douti(
      /* cc */ 0,
   /* END */ 0,
            /* Update the last DOUTI instruction to have the END flag set. */
      } else {
                  /* Update the data size and code size. Minimum temp count is 1. */
   program->temps_used = 1;
   program->data_size = total_data_size;
               }
      /**
   * Generate a single ld/st instruction. This can correspond to one or more
   * real ld/st instructions based on the value of count.
   *
   * \param ld true to generate load, false to generate store.
   * \param control Cache mode control.
   * \param temp_index Dest temp for load/source temp for store, in 32bits
   *                   register index.
   * \param address Source for load/dest for store in bytes.
   * \param count Number of dwords for load/store.
   * \param next_constant
   * \param total_data_size
   * \param total_code_size
   * \param buffer Pointer to the buffer for the program.
   * \param data_fence Issue data fence.
   * \param gen_mode Either code and data can be generated or sizes only updated.
   * \param dev_info PVR device information structure.
   * \returns Pointer to just beyond the buffer for the program.
   */
   uint32_t *pvr_pds_generate_single_ldst_instruction(
      bool ld,
   const struct pvr_pds_ldst_control *control,
   uint32_t temp_index,
   uint64_t address,
   uint32_t count,
   uint32_t *next_constant,
   uint32_t *total_data_size,
   uint32_t *total_code_size,
   uint32_t *restrict buffer,
   bool data_fence,
   enum pvr_pds_generate_mode gen_mode,
      {
      /* A single ld/ST here does NOT actually correspond to a single ld/ST
   * instruction, but may needs multiple ld/ST instructions because each ld/ST
   * instruction can only ld/ST a restricted max number of dwords which may
   * less than count passed here.
            uint32_t num_inst;
            if (ld) {
      /* ld must operate on 64bits unit, and it needs to load from and to 128
   * bits aligned. Apart from the last ld, all the other need to ld 2x(x =
   * 1, 2, ...) times 64bits unit.
   */
   uint32_t per_inst_count = 0;
            assert((gen_mode == PDS_GENERATE_SIZES) ||
                  count >>= 1;
            /* Found out how many ld instructions are needed and ld size for the all
   * possible ld instructions.
   */
   if (count <= PVR_ROGUE_PDSINST_LD_COUNT8_MAX_SIZE) {
      num_inst = 1;
      } else {
      per_inst_count = PVR_ROGUE_PDSINST_LD_COUNT8_MAX_SIZE;
                  num_inst = count / per_inst_count;
   last_inst_count = count - per_inst_count * num_inst;
               /* Generate all the instructions. */
   for (uint32_t i = 0; i < num_inst; i++) {
                                                      ld_src0 |= (((address >> 2) & PVR_ROGUE_PDSINST_LD_SRCADD_MASK)
         ld_src0 |= (((uint64_t)((i == num_inst - 1) ? last_inst_count
                                                                  } else {
                  /* Write it to the constant. */
   pvr_pds_write_constant64(buffer,
                        /* Adjust value for next ld instruction. */
   temp_index += per_inst_count;
                                 if (data_fence)
            } else {
      /* ST needs source memory address to be 32bits aligned. */
            /* Found out how many ST instructions are needed, each ST can only store
   * PVR_ROGUE_PDSINST_ST_COUNT4_MASK number of 32bits.
   */
   num_inst = count / PVR_ROGUE_PDSINST_ST_COUNT4_MAX_SIZE;
            /* Generate all the instructions. */
   for (uint32_t i = 0; i < num_inst; i++) {
                     if (gen_mode == PDS_GENERATE_DATA_SEGMENT) {
      uint32_t per_inst_count =
      (count <= PVR_ROGUE_PDSINST_ST_COUNT4_MAX_SIZE
                     st_src0 |= (((address >> 2) & PVR_ROGUE_PDSINST_ST_SRCADD_MASK)
         st_src0 |=
      (((uint64_t)per_inst_count & PVR_ROGUE_PDSINST_ST_COUNT4_MASK)
                                                         } else {
                  /* Write it to the constant. */
   pvr_pds_write_constant64(buffer,
                        /* Adjust value for next ST instruction. */
   temp_index += per_inst_count;
   count -= per_inst_count;
                                 if (data_fence)
                     (*total_code_size) += num_inst;
   if (data_fence)
            if (gen_mode != PDS_GENERATE_SIZES)
            }
      /**
   * Generate programs used to prepare stream out, i.e., clear stream out buffer
   * overflow flags and update Persistent temps by a ld instruction.
   *
   * This must be used in PPP state update.
   *
   * \param program Pointer to the stream out program.
   * \param buffer Pointer to the buffer for the program.
   * \param store_mode If true then the data is stored to memory. If false then
   *                   the data is loaded from memory.
   * \param gen_mode Either code and data can be generated or sizes only updated.
   * \param dev_info PVR device information structure.
   * \returns Pointer to just beyond the buffer for the program.
   */
   uint32_t *pvr_pds_generate_stream_out_init_program(
      struct pvr_pds_stream_out_init_program *restrict program,
   uint32_t *restrict buffer,
   bool store_mode,
   enum pvr_pds_generate_mode gen_mode,
      {
      uint32_t total_data_size = 0;
            /* Start counting constants from 0. */
                     if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* We only need to clear global stream out predicate, other predicates
   * are not used during the stream out buffer overflow test.
   */
               for (uint32_t index = 0; index < program->num_buffers; index++) {
      if (program->dev_address_for_buffer_data[index] != 0) {
               /* NOTE: store_mode == true case should be handled by
   * StreamOutTerminate.
   */
   buffer = pvr_pds_generate_single_ldst_instruction(
      !store_mode,
   NULL,
   PTDst,
   program->dev_address_for_buffer_data[index],
   program->pds_buffer_data_size[index],
   &next_constant,
   &total_data_size,
   &total_code_size,
   buffer,
   false,
   gen_mode,
                                 if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      /* We need to fence the loading. */
   *buffer++ = pvr_pds_inst_encode_wdf(0);
               /* Save size information to program */
   program->stream_out_init_pds_data_size =
         /* PDS program code size. */
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT)
         else if (gen_mode == PDS_GENERATE_CODE_SEGMENT)
               }
      /**
   * Generate stream out terminate program for stream out.
   *
   * If pds_persistent_temp_size_to_store is 0, the final primitive written value
   * will be stored.
   *
   * If pds_persistent_temp_size_to_store is non 0, the value of persistent temps
   * will be stored into memory.
   *
   * The stream out terminate program is used to update the PPP state and the data
   * and code section cannot be separate.
   *
   * \param program Pointer to the stream out program.
   * \param buffer Pointer to the buffer for the program.
   * \param gen_mode Either code and data can be generated or sizes only updated.
   * \param dev_info PVR device info structure.
   * \returns Pointer to just beyond the buffer for the program.
   */
   uint32_t *pvr_pds_generate_stream_out_terminate_program(
      struct pvr_pds_stream_out_terminate_program *restrict program,
   uint32_t *restrict buffer,
   enum pvr_pds_generate_mode gen_mode,
      {
      uint32_t next_constant;
            /* Start counting constants from 0. */
            /* Generate store program to store persistent temps. */
   buffer = pvr_pds_generate_single_ldst_instruction(
      false,
   NULL,
   PVR_ROGUE_PDSINST_REGS32TP_PTEMP32_LOWER,
   program->dev_address_for_storing_persistent_temp,
   program->pds_persistent_temp_size_to_store,
   &next_constant,
   &total_data_size,
   &total_code_size,
   buffer,
   false,
   gen_mode,
         total_code_size += 2;
   if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      *buffer++ = pvr_pds_inst_encode_wdf(0);
               /* Save size information to program. */
   program->stream_out_terminate_pds_data_size =
         /* PDS program code size. */
            if (gen_mode == PDS_GENERATE_DATA_SEGMENT)
         else if (gen_mode == PDS_GENERATE_CODE_SEGMENT)
               }
      /* DrawArrays works in several steps:
   *
   * 1) load data from draw_indirect buffer
   * 2) tweak data to match hardware formats
   * 3) write data to indexblock
   * 4) signal the VDM to continue
   *
   * This is complicated by HW limitations on alignment, as well as a HWBRN.
   *
   * 1) Load data.
   * Loads _must_ be 128-bit aligned. Because there is no such limitation in the
   * spec we must deal with this by choosing an appropriate earlier address and
   * loading enough dwords that we load the entirety of the buffer.
   *
   * if addr & 0xf:
   *   load [addr & ~0xf] 6 dwords -> tmp[0, 1, 2, 3, 4, 5]
   *   data = tmp[0 + (uiAddr & 0xf) >> 2]...
   * else
   *   load [addr] 4 dwords -> tmp[0, 1, 2, 3]
   *   data = tmp[0]...
   *
   *
   * 2) Tweak data.
   * primCount in the spec does not match the encoding of INDEX_INSTANCE_COUNT in
   * the VDM control stream. We must subtract 1 from the loaded primCount.
   *
   * However, there is a HWBRN that disallows the ADD32 instruction from sourcing
   * a tmp that is non-64-bit-aligned. To work around this, we must move primCount
   * into another tmp that has the correct alignment. Note: this is only required
   * when data = tmp[even], as primCount is data+1:
   *
   * if data = tmp[even]:
   *   primCount = data + 1 = tmp[odd] -- not 64-bit aligned!
   * else:
   *   primCount = data + 1 = tmp[even] -- already aligned, don't need workaround.
   *
   * This boils down to:
   *
   * primCount = data[1]
   * primCountSrc = data[1]
   * if brn_present && (data is even):
   *   mov scratch, primCount
   *   primCountSrc = scratch
   * endif
   * sub primCount, primCountSrc, 1
   *
   * 3) Store Data.
   * Write the now-tweaked data over the top of the indexblock.
   * To ensure the write completes before the VDM re-reads the data, we must cause
   * a data hazard by doing a dummy (dummy meaning we don't care about the
   * returned data) load from the same addresses. Again, because the ld must
   * always be 128-bit aligned (note: the ST is dword-aligned), we must ensure the
   * index block is 128-bit aligned. This is the client driver's responsibility.
   *
   * st data[0, 1, 2] -> (idxblock + 4)
   * load [idxblock] 4 dwords
   *
   * 4) Signal the VDM
   * This is simply a DOUTV with a src1 of 0, indicating the VDM should continue
   * where it is currently fenced on a dummy idxblock that has been inserted by
   * the driver.
   */
      #include "pvr_draw_indirect_arrays0.h"
   #include "pvr_draw_indirect_arrays1.h"
   #include "pvr_draw_indirect_arrays2.h"
   #include "pvr_draw_indirect_arrays3.h"
      #include "pvr_draw_indirect_arrays_base_instance0.h"
   #include "pvr_draw_indirect_arrays_base_instance1.h"
   #include "pvr_draw_indirect_arrays_base_instance2.h"
   #include "pvr_draw_indirect_arrays_base_instance3.h"
      #include "pvr_draw_indirect_arrays_base_instance_drawid0.h"
   #include "pvr_draw_indirect_arrays_base_instance_drawid1.h"
   #include "pvr_draw_indirect_arrays_base_instance_drawid2.h"
   #include "pvr_draw_indirect_arrays_base_instance_drawid3.h"
      #define ENABLE_SLC_MCU_CACHE_CONTROLS(device)        \
      ((device)->features.has_slc_mcu_cache_controls    \
      ? PVR_ROGUE_PDSINST_LD_LD_SRC0_SLCMODE_CACHED \
      void pvr_pds_generate_draw_arrays_indirect(
      struct pvr_pds_drawindirect_program *restrict program,
   uint32_t *restrict buffer,
   enum pvr_pds_generate_mode gen_mode,
      {
      if ((gen_mode == PDS_GENERATE_CODE_SEGMENT) ||
      (gen_mode == PDS_GENERATE_SIZES)) {
   const struct pvr_psc_program_output *psc_program = NULL;
   switch ((program->arg_buffer >> 2) % 4) {
   case 0:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      psc_program =
      } else {
            } else {
         }
      case 1:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      psc_program =
      } else {
            } else {
         }
      case 2:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      psc_program =
      } else {
            } else {
         }
      case 3:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      psc_program =
      } else {
            } else {
         }
               if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      memcpy(buffer,
      #if defined(DUMP_PDS)
               #endif
                     } else {
      switch ((program->arg_buffer >> 2) % 4) {
   case 0:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      pvr_write_draw_indirect_arrays_base_instance_drawid0_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_arrays_base_instance_drawid0_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid0_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid0_num_views(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid0_immediates(
      } else {
      pvr_write_draw_indirect_arrays_base_instance0_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_arrays_base_instance0_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance0_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance0_num_views(
      buffer,
            } else {
      pvr_write_draw_indirect_arrays0_di_data(buffer,
                     pvr_write_draw_indirect_arrays0_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays0_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays0_num_views(buffer,
            }
      case 1:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      pvr_write_draw_indirect_arrays_base_instance_drawid1_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_arrays_base_instance_drawid1_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid1_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid1_num_views(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid1_immediates(
      } else {
      pvr_write_draw_indirect_arrays_base_instance1_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_arrays_base_instance1_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance1_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance1_num_views(
      buffer,
            } else {
      pvr_write_draw_indirect_arrays1_di_data(buffer,
                     pvr_write_draw_indirect_arrays1_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays1_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays1_num_views(buffer,
            }
      case 2:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      pvr_write_draw_indirect_arrays_base_instance_drawid2_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_arrays_base_instance_drawid2_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid2_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid2_num_views(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid2_immediates(
      } else {
      pvr_write_draw_indirect_arrays_base_instance2_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_arrays_base_instance2_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance2_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance2_num_views(
      buffer,
            } else {
      pvr_write_draw_indirect_arrays2_di_data(buffer,
                     pvr_write_draw_indirect_arrays2_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays2_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays2_num_views(buffer,
            }
      case 3:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      pvr_write_draw_indirect_arrays_base_instance_drawid3_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_arrays_base_instance_drawid3_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid3_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid3_num_views(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance_drawid3_immediates(
      } else {
      pvr_write_draw_indirect_arrays_base_instance3_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_arrays_base_instance3_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance3_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays_base_instance3_num_views(
      buffer,
            } else {
      pvr_write_draw_indirect_arrays3_di_data(buffer,
                     pvr_write_draw_indirect_arrays3_write_vdm(
      buffer,
      pvr_write_draw_indirect_arrays3_flush_vdm(
      buffer,
      pvr_write_draw_indirect_arrays3_num_views(buffer,
            }
            }
      #include "pvr_draw_indirect_elements0.h"
   #include "pvr_draw_indirect_elements1.h"
   #include "pvr_draw_indirect_elements2.h"
   #include "pvr_draw_indirect_elements3.h"
   #include "pvr_draw_indirect_elements_base_instance0.h"
   #include "pvr_draw_indirect_elements_base_instance1.h"
   #include "pvr_draw_indirect_elements_base_instance2.h"
   #include "pvr_draw_indirect_elements_base_instance3.h"
   #include "pvr_draw_indirect_elements_base_instance_drawid0.h"
   #include "pvr_draw_indirect_elements_base_instance_drawid1.h"
   #include "pvr_draw_indirect_elements_base_instance_drawid2.h"
   #include "pvr_draw_indirect_elements_base_instance_drawid3.h"
      void pvr_pds_generate_draw_elements_indirect(
      struct pvr_pds_drawindirect_program *restrict program,
   uint32_t *restrict buffer,
   enum pvr_pds_generate_mode gen_mode,
      {
      if ((gen_mode == PDS_GENERATE_CODE_SEGMENT) ||
      (gen_mode == PDS_GENERATE_SIZES)) {
   const struct pvr_psc_program_output *psc_program = NULL;
   switch ((program->arg_buffer >> 2) % 4) {
   case 0:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      psc_program =
      } else {
            } else {
         }
      case 1:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      psc_program =
      } else {
            } else {
         }
      case 2:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      psc_program =
      } else {
            } else {
         }
      case 3:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      psc_program =
      } else {
            } else {
         }
               if (gen_mode == PDS_GENERATE_CODE_SEGMENT) {
      memcpy(buffer,
         #if defined(DUMP_PDS)
               #endif
                     } else {
      switch ((program->arg_buffer >> 2) % 4) {
   case 0:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      pvr_write_draw_indirect_elements_base_instance_drawid0_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_elements_base_instance_drawid0_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid0_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid0_num_views(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid0_idx_stride(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid0_idx_base(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid0_idx_header(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid0_immediates(
      } else {
      pvr_write_draw_indirect_elements_base_instance0_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_elements_base_instance0_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance0_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance0_num_views(
      buffer,
      pvr_write_draw_indirect_elements_base_instance0_idx_stride(
      buffer,
      pvr_write_draw_indirect_elements_base_instance0_idx_base(
      buffer,
      pvr_write_draw_indirect_elements_base_instance0_idx_header(
      buffer,
      pvr_write_draw_indirect_elements_base_instance0_immediates(
         } else {
      pvr_write_draw_indirect_elements0_di_data(buffer,
                     pvr_write_draw_indirect_elements0_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements0_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements0_num_views(buffer,
         pvr_write_draw_indirect_elements0_idx_stride(buffer,
         pvr_write_draw_indirect_elements0_idx_base(buffer,
         pvr_write_draw_indirect_elements0_idx_header(
      buffer,
         }
      case 1:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      pvr_write_draw_indirect_elements_base_instance_drawid1_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_elements_base_instance_drawid1_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid1_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid1_num_views(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid1_idx_stride(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid1_idx_base(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid1_idx_header(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid1_immediates(
      } else {
      pvr_write_draw_indirect_elements_base_instance1_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_elements_base_instance1_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance1_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance1_num_views(
      buffer,
      pvr_write_draw_indirect_elements_base_instance1_idx_stride(
      buffer,
      pvr_write_draw_indirect_elements_base_instance1_idx_base(
      buffer,
      pvr_write_draw_indirect_elements_base_instance1_idx_header(
      buffer,
      pvr_write_draw_indirect_elements_base_instance1_immediates(
         } else {
      pvr_write_draw_indirect_elements1_di_data(buffer,
                     pvr_write_draw_indirect_elements1_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements1_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements1_num_views(buffer,
         pvr_write_draw_indirect_elements1_idx_stride(buffer,
         pvr_write_draw_indirect_elements1_idx_base(buffer,
         pvr_write_draw_indirect_elements1_idx_header(
      buffer,
         }
      case 2:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      pvr_write_draw_indirect_elements_base_instance_drawid2_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_elements_base_instance_drawid2_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid2_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid2_num_views(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid2_idx_stride(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid2_idx_base(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid2_idx_header(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid2_immediates(
      } else {
      pvr_write_draw_indirect_elements_base_instance2_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_elements_base_instance2_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance2_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance2_num_views(
      buffer,
      pvr_write_draw_indirect_elements_base_instance2_idx_stride(
      buffer,
      pvr_write_draw_indirect_elements_base_instance2_idx_base(
      buffer,
      pvr_write_draw_indirect_elements_base_instance2_idx_header(
      buffer,
      pvr_write_draw_indirect_elements_base_instance2_immediates(
         } else {
      pvr_write_draw_indirect_elements2_di_data(buffer,
                     pvr_write_draw_indirect_elements2_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements2_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements2_num_views(buffer,
         pvr_write_draw_indirect_elements2_idx_stride(buffer,
         pvr_write_draw_indirect_elements2_idx_base(buffer,
         pvr_write_draw_indirect_elements2_idx_header(
      buffer,
         }
      case 3:
      if (program->support_base_instance) {
      if (program->increment_draw_id) {
      pvr_write_draw_indirect_elements_base_instance_drawid3_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_elements_base_instance_drawid3_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid3_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid3_num_views(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid3_idx_stride(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid3_idx_base(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid3_idx_header(
      buffer,
      pvr_write_draw_indirect_elements_base_instance_drawid3_immediates(
      } else {
      pvr_write_draw_indirect_elements_base_instance3_di_data(
      buffer,
   program->arg_buffer & ~0xfull,
      pvr_write_draw_indirect_elements_base_instance3_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance3_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements_base_instance3_num_views(
      buffer,
      pvr_write_draw_indirect_elements_base_instance3_idx_stride(
      buffer,
      pvr_write_draw_indirect_elements_base_instance3_idx_base(
      buffer,
      pvr_write_draw_indirect_elements_base_instance3_idx_header(
      buffer,
      pvr_write_draw_indirect_elements_base_instance3_immediates(
         } else {
      pvr_write_draw_indirect_elements3_di_data(buffer,
                     pvr_write_draw_indirect_elements3_write_vdm(
      buffer,
      pvr_write_draw_indirect_elements3_flush_vdm(
      buffer,
      pvr_write_draw_indirect_elements3_num_views(buffer,
         pvr_write_draw_indirect_elements3_idx_stride(buffer,
         pvr_write_draw_indirect_elements3_idx_base(buffer,
         pvr_write_draw_indirect_elements3_idx_header(
      buffer,
         }
            }
