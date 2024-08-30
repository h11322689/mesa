   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2021 GlobalLogic Ukraine
   * Copyright (C) 2021 Roman Stratiienko (r.stratiienko@gmail.com)
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "platform_android.h"
      #include <aidl/android/hardware/graphics/common/ChromaSiting.h>
   #include <aidl/android/hardware/graphics/common/Dataspace.h>
   #include <aidl/android/hardware/graphics/common/ExtendableType.h>
   #include <aidl/android/hardware/graphics/common/PlaneLayoutComponent.h>
   #include <aidl/android/hardware/graphics/common/PlaneLayoutComponentType.h>
   #include <gralloctypes/Gralloc4.h>
   #include <system/window.h>
      using aidl::android::hardware::graphics::common::ChromaSiting;
   using aidl::android::hardware::graphics::common::Dataspace;
   using aidl::android::hardware::graphics::common::ExtendableType;
   using aidl::android::hardware::graphics::common::PlaneLayout;
   using aidl::android::hardware::graphics::common::PlaneLayoutComponent;
   using aidl::android::hardware::graphics::common::PlaneLayoutComponentType;
   using android::hardware::hidl_handle;
   using android::hardware::hidl_vec;
   using android::hardware::graphics::common::V1_2::BufferUsage;
   using android::hardware::graphics::mapper::V4_0::Error;
   using android::hardware::graphics::mapper::V4_0::IMapper;
   using MetadataType =
            Error
   GetMetadata(android::sp<IMapper> mapper, const native_handle_t *buffer,
         {
                        auto ret = mapper->get(native_handle, type,
                              if (!ret.isOk())
               }
      std::optional<std::vector<PlaneLayout>>
   GetPlaneLayouts(android::sp<IMapper> mapper, const native_handle_t *buffer)
   {
               Error error =
      GetMetadata(mapper, buffer, android::gralloc4::MetadataType_PlaneLayouts,
         if (error != Error::NONE)
                     auto status =
            if (status != android::OK)
               }
      extern "C" {
      int
   mapper_metadata_get_buffer_info(struct ANativeWindowBuffer *buf,
         {
      static android::sp<IMapper> mapper = IMapper::getService();
   struct buffer_info buf_info = *out_buf_info;
   if (mapper == nullptr)
            if (!buf->handle)
            buf_info.width = buf->width;
            hidl_vec<uint8_t> encoded_format;
   auto err = GetMetadata(mapper, buf->handle,
               if (err != Error::NONE)
            auto status = android::gralloc4::decodePixelFormatFourCC(
         if (status != android::OK)
            hidl_vec<uint8_t> encoded_modifier;
   err = GetMetadata(mapper, buf->handle,
               if (err != Error::NONE)
            status = android::gralloc4::decodePixelFormatModifier(encoded_modifier,
         if (status != android::OK)
                     if (!layouts_opt)
                                       for (uint32_t i = 0; i < layouts.size(); i++) {
      buf_info.fds[i] =
         buf_info.pitches[i] = layouts[i].strideInBytes;
               /* optional attributes */
   hidl_vec<uint8_t> encoded_chroma_siting;
   err = GetMetadata(mapper, buf->handle,
               if (err == Error::NONE) {
      ExtendableType chroma_siting_ext;
   status = android::gralloc4::decodeChromaSiting(encoded_chroma_siting,
         if (status != android::OK)
            ChromaSiting chroma_siting =
         switch (chroma_siting) {
   case ChromaSiting::SITED_INTERSTITIAL:
      buf_info.horizontal_siting = __DRI_YUV_CHROMA_SITING_0_5;
   buf_info.vertical_siting = __DRI_YUV_CHROMA_SITING_0_5;
      case ChromaSiting::COSITED_HORIZONTAL:
      buf_info.horizontal_siting = __DRI_YUV_CHROMA_SITING_0;
   buf_info.vertical_siting = __DRI_YUV_CHROMA_SITING_0_5;
      default:
                     hidl_vec<uint8_t> encoded_dataspace;
   err = GetMetadata(mapper, buf->handle,
               if (err == Error::NONE) {
      Dataspace dataspace;
   status =
         if (status != android::OK)
            Dataspace standard =
         switch (standard) {
   case Dataspace::STANDARD_BT709:
      buf_info.yuv_color_space = __DRI_YUV_COLOR_SPACE_ITU_REC709;
      case Dataspace::STANDARD_BT601_625:
   case Dataspace::STANDARD_BT601_625_UNADJUSTED:
   case Dataspace::STANDARD_BT601_525:
   case Dataspace::STANDARD_BT601_525_UNADJUSTED:
      buf_info.yuv_color_space = __DRI_YUV_COLOR_SPACE_ITU_REC601;
      case Dataspace::STANDARD_BT2020:
   case Dataspace::STANDARD_BT2020_CONSTANT_LUMINANCE:
      buf_info.yuv_color_space = __DRI_YUV_COLOR_SPACE_ITU_REC2020;
      default:
                  Dataspace range =
         switch (range) {
   case Dataspace::RANGE_FULL:
      buf_info.sample_range = __DRI_YUV_FULL_RANGE;
      case Dataspace::RANGE_LIMITED:
      buf_info.sample_range = __DRI_YUV_NARROW_RANGE;
      default:
                                 }
      } // extern "C"
