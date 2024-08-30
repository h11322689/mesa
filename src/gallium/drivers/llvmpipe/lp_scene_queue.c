   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
         /**
   * Scene queue.  We'll use two queues.  One contains "full" scenes which
   * are produced by the "setup" code.  The other contains "empty" scenes
   * which are produced by the "rast" code when it finishes rendering a scene.
   */
      #include "util/u_thread.h"
   #include "util/u_memory.h"
   #include "lp_scene_queue.h"
   #include "util/u_math.h"
   #include "lp_setup_context.h"
         #define SCENE_QUEUE_SIZE MAX_SCENES
            /**
   * A queue of scenes
   */
   struct lp_scene_queue
   {
               mtx_t mutex;
            /* These values wrap around, so that head == tail means empty.  When used
   * to index the array, we use them modulo the queue size.  This scheme
   * works because the queue size is a power of two.
   */
   unsigned head;
      };
            /** Allocate a new scene queue */
   struct lp_scene_queue *
   lp_scene_queue_create(void)
   {
      /* Circular queue behavior depends on size being a power of two. */
   STATIC_ASSERT(SCENE_QUEUE_SIZE > 0);
                     if (!queue)
            (void) mtx_init(&queue->mutex, mtx_plain);
               }
         /** Delete a scene queue */
   void
   lp_scene_queue_destroy(struct lp_scene_queue *queue)
   {
      cnd_destroy(&queue->change);
   mtx_destroy(&queue->mutex);
      }
         /** Remove first lp_scene from head of queue */
   struct lp_scene *
   lp_scene_dequeue(struct lp_scene_queue *queue, bool wait)
   {
               if (wait) {
      /* Wait for queue to be not empty. */
   while (queue->head == queue->tail)
      } else {
      if (queue->head == queue->tail) {
      mtx_unlock(&queue->mutex);
                           cnd_signal(&queue->change);
               }
         /** Add an lp_scene to tail of queue */
   void
   lp_scene_enqueue(struct lp_scene_queue *queue, struct lp_scene *scene)
   {
               /* Wait for free space. */
   while (queue->tail - queue->head >= SCENE_QUEUE_SIZE)
                     cnd_signal(&queue->change);
      }
