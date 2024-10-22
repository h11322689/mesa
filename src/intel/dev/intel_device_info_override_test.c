   /*
   * Copyright © 2022 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #undef NDEBUG
      #include <assert.h>
   #include <stdlib.h>
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>
   #include <unistd.h>
      #include <xf86drm.h>
      #include "intel_device_info.h"
   #include "intel_device_info_test.h"
      int
   main(int argc, char *argv[])
   {
      drmDevicePtr devices[8];
            if (argc != 2) {
      fprintf(stderr, "format: %s verx10_number\n", argv[0]);
                        max_devices = drmGetDevices2(0, devices, ARRAY_SIZE(devices));
   if (max_devices < 1) {
      fprintf(stderr, "Device not found\n");
               for (int i = 0; i < max_devices; i++) {
      struct intel_device_info devinfo;
   const char *path = devices[i]->nodes[DRM_NODE_RENDER];
            if (fd < 0)
            bool success = intel_get_device_info_from_fd(fd, &devinfo);
            if (!success)
            fprintf(stderr, "%u\n", devinfo.verx10);
   assert(devinfo.verx10 == verx10);
   verify_device_info(&devinfo);
                  }
