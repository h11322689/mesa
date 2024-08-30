   #include "iris_context.h"
   #include "iris_fine_fence.h"
   #include "util/u_upload_mgr.h"
      static void
   iris_fine_fence_reset(struct iris_batch *batch)
   {
      u_upload_alloc(batch->fine_fences.uploader,
   0, sizeof(uint64_t), sizeof(uint64_t),
               WRITE_ONCE(*batch->fine_fences.map, 0);
      }
      void
   iris_fine_fence_init(struct iris_batch *batch)
   {
      batch->fine_fences.ref.res = NULL;
   batch->fine_fences.next = 0;
      }
      static uint32_t
   iris_fine_fence_next(struct iris_batch *batch)
   {
               if (batch->fine_fences.next == 0)
               }
      void
   iris_fine_fence_destroy(struct iris_screen *screen,
         {
      iris_syncobj_reference(screen->bufmgr, &fine->syncobj, NULL);
   pipe_resource_reference(&fine->ref.res, NULL);
      }
      struct iris_fine_fence *
   iris_fine_fence_new(struct iris_batch *batch)
   {
      struct iris_fine_fence *fine = calloc(1, sizeof(*fine));
   if (!fine)
                              iris_syncobj_reference(batch->screen->bufmgr, &fine->syncobj,
            pipe_resource_reference(&fine->ref.res, batch->fine_fences.ref.res);
   fine->ref.offset = batch->fine_fences.ref.offset;
            unsigned pc = PIPE_CONTROL_WRITE_IMMEDIATE |
               PIPE_CONTROL_RENDER_TARGET_FLUSH |
            if (batch->name == IRIS_BATCH_COMPUTE)
            iris_emit_pipe_control_write(batch, "fence: fine", pc,
                           }
