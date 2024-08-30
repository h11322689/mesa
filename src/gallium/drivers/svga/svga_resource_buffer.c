   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
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
      #include "svga_cmd.h"
      #include "pipe/p_state.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_thread.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_resource.h"
      #include "svga_context.h"
   #include "svga_screen.h"
   #include "svga_resource_buffer.h"
   #include "svga_resource_buffer_upload.h"
   #include "svga_resource_texture.h"
   #include "svga_sampler_view.h"
   #include "svga_winsys.h"
   #include "svga_debug.h"
         /**
   * Determine what buffers eventually need hardware backing.
   *
   * Vertex- and index buffers need hardware backing.  Constant buffers
   * do on vgpu10. Staging texture-upload buffers do when they are
   * supported.
   */
   static inline bool
   svga_buffer_needs_hw_storage(const struct svga_screen *ss,
         {
      unsigned bind_mask = (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER |
                  if (ss->sws->have_vgpu10) {
      /*
   * Driver-created upload const0- and staging texture upload buffers
   * tagged with PIPE_BIND_CUSTOM
   */
   bind_mask |= PIPE_BIND_CUSTOM;
   /**
   * Uniform buffer objects.
   * Don't create hardware storage for state-tracker constant buffers,
   * because we frequently map them for reading and writing, and
   * the length of those buffers are always small, so it is better
   * to just use system memory.
               if (template->flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT)
               }
         static inline bool
   need_buf_readback(struct svga_context *svga,
         {
               if (st->usage != PIPE_MAP_READ)
            /* No buffer surface has been created */
   if (!sbuf->bufsurf)
            return  ((sbuf->dirty ||
            }
         /**
   * Create a buffer transfer.
   *
   * Unlike texture DMAs (which are written immediately to the command buffer and
   * therefore inherently serialized with other context operations), for buffers
   * we try to coalesce multiple range mappings (i.e, multiple calls to this
   * function) into a single DMA command, for better efficiency in command
   * processing.  This means we need to exercise extra care here to ensure that
   * the end result is exactly the same as if one DMA was used for every mapped
   * range.
   */
   void *
   svga_buffer_transfer_map(struct pipe_context *pipe,
                           struct pipe_resource *resource,
   {
      struct svga_context *svga = svga_context(pipe);
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_buffer *sbuf = svga_buffer(resource);
   struct pipe_transfer *transfer;
   uint8_t *map = NULL;
                     assert(box->y == 0);
   assert(box->z == 0);
   assert(box->height == 1);
            transfer = MALLOC_STRUCT(pipe_transfer);
   if (!transfer) {
                  transfer->resource = resource;
   transfer->level = level;
   transfer->usage = usage;
   transfer->box = *box;
   transfer->stride = 0;
            if (usage & PIPE_MAP_WRITE) {
      /* If we write to the buffer for any reason, free any saved translated
   * vertices.
   */
               /* If it is a read transfer and the buffer is dirty or the buffer is bound
   * to a uav, we will need to read the subresource content from the device.
   */
   if (need_buf_readback(svga, transfer)) {
      /* Host-side buffers can be dirtied with vgpu10 features
   * (streamout and buffer copy) and sm5 feature via uav.
   */
            if (!sbuf->user) {
                  if (sbuf->dma.pending) {
      svga_buffer_upload_flush(svga, sbuf);
                        SVGA_RETRY(svga, SVGA3D_ReadbackGBSurface(svga->swc, sbuf->handle));
                              /* Mark the buffer surface state as UPDATED */
   assert(sbuf->bufsurf);
               if (usage & PIPE_MAP_WRITE) {
      if ((usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE) &&
      !(resource->flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT)) {
   /*
   * Flush any pending primitives, finish writing any pending DMA
   * commands, and tell the host to discard the buffer contents on
                                             /*
   * Instead of flushing the context command buffer, simply discard
   * the current hwbuf, and start a new one.
   * With GB objects, the map operation takes care of this
   * if passed the PIPE_MAP_DISCARD_WHOLE_RESOURCE flag,
                  if (!svga_have_gb_objects(svga))
               sbuf->map.num_ranges = 0;
               if (usage & PIPE_MAP_UNSYNCHRONIZED) {
      if (!sbuf->map.num_ranges) {
      /*
   * No pending ranges to upload so far, so we can tell the host to
                        } else {
      /*
   * Synchronizing, so flush any pending primitives, finish writing any
                                             if (svga_buffer_has_hw_storage(sbuf)) {
      /*
   * We have a pending DMA upload from a hardware buffer, therefore
   * we need to ensure that the host finishes processing that DMA
   * command before the gallium frontend can start overwriting the
   * hardware buffer.
   *
   * XXX: This could be avoided by tying the hardware buffer to
   * the transfer (just as done with textures), which would allow
   * overlapping DMAs commands to be queued on the same context
   * buffer. However, due to the likelihood of software vertex
   * processing, it is more convenient to hold on to the hardware
                        if (usage & PIPE_MAP_DONTBLOCK) {
      /*
   * Flushing the command buffer here will most likely cause
   * the map of the hwbuf below to block, so preemptively
                                                                        if (!sbuf->swbuf && !svga_buffer_has_hw_storage(sbuf)) {
      if (svga_buffer_create_hw_storage(ss, sbuf, sbuf->bind_flags) != PIPE_OK) {
      /*
   * We can't create a hardware buffer big enough, so create a malloc
   * buffer instead.
   */
   if (0) {
      debug_printf("%s: failed to allocate %u KB of DMA, "
                           sbuf->swbuf = align_malloc(sbuf->b.width0, 16);
   if (!sbuf->swbuf) {
      FREE(transfer);
                     if (sbuf->swbuf) {
      /* User/malloc buffer */
      }
   else if (svga_buffer_has_hw_storage(sbuf)) {
               map = SVGA_TRY_MAP(svga_buffer_hw_storage_map
         if (map == NULL && retry) {
      /*
   * At this point, svga_buffer_get_transfer() has already
   * hit the DISCARD_WHOLE_RESOURCE path and flushed HWTNL
   * for this buffer.
   */
   svga_retry_enter(svga);
   svga_context_flush(svga, NULL);
   map = svga_buffer_hw_storage_map(svga, sbuf, transfer->usage, &retry);
         }
   else {
                  if (map) {
      ++sbuf->map.count;
   map += transfer->box.x;
      } else {
                        done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         void
   svga_buffer_transfer_flush_region(struct pipe_context *pipe,
               {
      struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_buffer *sbuf = svga_buffer(transfer->resource);
   struct svga_context *svga = svga_context(pipe);
   unsigned offset = transfer->box.x + box->x;
            assert(transfer->usage & PIPE_MAP_WRITE);
            if (!(svga->swc->force_coherent || sbuf->key.coherent) || sbuf->swbuf) {
      mtx_lock(&ss->swc_mutex);
   svga_buffer_add_range(sbuf, offset, offset + length);
         }
         void
   svga_buffer_transfer_unmap(struct pipe_context *pipe,
         {
      struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_context *svga = svga_context(pipe);
                              assert(sbuf->map.count);
   if (sbuf->map.count) {
                              /* Note: we may wind up flushing here and unmapping other buffers
   * which leads to recursively locking ss->swc_mutex.
   */
               if (transfer->usage & PIPE_MAP_WRITE) {
      if (!(transfer->usage & PIPE_MAP_FLUSH_EXPLICIT)) {
      /*
   * Mapped range not flushed explicitly, so flush the whole buffer,
   * and tell the host to discard the contents when processing the DMA
                                    if (!(svga->swc->force_coherent || sbuf->key.coherent) || sbuf->swbuf)
               if (sbuf->swbuf &&
      (!sbuf->bind_flags || (sbuf->bind_flags & PIPE_BIND_CONSTANT_BUFFER))) {
   /*
   * Since the constant buffer is in system buffer, we need
   * to set the constant buffer dirty bits, so that the context
   * can update the changes in the device.
   * According to the GL spec, buffer bound to other contexts will
   * have to be explicitly rebound by the user to have the changes take
   * into effect.
   */
                  mtx_unlock(&ss->swc_mutex);
   FREE(transfer);
      }
         void
   svga_resource_destroy(struct pipe_screen *screen,
         {
      if (buf->target == PIPE_BUFFER) {
      struct svga_screen *ss = svga_screen(screen);
                              if (sbuf->handle)
            if (sbuf->uploaded.buffer)
            if (sbuf->hwbuf)
            if (sbuf->swbuf && !sbuf->user)
                     ss->hud.total_resource_bytes -= sbuf->size;
   assert(ss->hud.num_resources > 0);
   if (ss->hud.num_resources > 0)
               } else {
      struct svga_screen *ss = svga_screen(screen);
                              /*
   DBG("%s deleting %p\n", __func__, (void *) tex);
   */
            bool to_invalidate = svga_was_texture_rendered_to(tex);
            /* Destroy the backed surface handle if exists */
   if (tex->backed_handle)
                     FREE(tex->defined);
   FREE(tex->rendered_to);
   FREE(tex->dirty);
            assert(ss->hud.num_resources > 0);
   if (ss->hud.num_resources > 0)
         }
      struct pipe_resource *
   svga_buffer_create(struct pipe_screen *screen,
         {
      struct svga_screen *ss = svga_screen(screen);
   struct svga_buffer *sbuf;
                     sbuf = CALLOC_STRUCT(svga_buffer);
   if (!sbuf)
            sbuf->b = *template;
   pipe_reference_init(&sbuf->b.reference, 1);
   sbuf->b.screen = screen;
                     if (bind_flags & PIPE_BIND_CONSTANT_BUFFER) {
      /* Constant buffers can only have the PIPE_BIND_CONSTANT_BUFFER
   * flag set.
   */
   if (ss->sws->have_vgpu10) {
                     /* Although svga device only requires constant buffer size to be
   * in multiples of 16, in order to allow bind_flags promotion,
   * we are mandating all buffer size to be in multiples of 16.
   */
                        /* If the buffer is not used for constant buffer, set
   * the vertex/index bind flags as well so that the buffer will be
   * accepted for those uses.
   * Note that the PIPE_BIND_ flags we get from the gallium frontend are
   * just a hint about how the buffer may be used.  And OpenGL buffer
   * object may be used for many different things.
   * Also note that we do not unconditionally set the streamout
   * bind flag since streamout buffer is an output buffer and
   * might have performance implication.
   */
   if (!(template->bind & PIPE_BIND_CONSTANT_BUFFER) &&
      !(template->bind & PIPE_BIND_CUSTOM)) {
   /* Not a constant- or staging buffer.
   * The buffer may be used for vertex data or indexes.
   */
                  /* It may be used for shader resource as well. */
               if (svga_buffer_create_host_surface(ss, sbuf, bind_flags) != PIPE_OK)
      }
   else {
      sbuf->swbuf = align_malloc(sbuf->b.width0, 64);
   if (!sbuf->swbuf)
            /* Since constant buffer is usually small, it is much cheaper to
   * use system memory for the data just as it is being done for
   * the default constant buffer.
   */
   if ((bind_flags & PIPE_BIND_CONSTANT_BUFFER) || !bind_flags)
               debug_reference(&sbuf->b.reference,
            sbuf->bind_flags = bind_flags;
   sbuf->size = util_resource_size(&sbuf->b);
            ss->hud.num_resources++;
                  error2:
         error1:
      SVGA_STATS_TIME_POP(ss->sws);
      }
         struct pipe_resource *
   svga_user_buffer_create(struct pipe_screen *screen,
                     {
      struct svga_buffer *sbuf;
            sbuf = CALLOC_STRUCT(svga_buffer);
   if (!sbuf)
            pipe_reference_init(&sbuf->b.reference, 1);
   sbuf->b.screen = screen;
   sbuf->b.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   sbuf->b.usage = PIPE_USAGE_IMMUTABLE;
   sbuf->b.bind = bind;
   sbuf->b.width0 = bytes;
   sbuf->b.height0 = 1;
   sbuf->b.depth0 = 1;
            sbuf->bind_flags = bind;
   sbuf->swbuf = ptr;
            debug_reference(&sbuf->b.reference,
                           no_sbuf:
         }
