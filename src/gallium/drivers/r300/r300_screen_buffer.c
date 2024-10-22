   /*
   * Copyright 2010 Red Hat Inc.
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
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * Authors: Dave Airlie
   */
      #include <stdio.h>
      #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_math.h"
      #include "r300_screen_buffer.h"
      void r300_upload_index_buffer(struct r300_context *r300,
            struct pipe_resource **index_buffer,
      {
                        u_upload_data(r300->uploader,
                  0, count * index_size, 4,
            }
      void r300_resource_destroy(struct pipe_screen *screen,
         {
      if (buf->target == PIPE_BUFFER) {
                        if (rbuf->buf)
               } else {
      struct r300_screen *rscreen = r300_screen(screen);
            if (tex->tex.cmask_dwords) {
      mtx_lock(&rscreen->cmask_mutex);
   if (buf == rscreen->cmask_resource) {
         }
      }
   pb_reference(&tex->buf, NULL);
         }
      void *
   r300_buffer_transfer_map( struct pipe_context *context,
                           struct pipe_resource *resource,
   {
      struct r300_context *r300 = r300_context(context);
   struct radeon_winsys *rws = r300->screen->rws;
   struct r300_resource *rbuf = r300_resource(resource);
   struct pipe_transfer *transfer;
            transfer = slab_alloc(&r300->pool_transfers);
   transfer->resource = resource;
   transfer->level = level;
   transfer->usage = usage;
   transfer->box = *box;
   transfer->stride = 0;
            if (rbuf->malloced_buffer) {
      *ptransfer = transfer;
               if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE &&
      !(usage & PIPE_MAP_UNSYNCHRONIZED)) {
            /* Check if mapping this buffer would cause waiting for the GPU. */
   if (r300->rws->cs_is_buffer_referenced(&r300->cs, rbuf->buf, RADEON_USAGE_READWRITE) ||
         !r300->rws->buffer_wait(r300->rws, rbuf->buf, 0, RADEON_USAGE_READWRITE)) {
                  /* Create a new one in the same pipe_resource. */
   new_buf = r300->rws->buffer_create(r300->rws, rbuf->b.width0,
                     if (new_buf) {
                           /* We changed the buffer, now we need to bind it where the old one was bound. */
   for (i = 0; i < r300->nr_vertex_buffers; i++) {
      if (r300->vertex_buffer[i].buffer.resource == &rbuf->b) {
         r300->vertex_arrays_dirty = true;
                     /* Buffers are never used for write, therefore mapping for read can be
   * unsynchronized. */
   if (!(usage & PIPE_MAP_WRITE)) {
                           if (!map) {
      slab_free(&r300->pool_transfers, transfer);
               *ptransfer = transfer;
      }
      void r300_buffer_transfer_unmap( struct pipe_context *pipe,
         {
                  }
      struct pipe_resource *r300_buffer_create(struct pipe_screen *screen,
         {
      struct r300_screen *r300screen = r300_screen(screen);
                     rbuf->b = *templ;
   pipe_reference_init(&rbuf->b.reference, 1);
   rbuf->b.screen = screen;
   rbuf->domain = RADEON_DOMAIN_GTT;
   rbuf->buf = NULL;
            /* Allocate constant buffers and SWTCL vertex and index buffers in RAM.
   * Note that uploaded index buffers use the flag PIPE_BIND_CUSTOM, so that
   * we can distinguish them from user-created buffers.
   */
   if (templ->bind & PIPE_BIND_CONSTANT_BUFFER ||
      (!r300screen->caps.has_tcl && !(templ->bind & PIPE_BIND_CUSTOM))) {
   rbuf->malloced_buffer = align_malloc(templ->width0, 64);
               rbuf->buf =
      r300screen->rws->buffer_create(r300screen->rws, rbuf->b.width0,
                  if (!rbuf->buf) {
      FREE(rbuf);
      }
      }
