   /**************************************************************************
   *
   * Copyright 2015, 2018 Collabora
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #ifdef HAVE_LIBDRM
   #include <xf86drm.h>
   #endif
   #include "util/compiler.h"
   #include "util/macros.h"
      #include "eglcurrent.h"
   #include "egldevice.h"
   #include "eglglobals.h"
   #include "egllog.h"
   #include "egltypedefs.h"
      struct _egl_device {
                        EGLBoolean MESA_device_software;
   EGLBoolean EXT_device_drm;
         #ifdef HAVE_LIBDRM
         #endif
   };
      void
   _eglFiniDevice(void)
   {
                                 /* The first device is static allocated SW device */
   assert(dev_list);
   assert(_eglDeviceSupports(dev_list, _EGL_DEVICE_SOFTWARE));
            while (dev_list) {
      /* pop list head */
   dev = dev_list;
      #ifdef HAVE_LIBDRM
         assert(_eglDeviceSupports(dev, _EGL_DEVICE_DRM));
   #endif
                        }
      EGLBoolean
   _eglCheckDeviceHandle(EGLDeviceEXT device)
   {
               simple_mtx_lock(_eglGlobal.Mutex);
   cur = _eglGlobal.DeviceList;
   while (cur) {
      if (cur == (_EGLDevice *)device)
            }
   simple_mtx_unlock(_eglGlobal.Mutex);
      }
      _EGLDevice _eglSoftwareDevice = {
      /* TODO: EGL_EXT_device_drm support for KMS + llvmpipe */
   .extensions = "EGL_MESA_device_software EGL_EXT_device_drm_render_node",
   .MESA_device_software = EGL_TRUE,
      };
      #ifdef HAVE_LIBDRM
   /*
   * Negative value on error, zero if newly added, one if already in list.
   */
   static int
   _eglAddDRMDevice(drmDevicePtr device)
   {
                        /* TODO: uncomment this assert, which is a sanity check.
   *
   * assert(device->available_nodes & ((1 << DRM_NODE_PRIMARY)));
   *
   * DRM shim does not expose a primary node, so the CI would fail if we had
   * this assert. DRM shim is being used to run shader-db. We need to
   * investigate what should be done (probably fixing DRM shim).
                     /* The first device is always software */
   assert(dev);
            while (dev->Next) {
               assert(_eglDeviceSupports(dev, _EGL_DEVICE_DRM));
   if (drmDevicesEqual(device, dev->device) != 0)
               dev->Next = calloc(1, sizeof(_EGLDevice));
   if (!dev->Next)
            dev = dev->Next;
   dev->extensions = "EGL_EXT_device_drm EGL_EXT_device_drm_render_node";
   dev->EXT_device_drm = EGL_TRUE;
   dev->EXT_device_drm_render_node = EGL_TRUE;
               }
   #endif
      /* Finds a device in DeviceList, for the given fd.
   *
   * The fd must be of a render-capable device, as there are only render-capable
   * devices in DeviceList.
   *
   * If a software device, the fd is ignored.
   */
   _EGLDevice *
   _eglFindDevice(int fd, bool software)
   {
               simple_mtx_lock(_eglGlobal.Mutex);
            /* The first device is always software */
   assert(dev);
   assert(_eglDeviceSupports(dev, _EGL_DEVICE_SOFTWARE));
   if (software)
         #ifdef HAVE_LIBDRM
               if (drmGetDevice2(fd, 0, &device) != 0) {
      dev = NULL;
               while (dev->Next) {
               if (_eglDeviceSupports(dev, _EGL_DEVICE_DRM) &&
      drmDevicesEqual(device, dev->device) != 0) {
                  /* Couldn't find an EGLDevice for the device. */
         cleanup_drm:
            #else
      _eglLog(_EGL_FATAL,
            #endif
      out:
      simple_mtx_unlock(_eglGlobal.Mutex);
      }
      #ifdef HAVE_LIBDRM
   drmDevicePtr
   _eglDeviceDrm(_EGLDevice *dev)
   {
      if (!dev)
               }
   #endif
      _EGLDevice *
   _eglDeviceNext(_EGLDevice *dev)
   {
      if (!dev)
               }
      EGLBoolean
   _eglDeviceSupports(_EGLDevice *dev, _EGLDeviceExtension ext)
   {
      switch (ext) {
   case _EGL_DEVICE_SOFTWARE:
         case _EGL_DEVICE_DRM:
         case _EGL_DEVICE_DRM_RENDER_NODE:
         default:
      assert(0);
         }
      EGLBoolean
   _eglQueryDeviceAttribEXT(_EGLDevice *dev, EGLint attribute, EGLAttrib *value)
   {
      switch (attribute) {
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglQueryDeviceAttribEXT");
         }
      const char *
   _eglQueryDeviceStringEXT(_EGLDevice *dev, EGLint name)
   {
      switch (name) {
   case EGL_EXTENSIONS:
         case EGL_DRM_DEVICE_FILE_EXT:
      if (!_eglDeviceSupports(dev, _EGL_DEVICE_DRM))
   #ifdef HAVE_LIBDRM
         #else
         /* This should never happen: we don't yet support EGL_DEVICE_DRM for the
   * software device, and physical devices are only exposed when libdrm is
   * available. */
   assert(0);
   #endif
      case EGL_DRM_RENDER_NODE_FILE_EXT:
      if (!_eglDeviceSupports(dev, _EGL_DEVICE_DRM_RENDER_NODE))
   #ifdef HAVE_LIBDRM
         /* EGLDevice represents a software device, so no render node
   * should be advertised. */
   if (_eglDeviceSupports(dev, _EGL_DEVICE_SOFTWARE))
         /* We create EGLDevice's only for render capable devices. */
   assert(dev->device->available_nodes & (1 << DRM_NODE_RENDER));
   #else
         /* Physical devices are only exposed when libdrm is available. */
   assert(_eglDeviceSupports(dev, _EGL_DEVICE_SOFTWARE));
   #endif
      }
   _eglError(EGL_BAD_PARAMETER, "eglQueryDeviceStringEXT");
      }
      /* Do a fresh lookup for devices.
   *
   * Walks through the DeviceList, discarding no longer available ones
   * and adding new ones as applicable.
   *
   * Must be called with the global lock held.
   */
   int
   _eglDeviceRefreshList(void)
   {
      ASSERTED _EGLDevice *dev;
                     /* The first device is always software */
   assert(dev);
   assert(_eglDeviceSupports(dev, _EGL_DEVICE_SOFTWARE));
         #ifdef HAVE_LIBDRM
      drmDevicePtr devices[64];
            num_devs = drmGetDevices2(0, devices, ARRAY_SIZE(devices));
   for (int i = 0; i < num_devs; i++) {
      if (!(devices[i]->available_nodes & (1 << DRM_NODE_RENDER))) {
      drmFreeDevice(&devices[i]);
                        /* Device is not added - error or already present */
   if (ret != 0)
            if (ret >= 0)
         #endif
            }
      EGLBoolean
   _eglQueryDevicesEXT(EGLint max_devices, _EGLDevice **devices,
         {
      _EGLDevice *dev, *devs, *swrast;
            if ((devices && max_devices <= 0) || !num_devices)
                     num_devs = _eglDeviceRefreshList();
         #ifdef GALLIUM_SOFTPIPE
         #else
      swrast = NULL;
      #endif
         /* The first device is swrast. Start with the non-swrast device. */
            /* bail early if we only care about the count */
   if (!devices) {
      *num_devices = num_devs;
                        /* Add non-swrast devices first and add swrast last.
   *
   * By default, the user is likely to pick the first device so having the
   * software (aka least performant) one is not a good idea.
   */
   for (i = 0, dev = devs; dev && i < max_devices; i++) {
      devices[i] = dev;
               /* User requested the full device list, add the software device. */
   if (max_devices >= num_devs && swrast) {
      assert(_eglDeviceSupports(swrast, _EGL_DEVICE_SOFTWARE));
            out:
                  }
