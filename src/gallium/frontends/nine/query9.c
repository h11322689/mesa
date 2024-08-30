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
      #include "device9.h"
   #include "nine_state.h"
   #include "query9.h"
   #include "nine_helpers.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_context.h"
   #include "util/u_math.h"
   #include "nine_dump.h"
      #define DBG_CHANNEL DBG_QUERY
      static inline unsigned
   d3dquerytype_to_pipe_query(struct pipe_screen *screen, D3DQUERYTYPE type)
   {
      switch (type) {
   case D3DQUERYTYPE_EVENT:
         case D3DQUERYTYPE_OCCLUSION:
      return screen->get_param(screen, PIPE_CAP_OCCLUSION_QUERY) ?
      case D3DQUERYTYPE_TIMESTAMP:
      return screen->get_param(screen, PIPE_CAP_QUERY_TIMESTAMP) ?
      case D3DQUERYTYPE_TIMESTAMPDISJOINT:
   case D3DQUERYTYPE_TIMESTAMPFREQ:
      return screen->get_param(screen, PIPE_CAP_QUERY_TIMESTAMP) ?
      case D3DQUERYTYPE_VERTEXSTATS:
      return screen->get_param(screen,
            default:
            }
      #define GET_DATA_SIZE_CASE2(a, b) case D3DQUERYTYPE_##a: return sizeof(D3DDEVINFO_##b)
   #define GET_DATA_SIZE_CASET(a, b) case D3DQUERYTYPE_##a: return sizeof(b)
   static inline DWORD
   nine_query_result_size(D3DQUERYTYPE type)
   {
      switch (type) {
   GET_DATA_SIZE_CASE2(VERTEXSTATS, D3DVERTEXSTATS);
   GET_DATA_SIZE_CASET(EVENT, BOOL);
   GET_DATA_SIZE_CASET(OCCLUSION, DWORD);
   GET_DATA_SIZE_CASET(TIMESTAMP, UINT64);
   GET_DATA_SIZE_CASET(TIMESTAMPDISJOINT, BOOL);
   GET_DATA_SIZE_CASET(TIMESTAMPFREQ, UINT64);
   default:
      assert(0);
         }
      HRESULT
   nine_is_query_supported(struct pipe_screen *screen, D3DQUERYTYPE type)
   {
                        if (ptype == PIPE_QUERY_TYPES) {
      DBG("Query type %u (%s) not supported.\n",
            }
      }
      HRESULT
   NineQuery9_ctor( struct NineQuery9 *This,
               {
      struct NineDevice9 *device = pParams->device;
   const unsigned ptype = d3dquerytype_to_pipe_query(device->screen, Type);
                     hr = NineUnknown_ctor(&This->base, pParams);
   if (FAILED(hr))
            This->state = NINE_QUERY_STATE_FRESH;
                     if (ptype < PIPE_QUERY_TYPES) {
      This->pq = nine_context_create_query(device, ptype);
   if (!This->pq)
      } else {
                  This->instant =
      Type == D3DQUERYTYPE_EVENT ||
   Type == D3DQUERYTYPE_RESOURCEMANAGER ||
   Type == D3DQUERYTYPE_TIMESTAMP ||
   Type == D3DQUERYTYPE_TIMESTAMPFREQ ||
   Type == D3DQUERYTYPE_VCACHE ||
                     }
      void
   NineQuery9_dtor( struct NineQuery9 *This )
   {
                        if (This->pq) {
      if (This->state == NINE_QUERY_STATE_RUNNING)
                        }
      D3DQUERYTYPE NINE_WINAPI
   NineQuery9_GetType( struct NineQuery9 *This )
   {
         }
      DWORD NINE_WINAPI
   NineQuery9_GetDataSize( struct NineQuery9 *This )
   {
         }
      HRESULT NINE_WINAPI
   NineQuery9_Issue( struct NineQuery9 *This,
         {
                        user_assert((dwIssueFlags == D3DISSUE_BEGIN) ||
                  /* Wine tests: always return D3D_OK on D3DISSUE_BEGIN
   * even when the call is supposed to be forbidden */
   if (dwIssueFlags == D3DISSUE_BEGIN && This->instant)
            if (dwIssueFlags == D3DISSUE_BEGIN) {
      if (This->state == NINE_QUERY_STATE_RUNNING)
         nine_context_begin_query(device, &This->counter, This->pq);
      } else {
      if (This->state != NINE_QUERY_STATE_RUNNING &&
         This->type != D3DQUERYTYPE_EVENT &&
   This->type != D3DQUERYTYPE_TIMESTAMP)
   nine_context_end_query(device, &This->counter, This->pq);
      }
      }
      union nine_query_result
   {
      D3DDEVINFO_D3DVERTEXSTATS vertexstats;
   DWORD dw;
   BOOL b;
      };
      HRESULT NINE_WINAPI
   NineQuery9_GetData( struct NineQuery9 *This,
                     {
      struct NineDevice9 *device = This->base.device;
   bool ok, wait_query_result = false;
   union pipe_query_result presult;
            DBG("This=%p pData=%p dwSize=%d dwGetDataFlags=%d\n",
            /* according to spec we should return D3DERR_INVALIDCALL here, but
   * wine returns S_FALSE because it is apparently the behaviour
   * on windows */
   user_assert(This->state != NINE_QUERY_STATE_RUNNING, S_FALSE);
   user_assert(dwSize == 0 || pData, D3DERR_INVALIDCALL);
   user_assert(dwGetDataFlags == 0 ||
            if (This->state == NINE_QUERY_STATE_FRESH) {
      /* App forgot calling Issue. call it for it.
      * However Wine states that return value should
      NineQuery9_Issue(This, D3DISSUE_END);
               /* The documention mentions no special case for D3DQUERYTYPE_TIMESTAMP.
   * However Windows tests show that the query always succeeds when
   * D3DGETDATA_FLUSH is specified. */
   if (This->type == D3DQUERYTYPE_TIMESTAMP &&
      (dwGetDataFlags & D3DGETDATA_FLUSH))
            /* Note: We ignore dwGetDataFlags, because get_query_result will
            ok = nine_context_get_query_result(device, This->pq, &This->counter,
                           if (!dwSize)
            switch (This->type) {
   case D3DQUERYTYPE_EVENT:
      nresult.b = presult.b;
      case D3DQUERYTYPE_OCCLUSION:
      nresult.dw = presult.u64;
      case D3DQUERYTYPE_TIMESTAMP:
      nresult.u64 = presult.u64;
      case D3DQUERYTYPE_TIMESTAMPDISJOINT:
      nresult.b = presult.timestamp_disjoint.disjoint;
      case D3DQUERYTYPE_TIMESTAMPFREQ:
      /* Applications use it to convert the TIMESTAMP value to time.
      AMD drivers on win seem to return the actual hardware clock
   resolution and corresponding values in TIMESTAMP.
   However, this behaviour is not easy to replicate here.
   So instead we do what wine and opengl do, and use
   nanoseconds TIMESTAMPs.
      */
   nresult.u64 = 1000000000;
      case D3DQUERYTYPE_VERTEXSTATS:
      nresult.vertexstats.NumRenderedTriangles =
         nresult.vertexstats.NumExtraClippingTriangles =
            default:
      assert(0);
      }
               }
      IDirect3DQuery9Vtbl NineQuery9_vtable = {
      (void *)NineUnknown_QueryInterface,
   (void *)NineUnknown_AddRef,
   (void *)NineUnknown_Release,
   (void *)NineUnknown_GetDevice, /* actually part of Query9 iface */
   (void *)NineQuery9_GetType,
   (void *)NineQuery9_GetDataSize,
   (void *)NineQuery9_Issue,
      };
      static const GUID *NineQuery9_IIDs[] = {
      &IID_IDirect3DQuery9,
   &IID_IUnknown,
      };
      HRESULT
   NineQuery9_new( struct NineDevice9 *pDevice,
               {
         }
