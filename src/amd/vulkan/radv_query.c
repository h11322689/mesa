   /*
   * Copyrigh 2016 Red Hat Inc.
   * Based on anv:
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
      #include <assert.h>
   #include <fcntl.h>
   #include <stdbool.h>
   #include <string.h>
      #include "bvh/bvh.h"
   #include "meta/radv_meta.h"
   #include "nir/nir_builder.h"
   #include "util/u_atomic.h"
   #include "vulkan/vulkan_core.h"
   #include "radv_cs.h"
   #include "radv_private.h"
   #include "sid.h"
   #include "vk_acceleration_structure.h"
      #define TIMESTAMP_NOT_READY UINT64_MAX
      /* TODO: Add support for mesh/task queries on GFX11 */
   static const unsigned pipeline_statistics_indices[] = {7, 6, 3, 4, 5, 2, 1, 0, 8, 9, 10};
      static unsigned
   radv_get_pipelinestat_query_offset(VkQueryPipelineStatisticFlagBits query)
   {
      uint32_t idx = ffs(query) - 1;
      }
      static unsigned
   radv_get_pipelinestat_query_size(struct radv_device *device)
   {
      unsigned num_results = device->physical_device->rad_info.gfx_level >= GFX11 ? 14 : 11;
      }
      static void
   radv_store_availability(nir_builder *b, nir_def *flags, nir_def *dst_buf, nir_def *offset, nir_def *value32)
   {
                                                               }
      static nir_shader *
   build_occlusion_query_shader(struct radv_device *device)
   {
      /* the shader this builds is roughly
   *
   * push constants {
   * 	uint32_t flags;
   * 	uint32_t dst_stride;
   * };
   *
   * uint32_t src_stride = 16 * db_count;
   *
   * location(binding = 0) buffer dst_buf;
   * location(binding = 1) buffer src_buf;
   *
   * void main() {
   * 	uint64_t result = 0;
   * 	uint64_t src_offset = src_stride * global_id.x;
   * 	uint64_t dst_offset = dst_stride * global_id.x;
   * 	bool available = true;
   * 	for (int i = 0; i < db_count; ++i) {
   *		if (enabled_rb_mask & BITFIELD64_BIT(i)) {
   *			uint64_t start = src_buf[src_offset + 16 * i];
   *			uint64_t end = src_buf[src_offset + 16 * i + 8];
   *			if ((start & (1ull << 63)) && (end & (1ull << 63)))
   *				result += end - start;
   *			else
   *				available = false;
   *		}
   * 	}
   * 	uint32_t elem_size = flags & VK_QUERY_RESULT_64_BIT ? 8 : 4;
   * 	if ((flags & VK_QUERY_RESULT_PARTIAL_BIT) || available) {
   * 		if (flags & VK_QUERY_RESULT_64_BIT)
   * 			dst_buf[dst_offset] = result;
   * 		else
   * 			dst_buf[dst_offset] = (uint32_t)result.
   * 	}
   * 	if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
   * 		dst_buf[dst_offset + elem_size] = available;
   * 	}
   * }
   */
   nir_builder b = radv_meta_init_shader(device, MESA_SHADER_COMPUTE, "occlusion_query");
            nir_variable *result = nir_local_variable_create(b.impl, glsl_uint64_t_type(), "result");
   nir_variable *outer_counter = nir_local_variable_create(b.impl, glsl_int_type(), "outer_counter");
   nir_variable *start = nir_local_variable_create(b.impl, glsl_uint64_t_type(), "start");
   nir_variable *end = nir_local_variable_create(b.impl, glsl_uint64_t_type(), "end");
   nir_variable *available = nir_local_variable_create(b.impl, glsl_bool_type(), "available");
   uint64_t enabled_rb_mask = device->physical_device->rad_info.enabled_rb_mask;
                     nir_def *dst_buf = radv_meta_load_descriptor(&b, 0, 0);
                     nir_def *input_stride = nir_imm_int(&b, db_count * 16);
   nir_def *input_base = nir_imul(&b, input_stride, global_id);
   nir_def *output_stride = nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 4), .range = 8);
            nir_store_var(&b, result, nir_imm_int64(&b, 0), 0x1);
   nir_store_var(&b, outer_counter, nir_imm_int(&b, 0), 0x1);
            nir_def *query_result_wait = nir_test_mask(&b, flags, VK_QUERY_RESULT_WAIT_BIT);
   nir_push_if(&b, query_result_wait);
   {
      /* Wait on the upper word of the last DB entry. */
   nir_push_loop(&b);
   {
                                             nir_push_if(&b, nir_ige_imm(&b, load, 0x80000000));
   {
         }
      }
      }
                     nir_def *current_outer_count = nir_load_var(&b, outer_counter);
                              nir_def *load_offset = nir_imul_imm(&b, current_outer_count, 16);
                     nir_store_var(&b, start, nir_channel(&b, load, 0), 0x1);
            nir_def *start_done = nir_ilt_imm(&b, nir_load_var(&b, start), 0);
                     nir_store_var(&b, result,
                                    nir_pop_if(&b, NULL);
   nir_pop_if(&b, NULL);
                     nir_def *result_is_64bit = nir_test_mask(&b, flags, VK_QUERY_RESULT_64_BIT);
   nir_def *result_size = nir_bcsel(&b, result_is_64bit, nir_imm_int(&b, 8), nir_imm_int(&b, 4));
                                                nir_pop_if(&b, NULL);
            radv_store_availability(&b, flags, dst_buf, nir_iadd(&b, result_size, output_base),
               }
      static nir_shader *
   build_pipeline_statistics_query_shader(struct radv_device *device)
   {
               /* the shader this builds is roughly
   *
   * push constants {
   * 	uint32_t flags;
   * 	uint32_t dst_stride;
   * 	uint32_t stats_mask;
   * 	uint32_t avail_offset;
   * };
   *
   * uint32_t src_stride = pipelinestat_block_size * 2;
   *
   * location(binding = 0) buffer dst_buf;
   * location(binding = 1) buffer src_buf;
   *
   * void main() {
   * 	uint64_t src_offset = src_stride * global_id.x;
   * 	uint64_t dst_base = dst_stride * global_id.x;
   * 	uint64_t dst_offset = dst_base;
   * 	uint32_t elem_size = flags & VK_QUERY_RESULT_64_BIT ? 8 : 4;
   * 	uint32_t elem_count = stats_mask >> 16;
   * 	uint32_t available32 = src_buf[avail_offset + 4 * global_id.x];
   * 	if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
   * 		dst_buf[dst_offset + elem_count * elem_size] = available32;
   * 	}
   * 	if ((bool)available32) {
   * 		// repeat 11 times:
   * 		if (stats_mask & (1 << 0)) {
   * 			uint64_t start = src_buf[src_offset + 8 * indices[0]];
   * 			uint64_t end = src_buf[src_offset + 8 * indices[0] +
   * pipelinestat_block_size]; uint64_t result = end - start; if (flags & VK_QUERY_RESULT_64_BIT)
   * 				dst_buf[dst_offset] = result;
   * 			else
   * 				dst_buf[dst_offset] = (uint32_t)result.
   * 			dst_offset += elem_size;
   * 		}
   * 	} else if (flags & VK_QUERY_RESULT_PARTIAL_BIT) {
   *              // Set everything to 0 as we don't know what is valid.
   * 		for (int i = 0; i < elem_count; ++i)
   * 			dst_buf[dst_base + elem_size * i] = 0;
   * 	}
   * }
   */
   nir_builder b = radv_meta_init_shader(device, MESA_SHADER_COMPUTE, "pipeline_statistics_query");
            nir_variable *output_offset = nir_local_variable_create(b.impl, glsl_int_type(), "output_offset");
            nir_def *flags = nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 0), .range = 4);
   nir_def *stats_mask = nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 8), .range = 12);
            nir_def *dst_buf = radv_meta_load_descriptor(&b, 0, 0);
                     nir_def *input_stride = nir_imm_int(&b, pipelinestat_block_size * 2);
   nir_def *input_base = nir_imul(&b, input_stride, global_id);
   nir_def *output_stride = nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 4), .range = 8);
                              nir_def *result_is_64bit = nir_test_mask(&b, flags, VK_QUERY_RESULT_64_BIT);
   nir_def *elem_size = nir_bcsel(&b, result_is_64bit, nir_imm_int(&b, 8), nir_imm_int(&b, 4));
            radv_store_availability(&b, flags, dst_buf, nir_iadd(&b, output_base, nir_imul(&b, elem_count, elem_size)),
                     nir_store_var(&b, output_offset, output_base, 0x1);
   for (int i = 0; i < ARRAY_SIZE(pipeline_statistics_indices); ++i) {
               nir_def *start_offset = nir_iadd_imm(&b, input_base, pipeline_statistics_indices[i] * 8);
            nir_def *end_offset = nir_iadd_imm(&b, input_base, pipeline_statistics_indices[i] * 8 + pipelinestat_block_size);
                     /* Store result */
                                                                                                nir_variable *counter = nir_local_variable_create(b.impl, glsl_int_type(), "counter");
                     nir_def *current_counter = nir_load_var(&b, counter);
            nir_def *output_elem = nir_iadd(&b, output_base, nir_imul(&b, elem_size, current_counter));
                                                nir_pop_loop(&b, loop);
   nir_pop_if(&b, NULL); /* VK_QUERY_RESULT_PARTIAL_BIT */
   nir_pop_if(&b, NULL); /* nir_i2b(&b, available32) */
      }
      static nir_shader *
   build_tfb_query_shader(struct radv_device *device)
   {
      /* the shader this builds is roughly
   *
   * uint32_t src_stride = 32;
   *
   * location(binding = 0) buffer dst_buf;
   * location(binding = 1) buffer src_buf;
   *
   * void main() {
   *	uint64_t result[2] = {};
   *	bool available = false;
   *	uint64_t src_offset = src_stride * global_id.x;
   * 	uint64_t dst_offset = dst_stride * global_id.x;
   * 	uint64_t *src_data = src_buf[src_offset];
   *	uint32_t avail = (src_data[0] >> 32) &
   *			 (src_data[1] >> 32) &
   *			 (src_data[2] >> 32) &
   *			 (src_data[3] >> 32);
   *	if (avail & 0x80000000) {
   *		result[0] = src_data[3] - src_data[1];
   *		result[1] = src_data[2] - src_data[0];
   *		available = true;
   *	}
   * 	uint32_t result_size = flags & VK_QUERY_RESULT_64_BIT ? 16 : 8;
   * 	if ((flags & VK_QUERY_RESULT_PARTIAL_BIT) || available) {
   *		if (flags & VK_QUERY_RESULT_64_BIT) {
   *			dst_buf[dst_offset] = result;
   *		} else {
   *			dst_buf[dst_offset] = (uint32_t)result;
   *		}
   *	}
   *	if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
   *		dst_buf[dst_offset + result_size] = available;
   * 	}
   * }
   */
   nir_builder b = radv_meta_init_shader(device, MESA_SHADER_COMPUTE, "tfb_query");
            /* Create and initialize local variables. */
   nir_variable *result = nir_local_variable_create(b.impl, glsl_vector_type(GLSL_TYPE_UINT64, 2), "result");
            nir_store_var(&b, result, nir_replicate(&b, nir_imm_int64(&b, 0), 2), 0x3);
                     /* Load resources. */
   nir_def *dst_buf = radv_meta_load_descriptor(&b, 0, 0);
            /* Compute global ID. */
            /* Compute src/dst strides. */
   nir_def *input_stride = nir_imm_int(&b, 32);
   nir_def *input_base = nir_imul(&b, input_stride, global_id);
   nir_def *output_stride = nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 4), .range = 8);
            /* Load data from the query pool. */
   nir_def *load1 = nir_load_ssbo(&b, 4, 32, src_buf, input_base, .align_mul = 32);
            /* Check if result is available. */
   nir_def *avails[2];
   avails[0] = nir_iand(&b, nir_channel(&b, load1, 1), nir_channel(&b, load1, 3));
   avails[1] = nir_iand(&b, nir_channel(&b, load2, 1), nir_channel(&b, load2, 3));
            /* Only compute result if available. */
            /* Pack values. */
   nir_def *packed64[4];
   packed64[0] = nir_pack_64_2x32(&b, nir_trim_vector(&b, load1, 2));
   packed64[1] = nir_pack_64_2x32(&b, nir_vec2(&b, nir_channel(&b, load1, 2), nir_channel(&b, load1, 3)));
   packed64[2] = nir_pack_64_2x32(&b, nir_trim_vector(&b, load2, 2));
            /* Compute result. */
   nir_def *num_primitive_written = nir_isub(&b, packed64[3], packed64[1]);
            nir_store_var(&b, result, nir_vec2(&b, num_primitive_written, primitive_storage_needed), 0x3);
                     /* Determine if result is 64 or 32 bit. */
   nir_def *result_is_64bit = nir_test_mask(&b, flags, VK_QUERY_RESULT_64_BIT);
            /* Store the result if complete or partial results have been requested. */
            /* Store result. */
                                       nir_pop_if(&b, NULL);
            radv_store_availability(&b, flags, dst_buf, nir_iadd(&b, result_size, output_base),
               }
      static nir_shader *
   build_timestamp_query_shader(struct radv_device *device)
   {
      /* the shader this builds is roughly
   *
   * uint32_t src_stride = 8;
   *
   * location(binding = 0) buffer dst_buf;
   * location(binding = 1) buffer src_buf;
   *
   * void main() {
   *	uint64_t result = 0;
   *	bool available = false;
   *	uint64_t src_offset = src_stride * global_id.x;
   * 	uint64_t dst_offset = dst_stride * global_id.x;
   * 	uint64_t timestamp = src_buf[src_offset];
   *	if (timestamp != TIMESTAMP_NOT_READY) {
   *		result = timestamp;
   *		available = true;
   *	}
   * 	uint32_t result_size = flags & VK_QUERY_RESULT_64_BIT ? 8 : 4;
   * 	if ((flags & VK_QUERY_RESULT_PARTIAL_BIT) || available) {
   *		if (flags & VK_QUERY_RESULT_64_BIT) {
   *			dst_buf[dst_offset] = result;
   *		} else {
   *			dst_buf[dst_offset] = (uint32_t)result;
   *		}
   *	}
   *	if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
   *		dst_buf[dst_offset + result_size] = available;
   * 	}
   * }
   */
   nir_builder b = radv_meta_init_shader(device, MESA_SHADER_COMPUTE, "timestamp_query");
            /* Create and initialize local variables. */
   nir_variable *result = nir_local_variable_create(b.impl, glsl_uint64_t_type(), "result");
            nir_store_var(&b, result, nir_imm_int64(&b, 0), 0x1);
                     /* Load resources. */
   nir_def *dst_buf = radv_meta_load_descriptor(&b, 0, 0);
            /* Compute global ID. */
            /* Compute src/dst strides. */
   nir_def *input_stride = nir_imm_int(&b, 8);
   nir_def *input_base = nir_imul(&b, input_stride, global_id);
   nir_def *output_stride = nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 4), .range = 8);
            /* Load data from the query pool. */
            /* Pack the timestamp. */
   nir_def *timestamp;
            /* Check if result is available. */
            /* Only store result if available. */
            nir_store_var(&b, result, timestamp, 0x1);
                     /* Determine if result is 64 or 32 bit. */
   nir_def *result_is_64bit = nir_test_mask(&b, flags, VK_QUERY_RESULT_64_BIT);
            /* Store the result if complete or partial results have been requested. */
            /* Store result. */
                                                         radv_store_availability(&b, flags, dst_buf, nir_iadd(&b, result_size, output_base),
               }
      #define RADV_PGQ_STRIDE     32
   #define RADV_PGQ_STRIDE_GDS (RADV_PGQ_STRIDE + 8 * 2)
      static nir_shader *
   build_pg_query_shader(struct radv_device *device)
   {
      /* the shader this builds is roughly
   *
   * uint32_t src_stride = 32;
   *
   * location(binding = 0) buffer dst_buf;
   * location(binding = 1) buffer src_buf;
   *
   * void main() {
   *	uint64_t result = {};
   *	bool available = false;
   *	uint64_t src_offset = src_stride * global_id.x;
   * 	uint64_t dst_offset = dst_stride * global_id.x;
   * 	uint64_t *src_data = src_buf[src_offset];
   *	uint32_t avail = (src_data[0] >> 32) &
   *			 (src_data[2] >> 32);
   *	if (avail & 0x80000000) {
   *		result = src_data[2] - src_data[0];
   *	        if (use_gds) {
   *			uint32_t ngg_gds_result = 0;
   *			ngg_gds_result += src_data[9] - src_data[8];
   *			result += (uint64_t)ngg_gds_result;
   *	        }
   *		available = true;
   *	}
   * 	uint32_t result_size = flags & VK_QUERY_RESULT_64_BIT ? 8 : 4;
   * 	if ((flags & VK_QUERY_RESULT_PARTIAL_BIT) || available) {
   *		if (flags & VK_QUERY_RESULT_64_BIT) {
   *			dst_buf[dst_offset] = result;
   *		} else {
   *			dst_buf[dst_offset] = (uint32_t)result;
   *		}
   *	}
   *	if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
   *		dst_buf[dst_offset + result_size] = available;
   * 	}
   * }
   */
   nir_builder b = radv_meta_init_shader(device, MESA_SHADER_COMPUTE, "pg_query");
            /* Create and initialize local variables. */
   nir_variable *result = nir_local_variable_create(b.impl, glsl_uint64_t_type(), "result");
            nir_store_var(&b, result, nir_imm_int64(&b, 0), 0x1);
                     /* Load resources. */
   nir_def *dst_buf = radv_meta_load_descriptor(&b, 0, 0);
            /* Compute global ID. */
            /* Determine if the query pool uses GDS for NGG. */
            /* Compute src/dst strides. */
   nir_def *input_stride =
         nir_def *input_base = nir_imul(&b, input_stride, global_id);
   nir_def *output_stride = nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 4), .range = 16);
            /* Load data from the query pool. */
   nir_def *load1 = nir_load_ssbo(&b, 2, 32, src_buf, input_base, .align_mul = 32);
            /* Check if result is available. */
   nir_def *avails[2];
   avails[0] = nir_channel(&b, load1, 1);
   avails[1] = nir_channel(&b, load2, 1);
            nir_push_if(&b, uses_gds);
   {
      nir_def *gds_avail_start = nir_load_ssbo(&b, 1, 32, src_buf, nir_iadd_imm(&b, input_base, 36), .align_mul = 4);
   nir_def *gds_avail_end = nir_load_ssbo(&b, 1, 32, src_buf, nir_iadd_imm(&b, input_base, 44), .align_mul = 4);
   nir_def *gds_result_available =
               }
            /* Only compute result if available. */
            /* Pack values. */
   nir_def *packed64[2];
   packed64[0] = nir_pack_64_2x32(&b, nir_trim_vector(&b, load1, 2));
            /* Compute result. */
                     nir_push_if(&b, uses_gds);
   {
      nir_def *gds_start =
         nir_def *gds_end =
                        }
                     /* Determine if result is 64 or 32 bit. */
   nir_def *result_is_64bit = nir_test_mask(&b, flags, VK_QUERY_RESULT_64_BIT);
            /* Store the result if complete or partial results have been requested. */
            /* Store result. */
                                       nir_pop_if(&b, NULL);
            radv_store_availability(&b, flags, dst_buf, nir_iadd(&b, result_size, output_base),
               }
      static VkResult
   radv_device_init_meta_query_state_internal(struct radv_device *device)
   {
      VkResult result;
   nir_shader *occlusion_cs = NULL;
   nir_shader *pipeline_statistics_cs = NULL;
   nir_shader *tfb_cs = NULL;
   nir_shader *timestamp_cs = NULL;
            mtx_lock(&device->meta_state.mtx);
   if (device->meta_state.query.pipeline_statistics_query_pipeline) {
      mtx_unlock(&device->meta_state.mtx);
      }
   occlusion_cs = build_occlusion_query_shader(device);
   pipeline_statistics_cs = build_pipeline_statistics_query_shader(device);
   tfb_cs = build_tfb_query_shader(device);
   timestamp_cs = build_timestamp_query_shader(device);
            VkDescriptorSetLayoutCreateInfo occlusion_ds_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
   .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
   .bindingCount = 2,
   .pBindings = (VkDescriptorSetLayoutBinding[]){
      {.binding = 0,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
   .pImmutableSamplers = NULL},
   {.binding = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            result = radv_CreateDescriptorSetLayout(radv_device_to_handle(device), &occlusion_ds_create_info,
         if (result != VK_SUCCESS)
            VkPipelineLayoutCreateInfo occlusion_pl_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &device->meta_state.query.ds_layout,
   .pushConstantRangeCount = 1,
               result = radv_CreatePipelineLayout(radv_device_to_handle(device), &occlusion_pl_create_info,
         if (result != VK_SUCCESS)
            VkPipelineShaderStageCreateInfo occlusion_pipeline_shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(occlusion_cs),
   .pName = "main",
               VkComputePipelineCreateInfo occlusion_vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = occlusion_pipeline_shader_stage,
   .flags = 0,
               result =
      radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &occlusion_vk_pipeline_info,
      if (result != VK_SUCCESS)
            VkPipelineShaderStageCreateInfo pipeline_statistics_pipeline_shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(pipeline_statistics_cs),
   .pName = "main",
               VkComputePipelineCreateInfo pipeline_statistics_vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = pipeline_statistics_pipeline_shader_stage,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache,
               if (result != VK_SUCCESS)
            VkPipelineShaderStageCreateInfo tfb_pipeline_shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(tfb_cs),
   .pName = "main",
               VkComputePipelineCreateInfo tfb_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = tfb_pipeline_shader_stage,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &tfb_pipeline_info,
         if (result != VK_SUCCESS)
            VkPipelineShaderStageCreateInfo timestamp_pipeline_shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(timestamp_cs),
   .pName = "main",
               VkComputePipelineCreateInfo timestamp_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = timestamp_pipeline_shader_stage,
   .flags = 0,
               result =
      radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &timestamp_pipeline_info,
      if (result != VK_SUCCESS)
            VkPipelineShaderStageCreateInfo pg_pipeline_shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(pg_cs),
   .pName = "main",
               VkComputePipelineCreateInfo pg_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = pg_pipeline_shader_stage,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &pg_pipeline_info,
         fail:
      ralloc_free(occlusion_cs);
   ralloc_free(pipeline_statistics_cs);
   ralloc_free(tfb_cs);
   ralloc_free(pg_cs);
   ralloc_free(timestamp_cs);
   mtx_unlock(&device->meta_state.mtx);
      }
      VkResult
   radv_device_init_meta_query_state(struct radv_device *device, bool on_demand)
   {
      if (on_demand)
               }
      void
   radv_device_finish_meta_query_state(struct radv_device *device)
   {
      if (device->meta_state.query.tfb_query_pipeline)
      radv_DestroyPipeline(radv_device_to_handle(device), device->meta_state.query.tfb_query_pipeline,
         if (device->meta_state.query.pipeline_statistics_query_pipeline)
      radv_DestroyPipeline(radv_device_to_handle(device), device->meta_state.query.pipeline_statistics_query_pipeline,
         if (device->meta_state.query.occlusion_query_pipeline)
      radv_DestroyPipeline(radv_device_to_handle(device), device->meta_state.query.occlusion_query_pipeline,
         if (device->meta_state.query.timestamp_query_pipeline)
      radv_DestroyPipeline(radv_device_to_handle(device), device->meta_state.query.timestamp_query_pipeline,
         if (device->meta_state.query.pg_query_pipeline)
      radv_DestroyPipeline(radv_device_to_handle(device), device->meta_state.query.pg_query_pipeline,
         if (device->meta_state.query.p_layout)
      radv_DestroyPipelineLayout(radv_device_to_handle(device), device->meta_state.query.p_layout,
         if (device->meta_state.query.ds_layout)
      device->vk.dispatch_table.DestroyDescriptorSetLayout(
   }
      static void
   radv_query_shader(struct radv_cmd_buffer *cmd_buffer, VkPipeline *pipeline, struct radeon_winsys_bo *src_bo,
                     {
      struct radv_device *device = cmd_buffer->device;
   struct radv_meta_saved_state saved_state;
            if (!*pipeline) {
      VkResult ret = radv_device_init_meta_query_state_internal(device);
   if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
                  /* VK_EXT_conditional_rendering says that copy commands should not be
   * affected by conditional rendering.
   */
   radv_meta_save(&saved_state, cmd_buffer,
                  uint64_t src_buffer_size = MAX2(src_stride * count, avail_offset + 4 * count - src_offset);
            radv_buffer_init(&src_buffer, device, src_bo, src_buffer_size, src_offset);
                     radv_meta_push_descriptor_set(
      cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->meta_state.query.p_layout, 0, /* set */
   2,                                                                                /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                           .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
   .pBufferInfo = &(VkDescriptorBufferInfo){.buffer = radv_buffer_to_handle(&dst_buffer),
               {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 1,
   .dstArrayElement = 0,
         /* Encode the number of elements for easy access by the shader. */
   pipeline_stats_mask &= 0x7ff;
                     struct {
      uint32_t flags;
   uint32_t dst_stride;
   uint32_t pipeline_stats_mask;
   uint32_t avail_offset;
               radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.query.p_layout,
                     if (flags & VK_QUERY_RESULT_WAIT_BIT)
                     /* Ensure that the query copy dispatch is complete before a potential vkCmdResetPool because
   * there is an implicit execution dependency from each such query command to all query commands
   * previously submitted to the same queue.
   */
   cmd_buffer->active_query_flush_bits |=
            radv_buffer_finish(&src_buffer);
               }
      static void
   radv_destroy_query_pool(struct radv_device *device, const VkAllocationCallbacks *pAllocator,
         {
      if (pool->vk.query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR)
            if (pool->bo) {
      radv_rmv_log_bo_destroy(device, pool->bo);
               radv_rmv_log_resource_destroy(device, (uint64_t)radv_query_pool_to_handle(pool));
   vk_query_pool_finish(&pool->vk);
      }
      VkResult
   radv_create_query_pool(struct radv_device *device, const VkQueryPoolCreateInfo *pCreateInfo,
         {
      VkResult result;
   size_t pool_struct_size = pCreateInfo->queryType == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR
                  struct radv_query_pool *pool =
            if (!pool)
                     /* The number of primitives generated by geometry shader invocations is only counted by the
   * hardware if GS uses the legacy path. When NGG GS is used, the hardware can't know the number
   * of generated primitives and we have to increment it from the shader using a plain GDS atomic.
   *
   * The number of geometry shader invocations is correctly counted by the hardware for both NGG
   * and the legacy GS path but it increments for NGG VS/TES because they are merged with GS. To
   * avoid this counter to increment, it's also emulated.
   */
   pool->uses_gds =
      (device->physical_device->emulate_ngg_gs_query_pipeline_stat &&
   (pool->vk.pipeline_statistics & (VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
               switch (pCreateInfo->queryType) {
   case VK_QUERY_TYPE_OCCLUSION:
      pool->stride = 16 * device->physical_device->rad_info.max_render_backends;
      case VK_QUERY_TYPE_PIPELINE_STATISTICS:
      pool->stride = radv_get_pipelinestat_query_size(device) * 2;
      case VK_QUERY_TYPE_TIMESTAMP:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
      pool->stride = 8;
      case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      pool->stride = 32;
      case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      if (pool->uses_gds && device->physical_device->rad_info.gfx_level < GFX11) {
      /* When the hardware can use both the legacy and the NGG paths in the same begin/end pair,
   * allocate 2x64-bit values for the GDS counters.
   */
      } else {
         }
      case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
               if (result != VK_SUCCESS) {
      radv_destroy_query_pool(device, pAllocator, pool);
      }
      }
   default:
                  pool->availability_offset = pool->stride * pCreateInfo->queryCount;
   pool->size = pool->availability_offset;
   if (pCreateInfo->queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS)
            result = device->ws->buffer_create(device->ws, pool->size, 64, RADEON_DOMAIN_GTT,
         if (result != VK_SUCCESS) {
      radv_destroy_query_pool(device, pAllocator, pool);
               pool->ptr = device->ws->buffer_map(pool->bo);
   if (!pool->ptr) {
      radv_destroy_query_pool(device, pAllocator, pool);
               *pQueryPool = radv_query_pool_to_handle(pool);
   radv_rmv_log_query_pool_create(device, *pQueryPool, is_internal);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateQueryPool(VkDevice _device, const VkQueryPoolCreateInfo *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyQueryPool(VkDevice _device, VkQueryPool _pool, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!pool)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetQueryPoolResults(VkDevice _device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
   RADV_FROM_HANDLE(radv_query_pool, pool, queryPool);
   char *data = pData;
            if (vk_device_is_lost(&device->vk))
            for (unsigned query_idx = 0; query_idx < queryCount; ++query_idx, data += stride) {
      char *dest = data;
   unsigned query = firstQuery + query_idx;
   char *src = pool->ptr + query * pool->stride;
            switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_TIMESTAMP:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR: {
                     do {
                                          if (flags & VK_QUERY_RESULT_64_BIT) {
      if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
            } else {
      if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
            }
      }
   case VK_QUERY_TYPE_OCCLUSION: {
      p_atomic_uint64_t const *src64 = (p_atomic_uint64_t const *)src;
   uint32_t db_count = device->physical_device->rad_info.max_render_backends;
   uint64_t enabled_rb_mask = device->physical_device->rad_info.enabled_rb_mask;
                                                   do {
                        if (!(start & (1ull << 63)) || !(end & (1ull << 63)))
         else {
                                    if (flags & VK_QUERY_RESULT_64_BIT) {
      if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
            } else {
      if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
            }
      }
   case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
                     do {
                                 const uint64_t *start = (uint64_t *)src;
   const uint64_t *stop = (uint64_t *)(src + pipelinestat_block_size);
   if (flags & VK_QUERY_RESULT_64_BIT) {
      uint64_t *dst = (uint64_t *)dest;
   dest += util_bitcount(pool->vk.pipeline_statistics) * 8;
   for (int i = 0; i < ARRAY_SIZE(pipeline_statistics_indices); ++i) {
      if (pool->vk.pipeline_statistics & (1u << i)) {
      if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT)) {
         }
               } else {
      uint32_t *dst = (uint32_t *)dest;
   dest += util_bitcount(pool->vk.pipeline_statistics) * 4;
   for (int i = 0; i < ARRAY_SIZE(pipeline_statistics_indices); ++i) {
      if (pool->vk.pipeline_statistics & (1u << i)) {
      if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT)) {
         }
            }
      }
   case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT: {
      p_atomic_uint64_t const *src64 = (p_atomic_uint64_t const *)src;
                  /* SAMPLE_STREAMOUTSTATS stores this structure:
   * {
   *	u64 NumPrimitivesWritten;
   *	u64 PrimitiveStorageNeeded;
   * }
   */
   do {
      available = 1;
   for (int j = 0; j < 4; j++) {
      if (!(p_atomic_read(src64 + j) & 0x8000000000000000UL))
                                                if (flags & VK_QUERY_RESULT_64_BIT) {
      if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
         dest += 8;
   if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
            } else {
      if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
         dest += 4;
   if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
            }
      }
   case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT: {
      const bool uses_gds_query = pool->uses_gds && device->physical_device->rad_info.gfx_level < GFX11;
                  /* SAMPLE_STREAMOUTSTATS stores this structure:
   * {
   *	u64 NumPrimitivesWritten;
   *	u64 PrimitiveStorageNeeded;
   * }
   */
   do {
      available = 1;
   if (!(p_atomic_read(src64 + 0) & 0x8000000000000000UL) ||
      !(p_atomic_read(src64 + 2) & 0x8000000000000000UL)) {
      }
   if (uses_gds_query && (!(p_atomic_read(src64 + 4) & 0x8000000000000000UL) ||
                                                   if (uses_gds_query) {
      /* Accumulate the result that was copied from GDS in case NGG shader has been used. */
               if (flags & VK_QUERY_RESULT_64_BIT) {
      if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
            } else {
      if (available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
            }
      }
   case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      struct radv_pc_query_pool *pc_pool = (struct radv_pc_query_pool *)pool;
   const p_atomic_uint64_t *src64 = (const p_atomic_uint64_t *)src;
   bool avail;
   do {
      avail = true;
   for (unsigned i = 0; i < pc_pool->num_passes; ++i)
                              radv_pc_get_results(pc_pool, src64, dest);
   dest += pc_pool->num_counters * sizeof(union VkPerformanceCounterResultKHR);
      }
   default:
                  if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
      if (flags & VK_QUERY_RESULT_64_BIT) {
         } else {
                           }
      static void
   emit_query_flush(struct radv_cmd_buffer *cmd_buffer, struct radv_query_pool *pool)
   {
      if (cmd_buffer->pending_reset_query) {
      if (pool->size >= RADV_BUFFER_OPS_CS_THRESHOLD) {
      /* Only need to flush caches if the query pool size is
   * large enough to be reset using the compute shader
   * path. Small pools don't need any cache flushes
   * because we use a CP dma clear.
   */
            }
      static size_t
   radv_query_result_size(const struct radv_query_pool *pool, VkQueryResultFlags flags)
   {
      unsigned values = (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) ? 1 : 0;
   switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_TIMESTAMP:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
   case VK_QUERY_TYPE_OCCLUSION:
      values += 1;
      case VK_QUERY_TYPE_PIPELINE_STATISTICS:
      values += util_bitcount(pool->vk.pipeline_statistics);
      case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      values += 2;
      case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      values += 1;
      default:
         }
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
               {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_query_pool, pool, queryPool);
   RADV_FROM_HANDLE(radv_buffer, dst_buffer, dstBuffer);
   struct radeon_cmdbuf *cs = cmd_buffer->cs;
   uint64_t va = radv_buffer_get_va(pool->bo);
   uint64_t dest_va = radv_buffer_get_va(dst_buffer->bo);
   size_t dst_size = radv_query_result_size(pool, flags);
            if (!queryCount)
            radv_cs_add_buffer(cmd_buffer->device->ws, cmd_buffer->cs, pool->bo);
            /* Workaround engines that forget to properly specify WAIT_BIT because some driver implicitly
   * synchronizes before query copy.
   */
   if (cmd_buffer->device->instance->flush_before_query_copy)
            /* From the Vulkan spec 1.1.108:
   *
   * "vkCmdCopyQueryPoolResults is guaranteed to see the effect of
   *  previous uses of vkCmdResetQueryPool in the same queue, without any
   *  additional synchronization."
   *
   * So, we have to flush the caches if the compute shader path was used.
   */
            switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
      radv_query_shader(cmd_buffer, &cmd_buffer->device->meta_state.query.occlusion_query_pipeline, pool->bo,
                  case VK_QUERY_TYPE_PIPELINE_STATISTICS:
      if (flags & VK_QUERY_RESULT_WAIT_BIT) {
                                          /* This waits on the ME. All copies below are done on the ME */
         }
   radv_query_shader(cmd_buffer, &cmd_buffer->device->meta_state.query.pipeline_statistics_query_pipeline, pool->bo,
                        case VK_QUERY_TYPE_TIMESTAMP:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
      if (flags & VK_QUERY_RESULT_WAIT_BIT) {
      for (unsigned i = 0; i < queryCount; ++i, dest_va += stride) {
                              /* Wait on the high 32 bits of the timestamp in
   * case the low part is 0xffffffff.
   */
   radv_cp_wait_mem(cs, cmd_buffer->qf, WAIT_REG_MEM_NOT_EQUAL, local_src_va + 4, TIMESTAMP_NOT_READY >> 32,
                  radv_query_shader(cmd_buffer, &cmd_buffer->device->meta_state.query.timestamp_query_pipeline, pool->bo,
                  case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      if (flags & VK_QUERY_RESULT_WAIT_BIT) {
      for (unsigned i = 0; i < queryCount; i++) {
                              /* Wait on the upper word of all results. */
   for (unsigned j = 0; j < 4; j++, src_va += 8) {
                        radv_query_shader(cmd_buffer, &cmd_buffer->device->meta_state.query.tfb_query_pipeline, pool->bo, dst_buffer->bo,
                  case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      if (flags & VK_QUERY_RESULT_WAIT_BIT) {
               for (unsigned i = 0; i < queryCount; i++) {
                              /* Wait on the upper word of the PrimitiveStorageNeeded result. */
                  if (uses_gds_query) {
      radv_cp_wait_mem(cs, cmd_buffer->qf, WAIT_REG_MEM_GREATER_OR_EQUAL, src_va + 36, 0x80000000, 0xffffffff);
                     radv_query_shader(cmd_buffer, &cmd_buffer->device->meta_state.query.pg_query_pipeline, pool->bo, dst_buffer->bo,
                        default:
            }
      static uint32_t
   query_clear_value(VkQueryType type)
   {
      switch (type) {
   case VK_QUERY_TYPE_TIMESTAMP:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
         default:
            }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_query_pool, pool, queryPool);
   uint32_t value = query_clear_value(pool->vk.query_type);
            /* Make sure to sync all previous work if the given command buffer has
   * pending active queries. Otherwise the GPU might write queries data
   * after the reset operation.
   */
            flush_bits |= radv_fill_buffer(cmd_buffer, NULL, pool->bo, radv_buffer_get_va(pool->bo) + firstQuery * pool->stride,
            if (pool->vk.query_type == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
      flush_bits |=
      radv_fill_buffer(cmd_buffer, NULL, pool->bo,
            if (flush_bits) {
      /* Only need to flush caches for the compute shader path. */
   cmd_buffer->pending_reset_query = true;
         }
      VKAPI_ATTR void VKAPI_CALL
   radv_ResetQueryPool(VkDevice _device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
   {
               uint32_t value = query_clear_value(pool->vk.query_type);
   uint32_t *data = (uint32_t *)(pool->ptr + firstQuery * pool->stride);
            for (uint32_t *p = data; p != data_end; ++p)
            if (pool->vk.query_type == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
            }
      static unsigned
   event_type_for_stream(unsigned stream)
   {
      switch (stream) {
   default:
   case 0:
         case 1:
         case 2:
         case 3:
            }
      static void
   emit_sample_streamout(struct radv_cmd_buffer *cmd_buffer, uint64_t va, uint32_t index)
   {
                                 radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 2, 0));
   radeon_emit(cs, EVENT_TYPE(event_type_for_stream(index)) | EVENT_INDEX(3));
   radeon_emit(cs, va);
      }
      static void
   gfx10_copy_gds_query(struct radv_cmd_buffer *cmd_buffer, uint32_t gds_offset, uint64_t va)
   {
               /* Make sure GDS is idle before copying the value. */
   cmd_buffer->state.flush_bits |= RADV_CMD_FLAG_PS_PARTIAL_FLUSH | RADV_CMD_FLAG_INV_L2;
            radeon_emit(cs, PKT3(PKT3_COPY_DATA, 4, 0));
   radeon_emit(cs, COPY_DATA_SRC_SEL(COPY_DATA_GDS) | COPY_DATA_DST_SEL(COPY_DATA_DST_MEM) | COPY_DATA_WR_CONFIRM);
   radeon_emit(cs, gds_offset);
   radeon_emit(cs, 0);
   radeon_emit(cs, va);
      }
      static void
   radv_update_hw_pipelinestat(struct radv_cmd_buffer *cmd_buffer)
   {
               if (num_pipeline_stat_queries == 0) {
      cmd_buffer->state.flush_bits &= ~RADV_CMD_FLAG_START_PIPELINE_STATS;
      } else if (num_pipeline_stat_queries == 1) {
      cmd_buffer->state.flush_bits &= ~RADV_CMD_FLAG_STOP_PIPELINE_STATS;
         }
      static void
   emit_begin_query(struct radv_cmd_buffer *cmd_buffer, struct radv_query_pool *pool, uint64_t va, VkQueryType query_type,
         {
      struct radeon_cmdbuf *cs = cmd_buffer->cs;
   switch (query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
               ++cmd_buffer->state.active_occlusion_queries;
   if (cmd_buffer->state.active_occlusion_queries == 1) {
      if (flags & VK_QUERY_CONTROL_PRECISE_BIT) {
      /* This is the first occlusion query, enable
   * the hint if the precision bit is set.
   */
                  } else {
      if ((flags & VK_QUERY_CONTROL_PRECISE_BIT) && !cmd_buffer->state.perfect_occlusion_queries_enabled) {
      /* This is not the first query, but this one
   * needs to enable precision, DB_COUNT_CONTROL
   * has to be updated accordingly.
                                 if (cmd_buffer->device->physical_device->rad_info.gfx_level >= GFX11 &&
      cmd_buffer->device->physical_device->rad_info.pfp_fw_version >= EVENT_WRITE_ZPASS_PFP_VERSION) {
      } else {
      radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 2, 0));
   if (cmd_buffer->device->physical_device->rad_info.gfx_level >= GFX11) {
         } else {
            }
   radeon_emit(cs, va);
   radeon_emit(cs, va >> 32);
      case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
                                 if (radv_cmd_buffer_uses_mec(cmd_buffer)) {
      uint32_t cs_invoc_offset =
                     radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 2, 0));
   radeon_emit(cs, EVENT_TYPE(V_028A90_SAMPLE_PIPELINESTAT) | EVENT_INDEX(2));
   radeon_emit(cs, va);
            if (pool->uses_gds) {
      if (pool->vk.pipeline_statistics & VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT) {
                                 if (pool->vk.pipeline_statistics & VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT) {
                                                                  }
      }
   case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      if (cmd_buffer->device->physical_device->use_ngg_streamout) {
      /* generated prim counter */
                  /* written prim counter */
                                                   } else {
                           }
      case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT: {
      if (cmd_buffer->device->physical_device->rad_info.gfx_level >= GFX11) {
      /* On GFX11+, primitives generated query always use GDS. */
                                                   } else {
                                 if (old_streamout_enabled != radv_is_streamout_enabled(cmd_buffer)) {
            } else {
                           if (pool->uses_gds) {
      /* generated prim counter */
                                                               }
      }
   case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      radv_pc_begin_query(cmd_buffer, (struct radv_pc_query_pool *)pool, va);
      }
   default:
            }
      static void
   emit_end_query(struct radv_cmd_buffer *cmd_buffer, struct radv_query_pool *pool, uint64_t va, uint64_t avail_va,
         {
      struct radeon_cmdbuf *cs = cmd_buffer->cs;
   switch (query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
               cmd_buffer->state.active_occlusion_queries--;
   if (cmd_buffer->state.active_occlusion_queries == 0) {
      /* Reset the perfect occlusion queries hint now that no
   * queries are active.
                              if (cmd_buffer->device->physical_device->rad_info.gfx_level >= GFX11 &&
      cmd_buffer->device->physical_device->rad_info.pfp_fw_version >= EVENT_WRITE_ZPASS_PFP_VERSION) {
      } else {
      radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 2, 0));
   if (cmd_buffer->device->physical_device->rad_info.gfx_level >= GFX11) {
         } else {
            }
   radeon_emit(cs, va + 8);
               case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
                                                   if (radv_cmd_buffer_uses_mec(cmd_buffer)) {
      uint32_t cs_invoc_offset =
                     radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 2, 0));
   radeon_emit(cs, EVENT_TYPE(V_028A90_SAMPLE_PIPELINESTAT) | EVENT_INDEX(2));
   radeon_emit(cs, va);
            if (pool->uses_gds) {
      if (pool->vk.pipeline_statistics & VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT) {
                                 if (pool->vk.pipeline_statistics & VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT) {
                                          if (!cmd_buffer->state.active_pipeline_gds_queries)
               si_cs_emit_write_event_eop(cs, cmd_buffer->device->physical_device->rad_info.gfx_level, cmd_buffer->qf,
                  }
   case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      if (cmd_buffer->device->physical_device->use_ngg_streamout) {
      /* generated prim counter */
                  /* written prim counter */
                           if (!cmd_buffer->state.active_prims_xfb_gds_queries)
      } else {
                           }
      case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT: {
      if (cmd_buffer->device->physical_device->rad_info.gfx_level >= GFX11) {
      /* On GFX11+, primitives generated query always use GDS. */
                           if (!cmd_buffer->state.active_prims_gen_gds_queries)
      } else {
                                 if (old_streamout_enabled != radv_is_streamout_enabled(cmd_buffer)) {
            } else {
                           if (pool->uses_gds) {
      /* generated prim counter */
                           if (!cmd_buffer->state.active_prims_gen_gds_queries)
                  }
      }
   case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      radv_pc_end_query(cmd_buffer, (struct radv_pc_query_pool *)pool, va);
      }
   default:
                  cmd_buffer->active_query_flush_bits |=
         if (cmd_buffer->device->physical_device->rad_info.gfx_level >= GFX9) {
            }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdBeginQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query,
         {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_query_pool, pool, queryPool);
   struct radeon_cmdbuf *cs = cmd_buffer->cs;
                                          }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdEndQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, uint32_t index)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_query_pool, pool, queryPool);
   uint64_t va = radv_buffer_get_va(pool->bo);
   uint64_t avail_va = va + pool->availability_offset + 4 * query;
            /* Do not need to add the pool BO to the list because the query must
   * currently be active, which means the BO is already in the list.
   */
            /*
   * For multiview we have to emit a query for each bit in the mask,
   * however the first query we emit will get the totals for all the
   * operations, so we don't want to get a real value in the other
   * queries. This emits a fake begin/end sequence so the waiting
   * code gets a completed query value and doesn't hang, but the
   * query returns 0.
   */
   if (cmd_buffer->state.render.view_mask) {
      for (unsigned i = 1; i < util_bitcount(cmd_buffer->state.render.view_mask); i++) {
      va += pool->stride;
   avail_va += 4;
   emit_begin_query(cmd_buffer, pool, va, pool->vk.query_type, 0, 0);
            }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdWriteTimestamp2(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkQueryPool queryPool,
         {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_query_pool, pool, queryPool);
   struct radeon_cmdbuf *cs = cmd_buffer->cs;
   uint64_t va = radv_buffer_get_va(pool->bo);
                     if (cmd_buffer->device->instance->flush_before_timestamp_write) {
      /* Make sure previously launched waves have finished */
                        int num_queries = 1;
   if (cmd_buffer->state.render.view_mask)
                     for (unsigned i = 0; i < num_queries; i++) {
      if (stage == VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT) {
      radeon_emit(cs, PKT3(PKT3_COPY_DATA, 4, 0));
   radeon_emit(cs, COPY_DATA_COUNT_SEL | COPY_DATA_WR_CONFIRM | COPY_DATA_SRC_SEL(COPY_DATA_TIMESTAMP) |
         radeon_emit(cs, 0);
   radeon_emit(cs, 0);
   radeon_emit(cs, query_va);
      } else {
      si_cs_emit_write_event_eop(cs, cmd_buffer->device->physical_device->rad_info.gfx_level, cmd_buffer->qf,
            }
               cmd_buffer->active_query_flush_bits |=
         if (cmd_buffer->device->physical_device->rad_info.gfx_level >= GFX9) {
                     }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdWriteAccelerationStructuresPropertiesKHR(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount,
               {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_query_pool, pool, queryPool);
   struct radeon_cmdbuf *cs = cmd_buffer->cs;
   uint64_t pool_va = radv_buffer_get_va(pool->bo);
                                       for (uint32_t i = 0; i < accelerationStructureCount; ++i) {
      RADV_FROM_HANDLE(vk_acceleration_structure, accel_struct, pAccelerationStructures[i]);
            switch (queryType) {
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
      va += offsetof(struct radv_accel_struct_header, compacted_size);
      case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
      va += offsetof(struct radv_accel_struct_header, serialization_size);
      case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
      va += offsetof(struct radv_accel_struct_header, instance_count);
      case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
      va += offsetof(struct radv_accel_struct_header, size);
      default:
                  radeon_emit(cs, PKT3(PKT3_COPY_DATA, 4, 0));
   radeon_emit(cs, COPY_DATA_SRC_SEL(COPY_DATA_SRC_MEM) | COPY_DATA_DST_SEL(COPY_DATA_DST_MEM) |
         radeon_emit(cs, va);
   radeon_emit(cs, va >> 32);
   radeon_emit(cs, query_va);
                           }
