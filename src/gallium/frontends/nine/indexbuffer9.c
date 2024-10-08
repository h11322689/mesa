   /*
   * Copyright 2011 Joakim Sindholt <opensource@zhasha.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "indexbuffer9.h"
   #include "device9.h"
   #include "nine_helpers.h"
   #include "nine_pipe.h"
   #include "nine_dump.h"
      #include "pipe/p_screen.h"
   #include "pipe/p_context.h"
   #include "pipe/p_state.h"
   #include "pipe/p_defines.h"
   #include "util/format/u_formats.h"
   #include "util/u_box.h"
      #define DBG_CHANNEL DBG_INDEXBUFFER
      HRESULT
   NineIndexBuffer9_ctor( struct NineIndexBuffer9 *This,
               {
      HRESULT hr;
   DBG("This=%p pParams=%p pDesc=%p Usage=%s\n",
            hr = NineBuffer9_ctor(&This->base, pParams, D3DRTYPE_INDEXBUFFER,
         if (FAILED(hr))
            switch (pDesc->Format) {
   case D3DFMT_INDEX16: This->index_size = 2; break;
   case D3DFMT_INDEX32: This->index_size = 4; break;
   default:
      user_assert(!"Invalid index format.", D3DERR_INVALIDCALL);
               pDesc->Type = D3DRTYPE_INDEXBUFFER;
               }
      void
   NineIndexBuffer9_dtor( struct NineIndexBuffer9 *This )
   {
         }
      struct pipe_resource *
   NineIndexBuffer9_GetBuffer( struct NineIndexBuffer9 *This, unsigned *offset )
   {
      /* The resource may change */
      }
      HRESULT NINE_WINAPI
   NineIndexBuffer9_Lock( struct NineIndexBuffer9 *This,
                           {
         }
      HRESULT NINE_WINAPI
   NineIndexBuffer9_Unlock( struct NineIndexBuffer9 *This )
   {
         }
      HRESULT NINE_WINAPI
   NineIndexBuffer9_GetDesc( struct NineIndexBuffer9 *This,
         {
      user_assert(pDesc, E_POINTER);
   *pDesc = This->desc;
      }
      IDirect3DIndexBuffer9Vtbl NineIndexBuffer9_vtable = {
      (void *)NineUnknown_QueryInterface,
   (void *)NineUnknown_AddRef,
   (void *)NineUnknown_Release,
   (void *)NineUnknown_GetDevice, /* actually part of Resource9 iface */
   (void *)NineUnknown_SetPrivateData,
   (void *)NineUnknown_GetPrivateData,
   (void *)NineUnknown_FreePrivateData,
   (void *)NineResource9_SetPriority,
   (void *)NineResource9_GetPriority,
   (void *)NineResource9_PreLoad,
   (void *)NineResource9_GetType,
   (void *)NineIndexBuffer9_Lock,
   (void *)NineIndexBuffer9_Unlock,
      };
      static const GUID *NineIndexBuffer9_IIDs[] = {
      &IID_IDirect3DIndexBuffer9,
   &IID_IDirect3DResource9,
   &IID_IUnknown,
      };
      HRESULT
   NineIndexBuffer9_new( struct NineDevice9 *pDevice,
               {
         }
