   /**************************************************************************
   *
   * Copyright 2012-2021 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
      /*
   * Shader.cpp --
   *    Functions that manipulate shader resources.
   */
         #include "Shader.h"
   #include "ShaderParse.h"
   #include "State.h"
   #include "Query.h"
      #include "Debug.h"
   #include "Format.h"
      #include "tgsi/tgsi_ureg.h"
   #include "util/u_gen_mipmap.h"
   #include "util/u_sampler.h"
   #include "util/format/u_format.h"
         /*
   * ----------------------------------------------------------------------
   *
   * CreateEmptyShader --
   *
   *    Update the driver's currently bound constant buffers.
   *
   * ----------------------------------------------------------------------
   */
      void *
   CreateEmptyShader(Device *pDevice,
         {
      struct pipe_context *pipe = pDevice->pipe;
   struct ureg_program *ureg;
   const struct tgsi_token *tokens;
            if (processor == PIPE_SHADER_GEOMETRY) {
                  ureg = ureg_create(processor);
   if (!ureg)
                     tokens = ureg_get_tokens(ureg, &nr_tokens);
   if (!tokens)
                     struct pipe_shader_state state;
   memset(&state, 0, sizeof state);
            void *handle;
   switch (processor) {
   case PIPE_SHADER_FRAGMENT:
      handle = pipe->create_fs_state(pipe, &state);
      case PIPE_SHADER_VERTEX:
      handle = pipe->create_vs_state(pipe, &state);
      case PIPE_SHADER_GEOMETRY:
      handle = pipe->create_gs_state(pipe, &state);
      default:
      handle = NULL;
      }
                        }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateEmptyShader --
   *
   *    Update the driver's currently bound constant buffers.
   *
   * ----------------------------------------------------------------------
   */
      void
   DeleteEmptyShader(Device *pDevice,
         {
               if (processor == PIPE_SHADER_GEOMETRY) {
      assert(handle == NULL);
               assert(handle != NULL);
   switch (processor) {
   case PIPE_SHADER_FRAGMENT:
      pipe->delete_fs_state(pipe, handle);
      case PIPE_SHADER_VERTEX:
      pipe->delete_vs_state(pipe, handle);
      case PIPE_SHADER_GEOMETRY:
      pipe->delete_gs_state(pipe, handle);
      default:
            }
         /*
   * ----------------------------------------------------------------------
   *
   * SetConstantBuffers --
   *
   *    Update the driver's currently bound constant buffers.
   *
   * ----------------------------------------------------------------------
   */
      static void
   SetConstantBuffers(enum pipe_shader_type shader_type,    // IN
                     D3D10DDI_HDEVICE hDevice,             // IN
   {
      Device *pDevice = CastDevice(hDevice);
            for (UINT i = 0; i < NumBuffers; i++) {
      struct pipe_constant_buffer cb;
   memset(&cb, 0, sizeof cb);
   cb.buffer = CastPipeResource(phBuffers[i]);
   cb.buffer_offset = 0;
   cb.buffer_size = cb.buffer ? cb.buffer->width0 : 0;
   pipe->set_constant_buffer(pipe,
                           }
         /*
   * ----------------------------------------------------------------------
   *
   * SetSamplers --
   *
   *    Update the driver's currently bound sampler state.
   *
   * ----------------------------------------------------------------------
   */
      static void
   SetSamplers(enum pipe_shader_type shader_type,     // IN
               D3D10DDI_HDEVICE hDevice,              // IN
   UINT Offset,                          // IN
   {
      Device *pDevice = CastDevice(hDevice);
            void **samplers = pDevice->samplers[shader_type];
   for (UINT i = 0; i < NumSamplers; i++) {
      assert(Offset + i < PIPE_MAX_SAMPLERS);
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * SetSamplers --
   *
   *    Update the driver's currently bound sampler state.
   *
   * ----------------------------------------------------------------------
   */
      static void
   SetShaderResources(enum pipe_shader_type shader_type,                  // IN
                     D3D10DDI_HDEVICE hDevice,                                   // IN
   {
      Device *pDevice = CastDevice(hDevice);
                     struct pipe_sampler_view **sampler_views = pDevice->sampler_views[shader_type];
   for (UINT i = 0; i < NumViews; i++) {
      struct pipe_sampler_view *sampler_view =
         if (Offset + i < PIPE_MAX_SHADER_SAMPLER_VIEWS) {
         } else {
      if (sampler_view) {
      LOG_UNSUPPORTED(true);
                     /*
   * XXX: Now that the semantics are actually the same in gallium, should
   * probably think about not updating all always... It should just work.
   */
   pipe->set_sampler_views(pipe, shader_type, 0, PIPE_MAX_SHADER_SAMPLER_VIEWS,
      }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateShaderSize --
   *
   *    The CalcPrivateShaderSize function determines the size of
   *    the user-mode display driver's private region of memory
   *    (that is, the size of internal driver structures, not the
   *    size of the resource video memory) for a shader.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateShaderSize(D3D10DDI_HDEVICE hDevice,                                  // IN
               {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroyShader --
   *
   *    The DestroyShader function destroys the specified shader object.
   *    The shader object can be destoyed only if it is not currently
   *    bound to a display device.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DestroyShader(D3D10DDI_HDEVICE hDevice,   // IN
         {
               struct pipe_context *pipe = CastPipeContext(hDevice);
            if (pShader->handle) {
      switch (pShader->type) {
   case PIPE_SHADER_FRAGMENT:
      pipe->delete_fs_state(pipe, pShader->handle);
      case PIPE_SHADER_VERTEX:
      pipe->delete_vs_state(pipe, pShader->handle);
      case PIPE_SHADER_GEOMETRY:
      pipe->delete_gs_state(pipe, pShader->handle);
      default:
                     if (pShader->state.tokens) {
            }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateSamplerSize --
   *
   *    The CalcPrivateSamplerSize function determines the size of the
   *    user-mode display driver's private region of memory (that is,
   *    the size of internal driver structures, not the size of the
   *    resource video memory) for a sampler.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateSamplerSize(D3D10DDI_HDEVICE hDevice,                        // IN
         {
         }
         static uint
   translate_address_mode(D3D10_DDI_TEXTURE_ADDRESS_MODE AddressMode)
   {
      switch (AddressMode) {
   case D3D10_DDI_TEXTURE_ADDRESS_WRAP:
         case D3D10_DDI_TEXTURE_ADDRESS_MIRROR:
         case D3D10_DDI_TEXTURE_ADDRESS_CLAMP:
         case D3D10_DDI_TEXTURE_ADDRESS_BORDER:
         case D3D10_DDI_TEXTURE_ADDRESS_MIRRORONCE:
         default:
      assert(0);
         }
      static uint
   translate_comparison(D3D10_DDI_COMPARISON_FUNC Func)
   {
      switch (Func) {
   case D3D10_DDI_COMPARISON_NEVER:
         case D3D10_DDI_COMPARISON_LESS:
         case D3D10_DDI_COMPARISON_EQUAL:
         case D3D10_DDI_COMPARISON_LESS_EQUAL:
         case D3D10_DDI_COMPARISON_GREATER:
         case D3D10_DDI_COMPARISON_NOT_EQUAL:
         case D3D10_DDI_COMPARISON_GREATER_EQUAL:
         case D3D10_DDI_COMPARISON_ALWAYS:
         default:
      assert(0);
         }
      static uint
   translate_filter(D3D10_DDI_FILTER_TYPE Filter)
   {
      switch (Filter) {
   case D3D10_DDI_FILTER_TYPE_POINT:
         case D3D10_DDI_FILTER_TYPE_LINEAR:
         default:
      assert(0);
         }
      static uint
   translate_min_filter(D3D10_DDI_FILTER Filter)
   {
         }
      static uint
   translate_mag_filter(D3D10_DDI_FILTER Filter)
   {
         }
      /* Gallium uses a different enum for mipfilters, to accomodate the GL
   * MIPFILTER_NONE mode.
   */
   static uint
   translate_mip_filter(D3D10_DDI_FILTER Filter)
   {
      switch (D3D10_DDI_DECODE_MIP_FILTER(Filter)) {
   case D3D10_DDI_FILTER_TYPE_POINT:
         case D3D10_DDI_FILTER_TYPE_LINEAR:
         default:
      assert(0);
         }
      /*
   * ----------------------------------------------------------------------
   *
   * CreateSampler --
   *
   *    The CreateSampler function creates a sampler.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateSampler(D3D10DDI_HDEVICE hDevice,                        // IN
               __in const D3D10_DDI_SAMPLER_DESC *pSamplerDesc, // IN
   {
               struct pipe_context *pipe = CastPipeContext(hDevice);
                              /* d3d10 has seamless cube filtering always enabled */
            /* Wrapping modes. */
   state.wrap_s = translate_address_mode(pSamplerDesc->AddressU);
   state.wrap_t = translate_address_mode(pSamplerDesc->AddressV);
            /* Filtering */
   state.min_img_filter = translate_min_filter(pSamplerDesc->Filter);
   state.mag_img_filter = translate_mag_filter(pSamplerDesc->Filter);
            if (D3D10_DDI_DECODE_IS_ANISOTROPIC_FILTER(pSamplerDesc->Filter)) {
                  /* XXX: Handle the following bit.
   */
            /* Comparison. */
   if (D3D10_DDI_DECODE_IS_COMPARISON_FILTER(pSamplerDesc->Filter)) {
      state.compare_mode = PIPE_TEX_COMPARE_R_TO_TEXTURE;
               /* Level of detail. */
   state.lod_bias = pSamplerDesc->MipLODBias;
   state.min_lod = pSamplerDesc->MinLOD;
            /* Border color. */
   state.border_color.f[0] = pSamplerDesc->BorderColor[0];
   state.border_color.f[1] = pSamplerDesc->BorderColor[1];
   state.border_color.f[2] = pSamplerDesc->BorderColor[2];
               }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroySampler --
   *
   *    The DestroySampler function destroys the specified sampler object.
   *    The sampler object can be destoyed only if it is not currently
   *    bound to a display device.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DestroySampler(D3D10DDI_HDEVICE hDevice,     // IN
         {
               struct pipe_context *pipe = CastPipeContext(hDevice);
               }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateVertexShader --
   *
   *    The CreateVertexShader function creates a vertex shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateVertexShader(D3D10DDI_HDEVICE hDevice,                                  // IN
                     __in_ecount (pShaderCode[1]) const UINT *pCode,            // IN
   {
               struct pipe_context *pipe = CastPipeContext(hDevice);
            pShader->type = PIPE_SHADER_VERTEX;
            memset(&pShader->state, 0, sizeof pShader->state);
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * VsSetShader --
   *
   *    The VsSetShader function sets the vertex shader code so that all
   *    of the subsequent drawing operations use that code.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   VsSetShader(D3D10DDI_HDEVICE hDevice,  // IN
         {
               Device *pDevice = CastDevice(hDevice);
   struct pipe_context *pipe = pDevice->pipe;
   Shader *pShader = CastShader(hShader);
            pDevice->bound_vs = pShader;
   if (!state) {
                     }
         /*
   * ----------------------------------------------------------------------
   *
   * VsSetShaderResources --
   *
   *    The VsSetShaderResources function sets resources for a
   *    vertex shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   VsSetShaderResources(D3D10DDI_HDEVICE hDevice,                                   // IN
                           {
                     }
         /*
   * ----------------------------------------------------------------------
   *
   * VsSetConstantBuffers --
   *
   *    The VsSetConstantBuffers function sets constant buffers
   *    for a vertex shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   VsSetConstantBuffers(D3D10DDI_HDEVICE hDevice,                                      // IN
                     {
               SetConstantBuffers(PIPE_SHADER_VERTEX,
      }
         /*
   * ----------------------------------------------------------------------
   *
   * VsSetSamplers --
   *
   *    The VsSetSamplers function sets samplers for a vertex shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   VsSetSamplers(D3D10DDI_HDEVICE hDevice,                                       // IN
               UINT Offset,                                                    // IN
   {
                     }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateGeometryShader --
   *
   *    The CreateGeometryShader function creates a geometry shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateGeometryShader(D3D10DDI_HDEVICE hDevice,                                // IN
                           {
               struct pipe_context *pipe = CastPipeContext(hDevice);
            pShader->type = PIPE_SHADER_GEOMETRY;
            memset(&pShader->state, 0, sizeof pShader->state);
               }
         /*
   * ----------------------------------------------------------------------
   *
   * GsSetShader --
   *
   *    The GsSetShader function sets the geometry shader code so that
   *    all of the subsequent drawing operations use that code.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   GsSetShader(D3D10DDI_HDEVICE hDevice,  // IN
         {
               Device *pDevice = CastDevice(hDevice);
   struct pipe_context *pipe = CastPipeContext(hDevice);
   void *state = CastPipeShader(hShader);
                     if (pShader && !pShader->state.tokens) {
         } else {
      pDevice->bound_empty_gs = NULL;
         }
         /*
   * ----------------------------------------------------------------------
   *
   * GsSetShaderResources --
   *
   *    The GsSetShaderResources function sets resources for a
   *    geometry shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   GsSetShaderResources(D3D10DDI_HDEVICE hDevice,                                   // IN
                           {
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * GsSetConstantBuffers --
   *
   *    The GsSetConstantBuffers function sets constant buffers for
   *    a geometry shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   GsSetConstantBuffers(D3D10DDI_HDEVICE hDevice,                                      // IN
                     {
               SetConstantBuffers(PIPE_SHADER_GEOMETRY,
      }
         /*
   * ----------------------------------------------------------------------
   *
   * GsSetSamplers --
   *
   *    The GsSetSamplers function sets samplers for a geometry shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   GsSetSamplers(D3D10DDI_HDEVICE hDevice,                                       // IN
               UINT Offset,                                                    // IN
   {
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateGeometryShaderWithStreamOutput --
   *
   *    The CalcPrivateGeometryShaderWithStreamOutput function determines
   *    the size of the user-mode display driver's private region of memory
   *    (that is, the size of internal driver structures, not the size of
   *    the resource video memory) for a geometry shader with stream output.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateGeometryShaderWithStreamOutput(
      D3D10DDI_HDEVICE hDevice,                                                                             // IN
   __in const D3D10DDIARG_CREATEGEOMETRYSHADERWITHSTREAMOUTPUT *pCreateGeometryShaderWithStreamOutput,   // IN
      {
      LOG_ENTRYPOINT();
      }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateGeometryShaderWithStreamOutput --
   *
   *    The CreateGeometryShaderWithStreamOutput function creates a
   *    geometry shader with stream output.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateGeometryShaderWithStreamOutput(
      D3D10DDI_HDEVICE hDevice,                                                                             // IN
   __in const D3D10DDIARG_CREATEGEOMETRYSHADERWITHSTREAMOUTPUT *pData,   // IN
   D3D10DDI_HSHADER hShader,                                                                             // IN
   D3D10DDI_HRTSHADER hRTShader,                                                                         // IN
      {
               struct pipe_context *pipe = CastPipeContext(hDevice);
   Shader *pShader = CastShader(hShader);
   int total_components[PIPE_MAX_SO_BUFFERS] = {0};
   unsigned num_holes = 0;
                     memset(&pShader->state, 0, sizeof pShader->state);
   if (pData->pShaderCode) {
      pShader->state.tokens = Shader_tgsi_translate(pData->pShaderCode,
      }
            for (unsigned i = 0; i < pData->NumEntries; ++i) {
      CONST D3D10DDIARG_STREAM_OUTPUT_DECLARATION_ENTRY* pOutputStreamDecl =
         BYTE RegisterMask = pOutputStreamDecl->RegisterMask;
   unsigned start_component = 0;
   unsigned num_components = 0;
   if (RegisterMask) {
      while ((RegisterMask & 1) == 0) {
      ++start_component;
      }
   while (RegisterMask) {
      ++num_components;
      }
   assert(start_component < 4);
   assert(1 <= num_components && num_components <= 4);
   LOG_UNSUPPORTED(((1 << num_components) - 1) << start_component !=
               if (pOutputStreamDecl->RegisterIndex == 0xffffffff) {
         } else {
      unsigned idx = i - num_holes;
   pShader->state.stream_output.output[idx].start_component =
         pShader->state.stream_output.output[idx].num_components =
         pShader->state.stream_output.output[idx].output_buffer =
         pShader->state.stream_output.output[idx].register_index =
         pShader->state.stream_output.output[idx].dst_offset =
         if (pOutputStreamDecl->OutputSlot != 0)
      }
      }
   pShader->state.stream_output.num_outputs = pData->NumEntries - num_holes;
   for (unsigned i = 0; i < PIPE_MAX_SO_BUFFERS; ++i) {
      /* stream_output.stride[i] is in dwords */
   if (all_slot_zero) {
      pShader->state.stream_output.stride[i] =
      } else {
                        }
         /*
   * ----------------------------------------------------------------------
   *
   * SoSetTargets --
   *
   *    The SoSetTargets function sets stream output target resources.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   SoSetTargets(D3D10DDI_HDEVICE hDevice,                                     // IN
               UINT SOTargets,                                               // IN
   UINT ClearTargets,                                            // IN
   {
                        Device *pDevice = CastDevice(hDevice);
                     for (i = 0; i < SOTargets; ++i) {
      Resource *resource = CastResource(phResource[i]);
   struct pipe_resource *buffer = CastPipeResource(phResource[i]);
   struct pipe_stream_output_target *so_target =
            if (buffer) {
               if (!so_target ||
      so_target->buffer != buffer ||
   so_target->buffer_size != buffer_size) {
   if (so_target) {
         }
   so_target = pipe->create_stream_output_target(pipe, buffer,
                     }
               for (i = 0; i < ClearTargets; ++i) {
                  if (!pipe->set_stream_output_targets) {
      LOG_UNSUPPORTED(pipe->set_stream_output_targets);
               pipe->set_stream_output_targets(pipe, SOTargets, pDevice->so_targets,
      }
         /*
   * ----------------------------------------------------------------------
   *
   * CreatePixelShader --
   *
   *    The CreatePixelShader function converts pixel shader code into a
   *    hardware-specific format and associates this code with a
   *    shader handle.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreatePixelShader(D3D10DDI_HDEVICE hDevice,                                // IN
                     __in_ecount (pShaderCode[1]) const UINT *pShaderCode,    // IN
   {
               struct pipe_context *pipe = CastPipeContext(hDevice);
            pShader->type = PIPE_SHADER_FRAGMENT;
            memset(&pShader->state, 0, sizeof pShader->state);
   pShader->state.tokens = Shader_tgsi_translate(pShaderCode,
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * PsSetShader --
   *
   *    The PsSetShader function sets a pixel shader to be used
   *    in all drawing operations.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   PsSetShader(D3D10DDI_HDEVICE hDevice,  // IN
         {
               Device *pDevice = CastDevice(hDevice);
   struct pipe_context *pipe = pDevice->pipe;
            if (!state) {
                     }
         /*
   * ----------------------------------------------------------------------
   *
   * PsSetShaderResources --
   *
   *    The PsSetShaderResources function sets resources for a pixel shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   PsSetShaderResources(D3D10DDI_HDEVICE hDevice,                                   // IN
                           {
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * PsSetConstantBuffers --
   *
   *    The PsSetConstantBuffers function sets constant buffers for
   *    a pixel shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   PsSetConstantBuffers(D3D10DDI_HDEVICE hDevice,                                      // IN
                     {
               SetConstantBuffers(PIPE_SHADER_FRAGMENT,
      }
      /*
   * ----------------------------------------------------------------------
   *
   * PsSetSamplers --
   *
   *    The PsSetSamplers function sets samplers for a pixel shader.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   PsSetSamplers(D3D10DDI_HDEVICE hDevice,                                       // IN
               UINT Offset,                                                    // IN
   {
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * ShaderResourceViewReadAfterWriteHazard --
   *
   *    The ShaderResourceViewReadAfterWriteHazard function informs
   *    the usermode display driver that the specified resource was
   *    used as an output from the graphics processing unit (GPU)
   *    and that the resource will be used as an input to the GPU.
   *    A shader resource view is also provided to indicate which
   *    view caused the hazard.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   ShaderResourceViewReadAfterWriteHazard(D3D10DDI_HDEVICE hDevice,                          // IN
               {
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateShaderResourceViewSize --
   *
   *    The CalcPrivateShaderResourceViewSize function determines the size
   *    of the usermode display driver's private region of memory
   *    (that is, the size of internal driver structures, not the size of
   *    the resource video memory) for a shader resource view.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateShaderResourceViewSize(
      D3D10DDI_HDEVICE hDevice,                                                     // IN
      {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateShaderResourceViewSize --
   *
   *    The CalcPrivateShaderResourceViewSize function determines the size
   *    of the usermode display driver's private region of memory
   *    (that is, the size of internal driver structures, not the size of
   *    the resource video memory) for a shader resource view.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateShaderResourceViewSize1(
      D3D10DDI_HDEVICE hDevice,                                                     // IN
      {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateShaderResourceView --
   *
   *    The CreateShaderResourceView function creates a shader
   *    resource view.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateShaderResourceView(
      D3D10DDI_HDEVICE hDevice,                                                     // IN
   __in const D3D10DDIARG_CREATESHADERRESOURCEVIEW *pCreateSRView,   // IN
   D3D10DDI_HSHADERRESOURCEVIEW hShaderResourceView,                             // IN
      {
               struct pipe_context *pipe = CastPipeContext(hDevice);
   ShaderResourceView *pSRView = CastShaderResourceView(hShaderResourceView);
   struct pipe_resource *resource;
            struct pipe_sampler_view desc;
   memset(&desc, 0, sizeof desc);
   resource = CastPipeResource(pCreateSRView->hDrvResource);
            u_sampler_view_default_template(&desc,
                  switch (pCreateSRView->ResourceDimension) {
   case D3D10DDIRESOURCE_BUFFER: {
         const struct util_format_description *fdesc = util_format_description(format);
   desc.u.buf.offset = pCreateSRView->Buffer.FirstElement *
         desc.u.buf.size = pCreateSRView->Buffer.NumElements *
      }
      case D3D10DDIRESOURCE_TEXTURE1D:
      desc.u.tex.first_level = pCreateSRView->Tex1D.MostDetailedMip;
   desc.u.tex.last_level = pCreateSRView->Tex1D.MipLevels - 1 + desc.u.tex.first_level;
   desc.u.tex.first_layer = pCreateSRView->Tex1D.FirstArraySlice;
   desc.u.tex.last_layer = pCreateSRView->Tex1D.ArraySize - 1 + desc.u.tex.first_layer;
   assert(pCreateSRView->Tex1D.MipLevels != 0 && pCreateSRView->Tex1D.MipLevels != (UINT)-1);
   assert(pCreateSRView->Tex1D.ArraySize != 0 && pCreateSRView->Tex1D.ArraySize != (UINT)-1);
      case D3D10DDIRESOURCE_TEXTURE2D:
      desc.u.tex.first_level = pCreateSRView->Tex2D.MostDetailedMip;
   desc.u.tex.last_level = pCreateSRView->Tex2D.MipLevels - 1 + desc.u.tex.first_level;
   desc.u.tex.first_layer = pCreateSRView->Tex2D.FirstArraySlice;
   desc.u.tex.last_layer = pCreateSRView->Tex2D.ArraySize - 1 + desc.u.tex.first_layer;
   assert(pCreateSRView->Tex2D.MipLevels != 0 && pCreateSRView->Tex2D.MipLevels != (UINT)-1);
   assert(pCreateSRView->Tex2D.ArraySize != 0 && pCreateSRView->Tex2D.ArraySize != (UINT)-1);
      case D3D10DDIRESOURCE_TEXTURE3D:
      desc.u.tex.first_level = pCreateSRView->Tex3D.MostDetailedMip;
   desc.u.tex.last_level = pCreateSRView->Tex3D.MipLevels - 1 + desc.u.tex.first_level;
   /* layer info filled in by default_template */
   assert(pCreateSRView->Tex3D.MipLevels != 0 && pCreateSRView->Tex3D.MipLevels != (UINT)-1);
      case D3D10DDIRESOURCE_TEXTURECUBE:
      desc.u.tex.first_level = pCreateSRView->TexCube.MostDetailedMip;
   desc.u.tex.last_level = pCreateSRView->TexCube.MipLevels - 1 + desc.u.tex.first_level;
   /* layer info filled in by default_template */
   assert(pCreateSRView->TexCube.MipLevels != 0 && pCreateSRView->TexCube.MipLevels != (UINT)-1);
      default:
      assert(0);
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateShaderResourceView1 --
   *
   *    The CreateShaderResourceView function creates a shader
   *    resource view.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateShaderResourceView1(
      D3D10DDI_HDEVICE hDevice,                                                     // IN
   __in const D3D10_1DDIARG_CREATESHADERRESOURCEVIEW *pCreateSRView,   // IN
   D3D10DDI_HSHADERRESOURCEVIEW hShaderResourceView,                             // IN
      {
               struct pipe_context *pipe = CastPipeContext(hDevice);
   ShaderResourceView *pSRView = CastShaderResourceView(hShaderResourceView);
   struct pipe_resource *resource;
            struct pipe_sampler_view desc;
   memset(&desc, 0, sizeof desc);
   resource = CastPipeResource(pCreateSRView->hDrvResource);
            u_sampler_view_default_template(&desc,
                  switch (pCreateSRView->ResourceDimension) {
   case D3D10DDIRESOURCE_BUFFER: {
         const struct util_format_description *fdesc = util_format_description(format);
   desc.u.buf.offset = pCreateSRView->Buffer.FirstElement *
         desc.u.buf.size = pCreateSRView->Buffer.NumElements *
      }
      case D3D10DDIRESOURCE_TEXTURE1D:
      desc.u.tex.first_level = pCreateSRView->Tex1D.MostDetailedMip;
   desc.u.tex.last_level = pCreateSRView->Tex1D.MipLevels - 1 + desc.u.tex.first_level;
   desc.u.tex.first_layer = pCreateSRView->Tex1D.FirstArraySlice;
   desc.u.tex.last_layer = pCreateSRView->Tex1D.ArraySize - 1 + desc.u.tex.first_layer;
   assert(pCreateSRView->Tex1D.MipLevels != 0 && pCreateSRView->Tex1D.MipLevels != (UINT)-1);
   assert(pCreateSRView->Tex1D.ArraySize != 0 && pCreateSRView->Tex1D.ArraySize != (UINT)-1);
      case D3D10DDIRESOURCE_TEXTURE2D:
      desc.u.tex.first_level = pCreateSRView->Tex2D.MostDetailedMip;
   desc.u.tex.last_level = pCreateSRView->Tex2D.MipLevels - 1 + desc.u.tex.first_level;
   desc.u.tex.first_layer = pCreateSRView->Tex2D.FirstArraySlice;
   desc.u.tex.last_layer = pCreateSRView->Tex2D.ArraySize - 1 + desc.u.tex.first_layer;
   assert(pCreateSRView->Tex2D.MipLevels != 0 && pCreateSRView->Tex2D.MipLevels != (UINT)-1);
   assert(pCreateSRView->Tex2D.ArraySize != 0 && pCreateSRView->Tex2D.ArraySize != (UINT)-1);
      case D3D10DDIRESOURCE_TEXTURE3D:
      desc.u.tex.first_level = pCreateSRView->Tex3D.MostDetailedMip;
   desc.u.tex.last_level = pCreateSRView->Tex3D.MipLevels - 1 + desc.u.tex.first_level;
   /* layer info filled in by default_template */
   assert(pCreateSRView->Tex3D.MipLevels != 0 && pCreateSRView->Tex3D.MipLevels != (UINT)-1);
      case D3D10DDIRESOURCE_TEXTURECUBE:
      desc.u.tex.first_level = pCreateSRView->TexCube.MostDetailedMip;
   desc.u.tex.last_level = pCreateSRView->TexCube.MipLevels - 1 + desc.u.tex.first_level;
   desc.u.tex.first_layer = pCreateSRView->TexCube.First2DArrayFace;
   desc.u.tex.last_layer = 6*pCreateSRView->TexCube.NumCubes - 1 +
         assert(pCreateSRView->TexCube.MipLevels != 0 && pCreateSRView->TexCube.MipLevels != (UINT)-1);
      default:
      assert(0);
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroyShaderResourceView --
   *
   *    The DestroyShaderResourceView function destroys the specified
   *    shader resource view object. The shader resource view object
   *    can be destoyed only if it is not currently bound to a
   *    display device.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DestroyShaderResourceView(D3D10DDI_HDEVICE hDevice,                           // IN
         {
                           }
         /*
   * ----------------------------------------------------------------------
   *
   * GenMips --
   *
   *    The GenMips function generates the lower MIP-map levels
   *    on the specified shader-resource view.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   GenMips(D3D10DDI_HDEVICE hDevice,                           // IN
         {
               Device *pDevice = CastDevice(hDevice);
   if (!CheckPredicate(pDevice)) {
                  struct pipe_context *pipe = pDevice->pipe;
            util_gen_mipmap(pipe,
                  sampler_view->texture,
   sampler_view->format,
   sampler_view->u.tex.first_level,
   sampler_view->u.tex.last_level,
   }
         unsigned
   ShaderFindOutputMapping(Shader *shader, unsigned registerIndex)
   {
      if (!shader || !shader->state.tokens)
            for (unsigned i = 0; i < PIPE_MAX_SHADER_OUTPUTS; ++i) {
      if (shader->output_mapping[i] == registerIndex)
      }
      }
