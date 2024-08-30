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
      /**
   * Gallium query object support.
   *
   * The HW has native support for occlusion queries, with the query result
   * being loaded and stored by the TLB unit. From a SW perspective, we have to
   * be careful to make sure that the jobs that need to be tracking queries are
   * bracketed by the start and end of counting, even across FBO transitions.
   *
   * For the transform feedback PRIMITIVES_GENERATED/WRITTEN queries, we have to
   * do the calculations in software at draw time.
   */
      #include "v3d_query.h"
      struct v3d_query_pipe
   {
                  enum pipe_query_type type;
            uint32_t start, end;
   };
      static void
   v3d_destroy_query_pipe(struct v3d_context *v3d, struct v3d_query *query)
   {
                  v3d_bo_unreference(&pquery->bo);
   }
      static bool
   v3d_begin_query_pipe(struct v3d_context *v3d, struct v3d_query *query)
   {
                  switch (pquery->type) {
   case PIPE_QUERY_PRIMITIVES_GENERATED:
            /* If we are using PRIMITIVE_COUNTS_FEEDBACK to retrieve
   * primitive counts from the GPU (which we need when a GS
   * is present), then we need to update our counters now
   * to discard any primitives generated before this.
   */
   if (v3d->prog.gs)
         pquery->start = v3d->prims_generated;
      case PIPE_QUERY_PRIMITIVES_EMITTED:
            /* If we are inside transform feedback we need to update the
   * primitive counts to skip primitives recorded before this.
   */
   if (v3d->streamout.num_targets > 0)
            case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
            v3d_bo_unreference(&pquery->bo);
                        v3d->current_oq = pquery->bo;
      default:
                  }
      static bool
   v3d_end_query_pipe(struct v3d_context *v3d, struct v3d_query *query)
   {
                  switch (pquery->type) {
   case PIPE_QUERY_PRIMITIVES_GENERATED:
            /* If we are using PRIMITIVE_COUNTS_FEEDBACK to retrieve
   * primitive counts from the GPU (which we need when a GS
   * is present), then we need to update our counters now.
   */
   if (v3d->prog.gs)
         pquery->end = v3d->prims_generated;
      case PIPE_QUERY_PRIMITIVES_EMITTED:
            /* If transform feedback has ended, then we have already
   * updated the primitive counts at glEndTransformFeedback()
   * time. Otherwise, we have to do it now.
   */
   if (v3d->streamout.num_targets > 0)
            case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
            v3d->current_oq = NULL;
      default:
                  }
      static bool
   v3d_get_query_result_pipe(struct v3d_context *v3d, struct v3d_query *query,
         {
                  if (pquery->bo) {
                     if (wait) {
               } else {
                                                   switch (pquery->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
               case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
               case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
               default:
                  }
      static const struct v3d_query_funcs pipe_query_funcs = {
         .destroy_query = v3d_destroy_query_pipe,
   .begin_query = v3d_begin_query_pipe,
   .end_query = v3d_end_query_pipe,
   };
      struct pipe_query *
   v3d_create_query_pipe(struct v3d_context *v3d, unsigned query_type, unsigned index)
   {
         if (query_type >= PIPE_QUERY_DRIVER_SPECIFIC)
            struct v3d_query_pipe *pquery = calloc(1, sizeof(*pquery));
            pquery->type = query_type;
            /* Note that struct pipe_query isn't actually defined anywhere. */
   }
