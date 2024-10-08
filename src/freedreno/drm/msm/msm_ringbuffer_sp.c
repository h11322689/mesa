   /*
   * Copyright (C) 2018 Rob Clark <robclark@freedesktop.org>
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
   #include <pthread.h>
      #include "util/os_file.h"
      #include "drm/freedreno_ringbuffer_sp.h"
   #include "msm_priv.h"
      static int
   flush_submit_list(struct list_head *submit_list)
   {
      struct fd_submit_sp *fd_submit = to_fd_submit_sp(last_submit(submit_list));
   struct fd_pipe *pipe = fd_submit->base.pipe;
   struct msm_pipe *msm_pipe = to_msm_pipe(pipe);
   struct drm_msm_gem_submit req = {
      .flags = msm_pipe->pipe,
      };
                              /* Determine the number of extra cmds's from deferred submits that
   * we will be merging in:
   */
   foreach_submit (submit, submit_list) {
      assert(submit->pipe == &msm_pipe->base);
                                 /* Build up the table of cmds, and for all but the last submit in the
   * list, merge their bo tables into the last submit.
   */
   foreach_submit_safe (submit, submit_list) {
      struct fd_ringbuffer_sp *deferred_primary =
            for (unsigned i = 0; i < deferred_primary->u.nr_cmds; i++) {
      struct fd_bo *ring_bo = deferred_primary->u.cmds[i].ring_bo;
   cmds[cmd_idx].type = MSM_SUBMIT_CMD_BUF;
   cmds[cmd_idx].submit_idx = fd_submit_append_bo(fd_submit, ring_bo);
   cmds[cmd_idx].submit_offset = submit_offset(ring_bo, deferred_primary->offset);
   cmds[cmd_idx].size = deferred_primary->u.cmds[i].size;
                              /* We are merging all the submits in the list into the last submit,
   * so the remainder of the loop body doesn't apply to the last submit
   */
   if (submit == last_submit(submit_list)) {
      DEBUG_MSG("merged %u submits", cmd_idx);
               struct fd_submit_sp *fd_deferred_submit = to_fd_submit_sp(submit);
   for (unsigned i = 0; i < fd_deferred_submit->nr_bos; i++) {
      /* Note: if bo is used in both the current submit and the deferred
   * submit being merged, we expect to hit the fast-path as we add it
   * to the current submit:
   */
               /* Now that the cmds/bos have been transfered over to the current submit,
   * we can remove the deferred submit from the list and drop it's reference
   */
   list_del(&submit->node);
               if (fd_submit->in_fence_fd != -1) {
      req.flags |= MSM_SUBMIT_FENCE_FD_IN;
               if (pipe->no_implicit_sync) {
                  if (fd_submit->out_fence->use_fence_fd) {
                  /* Needs to be after get_cmd() as that could create bos/cmds table:
   *
   * NOTE allocate on-stack in the common case, but with an upper-
   * bound to limit on-stack allocation to 4k:
   */
   const unsigned bo_limit = 4096 / sizeof(struct drm_msm_gem_submit_bo);
   bool bos_on_stack = fd_submit->nr_bos < bo_limit;
   struct drm_msm_gem_submit_bo
         struct drm_msm_gem_submit_bo *submit_bos;
   if (bos_on_stack) {
         } else {
                  for (unsigned i = 0; i < fd_submit->nr_bos; i++) {
      submit_bos[i].flags = fd_submit->bos[i]->reloc_flags;
   submit_bos[i].handle = fd_submit->bos[i]->handle;
               req.bos = VOID2U64(submit_bos);
   req.nr_bos = fd_submit->nr_bos;
   req.cmds = VOID2U64(cmds);
                     ret = drmCommandWriteRead(msm_pipe->base.dev->fd, DRM_MSM_GEM_SUBMIT, &req,
         if (ret) {
      ERROR_MSG("submit failed: %d (%s)", ret, strerror(errno));
      } else if (!ret) {
      fd_submit->out_fence->kfence = req.fence;
               if (!bos_on_stack)
            if (fd_submit->in_fence_fd != -1)
               }
      struct fd_submit *
   msm_submit_sp_new(struct fd_pipe *pipe)
   {
      /* We don't do any translation from internal FD_RELOC flags to MSM flags. */
   STATIC_ASSERT(FD_RELOC_READ == MSM_SUBMIT_BO_READ);
   STATIC_ASSERT(FD_RELOC_WRITE == MSM_SUBMIT_BO_WRITE);
               }
