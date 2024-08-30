   /*
   * Copyright (C) 2020 Collabora Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "util/u_vbuf.h"
   #include "pan_context.h"
      void
   panfrost_analyze_sysvals(struct panfrost_compiled_shader *ss)
   {
      unsigned dirty = 0;
            for (unsigned i = 0; i < ss->sysvals.sysval_count; ++i) {
      switch (PAN_SYSVAL_TYPE(ss->sysvals.sysvals[i])) {
   case PAN_SYSVAL_VIEWPORT_SCALE:
   case PAN_SYSVAL_VIEWPORT_OFFSET:
                  case PAN_SYSVAL_TEXTURE_SIZE:
                  case PAN_SYSVAL_SSBO:
                  case PAN_SYSVAL_XFB:
                  case PAN_SYSVAL_SAMPLER:
                  case PAN_SYSVAL_IMAGE_SIZE:
                  case PAN_SYSVAL_NUM_WORK_GROUPS:
   case PAN_SYSVAL_LOCAL_GROUP_SIZE:
   case PAN_SYSVAL_WORK_DIM:
   case PAN_SYSVAL_VERTEX_INSTANCE_OFFSETS:
   case PAN_SYSVAL_NUM_VERTICES:
                  case PAN_SYSVAL_DRAWID:
                  case PAN_SYSVAL_SAMPLE_POSITIONS:
   case PAN_SYSVAL_MULTISAMPLED:
   case PAN_SYSVAL_RT_CONVERSION:
      /* Nothing beyond the batch itself */
      default:
                     ss->dirty_3d = dirty;
      }
      /*
   * Gets a GPU address for the associated index buffer. Only gauranteed to be
   * good for the duration of the draw (transient), could last longer. Bounds are
   * not calculated.
   */
   mali_ptr
   panfrost_get_index_buffer(struct panfrost_batch *batch,
               {
      struct panfrost_resource *rsrc = pan_resource(info->index.resource);
            if (!info->has_user_indices) {
      /* Only resources can be directly mapped */
   panfrost_batch_read_rsrc(batch, rsrc, PIPE_SHADER_VERTEX);
      } else {
      /* Otherwise, we need to upload to transient memory */
   const uint8_t *ibuf8 = (const uint8_t *)info->index.user;
   struct panfrost_ptr T = pan_pool_alloc_aligned(
            memcpy(T.cpu, ibuf8 + offset, draw->count * info->index_size);
         }
      /* Gets a GPU address for the associated index buffer. Only gauranteed to be
   * good for the duration of the draw (transient), could last longer. Also get
   * the bounds on the index buffer for the range accessed by the draw. We do
   * these operations together because there are natural optimizations which
   * require them to be together. */
      mali_ptr
   panfrost_get_index_buffer_bounded(struct panfrost_batch *batch,
                     {
      struct panfrost_resource *rsrc = pan_resource(info->index.resource);
   struct panfrost_context *ctx = batch->ctx;
            /* Note: if index_bounds_valid is set but the bounds are wrong, page faults
   * (at least on Mali-G52) can be triggered an underflow reading varyings.
   * Providing invalid index bounds in GLES is implementation-defined
   * behaviour. This should be fine for now but this needs to be revisited when
   * wiring up robustness later.
   */
   if (info->index_bounds_valid) {
      *min_index = info->min_index;
   *max_index = info->max_index;
      } else if (!info->has_user_indices) {
      /* Check the cache */
   needs_indices = !panfrost_minmax_cache_get(
               if (needs_indices) {
      /* Fallback */
            if (!info->has_user_indices)
      panfrost_minmax_cache_add(rsrc->index_cache, draw->start, draw->count,
               }
      /**
   * Given an (index, divisor) tuple, assign a vertex buffer. Midgard and
   * Bifrost put divisor information on the attribute buffer descriptor, so this
   * is the most we can compact in general. Crucially, this runs at vertex
   * elements CSO create time, not at draw time.
   */
   unsigned
   pan_assign_vertex_buffer(struct pan_vertex_buffer *buffers, unsigned *nr_bufs,
         {
      /* Look up the buffer */
   for (unsigned i = 0; i < (*nr_bufs); ++i) {
      if (buffers[i].vbi == vbi && buffers[i].divisor == divisor)
               /* Else, create a new buffer */
            buffers[idx] = (struct pan_vertex_buffer){
      .vbi = vbi,
                  }
      /*
   * Helper to add a PIPE_CLEAR_* to batch->draws and batch->resolve together,
   * meaning that we draw to a given target. Adding to only one mask does not
   * generally make sense, except for clears which add to batch->clear and
   * batch->resolve together.
   */
   static void
   panfrost_draw_target(struct panfrost_batch *batch, unsigned target)
   {
      batch->draws |= target;
      }
      /*
   * Draw time helper to set batch->{read, draws, resolve} based on current blend
   * and depth-stencil state. To be called when blend or depth/stencil dirty state
   * respectively changes.
   */
   void
   panfrost_set_batch_masks_blend(struct panfrost_batch *batch)
   {
      struct panfrost_context *ctx = batch->ctx;
            for (unsigned i = 0; i < batch->key.nr_cbufs; ++i) {
      if (blend->info[i].enabled && batch->key.cbufs[i])
         }
      void
   panfrost_set_batch_masks_zs(struct panfrost_batch *batch)
   {
      struct panfrost_context *ctx = batch->ctx;
            /* Assume depth is read (TODO: perf) */
   if (zsa->depth_enabled)
            if (zsa->depth_writemask)
            if (zsa->stencil[0].enabled) {
               /* Assume stencil is read (TODO: perf) */
         }
      void
   panfrost_track_image_access(struct panfrost_batch *batch,
               {
               if (image->shader_access & PIPE_IMAGE_ACCESS_WRITE) {
               bool is_buffer = rsrc->base.target == PIPE_BUFFER;
   unsigned level = is_buffer ? 0 : image->u.tex.level;
            if (is_buffer) {
      util_range_add(&rsrc->base, &rsrc->valid_buffer_range, 0,
         } else {
            }
