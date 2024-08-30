   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2021 GlobalLogic Ukraine
   * Copyright (C) 2021-2022 Roman Stratiienko (r.stratiienko@gmail.com)
   * SPDX-License-Identifier: MIT
   */
      #include <aidl/android/hardware/graphics/common/BufferUsage.h>
   #include <aidl/android/hardware/graphics/common/ChromaSiting.h>
   #include <aidl/android/hardware/graphics/common/Dataspace.h>
   #include <aidl/android/hardware/graphics/common/ExtendableType.h>
   #include <aidl/android/hardware/graphics/common/PlaneLayoutComponent.h>
   #include <aidl/android/hardware/graphics/common/PlaneLayoutComponentType.h>
   #include <gralloctypes/Gralloc4.h>
   #include <system/window.h>
      #include "util/log.h"
   #include "u_gralloc_internal.h"
      using aidl::android::hardware::graphics::common::BufferUsage;
   using aidl::android::hardware::graphics::common::ChromaSiting;
   using aidl::android::hardware::graphics::common::Dataspace;
   using aidl::android::hardware::graphics::common::ExtendableType;
   using aidl::android::hardware::graphics::common::PlaneLayout;
   using aidl::android::hardware::graphics::common::PlaneLayoutComponent;
   using aidl::android::hardware::graphics::common::PlaneLayoutComponentType;
   using android::hardware::hidl_handle;
   using android::hardware::hidl_vec;
   using android::hardware::graphics::mapper::V4_0::Error;
   using android::hardware::graphics::mapper::V4_0::IMapper;
      using MetadataType =
            Error
   GetMetadata(android::sp<IMapper> mapper, const native_handle_t *buffer,
         {
                        auto ret =
      mapper->get(native_handle, type,
               [&](const auto &get_error, const auto &get_metadata) {
         if (!ret.isOk())
               }
      std::optional<std::vector<PlaneLayout>>
   GetPlaneLayouts(android::sp<IMapper> mapper, const native_handle_t *buffer)
   {
               Error error = GetMetadata(mapper, buffer,
                  if (error != Error::NONE)
                     auto status =
            if (status != android::OK)
               }
      struct gralloc4 {
      struct u_gralloc base;
      };
      extern "C" {
      static int
   mapper4_get_buffer_basic_info(struct u_gralloc *gralloc,
               {
               if (gr4->mapper == nullptr)
            if (!hnd->handle)
            hidl_vec<uint8_t> encoded_format;
   auto err = GetMetadata(gr4->mapper, hnd->handle,
               if (err != Error::NONE)
                     auto status =
         if (status != android::OK)
                     hidl_vec<uint8_t> encoded_modifier;
   err = GetMetadata(gr4->mapper, hnd->handle,
               if (err != Error::NONE)
            status = android::gralloc4::decodePixelFormatModifier(encoded_modifier,
         if (status != android::OK)
            out->drm_fourcc = drm_fourcc;
                     if (!layouts_opt)
                                       for (uint32_t i = 0; i < layouts.size(); i++) {
      out->strides[i] = layouts[i].strideInBytes;
            /* offset == 0 means layer is located in different dma-buf */
   if (out->offsets[i] == 0 && i > 0)
            if (fd_index >= hnd->handle->numFds)
                           }
      static int
   mapper4_get_buffer_color_info(struct u_gralloc *gralloc,
               {
               if (gr4->mapper == nullptr)
            if (!hnd->handle)
            /* optional attributes */
   hidl_vec<uint8_t> encoded_chroma_siting;
   std::optional<ChromaSiting> chroma_siting;
   auto err = GetMetadata(gr4->mapper, hnd->handle,
               if (err == Error::NONE) {
      ExtendableType chroma_siting_ext;
   auto status = android::gralloc4::decodeChromaSiting(
         if (status != android::OK)
            chroma_siting =
               hidl_vec<uint8_t> encoded_dataspace;
   err = GetMetadata(gr4->mapper, hnd->handle,
               if (err == Error::NONE) {
      Dataspace dataspace;
   auto status =
         if (status != android::OK)
            Dataspace standard =
         switch (standard) {
   case Dataspace::STANDARD_BT709:
      out->yuv_color_space = __DRI_YUV_COLOR_SPACE_ITU_REC709;
      case Dataspace::STANDARD_BT601_625:
   case Dataspace::STANDARD_BT601_625_UNADJUSTED:
   case Dataspace::STANDARD_BT601_525:
   case Dataspace::STANDARD_BT601_525_UNADJUSTED:
      out->yuv_color_space = __DRI_YUV_COLOR_SPACE_ITU_REC601;
      case Dataspace::STANDARD_BT2020:
   case Dataspace::STANDARD_BT2020_CONSTANT_LUMINANCE:
      out->yuv_color_space = __DRI_YUV_COLOR_SPACE_ITU_REC2020;
      default:
                  Dataspace range =
         switch (range) {
   case Dataspace::RANGE_FULL:
      out->sample_range = __DRI_YUV_FULL_RANGE;
      case Dataspace::RANGE_LIMITED:
      out->sample_range = __DRI_YUV_NARROW_RANGE;
      default:
                     if (chroma_siting) {
      switch (*chroma_siting) {
   case ChromaSiting::SITED_INTERSTITIAL:
      out->horizontal_siting = __DRI_YUV_CHROMA_SITING_0_5;
   out->vertical_siting = __DRI_YUV_CHROMA_SITING_0_5;
      case ChromaSiting::COSITED_HORIZONTAL:
      out->horizontal_siting = __DRI_YUV_CHROMA_SITING_0;
   out->vertical_siting = __DRI_YUV_CHROMA_SITING_0_5;
      default:
                        }
      static int
   mapper4_get_front_rendering_usage(struct u_gralloc *gralloc,
         {
         #if ANDROID_API_LEVEL >= 33
                  #else
         #endif
   }
      static int
   destroy(struct u_gralloc *gralloc)
   {
      gralloc4 *gr = (struct gralloc4 *)gralloc;
               }
      struct u_gralloc *
   u_gralloc_imapper_api_create()
   {
      auto mapper = IMapper::getService();
   if (!mapper)
            auto gr = new gralloc4;
   gr->mapper = mapper;
   gr->base.ops.get_buffer_basic_info = mapper4_get_buffer_basic_info;
   gr->base.ops.get_buffer_color_info = mapper4_get_buffer_color_info;
   gr->base.ops.get_front_rendering_usage = mapper4_get_front_rendering_usage;
                        }
      } // extern "C"
