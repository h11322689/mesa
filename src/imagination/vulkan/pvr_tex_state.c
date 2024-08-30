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
      #include <stdint.h>
   #include <vulkan/vulkan.h>
      #include "hwdef/rogue_hw_defs.h"
   #include "pvr_csb.h"
   #include "pvr_device_info.h"
   #include "pvr_formats.h"
   #include "pvr_private.h"
   #include "pvr_tex_state.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "vk_format.h"
   #include "vk_log.h"
      static enum ROGUE_TEXSTATE_SWIZ pvr_get_hw_swizzle(VkComponentSwizzle comp,
         {
      switch (swz) {
   case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
         case PIPE_SWIZZLE_X:
         case PIPE_SWIZZLE_Y:
         case PIPE_SWIZZLE_Z:
         case PIPE_SWIZZLE_W:
         case PIPE_SWIZZLE_NONE:
      if (comp == VK_COMPONENT_SWIZZLE_A)
         else
      default:
            }
      VkResult
   pvr_pack_tex_state(struct pvr_device *device,
               {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   enum pvr_memlayout mem_layout;
            if (info->type == VK_IMAGE_VIEW_TYPE_1D &&
      info->mem_layout == PVR_MEMLAYOUT_LINEAR) {
   /* Change the memory layout to twiddled as there isn't a TEXSTATE_TEXTYPE
   * for 1D linear and 1D twiddled is equivalent.
   */
      } else {
                  if (info->is_cube && info->tex_state_type != PVR_TEXTURE_STATE_SAMPLE)
         else
            pvr_csb_pack (&state[0], TEXSTATE_IMAGE_WORD0, word0) {
      if (mem_layout == PVR_MEMLAYOUT_LINEAR) {
      switch (iview_type) {
   case VK_IMAGE_VIEW_TYPE_2D:
   case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
                  default:
            } else if (mem_layout == PVR_MEMLAYOUT_TWIDDLED) {
      switch (iview_type) {
   case VK_IMAGE_VIEW_TYPE_1D:
   case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
                  case VK_IMAGE_VIEW_TYPE_2D:
   case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
                  case VK_IMAGE_VIEW_TYPE_CUBE:
   case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
                  default:
            } else if (mem_layout == PVR_MEMLAYOUT_3DTWIDDLED) {
      switch (iview_type) {
   case VK_IMAGE_VIEW_TYPE_3D:
                  default:
            } else {
                  /* When sampling from a combined D/S image, the TPU will default to only
   * the depth aspect.
   * The driver must select the correct single aspect format when sampling
   * to avoid this.
   */
   word0.texformat =
         word0.smpcnt = util_logbase2(info->sample_count);
   word0.swiz0 =
         word0.swiz1 =
         word0.swiz2 =
         word0.swiz3 =
            /* Gamma */
   if (vk_format_is_srgb(info->format)) {
      /* Gamma for 2 Component Formats has to be handled differently. */
   if (vk_format_get_nr_components(info->format) == 2) {
      /* Enable Gamma only for Channel 0 if Channel 1 is an Alpha
   * Channel.
   */
   if (vk_format_has_alpha(info->format)) {
         } else {
                     /* If Channel 0 happens to be the Alpha Channel, the
   * ALPHA_MSB bit would not be set thereby disabling Gamma
   * for Channel 0.
         } else {
                     word0.width = info->extent.width - 1;
   if (iview_type != VK_IMAGE_VIEW_TYPE_1D &&
      iview_type != VK_IMAGE_VIEW_TYPE_1D_ARRAY)
            if (mem_layout == PVR_MEMLAYOUT_LINEAR) {
      pvr_csb_pack (&state[1], TEXSTATE_STRIDE_IMAGE_WORD1, word1) {
      assert(info->stride > 0U);
   word1.stride = info->stride - 1U;
                                          if (!PVR_HAS_FEATURE(dev_info, tpu_extended_integer_lookup) &&
      !PVR_HAS_FEATURE(dev_info, tpu_image_state_v2)) {
   if (info->flags & PVR_TEXFLAGS_INDEX_LOOKUP ||
                                    if (PVR_HAS_FEATURE(dev_info, tpu_image_state_v2) &&
      vk_format_is_compressed(info->format))
   word1.tpu_image_state_v2_compression_mode =
      } else {
      pvr_csb_pack (&state[1], TEXSTATE_IMAGE_WORD1, word1) {
      word1.num_mip_levels = info->mip_levels;
                  if (iview_type == VK_IMAGE_VIEW_TYPE_3D) {
      if (info->extent.depth > 0)
                                       if (array_layers > 0)
                        if (!PVR_HAS_FEATURE(dev_info, tpu_extended_integer_lookup) &&
      !PVR_HAS_FEATURE(dev_info, tpu_image_state_v2)) {
   if (info->flags & PVR_TEXFLAGS_INDEX_LOOKUP ||
                                                                  if (PVR_HAS_FEATURE(dev_info, tpu_image_state_v2) &&
      vk_format_is_compressed(info->format))
   word1.tpu_image_state_v2_compression_mode =
                  }
