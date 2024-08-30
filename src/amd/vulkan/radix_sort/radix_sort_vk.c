   // Copyright 2021 The Fuchsia Authors. All rights reserved.
   // Use of this source code is governed by a BSD-style license that can be
   // found in the LICENSE file.
      #include <inttypes.h>
   #include <stdio.h>
   #include <stdlib.h>
      #include "common/macros.h"
   #include "common/util.h"
   #include "common/vk/barrier.h"
   #include "radix_sort_vk_devaddr.h"
   #include "shaders/push.h"
      //
   //
   //
      #ifdef RS_VK_ENABLE_DEBUG_UTILS
   #include "common/vk/debug_utils.h"
   #endif
      //
   //
   //
      #ifdef RS_VK_ENABLE_EXTENSIONS
   #include "radix_sort_vk_ext.h"
   #endif
      //
   // FIXME(allanmac): memoize some of these calculations
   //
   void
   radix_sort_vk_get_memory_requirements(radix_sort_vk_t const *               rs,
               {
   //
   // Keyval size
   //
   mr->keyval_size = rs->config.keyval_dwords * sizeof(uint32_t);
      //
   // Subgroup and workgroup sizes
   //
   uint32_t const histo_sg_size    = 1 << rs->config.histogram.subgroup_size_log2;
   uint32_t const histo_wg_size    = 1 << rs->config.histogram.workgroup_size_log2;
   uint32_t const prefix_sg_size   = 1 << rs->config.prefix.subgroup_size_log2;
   uint32_t const scatter_wg_size  = 1 << rs->config.scatter.workgroup_size_log2;
   uint32_t const internal_sg_size = MAX_MACRO(uint32_t, histo_sg_size, prefix_sg_size);
      //
   // If for some reason count is zero then initialize appropriately.
   //
   if (count == 0)
      {
      mr->keyvals_size       = 0;
   mr->keyvals_alignment  = mr->keyval_size * histo_sg_size;
   mr->internal_size      = 0;
   mr->internal_alignment = internal_sg_size * sizeof(uint32_t);
   mr->indirect_size      = 0;
         else
      {
      //
   // Keyvals
            // Round up to the scatter block size.
   //
   // Then round up to the histogram block size.
   //
   // Fill the difference between this new count and the original keyval
   // count.
   //
   // How many scatter blocks?
   //
   uint32_t const scatter_block_kvs = scatter_wg_size * rs->config.scatter.block_rows;
   uint32_t const scatter_blocks    = (count + scatter_block_kvs - 1) / scatter_block_kvs;
            //
   // How many histogram blocks?
   //
   // Note that it's OK to have more max-valued digits counted by the histogram
   // than sorted by the scatters because the sort is stable.
   //
   uint32_t const histo_block_kvs = histo_wg_size * rs->config.histogram.block_rows;
   uint32_t const histo_blocks    = (count_ru_scatter + histo_block_kvs - 1) / histo_block_kvs;
            mr->keyvals_size      = mr->keyval_size * count_ru_histo;
            //
   // Internal
   //
   // NOTE: Assumes .histograms are before .partitions.
   //
   // Last scatter workgroup skips writing to a partition.
   //
   // One histogram per (keyval byte + partitions)
   //
            mr->internal_size      = (mr->keyval_size + partitions) * (RS_RADIX_SIZE * sizeof(uint32_t));
            //
   // Indirect
   //
   mr->indirect_size      = sizeof(struct rs_indirect_info);
         }
      //
   //
   //
   #ifdef RS_VK_ENABLE_DEBUG_UTILS
      static void
   rs_debug_utils_set(VkDevice device, struct radix_sort_vk * rs)
   {
   if (pfn_vkSetDebugUtilsObjectNameEXT != NULL)
      {
      VkDebugUtilsObjectNameInfoEXT duoni = {
   .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
   .pNext      = NULL,
   .objectType = VK_OBJECT_TYPE_PIPELINE,
            duoni.objectHandle = (uint64_t)rs->pipelines.named.init;
   duoni.pObjectName  = "radix_sort_init";
            duoni.objectHandle = (uint64_t)rs->pipelines.named.fill;
   duoni.pObjectName  = "radix_sort_fill";
            duoni.objectHandle = (uint64_t)rs->pipelines.named.histogram;
   duoni.pObjectName  = "radix_sort_histogram";
            duoni.objectHandle = (uint64_t)rs->pipelines.named.prefix;
   duoni.pObjectName  = "radix_sort_prefix";
            duoni.objectHandle = (uint64_t)rs->pipelines.named.scatter[0].even;
   duoni.pObjectName  = "radix_sort_scatter_0_even";
            duoni.objectHandle = (uint64_t)rs->pipelines.named.scatter[0].odd;
   duoni.pObjectName  = "radix_sort_scatter_0_odd";
            if (rs->config.keyval_dwords >= 2)
   {
      duoni.objectHandle = (uint64_t)rs->pipelines.named.scatter[1].even;
                  duoni.objectHandle = (uint64_t)rs->pipelines.named.scatter[1].odd;
   duoni.pObjectName  = "radix_sort_scatter_1_odd";
            }
      #endif
      //
   // How many pipelines are there?
   //
   static uint32_t
   rs_pipeline_count(struct radix_sort_vk const * rs)
   {
   return 1 +                            // init
            1 +                            // fill
   1 +                            // histogram
      }
      radix_sort_vk_t *
   radix_sort_vk_create(VkDevice                           device,
                     VkAllocationCallbacks const *      ac,
   VkPipelineCache                    pc,
   {
   //
   // Allocate radix_sort_vk
   //
   struct radix_sort_vk * const rs = calloc(1, sizeof(*rs));
      //
   // Save the config for layer
   //
   rs->config = config;
      //
   // How many pipelines?
   //
   uint32_t const pipeline_count = rs_pipeline_count(rs);
      //
   // Prepare to create pipelines
   //
   VkPushConstantRange const pcr[] = {
      { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,  //
      .offset     = 0,
         { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,  //
      .offset     = 0,
         { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,  //
      .offset     = 0,
         { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,  //
      .offset     = 0,
         { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,  //
      .offset     = 0,
         { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,  //
      .offset     = 0,
         { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,  //
      .offset     = 0,
         { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,  //
      .offset     = 0,
   };
      VkPipelineLayoutCreateInfo plci = {
         .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .pNext                  = NULL,
   .flags                  = 0,
   .setLayoutCount         = 0,
   .pSetLayouts            = NULL,
   .pushConstantRangeCount = 1,
      };
      for (uint32_t ii = 0; ii < pipeline_count; ii++)
      {
               if (vkCreatePipelineLayout(device, &plci, NULL, rs->pipeline_layouts.handles + ii) != VK_SUCCESS)
            //
   // Create compute pipelines
   //
   VkShaderModuleCreateInfo smci = {
         .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
   .pNext = NULL,
   .flags = 0,
   // .codeSize = ar_entries[...].size;
      };
      VkShaderModule sms[ARRAY_LENGTH_MACRO(rs->pipelines.handles)] = {0};
      for (uint32_t ii = 0; ii < pipeline_count; ii++)
      {
      smci.codeSize = spv_sizes[ii];
            if (vkCreateShaderModule(device, &smci, ac, sms + ii) != VK_SUCCESS)
               //
   // If necessary, set the expected subgroup size
      #define RS_SUBGROUP_SIZE_CREATE_INFO_SET(size_)                                                    \
   {                                                                                                \
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT,       \
   .pNext = NULL,                                                                                 \
      }
      #undef RS_SUBGROUP_SIZE_CREATE_INFO_NAME
   #define RS_SUBGROUP_SIZE_CREATE_INFO_NAME(name_)                                                   \
   RS_SUBGROUP_SIZE_CREATE_INFO_SET(1 << config.name_.subgroup_size_log2)
      #define RS_SUBGROUP_SIZE_CREATE_INFO_ZERO(name_) RS_SUBGROUP_SIZE_CREATE_INFO_SET(0)
      VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT const rsscis[] = {
      RS_SUBGROUP_SIZE_CREATE_INFO_ZERO(init),       // init
   RS_SUBGROUP_SIZE_CREATE_INFO_ZERO(fill),       // fill
   RS_SUBGROUP_SIZE_CREATE_INFO_NAME(histogram),  // histogram
   RS_SUBGROUP_SIZE_CREATE_INFO_NAME(prefix),     // prefix
   RS_SUBGROUP_SIZE_CREATE_INFO_NAME(scatter),    // scatter[0].even
   RS_SUBGROUP_SIZE_CREATE_INFO_NAME(scatter),    // scatter[0].odd
   RS_SUBGROUP_SIZE_CREATE_INFO_NAME(scatter),    // scatter[1].even
      };
      //
   // Define compute pipeline create infos
   //
   #undef RS_COMPUTE_PIPELINE_CREATE_INFO_DECL
   #define RS_COMPUTE_PIPELINE_CREATE_INFO_DECL(idx_)                                                 \
   { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,                                       \
      .pNext = NULL,                                                                                 \
   .flags = 0,                                                                                    \
   .stage = { .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,         \
               .pNext               = NULL,                                                        \
   .flags               = 0,                                                           \
   .stage               = VK_SHADER_STAGE_COMPUTE_BIT,                                 \
   .module              = sms[idx_],                                                   \
   .pName               = "main",                                                      \
   .layout             = rs->pipeline_layouts.handles[idx_],                                      \
   .basePipelineHandle = VK_NULL_HANDLE,                                                          \
         VkComputePipelineCreateInfo cpcis[] = {
      RS_COMPUTE_PIPELINE_CREATE_INFO_DECL(0),  // init
   RS_COMPUTE_PIPELINE_CREATE_INFO_DECL(1),  // fill
   RS_COMPUTE_PIPELINE_CREATE_INFO_DECL(2),  // histogram
   RS_COMPUTE_PIPELINE_CREATE_INFO_DECL(3),  // prefix
   RS_COMPUTE_PIPELINE_CREATE_INFO_DECL(4),  // scatter[0].even
   RS_COMPUTE_PIPELINE_CREATE_INFO_DECL(5),  // scatter[0].odd
   RS_COMPUTE_PIPELINE_CREATE_INFO_DECL(6),  // scatter[1].even
      };
      //
   // Which of these compute pipelines require subgroup size control?
   //
   for (uint32_t ii = 0; ii < pipeline_count; ii++)
      {
      if (rsscis[ii].requiredSubgroupSize > 1)
   {
                  //
   // Create the compute pipelines
   //
   if (vkCreateComputePipelines(device, pc, pipeline_count, cpcis, ac, rs->pipelines.handles) != VK_SUCCESS)
            //
   // Shader modules can be destroyed now
   //
   for (uint32_t ii = 0; ii < pipeline_count; ii++)
      {
               #ifdef RS_VK_ENABLE_DEBUG_UTILS
   //
   // Tag pipelines with names
   //
   rs_debug_utils_set(device, rs);
   #endif
      //
   // Calculate "internal" buffer offsets
   //
   size_t const keyval_bytes = rs->config.keyval_dwords * sizeof(uint32_t);
      // the .range calculation assumes an 8-bit radix
   rs->internal.histograms.offset = 0;
   rs->internal.histograms.range  = keyval_bytes * (RS_RADIX_SIZE * sizeof(uint32_t));
      //
   // NOTE(allanmac): The partitions.offset must be aligned differently if
   // RS_RADIX_LOG2 is less than the target's subgroup size log2.  At this time,
   // no GPU that meets this criteria.
   //
   rs->internal.partitions.offset = rs->internal.histograms.offset + rs->internal.histograms.range;
      return rs;
      fail_pipeline:
   for (uint32_t ii = 0; ii < pipeline_count; ii++)
      {
            fail_shader:
   for (uint32_t ii = 0; ii < pipeline_count; ii++)
      {
            fail_layout:
      for (uint32_t ii = 0; ii < pipeline_count; ii++)
   {
               free(rs);
   return NULL;
   }
      //
   //
   //
   void
   radix_sort_vk_destroy(struct radix_sort_vk * rs, VkDevice d, VkAllocationCallbacks const * const ac)
   {
   uint32_t const pipeline_count = rs_pipeline_count(rs);
      // destroy pipelines
   for (uint32_t ii = 0; ii < pipeline_count; ii++)
      {
               // destroy pipeline layouts
   for (uint32_t ii = 0; ii < pipeline_count; ii++)
      {
               free(rs);
   }
      //
   //
   //
   static VkDeviceAddress
   rs_get_devaddr(VkDevice device, VkDescriptorBufferInfo const * dbi)
   {
   VkBufferDeviceAddressInfo const bdai = {
         .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
   .pNext  = NULL,
      };
      VkDeviceAddress const devaddr = vkGetBufferDeviceAddress(device, &bdai) + dbi->offset;
      return devaddr;
   }
      //
   //
   //
   #ifdef RS_VK_ENABLE_EXTENSIONS
      void
   rs_ext_cmd_write_timestamp(struct radix_sort_vk_ext_timestamps * ext_timestamps,
               {
   if ((ext_timestamps != NULL) &&
            {
      vkCmdWriteTimestamp(cb,
                     }
      #endif
      //
   //
   //
      #ifdef RS_VK_ENABLE_EXTENSIONS
      struct radix_sort_vk_ext_base
   {
   void *                      ext;
   enum radix_sort_vk_ext_type type;
   };
      #endif
      //
   //
   //
   void
   radix_sort_vk_sort_devaddr(radix_sort_vk_t const *                   rs,
                           {
   //
   // Anything to do?
   //
   if ((info->count <= 1) || (info->key_bits == 0))
      {
                        #ifdef RS_VK_ENABLE_EXTENSIONS
   //
   // Any extensions?
   //
   struct radix_sort_vk_ext_timestamps * ext_timestamps = NULL;
      void * ext_next = info->ext;
      while (ext_next != NULL)
      {
               switch (base->type)
   {
      case RS_VK_EXT_TIMESTAMPS:
      ext_timestamps                 = ext_next;
   ext_timestamps->timestamps_set = 0;
                  #endif
         ////////////////////////////////////////////////////////////////////////
   //
   // OVERVIEW
   //
   //   1. Pad the keyvals in `scatter_even`.
   //   2. Zero the `histograms` and `partitions`.
   //      --- BARRIER ---
   //   3. HISTOGRAM is dispatched before PREFIX.
   //      --- BARRIER ---
   //   4. PREFIX is dispatched before the first SCATTER.
   //      --- BARRIER ---
   //   5. One or more SCATTER dispatches.
   //
   // Note that the `partitions` buffer can be zeroed anytime before the first
   // scatter.
   //
            //
   // Label the command buffer
      #ifdef RS_VK_ENABLE_DEBUG_UTILS
   if (pfn_vkCmdBeginDebugUtilsLabelEXT != NULL)
      {
      VkDebugUtilsLabelEXT const label = {
   .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
   .pNext      = NULL,
   .pLabelName = "radix_sort_vk_sort",
                  #endif
      //
   // How many passes?
   //
   uint32_t const keyval_bytes = rs->config.keyval_dwords * (uint32_t)sizeof(uint32_t);
   uint32_t const keyval_bits  = keyval_bytes * 8;
   uint32_t const key_bits     = MIN_MACRO(uint32_t, info->key_bits, keyval_bits);
   uint32_t const passes       = (key_bits + RS_RADIX_LOG2 - 1) / RS_RADIX_LOG2;
      *keyvals_sorted = ((passes & 1) != 0) ? info->keyvals_odd : info->keyvals_even.devaddr;
      ////////////////////////////////////////////////////////////////////////
   //
   // PAD KEYVALS AND ZERO HISTOGRAM/PARTITIONS
   //
   // Pad fractional blocks with max-valued keyvals.
   //
   // Zero the histograms and partitions buffer.
   //
   // This assumes the partitions follow the histograms.
   //
   #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
   #endif
      //
   // FIXME(allanmac): Consider precomputing some of these values and hang them
   // off `rs`.
   //
      //
   // How many scatter blocks?
   //
   uint32_t const scatter_wg_size   = 1 << rs->config.scatter.workgroup_size_log2;
   uint32_t const scatter_block_kvs = scatter_wg_size * rs->config.scatter.block_rows;
   uint32_t const scatter_blocks    = (info->count + scatter_block_kvs - 1) / scatter_block_kvs;
   uint32_t const count_ru_scatter  = scatter_blocks * scatter_block_kvs;
      //
   // How many histogram blocks?
   //
   // Note that it's OK to have more max-valued digits counted by the histogram
   // than sorted by the scatters because the sort is stable.
   //
   uint32_t const histo_wg_size   = 1 << rs->config.histogram.workgroup_size_log2;
   uint32_t const histo_block_kvs = histo_wg_size * rs->config.histogram.block_rows;
   uint32_t const histo_blocks    = (count_ru_scatter + histo_block_kvs - 1) / histo_block_kvs;
   uint32_t const count_ru_histo  = histo_blocks * histo_block_kvs;
      //
   // Fill with max values
   //
   if (count_ru_histo > info->count)
      {
      info->fill_buffer(cb,
                              //
   // Zero histograms and invalidate partitions.
   //
   // Note that the partition invalidation only needs to be performed once
   // because the even/odd scatter dispatches rely on the the previous pass to
   // leave the partitions in an invalid state.
   //
   // Note that the last workgroup doesn't read/write a partition so it doesn't
   // need to be initialized.
   //
   uint32_t const histo_partition_count = passes + scatter_blocks - 1;
   uint32_t       pass_idx              = (keyval_bytes - passes);
      VkDeviceSize const fill_base = pass_idx * (RS_RADIX_SIZE * sizeof(uint32_t));
      info->fill_buffer(cb,
                     &info->internal,
      ////////////////////////////////////////////////////////////////////////
   //
   // Pipeline: HISTOGRAM
   //
   // TODO(allanmac): All subgroups should try to process approximately the same
   // number of blocks in order to minimize tail effects.  This was implemented
   // and reverted but should be reimplemented and benchmarked later.
   //
   #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_TRANSFER_BIT);
   #endif
      vk_barrier_transfer_w_to_compute_r(cb);
      // clang-format off
   VkDeviceAddress const devaddr_histograms   = info->internal.devaddr + rs->internal.histograms.offset;
   VkDeviceAddress const devaddr_keyvals_even = info->keyvals_even.devaddr;
   // clang-format on
      //
   // Dispatch histogram
   //
   struct rs_push_histogram const push_histogram = {
         .devaddr_histograms = devaddr_histograms,
   .devaddr_keyvals    = devaddr_keyvals_even,
      };
      vkCmdPushConstants(cb,
                        rs->pipeline_layouts.named.histogram,
         vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, rs->pipelines.named.histogram);
      vkCmdDispatch(cb, histo_blocks, 1, 1);
      ////////////////////////////////////////////////////////////////////////
   //
   // Pipeline: PREFIX
   //
   // Launch one workgroup per pass.
   //
   #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
   #endif
      vk_barrier_compute_w_to_compute_r(cb);
      struct rs_push_prefix const push_prefix = {
            };
      vkCmdPushConstants(cb,
                        rs->pipeline_layouts.named.prefix,
         vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, rs->pipelines.named.prefix);
      vkCmdDispatch(cb, passes, 1, 1);
      ////////////////////////////////////////////////////////////////////////
   //
   // Pipeline: SCATTER
   //
   #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
   #endif
      vk_barrier_compute_w_to_compute_r(cb);
      // clang-format off
   uint32_t        const histogram_offset    = pass_idx * (RS_RADIX_SIZE * sizeof(uint32_t));
   VkDeviceAddress const devaddr_keyvals_odd = info->keyvals_odd;
   VkDeviceAddress const devaddr_partitions  = info->internal.devaddr + rs->internal.partitions.offset;
   // clang-format on
      struct rs_push_scatter push_scatter = {
         .devaddr_keyvals_even = devaddr_keyvals_even,
   .devaddr_keyvals_odd  = devaddr_keyvals_odd,
   .devaddr_partitions   = devaddr_partitions,
   .devaddr_histograms   = devaddr_histograms + histogram_offset,
      };
      {
               vkCmdPushConstants(cb,
                     rs->pipeline_layouts.named.scatter[pass_dword].even,
            vkCmdBindPipeline(cb,
            }
      bool is_even = true;
      while (true)
      {
               //
   // Continue?
   //
   if (++pass_idx >= keyval_bytes)
      #ifdef RS_VK_ENABLE_EXTENSIONS
         #endif
                  // clang-format off
   is_even                         ^= true;
   push_scatter.devaddr_histograms += (RS_RADIX_SIZE * sizeof(uint32_t));
   push_scatter.pass_offset         = (pass_idx & 3) * RS_RADIX_LOG2;
                     //
   // Update push constants that changed
   //
   VkPipelineLayout const pl = is_even ? rs->pipeline_layouts.named.scatter[pass_dword].even  //
         vkCmdPushConstants(cb,
                     pl,
            //
   // Bind new pipeline
   //
   VkPipeline const p = is_even ? rs->pipelines.named.scatter[pass_dword].even  //
                     #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
   #endif
      //
   // End the label
   //
   #ifdef RS_VK_ENABLE_DEBUG_UTILS
   if (pfn_vkCmdEndDebugUtilsLabelEXT != NULL)
      {
            #endif
   }
      //
   //
   //
   void
   radix_sort_vk_sort_indirect_devaddr(radix_sort_vk_t const *                            rs,
                           {
   //
   // Anything to do?
   //
   if (info->key_bits == 0)
      {
      *keyvals_sorted = info->keyvals_even;
            #ifdef RS_VK_ENABLE_EXTENSIONS
   //
   // Any extensions?
   //
   struct radix_sort_vk_ext_timestamps * ext_timestamps = NULL;
      void * ext_next = info->ext;
      while (ext_next != NULL)
      {
               switch (base->type)
   {
      case RS_VK_EXT_TIMESTAMPS:
      ext_timestamps                 = ext_next;
   ext_timestamps->timestamps_set = 0;
                  #endif
         ////////////////////////////////////////////////////////////////////////
   //
   // OVERVIEW
   //
   //   1. Init
   //      --- BARRIER ---
   //   2. Pad the keyvals in `scatter_even`.
   //   3. Zero the `histograms` and `partitions`.
   //      --- BARRIER ---
   //   4. HISTOGRAM is dispatched before PREFIX.
   //      --- BARRIER ---
   //   5. PREFIX is dispatched before the first SCATTER.
   //      --- BARRIER ---
   //   6. One or more SCATTER dispatches.
   //
   // Note that the `partitions` buffer can be zeroed anytime before the first
   // scatter.
   //
            //
   // Label the command buffer
      #ifdef RS_VK_ENABLE_DEBUG_UTILS
   if (pfn_vkCmdBeginDebugUtilsLabelEXT != NULL)
      {
      VkDebugUtilsLabelEXT const label = {
   .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
   .pNext      = NULL,
   .pLabelName = "radix_sort_vk_sort_indirect",
                  #endif
      //
   // How many passes?
   //
   uint32_t const keyval_bytes = rs->config.keyval_dwords * (uint32_t)sizeof(uint32_t);
   uint32_t const keyval_bits  = keyval_bytes * 8;
   uint32_t const key_bits     = MIN_MACRO(uint32_t, info->key_bits, keyval_bits);
   uint32_t const passes       = (key_bits + RS_RADIX_LOG2 - 1) / RS_RADIX_LOG2;
   uint32_t       pass_idx     = (keyval_bytes - passes);
      *keyvals_sorted = ((passes & 1) != 0) ? info->keyvals_odd : info->keyvals_even;
      //
   // NOTE(allanmac): Some of these initializations appear redundant but for now
   // we're going to assume the compiler will elide them.
   //
   // clang-format off
   VkDeviceAddress const devaddr_info         = info->indirect.devaddr;
   VkDeviceAddress const devaddr_count        = info->count;
   VkDeviceAddress const devaddr_histograms   = info->internal + rs->internal.histograms.offset;
   VkDeviceAddress const devaddr_keyvals_even = info->keyvals_even;
   // clang-format on
      //
   // START
   //
   #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
   #endif
      //
   // INIT
   //
   {
                  .devaddr_info  = devaddr_info,
   .devaddr_count = devaddr_count,
               vkCmdPushConstants(cb,
                     rs->pipeline_layouts.named.init,
                        }
      #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
   #endif
      vk_barrier_compute_w_to_indirect_compute_r(cb);
      {
      //
   // PAD
   //
               .devaddr_info   = devaddr_info + offsetof(struct rs_indirect_info, pad),
   .devaddr_dwords = devaddr_keyvals_even,
               vkCmdPushConstants(cb,
                     rs->pipeline_layouts.named.fill,
                        }
      //
   // ZERO
   //
   {
                           .devaddr_info   = devaddr_info + offsetof(struct rs_indirect_info, zero),
   .devaddr_dwords = devaddr_histograms + histo_offset,
               vkCmdPushConstants(cb,
                     rs->pipeline_layouts.named.fill,
                        }
      #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
   #endif
      vk_barrier_compute_w_to_compute_r(cb);
      //
   // HISTOGRAM
   //
   {
                  .devaddr_histograms = devaddr_histograms,
   .devaddr_keyvals    = devaddr_keyvals_even,
               vkCmdPushConstants(cb,
                     rs->pipeline_layouts.named.histogram,
                     info->dispatch_indirect(cb,
            }
      #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
   #endif
      vk_barrier_compute_w_to_compute_r(cb);
      //
   // PREFIX
   //
   {
      struct rs_push_prefix const push_prefix = {
                  vkCmdPushConstants(cb,
                     rs->pipeline_layouts.named.prefix,
                        }
      #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
   #endif
      vk_barrier_compute_w_to_compute_r(cb);
      //
   // SCATTER
   //
   {
      // clang-format off
   uint32_t        const histogram_offset    = pass_idx * (RS_RADIX_SIZE * sizeof(uint32_t));
   VkDeviceAddress const devaddr_keyvals_odd = info->keyvals_odd;
   VkDeviceAddress const devaddr_partitions  = info->internal + rs->internal.partitions.offset;
            struct rs_push_scatter push_scatter = {
      .devaddr_keyvals_even = devaddr_keyvals_even,
   .devaddr_keyvals_odd  = devaddr_keyvals_odd,
   .devaddr_partitions   = devaddr_partitions,
   .devaddr_histograms   = devaddr_histograms + histogram_offset,
               {
               vkCmdPushConstants(cb,
                     rs->pipeline_layouts.named.scatter[pass_dword].even,
            vkCmdBindPipeline(cb,
                              while (true)
      {
   info->dispatch_indirect(cb,
                  //
   // Continue?
   //
   if (++pass_idx >= keyval_bytes)
      #ifdef RS_VK_ENABLE_EXTENSIONS
         #endif
                     // clang-format off
   is_even                         ^= true;
   push_scatter.devaddr_histograms += (RS_RADIX_SIZE * sizeof(uint32_t));
   push_scatter.pass_offset         = (pass_idx & 3) * RS_RADIX_LOG2;
                     //
   // Update push constants that changed
   //
   VkPipelineLayout const pl = is_even
               vkCmdPushConstants(
      cb,
   pl,
   VK_SHADER_STAGE_COMPUTE_BIT,
   OFFSETOF_MACRO(struct rs_push_scatter, devaddr_histograms),
               //
   // Bind new pipeline
   //
   VkPipeline const p = is_even ? rs->pipelines.named.scatter[pass_dword].even  //
            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, p);
   }
      #ifdef RS_VK_ENABLE_EXTENSIONS
   rs_ext_cmd_write_timestamp(ext_timestamps, cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
   #endif
      //
   // End the label
   //
   #ifdef RS_VK_ENABLE_DEBUG_UTILS
   if (pfn_vkCmdEndDebugUtilsLabelEXT != NULL)
      {
            #endif
   }
      //
   //
   //
   static void
   radix_sort_vk_fill_buffer(VkCommandBuffer                     cb,
                           {
   vkCmdFillBuffer(cb, buffer_info->buffer, buffer_info->offset + offset, size, data);
   }
      //
   //
   //
   void
   radix_sort_vk_sort(radix_sort_vk_t const *           rs,
                     radix_sort_vk_sort_info_t const * info,
   {
   struct radix_sort_vk_sort_devaddr_info const di = {
      .ext          = info->ext,
   .key_bits     = info->key_bits,
   .count        = info->count,
   .keyvals_even = { .buffer  = info->keyvals_even.buffer,
               .keyvals_odd  = rs_get_devaddr(device, &info->keyvals_odd),
   .internal     = { .buffer  = info->internal.buffer,
                  };
      VkDeviceAddress di_keyvals_sorted;
      radix_sort_vk_sort_devaddr(rs, &di, device, cb, &di_keyvals_sorted);
      *keyvals_sorted = (di_keyvals_sorted == di.keyvals_even.devaddr)  //
               }
      //
   //
   //
   static void
   radix_sort_vk_dispatch_indirect(VkCommandBuffer                     cb,
               {
   vkCmdDispatchIndirect(cb, buffer_info->buffer, buffer_info->offset + offset);
   }
      //
   //
   //
   void
   radix_sort_vk_sort_indirect(radix_sort_vk_t const *                    rs,
                           {
   struct radix_sort_vk_sort_indirect_devaddr_info const idi = {
      .ext               = info->ext,
   .key_bits          = info->key_bits,
   .count             = rs_get_devaddr(device, &info->count),
   .keyvals_even      = rs_get_devaddr(device, &info->keyvals_even),
   .keyvals_odd       = rs_get_devaddr(device, &info->keyvals_odd),
   .internal          = rs_get_devaddr(device, &info->internal),
   .indirect          = { .buffer  = info->indirect.buffer,
                  };
      VkDeviceAddress idi_keyvals_sorted;
      radix_sort_vk_sort_indirect_devaddr(rs, &idi, device, cb, &idi_keyvals_sorted);
      *keyvals_sorted = (idi_keyvals_sorted == idi.keyvals_even)  //
               }
      //
   //
   //
