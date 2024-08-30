   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
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
   * svga_cmd.c --
   *
   *      Command construction utility for the SVGA3D protocol used by
   *      the VMware SVGA device, based on the svgautil library.
   */
      #include "svga_winsys.h"
   #include "svga_resource_buffer.h"
   #include "svga_resource_texture.h"
   #include "svga_surface.h"
   #include "svga_cmd.h"
      /*
   *----------------------------------------------------------------------
   *
   * surface_to_surfaceid --
   *
   *      Utility function for surface ids.
   *      Can handle null surface. Does a surface_reallocation so you need
   *      to have allocated the fifo space before converting.
   *
   *
   * param flags  mask of SVGA_RELOC_READ / _WRITE
   *
   * Results:
   *      id is filled out.
   *
   * Side effects:
   *      One surface relocation is performed for texture handle.
   *
   *----------------------------------------------------------------------
   */
      static inline void
   surface_to_surfaceid(struct svga_winsys_context *swc, // IN
                     {
      if (surface) {
      struct svga_surface *s = svga_surface(surface);
   swc->surface_relocation(swc, &id->sid, NULL, s->handle, flags);
   id->face = s->real_layer; /* faces have the same order */
      }
   else {
      swc->surface_relocation(swc, &id->sid, NULL, NULL, flags);
   id->face = 0;
         }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_FIFOReserve --
   *
   *      Reserve space for an SVGA3D FIFO command.
   *
   *      The 2D SVGA commands have been around for a while, so they
   *      have a rather asymmetric structure. The SVGA3D protocol is
   *      more uniform: each command begins with a header containing the
   *      command number and the full size.
   *
   *      This is a convenience wrapper around SVGA_FIFOReserve. We
   *      reserve space for the whole command, and write the header.
   *
   *      This function must be paired with SVGA_FIFOCommitAll().
   *
   * Results:
   *      Returns a pointer to the space reserved for command-specific
   *      data. It must be 'cmdSize' bytes long.
   *
   * Side effects:
   *      Begins a FIFO reservation.
   *
   *----------------------------------------------------------------------
   */
      void *
   SVGA3D_FIFOReserve(struct svga_winsys_context *swc,
                     {
               header = swc->reserve(swc, sizeof *header + cmdSize, nr_relocs);
   if (!header)
            header->id = cmd;
                                 }
         void
   SVGA_FIFOCommitAll(struct svga_winsys_context *swc)
   {
         }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_DefineContext --
   *
   *      Create a new context, to be referred to with the provided ID.
   *
   *      Context objects encapsulate all render state, and shader
   *      objects are per-context.
   *
   *      Surfaces are not per-context. The same surface can be shared
   *      between multiple contexts, and surface operations can occur
   *      without a context.
   *
   *      If the provided context ID already existed, it is redefined.
   *
   *      Context IDs are arbitrary small non-negative integers,
   *      global to the entire SVGA device.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_DefineContext(struct svga_winsys_context *swc)  // IN
   {
               cmd = SVGA3D_FIFOReserve(swc,
         if (!cmd)
                                 }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_DestroyContext --
   *
   *      Delete a context created with SVGA3D_DefineContext.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_DestroyContext(struct svga_winsys_context *swc)  // IN
   {
               cmd = SVGA3D_FIFOReserve(swc,
         if (!cmd)
                                 }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_BeginDefineSurface --
   *
   *      Begin a SURFACE_DEFINE command. This reserves space for it in
   *      the FIFO, and returns pointers to the command's faces and
   *      mipsizes arrays.
   *
   *      This function must be paired with SVGA_FIFOCommitAll().
   *      The faces and mipSizes arrays are initialized to zero.
   *
   *      This creates a "surface" object in the SVGA3D device,
   *      with the provided surface ID (sid). Surfaces are generic
   *      containers for host VRAM objects like textures, vertex
   *      buffers, and depth/stencil buffers.
   *
   *      Surfaces are hierarchical:
   *
   *        - Surface may have multiple faces (for cube maps)
   *
   *          - Each face has a list of mipmap levels
   *
   *             - Each mipmap image may have multiple volume
   *               slices, if the image is three dimensional.
   *
   *                - Each slice is a 2D array of 'blocks'
   *
   *                   - Each block may be one or more pixels.
   *                     (Usually 1, more for DXT or YUV formats.)
   *
   *      Surfaces are generic host VRAM objects. The SVGA3D device
   *      may optimize surfaces according to the format they were
   *      created with, but this format does not limit the ways in
   *      which the surface may be used. For example, a depth surface
   *      can be used as a texture, or a floating point image may
   *      be used as a vertex buffer. Some surface usages may be
   *      lower performance, due to software emulation, but any
   *      usage should work with any surface.
   *
   *      If 'sid' is already defined, the old surface is deleted
   *      and this new surface replaces it.
   *
   *      Surface IDs are arbitrary small non-negative integers,
   *      global to the entire SVGA device.
   *
   * Results:
   *      Returns pointers to arrays allocated in the FIFO for 'faces'
   *      and 'mipSizes'.
   *
   * Side effects:
   *      Begins a FIFO reservation.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_BeginDefineSurface(struct svga_winsys_context *swc,
                           struct svga_winsys_surface *sid, // IN
   SVGA3dSurface1Flags flags,    // IN
   {
               cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            swc->surface_relocation(swc, &cmd->sid, NULL, sid,
         cmd->surfaceFlags = flags;
            *faces = &cmd->face[0];
            memset(*faces, 0, sizeof **faces * SVGA3D_MAX_SURFACE_FACES);
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_DefineSurface2D --
   *
   *      This is a simplified version of SVGA3D_BeginDefineSurface(),
   *      which does not support cube maps, mipmaps, or volume textures.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_DefineSurface2D(struct svga_winsys_context *swc,    // IN
                           {
      SVGA3dSize *mipSizes;
   SVGA3dSurfaceFace *faces;
            ret = SVGA3D_BeginDefineSurface(swc,
         if (ret != PIPE_OK)
                     mipSizes[0].width = width;
   mipSizes[0].height = height;
                        }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_DestroySurface --
   *
   *      Release the host VRAM encapsulated by a particular surface ID.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_DestroySurface(struct svga_winsys_context *swc,
         {
               cmd = SVGA3D_FIFOReserve(swc,
         if (!cmd)
            swc->surface_relocation(swc, &cmd->sid, NULL, sid,
                     }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_SurfaceDMA--
   *
   *      Emit a SURFACE_DMA command.
   *
   *      When the SVGA3D device asynchronously processes this FIFO
   *      command, a DMA operation is performed between host VRAM and
   *      a generic SVGAGuestPtr. The guest pointer may refer to guest
   *      VRAM (provided by the SVGA PCI device) or to guest system
   *      memory that has been set up as a Guest Memory Region (GMR)
   *      by the SVGA device.
   *
   *      The guest's DMA buffer must remain valid (not freed, paged out,
   *      or overwritten) until the host has finished processing this
   *      command. The guest can determine that the host has finished
   *      by using the SVGA device's FIFO Fence mechanism.
   *
   *      The guest's image buffer can be an arbitrary size and shape.
   *      Guest image data is interpreted according to the SVGA3D surface
   *      format specified when the surface was defined.
   *
   *      The caller may optionally define the guest image's pitch.
   *      guestImage->pitch can either be zero (assume image is tightly
   *      packed) or it must be the number of bytes between vertically
   *      adjacent image blocks.
   *
   *      The provided copybox list specifies which regions of the source
   *      image are to be copied, and where they appear on the destination.
   *
   *      NOTE: srcx/srcy are always on the guest image and x/y are
   *      always on the host image, regardless of the actual transfer
   *      direction!
   *
   *      For efficiency, the SVGA3D device is free to copy more data
   *      than specified. For example, it may round copy boxes outwards
   *      such that they lie on particular alignment boundaries.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_SurfaceDMA(struct svga_winsys_context *swc,
                     struct svga_transfer *st,         // IN
   SVGA3dTransferType transfer,      // IN
   {
      struct svga_texture *texture = svga_texture(st->base.resource);
   SVGA3dCmdSurfaceDMA *cmd;
   SVGA3dCmdSurfaceDMASuffix *pSuffix;
   uint32 boxesSize = sizeof *boxes * numBoxes;
   unsigned region_flags;
                     if (transfer == SVGA3D_WRITE_HOST_VRAM) {
      region_flags = SVGA_RELOC_READ;
      }
   else if (transfer == SVGA3D_READ_HOST_VRAM) {
      region_flags = SVGA_RELOC_WRITE;
      }
   else {
      assert(0);
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            swc->region_relocation(swc, &cmd->guest.ptr, st->hwbuf, 0, region_flags);
            swc->surface_relocation(swc, &cmd->host.sid, NULL,
         cmd->host.face = st->slice; /* PIPE_TEX_FACE_* and SVGA3D_CUBEFACE_* match */
                              pSuffix = (SVGA3dCmdSurfaceDMASuffix *)((uint8_t*)cmd + sizeof *cmd + boxesSize);
   pSuffix->suffixSize = sizeof *pSuffix;
   pSuffix->maximumOffset = st->hw_nblocksy*st->base.stride;
            swc->commit(swc);
               }
         enum pipe_error
   SVGA3D_BufferDMA(struct svga_winsys_context *swc,
                  struct svga_winsys_buffer *guest,
   struct svga_winsys_surface *host,
   SVGA3dTransferType transfer,      // IN
   uint32 size,                      // IN
      {
      SVGA3dCmdSurfaceDMA *cmd;
   SVGA3dCopyBox *box;
   SVGA3dCmdSurfaceDMASuffix *pSuffix;
   unsigned region_flags;
   unsigned surface_flags;
               if (transfer == SVGA3D_WRITE_HOST_VRAM) {
      region_flags = SVGA_RELOC_READ;
      }
   else if (transfer == SVGA3D_READ_HOST_VRAM) {
      region_flags = SVGA_RELOC_WRITE;
      }
   else {
      assert(0);
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            swc->region_relocation(swc, &cmd->guest.ptr, guest, 0, region_flags);
            swc->surface_relocation(swc, &cmd->host.sid,
         cmd->host.face = 0;
                     box = (SVGA3dCopyBox *)&cmd[1];
   box->x = host_offset;
   box->y = 0;
   box->z = 0;
   box->w = size;
   box->h = 1;
   box->d = 1;
   box->srcx = guest_offset;
   box->srcy = 0;
            pSuffix = (SVGA3dCmdSurfaceDMASuffix *)((uint8_t*)cmd + sizeof *cmd + sizeof *box);
   pSuffix->suffixSize = sizeof *pSuffix;
   pSuffix->maximumOffset = guest_offset + size;
            swc->commit(swc);
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_SetRenderTarget --
   *
   *      Bind a surface object to a particular render target attachment
   *      point on the current context. Render target attachment points
   *      exist for color buffers, a depth buffer, and a stencil buffer.
   *
   *      The SVGA3D device is quite lenient about the types of surfaces
   *      that may be used as render targets. The color buffers must
   *      all be the same size, but the depth and stencil buffers do not
   *      have to be the same size as the color buffer. All attachments
   *      are optional.
   *
   *      Some combinations of render target formats may require software
   *      emulation, depending on the capabilities of the host graphics
   *      API and graphics hardware.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_SetRenderTarget(struct svga_winsys_context *swc,
               {
               cmd = SVGA3D_FIFOReserve(swc,
         if (!cmd)
            cmd->cid = swc->cid;
   cmd->type = type;
   surface_to_surfaceid(swc, surface, &cmd->target, SVGA_RELOC_WRITE);
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_DefineShader --
   *
   *      Upload the bytecode for a new shader. The bytecode is "SVGA3D
   *      format", which is theoretically a binary-compatible superset
   *      of Microsoft's DirectX shader bytecode. In practice, the
   *      SVGA3D bytecode doesn't yet have any extensions to DirectX's
   *      bytecode format.
   *
   *      The SVGA3D device supports shader models 1.1 through 2.0.
   *
   *      The caller chooses a shader ID (small positive integer) by
   *      which this shader will be identified in future commands. This
   *      ID is in a namespace which is per-context and per-shader-type.
   *
   *      'bytecodeLen' is specified in bytes. It must be a multiple of 4.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_DefineShader(struct svga_winsys_context *swc,
                     uint32 shid,                  // IN
   {
                        cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            cmd->cid = swc->cid;
   cmd->shid = shid;
   cmd->type = type;
   memcpy(&cmd[1], bytecode, bytecodeLen);
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_DestroyShader --
   *
   *      Delete a shader that was created by SVGA3D_DefineShader. If
   *      the shader was the current vertex or pixel shader for its
   *      context, rendering results are undefined until a new shader is
   *      bound.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_DestroyShader(struct svga_winsys_context *swc,
               {
               cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            cmd->cid = swc->cid;
   cmd->shid = shid;
   cmd->type = type;
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_SetShaderConst --
   *
   *      Set the value of a shader constant.
   *
   *      Shader constants are analogous to uniform variables in GLSL,
   *      except that they belong to the render context rather than to
   *      an individual shader.
   *
   *      Constants may have one of three types: A 4-vector of floats,
   *      a 4-vector of integers, or a single boolean flag.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_SetShaderConst(struct svga_winsys_context *swc,
                           {
               cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            cmd->cid = swc->cid;
   cmd->reg = reg;
   cmd->type = type;
                     case SVGA3D_CONST_TYPE_FLOAT:
   case SVGA3D_CONST_TYPE_INT:
      memcpy(&cmd->values, value, sizeof cmd->values);
         case SVGA3D_CONST_TYPE_BOOL:
      memset(&cmd->values, 0, sizeof cmd->values);
   cmd->values[0] = *(uint32*)value;
         default:
      assert(0);
         }
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_SetShaderConsts --
   *
   *      Set the value of successive shader constants.
   *
   *      Shader constants are analogous to uniform variables in GLSL,
   *      except that they belong to the render context rather than to
   *      an individual shader.
   *
   *      Constants may have one of three types: A 4-vector of floats,
   *      a 4-vector of integers, or a single boolean flag.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_SetShaderConsts(struct svga_winsys_context *swc,
                           uint32 reg,                   // IN
   {
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
   cmd->reg = reg;
   cmd->type = type;
                                 }
                  /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_SetShader --
   *
   *      Switch active shaders. This binds a new vertex or pixel shader
   *      to the specified context.
   *
   *      A shader ID of SVGA3D_INVALID_ID unbinds any shader, switching
   *      back to the fixed function vertex or pixel pipeline.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_SetShader(struct svga_winsys_context *swc,
               {
                        cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            cmd->cid = swc->cid;
   cmd->type = type;
   cmd->shid = shid;
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_BeginClear --
   *
   *      Begin a CLEAR command. This reserves space for it in the FIFO,
   *      and returns a pointer to the command's rectangle array.  This
   *      function must be paired with SVGA_FIFOCommitAll().
   *
   *      Clear is a rendering operation which fills a list of
   *      rectangles with constant values on all render target types
   *      indicated by 'flags'.
   *
   *      Clear is not affected by clipping, depth test, or other
   *      render state which affects the fragment pipeline.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      May write to attached render target surfaces.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_BeginClear(struct svga_winsys_context *swc,
                     SVGA3dClearFlag flags,  // IN
   uint32 color,           // IN
   float depth,            // IN
   {
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
   cmd->clearFlag = flags;
   cmd->color = color;
   cmd->depth = depth;
   cmd->stencil = stencil;
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_ClearRect --
   *
   *      This is a simplified version of SVGA3D_BeginClear().
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_ClearRect(struct svga_winsys_context *swc,
                  SVGA3dClearFlag flags,  // IN
   uint32 color,           // IN
   float depth,            // IN
   uint32 stencil,         // IN
   uint32 x,               // IN
      {
      SVGA3dRect *rect;
            ret = SVGA3D_BeginClear(swc, flags, color, depth, stencil, &rect, 1);
   if (ret != PIPE_OK)
            memset(rect, 0, sizeof *rect);
   rect->x = x;
   rect->y = y;
   rect->w = w;
   rect->h = h;
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_BeginDrawPrimitives --
   *
   *      Begin a DRAW_PRIMITIVES command. This reserves space for it in
   *      the FIFO, and returns a pointer to the command's arrays.
   *      This function must be paired with SVGA_FIFOCommitAll().
   *
   *      Drawing commands consist of two variable-length arrays:
   *      SVGA3dVertexDecl elements declare a set of vertex buffers to
   *      use while rendering, and SVGA3dPrimitiveRange elements specify
   *      groups of primitives each with an optional index buffer.
   *
   *      The decls and ranges arrays are initialized to zero.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      May write to attached render target surfaces.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_BeginDrawPrimitives(struct svga_winsys_context *swc,
                           {
      SVGA3dCmdDrawPrimitives *cmd;
   SVGA3dVertexDecl *declArray;
   SVGA3dPrimitiveRange *rangeArray;
   uint32 declSize = sizeof **decls * numVertexDecls;
            cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
   cmd->numVertexDecls = numVertexDecls;
            declArray = (SVGA3dVertexDecl*) &cmd[1];
            memset(declArray, 0, declSize);
            *decls = declArray;
                                 }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_BeginSurfaceCopy --
   *
   *      Begin a SURFACE_COPY command. This reserves space for it in
   *      the FIFO, and returns a pointer to the command's arrays.  This
   *      function must be paired with SVGA_FIFOCommitAll().
   *
   *      The box array is initialized with zeroes.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      Asynchronously copies a list of boxes from surface to surface.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_BeginSurfaceCopy(struct svga_winsys_context *swc,
                           {
      SVGA3dCmdSurfaceCopy *cmd;
            cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            surface_to_surfaceid(swc, src, &cmd->src, SVGA_RELOC_READ);
   surface_to_surfaceid(swc, dest, &cmd->dest, SVGA_RELOC_WRITE);
                        }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_SurfaceStretchBlt --
   *
   *      Issue a SURFACE_STRETCHBLT command: an asynchronous
   *      surface-to-surface blit, with scaling.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      Asynchronously copies one box from surface to surface.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_SurfaceStretchBlt(struct svga_winsys_context *swc,
                           struct pipe_surface *src,    // IN
   {
               cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            surface_to_surfaceid(swc, src, &cmd->src, SVGA_RELOC_READ);
   surface_to_surfaceid(swc, dest, &cmd->dest, SVGA_RELOC_WRITE);
   cmd->boxSrc = *boxSrc;
   cmd->boxDest = *boxDest;
   cmd->mode = mode;
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_SetViewport --
   *
   *      Set the current context's viewport rectangle. The viewport
   *      is clipped to the dimensions of the current render target,
   *      then all rendering is clipped to the viewport.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_SetViewport(struct svga_winsys_context *swc,
         {
               cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            cmd->cid = swc->cid;
   cmd->rect = *rect;
               }
               /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_SetScissorRect --
   *
   *      Set the current context's scissor rectangle. If scissoring
   *      is enabled then all rendering is clipped to the scissor bounds.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_SetScissorRect(struct svga_winsys_context *swc,
         {
               cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            cmd->cid = swc->cid;
   cmd->rect = *rect;
               }
      /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_SetClipPlane --
   *
   *      Set one of the current context's clip planes. If the clip
   *      plane is enabled then all 3d rendering is clipped against
   *      the plane.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_SetClipPlane(struct svga_winsys_context *swc,
         {
               cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            cmd->cid = swc->cid;
   cmd->index = index;
   cmd->plane[0] = plane[0];
   cmd->plane[1] = plane[1];
   cmd->plane[2] = plane[2];
   cmd->plane[3] = plane[3];
               }
      /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_SetZRange --
   *
   *      Set the range of the depth buffer to use. 'min' and 'max'
   *      are values between 0.0 and 1.0.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_SetZRange(struct svga_winsys_context *swc,
               {
               cmd = SVGA3D_FIFOReserve(swc,
               if (!cmd)
            cmd->cid = swc->cid;
   cmd->zRange.min = zMin;
   cmd->zRange.max = zMax;
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_BeginSetTextureState --
   *
   *      Begin a SETTEXTURESTATE command. This reserves space for it in
   *      the FIFO, and returns a pointer to the command's texture state
   *      array.  This function must be paired with SVGA_FIFOCommitAll().
   *
   *      This command sets rendering state which is per-texture-unit.
   *
   *      XXX: Individual texture states need documentation. However,
   *           they are very similar to the texture states defined by
   *           Direct3D. The D3D documentation is a good starting point
   *           for understanding SVGA3D texture states.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_BeginSetTextureState(struct svga_winsys_context *swc,
               {
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_BeginSetRenderState --
   *
   *      Begin a SETRENDERSTATE command. This reserves space for it in
   *      the FIFO, and returns a pointer to the command's texture state
   *      array.  This function must be paired with SVGA_FIFOCommitAll().
   *
   *      This command sets rendering state which is global to the context.
   *
   *      XXX: Individual render states need documentation. However,
   *           they are very similar to the render states defined by
   *           Direct3D. The D3D documentation is a good starting point
   *           for understanding SVGA3D render states.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_BeginSetRenderState(struct svga_winsys_context *swc,
               {
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
               }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_BeginGBQuery--
   *
   *      GB resource version of SVGA3D_BeginQuery.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      Commits space in the FIFO memory.
   *
   *----------------------------------------------------------------------
   */
      static enum pipe_error
   SVGA3D_BeginGBQuery(struct svga_winsys_context *swc,
         {
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
                        }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_BeginQuery--
   *
   *      Issues a SVGA_3D_CMD_BEGIN_QUERY command.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      Commits space in the FIFO memory.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_BeginQuery(struct svga_winsys_context *swc,
         {
               if (swc->have_gb_objects)
            cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
                        }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_EndGBQuery--
   *
   *      GB resource version of SVGA3D_EndQuery.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      Commits space in the FIFO memory.
   *
   *----------------------------------------------------------------------
   */
      static enum pipe_error
   SVGA3D_EndGBQuery(struct svga_winsys_context *swc,
      SVGA3dQueryType type,              // IN
      {
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
            swc->mob_relocation(swc, &cmd->mobid, &cmd->offset, buffer,
            swc->commit(swc);
         }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_EndQuery--
   *
   *      Issues a SVGA_3D_CMD_END_QUERY command.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      Commits space in the FIFO memory.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_EndQuery(struct svga_winsys_context *swc,
               {
               if (swc->have_gb_objects)
            cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
            swc->region_relocation(swc, &cmd->guestResult, buffer, 0,
                        }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_WaitForGBQuery--
   *
   *      GB resource version of SVGA3D_WaitForQuery.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      Commits space in the FIFO memory.
   *
   *----------------------------------------------------------------------
   */
      static enum pipe_error
   SVGA3D_WaitForGBQuery(struct svga_winsys_context *swc,
         SVGA3dQueryType type,              // IN
   {
               cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
            swc->mob_relocation(swc, &cmd->mobid, &cmd->offset, buffer,
                        }
         /*
   *----------------------------------------------------------------------
   *
   * SVGA3D_WaitForQuery--
   *
   *      Issues a SVGA_3D_CMD_WAIT_FOR_QUERY command.  This reserves space
   *      for it in the FIFO.  This doesn't actually wait for the query to
   *      finish but instead tells the host to start a wait at the driver
   *      level.  The caller can wait on the status variable in the
   *      guestPtr memory or send an insert fence instruction after this
   *      command and wait on the fence.
   *
   * Results:
   *      None.
   *
   * Side effects:
   *      Commits space in the FIFO memory.
   *
   *----------------------------------------------------------------------
   */
      enum pipe_error
   SVGA3D_WaitForQuery(struct svga_winsys_context *swc,
               {
               if (swc->have_gb_objects)
            cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
            swc->region_relocation(swc, &cmd->guestResult, buffer, 0,
                        }
         enum pipe_error
   SVGA3D_BindGBShader(struct svga_winsys_context *swc,
         {
      SVGA3dCmdBindGBShader *cmd = 
      SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            swc->shader_relocation(swc, &cmd->shid, &cmd->mobid,
                        }
         enum pipe_error
   SVGA3D_SetGBShader(struct svga_winsys_context *swc,
               {
               assert(type == SVGA3D_SHADERTYPE_VS || type == SVGA3D_SHADERTYPE_PS);
      cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
   cmd->type = type;
   if (gbshader)
         else
                     }
         /**
   * \param flags  mask of SVGA_RELOC_READ / _WRITE
   */
   enum pipe_error
   SVGA3D_BindGBSurface(struct svga_winsys_context *swc,
         {
      SVGA3dCmdBindGBSurface *cmd = 
      SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            swc->surface_relocation(swc, &cmd->sid, &cmd->mobid, surface,
                        }
         /**
   * Update an image in a guest-backed surface.
   * (Inform the device that the guest-contents have been updated.)
   */
   enum pipe_error
   SVGA3D_UpdateGBImage(struct svga_winsys_context *swc,
                        {
      SVGA3dCmdUpdateGBImage *cmd =
      SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            swc->surface_relocation(swc, &cmd->image.sid, NULL, surface,
         cmd->image.face = face;
   cmd->image.mipmap = mipLevel;
            swc->commit(swc);
               }
         /**
   * Update an entire guest-backed surface.
   * (Inform the device that the guest-contents have been updated.)
   */
   enum pipe_error
   SVGA3D_UpdateGBSurface(struct svga_winsys_context *swc,
         {
      SVGA3dCmdUpdateGBSurface *cmd =
      SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            swc->surface_relocation(swc, &cmd->sid, NULL, surface,
            swc->commit(swc);
               }
         /**
   * Readback an image in a guest-backed surface.
   * (Request the device to flush the dirty contents into the guest.)
   */
   enum pipe_error
   SVGA3D_ReadbackGBImage(struct svga_winsys_context *swc,
               {
      SVGA3dCmdReadbackGBImage *cmd =
      SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            swc->surface_relocation(swc, &cmd->image.sid, NULL, surface,
         cmd->image.face = face;
            swc->commit(swc);
               }
         /**
   * Readback an entire guest-backed surface.
   * (Request the device to flush the dirty contents into the guest.)
   */
   enum pipe_error
   SVGA3D_ReadbackGBSurface(struct svga_winsys_context *swc,
         {
      SVGA3dCmdReadbackGBSurface *cmd =
      SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            swc->surface_relocation(swc, &cmd->sid, NULL, surface,
            swc->commit(swc);
               }
         enum pipe_error
   SVGA3D_ReadbackGBImagePartial(struct svga_winsys_context *swc,
                           {
      SVGA3dCmdReadbackGBImagePartial *cmd =
      SVGA3D_FIFOReserve(swc,
                  if (!cmd)
            swc->surface_relocation(swc, &cmd->image.sid, NULL, surface,
         cmd->image.face = face;
   cmd->image.mipmap = mipLevel;
   cmd->box = *box;
            swc->commit(swc);
               }
         enum pipe_error
   SVGA3D_InvalidateGBImagePartial(struct svga_winsys_context *swc,
                           {
      SVGA3dCmdInvalidateGBImagePartial *cmd =
      SVGA3D_FIFOReserve(swc,
                  if (!cmd)
            swc->surface_relocation(swc, &cmd->image.sid, NULL, surface,
         cmd->image.face = face;
   cmd->image.mipmap = mipLevel;
   cmd->box = *box;
                        }
      enum pipe_error
   SVGA3D_InvalidateGBSurface(struct svga_winsys_context *swc,
         {
      SVGA3dCmdInvalidateGBSurface *cmd =
      SVGA3D_FIFOReserve(swc,
                  if (!cmd)
            swc->surface_relocation(swc, &cmd->sid, NULL, surface,
                     }
      enum pipe_error
   SVGA3D_SetGBShaderConstsInline(struct svga_winsys_context *swc,
                                 {
                        cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            cmd->cid = swc->cid;
   cmd->regStart = regStart;
   cmd->shaderType = shaderType;
                                 }
