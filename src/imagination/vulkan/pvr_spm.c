   /*
   * Copyright © 2023 Imagination Technologies Ltd.
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
      #include <stdint.h>
   #include <stddef.h>
   #include <string.h>
   #include <vulkan/vulkan_core.h>
      #include "c11/threads.h"
   #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_csb.h"
   #include "pvr_csb_enum_helpers.h"
   #include "pvr_device_info.h"
   #include "pvr_formats.h"
   #include "pvr_hw_pass.h"
   #include "pvr_job_common.h"
   #include "pvr_pds.h"
   #include "pvr_private.h"
   #include "pvr_shader_factory.h"
   #include "pvr_spm.h"
   #include "pvr_static_shaders.h"
   #include "pvr_tex_state.h"
   #include "pvr_types.h"
   #include "pvr_uscgen.h"
   #include "util/bitscan.h"
   #include "util/macros.h"
   #include "util/simple_mtx.h"
   #include "util/u_atomic.h"
   #include "vk_alloc.h"
   #include "vk_log.h"
      struct pvr_spm_scratch_buffer {
      uint32_t ref_count;
   struct pvr_bo *bo;
      };
      void pvr_spm_init_scratch_buffer_store(struct pvr_device *device)
   {
      struct pvr_spm_scratch_buffer_store *store =
            simple_mtx_init(&store->mtx, mtx_plain);
      }
      void pvr_spm_finish_scratch_buffer_store(struct pvr_device *device)
   {
      struct pvr_spm_scratch_buffer_store *store =
            /* Either a framebuffer was never created so no scratch buffer was ever
   * created or all framebuffers have been freed so only the store's reference
   * remains.
   */
                     if (store->head_ref) {
      pvr_bo_free(device, store->head_ref->bo);
         }
      uint64_t
   pvr_spm_scratch_buffer_calc_required_size(const struct pvr_render_pass *pass,
               {
      uint64_t dwords_per_pixel;
            /* If we're allocating an SPM scratch buffer we'll have a minimum of 1 output
   * reg and/or tile_buffer.
   */
   uint32_t nr_tile_buffers = 1;
            for (uint32_t i = 0; i < pass->hw_setup->render_count; i++) {
      const struct pvr_renderpass_hwsetup_render *hw_render =
            nr_tile_buffers = MAX2(nr_tile_buffers, hw_render->tile_buffers_count);
               dwords_per_pixel =
            buffer_size = ALIGN_POT((uint64_t)framebuffer_width,
         buffer_size *=
               }
      static VkResult
   pvr_spm_scratch_buffer_alloc(struct pvr_device *device,
               {
      const uint32_t cache_line_size =
         struct pvr_spm_scratch_buffer *scratch_buffer;
   struct pvr_bo *bo;
            result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS) {
      *buffer_out = NULL;
               scratch_buffer = vk_alloc(&device->vk.alloc,
                     if (!scratch_buffer) {
      pvr_bo_free(device, bo);
   *buffer_out = NULL;
               *scratch_buffer = (struct pvr_spm_scratch_buffer){
      .bo = bo,
                           }
      static void
   pvr_spm_scratch_buffer_release_locked(struct pvr_device *device,
         {
      struct pvr_spm_scratch_buffer_store *store =
                     if (p_atomic_dec_zero(&buffer->ref_count)) {
      pvr_bo_free(device, buffer->bo);
         }
      void pvr_spm_scratch_buffer_release(struct pvr_device *device,
         {
      struct pvr_spm_scratch_buffer_store *store =
                                 }
      static void pvr_spm_scratch_buffer_store_set_head_ref_locked(
      struct pvr_spm_scratch_buffer_store *store,
      {
      simple_mtx_assert_locked(&store->mtx);
            p_atomic_inc(&buffer->ref_count);
      }
      static void pvr_spm_scratch_buffer_store_release_head_ref_locked(
      struct pvr_device *device,
      {
                           }
      VkResult pvr_spm_scratch_buffer_get_buffer(
      struct pvr_device *device,
   uint64_t size,
      {
      struct pvr_spm_scratch_buffer_store *store =
                           /* When a render requires a PR the fw will wait for other renders to end,
   * free the PB space, unschedule any other vert/frag jobs and solely run the
   * PR on the whole device until completion.
   * Thus we can safely use the same scratch buffer across multiple
   * framebuffers as the scratch buffer is only used during PRs and only one PR
   * can ever be executed at any one time.
   */
   if (store->head_ref && store->head_ref->size <= size) {
         } else {
               if (store->head_ref)
            result = pvr_spm_scratch_buffer_alloc(device, size, &buffer);
   if (result != VK_SUCCESS) {
                                             p_atomic_inc(&buffer->ref_count);
   simple_mtx_unlock(&store->mtx);
               }
      VkResult pvr_device_init_spm_load_state(struct pvr_device *device)
   {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   uint32_t pds_texture_aligned_offsets[PVR_SPM_LOAD_PROGRAM_COUNT];
   uint32_t pds_kick_aligned_offsets[PVR_SPM_LOAD_PROGRAM_COUNT];
   uint32_t usc_aligned_offsets[PVR_SPM_LOAD_PROGRAM_COUNT];
   uint32_t pds_allocation_size = 0;
   uint32_t usc_allocation_size = 0;
   struct pvr_suballoc_bo *pds_bo;
   struct pvr_suballoc_bo *usc_bo;
   uint8_t *mem_ptr;
            static_assert(PVR_SPM_LOAD_PROGRAM_COUNT == ARRAY_SIZE(spm_load_collection),
            /* TODO: We don't need to upload all the programs since the set contains
   * programs for devices with 8 output regs as well. We can save some memory
   * by not uploading them on devices without the feature.
   * It's likely that once the compiler is hooked up we'll be using the shader
   * cache and generate the shaders as needed so this todo will be unnecessary.
                     for (uint32_t i = 0; i < ARRAY_SIZE(spm_load_collection); i++) {
      usc_aligned_offsets[i] = usc_allocation_size;
               result = pvr_bo_suballoc(&device->suballoc_usc,
                           if (result != VK_SUCCESS)
                     for (uint32_t i = 0; i < ARRAY_SIZE(spm_load_collection); i++) {
      memcpy(mem_ptr + usc_aligned_offsets[i],
                              for (uint32_t i = 0; i < ARRAY_SIZE(spm_load_collection); i++) {
      struct pvr_pds_pixel_shader_sa_program pds_texture_program = {
      /* DMA for clear colors and tile buffer address parts. */
      };
            /* TODO: This looks a bit odd and isn't consistent with other code where
   * we're getting the size of the PDS program. Can we improve this?
   */
   pvr_pds_set_sizes_pixel_shader_uniform_texture_code(&pds_texture_program);
   pvr_pds_set_sizes_pixel_shader_sa_texture_data(&pds_texture_program,
            /* TODO: Looking at the pvr_pds_generate_...() functions and the run-time
   * behavior the data size is always the same here. Should we try saving
   * some memory by adjusting things based on that?
   */
   device->spm_load_state.load_program[i].pds_texture_program_data_size =
            pds_texture_aligned_offsets[i] = pds_allocation_size;
   /* FIXME: Figure out the define for alignment of 16. */
   pds_allocation_size +=
                     pds_kick_aligned_offsets[i] = pds_allocation_size;
   /* FIXME: Figure out the define for alignment of 16. */
   pds_allocation_size +=
      ALIGN_POT(PVR_DW_TO_BYTES(pds_kick_program.code_size +
                  /* FIXME: Figure out the define for alignment of 16. */
   result = pvr_bo_suballoc(&device->suballoc_pds,
                           if (result != VK_SUCCESS) {
      pvr_bo_suballoc_free(usc_bo);
                        for (uint32_t i = 0; i < ARRAY_SIZE(spm_load_collection); i++) {
      struct pvr_pds_pixel_shader_sa_program pds_texture_program = {
      /* DMA for clear colors and tile buffer address parts. */
      };
   const pvr_dev_addr_t usc_program_dev_addr =
         struct pvr_pds_kickusc_program pds_kick_program = { 0 };
            pvr_pds_generate_pixel_shader_sa_code_segment(
                  if (spm_load_collection[i].info->msaa_sample_count > 1)
         else
            pvr_pds_setup_doutu(&pds_kick_program.usc_task_control,
                              /* Generated both code and data. */
   pvr_pds_generate_pixel_shader_program(
                  device->spm_load_state.load_program[i].pds_pixel_program_offset =
         device->spm_load_state.load_program[i].pds_uniform_program_offset =
            /* TODO: From looking at the pvr_pds_generate_...() functions, it seems
   * like temps_used is always 1. Should we remove this and hard code it
   * with a define in the PDS code?
   */
   device->spm_load_state.load_program[i].pds_texture_program_temps_count =
               device->spm_load_state.usc_programs = usc_bo;
               }
      void pvr_device_finish_spm_load_state(struct pvr_device *device)
   {
      pvr_bo_suballoc_free(device->spm_load_state.pds_programs);
      }
      static inline enum PVRX(PBESTATE_PACKMODE)
         {
      switch (dword_count) {
   case 1:
         case 2:
         case 3:
         case 4:
         default:
            }
      /**
   * \brief Sets up PBE registers and state values per a single render output.
   *
   * On a PR we want to store tile data to the scratch buffer so we need to
   * setup the Pixel Back End (PBE) to write the data to the scratch buffer. This
   * function sets up the PBE state and register values required to do so, for a
   * single resource whether it be a tile buffer or the output register set.
   *
   * \return Size of the data saved into the scratch buffer in bytes.
   */
   static uint64_t pvr_spm_setup_pbe_state(
      const struct pvr_device_info *dev_info,
   const VkExtent2D *framebuffer_size,
   uint32_t dword_count,
   enum pvr_pbe_source_start_pos source_start,
   uint32_t sample_count,
   pvr_dev_addr_t scratch_buffer_addr,
   uint32_t pbe_state_words_out[static const ROGUE_NUM_PBESTATE_STATE_WORDS],
      {
      const uint32_t stride =
      ALIGN_POT(framebuffer_size->width,
         const struct pvr_pbe_surf_params surface_params = {
      .swizzle = {
      [0] = PIPE_SWIZZLE_X,
   [1] = PIPE_SWIZZLE_Y,
   [2] = PIPE_SWIZZLE_Z,
      },
   .pbe_packmode = pvr_spm_get_pbe_packmode(dword_count),
   .source_format = PVRX(PBESTATE_SOURCE_FORMAT_8_PER_CHANNEL),
   .addr = scratch_buffer_addr,
   .mem_layout = PVR_MEMLAYOUT_LINEAR,
      };
   const struct pvr_pbe_render_params render_params = {
      .max_x_clip = framebuffer_size->width - 1,
   .max_y_clip = framebuffer_size->height - 1,
               pvr_pbe_pack_state(dev_info,
                              return (uint64_t)stride * framebuffer_size->height * sample_count *
      }
      static inline void pvr_set_pbe_all_valid_mask(struct usc_mrt_desc *desc)
   {
      for (uint32_t i = 0; i < ARRAY_SIZE(desc->valid_mask); i++)
      }
      #define PVR_DEV_ADDR_ADVANCE(_addr, _offset) \
            /**
   * \brief Sets up PBE registers, PBE state values and MRT data per a single
   * render output requiring 8 dwords to be written.
   *
   * On a PR we want to store tile data to the scratch buffer so we need to
   * setup the Pixel Back End (PBE) to write the data to the scratch buffer, as
   * well as setup the Multiple Render Target (MRT) info so the compiler knows
   * what data needs to be stored (output regs or tile buffers) and generate the
   * appropriate EOT shader.
   *
   * This function is only available for devices with the eight_output_registers
   * feature thus requiring 8 dwords to be stored.
   *
   * \return Size of the data saved into the scratch buffer in bytes.
   */
   static uint64_t pvr_spm_setup_pbe_eight_dword_write(
      const struct pvr_device_info *dev_info,
   const VkExtent2D *framebuffer_size,
   uint32_t sample_count,
   enum usc_mrt_resource_type source_type,
   uint32_t tile_buffer_idx,
   pvr_dev_addr_t scratch_buffer_addr,
   uint32_t pbe_state_word_0_out[static const ROGUE_NUM_PBESTATE_STATE_WORDS],
   uint32_t pbe_state_word_1_out[static const ROGUE_NUM_PBESTATE_STATE_WORDS],
   uint64_t pbe_reg_word_0_out[static const ROGUE_NUM_PBESTATE_REG_WORDS],
   uint64_t pbe_reg_word_1_out[static const ROGUE_NUM_PBESTATE_REG_WORDS],
      {
      const uint32_t max_pbe_write_size_dw = 4;
   uint32_t render_target_used = 0;
            assert(PVR_HAS_FEATURE(dev_info, eight_output_registers));
            /* To store 8 dwords we need to split this into two
   * ROGUE_PBESTATE_PACKMODE_U32U32U32U32 stores with the second one using
   * PVR_PBE_STARTPOS_BIT128 as the source offset to store the last 4 dwords.
            mem_stored = pvr_spm_setup_pbe_state(dev_info,
                                                                  mem_stored += pvr_spm_setup_pbe_state(dev_info,
                                                         render_target_used++;
               }
      /**
   * \brief Create and upload the EOT PDS program.
   *
   * Essentially DOUTU the USC EOT shader.
   */
   /* TODO: See if we can dedup this with
   * pvr_sub_cmd_gfx_per_job_fragment_programs_create_and_upload().
   */
   static VkResult pvr_pds_pixel_event_program_create_and_upload(
      struct pvr_device *device,
   const struct pvr_suballoc_bo *usc_eot_program,
   uint32_t usc_temp_count,
      {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   struct pvr_pds_event_program program = { 0 };
   uint32_t *staging_buffer;
            pvr_pds_setup_doutu(&program.task_control,
                              staging_buffer =
      vk_alloc(&device->vk.alloc,
            PVR_DW_TO_BYTES(device->pixel_event_data_size_in_dwords),
   if (!staging_buffer)
            pvr_pds_generate_pixel_event_data_segment(&program,
                  result = pvr_gpu_upload_pds(device,
                              staging_buffer,
   device->pixel_event_data_size_in_dwords,
   4,
      vk_free(&device->vk.alloc, staging_buffer);
      }
      /**
   * \brief Sets up the End of Tile (EOT) program for SPM.
   *
   * This sets up an EOT program to store the render pass'es on-chip and
   * off-chip tile data to the SPM scratch buffer on the EOT event.
   */
   VkResult
   pvr_spm_init_eot_state(struct pvr_device *device,
                           {
      const VkExtent2D framebuffer_size = {
      .width = framebuffer->width,
      };
   uint32_t pbe_state_words[PVR_MAX_COLOR_ATTACHMENTS]
         const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   uint32_t total_render_target_used = 0;
   struct pvr_pds_upload pds_eot_program;
   struct util_dynarray usc_shader_binary;
   uint32_t usc_temp_count;
            pvr_dev_addr_t next_scratch_buffer_addr =
                  /* TODO: See if instead of having a separate path for devices with 8 output
   * regs we can instead do this in a loop and dedup some stuff.
   */
   assert(util_is_power_of_two_or_zero(hw_render->output_regs_count) &&
         if (hw_render->output_regs_count == 8) {
                        mem_stored = pvr_spm_setup_pbe_eight_dword_write(
      dev_info,
   &framebuffer_size,
   hw_render->sample_count,
   USC_MRT_RESOURCE_TYPE_OUTPUT_REG,
   0,
   next_scratch_buffer_addr,
   pbe_state_words[total_render_target_used],
   pbe_state_words[total_render_target_used + 1],
   spm_eot_state->pbe_reg_words[total_render_target_used],
               PVR_DEV_ADDR_ADVANCE(next_scratch_buffer_addr, mem_stored);
                     for (uint32_t i = 0; i < hw_render->tile_buffers_count; i++) {
                                    mem_stored = pvr_spm_setup_pbe_eight_dword_write(
      dev_info,
   &framebuffer_size,
   hw_render->sample_count,
   USC_MRT_RESOURCE_TYPE_MEMORY,
   i,
   next_scratch_buffer_addr,
   pbe_state_words[total_render_target_used],
   pbe_state_words[total_render_target_used + 1],
   spm_eot_state->pbe_reg_words[total_render_target_used],
               PVR_DEV_ADDR_ADVANCE(next_scratch_buffer_addr, mem_stored);
         } else {
               mem_stored = pvr_spm_setup_pbe_state(
      dev_info,
   &framebuffer_size,
   hw_render->output_regs_count,
   PVR_PBE_STARTPOS_BIT0,
   hw_render->sample_count,
   next_scratch_buffer_addr,
                                          for (uint32_t i = 0; i < hw_render->tile_buffers_count; i++) {
                              mem_stored = pvr_spm_setup_pbe_state(
      dev_info,
   &framebuffer_size,
   hw_render->output_regs_count,
   PVR_PBE_STARTPOS_BIT0,
   hw_render->sample_count,
   next_scratch_buffer_addr,
                                       pvr_uscgen_eot("SPM EOT",
                  total_render_target_used,
         /* TODO: Create a #define in the compiler code to replace the 16. */
   result = pvr_gpu_upload_usc(device,
                                       if (result != VK_SUCCESS)
            result = pvr_pds_pixel_event_program_create_and_upload(
      device,
   spm_eot_state->usc_eot_program,
   usc_temp_count,
      if (result != VK_SUCCESS) {
      pvr_bo_suballoc_free(spm_eot_state->usc_eot_program);
               spm_eot_state->pixel_event_program_data_upload = pds_eot_program.pvr_bo;
                        }
      void pvr_spm_finish_eot_state(struct pvr_device *device,
         {
      pvr_bo_suballoc_free(spm_eot_state->pixel_event_program_data_upload);
      }
      static VkFormat pvr_get_format_from_dword_count(uint32_t dword_count)
   {
      switch (dword_count) {
   case 1:
         case 2:
         case 4:
         default:
            }
      static VkResult pvr_spm_setup_texture_state_words(
      struct pvr_device *device,
   uint32_t dword_count,
   const VkExtent2D framebuffer_size,
   uint32_t sample_count,
   pvr_dev_addr_t scratch_buffer_addr,
   uint64_t image_descriptor[static const ROGUE_NUM_TEXSTATE_IMAGE_WORDS],
      {
      /* We can ignore the framebuffer's layer count since we only support
   * writing to layer 0.
   */
   struct pvr_texture_state_info info = {
      .format = pvr_get_format_from_dword_count(dword_count),
            .type = VK_IMAGE_VIEW_TYPE_2D,
   .tex_state_type = PVR_TEXTURE_STATE_STORAGE,
   .extent = {
      .width = framebuffer_size.width,
                        .sample_count = sample_count,
               };
   const uint64_t aligned_fb_width =
      ALIGN_POT(framebuffer_size.width,
      const uint64_t fb_area = aligned_fb_width * framebuffer_size.height;
   const uint8_t *format_swizzle;
            format_swizzle = pvr_get_format_swizzle(info.format);
            result = pvr_pack_tex_state(device, &info, image_descriptor);
   if (result != VK_SUCCESS)
                        }
      /* FIXME: Can we dedup this with pvr_load_op_pds_data_create_and_upload() ? */
   static VkResult pvr_pds_bgnd_program_create_and_upload(
      struct pvr_device *device,
   uint32_t texture_program_data_size_in_dwords,
   const struct pvr_bo *consts_buffer,
   uint32_t const_shared_regs,
      {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   struct pvr_pds_pixel_shader_sa_program texture_program = { 0 };
   uint32_t staging_buffer_size;
   uint32_t *staging_buffer;
            pvr_csb_pack (&texture_program.texture_dma_address[0],
                              pvr_csb_pack (&texture_program.texture_dma_control[0],
                  doutd_src1.dest = PVRX(PDSINST_DOUTD_DEST_COMMON_STORE);
                     #if defined(DEBUG)
      pvr_pds_set_sizes_pixel_shader_sa_texture_data(&texture_program, dev_info);
      #endif
                  staging_buffer = vk_alloc(&device->vk.alloc,
                     if (!staging_buffer)
            pvr_pds_generate_pixel_shader_sa_texture_state_data(&texture_program,
                  /* FIXME: Figure out the define for alignment of 16. */
   result = pvr_gpu_upload_pds(device,
                              &staging_buffer[0],
   texture_program_data_size_in_dwords,
   16,
      if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, staging_buffer);
                           }
      VkResult
   pvr_spm_init_bgobj_state(struct pvr_device *device,
                           {
      const uint32_t spm_load_program_idx =
      pvr_get_spm_load_program_index(hw_render->sample_count,
            const VkExtent2D framebuffer_size = {
      .width = framebuffer->width,
      };
   pvr_dev_addr_t next_scratch_buffer_addr =
         struct pvr_spm_per_load_program_state *load_program_state;
   struct pvr_pds_upload pds_texture_data_upload;
   const struct pvr_shader_factory_info *info;
   union pvr_sampler_descriptor *descriptor;
   uint64_t consts_buffer_size;
   uint32_t dword_count;
   uint32_t *mem_ptr;
            assert(spm_load_program_idx < ARRAY_SIZE(spm_load_collection));
                     /* TODO: Remove this check, along with the pvr_finishme(), once the zeroed
   * shaders are replaced by the real shaders.
   */
   if (!consts_buffer_size)
                     result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
                     if (info->driver_const_location_map) {
               for (uint32_t i = 0; i < PVR_SPM_LOAD_CONST_COUNT; i += 2) {
            #if defined(DEBUG)
               #endif
                                          assert(const_map[i] == const_map[i + 1] + 1);
   mem_ptr[const_map[i]] = tile_buffer_addr.addr >> 32;
                  /* TODO: The 32 comes from how the shaders are compiled. We should
   * unhardcode it when this is hooked up to the compiler.
   */
   descriptor = (union pvr_sampler_descriptor *)(mem_ptr + 32);
            pvr_csb_pack (&descriptor->data.sampler_word, TEXSTATE_SAMPLER, sampler) {
      sampler.non_normalized_coords = true;
   sampler.addrmode_v = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
   sampler.addrmode_u = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
   sampler.minfilter = PVRX(TEXSTATE_FILTER_POINT);
   sampler.magfilter = PVRX(TEXSTATE_FILTER_POINT);
   sampler.maxlod = PVRX(TEXSTATE_CLAMP_MIN);
   sampler.minlod = PVRX(TEXSTATE_CLAMP_MIN);
               /* Even if we might have 8 output regs we can only pack and write 4 dwords
   * using R32G32B32A32_UINT.
   */
   if (hw_render->tile_buffers_count > 0)
         else
            for (uint32_t i = 0; i < emit_count; i++) {
      uint64_t *mem_ptr_u64 = (uint64_t *)mem_ptr;
            STATIC_ASSERT(ROGUE_NUM_TEXSTATE_IMAGE_WORDS * sizeof(uint64_t) /
                        result = pvr_spm_setup_texture_state_words(device,
                                       if (result != VK_SUCCESS)
                        assert(spm_load_program_idx <
         load_program_state =
            result = pvr_pds_bgnd_program_create_and_upload(
      device,
   load_program_state->pds_texture_program_data_size,
   spm_bgobj_state->consts_buffer,
   info->const_shared_regs,
      if (result != VK_SUCCESS)
                              /* clang-format off */
   pvr_csb_pack (&spm_bgobj_state->pds_reg_values[0],
                  /* clang-format on */
   value.shader_addr = load_program_state->pds_pixel_program_offset;
               /* clang-format off */
   pvr_csb_pack (&spm_bgobj_state->pds_reg_values[1],
                  /* clang-format on */
   value.texturedata_addr =
               /* clang-format off */
   pvr_csb_pack (&spm_bgobj_state->pds_reg_values[2],
                  /* clang-format on */
   value.usc_sharedsize =
      DIV_ROUND_UP(info->const_shared_regs,
      value.pds_texturestatesize = DIV_ROUND_UP(
      pds_texture_data_upload.data_size,
      value.pds_tempsize =
      DIV_ROUND_UP(load_program_state->pds_texture_program_temps_count,
                  err_free_consts_buffer:
                  }
      void pvr_spm_finish_bgobj_state(struct pvr_device *device,
         {
      pvr_bo_suballoc_free(spm_bgobj_state->pds_texture_data_upload);
      }
      #undef PVR_DEV_ADDR_ADVANCE
