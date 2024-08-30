   /**********************************************************
   * Copyright 2008-2013 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      /**
   * @file svga_cmd_vgpu10.c
   *
   * Command construction utility for the vgpu10 SVGA3D protocol.
   *
   * \author Mingcheng Chen
   * \author Brian Paul
   */
         #include "svga_winsys.h"
   #include "svga_resource_buffer.h"
   #include "svga_resource_texture.h"
   #include "svga_surface.h"
   #include "svga_cmd.h"
         /**
   * Emit a surface relocation for RenderTargetViewId
   */
   static void
   view_relocation(struct svga_winsys_context *swc, // IN
                     {
      if (surface) {
      struct svga_surface *s = svga_surface(surface);
   assert(s->handle);
      }
   else {
            }
         /**
   * Emit a surface relocation for a ResourceId.
   */
   static void
   surface_to_resourceid(struct svga_winsys_context *swc, // IN
                     {
      if (surface) {
         }
   else {
            }
         #define SVGA3D_CREATE_COMMAND(CommandName, CommandCode) \
   SVGA3dCmdDX##CommandName *cmd; \
   { \
      cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_##CommandCode, \
         if (!cmd) \
      }
      #define SVGA3D_CREATE_CMD_COUNT(CommandName, CommandCode, ElementClassName) \
   SVGA3dCmdDX##CommandName *cmd; \
   { \
      assert(count > 0); \
   cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_##CommandCode, \
               if (!cmd) \
      }
      #define SVGA3D_COPY_BASIC(VariableName) \
   { \
         }
      #define SVGA3D_COPY_BASIC_2(VariableName1, VariableName2) \
   { \
      SVGA3D_COPY_BASIC(VariableName1); \
      }
      #define SVGA3D_COPY_BASIC_3(VariableName1, VariableName2, VariableName3) \
   { \
      SVGA3D_COPY_BASIC_2(VariableName1, VariableName2); \
      }
      #define SVGA3D_COPY_BASIC_4(VariableName1, VariableName2, VariableName3, \
         { \
      SVGA3D_COPY_BASIC_2(VariableName1, VariableName2); \
      }
      #define SVGA3D_COPY_BASIC_5(VariableName1, VariableName2, VariableName3, \
         {\
      SVGA3D_COPY_BASIC_3(VariableName1, VariableName2, VariableName3); \
      }
      #define SVGA3D_COPY_BASIC_6(VariableName1, VariableName2, VariableName3, \
         {\
      SVGA3D_COPY_BASIC_3(VariableName1, VariableName2, VariableName3); \
      }
      #define SVGA3D_COPY_BASIC_7(VariableName1, VariableName2, VariableName3, \
               {\
      SVGA3D_COPY_BASIC_4(VariableName1, VariableName2, VariableName3, \
            }
      #define SVGA3D_COPY_BASIC_8(VariableName1, VariableName2, VariableName3, \
               {\
      SVGA3D_COPY_BASIC_4(VariableName1, VariableName2, VariableName3, \
         SVGA3D_COPY_BASIC_4(VariableName5, VariableName6, VariableName7, \
      }
      #define SVGA3D_COPY_BASIC_9(VariableName1, VariableName2, VariableName3, \
               {\
      SVGA3D_COPY_BASIC_5(VariableName1, VariableName2, VariableName3, \
         SVGA3D_COPY_BASIC_4(VariableName6, VariableName7, VariableName8, \
      }
         enum pipe_error
   SVGA3D_vgpu10_PredCopyRegion(struct svga_winsys_context *swc,
                                 {
      SVGA3dCmdDXPredCopyRegion *cmd =
      SVGA3D_FIFOReserve(swc,
                  if (!cmd)
            swc->surface_relocation(swc, &cmd->dstSid, NULL, dstSurf, SVGA_RELOC_WRITE);
   swc->surface_relocation(swc, &cmd->srcSid, NULL, srcSurf, SVGA_RELOC_READ);
   cmd->dstSubResource = dstSubResource;
   cmd->srcSubResource = srcSubResource;
                        }
         enum pipe_error
   SVGA3D_vgpu10_PredCopy(struct svga_winsys_context *swc,
               {
      SVGA3dCmdDXPredCopy *cmd =
      SVGA3D_FIFOReserve(swc,
                  if (!cmd)
            swc->surface_relocation(swc, &cmd->dstSid, NULL, dstSurf, SVGA_RELOC_WRITE);
                        }
      enum pipe_error
   SVGA3D_vgpu10_SetViewports(struct svga_winsys_context *swc,
               {
               cmd->pad0 = 0;
            swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_vgpu10_SetShader(struct svga_winsys_context *swc,
                     {
      SVGA3dCmdDXSetShader *cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
                     cmd->type = type;
   cmd->shaderId = shaderId;
               }
         enum pipe_error
   SVGA3D_vgpu10_SetShaderResources(struct svga_winsys_context *swc,
                                 {
      SVGA3dCmdDXSetShaderResources *cmd;
   SVGA3dShaderResourceViewId *cmd_ids;
            cmd = SVGA3D_FIFOReserve(swc,
                           if (!cmd)
               cmd->type = type;
            cmd_ids = (SVGA3dShaderResourceViewId *) (cmd + 1);
   for (i = 0; i < count; i++) {
      swc->surface_relocation(swc, cmd_ids + i, NULL, views[i],
                     swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_vgpu10_SetSamplers(struct svga_winsys_context *swc,
                           {
               SVGA3D_COPY_BASIC_2(startSampler, type);
            swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_vgpu10_ClearRenderTargetView(struct svga_winsys_context *swc,
               {
      SVGA3dCmdDXClearRenderTargetView *cmd;
            cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
               /* NOTE: The following is pretty tricky.  We need to emit a view/surface
   * relocation and we have to provide a pointer to an ID which lies in
   * the bounds of the command space which we just allocated.  However,
   * we then need to overwrite it with the original RenderTargetViewId.
   */
   view_relocation(swc, color_surf, &cmd->renderTargetViewId,
                           swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_vgpu10_SetRenderTargets(struct svga_winsys_context *swc,
                     {
      const unsigned surf_count = color_count + 1;
   SVGA3dCmdDXSetRenderTargets *cmd;
   SVGA3dRenderTargetViewId *ctarget;
   struct svga_surface *ss;
                     cmd = SVGA3D_FIFOReserve(swc,
                           if (!cmd)
            /* NOTE: See earlier comment about the tricky handling of the ViewIds.
            /* Depth / Stencil buffer */
   if (depth_stencil_surf) {
      ss = svga_surface(depth_stencil_surf);
   view_relocation(swc, depth_stencil_surf, &cmd->depthStencilViewId,
            }
   else {
      /* no depth/stencil buffer - still need a relocation */
   view_relocation(swc, NULL, &cmd->depthStencilViewId,
                     /* Color buffers */
   ctarget = (SVGA3dRenderTargetViewId *) &cmd[1];
   for (i = 0; i < color_count; i++) {
      if (color_surfs[i]) {
      ss = svga_surface(color_surfs[i]);
   view_relocation(swc, color_surfs[i], ctarget + i, SVGA_RELOC_WRITE);
      }
   else {
      view_relocation(swc, NULL, ctarget + i, SVGA_RELOC_WRITE);
                  swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_vgpu10_SetBlendState(struct svga_winsys_context *swc,
                     {
               SVGA3D_COPY_BASIC_2(blendId, sampleMask);
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetDepthStencilState(struct svga_winsys_context *swc,
               {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetRasterizerState(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetPredication(struct svga_winsys_context *swc,
               {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_SET_PREDICATION,
            if (!cmd)
            cmd->queryId = queryId;
   cmd->predicateValue = predicateValue;
   swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetSOTargets(struct svga_winsys_context *swc,
                     {
      SVGA3dCmdDXSetSOTargets *cmd;
   SVGA3dSoTarget *sot;
            cmd = SVGA3D_FIFOReserve(swc,
                              if (!cmd)
            cmd->pad0 = 0;
   sot = (SVGA3dSoTarget *)(cmd + 1);
   for (i = 0; i < count; i++, sot++) {
      if (surfaces[i]) {
      sot->offset = targets[i].offset;
   sot->sizeInBytes = targets[i].sizeInBytes;
   swc->surface_relocation(swc, &sot->sid, NULL, surfaces[i],
      }
   else {
      sot->offset = 0;
   sot->sizeInBytes = ~0u;
   swc->surface_relocation(swc, &sot->sid, NULL, NULL,
         }
   swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetScissorRects(struct svga_winsys_context *swc,
               {
               assert(count > 0);
   cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_SET_SCISSORRECTS,
                     if (!cmd)
            cmd->pad0 = 0;
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetStreamOutput(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_Draw(struct svga_winsys_context *swc,
               {
                        swc->hints |= SVGA_HINT_FLAG_CAN_PRE_FLUSH;
   swc->commit(swc);
   swc->num_draw_commands++;
      }
      enum pipe_error
   SVGA3D_vgpu10_DrawIndexed(struct svga_winsys_context *swc,
                     {
               SVGA3D_COPY_BASIC_3(indexCount, startIndexLocation,
            swc->hints |= SVGA_HINT_FLAG_CAN_PRE_FLUSH;
   swc->commit(swc);
   swc->num_draw_commands++;
      }
      enum pipe_error
   SVGA3D_vgpu10_DrawInstanced(struct svga_winsys_context *swc,
                           {
               SVGA3D_COPY_BASIC_4(vertexCountPerInstance, instanceCount,
            swc->hints |= SVGA_HINT_FLAG_CAN_PRE_FLUSH;
   swc->commit(swc);
   swc->num_draw_commands++;
      }
      enum pipe_error
   SVGA3D_vgpu10_DrawIndexedInstanced(struct svga_winsys_context *swc,
                                 {
               SVGA3D_COPY_BASIC_5(indexCountPerInstance, instanceCount,
                     swc->hints |= SVGA_HINT_FLAG_CAN_PRE_FLUSH;
   swc->commit(swc);
   swc->num_draw_commands++;
      }
      enum pipe_error
   SVGA3D_vgpu10_DrawAuto(struct svga_winsys_context *swc)
   {
               cmd->pad0 = 0;
   swc->hints |= SVGA_HINT_FLAG_CAN_PRE_FLUSH;
   swc->commit(swc);
   swc->num_draw_commands++;
      }
      enum pipe_error
   SVGA3D_vgpu10_DefineQuery(struct svga_winsys_context *swc,
                     {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroyQuery(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_BindQuery(struct svga_winsys_context *swc,
               {
      SVGA3dCmdDXBindQuery *cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->queryId = queryId;
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetQueryOffset(struct svga_winsys_context *swc,
               {
      SVGA3D_CREATE_COMMAND(SetQueryOffset, SET_QUERY_OFFSET);
   SVGA3D_COPY_BASIC_2(queryId, mobOffset);
   swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_BeginQuery(struct svga_winsys_context *swc,
         {
      SVGA3D_CREATE_COMMAND(BeginQuery, BEGIN_QUERY);
   cmd->queryId = queryId;
   swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_EndQuery(struct svga_winsys_context *swc,
         {
      SVGA3D_CREATE_COMMAND(EndQuery, END_QUERY);
   cmd->queryId = queryId;
   swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_vgpu10_ClearDepthStencilView(struct svga_winsys_context *swc,
                           {
      SVGA3dCmdDXClearDepthStencilView *cmd;
            cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            /* NOTE: The following is pretty tricky.  We need to emit a view/surface
   * relocation and we have to provide a pointer to an ID which lies in
   * the bounds of the command space which we just allocated.  However,
   * we then need to overwrite it with the original DepthStencilViewId.
   */
   view_relocation(swc, ds_surf, &cmd->depthStencilViewId,
         cmd->depthStencilViewId = ss->view_id;
   cmd->flags = flags;
   cmd->stencil = stencil;
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DefineShaderResourceView(struct svga_winsys_context *swc,
                                 {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_DEFINE_SHADERRESOURCE_VIEW,
               if (!cmd)
                     swc->surface_relocation(swc, &cmd->sid, NULL, surface,
                     swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroyShaderResourceView(struct svga_winsys_context *swc,
         {
      SVGA3D_CREATE_COMMAND(DestroyShaderResourceView,
                     swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_vgpu10_DefineRenderTargetView(struct svga_winsys_context *swc,
                                 {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_DEFINE_RENDERTARGET_VIEW,
               if (!cmd)
            SVGA3D_COPY_BASIC_3(renderTargetViewId, format, resourceDimension);
            surface_to_resourceid(swc, surface,
                  swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroyRenderTargetView(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_vgpu10_DefineDepthStencilView(struct svga_winsys_context *swc,
                                 {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_VIEW,
               if (!cmd)
            SVGA3D_COPY_BASIC_3(depthStencilViewId, format, resourceDimension);
   cmd->mipSlice = desc->tex.mipSlice;
   cmd->firstArraySlice = desc->tex.firstArraySlice;
   cmd->arraySize = desc->tex.arraySize;
   cmd->flags = 0;
   cmd->pad0 = 0;
            surface_to_resourceid(swc, surface,
                  swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroyDepthStencilView(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DefineElementLayout(struct svga_winsys_context *swc,
                     {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_DEFINE_ELEMENTLAYOUT,
               if (!cmd)
            cmd->elementLayoutId = elementLayoutId;
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroyElementLayout(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DefineBlendState(struct svga_winsys_context *swc,
                           {
                        for (i = 0; i < SVGA3D_MAX_RENDER_TARGETS; i++) {
      /* At most, one of blend or logicop can be enabled */
               cmd->blendId = blendId;
   cmd->alphaToCoverageEnable = alphaToCoverageEnable;
   cmd->independentBlendEnable = independentBlendEnable;
   memcpy(cmd->perRT, perRT, sizeof(cmd->perRT));
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroyBlendState(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DefineDepthStencilState(struct svga_winsys_context *swc,
                                       SVGA3dDepthStencilStateId depthStencilId,
   uint8 depthEnable,
   SVGA3dDepthWriteMask depthWriteMask,
   SVGA3dComparisonFunc depthFunc,
   uint8 stencilEnable,
   uint8 frontEnable,
   uint8 backEnable,
   uint8 stencilReadMask,
   uint8 stencilWriteMask,
   uint8 frontStencilFailOp,
   uint8 frontStencilDepthFailOp,
   {
               SVGA3D_COPY_BASIC_9(depthStencilId, depthEnable,
                     depthWriteMask, depthFunc,
   SVGA3D_COPY_BASIC_8(frontStencilFailOp, frontStencilDepthFailOp,
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroyDepthStencilState(struct svga_winsys_context *swc,
         {
      SVGA3D_CREATE_COMMAND(DestroyDepthStencilState,
                     swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DefineRasterizerState(struct svga_winsys_context *swc,
                                       SVGA3dRasterizerStateId rasterizerId,
   uint8 fillMode,
   SVGA3dCullMode cullMode,
   uint8 frontCounterClockwise,
   int32 depthBias,
   float depthBiasClamp,
   float slopeScaledDepthBias,
   uint8 depthClipEnable,
   uint8 scissorEnable,
   uint8 multisampleEnable,
   {
               SVGA3D_COPY_BASIC_5(rasterizerId, fillMode,
               SVGA3D_COPY_BASIC_6(depthBiasClamp, slopeScaledDepthBias,
               cmd->lineWidth = lineWidth;
   cmd->lineStippleEnable = lineStippleEnable;
   cmd->lineStippleFactor = lineStippleFactor;
   cmd->lineStipplePattern = lineStipplePattern;
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroyRasterizerState(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DefineSamplerState(struct svga_winsys_context *swc,
                                    SVGA3dSamplerId samplerId,
   SVGA3dFilter filter,
   uint8 addressU,
   uint8 addressV,
   uint8 addressW,
      {
               SVGA3D_COPY_BASIC_6(samplerId, filter,
               SVGA3D_COPY_BASIC_5(maxAnisotropy, comparisonFunc,
               cmd->pad0 = 0;
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroySamplerState(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_vgpu10_DefineAndBindShader(struct svga_winsys_context *swc,
                           {
      SVGA3dCmdHeader *header;
   SVGA3dCmdDXDefineShader *dcmd;
   SVGA3dCmdDXBindShader *bcmd;
   unsigned totalSize = 2 * sizeof(*header) +
            /* Make sure there is room for both commands */
   header = swc->reserve(swc, totalSize, 2);
   if (!header)
            /* DXDefineShader command */
   header->id = SVGA_3D_CMD_DX_DEFINE_SHADER;
   header->size = sizeof(*dcmd);
   dcmd = (SVGA3dCmdDXDefineShader *)(header + 1);
   dcmd->shaderId = shaderId;
   dcmd->type = type;
            /* DXBindShader command */
            header->id = SVGA_3D_CMD_DX_BIND_SHADER;
   header->size = sizeof(*bcmd);
            bcmd->cid = swc->cid;
   swc->shader_relocation(swc, NULL, &bcmd->mobid,
                     swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroyShader(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DefineStreamOutput(struct svga_winsys_context *swc,
         SVGA3dStreamOutputId soid,
   uint32 numOutputStreamEntries,
   uint32 streamOutputStrideInBytes[SVGA3D_DX_MAX_SOTARGETS],
   {
      unsigned i;
            cmd->soid = soid;
            for (i = 0; i < ARRAY_SIZE(cmd->streamOutputStrideInBytes); i++)
            memcpy(cmd->decl, decl,
                  cmd->rasterizedStream = 0;
   swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_DestroyStreamOutput(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetInputLayout(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetVertexBuffers(struct svga_winsys_context *swc,
                           {
      SVGA3dCmdDXSetVertexBuffers *cmd;
   SVGA3dVertexBuffer *bufs;
                     cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_SET_VERTEX_BUFFERS,
                     if (!cmd)
                     bufs = (SVGA3dVertexBuffer *) &cmd[1];
   for (i = 0; i < count; i++) {
      bufs[i].stride = bufferInfo[i].stride;
   bufs[i].offset = bufferInfo[i].offset;
   swc->surface_relocation(swc, &bufs[i].sid, NULL, surfaces[i],
               swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetVertexBuffersOffsetAndSize(struct svga_winsys_context *swc,
                     {
      SVGA3dCmdDXSetVertexBuffersOffsetAndSize *cmd;
   SVGA3dVertexBufferOffsetAndSize *bufs;
                     cmd = SVGA3D_FIFOReserve(swc,
                           if (!cmd)
                     bufs = (SVGA3dVertexBufferOffsetAndSize *) &cmd[1];
   for (i = 0; i < count; i++) {
      bufs[i].stride = bufferInfo[i].stride;
   bufs[i].offset = bufferInfo[i].offset;
               swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetTopology(struct svga_winsys_context *swc,
         {
                        swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetIndexBuffer(struct svga_winsys_context *swc,
                     {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_SET_INDEX_BUFFER,
               if (!cmd)
            swc->surface_relocation(swc, &cmd->sid, NULL, indexes, SVGA_RELOC_READ);
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetIndexBuffer_v2(struct svga_winsys_context *swc,
                           {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_SET_INDEX_BUFFER_V2,
               if (!cmd)
            swc->surface_relocation(swc, &cmd->sid, NULL, indexes, SVGA_RELOC_READ);
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetIndexBufferOffsetAndSize(struct svga_winsys_context *swc,
                     {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_SET_INDEX_BUFFER_OFFSET_AND_SIZE,
               if (!cmd)
                     swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_SetSingleConstantBuffer(struct svga_winsys_context *swc,
                                 {
               assert(offsetInBytes % 256 == 0);
   if (!surface)
         else
            cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_SET_SINGLE_CONSTANT_BUFFER,
               if (!cmd)
            cmd->slot = slot;
   cmd->type = type;
   swc->surface_relocation(swc, &cmd->sid, NULL, surface, SVGA_RELOC_READ);
   cmd->offsetInBytes = offsetInBytes;
                        }
         enum pipe_error
   SVGA3D_vgpu10_SetConstantBufferOffset(struct svga_winsys_context *swc,
                     {
                        cmd = SVGA3D_FIFOReserve(swc, command,
               if (!cmd)
            cmd->slot = slot;
                        }
         enum pipe_error
   SVGA3D_vgpu10_ReadbackSubResource(struct svga_winsys_context *swc,
               {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_READBACK_SUBRESOURCE,
               if (!cmd)
            swc->surface_relocation(swc, &cmd->sid, NULL, surface,
                  swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_UpdateSubResource(struct svga_winsys_context *swc,
                     {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_UPDATE_SUBRESOURCE,
               if (!cmd)
            swc->surface_relocation(swc, &cmd->sid, NULL, surface,
         cmd->subResource = subResource;
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_GenMips(struct svga_winsys_context *swc,
               {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_GENMIPS,
            if (!cmd)
            swc->surface_relocation(swc, &cmd->shaderResourceViewId, NULL, view,
                  swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_vgpu10_BufferCopy(struct svga_winsys_context *swc,
                     {
                        if (!cmd)
            swc->surface_relocation(swc, &cmd->dest, NULL, dst, SVGA_RELOC_WRITE);
   swc->surface_relocation(swc, &cmd->src, NULL, src, SVGA_RELOC_READ);
   cmd->destX = dstx;
   cmd->srcX = srcx;
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_TransferFromBuffer(struct svga_winsys_context *swc,
                                       {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_TRANSFER_FROM_BUFFER,
            if (!cmd)
            swc->surface_relocation(swc, &cmd->srcSid, NULL, src, SVGA_RELOC_READ);
   swc->surface_relocation(swc, &cmd->destSid, NULL, dst, SVGA_RELOC_WRITE);
   cmd->srcOffset = srcOffset;
   cmd->srcPitch = srcPitch;
   cmd->srcSlicePitch = srcSlicePitch;
   cmd->destSubResource = dstSubResource;
            swc->commit(swc);
      }
      enum pipe_error
   SVGA3D_vgpu10_IntraSurfaceCopy(struct svga_winsys_context *swc,
                     {
      SVGA3dCmdIntraSurfaceCopy *cmd =
      SVGA3D_FIFOReserve(swc,
                  if (!cmd)
            swc->surface_relocation(swc, &cmd->surface.sid, NULL, surface, SVGA_RELOC_READ | SVGA_RELOC_WRITE);
   cmd->surface.face = face;
   cmd->surface.mipmap = level;
                        }
      enum pipe_error
   SVGA3D_vgpu10_ResolveCopy(struct svga_winsys_context *swc,
                           unsigned dstSubResource,
   {
      SVGA3dCmdDXResolveCopy *cmd =
      SVGA3D_FIFOReserve(swc,
                  if (!cmd)
            cmd->dstSubResource = dstSubResource;
   swc->surface_relocation(swc, &cmd->dstSid, NULL, dst, SVGA_RELOC_WRITE);
   cmd->srcSubResource = srcSubResource;
   swc->surface_relocation(swc, &cmd->srcSid, NULL, src, SVGA_RELOC_READ);
                        }
         enum pipe_error
   SVGA3D_sm5_DrawIndexedInstancedIndirect(struct svga_winsys_context *swc,
               {
      SVGA3dCmdDXDrawIndexedInstancedIndirect *cmd =
      SVGA3D_FIFOReserve(swc,
                  if (!cmd)
            swc->surface_relocation(swc, &cmd->argsBufferSid, NULL, argBuffer,
                              }
         enum pipe_error
   SVGA3D_sm5_DrawInstancedIndirect(struct svga_winsys_context *swc,
               {
      SVGA3dCmdDXDrawInstancedIndirect *cmd =
      SVGA3D_FIFOReserve(swc,
                  if (!cmd)
            swc->surface_relocation(swc, &cmd->argsBufferSid, NULL, argBuffer,
                              }
         enum pipe_error
   SVGA3D_sm5_DefineUAView(struct svga_winsys_context *swc,
                           SVGA3dUAViewId uaViewId,
   {
               cmd = SVGA3D_FIFOReserve(swc, SVGA_3D_CMD_DX_DEFINE_UA_VIEW,
               if (!cmd)
                     swc->surface_relocation(swc, &cmd->sid, NULL, surface,
                     swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_sm5_DestroyUAView(struct svga_winsys_context *swc,
         {
      SVGA3D_CREATE_COMMAND(DestroyUAView, DESTROY_UA_VIEW);
   cmd->uaViewId = uaViewId;
   swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_sm5_SetUAViews(struct svga_winsys_context *swc,
                           {
      SVGA3dCmdDXSetUAViews *cmd;
   SVGA3dUAViewId *cmd_uavIds;
            cmd = SVGA3D_FIFOReserve(swc,
                           if (!cmd)
            cmd->uavSpliceIndex = uavSpliceIndex;
            for (i = 0; i < count; i++, cmd_uavIds++) {
      swc->surface_relocation(swc, cmd_uavIds, NULL,
                           swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_sm5_Dispatch(struct svga_winsys_context *swc,
         {
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->threadGroupCountX = threadGroupCount[0];
   cmd->threadGroupCountY = threadGroupCount[1];
            swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_sm5_DispatchIndirect(struct svga_winsys_context *swc,
               {
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            swc->surface_relocation(swc, &cmd->argsBufferSid, NULL, argBuffer,
                  swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_sm5_SetCSUAViews(struct svga_winsys_context *swc,
                     {
      SVGA3dCmdDXSetCSUAViews *cmd;
   SVGA3dUAViewId *cmd_uavIds;
            cmd = SVGA3D_FIFOReserve(swc,
                           if (!cmd)
            cmd->startIndex = 0;
            for (i = 0; i < count; i++, cmd_uavIds++) {
      swc->surface_relocation(swc, cmd_uavIds, NULL,
                           swc->commit(swc);
      }
      /**
   * We don't want any flush between DefineStreamOutputWithMob and
   * BindStreamOutput because it will cause partial state in command
   * buffer. This function make that sure there is enough room for
   * both commands before issuing them
   */
      enum pipe_error
   SVGA3D_sm5_DefineAndBindStreamOutput(struct svga_winsys_context *swc,
         SVGA3dStreamOutputId soid,
   uint32 numOutputStreamEntries,
   uint32 numOutputStreamStrides,
   uint32 streamOutputStrideInBytes[SVGA3D_DX_MAX_SOTARGETS],
   struct svga_winsys_buffer *declBuf,
   uint32 rasterizedStream,
   {
      unsigned i;
   SVGA3dCmdHeader *header;
   SVGA3dCmdDXDefineStreamOutputWithMob *dcmd;
            unsigned totalSize = 2 * sizeof(*header) +
            /* Make sure there is room for both commands */
   header = swc->reserve(swc, totalSize, 2);
   if (!header)
            /* DXDefineStreamOutputWithMob command */
   header->id = SVGA_3D_CMD_DX_DEFINE_STREAMOUTPUT_WITH_MOB;
   header->size = sizeof(*dcmd);
   dcmd = (SVGA3dCmdDXDefineStreamOutputWithMob *)(header + 1);
   dcmd->soid= soid;
   dcmd->numOutputStreamEntries = numOutputStreamEntries;
   dcmd->numOutputStreamStrides = numOutputStreamStrides;
            for (i = 0; i < ARRAY_SIZE(dcmd->streamOutputStrideInBytes); i++)
               /* DXBindStreamOutput command */
            header->id = SVGA_3D_CMD_DX_BIND_STREAMOUTPUT;
   header->size = sizeof(*bcmd);
            bcmd->soid = soid;
   bcmd->offsetInBytes = 0;
   swc->mob_relocation(swc, &bcmd->mobid,
                  bcmd->sizeInBytes = sizeInBytes;
               swc->commit(swc);
      }
         enum pipe_error
   SVGA3D_sm5_DefineRasterizerState_v2(struct svga_winsys_context *swc,
                                       SVGA3dRasterizerStateId rasterizerId,
   uint8 fillMode,
   SVGA3dCullMode cullMode,
   uint8 frontCounterClockwise,
   int32 depthBias,
   float depthBiasClamp,
   float slopeScaledDepthBias,
   uint8 depthClipEnable,
   uint8 scissorEnable,
   uint8 multisampleEnable,
   uint8 antialiasedLineEnable,
   {
               SVGA3D_COPY_BASIC_5(rasterizerId, fillMode,
               SVGA3D_COPY_BASIC_6(depthBiasClamp, slopeScaledDepthBias,
               cmd->lineWidth = lineWidth;
   cmd->lineStippleEnable = lineStippleEnable;
   cmd->lineStippleFactor = lineStippleFactor;
   cmd->lineStipplePattern = lineStipplePattern;
   cmd->provokingVertexLast = provokingVertexLast;
            swc->commit(swc);
      }
