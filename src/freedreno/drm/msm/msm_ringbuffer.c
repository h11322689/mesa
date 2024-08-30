   /*
   * Copyright (C) 2012-2018 Rob Clark <robclark@freedesktop.org>
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
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include <assert.h>
   #include <inttypes.h>
      #include "util/hash_table.h"
   #include "util/set.h"
   #include "util/slab.h"
      #include "drm/freedreno_ringbuffer.h"
   #include "msm_priv.h"
      /* The legacy implementation of submit/ringbuffer, which still does the
   * traditional reloc and cmd tracking
   */
      #define INIT_SIZE 0x1000
      struct msm_submit {
               DECLARE_ARRAY(struct drm_msm_gem_submit_bo, submit_bos);
            /* maps fd_bo to idx in bos table: */
                     /* hash-set of associated rings: */
            /* Allow for sub-allocation of stateobj ring buffers (ie. sharing
   * the same underlying bo)..
   *
   * We also rely on previous stateobj having been fully constructed
   * so we can reclaim extra space at it's end.
   */
      };
   FD_DEFINE_CAST(fd_submit, msm_submit);
      /* for FD_RINGBUFFER_GROWABLE rb's, tracks the 'finalized' cmdstream buffers
   * and sizes.  Ie. a finalized buffer can have no more commands appended to
   * it.
   */
   struct msm_cmd {
      struct fd_bo *ring_bo;
   unsigned size;
      };
      static struct msm_cmd *
   cmd_new(struct fd_bo *ring_bo)
   {
      struct msm_cmd *cmd = malloc(sizeof(*cmd));
   cmd->ring_bo = fd_bo_ref(ring_bo);
   cmd->size = 0;
   cmd->nr_relocs = cmd->max_relocs = 0;
   cmd->relocs = NULL;
      }
      static void
   cmd_free(struct msm_cmd *cmd)
   {
      fd_bo_del(cmd->ring_bo);
   free(cmd->relocs);
      }
      struct msm_ringbuffer {
               /* for FD_RINGBUFFER_STREAMING rb's which are sub-allocated */
            union {
      /* for _FD_RINGBUFFER_OBJECT case: */
   struct {
      struct fd_pipe *pipe;
   DECLARE_ARRAY(struct fd_bo *, reloc_bos);
      };
   /* for other cases: */
   struct {
      struct fd_submit *submit;
                  struct msm_cmd *cmd; /* current cmd */
      };
   FD_DEFINE_CAST(fd_ringbuffer, msm_ringbuffer);
      static void finalize_current_cmd(struct fd_ringbuffer *ring);
   static struct fd_ringbuffer *
   msm_ringbuffer_init(struct msm_ringbuffer *msm_ring, uint32_t size,
            /* add (if needed) bo to submit and return index: */
   static uint32_t
   append_bo(struct msm_submit *submit, struct fd_bo *bo)
   {
               /* NOTE: it is legal to use the same bo on different threads for
   * different submits.  But it is not legal to use the same submit
   * from given threads.
   */
            if (unlikely((idx >= submit->nr_submit_bos) ||
            uint32_t hash = _mesa_hash_pointer(bo);
            entry = _mesa_hash_table_search_pre_hashed(submit->bo_table, hash, bo);
   if (entry) {
      /* found */
      } else {
      idx = APPEND(
      submit, submit_bos,
   (struct drm_msm_gem_submit_bo){
      .flags = bo->reloc_flags & (MSM_SUBMIT_BO_READ | MSM_SUBMIT_BO_WRITE),
   .handle = bo->handle,
                  _mesa_hash_table_insert_pre_hashed(submit->bo_table, hash, bo,
      }
                  }
      static void
   append_ring(struct set *set, struct fd_ringbuffer *ring)
   {
               if (!_mesa_set_search_pre_hashed(set, hash, ring)) {
      fd_ringbuffer_ref(ring);
         }
      static void
   msm_submit_suballoc_ring_bo(struct fd_submit *submit,
         {
      struct msm_submit *msm_submit = to_msm_submit(submit);
   unsigned suballoc_offset = 0;
            if (msm_submit->suballoc_ring) {
      struct msm_ringbuffer *suballoc_ring =
            suballoc_bo = suballoc_ring->ring_bo;
   suballoc_offset =
                     if ((size + suballoc_offset) > suballoc_bo->size) {
                     if (!suballoc_bo) {
      // TODO possibly larger size for streaming bo?
   msm_ring->ring_bo = fd_bo_new_ring(submit->pipe->dev, 0x8000);
      } else {
      msm_ring->ring_bo = fd_bo_ref(suballoc_bo);
                                 if (old_suballoc_ring)
      }
      static struct fd_ringbuffer *
   msm_submit_new_ringbuffer(struct fd_submit *submit, uint32_t size,
         {
      struct msm_submit *msm_submit = to_msm_submit(submit);
                              /* NOTE: needs to be before _suballoc_ring_bo() since it could
   * increment the refcnt of the current ring
   */
            if (flags & FD_RINGBUFFER_STREAMING) {
         } else {
      if (flags & FD_RINGBUFFER_GROWABLE)
            msm_ring->offset = 0;
               if (!msm_ringbuffer_init(msm_ring, size, flags))
               }
      static struct drm_msm_gem_submit_reloc *
   handle_stateobj_relocs(struct msm_submit *submit, struct msm_ringbuffer *ring)
   {
      struct msm_cmd *cmd = ring->cmd;
                     for (unsigned i = 0; i < cmd->nr_relocs; i++) {
      unsigned idx = cmd->relocs[i].reloc_idx;
            relocs[i] = cmd->relocs[i];
                  }
      static struct fd_fence *
   msm_submit_flush(struct fd_submit *submit, int in_fence_fd, bool use_fence_fd)
   {
      struct msm_submit *msm_submit = to_msm_submit(submit);
   struct msm_pipe *msm_pipe = to_msm_pipe(submit->pipe);
   struct drm_msm_gem_submit req = {
      .flags = msm_pipe->pipe,
      };
            finalize_current_cmd(submit->primary);
            unsigned nr_cmds = 0;
            set_foreach (msm_submit->ring_set, entry) {
      struct fd_ringbuffer *ring = (void *)entry->key;
   if (ring->flags & _FD_RINGBUFFER_OBJECT) {
      nr_cmds += 1;
      } else {
      if (ring != submit->primary)
                        void *obj_relocs[nr_objs];
   struct drm_msm_gem_submit_cmd cmds[nr_cmds];
            set_foreach (msm_submit->ring_set, entry) {
      struct fd_ringbuffer *ring = (void *)entry->key;
                     // TODO handle relocs:
                                       cmds[i].type = MSM_SUBMIT_CMD_IB_TARGET_BUF;
   cmds[i].submit_idx = append_bo(msm_submit, msm_ring->ring_bo);
   cmds[i].submit_offset = submit_offset(msm_ring->ring_bo, msm_ring->offset);
   cmds[i].size = offset_bytes(ring->cur, ring->start);
   cmds[i].pad = 0;
                     } else {
      for (unsigned j = 0; j < msm_ring->u.nr_cmds; j++) {
      if (ring->flags & FD_RINGBUFFER_PRIMARY) {
         } else {
         }
   struct fd_bo *ring_bo = msm_ring->u.cmds[j]->ring_bo;
   cmds[i].submit_idx = append_bo(msm_submit, ring_bo);
   cmds[i].submit_offset = submit_offset(ring_bo, msm_ring->offset);
   cmds[i].size = msm_ring->u.cmds[j]->size;
   cmds[i].pad = 0;
                                             simple_mtx_lock(&fence_lock);
   for (unsigned j = 0; j < msm_submit->nr_bos; j++) {
         }
            if (in_fence_fd != -1) {
      req.flags |= MSM_SUBMIT_FENCE_FD_IN | MSM_SUBMIT_NO_IMPLICIT;
               if (out_fence->use_fence_fd) {
                  /* needs to be after get_cmd() as that could create bos/cmds table: */
   req.bos = VOID2U64(msm_submit->submit_bos),
   req.nr_bos = msm_submit->nr_submit_bos;
                     ret = drmCommandWriteRead(submit->pipe->dev->fd, DRM_MSM_GEM_SUBMIT, &req,
         if (ret) {
      ERROR_MSG("submit failed: %d (%s)", ret, strerror(errno));
   fd_fence_del(out_fence);
   out_fence = NULL;
      } else if (!ret && out_fence) {
      out_fence->kfence = req.fence;
   out_fence->ufence = submit->fence;
               for (unsigned o = 0; o < nr_objs; o++)
               }
      static void
   unref_rings(struct set_entry *entry)
   {
      struct fd_ringbuffer *ring = (void *)entry->key;
      }
      static void
   msm_submit_destroy(struct fd_submit *submit)
   {
               if (msm_submit->suballoc_ring)
            _mesa_hash_table_destroy(msm_submit->bo_table, NULL);
            // TODO it would be nice to have a way to assert() if all
   // rb's haven't been free'd back to the slab, because that is
   // an indication that we are leaking bo's
            for (unsigned i = 0; i < msm_submit->nr_bos; i++)
            free(msm_submit->submit_bos);
   free(msm_submit->bos);
      }
      static const struct fd_submit_funcs submit_funcs = {
      .new_ringbuffer = msm_submit_new_ringbuffer,
   .flush = msm_submit_flush,
      };
      struct fd_submit *
   msm_submit_new(struct fd_pipe *pipe)
   {
      struct msm_submit *msm_submit = calloc(1, sizeof(*msm_submit));
            msm_submit->bo_table = _mesa_hash_table_create(NULL, _mesa_hash_pointer,
         msm_submit->ring_set =
         // TODO tune size:
            submit = &msm_submit->base;
               }
      static void
   finalize_current_cmd(struct fd_ringbuffer *ring)
   {
                        if (!msm_ring->cmd)
                     msm_ring->cmd->size = offset_bytes(ring->cur, ring->start);
   APPEND(&msm_ring->u, cmds, msm_ring->cmd);
      }
      static void
   msm_ringbuffer_grow(struct fd_ringbuffer *ring, uint32_t size)
   {
      struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
                              fd_bo_del(msm_ring->ring_bo);
   msm_ring->ring_bo = fd_bo_new_ring(pipe->dev, size);
            ring->start = fd_bo_map(msm_ring->ring_bo);
   ring->end = &(ring->start[size / 4]);
   ring->cur = ring->start;
      }
      static void
   msm_ringbuffer_emit_reloc(struct fd_ringbuffer *ring,
         {
      struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
   struct fd_pipe *pipe;
            if (ring->flags & _FD_RINGBUFFER_OBJECT) {
               /* this gets fixed up at submit->flush() time, since this state-
   * object rb can be used with many different submits
   */
               } else {
                                    APPEND(msm_ring->cmd, relocs,
         (struct drm_msm_gem_submit_reloc){
      .reloc_idx = reloc_idx,
   .reloc_offset = reloc->offset,
   .or = reloc->orval,
   .shift = reloc->shift,
   .submit_offset =
                  if (pipe->is_64bit) {
      APPEND(msm_ring->cmd, relocs,
         (struct drm_msm_gem_submit_reloc){
      .reloc_idx = reloc_idx,
   .reloc_offset = reloc->offset,
   .or = reloc->orval >> 32,
   .shift = reloc->shift - 32,
                     }
      static void
   append_stateobj_rings(struct msm_submit *submit, struct fd_ringbuffer *target)
   {
                        set_foreach (msm_target->u.ring_set, entry) {
                        if (ring->flags & _FD_RINGBUFFER_OBJECT) {
               }
      static uint32_t
   msm_ringbuffer_emit_reloc_ring(struct fd_ringbuffer *ring,
         {
      struct msm_ringbuffer *msm_target = to_msm_ringbuffer(target);
   struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
   struct fd_bo *bo;
            if ((target->flags & FD_RINGBUFFER_GROWABLE) &&
      (cmd_idx < msm_target->u.nr_cmds)) {
   bo = msm_target->u.cmds[cmd_idx]->ring_bo;
      } else {
      bo = msm_target->ring_bo;
               msm_ringbuffer_emit_reloc(ring, &(struct fd_reloc){
                              if (!size)
            if ((target->flags & _FD_RINGBUFFER_OBJECT) &&
      !(ring->flags & _FD_RINGBUFFER_OBJECT)) {
                        if (ring->flags & _FD_RINGBUFFER_OBJECT) {
         } else {
      struct msm_submit *msm_submit = to_msm_submit(msm_ring->u.submit);
                  }
      static uint32_t
   msm_ringbuffer_cmd_count(struct fd_ringbuffer *ring)
   {
      if (ring->flags & FD_RINGBUFFER_GROWABLE)
            }
      static bool
   msm_ringbuffer_check_size(struct fd_ringbuffer *ring)
   {
      assert(!(ring->flags & _FD_RINGBUFFER_OBJECT));
   struct msm_ringbuffer *msm_ring = to_msm_ringbuffer(ring);
   struct fd_submit *submit = msm_ring->u.submit;
            if ((fd_device_version(pipe->dev) < FD_VERSION_UNLIMITED_CMDS) &&
      ((ring->cur - ring->start) > (ring->size / 4 - 0x1000))) {
               if (to_msm_submit(submit)->nr_bos > MAX_ARRAY_SIZE/2) {
                     }
      static void
   msm_ringbuffer_destroy(struct fd_ringbuffer *ring)
   {
               fd_bo_del(msm_ring->ring_bo);
   if (msm_ring->cmd)
            if (ring->flags & _FD_RINGBUFFER_OBJECT) {
      for (unsigned i = 0; i < msm_ring->u.nr_reloc_bos; i++) {
                           free(msm_ring->u.reloc_bos);
      } else {
               for (unsigned i = 0; i < msm_ring->u.nr_cmds; i++) {
                  free(msm_ring->u.cmds);
         }
      static const struct fd_ringbuffer_funcs ring_funcs = {
      .grow = msm_ringbuffer_grow,
   .emit_reloc = msm_ringbuffer_emit_reloc,
   .emit_reloc_ring = msm_ringbuffer_emit_reloc_ring,
   .cmd_count = msm_ringbuffer_cmd_count,
   .check_size = msm_ringbuffer_check_size,
      };
      static inline struct fd_ringbuffer *
   msm_ringbuffer_init(struct msm_ringbuffer *msm_ring, uint32_t size,
         {
                        uint8_t *base = fd_bo_map(msm_ring->ring_bo);
   ring->start = (void *)(base + msm_ring->offset);
   ring->end = &(ring->start[size / 4]);
            ring->size = size;
                     msm_ring->u.cmds = NULL;
                        }
      struct fd_ringbuffer *
   msm_ringbuffer_new_object(struct fd_pipe *pipe, uint32_t size)
   {
               msm_ring->u.pipe = pipe;
   msm_ring->offset = 0;
   msm_ring->ring_bo = fd_bo_new_ring(pipe->dev, size);
            msm_ring->u.reloc_bos = NULL;
            msm_ring->u.ring_set =
               }
