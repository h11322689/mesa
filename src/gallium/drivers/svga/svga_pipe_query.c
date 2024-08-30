   /**********************************************************
   * Copyright 2008-2015 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "pipe/p_state.h"
   #include "pipe/p_context.h"
      #include "util/u_bitmask.h"
   #include "util/u_memory.h"
      #include "svga_cmd.h"
   #include "svga_context.h"
   #include "svga_screen.h"
   #include "svga_resource_buffer.h"
   #include "svga_winsys.h"
   #include "svga_debug.h"
         /* Fixme: want a public base class for all pipe structs, even if there
   * isn't much in them.
   */
   struct pipe_query {
         };
      struct svga_query {
      struct pipe_query base;
   unsigned type;                  /**< PIPE_QUERY_x or SVGA_QUERY_x */
            unsigned id;                    /** Per-context query identifier */
                              /* For VGPU9 */
   struct svga_winsys_buffer *hwbuf;
            /** For VGPU10 */
   struct svga_winsys_gb_query *gb_query;
   SVGA3dDXQueryFlags flags;
   unsigned offset;                /**< offset to the gb_query memory */
            /** For non-GPU SVGA_QUERY_x queries */
      };
         /** cast wrapper */
   static inline struct svga_query *
   svga_query(struct pipe_query *q)
   {
         }
      /**
   * VGPU9
   */
      static bool
   svga_get_query_result(struct pipe_context *pipe,
                        static enum pipe_error
   define_query_vgpu9(struct svga_context *svga,
         {
               sq->hwbuf = svga_winsys_buffer_create(svga, 1,
               if (!sq->hwbuf)
            sq->queryResult = (SVGA3dQueryResult *)
         if (!sq->queryResult) {
      sws->buffer_destroy(sws, sq->hwbuf);
               sq->queryResult->totalSize = sizeof *sq->queryResult;
            /* We request the buffer to be pinned and assume it is always mapped.
   * The reason is that we don't want to wait for fences when checking the
   * query status.
   */
               }
      static void
   begin_query_vgpu9(struct svga_context *svga, struct svga_query *sq)
   {
               if (sq->queryResult->state == SVGA3D_QUERYSTATE_PENDING) {
      /* The application doesn't care for the pending query result.
   * We cannot let go of the existing buffer and just get a new one
   * because its storage may be reused for other purposes and clobbered
   * by the host when it determines the query result.  So the only
   * option here is to wait for the existing query's result -- not a
   * big deal, given that no sane application would do this.
   */
   uint64_t result;
   svga_get_query_result(&svga->pipe, &sq->base, true, (void*)&result);
               sq->queryResult->state = SVGA3D_QUERYSTATE_NEW;
               }
      static void
   end_query_vgpu9(struct svga_context *svga, struct svga_query *sq)
   {
      /* Set to PENDING before sending EndQuery. */
               }
      static bool
   get_query_result_vgpu9(struct svga_context *svga, struct svga_query *sq,
         {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
            if (!sq->fence) {
      /* The query status won't be updated by the host unless
   * SVGA_3D_CMD_WAIT_FOR_QUERY is emitted. Unfortunately this will cause
   * a synchronous wait on the host.
   */
   SVGA_RETRY(svga, SVGA3D_WaitForQuery(svga->swc, sq->svga_type,
         svga_context_flush(svga, &sq->fence);
               state = sq->queryResult->state;
   if (state == SVGA3D_QUERYSTATE_PENDING) {
      if (!wait)
         sws->fence_finish(sws, sq->fence, OS_TIMEOUT_INFINITE,
                     assert(state == SVGA3D_QUERYSTATE_SUCCEEDED ||
            *result = (uint64_t)sq->queryResult->result32;
      }
         /**
   * VGPU10
   *
   * There is one query mob allocated for each context to be shared by all
   * query types. The mob is used to hold queries's state and result. Since
   * each query result type is of different length, to ease the query allocation
   * management, the mob is divided into memory blocks. Each memory block
   * will hold queries of the same type. Multiple memory blocks can be allocated
   * for a particular query type.
   *
   * Currently each memory block is of 184 bytes. We support up to 512
   * memory blocks. The query memory size is arbitrary right now.
   * Each occlusion query takes about 8 bytes. One memory block can accomodate
   * 23 occlusion queries. 512 of those blocks can support up to 11K occlusion
   * queries. That seems reasonable for now. If we think this limit is
   * not enough, we can increase the limit or try to grow the mob in runtime.
   * Note, SVGA device does not impose one mob per context for queries,
   * we could allocate multiple mobs for queries; however, wddm KMD does not
   * currently support that.
   *
   * Also note that the GL guest driver does not issue any of the
   * following commands: DXMoveQuery, DXBindAllQuery & DXReadbackAllQuery.
   */
   #define SVGA_QUERY_MEM_BLOCK_SIZE    (sizeof(SVGADXQueryResultUnion) * 2)
   #define SVGA_QUERY_MEM_SIZE          (512 * SVGA_QUERY_MEM_BLOCK_SIZE)
      struct svga_qmem_alloc_entry
   {
      unsigned start_offset;               /* start offset of the memory block */
   unsigned block_index;                /* block index of the memory block */
   unsigned query_size;                 /* query size in this memory block */
   unsigned nquery;                     /* number of queries allocated */
   struct util_bitmask *alloc_mask;     /* allocation mask */
      };
         /**
   * Allocate a memory block from the query object memory
   * \return NULL if out of memory, else pointer to the query memory block
   */
   static struct svga_qmem_alloc_entry *
   allocate_query_block(struct svga_context *svga)
   {
      int index;
   unsigned offset;
            /* Find the next available query block */
            if (index == UTIL_BITMASK_INVALID_INDEX)
            offset = index * SVGA_QUERY_MEM_BLOCK_SIZE;
   if (offset >= svga->gb_query_len) {
               /* Deallocate the out-of-range index */
   util_bitmask_clear(svga->gb_query_alloc_mask, index);
            /**
   * All the memory blocks are allocated, lets see if there is
   * any empty memory block around that can be freed up.
   */
   for (i = 0; i < SVGA3D_QUERYTYPE_MAX && index == -1; i++) {
               alloc_entry = svga->gb_query_map[i];
   while (alloc_entry && index == -1) {
      if (alloc_entry->nquery == 0) {
      /* This memory block is empty, it can be recycled. */
   if (prev_alloc_entry) {
         } else {
         }
      } else {
      prev_alloc_entry = alloc_entry;
                     if (index == -1) {
      debug_printf("Query memory object is full\n");
                  if (!alloc_entry) {
      assert(index != -1);
   alloc_entry = CALLOC_STRUCT(svga_qmem_alloc_entry);
                  }
      /**
   * Allocate a slot in the specified memory block.
   * All slots in this memory block are of the same size.
   *
   * \return -1 if out of memory, else index of the query slot
   */
   static int
   allocate_query_slot(struct svga_context *svga,
         {
      int index;
            /* Find the next available slot */
            if (index == UTIL_BITMASK_INVALID_INDEX)
            offset = index * alloc->query_size;
   if (offset >= SVGA_QUERY_MEM_BLOCK_SIZE)
                        }
      /**
   * Deallocate the specified slot in the memory block.
   * If all slots are freed up, then deallocate the memory block
   * as well, so it can be allocated for other query type
   */
   static void
   deallocate_query_slot(struct svga_context *svga,
               {
               util_bitmask_clear(alloc->alloc_mask, index);
            /**
   * Don't worry about deallocating the empty memory block here.
   * The empty memory block will be recycled when no more memory block
   * can be allocated.
      }
      static struct svga_qmem_alloc_entry *
   allocate_query_block_entry(struct svga_context *svga,
         {
               alloc_entry = allocate_query_block(svga);
   if (!alloc_entry)
            assert(alloc_entry->block_index != -1);
   alloc_entry->start_offset =
         alloc_entry->nquery = 0;
   alloc_entry->alloc_mask = util_bitmask_create();
   alloc_entry->next = NULL;
               }
      /**
   * Allocate a memory slot for a query of the specified type.
   * It will first search through the memory blocks that are allocated
   * for the query type. If no memory slot is available, it will try
   * to allocate another memory block within the query object memory for
   * this query type.
   */
   static int
   allocate_query(struct svga_context *svga,
               {
      struct svga_qmem_alloc_entry *alloc_entry;
   int slot_index = -1;
                              if (!alloc_entry) {
      /**
   * No query memory block has been allocated for this query type,
   * allocate one now
   */
   alloc_entry = allocate_query_block_entry(svga, len);
   if (!alloc_entry)
                     /* Allocate a slot within the memory block allocated for this query type */
            if (slot_index == -1) {
      /* This query memory block is full, allocate another one */
   alloc_entry = allocate_query_block_entry(svga, len);
   if (!alloc_entry)
         alloc_entry->next = svga->gb_query_map[type];
   svga->gb_query_map[type] = alloc_entry;
               assert(slot_index != -1);
               }
         /**
   * Deallocate memory slot allocated for the specified query
   */
   static void
   deallocate_query(struct svga_context *svga,
         {
      struct svga_qmem_alloc_entry *alloc_entry;
   unsigned slot_index;
                     while (alloc_entry) {
      if (offset >= alloc_entry->start_offset &&
               /* The slot belongs to this memory block, deallocate it */
   slot_index = (offset - alloc_entry->start_offset) /
         deallocate_query_slot(svga, alloc_entry, slot_index);
      } else {
               }
         /**
   * Destroy the gb query object and all the related query structures
   */
   static void
   destroy_gb_query_obj(struct svga_context *svga)
   {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
            for (i = 0; i < SVGA3D_QUERYTYPE_MAX; i++) {
      struct svga_qmem_alloc_entry *alloc_entry, *next;
   alloc_entry = svga->gb_query_map[i];
   while (alloc_entry) {
      next = alloc_entry->next;
   util_bitmask_destroy(alloc_entry->alloc_mask);
   FREE(alloc_entry);
      }
               if (svga->gb_query)
                     }
      /**
   * Define query and create the gb query object if it is not already created.
   * There is only one gb query object per context which will be shared by
   * queries of all types.
   */
   static enum pipe_error
   define_query_vgpu10(struct svga_context *svga,
         {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
   int qlen;
                     if (svga->gb_query == NULL) {
      /* Create a gb query object */
   svga->gb_query = sws->query_create(sws, SVGA_QUERY_MEM_SIZE);
   if (!svga->gb_query)
         svga->gb_query_len = SVGA_QUERY_MEM_SIZE;
   memset (svga->gb_query_map, 0, sizeof(svga->gb_query_map));
            /* Bind the query object to the context */
   SVGA_RETRY(svga, svga->swc->query_bind(svga->swc, svga->gb_query,
                        /* Make sure query length is in multiples of 8 bytes */
            /* Find a slot for this query in the gb object */
   sq->offset = allocate_query(svga, sq->svga_type, qlen);
   if (sq->offset == -1)
                     SVGA_DBG(DEBUG_QUERY, "   query type=%d qid=0x%x offset=%d\n",
            /**
   * Send SVGA3D commands to define the query
   */
   SVGA_RETRY_OOM(svga, ret, SVGA3D_vgpu10_DefineQuery(svga->swc, sq->id,
               if (ret != PIPE_OK)
            SVGA_RETRY(svga, SVGA3D_vgpu10_BindQuery(svga->swc, sq->gb_query, sq->id));
   SVGA_RETRY(svga, SVGA3D_vgpu10_SetQueryOffset(svga->swc, sq->id,
               }
      static void
   destroy_query_vgpu10(struct svga_context *svga, struct svga_query *sq)
   {
               /* Deallocate the memory slot allocated for this query */
      }
         /**
   * Rebind queryies to the context.
   */
   static void
   rebind_vgpu10_query(struct svga_context *svga)
   {
      SVGA_RETRY(svga, svga->swc->query_bind(svga->swc, svga->gb_query,
            }
         static enum pipe_error
   begin_query_vgpu10(struct svga_context *svga, struct svga_query *sq)
   {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
                     /* Initialize the query state to NEW */
   status = sws->query_init(sws, sq->gb_query, sq->offset, SVGA3D_QUERYSTATE_NEW);
   if (status)
            if (svga->rebind.flags.query) {
                  /* Send the BeginQuery command to the device */
   SVGA_RETRY(svga, SVGA3D_vgpu10_BeginQuery(svga->swc, sq->id));
      }
      static void
   end_query_vgpu10(struct svga_context *svga, struct svga_query *sq)
   {
      if (svga->rebind.flags.query) {
                     }
      static bool
   get_query_result_vgpu10(struct svga_context *svga, struct svga_query *sq,
         {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
            if (svga->rebind.flags.query) {
                           if (queryState != SVGA3D_QUERYSTATE_SUCCEEDED && !sq->fence) {
      /* We don't have the query result yet, and the query hasn't been
   * submitted.  We need to submit it now since the GL spec says
   * "Querying the state for a given occlusion query forces that
   * occlusion query to complete within a finite amount of time."
   */
               if (queryState == SVGA3D_QUERYSTATE_PENDING ||
      queryState == SVGA3D_QUERYSTATE_NEW) {
   if (!wait)
         sws->fence_finish(sws, sq->fence, OS_TIMEOUT_INFINITE,
                     assert(queryState == SVGA3D_QUERYSTATE_SUCCEEDED ||
               }
      static struct pipe_query *
   svga_create_query(struct pipe_context *pipe,
               {
      struct svga_context *svga = svga_context(pipe);
   struct svga_query *sq;
                     sq = CALLOC_STRUCT(svga_query);
   if (!sq)
            /* Allocate an integer ID for the query */
   sq->id = util_bitmask_add(svga->query_id_bm);
   if (sq->id == UTIL_BITMASK_INVALID_INDEX)
            SVGA_DBG(DEBUG_QUERY, "%s type=%d sq=0x%x id=%d\n", __func__,
            switch (query_type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      sq->svga_type = SVGA3D_QUERYTYPE_OCCLUSION;
   if (svga_have_vgpu10(svga)) {
      ret = define_query_vgpu10(svga, sq,
                        /**
   * In OpenGL, occlusion counter query can be used in conditional
   * rendering; however, in DX10, only OCCLUSION_PREDICATE query can
   * be used for predication. Hence, we need to create an occlusion
   * predicate query along with the occlusion counter query. So when
   * the occlusion counter query is used for predication, the associated
   * query of occlusion predicate type will be used
   * in the SetPredication command.
               } else {
      ret = define_query_vgpu9(svga, sq);
   if (ret != PIPE_OK)
      }
      case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      if (svga_have_vgpu10(svga)) {
      sq->svga_type = SVGA3D_QUERYTYPE_OCCLUSIONPREDICATE;
   ret = define_query_vgpu10(svga, sq,
         if (ret != PIPE_OK)
      } else {
      sq->svga_type = SVGA3D_QUERYTYPE_OCCLUSION;
   ret = define_query_vgpu9(svga, sq);
   if (ret != PIPE_OK)
      }
      case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   case PIPE_QUERY_SO_STATISTICS:
               /* Until the device supports the new query type for multiple streams,
   * we will use the single stream query type for stream 0.
   */
   if (svga_have_sm5(svga) && index > 0) {
                  }
   else {
      assert(index == 0);
      }
   ret = define_query_vgpu10(svga, sq,
         if (ret != PIPE_OK)
            case PIPE_QUERY_TIMESTAMP:
      assert(svga_have_vgpu10(svga));
   sq->svga_type = SVGA3D_QUERYTYPE_TIMESTAMP;
   ret = define_query_vgpu10(svga, sq,
         if (ret != PIPE_OK)
            case SVGA_QUERY_NUM_DRAW_CALLS:
   case SVGA_QUERY_NUM_FALLBACKS:
   case SVGA_QUERY_NUM_FLUSHES:
   case SVGA_QUERY_NUM_VALIDATIONS:
   case SVGA_QUERY_NUM_BUFFERS_MAPPED:
   case SVGA_QUERY_NUM_TEXTURES_MAPPED:
   case SVGA_QUERY_NUM_BYTES_UPLOADED:
   case SVGA_QUERY_NUM_COMMAND_BUFFERS:
   case SVGA_QUERY_COMMAND_BUFFER_SIZE:
   case SVGA_QUERY_SURFACE_WRITE_FLUSHES:
   case SVGA_QUERY_MEMORY_USED:
   case SVGA_QUERY_NUM_SHADERS:
   case SVGA_QUERY_NUM_RESOURCES:
   case SVGA_QUERY_NUM_STATE_OBJECTS:
   case SVGA_QUERY_NUM_SURFACE_VIEWS:
   case SVGA_QUERY_NUM_GENERATE_MIPMAP:
   case SVGA_QUERY_NUM_READBACKS:
   case SVGA_QUERY_NUM_RESOURCE_UPDATES:
   case SVGA_QUERY_NUM_BUFFER_UPLOADS:
   case SVGA_QUERY_NUM_CONST_BUF_UPDATES:
   case SVGA_QUERY_NUM_CONST_UPDATES:
   case SVGA_QUERY_NUM_FAILED_ALLOCATIONS:
   case SVGA_QUERY_NUM_COMMANDS_PER_DRAW:
   case SVGA_QUERY_NUM_SHADER_RELOCATIONS:
   case SVGA_QUERY_NUM_SURFACE_RELOCATIONS:
   case SVGA_QUERY_SHADER_MEM_USED:
         case SVGA_QUERY_FLUSH_TIME:
   case SVGA_QUERY_MAP_BUFFER_TIME:
      /* These queries need os_time_get() */
   svga->hud.uses_time = true;
         default:
                                 fail:
      FREE(sq);
      }
      static void
   svga_destroy_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct svga_context *svga = svga_context(pipe);
   struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
            if (!q) {
      destroy_gb_query_obj(svga);
                        SVGA_DBG(DEBUG_QUERY, "%s sq=0x%x id=%d\n", __func__,
            switch (sq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      if (svga_have_vgpu10(svga)) {
      /* make sure to also destroy any associated predicate query */
   if (sq->predicate)
            } else {
         }
   sws->fence_reference(sws, &sq->fence, NULL);
      case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   case PIPE_QUERY_SO_STATISTICS:
   case PIPE_QUERY_TIMESTAMP:
      assert(svga_have_vgpu10(svga));
   destroy_query_vgpu10(svga, sq);
   sws->fence_reference(sws, &sq->fence, NULL);
      case SVGA_QUERY_NUM_DRAW_CALLS:
   case SVGA_QUERY_NUM_FALLBACKS:
   case SVGA_QUERY_NUM_FLUSHES:
   case SVGA_QUERY_NUM_VALIDATIONS:
   case SVGA_QUERY_MAP_BUFFER_TIME:
   case SVGA_QUERY_NUM_BUFFERS_MAPPED:
   case SVGA_QUERY_NUM_TEXTURES_MAPPED:
   case SVGA_QUERY_NUM_BYTES_UPLOADED:
   case SVGA_QUERY_NUM_COMMAND_BUFFERS:
   case SVGA_QUERY_COMMAND_BUFFER_SIZE:
   case SVGA_QUERY_FLUSH_TIME:
   case SVGA_QUERY_SURFACE_WRITE_FLUSHES:
   case SVGA_QUERY_MEMORY_USED:
   case SVGA_QUERY_NUM_SHADERS:
   case SVGA_QUERY_NUM_RESOURCES:
   case SVGA_QUERY_NUM_STATE_OBJECTS:
   case SVGA_QUERY_NUM_SURFACE_VIEWS:
   case SVGA_QUERY_NUM_GENERATE_MIPMAP:
   case SVGA_QUERY_NUM_READBACKS:
   case SVGA_QUERY_NUM_RESOURCE_UPDATES:
   case SVGA_QUERY_NUM_BUFFER_UPLOADS:
   case SVGA_QUERY_NUM_CONST_BUF_UPDATES:
   case SVGA_QUERY_NUM_CONST_UPDATES:
   case SVGA_QUERY_NUM_FAILED_ALLOCATIONS:
   case SVGA_QUERY_NUM_COMMANDS_PER_DRAW:
   case SVGA_QUERY_NUM_SHADER_RELOCATIONS:
   case SVGA_QUERY_NUM_SURFACE_RELOCATIONS:
   case SVGA_QUERY_SHADER_MEM_USED:
      /* nothing */
      default:
                  /* Free the query id */
               }
         static bool
   svga_begin_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct svga_context *svga = svga_context(pipe);
   struct svga_query *sq = svga_query(q);
            assert(sq);
            /* Need to flush out buffered drawing commands so that they don't
   * get counted in the query results.
   */
            switch (sq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      if (svga_have_vgpu10(svga)) {
      ret = begin_query_vgpu10(svga, sq);
   /* also need to start the associated occlusion predicate query */
   if (sq->predicate) {
      enum pipe_error status;
   status = begin_query_vgpu10(svga, svga_query(sq->predicate));
   assert(status == PIPE_OK);
         } else {
         }
   assert(ret == PIPE_OK);
   (void) ret;
      case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   case PIPE_QUERY_SO_STATISTICS:
   case PIPE_QUERY_TIMESTAMP:
      assert(svga_have_vgpu10(svga));
   ret = begin_query_vgpu10(svga, sq);
   assert(ret == PIPE_OK);
      case SVGA_QUERY_NUM_DRAW_CALLS:
      sq->begin_count = svga->hud.num_draw_calls;
      case SVGA_QUERY_NUM_FALLBACKS:
      sq->begin_count = svga->hud.num_fallbacks;
      case SVGA_QUERY_NUM_FLUSHES:
      sq->begin_count = svga->hud.num_flushes;
      case SVGA_QUERY_NUM_VALIDATIONS:
      sq->begin_count = svga->hud.num_validations;
      case SVGA_QUERY_MAP_BUFFER_TIME:
      sq->begin_count = svga->hud.map_buffer_time;
      case SVGA_QUERY_NUM_BUFFERS_MAPPED:
      sq->begin_count = svga->hud.num_buffers_mapped;
      case SVGA_QUERY_NUM_TEXTURES_MAPPED:
      sq->begin_count = svga->hud.num_textures_mapped;
      case SVGA_QUERY_NUM_BYTES_UPLOADED:
      sq->begin_count = svga->hud.num_bytes_uploaded;
      case SVGA_QUERY_NUM_COMMAND_BUFFERS:
      sq->begin_count = svga->swc->num_command_buffers;
      case SVGA_QUERY_COMMAND_BUFFER_SIZE:
      sq->begin_count = svga->hud.command_buffer_size;
      case SVGA_QUERY_FLUSH_TIME:
      sq->begin_count = svga->hud.flush_time;
      case SVGA_QUERY_SURFACE_WRITE_FLUSHES:
      sq->begin_count = svga->hud.surface_write_flushes;
      case SVGA_QUERY_NUM_READBACKS:
      sq->begin_count = svga->hud.num_readbacks;
      case SVGA_QUERY_NUM_RESOURCE_UPDATES:
      sq->begin_count = svga->hud.num_resource_updates;
      case SVGA_QUERY_NUM_BUFFER_UPLOADS:
      sq->begin_count = svga->hud.num_buffer_uploads;
      case SVGA_QUERY_NUM_CONST_BUF_UPDATES:
      sq->begin_count = svga->hud.num_const_buf_updates;
      case SVGA_QUERY_NUM_CONST_UPDATES:
      sq->begin_count = svga->hud.num_const_updates;
      case SVGA_QUERY_NUM_SHADER_RELOCATIONS:
      sq->begin_count = svga->swc->num_shader_reloc;
      case SVGA_QUERY_NUM_SURFACE_RELOCATIONS:
      sq->begin_count = svga->swc->num_surf_reloc;
      case SVGA_QUERY_MEMORY_USED:
   case SVGA_QUERY_NUM_SHADERS:
   case SVGA_QUERY_NUM_RESOURCES:
   case SVGA_QUERY_NUM_STATE_OBJECTS:
   case SVGA_QUERY_NUM_SURFACE_VIEWS:
   case SVGA_QUERY_NUM_GENERATE_MIPMAP:
   case SVGA_QUERY_NUM_FAILED_ALLOCATIONS:
   case SVGA_QUERY_NUM_COMMANDS_PER_DRAW:
   case SVGA_QUERY_SHADER_MEM_USED:
      /* nothing */
      default:
                  SVGA_DBG(DEBUG_QUERY, "%s sq=0x%x id=%d type=%d svga_type=%d\n",
                        }
         static bool
   svga_end_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct svga_context *svga = svga_context(pipe);
            assert(sq);
            SVGA_DBG(DEBUG_QUERY, "%s sq=0x%x type=%d\n",
            if (sq->type == PIPE_QUERY_TIMESTAMP && !sq->active)
            SVGA_DBG(DEBUG_QUERY, "%s sq=0x%x id=%d type=%d svga_type=%d\n",
                              switch (sq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      if (svga_have_vgpu10(svga)) {
      end_query_vgpu10(svga, sq);
   /* also need to end the associated occlusion predicate query */
   if (sq->predicate) {
            } else {
         }
      case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   case PIPE_QUERY_SO_STATISTICS:
   case PIPE_QUERY_TIMESTAMP:
      assert(svga_have_vgpu10(svga));
   end_query_vgpu10(svga, sq);
      case SVGA_QUERY_NUM_DRAW_CALLS:
      sq->end_count = svga->hud.num_draw_calls;
      case SVGA_QUERY_NUM_FALLBACKS:
      sq->end_count = svga->hud.num_fallbacks;
      case SVGA_QUERY_NUM_FLUSHES:
      sq->end_count = svga->hud.num_flushes;
      case SVGA_QUERY_NUM_VALIDATIONS:
      sq->end_count = svga->hud.num_validations;
      case SVGA_QUERY_MAP_BUFFER_TIME:
      sq->end_count = svga->hud.map_buffer_time;
      case SVGA_QUERY_NUM_BUFFERS_MAPPED:
      sq->end_count = svga->hud.num_buffers_mapped;
      case SVGA_QUERY_NUM_TEXTURES_MAPPED:
      sq->end_count = svga->hud.num_textures_mapped;
      case SVGA_QUERY_NUM_BYTES_UPLOADED:
      sq->end_count = svga->hud.num_bytes_uploaded;
      case SVGA_QUERY_NUM_COMMAND_BUFFERS:
      sq->end_count = svga->swc->num_command_buffers;
      case SVGA_QUERY_COMMAND_BUFFER_SIZE:
      sq->end_count = svga->hud.command_buffer_size;
      case SVGA_QUERY_FLUSH_TIME:
      sq->end_count = svga->hud.flush_time;
      case SVGA_QUERY_SURFACE_WRITE_FLUSHES:
      sq->end_count = svga->hud.surface_write_flushes;
      case SVGA_QUERY_NUM_READBACKS:
      sq->end_count = svga->hud.num_readbacks;
      case SVGA_QUERY_NUM_RESOURCE_UPDATES:
      sq->end_count = svga->hud.num_resource_updates;
      case SVGA_QUERY_NUM_BUFFER_UPLOADS:
      sq->end_count = svga->hud.num_buffer_uploads;
      case SVGA_QUERY_NUM_CONST_BUF_UPDATES:
      sq->end_count = svga->hud.num_const_buf_updates;
      case SVGA_QUERY_NUM_CONST_UPDATES:
      sq->end_count = svga->hud.num_const_updates;
      case SVGA_QUERY_NUM_SHADER_RELOCATIONS:
      sq->end_count = svga->swc->num_shader_reloc;
      case SVGA_QUERY_NUM_SURFACE_RELOCATIONS:
      sq->end_count = svga->swc->num_surf_reloc;
      case SVGA_QUERY_MEMORY_USED:
   case SVGA_QUERY_NUM_SHADERS:
   case SVGA_QUERY_NUM_RESOURCES:
   case SVGA_QUERY_NUM_STATE_OBJECTS:
   case SVGA_QUERY_NUM_SURFACE_VIEWS:
   case SVGA_QUERY_NUM_GENERATE_MIPMAP:
   case SVGA_QUERY_NUM_FAILED_ALLOCATIONS:
   case SVGA_QUERY_NUM_COMMANDS_PER_DRAW:
   case SVGA_QUERY_SHADER_MEM_USED:
      /* nothing */
      default:
         }
   sq->active = false;
      }
         static bool
   svga_get_query_result(struct pipe_context *pipe,
                     {
      struct svga_screen *svgascreen = svga_screen(pipe->screen);
   struct svga_context *svga = svga_context(pipe);
   struct svga_query *sq = svga_query(q);
   uint64_t *result = (uint64_t *)vresult;
                     SVGA_DBG(DEBUG_QUERY, "%s sq=0x%x id=%d wait: %d\n",
            switch (sq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      if (svga_have_vgpu10(svga)) {
      SVGADXOcclusionQueryResult occResult;
   ret = get_query_result_vgpu10(svga, sq, wait,
            } else {
         }
      case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE: {
      if (svga_have_vgpu10(svga)) {
      SVGADXOcclusionPredicateQueryResult occResult;
   ret = get_query_result_vgpu10(svga, sq, wait,
            } else {
      uint64_t count = 0;
   ret = get_query_result_vgpu9(svga, sq, wait, &count);
      }
      }
   case PIPE_QUERY_SO_STATISTICS: {
      SVGADXStreamOutStatisticsQueryResult sResult;
   struct pipe_query_data_so_statistics *pResult =
            assert(svga_have_vgpu10(svga));
   ret = get_query_result_vgpu10(svga, sq, wait,
         pResult->num_primitives_written = sResult.numPrimitivesWritten;
   pResult->primitives_storage_needed = sResult.numPrimitivesRequired;
      }
   case PIPE_QUERY_TIMESTAMP: {
               assert(svga_have_vgpu10(svga));
   ret = get_query_result_vgpu10(svga, sq, wait,
         *result = (uint64_t)sResult.timestamp;
      }
   case PIPE_QUERY_PRIMITIVES_GENERATED: {
               assert(svga_have_vgpu10(svga));
   ret = get_query_result_vgpu10(svga, sq, wait,
         *result = (uint64_t)sResult.numPrimitivesRequired;
      }
   case PIPE_QUERY_PRIMITIVES_EMITTED: {
               assert(svga_have_vgpu10(svga));
   ret = get_query_result_vgpu10(svga, sq, wait,
         *result = (uint64_t)sResult.numPrimitivesWritten;
      }
   /* These are per-frame counters */
   case SVGA_QUERY_NUM_DRAW_CALLS:
   case SVGA_QUERY_NUM_FALLBACKS:
   case SVGA_QUERY_NUM_FLUSHES:
   case SVGA_QUERY_NUM_VALIDATIONS:
   case SVGA_QUERY_MAP_BUFFER_TIME:
   case SVGA_QUERY_NUM_BUFFERS_MAPPED:
   case SVGA_QUERY_NUM_TEXTURES_MAPPED:
   case SVGA_QUERY_NUM_BYTES_UPLOADED:
   case SVGA_QUERY_NUM_COMMAND_BUFFERS:
   case SVGA_QUERY_COMMAND_BUFFER_SIZE:
   case SVGA_QUERY_FLUSH_TIME:
   case SVGA_QUERY_SURFACE_WRITE_FLUSHES:
   case SVGA_QUERY_NUM_READBACKS:
   case SVGA_QUERY_NUM_RESOURCE_UPDATES:
   case SVGA_QUERY_NUM_BUFFER_UPLOADS:
   case SVGA_QUERY_NUM_CONST_BUF_UPDATES:
   case SVGA_QUERY_NUM_CONST_UPDATES:
   case SVGA_QUERY_NUM_SHADER_RELOCATIONS:
   case SVGA_QUERY_NUM_SURFACE_RELOCATIONS:
      vresult->u64 = sq->end_count - sq->begin_count;
      /* These are running total counters */
   case SVGA_QUERY_MEMORY_USED:
      vresult->u64 = svgascreen->hud.total_resource_bytes;
      case SVGA_QUERY_NUM_SHADERS:
      vresult->u64 = svga->hud.num_shaders;
      case SVGA_QUERY_NUM_RESOURCES:
      vresult->u64 = svgascreen->hud.num_resources;
      case SVGA_QUERY_NUM_STATE_OBJECTS:
      vresult->u64 = (svga->hud.num_blend_objects +
                  svga->hud.num_depthstencil_objects +
   svga->hud.num_rasterizer_objects +
         case SVGA_QUERY_NUM_SURFACE_VIEWS:
      vresult->u64 = svga->hud.num_surface_views;
      case SVGA_QUERY_NUM_GENERATE_MIPMAP:
      vresult->u64 = svga->hud.num_generate_mipmap;
      case SVGA_QUERY_NUM_FAILED_ALLOCATIONS:
      vresult->u64 = svgascreen->hud.num_failed_allocations;
      case SVGA_QUERY_NUM_COMMANDS_PER_DRAW:
      vresult->f = (float) svga->swc->num_commands
            case SVGA_QUERY_SHADER_MEM_USED:
      vresult->u64 = svga->hud.shader_mem_used;
      default:
                              }
      static void
   svga_render_condition(struct pipe_context *pipe, struct pipe_query *q,
         {
      struct svga_context *svga = svga_context(pipe);
   struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
   struct svga_query *sq = svga_query(q);
                     assert(svga_have_vgpu10(svga));
   if (sq == NULL) {
         }
   else {
      assert(sq->svga_type == SVGA3D_QUERYTYPE_OCCLUSION ||
            if (sq->svga_type == SVGA3D_QUERYTYPE_OCCLUSION) {
      assert(sq->predicate);
   /**
   * For conditional rendering, make sure to use the associated
   * predicate query.
   */
      }
            if ((mode == PIPE_RENDER_COND_WAIT ||
      mode == PIPE_RENDER_COND_BY_REGION_WAIT) && sq->fence) {
   sws->fence_finish(sws, sq->fence, OS_TIMEOUT_INFINITE,
         }
   /*
   * if the kernel module doesn't support the predication command,
   * we'll just render unconditionally.
   * This is probably acceptable for the typical case of occlusion culling.
   */
   if (sws->have_set_predication_cmd) {
      SVGA_RETRY(svga, SVGA3D_vgpu10_SetPredication(svga->swc, queryId,
         svga->pred.query_id = queryId;
                  }
         /*
   * This function is a workaround because we lack the ability to query
   * renderer's time synchronously.
   */
   static uint64_t
   svga_get_timestamp(struct pipe_context *pipe)
   {
      struct pipe_query *q = svga_create_query(pipe, PIPE_QUERY_TIMESTAMP, 0);
            util_query_clear_result(&result, PIPE_QUERY_TIMESTAMP);
   svga_begin_query(pipe, q);
   svga_end_query(pipe,q);
   svga_get_query_result(pipe, q, true, &result);
               }
         static void
   svga_set_active_query_state(struct pipe_context *pipe, bool enable)
   {
   }
         /**
   * \brief Toggle conditional rendering if already enabled
   *
   * \param svga[in]  The svga context
   * \param render_condition_enabled[in]  Whether to ignore requests to turn
   * conditional rendering off
   * \param on[in]  Whether to turn conditional rendering on or off
   */
   void
   svga_toggle_render_condition(struct svga_context *svga,
               {
               if (render_condition_enabled ||
      svga->pred.query_id == SVGA3D_INVALID_ID) {
               /*
   * If we get here, it means that the system supports
   * conditional rendering since svga->pred.query_id has already been
   * modified for this context and thus support has already been
   * verified.
   */
            SVGA_RETRY(svga, SVGA3D_vgpu10_SetPredication(svga->swc, query_id,
      }
         void
   svga_init_query_functions(struct svga_context *svga)
   {
      svga->pipe.create_query = svga_create_query;
   svga->pipe.destroy_query = svga_destroy_query;
   svga->pipe.begin_query = svga_begin_query;
   svga->pipe.end_query = svga_end_query;
   svga->pipe.get_query_result = svga_get_query_result;
   svga->pipe.set_active_query_state = svga_set_active_query_state;
   svga->pipe.render_condition = svga_render_condition;
      }
