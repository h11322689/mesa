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
   #include "pvr_types.h"
   #include "util/log.h"
   #include "util/macros.h"
      #define R32_C(x) ((x) + PVR_ROGUE_PDSINST_REGS32_CONST32_LOWER)
   #define R32_T(x) ((x) + PVR_ROGUE_PDSINST_REGS32_TEMP32_LOWER)
   #define R32_P(x) ((x) + PVR_ROGUE_PDSINST_REGS32_PTEMP32_LOWER)
      #define R32TP_T(x) ((x) + PVR_ROGUE_PDSINST_REGS32TP_TEMP32_LOWER)
   #define R32TP_P(x) ((x) + PVR_ROGUE_PDSINST_REGS32TP_PTEMP32_LOWER)
      #define R64_C(x) ((x) + PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER)
   #define R64_T(x) ((x) + PVR_ROGUE_PDSINST_REGS64_TEMP64_LOWER)
   #define R64_P(x) ((x) + PVR_ROGUE_PDSINST_REGS64_PTEMP64_LOWER)
      #define R64TP_T(x) ((x) + PVR_ROGUE_PDSINST_REGS64TP_TEMP64_LOWER)
   #define R64TP_P(x) ((x) + PVR_ROGUE_PDSINST_REGS64TP_PTEMP64_LOWER)
      /* 32-bit PTemp index for draw indirect base instance. */
   #define PVR_INDIRECT_BASE_INSTANCE_PTEMP 1U
      /* Number of constants to reserve per DDMAD instruction in the PDS Vertex. */
   #define PVR_PDS_DDMAD_NUM_CONSTS 8
      #if defined(TRACE_PDS)
   /* Some macros for a pretty printing. */
      #   define pvr_debug_pds_const(reg, size, annotation) \
         #   define pvr_debug_pds_temp(reg, size, annotation) \
         #   define pvr_debug_pds_note(...) mesa_logd("              // " __VA_ARGS__)
   #   define pvr_debug_pds_flag(flags, flag) \
         {                                    \
      if ((flags & flag) == flag)       \
      #   define pvr_debug(annotation) mesa_logd(annotation)
      #else
   #   define pvr_debug_pds_const(reg, size, annotation)
   #   define pvr_debug_pds_temp(reg, size, annotation)
   #   define pvr_debug_pds_note(...)
   #   define pvr_debug_pds_flag(flags, flag)
   #   define pvr_debug(annotation)
   #endif
      struct pvr_pds_const_map_entry_write_state {
      const struct pvr_pds_info *PDS_info;
   struct pvr_const_map_entry *entry;
   size_t size_of_last_entry_in_bytes;
   uint32_t entry_count;
      };
      static void pvr_init_pds_const_map_entry_write_state(
      struct pvr_pds_info *PDS_info,
      {
      entry_write_state->PDS_info = PDS_info;
   entry_write_state->entry = PDS_info->entries;
   entry_write_state->size_of_last_entry_in_bytes = 0;
   entry_write_state->entry_count = 0;
      }
      /* Returns a pointer to the next struct pvr_const_map_entry. */
   static void *pvr_prepare_next_pds_const_map_entry(
      struct pvr_pds_const_map_entry_write_state *entry_write_state,
      {
      /* Move on to the next entry. */
   uint8_t *next_entry = ((uint8_t *)entry_write_state->entry +
                  entry_write_state->size_of_last_entry_in_bytes = size_of_next_entry_in_bytes;
   entry_write_state->entry_count++;
            /* Check if we can write into the next entry. */
   assert(entry_write_state->entries_size_in_bytes <=
               }
      static void pvr_write_pds_const_map_entry_vertex_attribute_address(
      struct pvr_pds_const_map_entry_write_state *entry_write_state,
   const struct pvr_pds_vertex_dma *DMA,
   uint32_t const_val,
      {
      pvr_debug_pds_note("DMA %d dwords, stride %d, offset %d, bindingIdx %d",
                              if (use_robust_vertex_fetch) {
      struct pvr_const_map_entry_robust_vertex_attribute_address
            robust_attribute_entry =
      pvr_prepare_next_pds_const_map_entry(entry_write_state,
      robust_attribute_entry->type =
         robust_attribute_entry->const_offset = const_val;
   robust_attribute_entry->binding_index = DMA->binding_index;
   robust_attribute_entry->component_size_in_bytes =
         robust_attribute_entry->offset = DMA->offset;
   robust_attribute_entry->stride = DMA->stride;
   robust_attribute_entry->size_in_dwords = DMA->size_in_dwords;
   robust_attribute_entry->robustness_buffer_offset =
      } else {
               attribute_entry =
      pvr_prepare_next_pds_const_map_entry(entry_write_state,
      attribute_entry->type =
         attribute_entry->const_offset = const_val;
   attribute_entry->binding_index = DMA->binding_index;
   attribute_entry->offset = DMA->offset;
   attribute_entry->stride = DMA->stride;
         }
      static ALWAYS_INLINE uint32_t pvr_pds_encode_doutu(uint32_t cc,
               {
      return pvr_pds_inst_encode_dout(cc,
                        }
      static uint32_t
   pvr_encode_burst(struct pvr_pds_const_map_entry_write_state *entry_write_state,
                  bool last_dma,
   bool halt,
   unsigned int const32,
   unsigned int const64,
      {
               /* Encode literal value. */
   literal_value = dma_size_in_dwords
         literal_value |= destination
         literal_value |= PVR_ROGUE_PDSINST_DOUT_FIELDS_DOUTD_SRC1_CMODE_CACHED |
            if (last_dma)
            /* Create const map entry. */
            literal_entry = pvr_prepare_next_pds_const_map_entry(entry_write_state,
         literal_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32;
   literal_entry->const_offset = const32;
            /* Encode DOUTD */
   return pvr_pds_inst_encode_dout(0,
                        }
      #define pvr_encode_burst_cs(psDataEntry,        \
                              last_dma,           \
   halt,               \
   pvr_encode_burst(                            \
      psDataEntry,                              \
   last_dma,                                 \
   halt,                                     \
   const32,                                  \
   const64,                                  \
   dma_size_in_dwords,                       \
   destination,                              \
      static uint32_t pvr_encode_direct_write(
      struct pvr_pds_const_map_entry_write_state *entry_write_state,
   bool last_dma,
   bool halt,
   unsigned int const32,
   unsigned int const64,
   uint32_t data_mask,
   unsigned int destination,
   uint32_t destination_store,
      {
               uint32_t instruction =
      pvr_pds_inst_encode_dout(0,
                           literal_entry = pvr_prepare_next_pds_const_map_entry(entry_write_state,
         literal_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32;
   literal_entry->const_offset = const32;
            if (PVR_HAS_FEATURE(dev_info, slc_mcu_cache_controls)) {
      literal_entry->literal_value |=
               literal_entry->literal_value |=
            if (data_mask == 0x1) {
      literal_entry->literal_value |=
      } else if (data_mask == 0x2) {
      literal_entry->literal_value |=
      } else {
      literal_entry->literal_value |=
               if (last_dma) {
      literal_entry->literal_value |=
                  }
      /* Constant and Temporary register allocation
   * - reserve space for a 32-bit register or a 64-bit register
   * - returned indices are offsets to 32-bit register locations
   * - 64-bit registers need to be aligned to even indices.
   */
   #define RESERVE_32BIT 1U
   #define RESERVE_64BIT 2U
      #if defined(DEBUG)
   #   define pvr_find_constant(usage, words, name) \
         #   define pvr_get_temps(usage, words, name) pvr_get_temps2(usage, words, name)
   #else
   #   define pvr_find_constant(usage, words, name) \
         #   define pvr_get_temps(usage, words, name) pvr_get_temps2(usage, words, NULL)
   #endif
      static uint32_t
   pvr_find_constant2(uint8_t *const_usage, uint8_t words, const char *const_name)
   {
      uint32_t const_index = ~0;
   uint32_t step = words;
                     /* Find a register at 'step' alignment that satisfies the mask. */
   for (uint32_t i = 0; i < PVR_MAX_VERTEX_ATTRIB_DMAS; i++) {
      for (uint32_t b = 0; b < PVR_PDS_DDMAD_NUM_CONSTS; b += step) {
      if ((const_usage[i] & (mask << b)) != 0)
         const_usage[i] |= (mask << b);
   const_index = i * 8 + b;
   pvr_debug_pds_const(const_index, words * 32, const_name);
                  unreachable("Unexpected: Space cannot be found for constant");
      }
      #define PVR_MAX_PDS_TEMPS 32
   struct pvr_temp_usage {
      uint32_t temp_usage;
   uint8_t temp_used;
      };
      #define PVR_INVALID_TEMP UINT8_C(~0)
      static uint8_t pvr_get_temps2(struct pvr_temp_usage *temps,
               {
      uint8_t step = temps_needed;
            assert(temps_needed == 1 || temps_needed == 2);
            for (uint8_t i = 0; i < PVR_MAX_PDS_TEMPS; i += step) {
      if ((temps->temp_usage & (mask << i)) != 0)
                     temps->temp_usage |= (mask << i);
   temps->temp_used += temps_needed;
   temps->temps_needed =
                                 unreachable("Unexpected: Space cannot be found for temps");
      }
      /**
   * Wrapper macro to add a toggle for "data mode", allowing us to calculate the
   * size of a PDS program without actually attempting to store it.
   *
   * \param dest The array/memory pointer where the PDS program should be stored.
   *             If the given code is NULL, automatically switch to count mode
   *             instead of attempting to fill in unallocated memory.
   * \param counter The local counter that holds the total instruction count.
   * \param statement What function call/value should be stored at dest[counter]
   *                  when condition is false.
   */
      #define PVR_PDS_MODE_TOGGLE(dest, counter, statement) \
      if (!dest) {                                       \
         } else {                                           \
      dest[counter++] = statement;                    \
            /**
   * Generates the PDS vertex primary program for the dma's listed in the input
   * structure. Produces the constant map for the Vulkan driver based upon the
   * requirements of the instructions added to the program.
   *
   * PDS Data Layout
   * ---------------
   *
   * The PDS data is optimized for the DDMAD layout, with the data for those
   * instructions laid out first. The data required for other instructions is laid
   * out in the entries unused by the DDMADs.
   *
   * DDMAD layout
   * \verbatim
   * 	bank | index | usage
   * 	0    |  0:1  | temps (current index)[-]
   * 	2    |  2:3  | stride[32]
   * 	1    |  4:5  | base address[64]
   * 	3    |  6:7  | ctrl[64]
   * \endverbatim
   *
   *  Each DMA whose stride > 0 requires one entry, laid out as above. We stride
   * 	over the banks to ensure that each ddmad reads each of its operands from a
   * 	different bank (i.e. remove bank clashes)
   *
   * 	Note: This is "wasting" const[0:1] and const[2], however these free
   * 	registers will be used by other, non-ddmad instructions.
   *
   * 	The const register usage is maintained in the au8ConstUsage array, the
   * DDMAD instructions, for example, will utilize the top 5 registers in each
   * block of 8 hence a 'usage mask' of 0xF8 (0b11111000).
   *
   * 	Constant Map
   * 	------------
   *
   * 	The constant map is built up as we add PDS instructions and passed back
   * for the driver to fill in the PDS data section with the correct parameters
   * for each draw call.
   *
   * \param input_program PDS Program description.
   * \param code Buffer to be filled in with the PDS program. If NULL is provided,
   *             automatically switch to count-mode, preventing writes to
   *             unallocated memory.
   * \param info PDS info structure filled in for the driver, contains the
   *             constant map.
   * \param use_robust_vertex_fetch Do vertex fetches apply range checking.
   * \param dev_info pvr device information struct.
   */
   void pvr_pds_generate_vertex_primary_program(
      struct pvr_pds_vertex_primary_program_input *input_program,
   uint32_t *code,
   struct pvr_pds_info *info,
   bool use_robust_vertex_fetch,
      {
      struct pvr_pds_const_map_entry_write_state entry_write_state;
            uint32_t instruction = 0; /* index into code */
   uint32_t index; /* index used for current attribute, either vertex or
                  uint32_t total_dma_count = 0;
            uint32_t write_instance_control = ~0;
   uint32_t write_vertex_control = ~0;
   uint32_t write_base_instance_control = ~0;
   uint32_t write_base_vertex_control = ~0;
            uint32_t ddmad_count = 0;
            uint32_t base_instance = 0;
   uint32_t base_vertex = 0;
                                       uint32_t max_index_temp = PVR_INVALID_TEMP;
            uint32_t index_id_temp = PVR_INVALID_TEMP;
   uint32_t base_instance_ID_temp = PVR_INVALID_TEMP;
            /* Debug tracing of program flags. */
   pvr_debug("pvr_pds_generate_vertex_primary_program");
   pvr_debug("=================================================");
   pvr_debug_pds_flag(input_program->flags,
         pvr_debug_pds_flag(input_program->flags,
         pvr_debug_pds_flag(input_program->flags,
         pvr_debug_pds_flag(input_program->flags,
         pvr_debug_pds_flag(input_program->flags,
         pvr_debug_pds_flag(input_program->flags,
         pvr_debug_pds_flag(input_program->flags,
                           /* At a minimum we need 2 dwords for the DOUTU, but since we allocate in
   * blocks of 4 we can reserve dwords for the instance/vertex DOUTW.
   */
            /* Reserve 2 temps - these are automatically filled in by the VDM
   *
   * For instanced draw calls we manually increment the instance id by the
   * base-instance offset which is either provided as a constant, or in a
   * ptemp (for draw indirect)
   *
   * temp - contents
   * ---------------
   * 0    - index id (pre-filled)
   * 1    - base instance + instance id
   */
   index_id_temp = pvr_get_temps(&temp_usage, RESERVE_32BIT, "VDM Index id");
   instance_ID_temp =
            /* Reserve the lowest 2 dwords for DOUTU.
   * [------XX]
   */
            /* Reserve consts for all the DDMAD's. */
   for (uint32_t dma = 0; dma < input_program->dma_count; dma++) {
      /* Mark the consts required by this ddmad "in-use".
   * [XXXXX---]
   */
               /* Start off by assuming we can fit everything in the 8 dwords/ddmad
   * footprint, if any DOUTD/DOUTW falls outside we will increase this
   * counter.
   */
   if (ddmad_count)
            if (input_program->flags & PVR_PDS_VERTEX_FLAGS_VERTEX_ID_REQUIRED) {
      doutw_count++;
   write_vertex_control =
               if (input_program->flags & PVR_PDS_VERTEX_FLAGS_INSTANCE_ID_REQUIRED) {
      doutw_count++;
   write_instance_control = pvr_find_constant(const_usage,
                     if (input_program->flags & PVR_PDS_VERTEX_FLAGS_BASE_INSTANCE_REQUIRED) {
      doutw_count++;
   write_base_instance_control =
      pvr_find_constant(const_usage,
                  if (input_program->flags & PVR_PDS_VERTEX_FLAGS_BASE_VERTEX_REQUIRED) {
      doutw_count++;
   write_base_vertex_control = pvr_find_constant(const_usage,
                  /* Load base vertex from constant for non-indirect variants. */
   if ((input_program->flags & PVR_PDS_VERTEX_FLAGS_DRAW_INDIRECT_VARIANT) ==
      0) {
                                 psBaseVertexEntry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      psBaseVertexEntry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_BASE_VERTEX;
                  if (input_program->flags & PVR_PDS_VERTEX_FLAGS_DRAW_INDEX_REQUIRED) {
      doutw_count++;
   pvr_write_draw_index_control =
            /* Set draw index to 0 for non-indirect variants. */
   if ((input_program->flags & PVR_PDS_VERTEX_FLAGS_DRAW_INDIRECT_VARIANT) ==
                                    literal_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      literal_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32;
   literal_entry->const_offset = draw_index;
                  if (input_program->flags & PVR_PDS_VERTEX_FLAGS_DRAW_INDIRECT_VARIANT) {
      /* Load absolute instance id into uiInstanceIdTemp. */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_add32(
      /* cc    */ 0,
   /* alum  */ 0,
   /* sna   */ 0,
   /* src0  */ R32_P(PVR_INDIRECT_BASE_INSTANCE_PTEMP),
      } else if (input_program->flags &
            struct pvr_const_map_entry_base_instance *base_instance_entry =
            base_instance =
            PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_add32(
      /* cc    */ 0,
               base_instance_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      base_instance_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_BASE_INSTANCE;
      } else if (input_program->flags &
            struct pvr_const_map_entry_base_instance *base_instance_entry =
            base_instance = pvr_find_constant(const_usage,
                  /* Base instance provided by the driver. */
   base_instance_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      base_instance_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_BASE_INSTANCE;
                                 if (use_robust_vertex_fetch) {
               if (PVR_HAS_FEATURE(dev_info, pds_ddmadt)) {
               /* Load 0 into instance_ID_temp. */
   PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_limm(0, /* cc */
      } else {
               max_index_temp =
                        PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_sftlp64(
      0, /* cc */
   PVR_ROGUE_PDSINST_LOP_XOR, /* LOP */
   1, /* IM */
   R64TP_T(zero_temp >> 1), /* SRC0 (REGS64TP)
         R64TP_T(zero_temp >> 1), /* SRC1 (REGS64TP)
      PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_sftlp64(
      0, /* cc */
   PVR_ROGUE_PDSINST_LOP_NONE, /* LOP */
   1, /* IM */
   R64TP_T(zero_temp >> 1), /* SRC0 (REGS64TP)
         0, /* SRC1 (REGS64TP) */
      PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_sftlp64(
      0, /* cc */
   PVR_ROGUE_PDSINST_LOP_NONE, /* LOP */
   1, /* IM */
   R64TP_T(zero_temp >> 1), /* SRC0 (REGS64TP)
         0, /* SRC1 (REGS64TP) */
               if (input_program->dma_count && use_robust_vertex_fetch) {
      PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_bra(PVR_ROGUE_PDSINST_PREDICATE_KEEP, /* SRCC */
                           0, /* Neg */
            for (uint32_t dma = 0; dma < input_program->dma_count; dma++) {
      uint32_t const_base = dma * PVR_PDS_DDMAD_NUM_CONSTS;
   uint32_t control_word;
            const struct pvr_pds_vertex_dma *vertex_dma =
                           /* The id we use to index into this dma. */
   if (vertex_dma->flags & PVR_PDS_VERTEX_DMA_FLAGS_INSTANCE_RATE) {
                     /* 4    - madd 0 - needs to be 64-bit aligned
   * 5    - madd 1
   */
   if (vertex_dma->divisor > 1) {
      const uint32_t adjusted_instance_ID_temp =
      pvr_get_temps(&temp_usage,
                           /* 1. Remove base instance value from temp 1 to get instance id
   * 2. Divide the instance id by the divisor - Iout = (Iin *
   *    Multiplier) >> (shift+31)
   * 3. Add the base instance back on.
   *
                  PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_add64(
                        if (input_program->flags &
      PVR_PDS_VERTEX_FLAGS_DRAW_INDIRECT_VARIANT) {
   /* Subtract base instance from temp 1, put into
   * adjusted_instance_ID_temp.
   */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_add32(
      /* cc    */ 0,
   /* alum  */ 0,
   /* sna   */ 1,
   /* src0  */ R32_T(instance_ID_temp),
      } else if (input_program->flags &
            /* Subtract base instance from temp 1, put into
   * adjusted_instance_ID_temp.
   */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_add32(
      /* cc    */ 0,
   /* alum  */ 0,
   /* sna   */ 1,
   /* src0  */ R32_T(instance_ID_temp),
      } else {
      /* Copy instance from temp 1 to adjusted_instance_ID_temp.
   */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_add32(
      /* cc    */ 0,
   /* alum  */ 0,
   /* sna   */ 0,
   /* src0  */ R32_T(instance_ID_temp),
   /* src1  */ R32_T(MADD_temp), /* MADD_temp is set
                     /* shift = the bit of the next highest power of two. */
   uint32_t shift_unsigned =
                                       const uint32_t multipier_constant =
                        /* multiplier = ( 2^(shift + 31) + (divisor - 1) ) / divisor,
         uint64_t multipier64 =
      (uint64_t)((((uint64_t)1 << ((uint64_t)shift_unsigned + 31)) +
                     pvr_debug_pds_note(" - Value of MultiplierConstant = %u",
                  literal_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      literal_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32;
                  /* (Iin * Multiplier) */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_rogue_inst_encode_mad(0, /* Sign of add is positive */
                                       /*  >> (shift + 31) */
                  if (shift < -31) {
      /* >> (31) */
   shift_2s_comp = 0xFFFE1;
   PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_sftlp64(
      /* cc */ 0,
   /* LOP */ PVR_ROGUE_PDSINST_LOP_NONE,
   /* IM */ 1, /*  enable immediate */
   /* SRC0 */ R64_T(MADD_temp / 2),
   /* SRC1 */ 0, /* This won't be used
                  /* >> (shift + 31) */
   shift_2s_comp = *((uint32_t *)&shift);
   PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_sftlp64(
      /* cc */ 0,
   /* LOP */ PVR_ROGUE_PDSINST_LOP_NONE,
   /* IM */ 1, /*  enable immediate */
                     if (input_program->flags &
      PVR_PDS_VERTEX_FLAGS_DRAW_INDIRECT_VARIANT) {
   /* Add base instance. */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_add32(
      /* cc    */ 0,
   /* alum  */ 0,
   /* sna   */ 0,
   /* src0  */ R32_T(MADD_temp),
      } else if (input_program->flags &
            /* Add base instance. */
   PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_add32(
      /* cc    */ 0,
                  pvr_debug_pds_note(
            } else if (vertex_dma->divisor == 0) {
      if (base_instance_ID_temp == PVR_INVALID_TEMP) {
      base_instance_ID_temp = pvr_get_temps(&temp_usage,
                     /* Load 0 into instance_ID_temp. */
   PVR_PDS_MODE_TOGGLE(code,
                     instruction,
                  if (input_program->flags &
      PVR_PDS_VERTEX_FLAGS_DRAW_INDIRECT_VARIANT) {
   /* Add base instance. */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_add32(
      /* cc    */ 0,
   /* alum  */ 0,
                     } else if (input_program->flags &
            /* Add base instance. */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_add32(
      /* cc    */ 0,
   /* alum  */ 0,
   /* sna   */ 0,
                     pvr_debug_pds_note(
            } else {
      pvr_debug_pds_note(
               } else {
      pvr_debug_pds_note(
                     /* DDMAD Const Usage [__XX_---] */
   pvr_write_pds_const_map_entry_vertex_attribute_address(
      &entry_write_state,
   vertex_dma,
               /* DDMAD Const Usage [__XXX---] */
   literal_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      literal_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32;
   literal_entry->const_offset = const_base + 3;
            control_word = vertex_dma->size_in_dwords
         control_word |= vertex_dma->destination
         control_word |= (PVR_ROGUE_PDSINST_DDMAD_FIELDS_SRC3_DEST_UNIFIED_STORE |
            /* DDMADT instructions will do a dummy doutd when OOB if
   * PVR_ROGUE_PDSINST_DDMAD_FIELDS_SRC3_LAST_EN is set but as the driver
   * would need to do another doutd after an OOB DDMADT to provide the 'in
   * bounds' data the DDMADT can't be set as LAST.
   *
   * This requires us to include a final dummy DDMAD.LAST instruction.
   *
   * Pseudocode taken from SeriesXE2017.PDS Instruction Controller
   * Specification.doc
   *
   *	DDMAD src0,src1,src2,src3
   *
   *	calculated_source_address := src0*src1+src2
   *	base_address              := src2
   *	dma_parameters            := src3[31:0]
   *	buffer_size               := src3[63:33]
   *	test                      := src3[32]
   *
   *	if (test == 1) {
   *	   // DDMAD(T)
   *	   if (calculated_source_address[39:0] + (burst_size<<2) <=
   *         base_address[39:0] + buffer_size) {
   *        OOB := 0
   *        DOUTD calculated_source_address,dma_paramters
   *     } else {
   *        OOB := 1
   *        if (last_instance == 1) {
   *           dma_parameters[BURST_SIZE] := 0
   *           DOUTD calculated_source_address,dma_paramters
   *	      }
   *	   }
   *	} else {
   *	   // DDMAD
   *	   DOUTD calculated_source_address,dma_paramters
   *	}
            if (last_dma && (!PVR_HAS_FEATURE(dev_info, pds_ddmadt) ||
            pvr_debug_pds_note("LAST DDMAD");
               /* DDMAD Const Usage [_XXXX---] */
   literal_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      literal_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32;
   literal_entry->const_offset = (const_base + 6);
            if (PVR_HAS_FEATURE(dev_info, pds_ddmadt)) {
      /* DDMAD Const Usage [XXXXX---]
   * With DDMADT an extra 32bits of SRC3 contains the information for
                  if (use_robust_vertex_fetch) {
      struct pvr_pds_const_map_entry_vertex_attr_ddmadt_oob_buffer_size
         obb_buffer_size =
                  obb_buffer_size->type =
         obb_buffer_size->const_offset = const_base + 7;
      } else {
      literal_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      literal_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32;
   literal_entry->const_offset = const_base + 7;
               PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_ddmad(0, /* cc */
                           0, /* END */
   R32_C(const_base + 3), /* SRC0 (REGS32) */
   index, /* SRC1 (REGS32T) */
   R64_C((const_base + 4) >> 1), /* SRC2
               if (use_robust_vertex_fetch) {
      /* If not out of bounds, skip next DDMAD instructions. */
   PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_ddmad(
      1, /* cc */
   0, /* END */
   R32_C(const_base + 3), /* SRC0 (REGS32) */
   R32_T(zero_temp), /* SRC1 (REGS32T) */
   R64_C((const_base + 4) >> 1), /* SRC2
                     /* Now the driver must have a dummy DDMAD marked as last. */
   if (last_dma) {
      uint32_t dummy_dma_const = pvr_find_constant(const_usage,
                              literal_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
                           literal_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
                           literal_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
                           literal_entry->literal_value |=
         literal_entry->literal_value |=
      (PVR_ROGUE_PDSINST_DDMAD_FIELDS_SRC3_DEST_UNIFIED_STORE |
                     literal_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
                           PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_ddmad(
      0, /* cc */
   0, /* END */
   R32_C(zero_const), /* SRC0 (REGS32)
         R32_T(zero_temp), /* SRC1 (REGS32T)
         R64_C((dummy_dma_const) >> 1), /* SRC2
                     } else {
      if (use_robust_vertex_fetch) {
                                             max_index_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      max_index_entry->const_offset = max_index_const;
   max_index_entry->type =
         max_index_entry->binding_index = vertex_dma->binding_index;
   max_index_entry->offset = vertex_dma->offset;
   max_index_entry->stride = vertex_dma->stride;
   max_index_entry->size_in_dwords = vertex_dma->size_in_dwords;
                  PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_add32(0, /* cc */
                           0, /* ALUM */
   PVR_ROGUE_PDSINST_LOP_NONE, /* SNA */
   R32_C(max_index_const), /* SRC0
                     PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_sftlp32(
      1, /* IM */
   0, /* cc */
   PVR_ROGUE_PDSINST_LOP_NONE, /* LOP */
   index, /* SRC0 (REGS32T) */
                     PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_cmp(
      0, /* cc enable */
   PVR_ROGUE_PDSINST_COP_GT, /* Operation */
   R64TP_T(current_index_temp >> 1), /* SRC
                        PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_sftlp32(
      1, /* IM */
   1, /* cc */
   PVR_ROGUE_PDSINST_LOP_NONE, /* LOP */
   zero_temp, /* SRC0 (REGS32T) */
                     PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_ddmad(
      0, /* cc  */
   0, /* END */
   R32_C(const_base + 3), /* SRC0 (REGS32) */
   current_index_temp, /* SRC1 (REGS32T) */
   R64_C((const_base + 4) >> 1), /* SRC2
   } else {
      PVR_PDS_MODE_TOGGLE(code,
                     instruction,
   pvr_pds_inst_encode_ddmad(
      /* cc    */ 0,
   /* end   */ 0,
                  if (input_program->flags & PVR_PDS_VERTEX_FLAGS_VERTEX_ID_REQUIRED) {
               PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_encode_direct_write(
      &entry_write_state,
   last_dma,
   false,
   R64_C(write_vertex_control),
   R64_T(0),
   0x1,
   input_program->vertex_id_register,
               if (input_program->flags & PVR_PDS_VERTEX_FLAGS_INSTANCE_ID_REQUIRED) {
               PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_encode_direct_write(
      &entry_write_state,
   last_dma,
   false,
   R64_C(write_instance_control),
   R64_T(0),
   0x2,
   input_program->instance_id_register,
               if (input_program->flags & PVR_PDS_VERTEX_FLAGS_BASE_INSTANCE_REQUIRED) {
               if (input_program->flags & PVR_PDS_VERTEX_FLAGS_DRAW_INDIRECT_VARIANT) {
      /* Base instance comes from ptemp 1. */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_encode_direct_write(
      &entry_write_state,
   last_dma,
   false,
   R64_C(write_base_instance_control),
   R64_P(PVR_INDIRECT_BASE_INSTANCE_PTEMP >> 1),
   0x2,
   input_program->base_instance_register,
      } else {
               /* Base instance comes from driver constant. */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_encode_direct_write(
      &entry_write_state,
   last_dma,
   false,
   R64_C(write_base_instance_control),
   R64_C(base_instance >> 1),
   data_mask,
   input_program->base_instance_register,
                  if (input_program->flags & PVR_PDS_VERTEX_FLAGS_BASE_VERTEX_REQUIRED) {
               if (input_program->flags & PVR_PDS_VERTEX_FLAGS_DRAW_INDIRECT_VARIANT) {
      /* Base vertex comes from ptemp 0 (initialized by PDS hardware). */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_encode_direct_write(
      &entry_write_state,
   last_dma,
   false,
   R64_C(write_base_vertex_control),
   R64_P(0),
   0x1,
   input_program->base_vertex_register,
      } else {
               /* Base vertex comes from driver constant (literal 0). */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_encode_direct_write(
      &entry_write_state,
   last_dma,
   false,
   R64_C(write_base_vertex_control),
   R64_C(base_vertex >> 1),
   data_mask,
   input_program->base_vertex_register,
                  if (input_program->flags & PVR_PDS_VERTEX_FLAGS_DRAW_INDEX_REQUIRED) {
               if (input_program->flags & PVR_PDS_VERTEX_FLAGS_DRAW_INDIRECT_VARIANT) {
      /* Draw index comes from ptemp 3. */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_encode_direct_write(
      &entry_write_state,
   last_dma,
   false,
   R64_C(pvr_write_draw_index_control),
   R64_P(1),
   0x2,
   input_program->draw_index_register,
      } else {
               /* Draw index comes from constant (literal 0). */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_encode_direct_write(
      &entry_write_state,
   last_dma,
   false,
   R64_C(pvr_write_draw_index_control),
   R64_C(draw_index >> 1),
   data_mask,
   input_program->draw_index_register,
                  doutu_address_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      doutu_address_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_DOUTU_ADDRESS;
   doutu_address_entry->const_offset = 0;
            if (use_robust_vertex_fetch) {
      /* Restore IF0 */
   PVR_PDS_MODE_TOGGLE(
      code,
   instruction,
   pvr_pds_inst_encode_bra(PVR_ROGUE_PDSINST_PREDICATE_KEEP, /* SRCCC */
                              PVR_PDS_MODE_TOGGLE(code, instruction, pvr_pds_encode_doutu(1, 1, 0));
                     for (uint32_t i = 0; i < ARRAY_SIZE(const_usage); i++) {
      if (const_usage[i] == 0)
            info->data_size_in_dwords =
               info->temps_required = temp_usage.temps_needed;
   info->entry_count = entry_write_state.entry_count;
   info->entries_written_size_in_bytes =
                     }
      void pvr_pds_generate_descriptor_upload_program(
      struct pvr_pds_descriptor_program_input *input_program,
   uint32_t *code_section,
      {
      unsigned int num_consts64;
   unsigned int num_consts32;
   unsigned int next_const64;
   unsigned int next_const32;
   unsigned int instruction = 0;
            unsigned int total_dma_count = 0;
                     /* Calculate the total register usage so we can stick 32-bit consts
   * after 64. Each DOUTD/DDMAD requires 1 32-bit constant and 1 64-bit
   * constant.
   */
   num_consts32 = input_program->descriptor_set_count;
   num_consts64 = input_program->descriptor_set_count;
            /* 1 DOUTD for buffer containing address literals. */
   if (input_program->addr_literal_count > 0) {
      num_consts32++;
   num_consts64++;
                        for (unsigned int index = 0; index < input_program->buffer_count; index++) {
               /* This switch statement looks pointless but we want to optimize DMAs
   * that can be done as a DOUTW.
   */
   switch (buffer->type) {
   default: {
      /* 1 DOUTD per compile time buffer: */
   num_consts32++;
   num_consts64++;
   total_dma_count++;
      }
               /* DOUTU for the secondary update program requires a 64-bit constant. */
   if (input_program->secondary_program_present)
                     /* Start counting constants. */
   next_const64 = 0;
            if (input_program->addr_literal_count > 0) {
      bool last_dma = (++running_dma_count == total_dma_count);
            unsigned int size_in_dwords = input_program->addr_literal_count *
                  struct pvr_pds_const_map_entry_addr_literal_buffer
            addr_literal_buffer_entry = pvr_prepare_next_pds_const_map_entry(
                  addr_literal_buffer_entry->type =
         addr_literal_buffer_entry->size = PVR_DW_TO_BYTES(size_in_dwords);
            for (unsigned int i = 0; i < input_program->addr_literal_count; i++) {
               /* Check that the destinations for the addr literals are contiguous.
   * Not supporting non contiguous ranges as that would either require a
   * single large buffer with wasted memory for DMA, or multiple buffers
   * to DMA.
   */
   if (i > 0) {
      const uint32_t current_addr_literal_destination =
                        /* 2 regs to store 64 bits address. */
   assert(current_addr_literal_destination ==
               addr_literal_entry =
                  addr_literal_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_ADDR_LITERAL;
               PVR_PDS_MODE_TOGGLE(code_section,
                     instruction,
   pvr_encode_burst_cs(&entry_write_state,
                        next_const64++;
               /* For each descriptor set perform a DOUTD. */
   for (unsigned int descriptor_index = 0;
      descriptor_index < input_program->descriptor_set_count;
   descriptor_index++) {
   struct pvr_const_map_entry_descriptor_set *descriptor_set_entry;
   struct pvr_pds_descriptor_set *descriptor_set =
            bool last_dma = (++running_dma_count == total_dma_count);
            descriptor_set_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      descriptor_set_entry->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_DESCRIPTOR_SET;
   descriptor_set_entry->const_offset = next_const64 * 2;
   descriptor_set_entry->descriptor_set = descriptor_set->descriptor_set;
   descriptor_set_entry->primary = descriptor_set->primary;
            PVR_PDS_MODE_TOGGLE(code_section,
                     instruction,
   pvr_encode_burst_cs(&entry_write_state,
                        next_const64++;
               for (unsigned int index = 0; index < input_program->buffer_count; index++) {
               bool last_dma = (++running_dma_count == total_dma_count);
            switch (buffer->type) {
   case PVR_BUFFER_TYPE_PUSH_CONSTS: {
               special_buffer_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      special_buffer_entry->type =
         special_buffer_entry->buffer_type = PVR_BUFFER_TYPE_PUSH_CONSTS;
   special_buffer_entry->buffer_index = buffer->source_offset;
      }
   case PVR_BUFFER_TYPE_DYNAMIC: {
               special_buffer_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      special_buffer_entry->type =
         special_buffer_entry->buffer_type = PVR_BUFFER_TYPE_DYNAMIC;
   special_buffer_entry->buffer_index = buffer->source_offset;
      }
   case PVR_BUFFER_TYPE_COMPILE_TIME: {
               special_buffer_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      special_buffer_entry->type =
         special_buffer_entry->buffer_type = PVR_BUFFER_TYPE_COMPILE_TIME;
   special_buffer_entry->buffer_index = compile_time_buffer_index++;
      }
   case PVR_BUFFER_TYPE_BUFFER_LENGTHS: {
               special_buffer_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      special_buffer_entry->type =
         special_buffer_entry->buffer_type = PVR_BUFFER_TYPE_BUFFER_LENGTHS;
      }
   case PVR_BUFFER_TYPE_BLEND_CONSTS: {
               special_buffer_entry =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      special_buffer_entry->type =
         special_buffer_entry->buffer_type = PVR_BUFFER_TYPE_BLEND_CONSTS;
   special_buffer_entry->buffer_index =
            }
   case PVR_BUFFER_TYPE_UBO: {
               constant_buffer_entry = pvr_prepare_next_pds_const_map_entry(
      &entry_write_state,
      constant_buffer_entry->type =
         constant_buffer_entry->buffer_id = buffer->buffer_id;
   constant_buffer_entry->desc_set = buffer->desc_set;
   constant_buffer_entry->binding = buffer->binding;
   constant_buffer_entry->offset = buffer->source_offset;
   constant_buffer_entry->size_in_dwords = buffer->size_in_dwords;
      }
   case PVR_BUFFER_TYPE_UBO_ZEROING: {
                     constant_buffer_entry = pvr_prepare_next_pds_const_map_entry(
      &entry_write_state,
      constant_buffer_entry->type =
         constant_buffer_entry->buffer_id = buffer->buffer_id;
   constant_buffer_entry->offset = buffer->source_offset;
   constant_buffer_entry->size_in_dwords = buffer->size_in_dwords;
      }
                     PVR_PDS_MODE_TOGGLE(code_section,
                     instruction,
   pvr_encode_burst_cs(&entry_write_state,
                        next_const64++;
               if (total_dma_count != running_dma_count)
            if (input_program->secondary_program_present) {
               PVR_PDS_MODE_TOGGLE(code_section,
                  doutu_address =
      pvr_prepare_next_pds_const_map_entry(&entry_write_state,
      doutu_address->type = PVR_PDS_CONST_MAP_ENTRY_TYPE_DOUTU_ADDRESS;
   doutu_address->const_offset = next_const64 * 2;
                        if (instruction == 0 && input_program->must_not_be_empty) {
      PVR_PDS_MODE_TOGGLE(code_section,
                           info->entry_count = entry_write_state.entry_count;
   info->entries_written_size_in_bytes =
            }
