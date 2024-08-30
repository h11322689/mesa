   /*
   * Copyright 2018 Chromium.
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
   */
      #include "util/u_box.h"
   #include "util/u_inlines.h"
      #include "virtio-gpu/virgl_protocol.h"
   #include "virgl_context.h"
   #include "virgl_screen.h"
   #include "virgl_encode.h"
   #include "virgl_resource.h"
   #include "virgl_transfer_queue.h"
      struct list_action_args
   {
      void *data;
   struct virgl_transfer *queued;
      };
      typedef bool (*compare_transfers_t)(struct virgl_transfer *queued,
            typedef void (*list_action_t)(struct virgl_transfer_queue *queue,
            struct list_iteration_args
   {
      void *data;
   list_action_t action;
   compare_transfers_t compare;
      };
      static int
   transfer_dim(const struct virgl_transfer *xfer)
   {
      switch (xfer->base.resource->target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
         case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
         default:
            }
      static void
   box_min_max(const struct pipe_box *box, int dim, int *min, int *max)
   {
      switch (dim) {
   case 0:
      if (box->width > 0) {
      *min = box->x;
      } else {
      *max = box->x;
      }
      case 1:
      if (box->height > 0) {
      *min = box->y;
      } else {
      *max = box->y;
      }
      default:
      if (box->depth > 0) {
      *min = box->z;
      } else {
      *max = box->z;
      }
         }
      static bool
   transfer_overlap(const struct virgl_transfer *xfer,
                  const struct virgl_hw_res *hw_res,
      {
               if (xfer->hw_res != hw_res || xfer->base.level != level)
            for (int dim = 0; dim < dim_count; dim++) {
      int xfer_min;
   int xfer_max;
   int box_min;
            box_min_max(&xfer->base.box, dim, &xfer_min, &xfer_max);
            if (include_touching) {
      /* touching is considered overlapping */
   if (xfer_min > box_max || xfer_max < box_min)
      } else {
      /* touching is not considered overlapping */
   if (xfer_min >= box_max || xfer_max <= box_min)
                     }
      static struct virgl_transfer *
   virgl_transfer_queue_find_overlap(const struct virgl_transfer_queue *queue,
                           {
      struct virgl_transfer *xfer;
   LIST_FOR_EACH_ENTRY(xfer, &queue->transfer_list, queue_link) {
      if (transfer_overlap(xfer, hw_res, level, box, include_touching))
                  }
      static bool transfers_intersect(struct virgl_transfer *queued,
         {
      return transfer_overlap(queued, current->hw_res, current->base.level,
      }
      static void remove_transfer(struct virgl_transfer_queue *queue,
         {
      list_del(&queued->queue_link);
      }
      static void replace_unmapped_transfer(struct virgl_transfer_queue *queue,
         {
      struct virgl_transfer *current = args->current;
            u_box_union_2d(&current->base.box, &current->base.box, &queued->base.box);
            remove_transfer(queue, queued);
      }
      static void transfer_put(struct virgl_transfer_queue *queue,
         {
               queue->vs->vws->transfer_put(queue->vs->vws, queued->hw_res,
                           }
      static void transfer_write(struct virgl_transfer_queue *queue,
         {
      struct virgl_transfer *queued = args->queued;
            // Takes a reference on the HW resource, which is released after
   // the exec buffer command.
               }
      static void compare_and_perform_action(struct virgl_transfer_queue *queue,
         {
      struct list_action_args args;
            memset(&args, 0, sizeof(args));
   args.current = iter->current;
            LIST_FOR_EACH_ENTRY_SAFE(queued, tmp, &queue->transfer_list, queue_link) {
      if (iter->compare(queued, iter->current)) {
      args.queued = queued;
            }
      static void perform_action(struct virgl_transfer_queue *queue,
         {
      struct list_action_args args;
            memset(&args, 0, sizeof(args));
            LIST_FOR_EACH_ENTRY_SAFE(queued, tmp, &queue->transfer_list, queue_link) {
      args.queued = queued;
         }
      static void add_internal(struct virgl_transfer_queue *queue,
         {
      uint32_t dwords = VIRGL_TRANSFER3D_SIZE + 1;
   if (queue->tbuf) {
      if (queue->num_dwords + dwords >= VIRGL_MAX_TBUF_DWORDS) {
                     memset(&iter, 0, sizeof(iter));
   iter.action = transfer_write;
                  vws->submit_cmd(vws, queue->tbuf, NULL);
                  list_addtail(&transfer->queue_link, &queue->transfer_list);
      }
      void virgl_transfer_queue_init(struct virgl_transfer_queue *queue,
         {
               queue->vs = vs;
   queue->vctx = vctx;
                     if ((vs->caps.caps.v2.capability_bits & VIRGL_CAP_TRANSFER) &&
      vs->vws->supports_encoded_transfers)
      else
      }
      void virgl_transfer_queue_fini(struct virgl_transfer_queue *queue)
   {
      struct virgl_winsys *vws = queue->vs->vws;
                     iter.action = transfer_put;
            if (queue->tbuf)
            queue->vs = NULL;
   queue->vctx = NULL;
   queue->tbuf = NULL;
      }
      int virgl_transfer_queue_unmap(struct virgl_transfer_queue *queue,
         {
               /* We don't support copy transfers in the transfer queue. */
            /* Attempt to merge multiple intersecting transfers into a single one. */
   if (transfer->base.resource->target == PIPE_BUFFER) {
      memset(&iter, 0, sizeof(iter));
   iter.current = transfer;
   iter.compare = transfers_intersect;
   iter.action = replace_unmapped_transfer;
               add_internal(queue, transfer);
      }
      int virgl_transfer_queue_clear(struct virgl_transfer_queue *queue,
         {
               memset(&iter, 0, sizeof(iter));
   if (queue->tbuf) {
      uint32_t prior_num_dwords = cbuf->cdw;
            iter.action = transfer_write;
   iter.data = cbuf;
            virgl_encode_end_transfers(cbuf);
      } else {
      iter.action = transfer_put;
                           }
      bool virgl_transfer_queue_is_queued(struct virgl_transfer_queue *queue,
         {
      return virgl_transfer_queue_find_overlap(queue,
                        }
      bool
   virgl_transfer_queue_extend_buffer(struct virgl_transfer_queue *queue,
                     {
      struct virgl_transfer *queued;
            u_box_1d(offset, size, &box);
   queued = virgl_transfer_queue_find_overlap(queue, hw_res, 0, &box, true);
   if (!queued)
            assert(queued->base.resource->target == PIPE_BUFFER);
            memcpy(queued->hw_res_map + offset, data, size);
   u_box_union_2d(&queued->base.box, &queued->base.box, &box);
               }
