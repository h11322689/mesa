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
   * InputAssembly.cpp --
   *    Functions that manipulate the input assembly stage.
   */
         #include <stdio.h>
      #include "InputAssembly.h"
   #include "State.h"
      #include "Debug.h"
   #include "Format.h"
         /*
   * ----------------------------------------------------------------------
   *
   * IaSetTopology --
   *
   *    The IaSetTopology function sets the primitive topology to
   *    enable drawing for the input assember.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   IaSetTopology(D3D10DDI_HDEVICE hDevice,                        // IN
         {
                        enum mesa_prim primitive;
   switch (PrimitiveTopology) {
   case D3D10_DDI_PRIMITIVE_TOPOLOGY_UNDEFINED:
      /* Apps might set topology to UNDEFINED when cleaning up on exit. */
   primitive = MESA_PRIM_COUNT;
      case D3D10_DDI_PRIMITIVE_TOPOLOGY_POINTLIST:
      primitive = MESA_PRIM_POINTS;
      case D3D10_DDI_PRIMITIVE_TOPOLOGY_LINELIST:
      primitive = MESA_PRIM_LINES;
      case D3D10_DDI_PRIMITIVE_TOPOLOGY_LINESTRIP:
      primitive = MESA_PRIM_LINE_STRIP;
      case D3D10_DDI_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
      primitive = MESA_PRIM_TRIANGLES;
      case D3D10_DDI_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
      primitive = MESA_PRIM_TRIANGLE_STRIP;
      case D3D10_DDI_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
      primitive = MESA_PRIM_LINES_ADJACENCY;
      case D3D10_DDI_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
      primitive = MESA_PRIM_LINE_STRIP_ADJACENCY;
      case D3D10_DDI_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
      primitive = MESA_PRIM_TRIANGLES_ADJACENCY;
      case D3D10_DDI_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
      primitive = MESA_PRIM_TRIANGLE_STRIP_ADJACENCY;
      default:
      assert(0);
   primitive = MESA_PRIM_COUNT;
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * IaSetVertexBuffers --
   *
   *    The IaSetVertexBuffers function sets vertex buffers
   *    for an input assembler.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   IaSetVertexBuffers(D3D10DDI_HDEVICE hDevice,                                     // IN
                     UINT StartBuffer,                                             // IN
   UINT NumBuffers,                                              // IN
   {
                        Device *pDevice = CastDevice(hDevice);
            for (i = 0; i < NumBuffers; i++) {
      struct pipe_vertex_buffer *vb = &pDevice->vertex_buffers[StartBuffer + i];
   struct pipe_resource *resource = CastPipeResource(phBuffers[i]);
   Resource *res = CastResource(phBuffers[i]);
   struct pipe_stream_output_target *so_target =
            if (so_target && pDevice->draw_so_target != so_target) {
      if (pDevice->draw_so_target) {
         }
   pipe_so_target_reference(&pDevice->draw_so_target,
               if (resource) {
      pDevice->vertex_strides[StartBuffer + i] = pStrides[i];
   vb->buffer_offset = pOffsets[i];
   if (vb->is_user_buffer) {
      vb->buffer.resource = NULL;
      }
      }
   else {
      pDevice->vertex_strides[StartBuffer + i] = 0;
   vb->buffer_offset = 0;
   if (!vb->is_user_buffer) {
      pipe_resource_reference(&vb->buffer.resource, NULL);
      }
                  for (i = 0; i < PIPE_MAX_ATTRIBS; ++i) {
               /* XXX this is odd... */
   if (!vb->is_user_buffer && !vb->buffer.resource) {
      pDevice->vertex_strides[i] = 0;
   vb->buffer_offset = 0;
   vb->is_user_buffer = true;
                  /* Resubmit old and new vertex buffers.
   */
   cso_set_vertex_buffers(pDevice->cso, PIPE_MAX_ATTRIBS, 0, false, pDevice->vertex_buffers);
      }
         /*
   * ----------------------------------------------------------------------
   *
   * IaSetIndexBuffer --
   *
   *    The IaSetIndexBuffer function sets an index buffer for
   *    an input assembler.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   IaSetIndexBuffer(D3D10DDI_HDEVICE hDevice,   // IN
                     {
               Device *pDevice = CastDevice(hDevice);
            if (resource) {
               switch (Format) {
   case DXGI_FORMAT_R16_UINT:
      pDevice->index_size = 2;
   pDevice->restart_index = 0xffff;
      case DXGI_FORMAT_R32_UINT:
      pDevice->restart_index = 0xffffffff;
   pDevice->index_size = 4;
      default:
      assert(0);             /* should not happen */
   pDevice->index_size = 2;
      }
      } else {
            }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateElementLayoutSize --
   *
   *    The CalcPrivateElementLayoutSize function determines the size
   *    of the user-mode display driver's private region of memory
   *    (that is, the size of internal driver structures, not the size
   *    of the resource video memory) for an element layout.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateElementLayoutSize(
      D3D10DDI_HDEVICE hDevice,                                         // IN
      {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateElementLayout --
   *
   *    The CreateElementLayout function creates an element layout.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateElementLayout(
      D3D10DDI_HDEVICE hDevice,                                         // IN
   __in const D3D10DDIARG_CREATEELEMENTLAYOUT *pCreateElementLayout, // IN
   D3D10DDI_HELEMENTLAYOUT hElementLayout,                           // IN
      {
               ElementLayout *pElementLayout = CastElementLayout(hElementLayout);
            unsigned num_elements = pCreateElementLayout->NumElements;
   unsigned max_elements = 0;
   for (unsigned i = 0; i < num_elements; i++) {
      const D3D10DDIARG_INPUT_ELEMENT_DESC* pVertexElement =
         struct pipe_vertex_element *ve =
            ve->src_offset          = pVertexElement->AlignedByteOffset;
   ve->vertex_buffer_index = pVertexElement->InputSlot;
            switch (pVertexElement->InputSlotClass) {
   case D3D10_DDI_INPUT_PER_VERTEX_DATA:
      ve->instance_divisor = 0;
      case D3D10_DDI_INPUT_PER_INSTANCE_DATA:
      if (!pVertexElement->InstanceDataStepRate) {
      LOG_UNSUPPORTED(!pVertexElement->InstanceDataStepRate);
      } else {
         }
      default:
      assert(0);
                           /* XXX: What do we do when there's a gap? */
   if (max_elements != num_elements) {
                     }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroyElementLayout --
   *
   *    The DestroyElementLayout function destroys the specified
   *    element layout object. The element layout object can be
   *    destoyed only if it is not currently bound to a display device.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DestroyElementLayout(D3D10DDI_HDEVICE hDevice,                 // IN
         {
            }
         /*
   * ----------------------------------------------------------------------
   *
   * IaSetInputLayout --
   *
   *    The IaSetInputLayout function sets an input layout for
   *    the input assembler.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   IaSetInputLayout(D3D10DDI_HDEVICE hDevice,               // IN
         {
               Device *pDevice = CastDevice(hDevice);
   pDevice->element_layout = CastElementLayout(hInputLayout);
         }
