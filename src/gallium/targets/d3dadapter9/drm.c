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
      /* XXX: header order is slightly screwy here */
   #include "loader.h"
      #include "adapter9.h"
      #include "pipe-loader/pipe_loader.h"
      #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
      #include "target-helpers/drm_helper.h"
   #include "target-helpers/sw_helper.h"
   #include "frontend/drm_driver.h"
      #include "d3dadapter/d3dadapter9.h"
   #include "d3dadapter/drm.h"
      #include "util/xmlconfig.h"
   #include "util/driconf.h"
      #include "drm-uapi/drm.h"
   #include <sys/ioctl.h>
   #include <fcntl.h>
   #include <stdio.h>
      #define DBG_CHANNEL DBG_ADAPTER
      /* On non-x86 archs, Box86 has issues with thread_submit. */
   #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
   #define DEFAULT_THREADSUBMIT true
   #else
   #define DEFAULT_THREADSUBMIT false
   #endif
      const driOptionDescription __driConfigOptionsNine[] = {
      DRI_CONF_SECTION_PERFORMANCE
         DRI_CONF_SECTION_END
   DRI_CONF_SECTION_NINE
      DRI_CONF_NINE_OVERRIDEVENDOR(-1)
   DRI_CONF_NINE_THROTTLE(-2)
   DRI_CONF_NINE_THREADSUBMIT(DEFAULT_THREADSUBMIT)
   DRI_CONF_NINE_ALLOWDISCARDDELAYEDRELEASE(true)
   DRI_CONF_NINE_TEARFREEDISCARD(true)
   DRI_CONF_NINE_CSMT(-1)
   DRI_CONF_NINE_DYNAMICTEXTUREWORKAROUND(true)
   DRI_CONF_NINE_SHADERINLINECONSTANTS(false)
   DRI_CONF_NINE_SHMEM_LIMIT()
   DRI_CONF_NINE_FORCESWRENDERINGONCPU(false)
      DRI_CONF_SECTION_END
   DRI_CONF_SECTION_DEBUG
            };
      struct fallback_card_config {
      const char *name;
   unsigned vendor_id;
      } fallback_cards[] = {
         {"NV124", 0x10de, 0x13C2}, /* NVIDIA GeForce GTX 970 */
   {"HAWAII", 0x1002, 0x67b1}, /* AMD Radeon R9 290 */
   {"Haswell Mobile", 0x8086, 0x13C2}, /* Intel Haswell Mobile */
   };
      /* prototypes */
   void
   d3d_match_vendor_id( D3DADAPTER_IDENTIFIER9* drvid,
                        void d3d_fill_driver_version(D3DADAPTER_IDENTIFIER9* drvid);
      void d3d_fill_cardname(D3DADAPTER_IDENTIFIER9* drvid);
      struct d3dadapter9drm_context
   {
      struct d3dadapter9_context base;
   struct pipe_loader_device *dev, *swdev;
      };
      static void
   drm_destroy( struct d3dadapter9_context *ctx )
   {
               if (ctx->ref && ctx->hal != ctx->ref)
         /* because ref is a wrapper around hal, freeing ref frees hal too. */
   else if (ctx->hal)
            if (drm->swdev && drm->swdev != drm->dev)
         if (drm->dev)
            close(drm->fd);
      }
      static inline void
   get_bus_info( int fd,
               DWORD *vendorid,
   DWORD *deviceid,
   {
               if (loader_get_pci_id_for_fd(fd, &vid, &did)) {
      DBG("PCI info: vendor=0x%04x, device=0x%04x\n",
         *vendorid = vid;
   *deviceid = did;
   *subsysid = 0;
      } else {
      DBG("Unable to detect card. Faking %s\n", fallback_cards[0].name);
   *vendorid = fallback_cards[0].vendor_id;
   *deviceid = fallback_cards[0].device_id;
   *subsysid = 0;
         }
      static inline void
   read_descriptor( struct d3dadapter9_context *ctx,
         {
      unsigned i;
   BOOL found;
            memset(drvid, 0, sizeof(*drvid));
   get_bus_info(fd, &drvid->VendorId, &drvid->DeviceId,
         snprintf(drvid->DeviceName, sizeof(drvid->DeviceName),
         snprintf(drvid->Description, sizeof(drvid->Description),
            if (override_vendorid > 0) {
      found = false;
   /* fill in device_id and card name for fake vendor */
   for (i = 0; i < sizeof(fallback_cards)/sizeof(fallback_cards[0]); i++) {
         if (fallback_cards[i].vendor_id == override_vendorid) {
      DBG("Faking card '%s' vendor 0x%04x, device 0x%04x\n",
            fallback_cards[i].name,
      drvid->VendorId = fallback_cards[i].vendor_id;
   drvid->DeviceId = fallback_cards[i].device_id;
   snprintf(drvid->Description, sizeof(drvid->Description),
         found = true;
      }
   if (!found) {
            }
   /* choose fall-back vendor if necessary to allow
   * the following functions to return sane results */
   d3d_match_vendor_id(drvid, fallback_cards[0].vendor_id, fallback_cards[0].device_id, fallback_cards[0].name);
   /* fill in driver name and version info */
   d3d_fill_driver_version(drvid);
   /* override Description field with Windows like names */
            /* this driver isn't WHQL certified */
            /* this value is fixed */
   drvid->DeviceIdentifier.Data1 = 0xaeb2cdd4;
   drvid->DeviceIdentifier.Data2 = 0x6e41;
   drvid->DeviceIdentifier.Data3 = 0x43ea;
   drvid->DeviceIdentifier.Data4[0] = 0x94;
   drvid->DeviceIdentifier.Data4[1] = 0x1c;
   drvid->DeviceIdentifier.Data4[2] = 0x83;
   drvid->DeviceIdentifier.Data4[3] = 0x61;
   drvid->DeviceIdentifier.Data4[4] = 0xcc;
   drvid->DeviceIdentifier.Data4[5] = 0x76;
   drvid->DeviceIdentifier.Data4[6] = 0x07;
      }
      static HRESULT WINAPI
   drm_create_adapter( int fd,
         {
      struct d3dadapter9drm_context *ctx = CALLOC_STRUCT(d3dadapter9drm_context);
   HRESULT hr;
   bool different_device;
   driOptionCache defaultInitOptions;
   driOptionCache userInitOptions;
   int throttling_value_user = -2;
   int override_vendorid = -1;
                              /* Although the fd is provided from external source, mesa/nine
   * takes ownership of it. */
   different_device = loader_get_user_preferred_fd(&fd, NULL);
   ctx->fd = fd;
            if (!pipe_loader_drm_probe_fd(&ctx->dev, fd, false)) {
      ERR("Failed to probe drm fd %d.\n", fd);
   FREE(ctx);
   close(fd);
               ctx->base.hal = pipe_loader_create_screen(ctx->dev);
   if (!ctx->base.hal) {
      ERR("Unable to load requested driver.\n");
   drm_destroy(&ctx->base);
               if (!ctx->base.hal->get_param(ctx->base.hal, PIPE_CAP_DMABUF)) {
      ERR("The driver is not capable of dma-buf sharing."
         drm_destroy(&ctx->base);
               /* Previously was set to PIPE_CAP_MAX_FRAMES_IN_FLIGHT,
   * but the change of value of this cap to 1 seems to cause
   * regressions. */
   ctx->base.throttling_value = 2;
            driParseOptionInfo(&defaultInitOptions, __driConfigOptionsNine,
         driParseConfigFiles(&userInitOptions, &defaultInitOptions, 0,
         if (driCheckOption(&userInitOptions, "throttle_value", DRI_INT)) {
      throttling_value_user = driQueryOptioni(&userInitOptions, "throttle_value");
   if (throttling_value_user == -1)
         else if (throttling_value_user >= 0) {
         ctx->base.throttling = true;
               ctx->base.vblank_mode = driQueryOptioni(&userInitOptions, "vblank_mode");
   ctx->base.thread_submit = driQueryOptionb(&userInitOptions, "thread_submit"); /* TODO: default to TRUE if different_device */
            ctx->base.discard_delayed_release = driQueryOptionb(&userInitOptions, "discard_delayed_release");
            if (ctx->base.tearfree_discard && !ctx->base.discard_delayed_release) {
      ERR("tearfree_discard requires discard_delayed_release\n");
               ctx->base.csmt_force = driQueryOptioni(&userInitOptions, "csmt_force");
   ctx->base.dynamic_texture_workaround = driQueryOptionb(&userInitOptions, "dynamic_texture_workaround");
   ctx->base.shader_inline_constants = driQueryOptionb(&userInitOptions, "shader_inline_constants");
   ctx->base.memfd_virtualsizelimit = driQueryOptioni(&userInitOptions, "texture_memory_limit");
   ctx->base.override_vram_size = driQueryOptioni(&userInitOptions, "override_vram_size");
   ctx->base.force_emulation = driQueryOptionb(&userInitOptions, "force_features_emulation");
            driDestroyOptionCache(&userInitOptions);
            sw_rendering |= debug_get_bool_option("D3D_ALWAYS_SOFTWARE", false);
   /* wrap it to create a software screen that can share resources */
   if (sw_rendering && pipe_loader_sw_probe_wrapped(&ctx->swdev, ctx->base.hal))
         else {
      /* Use the hardware for sw rendering */
   ctx->swdev = ctx->dev;
               /* read out PCI info */
            /* create and return new ID3DAdapter9 */
   hr = NineAdapter9_new(&ctx->base, (struct NineAdapter9 **)ppAdapter);
   if (FAILED(hr)) {
      drm_destroy(&ctx->base);
                  }
      const struct D3DAdapter9DRM drm9_desc = {
      .major_version = D3DADAPTER9DRM_MAJOR,
   .minor_version = D3DADAPTER9DRM_MINOR,
      };
