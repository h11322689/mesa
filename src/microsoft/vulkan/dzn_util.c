   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dzn_private.h"
      #define D3D12_IGNORE_SDK_LAYERS
   #define COBJMACROS
   #include <directx/d3d12.h>
      #include <vulkan/vulkan.h>
      #include "util/format/u_format.h"
   #include "util/log.h"
      #include <directx/d3d12sdklayers.h>
   #include <util/u_dl.h>
      static const DXGI_FORMAT formats[PIPE_FORMAT_COUNT] = {
   #define MAP_FORMAT_NORM(FMT) \
      [PIPE_FORMAT_ ## FMT ## _UNORM] = DXGI_FORMAT_ ## FMT ## _UNORM, \
         #define MAP_FORMAT_INT(FMT) \
      [PIPE_FORMAT_ ## FMT ## _UINT] = DXGI_FORMAT_ ## FMT ## _UINT, \
         #define MAP_FORMAT_SRGB(FMT) \
            #define MAP_FORMAT_FLOAT(FMT) \
            #define MAP_EMU_FORMAT_NO_ALPHA(BITS, TYPE) \
      [PIPE_FORMAT_L ## BITS ## _ ## TYPE] = DXGI_FORMAT_R ## BITS ## _ ## TYPE, \
   [PIPE_FORMAT_I ## BITS ## _ ## TYPE] = DXGI_FORMAT_R ## BITS ## _ ## TYPE, \
   [PIPE_FORMAT_L ## BITS ## A ## BITS ## _ ## TYPE] = \
         #define MAP_EMU_FORMAT(BITS, TYPE) \
      [PIPE_FORMAT_A ## BITS ## _ ## TYPE] = DXGI_FORMAT_R ## BITS ## _ ## TYPE, \
            MAP_FORMAT_NORM(R8)
            MAP_FORMAT_NORM(R8G8)
            MAP_FORMAT_NORM(R8G8B8A8)
   MAP_FORMAT_INT(R8G8B8A8)
            [PIPE_FORMAT_B8G8R8X8_UNORM] = DXGI_FORMAT_B8G8R8X8_UNORM,
   [PIPE_FORMAT_B8G8R8A8_UNORM] = DXGI_FORMAT_B8G8R8A8_UNORM,
   [PIPE_FORMAT_B4G4R4A4_UNORM] = DXGI_FORMAT_B4G4R4A4_UNORM,
   [PIPE_FORMAT_A4R4G4B4_UNORM] = DXGI_FORMAT_A4B4G4R4_UNORM,
   [PIPE_FORMAT_A4B4G4R4_UNORM] = DXGI_FORMAT_A4B4G4R4_UNORM,
   [PIPE_FORMAT_B5G6R5_UNORM] = DXGI_FORMAT_B5G6R5_UNORM,
                     MAP_FORMAT_INT(R32)
   MAP_FORMAT_FLOAT(R32)
   MAP_FORMAT_INT(R32G32)
   MAP_FORMAT_FLOAT(R32G32)
   MAP_FORMAT_INT(R32G32B32)
   MAP_FORMAT_FLOAT(R32G32B32)
   MAP_FORMAT_INT(R32G32B32A32)
            MAP_FORMAT_NORM(R16)
   MAP_FORMAT_INT(R16)
            MAP_FORMAT_NORM(R16G16)
   MAP_FORMAT_INT(R16G16)
            MAP_FORMAT_NORM(R16G16B16A16)
   MAP_FORMAT_INT(R16G16B16A16)
            [PIPE_FORMAT_A8_UNORM] = DXGI_FORMAT_A8_UNORM,
   MAP_EMU_FORMAT_NO_ALPHA(8, UNORM)
   MAP_EMU_FORMAT(8, SNORM)
   MAP_EMU_FORMAT(8, SINT)
   MAP_EMU_FORMAT(8, UINT)
   MAP_EMU_FORMAT(16, UNORM)
   MAP_EMU_FORMAT(16, SNORM)
   MAP_EMU_FORMAT(16, SINT)
   MAP_EMU_FORMAT(16, UINT)
   MAP_EMU_FORMAT(16, FLOAT)
   MAP_EMU_FORMAT(32, SINT)
   MAP_EMU_FORMAT(32, UINT)
            [PIPE_FORMAT_R9G9B9E5_FLOAT] = DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
   [PIPE_FORMAT_R11G11B10_FLOAT] = DXGI_FORMAT_R11G11B10_FLOAT,
   [PIPE_FORMAT_R10G10B10A2_UINT] = DXGI_FORMAT_R10G10B10A2_UINT,
            [PIPE_FORMAT_DXT1_RGB] = DXGI_FORMAT_BC1_UNORM,
   [PIPE_FORMAT_DXT1_RGBA] = DXGI_FORMAT_BC1_UNORM,
   [PIPE_FORMAT_DXT3_RGBA] = DXGI_FORMAT_BC2_UNORM,
            [PIPE_FORMAT_DXT1_SRGB] = DXGI_FORMAT_BC1_UNORM_SRGB,
   [PIPE_FORMAT_DXT1_SRGBA] = DXGI_FORMAT_BC1_UNORM_SRGB,
   [PIPE_FORMAT_DXT3_SRGBA] = DXGI_FORMAT_BC2_UNORM_SRGB,
            [PIPE_FORMAT_RGTC1_UNORM] = DXGI_FORMAT_BC4_UNORM,
   [PIPE_FORMAT_RGTC1_SNORM] = DXGI_FORMAT_BC4_SNORM,
   [PIPE_FORMAT_RGTC2_UNORM] = DXGI_FORMAT_BC5_UNORM,
            [PIPE_FORMAT_BPTC_RGB_UFLOAT] = DXGI_FORMAT_BC6H_UF16,
   [PIPE_FORMAT_BPTC_RGB_FLOAT] = DXGI_FORMAT_BC6H_SF16,
   [PIPE_FORMAT_BPTC_RGBA_UNORM] = DXGI_FORMAT_BC7_UNORM,
            [PIPE_FORMAT_Z32_FLOAT] = DXGI_FORMAT_R32_TYPELESS,
   [PIPE_FORMAT_Z16_UNORM] = DXGI_FORMAT_R16_TYPELESS,
   [PIPE_FORMAT_Z24X8_UNORM] = DXGI_FORMAT_R24G8_TYPELESS,
            [PIPE_FORMAT_Z24_UNORM_S8_UINT] = DXGI_FORMAT_R24G8_TYPELESS,
   [PIPE_FORMAT_Z32_FLOAT_S8X24_UINT] = DXGI_FORMAT_R32G8X24_TYPELESS,
      };
      DXGI_FORMAT
   dzn_pipe_to_dxgi_format(enum pipe_format in)
   {
         }
      DXGI_FORMAT
   dzn_get_typeless_dxgi_format(DXGI_FORMAT in)
   {
      if (in >= DXGI_FORMAT_R32G32B32A32_TYPELESS && in <= DXGI_FORMAT_R32G32B32A32_SINT)
         if (in >= DXGI_FORMAT_R32G32B32_TYPELESS && in <= DXGI_FORMAT_R32G32B32_SINT)
         if (in >= DXGI_FORMAT_R16G16B16A16_TYPELESS && in <= DXGI_FORMAT_R16G16B16A16_SINT)
         if (in >= DXGI_FORMAT_R32G32_TYPELESS && in <= DXGI_FORMAT_R32G32_SINT)
         if (in >= DXGI_FORMAT_R32G8X24_TYPELESS && in <= DXGI_FORMAT_X32_TYPELESS_G8X24_UINT)
         if (in >= DXGI_FORMAT_R10G10B10A2_TYPELESS && in <= DXGI_FORMAT_R10G10B10A2_UINT)
         if (in >= DXGI_FORMAT_R8G8B8A8_TYPELESS && in <= DXGI_FORMAT_R8G8B8A8_SINT)
         if (in >= DXGI_FORMAT_R16G16_TYPELESS && in <= DXGI_FORMAT_R16G16_SINT)
         if (in >= DXGI_FORMAT_R32_TYPELESS && in <= DXGI_FORMAT_R32_SINT)
         if (in >= DXGI_FORMAT_R24G8_TYPELESS && in <= DXGI_FORMAT_X24_TYPELESS_G8_UINT)
         if (in >= DXGI_FORMAT_R8G8_TYPELESS && in <= DXGI_FORMAT_R8G8_SINT)
         if (in >= DXGI_FORMAT_R16_TYPELESS && in <= DXGI_FORMAT_R16_SINT)
         if (in >= DXGI_FORMAT_R8_TYPELESS && in <= DXGI_FORMAT_R8_SINT)
         if (in >= DXGI_FORMAT_BC1_TYPELESS && in <= DXGI_FORMAT_BC1_UNORM_SRGB)
         if (in >= DXGI_FORMAT_BC2_TYPELESS && in <= DXGI_FORMAT_BC2_UNORM_SRGB)
         if (in >= DXGI_FORMAT_BC3_TYPELESS && in <= DXGI_FORMAT_BC3_UNORM_SRGB)
         if (in >= DXGI_FORMAT_BC4_TYPELESS && in <= DXGI_FORMAT_BC4_SNORM)
         if (in >= DXGI_FORMAT_BC5_TYPELESS && in <= DXGI_FORMAT_BC5_SNORM)
         if (in == DXGI_FORMAT_B8G8R8A8_UNORM ||
      (in >= DXGI_FORMAT_B8G8R8A8_TYPELESS && in <= DXGI_FORMAT_B8G8R8A8_UNORM_SRGB))
      if (in == DXGI_FORMAT_B8G8R8X8_UNORM ||
      (in >= DXGI_FORMAT_B8G8R8X8_TYPELESS && in <= DXGI_FORMAT_B8G8R8X8_UNORM_SRGB))
      if (in >= DXGI_FORMAT_BC6H_TYPELESS && in <= DXGI_FORMAT_BC6H_SF16)
         if (in >= DXGI_FORMAT_BC7_TYPELESS && in <= DXGI_FORMAT_BC7_UNORM_SRGB)
               }
      static D3D12_FILTER_TYPE
   translate_filter_type(VkFilter type)
   {
      switch (type) {
   case VK_FILTER_NEAREST: return D3D12_FILTER_TYPE_POINT;
   case VK_FILTER_LINEAR: return D3D12_FILTER_TYPE_LINEAR;
   default:
      assert(!"Unsupported filter mode");
         }
      static D3D12_FILTER_TYPE
   translate_mip_filter_type(VkSamplerMipmapMode type)
   {
      switch (type) {
   case VK_SAMPLER_MIPMAP_MODE_NEAREST: return D3D12_FILTER_TYPE_POINT;
   case VK_SAMPLER_MIPMAP_MODE_LINEAR: return D3D12_FILTER_TYPE_LINEAR;
   default:
      assert(!"Unsupported filter mode");
         }
      D3D12_FILTER
   dzn_translate_sampler_filter(const struct dzn_physical_device *pdev,
         {
      D3D12_FILTER_REDUCTION_TYPE reduction = create_info->compareEnable ?
            if (create_info->anisotropyEnable) {
      if (create_info->mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST &&
      pdev->options19.AnisoFilterWithPointMipSupported)
                  return D3D12_ENCODE_BASIC_FILTER(
      translate_filter_type(create_info->minFilter),
   translate_filter_type(create_info->magFilter),
   translate_mip_filter_type(create_info->mipmapMode),
   }
      D3D12_COMPARISON_FUNC
   dzn_translate_compare_op(VkCompareOp in)
   {
      switch (in) {
   case VK_COMPARE_OP_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
   case VK_COMPARE_OP_LESS: return D3D12_COMPARISON_FUNC_LESS;
   case VK_COMPARE_OP_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
   case VK_COMPARE_OP_LESS_OR_EQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
   case VK_COMPARE_OP_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
   case VK_COMPARE_OP_NOT_EQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
   case VK_COMPARE_OP_GREATER_OR_EQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
   case VK_COMPARE_OP_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
   default: unreachable("Invalid compare op");
      }
      void
   dzn_translate_viewport(D3D12_VIEWPORT *out,
         {
      out->TopLeftX = in->x;
   out->TopLeftY = in->height < 0 ? in->height + in->y : in->y;
   out->Width = in->width;
   out->Height = fabs(in->height);
   out->MinDepth = MIN2(in->minDepth, in->maxDepth);
      }
      void
   dzn_translate_rect(D3D12_RECT *out,
         {
      out->left = in->offset.x;
   out->top = in->offset.y;
   out->right = in->offset.x + in->extent.width;
      }
      static ID3D12Debug *
   get_debug_interface(struct util_dl_library *d3d12_mod, ID3D12DeviceFactory *factory)
   {
      typedef HRESULT(WINAPI *PFN_D3D12_GET_DEBUG_INTERFACE)(REFIID riid, void **ppFactory);
            ID3D12Debug *debug;
   if (factory) {
      if (FAILED(ID3D12DeviceFactory_GetConfigurationInterface(factory, &CLSID_D3D12Debug, &IID_ID3D12Debug, (void **)&debug))) {
      mesa_loge("Failed to retrieve ID3D12Debug from device factory\n");
         } else {
      D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)util_dl_get_proc_address(d3d12_mod, "D3D12GetDebugInterface");
   if (!D3D12GetDebugInterface) {
      mesa_loge("failed to load D3D12GetDebugInterface from D3D12.DLL\n");
               if (FAILED(D3D12GetDebugInterface(&IID_ID3D12Debug, (void **)&debug))) {
      mesa_loge("D3D12GetDebugInterface failed\n");
                     }
      void
   d3d12_enable_debug_layer(struct util_dl_library *d3d12_mod, ID3D12DeviceFactory *factory)
   {
      ID3D12Debug *debug = get_debug_interface(d3d12_mod, factory);
   if (debug) {
      ID3D12Debug_EnableDebugLayer(debug);
         }
      void
   d3d12_enable_gpu_validation(struct util_dl_library *d3d12_mod, ID3D12DeviceFactory *factory)
   {
      ID3D12Debug *debug = get_debug_interface(d3d12_mod, factory);
   if (debug) {
      ID3D12Debug3 *debug3;
   if (SUCCEEDED(ID3D12Debug_QueryInterface(debug,
                  ID3D12Debug3_SetEnableGPUBasedValidation(debug3, true);
      }
         }
      ID3D12Device4 *
   d3d12_create_device(struct util_dl_library *d3d12_mod, IUnknown *adapter, ID3D12DeviceFactory *factory, bool experimental_features)
   {
   #ifdef _WIN32
         #endif
      {
      if (factory) {
      if (FAILED(ID3D12DeviceFactory_EnableExperimentalFeatures(factory, 1, &D3D12ExperimentalShaderModels, NULL, NULL))) {
      mesa_loge("failed to enable experimental shader models\n");
         } else {
      typedef HRESULT(WINAPI *PFN_D3D12ENABLEEXPERIMENTALFEATURES)(UINT, const IID *, void *, UINT *);
   PFN_D3D12ENABLEEXPERIMENTALFEATURES D3D12EnableExperimentalFeatures =
         if (FAILED(D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModels, NULL, NULL))) {
      mesa_loge("failed to enable experimental shader models\n");
                     ID3D12Device4 *dev;
   if (factory) {
      ID3D12DeviceFactory_SetFlags(factory, D3D12_DEVICE_FACTORY_FLAG_ALLOW_RETURNING_EXISTING_DEVICE |
         if (FAILED(ID3D12DeviceFactory_CreateDevice(factory, adapter, D3D_FEATURE_LEVEL_11_0,
                  mesa_loge("ID3D12DeviceFactory::CreateDevice failed\n");
         } else {
      typedef HRESULT(WINAPI *PFN_D3D12CREATEDEVICE)(IUnknown *, D3D_FEATURE_LEVEL, REFIID, void **);
            D3D12CreateDevice = (PFN_D3D12CREATEDEVICE)util_dl_get_proc_address(d3d12_mod, "D3D12CreateDevice");
   if (!D3D12CreateDevice) {
      mesa_loge("failed to load D3D12CreateDevice from D3D12\n");
               if (FAILED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0,
                  mesa_loge("D3D12CreateDevice failed\n");
         }
      }
      PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE
   d3d12_get_serialize_root_sig(struct util_dl_library *d3d12_mod)
   {
      return (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)
      }
