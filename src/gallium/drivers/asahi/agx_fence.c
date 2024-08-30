   /*
   * Based on panfrost/pan_fence.c:
   *
   * Copyright 2022 Amazon.com, Inc. or its affiliates.
   * Copyright 2008 VMware, Inc.
   * Copyright 2014 Broadcom
   * Copyright 2018 Alyssa Rosenzweig
   * Copyright 2019 Collabora, Ltd.
   * Copyright 2012 Rob Clark
   * SPDX-License-Identifier: MIT
   */
      #include <xf86drm.h>
      #include "agx_fence.h"
   #include "agx_state.h"
      #include "util/libsync.h"
   #include "util/os_time.h"
   #include "util/u_inlines.h"
      void
   agx_fence_reference(struct pipe_screen *pscreen, struct pipe_fence_handle **ptr,
         {
      struct agx_device *dev = agx_device(pscreen);
            if (pipe_reference(old ? &old->reference : NULL,
            drmSyncobjDestroy(dev->fd, old->syncobj);
                  }
      bool
   agx_fence_finish(struct pipe_screen *pscreen, struct pipe_context *ctx,
         {
      struct agx_device *dev = agx_device(pscreen);
            if (fence->signaled)
            uint64_t abs_timeout = os_time_get_absolute_timeout(timeout);
   if (abs_timeout == OS_TIMEOUT_INFINITE)
            ret = drmSyncobjWait(dev->fd, &fence->syncobj, 1, abs_timeout,
            assert(ret >= 0 || ret == -ETIME);
   fence->signaled = (ret >= 0);
      }
      int
   agx_fence_get_fd(struct pipe_screen *screen, struct pipe_fence_handle *f)
   {
      struct agx_device *dev = agx_device(screen);
            int ret = drmSyncobjExportSyncFile(dev->fd, f->syncobj, &fd);
   assert(ret >= 0);
               }
      struct pipe_fence_handle *
   agx_fence_from_fd(struct agx_context *ctx, int fd, enum pipe_fd_type type)
   {
      struct agx_device *dev = agx_device(ctx->base.screen);
            struct pipe_fence_handle *f = calloc(1, sizeof(*f));
   if (!f)
            if (type == PIPE_FD_TYPE_NATIVE_SYNC) {
      ret = drmSyncobjCreate(dev->fd, 0, &f->syncobj);
   if (ret) {
      agx_msg("create syncobj failed\n");
               ret = drmSyncobjImportSyncFile(dev->fd, f->syncobj, fd);
   if (ret) {
      agx_msg("import syncfile failed\n");
         } else {
      assert(type == PIPE_FD_TYPE_SYNCOBJ);
   ret = drmSyncobjFDToHandle(dev->fd, fd, &f->syncobj);
   if (ret) {
      agx_msg("import syncobj FD failed\n");
                                 err_destroy_syncobj:
         err_free_fence:
      free(f);
      }
      struct pipe_fence_handle *
   agx_fence_create(struct agx_context *ctx)
   {
      struct agx_device *dev = agx_device(ctx->base.screen);
            /* Snapshot the last rendering out fence. We'd rather have another
   * syncobj instead of a sync file, but this is all we get.
   * (HandleToFD/FDToHandle just gives you another syncobj ID for the
   * same syncobj).
   */
   ret = drmSyncobjExportSyncFile(dev->fd, ctx->syncobj, &fd);
   assert(ret >= 0 && fd != -1 && "export failed");
   if (ret || fd == -1) {
      agx_msg("export failed\n");
               struct pipe_fence_handle *f =
                        }
      void
   agx_create_fence_fd(struct pipe_context *pctx,
               {
         }
      void
   agx_fence_server_sync(struct pipe_context *pctx, struct pipe_fence_handle *f)
   {
      struct agx_device *dev = agx_device(pctx->screen);
   struct agx_context *ctx = agx_context(pctx);
            ret = drmSyncobjExportSyncFile(dev->fd, f->syncobj, &fd);
            sync_accumulate("asahi", &ctx->in_sync_fd, fd);
      }
