   /**************************************************************************
   *
   * Copyright 2012-2021 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   * THE SOFTWARE.
   *
   **************************************************************************/
         #include <stdio.h>
   #include <stddef.h>
   #include <string.h>
      #include <initguid.h>
   #include <windows.h>
      #include <d3d11.h>
      #include <wrl/client.h>
      using Microsoft::WRL::ComPtr;
      #include "tri_vs_4_0.h"
   #include "tri_ps_4_0.h"
         int
   main(int argc, char *argv[])
   {
                        WNDCLASSEX wc = {
      sizeof(WNDCLASSEX),
   CS_CLASSDC,
   DefWindowProc,
   0,
   0,
   hInstance,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   "tri",
      };
            const int WindowWidth = 250;
                     RECT rect = {0, 0, WindowWidth, WindowHeight};
            HWND hWnd = CreateWindow(wc.lpszClassName,
                           "Simple example using DirectX10",
   dwStyle,
   CW_USEDEFAULT, CW_USEDEFAULT,
   rect.right - rect.left,
   rect.bottom - rect.top,
   if (!hWnd) {
                  UINT Flags = 0;
   hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_NULL, 0, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, nullptr, nullptr, nullptr);
   if (SUCCEEDED(hr)) {
                  static const D3D_FEATURE_LEVEL FeatureLevels[] = {
                  HMODULE hSoftware = LoadLibraryA("d3d10sw.dll");
   if (!hSoftware) {
                  DXGI_SWAP_CHAIN_DESC SwapChainDesc;
   ZeroMemory(&SwapChainDesc, sizeof SwapChainDesc);
   SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;;
   SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
   SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
   SwapChainDesc.SampleDesc.Quality = 0;
   SwapChainDesc.SampleDesc.Count = 1;
   SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
   SwapChainDesc.BufferCount = 2;
   SwapChainDesc.OutputWindow = hWnd;
   SwapChainDesc.Windowed = true;
            ComPtr<ID3D11Device> pDevice;
   ComPtr<ID3D11DeviceContext> pDeviceContext;
   ComPtr<IDXGISwapChain> pSwapChain;
   hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                       D3D_DRIVER_TYPE_SOFTWARE,
   hSoftware,
   Flags,
   FeatureLevels,
   _countof(FeatureLevels),
   if (FAILED(hr)) {
                  ComPtr<IDXGIDevice> pDXGIDevice;
   hr = pDevice->QueryInterface(IID_IDXGIDevice, (void **)&pDXGIDevice);
   if (FAILED(hr)) {
                  ComPtr<IDXGIAdapter> pAdapter;
   hr = pDXGIDevice->GetAdapter(&pAdapter);
   if (FAILED(hr)) {
                  DXGI_ADAPTER_DESC Desc;
   hr = pAdapter->GetDesc(&Desc);
   if (FAILED(hr)) {
                           ComPtr<ID3D11Texture2D> pBackBuffer;
   hr = pSwapChain->GetBuffer(0, IID_ID3D11Texture2D, (void **)&pBackBuffer);
   if (FAILED(hr)) {
                  D3D11_RENDER_TARGET_VIEW_DESC RenderTargetViewDesc;
   ZeroMemory(&RenderTargetViewDesc, sizeof RenderTargetViewDesc);
   RenderTargetViewDesc.Format = SwapChainDesc.BufferDesc.Format;
   RenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            ComPtr<ID3D11RenderTargetView> pRenderTargetView;
   hr = pDevice->CreateRenderTargetView(pBackBuffer.Get(), &RenderTargetViewDesc, &pRenderTargetView);
   if (FAILED(hr)) {
                              const float clearColor[4] = { 0.3f, 0.1f, 0.3f, 1.0f };
            ComPtr<ID3D11VertexShader> pVertexShader;
   hr = pDevice->CreateVertexShader(g_VS, sizeof g_VS, nullptr, &pVertexShader);
   if (FAILED(hr)) {
                  struct Vertex {
      float position[4];
               static const D3D11_INPUT_ELEMENT_DESC InputElementDescs[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
               ComPtr<ID3D11InputLayout> pVertexLayout;
   hr = pDevice->CreateInputLayout(InputElementDescs,
                     if (FAILED(hr)) {
                           ComPtr<ID3D11PixelShader> pPixelShader;
   hr = pDevice->CreatePixelShader(g_PS, sizeof g_PS, nullptr, &pPixelShader);
   if (FAILED(hr)) {
                  pDeviceContext->VSSetShader(pVertexShader.Get(), nullptr, 0);
            static const Vertex vertices[] = {
      { { -0.9f, -0.9f, 0.5f, 1.0f}, { 0.8f, 0.0f, 0.0f, 0.1f } },
   { {  0.9f, -0.9f, 0.5f, 1.0f}, { 0.0f, 0.9f, 0.0f, 0.1f } },
               D3D11_BUFFER_DESC BufferDesc;
   ZeroMemory(&BufferDesc, sizeof BufferDesc);
   BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
   BufferDesc.ByteWidth = sizeof vertices;
   BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
   BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            D3D11_SUBRESOURCE_DATA BufferData;
   BufferData.pSysMem = vertices;
   BufferData.SysMemPitch = 0;
            ComPtr<ID3D11Buffer> pVertexBuffer;
   hr = pDevice->CreateBuffer(&BufferDesc, &BufferData, &pVertexBuffer);
   if (FAILED(hr)) {
                  UINT Stride = sizeof(Vertex);
   UINT Offset = 0;
            D3D11_VIEWPORT ViewPort;
   ViewPort.TopLeftX = 0;
   ViewPort.TopLeftY = 0;
   ViewPort.Width = WindowWidth;
   ViewPort.Height = WindowHeight;
   ViewPort.MinDepth = 0.0f;
   ViewPort.MaxDepth = 1.0f;
            D3D11_RASTERIZER_DESC RasterizerDesc;
   ZeroMemory(&RasterizerDesc, sizeof RasterizerDesc);
   RasterizerDesc.CullMode = D3D11_CULL_NONE;
   RasterizerDesc.FillMode = D3D11_FILL_SOLID;
   RasterizerDesc.FrontCounterClockwise = true;
   RasterizerDesc.DepthClipEnable = true;
   ComPtr<ID3D11RasterizerState> pRasterizerState;
   hr = pDevice->CreateRasterizerState(&RasterizerDesc, &pRasterizerState);
   if (FAILED(hr)) {
         }
                                                ID3D11Buffer *pNullBuffer = nullptr;
   UINT NullStride = 0;
   UINT NullOffset = 0;
                                                                     }
   