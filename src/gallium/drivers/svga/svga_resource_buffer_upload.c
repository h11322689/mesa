   /**********************************************************
   * Copyright 2008-2022 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
         #include "util/u_thread.h"
   #include "pipe/p_state.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "svga_cmd.h"
   #include "svga_context.h"
   #include "svga_debug.h"
   #include "svga_resource_buffer.h"
   #include "svga_resource_buffer_upload.h"
   #include "svga_screen.h"
   #include "svga_winsys.h"
      /**
   * Describes a complete SVGA_3D_CMD_UPDATE_GB_IMAGE command
   *
   */
   struct svga_3d_update_gb_image {
      SVGA3dCmdHeader header;
      };
      struct svga_3d_invalidate_gb_image {
      SVGA3dCmdHeader header;
      };
         static void
   svga_buffer_upload_ranges(struct svga_context *, struct svga_buffer *);
         /**
   * Allocate a winsys_buffer (ie. DMA, aka GMR memory).
   *
   * It will flush and retry in case the first attempt to create a DMA buffer
   * fails, so it should not be called from any function involved in flushing
   * to avoid recursion.
   */
   struct svga_winsys_buffer *
   svga_winsys_buffer_create( struct svga_context *svga,
                     {
      struct svga_screen *svgascreen = svga_screen(svga->pipe.screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
            /* Just try */
   buf = SVGA_TRY_PTR(sws->buffer_create(sws, alignment, usage, size));
   if (!buf) {
      SVGA_DBG(DEBUG_DMA|DEBUG_PERF, "flushing context to find %d bytes GMR\n",
            /* Try flushing all pending DMAs */
   svga_retry_enter(svga);
   svga_context_flush(svga, NULL);
   buf = sws->buffer_create(sws, alignment, usage, size);
                  }
         /**
   * Destroy HW storage if separate from the host surface.
   * In the GB case, the HW storage is associated with the host surface
   * and is therefore a No-op.
   */
   void
   svga_buffer_destroy_hw_storage(struct svga_screen *ss, struct svga_buffer *sbuf)
   {
               assert(sbuf->map.count == 0);
   assert(sbuf->hwbuf);
   if (sbuf->hwbuf) {
      sws->buffer_destroy(sws, sbuf->hwbuf);
         }
            /**
   * Allocate DMA'ble or Updatable storage for the buffer.
   *
   * Called before mapping a buffer.
   */
   enum pipe_error
   svga_buffer_create_hw_storage(struct svga_screen *ss,
               {
               if (ss->sws->have_gb_objects) {
      assert(sbuf->handle || !sbuf->dma.pending);
      }
   if (!sbuf->hwbuf) {
      struct svga_winsys_screen *sws = ss->sws;
   unsigned alignment = 16;
   unsigned usage = 0;
            sbuf->hwbuf = sws->buffer_create(sws, alignment, usage, size);
   if (!sbuf->hwbuf)
                           }
         /**
   * Allocate graphics memory for vertex/index/constant/texture buffer.
   */
   enum pipe_error
   svga_buffer_create_host_surface(struct svga_screen *ss,
               {
                        if (!sbuf->handle) {
                        sbuf->key.format = SVGA3D_BUFFER;
   if (bind_flags & PIPE_BIND_VERTEX_BUFFER) {
      sbuf->key.flags |= SVGA3D_SURFACE_HINT_VERTEXBUFFER;
      }
   if (bind_flags & PIPE_BIND_INDEX_BUFFER) {
      sbuf->key.flags |= SVGA3D_SURFACE_HINT_INDEXBUFFER;
      }
   if (bind_flags & PIPE_BIND_CONSTANT_BUFFER)
            if (bind_flags & PIPE_BIND_STREAM_OUTPUT)
            if (bind_flags & PIPE_BIND_SAMPLER_VIEW)
            if (bind_flags & PIPE_BIND_COMMAND_ARGS_BUFFER) {
      assert(ss->sws->have_sm5);
               if (!bind_flags && sbuf->b.usage == PIPE_USAGE_STAGING) {
      /* This surface is to be used with the
   * SVGA3D_CMD_DX_TRANSFER_FROM_BUFFER command, and no other
   * bind flags are allowed to be set for this surface.
   */
               if (ss->sws->have_gl43 &&
      (bind_flags & (PIPE_BIND_SHADER_BUFFER | PIPE_BIND_SHADER_IMAGE)) &&
   (!(bind_flags & (PIPE_BIND_STREAM_OUTPUT)))) {
   /* This surface can be bound to a uav. */
   assert((bind_flags & PIPE_BIND_CONSTANT_BUFFER) == 0);
   sbuf->key.flags |= SVGA3D_SURFACE_BIND_UAVIEW |
               if (sbuf->b.flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT) {
      /* This surface can be mapped persistently. We use
   * coherent memory if available to avoid implementing memory barriers
   * for persistent non-coherent memory for now.
                  if (ss->sws->have_gl43) {
      /* Set the persistent bit so if the buffer is to be bound
   * as constant buffer, we'll access it as raw buffer
   * instead of copying the content back and forth between the
   * mapped buffer surface and the constant buffer surface.
                  /* Set the raw views bind flag only if the mapped buffer surface
   * is not already bound as constant buffer since constant buffer
   * surface cannot have other bind flags.
   */
   sbuf->key.flags |= SVGA3D_SURFACE_BIND_UAVIEW |
                  bind_flags = bind_flags | PIPE_BIND_SHADER_BUFFER;
                     sbuf->key.size.width = sbuf->b.width0;
   sbuf->key.size.height = 1;
            sbuf->key.numFaces = 1;
   sbuf->key.numMipLevels = 1;
   sbuf->key.cachable = 1;
   sbuf->key.arraySize = 1;
            SVGA_DBG(DEBUG_DMA, "surface_create for buffer sz %d\n",
            sbuf->handle = svga_screen_surface_create(ss, bind_flags,
               if (!sbuf->handle)
            /* Set the discard flag on the first time the buffer is written
   * as svga_screen_surface_create might have passed a recycled host
   * buffer. This is only needed for host-backed mode. As in guest-backed
   * mode, the recycled buffer would have been invalidated.
   */
   if (!ss->sws->have_gb_objects)
            SVGA_DBG(DEBUG_DMA, "   --> got sid %p sz %d (buffer)\n",
            /* Add the new surface to the buffer surface list */
   sbuf->bufsurf = svga_buffer_add_host_surface(sbuf, sbuf->handle,
               if (sbuf->bufsurf == NULL)
            invalidated ? SVGA_SURFACE_STATE_INVALIDATED :
                  if (ss->sws->have_gb_objects) {
      /* Initialize the surface with zero */
   ss->sws->surface_init(ss->sws, sbuf->handle, svga_surface_size(&sbuf->key),
                     }
         /**
   * Recreates a host surface with the new bind flags.
   */
   enum pipe_error
   svga_buffer_recreate_host_surface(struct svga_context *svga,
               {
      enum pipe_error ret = PIPE_OK;
            assert(sbuf->bind_flags != bind_flags);
                     /* Create a new resource with the requested bind_flags */
   ret = svga_buffer_create_host_surface(svga_screen(svga->pipe.screen),
         if (ret == PIPE_OK) {
      /* Copy the surface data */
   assert(sbuf->handle);
   assert(sbuf->bufsurf);
   SVGA_RETRY(svga, SVGA3D_vgpu10_BufferCopy(svga->swc, old_handle,
                  /* Mark this surface as RENDERED */
               /* Set the new bind flags for this buffer resource */
            /* Set the dirty bit to signal a read back is needed before the data copied
   * to this new surface can be referenced.
   */
               }
         /**
   * Returns TRUE if the surface bind flags is compatible with the new bind flags.
   */
   static bool
   compatible_bind_flags(unsigned bind_flags,
         {
      if ((bind_flags & tobind_flags) == tobind_flags)
         else if ((bind_flags|tobind_flags) & PIPE_BIND_CONSTANT_BUFFER)
         else if ((bind_flags & PIPE_BIND_STREAM_OUTPUT) &&
            /* Stream out cannot be mixed with UAV */
      else
      }
         /**
   * Returns a buffer surface from the surface list
   * that has the requested bind flags or its existing bind flags
   * can be promoted to include the new bind flags.
   */
   static struct svga_buffer_surface *
   svga_buffer_get_host_surface(struct svga_buffer *sbuf,
         {
               LIST_FOR_EACH_ENTRY(bufsurf, &sbuf->surfaces, list) {
      if (compatible_bind_flags(bufsurf->bind_flags, bind_flags))
      }
      }
         /**
   * Adds the host surface to the buffer surface list.
   */
   struct svga_buffer_surface *
   svga_buffer_add_host_surface(struct svga_buffer *sbuf,
                     {
               bufsurf = CALLOC_STRUCT(svga_buffer_surface);
   if (!bufsurf)
            bufsurf->bind_flags = bind_flags;
   bufsurf->handle = handle;
            /* add the surface to the surface list */
            /* Set the new bind flags for this buffer resource */
               }
         /**
   * Start using the specified surface for this buffer resource.
   */
   void
   svga_buffer_bind_host_surface(struct svga_context *svga,
               {
      /* Update the to-bind surface */
   assert(bufsurf->handle);
            /* If we are switching from stream output to other buffer,
   * make sure to copy the buffer content.
   */
   if (sbuf->bind_flags & PIPE_BIND_STREAM_OUTPUT) {
      SVGA_RETRY(svga, SVGA3D_vgpu10_BufferCopy(svga->swc, sbuf->handle,
                           /* Set this surface as the current one */
   sbuf->handle = bufsurf->handle;
   sbuf->key = bufsurf->key;
   sbuf->bind_flags = bufsurf->bind_flags;
      }
         /**
   * Prepare a host surface that can be used as indicated in the
   * tobind_flags. If the existing host surface is not created
   * with the necessary binding flags and if the new bind flags can be
   * combined with the existing bind flags, then we will recreate a
   * new surface with the combined bind flags. Otherwise, we will create
   * a surface for that incompatible bind flags.
   * For example, if a stream output buffer is reused as a constant buffer,
   * since constant buffer surface cannot be bound as a stream output surface,
   * two surfaces will be created, one for stream output,
   * and another one for constant buffer.
   */
   enum pipe_error
   svga_buffer_validate_host_surface(struct svga_context *svga,
               {
      struct svga_buffer_surface *bufsurf;
            /* upload any dirty ranges */
            /* Flush any pending upload first */
            /* First check from the cached buffer surface list to see if there is
   * already a buffer surface that has the requested bind flags, or
   * surface with compatible bind flags that can be promoted.
   */
            if (bufsurf) {
      if ((bufsurf->bind_flags & tobind_flags) == tobind_flags) {
      /* there is a surface with the requested bind flags */
                  /* Recreate a host surface with the combined bind flags */
   ret = svga_buffer_recreate_host_surface(svga, sbuf,
                  /* Destroy the old surface */
   svga_screen_surface_destroy(svga_screen(sbuf->b.screen),
                        list_del(&bufsurf->list);
         } else {
      /* Need to create a new surface if the bind flags are incompatible,
   * such as constant buffer surface & stream output surface.
   */
   ret = svga_buffer_recreate_host_surface(svga, sbuf,
      }
      }
         void
   svga_buffer_destroy_host_surface(struct svga_screen *ss,
         {
               LIST_FOR_EACH_ENTRY_SAFE(bufsurf, next, &sbuf->surfaces, list) {
      SVGA_DBG(DEBUG_DMA, " ungrab sid %p sz %d\n",
         svga_screen_surface_destroy(ss, &bufsurf->key,
                     }
         /**
   * Insert a number of preliminary UPDATE_GB_IMAGE commands in the
   * command buffer, equal to the current number of mapped ranges.
   * The UPDATE_GB_IMAGE commands will be patched with the
   * actual ranges just before flush.
   */
   static enum pipe_error
   svga_buffer_upload_gb_command(struct svga_context *svga,
         {
      struct svga_winsys_context *swc = svga->swc;
   SVGA3dCmdUpdateGBImage *update_cmd;
   struct svga_3d_update_gb_image *whole_update_cmd = NULL;
   const uint32 numBoxes = sbuf->map.num_ranges;
   struct pipe_resource *dummy;
            if (swc->force_coherent || sbuf->key.coherent)
            assert(svga_have_gb_objects(svga));
   assert(numBoxes);
            /* Allocate FIFO space for 'numBoxes' UPDATE_GB_IMAGE commands */
   const unsigned total_commands_size =
            update_cmd = SVGA3D_FIFOReserve(swc,
               if (!update_cmd)
            /* The whole_update_command is a SVGA3dCmdHeader plus the
   * SVGA3dCmdUpdateGBImage command.
   */
            /* Init the first UPDATE_GB_IMAGE command */
   whole_update_cmd->header.size = sizeof(*update_cmd);
   swc->surface_relocation(swc, &update_cmd->image.sid, NULL, sbuf->handle,
         update_cmd->image.face = 0;
            /* Save pointer to the first UPDATE_GB_IMAGE command so that we can
   * fill in the box info below.
   */
            /*
   * Copy the face, mipmap, etc. info to all subsequent commands.
   * Also do the surface relocation for each subsequent command.
   */
   for (i = 1; i < numBoxes; ++i) {
      whole_update_cmd++;
            swc->surface_relocation(swc, &whole_update_cmd->body.image.sid, NULL,
                     /* Increment reference count */
   sbuf->dma.svga = svga;
   dummy = NULL;
   pipe_resource_reference(&dummy, &sbuf->b);
            swc->hints |= SVGA_HINT_FLAG_CAN_PRE_FLUSH;
                        }
         /**
   * Issue DMA commands to transfer guest memory to the host.
   * Note that the memory segments (offset, size) will be patched in
   * later in the svga_buffer_upload_flush() function.
   */
   static enum pipe_error
   svga_buffer_upload_hb_command(struct svga_context *svga,
         {
      struct svga_winsys_context *swc = svga->swc;
   struct svga_winsys_buffer *guest = sbuf->hwbuf;
   struct svga_winsys_surface *host = sbuf->handle;
   const SVGA3dTransferType transfer = SVGA3D_WRITE_HOST_VRAM;
   SVGA3dCmdSurfaceDMA *cmd;
   const uint32 numBoxes = sbuf->map.num_ranges;
   SVGA3dCopyBox *boxes;
   SVGA3dCmdSurfaceDMASuffix *pSuffix;
   unsigned region_flags;
   unsigned surface_flags;
                     if (transfer == SVGA3D_WRITE_HOST_VRAM) {
      region_flags = SVGA_RELOC_READ;
      }
   else if (transfer == SVGA3D_READ_HOST_VRAM) {
      region_flags = SVGA_RELOC_WRITE;
      }
   else {
      assert(0);
                        cmd = SVGA3D_FIFOReserve(swc,
                     if (!cmd)
            swc->region_relocation(swc, &cmd->guest.ptr, guest, 0, region_flags);
            swc->surface_relocation(swc, &cmd->host.sid, NULL, host, surface_flags);
   cmd->host.face = 0;
                     sbuf->dma.boxes = (SVGA3dCopyBox *)&cmd[1];
            /* Increment reference count */
   dummy = NULL;
            pSuffix = (SVGA3dCmdSurfaceDMASuffix *)((uint8_t*)cmd + sizeof *cmd + numBoxes * sizeof *boxes);
   pSuffix->suffixSize = sizeof *pSuffix;
   pSuffix->maximumOffset = sbuf->b.width0;
                     swc->hints |= SVGA_HINT_FLAG_CAN_PRE_FLUSH;
                        }
         /**
   * Issue commands to transfer guest memory to the host.
   */
   static enum pipe_error
   svga_buffer_upload_command(struct svga_context *svga, struct svga_buffer *sbuf)
   {
      if (svga_have_gb_objects(svga)) {
         } else {
            }
         /**
   * Patch up the upload DMA command reserved by svga_buffer_upload_command
   * with the final ranges.
   */
   void
   svga_buffer_upload_flush(struct svga_context *svga, struct svga_buffer *sbuf)
   {
      unsigned i;
            if (!sbuf->dma.pending || svga->swc->force_coherent ||
      sbuf->key.coherent) {
   //debug_printf("no dma pending on buffer\n");
               assert(sbuf->handle);
   assert(sbuf->map.num_ranges);
            /*
   * Patch the DMA/update command with the final copy box.
   */
   if (svga_have_gb_objects(svga)) {
                        for (i = 0; i < sbuf->map.num_ranges; ++i, ++update) {
                              box->x = sbuf->map.ranges[i].start;
   box->y = 0;
   box->z = 0;
   box->w = sbuf->map.ranges[i].end - sbuf->map.ranges[i].start;
                                 svga->hud.num_bytes_uploaded += box->w;
         }
   else {
      assert(sbuf->hwbuf);
   assert(sbuf->dma.boxes);
            for (i = 0; i < sbuf->map.num_ranges; ++i) {
                              box->x = sbuf->map.ranges[i].start;
   box->y = 0;
   box->z = 0;
   box->w = sbuf->map.ranges[i].end - sbuf->map.ranges[i].start;
   box->h = 1;
   box->d = 1;
   box->srcx = sbuf->map.ranges[i].start;
                                 svga->hud.num_bytes_uploaded += box->w;
                                    assert(sbuf->head.prev && sbuf->head.next);
   list_del(&sbuf->head);  /* remove from svga->dirty_buffers list */
   sbuf->dma.pending = false;
   sbuf->dma.flags.discard = false;
            sbuf->dma.svga = NULL;
   sbuf->dma.boxes = NULL;
            /* Decrement reference count (and potentially destroy) */
   dummy = &sbuf->b;
      }
         /**
   * Note a dirty range.
   *
   * This function only notes the range down. It doesn't actually emit a DMA
   * upload command. That only happens when a context tries to refer to this
   * buffer, and the DMA upload command is added to that context's command
   * buffer.
   *
   * We try to lump as many contiguous DMA transfers together as possible.
   */
   void
   svga_buffer_add_range(struct svga_buffer *sbuf, unsigned start, unsigned end)
   {
      unsigned i;
   unsigned nearest_range;
                     if (sbuf->map.num_ranges < SVGA_BUFFER_MAX_RANGES) {
      nearest_range = sbuf->map.num_ranges;
      } else {
      nearest_range = SVGA_BUFFER_MAX_RANGES - 1;
               /*
   * Try to grow one of the ranges.
   */
   for (i = 0; i < sbuf->map.num_ranges; ++i) {
      const int left_dist = start - sbuf->map.ranges[i].end;
   const int right_dist = sbuf->map.ranges[i].start - end;
            if (dist <= 0) {
      /*
   * Ranges are contiguous or overlapping -- extend this one and return.
   *
   * Note that it is not this function's task to prevent overlapping
   * ranges, as the GMR was already given so it is too late to do
   * anything.  If the ranges overlap here it must surely be because
   * PIPE_MAP_UNSYNCHRONIZED was set.
   */
   sbuf->map.ranges[i].start = MIN2(sbuf->map.ranges[i].start, start);
   sbuf->map.ranges[i].end   = MAX2(sbuf->map.ranges[i].end,   end);
      }
   else {
      /*
   * Discontiguous ranges -- keep track of the nearest range.
   */
   if (dist < nearest_dist) {
      nearest_range = i;
                     /*
   * We cannot add a new range to an existing DMA command, so patch-up the
   * pending DMA upload and start clean.
                     assert(!sbuf->dma.pending);
   assert(!sbuf->dma.svga);
            if (sbuf->map.num_ranges < SVGA_BUFFER_MAX_RANGES) {
      /*
   * Add a new range.
            sbuf->map.ranges[sbuf->map.num_ranges].start = start;
   sbuf->map.ranges[sbuf->map.num_ranges].end = end;
      } else {
      /*
   * Everything else failed, so just extend the nearest range.
   *
   * It is OK to do this because we always keep a local copy of the
   * host buffer data, for SW TNL, and the host never modifies the buffer.
            assert(nearest_range < SVGA_BUFFER_MAX_RANGES);
   assert(nearest_range < sbuf->map.num_ranges);
   sbuf->map.ranges[nearest_range].start =
         sbuf->map.ranges[nearest_range].end =
         }
         /**
   * Copy the contents of the malloc buffer to a hardware buffer.
   */
   static enum pipe_error
   svga_buffer_update_hw(struct svga_context *svga, struct svga_buffer *sbuf,
         {
      assert(!sbuf->user);
   if (!svga_buffer_has_hw_storage(sbuf)) {
      struct svga_screen *ss = svga_screen(sbuf->b.screen);
   enum pipe_error ret;
   bool retry;
   void *map;
            assert(sbuf->swbuf);
   if (!sbuf->swbuf)
            ret = svga_buffer_create_hw_storage(svga_screen(sbuf->b.screen), sbuf,
         if (ret != PIPE_OK)
            mtx_lock(&ss->swc_mutex);
   map = svga_buffer_hw_storage_map(svga, sbuf, PIPE_MAP_WRITE, &retry);
   assert(map);
   assert(!retry);
   if (!map) {
      mtx_unlock(&ss->swc_mutex);
   svga_buffer_destroy_hw_storage(ss, sbuf);
               /* Copy data from malloc'd swbuf to the new hardware buffer */
   for (i = 0; i < sbuf->map.num_ranges; i++) {
      unsigned start = sbuf->map.ranges[i].start;
   unsigned len = sbuf->map.ranges[i].end - start;
               if (svga->swc->force_coherent || sbuf->key.coherent)
                     /* This user/malloc buffer is now indistinguishable from a gpu buffer */
   assert(sbuf->map.count == 0);
   if (sbuf->map.count == 0) {
      if (sbuf->user)
         else
                                    }
         /**
   * Upload the buffer to the host in a piecewise fashion.
   *
   * Used when the buffer is too big to fit in the GMR aperture.
   * This function should never get called in the guest-backed case
   * since we always have a full-sized hardware storage backing the
   * host surface.
   */
   static enum pipe_error
   svga_buffer_upload_piecewise(struct svga_screen *ss,
               {
      struct svga_winsys_screen *sws = ss->sws;
   const unsigned alignment = sizeof(void *);
   const unsigned usage = 0;
            assert(sbuf->map.num_ranges);
   assert(!sbuf->dma.pending);
                     for (i = 0; i < sbuf->map.num_ranges; ++i) {
      const struct svga_buffer_range *range = &sbuf->map.ranges[i];
   unsigned offset = range->start;
            while (offset < range->end) {
                                    hwbuf = sws->buffer_create(sws, alignment, usage, size);
   while (!hwbuf) {
      size /= 2;
   if (!size)
                                    map = sws->buffer_map(sws, hwbuf,
               assert(map);
   if (map) {
      memcpy(map, (const char *) sbuf->swbuf + offset, size);
               SVGA_RETRY(svga, SVGA3D_BufferDMA(svga->swc,
                                                                  }
         /**
   * A helper function to add an update command for the dirty ranges if there
   * isn't already one.
   */
   static void
   svga_buffer_upload_ranges(struct svga_context *svga,
         {
      struct pipe_screen *screen = svga->pipe.screen;
   struct svga_screen *ss = svga_screen(screen);
            if (sbuf->map.num_ranges) {
      if (!sbuf->dma.pending) {
               /* Migrate the data from swbuf -> hwbuf if necessary */
   ret = svga_buffer_update_hw(svga, sbuf, sbuf->bind_flags);
   if (ret == PIPE_OK) {
      /* Emit DMA or UpdateGBImage commands */
   SVGA_RETRY_OOM(svga, ret, svga_buffer_upload_command(svga, sbuf));
   if (ret == PIPE_OK) {
      sbuf->dma.pending = true;
   assert(!sbuf->head.prev && !sbuf->head.next);
         }
   else if (ret == PIPE_ERROR_OUT_OF_MEMORY) {
      /*
   * The buffer is too big to fit in the GMR aperture, so break it in
   * smaller pieces.
   */
               if (ret != PIPE_OK) {
      /*
   * Something unexpected happened above. There is very little that
   * we can do other than proceeding while ignoring the dirty ranges.
   */
   assert(0);
         }
   else {
      /*
   * There a pending dma already. Make sure it is from this context.
   */
         }
      }
         /**
   * Get (or create/upload) the winsys surface handle so that we can
   * refer to this buffer in fifo commands.
   * This function will create the host surface, and in the GB case also the
   * hardware storage. In the non-GB case, the hardware storage will be created
   * if there are mapped ranges and the data is currently in a malloc'ed buffer.
   */
   struct svga_winsys_surface *
   svga_buffer_handle(struct svga_context *svga, struct pipe_resource *buf,
         {
      struct pipe_screen *screen = svga->pipe.screen;
   struct svga_screen *ss = svga_screen(screen);
   struct svga_buffer *sbuf;
            if (!buf)
                              if (sbuf->handle) {
      if ((sbuf->bind_flags & tobind_flags) != tobind_flags) {
      /* If the allocated resource's bind flags do not include the
   * requested bind flags, validate the host surface.
   */
   ret = svga_buffer_validate_host_surface(svga, sbuf, tobind_flags);
   if (ret != PIPE_OK)
         } else {
      /* If there is no resource handle yet, then combine the buffer bind
   * flags and the tobind_flags if they are compatible.
   * If not, just use the tobind_flags for creating the resource handle.
   */
   if (compatible_bind_flags(sbuf->bind_flags, tobind_flags))
         else
                     /* This call will set sbuf->handle */
   if (svga_have_gb_objects(svga)) {
         } else {
         }
   if (ret != PIPE_OK)
               assert(sbuf->handle);
   assert(sbuf->bufsurf);
   if (svga->swc->force_coherent || sbuf->key.coherent)
            /* upload any dirty ranges */
                        }
         void
   svga_context_flush_buffers(struct svga_context *svga)
   {
                        curr = svga->dirty_buffers.next;
   next = curr->next;
   while (curr != &svga->dirty_buffers) {
               assert(p_atomic_read(&sbuf->b.reference.count) != 0);
                     curr = next;
                  }
