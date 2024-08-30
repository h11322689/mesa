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
   #include <vulkan/vulkan_core.h>
      #include "pvr_bo.h"
   #include "pvr_private.h"
   #include "pvr_robustness.h"
   #include "util/u_math.h"
      enum pvr_robustness_buffer_format {
      PVR_ROBUSTNESS_BUFFER_FORMAT_UINT64,
   PVR_ROBUSTNESS_BUFFER_FORMAT_UINT32,
   PVR_ROBUSTNESS_BUFFER_FORMAT_UINT16,
   PVR_ROBUSTNESS_BUFFER_FORMAT_UINT8,
   PVR_ROBUSTNESS_BUFFER_FORMAT_SINT64,
   PVR_ROBUSTNESS_BUFFER_FORMAT_SINT32,
   PVR_ROBUSTNESS_BUFFER_FORMAT_SINT16,
   PVR_ROBUSTNESS_BUFFER_FORMAT_SINT8,
   PVR_ROBUSTNESS_BUFFER_FORMAT_SFLOAT64,
   PVR_ROBUSTNESS_BUFFER_FORMAT_SFLOAT32,
   PVR_ROBUSTNESS_BUFFER_FORMAT_SFLOAT16,
   PVR_ROBUSTNESS_BUFFER_FORMAT_A8B8G8R8_UINT,
   PVR_ROBUSTNESS_BUFFER_FORMAT_A8B8G8R8_SINT,
   PVR_ROBUSTNESS_BUFFER_FORMAT_A2R10G10B10_UINT,
   PVR_ROBUSTNESS_BUFFER_FORMAT_A2R10G10B10_SINT,
   PVR_ROBUSTNESS_BUFFER_FORMAT_R4G4B4A4_UNORM,
   PVR_ROBUSTNESS_BUFFER_FORMAT_R5G5B5A1_UNORM,
   PVR_ROBUSTNESS_BUFFER_FORMAT_A1R5G5B5_UNORM,
      };
      /* Offsets in bytes of the [0, 0, 0, 1] vectors within the robustness buffer */
   static uint16_t robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_COUNT];
      VkResult pvr_init_robustness_buffer(struct pvr_device *device)
   {
      uint16_t offset = 0;
   uint8_t *robustness_buffer_map;
         #define ROBUSTNESS_BUFFER_OFFSET_ALIGN16(cur_offset, add) \
               robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_UINT64] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_UINT32] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_UINT16] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_UINT8] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_SINT64] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_SINT32] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_SINT16] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_SINT8] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_SFLOAT64] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_SFLOAT32] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_SFLOAT16] = offset;
            robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_A8B8G8R8_UINT] =
                  robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_A8B8G8R8_SINT] =
                  robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_A2R10G10B10_UINT] =
                  robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_A2R10G10B10_SINT] =
                  robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_R4G4B4A4_UNORM] =
                  robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_R5G5B5A1_UNORM] =
                  robustness_buffer_offsets[PVR_ROBUSTNESS_BUFFER_FORMAT_A1R5G5B5_UNORM] =
               #undef ROBUSTNESS_BUFFER_OFFSET_ALIGN16
         result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
                  #define ROBUSTNESS_BUFFER_RGBA(format, type, zero, one)                     \
      do {                                                                     \
      type *const buffer =                                                  \
         buffer[0] = (type)zero;                                               \
   buffer[1] = (type)zero;                                               \
   buffer[2] = (type)zero;                                               \
            #define ROBUSTNESS_BUFFER_ABGR(format, type, zero, one)                     \
      do {                                                                     \
      type *const buffer =                                                  \
         buffer[0] = (type)one;                                                \
   buffer[1] = (type)zero;                                               \
   buffer[2] = (type)zero;                                               \
            #define ROBUSTNESS_BUFFER_PACKED(format, type, val)                         \
      do {                                                                     \
      type *const buffer =                                                  \
                     ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_UINT64,
                     ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_UINT32,
                     ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_UINT16,
                     ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_UINT8,
                        ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_SINT64,
                     ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_SINT32,
                     ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_SINT16,
                     ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_SINT8,
                        ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_SFLOAT64,
                     ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_SFLOAT32,
                     ROBUSTNESS_BUFFER_RGBA(PVR_ROBUSTNESS_BUFFER_FORMAT_SFLOAT16,
                        ROBUSTNESS_BUFFER_ABGR(PVR_ROBUSTNESS_BUFFER_FORMAT_A8B8G8R8_UINT,
                     ROBUSTNESS_BUFFER_ABGR(PVR_ROBUSTNESS_BUFFER_FORMAT_A8B8G8R8_UINT,
                        ROBUSTNESS_BUFFER_PACKED(PVR_ROBUSTNESS_BUFFER_FORMAT_A2R10G10B10_UINT,
               ROBUSTNESS_BUFFER_PACKED(PVR_ROBUSTNESS_BUFFER_FORMAT_A2R10G10B10_SINT,
               ROBUSTNESS_BUFFER_PACKED(PVR_ROBUSTNESS_BUFFER_FORMAT_R4G4B4A4_UNORM,
               ROBUSTNESS_BUFFER_PACKED(PVR_ROBUSTNESS_BUFFER_FORMAT_R5G5B5A1_UNORM,
               ROBUSTNESS_BUFFER_PACKED(PVR_ROBUSTNESS_BUFFER_FORMAT_A1R5G5B5_UNORM,
               #undef ROBUSTNESS_BUFFER_RGBA
   #undef ROBUSTNESS_BUFFER_ABGR
   #undef ROBUSTNESS_BUFFER_PACKED
            }
      void pvr_robustness_buffer_finish(struct pvr_device *device)
   {
         }
      uint16_t pvr_get_robustness_buffer_format_offset(VkFormat format)
   {
      switch (format) {
   case VK_FORMAT_R64G64B64A64_SFLOAT:
            case VK_FORMAT_R32G32B32A32_SFLOAT:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
            case VK_FORMAT_R64G64B64A64_UINT:
            case VK_FORMAT_R32G32B32A32_UINT:
            case VK_FORMAT_R16G16B16A16_UNORM:
   case VK_FORMAT_R16G16B16A16_USCALED:
   case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R8G8B8A8_UNORM:
   case VK_FORMAT_R8G8B8A8_USCALED:
   case VK_FORMAT_R8G8B8A8_UINT:
   case VK_FORMAT_R8G8B8A8_SRGB:
   case VK_FORMAT_B8G8R8A8_UNORM:
   case VK_FORMAT_B8G8R8A8_USCALED:
   case VK_FORMAT_B8G8R8A8_UINT:
   case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_R64G64B64A64_SINT:
            case VK_FORMAT_R32G32B32A32_SINT:
            case VK_FORMAT_R16G16B16A16_SNORM:
   case VK_FORMAT_R16G16B16A16_SSCALED:
   case VK_FORMAT_R16G16B16A16_SINT:
            case VK_FORMAT_R8G8B8A8_SNORM:
   case VK_FORMAT_R8G8B8A8_SSCALED:
   case VK_FORMAT_R8G8B8A8_SINT:
   case VK_FORMAT_B8G8R8A8_SNORM:
   case VK_FORMAT_B8G8R8A8_SSCALED:
   case VK_FORMAT_B8G8R8A8_SINT:
            case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
   case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
   case VK_FORMAT_A8B8G8R8_UINT_PACK32:
   case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
      return robustness_buffer_offsets
         case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
   case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
   case VK_FORMAT_A8B8G8R8_SINT_PACK32:
      return robustness_buffer_offsets
         case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
   case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
   case VK_FORMAT_A2R10G10B10_UINT_PACK32:
   case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
   case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
   case VK_FORMAT_A2B10G10R10_UINT_PACK32:
      return robustness_buffer_offsets
         case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
   case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
   case VK_FORMAT_A2R10G10B10_SINT_PACK32:
   case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
   case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
   case VK_FORMAT_A2B10G10R10_SINT_PACK32:
      return robustness_buffer_offsets
         case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
   case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
      return robustness_buffer_offsets
         case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
   case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
      return robustness_buffer_offsets
         case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
      return robustness_buffer_offsets
         default:
            }
