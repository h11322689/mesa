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
   #include <stdio.h>
   #include <vulkan/vulkan.h>
      #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_cdm_load_sr.h"
   #include "pvr_common.h"
   #include "pvr_csb.h"
   #include "pvr_job_context.h"
   #include "pvr_pds.h"
   #include "pvr_private.h"
   #include "pvr_transfer_frag_store.h"
   #include "pvr_types.h"
   #include "pvr_uscgen.h"
   #include "pvr_vdm_load_sr.h"
   #include "pvr_vdm_store_sr.h"
   #include "pvr_winsys.h"
   #include "util/macros.h"
   #include "util/os_file.h"
   #include "util/u_dynarray.h"
   #include "vk_alloc.h"
   #include "vk_log.h"
      /* TODO: Is there some way to ensure the Vulkan driver doesn't exceed this
   * value when constructing the control stream?
   */
   /* The VDM callstack is used by the hardware to implement control stream links
   * with a return, i.e. sub-control streams/subroutines. This value specifies the
   * maximum callstack depth.
   */
   #define PVR_VDM_CALLSTACK_MAX_DEPTH 1U
      #define ROGUE_PDS_TASK_PROGRAM_SIZE 256U
      static VkResult pvr_ctx_reset_cmd_init(struct pvr_device *device,
         {
               /* The reset framework depends on compute support in the hw. */
            if (PVR_HAS_QUIRK(dev_info, 51764))
            if (PVR_HAS_QUIRK(dev_info, 58839))
               }
      static void pvr_ctx_reset_cmd_fini(struct pvr_device *device,
            {
         }
      static VkResult pvr_pds_pt_store_program_create_and_upload(
      struct pvr_device *device,
   struct pvr_bo *pt_bo,
   uint32_t pt_bo_size,
      {
      struct pvr_pds_stream_out_terminate_program program = { 0 };
   const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const uint32_t cache_line_size = rogue_get_slc_cache_line_size(dev_info);
   size_t staging_buffer_size;
   uint32_t *staging_buffer;
   uint32_t *data_buffer;
   uint32_t *code_buffer;
            /* Check the bo size can be converted to dwords without any rounding. */
            program.pds_persistent_temp_size_to_store = pt_bo_size / 4;
            pvr_pds_generate_stream_out_terminate_program(&program,
                        staging_buffer_size = (program.stream_out_terminate_pds_data_size +
                  staging_buffer = vk_zalloc(&device->vk.alloc,
                     if (!staging_buffer)
            data_buffer = staging_buffer;
   code_buffer =
      pvr_pds_generate_stream_out_terminate_program(&program,
                  pvr_pds_generate_stream_out_terminate_program(&program,
                        /* This PDS program is passed to the HW via the PPP state words. These only
   * allow the data segment address to be specified and expect the code
   * segment to immediately follow. Assume the code alignment is the same as
   * the data.
   */
   result =
      pvr_gpu_upload_pds(device,
                     data_buffer,
   program.stream_out_terminate_pds_data_size,
   PVRX(TA_STATE_STREAM_OUT1_PDS_DATA_SIZE_UNIT_SIZE),
   code_buffer,
                     }
      static VkResult pvr_pds_pt_resume_program_create_and_upload(
      struct pvr_device *device,
   struct pvr_bo *pt_bo,
   uint32_t pt_bo_size,
      {
      struct pvr_pds_stream_out_init_program program = { 0 };
   const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const uint32_t cache_line_size = rogue_get_slc_cache_line_size(dev_info);
   size_t staging_buffer_size;
   uint32_t *staging_buffer;
   uint32_t *data_buffer;
   uint32_t *code_buffer;
            /* Check the bo size can be converted to dwords without any rounding. */
            program.num_buffers = 1;
   program.pds_buffer_data_size[0] = pt_bo_size / 4;
            pvr_pds_generate_stream_out_init_program(&program,
                              staging_buffer_size = (program.stream_out_init_pds_data_size +
                  staging_buffer = vk_zalloc(&device->vk.alloc,
                     if (!staging_buffer)
            data_buffer = staging_buffer;
   code_buffer =
      pvr_pds_generate_stream_out_init_program(&program,
                        pvr_pds_generate_stream_out_init_program(&program,
                              /* This PDS program is passed to the HW via the PPP state words. These only
   * allow the data segment address to be specified and expect the code
   * segment to immediately follow. Assume the code alignment is the same as
   * the data.
   */
   result =
      pvr_gpu_upload_pds(device,
                     data_buffer,
   program.stream_out_init_pds_data_size,
   PVRX(TA_STATE_STREAM_OUT1_PDS_DATA_SIZE_UNIT_SIZE),
   code_buffer,
                     }
      static VkResult
   pvr_render_job_pt_programs_setup(struct pvr_device *device,
         {
               result = pvr_bo_alloc(device,
                        device->heaps.pds_heap,
      if (result != VK_SUCCESS)
            result = pvr_pds_pt_store_program_create_and_upload(
      device,
   pt_programs->store_resume_state_bo,
   ROGUE_LLS_PDS_PERSISTENT_TEMPS_BUFFER_SIZE,
      if (result != VK_SUCCESS)
            result = pvr_pds_pt_resume_program_create_and_upload(
      device,
   pt_programs->store_resume_state_bo,
   ROGUE_LLS_PDS_PERSISTENT_TEMPS_BUFFER_SIZE,
      if (result != VK_SUCCESS)
                  err_free_pds_store_program:
            err_free_store_resume_state_bo:
                  }
      static void
   pvr_render_job_pt_programs_cleanup(struct pvr_device *device,
         {
      pvr_bo_suballoc_free(pt_programs->pds_resume_program.pvr_bo);
   pvr_bo_suballoc_free(pt_programs->pds_store_program.pvr_bo);
      }
      static void pvr_pds_ctx_sr_program_setup(
      bool cc_enable,
   uint64_t usc_program_upload_offset,
   uint8_t usc_temps,
   pvr_dev_addr_t sr_addr,
      {
      /* The PDS task is the same for stores and loads. */
      .cc_enable = cc_enable,
   .doutw_control = {
      .dest_store = PDS_UNIFIED_STORE,
   .num_const64 = 2,
   .doutw_data = {
   [0] = sr_addr.addr,
   [1] = sr_addr.addr + ROGUE_LLS_SHARED_REGS_RESERVE_SIZE,
   },
      },
   };
         pvr_pds_setup_doutu(&program_out->usc_task.usc_task_control,
                        }
      /* Note: pvr_pds_compute_ctx_sr_program_create_and_upload() is very similar to
   * this. If there is a problem here it's likely that the same problem exists
   * there so don't forget to update the compute function.
   */
   static VkResult pvr_pds_render_ctx_sr_program_create_and_upload(
      struct pvr_device *device,
   uint64_t usc_program_upload_offset,
   uint8_t usc_temps,
   pvr_dev_addr_t sr_addr,
      {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const uint32_t cache_line_size = rogue_get_slc_cache_line_size(dev_info);
   const uint32_t pds_data_alignment =
            /* FIXME: pvr_pds_generate_shared_storing_program() doesn't return the data
   * and code size when using the PDS_GENERATE_SIZES mode.
   */
   STATIC_ASSERT(ROGUE_PDS_TASK_PROGRAM_SIZE % 4 == 0);
   uint32_t staging_buffer[ROGUE_PDS_TASK_PROGRAM_SIZE / 4U] = { 0 };
   struct pvr_pds_shared_storing_program program;
   ASSERTED uint32_t *buffer_end;
            pvr_pds_ctx_sr_program_setup(false,
                              pvr_pds_generate_shared_storing_program(&program,
                                 buffer_end =
      pvr_pds_generate_shared_storing_program(&program,
                     assert((uint32_t)(buffer_end - staging_buffer) * sizeof(staging_buffer[0]) <
            return pvr_gpu_upload_pds(device,
                           &staging_buffer[0],
   program.data_size,
   PVRX(VDMCTRL_PDS_STATE1_PDS_DATA_ADDR_ALIGNMENT),
      }
      /* Note: pvr_pds_render_ctx_sr_program_create_and_upload() is very similar to
   * this. If there is a problem here it's likely that the same problem exists
   * there so don't forget to update the render_ctx function.
   */
   static VkResult pvr_pds_compute_ctx_sr_program_create_and_upload(
      struct pvr_device *device,
   bool is_loading_program,
   uint64_t usc_program_upload_offset,
   uint8_t usc_temps,
   pvr_dev_addr_t sr_addr,
      {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const uint32_t cache_line_size = rogue_get_slc_cache_line_size(dev_info);
   const uint32_t pds_data_alignment =
            /* FIXME: pvr_pds_generate_shared_storing_program() doesn't return the data
   * and code size when using the PDS_GENERATE_SIZES mode.
   */
   STATIC_ASSERT(ROGUE_PDS_TASK_PROGRAM_SIZE % 4 == 0);
   uint32_t staging_buffer[ROGUE_PDS_TASK_PROGRAM_SIZE / 4U] = { 0 };
   struct pvr_pds_shared_storing_program program;
   uint32_t *buffer_ptr;
            pvr_pds_ctx_sr_program_setup(PVR_HAS_ERN(dev_info, 35421),
                              if (is_loading_program && PVR_NEED_SW_COMPUTE_PDS_BARRIER(dev_info)) {
      pvr_pds_generate_compute_shared_loading_program(&program,
                  } else {
      pvr_pds_generate_shared_storing_program(&program,
                                    buffer_ptr =
      pvr_pds_generate_compute_barrier_conditional(&staging_buffer[code_offset],
         if (is_loading_program && PVR_NEED_SW_COMPUTE_PDS_BARRIER(dev_info)) {
      buffer_ptr = pvr_pds_generate_compute_shared_loading_program(
      &program,
   buffer_ptr,
   PDS_GENERATE_CODE_SEGMENT,
   } else {
      buffer_ptr =
      pvr_pds_generate_shared_storing_program(&program,
                        assert((uint32_t)(buffer_ptr - staging_buffer) * sizeof(staging_buffer[0]) <
            STATIC_ASSERT(PVRX(CR_CDM_CONTEXT_PDS0_DATA_ADDR_ALIGNMENT) ==
            STATIC_ASSERT(PVRX(CR_CDM_CONTEXT_PDS0_CODE_ADDR_ALIGNMENT) ==
            return pvr_gpu_upload_pds(
      device,
   &staging_buffer[0],
   program.data_size,
   PVRX(CR_CDM_CONTEXT_PDS0_DATA_ADDR_ALIGNMENT),
   &staging_buffer[code_offset],
   (uint32_t)(buffer_ptr - &staging_buffer[code_offset]),
   PVRX(CR_CDM_CONTEXT_PDS0_CODE_ADDR_ALIGNMENT),
   cache_line_size,
   }
      enum pvr_ctx_sr_program_target {
      PVR_CTX_SR_RENDER_TARGET,
      };
      static VkResult pvr_ctx_sr_programs_setup(struct pvr_device *device,
               {
      const uint64_t store_load_state_bo_size =
      PVRX(LLS_USC_SHARED_REGS_BUFFER_SIZE) +
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const uint32_t cache_line_size = rogue_get_slc_cache_line_size(dev_info);
   uint64_t usc_store_program_upload_offset;
   uint64_t usc_load_program_upload_offset;
   const uint8_t *usc_load_sr_code;
   uint32_t usc_load_sr_code_size;
            /* Note that this is being used for both compute and render ctx. There is no
   * compute equivalent define for the VDMCTRL unit size.
   */
   /* 4 blocks (16 dwords / 64 bytes) in USC to prevent fragmentation. */
   sr_programs->usc.unified_size =
            result = pvr_bo_alloc(device,
                        device->heaps.pds_heap,
      if (result != VK_SUCCESS)
                              result = pvr_gpu_upload_usc(device,
                           if (result != VK_SUCCESS)
            usc_store_program_upload_offset =
      sr_programs->usc.store_program_bo->dev_addr.addr -
                  if (target == PVR_CTX_SR_COMPUTE_TARGET && PVR_HAS_QUIRK(dev_info, 62269)) {
               usc_load_sr_code = pvr_cdm_load_sr_code;
      } else {
               usc_load_sr_code = pvr_vdm_load_sr_code;
               result = pvr_gpu_upload_usc(device,
                           if (result != VK_SUCCESS)
            usc_load_program_upload_offset =
      sr_programs->usc.load_program_bo->dev_addr.addr -
         /* FIXME: The number of USC temps should be output alongside
   * pvr_vdm_store_sr_code rather than hard coded.
   */
   /* Create and upload the PDS load and store programs. Point them to the
   * appropriate USC load and store programs.
   */
   switch (target) {
   case PVR_CTX_SR_RENDER_TARGET:
      /* PDS state update: SR state store. */
   result = pvr_pds_render_ctx_sr_program_create_and_upload(
      device,
   usc_store_program_upload_offset,
   8,
   sr_programs->store_load_state_bo->vma->dev_addr,
      if (result != VK_SUCCESS)
            /* PDS state update: SR state load. */
   result = pvr_pds_render_ctx_sr_program_create_and_upload(
      device,
   usc_load_program_upload_offset,
   20,
   sr_programs->store_load_state_bo->vma->dev_addr,
      if (result != VK_SUCCESS)
                  case PVR_CTX_SR_COMPUTE_TARGET:
      /* PDS state update: SR state store. */
   result = pvr_pds_compute_ctx_sr_program_create_and_upload(
      device,
   false,
   usc_store_program_upload_offset,
   8,
   sr_programs->store_load_state_bo->vma->dev_addr,
      if (result != VK_SUCCESS)
            /* PDS state update: SR state load. */
   result = pvr_pds_compute_ctx_sr_program_create_and_upload(
      device,
   true,
   usc_load_program_upload_offset,
   20,
   sr_programs->store_load_state_bo->vma->dev_addr,
      if (result != VK_SUCCESS)
                  default:
      unreachable("Invalid target.");
                     err_free_pds_store_program_bo:
            err_free_usc_load_program_bo:
            err_free_usc_store_program_bo:
            err_free_store_load_state_bo:
                  }
      static void pvr_ctx_sr_programs_cleanup(struct pvr_device *device,
         {
      pvr_bo_suballoc_free(sr_programs->pds.load_program.pvr_bo);
   pvr_bo_suballoc_free(sr_programs->pds.store_program.pvr_bo);
   pvr_bo_suballoc_free(sr_programs->usc.load_program_bo);
   pvr_bo_suballoc_free(sr_programs->usc.store_program_bo);
      }
      static VkResult
   pvr_render_ctx_switch_programs_setup(struct pvr_device *device,
         {
               result = pvr_render_job_pt_programs_setup(device, &programs->pt);
   if (result != VK_SUCCESS)
            result = pvr_ctx_sr_programs_setup(device,
               if (result != VK_SUCCESS)
                  err_pt_programs_cleanup:
                  }
      static void
   pvr_render_ctx_switch_programs_cleanup(struct pvr_device *device,
         {
      pvr_ctx_sr_programs_cleanup(device, &programs->sr);
      }
      static VkResult pvr_render_ctx_switch_init(struct pvr_device *device,
         {
      struct pvr_render_ctx_switch *ctx_switch = &ctx->ctx_switch;
   const uint64_t vdm_state_bo_flags = PVR_BO_ALLOC_FLAG_GPU_UNCACHED |
         const uint64_t geom_state_bo_flags = PVR_BO_ALLOC_FLAG_GPU_UNCACHED |
         VkResult result;
            result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
            result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
            for (i = 0; i < ARRAY_SIZE(ctx_switch->programs); i++) {
      result =
         if (result != VK_SUCCESS)
                     err_programs_cleanup:
      for (uint32_t j = 0; j < i; j++)
                  err_pvr_bo_free_vdm_state_bo:
                  }
      static void pvr_render_ctx_switch_fini(struct pvr_device *device,
         {
               for (uint32_t i = 0; i < ARRAY_SIZE(ctx_switch->programs); i++)
            pvr_bo_free(device, ctx_switch->geom_state_bo);
      }
      static void
   pvr_rogue_get_vdmctrl_pds_state_words(struct pvr_pds_upload *pds_program,
                           {
      pvr_csb_pack (state0_out, VDMCTRL_PDS_STATE0, state) {
      /* Convert the data size from dwords to bytes. */
            state.dm_target = PVRX(VDMCTRL_DM_TARGET_VDM);
   state.usc_target = usc_target;
   state.usc_common_size = 0;
   state.usc_unified_size = usc_unified_size;
            assert(pds_data_size % PVRX(VDMCTRL_PDS_STATE0_PDS_DATA_SIZE_UNIT_SIZE) ==
         state.pds_data_size =
               pvr_csb_pack (state1_out, VDMCTRL_PDS_STATE1, state) {
      state.pds_data_addr = PVR_DEV_ADDR(pds_program->data_offset);
   state.sd_type = PVRX(VDMCTRL_SD_TYPE_PDS);
         }
      static void
   pvr_rogue_get_geom_state_stream_out_words(struct pvr_pds_upload *pds_program,
               {
      pvr_csb_pack (stream_out1_out, TA_STATE_STREAM_OUT1, state) {
      /* Convert the data size from dwords to bytes. */
                     assert(pds_data_size %
               state.pds_data_size =
                        pvr_csb_pack (stream_out2_out, TA_STATE_STREAM_OUT2, state) {
            }
      static void pvr_render_ctx_ws_static_state_init(
      struct pvr_render_ctx *ctx,
      {
      uint64_t *q_dst;
            q_dst = &static_state->vdm_ctx_state_base_addr;
   pvr_csb_pack (q_dst, CR_VDM_CONTEXT_STATE_BASE, base) {
                  q_dst = &static_state->geom_ctx_state_base_addr;
   pvr_csb_pack (q_dst, CR_TA_CONTEXT_STATE_BASE, base) {
                  for (uint32_t i = 0; i < ARRAY_SIZE(ctx->ctx_switch.programs); i++) {
      struct rogue_pt_programs *pt_prog = &ctx->ctx_switch.programs[i].pt;
            /* Context store state. */
   q_dst = &static_state->geom_state[i].vdm_ctx_store_task0;
   pvr_csb_pack (q_dst, CR_VDM_CONTEXT_STORE_TASK0, task0) {
      pvr_rogue_get_vdmctrl_pds_state_words(&sr_prog->pds.store_program,
                                 d_dst = &static_state->geom_state[i].vdm_ctx_store_task1;
   pvr_csb_pack (d_dst, CR_VDM_CONTEXT_STORE_TASK1, task1) {
      pvr_csb_pack (&task1.pds_state2, VDMCTRL_PDS_STATE2, state) {
      state.pds_code_addr =
                  q_dst = &static_state->geom_state[i].vdm_ctx_store_task2;
   pvr_csb_pack (q_dst, CR_VDM_CONTEXT_STORE_TASK2, task2) {
      pvr_rogue_get_geom_state_stream_out_words(&pt_prog->pds_store_program,
                     /* Context resume state. */
   q_dst = &static_state->geom_state[i].vdm_ctx_resume_task0;
   pvr_csb_pack (q_dst, CR_VDM_CONTEXT_RESUME_TASK0, task0) {
      pvr_rogue_get_vdmctrl_pds_state_words(&sr_prog->pds.load_program,
                                 d_dst = &static_state->geom_state[i].vdm_ctx_resume_task1;
   pvr_csb_pack (d_dst, CR_VDM_CONTEXT_RESUME_TASK1, task1) {
      pvr_csb_pack (&task1.pds_state2, VDMCTRL_PDS_STATE2, state) {
      state.pds_code_addr =
                  q_dst = &static_state->geom_state[i].vdm_ctx_resume_task2;
   pvr_csb_pack (q_dst, CR_VDM_CONTEXT_RESUME_TASK2, task2) {
      pvr_rogue_get_geom_state_stream_out_words(&pt_prog->pds_resume_program,
                  }
      static void pvr_render_ctx_ws_create_info_init(
      struct pvr_render_ctx *ctx,
   enum pvr_winsys_ctx_priority priority,
      {
      create_info->priority = priority;
               }
      VkResult pvr_render_ctx_create(struct pvr_device *device,
               {
      const uint64_t vdm_callstack_size =
         struct pvr_winsys_render_ctx_create_info create_info;
   struct pvr_render_ctx *ctx;
            ctx = vk_alloc(&device->vk.alloc,
                     if (!ctx)
                     result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
            result = pvr_render_ctx_switch_init(device, ctx);
   if (result != VK_SUCCESS)
            result = pvr_ctx_reset_cmd_init(device, &ctx->reset_cmd);
   if (result != VK_SUCCESS)
            /* ctx must be fully initialized by this point since
   * pvr_render_ctx_ws_create_info_init() depends on this.
   */
            result = device->ws->ops->render_ctx_create(device->ws,
               if (result != VK_SUCCESS)
                           err_render_ctx_reset_cmd_fini:
            err_render_ctx_switch_fini:
            err_free_vdm_callstack_bo:
            err_vk_free_ctx:
                  }
      void pvr_render_ctx_destroy(struct pvr_render_ctx *ctx)
   {
                        pvr_ctx_reset_cmd_fini(device, &ctx->reset_cmd);
   pvr_render_ctx_switch_fini(device, ctx);
   pvr_bo_free(device, ctx->vdm_callstack_bo);
      }
      static VkResult pvr_pds_sr_fence_terminate_program_create_and_upload(
      struct pvr_device *device,
      {
      const uint32_t pds_data_alignment =
         const struct pvr_device_runtime_info *dev_runtime_info =
         ASSERTED const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   uint32_t staging_buffer[PVRX(PDS_TASK_PROGRAM_SIZE) >> 2U];
   struct pvr_pds_fence_program program = { 0 };
   ASSERTED uint32_t *buffer_end;
   uint32_t code_offset;
            /* SW_COMPUTE_PDS_BARRIER is not supported with 2 or more phantoms. */
   assert(!(PVR_NEED_SW_COMPUTE_PDS_BARRIER(dev_info) &&
            pvr_pds_generate_fence_terminate_program(&program,
                        /* FIXME: pvr_pds_generate_fence_terminate_program() zeros out the data_size
   * when we generate the code segment. Implement
   * PDS_GENERATE_CODEDATA_SEGMENTS? Or wait for the pds gen api to change?
   * This behavior doesn't seem consistent with the rest of the api. For now
   * we store the size in a variable.
   */
   data_size = program.data_size;
            buffer_end =
      pvr_pds_generate_fence_terminate_program(&program,
                     assert((uint64_t)(buffer_end - staging_buffer) * sizeof(staging_buffer[0]) <
            return pvr_gpu_upload_pds(device,
                           staging_buffer,
   data_size,
   PVRX(CR_CDM_TERMINATE_PDS_DATA_ADDR_ALIGNMENT),
      }
      static void pvr_compute_ctx_ws_static_state_init(
      const struct pvr_device_info *const dev_info,
   const struct pvr_compute_ctx *const ctx,
      {
                        pvr_csb_pack (&static_state->cdm_ctx_store_pds0,
                  state.data_addr =
         state.code_addr =
               pvr_csb_pack (&static_state->cdm_ctx_store_pds0_b,
                  state.data_addr =
         state.code_addr =
               pvr_csb_pack (&static_state->cdm_ctx_store_pds1,
                  const uint32_t store_program_data_size =
            state.pds_seq_dep = true;
   state.usc_seq_dep = false;
   state.target = true;
   state.unified_size = ctx_switch->sr[0].usc.unified_size;
   state.common_shared = false;
   state.common_size = 0;
            assert(store_program_data_size %
               state.data_size = store_program_data_size /
                                 pvr_csb_pack (&static_state->cdm_ctx_terminate_pds,
                  state.data_addr =
         state.code_addr =
               pvr_csb_pack (&static_state->cdm_ctx_terminate_pds1,
                  /* Convert the data size from dwords to bytes. */
   const uint32_t fence_terminate_program_data_size =
            state.pds_seq_dep = true;
   state.usc_seq_dep = false;
   state.target = !PVR_HAS_FEATURE(dev_info, compute_morton_capable);
   state.unified_size = 0;
   /* Common store is for shareds -- this will free the partitions. */
   state.common_shared = true;
   state.common_size = 0;
            assert(fence_terminate_program_data_size %
               state.data_size = fence_terminate_program_data_size /
                              pvr_csb_pack (&static_state->cdm_ctx_resume_pds0,
                  state.data_addr =
         state.code_addr =
               pvr_csb_pack (&static_state->cdm_ctx_resume_pds0_b,
                  state.data_addr =
         state.code_addr =
         }
      static void pvr_compute_ctx_ws_create_info_init(
      const struct pvr_compute_ctx *const ctx,
   enum pvr_winsys_ctx_priority priority,
      {
               pvr_compute_ctx_ws_static_state_init(&ctx->device->pdevice->dev_info,
            }
      VkResult pvr_compute_ctx_create(struct pvr_device *const device,
               {
      struct pvr_winsys_compute_ctx_create_info create_info;
   struct pvr_compute_ctx *ctx;
            ctx = vk_alloc(&device->vk.alloc,
                     if (!ctx)
                     result = pvr_bo_alloc(
      device,
   device->heaps.general_heap,
   rogue_get_cdm_context_resume_buffer_size(&device->pdevice->dev_info),
   rogue_get_cdm_context_resume_buffer_alignment(&device->pdevice->dev_info),
   PVR_WINSYS_BO_FLAG_CPU_ACCESS | PVR_WINSYS_BO_FLAG_GPU_UNCACHED,
      if (result != VK_SUCCESS)
            /* TODO: Change this so that enabling storage to B doesn't change the array
   * size. Instead of looping we could unroll this and have the second
   * programs setup depending on the B enable. Doing it that way would make
   * things more obvious.
   */
   for (uint32_t i = 0; i < ARRAY_SIZE(ctx->ctx_switch.sr); i++) {
      result = pvr_ctx_sr_programs_setup(device,
               if (result != VK_SUCCESS) {
                                    result = pvr_pds_sr_fence_terminate_program_create_and_upload(
      device,
      if (result != VK_SUCCESS)
                     result = pvr_ctx_reset_cmd_init(device, &ctx->reset_cmd);
   if (result != VK_SUCCESS)
            result = device->ws->ops->compute_ctx_create(device->ws,
               if (result != VK_SUCCESS)
                           err_fini_reset_cmd:
            err_free_pds_fence_terminate_program:
            err_free_sr_programs:
      for (uint32_t i = 0; i < ARRAY_SIZE(ctx->ctx_switch.sr); ++i)
         err_free_state_buffer:
            err_free_ctx:
                  }
      void pvr_compute_ctx_destroy(struct pvr_compute_ctx *const ctx)
   {
                                 pvr_bo_suballoc_free(ctx->ctx_switch.sr_fence_terminate_program.pvr_bo);
   for (uint32_t i = 0; i < ARRAY_SIZE(ctx->ctx_switch.sr); ++i)
                        }
      static void pvr_transfer_ctx_ws_create_info_init(
      enum pvr_winsys_ctx_priority priority,
      {
         }
      static VkResult pvr_transfer_eot_shaders_init(struct pvr_device *device,
         {
               /* Setup start indexes of the shared registers that will contain the PBE
   * state words for each render target. These must match the indexes used in
   * pvr_pds_generate_pixel_event(), which is used to generate the
   * corresponding PDS program in pvr_pbe_setup_emit() via
   * pvr_pds_generate_pixel_event_data_segment() and
   * pvr_pds_generate_pixel_event_code_segment().
   */
   /* TODO: store the shared register information somewhere so that it can be
   * shared with pvr_pbe_setup_emit() rather than having the shared register
   * indexes and number of shared registers hard coded in
   * pvr_pds_generate_pixel_event().
   */
   for (uint32_t i = 0; i < ARRAY_SIZE(rt_pbe_regs); i++)
                     for (uint32_t i = 0; i < ARRAY_SIZE(ctx->usc_eot_bos); i++) {
      const uint32_t cache_line_size =
         const unsigned rt_count = i + 1;
   struct util_dynarray eot_bin;
                     result = pvr_gpu_upload_usc(device,
                           util_dynarray_fini(&eot_bin);
   if (result != VK_SUCCESS) {
                                       }
      static void pvr_transfer_eot_shaders_fini(struct pvr_device *device,
         {
      for (uint32_t i = 0; i < ARRAY_SIZE(ctx->usc_eot_bos); i++)
      }
      static VkResult pvr_transfer_ctx_shaders_init(struct pvr_device *device,
         {
               result = pvr_transfer_frag_store_init(device, &ctx->frag_store);
   if (result != VK_SUCCESS)
            result = pvr_transfer_eot_shaders_init(device, ctx);
   if (result != VK_SUCCESS)
                  err_frag_store_fini:
            err_out:
         }
      static void pvr_transfer_ctx_shaders_fini(struct pvr_device *device,
         {
      pvr_transfer_eot_shaders_fini(device, ctx);
      }
      VkResult pvr_transfer_ctx_create(struct pvr_device *const device,
               {
      struct pvr_winsys_transfer_ctx_create_info create_info;
   struct pvr_transfer_ctx *ctx;
            ctx = vk_zalloc(&device->vk.alloc,
                     if (!ctx)
                     result = pvr_ctx_reset_cmd_init(device, &ctx->reset_cmd);
   if (result != VK_SUCCESS)
                     result = device->ws->ops->transfer_ctx_create(device->ws,
               if (result != VK_SUCCESS)
            result = pvr_transfer_ctx_shaders_init(device, ctx);
   if (result != VK_SUCCESS)
            /* Create the PDS Uniform/Tex state code segment array. */
   for (uint32_t i = 0U; i < ARRAY_SIZE(ctx->pds_unitex_code); i++) {
      for (uint32_t j = 0U; j < ARRAY_SIZE(ctx->pds_unitex_code[0U]); j++) {
                     result = pvr_pds_unitex_state_program_create_and_upload(
      device,
   NULL,
   i,
   j,
      if (result != VK_SUCCESS) {
                                       err_free_pds_unitex_bos:
      for (uint32_t i = 0U; i < ARRAY_SIZE(ctx->pds_unitex_code); i++) {
      for (uint32_t j = 0U; j < ARRAY_SIZE(ctx->pds_unitex_code[0U]); j++) {
                                          err_destroy_transfer_ctx:
            err_fini_reset_cmd:
            err_free_ctx:
                  }
      void pvr_transfer_ctx_destroy(struct pvr_transfer_ctx *const ctx)
   {
               for (uint32_t i = 0U; i < ARRAY_SIZE(ctx->pds_unitex_code); i++) {
      for (uint32_t j = 0U; j < ARRAY_SIZE(ctx->pds_unitex_code[0U]); j++) {
                                    pvr_transfer_ctx_shaders_fini(device, ctx);
   device->ws->ops->transfer_ctx_destroy(ctx->ws_ctx);
   pvr_ctx_reset_cmd_fini(device, &ctx->reset_cmd);
      }
