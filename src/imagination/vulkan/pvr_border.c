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
   #include <stdbool.h>
   #include <stdint.h>
   #include <string.h>
   #include <vulkan/vulkan_core.h>
      #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_border.h"
   #include "pvr_device_info.h"
   #include "pvr_formats.h"
   #include "pvr_private.h"
   #include "util/bitset.h"
   #include "util/format/u_format.h"
   #include "util/format/u_formats.h"
   #include "util/log.h"
   #include "util/macros.h"
   #include "vk_format.h"
   #include "vk_log.h"
   #include "vk_sampler.h"
      struct pvr_border_color_table_value {
         } PACKED;
   static_assert(sizeof(struct pvr_border_color_table_value) ==
                  struct pvr_border_color_table_entry {
         } PACKED;
      static inline void pvr_border_color_table_pack_single(
      struct pvr_border_color_table_value *const dst,
   const union pipe_color_union *const color,
   const struct pvr_tex_format_description *const format,
      {
      const enum pipe_format pipe_format = is_int ? format->pipe_format_int
            if (pipe_format == PIPE_FORMAT_NONE)
                     if (util_format_is_depth_or_stencil(pipe_format)) {
      if (is_int) {
      const uint8_t s_color[4] = {
      color->ui[0],
   color->ui[1],
   color->ui[2],
                  } else {
            } else {
            }
      static inline void pvr_border_color_table_pack_single_compressed(
      struct pvr_border_color_table_value *const dst,
   const union pipe_color_union *const color,
   const struct pvr_tex_format_compressed_description *const format,
      {
      if (PVR_HAS_FEATURE(dev_info, tpu_border_colour_enhanced)) {
      const struct pvr_tex_format_description *format_simple =
            pvr_border_color_table_pack_single(dst, color, format_simple, false);
                        pvr_finishme("Devices without tpu_border_colour_enhanced require entries "
            }
      static int32_t
   pvr_border_color_table_alloc_entry(struct pvr_border_color_table *const table)
   {
      /* BITSET_FFS() returns a 1-indexed position or 0 if no bits are set. */
   int32_t index = BITSET_FFS(table->unused_entries);
   if (!index--)
                        }
      static void
   pvr_border_color_table_free_entry(struct pvr_border_color_table *const table,
         {
      assert(pvr_border_color_table_is_index_valid(table, index));
      }
      static void
   pvr_border_color_table_fill_entry(struct pvr_border_color_table *const table,
                           {
      struct pvr_border_color_table_entry *const entries = table->table->bo->map;
   struct pvr_border_color_table_entry *const entry = &entries[index];
            for (; tex_format < PVR_TEX_FORMAT_COUNT; tex_format++) {
      if (!pvr_tex_format_is_supported(tex_format))
            pvr_border_color_table_pack_single(
      &entry->values[tex_format],
   color,
   pvr_get_tex_format_description(tex_format),
            for (; tex_format < PVR_TEX_FORMAT_COUNT * 2; tex_format++) {
      if (!pvr_tex_format_compressed_is_supported(tex_format))
            pvr_border_color_table_pack_single_compressed(
      &entry->values[tex_format],
   color,
   pvr_get_tex_format_compressed_description(tex_format),
      }
      VkResult pvr_border_color_table_init(struct pvr_border_color_table *const table,
         {
      const struct pvr_device_info *const dev_info = &device->pdevice->dev_info;
   const uint32_t cache_line_size = rogue_get_slc_cache_line_size(dev_info);
   const uint32_t table_size = sizeof(struct pvr_border_color_table_entry) *
                     /* Initialize to ones so ffs can be used to find unused entries. */
            result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
            BITSET_CLEAR_RANGE_INSIDE_WORD(table->unused_entries,
                        for (uint32_t i = 0; i < PVR_BORDER_COLOR_TABLE_NR_BUILTIN_ENTRIES; i++) {
      const VkClearColorValue color = vk_border_color_value(i);
            pvr_border_color_table_fill_entry(table,
                                                err_out:
         }
      void pvr_border_color_table_finish(struct pvr_border_color_table *const table,
         {
   #if defined(DEBUG)
      BITSET_SET_RANGE_INSIDE_WORD(table->unused_entries,
               BITSET_NOT(table->unused_entries);
      #endif
            }
      VkResult pvr_border_color_table_get_or_create_entry(
      UNUSED struct pvr_border_color_table *const table,
   const struct pvr_sampler *const sampler,
      {
               if (vk_type <= PVR_BORDER_COLOR_TABLE_NR_BUILTIN_ENTRIES) {
      *index_out = vk_type;
               pvr_finishme("VK_EXT_custom_border_color is currently unsupported.");
      }
