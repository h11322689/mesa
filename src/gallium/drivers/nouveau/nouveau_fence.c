   /*
   * Copyright 2010 Christoph Bumiller
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "nouveau_screen.h"
   #include "nouveau_context.h"
   #include "nouveau_winsys.h"
   #include "nouveau_fence.h"
   #include "util/os_time.h"
      #if DETECT_OS_UNIX
   #include <sched.h>
   #endif
      static bool
   _nouveau_fence_wait(struct nouveau_fence *fence, struct util_debug_callback *debug);
      bool
   nouveau_fence_new(struct nouveau_context *nv, struct nouveau_fence **fence)
   {
      *fence = CALLOC_STRUCT(nouveau_fence);
   if (!*fence)
            int ret = nouveau_bo_new(nv->screen->device, NOUVEAU_BO_GART, 0x1000, 0x1000, NULL, &(*fence)->bo);
   if (ret) {
      FREE(*fence);
               (*fence)->screen = nv->screen;
   (*fence)->context = nv;
   (*fence)->ref = 1;
               }
      static void
   nouveau_fence_trigger_work(struct nouveau_fence *fence)
   {
                        LIST_FOR_EACH_ENTRY_SAFE(work, tmp, &fence->work, list) {
      work->func(work->data);
   list_del(&work->list);
         }
      static void
   _nouveau_fence_emit(struct nouveau_fence *fence)
   {
      struct nouveau_screen *screen = fence->screen;
                     assert(fence->state != NOUVEAU_FENCE_STATE_EMITTING);
   if (fence->state >= NOUVEAU_FENCE_STATE_EMITTED)
            /* set this now, so that if fence.emit triggers a flush we don't recurse */
                     if (fence_list->tail)
         else
                              assert(fence->state == NOUVEAU_FENCE_STATE_EMITTING);
      }
      static void
   nouveau_fence_del(struct nouveau_fence *fence)
   {
      struct nouveau_fence *it;
                     if (fence->state == NOUVEAU_FENCE_STATE_EMITTED ||
      fence->state == NOUVEAU_FENCE_STATE_FLUSHED) {
   if (fence == fence_list->head) {
      fence_list->head = fence->next;
   if (!fence_list->head)
      } else {
      for (it = fence_list->head; it && it->next != fence; it = it->next);
   it->next = fence->next;
   if (fence_list->tail == fence)
                  if (!list_is_empty(&fence->work)) {
      debug_printf("WARNING: deleting fence with work still pending !\n");
               nouveau_bo_ref(NULL, &fence->bo);
      }
      void
   nouveau_fence_cleanup(struct nouveau_context *nv)
   {
      if (nv->fence) {
      struct nouveau_fence_list *fence_list = &nv->screen->fence;
            /* nouveau_fence_wait will create a new current fence, so wait on the
   * _current_ one, and remove both.
   */
   simple_mtx_lock(&fence_list->lock);
   _nouveau_fence_ref(nv->fence, &current);
   _nouveau_fence_wait(current, NULL);
   _nouveau_fence_ref(NULL, &current);
   _nouveau_fence_ref(NULL, &nv->fence);
         }
      void
   _nouveau_fence_update(struct nouveau_screen *screen, bool flushed)
   {
      struct nouveau_fence *fence;
   struct nouveau_fence *next = NULL;
   struct nouveau_fence_list *fence_list = &screen->fence;
                     /* If running under drm-shim, let all fences be signalled so things run to
   * completion (avoids a hang at the end of shader-db).
   */
   if (unlikely(screen->disable_fences))
            if (fence_list->sequence_ack == sequence)
                  for (fence = fence_list->head; fence; fence = next) {
      next = fence->next;
                     nouveau_fence_trigger_work(fence);
            if (sequence == fence_list->sequence_ack)
      }
   fence_list->head = next;
   if (!next)
            if (flushed) {
      for (fence = next; fence; fence = fence->next)
      if (fence->state == NOUVEAU_FENCE_STATE_EMITTED)
      }
      #define NOUVEAU_FENCE_MAX_SPINS (1 << 31)
      static bool
   _nouveau_fence_signalled(struct nouveau_fence *fence)
   {
                        if (fence->state == NOUVEAU_FENCE_STATE_SIGNALLED)
            if (fence->state >= NOUVEAU_FENCE_STATE_EMITTED)
               }
      static bool
   nouveau_fence_kick(struct nouveau_fence *fence)
   {
      struct nouveau_context *context = fence->context;
   struct nouveau_screen *screen = fence->screen;
   struct nouveau_fence_list *fence_list = &screen->fence;
                     /* wtf, someone is waiting on a fence in flush_notify handler? */
            if (fence->state < NOUVEAU_FENCE_STATE_EMITTED) {
      if (PUSH_AVAIL(context->pushbuf) < 16)
                     if (fence->state < NOUVEAU_FENCE_STATE_FLUSHED) {
      if (nouveau_pushbuf_kick(context->pushbuf, context->pushbuf->channel))
               if (current)
                        }
      static bool
   _nouveau_fence_wait(struct nouveau_fence *fence, struct util_debug_callback *debug)
   {
      struct nouveau_screen *screen = fence->screen;
   struct nouveau_fence_list *fence_list = &screen->fence;
                     if (debug && debug->debug_message)
            if (!nouveau_fence_kick(fence))
            if (fence->state < NOUVEAU_FENCE_STATE_SIGNALLED) {
      NOUVEAU_DRV_STAT(screen, any_non_kernel_fence_sync_count, 1);
   int ret = nouveau_bo_wait(fence->bo, NOUVEAU_BO_RDWR, screen->client);
   if (ret) {
      debug_printf("Wait on fence %u (ack = %u, next = %u) errored with %s !\n",
                           _nouveau_fence_update(screen, false);
   if (fence->state != NOUVEAU_FENCE_STATE_SIGNALLED)
            if (debug && debug->debug_message)
      util_debug_message(debug, PERF_INFO,
                     }
      void
   _nouveau_fence_next(struct nouveau_context *nv)
   {
                        if (nv->fence->state < NOUVEAU_FENCE_STATE_EMITTING) {
      if (p_atomic_read(&nv->fence->ref) > 1)
         else
                           }
      void
   nouveau_fence_unref_bo(void *data)
   {
                  }
      bool
   nouveau_fence_work(struct nouveau_fence *fence,
         {
      struct nouveau_fence_work *work;
            if (!fence || fence->state == NOUVEAU_FENCE_STATE_SIGNALLED) {
      func(data);
               work = CALLOC_STRUCT(nouveau_fence_work);
   if (!work)
         work->func = func;
            /* the fence might get deleted by fence_kick */
            simple_mtx_lock(&screen->fence.lock);
   list_add(&work->list, &fence->work);
   if (++fence->work_count > 64)
         simple_mtx_unlock(&screen->fence.lock);
      }
      void
   _nouveau_fence_ref(struct nouveau_fence *fence, struct nouveau_fence **ref)
   {
      if (fence)
            if (*ref) {
      simple_mtx_assert_locked(&(*ref)->screen->fence.lock);
   if (p_atomic_dec_zero(&(*ref)->ref))
                  }
      void
   nouveau_fence_ref(struct nouveau_fence *fence, struct nouveau_fence **ref)
   {
      struct nouveau_fence_list *fence_list = NULL;
   if (ref && *ref)
            if (fence_list)
                     if (fence_list)
      }
      bool
   nouveau_fence_wait(struct nouveau_fence *fence, struct util_debug_callback *debug)
   {
      struct nouveau_fence_list *fence_list = &fence->screen->fence;
   simple_mtx_lock(&fence_list->lock);
   bool res = _nouveau_fence_wait(fence, debug);
   simple_mtx_unlock(&fence_list->lock);
      }
      void
   nouveau_fence_next_if_current(struct nouveau_context *nv, struct nouveau_fence *fence)
   {
      simple_mtx_lock(&fence->screen->fence.lock);
   if (nv->fence == fence)
            }
      bool
   nouveau_fence_signalled(struct nouveau_fence *fence)
   {
      simple_mtx_lock(&fence->screen->fence.lock);
   bool ret = _nouveau_fence_signalled(fence);
   simple_mtx_unlock(&fence->screen->fence.lock);
      }
