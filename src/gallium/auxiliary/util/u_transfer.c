   #include "pipe/p_context.h"
   #include "util/u_surface.h"
   #include "util/u_inlines.h"
   #include "util/u_transfer.h"
   #include "util/u_memory.h"
      void u_default_buffer_subdata(struct pipe_context *pipe,
                     {
      struct pipe_transfer *transfer = NULL;
   struct pipe_box box;
                     /* the write flag is implicit by the nature of buffer_subdata */
            /* buffer_subdata implicitly discards the rewritten buffer range.
   * PIPE_MAP_DIRECTLY supresses that.
   */
   if (!(usage & PIPE_MAP_DIRECTLY)) {
      if (offset == 0 && size == resource->width0) {
         } else {
                              map = pipe->buffer_map(pipe, resource, 0, usage, &box, &transfer);
   if (!map)
            memcpy(map, data, size);
      }
      void u_default_clear_buffer(struct pipe_context *pipe,
                           {
      struct pipe_transfer *transfer = NULL;
   struct pipe_box box;
            /* the write flag is implicit by the nature of buffer_subdata */
            /* clear_buffer implicitly discards the rewritten buffer range. */
   if (offset == 0 && size == resource->width0) {
         } else {
                           map = pipe->buffer_map(pipe, resource, 0, usage, &box, &transfer);
   if (!map)
            assert(clear_value_size > 0);
   for (unsigned off = 0; off < size; off += clear_value_size)
            }
      void u_default_texture_subdata(struct pipe_context *pipe,
                                 struct pipe_resource *resource,
   unsigned level,
   {
      struct pipe_transfer *transfer = NULL;
   const uint8_t *src_data = data;
                     /* the write flag is implicit by the nature of texture_subdata */
            /* texture_subdata implicitly discards the rewritten buffer range */
            map = pipe->texture_map(pipe,
                           if (!map)
            util_copy_box(map,
               resource->format,
   transfer->stride, /* bytes */
   transfer->layer_stride, /* bytes */
   0, 0, 0,
   box->width,
   box->height,
   box->depth,
   src_data,
               }
      void u_default_transfer_flush_region(UNUSED struct pipe_context *pipe,
               {
      /* This is a no-op implementation, nothing to do.
      }
