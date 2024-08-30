   /**************************************************************************
      Copyright 2002-2008 VMware, Inc.
      All Rights Reserved.
      Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   on the rights to use, copy, modify, merge, publish, distribute, sub
   license, and/or sell copies of the Software, and to permit persons to whom
   the Software is furnished to do so, subject to the following conditions:
      The above copyright notice and this permission notice (including the next
   paragraph) shall be included in all copies or substantial portions of the
   Software.
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.
      **************************************************************************/
      /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   */
            /* Display list compiler attempts to store lists of vertices with the
   * same vertex layout.  Additionally it attempts to minimize the need
   * for execute-time fixup of these vertex lists, allowing them to be
   * cached on hardware.
   *
   * There are still some circumstances where this can be thwarted, for
   * example by building a list that consists of one very long primitive
   * (eg Begin(Triangles), 1000 vertices, End), and calling that list
   * from inside a different begin/end object (Begin(Lines), CallList,
   * End).
   *
   * In that case the code will have to replay the list as individual
   * commands through the Exec dispatch table, or fix up the copied
   * vertices at execute-time.
   *
   * The other case where fixup is required is when a vertex attribute
   * is introduced in the middle of a primitive.  Eg:
   *  Begin(Lines)
   *  TexCoord1f()           Vertex2f()
   *  TexCoord1f() Color3f() Vertex2f()
   *  End()
   *
   *  If the current value of Color isn't known at compile-time, this
   *  primitive will require fixup.
   *
   *
   * The list compiler currently doesn't attempt to compile lists
   * containing EvalCoord or EvalPoint commands.  On encountering one of
   * these, compilation falls back to opcodes.
   *
   * This could be improved to fallback only when a mix of EvalCoord and
   * Vertex commands are issued within a single primitive.
   *
   * The compilation process works as follows. All vertex attributes
   * except position are copied to vbo_save_context::attrptr (see ATTR_UNION).
   * 'attrptr' are pointers to vbo_save_context::vertex ordered according to the enabled
   * attributes (se upgrade_vertex).
   * When the position attribute is received, all the attributes are then 
   * copied to the vertex_store (see the end of ATTR_UNION).
   * The vertex_store is simply an extensible float array.
   * When the vertex list needs to be compiled (see compile_vertex_list),
   * several transformations are performed:
   *   - some primitives are merged together (eg: two consecutive GL_TRIANGLES
   * with 3 vertices can be merged in a single GL_TRIANGLES with 6 vertices).
   *   - an index buffer is built.
   *   - identical vertices are detected and only one is kept.
   * At the end of this transformation, the index buffer and the vertex buffer
   * are uploaded in vRAM in the same buffer object.
   * This buffer object is shared between multiple display list to allow
   * draw calls merging later.
   *
   * The layout of this buffer for two display lists is:
   *    V0A0|V0A1|V1A0|V1A1|P0I0|P0I1|V0A0V0A1V0A2|V1A1V1A1V1A2|...
   *                                 ` new list starts
   *        - VxAy: vertex x, attributes y
   *        - PxIy: draw x, index y
   *
   * To allow draw call merging, display list must use the same VAO, including
   * the same Offset in the buffer object. To achieve this, the start values of
   * the primitive are shifted and the indices adjusted (see offset_diff and
   * start_offset in compile_vertex_list).
   *
   * Display list using the loopback code (see vbo_save_playback_vertex_list_loopback),
   * can't be drawn with an index buffer so this transformation is disabled
   * in this case.
   */
         #include "util/glheader.h"
   #include "main/arrayobj.h"
   #include "main/bufferobj.h"
   #include "main/context.h"
   #include "main/dlist.h"
   #include "main/enums.h"
   #include "main/eval.h"
   #include "main/macros.h"
   #include "main/draw_validate.h"
   #include "main/api_arrayelt.h"
   #include "main/dispatch.h"
   #include "main/state.h"
   #include "main/varray.h"
   #include "util/bitscan.h"
   #include "util/u_memory.h"
   #include "util/hash_table.h"
   #include "gallium/auxiliary/indices/u_indices.h"
   #include "util/u_prim.h"
      #include "gallium/include/pipe/p_state.h"
      #include "vbo_private.h"
   #include "api_exec_decl.h"
   #include "api_save.h"
      #ifdef ERROR
   #undef ERROR
   #endif
      /* An interesting VBO number/name to help with debugging */
   #define VBO_BUF_ID  12345
      static void GLAPIENTRY
   _save_Materialfv(GLenum face, GLenum pname, const GLfloat *params);
      static void GLAPIENTRY
   _save_EvalCoord1f(GLfloat u);
      static void GLAPIENTRY
   _save_EvalCoord2f(GLfloat u, GLfloat v);
      /*
   * NOTE: Old 'parity' issue is gone, but copying can still be
   * wrong-footed on replay.
   */
   static GLuint
   copy_vertices(struct gl_context *ctx,
               {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
   struct _mesa_prim *prim = &node->cold->prims[node->cold->prim_count - 1];
            if (prim->end || !prim->count || !sz)
            const fi_type *src = src_buffer + prim->start * sz;
   assert(save->copied.buffer == NULL);
            unsigned r = vbo_copy_vertices(ctx, prim->mode, prim->start, &prim->count,
         if (!r) {
      free(save->copied.buffer);
      }
      }
         static struct vbo_save_primitive_store *
   realloc_prim_store(struct vbo_save_primitive_store *store, int prim_count)
   {
      if (store == NULL)
            uint32_t old_size = store->size;
   store->size = prim_count;
   assert (old_size < store->size);
   store->prims = realloc(store->prims, store->size * sizeof(struct _mesa_prim));
               }
         static void
   reset_counters(struct gl_context *ctx)
   {
               save->vertex_store->used = 0;
   save->prim_store->used = 0;
      }
      /**
   * For a list of prims, try merging prims that can just be extensions of the
   * previous prim.
   */
   static void
   merge_prims(struct gl_context *ctx, struct _mesa_prim *prim_list,
         {
      GLuint i;
            for (i = 1; i < *prim_count; i++) {
                        if (vbo_merge_draws(ctx, true,
                     prev_prim->mode, this_prim->mode,
   prev_prim->start, this_prim->start,
   &prev_prim->count, this_prim->count,
      /* We've found a prim that just extend the previous one.  Tack it
   * onto the previous one, and let this primitive struct get dropped.
   */
               /* If any previous primitives have been dropped, then we need to copy
   * this later one into the next available slot.
   */
   prev_prim++;
   if (prev_prim != this_prim)
                  }
         /**
   * Convert GL_LINE_LOOP primitive into GL_LINE_STRIP so that drivers
   * don't have to worry about handling the _mesa_prim::begin/end flags.
   * See https://bugs.freedesktop.org/show_bug.cgi?id=81174
   */
   static void
   convert_line_loop_to_strip(struct vbo_save_context *save,
         {
                        if (prim->end) {
      /* Copy the 0th vertex to end of the buffer and extend the
   * vertex count by one to finish the line loop.
   */
   const GLuint sz = save->vertex_size;
   /* 0th vertex: */
   const fi_type *src = save->vertex_store->buffer_in_ram + prim->start * sz;
   /* end of buffer: */
                     prim->count++;
   node->cold->vertex_count++;
               if (!prim->begin) {
      /* Drawing the second or later section of a long line loop.
   * Skip the 0th vertex.
   */
   prim->start++;
                  }
         /* Compare the present vao if it has the same setup. */
   static bool
   compare_vao(gl_vertex_processing_mode mode,
               const struct gl_vertex_array_object *vao,
   const struct gl_buffer_object *bo, GLintptr buffer_offset,
   GLuint stride, GLbitfield64 vao_enabled,
   const GLubyte size[VBO_ATTRIB_MAX],
   {
      if (!vao)
            /* If the enabled arrays are not the same we are not equal. */
   if (vao_enabled != vao->Enabled)
            /* Check the buffer binding at 0 */
   if (vao->BufferBinding[0].BufferObj != bo)
         /* BufferBinding[0].Offset != buffer_offset is checked per attribute */
   if (vao->BufferBinding[0].Stride != stride)
                  /* Retrieve the mapping from VBO_ATTRIB to VERT_ATTRIB space */
            /* Now check the enabled arrays */
   GLbitfield mask = vao_enabled;
   while (mask) {
      const int attr = u_bit_scan(&mask);
   const unsigned char vbo_attr = vao_to_vbo_map[attr];
   const GLenum16 tp = type[vbo_attr];
   const GLintptr off = offset[vbo_attr] + buffer_offset;
   const struct gl_array_attributes *attrib = &vao->VertexAttrib[attr];
   if (attrib->RelativeOffset + vao->BufferBinding[0].Offset != off)
         if (attrib->Format.User.Type != tp)
         if (attrib->Format.User.Size != size[vbo_attr])
         assert(!attrib->Format.User.Bgra);
   assert(attrib->Format.User.Normalized == GL_FALSE);
   assert(attrib->Format.User.Integer == vbo_attrtype_to_integer_flag(tp));
   assert(attrib->Format.User.Doubles == vbo_attrtype_to_double_flag(tp));
                  }
         /* Create or reuse the vao for the vertex processing mode. */
   static void
   update_vao(struct gl_context *ctx,
            gl_vertex_processing_mode mode,
   struct gl_vertex_array_object **vao,
   struct gl_buffer_object *bo, GLintptr buffer_offset,
   GLuint stride, GLbitfield64 vbo_enabled,
   const GLubyte size[VBO_ATTRIB_MAX],
      {
      /* Compute the bitmasks of vao_enabled arrays */
            /*
   * Check if we can possibly reuse the exisiting one.
   * In the long term we should reset them when something changes.
   */
   if (compare_vao(mode, *vao, bo, buffer_offset, stride,
                  /* The initial refcount is 1 */
   _mesa_reference_vao(ctx, vao, NULL);
            /*
   * assert(stride <= ctx->Const.MaxVertexAttribStride);
   * MaxVertexAttribStride is not set for drivers that does not
   * expose GL 44 or GLES 31.
            /* Bind the buffer object at binding point 0 */
   _mesa_bind_vertex_buffer(ctx, *vao, 0, bo, buffer_offset, stride, false,
            /* Retrieve the mapping from VBO_ATTRIB to VERT_ATTRIB space
   * Note that the position/generic0 aliasing is done in the VAO.
   */
   const GLubyte *const vao_to_vbo_map = _vbo_attribute_alias_map[mode];
   /* Now set the enable arrays */
   GLbitfield mask = vao_enabled;
   while (mask) {
      const int vao_attr = u_bit_scan(&mask);
   const GLubyte vbo_attr = vao_to_vbo_map[vao_attr];
            _vbo_set_attrib_format(ctx, *vao, vao_attr, buffer_offset,
            }
   _mesa_enable_vertex_array_attribs(ctx, *vao, vao_enabled);
   assert(vao_enabled == (*vao)->Enabled);
            /* Finalize and freeze the VAO */
      }
      static void wrap_filled_vertex(struct gl_context *ctx);
      /* Grow the vertex storage to accomodate for vertex_count new vertices */
   static void
   grow_vertex_storage(struct gl_context *ctx, int vertex_count)
   {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
            int new_size = (save->vertex_store->used +
            /* Limit how much memory we allocate. */
   if (save->prim_store->used > 0 &&
      vertex_count > 0 &&
   new_size > VBO_SAVE_BUFFER_SIZE) {
   wrap_filled_vertex(ctx);
               if (new_size > save->vertex_store->buffer_in_ram_size) {
      save->vertex_store->buffer_in_ram_size = new_size;
   save->vertex_store->buffer_in_ram = realloc(save->vertex_store->buffer_in_ram,
         if (save->vertex_store->buffer_in_ram == NULL)
         }
      struct vertex_key {
      unsigned vertex_size;
      };
      static uint32_t _hash_vertex_key(const void *key)
   {
      struct vertex_key *k = (struct vertex_key*)key;
   unsigned sz = k->vertex_size;
   assert(sz);
      }
      static bool _compare_vertex_key(const void *key1, const void *key2)
   {
      struct vertex_key *k1 = (struct vertex_key*)key1;
   struct vertex_key *k2 = (struct vertex_key*)key2;
   /* All the compared vertices are going to be drawn with the same VAO,
   * so we can compare the attributes. */
   assert (k1->vertex_size == k2->vertex_size);
   return memcmp(k1->vertex_attributes,
            }
      static void _free_entry(struct hash_entry *entry)
   {
         }
      /* Add vertex to the vertex buffer and return its index. If this vertex is a duplicate
   * of an existing vertex, return the original index instead.
   */
   static uint32_t
   add_vertex(struct vbo_save_context *save, struct hash_table *hash_to_index,
         {
      /* If vertex deduplication is disabled return the original index. */
   if (!hash_to_index)
                     struct vertex_key *key = malloc(sizeof(struct vertex_key));
   key->vertex_size = save->vertex_size;
            struct hash_entry *entry = _mesa_hash_table_search(hash_to_index, key);
   if (entry) {
      free(key);
   /* We found an existing vertex with the same hash, return its index. */
      } else {
      /* This is a new vertex. Determine a new index and copy its attributes to the vertex
   * buffer. Note that 'new_buffer' is created at each list compilation so we write vertices
   * starting at index 0.
   */
   uint32_t n = _mesa_hash_table_num_entries(hash_to_index);
            memcpy(&new_buffer[save->vertex_size * n],
                           /* The index buffer is shared between list compilations, so add the base index to get
   * the final index.
   */
         }
         static uint32_t
   get_vertex_count(struct vbo_save_context *save)
   {
      if (!save->vertex_size)
            }
         /**
   * Insert the active immediate struct onto the display list currently
   * being built.
   */
   static void
   compile_vertex_list(struct gl_context *ctx)
   {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
            /* Allocate space for this structure in the display list currently
   * being compiled.
   */
   node = (struct vbo_save_vertex_list *)
            if (!node)
                     /* Make sure the pointer is aligned to the size of a pointer */
                     node->cold->vertex_count = get_vertex_count(save);
   node->cold->wrap_count = save->copied.nr;
   node->cold->prims = malloc(sizeof(struct _mesa_prim) * save->prim_store->used);
   memcpy(node->cold->prims, save->prim_store->prims, sizeof(struct _mesa_prim) * save->prim_store->used);
   node->cold->ib.obj = NULL;
            if (save->no_current_update) {
         }
   else {
      GLuint current_size = save->vertex_size - save->attrsz[0];
            if (current_size) {
      node->cold->current_data = malloc(current_size * sizeof(GLfloat));
   if (node->cold->current_data) {
      const char *buffer = (const char *)save->vertex_store->buffer_in_ram;
                                 memcpy(node->cold->current_data, buffer + vertex_offset + attr_offset,
      } else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Current value allocation");
                              if (save->dangling_attr_ref)
            /* Copy duplicated vertices
   */
            if (node->cold->prims[node->cold->prim_count - 1].mode == GL_LINE_LOOP) {
                           GLintptr buffer_offset = 0;
            /* Create an index buffer. */
   node->cold->min_index = node->cold->max_index = 0;
   if (node->cold->vertex_count == 0 || node->cold->prim_count == 0)
            /* We won't modify node->prims, so use a const alias to avoid unintended
   * writes to it. */
            int end = original_prims[node->cold->prim_count - 1].start +
                  node->cold->min_index = node->cold->prims[0].start;
            /* converting primitive types may result in many more indices */
   bool all_prims_supported = (ctx->Const.DriverSupportedPrimMask & BITFIELD_MASK(MESA_PRIM_COUNT)) == BITFIELD_MASK(MESA_PRIM_COUNT);
   int max_index_count = total_vert_count * (all_prims_supported ? 2 : 3);
   uint32_t* indices = (uint32_t*) malloc(max_index_count * sizeof(uint32_t));
   void *tmp_indices = all_prims_supported ? NULL : malloc(max_index_count * sizeof(uint32_t));
            int idx = 0;
   struct hash_table *vertex_to_index = NULL;
            /* The loopback replay code doesn't use the index buffer, so we can't
   * dedup vertices in this case.
   */
   if (!ctx->ListState.Current.UseLoopback) {
      vertex_to_index = _mesa_hash_table_create(NULL, _hash_vertex_key, _compare_vertex_key);
                        int last_valid_prim = -1;
   /* Construct indices array. */
   for (unsigned i = 0; i < node->cold->prim_count; i++) {
      assert(original_prims[i].basevertex == 0);
   GLubyte mode = original_prims[i].mode;
   bool converted_prim = false;
   unsigned index_size;
   bool outputting_quads = !!(ctx->Const.DriverSupportedPrimMask &
                  int vertex_count = original_prims[i].count;
   if (!vertex_count) {
                  /* Increase indices storage if the original estimation was too small. */
   if (idx + verts_per_primitive * vertex_count > max_index_count) {
      max_index_count = max_index_count + verts_per_primitive * vertex_count;
   indices = (uint32_t*) realloc(indices, max_index_count * sizeof(uint32_t));
               /* Line strips may get converted to lines */
   if (mode == GL_LINE_STRIP)
            if (!(ctx->Const.DriverSupportedPrimMask & BITFIELD_BIT(mode))) {
      unsigned new_count;
   u_generate_func trans_func;
   enum mesa_prim pmode = (enum mesa_prim)mode;
   u_index_generator(ctx->Const.DriverSupportedPrimMask,
                     pmode, original_prims[i].start, vertex_count,
   if (new_count > 0)
         vertex_count = new_count;
   mode = (GLubyte)pmode;
               /* If 2 consecutive prims use the same mode => merge them. */
   bool merge_prims = last_valid_prim >= 0 &&
                        /* index generation uses uint16_t if the index count is small enough */
   #define CAST_INDEX(BASE, SIZE, IDX) ((SIZE == 2 ? (uint32_t)(((uint16_t*)BASE)[IDX]) : ((uint32_t*)BASE)[IDX]))
         /* To be able to merge consecutive triangle strips we need to insert
   * a degenerate triangle.
   */
   if (merge_prims &&
      mode == GL_TRIANGLE_STRIP) {
   /* Insert a degenerate triangle */
                  indices[idx] = indices[idx - 1];
   indices[idx + 1] = add_vertex(save, vertex_to_index,
                              if (tri_count % 2) {
      /* Add another index to preserve winding order */
   indices[idx++] = add_vertex(save, vertex_to_index,
                                       /* Convert line strips to lines if it'll allow if the previous
   * prim mode is GL_LINES (so merge_prims is true) or if the next
   * primitive mode is GL_LINES or GL_LINE_LOOP.
   */
   if (original_prims[i].mode == GL_LINE_STRIP &&
      (merge_prims ||
   (i < node->cold->prim_count - 1 &&
      (original_prims[i + 1].mode == GL_LINE_STRIP ||
      for (unsigned j = 0; j < vertex_count; j++) {
      indices[idx++] = add_vertex(save, vertex_to_index,
               /* Repeat all but the first/last indices. */
   if (j && j != vertex_count - 1) {
      indices[idx++] = add_vertex(save, vertex_to_index,
                  } else {
      /* We didn't convert to LINES, so restore the original mode */
                  for (unsigned j = 0; j < vertex_count; j++) {
      indices[idx++] = add_vertex(save, vertex_to_index,
                        /* Duplicate the last vertex for incomplete primitives */
   if (vertex_count > 0) {
      unsigned min_vert = u_prim_vertex_count(mode)->min;
   for (unsigned j = vertex_count; j < min_vert; j++) {
      indices[idx++] = add_vertex(save, vertex_to_index,
                        #undef CAST_INDEX
         if (merge_prims) {
      /* Update vertex count. */
      } else {
      /* Keep this primitive */
   last_valid_prim += 1;
   assert(last_valid_prim <= i);
   merged_prims = realloc(merged_prims, (1 + last_valid_prim) * sizeof(struct _mesa_prim));
   merged_prims[last_valid_prim] = original_prims[i];
   merged_prims[last_valid_prim].start = start;
      }
            /* converted prims will filter incomplete primitives and may have no indices */
               unsigned merged_prim_count = last_valid_prim + 1;
   node->cold->ib.ptr = NULL;
   node->cold->ib.count = idx;
            /* How many bytes do we need to store the indices and the vertices */
   total_vert_count = vertex_to_index ? (max_index + 1) : idx;
   unsigned total_bytes_needed = idx * sizeof(uint32_t) +
            const GLintptr old_offset = save->VAO[0] ?
         if (old_offset != save->current_bo_bytes_used && stride > 0) {
      GLintptr offset_diff = save->current_bo_bytes_used - old_offset;
   while (offset_diff > 0 &&
         save->current_bo_bytes_used < save->current_bo->Size &&
      save->current_bo_bytes_used++;
         }
            /* Can we reuse the previous bo or should we allocate a new one? */
   int available_bytes = save->current_bo ? save->current_bo->Size - save->current_bo_bytes_used : 0;
   if (total_bytes_needed > available_bytes) {
      if (save->current_bo)
         save->current_bo = _mesa_bufferobj_alloc(ctx, VBO_BUF_ID + 1);
   bool success = _mesa_bufferobj_data(ctx,
                                       if (!success) {
      _mesa_reference_buffer_object(ctx, &save->current_bo, NULL);
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "IB allocation");
      } else {
      save->current_bo_bytes_used = 0;
      }
      } else {
      assert(old_offset <= buffer_offset);
   const GLintptr offset_diff = buffer_offset - old_offset;
   if (offset_diff > 0 && stride > 0 && offset_diff % stride == 0) {
      /* The vertex size is an exact multiple of the buffer offset.
   * This means that we can use zero-based vertex attribute pointers
   * and specify the start of the primitive with the _mesa_prim::start
   * field.  This results in issuing several draw calls with identical
   * vertex attribute information.  This can result in fewer state
   * changes in drivers.  In particular, the Gallium CSO module will
   * filter out redundant vertex buffer changes.
   */
   /* We cannot immediately update the primitives as some methods below
   * still need the uncorrected start vertices
   */
   start_offset = offset_diff/stride;
   assert(old_offset == buffer_offset - offset_diff);
               /* Correct the primitive starts, we can only do this here as copy_vertices
   * and convert_line_loop_to_strip above consume the uncorrected starts.
   * On the other hand the _vbo_loopback_vertex_list call below needs the
   * primitives to be corrected already.
   */
   for (unsigned i = 0; i < node->cold->prim_count; i++) {
         }
   /* start_offset shifts vertices (so v[0] becomes v[start_offset]), so we have
   * to apply this transformation to all indices and max_index.
   */
   for (unsigned i = 0; i < idx; i++)
                              /* Upload the vertices first (see buffer_offset) */
   _mesa_bufferobj_subdata(ctx,
                           save->current_bo_bytes_used += total_vert_count * save->vertex_size * sizeof(fi_type);
         if (vertex_to_index) {
         _mesa_hash_table_destroy(vertex_to_index, _free_entry);
               /* Since we append the indices to an existing buffer, we need to adjust the start value of each
   * primitive (not the indices themselves). */
   if (!ctx->ListState.Current.UseLoopback) {
      save->current_bo_bytes_used += align(save->current_bo_bytes_used, 4) - save->current_bo_bytes_used;
   int indices_offset = save->current_bo_bytes_used / 4;
   for (int i = 0; i < merged_prim_count; i++) {
                     /* Then upload the indices. */
   if (node->cold->ib.obj) {
      _mesa_bufferobj_subdata(ctx,
                              } else {
      node->cold->vertex_count = 0;
               /* Prepare for DrawGallium */
   memset(&node->cold->info, 0, sizeof(struct pipe_draw_info));
   /* The other info fields will be updated in vbo_save_playback_vertex_list */
   node->cold->info.index_size = 4;
   node->cold->info.instance_count = 1;
   node->cold->info.index.resource = node->cold->ib.obj->buffer;
   if (merged_prim_count == 1) {
      node->cold->info.mode = merged_prims[0].mode;
   node->start_count.start = merged_prims[0].start;
   node->start_count.count = merged_prims[0].count;
   node->start_count.index_bias = 0;
      } else {
      node->modes = malloc(merged_prim_count * sizeof(unsigned char));
   node->start_counts = malloc(merged_prim_count * sizeof(struct pipe_draw_start_count_bias));
   for (unsigned i = 0; i < merged_prim_count; i++) {
      node->start_counts[i].start = merged_prims[i].start;
   node->start_counts[i].count = merged_prims[i].count;
   node->start_counts[i].index_bias = 0;
         }
   node->num_draws = merged_prim_count;
   if (node->num_draws > 1) {
      bool same_mode = true;
   for (unsigned i = 1; i < node->num_draws && same_mode; i++) {
         }
   if (same_mode) {
      /* All primitives use the same mode, so we can simplify a bit */
   node->cold->info.mode = node->modes[0];
   free(node->modes);
                  free(indices);
   free(tmp_indices);
         end:
               if (!save->current_bo) {
      save->current_bo = _mesa_bufferobj_alloc(ctx, VBO_BUF_ID + 1);
   bool success = _mesa_bufferobj_data(ctx,
                                       if (!success)
               GLuint offsets[VBO_ATTRIB_MAX];
   for (unsigned i = 0, offset = 0; i < VBO_ATTRIB_MAX; ++i) {
      offsets[i] = offset;
      }
   /* Create a pair of VAOs for the possible VERTEX_PROCESSING_MODEs
   * Note that this may reuse the previous one of possible.
   */
   for (gl_vertex_processing_mode vpm = VP_MODE_FF; vpm < VP_MODE_MAX; ++vpm) {
      /* create or reuse the vao */
   update_vao(ctx, vpm, &save->VAO[vpm],
               /* Reference the vao in the dlist */
   node->cold->VAO[vpm] = NULL;
               /* Prepare for DrawGalliumVertexState */
   if (node->num_draws && ctx->Driver.DrawGalliumVertexState) {
      for (unsigned i = 0; i < VP_MODE_MAX; i++) {
                     node->state[i] =
      ctx->Driver.CreateGalliumVertexState(ctx, node->cold->VAO[i],
            node->private_refcount[i] = 0;
               node->ctx = ctx;
   node->mode = node->cold->info.mode;
               /* Deal with GL_COMPILE_AND_EXECUTE:
   */
   if (ctx->ExecuteFlag) {
      /* _vbo_loopback_vertex_list doesn't use the index buffer, so we have to
   * use buffer_in_ram (which contains all vertices) instead of current_bo
   * (which contains deduplicated vertices *when* UseLoopback is false).
   *
   * The problem is that the VAO offset is based on current_bo's layout,
   * so we have to use a temp value.
   */
   struct gl_vertex_array_object *vao = node->cold->VAO[VP_MODE_SHADER];
   GLintptr original = vao->BufferBinding[0].Offset;
   /* 'start_offset' has been added to all primitives 'start', so undo it here. */
   vao->BufferBinding[0].Offset = -(GLintptr)(start_offset * stride);
   _vbo_loopback_vertex_list(ctx, node, save->vertex_store->buffer_in_ram);
               /* Reset our structures for the next run of vertices:
   */
      }
         /**
   * This is called when we fill a vertex buffer before we hit a glEnd().
   * We
   * TODO -- If no new vertices have been stored, don't bother saving it.
   */
   static void
   wrap_buffers(struct gl_context *ctx)
   {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
   GLint i = save->prim_store->used - 1;
            assert(i < (GLint) save->prim_store->size);
            /* Close off in-progress primitive.
   */
   save->prim_store->prims[i].count = (get_vertex_count(save) - save->prim_store->prims[i].start);
            /* store the copied vertices, and allocate a new list.
   */
            /* Restart interrupted primitive
   */
   save->prim_store->prims[0].mode = mode;
   save->prim_store->prims[0].begin = 0;
   save->prim_store->prims[0].end = 0;
   save->prim_store->prims[0].start = 0;
   save->prim_store->prims[0].count = 0;
      }
         /**
   * Called only when buffers are wrapped as the result of filling the
   * vertex_store struct.
   */
   static void
   wrap_filled_vertex(struct gl_context *ctx)
   {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
            /* Emit a glEnd to close off the last vertex list.
   */
                     /* Copy stored stored vertices to start of new list.
   */
            fi_type *buffer_ptr = save->vertex_store->buffer_in_ram;
   if (numComponents) {
      assert(save->copied.buffer);
   memcpy(buffer_ptr,
         save->copied.buffer,
   free(save->copied.buffer);
      }
      }
         static void
   copy_to_current(struct gl_context *ctx)
   {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
            while (enabled) {
      const int i = u_bit_scan64(&enabled);
            if (save->attrtype[i] == GL_DOUBLE ||
      save->attrtype[i] == GL_UNSIGNED_INT64_ARB)
      else
      COPY_CLEAN_4V_TYPE_AS_UNION(save->current[i], save->attrsz[i],
      }
         static void
   copy_from_current(struct gl_context *ctx)
   {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
            while (enabled) {
               switch (save->attrsz[i]) {
   case 4:
      save->attrptr[i][3] = save->current[i][3];
      case 3:
      save->attrptr[i][2] = save->current[i][2];
      case 2:
      save->attrptr[i][1] = save->current[i][1];
      case 1:
      save->attrptr[i][0] = save->current[i][0];
      case 0:
               }
         /**
   * Called when we increase the size of a vertex attribute.  For example,
   * if we've seen one or more glTexCoord2f() calls and now we get a
   * glTexCoord3f() call.
   * Flush existing data, set new attrib size, replay copied vertices.
   */
   static void
   upgrade_vertex(struct gl_context *ctx, GLuint attr, GLuint newsz)
   {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
   GLuint oldsz;
   GLuint i;
            /* Store the current run of vertices, and emit a GL_END.  Emit a
   * BEGIN in the new buffer.
   */
   if (save->vertex_store->used)
         else
            /* Do a COPY_TO_CURRENT to ensure back-copying works for the case
   * when the attribute already exists in the vertex and is having
   * its size increased.
   */
            /* Fix up sizes:
   */
   oldsz = save->attrsz[attr];
   save->attrsz[attr] = newsz;
                     /* Recalculate all the attrptr[] values:
   */
   tmp = save->vertex;
   for (i = 0; i < VBO_ATTRIB_MAX; i++) {
      if (save->attrsz[i]) {
      save->attrptr[i] = tmp;
      }
   else {
                     /* Copy from current to repopulate the vertex with correct values.
   */
            /* Replay stored vertices to translate them to new format here.
   *
   * If there are copied vertices and the new (upgraded) attribute
   * has not been defined before, this list is somewhat degenerate,
   * and will need fixup at runtime.
   */
   if (save->copied.nr) {
      assert(save->copied.buffer);
   const fi_type *data = save->copied.buffer;
   grow_vertex_storage(ctx, save->copied.nr);
            /* Need to note this and fix up later. This can be done in
   * ATTR_UNION (by copying the new attribute values to the
   * vertices we're copying here) or at runtime (or loopback).
   */
   if (attr != VBO_ATTRIB_POS && save->currentsz[attr][0] == 0) {
      assert(oldsz == 0);
               for (i = 0; i < save->copied.nr; i++) {
      GLbitfield64 enabled = save->enabled;
   while (enabled) {
      const int j = u_bit_scan64(&enabled);
   assert(save->attrsz[j]);
   if (j == attr) {
      int k;
   const fi_type *src = oldsz ? data : save->current[attr];
   int copy = oldsz ? oldsz : newsz;
   for (k = 0; k < copy; k++)
         for (; k < newsz; k++) {
      switch (save->attrtype[j]) {
      case GL_FLOAT:
      dest[k] = FLOAT_AS_UNION(k == 3);
      case GL_INT:
      dest[k] = INT_AS_UNION(k == 3);
      case GL_UNSIGNED_INT:
      dest[k] = UINT_AS_UNION(k == 3);
      default:
      dest[k] = FLOAT_AS_UNION(k == 3);
   assert(!"Unexpected type in upgrade_vertex");
      }
   dest += newsz;
      } else {
      GLint sz = save->attrsz[j];
   for (int k = 0; k < sz; k++)
         data += sz;
                     save->vertex_store->used += save->vertex_size * save->copied.nr;
   free(save->copied.buffer);
         }
         /**
   * This is called when the size of a vertex attribute changes.
   * For example, after seeing one or more glTexCoord2f() calls we
   * get a glTexCoord4f() or glTexCoord1f() call.
   */
   static bool
   fixup_vertex(struct gl_context *ctx, GLuint attr,
         {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
            if (new_attr_is_bigger ||
      newType != save->attrtype[attr]) {
   /* New size is larger.  Need to flush existing vertices and get
   * an enlarged vertex format.
   */
      }
   else if (sz < save->active_sz[attr]) {
      GLuint i;
            /* New size is equal or smaller - just need to fill in some
   * zeros.
   */
   for (i = sz; i <= save->attrsz[attr]; i++)
                                    }
         /**
   * Reset the current size of all vertex attributes to the default
   * value of 0.  This signals that we haven't yet seen any per-vertex
   * commands such as glNormal3f() or glTexCoord2f().
   */
   static void
   reset_vertex(struct gl_context *ctx)
   {
               while (save->enabled) {
      const int i = u_bit_scan64(&save->enabled);
   assert(save->attrsz[i]);
   save->attrsz[i] = 0;
                  }
         /**
   * If index=0, does glVertexAttrib*() alias glVertex() to emit a vertex?
   * It depends on a few things, including whether we're inside or outside
   * of glBegin/glEnd.
   */
   static inline bool
   is_vertex_position(const struct gl_context *ctx, GLuint index)
   {
      return (index == 0 &&
            }
            #define ERROR(err)   _mesa_compile_error(ctx, err, __func__);
         /* Only one size for each attribute may be active at once.  Eg. if
   * Color3f is installed/active, then Color4f may not be, even if the
   * vertex actually contains 4 color coordinates.  This is because the
   * 3f version won't otherwise set color[3] to 1.0 -- this is the job
   * of the chooser function when switching between Color4f and Color3f.
   */
   #define ATTR_UNION(A, N, T, C, V0, V1, V2, V3)                  \
   do {                                                            \
      struct vbo_save_context *save = &vbo_context(ctx)->save;     \
   int sz = (sizeof(C) / sizeof(GLfloat));                      \
         if (save->active_sz[A] != N) {                               \
      bool had_dangling_ref = save->dangling_attr_ref;          \
   if (fixup_vertex(ctx, A, N * sz, T) &&                    \
      !had_dangling_ref && save->dangling_attr_ref &&       \
   A != VBO_ATTRIB_POS) {                                \
   fi_type *dest = save->vertex_store->buffer_in_ram;     \
   /* Copy the new attr values to the already copied      \
   * vertices.                                           \
   */                                                    \
   for (int i = 0; i < save->copied.nr; i++) {            \
      GLbitfield64 enabled = save->enabled;               \
   while (enabled) {                                   \
      const int j = u_bit_scan64(&enabled);            \
   if (j == A) {                                    \
      if (N>0) ((C*) dest)[0] = V0;                 \
   if (N>1) ((C*) dest)[1] = V1;                 \
   if (N>2) ((C*) dest)[2] = V2;                 \
      }                                                \
         }                                                      \
         }                                                            \
         {                                                            \
      C *dest = (C *)save->attrptr[A];                          \
   if (N>0) dest[0] = V0;                                    \
   if (N>1) dest[1] = V1;                                    \
   if (N>2) dest[2] = V2;                                    \
   if (N>3) dest[3] = V3;                                    \
      }                                                            \
         if ((A) == VBO_ATTRIB_POS) {                                 \
      fi_type *buffer_ptr = save->vertex_store->buffer_in_ram + \
               for (int i = 0; i < save->vertex_size; i++)               \
   buffer_ptr[i] = save->vertex[i];                        \
         save->vertex_store->used += save->vertex_size;            \
   unsigned used_next = (save->vertex_store->used +          \
         if (used_next > save->vertex_store->buffer_in_ram_size)   \
         } while (0)
      #define TAG(x) _save_##x
      #include "vbo_attrib_tmp.h"
         #define MAT( ATTR, N, face, params )                            \
   do {                                                            \
      if (face != GL_BACK)                                         \
         if (face != GL_FRONT)                                        \
      } while (0)
         /**
   * Save a glMaterial call found between glBegin/End.
   * glMaterial calls outside Begin/End are handled in dlist.c.
   */
   static void GLAPIENTRY
   _save_Materialfv(GLenum face, GLenum pname, const GLfloat *params)
   {
               if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "glMaterial(face)");
               switch (pname) {
   case GL_EMISSION:
      MAT(VBO_ATTRIB_MAT_FRONT_EMISSION, 4, face, params);
      case GL_AMBIENT:
      MAT(VBO_ATTRIB_MAT_FRONT_AMBIENT, 4, face, params);
      case GL_DIFFUSE:
      MAT(VBO_ATTRIB_MAT_FRONT_DIFFUSE, 4, face, params);
      case GL_SPECULAR:
      MAT(VBO_ATTRIB_MAT_FRONT_SPECULAR, 4, face, params);
      case GL_SHININESS:
      if (*params < 0 || *params > ctx->Const.MaxShininess) {
         }
   else {
         }
      case GL_COLOR_INDEXES:
      MAT(VBO_ATTRIB_MAT_FRONT_INDEXES, 3, face, params);
      case GL_AMBIENT_AND_DIFFUSE:
      MAT(VBO_ATTRIB_MAT_FRONT_AMBIENT, 4, face, params);
   MAT(VBO_ATTRIB_MAT_FRONT_DIFFUSE, 4, face, params);
      default:
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "glMaterial(pname)");
         }
         static void
   vbo_init_dispatch_save_begin_end(struct gl_context *ctx);
         /* Cope with EvalCoord/CallList called within a begin/end object:
   *     -- Flush current buffer
   *     -- Fallback to opcodes for the rest of the begin/end object.
   */
   static void
   dlist_fallback(struct gl_context *ctx)
   {
               if (save->vertex_store->used || save->prim_store->used) {
      if (save->prim_store->used > 0 && save->vertex_store->used > 0) {
      assert(save->vertex_size);
   /* Close off in-progress primitive. */
   GLint i = save->prim_store->used - 1;
   save->prim_store->prims[i].count =
      get_vertex_count(save) -
            /* Need to replay this display list with loopback,
   * unfortunately, otherwise this primitive won't be handled
   * properly:
   */
                        copy_to_current(ctx);
   reset_vertex(ctx);
   if (save->out_of_memory) {
         }
   else {
         }
      }
         static void GLAPIENTRY
   _save_EvalCoord1f(GLfloat u)
   {
      GET_CURRENT_CONTEXT(ctx);
   dlist_fallback(ctx);
      }
      static void GLAPIENTRY
   _save_EvalCoord1fv(const GLfloat * v)
   {
      GET_CURRENT_CONTEXT(ctx);
   dlist_fallback(ctx);
      }
      static void GLAPIENTRY
   _save_EvalCoord2f(GLfloat u, GLfloat v)
   {
      GET_CURRENT_CONTEXT(ctx);
   dlist_fallback(ctx);
      }
      static void GLAPIENTRY
   _save_EvalCoord2fv(const GLfloat * v)
   {
      GET_CURRENT_CONTEXT(ctx);
   dlist_fallback(ctx);
      }
      static void GLAPIENTRY
   _save_EvalPoint1(GLint i)
   {
      GET_CURRENT_CONTEXT(ctx);
   dlist_fallback(ctx);
      }
      static void GLAPIENTRY
   _save_EvalPoint2(GLint i, GLint j)
   {
      GET_CURRENT_CONTEXT(ctx);
   dlist_fallback(ctx);
      }
      static void GLAPIENTRY
   _save_CallList(GLuint l)
   {
      GET_CURRENT_CONTEXT(ctx);
   dlist_fallback(ctx);
      }
      static void GLAPIENTRY
   _save_CallLists(GLsizei n, GLenum type, const GLvoid * v)
   {
      GET_CURRENT_CONTEXT(ctx);
   dlist_fallback(ctx);
      }
            /**
   * Called when a glBegin is getting compiled into a display list.
   * Updating of ctx->Driver.CurrentSavePrimitive is already taken care of.
   */
   void
   vbo_save_NotifyBegin(struct gl_context *ctx, GLenum mode,
         {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
                     if (!save->prim_store || i >= save->prim_store->size) {
         }
   save->prim_store->prims[i].mode = mode & VBO_SAVE_PRIM_MODE_MASK;
   save->prim_store->prims[i].begin = 1;
   save->prim_store->prims[i].end = 0;
   save->prim_store->prims[i].start = get_vertex_count(save);
                              /* We need to call vbo_save_SaveFlushVertices() if there's state change */
      }
         static void GLAPIENTRY
   _save_End(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct vbo_save_context *save = &vbo_context(ctx)->save;
            ctx->Driver.CurrentSavePrimitive = PRIM_OUTSIDE_BEGIN_END;
   save->prim_store->prims[i].end = 1;
            /* Swap out this vertex format while outside begin/end.  Any color,
   * etc. received between here and the next begin will be compiled
   * as opcodes.
   */
   if (save->out_of_memory) {
         }
   else {
            }
         static void GLAPIENTRY
   _save_Begin(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   (void) mode;
      }
         static void GLAPIENTRY
   _save_PrimitiveRestartNV(void)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (save->prim_store->used == 0) {
      /* We're not inside a glBegin/End pair, so calling glPrimitiverRestartNV
   * is an error.
   */
   _mesa_compile_error(ctx, GL_INVALID_OPERATION,
      } else {
      /* get current primitive mode */
   GLenum curPrim = save->prim_store->prims[save->prim_store->used - 1].mode;
            /* restart primitive */
   CALL_End(ctx->Dispatch.Current, ());
         }
         void GLAPIENTRY
   save_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
   {
      GET_CURRENT_CONTEXT(ctx);
            vbo_save_NotifyBegin(ctx, GL_QUADS, false);
   CALL_Vertex2f(dispatch, (x1, y1));
   CALL_Vertex2f(dispatch, (x2, y1));
   CALL_Vertex2f(dispatch, (x2, y2));
   CALL_Vertex2f(dispatch, (x1, y2));
      }
         void GLAPIENTRY
   save_Rectdv(const GLdouble *v1, const GLdouble *v2)
   {
         }
      void GLAPIENTRY
   save_Rectfv(const GLfloat *v1, const GLfloat *v2)
   {
         }
      void GLAPIENTRY
   save_Recti(GLint x1, GLint y1, GLint x2, GLint y2)
   {
         }
      void GLAPIENTRY
   save_Rectiv(const GLint *v1, const GLint *v2)
   {
         }
      void GLAPIENTRY
   save_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
   {
         }
      void GLAPIENTRY
   save_Rectsv(const GLshort *v1, const GLshort *v2)
   {
         }
      void GLAPIENTRY
   save_DrawArrays(GLenum mode, GLint start, GLsizei count)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_vertex_array_object *vao = ctx->Array.VAO;
   struct vbo_save_context *save = &vbo_context(ctx)->save;
            if (!_mesa_is_valid_prim_mode(ctx, mode)) {
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "glDrawArrays(mode)");
      }
   if (count < 0) {
      _mesa_compile_error(ctx, GL_INVALID_VALUE, "glDrawArrays(count<0)");
               if (save->out_of_memory)
                     /* Make sure to process any VBO binding changes */
                              for (i = 0; i < count; i++)
                     }
         void GLAPIENTRY
   save_MultiDrawArrays(GLenum mode, const GLint *first,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (!_mesa_is_valid_prim_mode(ctx, mode)) {
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "glMultiDrawArrays(mode)");
               if (primcount < 0) {
      _mesa_compile_error(ctx, GL_INVALID_VALUE,
                     unsigned vertcount = 0;
   for (i = 0; i < primcount; i++) {
      if (count[i] < 0) {
      _mesa_compile_error(ctx, GL_INVALID_VALUE,
            }
                        for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
               }
         static void
   array_element(struct gl_context *ctx,
         {
      /* Section 10.3.5 Primitive Restart:
   * [...]
   *    When one of the *BaseVertex drawing commands specified in section 10.5
   * is used, the primitive restart comparison occurs before the basevertex
   * offset is added to the array index.
   */
   /* If PrimitiveRestart is enabled and the index is the RestartIndex
   * then we call PrimitiveRestartNV and return.
   */
   if (ctx->Array._PrimitiveRestart[index_size_shift] &&
      elt == ctx->Array._RestartIndex[index_size_shift]) {
   CALL_PrimitiveRestartNV(ctx->Dispatch.Current, ());
                  }
         /* Could do better by copying the arrays and element list intact and
   * then emitting an indexed prim at runtime.
   */
   void GLAPIENTRY
   save_DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct vbo_save_context *save = &vbo_context(ctx)->save;
   struct gl_vertex_array_object *vao = ctx->Array.VAO;
   struct gl_buffer_object *indexbuf = vao->IndexBufferObj;
            if (!_mesa_is_valid_prim_mode(ctx, mode)) {
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "glDrawElements(mode)");
      }
   if (count < 0) {
      _mesa_compile_error(ctx, GL_INVALID_VALUE, "glDrawElements(count<0)");
      }
   if (type != GL_UNSIGNED_BYTE &&
      type != GL_UNSIGNED_SHORT &&
   type != GL_UNSIGNED_INT) {
   _mesa_compile_error(ctx, GL_INVALID_VALUE, "glDrawElements(count<0)");
               if (save->out_of_memory)
                     /* Make sure to process any VBO binding changes */
                     if (indexbuf)
      indices =
                  switch (type) {
   case GL_UNSIGNED_BYTE:
      for (i = 0; i < count; i++)
            case GL_UNSIGNED_SHORT:
      for (i = 0; i < count; i++)
            case GL_UNSIGNED_INT:
      for (i = 0; i < count; i++)
            default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawElements(type)");
                           }
      void GLAPIENTRY
   save_DrawElements(GLenum mode, GLsizei count, GLenum type,
         {
         }
         void GLAPIENTRY
   save_DrawRangeElements(GLenum mode, GLuint start, GLuint end,
               {
      GET_CURRENT_CONTEXT(ctx);
            if (!_mesa_is_valid_prim_mode(ctx, mode)) {
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "glDrawRangeElements(mode)");
      }
   if (count < 0) {
      _mesa_compile_error(ctx, GL_INVALID_VALUE,
            }
   if (type != GL_UNSIGNED_BYTE &&
      type != GL_UNSIGNED_SHORT &&
   type != GL_UNSIGNED_INT) {
   _mesa_compile_error(ctx, GL_INVALID_ENUM, "glDrawRangeElements(type)");
      }
   if (end < start) {
      _mesa_compile_error(ctx, GL_INVALID_VALUE,
                     if (save->out_of_memory)
               }
      void GLAPIENTRY
   save_DrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end,
               {
               if (end < start) {
      _mesa_compile_error(ctx, GL_INVALID_VALUE,
                        }
      void GLAPIENTRY
   save_MultiDrawElements(GLenum mode, const GLsizei *count, GLenum type,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct _glapi_table *dispatch = ctx->Dispatch.Current;
            int vertcount = 0;
   for (i = 0; i < primcount; i++) {
         }
            for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
               }
         void GLAPIENTRY
   save_MultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count,
                           {
      GET_CURRENT_CONTEXT(ctx);
   struct _glapi_table *dispatch = ctx->Dispatch.Current;
            int vertcount = 0;
   for (i = 0; i < primcount; i++) {
         }
            for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
      CALL_DrawElementsBaseVertex(dispatch, (mode, count[i], type,
                  }
         static void
   vbo_init_dispatch_save_begin_end(struct gl_context *ctx)
   {
   #define NAME_AE(x) _mesa_##x
   #define NAME_CALLLIST(x) _save_##x
   #define NAME(x) _save_##x
   #define NAME_ES(x) _save_##x
         struct _glapi_table *tab = ctx->Dispatch.Save;
      }
         void
   vbo_save_SaveFlushVertices(struct gl_context *ctx)
   {
               /* Noop when we are actually active:
   */
   if (ctx->Driver.CurrentSavePrimitive <= PRIM_MAX)
            if (save->vertex_store->used || save->prim_store->used)
            copy_to_current(ctx);
   reset_vertex(ctx);
      }
         /**
   * Called from glNewList when we're starting to compile a display list.
   */
   void
   vbo_save_NewList(struct gl_context *ctx, GLuint list, GLenum mode)
   {
               (void) list;
            if (!save->prim_store)
            if (!save->vertex_store)
            reset_vertex(ctx);
      }
         /**
   * Called from glEndList when we're finished compiling a display list.
   */
   void
   vbo_save_EndList(struct gl_context *ctx)
   {
               /* EndList called inside a (saved) Begin/End pair?
   */
   if (_mesa_inside_dlist_begin_end(ctx)) {
      if (save->prim_store->used > 0) {
      GLint i = save->prim_store->used - 1;
   ctx->Driver.CurrentSavePrimitive = PRIM_OUTSIDE_BEGIN_END;
   save->prim_store->prims[i].end = 0;
               /* Make sure this vertex list gets replayed by the "loopback"
   * mechanism:
   */
   save->dangling_attr_ref = GL_TRUE;
            /* Swap out this vertex format while outside begin/end.  Any color,
   * etc. received between here and the next begin will be compiled
   * as opcodes.
   */
                  }
      /**
   * Called during context creation/init.
   */
   static void
   current_init(struct gl_context *ctx)
   {
      struct vbo_save_context *save = &vbo_context(ctx)->save;
            for (i = VBO_ATTRIB_POS; i <= VBO_ATTRIB_EDGEFLAG; i++) {
      save->currentsz[i] = &ctx->ListState.ActiveAttribSize[i];
               for (i = VBO_ATTRIB_FIRST_MATERIAL; i <= VBO_ATTRIB_LAST_MATERIAL; i++) {
      const GLuint j = i - VBO_ATTRIB_FIRST_MATERIAL;
   assert(j < MAT_ATTRIB_MAX);
   save->currentsz[i] = &ctx->ListState.ActiveMaterialSize[j];
         }
         /**
   * Initialize the display list compiler.  Called during context creation.
   */
   void
   vbo_save_api_init(struct vbo_save_context *save)
   {
                  }
