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
   #include <stddef.h>
   #include <stdint.h>
   #include <string.h>
   #include <vulkan/vulkan.h>
      #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_formats.h"
   #include "pvr_pds.h"
   #include "pvr_private.h"
   #include "pvr_shader_factory.h"
   #include "pvr_static_shaders.h"
   #include "pvr_tex_state.h"
   #include "pvr_types.h"
   #include "vk_alloc.h"
   #include "vk_command_pool.h"
   #include "vk_util.h"
      static inline void pvr_init_primary_compute_pds_program(
         {
      pvr_pds_compute_shader_program_init(program);
   program->local_input_regs[0] = 0;
   /* Workgroup id is in reg0. */
   program->work_group_input_regs[0] = 0;
   program->flattened_work_groups = true;
      }
      static VkResult pvr_create_compute_secondary_prog(
      struct pvr_device *device,
   const struct pvr_shader_factory_info *shader_factory_info,
      {
      const size_t size =
         struct pvr_pds_descriptor_program_input sec_pds_program;
   struct pvr_pds_info *info = &query_prog->info;
   uint32_t staging_buffer_size;
   uint32_t *staging_buffer;
            info->entries =
         if (!info->entries)
                     sec_pds_program = (struct pvr_pds_descriptor_program_input){
      .buffer_count = 1,
   .buffers = {
      [0] = {
      .buffer_id = 0,
   .source_offset = 0,
   .type = PVR_BUFFER_TYPE_COMPILE_TIME,
   .size_in_dwords = shader_factory_info->const_shared_regs,
                                       staging_buffer = vk_alloc(&device->vk.alloc,
                     if (!staging_buffer) {
      vk_free(&device->vk.alloc, info->entries);
               pvr_pds_generate_descriptor_upload_program(&sec_pds_program,
                           /* FIXME: Figure out the define for alignment of 16. */
   result = pvr_gpu_upload_pds(device,
                              NULL,
   0,
   0,
      if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, staging_buffer);
   vk_free(&device->vk.alloc, info->entries);
                           }
      static void
   pvr_destroy_compute_secondary_prog(struct pvr_device *device,
         {
      pvr_bo_suballoc_free(program->pds_sec_code.pvr_bo);
      }
      static VkResult pvr_create_compute_query_program(
      struct pvr_device *device,
   const struct pvr_shader_factory_info *shader_factory_info,
      {
      const uint32_t cache_line_size =
         struct pvr_pds_compute_shader_program pds_primary_prog;
            /* No support for query constant calc program. */
   assert(shader_factory_info->const_calc_prog_inst_bytes == 0);
   /* No support for query coefficient update program. */
            result = pvr_gpu_upload_usc(device,
                           if (result != VK_SUCCESS)
                     pvr_pds_setup_doutu(&pds_primary_prog.usc_task_control,
                              result =
      pvr_pds_compute_shader_create_and_upload(device,
            if (result != VK_SUCCESS)
            query_prog->primary_data_size_dw = pds_primary_prog.data_size;
            result = pvr_create_compute_secondary_prog(device,
               if (result != VK_SUCCESS)
                  err_free_pds_prim_code_bo:
            err_free_usc_bo:
                  }
      /* TODO: See if we can dedup this with pvr_setup_descriptor_mappings() or
   * pvr_setup_descriptor_mappings().
   */
   static VkResult pvr_write_compute_query_pds_data_section(
      struct pvr_cmd_buffer *cmd_buffer,
   const struct pvr_compute_query_shader *query_prog,
      {
      const struct pvr_pds_info *const info = &query_prog->info;
   struct pvr_suballoc_bo *pvr_bo;
   const uint8_t *entries;
   uint32_t *dword_buffer;
   uint64_t *qword_buffer;
            result = pvr_cmd_buffer_alloc_mem(cmd_buffer,
                     if (result != VK_SUCCESS)
            dword_buffer = (uint32_t *)pvr_bo_suballoc_get_map_addr(pvr_bo);
                     /* TODO: Remove this when we can test this path and make sure that this is
   * not needed. If it's needed we should probably be using LITERAL entries for
   * this instead.
   */
                     for (uint32_t i = 0; i < info->entry_count; i++) {
      const struct pvr_const_map_entry *const entry_header =
            switch (entry_header->type) {
   case PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32: {
                     PVR_WRITE(dword_buffer,
                        entries += sizeof(*literal);
      }
   case PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL64: {
                     PVR_WRITE(qword_buffer,
                        entries += sizeof(*literal);
      }
   case PVR_PDS_CONST_MAP_ENTRY_TYPE_DOUTU_ADDRESS: {
      const struct pvr_const_map_entry_doutu_address *const doutu_addr =
         const pvr_dev_addr_t exec_addr =
      PVR_DEV_ADDR_OFFSET(query_prog->pds_sec_code.pvr_bo->dev_addr,
                        PVR_WRITE(qword_buffer,
                        entries += sizeof(*doutu_addr);
      }
   case PVR_PDS_CONST_MAP_ENTRY_TYPE_SPECIAL_BUFFER: {
                     switch (special_buff_entry->buffer_type) {
                     PVR_WRITE(qword_buffer,
            addr,
                  default:
                  entries += sizeof(*special_buff_entry);
      }
   default:
                     pipeline->pds_shared_update_data_offset =
      pvr_bo->dev_addr.addr -
            }
      static void pvr_write_private_compute_dispatch(
      struct pvr_cmd_buffer *cmd_buffer,
   struct pvr_private_compute_pipeline *pipeline,
      {
      struct pvr_sub_cmd *sub_cmd = cmd_buffer->state.current_sub_cmd;
   const uint32_t workgroup_size[PVR_WORKGROUP_DIMENSIONS] = {
      DIV_ROUND_UP(num_query_indices, 32),
   1,
                        pvr_compute_update_shared_private(cmd_buffer, &sub_cmd->compute, pipeline);
   pvr_compute_update_kernel_private(cmd_buffer,
                        }
      static void
   pvr_destroy_compute_query_program(struct pvr_device *device,
         {
      pvr_destroy_compute_secondary_prog(device, program);
   pvr_bo_suballoc_free(program->pds_prim_code.pvr_bo);
      }
      static VkResult pvr_create_multibuffer_compute_query_program(
      struct pvr_device *device,
   const struct pvr_shader_factory_info *const *shader_factory_info,
      {
      const uint32_t core_count = device->pdevice->dev_runtime_info.core_count;
   VkResult result;
            for (i = 0; i < core_count; i++) {
      result = pvr_create_compute_query_program(device,
               if (result != VK_SUCCESS)
                     err_destroy_compute_query_program:
      for (uint32_t j = 0; j < i; j++)
               }
      VkResult pvr_device_create_compute_query_programs(struct pvr_device *device)
   {
      const uint32_t core_count = device->pdevice->dev_runtime_info.core_count;
            result = pvr_create_compute_query_program(device,
               if (result != VK_SUCCESS)
            device->copy_results_shaders =
      vk_alloc(&device->vk.alloc,
            sizeof(*device->copy_results_shaders) * core_count,
   if (!device->copy_results_shaders) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               result = pvr_create_multibuffer_compute_query_program(
      device,
   copy_query_results_collection,
      if (result != VK_SUCCESS)
            device->reset_queries_shaders =
      vk_alloc(&device->vk.alloc,
            sizeof(*device->reset_queries_shaders) * core_count,
   if (!device->reset_queries_shaders) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               result = pvr_create_multibuffer_compute_query_program(
      device,
   reset_query_collection,
      if (result != VK_SUCCESS)
                  err_vk_free_reset_queries_shaders:
            err_destroy_copy_results_query_programs:
      for (uint32_t i = 0; i < core_count; i++) {
      pvr_destroy_compute_query_program(device,
            err_vk_free_copy_results_shaders:
            err_destroy_availability_query_program:
                  }
      void pvr_device_destroy_compute_query_programs(struct pvr_device *device)
   {
                        for (uint32_t i = 0; i < core_count; i++) {
      pvr_destroy_compute_query_program(device,
         pvr_destroy_compute_query_program(device,
               vk_free(&device->vk.alloc, device->copy_results_shaders);
      }
      static void pvr_init_tex_info(const struct pvr_device_info *dev_info,
                     {
      const uint8_t *swizzle_arr = pvr_get_format_swizzle(tex_info->format);
   bool is_view_1d = !PVR_HAS_FEATURE(dev_info, tpu_extended_integer_lookup) &&
            *tex_info = (struct pvr_texture_state_info){
      .format = VK_FORMAT_R32_UINT,
   .mem_layout = PVR_MEMLAYOUT_LINEAR,
   .flags = PVR_TEXFLAGS_INDEX_LOOKUP,
   .type = is_view_1d ? VK_IMAGE_VIEW_TYPE_1D : VK_IMAGE_VIEW_TYPE_2D,
   .is_cube = false,
   .tex_state_type = PVR_TEXTURE_STATE_SAMPLE,
   .extent = { .width = width, .height = 1, .depth = 0 },
   .array_size = 1,
   .base_level = 0,
   .mip_levels = 1,
   .mipmaps_present = false,
   .sample_count = 1,
   .stride = width,
   .offset = 0,
   .swizzle = { [0] = swizzle_arr[0],
               [1] = swizzle_arr[1],
            }
      /* TODO: Split this function into per program type functions. */
   VkResult pvr_add_query_program(struct pvr_cmd_buffer *cmd_buffer,
         {
      struct pvr_device *device = cmd_buffer->device;
   const uint32_t core_count = device->pdevice->dev_runtime_info.core_count;
   const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const struct pvr_shader_factory_info *shader_factory_info;
   uint64_t sampler_state[ROGUE_NUM_TEXSTATE_SAMPLER_WORDS];
   const struct pvr_compute_query_shader *query_prog;
   struct pvr_private_compute_pipeline pipeline;
   const uint32_t buffer_count = core_count;
   struct pvr_texture_state_info tex_info;
   uint32_t num_query_indices;
   uint32_t *const_buffer;
   struct pvr_suballoc_bo *pvr_bo;
            pvr_csb_pack (&sampler_state[0U], TEXSTATE_SAMPLER, reg) {
      reg.addrmode_u = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
   reg.addrmode_v = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
   reg.addrmode_w = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
   reg.minfilter = PVRX(TEXSTATE_FILTER_POINT);
   reg.magfilter = PVRX(TEXSTATE_FILTER_POINT);
   reg.non_normalized_coords = true;
               /* clang-format off */
   pvr_csb_pack (&sampler_state[1], TEXSTATE_SAMPLER_WORD1, sampler_word1) {}
            switch (query_info->type) {
   case PVR_QUERY_TYPE_AVAILABILITY_WRITE:
      /* Adds a compute shader (fenced on the last 3D) that writes a non-zero
   * value in availability_bo at every index in index_bo.
   */
   query_prog = &device->availability_shader;
   shader_factory_info = &availability_query_write_info;
   num_query_indices = query_info->availability_write.num_query_indices;
         case PVR_QUERY_TYPE_COPY_QUERY_RESULTS:
      /* Adds a compute shader to copy availability and query value data. */
   query_prog = &device->copy_results_shaders[buffer_count - 1];
   shader_factory_info = copy_query_results_collection[buffer_count - 1];
   num_query_indices = query_info->copy_query_results.query_count;
         case PVR_QUERY_TYPE_RESET_QUERY_POOL:
      /* Adds a compute shader to reset availability and query value data. */
   query_prog = &device->reset_queries_shaders[buffer_count - 1];
   shader_factory_info = reset_query_collection[buffer_count - 1];
   num_query_indices = query_info->reset_query_pool.query_count;
         default:
                  result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer,
         if (result != VK_SUCCESS)
            pipeline.pds_code_offset = query_prog->pds_prim_code.code_offset;
            pipeline.pds_shared_update_code_offset =
         pipeline.pds_data_size_dw = query_prog->primary_data_size_dw;
            pipeline.coeff_regs_count = shader_factory_info->coeff_regs;
   pipeline.unified_store_regs_count = shader_factory_info->input_regs;
            const_buffer =
      vk_alloc(&cmd_buffer->vk.pool->alloc,
            PVR_DW_TO_BYTES(shader_factory_info->const_shared_regs),
   if (!const_buffer) {
      return vk_command_buffer_set_error(&cmd_buffer->vk,
                  #define DRIVER_CONST(index)                                            \
      assert(shader_factory_info->driver_const_location_map[index] <      \
         const_buffer[shader_factory_info->driver_const_location_map[index]]
            switch (query_info->type) {
   case PVR_QUERY_TYPE_AVAILABILITY_WRITE: {
      uint64_t image_sampler_state[3][ROGUE_NUM_TEXSTATE_SAMPLER_WORDS];
            memcpy(&image_sampler_state[image_sampler_idx][0],
         &sampler_state[0],
            pvr_init_tex_info(dev_info,
                        result = pvr_pack_tex_state(device,
               if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, const_buffer);
                        pvr_init_tex_info(
      dev_info,
   &tex_info,
               result = pvr_pack_tex_state(device,
               if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, const_buffer);
                        memcpy(&const_buffer[0],
                  /* Only PVR_QUERY_AVAILABILITY_WRITE_COUNT driver consts allowed. */
   assert(shader_factory_info->num_driver_consts ==
            DRIVER_CONST(PVR_QUERY_AVAILABILITY_WRITE_INDEX_COUNT) =
                     case PVR_QUERY_TYPE_COPY_QUERY_RESULTS: {
      PVR_FROM_HANDLE(pvr_query_pool,
               PVR_FROM_HANDLE(pvr_buffer,
               const uint32_t image_sampler_state_arr_size =
         uint32_t image_sampler_idx = 0;
   pvr_dev_addr_t addr;
            STACK_ARRAY(uint64_t, image_sampler_state, image_sampler_state_arr_size);
   if (!image_sampler_state) {
               return vk_command_buffer_set_error(&cmd_buffer->vk,
         #define SAMPLER_ARR_2D(_arr, _i, _j) \
                  memcpy(&SAMPLER_ARR_2D(image_sampler_state, image_sampler_idx, 0),
         &sampler_state[0],
                                       result = pvr_pack_tex_state(
      device,
   &tex_info,
      if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, const_buffer);
                        for (uint32_t i = 0; i < buffer_count; i++) {
                              result = pvr_pack_tex_state(
      device,
   &tex_info,
      if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, const_buffer);
                           memcpy(&const_buffer[0],
                           /* Only PVR_COPY_QUERY_POOL_RESULTS_COUNT driver consts allowed. */
   assert(shader_factory_info->num_driver_consts ==
            /* Assert if no memory is bound to destination buffer. */
            addr = buffer->dev_addr;
   addr.addr += query_info->copy_query_results.dst_offset;
   addr.addr += query_info->copy_query_results.first_query *
            DRIVER_CONST(PVR_COPY_QUERY_POOL_RESULTS_INDEX_COUNT) = num_query_indices;
   DRIVER_CONST(PVR_COPY_QUERY_POOL_RESULTS_BASE_ADDRESS_LOW) = addr.addr &
         DRIVER_CONST(PVR_COPY_QUERY_POOL_RESULTS_BASE_ADDRESS_HIGH) = addr.addr >>
         DRIVER_CONST(PVR_COPY_QUERY_POOL_RESULTS_DEST_STRIDE) =
         DRIVER_CONST(PVR_COPY_QUERY_POOL_RESULTS_PARTIAL_RESULT_FLAG) =
         DRIVER_CONST(PVR_COPY_QUERY_POOL_RESULTS_64_BIT_FLAG) =
         DRIVER_CONST(PVR_COPY_QUERY_POOL_RESULTS_WITH_AVAILABILITY_FLAG) =
      query_info->copy_query_results.flags &
                  case PVR_QUERY_TYPE_RESET_QUERY_POOL: {
      PVR_FROM_HANDLE(pvr_query_pool,
               const uint32_t image_sampler_state_arr_size =
         uint32_t image_sampler_idx = 0;
   pvr_dev_addr_t addr;
            STACK_ARRAY(uint64_t, image_sampler_state, image_sampler_state_arr_size);
   if (!image_sampler_state) {
               return vk_command_buffer_set_error(&cmd_buffer->vk,
               memcpy(&SAMPLER_ARR_2D(image_sampler_state, image_sampler_idx, 0),
         &sampler_state[0],
                     for (uint32_t i = 0; i < buffer_count; i++) {
                              result = pvr_pack_tex_state(
      device,
   &tex_info,
      if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, const_buffer);
                                             result = pvr_pack_tex_state(
      device,
   &tex_info,
      if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, const_buffer);
                  #undef SAMPLER_ARR_2D
            memcpy(&const_buffer[0],
                           /* Only PVR_RESET_QUERY_POOL_COUNT driver consts allowed. */
   assert(shader_factory_info->num_driver_consts ==
            DRIVER_CONST(PVR_RESET_QUERY_POOL_INDEX_COUNT) = num_query_indices;
               default:
               #undef DRIVER_CONST
         for (uint32_t i = 0; i < shader_factory_info->num_static_const; i++) {
      const struct pvr_static_buffer *load =
            /* Assert if static const is out of range. */
   assert(load->dst_idx < shader_factory_info->const_shared_regs);
               result = pvr_cmd_buffer_upload_general(
      cmd_buffer,
   const_buffer,
   PVR_DW_TO_BYTES(shader_factory_info->const_shared_regs),
      if (result != VK_SUCCESS) {
                                             /* PDS data section for the secondary/constant upload. */
   result = pvr_write_compute_query_pds_data_section(cmd_buffer,
               if (result != VK_SUCCESS)
            pipeline.workgroup_size.width = ROGUE_MAX_INSTANCES_PER_TASK;
   pipeline.workgroup_size.height = 1;
                        }
