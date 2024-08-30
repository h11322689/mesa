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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <gtest/gtest.h>
      #include <windows.h>
   #include <unknwn.h>
   #include <GL/gl.h>
      #undef GetMessage
      class window
   {
   public:
      window(UINT width = 64, UINT height = 64);
            HWND get_hwnd() const { return _window; };
   HDC get_hdc() const { return _hdc; };
   bool valid() const { return _window && _hdc && _hglrc; }
   void show() {
                        private:
      HWND _window = nullptr;
   HDC _hdc = nullptr;
      };
      window::window(uint32_t width, uint32_t height)
   {
      _window = CreateWindowW(
      L"STATIC",
   L"OpenGLTestWindow",
   WS_OVERLAPPEDWINDOW,
   0,
   0,
   width,
   height,
   NULL,
   NULL,
   NULL,
               if (_window == nullptr)
                     PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),  /* size */
   1,                              /* version */
   PFD_SUPPORT_OPENGL |
   PFD_DRAW_TO_WINDOW |
   PFD_DOUBLEBUFFER,               /* support double-buffering */
   PFD_TYPE_RGBA,                  /* color type */
   8,                              /* prefered color depth */
   0, 0, 0, 0, 0, 0,               /* color bits (ignored) */
   0,                              /* no alpha buffer */
   0,                              /* alpha bits (ignored) */
   0,                              /* no accumulation buffer */
   0, 0, 0, 0,                     /* accum bits (ignored) */
   32,                             /* depth buffer */
   0,                              /* no stencil buffer */
   0,                              /* no auxiliary buffers */
   PFD_MAIN_PLANE,                 /* main layer */
   0,                              /* reserved */
      };
   int pixel_format = ChoosePixelFormat(_hdc, &pfd);
   if (pixel_format == 0)
         if (!SetPixelFormat(_hdc, pixel_format, &pfd))
            _hglrc = wglCreateContext(_hdc);
   if (!_hglrc)
               }
      void window::recreate_attribs(const int *attribs)
   {
      using pwglCreateContextAttribsARB = HGLRC(WINAPI*)(HDC, HGLRC, const int *);
   auto wglCreateContextAttribsARB = (pwglCreateContextAttribsARB)wglGetProcAddress("wglCreateContextAttribsARB");
   if (!wglCreateContextAttribsARB)
            wglMakeCurrent(nullptr, nullptr);
   wglDeleteContext(_hglrc);
   _hglrc = wglCreateContextAttribsARB(_hdc, nullptr, attribs);
   if (!_hglrc)
               }
      window::~window()
   {
      if (_hglrc) {
      wglMakeCurrent(NULL, NULL);
      }
   if (_hdc)
         if (_window)
      }
      TEST(wgl, basic_create)
   {
      window wnd;
            const char *version = (const char *)glGetString(GL_VERSION);
      }
      #ifdef GALLIUM_D3D12
   /* Fixture for tests for the d3d12 backend. Will be skipped if
   * the environment isn't set up to run them.
   */
   #include <directx/d3d12.h>
   #include <dxguids/dxguids.h>
   #include <wrl/client.h>
   #include <memory>
   using Microsoft::WRL::ComPtr;
      class d3d12 : public ::testing::Test
   {
         };
      void d3d12::SetUp()
   {
      window wnd;
            const char *renderer = (const char *)glGetString(GL_RENDERER);
   if (!strstr(renderer, "D3D12"))
      }
      static bool
   info_queue_has_swapchain(ID3D12DebugDevice *debug_device, ID3D12InfoQueue *info_queue)
   {
                        uint32_t num_messages = info_queue->GetNumStoredMessages();
   for (uint32_t i = 0; i < num_messages; ++i) {
      SIZE_T message_size = 0;
   info_queue->GetMessage(i, nullptr, &message_size);
            std::unique_ptr<byte[]> message_bytes(new byte[message_size]);
   D3D12_MESSAGE *message = (D3D12_MESSAGE *)message_bytes.get();
            if (strstr(message->pDescription, "SwapChain")) {
      info_queue->ClearStoredMessages();
   info_queue->PopStorageFilter();
         }
   info_queue->ClearStoredMessages();
   info_queue->PopStorageFilter();
      }
      TEST_F(d3d12, swapchain_cleanup)
   {
      ComPtr<ID3D12InfoQueue> info_queue;
   ComPtr<ID3D12DebugDevice> debug_device;
   if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&info_queue))) ||
      FAILED(info_queue.As(&debug_device)))
                  {
      window wnd;
   wnd.show();
   glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);
                           }
      #define WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
   #define WGL_LOSE_CONTEXT_ON_RESET_ARB               0x8252
   using pglGetGraphicsResetStatusARB = GLenum(APIENTRY*)();
   TEST_F(d3d12, context_reset)
   {
      ComPtr<ID3D12Device5> device;
   if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
                     {
      window wnd;
   wnd.recreate_attribs(attribs);
            wnd.show();
   glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);
            auto glGetGraphicsResetStatusARB = (pglGetGraphicsResetStatusARB)wglGetProcAddress("glGetGraphicsResetStatusARB");
   if (!glGetGraphicsResetStatusARB)
                     device->RemoveDevice();
                        {
      window wnd;
            wnd.recreate_attribs(attribs);
            wnd.show();
   auto glGetGraphicsResetStatusARB = (pglGetGraphicsResetStatusARB)wglGetProcAddress("glGetGraphicsResetStatusARB");
            glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);
         }
   #endif
