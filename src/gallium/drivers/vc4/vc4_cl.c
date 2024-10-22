   /*
   * Copyright © 2014 Broadcom
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
      #include "util/u_math.h"
   #include "util/ralloc.h"
   #include "vc4_context.h"
      void
   vc4_init_cl(struct vc4_job *job, struct vc4_cl *cl)
   {
         cl->base = rzalloc_size(job, 1); /* TODO: don't use rzalloc */
   cl->next = cl->base;
   cl->size = 0;
   }
      void
   cl_ensure_space(struct vc4_cl *cl, uint32_t space)
   {
                  if (offset + space <= cl->size)
                     cl->base = reralloc(ralloc_parent(cl->base), cl->base, uint8_t, size);
   cl->size = size;
   }
      void
   vc4_reset_cl(struct vc4_cl *cl)
   {
         assert(cl->reloc_count == 0);
   }
      uint32_t
   vc4_gem_hindex(struct vc4_job *job, struct vc4_bo *bo)
   {
         uint32_t hindex;
   uint32_t *current_handles = job->bo_handles.base;
   uint32_t cl_hindex_count = cl_offset(&job->bo_handles) / 4;
            if (last_hindex < cl_hindex_count &&
         current_handles[last_hindex] == bo->handle) {
            for (hindex = 0; hindex < cl_hindex_count; hindex++) {
            if (current_handles[hindex] == bo->handle) {
                              out = cl_start(&job->bo_handles);
   cl_u32(&out, bo->handle);
            out = cl_start(&job->bo_pointers);
   cl_ptr(&out, vc4_bo_reference(bo));
                     bo->last_hindex = hindex;
   }
