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
      #include <assert.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <vulkan/vulkan_core.h>
      #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_common.h"
   #include "pvr_device_info.h"
   #include "pvr_job_transfer.h"
   #include "pvr_pds.h"
   #include "pvr_private.h"
   #include "pvr_transfer_frag_store.h"
   #include "pvr_types.h"
   #include "pvr_uscgen.h"
   #include "util/hash_table.h"
   #include "util/macros.h"
   #include "util/ralloc.h"
   #include "util/u_dynarray.h"
   #include "util/u_math.h"
   #include "vk_log.h"
      #define PVR_TRANSFER_BYTE_UNWIND_MAX 16U
      struct pvr_transfer_frag_store_entry_data {
      pvr_dev_addr_t kick_usc_pds_offset;
            struct pvr_suballoc_bo *usc_upload;
      };
      #define to_pvr_entry_data(_entry) \
      _Generic((_entry), \
               VkResult pvr_transfer_frag_store_init(struct pvr_device *device,
         {
               *store = (struct pvr_transfer_frag_store){
      .max_multisample = PVR_GET_FEATURE_VALUE(dev_info, max_multisample, 1U),
               if (!store->hash_table)
               }
      /**
   * \brief Returns a key based on shader properties.
   *
   * Returns a unique key that can be used to uniquely identify a transfer
   * fragment shader based on the provided shader properties.
   *
   * Make sure that the non valid parts of shader_props are memset to 0. Otherwise
   * these bits might appear in the key as uninitialized data and might not
   * match a key for the same shader.
   */
   static uint32_t pvr_transfer_frag_shader_key(
      uint32_t max_multisample,
      {
      const struct pvr_tq_layer_properties *layer = &shader_props->layer_props;
            uint32_t num_layers_bits = util_logbase2_ceil(PVR_TRANSFER_MAX_LAYERS + 1U);
   uint32_t layer_float_bits = util_logbase2_ceil(PVR_INT_COORD_SET_FLOATS_NUM);
   uint32_t pixel_src_bits = util_logbase2_ceil(PVR_TRANSFER_PBE_PIXEL_SRC_NUM);
   uint32_t byte_unwind_bits = util_logbase2_ceil(PVR_TRANSFER_BYTE_UNWIND_MAX);
   uint32_t resolve_op_bits = util_logbase2_ceil(resolve_op_num);
   uint32_t sample_cnt_bits = util_last_bit(util_logbase2(max_multisample));
         #if defined(DEBUG)
         #   define shift_hash(hash, num)   \
         do {                         \
      max_shift += (num);       \
   assert(max_shift <= 32U); \
            #else
   #   define shift_hash(hash, num) hash <<= (num)
   #endif
                  shift_hash(hash, layer_float_bits);
            shift_hash(hash, 1U);
            shift_hash(hash, 1U);
            shift_hash(hash, 1U);
            shift_hash(hash, pixel_src_bits);
            shift_hash(hash, resolve_op_bits);
            assert(util_is_power_of_two_nonzero(layer->sample_count));
   shift_hash(hash, sample_cnt_bits);
            shift_hash(hash, 1U);
            shift_hash(hash, byte_unwind_bits);
            shift_hash(hash, 1U);
                     shift_hash(hash, 1U);
            shift_hash(hash, 1U);
            shift_hash(hash, 1U);
            shift_hash(hash, num_layers_bits);
   /* Just 1 layer. */
            shift_hash(hash, 3U);
   /* alpha type none */
         #undef shift_hash
            }
      #define to_hash_table_key(_key) ((void *)(uintptr_t)(_key))
      static VkResult pvr_transfer_frag_store_entry_data_compile(
      struct pvr_device *device,
   struct pvr_transfer_frag_store_entry_data *const entry_data,
   const struct pvr_tq_shader_properties *shader_props,
      {
      const uint32_t image_desc_offset =
         const uint32_t sampler_desc_offset =
            const uint32_t cache_line_size =
            struct pvr_tq_frag_sh_reg_layout *sh_reg_layout = &entry_data->sh_reg_layout;
   uint32_t next_free_sh_reg = 0;
   struct util_dynarray shader;
            /* TODO: Allocate all combined image samplers if needed? Otherwise change the
   * array to a single descriptor.
   */
   sh_reg_layout->combined_image_samplers.offsets[0].image =
         sh_reg_layout->combined_image_samplers.offsets[0].sampler =
         sh_reg_layout->combined_image_samplers.count = 1;
            /* TODO: Handle dynamic_const_regs used for PVR_INT_COORD_SET_FLOATS_{4,6}, Z
   * position, texel unwind, etc. when compiler adds support for them.
   */
   sh_reg_layout->dynamic_consts.offset = next_free_sh_reg;
                     pvr_uscgen_tq_frag(shader_props,
                        result = pvr_gpu_upload_usc(device,
                           util_dynarray_fini(&shader);
   if (result != VK_SUCCESS)
               }
      static VkResult pvr_transfer_frag_store_entry_data_create(
      struct pvr_device *device,
   struct pvr_transfer_frag_store *store,
   const struct pvr_tq_shader_properties *shader_props,
      {
      struct pvr_pds_kickusc_program kick_usc_pds_prog = { 0 };
   struct pvr_transfer_frag_store_entry_data *entry_data;
   pvr_dev_addr_t dev_addr;
   uint32_t num_usc_temps;
            entry_data = ralloc(store->hash_table, __typeof__(*entry_data));
   if (!entry_data)
            result = pvr_transfer_frag_store_entry_data_compile(device,
                     if (result != VK_SUCCESS)
            dev_addr = entry_data->usc_upload->dev_addr;
            pvr_pds_setup_doutu(&kick_usc_pds_prog.usc_task_control,
                     dev_addr.addr,
   num_usc_temps,
                     result = pvr_bo_alloc(device,
                        device->heaps.pds_heap,
   PVR_DW_TO_BYTES(kick_usc_pds_prog.data_size +
      if (result != VK_SUCCESS)
            pvr_pds_kick_usc(&kick_usc_pds_prog,
                  entry_data->kick_usc_pds_upload->bo->map,
         dev_addr = entry_data->kick_usc_pds_upload->vma->dev_addr;
   dev_addr.addr -= device->heaps.pds_heap->base_addr.addr;
                           err_free_usc_upload:
            err_free_entry:
                  }
      static void inline pvr_transfer_frag_store_entry_data_destroy_no_ralloc_free(
      struct pvr_device *device,
      {
      pvr_bo_free(device, entry_data->kick_usc_pds_upload);
      }
      static void inline pvr_transfer_frag_store_entry_data_destroy(
      struct pvr_device *device,
      {
      pvr_transfer_frag_store_entry_data_destroy_no_ralloc_free(device,
         /* Casting away the const :( */
      }
      static VkResult pvr_transfer_frag_store_get_entry(
      struct pvr_device *device,
   struct pvr_transfer_frag_store *store,
   const struct pvr_tq_shader_properties *shader_props,
      {
      const uint32_t key =
         const struct hash_entry *entry;
            entry = _mesa_hash_table_search(store->hash_table, to_hash_table_key(key));
   if (!entry) {
      /* Init so that gcc stops complaining. */
            result = pvr_transfer_frag_store_entry_data_create(device,
                     if (result != VK_SUCCESS)
                     entry = _mesa_hash_table_insert(store->hash_table,
               if (!entry) {
      pvr_transfer_frag_store_entry_data_destroy(device, entry_data);
                              }
      VkResult pvr_transfer_frag_store_get_shader_info(
      struct pvr_device *device,
   struct pvr_transfer_frag_store *store,
   const struct pvr_tq_shader_properties *shader_props,
   pvr_dev_addr_t *const pds_dev_addr_out,
      {
      /* Init so that gcc stops complaining. */
   const struct pvr_transfer_frag_store_entry_data *entry_data = NULL;
            result = pvr_transfer_frag_store_get_entry(device,
                     if (result != VK_SUCCESS)
            *pds_dev_addr_out = entry_data->kick_usc_pds_offset;
               }
      void pvr_transfer_frag_store_fini(struct pvr_device *device,
         {
      hash_table_foreach_remove(store->hash_table, entry)
   {
      /* ralloc_free() in _mesa_hash_table_destroy() will free each entry's
   * memory so let's not waste extra time freeing them one by one and
   * unliking.
   */
   pvr_transfer_frag_store_entry_data_destroy_no_ralloc_free(
      device,
               }
