   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <fcntl.h>
   #include <stdbool.h>
   #include <vulkan/vulkan.h>
   #include <xf86drm.h>
      #include "powervr/pvr_drm_public.h"
   #include "pvr_private.h"
   #include "pvr_winsys.h"
   #include "vk_log.h"
      #if defined(PVR_SUPPORT_SERVICES_DRIVER)
   #   include "pvrsrvkm/pvr_srv_public.h"
   #endif
      void pvr_winsys_destroy(struct pvr_winsys *ws)
   {
      const int display_fd = ws->display_fd;
                     if (display_fd >= 0)
            if (render_fd >= 0)
      }
      VkResult pvr_winsys_create(const char *render_path,
                     {
      drmVersionPtr version;
   VkResult result;
   int display_fd;
            render_fd = open(render_path, O_RDWR | O_CLOEXEC);
   if (render_fd < 0) {
      result = vk_errorf(NULL,
                                 if (display_path) {
      display_fd = open(display_path, O_RDWR | O_CLOEXEC);
   if (display_fd < 0) {
      result = vk_errorf(NULL,
                           } else {
                  version = drmGetVersion(render_fd);
   if (!version) {
      result = vk_errorf(NULL,
                           if (strcmp(version->name, "powervr") == 0) {
      #if defined(PVR_SUPPORT_SERVICES_DRIVER)
      } else if (strcmp(version->name, "pvr") == 0) {
      #endif
      } else {
      result = vk_errorf(
      NULL,
   VK_ERROR_INCOMPATIBLE_DRIVER,
                     if (result != VK_SUCCESS)
                  err_close_display_fd:
      if (display_fd >= 0)
         err_close_render_fd:
            err_out:
         }
