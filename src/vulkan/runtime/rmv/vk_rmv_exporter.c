   /*
   * Copyright © 2022 Friedrich Vock
   *
   * Exporter based on Radeon Memory Visualizer code which is
   *
   * Copyright (c) 2017-2022 Advanced Micro Devices, Inc.
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
      #include "vk_rmv_common.h"
   #include "vk_rmv_tokens.h"
      #include "util/format/u_format.h"
   #include "util/u_process.h"
   #include "vulkan/util/vk_format.h"
      static int
   vk_rmv_token_compare(const void *first, const void *second)
   {
      const struct vk_rmv_token *first_token = (struct vk_rmv_token *)first;
   const struct vk_rmv_token *second_token = (struct vk_rmv_token *)second;
   if (first_token->timestamp < second_token->timestamp)
         else if (first_token->timestamp > second_token->timestamp)
            }
      enum rmt_format {
      RMT_FORMAT_UNDEFINED,
   RMT_FORMAT_R1_UNORM,
   RMT_FORMAT_R1_USCALED,
   RMT_FORMAT_R4G4_UNORM,
   RMT_FORMAT_R4G4_USCALED,
   RMT_FORMAT_L4A4_UNORM,
   RMT_FORMAT_R4G4B4A4_UNORM,
   RMT_FORMAT_R4G4B4A4_USCALED,
   RMT_FORMAT_R5G6B5_UNORM,
   RMT_FORMAT_R5G6B5_USCALED,
   RMT_FORMAT_R5G5B5A1_UNORM,
   RMT_FORMAT_R5G5B5A1_USCALED,
   RMT_FORMAT_R1G5B5A5_UNORM,
   RMT_FORMAT_R1G5B5A5_USCALED,
   RMT_FORMAT_R8_XNORM,
   RMT_FORMAT_R8_SNORM,
   RMT_FORMAT_R8_USCALED,
   RMT_FORMAT_R8_SSCALED,
   RMT_FORMAT_R8_UINT,
   RMT_FORMAT_R8_SINT,
   RMT_FORMAT_R8_SRGB,
   RMT_FORMAT_A8_UNORM,
   RMT_FORMAT_L8_UNORM,
   RMT_FORMAT_P8_UINT,
   RMT_FORMAT_R8G8_UNORM,
   RMT_FORMAT_R8G8_SNORM,
   RMT_FORMAT_R8G8_USCALED,
   RMT_FORMAT_R8G8_SSCALED,
   RMT_FORMAT_R8G8_UINT,
   RMT_FORMAT_R8G8_SINT,
   RMT_FORMAT_R8G8_SRGB,
   RMT_FORMAT_L8A8_UNORM,
   RMT_FORMAT_R8G8B8A8_UNORM,
   RMT_FORMAT_R8G8B8A8_SNORM,
   RMT_FORMAT_R8G8B8A8_USCALED,
   RMT_FORMAT_R8G8B8A8_SSCALED,
   RMT_FORMAT_R8G8B8A8_UINT,
   RMT_FORMAT_R8G8B8A8_SINT,
   RMT_FORMAT_R8G8B8A8_SRGB,
   RMT_FORMAT_U8V8_SNORM_L8W8_UNORM,
   RMT_FORMAT_R10G11B11_FLOAT,
   RMT_FORMAT_R11G11B10_FLOAT,
   RMT_FORMAT_R10G10B10A2_UNORM,
   RMT_FORMAT_R10G10B10A2_SNORM,
   RMT_FORMAT_R10G10B10A2_USCALED,
   RMT_FORMAT_R10G10B10A2_SSCALED,
   RMT_FORMAT_R10G10B10A2_UINT,
   RMT_FORMAT_R10G10B10A2_SINT,
   RMT_FORMAT_R10G10B10A2_BIAS_UNORM,
   RMT_FORMAT_U10V10W10_SNORMA2_UNORM,
   RMT_FORMAT_R16_UNORM,
   RMT_FORMAT_R16_SNORM,
   RMT_FORMAT_R16_USCALED,
   RMT_FORMAT_R16_SSCALED,
   RMT_FORMAT_R16_UINT,
   RMT_FORMAT_R16_SINT,
   RMT_FORMAT_R16_FLOAT,
   RMT_FORMAT_L16_UNORM,
   RMT_FORMAT_R16G16_UNORM,
   RMT_FORMAT_R16G16_SNORM,
   RMT_FORMAT_R16G16_USCALED,
   RMT_FORMAT_R16G16_SSCALED,
   RMT_FORMAT_R16G16_UINT,
   RMT_FORMAT_R16G16_SINT,
   RMT_FORMAT_R16G16_FLOAT,
   RMT_FORMAT_R16G16B16A16_UNORM,
   RMT_FORMAT_R16G16B16A16_SNORM,
   RMT_FORMAT_R16G16B16A16_USCALED,
   RMT_FORMAT_R16G16B16A16_SSCALED,
   RMT_FORMAT_R16G16B16A16_UINT,
   RMT_FORMAT_R16G16B16A16_SINT,
   RMT_FORMAT_R16G16B16A16_FLOAT,
   RMT_FORMAT_R32_UINT,
   RMT_FORMAT_R32_SINT,
   RMT_FORMAT_R32_FLOAT,
   RMT_FORMAT_R32G32_UINT,
   RMT_FORMAT_R32G32_SINT,
   RMT_FORMAT_R32G32_FLOAT,
   RMT_FORMAT_R32G32B32_UINT,
   RMT_FORMAT_R32G32B32_SINT,
   RMT_FORMAT_R32G32B32_FLOAT,
   RMT_FORMAT_R32G32B32A32_UINT,
   RMT_FORMAT_R32G32B32A32_SINT,
   RMT_FORMAT_R32G32B32A32_FLOAT,
   RMT_FORMAT_D16_UNORM_S8_UINT,
   RMT_FORMAT_D32_UNORM_S8_UINT,
   RMT_FORMAT_R9G9B9E5_FLOAT,
   RMT_FORMAT_BC1_UNORM,
   RMT_FORMAT_BC1_SRGB,
   RMT_FORMAT_BC2_UNORM,
   RMT_FORMAT_BC2_SRGB,
   RMT_FORMAT_BC3_UNORM,
   RMT_FORMAT_BC3_SRGB,
   RMT_FORMAT_BC4_UNORM,
   RMT_FORMAT_BC4_SRGB,
   RMT_FORMAT_BC5_UNORM,
   RMT_FORMAT_BC5_SRGB,
   RMT_FORMAT_BC6_UNORM,
   RMT_FORMAT_BC6_SRGB,
   RMT_FORMAT_BC7_UNORM,
   RMT_FORMAT_BC7_SRGB,
   RMT_FORMAT_ETC2_R8G8B8_UNORM,
   RMT_FORMAT_ETC2_R8G8B8_SRGB,
   RMT_FORMAT_ETC2_R8G8B8A1_UNORM,
   RMT_FORMAT_ETC2_R8G8B8A1_SRGB,
   RMT_FORMAT_ETC2_R8G8B8A8_UNORM,
   RMT_FORMAT_ETC2_R8G8B8A8_SRGB,
   RMT_FORMAT_ETC2_R11_UNORM,
   RMT_FORMAT_ETC2_R11_SNORM,
   RMT_FORMAT_ETC2_R11G11_UNORM,
   RMT_FORMAT_ETC2_R11G11_SNORM,
   RMT_FORMAT_ASTCLD_R4X4_UNORM,
   RMT_FORMAT_ASTCLD_R4X4_SRGB,
   RMT_FORMAT_ASTCLD_R5X4_UNORM,
   RMT_FORMAT_ASTCLD_R5X4_SRGB,
   RMT_FORMAT_ASTCLD_R5X5_UNORM,
   RMT_FORMAT_ASTCLD_R5X5_SRGB,
   RMT_FORMAT_ASTCLD_R6X5_UNORM,
   RMT_FORMAT_ASTCLD_R6X5_SRGB,
   RMT_FORMAT_ASTCLD_R6X6_UNORM,
   RMT_FORMAT_ASTCLD_R6X6_SRGB,
   RMT_FORMAT_ASTCLD_R8X5_UNORM,
   RMT_FORMAT_ASTCLD_R8X5_SRGB,
   RMT_FORMAT_ASTCLD_R8X6_UNORM,
   RMT_FORMAT_ASTCLD_R8X6_SRGB,
   RMT_FORMAT_ASTCLD_R8X8_UNORM,
   RMT_FORMAT_ASTCLD_R8X8_SRGB,
   RMT_FORMAT_ASTCLD_R10X5_UNORM,
   RMT_FORMAT_ASTCLD_R10X5_SRGB,
   RMT_FORMAT_ASTCLD_R10X6_UNORM,
   RMT_FORMAT_ASTCLD_R10X6_SRGB,
   RMT_FORMAT_ASTCLD_R10X8_UNORM,
   RMT_FORMAT_ASTCLD_R10X10_UNORM,
   RMT_FORMAT_ASTCLD_R12X10_UNORM,
   RMT_FORMAT_ASTCLD_R12X10_SRGB,
   RMT_FORMAT_ASTCLD_R12X12_UNORM,
   RMT_FORMAT_ASTCLD_R12X12_SRGB,
   RMT_FORMAT_ASTCHD_R4x4_FLOAT,
   RMT_FORMAT_ASTCHD_R5x4_FLOAT,
   RMT_FORMAT_ASTCHD_R5x5_FLOAT,
   RMT_FORMAT_ASTCHD_R6x5_FLOAT,
   RMT_FORMAT_ASTCHD_R6x6_FLOAT,
   RMT_FORMAT_ASTCHD_R8x5_FLOAT,
   RMT_FORMAT_ASTCHD_R8x6_FLOAT,
   RMT_FORMAT_ASTCHD_R8x8_FLOAT,
   RMT_FORMAT_ASTCHD_R10x5_FLOAT,
   RMT_FORMAT_ASTCHD_R10x6_FLOAT,
   RMT_FORMAT_ASTCHD_R10x8_FLOAT,
   RMT_FORMAT_ASTCHD_R10x10_FLOAT,
   RMT_FORMAT_ASTCHD_R12x10_FLOAT,
   RMT_FORMAT_ASTCHD_R12x12_FLOAT,
   RMT_FORMAT_R8G8B8G8_UNORM,
   RMT_FORMAT_R8G8B8G8_USCALED,
   RMT_FORMAT_G8R8G8B8_UNORM,
   RMT_FORMAT_G8R8G8B8_USCALED,
   RMT_FORMAT_AYUV,
   RMT_FORMAT_UYVY,
   RMT_FORMAT_VYUY,
   RMT_FORMAT_YUY2,
   RMT_FORMAT_YVY2,
   RMT_FORMAT_YV12,
   RMT_FORMAT_NV11,
   RMT_FORMAT_NV12,
   RMT_FORMAT_NV21,
   RMT_FORMAT_P016,
      };
      enum rmt_swizzle {
      RMT_SWIZZLE_ZERO,
   RMT_SWIZZLE_ONE,
   RMT_SWIZZLE_R,
   RMT_SWIZZLE_G,
   RMT_SWIZZLE_B,
      };
      static inline enum rmt_format
   vk_to_rmt_format(VkFormat format)
   {
      switch (format) {
   case VK_FORMAT_R8_UNORM:
         case VK_FORMAT_R8_SNORM:
         case VK_FORMAT_R8_USCALED:
         case VK_FORMAT_R8_SSCALED:
         case VK_FORMAT_R8_UINT:
         case VK_FORMAT_R8_SINT:
         case VK_FORMAT_R8_SRGB:
         case VK_FORMAT_R8G8_UNORM:
         case VK_FORMAT_R8G8_SNORM:
         case VK_FORMAT_R8G8_USCALED:
         case VK_FORMAT_R8G8_SSCALED:
         case VK_FORMAT_R8G8_UINT:
         case VK_FORMAT_R8G8_SINT:
         case VK_FORMAT_R8G8_SRGB:
         case VK_FORMAT_R8G8B8A8_UNORM:
   case VK_FORMAT_B8G8R8A8_UNORM:
   case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
         case VK_FORMAT_R8G8B8A8_SNORM:
   case VK_FORMAT_B8G8R8A8_SNORM:
   case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
         case VK_FORMAT_R8G8B8A8_USCALED:
   case VK_FORMAT_B8G8R8A8_USCALED:
   case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
         case VK_FORMAT_R8G8B8A8_SSCALED:
   case VK_FORMAT_B8G8R8A8_SSCALED:
   case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
         case VK_FORMAT_R8G8B8A8_UINT:
   case VK_FORMAT_B8G8R8A8_UINT:
   case VK_FORMAT_A8B8G8R8_UINT_PACK32:
         case VK_FORMAT_R8G8B8A8_SINT:
   case VK_FORMAT_B8G8R8A8_SINT:
   case VK_FORMAT_A8B8G8R8_SINT_PACK32:
         case VK_FORMAT_R8G8B8A8_SRGB:
   case VK_FORMAT_B8G8R8A8_SRGB:
   case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
         case VK_FORMAT_R16_UNORM:
         case VK_FORMAT_R16_SNORM:
         case VK_FORMAT_R16_USCALED:
         case VK_FORMAT_R16_SSCALED:
         case VK_FORMAT_R16_UINT:
         case VK_FORMAT_R16_SINT:
         case VK_FORMAT_R16G16_UNORM:
         case VK_FORMAT_R16G16_SNORM:
         case VK_FORMAT_R16G16_USCALED:
         case VK_FORMAT_R16G16_SSCALED:
         case VK_FORMAT_R16G16_UINT:
         case VK_FORMAT_R16G16_SINT:
         case VK_FORMAT_R16G16_SFLOAT:
         case VK_FORMAT_R16G16B16A16_UNORM:
         case VK_FORMAT_R16G16B16A16_SNORM:
         case VK_FORMAT_R16G16B16A16_USCALED:
         case VK_FORMAT_R16G16B16A16_SSCALED:
         case VK_FORMAT_R16G16B16A16_UINT:
         case VK_FORMAT_R16G16B16A16_SINT:
         case VK_FORMAT_R16G16B16A16_SFLOAT:
         case VK_FORMAT_R32_UINT:
         case VK_FORMAT_R32_SINT:
         case VK_FORMAT_R32_SFLOAT:
         case VK_FORMAT_R32G32_UINT:
         case VK_FORMAT_R32G32_SINT:
         case VK_FORMAT_R32G32_SFLOAT:
         case VK_FORMAT_R32G32B32_UINT:
         case VK_FORMAT_R32G32B32_SINT:
         case VK_FORMAT_R32G32B32_SFLOAT:
         case VK_FORMAT_R32G32B32A32_UINT:
         case VK_FORMAT_R32G32B32A32_SINT:
         case VK_FORMAT_R32G32B32A32_SFLOAT:
         case VK_FORMAT_D16_UNORM_S8_UINT:
         case VK_FORMAT_D32_SFLOAT_S8_UINT:
         case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
         case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
         case VK_FORMAT_BC2_UNORM_BLOCK:
         case VK_FORMAT_BC2_SRGB_BLOCK:
         case VK_FORMAT_BC3_UNORM_BLOCK:
         case VK_FORMAT_BC3_SRGB_BLOCK:
         case VK_FORMAT_BC4_UNORM_BLOCK:
         case VK_FORMAT_BC5_UNORM_BLOCK:
         case VK_FORMAT_BC7_UNORM_BLOCK:
         case VK_FORMAT_BC7_SRGB_BLOCK:
         case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
         case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
         case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
         case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
         case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
         case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
         case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
         case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
         case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
         case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
         case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
         case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
         case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
         case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
         case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
         case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
         case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
         case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
         case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
         case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
         case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
         case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
         case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
         case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
         case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
         case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
         case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
         case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
         case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
         case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
         case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
         case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
         case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
         case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
         default:
            }
      static void
   rmt_format_to_swizzle(VkFormat format, enum rmt_swizzle *swizzles)
   {
      const struct util_format_description *description =
         for (unsigned i = 0; i < 4; ++i) {
      switch (description->swizzle[i]) {
   case PIPE_SWIZZLE_X:
      swizzles[i] = RMT_SWIZZLE_R;
      case PIPE_SWIZZLE_Y:
      swizzles[i] = RMT_SWIZZLE_G;
      case PIPE_SWIZZLE_Z:
      swizzles[i] = RMT_SWIZZLE_B;
      case PIPE_SWIZZLE_W:
      swizzles[i] = RMT_SWIZZLE_A;
      case PIPE_SWIZZLE_0:
   case PIPE_SWIZZLE_NONE:
      swizzles[i] = RMT_SWIZZLE_ZERO;
      case PIPE_SWIZZLE_1:
      swizzles[i] = RMT_SWIZZLE_ONE;
            }
      #define RMT_FILE_MAGIC_NUMBER          0x494e494d
   #define RMT_FILE_VERSION_MAJOR         1
   #define RMT_FILE_VERSION_MINOR         0
   #define RMT_FILE_ADAPTER_NAME_MAX_SIZE 128
      enum rmt_heap_type {
      RMT_HEAP_TYPE_LOCAL,     /* DEVICE_LOCAL | HOST_VISIBLE */
   RMT_HEAP_TYPE_INVISIBLE, /* DEVICE_LOCAL */
   RMT_HEAP_TYPE_SYSTEM,    /* HOST_VISIBLE | HOST_COHERENT */
   RMT_HEAP_TYPE_NONE,
      };
      enum rmt_file_chunk_type {
      RMT_FILE_CHUNK_TYPE_ASIC_INFO, /* Seems to be unused in RMV */
   RMT_FILE_CHUNK_TYPE_API_INFO,
   RMT_FILE_CHUNK_TYPE_SYSTEM_INFO,
   RMT_FILE_CHUNK_TYPE_RMT_DATA,
   RMT_FILE_CHUNK_TYPE_SEGMENT_INFO,
   RMT_FILE_CHUNK_TYPE_PROCESS_START,
   RMT_FILE_CHUNK_TYPE_SNAPSHOT_INFO,
      };
      /**
   * RMT API info.
   */
   enum rmt_api_type {
      RMT_API_TYPE_DIRECTX_12,
   RMT_API_TYPE_VULKAN,
   RMT_API_TYPE_GENERIC,
      };
      struct rmt_file_chunk_id {
      enum rmt_file_chunk_type type : 8;
   int32_t index : 8;
      };
      struct rmt_file_chunk_header {
      struct rmt_file_chunk_id chunk_id;
   uint16_t minor_version;
   uint16_t major_version;
   int32_t size_in_bytes;
      };
      struct rmt_file_header_flags {
      union {
      struct {
                        };
      struct rmt_file_header {
      uint32_t magic_number;
   uint32_t version_major;
   uint32_t version_minor;
   struct rmt_file_header_flags flags;
   int32_t chunk_offset;
   int32_t second;
   int32_t minute;
   int32_t hour;
   int32_t day_in_month;
   int32_t month;
   int32_t year;
   int32_t day_in_week;
   int32_t day_in_year;
      };
      static_assert(sizeof(struct rmt_file_header) == 56, "rmt_file_header doesn't match RMV spec");
      static void
   rmt_fill_header(struct rmt_file_header *header)
   {
      struct tm *timep, result;
            header->magic_number = RMT_FILE_MAGIC_NUMBER;
   header->version_major = RMT_FILE_VERSION_MAJOR;
   header->version_minor = RMT_FILE_VERSION_MINOR;
   header->flags.value = 0;
            time(&raw_time);
            header->second = timep->tm_sec;
   header->minute = timep->tm_min;
   header->hour = timep->tm_hour;
   header->day_in_month = timep->tm_mday;
   header->month = timep->tm_mon;
   header->year = timep->tm_year;
   header->day_in_week = timep->tm_wday;
   header->day_in_year = timep->tm_yday;
      }
      /*
   * RMT data.
   */
   struct rmt_file_chunk_rmt_data {
      struct rmt_file_chunk_header header;
   uint64_t process_id;
      };
      static_assert(sizeof(struct rmt_file_chunk_rmt_data) == 32,
            static void
   rmt_fill_chunk_rmt_data(size_t token_stream_size, struct rmt_file_chunk_rmt_data *chunk)
   {
      chunk->header.chunk_id.type = RMT_FILE_CHUNK_TYPE_RMT_DATA;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 1;
   chunk->header.minor_version = 6;
               }
      /*
   * RMT System info. Equivalent to SQTT CPU info.
   */
   struct rmt_file_chunk_system_info {
      struct rmt_file_chunk_header header;
   uint32_t vendor_id[4];
   uint32_t processor_brand[12];
   uint32_t reserved[2];
   uint64_t cpu_timestamp_freq;
   uint32_t clock_speed;
   uint32_t num_logical_cores;
   uint32_t num_physical_cores;
      };
      static_assert(sizeof(struct rmt_file_chunk_system_info) == 112,
            /* same as vk_sqtt_fill_cpu_info. TODO: Share with ac_rgp.c */
   static void
   rmt_fill_chunk_system_info(struct rmt_file_chunk_system_info *chunk)
   {
      uint32_t cpu_clock_speed_total = 0;
   uint64_t system_ram_size = 0;
   char line[1024];
            chunk->header.chunk_id.type = RMT_FILE_CHUNK_TYPE_SYSTEM_INFO;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 0;
            /* For some reason, RMV allocates scratch data based on the
   * maximum timestamp in clock ticks. A tick of 1ns produces extremely
   * large timestamps, which causes RMV to run out of memory. Therefore,
   * all timestamps are translated as if the clock ran at 1 MHz. */
            strncpy((char *)chunk->vendor_id, "Unknown", sizeof(chunk->vendor_id));
   strncpy((char *)chunk->processor_brand, "Unknown", sizeof(chunk->processor_brand));
   chunk->clock_speed = 0;
   chunk->num_logical_cores = 0;
   chunk->num_physical_cores = 0;
   chunk->system_ram_size = 0;
   if (os_get_total_physical_memory(&system_ram_size))
            /* Parse cpuinfo to get more detailled information. */
   f = fopen("/proc/cpuinfo", "r");
   if (!f)
            while (fgets(line, sizeof(line), f)) {
               /* Parse vendor name. */
   str = strstr(line, "vendor_id");
   if (str) {
      char *ptr = (char *)chunk->vendor_id;
   char *v = strtok(str, ":");
   v = strtok(NULL, ":");
   strncpy(ptr, v + 1, sizeof(chunk->vendor_id) - 1);
               /* Parse processor name. */
   str = strstr(line, "model name");
   if (str) {
      char *ptr = (char *)chunk->processor_brand;
   char *v = strtok(str, ":");
   v = strtok(NULL, ":");
   strncpy(ptr, v + 1, sizeof(chunk->processor_brand) - 1);
               /* Parse the current CPU clock speed for each cores. */
   str = strstr(line, "cpu MHz");
   if (str) {
      uint32_t v = 0;
   if (sscanf(str, "cpu MHz : %d", &v) == 1)
               /* Parse the number of logical cores. */
   str = strstr(line, "siblings");
   if (str) {
      uint32_t v = 0;
   if (sscanf(str, "siblings : %d", &v) == 1)
               /* Parse the number of physical cores. */
   str = strstr(line, "cpu cores");
   if (str) {
      uint32_t v = 0;
   if (sscanf(str, "cpu cores : %d", &v) == 1)
                  if (chunk->num_logical_cores)
               }
      /*
   * RMT Segment info.
   */
   struct rmt_file_chunk_segment_info {
      struct rmt_file_chunk_header header;
   uint64_t base_address;
   uint64_t size;
   enum rmt_heap_type heap_type;
      };
      static_assert(sizeof(struct rmt_file_chunk_segment_info) == 40,
            static void
   rmt_fill_chunk_segment_info(struct vk_memory_trace_data *data, struct vk_rmv_device_info *info,
         {
      chunk->header.chunk_id.type = RMT_FILE_CHUNK_TYPE_SEGMENT_INFO;
   chunk->header.chunk_id.index = index;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 0;
            chunk->memory_index = index;
   chunk->heap_type = (enum rmt_heap_type)index;
   chunk->base_address = info->memory_infos[index].physical_base_address;
      }
      /*
   * RMT PCIe adapter info
   */
   struct rmt_file_chunk_adapter_info {
      struct rmt_file_chunk_header header;
   char name[RMT_FILE_ADAPTER_NAME_MAX_SIZE];
   uint32_t pcie_family_id;
   uint32_t pcie_revision_id;
   uint32_t device_id;
   uint32_t minimum_engine_clock;
   uint32_t maximum_engine_clock;
   uint32_t memory_type;
   uint32_t memory_operations_per_clock;
   uint32_t memory_bus_width;
   uint32_t memory_bandwidth;
   uint32_t minimum_memory_clock;
      };
      static_assert(sizeof(struct rmt_file_chunk_adapter_info) == 188,
            static void
   rmt_fill_chunk_adapter_info(struct vk_rmv_device_info *info,
         {
      chunk->header.chunk_id.type = RMT_FILE_CHUNK_TYPE_ADAPTER_INFO;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 0;
            memcpy(chunk->name, info->device_name, RMT_FILE_ADAPTER_NAME_MAX_SIZE);
   chunk->pcie_family_id = info->pcie_family_id;
   chunk->pcie_revision_id = info->pcie_revision_id;
   chunk->device_id = info->pcie_device_id;
   chunk->minimum_engine_clock = info->minimum_shader_clock;
   chunk->maximum_engine_clock = info->maximum_shader_clock;
   chunk->memory_type = info->vram_type;
            chunk->memory_bus_width = info->vram_bus_width;
   chunk->minimum_memory_clock = info->minimum_memory_clock;
   chunk->maximum_memory_clock = info->maximum_memory_clock;
   /* Convert bandwidth from GB/s to MiB/s */
   chunk->memory_bandwidth =
      }
      /*
   * RMT snapshot info
   */
   struct rmt_file_chunk_snapshot_info {
      struct rmt_file_chunk_header header;
   uint64_t snapshot_time;
   int32_t name_length;
   int32_t padding;
   /* The name follows after this struct */
      };
      static_assert(sizeof(struct rmt_file_chunk_snapshot_info) == 32,
            static void
   rmt_fill_chunk_snapshot_info(uint64_t timestamp, int32_t name_length,
         {
      chunk->header.chunk_id.type = RMT_FILE_CHUNK_TYPE_SNAPSHOT_INFO;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 1;
   chunk->header.minor_version = 6;
            chunk->snapshot_time = timestamp;
      }
      /*
   * RMT stream tokens
   */
      enum rmt_token_type {
      RMT_TOKEN_TYPE_TIMESTAMP,
   RMT_TOKEN_TYPE_RESERVED0,
   RMT_TOKEN_TYPE_RESERVED1,
   RMT_TOKEN_TYPE_PAGE_TABLE_UPDATE,
   RMT_TOKEN_TYPE_USERDATA,
   RMT_TOKEN_TYPE_MISC,
   RMT_TOKEN_TYPE_RESOURCE_REFERENCE,
   RMT_TOKEN_TYPE_RESOURCE_BIND,
   RMT_TOKEN_TYPE_PROCESS_EVENT,
   RMT_TOKEN_TYPE_PAGE_REFERENCE,
   RMT_TOKEN_TYPE_CPU_MAP,
   RMT_TOKEN_TYPE_VIRTUAL_FREE,
   RMT_TOKEN_TYPE_VIRTUAL_ALLOCATE,
   RMT_TOKEN_TYPE_RESOURCE_CREATE,
   RMT_TOKEN_TYPE_TIME_DELTA,
      };
      static enum rmt_token_type
   token_type_to_rmt(enum vk_rmv_token_type type)
   {
      switch (type) {
   case VK_RMV_TOKEN_TYPE_PAGE_TABLE_UPDATE:
         case VK_RMV_TOKEN_TYPE_USERDATA:
         case VK_RMV_TOKEN_TYPE_MISC:
         case VK_RMV_TOKEN_TYPE_RESOURCE_REFERENCE:
         case VK_RMV_TOKEN_TYPE_RESOURCE_BIND:
         case VK_RMV_TOKEN_TYPE_CPU_MAP:
         case VK_RMV_TOKEN_TYPE_VIRTUAL_FREE:
         case VK_RMV_TOKEN_TYPE_VIRTUAL_ALLOCATE:
         case VK_RMV_TOKEN_TYPE_RESOURCE_CREATE:
         case VK_RMV_TOKEN_TYPE_RESOURCE_DESTROY:
         default:
            }
      enum rmt_descriptor_type {
      RMT_DESCRIPTOR_TYPE_CSV_SRV_UAV,
   RMT_DESCRIPTOR_TYPE_SAMPLER,
   RMT_DESCRIPTOR_TYPE_RTV,
   RMT_DESCRIPTOR_TYPE_DSV,
   RMT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
   RMT_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   RMT_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   RMT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
   RMT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
   RMT_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
   RMT_DESCRIPTOR_TYPE_STORAGE_BUFFER,
   RMT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
   RMT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
   RMT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
   RMT_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
   RMT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE,
      };
      static enum rmt_descriptor_type
   vk_to_rmt_descriptor_type(VkDescriptorType type)
   {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
         case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
         case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
         case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
         case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
         case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
         case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
         case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
         case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
         case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
         case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
         default:
      /* This is reachable, error should be handled by caller */
         };
      static uint32_t
   rmt_valid_pool_size_count(struct vk_rmv_descriptor_pool_description *description)
   {
      uint32_t count = 0;
   for (uint32_t i = 0; i < description->pool_size_count; ++i) {
      enum rmt_descriptor_type rmt_type =
         if (rmt_type == RMT_DESCRIPTOR_TYPE_INVALID)
      /* Unknown descriptor type, skip */
         }
      }
      enum rmt_resource_owner_type {
      RMT_RESOURCE_OWNER_TYPE_APPLICATION,
   RMT_RESOURCE_OWNER_TYPE_PAL,
   RMT_RESOURCE_OWNER_TYPE_CLIENT_DRIVER,
      };
      static void
   rmt_file_write_bits(uint64_t *dst, uint64_t data, unsigned first_bit, unsigned last_bit)
   {
      unsigned index = first_bit / 64;
            /* Data crosses an uint64_t boundary, split */
   if (index != last_bit / 64) {
      unsigned first_part_size = 64 - shift;
   rmt_file_write_bits(dst, data & ((1ULL << first_part_size) - 1ULL), first_bit,
            } else {
      assert(data <= (1ULL << (uint64_t)(last_bit - first_bit + 1ULL)) - 1ULL);
         }
      static void
   rmt_file_write_token_bits(uint64_t *dst, uint64_t data, unsigned first_bit, unsigned last_bit)
   {
         }
      static enum rmt_heap_type
   rmt_file_domain_to_heap_type(enum vk_rmv_kernel_memory_domain domain, bool has_cpu_access)
   {
      switch (domain) {
   case VK_RMV_KERNEL_MEMORY_DOMAIN_CPU:
   case VK_RMV_KERNEL_MEMORY_DOMAIN_GTT:
         case VK_RMV_KERNEL_MEMORY_DOMAIN_VRAM:
         default:
            }
      /*
   * Write helpers for stream tokens
   */
      /* The timestamp frequency, in clock units / second.
   * Currently set to 1MHz. */
   #define RMT_TIMESTAMP_FREQUENCY (1 * 1000000)
   /* Factor needed to convert nanosecond timestamps as returned by os_get_time_nano
   * to RMV timestamps */
   #define RMT_TIMESTAMP_DIVISOR (1000000000L / RMT_TIMESTAMP_FREQUENCY)
      static void
   rmt_dump_timestamp(struct vk_rmv_timestamp_token *token, FILE *output)
   {
      uint64_t data[2] = {0};
   rmt_file_write_bits(data, RMT_TOKEN_TYPE_TIMESTAMP, 0, 3);
   /* RMT stores clock ticks divided by 32 */
   rmt_file_write_bits(data, token->value / 32, 4, 63);
   rmt_file_write_bits(data, RMT_TIMESTAMP_FREQUENCY, 64, 89);
      }
      static void
   rmt_dump_time_delta(uint64_t delta, FILE *output)
   {
      uint64_t data = 0;
   rmt_file_write_bits(&data, RMT_TOKEN_TYPE_TIME_DELTA, 0, 3);
   rmt_file_write_bits(&data, 7, 4, 7); /* no. of delta bytes */
   rmt_file_write_bits(&data, delta, 8, 63);
      }
      static void
   rmt_dump_event_resource(struct vk_rmv_event_description *description, FILE *output)
   {
      /* 8 bits of flags are the only thing in the payload */
      }
      static void
   rmt_dump_border_color_palette_resource(struct vk_rmv_border_color_palette_description *description,
         {
      /* no. of entries is the only thing in the payload */
      }
      enum rmt_page_size {
      RMT_PAGE_SIZE_UNMAPPED,
   RMT_PAGE_SIZE_4_KB,
   RMT_PAGE_SIZE_64_KB,
   RMT_PAGE_SIZE_256_KB,
   RMT_PAGE_SIZE_1_MB,
      };
      static enum rmt_page_size
   rmt_size_to_page_size(uint32_t size)
   {
      switch (size) {
   case 4096:
         case 65536:
         case 262144:
         case 1048576:
         case 2097152:
         default:
            }
      static void
   rmt_dump_heap_resource(struct vk_rmv_heap_description *description, FILE *output)
   {
      uint64_t data[2] = {0};
   rmt_file_write_bits(data, description->alloc_flags, 0, 3);
   rmt_file_write_bits(data, description->size, 4, 68);
   rmt_file_write_bits(data, rmt_size_to_page_size(description->alignment), 69, 73);
   rmt_file_write_bits(data, description->heap_index, 74, 77);
      }
      enum rmt_buffer_usage_flags {
      RMT_BUFFER_USAGE_FLAGS_TRANSFER_SOURCE = 1 << 0,
   RMT_BUFFER_USAGE_FLAGS_TRANSFER_DESTINATION = 1 << 1,
   RMT_BUFFER_USAGE_FLAGS_UNIFORM_TEXEL_BUFFER = 1 << 2,
   RMT_BUFFER_USAGE_FLAGS_STORAGE_TEXEL_BUFFER = 1 << 3,
   RMT_BUFFER_USAGE_FLAGS_UNIFORM_BUFFER = 1 << 4,
   RMT_BUFFER_USAGE_FLAGS_STORAGE_BUFFER = 1 << 5,
   RMT_BUFFER_USAGE_FLAGS_INDEX_BUFFER = 1 << 6,
   RMT_BUFFER_USAGE_FLAGS_VERTEX_BUFFER = 1 << 7,
   RMT_BUFFER_USAGE_FLAGS_INDIRECT_BUFFER = 1 << 8,
   RMT_BUFFER_USAGE_FLAGS_TRANSFORM_FEEDBACK_BUFFER = 1 << 9,
   RMT_BUFFER_USAGE_FLAGS_TRANSFORM_FEEDBACK_COUNTER_BUFFER = 1 << 10,
   RMT_BUFFER_USAGE_FLAGS_CONDITIONAL_RENDERING = 1 << 11,
   RMT_BUFFER_USAGE_FLAGS_RAY_TRACING = 1 << 12,
      };
      static void
   rmt_dump_buffer_resource(struct vk_rmv_buffer_description *description, FILE *output)
   {
      /* flags up to indirect buffer are equivalent */
   uint32_t usage_flags =
            if (description->usage_flags & VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT)
         if (description->usage_flags & VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT)
         if (description->usage_flags & VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT)
         if (description->usage_flags &
      (VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
   VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR))
      if (description->usage_flags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
            uint64_t data[2] = {0};
   rmt_file_write_bits(data, description->create_flags, 0, 7);
   rmt_file_write_bits(data, usage_flags, 8, 23);
   rmt_file_write_bits(data, description->size, 24, 87);
      }
      enum rmt_tiling {
      RMT_TILING_LINEAR,
   RMT_TILING_OPTIMAL,
      };
      enum rmt_tiling_optimization_mode {
      RMT_TILING_OPTIMIZATION_MODE_BALANCED,
   RMT_TILING_OPTIMIZATION_MODE_SPACE,
      };
      enum rmt_metadata_mode {
      RMT_METADATA_MODE_DEFAULT,
   RMT_METADATA_MODE_OPTIMIZE_TEX_PREFETCH,
      };
      enum rmt_image_create_flags {
      RMT_IMAGE_CREATE_INVARIANT = 1 << 0,
   RMT_IMAGE_CREATE_CLONEABLE = 1 << 1,
   RMT_IMAGE_CREATE_SHAREABLE = 1 << 2,
   RMT_IMAGE_CREATE_FLIPPABLE = 1 << 3,
   RMT_IMAGE_CREATE_STEREO = 1 << 4,
   RMT_IMAGE_CREATE_CUBEMAP = 1 << 5,
      };
      enum rmt_image_usage_flags {
      RMT_IMAGE_USAGE_SHADER_READ = 1 << 0,
   RMT_IMAGE_USAGE_SHADER_WRITE = 1 << 1,
   RMT_IMAGE_USAGE_RESOLVE_SRC = 1 << 2,
   RMT_IMAGE_USAGE_RESOLVE_DST = 1 << 3,
   RMT_IMAGE_USAGE_COLOR_TARGET = 1 << 4,
      };
      static void
   rmt_dump_image_resource(struct vk_rmv_image_description *description, FILE *output)
   {
               enum rmt_tiling tiling;
   switch (description->tiling) {
   case VK_IMAGE_TILING_LINEAR:
      tiling = RMT_TILING_LINEAR;
      case VK_IMAGE_TILING_OPTIMAL:
   case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT:
      tiling = RMT_TILING_OPTIMAL;
      default:
                  uint32_t create_flags = 0;
   if (description->create_flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
         if (description->create_flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT)
            uint32_t usage_flags = 0;
   if (description->usage_flags & VK_IMAGE_USAGE_SAMPLED_BIT ||
      description->usage_flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
      if (description->usage_flags & VK_IMAGE_USAGE_STORAGE_BIT)
         if (description->usage_flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
         if (description->usage_flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
         if (description->usage_flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
         if (description->usage_flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            enum rmt_swizzle swizzles[4] = {RMT_SWIZZLE_ZERO, RMT_SWIZZLE_ZERO, RMT_SWIZZLE_ZERO,
                  rmt_file_write_bits(data, create_flags, 0, 19);
   rmt_file_write_bits(data, usage_flags, 20, 34);
   rmt_file_write_bits(data, description->type, 35, 36);
   rmt_file_write_bits(data, description->extent.width - 1, 37, 50);
   rmt_file_write_bits(data, description->extent.height - 1, 51, 64);
   rmt_file_write_bits(data, description->extent.depth - 1, 65, 78);
   rmt_file_write_bits(data, swizzles[0], 79, 81);
   rmt_file_write_bits(data, swizzles[1], 82, 84);
   rmt_file_write_bits(data, swizzles[2], 85, 87);
   rmt_file_write_bits(data, swizzles[3], 88, 90);
   rmt_file_write_bits(data, vk_to_rmt_format(description->format), 91, 98);
   rmt_file_write_bits(data, description->num_mips, 99, 102);
   rmt_file_write_bits(data, description->num_slices - 1, 103, 113);
   rmt_file_write_bits(data, description->log2_samples, 114, 116);
   rmt_file_write_bits(data, description->log2_storage_samples, 117, 118);
   rmt_file_write_bits(data, tiling, 119, 120);
   rmt_file_write_bits(data, RMT_TILING_OPTIMIZATION_MODE_BALANCED, 121, 122);
   rmt_file_write_bits(data, RMT_METADATA_MODE_DEFAULT, 123, 124);
   rmt_file_write_bits(data, description->alignment_log2, 125, 129);
   rmt_file_write_bits(data, description->presentable, 130, 130);
   rmt_file_write_bits(data, description->size, 131, 162);
   rmt_file_write_bits(data, description->metadata_offset, 163, 194);
   rmt_file_write_bits(data, description->metadata_size, 195, 226);
   rmt_file_write_bits(data, description->metadata_header_offset, 227, 258);
   rmt_file_write_bits(data, description->metadata_header_size, 259, 290);
   rmt_file_write_bits(data, description->image_alignment_log2, 291, 295);
   rmt_file_write_bits(data, description->metadata_alignment_log2, 296, 300);
   /* metadata header alignment */
   rmt_file_write_bits(data, description->metadata_alignment_log2, 301, 305);
   /* is fullscreen presentable */
   rmt_file_write_bits(data, description->presentable, 306, 306);
      }
      enum rmt_query_pool_type {
      RMT_QUERY_POOL_TYPE_OCCLUSION,
   RMT_QUERY_POOL_TYPE_PIPELINE,
      };
      static void
   rmt_dump_query_pool_resource(struct vk_rmv_query_pool_description *description, FILE *output)
   {
      enum rmt_query_pool_type pool_type;
   switch (description->type) {
   case VK_QUERY_TYPE_OCCLUSION:
      pool_type = RMT_QUERY_POOL_TYPE_OCCLUSION;
      case VK_QUERY_TYPE_PIPELINE_STATISTICS:
      pool_type = RMT_QUERY_POOL_TYPE_PIPELINE;
      case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      pool_type = RMT_QUERY_POOL_TYPE_STREAMOUT;
      default:
      unreachable("invalid query pool type");
               uint64_t data = 0;
   rmt_file_write_bits(&data, pool_type, 0, 1);
   rmt_file_write_bits(&data, description->has_cpu_access, 2, 2);
      }
      enum rmt_pipeline_flags {
      RMT_PIPELINE_FLAG_INTERNAL = (1 << 0),
      };
      enum rmt_pipeline_stage_flags {
      RMT_PIPELINE_STAGE_FRAGMENT = 1 << 0,
   RMT_PIPELINE_STAGE_TESS_CONTROL = 1 << 1,
   RMT_PIPELINE_STAGE_TESS_EVAL = 1 << 2,
   RMT_PIPELINE_STAGE_VERTEX = 1 << 3,
   RMT_PIPELINE_STAGE_GEOMETRY = 1 << 4,
   RMT_PIPELINE_STAGE_COMPUTE = 1 << 5,
   RMT_PIPELINE_STAGE_TASK = 1 << 6,
      };
      static void
   rmt_dump_pipeline_resource(struct vk_rmv_pipeline_description *description, FILE *output)
   {
               enum rmt_pipeline_flags flags = 0;
   if (description->is_internal)
            enum rmt_pipeline_stage_flags stage_flags = 0;
   if (description->shader_stages & VK_SHADER_STAGE_FRAGMENT_BIT)
         if (description->shader_stages & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
         if (description->shader_stages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
         if (description->shader_stages & VK_SHADER_STAGE_VERTEX_BIT)
         if (description->shader_stages & VK_SHADER_STAGE_GEOMETRY_BIT)
         if (description->shader_stages & VK_SHADER_STAGE_COMPUTE_BIT ||
      description->shader_stages & VK_SHADER_STAGE_RAYGEN_BIT_KHR ||
   description->shader_stages & VK_SHADER_STAGE_INTERSECTION_BIT_KHR ||
   description->shader_stages & VK_SHADER_STAGE_ANY_HIT_BIT_KHR ||
   description->shader_stages & VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR ||
   description->shader_stages & VK_SHADER_STAGE_MISS_BIT_KHR ||
   description->shader_stages & VK_SHADER_STAGE_CALLABLE_BIT_KHR)
      if (description->shader_stages & VK_SHADER_STAGE_TASK_BIT_EXT)
         if (description->shader_stages & VK_SHADER_STAGE_MESH_BIT_EXT)
            rmt_file_write_bits(data, flags, 0, 7);
   rmt_file_write_bits(data, description->hash_hi, 8, 71);
   rmt_file_write_bits(data, description->hash_lo, 72, 135);
   rmt_file_write_bits(data, stage_flags, 136, 143);
   rmt_file_write_bits(data, description->is_ngg, 144, 144);
      }
      static void
   rmt_dump_descriptor_pool_resource(struct vk_rmv_descriptor_pool_description *description,
         {
      uint64_t data = 0;
   /* TODO: figure out a better way of handling descriptor counts > 65535 */
   rmt_file_write_bits(&data, MIN2(description->max_sets, 65535), 0, 15);
   rmt_file_write_bits(&data, rmt_valid_pool_size_count(description), 16, 23);
            for (uint32_t i = 0; i < description->pool_size_count; ++i) {
      data = 0;
   enum rmt_descriptor_type rmt_type =
         if (rmt_type == RMT_DESCRIPTOR_TYPE_INVALID)
      /* Unknown descriptor type, skip */
      rmt_file_write_bits(&data, rmt_type, 0, 15);
   rmt_file_write_bits(&data, MIN2(description->pool_sizes[i].descriptorCount, 65535), 16, 31);
         }
      static void
   rmt_dump_command_buffer_resource(struct vk_rmv_command_buffer_description *description,
         {
      uint64_t data[6] = {0};
   rmt_file_write_bits(data, 0, 0, 3); /* flags */
   /* heap for executable commands */
   rmt_file_write_bits(data, rmt_file_domain_to_heap_type(description->preferred_domain, true), 4,
         /* executable command allocation size */
   rmt_file_write_bits(data, description->executable_size, 8, 63);
   /* executable command size usable by command buffers */
   rmt_file_write_bits(data, description->app_available_executable_size, 64, 119);
   /* heap for embedded data */
   rmt_file_write_bits(data, rmt_file_domain_to_heap_type(description->preferred_domain, true), 120,
         /* embedded data allocation size */
   rmt_file_write_bits(data, description->embedded_data_size, 124, 179);
   /* embedded data size usable by command buffers */
   rmt_file_write_bits(data, description->app_available_embedded_data_size, 180, 235);
   /* heap for scratch data */
   rmt_file_write_bits(data, rmt_file_domain_to_heap_type(description->preferred_domain, true), 4,
         /* scratch data allocation size */
   rmt_file_write_bits(data, description->scratch_size, 240, 295);
   /* scratch data size usable by command buffers */
               }
      static void
   rmt_dump_resource_create(struct vk_rmv_resource_create_token *token, FILE *output)
   {
      uint64_t data = 0;
   rmt_file_write_token_bits(&data, token->resource_id, 8, 39);
   rmt_file_write_token_bits(&data,
                     rmt_file_write_token_bits(&data, token->type, 48, 53);
            switch (token->type) {
   case VK_RMV_RESOURCE_TYPE_GPU_EVENT:
      rmt_dump_event_resource(&token->event, output);
      case VK_RMV_RESOURCE_TYPE_BORDER_COLOR_PALETTE:
      rmt_dump_border_color_palette_resource(&token->border_color_palette, output);
      case VK_RMV_RESOURCE_TYPE_HEAP:
      rmt_dump_heap_resource(&token->heap, output);
      case VK_RMV_RESOURCE_TYPE_BUFFER:
      rmt_dump_buffer_resource(&token->buffer, output);
      case VK_RMV_RESOURCE_TYPE_IMAGE:
      rmt_dump_image_resource(&token->image, output);
      case VK_RMV_RESOURCE_TYPE_QUERY_HEAP:
      rmt_dump_query_pool_resource(&token->query_pool, output);
      case VK_RMV_RESOURCE_TYPE_PIPELINE:
      rmt_dump_pipeline_resource(&token->pipeline, output);
      case VK_RMV_RESOURCE_TYPE_DESCRIPTOR_POOL:
      rmt_dump_descriptor_pool_resource(&token->descriptor_pool, output);
      case VK_RMV_RESOURCE_TYPE_COMMAND_ALLOCATOR:
      rmt_dump_command_buffer_resource(&token->command_buffer, output);
      default:
            }
      static void
   rmt_dump_resource_bind(struct vk_rmv_resource_bind_token *token, FILE *output)
   {
      uint64_t data[3] = {0};
   rmt_file_write_token_bits(data, token->address & 0xFFFFFFFFFFFF, 8, 55);
   rmt_file_write_token_bits(data, token->size, 56, 99);
   rmt_file_write_token_bits(data, token->is_system_memory, 100, 100);
   rmt_file_write_token_bits(data, token->resource_id, 104, 135);
      }
      static void
   rmt_dump_resource_reference(struct vk_rmv_resource_reference_token *token,
         {
      uint64_t data = 0;
   rmt_file_write_token_bits(&data, token->residency_removed, 8, 8);
   rmt_file_write_token_bits(&data, token->virtual_address & 0xFFFFFFFFFFFF, 9, 56);
      }
      static void
   rmt_dump_resource_destroy(struct vk_rmv_resource_destroy_token *token, FILE *output)
   {
      uint64_t data = 0;
   rmt_file_write_token_bits(&data, token->resource_id, 8, 39);
      }
      enum rmt_virtual_allocation_owner_type {
      RMT_VIRTUAL_ALLOCATION_OWNER_TYPE_APPLICATION,
   RMT_VIRTUAL_ALLOCATION_OWNER_TYPE_PAL,
   RMT_VIRTUAL_ALLOCATION_OWNER_TYPE_CLIENT_DRIVER,
      };
      static void
   rmt_dump_virtual_alloc(struct vk_rmv_virtual_allocate_token *token, FILE *output)
   {
      uint64_t data[2] = {0};
   rmt_file_write_token_bits(data, token->page_count - 1, 8, 31);
   rmt_file_write_token_bits(data,
                           rmt_file_write_token_bits(data, token->address & 0xFFFFFFFFFFFF, 34, 81);
   if (token->preferred_domains) {
      rmt_file_write_token_bits(
      data, rmt_file_domain_to_heap_type(token->preferred_domains, !token->is_in_invisible_vram),
      /* num. of heap types */
      } else
            }
      static void
   rmt_dump_virtual_free(struct vk_rmv_virtual_free_token *token, FILE *output)
   {
      uint64_t data = 0;
   rmt_file_write_token_bits(&data, token->address & 0xFFFFFFFFFFFF, 8, 56);
      }
      enum rmt_page_table_controller {
      RMT_PAGE_TABLE_CONTROLLER_OS,
      };
      static void
   rmt_dump_page_table_update(struct vk_rmv_page_table_update_token *token,
         {
      uint64_t virtual_page_idx = (token->virtual_address / 4096);
                     uint64_t data[3] = {0};
   rmt_file_write_token_bits(data, virtual_page_idx & 0xFFFFFFFFF, 8, 43);
   rmt_file_write_token_bits(data, physical_page_idx & 0xFFFFFFFFF, 44, 79);
   rmt_file_write_token_bits(data, token->page_count, 80, 99);
   rmt_file_write_token_bits(data, page_size, 100, 102);
   rmt_file_write_token_bits(data, token->is_unmap, 103, 103);
   rmt_file_write_token_bits(data, token->pid, 104, 135);
   rmt_file_write_token_bits(data, token->type, 136, 137);
   rmt_file_write_token_bits(data, RMT_PAGE_TABLE_CONTROLLER_KMD, 138, 138);
      }
      enum rmt_userdata_type {
      RMT_USERDATA_TYPE_NAME,
   RMT_USERDATA_TYPE_SNAPSHOT,
   RMT_USERDATA_TYPE_BINARY,
   RMT_USERDATA_TYPE_RESERVED,
   RMT_USERDATA_TYPE_CORRELATION,
      };
      static void
   rmt_dump_userdata(struct vk_rmv_userdata_token *token, FILE *output)
   {
      uint64_t data = 0;
   /* userdata type */
   rmt_file_write_token_bits(&data, RMT_USERDATA_TYPE_NAME, 8, 11);
   /* size of userdata payload */
            fwrite(&data, 3, 1, output);
   fwrite(token->name, 1, strlen(token->name) + 1, output);
      }
      static void
   rmt_dump_misc(struct vk_rmv_misc_token *token, FILE *output)
   {
      uint64_t data = 0;
   rmt_file_write_token_bits(&data, token->type, 8, 11);
      }
      static void
   rmt_dump_cpu_map(struct vk_rmv_cpu_map_token *token, FILE *output)
   {
      uint64_t data = 0;
   rmt_file_write_token_bits(&data, token->address & 0xFFFFFFFFFFFF, 8, 55);
   rmt_file_write_token_bits(&data, token->unmapped, 56, 56);
      }
      static void
   rmt_dump_data(struct vk_memory_trace_data *data, FILE *output)
   {
      struct rmt_file_header header = {0};
   struct rmt_file_chunk_system_info system_info_chunk = {0};
   struct rmt_file_chunk_adapter_info adapter_info_chunk = {0};
            /* RMT header */
   rmt_fill_header(&header);
            /* System info */
   rmt_fill_chunk_system_info(&system_info_chunk);
            /* Segment info */
   for (int32_t i = 0; i < 3; ++i) {
               rmt_fill_chunk_segment_info(data, &data->device_info, &segment_info_chunk, i);
               /* Adapter info */
   rmt_fill_chunk_adapter_info(&data->device_info, &adapter_info_chunk);
            long chunk_start = ftell(output);
   /* Write a dummy data chunk to reserve space */
            qsort(data->tokens.data, util_dynarray_num_elements(&data->tokens, struct vk_rmv_token),
            uint64_t current_timestamp = 0;
   if (util_dynarray_num_elements(&data->tokens, struct vk_rmv_token))
      current_timestamp =
                  struct vk_rmv_timestamp_token timestamp_token;
   timestamp_token.value = 0;
            util_dynarray_foreach (&data->tokens, struct vk_rmv_token, token) {
      /* Only temporarily modify the token's timestamp in case of multiple traces */
   uint64_t old_timestamp = token->timestamp;
   /* adjust timestamp to 1 MHz, see rmt_fill_chunk_system_info */
                     /* Time values are stored divided by 32 */
            /*
   * Each token can hold up to 4 bits of time delta. If the delta doesn't
   * fit in 4 bits, an additional token containing more space for the delta
   * has to be emitted.
   */
   if (delta > 0xF) {
      rmt_dump_time_delta(delta, output);
               uint64_t token_header = 0;
   rmt_file_write_bits(&token_header, token_type_to_rmt(token->type), 0, 3);
   rmt_file_write_bits(&token_header, delta, 4, 7);
            switch (token->type) {
   case VK_RMV_TOKEN_TYPE_VIRTUAL_ALLOCATE:
      rmt_dump_virtual_alloc(&token->data.virtual_allocate, output);
      case VK_RMV_TOKEN_TYPE_VIRTUAL_FREE:
      rmt_dump_virtual_free(&token->data.virtual_free, output);
      case VK_RMV_TOKEN_TYPE_PAGE_TABLE_UPDATE:
      rmt_dump_page_table_update(&token->data.page_table_update, output);
      case VK_RMV_TOKEN_TYPE_RESOURCE_CREATE:
      rmt_dump_resource_create(&token->data.resource_create, output);
      case VK_RMV_TOKEN_TYPE_RESOURCE_DESTROY:
      rmt_dump_resource_destroy(&token->data.resource_destroy, output);
      case VK_RMV_TOKEN_TYPE_RESOURCE_BIND:
      rmt_dump_resource_bind(&token->data.resource_bind, output);
      case VK_RMV_TOKEN_TYPE_RESOURCE_REFERENCE:
      rmt_dump_resource_reference(&token->data.resource_reference, output);
      case VK_RMV_TOKEN_TYPE_USERDATA:
      rmt_dump_userdata(&token->data.userdata, output);
      case VK_RMV_TOKEN_TYPE_MISC:
      rmt_dump_misc(&token->data.misc, output);
      case VK_RMV_TOKEN_TYPE_CPU_MAP:
      rmt_dump_cpu_map(&token->data.cpu_map, output);
      default:
                  current_timestamp = token->timestamp;
      }
            /* Go back and write the correct chunk data. */
   fseek(output, chunk_start, SEEK_SET);
   rmt_fill_chunk_rmt_data(stream_end - stream_start, &data_chunk);
      }
      int
   vk_dump_rmv_capture(struct vk_memory_trace_data *data)
   {
      char filename[2048];
   struct tm now;
            time_t t = time(NULL);
            snprintf(filename, sizeof(filename), "/tmp/%s_%04d.%02d.%02d_%02d.%02d.%02d.rmv",
                  f = fopen(filename, "wb");
   if (!f)
                              fclose(f);
      }
