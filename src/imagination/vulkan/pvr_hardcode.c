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
      #include <stdbool.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <string.h>
   #include <vulkan/vulkan_core.h>
      #include "compiler/shader_enums.h"
   #include "hwdef/rogue_hw_utils.h"
   #include "pvr_device_info.h"
   #include "pvr_hardcode.h"
   #include "pvr_private.h"
   #include "rogue/rogue.h"
   #include "usc/hardcoded_apps/pvr_simple_compute.h"
   #include "util/macros.h"
   #include "util/u_dynarray.h"
   #include "util/u_process.h"
      /**
   * \file pvr_hardcode.c
   *
   * \brief Contains hard coding functions.
   * This should eventually be deleted as the compiler becomes more capable.
   */
      #define PVR_AXE_1_16M_BVNC PVR_BVNC_PACK(33, 15, 11, 3)
   #define PVR_GX6250_BVNC PVR_BVNC_PACK(4, 40, 2, 51)
      #define util_dynarray_append_mem(buf, size, mem) \
            enum pvr_hard_code_shader_type {
      PVR_HARD_CODE_SHADER_TYPE_COMPUTE,
      };
      static const struct pvr_hard_coding_data {
      const char *const name;
   uint64_t bvnc;
            union {
      struct {
                                                struct {
                     uint8_t *const *const vert_shaders;
   unsigned *vert_shader_sizes;
                                                            } hard_coding_table[] = {
      {
      .name = "simple-compute",
   .bvnc = PVR_GX6250_BVNC,
            .compute = {
                     .shader_info = {
      .uses_atomic_ops = false,
                  .const_shared_reg_count = 4,
   .input_register_count = 8,
   .work_size = 1 * 1 * 1,
               .build_info = {
      .ubo_data = { 0 },
   .compile_time_consts_data = {
                  .local_invocation_regs = { 0, 1 },
   .work_group_regs = { 0, 1, 2 },
                  .explicit_conts_usage = {
                     };
      static inline uint64_t
   pvr_device_get_bvnc(const struct pvr_device_info *const dev_info)
   {
                  }
      bool pvr_has_hard_coded_shaders(const struct pvr_device_info *const dev_info)
   {
      const char *const program = util_get_process_name();
            for (uint32_t i = 0; i < ARRAY_SIZE(hard_coding_table); i++) {
      if (bvnc != hard_coding_table[i].bvnc)
            if (strcmp(program, hard_coding_table[i].name) == 0)
                  }
      static const struct pvr_hard_coding_data *
   pvr_get_hard_coding_data(const struct pvr_device_info *const dev_info)
   {
      const char *const program = util_get_process_name();
            for (uint32_t i = 0; i < ARRAY_SIZE(hard_coding_table); i++) {
      if (bvnc != hard_coding_table[i].bvnc)
            if (strcmp(program, hard_coding_table[i].name) == 0)
                           }
      VkResult pvr_hard_code_compute_pipeline(
      struct pvr_device *const device,
   struct pvr_compute_shader_state *const shader_state_out,
      {
      const uint32_t cache_line_size =
         const struct pvr_hard_coding_data *const data =
                              *build_info_out = data->compute.build_info;
            return pvr_gpu_upload_usc(device,
                        }
      uint32_t
   pvr_hard_code_graphics_get_flags(const struct pvr_device_info *const dev_info)
   {
      const struct pvr_hard_coding_data *const data =
                        }
      void pvr_hard_code_graphics_shader(const struct pvr_device_info *const dev_info,
                     {
      const struct pvr_hard_coding_data *const data =
            assert(data->type == PVR_HARD_CODE_SHADER_TYPE_GRAPHICS);
   assert(pipeline_n < data->graphics.shader_count);
            mesa_logd("Hard coding %s stage shader for \"%s\" demo.",
                  switch (stage) {
   case MESA_SHADER_VERTEX:
      util_dynarray_append_mem(shader_out,
                     case MESA_SHADER_FRAGMENT:
      util_dynarray_append_mem(shader_out,
                     default:
            }
      void pvr_hard_code_graphics_vertex_state(
      const struct pvr_device_info *const dev_info,
   uint32_t pipeline_n,
      {
      const struct pvr_hard_coding_data *const data =
            assert(data->type == PVR_HARD_CODE_SHADER_TYPE_GRAPHICS);
   assert(pipeline_n < data->graphics.shader_count);
               }
      void pvr_hard_code_graphics_fragment_state(
      const struct pvr_device_info *const dev_info,
   uint32_t pipeline_n,
      {
      const struct pvr_hard_coding_data *const data =
            assert(data->type == PVR_HARD_CODE_SHADER_TYPE_GRAPHICS);
   assert(pipeline_n < data->graphics.shader_count);
               }
      void pvr_hard_code_graphics_get_build_info(
      const struct pvr_device_info *const dev_info,
   uint32_t pipeline_n,
   gl_shader_stage stage,
   struct rogue_common_build_data *const common_build_data,
   struct rogue_build_data *const build_data,
      {
      const struct pvr_hard_coding_data *const data =
            assert(data->type == PVR_HARD_CODE_SHADER_TYPE_GRAPHICS);
   assert(pipeline_n < data->graphics.shader_count);
            switch (stage) {
   case MESA_SHADER_VERTEX:
      assert(data->graphics.build_infos[pipeline_n]->vert_common_data.temps ==
                  assert(data->graphics.build_infos[pipeline_n]->vert_common_data.coeffs ==
                  build_data->vs = data->graphics.build_infos[pipeline_n]->stage_data.vs;
   *common_build_data =
         *explicit_const_usage =
                  case MESA_SHADER_FRAGMENT:
      assert(data->graphics.build_infos[pipeline_n]->frag_common_data.temps ==
                  assert(data->graphics.build_infos[pipeline_n]->frag_common_data.coeffs ==
                  build_data->fs = data->graphics.build_infos[pipeline_n]->stage_data.fs;
   *common_build_data =
         *explicit_const_usage =
                  default:
            }
      void pvr_hard_code_get_idfwdf_program(
      const struct pvr_device_info *const dev_info,
   struct util_dynarray *program_out,
   uint32_t *usc_shareds_out,
      {
                                 *usc_shareds_out = 12U;
      }
      void pvr_hard_code_get_passthrough_vertex_shader(
      const struct pvr_device_info *const dev_info,
      {
               mesa_loge(
               };
      /* Render target array (RTA). */
   void pvr_hard_code_get_passthrough_rta_vertex_shader(
      const struct pvr_device_info *const dev_info,
      {
                        mesa_loge("No hard coded passthrough rta vertex shader. Returning "
      }
