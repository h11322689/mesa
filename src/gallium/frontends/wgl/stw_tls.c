   /**************************************************************************
   *
   * Copyright 2009-2013 VMware, Inc.
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
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <windows.h>
   #include <tlhelp32.h>
      #include "util/compiler.h"
   #include "util/u_debug.h"
   #include "stw_tls.h"
      static DWORD tlsIndex = TLS_OUT_OF_INDEXES;
         /**
   * Static mutex to protect the access to g_pendingTlsData global and
   * stw_tls_data::next member.
   */
   static CRITICAL_SECTION g_mutex = {
         };
      /**
   * There is no way to invoke TlsSetValue for a different thread, so we
   * temporarily put the thread data for non-current threads here.
   */
   static struct stw_tls_data *g_pendingTlsData = NULL;
         static struct stw_tls_data *
   stw_tls_data_create(DWORD dwThreadId);
      static struct stw_tls_data *
   stw_tls_lookup_pending_data(DWORD dwThreadId);
         bool
   stw_tls_init(void)
   {
      tlsIndex = TlsAlloc();
   if (tlsIndex == TLS_OUT_OF_INDEXES) {
                  /*
   * DllMain is called with DLL_THREAD_ATTACH only for threads created after
   * the DLL is loaded by the process.  So enumerate and add our hook to all
   * previously existing threads.
   *
   * XXX: Except for the current thread since it there is an explicit
   * stw_tls_init_thread() call for it later on.
      #ifndef _GAMING_XBOX
      DWORD dwCurrentProcessId = GetCurrentProcessId();
   DWORD dwCurrentThreadId = GetCurrentThreadId();
   HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwCurrentProcessId);
   if (hSnapshot != INVALID_HANDLE_VALUE) {
      THREADENTRY32 te;
   te.dwSize = sizeof te;
   if (Thread32First(hSnapshot, &te)) {
      do {
      if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) +
            if (te.th32OwnerProcessID == dwCurrentProcessId) {
      if (te.th32ThreadID != dwCurrentThreadId) {
      struct stw_tls_data *data;
   data = stw_tls_data_create(te.th32ThreadID);
   if (data) {
      EnterCriticalSection(&g_mutex);
   data->next = g_pendingTlsData;
   g_pendingTlsData = data;
               }
         }
         #endif /* _GAMING_XBOX */
            }
         /**
   * Install windows hook for a given thread (not necessarily the current one).
   */
   static struct stw_tls_data *
   stw_tls_data_create(DWORD dwThreadId)
   {
               if (0) {
                  data = calloc(1, sizeof *data);
   if (!data) {
                        #ifndef _GAMING_XBOX
      data->hCallWndProcHook = SetWindowsHookEx(WH_CALLWNDPROC,
                  #else
         #endif
      if (data->hCallWndProcHook == NULL) {
                        no_hook:
         no_data:
         }
      /**
   * Destroy the per-thread data/hook.
   *
   * It is important to remove all hooks when unloading our DLL, otherwise our
   * hook function might be called after it is no longer there.
   */
   static void
   stw_tls_data_destroy(struct stw_tls_data *data)
   {
      assert(data);
   if (!data) {
                  if (0) {
               #ifndef _GAMING_XBOX
      if (data->hCallWndProcHook) {
      UnhookWindowsHookEx(data->hCallWndProcHook);
         #endif
            }
      bool
   stw_tls_init_thread(void)
   {
               if (tlsIndex == TLS_OUT_OF_INDEXES) {
                  data = stw_tls_data_create(GetCurrentThreadId());
   if (!data) {
                              }
      void
   stw_tls_cleanup_thread(void)
   {
               if (tlsIndex == TLS_OUT_OF_INDEXES) {
                  data = (struct stw_tls_data *) TlsGetValue(tlsIndex);
   if (data) {
         } else {
      /* See if there this thread's data in on the pending list */
               if (data) {
            }
      void
   stw_tls_cleanup(void)
   {
      if (tlsIndex != TLS_OUT_OF_INDEXES) {
      /*
   * Destroy all items in g_pendingTlsData linked list.
   */
   EnterCriticalSection(&g_mutex);
   while (g_pendingTlsData) {
      struct stw_tls_data * data = g_pendingTlsData;
   g_pendingTlsData = data->next;
      }
            TlsFree(tlsIndex);
         }
      /*
   * Search for the current thread in the g_pendingTlsData linked list.
   *
   * It will remove and return the node on success, or return NULL on failure.
   */
   static struct stw_tls_data *
   stw_tls_lookup_pending_data(DWORD dwThreadId)
   {
      struct stw_tls_data ** p_data;
            EnterCriticalSection(&g_mutex);
   for (p_data = &g_pendingTlsData; *p_data; p_data = &(*p_data)->next) {
      if ((*p_data)->dwThreadId == dwThreadId) {
         /*
      * Unlink the node.
   */
         *p_data = data->next;
      break;
            }
               }
      struct stw_tls_data *
   stw_tls_get_data(void)
   {
      struct stw_tls_data *data;
      if (tlsIndex == TLS_OUT_OF_INDEXES) {
         }
      data = (struct stw_tls_data *) TlsGetValue(tlsIndex);
   if (!data) {
               /*
   * Search for the current thread in the g_pendingTlsData linked list.
   */
            if (!data) {
      /*
      assert(!"Failed to find thread data for thread id");
               /*
   * DllMain is called with DLL_THREAD_ATTACH only by threads created
   * after the DLL is loaded by the process
   */
   data = stw_tls_data_create(dwCurrentThreadId);
   if (!data) {
                                 assert(data);
   assert(data->dwThreadId == GetCurrentThreadId());
               }
