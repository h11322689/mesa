   /*
   * Copyright © 2017 Keith Packard
   *
   * Permission to use, copy, modify, distribute, and sell this software and its
   * documentation for any purpose is hereby granted without fee, provided that
   * the above copyright notice appear in all copies and that both that copyright
   * notice and this permission notice appear in supporting documentation, and
   * that the name of the copyright holders not be used in advertising or
   * publicity pertaining to distribution of the software without specific,
   * written prior permission.  The copyright holders make no representations
   * about the suitability of this software for any purpose.  It is provided "as
   * is" without express or implied warranty.
   *
   * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
   * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
   * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
   * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
   * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   * OF THIS SOFTWARE.
   */
      #include "util/macros.h"
   #include <stdlib.h>
   #include <stdio.h>
   #include <sys/stat.h>
   #include <unistd.h>
   #include <errno.h>
   #include <string.h>
   #include <fcntl.h>
   #include <poll.h>
   #include <stdbool.h>
   #include <math.h>
   #include <xf86drm.h>
   #include <xf86drmMode.h>
   #ifdef HAVE_LIBUDEV
   #include <libudev.h>
   #endif
   #include "drm-uapi/drm_fourcc.h"
   #ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
   #include <xcb/randr.h>
   #include <X11/Xlib-xcb.h>
   #endif
   #include "util/hash_table.h"
   #include "util/list.h"
   #include "util/os_time.h"
   #include "util/timespec.h"
      #include "vk_device.h"
   #include "vk_fence.h"
   #include "vk_instance.h"
   #include "vk_physical_device.h"
   #include "vk_sync.h"
   #include "vk_util.h"
   #include "wsi_common_entrypoints.h"
   #include "wsi_common_private.h"
   #include "wsi_common_display.h"
   #include "wsi_common_queue.h"
      #if 0
   #define wsi_display_debug(...) fprintf(stderr, __VA_ARGS__)
   #define wsi_display_debug_code(...)     __VA_ARGS__
   #else
   #define wsi_display_debug(...)
   #define wsi_display_debug_code(...)
   #endif
      /* These have lifetime equal to the instance, so they effectively
   * never go away. This means we must keep track of them separately
   * from all other resources.
   */
   typedef struct wsi_display_mode {
      struct list_head             list;
   struct wsi_display_connector *connector;
   bool                         valid; /* was found in most recent poll */
   bool                         preferred;
   uint32_t                     clock; /* in kHz */
   uint16_t                     hdisplay, hsync_start, hsync_end, htotal, hskew;
   uint16_t                     vdisplay, vsync_start, vsync_end, vtotal, vscan;
      } wsi_display_mode;
      typedef struct wsi_display_connector {
      struct list_head             list;
   struct wsi_display           *wsi;
   uint32_t                     id;
   uint32_t                     crtc_id;
   char                         *name;
   bool                         connected;
   bool                         active;
   struct list_head             display_modes;
   wsi_display_mode             *current_mode;
   drmModeModeInfo              current_drm_mode;
      #ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
         #endif
   } wsi_display_connector;
      struct wsi_display {
                                 /* Used with syncobj imported from driver side. */
            pthread_mutex_t              wait_mutex;
   pthread_cond_t               wait_cond;
            pthread_cond_t               hotplug_cond;
               };
      #define wsi_for_each_display_mode(_mode, _conn)                 \
      list_for_each_entry_safe(struct wsi_display_mode, _mode,     \
         #define wsi_for_each_connector(_conn, _dev)                             \
      list_for_each_entry_safe(struct wsi_display_connector, _conn,        \
         enum wsi_image_state {
      WSI_IMAGE_IDLE,
   WSI_IMAGE_DRAWING,
   WSI_IMAGE_QUEUED,
   WSI_IMAGE_FLIPPING,
      };
      struct wsi_display_image {
      struct wsi_image             base;
   struct wsi_display_swapchain *chain;
   enum wsi_image_state         state;
   uint32_t                     fb_id;
   uint32_t                     buffer[4];
   uint64_t                     flip_sequence;
      };
      struct wsi_display_swapchain {
      struct wsi_swapchain         base;
   struct wsi_display           *wsi;
   VkIcdSurfaceDisplay          *surface;
   uint64_t                     flip_sequence;
            pthread_mutex_t              present_id_mutex;
   pthread_cond_t               present_id_cond;
   uint64_t                     present_id;
               };
      struct wsi_display_fence {
      struct list_head             link;
   struct wsi_display           *wsi;
   bool                         event_received;
   bool                         destroyed;
   uint32_t                     syncobj; /* syncobj to signal on event */
   uint64_t                     sequence;
      };
      struct wsi_display_sync {
      struct vk_sync               sync;
      };
      static uint64_t fence_sequence;
      ICD_DEFINE_NONDISP_HANDLE_CASTS(wsi_display_mode, VkDisplayModeKHR)
   ICD_DEFINE_NONDISP_HANDLE_CASTS(wsi_display_connector, VkDisplayKHR)
      static bool
   wsi_display_mode_matches_drm(wsi_display_mode *wsi,
         {
      return wsi->clock == drm->clock &&
      wsi->hdisplay == drm->hdisplay &&
   wsi->hsync_start == drm->hsync_start &&
   wsi->hsync_end == drm->hsync_end &&
   wsi->htotal == drm->htotal &&
   wsi->hskew == drm->hskew &&
   wsi->vdisplay == drm->vdisplay &&
   wsi->vsync_start == drm->vsync_start &&
   wsi->vsync_end == drm->vsync_end &&
   wsi->vtotal == drm->vtotal &&
   MAX2(wsi->vscan, 1) == MAX2(drm->vscan, 1) &&
   }
      static double
   wsi_display_mode_refresh(struct wsi_display_mode *wsi)
   {
      return (double) wsi->clock * 1000.0 / ((double) wsi->htotal *
            }
      static uint64_t wsi_rel_to_abs_time(uint64_t rel_time)
   {
               /* check for overflow */
   if (rel_time > UINT64_MAX - current_time)
               }
      static struct wsi_display_mode *
   wsi_display_find_drm_mode(struct wsi_display_connector *connector,
         {
      wsi_for_each_display_mode(display_mode, connector) {
      if (wsi_display_mode_matches_drm(display_mode, mode))
      }
      }
      static void
   wsi_display_invalidate_connector_modes(struct wsi_display_connector *connector)
   {
      wsi_for_each_display_mode(display_mode, connector) {
            }
      static VkResult
   wsi_display_register_drm_mode(struct wsi_device *wsi_device,
               {
      struct wsi_display *wsi =
         struct wsi_display_mode *display_mode =
            if (display_mode) {
      display_mode->valid = true;
               display_mode = vk_zalloc(wsi->alloc, sizeof (struct wsi_display_mode),
         if (!display_mode)
            display_mode->connector = connector;
   display_mode->valid = true;
   display_mode->preferred = (drm_mode->type & DRM_MODE_TYPE_PREFERRED) != 0;
   display_mode->clock = drm_mode->clock; /* kHz */
   display_mode->hdisplay = drm_mode->hdisplay;
   display_mode->hsync_start = drm_mode->hsync_start;
   display_mode->hsync_end = drm_mode->hsync_end;
   display_mode->htotal = drm_mode->htotal;
   display_mode->hskew = drm_mode->hskew;
   display_mode->vdisplay = drm_mode->vdisplay;
   display_mode->vsync_start = drm_mode->vsync_start;
   display_mode->vsync_end = drm_mode->vsync_end;
   display_mode->vtotal = drm_mode->vtotal;
   display_mode->vscan = drm_mode->vscan;
            list_addtail(&display_mode->list, &connector->display_modes);
      }
      /*
   * Update our information about a specific connector
   */
      static struct wsi_display_connector *
   wsi_display_find_connector(struct wsi_device *wsi_device,
         {
      struct wsi_display *wsi =
            wsi_for_each_connector(connector, wsi) {
      if (connector->id == connector_id)
                  }
      static struct wsi_display_connector *
   wsi_display_alloc_connector(struct wsi_display *wsi,
         {
      struct wsi_display_connector *connector =
      vk_zalloc(wsi->alloc, sizeof (struct wsi_display_connector),
      if (!connector)
            connector->id = connector_id;
   connector->wsi = wsi;
   connector->active = false;
   /* XXX use EDID name */
   connector->name = "monitor";
   list_inithead(&connector->display_modes);
      }
      static struct wsi_display_connector *
   wsi_display_get_connector(struct wsi_device *wsi_device,
               {
      struct wsi_display *wsi =
            if (drm_fd < 0)
            drmModeConnectorPtr drm_connector =
            if (!drm_connector)
            struct wsi_display_connector *connector =
            if (!connector) {
      connector = wsi_display_alloc_connector(wsi, connector_id);
   if (!connector) {
      drmModeFreeConnector(drm_connector);
      }
                        /* Look for a DPMS property if we haven't already found one */
   for (int p = 0; connector->dpms_property == 0 &&
         {
      drmModePropertyPtr prop = drmModeGetProperty(drm_fd,
         if (!prop)
         if (prop->flags & DRM_MODE_PROP_ENUM) {
      if (!strcmp(prop->name, "DPMS"))
      }
               /* Mark all connector modes as invalid */
            /*
   * List current modes, adding new ones and marking existing ones as
   * valid
   */
   for (int m = 0; m < drm_connector->count_modes; m++) {
      VkResult result = wsi_display_register_drm_mode(wsi_device,
               if (result != VK_SUCCESS) {
      drmModeFreeConnector(drm_connector);
                              }
      #define MM_PER_PIXEL     (1.0/96.0 * 25.4)
      static uint32_t
   mode_size(struct wsi_display_mode *mode)
   {
      /* fortunately, these are both uint16_t, so this is easy */
      }
      static void
   wsi_display_fill_in_display_properties(struct wsi_display_connector *connector,
         {
      assert(properties2->sType == VK_STRUCTURE_TYPE_DISPLAY_PROPERTIES_2_KHR);
            properties->display = wsi_display_connector_to_handle(connector);
            /* Find the first preferred mode and assume that's the physical
   * resolution. If there isn't a preferred mode, find the largest mode and
   * use that.
            struct wsi_display_mode *preferred_mode = NULL, *largest_mode = NULL;
   wsi_for_each_display_mode(display_mode, connector) {
      if (!display_mode->valid)
         if (display_mode->preferred) {
      preferred_mode = display_mode;
      }
   if (largest_mode == NULL ||
         {
                     if (preferred_mode) {
      properties->physicalResolution.width = preferred_mode->hdisplay;
      } else if (largest_mode) {
      properties->physicalResolution.width = largest_mode->hdisplay;
      } else {
      properties->physicalResolution.width = 1024;
               /* Make up physical size based on 96dpi */
   properties->physicalDimensions.width =
         properties->physicalDimensions.height =
            properties->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
   properties->planeReorderPossible = VK_FALSE;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice,
               {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
   struct wsi_device *wsi_device = pdevice->wsi_device;
   struct wsi_display *wsi =
            if (pProperties == NULL) {
      return wsi_GetPhysicalDeviceDisplayProperties2KHR(physicalDevice,
            } else {
      /* If we're actually returning properties, allocate a temporary array of
   * VkDisplayProperties2KHR structs, call properties2 to fill them out,
   * and then copy them to the client.  This seems a bit expensive but
   * wsi_display_get_physical_device_display_properties2() calls
   * drmModeGetResources() which does an ioctl and then a bunch of
   * allocations so this should get lost in the noise.
   */
   VkDisplayProperties2KHR *props2 =
      vk_zalloc(wsi->alloc, sizeof(*props2) * *pPropertyCount, 8,
      if (props2 == NULL)
            for (uint32_t i = 0; i < *pPropertyCount; i++)
            VkResult result =
                  if (result == VK_SUCCESS || result == VK_INCOMPLETE) {
      for (uint32_t i = 0; i < *pPropertyCount; i++)
                              }
      static VkResult
   wsi_get_connectors(VkPhysicalDevice physicalDevice)
   {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
   struct wsi_device *wsi_device = pdevice->wsi_device;
   struct wsi_display *wsi =
            if (wsi->fd < 0)
                     if (!mode_res)
            /* Get current information */
   for (int c = 0; c < mode_res->count_connectors; c++) {
      struct wsi_display_connector *connector =
      wsi_display_get_connector(wsi_device, wsi->fd,
      if (!connector) {
      drmModeFreeResources(mode_res);
                  drmModeFreeResources(mode_res);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceDisplayProperties2KHR(VkPhysicalDevice physicalDevice,
               {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
   struct wsi_device *wsi_device = pdevice->wsi_device;
   struct wsi_display *wsi =
            /* Get current information */
   VkResult result = wsi_get_connectors(physicalDevice);
   if (result != VK_SUCCESS)
            VK_OUTARRAY_MAKE_TYPED(VkDisplayProperties2KHR, conn,
            wsi_for_each_connector(connector, wsi) {
      if (connector->connected) {
      vk_outarray_append_typed(VkDisplayProperties2KHR, &conn, prop) {
                              bail:
      *pPropertyCount = 0;
      }
      /*
   * Implement vkGetPhysicalDeviceDisplayPlanePropertiesKHR (VK_KHR_display
   */
   static void
   wsi_display_fill_in_display_plane_properties(
      struct wsi_display_connector *connector,
      {
      assert(properties->sType == VK_STRUCTURE_TYPE_DISPLAY_PLANE_PROPERTIES_2_KHR);
            if (connector && connector->active) {
      prop->currentDisplay = wsi_display_connector_to_handle(connector);
      } else {
      prop->currentDisplay = VK_NULL_HANDLE;
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceDisplayPlanePropertiesKHR(VkPhysicalDevice physicalDevice,
               {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
   struct wsi_device *wsi_device = pdevice->wsi_device;
   struct wsi_display *wsi =
            VkResult result = wsi_get_connectors(physicalDevice);
   if (result != VK_SUCCESS)
            VK_OUTARRAY_MAKE_TYPED(VkDisplayPlanePropertiesKHR, conn,
            wsi_for_each_connector(connector, wsi) {
      vk_outarray_append_typed(VkDisplayPlanePropertiesKHR, &conn, prop) {
      VkDisplayPlaneProperties2KHR prop2 = {
         };
   wsi_display_fill_in_display_plane_properties(connector, &prop2);
         }
         bail:
      *pPropertyCount = 0;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceDisplayPlaneProperties2KHR(VkPhysicalDevice physicalDevice,
               {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
   struct wsi_device *wsi_device = pdevice->wsi_device;
   struct wsi_display *wsi =
            /* Get current information */
   VkResult result = wsi_get_connectors(physicalDevice);
   if (result != VK_SUCCESS)
            VK_OUTARRAY_MAKE_TYPED(VkDisplayPlaneProperties2KHR, conn,
            wsi_for_each_connector(connector, wsi) {
      vk_outarray_append_typed(VkDisplayPlaneProperties2KHR, &conn, prop) {
            }
         bail:
      *pPropertyCount = 0;
      }
      /*
   * Implement vkGetDisplayPlaneSupportedDisplaysKHR (VK_KHR_display)
   */
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetDisplayPlaneSupportedDisplaysKHR(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
   struct wsi_device *wsi_device = pdevice->wsi_device;
   struct wsi_display *wsi =
                              wsi_for_each_connector(connector, wsi) {
      if (c == planeIndex && connector->connected) {
      vk_outarray_append_typed(VkDisplayKHR, &conn, display) {
            }
      }
      }
      /*
   * Implement vkGetDisplayModePropertiesKHR (VK_KHR_display)
   */
      static void
   wsi_display_fill_in_display_mode_properties(
      struct wsi_display_mode *display_mode,
      {
      assert(properties->sType == VK_STRUCTURE_TYPE_DISPLAY_MODE_PROPERTIES_2_KHR);
            prop->displayMode = wsi_display_mode_to_handle(display_mode);
   prop->parameters.visibleRegion.width = display_mode->hdisplay;
   prop->parameters.visibleRegion.height = display_mode->vdisplay;
   prop->parameters.refreshRate =
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice,
                     {
      struct wsi_display_connector *connector =
            VK_OUTARRAY_MAKE_TYPED(VkDisplayModePropertiesKHR, conn,
            wsi_for_each_display_mode(display_mode, connector) {
      if (!display_mode->valid)
            vk_outarray_append_typed(VkDisplayModePropertiesKHR, &conn, prop) {
      VkDisplayModeProperties2KHR prop2 = {
         };
   wsi_display_fill_in_display_mode_properties(display_mode, &prop2);
         }
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetDisplayModeProperties2KHR(VkPhysicalDevice physicalDevice,
                     {
      struct wsi_display_connector *connector =
            VK_OUTARRAY_MAKE_TYPED(VkDisplayModeProperties2KHR, conn,
            wsi_for_each_display_mode(display_mode, connector) {
      if (!display_mode->valid)
            vk_outarray_append_typed(VkDisplayModeProperties2KHR, &conn, prop) {
            }
      }
      static bool
   wsi_display_mode_matches_vk(wsi_display_mode *wsi,
         {
      return (vk->visibleRegion.width == wsi->hdisplay &&
            }
      /*
   * Implement vkCreateDisplayModeKHR (VK_KHR_display)
   */
   VKAPI_ATTR VkResult VKAPI_CALL
   wsi_CreateDisplayModeKHR(VkPhysicalDevice physicalDevice,
                           {
      struct wsi_display_connector *connector =
            if (pCreateInfo->flags != 0)
            /* Check and see if the requested mode happens to match an existing one and
   * return that. This makes the conformance suite happy. Doing more than
   * this would involve embedding the CVT function into the driver, which seems
   * excessive.
   */
   wsi_for_each_display_mode(display_mode, connector) {
      if (display_mode->valid) {
      if (wsi_display_mode_matches_vk(display_mode, &pCreateInfo->parameters)) {
      *pMode = wsi_display_mode_to_handle(display_mode);
            }
      }
      /*
   * Implement vkGetDisplayPlaneCapabilities
   */
   VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetDisplayPlaneCapabilitiesKHR(VkPhysicalDevice physicalDevice,
                     {
               /* XXX use actual values */
   pCapabilities->supportedAlpha = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
   pCapabilities->minSrcPosition.x = 0;
   pCapabilities->minSrcPosition.y = 0;
   pCapabilities->maxSrcPosition.x = 0;
   pCapabilities->maxSrcPosition.y = 0;
   pCapabilities->minSrcExtent.width = mode->hdisplay;
   pCapabilities->minSrcExtent.height = mode->vdisplay;
   pCapabilities->maxSrcExtent.width = mode->hdisplay;
   pCapabilities->maxSrcExtent.height = mode->vdisplay;
   pCapabilities->minDstPosition.x = 0;
   pCapabilities->minDstPosition.y = 0;
   pCapabilities->maxDstPosition.x = 0;
   pCapabilities->maxDstPosition.y = 0;
   pCapabilities->minDstExtent.width = mode->hdisplay;
   pCapabilities->minDstExtent.height = mode->vdisplay;
   pCapabilities->maxDstExtent.width = mode->hdisplay;
   pCapabilities->maxDstExtent.height = mode->vdisplay;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetDisplayPlaneCapabilities2KHR(VkPhysicalDevice physicalDevice,
               {
      assert(pCapabilities->sType ==
            VkResult result =
      wsi_GetDisplayPlaneCapabilitiesKHR(physicalDevice,
                     vk_foreach_struct(ext, pCapabilities->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR: {
      VkSurfaceProtectedCapabilitiesKHR *protected = (void *)ext;
   protected->supportsProtected = VK_FALSE;
               default:
      /* Ignored */
                     }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_CreateDisplayPlaneSurfaceKHR(VkInstance _instance,
                     {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
                     surface = vk_zalloc2(&instance->alloc, pAllocator, sizeof(*surface), 8,
         if (surface == NULL)
                     surface->displayMode = pCreateInfo->displayMode;
   surface->planeIndex = pCreateInfo->planeIndex;
   surface->planeStackIndex = pCreateInfo->planeStackIndex;
   surface->transform = pCreateInfo->transform;
   surface->globalAlpha = pCreateInfo->globalAlpha;
   surface->alphaMode = pCreateInfo->alphaMode;
                        }
      static VkResult
   wsi_display_surface_get_support(VkIcdSurfaceBase *surface,
                     {
      struct wsi_display *wsi =
            *pSupported = wsi->fd != -1;
      }
      static VkResult
   wsi_display_surface_get_capabilities(VkIcdSurfaceBase *surface_base,
               {
      VkIcdSurfaceDisplay *surface = (VkIcdSurfaceDisplay *) surface_base;
            caps->currentExtent.width = mode->hdisplay;
            caps->minImageExtent = (VkExtent2D) { 1, 1 };
   caps->maxImageExtent = (VkExtent2D) {
      wsi_device->maxImageDimension2D,
                        caps->minImageCount = 2;
            caps->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
   caps->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
   caps->maxImageArrayLayers = 1;
   caps->supportedUsageFlags =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
   VK_IMAGE_USAGE_SAMPLED_BIT |
   VK_IMAGE_USAGE_TRANSFER_DST_BIT |
   VK_IMAGE_USAGE_STORAGE_BIT |
   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
         VK_FROM_HANDLE(vk_physical_device, pdevice, wsi_device->pdevice);
   if (pdevice->supported_extensions.EXT_attachment_feedback_loop_layout)
               }
      static VkResult
   wsi_display_surface_get_surface_counters(VkSurfaceCounterFlagsEXT *counters)
   {
      *counters = VK_SURFACE_COUNTER_VBLANK_BIT_EXT;
      }
      static VkResult
   wsi_display_surface_get_capabilities2(VkIcdSurfaceBase *icd_surface,
                     {
      assert(caps->sType == VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR);
            result = wsi_display_surface_get_capabilities(icd_surface, wsi_device,
         if (result != VK_SUCCESS)
            struct wsi_surface_supported_counters *counters =
         const VkSurfacePresentModeEXT *present_mode =
            if (counters) {
                  vk_foreach_struct(ext, caps->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR: {
      VkSurfaceProtectedCapabilitiesKHR *protected = (void *)ext;
   protected->supportsProtected = VK_FALSE;
               case VK_STRUCTURE_TYPE_SURFACE_PRESENT_SCALING_CAPABILITIES_EXT: {
      /* Unsupported. */
   VkSurfacePresentScalingCapabilitiesEXT *scaling = (void *)ext;
   scaling->supportedPresentScaling = 0;
   scaling->supportedPresentGravityX = 0;
   scaling->supportedPresentGravityY = 0;
   scaling->minScaledImageExtent = caps->surfaceCapabilities.minImageExtent;
   scaling->maxScaledImageExtent = caps->surfaceCapabilities.maxImageExtent;
               case VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_COMPATIBILITY_EXT: {
      /* We only support FIFO. */
   VkSurfacePresentModeCompatibilityEXT *compat = (void *)ext;
   if (compat->pPresentModes) {
      if (compat->presentModeCount) {
      assert(present_mode);
   compat->pPresentModes[0] = present_mode->presentMode;
         } else {
         }
               default:
      /* Ignored */
                     }
      struct wsi_display_surface_format {
      VkSurfaceFormatKHR surface_format;
      };
      static const struct wsi_display_surface_format
   available_surface_formats[] = {
      {
      .surface_format = {
      .format = VK_FORMAT_B8G8R8A8_SRGB,
      },
      },
   {
      .surface_format = {
      .format = VK_FORMAT_B8G8R8A8_UNORM,
      },
         };
      static void
   get_sorted_vk_formats(struct wsi_device *wsi_device, VkSurfaceFormatKHR *sorted_formats)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(available_surface_formats); i++)
            if (wsi_device->force_bgra8_unorm_first) {
      for (unsigned i = 0; i < ARRAY_SIZE(available_surface_formats); i++) {
      if (sorted_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
      VkSurfaceFormatKHR tmp = sorted_formats[i];
   sorted_formats[i] = sorted_formats[0];
   sorted_formats[0] = tmp;
               }
      static VkResult
   wsi_display_surface_get_formats(VkIcdSurfaceBase *icd_surface,
                     {
      VK_OUTARRAY_MAKE_TYPED(VkSurfaceFormatKHR, out,
            VkSurfaceFormatKHR sorted_formats[ARRAY_SIZE(available_surface_formats)];
            for (unsigned i = 0; i < ARRAY_SIZE(sorted_formats); i++) {
      vk_outarray_append_typed(VkSurfaceFormatKHR, &out, f) {
                        }
      static VkResult
   wsi_display_surface_get_formats2(VkIcdSurfaceBase *surface,
                           {
      VK_OUTARRAY_MAKE_TYPED(VkSurfaceFormat2KHR, out,
            VkSurfaceFormatKHR sorted_formats[ARRAY_SIZE(available_surface_formats)];
            for (unsigned i = 0; i < ARRAY_SIZE(sorted_formats); i++) {
      vk_outarray_append_typed(VkSurfaceFormat2KHR, &out, f) {
      assert(f->sType == VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR);
                     }
      static VkResult
   wsi_display_surface_get_present_modes(VkIcdSurfaceBase *surface,
                     {
      VK_OUTARRAY_MAKE_TYPED(VkPresentModeKHR, conn,
            vk_outarray_append_typed(VkPresentModeKHR, &conn, present) {
                     }
      static VkResult
   wsi_display_surface_get_present_rectangles(VkIcdSurfaceBase *surface_base,
                     {
      VkIcdSurfaceDisplay *surface = (VkIcdSurfaceDisplay *) surface_base;
   wsi_display_mode *mode = wsi_display_mode_from_handle(surface->displayMode);
            if (wsi_device_matches_drm_fd(wsi_device, mode->connector->wsi->fd)) {
      vk_outarray_append_typed(VkRect2D, &out, rect) {
      *rect = (VkRect2D) {
      .offset = { 0, 0 },
                        }
      static void
   wsi_display_destroy_buffer(struct wsi_display *wsi,
         {
      (void) drmIoctl(wsi->fd, DRM_IOCTL_GEM_CLOSE,
      }
      static VkResult
   wsi_display_image_init(struct wsi_swapchain *drv_chain,
               {
      struct wsi_display_swapchain *chain =
         struct wsi_display *wsi = chain->wsi;
            for (unsigned i = 0; i < ARRAY_SIZE(available_surface_formats); i++) {
      if (create_info->imageFormat == available_surface_formats[i].surface_format.format &&
      create_info->imageColorSpace == available_surface_formats[i].surface_format.colorSpace) {
   drm_format = available_surface_formats[i].drm_format;
                  /* the application provided an invalid format, bail */
   if (drm_format == 0)
            VkResult result = wsi_create_image(&chain->base, &chain->base.image_info,
         if (result != VK_SUCCESS)
                     for (unsigned int i = 0; i < image->base.num_planes; i++) {
      int ret = drmPrimeFDToHandle(wsi->fd, image->base.dma_buf_fd,
         if (ret < 0)
               image->chain = chain;
   image->state = WSI_IMAGE_IDLE;
            int ret = drmModeAddFB2(wsi->fd,
                           create_info->imageExtent.width,
   create_info->imageExtent.height,
            if (ret)
                  fail_fb:
   fail_handle:
      for (unsigned int i = 0; i < image->base.num_planes; i++) {
      if (image->buffer[i])
                           }
      static void
   wsi_display_image_finish(struct wsi_swapchain *drv_chain,
         {
      struct wsi_display_swapchain *chain =
                  drmModeRmFB(wsi->fd, image->fb_id);
   for (unsigned int i = 0; i < image->base.num_planes; i++)
            }
      static VkResult
   wsi_display_swapchain_destroy(struct wsi_swapchain *drv_chain,
         {
      struct wsi_display_swapchain *chain =
            for (uint32_t i = 0; i < chain->base.image_count; i++)
            pthread_mutex_destroy(&chain->present_id_mutex);
            wsi_swapchain_finish(&chain->base);
   vk_free(allocator, chain);
      }
      static struct wsi_image *
   wsi_display_get_wsi_image(struct wsi_swapchain *drv_chain,
         {
      struct wsi_display_swapchain *chain =
               }
      static void
   wsi_display_idle_old_displaying(struct wsi_display_image *active_image)
   {
               wsi_display_debug("idle everyone but %ld\n",
         for (uint32_t i = 0; i < chain->base.image_count; i++)
      if (chain->images[i].state == WSI_IMAGE_DISPLAYING &&
         {
      wsi_display_debug("idle %d\n", i);
      }
      static VkResult
   _wsi_display_queue_next(struct wsi_swapchain *drv_chain);
      static void
   wsi_display_present_complete(struct wsi_display_swapchain *swapchain,
         {
      if (image->present_id) {
      pthread_mutex_lock(&swapchain->present_id_mutex);
   if (image->present_id > swapchain->present_id) {
      swapchain->present_id = image->present_id;
      }
         }
      static void
   wsi_display_surface_error(struct wsi_display_swapchain *swapchain, VkResult result)
   {
      pthread_mutex_lock(&swapchain->present_id_mutex);
   swapchain->present_id = UINT64_MAX;
   swapchain->present_id_error = result;
   pthread_cond_broadcast(&swapchain->present_id_cond);
      }
      static void
   wsi_display_page_flip_handler2(int fd,
                                 {
      struct wsi_display_image *image = data;
            wsi_display_debug("image %ld displayed at %d\n",
         image->state = WSI_IMAGE_DISPLAYING;
            wsi_display_idle_old_displaying(image);
   VkResult result = _wsi_display_queue_next(&(chain->base));
   if (result != VK_SUCCESS)
      }
      static void wsi_display_fence_event_handler(struct wsi_display_fence *fence);
      static void wsi_display_page_flip_handler(int fd,
                           {
         }
      static void wsi_display_vblank_handler(int fd, unsigned int frame,
               {
                  }
      static void wsi_display_sequence_handler(int fd, uint64_t frame,
         {
      struct wsi_display_fence *fence =
               }
      static drmEventContext event_context = {
      .version = DRM_EVENT_CONTEXT_VERSION,
      #if DRM_EVENT_CONTEXT_VERSION >= 3
         #endif
      .vblank_handler = wsi_display_vblank_handler,
      };
      static void *
   wsi_display_wait_thread(void *data)
   {
      struct wsi_display *wsi = data;
   struct pollfd pollfd = {
      .fd = wsi->fd,
               pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
   for (;;) {
      int ret = poll(&pollfd, 1, -1);
   if (ret > 0) {
      pthread_mutex_lock(&wsi->wait_mutex);
   (void) drmHandleEvent(wsi->fd, &event_context);
   pthread_cond_broadcast(&wsi->wait_cond);
         }
      }
      static int
   wsi_display_start_wait_thread(struct wsi_display *wsi)
   {
      if (!wsi->wait_thread) {
      int ret = pthread_create(&wsi->wait_thread, NULL,
         if (ret)
      }
      }
      static void
   wsi_display_stop_wait_thread(struct wsi_display *wsi)
   {
      pthread_mutex_lock(&wsi->wait_mutex);
   if (wsi->wait_thread) {
      pthread_cancel(wsi->wait_thread);
   pthread_join(wsi->wait_thread, NULL);
      }
      }
      static int
   cond_timedwait_ns(pthread_cond_t *cond,
               {
      struct timespec abs_timeout = {
      .tv_sec = timeout_ns / 1000000000ULL,
               int ret = pthread_cond_timedwait(cond, mutex, &abs_timeout);
   wsi_display_debug("%9ld done waiting for event %d\n", pthread_self(), ret);
      }
      /*
   * Wait for at least one event from the kernel to be processed.
   * Call with wait_mutex held
   */
   static int
   wsi_display_wait_for_event(struct wsi_display *wsi,
         {
               if (ret)
               }
      /* Wait for device event to be processed.
   * Call with wait_mutex held
   */
   static int
   wsi_device_wait_for_event(struct wsi_display *wsi,
         {
         }
      static VkResult
   wsi_display_release_images(struct wsi_swapchain *drv_chain,
         {
      struct wsi_display_swapchain *chain = (struct wsi_display_swapchain *)drv_chain;
   if (chain->status == VK_ERROR_SURFACE_LOST_KHR)
            for (uint32_t i = 0; i < count; i++) {
      uint32_t index = indices[i];
   assert(index < chain->base.image_count);
   assert(chain->images[index].state == WSI_IMAGE_DRAWING);
                  }
      static VkResult
   wsi_display_acquire_next_image(struct wsi_swapchain *drv_chain,
               {
      struct wsi_display_swapchain *chain =
         struct wsi_display *wsi = chain->wsi;
   int ret = 0;
            /* Bail early if the swapchain is broken */
   if (chain->status != VK_SUCCESS)
            uint64_t timeout = info->timeout;
   if (timeout != 0 && timeout != UINT64_MAX)
            pthread_mutex_lock(&wsi->wait_mutex);
   for (;;) {
      for (uint32_t i = 0; i < chain->base.image_count; i++) {
      if (chain->images[i].state == WSI_IMAGE_IDLE) {
      *image_index = i;
   wsi_display_debug("image %d available\n", i);
   chain->images[i].state = WSI_IMAGE_DRAWING;
   result = VK_SUCCESS;
      }
               if (ret == ETIMEDOUT) {
      result = VK_TIMEOUT;
                        if (ret && ret != ETIMEDOUT) {
      result = VK_ERROR_SURFACE_LOST_KHR;
   wsi_display_surface_error(chain, result);
            done:
               if (result != VK_SUCCESS)
               }
      /*
   * Check whether there are any other connectors driven by this crtc
   */
   static bool
   wsi_display_crtc_solo(struct wsi_display *wsi,
                     {
      /* See if any other connectors share the same encoder */
   for (int c = 0; c < mode_res->count_connectors; c++) {
      if (mode_res->connectors[c] == connector->connector_id)
            drmModeConnectorPtr other_connector =
            if (other_connector) {
      bool match = (other_connector->encoder_id == connector->encoder_id);
   drmModeFreeConnector(other_connector);
   if (match)
                  /* See if any other encoders share the same crtc */
   for (int e = 0; e < mode_res->count_encoders; e++) {
      if (mode_res->encoders[e] == connector->encoder_id)
            drmModeEncoderPtr other_encoder =
            if (other_encoder) {
      bool match = (other_encoder->crtc_id == crtc_id);
   drmModeFreeEncoder(other_encoder);
   if (match)
         }
      }
      /*
   * Pick a suitable CRTC to drive this connector. Prefer a CRTC which is
   * currently driving this connector and not any others. Settle for a CRTC
   * which is currently idle.
   */
   static uint32_t
   wsi_display_select_crtc(const struct wsi_display_connector *connector,
               {
               /* See what CRTC is currently driving this connector */
   if (drm_connector->encoder_id) {
      drmModeEncoderPtr encoder =
            if (encoder) {
      uint32_t crtc_id = encoder->crtc_id;
   drmModeFreeEncoder(encoder);
   if (crtc_id) {
      if (wsi_display_crtc_solo(wsi, mode_res, drm_connector, crtc_id))
            }
   uint32_t crtc_id = 0;
   for (int c = 0; crtc_id == 0 && c < mode_res->count_crtcs; c++) {
      drmModeCrtcPtr crtc = drmModeGetCrtc(wsi->fd, mode_res->crtcs[c]);
   if (crtc && crtc->buffer_id == 0)
            }
      }
      static VkResult
   wsi_display_setup_connector(wsi_display_connector *connector,
         {
               if (connector->current_mode == display_mode && connector->crtc_id)
                     drmModeResPtr mode_res = drmModeGetResources(wsi->fd);
   if (!mode_res) {
      if (errno == ENOMEM)
         else
                     drmModeConnectorPtr drm_connector =
            if (!drm_connector) {
      if (errno == ENOMEM)
         else
                     /* Pick a CRTC if we don't have one */
   if (!connector->crtc_id) {
      connector->crtc_id = wsi_display_select_crtc(connector,
         if (!connector->crtc_id) {
      result = VK_ERROR_SURFACE_LOST_KHR;
                              /* Find the drm mode corresponding to the requested VkDisplayMode */
            for (int m = 0; m < drm_connector->count_modes; m++) {
      drm_mode = &drm_connector->modes[m];
   if (wsi_display_mode_matches_drm(display_mode, drm_mode))
                     if (!drm_mode) {
      result = VK_ERROR_SURFACE_LOST_KHR;
               connector->current_mode = display_mode;
            bail_connector:
         bail_mode_res:
         bail:
            }
      static VkResult
   wsi_display_fence_wait(struct wsi_display_fence *fence, uint64_t timeout)
   {
      wsi_display_debug("%9lu wait fence %lu %ld\n",
               wsi_display_debug_code(uint64_t start_ns = os_time_get_nano());
            VkResult result;
   int ret = 0;
   for (;;) {
      if (fence->event_received) {
      wsi_display_debug("%9lu fence %lu passed\n",
         result = VK_SUCCESS;
               if (ret == ETIMEDOUT) {
      wsi_display_debug("%9lu fence %lu timeout\n",
         result = VK_TIMEOUT;
               if (fence->device_event)
         else
            if (ret && ret != ETIMEDOUT) {
      wsi_display_debug("%9lu fence %lu error\n",
         result = VK_ERROR_DEVICE_LOST;
         }
   pthread_mutex_unlock(&fence->wsi->wait_mutex);
   wsi_display_debug("%9lu fence wait %f ms\n",
                        }
      static void
   wsi_display_fence_check_free(struct wsi_display_fence *fence)
   {
      if (fence->event_received && fence->destroyed)
      }
      static void wsi_display_fence_event_handler(struct wsi_display_fence *fence)
   {
      if (fence->syncobj) {
      (void) drmSyncobjSignal(fence->wsi->syncobj_fd, &fence->syncobj, 1);
               fence->event_received = true;
      }
      static void
   wsi_display_fence_destroy(struct wsi_display_fence *fence)
   {
      /* Destroy hotplug fence list. */
   if (fence->device_event) {
      pthread_mutex_lock(&fence->wsi->wait_mutex);
   list_del(&fence->link);
   pthread_mutex_unlock(&fence->wsi->wait_mutex);
               assert(!fence->destroyed);
   fence->destroyed = true;
      }
      static struct wsi_display_fence *
   wsi_display_fence_alloc(struct wsi_display *wsi, int sync_fd)
   {
      struct wsi_display_fence *fence =
      vk_zalloc(wsi->alloc, sizeof (*fence),
         if (!fence)
            if (sync_fd >= 0) {
               if (ret) {
      vk_free(wsi->alloc, fence);
                  fence->wsi = wsi;
   fence->event_received = false;
   fence->destroyed = false;
   fence->sequence = ++fence_sequence;
      }
      static VkResult
   wsi_display_sync_init(struct vk_device *device,
               {
      assert(initial_value == 0);
      }
      static void
   wsi_display_sync_finish(struct vk_device *device,
         {
      struct wsi_display_sync *wsi_sync =
         if (wsi_sync->fence)
      }
      static VkResult
   wsi_display_sync_wait(struct vk_device *device,
                           {
      struct wsi_display_sync *wsi_sync =
            assert(wait_value == 0);
               }
      static const struct vk_sync_type wsi_display_sync_type = {
      .size = sizeof(struct wsi_display_sync),
   .features = VK_SYNC_FEATURE_BINARY |
         .init = wsi_display_sync_init,
   .finish = wsi_display_sync_finish,
      };
      static VkResult
   wsi_display_sync_create(struct vk_device *device,
               {
      VkResult result = vk_sync_create(device, &wsi_display_sync_type,
               if (result != VK_SUCCESS)
            struct wsi_display_sync *sync =
                        }
      static VkResult
   wsi_register_vblank_event(struct wsi_display_fence *fence,
                           const struct wsi_device *wsi_device,
   {
      struct wsi_display *wsi =
         struct wsi_display_connector *connector =
            if (wsi->fd < 0)
            /* A display event may be registered before the first page flip at which
   * point crtc_id will be 0. If this is the case we setup the connector
   * here to allow drmCrtcQueueSequence to succeed.
   */
   if (!connector->crtc_id) {
      VkResult ret = wsi_display_setup_connector(connector,
         if (ret != VK_SUCCESS)
               for (;;) {
      int ret = drmCrtcQueueSequence(wsi->fd, connector->crtc_id,
                              if (!ret)
                        /* Something unexpected happened. Pause for a moment so the
                  wsi_display_debug("queue vblank event %lu failed\n", fence->sequence);
   struct timespec delay = {
      .tv_sec = 0,
      };
   nanosleep(&delay, NULL);
               /* The kernel event queue is full. Wait for some events to be
   * processed and try again
            pthread_mutex_lock(&wsi->wait_mutex);
   ret = wsi_display_wait_for_event(wsi, wsi_rel_to_abs_time(100000000ull));
            if (ret) {
      wsi_display_debug("vblank queue full, event wait failed\n");
            }
      /*
   * Check to see if the kernel has no flip queued and if there's an image
   * waiting to be displayed.
   */
   static VkResult
   _wsi_display_queue_next(struct wsi_swapchain *drv_chain)
   {
      struct wsi_display_swapchain *chain =
         struct wsi_display *wsi = chain->wsi;
   VkIcdSurfaceDisplay *surface = chain->surface;
   wsi_display_mode *display_mode =
                  if (wsi->fd < 0) {
      wsi_display_surface_error(chain, VK_ERROR_SURFACE_LOST_KHR);
               if (display_mode != connector->current_mode)
                        /* Check to see if there is an image to display, or if some image is
                     for (uint32_t i = 0; i < chain->base.image_count; i++) {
               switch (tmp_image->state) {
   case WSI_IMAGE_FLIPPING:
      /* already flipping, don't send another to the kernel yet */
      case WSI_IMAGE_QUEUED:
      /* find the oldest queued */
   if (!image || tmp_image->flip_sequence < image->flip_sequence)
            default:
                     if (!image)
            int ret;
   if (connector->active) {
      ret = drmModePageFlip(wsi->fd, connector->crtc_id, image->fb_id,
         if (ret == 0) {
      image->state = WSI_IMAGE_FLIPPING;
      }
      } else {
                  if (ret == -EINVAL) {
               if (result != VK_SUCCESS) {
      image->state = WSI_IMAGE_IDLE;
               /* XXX allow setting of position */
   ret = drmModeSetCrtc(wsi->fd, connector->crtc_id,
                     if (ret == 0) {
      /* Disable the HW cursor as the app doesn't have a mechanism
   * to control it.
   * Refer to question 12 of the VK_KHR_display spec.
   */
   ret = drmModeSetCursor(wsi->fd, connector->crtc_id, 0, 0, 0 );
   if (ret != 0) {
                  /* Assume that the mode set is synchronous and that any
   * previous image is now idle.
   */
   image->state = WSI_IMAGE_DISPLAYING;
   wsi_display_present_complete(chain, image);
   wsi_display_idle_old_displaying(image);
   connector->active = true;
                  if (ret != -EACCES) {
      connector->active = false;
   image->state = WSI_IMAGE_IDLE;
   wsi_display_surface_error(chain, VK_ERROR_SURFACE_LOST_KHR);
               /* Some other VT is currently active. Sit here waiting for
   * our VT to become active again by polling once a second
   */
   usleep(1000 * 1000);
         }
      static VkResult
   wsi_display_queue_present(struct wsi_swapchain *drv_chain,
                     {
      struct wsi_display_swapchain *chain =
         struct wsi_display *wsi = chain->wsi;
   struct wsi_display_image *image = &chain->images[image_index];
            /* Bail early if the swapchain is broken */
   if (chain->status != VK_SUCCESS)
                     assert(image->state == WSI_IMAGE_DRAWING);
                     /* Make sure that the page flip handler is processed in finite time if using present wait. */
   if (present_id)
            image->flip_sequence = ++chain->flip_sequence;
            result = _wsi_display_queue_next(drv_chain);
   if (result != VK_SUCCESS)
                     if (result != VK_SUCCESS)
               }
      static VkResult
   wsi_display_wait_for_present(struct wsi_swapchain *wsi_chain,
               {
      struct wsi_display_swapchain *chain = (struct wsi_display_swapchain *)wsi_chain;
   struct timespec abs_timespec;
            if (timeout != 0)
            /* Need to observe that the swapchain semaphore has been unsignalled,
   * as this is guaranteed when a present is complete. */
   VkResult result = wsi_swapchain_wait_for_present_semaphore(
         if (result != VK_SUCCESS)
                     pthread_mutex_lock(&chain->present_id_mutex);
   while (chain->present_id < waitValue) {
      int ret = pthread_cond_timedwait(&chain->present_id_cond,
               if (ret == ETIMEDOUT) {
      result = VK_TIMEOUT;
      }
   if (ret) {
      result = VK_ERROR_DEVICE_LOST;
                  if (result == VK_SUCCESS && chain->present_id_error)
         pthread_mutex_unlock(&chain->present_id_mutex);
      }
      static VkResult
   wsi_display_surface_create_swapchain(
      VkIcdSurfaceBase *icd_surface,
   VkDevice device,
   struct wsi_device *wsi_device,
   const VkSwapchainCreateInfoKHR *create_info,
   const VkAllocationCallbacks *allocator,
      {
      struct wsi_display *wsi =
                     const unsigned num_images = create_info->minImageCount;
   struct wsi_display_swapchain *chain =
      vk_zalloc(allocator,
               if (chain == NULL)
            struct wsi_drm_image_params image_params = {
      .base.image_type = WSI_IMAGE_TYPE_DRM,
               int ret = pthread_mutex_init(&chain->present_id_mutex, NULL);
   if (ret != 0) {
      vk_free(allocator, chain);
               bool bret = wsi_init_pthread_cond_monotonic(&chain->present_id_cond);
   if (!bret) {
      pthread_mutex_destroy(&chain->present_id_mutex);
   vk_free(allocator, chain);
               VkResult result = wsi_swapchain_init(wsi_device, &chain->base, device,
               if (result != VK_SUCCESS) {
      pthread_cond_destroy(&chain->present_id_cond);
   pthread_mutex_destroy(&chain->present_id_mutex);
   vk_free(allocator, chain);
               chain->base.destroy = wsi_display_swapchain_destroy;
   chain->base.get_wsi_image = wsi_display_get_wsi_image;
   chain->base.acquire_next_image = wsi_display_acquire_next_image;
   chain->base.release_images = wsi_display_release_images;
   chain->base.queue_present = wsi_display_queue_present;
   chain->base.wait_for_present = wsi_display_wait_for_present;
   chain->base.present_mode = wsi_swapchain_get_present_mode(wsi_device, create_info);
            chain->wsi = wsi;
                     for (uint32_t image = 0; image < chain->base.image_count; image++) {
      result = wsi_display_image_init(&chain->base,
               if (result != VK_SUCCESS) {
      while (image > 0) {
      --image;
   wsi_display_image_finish(&chain->base,
      }
   pthread_cond_destroy(&chain->present_id_cond);
   pthread_mutex_destroy(&chain->present_id_mutex);
   wsi_swapchain_finish(&chain->base);
   vk_free(allocator, chain);
                                 fail_init_images:
         }
      /*
   * Local version fo the libdrm helper. Added to avoid depending on bleeding
   * edge version of the library.
   */
   static int
   local_drmIsMaster(int fd)
   {
      /* Detect master by attempting something that requires master.
   *
   * Authenticating magic tokens requires master and 0 is an
   * internal kernel detail which we could use. Attempting this on
   * a master fd would fail therefore fail with EINVAL because 0
   * is invalid.
   *
   * A non-master fd will fail with EACCES, as the kernel checks
   * for master before attempting to do anything else.
   *
   * Since we don't want to leak implementation details, use
   * EACCES.
   */
      }
      #ifdef HAVE_LIBUDEV
   static void *
   udev_event_listener_thread(void *data)
   {
      struct wsi_device *wsi_device = data;
   struct wsi_display *wsi =
            struct udev *u = udev_new();
   if (!u)
            struct udev_monitor *mon =
         if (!mon)
            int ret =
         if (ret < 0)
            ret = udev_monitor_enable_receiving(mon);
   if (ret < 0)
                              for (;;) {
      nfds_t nfds = 1;
   struct pollfd fds[1] =  {
      {
      .fd = udev_fd,
                  int ret = poll(fds, nfds, -1);
   if (ret > 0) {
                        /* Ignore event if it is not a hotplug event */
                  /* Note, this supports both drmSyncobjWait for fence->syncobj
   * and wsi_display_wait_for_event.
   */
   pthread_mutex_lock(&wsi->wait_mutex);
   pthread_cond_broadcast(&wsi->hotplug_cond);
   list_for_each_entry(struct wsi_display_fence, fence,
            if (fence->syncobj)
            }
   pthread_mutex_unlock(&wsi->wait_mutex);
         } else if (ret < 0) {
                     udev_monitor_unref(mon);
                  fail_udev_monitor:
         fail_udev:
         fail:
      wsi_display_debug("critical hotplug thread error\n");
      }
   #endif
      VkResult
   wsi_display_init_wsi(struct wsi_device *wsi_device,
               {
      struct wsi_display *wsi = vk_zalloc(alloc, sizeof(*wsi), 8,
                  if (!wsi) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               wsi->fd = display_fd;
   if (wsi->fd != -1 && !local_drmIsMaster(wsi->fd))
                                       int ret = pthread_mutex_init(&wsi->wait_mutex, NULL);
   if (ret) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               if (!wsi_init_pthread_cond_monotonic(&wsi->wait_cond)) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               if (!wsi_init_pthread_cond_monotonic(&wsi->hotplug_cond)) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               wsi->base.get_support = wsi_display_surface_get_support;
   wsi->base.get_capabilities2 = wsi_display_surface_get_capabilities2;
   wsi->base.get_formats = wsi_display_surface_get_formats;
   wsi->base.get_formats2 = wsi_display_surface_get_formats2;
   wsi->base.get_present_modes = wsi_display_surface_get_present_modes;
   wsi->base.get_present_rectangles = wsi_display_surface_get_present_rectangles;
                           fail_hotplug_cond:
         fail_cond:
         fail_mutex:
         fail:
         }
      void
   wsi_display_finish_wsi(struct wsi_device *wsi_device,
         {
      struct wsi_display *wsi =
            if (wsi) {
      wsi_for_each_connector(connector, wsi) {
      wsi_for_each_display_mode(mode, connector) {
         }
                        if (wsi->hotplug_thread) {
      pthread_cancel(wsi->hotplug_thread);
               pthread_mutex_destroy(&wsi->wait_mutex);
   pthread_cond_destroy(&wsi->wait_cond);
                  }
      /*
   * Implement vkReleaseDisplay
   */
   VKAPI_ATTR VkResult VKAPI_CALL
   wsi_ReleaseDisplayEXT(VkPhysicalDevice physicalDevice,
         {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
   struct wsi_device *wsi_device = pdevice->wsi_device;
   struct wsi_display *wsi =
            if (wsi->fd >= 0) {
               close(wsi->fd);
                     #ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
         #endif
            }
      #ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
      static struct wsi_display_connector *
   wsi_display_find_output(struct wsi_device *wsi_device,
         {
      struct wsi_display *wsi =
            wsi_for_each_connector(connector, wsi) {
      if (connector->output == output)
                  }
      /*
   * Given a RandR output, find the associated kernel connector_id by
   * looking at the CONNECTOR_ID property provided by the X server
   */
      static uint32_t
   wsi_display_output_to_connector_id(xcb_connection_t *connection,
               {
      uint32_t connector_id = 0;
            if (connector_id_atom == 0) {
   /* Go dig out the CONNECTOR_ID property */
      xcb_intern_atom_cookie_t ia_c = xcb_intern_atom(connection,
                     xcb_intern_atom_reply_t *ia_r = xcb_intern_atom_reply(connection,
               if (ia_r) {
      *connector_id_atom_p = connector_id_atom = ia_r->atom;
                  /* If there's an CONNECTOR_ID atom in the server, then there may be a
   * CONNECTOR_ID property. Otherwise, there will not be and we don't even
   * need to bother.
   */
               xcb_randr_query_version_cookie_t qv_c =
         xcb_randr_get_output_property_cookie_t gop_c =
      xcb_randr_get_output_property(connection,
                                 output,
      xcb_randr_query_version_reply_t *qv_r =
         free(qv_r);
   xcb_randr_get_output_property_reply_t *gop_r =
         if (gop_r) {
      if (gop_r->num_items == 1 && gop_r->format == 32)
               }
      }
      static bool
   wsi_display_check_randr_version(xcb_connection_t *connection)
   {
      xcb_randr_query_version_cookie_t qv_c =
         xcb_randr_query_version_reply_t *qv_r =
                  if (!qv_r)
            /* Check for version 1.6 or newer */
   ret = (qv_r->major_version > 1 ||
            free(qv_r);
      }
      /*
   * Given a kernel connector id, find the associated RandR output using the
   * CONNECTOR_ID property
   */
      static xcb_randr_output_t
   wsi_display_connector_id_to_output(xcb_connection_t *connection,
         {
      if (!wsi_display_check_randr_version(connection))
                     xcb_atom_t connector_id_atom = 0;
            /* Search all of the screens for the provided output */
   xcb_screen_iterator_t iter;
   for (iter = xcb_setup_roots_iterator(setup);
      output == 0 && iter.rem;
      {
      xcb_randr_get_screen_resources_cookie_t gsr_c =
         xcb_randr_get_screen_resources_reply_t *gsr_r =
            if (!gsr_r)
            xcb_randr_output_t *ro = xcb_randr_get_screen_resources_outputs(gsr_r);
            for (o = 0; o < gsr_r->num_outputs; o++) {
      if (wsi_display_output_to_connector_id(connection,
               {
      output = ro[o];
         }
      }
      }
      /*
   * Given a RandR output, find out which screen it's associated with
   */
   static xcb_window_t
   wsi_display_output_to_root(xcb_connection_t *connection,
         {
      if (!wsi_display_check_randr_version(connection))
            const xcb_setup_t *setup = xcb_get_setup(connection);
            /* Search all of the screens for the provided output */
   for (xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
      root == 0 && iter.rem;
      {
      xcb_randr_get_screen_resources_cookie_t gsr_c =
         xcb_randr_get_screen_resources_reply_t *gsr_r =
            if (!gsr_r)
                     for (int o = 0; o < gsr_r->num_outputs; o++) {
      if (ro[o] == output) {
      root = iter.data->root;
         }
      }
      }
      static bool
   wsi_display_mode_matches_x(struct wsi_display_mode *wsi,
         {
      return wsi->clock == (xcb->dot_clock + 500) / 1000 &&
      wsi->hdisplay == xcb->width &&
   wsi->hsync_start == xcb->hsync_start &&
   wsi->hsync_end == xcb->hsync_end &&
   wsi->htotal == xcb->htotal &&
   wsi->hskew == xcb->hskew &&
   wsi->vdisplay == xcb->height &&
   wsi->vsync_start == xcb->vsync_start &&
   wsi->vsync_end == xcb->vsync_end &&
   wsi->vtotal == xcb->vtotal &&
   wsi->vscan <= 1 &&
   }
      static struct wsi_display_mode *
   wsi_display_find_x_mode(struct wsi_device *wsi_device,
               {
      wsi_for_each_display_mode(display_mode, connector) {
      if (wsi_display_mode_matches_x(display_mode, mode))
      }
      }
      static VkResult
   wsi_display_register_x_mode(struct wsi_device *wsi_device,
                     {
      struct wsi_display *wsi =
         struct wsi_display_mode *display_mode =
            if (display_mode) {
      display_mode->valid = true;
               display_mode = vk_zalloc(wsi->alloc, sizeof (struct wsi_display_mode),
         if (!display_mode)
            display_mode->connector = connector;
   display_mode->valid = true;
   display_mode->preferred = preferred;
   display_mode->clock = (x_mode->dot_clock + 500) / 1000; /* kHz */
   display_mode->hdisplay = x_mode->width;
   display_mode->hsync_start = x_mode->hsync_start;
   display_mode->hsync_end = x_mode->hsync_end;
   display_mode->htotal = x_mode->htotal;
   display_mode->hskew = x_mode->hskew;
   display_mode->vdisplay = x_mode->height;
   display_mode->vsync_start = x_mode->vsync_start;
   display_mode->vsync_end = x_mode->vsync_end;
   display_mode->vtotal = x_mode->vtotal;
   display_mode->vscan = 0;
            list_addtail(&display_mode->list, &connector->display_modes);
      }
      static struct wsi_display_connector *
   wsi_display_get_output(struct wsi_device *wsi_device,
               {
      struct wsi_display *wsi =
         struct wsi_display_connector *connector;
            xcb_window_t root = wsi_display_output_to_root(connection, output);
   if (!root)
            /* See if we already have a connector for this output */
            if (!connector) {
               /*
   * Go get the kernel connector ID for this X output
   */
   connector_id = wsi_display_output_to_connector_id(connection,
                  /* Any X server with lease support will have this atom */
   if (!connector_id) {
                  /* See if we already have a connector for this id */
            if (connector == NULL) {
      connector = wsi_display_alloc_connector(wsi, connector_id);
   if (!connector) {
         }
      }
               xcb_randr_get_screen_resources_cookie_t src =
         xcb_randr_get_output_info_cookie_t oic =
         xcb_randr_get_screen_resources_reply_t *srr =
         xcb_randr_get_output_info_reply_t *oir =
            if (oir && srr) {
               connector->connected =
                     xcb_randr_mode_t *x_modes = xcb_randr_get_output_info_modes(oir);
   for (int m = 0; m < oir->num_modes; m++) {
      xcb_randr_mode_info_iterator_t i =
         while (i.rem) {
      xcb_randr_mode_info_t *mi = i.data;
   if (mi->id == x_modes[m]) {
      VkResult result = wsi_display_register_x_mode(
         if (result != VK_SUCCESS) {
      free(oir);
   free(srr);
      }
      }
                     free(oir);
   free(srr);
      }
      static xcb_randr_crtc_t
   wsi_display_find_crtc_for_output(xcb_connection_t *connection,
               {
      xcb_randr_get_screen_resources_cookie_t gsr_c =
         xcb_randr_get_screen_resources_reply_t *gsr_r =
            if (!gsr_r)
            xcb_randr_crtc_t *rc = xcb_randr_get_screen_resources_crtcs(gsr_r);
   xcb_randr_crtc_t idle_crtc = 0;
            /* Find either a crtc already connected to the desired output or idle */
   for (int c = 0; active_crtc == 0 && c < gsr_r->num_crtcs; c++) {
      xcb_randr_get_crtc_info_cookie_t gci_c =
         xcb_randr_get_crtc_info_reply_t *gci_r =
            if (gci_r) {
      if (gci_r->mode) {
      int num_outputs = xcb_randr_get_crtc_info_outputs_length(gci_r);
                              } else if (idle_crtc == 0) {
      int num_possible = xcb_randr_get_crtc_info_possible_length(gci_r);
                  for (int p = 0; p < num_possible; p++)
      if (possible[p] == output) {
      idle_crtc = rc[c];
      }
         }
            if (active_crtc)
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_AcquireXlibDisplayEXT(VkPhysicalDevice physicalDevice,
               {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
   struct wsi_device *wsi_device = pdevice->wsi_device;
   struct wsi_display *wsi =
         xcb_connection_t *connection = XGetXCBConnection(dpy);
   struct wsi_display_connector *connector =
                  /* XXX no support for multiple leases yet */
   if (wsi->fd >= 0)
            if (!connector->output) {
      connector->output = wsi_display_connector_id_to_output(connection,
            /* Check and see if we found the output */
   if (!connector->output)
               root = wsi_display_output_to_root(connection, connector->output);
   if (!root)
            xcb_randr_crtc_t crtc = wsi_display_find_crtc_for_output(connection,
                  if (!crtc)
         #ifdef HAVE_DRI3_MODIFIERS
      xcb_randr_lease_t lease = xcb_generate_id(connection);
   xcb_randr_create_lease_cookie_t cl_c =
      xcb_randr_create_lease(connection, root, lease, 1, 1,
      xcb_randr_create_lease_reply_t *cl_r =
         if (!cl_r)
            int fd = -1;
   if (cl_r->nfd > 0) {
                  }
   free (cl_r);
   if (fd < 0)
               #endif
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetRandROutputDisplayEXT(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
   struct wsi_device *wsi_device = pdevice->wsi_device;
   xcb_connection_t *connection = XGetXCBConnection(dpy);
   struct wsi_display_connector *connector =
      wsi_display_get_output(wsi_device, connection,
         if (connector)
         else
            }
      #endif
      /* VK_EXT_display_control */
   VKAPI_ATTR VkResult VKAPI_CALL
   wsi_DisplayPowerControlEXT(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
   struct wsi_device *wsi_device = device->physical->wsi_device;
   struct wsi_display *wsi =
         struct wsi_display_connector *connector =
                  if (wsi->fd < 0)
            switch (pDisplayPowerInfo->powerState) {
   case VK_DISPLAY_POWER_STATE_OFF_EXT:
      mode = DRM_MODE_DPMS_OFF;
      case VK_DISPLAY_POWER_STATE_SUSPEND_EXT:
      mode = DRM_MODE_DPMS_SUSPEND;
      default:
      mode = DRM_MODE_DPMS_ON;
      }
   drmModeConnectorSetProperty(wsi->fd,
                        }
      VkResult
   wsi_register_device_event(VkDevice _device,
                           struct wsi_device *wsi_device,
   {
      VK_FROM_HANDLE(vk_device, device, _device);
   struct wsi_display *wsi =
               #ifdef HAVE_LIBUDEV
      /* Start listening for output change notifications. */
   pthread_mutex_lock(&wsi->wait_mutex);
   if (!wsi->hotplug_thread) {
      if (pthread_create(&wsi->hotplug_thread, NULL, udev_event_listener_thread,
            pthread_mutex_unlock(&wsi->wait_mutex);
         }
      #endif
         struct wsi_display_fence *fence;
   assert(device_event_info->deviceEvent ==
                     if (!fence)
                     pthread_mutex_lock(&wsi->wait_mutex);
   list_addtail(&fence->link, &wsi_device->hotplug_fences);
            if (sync_out) {
      ret = wsi_display_sync_create(device, fence, sync_out);
   if (ret != VK_SUCCESS)
      } else {
                     }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_RegisterDeviceEventEXT(VkDevice _device, const VkDeviceEventInfoEXT *device_event_info,
         {
      VK_FROM_HANDLE(vk_device, device, _device);
   struct vk_fence *fence;
            const VkFenceCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      };
   ret = vk_fence_create(device, &info, allocator, &fence);
   if (ret != VK_SUCCESS)
            ret = wsi_register_device_event(_device,
                                 if (ret == VK_SUCCESS)
         else
            }
      VkResult
   wsi_register_display_event(VkDevice _device,
                              struct wsi_device *wsi_device,
      {
      VK_FROM_HANDLE(vk_device, device, _device);
   struct wsi_display *wsi =
         struct wsi_display_fence *fence;
            switch (display_event_info->displayEvent) {
                        if (!fence)
            ret = wsi_register_vblank_event(fence, wsi_device, display,
            if (ret == VK_SUCCESS) {
      if (sync_out) {
      ret = wsi_display_sync_create(device, fence, sync_out);
   if (ret != VK_SUCCESS)
      } else {
            } else if (fence != NULL) {
      if (fence->syncobj)
                        default:
      ret = VK_ERROR_FEATURE_NOT_PRESENT;
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_RegisterDisplayEventEXT(VkDevice _device, VkDisplayKHR display,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
   struct vk_fence *fence;
            const VkFenceCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      };
   ret = vk_fence_create(device, &info, allocator, &fence);
   if (ret != VK_SUCCESS)
            ret = wsi_register_display_event(
      _device, device->physical->wsi_device,
         if (ret == VK_SUCCESS)
         else
            }
      void
   wsi_display_setup_syncobj_fd(struct wsi_device *wsi_device,
         {
      struct wsi_display *wsi =
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetSwapchainCounterEXT(VkDevice _device,
                     {
      VK_FROM_HANDLE(vk_device, device, _device);
   struct wsi_device *wsi_device = device->physical->wsi_device;
   struct wsi_display *wsi =
         struct wsi_display_swapchain *swapchain =
         struct wsi_display_connector *connector =
            if (wsi->fd < 0)
            if (!connector->active) {
      *pCounterValue = 0;
               int ret = drmCrtcGetSequence(wsi->fd, connector->crtc_id,
         if (ret)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_AcquireDrmDisplayEXT(VkPhysicalDevice physicalDevice,
               {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
            if (!wsi_device_matches_drm_fd(wsi_device, drmFd))
            struct wsi_display *wsi =
            /* XXX no support for mulitple leases yet */
   if (wsi->fd >= 0 || !local_drmIsMaster(drmFd))
            struct wsi_display_connector *connector =
            drmModeConnectorPtr drm_connector =
            if (!drm_connector)
                     wsi->fd = drmFd;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetDrmDisplayEXT(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
            if (!wsi_device_matches_drm_fd(wsi_device, drmFd)) {
      *pDisplay = VK_NULL_HANDLE;
               struct wsi_display_connector *connector =
            if (!connector) {
      *pDisplay = VK_NULL_HANDLE;
               *pDisplay = wsi_display_connector_to_handle(connector);
      }
