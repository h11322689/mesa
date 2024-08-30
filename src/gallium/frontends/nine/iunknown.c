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
      #include "iunknown.h"
   #include "util/u_atomic.h"
   #include "util/hash_table.h"
      #include "nine_helpers.h"
   #include "nine_pdata.h"
   #include "nine_lock.h"
      #define DBG_CHANNEL DBG_UNKNOWN
      HRESULT
   NineUnknown_ctor( struct NineUnknown *This,
         {
      if (pParams->container) {
      This->refs = 0;
   This->forward = true;
   This->bind = 0;
      } else if (pParams->start_with_bind_not_ref) {
      This->refs = 0;
   This->forward = false;
      } else {
      This->refs = 1;
   This->forward = false;
               This->container = pParams->container;
   This->device = pParams->device;
   if (This->refs && This->device)
            This->vtable = pParams->vtable;
   This->vtable_internal = pParams->vtable;
   This->guids = pParams->guids;
            This->pdata = _mesa_hash_table_create(NULL, ht_guid_hash, ht_guid_compare);
   if (!This->pdata)
               }
      void
   NineUnknown_dtor( struct NineUnknown *This )
   {
      if (This->refs && This->device) /* Possible only if early exit after a ctor failed */
            if (This->pdata)
               }
      HRESULT NINE_WINAPI
   NineUnknown_QueryInterface( struct NineUnknown *This,
               {
      unsigned i = 0;
            DBG("This=%p riid=%p id=%s ppvObject=%p\n",
                              do {
      if (GUID_equal(This->guids[i], riid)) {
         *ppvObject = This;
   /* Tests showed that this call succeeds even on objects with
   * zero refcount. This can happen if the app released all references
   * but the resource is still bound.
   */
   NineUnknown_AddRef(This);
               *ppvObject = NULL;
      }
      ULONG NINE_WINAPI
   NineUnknown_AddRef( struct NineUnknown *This )
   {
      ULONG r;
   if (This->forward)
         else
            if (r == 1) {
      if (This->device)
      }
      }
      ULONG NINE_WINAPI
   NineUnknown_Release( struct NineUnknown *This )
   {
      if (This->forward)
            /* Cannot decrease lower than 0. This is a thing
   * according to wine tests. It's not just clamping
   * the result as AddRef after Release while refs is 0
   * will give 1 */
   if (!p_atomic_read(&This->refs))
                     if (r == 0) {
      struct NineDevice9 *device = This->device;
            if (!This->container && This->bind == 0) {
         }
   if (device) {
            }
      }
      /* No need to lock the mutex protecting nine (when D3DCREATE_MULTITHREADED)
   * for AddRef and Release, except for dtor as some of the dtors require it. */
   ULONG NINE_WINAPI
   NineUnknown_ReleaseWithDtorLock( struct NineUnknown *This )
   {
      if (This->forward)
                     if (r == 0) {
      struct NineDevice9 *device = This->device;
   /* Containers (here with !forward) take care of item destruction */
   if (!This->container && This->bind == 0) {
         NineLockGlobalMutex();
   This->dtor(This);
   }
   if (device) {
            }
      }
      HRESULT NINE_WINAPI
   NineUnknown_GetDevice( struct NineUnknown *This,
         {
      user_assert(ppDevice, E_POINTER);
   NineUnknown_AddRef(NineUnknown(This->device));
   *ppDevice = (IDirect3DDevice9 *)This->device;
      }
      HRESULT NINE_WINAPI
   NineUnknown_SetPrivateData( struct NineUnknown *This,
                           {
      struct pheader *header;
   const void *user_data = pData;
   char guid_str[64];
            DBG("This=%p GUID=%s pData=%p SizeOfData=%u Flags=%x\n",
                     if (Flags & D3DSPD_IUNKNOWN)
            /* data consists of a header and the actual data. avoiding 2 mallocs */
   header = CALLOC_VARIANT_LENGTH_STRUCT(pheader, SizeOfData);
   if (!header) { DBG("Returning E_OUTOFMEMORY\n"); return E_OUTOFMEMORY; }
            /* if the refguid already exists, delete it */
            /* IUnknown special case */
   if (header->unknown) {
      /* here the pointer doesn't point to the data we want, so point at the
                     header->size = SizeOfData;
   header_data = (void *)header + sizeof(*header);
   memcpy(header_data, user_data, header->size);
            DBG("New header %p, size %d\n", header, (int)header->size);
   _mesa_hash_table_insert(This->pdata, &header->guid, header);
   if (header->unknown) { IUnknown_AddRef(*(IUnknown **)header_data); }
      }
      HRESULT NINE_WINAPI
   NineUnknown_GetPrivateData( struct NineUnknown *This,
                     {
      struct hash_entry *entry;
   struct pheader *header;
   DWORD sizeofdata;
   char guid_str[64];
            DBG("This=%p GUID=%s pData=%p pSizeOfData=%p\n",
                     entry = _mesa_hash_table_search(This->pdata, refguid);
                     user_assert(pSizeOfData, E_POINTER);
   sizeofdata = *pSizeOfData;
   *pSizeOfData = header->size;
            if (!pData) {
      DBG("Returning early D3D_OK\n");
      }
   if (sizeofdata < header->size) {
      DBG("Returning D3DERR_MOREDATA\n");
               header_data = (void *)header + sizeof(*header);
   if (header->unknown) { IUnknown_AddRef(*(IUnknown **)header_data); }
   memcpy(pData, header_data, header->size);
               }
      HRESULT NINE_WINAPI
   NineUnknown_FreePrivateData( struct NineUnknown *This,
         {
      struct hash_entry *entry;
                              entry = _mesa_hash_table_search(This->pdata, refguid);
   if (!entry) {
      DBG("Nothing to free\n");
               DBG("Freeing %p\n", entry->data);
   ht_guid_delete(entry);
               }
