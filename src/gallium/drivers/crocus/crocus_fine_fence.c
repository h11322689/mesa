   #include "crocus_context.h"
   #include "crocus_fine_fence.h"
   #include "util/u_upload_mgr.h"
      static void
   crocus_fine_fence_reset(struct crocus_batch *batch)
   {
      u_upload_alloc(batch->fine_fences.uploader,
                     WRITE_ONCE(*batch->fine_fences.map, 0);
      }
      void
   crocus_fine_fence_init(struct crocus_batch *batch)
   {
      batch->fine_fences.ref.res = NULL;
   batch->fine_fences.next = 0;
   if (batch_has_fine_fence(batch))
      }
      static uint32_t
   crocus_fine_fence_next(struct crocus_batch *batch)
   {
      if (!batch_has_fine_fence(batch))
                     if (batch->fine_fences.next == 0)
               }
      void
   crocus_fine_fence_destroy(struct crocus_screen *screen,
         {
      crocus_syncobj_reference(screen, &fine->syncobj, NULL);
   pipe_resource_reference(&fine->ref.res, NULL);
      }
      struct crocus_fine_fence *
   crocus_fine_fence_new(struct crocus_batch *batch, unsigned flags)
   {
      struct crocus_fine_fence *fine = calloc(1, sizeof(*fine));
   if (!fine)
                              crocus_syncobj_reference(batch->screen, &fine->syncobj,
            if (!batch_has_fine_fence(batch))
         pipe_resource_reference(&fine->ref.res, batch->fine_fences.ref.res);
   fine->ref.offset = batch->fine_fences.ref.offset;
   fine->map = batch->fine_fences.map;
            unsigned pc;
   if (flags & CROCUS_FENCE_TOP_OF_PIPE) {
         } else {
      pc = PIPE_CONTROL_WRITE_IMMEDIATE |
      PIPE_CONTROL_RENDER_TARGET_FLUSH |
   PIPE_CONTROL_TILE_CACHE_FLUSH |
   PIPE_CONTROL_DEPTH_CACHE_FLUSH |
   }
   crocus_emit_pipe_control_write(batch, "fence: fine", pc,
                           }
