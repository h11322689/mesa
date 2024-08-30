   /*
   * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
   * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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
      #include "r300_transfer.h"
   #include "r300_texture_desc.h"
   #include "r300_screen_buffer.h"
      #include "util/u_memory.h"
   #include "util/format/u_format.h"
   #include "util/u_box.h"
      struct r300_transfer {
      /* Parent class */
            /* Linear texture. */
      };
      /* Convenience cast wrapper. */
   static inline struct r300_transfer*
   r300_transfer(struct pipe_transfer* transfer)
   {
         }
      /* Copy from a tiled texture to a detiled one. */
   static void r300_copy_from_tiled_texture(struct pipe_context *ctx,
         {
      struct pipe_transfer *transfer = (struct pipe_transfer*)r300transfer;
   struct pipe_resource *src = transfer->resource;
            if (src->nr_samples <= 1) {
      ctx->resource_copy_region(ctx, dst, 0, 0, 0, 0,
      } else {
      /* Resolve the resource. */
            memset(&blit, 0, sizeof(blit));
   blit.src.resource = src;
   blit.src.format = src->format;
   blit.src.level = transfer->level;
   blit.src.box = transfer->box;
   blit.dst.resource = dst;
   blit.dst.format = dst->format;
   blit.dst.box.width = transfer->box.width;
   blit.dst.box.height = transfer->box.height;
   blit.dst.box.depth = transfer->box.depth;
   blit.mask = PIPE_MASK_RGBA;
                  }
      /* Copy a detiled texture to a tiled one. */
   static void r300_copy_into_tiled_texture(struct pipe_context *ctx,
         {
      struct pipe_transfer *transfer = (struct pipe_transfer*)r300transfer;
   struct pipe_resource *tex = transfer->resource;
            u_box_3d(0, 0, 0,
                  ctx->resource_copy_region(ctx, tex, transfer->level,
                  /* XXX remove this. */
      }
      void *
   r300_texture_transfer_map(struct pipe_context *ctx,
                           struct pipe_resource *texture,
   {
      struct r300_context *r300 = r300_context(ctx);
   struct r300_resource *tex = r300_resource(texture);
   struct r300_transfer *trans;
   bool referenced_cs, referenced_hw;
   enum pipe_format format = tex->b.format;
            referenced_cs =
         if (referenced_cs) {
         } else {
      referenced_hw =
               trans = CALLOC_STRUCT(r300_transfer);
   if (trans) {
      /* Initialize the transfer object. */
   trans->transfer.resource = texture;
   trans->transfer.level = level;
   trans->transfer.usage = usage;
            /* If the texture is tiled, we must create a temporary detiled texture
      * for this transfer.
      if (tex->tex.microtile || tex->tex.macrotile[level] ||
         (referenced_hw && !(usage & PIPE_MAP_READ) &&
                  if (r300->blitter->running) {
                        memset(&base, 0, sizeof(base));
   base.target = PIPE_TEXTURE_2D;
   base.format = texture->format;
   base.width0 = box->width;
   base.height0 = box->height;
   base.depth0 = 1;
   base.array_size = 1;
                  /* We must set the correct texture target and dimensions if needed for a 3D transfer. */
                     if (base.target == PIPE_TEXTURE_3D) {
                     /* Create the temporary texture. */
   trans->linear_texture = r300_resource(
                  if (!trans->linear_texture) {
                                                if (!trans->linear_texture) {
      fprintf(stderr,
         FREE(trans);
                                 /* Set the stride. */
   trans->transfer.stride =
                        if (usage & PIPE_MAP_READ) {
                           /* Always referenced in the blit. */
      } else {
         /* Unpipelined transfer. */
   trans->transfer.stride = tex->tex.stride_in_bytes[level];
                  if (referenced_cs &&
      !(usage & PIPE_MAP_UNSYNCHRONIZED)) {
                  if (trans->linear_texture) {
      /* The detiled texture is of the same size as the region being mapped
         map = r300->rws->buffer_map(r300->rws, trans->linear_texture->buf,
         if (!map) {
         pipe_resource_reference(
         FREE(trans);
   *transfer = &trans->transfer;
            } else {
      /* Tiling is disabled. */
   map = r300->rws->buffer_map(r300->rws, tex->buf, &r300->cs, usage);
   if (!map) {
         FREE(trans);
      *transfer = &trans->transfer;
         return map + trans->transfer.offset +
               }
      void r300_texture_transfer_unmap(struct pipe_context *ctx,
         {
               if (trans->linear_texture) {
      if (transfer->usage & PIPE_MAP_WRITE) {
                  pipe_resource_reference(
      }
      }
