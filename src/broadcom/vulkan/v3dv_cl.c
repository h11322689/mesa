   /*
   * Copyright © 2019 Raspberry Pi Ltd
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "v3dv_private.h"
      /* We don't expect that the packets we use in this file change across hw
   * versions, so we just explicitly set the V3D_VERSION and include v3dx_pack
   * here
   */
   #define V3D_VERSION 33
   #include "broadcom/common/v3d_macros.h"
   #include "broadcom/cle/v3dx_pack.h"
      void
   v3dv_cl_init(struct v3dv_job *job, struct v3dv_cl *cl)
   {
      cl->base = NULL;
   cl->next = cl->base;
   cl->bo = NULL;
   cl->size = 0;
   cl->job = job;
      }
      void
   v3dv_cl_destroy(struct v3dv_cl *cl)
   {
      list_for_each_entry_safe(struct v3dv_bo, bo, &cl->bo_list, list_link) {
      assert(cl->job);
   list_del(&bo->list_link);
               /* Leave the CL in a reset state to catch use after destroy instances */
      }
      static bool
   cl_alloc_bo(struct v3dv_cl *cl, uint32_t space, bool use_branch)
   {
      /* If we are growing, double the BO allocation size to reduce the number
   * of allocations with large command buffers. This has a very significant
   * impact on the number of draw calls per second reported by vkoverhead.
   */
   space = align(space, 4096);
   if (cl->bo)
            struct v3dv_bo *bo = v3dv_bo_alloc(cl->job->device, space, "CL", true);
   if (!bo) {
      fprintf(stderr, "failed to allocate memory for command list\n");
   v3dv_flag_oom(NULL, cl->job);
                        bool ok = v3dv_bo_map(cl->job->device, bo, bo->size);
   if (!ok) {
      fprintf(stderr, "failed to map command list buffer\n");
   v3dv_flag_oom(NULL, cl->job);
               /* Chain to the new BO from the old one if requested */
   if (use_branch && cl->bo) {
      cl_emit(cl, BRANCH, branch) {
            } else {
                  cl->bo = bo;
   cl->base = cl->bo->map;
   cl->size = cl->bo->size;
               }
      uint32_t
   v3dv_cl_ensure_space(struct v3dv_cl *cl, uint32_t space, uint32_t alignment)
   {
               if (offset + space <= cl->size) {
      cl->next = cl->base + offset;
               cl_alloc_bo(cl, space, false);
      }
      void
   v3dv_cl_ensure_space_with_branch(struct v3dv_cl *cl, uint32_t space)
   {
      /* We do not want to emit branches from secondary command lists, instead,
   * we will branch to them when we execute them in a primary using
   * 'branch to sub list' commands, expecting each linked secondary to
   * end with a 'return from sub list' command.
   */
   bool needs_return_from_sub_list = false;
   if (cl->job->type == V3DV_JOB_TYPE_GPU_CL_SECONDARY && cl->size > 0)
            /*
   * The CLE processor in the simulator tries to read V3D_CL_MAX_INSTR_SIZE
   * bytes form the CL for each new instruction. If the last instruction in our
   * CL is smaller than that, and there are not at least V3D_CL_MAX_INSTR_SIZE
   * bytes until the end of the BO, it will read out of bounds and possibly
   * cause a GMP violation interrupt to trigger. Ensure we always have at
   * least that many bytes available to read with the last instruction.
   */
            if (v3dv_cl_offset(cl) + space <= cl->size)
            if (needs_return_from_sub_list)
               }
