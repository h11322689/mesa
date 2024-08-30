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
      #include "d3d12_screen.h"
   #include "d3d12_public.h"
   #include "d3d12_debug.h"
      #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/u_dl.h"
      #include <dxgi1_4.h>
      static IDXGIFactory4 *
   get_dxgi_factory()
   {
      static const GUID IID_IDXGIFactory4 = {
      0x1bc6ea02, 0xef36, 0x464f,
               util_dl_library *dxgi_mod = util_dl_open(UTIL_DL_PREFIX "dxgi" UTIL_DL_EXT);
   if (!dxgi_mod) {
      debug_printf("D3D12: failed to load DXGI.DLL\n");
               typedef HRESULT(WINAPI *PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID riid, void **ppFactory);
            CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)util_dl_get_proc_address(dxgi_mod, "CreateDXGIFactory2");
   if (!CreateDXGIFactory2) {
      debug_printf("D3D12: failed to load CreateDXGIFactory2 from DXGI.DLL\n");
                  #ifndef DEBUG
         #endif
               IDXGIFactory4 *factory = NULL;
   HRESULT hr = CreateDXGIFactory2(flags, IID_IDXGIFactory4, (void **)&factory);
   if (FAILED(hr)) {
      debug_printf("D3D12: CreateDXGIFactory2 failed: %08x\n", hr);
                  }
      static IDXGIAdapter3 *
   choose_dxgi_adapter(IDXGIFactory4 *factory, LUID *adapter)
   {
      IDXGIAdapter3 *ret;
   if (adapter) {
      if (SUCCEEDED(factory->EnumAdapterByLuid(*adapter,
                           bool want_warp = debug_get_bool_option("LIBGL_ALWAYS_SOFTWARE", false);
   if (want_warp) {
      if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&ret))))
         debug_printf("D3D12: failed to enum warp adapter\n");
               // The first adapter is the default
   IDXGIAdapter1 *adapter1;
   if (SUCCEEDED(factory->EnumAdapters1(0, &adapter1))) {
      HRESULT hr = adapter1->QueryInterface(&ret);
   adapter1->Release();
   if (SUCCEEDED(hr))
                  }
      static const char *
   dxgi_get_name(struct pipe_screen *screen)
   {
      struct d3d12_dxgi_screen *dxgi_screen = d3d12_dxgi_screen(d3d12_screen(screen));
   static char buf[1000];
   if (dxgi_screen->description[0] == L'\0')
            snprintf(buf, sizeof(buf), "D3D12 (%S)", dxgi_screen->description);
      }
      static void
   dxgi_get_memory_info(struct d3d12_screen *screen, struct d3d12_memory_info *output)
   {
      struct d3d12_dxgi_screen *dxgi_screen = d3d12_dxgi_screen(screen);
   DXGI_QUERY_VIDEO_MEMORY_INFO local_info, nonlocal_info;
   dxgi_screen->adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local_info);
   dxgi_screen->adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonlocal_info);
   output->budget = local_info.Budget + nonlocal_info.Budget;
      }
      static void
   d3d12_deinit_dxgi_screen(struct d3d12_screen *dscreen)
   {
      d3d12_deinit_screen(dscreen);
   struct d3d12_dxgi_screen *screen = d3d12_dxgi_screen(dscreen);
   if (screen->adapter) {
      screen->adapter->Release();
      }
   if (screen->factory) {
      screen->factory->Release();
         }
      static void
   d3d12_destroy_dxgi_screen(struct pipe_screen *pscreen)
   {
      struct d3d12_screen *screen = d3d12_screen(pscreen);
   d3d12_deinit_dxgi_screen(screen);
      }
      static bool
   d3d12_init_dxgi_screen(struct d3d12_screen *dscreen)
   {
      struct d3d12_dxgi_screen *screen = d3d12_dxgi_screen(dscreen);
   screen->factory = get_dxgi_factory();
   if (!screen->factory)
            LUID *adapter_luid = &dscreen->adapter_luid;
   if (adapter_luid->HighPart == 0 && adapter_luid->LowPart == 0)
            screen->adapter = choose_dxgi_adapter(screen->factory, adapter_luid);
   if (!screen->adapter) {
      debug_printf("D3D12: no suitable adapter\n");
               DXGI_ADAPTER_DESC1 adapter_desc = {};
   if (FAILED(screen->adapter->GetDesc1(&adapter_desc))) {
      debug_printf("D3D12: failed to retrieve adapter description\n");
               LARGE_INTEGER driver_version;
   screen->adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driver_version);
            screen->base.vendor_id = adapter_desc.VendorId;
   screen->base.device_id = adapter_desc.DeviceId;
   screen->base.subsys_id = adapter_desc.SubSysId;
   screen->base.revision = adapter_desc.Revision;
   // Note: memory sizes in bytes, but stored in size_t, so may be capped at 4GB.
   // In that case, adding before conversion to MB can easily overflow.
   screen->base.memory_size_megabytes = (adapter_desc.DedicatedVideoMemory >> 20) +
               wcsncpy(screen->description, adapter_desc.Description, ARRAY_SIZE(screen->description));
   screen->base.base.get_name = dxgi_get_name;
            if (!d3d12_init_screen(&screen->base, screen->adapter)) {
      debug_printf("D3D12: failed to initialize DXGI screen\n");
                  }
      struct pipe_screen *
   d3d12_create_dxgi_screen(struct sw_winsys *winsys, LUID *adapter_luid)
   {
      struct d3d12_dxgi_screen *screen = CALLOC_STRUCT(d3d12_dxgi_screen);
   if (!screen)
            if (!d3d12_init_screen_base(&screen->base, winsys, adapter_luid)) {
      d3d12_destroy_screen(&screen->base);
      }
   screen->base.base.destroy = d3d12_destroy_dxgi_screen;
   screen->base.init = d3d12_init_dxgi_screen;
            if (!d3d12_init_dxgi_screen(&screen->base)) {
      d3d12_destroy_dxgi_screen(&screen->base.base);
                  }
