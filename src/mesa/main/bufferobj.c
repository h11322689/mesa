   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
         /**
   * \file bufferobj.c
   * \brief Functions for the GL_ARB_vertex/pixel_buffer_object extensions.
   * \author Brian Paul, Ian Romanick
   */
      #include <stdbool.h>
   #include <inttypes.h>  /* for PRId64 macro */
   #include "util/u_debug.h"
   #include "util/glheader.h"
   #include "enums.h"
   #include "hash.h"
   #include "context.h"
   #include "bufferobj.h"
   #include "externalobjects.h"
   #include "mtypes.h"
   #include "teximage.h"
   #include "glformats.h"
   #include "texstore.h"
   #include "transformfeedback.h"
   #include "varray.h"
   #include "util/u_atomic.h"
   #include "util/u_memory.h"
   #include "api_exec_decl.h"
   #include "util/set.h"
      #include "state_tracker/st_debug.h"
   #include "state_tracker/st_atom.h"
   #include "frontend/api.h"
      #include "util/u_inlines.h"
   /* Debug flags */
   /*#define VBO_DEBUG*/
   /*#define BOUNDS_CHECK*/
         /**
   * We count the number of buffer modification calls to check for
   * inefficient buffer use.  This is the number of such calls before we
   * issue a warning.
   */
   #define BUFFER_WARNING_CALL_COUNT 4
         /**
   * Replace data in a subrange of buffer object.  If the data range
   * specified by size + offset extends beyond the end of the buffer or
   * if data is NULL, no copy is performed.
   * Called via glBufferSubDataARB().
   */
   void
   _mesa_bufferobj_subdata(struct gl_context *ctx,
                     {
      /* we may be called from VBO code, so double-check params here */
   assert(offset >= 0);
   assert(size >= 0);
            if (!size)
            /*
   * According to ARB_vertex_buffer_object specification, if data is null,
   * then the contents of the buffer object's data store is undefined. We just
   * ignore, and leave it unchanged.
   */
   if (!data)
            if (!obj->buffer) {
      /* we probably ran out of memory during buffer allocation */
               /* Now that transfers are per-context, we don't have to figure out
   * flushing here.  Usually drivers won't need to flush in this case
   * even if the buffer is currently referenced by hardware - they
   * just queue the upload as dma rather than mapping the underlying
   * buffer directly.
   *
   * If the buffer is mapped, suppress implicit buffer range invalidation
   * by using PIPE_MAP_DIRECTLY.
   */
            pipe->buffer_subdata(pipe, obj->buffer,
                  }
         /**
   * Called via glGetBufferSubDataARB().
   */
   static void
   bufferobj_get_subdata(struct gl_context *ctx,
                     {
      /* we may be called from VBO code, so double-check params here */
   assert(offset >= 0);
   assert(size >= 0);
            if (!size)
            if (!obj->buffer) {
      /* we probably ran out of memory during buffer allocation */
               pipe_buffer_read(ctx->pipe, obj->buffer,
      }
      void
   _mesa_bufferobj_get_subdata(struct gl_context *ctx,
                     {
         }
      /**
   * Return bitmask of PIPE_BIND_x flags corresponding a GL buffer target.
   */
   static unsigned
   buffer_target_to_bind_flags(GLenum target)
   {
      switch (target) {
   case GL_PIXEL_PACK_BUFFER_ARB:
   case GL_PIXEL_UNPACK_BUFFER_ARB:
         case GL_ARRAY_BUFFER_ARB:
         case GL_ELEMENT_ARRAY_BUFFER_ARB:
         case GL_TEXTURE_BUFFER:
         case GL_TRANSFORM_FEEDBACK_BUFFER:
         case GL_UNIFORM_BUFFER:
         case GL_DRAW_INDIRECT_BUFFER:
   case GL_PARAMETER_BUFFER_ARB:
         case GL_ATOMIC_COUNTER_BUFFER:
   case GL_SHADER_STORAGE_BUFFER:
         case GL_QUERY_BUFFER:
         default:
            }
         /**
   * Return bitmask of PIPE_RESOURCE_x flags corresponding to GL_MAP_x flags.
   */
   static unsigned
   storage_flags_to_buffer_flags(GLbitfield storageFlags)
   {
      unsigned flags = 0;
   if (storageFlags & GL_MAP_PERSISTENT_BIT)
         if (storageFlags & GL_MAP_COHERENT_BIT)
         if (storageFlags & GL_SPARSE_STORAGE_BIT_ARB)
            }
         /**
   * From a buffer object's target, immutability flag, storage flags and
   * usage hint, return a pipe_resource_usage value (PIPE_USAGE_DYNAMIC,
   * STREAM, etc).
   */
   static enum pipe_resource_usage
   buffer_usage(GLenum target, GLboolean immutable,
         {
      /* "immutable" means that "storageFlags" was set by the user and "usage"
   * was guessed by Mesa. Otherwise, "usage" was set by the user and
   * storageFlags was guessed by Mesa.
   *
   * Therefore, use storageFlags with immutable, else use "usage".
   */
   if (immutable) {
      /* BufferStorage */
   if (storageFlags & GL_MAP_READ_BIT)
         else if (storageFlags & GL_CLIENT_STORAGE_BIT)
         else
      }
   else {
      /* These are often read by the CPU, so enable CPU caches. */
   if (target == GL_PIXEL_PACK_BUFFER ||
                  /* BufferData */
   switch (usage) {
   case GL_DYNAMIC_DRAW:
   case GL_DYNAMIC_COPY:
         case GL_STREAM_DRAW:
   case GL_STREAM_COPY:
         case GL_STATIC_READ:
   case GL_DYNAMIC_READ:
   case GL_STREAM_READ:
         case GL_STATIC_DRAW:
   case GL_STATIC_COPY:
   default:
               }
         static ALWAYS_INLINE GLboolean
   bufferobj_data(struct gl_context *ctx,
                  GLenum target,
   GLsizeiptrARB size,
   const void *data,
   struct gl_memory_object *memObj,
   GLuint64 offset,
      {
      struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
            if (size > UINT32_MAX || offset > UINT32_MAX) {
      /* pipe_resource.width0 is 32 bits only and increasing it
   * to 64 bits doesn't make much sense since hw support
   * for > 4GB resources is limited.
   */
   obj->Size = 0;
               if (target != GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD &&
      size && obj->buffer &&
   obj->Size == size &&
   obj->Usage == usage &&
   obj->StorageFlags == storageFlags) {
   if (data) {
      /* Just discard the old contents and write new data.
   * This should be the same as creating a new buffer, but we avoid
   * a lot of validation in Mesa.
   *
   * If the buffer is mapped, we can't discard it.
   *
   * PIPE_MAP_DIRECTLY supresses implicit buffer range
   * invalidation.
   */
   pipe->buffer_subdata(pipe, obj->buffer,
                        } else if (is_mapped) {
         } else if (screen->get_param(screen, PIPE_CAP_INVALIDATE_BUFFER)) {
      pipe->invalidate_resource(pipe, obj->buffer);
                  obj->Size = size;
   obj->Usage = usage;
                              if (storageFlags & MESA_GALLIUM_VERTEX_STATE_STORAGE)
            if (ST_DEBUG & DEBUG_BUFFER) {
      debug_printf("Create buffer size %" PRId64 " bind 0x%x\n",
               if (size != 0) {
               memset(&buffer, 0, sizeof buffer);
   buffer.target = PIPE_BUFFER;
   buffer.format = PIPE_FORMAT_R8_UNORM; /* want TYPELESS or similar */
   buffer.bind = bindings;
   buffer.usage =
         buffer.flags = storage_flags_to_buffer_flags(storageFlags);
   buffer.width0 = size;
   buffer.height0 = 1;
   buffer.depth0 = 1;
            if (memObj) {
      obj->buffer = screen->resource_from_memobj(screen, &buffer,
            }
   else if (target == GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD) {
      obj->buffer =
      }
   else {
               if (obj->buffer && data)
               if (!obj->buffer) {
      /* out of memory */
   obj->Size = 0;
                           /* The current buffer may be bound, so we have to revalidate all atoms that
   * might be using it.
   */
   if (obj->UsageHistory & USAGE_ARRAY_BUFFER)
         if (obj->UsageHistory & USAGE_UNIFORM_BUFFER)
         if (obj->UsageHistory & USAGE_SHADER_STORAGE_BUFFER)
         if (obj->UsageHistory & USAGE_TEXTURE_BUFFER)
         if (obj->UsageHistory & USAGE_ATOMIC_COUNTER_BUFFER)
               }
      /**
   * Allocate space for and store data in a buffer object.  Any data that was
   * previously stored in the buffer object is lost.  If data is NULL,
   * memory will be allocated, but no copy will occur.
   * Called via ctx->Driver.BufferData().
   * \return GL_TRUE for success, GL_FALSE if out of memory
   */
   GLboolean
   _mesa_bufferobj_data(struct gl_context *ctx,
                     GLenum target,
   GLsizeiptrARB size,
   const void *data,
   {
         }
      static GLboolean
   bufferobj_data_mem(struct gl_context *ctx,
                     GLenum target,
   GLsizeiptrARB size,
   struct gl_memory_object *memObj,
   {
         }
      /**
   * Convert GLbitfield of GL_MAP_x flags to gallium pipe_map_flags flags.
   * \param wholeBuffer  is the whole buffer being mapped?
   */
   enum pipe_map_flags
   _mesa_access_flags_to_transfer_flags(GLbitfield access, bool wholeBuffer)
   {
               if (access & GL_MAP_WRITE_BIT)
            if (access & GL_MAP_READ_BIT)
            if (access & GL_MAP_FLUSH_EXPLICIT_BIT)
            if (access & GL_MAP_INVALIDATE_BUFFER_BIT) {
         }
   else if (access & GL_MAP_INVALIDATE_RANGE_BIT) {
      if (wholeBuffer)
         else
               if (access & GL_MAP_UNSYNCHRONIZED_BIT)
            if (access & GL_MAP_PERSISTENT_BIT)
            if (access & GL_MAP_COHERENT_BIT)
            /* ... other flags ...
            if (access & MESA_MAP_NOWAIT_BIT)
         if (access & MESA_MAP_THREAD_SAFE_BIT)
         if (access & MESA_MAP_ONCE)
               }
         /**
   * Called via glMapBufferRange().
   */
   void *
   _mesa_bufferobj_map_range(struct gl_context *ctx,
                     {
               assert(offset >= 0);
   assert(length >= 0);
   assert(offset < obj->Size);
            enum pipe_map_flags transfer_flags =
      _mesa_access_flags_to_transfer_flags(access,
         /* Sometimes games do silly things like MapBufferRange(UNSYNC|DISCARD_RANGE)
   * In this case, the the UNSYNC is a bit redundant, but the games rely
   * on the driver rebinding/replacing the backing storage rather than
   * going down the UNSYNC path (ie. honoring DISCARD_x first before UNSYNC).
   */
   if (unlikely(ctx->st_opts->ignore_map_unsynchronized)) {
      if (transfer_flags & (PIPE_MAP_DISCARD_RANGE | PIPE_MAP_DISCARD_WHOLE_RESOURCE))
               if (ctx->Const.ForceMapBufferSynchronized)
            obj->Mappings[index].Pointer = pipe_buffer_map_range(pipe,
                           if (obj->Mappings[index].Pointer) {
      obj->Mappings[index].Offset = offset;
   obj->Mappings[index].Length = length;
      }
   else {
                     }
         void
   _mesa_bufferobj_flush_mapped_range(struct gl_context *ctx,
                     {
               /* Subrange is relative to mapped range */
   assert(offset >= 0);
   assert(length >= 0);
   assert(offset + length <= obj->Mappings[index].Length);
            if (!length)
            pipe_buffer_flush_mapped_range(pipe, obj->transfer[index],
            }
         /**
   * Called via glUnmapBufferARB().
   */
   GLboolean
   _mesa_bufferobj_unmap(struct gl_context *ctx, struct gl_buffer_object *obj,
         {
               if (obj->Mappings[index].Length)
            obj->transfer[index] = NULL;
   obj->Mappings[index].Pointer = NULL;
   obj->Mappings[index].Offset = 0;
   obj->Mappings[index].Length = 0;
      }
         /**
   * Called via glCopyBufferSubData().
   */
   static void
   bufferobj_copy_subdata(struct gl_context *ctx,
                           {
      struct pipe_context *pipe = ctx->pipe;
            dst->MinMaxCacheDirty = true;
   if (!size)
            /* buffer should not already be mapped */
   assert(!_mesa_check_disallowed_mapping(src));
                     pipe->resource_copy_region(pipe, dst->buffer, 0, writeOffset, 0, 0,
      }
      static void
   clear_buffer_subdata_sw(struct gl_context *ctx,
                           {
      GLsizeiptr i;
            dest = _mesa_bufferobj_map_range(ctx, offset, size,
                        if (!dest) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glClearBuffer[Sub]Data");
               if (clearValue == NULL) {
      /* Clear with zeros, per the spec */
   memset(dest, 0, size);
   _mesa_bufferobj_unmap(ctx, bufObj, MAP_INTERNAL);
               for (i = 0; i < size/clearValueSize; ++i) {
      memcpy(dest, clearValue, clearValueSize);
                  }
      /**
   * Helper to warn of possible performance issues, such as frequently
   * updating a buffer created with GL_STATIC_DRAW.  Called via the macro
   * below.
   */
   static void
   buffer_usage_warning(struct gl_context *ctx, GLuint *id, const char *fmt, ...)
   {
               va_start(args, fmt);
   _mesa_gl_vdebugf(ctx, id,
                  MESA_DEBUG_SOURCE_API,
         }
      #define BUFFER_USAGE_WARNING(CTX, FMT, ...) \
      do { \
      static GLuint id = 0; \
               /**
   * Used as a placeholder for buffer objects between glGenBuffers() and
   * glBindBuffer() so that glIsBuffer() can work correctly.
   */
   static struct gl_buffer_object DummyBufferObject = {
      .MinMaxCacheMutex = SIMPLE_MTX_INITIALIZER,
      };
         /**
   * Return pointer to address of a buffer object target.
   * \param ctx  the GL context
   * \param target  the buffer object target to be retrieved.
   * \return   pointer to pointer to the buffer object bound to \c target in the
   *           specified context or \c NULL if \c target is invalid.
   */
   static ALWAYS_INLINE struct gl_buffer_object **
   get_buffer_target(struct gl_context *ctx, GLenum target, bool no_error)
   {
      /* Other targets are only supported in desktop OpenGL and OpenGL ES 3.0. */
   if (!no_error && !_mesa_is_desktop_gl(ctx) && !_mesa_is_gles3(ctx)) {
      switch (target) {
   case GL_ARRAY_BUFFER:
   case GL_ELEMENT_ARRAY_BUFFER:
   case GL_PIXEL_PACK_BUFFER:
   case GL_PIXEL_UNPACK_BUFFER:
         default:
                     switch (target) {
   case GL_ARRAY_BUFFER_ARB:
         case GL_ELEMENT_ARRAY_BUFFER_ARB:
         case GL_PIXEL_PACK_BUFFER_EXT:
         case GL_PIXEL_UNPACK_BUFFER_EXT:
         case GL_COPY_READ_BUFFER:
         case GL_COPY_WRITE_BUFFER:
         case GL_QUERY_BUFFER:
      if (no_error || _mesa_has_ARB_query_buffer_object(ctx))
            case GL_DRAW_INDIRECT_BUFFER:
      if (no_error ||
      (_mesa_is_desktop_gl(ctx) && ctx->Extensions.ARB_draw_indirect) ||
   _mesa_is_gles31(ctx)) {
      }
      case GL_PARAMETER_BUFFER_ARB:
      if (no_error || _mesa_has_ARB_indirect_parameters(ctx)) {
         }
      case GL_DISPATCH_INDIRECT_BUFFER:
      if (no_error || _mesa_has_compute_shaders(ctx)) {
         }
      case GL_TRANSFORM_FEEDBACK_BUFFER:
      if (no_error || ctx->Extensions.EXT_transform_feedback) {
         }
      case GL_TEXTURE_BUFFER:
      if (no_error ||
      _mesa_has_ARB_texture_buffer_object(ctx) ||
   _mesa_has_OES_texture_buffer(ctx)) {
      }
      case GL_UNIFORM_BUFFER:
      if (no_error || ctx->Extensions.ARB_uniform_buffer_object) {
         }
      case GL_SHADER_STORAGE_BUFFER:
      if (no_error ||
      ctx->Extensions.ARB_shader_storage_buffer_object ||
   _mesa_is_gles31(ctx)) {
      }
      case GL_ATOMIC_COUNTER_BUFFER:
      if (no_error ||
      ctx->Extensions.ARB_shader_atomic_counters || _mesa_is_gles31(ctx)) {
      }
      case GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD:
      if (no_error || ctx->Extensions.AMD_pinned_memory) {
         }
      }
      }
         /**
   * Get the buffer object bound to the specified target in a GL context.
   * \param ctx  the GL context
   * \param target  the buffer object target to be retrieved.
   * \param error  the GL error to record if target is illegal.
   * \return   pointer to the buffer object bound to \c target in the
   *           specified context or \c NULL if \c target is invalid.
   */
   static inline struct gl_buffer_object *
   get_buffer(struct gl_context *ctx, const char *func, GLenum target,
         {
               if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(target)", func);
               if (!*bufObj) {
      _mesa_error(ctx, error, "%s(no buffer bound)", func);
                  }
         /**
   * Convert a GLbitfield describing the mapped buffer access flags
   * into one of GL_READ_WRITE, GL_READ_ONLY, or GL_WRITE_ONLY.
   */
   static GLenum
   simplified_access_mode(struct gl_context *ctx, GLbitfield access)
   {
      const GLbitfield rwFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
   if ((access & rwFlags) == rwFlags)
         if ((access & GL_MAP_READ_BIT) == GL_MAP_READ_BIT)
         if ((access & GL_MAP_WRITE_BIT) == GL_MAP_WRITE_BIT)
            /* Otherwise, AccessFlags is zero (the default state).
   *
   * Table 2.6 on page 31 (page 44 of the PDF) of the OpenGL 1.5 spec says:
   *
   * Name           Type  Initial Value  Legal Values
   * ...            ...   ...            ...
   * BUFFER_ACCESS  enum  READ_WRITE     READ_ONLY, WRITE_ONLY
   *                                     READ_WRITE
   *
   * However, table 6.8 in the GL_OES_mapbuffer extension says:
   *
   * Get Value         Type Get Command          Value          Description
   * ---------         ---- -----------          -----          -----------
   * BUFFER_ACCESS_OES Z1   GetBufferParameteriv WRITE_ONLY_OES buffer map flag
   *
   * The difference is because GL_OES_mapbuffer only supports mapping buffers
   * write-only.
   */
               }
         /**
   * Test if the buffer is mapped, and if so, if the mapped range overlaps the
   * given range.
   * The regions do not overlap if and only if the end of the given
   * region is before the mapped region or the start of the given region
   * is after the mapped region.
   *
   * \param obj     Buffer object target on which to operate.
   * \param offset  Offset of the first byte of the subdata range.
   * \param size    Size, in bytes, of the subdata range.
   * \return   true if ranges overlap, false otherwise
   *
   */
   static bool
   bufferobj_range_mapped(const struct gl_buffer_object *obj,
         {
      if (_mesa_bufferobj_mapped(obj, MAP_USER)) {
      const GLintptr end = offset + size;
   const GLintptr mapEnd = obj->Mappings[MAP_USER].Offset +
            if (!(end <= obj->Mappings[MAP_USER].Offset || offset >= mapEnd)) {
            }
      }
         /**
   * Tests the subdata range parameters and sets the GL error code for
   * \c glBufferSubDataARB, \c glGetBufferSubDataARB and
   * \c glClearBufferSubData.
   *
   * \param ctx     GL context.
   * \param bufObj  The buffer object.
   * \param offset  Offset of the first byte of the subdata range.
   * \param size    Size, in bytes, of the subdata range.
   * \param mappedRange  If true, checks if an overlapping range is mapped.
   *                     If false, checks if buffer is mapped.
   * \param caller  Name of calling function for recording errors.
   * \return   false if error, true otherwise
   *
   * \sa glBufferSubDataARB, glGetBufferSubDataARB, glClearBufferSubData
   */
   static bool
   buffer_object_subdata_range_good(struct gl_context *ctx,
                     {
      if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(size < 0)", caller);
               if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(offset < 0)", caller);
               if (offset + size > bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
               "%s(offset %lu + size %lu > buffer size %lu)", caller,
   (unsigned long) offset,
               if (bufObj->Mappings[MAP_USER].AccessFlags & GL_MAP_PERSISTENT_BIT)
            if (mappedRange) {
      if (bufferobj_range_mapped(bufObj, offset, size)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     }
   else {
      if (_mesa_bufferobj_mapped(bufObj, MAP_USER)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                                 }
         /**
   * Test the format and type parameters and set the GL error code for
   * \c glClearBufferData, \c glClearNamedBufferData, \c glClearBufferSubData
   * and \c glClearNamedBufferSubData.
   *
   * \param ctx             GL context.
   * \param internalformat  Format to which the data is to be converted.
   * \param format          Format of the supplied data.
   * \param type            Type of the supplied data.
   * \param caller          Name of calling function for recording errors.
   * \return   If internalformat, format and type are legal the mesa_format
   *           corresponding to internalformat, otherwise MESA_FORMAT_NONE.
   *
   * \sa glClearBufferData, glClearNamedBufferData, glClearBufferSubData and
   *     glClearNamedBufferSubData.
   */
   static mesa_format
   validate_clear_buffer_format(struct gl_context *ctx,
                     {
      mesa_format mesaFormat;
            mesaFormat = _mesa_validate_texbuffer_format(ctx, internalformat);
   if (mesaFormat == MESA_FORMAT_NONE) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                     /* NOTE: not mentioned in ARB_clear_buffer_object but according to
   * EXT_texture_integer there is no conversion between integer and
   * non-integer formats
   */
   if (_mesa_is_enum_format_signed_int(format) !=
      _mesa_is_format_integer_color(mesaFormat)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (!_mesa_is_color_format(format)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     errorFormatType = _mesa_error_check_format_and_type(ctx, format, type);
   if (errorFormatType != GL_NO_ERROR) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                        }
         /**
   * Convert user-specified clear value to the specified internal format.
   *
   * \param ctx             GL context.
   * \param internalformat  Format to which the data is converted.
   * \param clearValue      Points to the converted clear value.
   * \param format          Format of the supplied data.
   * \param type            Type of the supplied data.
   * \param data            Data which is to be converted to internalformat.
   * \param caller          Name of calling function for recording errors.
   * \return   true if data could be converted, false otherwise.
   *
   * \sa glClearBufferData, glClearBufferSubData
   */
   static bool
   convert_clear_buffer_data(struct gl_context *ctx,
                     {
               if (_mesa_texstore(ctx, 1, internalformatBase, internalformat,
                     }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", caller);
         }
      void
   _mesa_bufferobj_release_buffer(struct gl_buffer_object *obj)
   {
      if (!obj->buffer)
            /* Subtract the remaining private references before unreferencing
   * the buffer. See the header file for explanation.
   */
   if (obj->private_refcount) {
      assert(obj->private_refcount > 0);
   p_atomic_add(&obj->buffer->reference.count,
            }
               }
      /**
   * Delete a buffer object.
   *
   * Default callback for the \c dd_function_table::DeleteBuffer() hook.
   */
   void
   _mesa_delete_buffer_object(struct gl_context *ctx,
         {
      assert(bufObj->RefCount == 0);
   _mesa_buffer_unmap_all_mappings(ctx, bufObj);
                     /* assign strange values here to help w/ debugging */
   bufObj->RefCount = -1000;
            simple_mtx_destroy(&bufObj->MinMaxCacheMutex);
   free(bufObj->Label);
      }
         /**
   * Get the value of MESA_NO_MINMAX_CACHE.
   */
   static bool
   get_no_minmax_cache()
   {
      static bool read = false;
            if (!read) {
      disable = debug_get_bool_option("MESA_NO_MINMAX_CACHE", false);
                  }
      /**
   * Callback called from _mesa_HashWalk()
   */
   static void
   count_buffer_size(void *data, void *userData)
   {
      const struct gl_buffer_object *bufObj =
                     }
         /**
   * Compute total size (in bytes) of all buffer objects for the given context.
   * For debugging purposes.
   */
   GLuint
   _mesa_total_buffer_object_memory(struct gl_context *ctx)
   {
               _mesa_HashWalkMaybeLocked(ctx->Shared->BufferObjects, count_buffer_size,
               }
      /**
   * Initialize the state associated with buffer objects
   */
   void
   _mesa_init_buffer_objects( struct gl_context *ctx )
   {
               for (i = 0; i < MAX_COMBINED_UNIFORM_BUFFERS; i++) {
      _mesa_reference_buffer_object(ctx,
   &ctx->UniformBufferBindings[i].BufferObject,
   NULL);
   ctx->UniformBufferBindings[i].Offset = -1;
               for (i = 0; i < MAX_COMBINED_SHADER_STORAGE_BUFFERS; i++) {
      _mesa_reference_buffer_object(ctx,
               ctx->ShaderStorageBufferBindings[i].Offset = -1;
               for (i = 0; i < MAX_COMBINED_ATOMIC_BUFFERS; i++) {
      _mesa_reference_buffer_object(ctx,
   &ctx->AtomicBufferBindings[i].BufferObject,
   NULL);
   ctx->AtomicBufferBindings[i].Offset = 0;
         }
      /**
   * Detach the context from the buffer to re-enable buffer reference counting
   * for this context.
   */
   static void
   detach_ctx_from_buffer(struct gl_context *ctx, struct gl_buffer_object *buf)
   {
               /* Move private non-atomic context references to the global ref count. */
   p_atomic_add(&buf->RefCount, buf->CtxRefCount);
   buf->CtxRefCount = 0;
            /* Remove the context reference where the context holds one
   * reference for the lifetime of the buffer ID to skip refcount
   * atomics instead of each binding point holding the reference.
   */
      }
      /**
   * Zombie buffers are buffers that were created by one context and deleted
   * by another context. The creating context holds a global reference for each
   * buffer it created that can't be unreferenced when another context deletes
   * it. Such a buffer becomes a zombie, which means that it's no longer usable
   * by OpenGL, but the creating context still holds its global reference of
   * the buffer. Only the creating context can remove the reference, which is
   * what this function does.
   *
   * For all zombie buffers, decrement the reference count if the current
   * context owns the buffer.
   */
   static void
   unreference_zombie_buffers_for_ctx(struct gl_context *ctx)
   {
      /* It's assumed that the mutex of Shared->BufferObjects is locked. */
   set_foreach(ctx->Shared->ZombieBufferObjects, entry) {
               if (buf->Ctx == ctx) {
      _mesa_set_remove(ctx->Shared->ZombieBufferObjects, entry);
            }
      /**
   * When a context creates buffers, it holds a global buffer reference count
   * for each buffer and doesn't update their RefCount. When the context is
   * destroyed before the buffers are destroyed, the context must remove
   * its global reference from the buffers, so that the buffers can live
   * on their own.
   *
   * At this point, the buffers shouldn't be bound in any bounding point owned
   * by the context. (it would crash if they did)
   */
   static void
   detach_unrefcounted_buffer_from_ctx(void *data, void *userData)
   {
      struct gl_context *ctx = (struct gl_context *)userData;
            if (buf->Ctx == ctx) {
      /* Detach the current context from live objects. There should be no
   * bound buffer in the context at this point, therefore we can just
   * unreference the global reference. Other contexts and texture objects
   * might still be using the buffer.
   */
   assert(buf->CtxRefCount == 0);
   buf->Ctx = NULL;
         }
      void
   _mesa_free_buffer_objects( struct gl_context *ctx )
   {
                        _mesa_reference_buffer_object(ctx, &ctx->CopyReadBuffer, NULL);
                                                                           for (i = 0; i < MAX_COMBINED_UNIFORM_BUFFERS; i++) {
      _mesa_reference_buffer_object(ctx,
   &ctx->UniformBufferBindings[i].BufferObject,
               for (i = 0; i < MAX_COMBINED_SHADER_STORAGE_BUFFERS; i++) {
      _mesa_reference_buffer_object(ctx,
                     for (i = 0; i < MAX_COMBINED_ATOMIC_BUFFERS; i++) {
      _mesa_reference_buffer_object(ctx,
   &ctx->AtomicBufferBindings[i].BufferObject,
               _mesa_HashLockMutex(ctx->Shared->BufferObjects);
   unreference_zombie_buffers_for_ctx(ctx);
   _mesa_HashWalkLocked(ctx->Shared->BufferObjects,
            }
      struct gl_buffer_object *
   _mesa_bufferobj_alloc(struct gl_context *ctx, GLuint id)
   {
      struct gl_buffer_object *buf = CALLOC_STRUCT(gl_buffer_object);
   if (!buf)
            buf->RefCount = 1;
   buf->Name = id;
            simple_mtx_init(&buf->MinMaxCacheMutex, mtx_plain);
   if (get_no_minmax_cache())
            }
   /**
   * Create a buffer object that will be backed by an OpenGL buffer ID
   * where the creating context will hold one global buffer reference instead
   * of updating buffer RefCount for every binding point.
   *
   * This shouldn't be used for internal buffers.
   */
   static struct gl_buffer_object *
   new_gl_buffer_object(struct gl_context *ctx, GLuint id)
   {
               buf->Ctx = ctx;
   buf->RefCount++; /* global buffer reference held by the context */
      }
      static ALWAYS_INLINE bool
   handle_bind_buffer_gen(struct gl_context *ctx,
                     {
               if (unlikely(!no_error && !buf && _mesa_is_desktop_gl_core(ctx))) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(non-gen name)", caller);
               if (unlikely(!buf || buf == &DummyBufferObject)) {
      /* If this is a new buffer object id, or one which was generated but
   * never used before, allocate a buffer object now.
   */
   *buf_handle = new_gl_buffer_object(ctx, buffer);
   if (!no_error && !*buf_handle) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", caller);
      }
   _mesa_HashLockMaybeLocked(ctx->Shared->BufferObjects,
         _mesa_HashInsertLocked(ctx->Shared->BufferObjects, buffer,
         /* If one context only creates buffers and another context only deletes
   * buffers, buffers don't get released because it only produces zombie
   * buffers. Only the context that has created the buffers can release
   * them. Thus, when we create buffers, we prune the list of zombie
   * buffers.
   */
   unreference_zombie_buffers_for_ctx(ctx);
   _mesa_HashUnlockMaybeLocked(ctx->Shared->BufferObjects,
                  }
      bool
   _mesa_handle_bind_buffer_gen(struct gl_context *ctx,
                     {
         }
      /**
   * Bind the specified target to buffer for the specified context.
   * Called by glBindBuffer() and other functions.
   */
   static void
   bind_buffer_object(struct gl_context *ctx,
               {
      struct gl_buffer_object *oldBufObj;
                     /* Fast path that unbinds. It's better when NULL is a literal, so that
   * the compiler can simplify this code after inlining.
   */
   if (buffer == 0) {
      _mesa_reference_buffer_object(ctx, bindTarget, NULL);
               /* Get pointer to old buffer object (to be unbound) */
   oldBufObj = *bindTarget;
   GLuint old_name = oldBufObj && !oldBufObj->DeletePending ? oldBufObj->Name : 0;
   if (unlikely(old_name == buffer))
            newBufObj = _mesa_lookup_bufferobj(ctx, buffer);
   /* Get a new buffer object if it hasn't been created. */
   if (unlikely(!handle_bind_buffer_gen(ctx, buffer, &newBufObj, "glBindBuffer",
                  /* At this point, the compiler should deduce that newBufObj is non-NULL if
   * everything has been inlined, so the compiler should simplify this.
   */
      }
         /**
   * Update the default buffer objects in the given context to reference those
   * specified in the shared state and release those referencing the old
   * shared state.
   */
   void
   _mesa_update_default_objects_buffer_objects(struct gl_context *ctx)
   {
      /* Bind 0 to remove references to those in the shared context hash table. */
   bind_buffer_object(ctx, &ctx->Array.ArrayBufferObj, 0, false);
   bind_buffer_object(ctx, &ctx->Array.VAO->IndexBufferObj, 0, false);
   bind_buffer_object(ctx, &ctx->Pack.BufferObj, 0, false);
      }
            /**
   * Return the gl_buffer_object for the given ID.
   * Always return NULL for ID 0.
   */
   struct gl_buffer_object *
   _mesa_lookup_bufferobj(struct gl_context *ctx, GLuint buffer)
   {
      if (buffer == 0)
         else
      return (struct gl_buffer_object *)
         }
         struct gl_buffer_object *
   _mesa_lookup_bufferobj_locked(struct gl_context *ctx, GLuint buffer)
   {
      if (buffer == 0)
         else
      return (struct gl_buffer_object *)
   }
      /**
   * A convenience function for direct state access functions that throws
   * GL_INVALID_OPERATION if buffer is not the name of an existing
   * buffer object.
   */
   struct gl_buffer_object *
   _mesa_lookup_bufferobj_err(struct gl_context *ctx, GLuint buffer,
         {
               bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufObj || bufObj == &DummyBufferObject) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
         /**
   * Look up a buffer object for a multi-bind function.
   *
   * Unlike _mesa_lookup_bufferobj(), this function also takes care
   * of generating an error if the buffer ID is not zero or the name
   * of an existing buffer object.
   *
   * If the buffer ID refers to an existing buffer object, a pointer
   * to the buffer object is returned.  If the ID is zero, NULL is returned.
   * If the ID is not zero and does not refer to a valid buffer object, this
   * function returns NULL.
   *
   * This function assumes that the caller has already locked the
   * hash table mutex by calling
   * _mesa_HashLockMutex(ctx->Shared->BufferObjects).
   */
   struct gl_buffer_object *
   _mesa_multi_bind_lookup_bufferobj(struct gl_context *ctx,
                     {
                        if (buffers[index] != 0) {
               /* The multi-bind functions don't create the buffer objects
         if (bufObj == &DummyBufferObject)
            if (!bufObj) {
      /* The ARB_multi_bind spec says:
   *
   *    "An INVALID_OPERATION error is generated if any value
   *     in <buffers> is not zero or the name of an existing
   *     buffer object (per binding)."
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
               "%s(buffers[%u]=%u is not zero or the name "
                     }
         /**
   * If *ptr points to obj, set ptr = the Null/default buffer object.
   * This is a helper for buffer object deletion.
   * The GL spec says that deleting a buffer object causes it to get
   * unbound from all arrays in the current context.
   */
   static void
   unbind(struct gl_context *ctx,
         struct gl_vertex_array_object *vao, unsigned index,
   {
      if (vao->BufferBinding[index].BufferObj == obj) {
      _mesa_bind_vertex_buffer(ctx, vao, index, NULL,
               }
      void
   _mesa_buffer_unmap_all_mappings(struct gl_context *ctx,
         {
      for (int i = 0; i < MAP_COUNT; i++) {
      if (_mesa_bufferobj_mapped(bufObj, i)) {
      _mesa_bufferobj_unmap(ctx, bufObj, i);
   assert(bufObj->Mappings[i].Pointer == NULL);
            }
         /**********************************************************************/
   /* API Functions                                                      */
   /**********************************************************************/
      void GLAPIENTRY
   _mesa_BindBuffer_no_error(GLenum target, GLuint buffer)
   {
               struct gl_buffer_object **bindTarget = get_buffer_target(ctx, target, true);
      }
         void GLAPIENTRY
   _mesa_BindBuffer(GLenum target, GLuint buffer)
   {
               if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glBindBuffer(%s, %u)\n",
               struct gl_buffer_object **bindTarget = get_buffer_target(ctx, target, false);
   if (!bindTarget) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferARB(target %s)",
                        }
      /**
   * Binds a buffer object to a binding point.
   *
   * The caller is responsible for validating the offset,
   * flushing the vertices and updating NewDriverState.
   */
   static void
   set_buffer_binding(struct gl_context *ctx,
                     struct gl_buffer_binding *binding,
   struct gl_buffer_object *bufObj,
   {
               binding->Offset = offset;
   binding->Size = size;
            /* If this is a real buffer object, mark it has having been used
   * at some point as an atomic counter buffer.
   */
   if (size >= 0)
      }
      static void
   set_buffer_multi_binding(struct gl_context *ctx,
                           const GLuint *buffers,
   int idx,
   const char *caller,
   struct gl_buffer_binding *binding,
   {
               if (binding->BufferObject && binding->BufferObject->Name == buffers[idx])
         else {
      bool error;
   bufObj = _mesa_multi_bind_lookup_bufferobj(ctx, buffers, idx, caller,
         if (error)
               if (!bufObj)
         else
      }
      static void
   bind_buffer(struct gl_context *ctx,
               struct gl_buffer_binding *binding,
   struct gl_buffer_object *bufObj,
   GLintptr offset,
   GLsizeiptr size,
   GLboolean autoSize,
   {
      if (binding->BufferObject == bufObj &&
      binding->Offset == offset &&
   binding->Size == size &&
   binding->AutomaticSize == autoSize) {
               FLUSH_VERTICES(ctx, 0, 0);
               }
      /**
   * Binds a buffer object to a uniform buffer binding point.
   *
   * Unlike set_buffer_binding(), this function also flushes vertices
   * and updates NewDriverState.  It also checks if the binding
   * has actually changed before updating it.
   */
   static void
   bind_uniform_buffer(struct gl_context *ctx,
                     GLuint index,
   struct gl_buffer_object *bufObj,
   {
      bind_buffer(ctx, &ctx->UniformBufferBindings[index],
                  }
      /**
   * Binds a buffer object to a shader storage buffer binding point.
   *
   * Unlike set_ssbo_binding(), this function also flushes vertices
   * and updates NewDriverState.  It also checks if the binding
   * has actually changed before updating it.
   */
   static void
   bind_shader_storage_buffer(struct gl_context *ctx,
                                 {
      bind_buffer(ctx, &ctx->ShaderStorageBufferBindings[index],
                  }
      /**
   * Binds a buffer object to an atomic buffer binding point.
   *
   * Unlike set_atomic_binding(), this function also flushes vertices
   * and updates NewDriverState.  It also checks if the binding
   * has actually changed before updating it.
   */
   static void
   bind_atomic_buffer(struct gl_context *ctx, unsigned index,
               {
      bind_buffer(ctx, &ctx->AtomicBufferBindings[index],
                  }
      /**
   * Bind a buffer object to a uniform block binding point.
   * As above, but offset = 0.
   */
   static void
   bind_buffer_base_uniform_buffer(struct gl_context *ctx,
      GLuint index,
      {
      if (index >= ctx->Const.MaxUniformBufferBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferBase(index=%d)", index);
                        if (!bufObj)
         else
      }
      /**
   * Bind a buffer object to a shader storage block binding point.
   * As above, but offset = 0.
   */
   static void
   bind_buffer_base_shader_storage_buffer(struct gl_context *ctx,
               {
      if (index >= ctx->Const.MaxShaderStorageBufferBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferBase(index=%d)", index);
                        if (!bufObj)
         else
      }
      /**
   * Bind a buffer object to a shader storage block binding point.
   * As above, but offset = 0.
   */
   static void
   bind_buffer_base_atomic_buffer(struct gl_context *ctx,
               {
      if (index >= ctx->Const.MaxAtomicBufferBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferBase(index=%d)", index);
                        if (!bufObj)
         else
      }
      /**
   * Delete a set of buffer objects.
   *
   * \param n      Number of buffer objects to delete.
   * \param ids    Array of \c n buffer object IDs.
   */
   static void
   delete_buffers(struct gl_context *ctx, GLsizei n, const GLuint *ids)
   {
               _mesa_HashLockMaybeLocked(ctx->Shared->BufferObjects,
                  for (GLsizei i = 0; i < n; i++) {
      struct gl_buffer_object *bufObj =
         if (bufObj) {
                                       /* unbind any vertex pointers bound to this buffer */
   for (j = 0; j < ARRAY_SIZE(vao->BufferBinding); j++) {
                  if (ctx->Array.ArrayBufferObj == bufObj) {
         }
   if (vao->IndexBufferObj == bufObj) {
                  /* unbind ARB_draw_indirect binding point */
   if (ctx->DrawIndirectBuffer == bufObj) {
                  /* unbind ARB_indirect_parameters binding point */
   if (ctx->ParameterBuffer == bufObj) {
                  /* unbind ARB_compute_shader binding point */
   if (ctx->DispatchIndirectBuffer == bufObj) {
                  /* unbind ARB_copy_buffer binding points */
   if (ctx->CopyReadBuffer == bufObj) {
         }
   if (ctx->CopyWriteBuffer == bufObj) {
                  /* unbind transform feedback binding points */
   if (ctx->TransformFeedback.CurrentBuffer == bufObj) {
         }
   for (j = 0; j < MAX_FEEDBACK_BUFFERS; j++) {
      if (ctx->TransformFeedback.CurrentObject->Buffers[j] == bufObj) {
      _mesa_bind_buffer_base_transform_feedback(ctx,
                        /* unbind UBO binding points */
   for (j = 0; j < ctx->Const.MaxUniformBufferBindings; j++) {
      if (ctx->UniformBufferBindings[j].BufferObject == bufObj) {
                     if (ctx->UniformBuffer == bufObj) {
                  /* unbind SSBO binding points */
   for (j = 0; j < ctx->Const.MaxShaderStorageBufferBindings; j++) {
      if (ctx->ShaderStorageBufferBindings[j].BufferObject == bufObj) {
                     if (ctx->ShaderStorageBuffer == bufObj) {
                  /* unbind Atomci Buffer binding points */
   for (j = 0; j < ctx->Const.MaxAtomicBufferBindings; j++) {
      if (ctx->AtomicBufferBindings[j].BufferObject == bufObj) {
                     if (ctx->AtomicBuffer == bufObj) {
                  /* unbind any pixel pack/unpack pointers bound to this buffer */
   if (ctx->Pack.BufferObj == bufObj) {
         }
   if (ctx->Unpack.BufferObj == bufObj) {
                  if (ctx->Texture.BufferObject == bufObj) {
                  if (ctx->ExternalVirtualMemoryBuffer == bufObj) {
                  /* unbind query buffer binding point */
   if (ctx->QueryBuffer == bufObj) {
                  /* The ID is immediately freed for re-use */
   _mesa_HashRemoveLocked(ctx->Shared->BufferObjects, ids[i]);
   /* Make sure we do not run into the classic ABA problem on bind.
   * We don't want to allow re-binding a buffer object that's been
   * "deleted" by glDeleteBuffers().
   *
   * The explicit rebinding to the default object in the current context
   * prevents the above in the current context, but another context
   * sharing the same objects might suffer from this problem.
   * The alternative would be to do the hash lookup in any case on bind
   * which would introduce more runtime overhead than this.
                  /* The GLuint ID holds one reference and the context that created
   * the buffer holds the other one.
                  if (bufObj->Ctx == ctx) {
         } else if (bufObj->Ctx) {
      /* Only the context holding it can release it. */
                              _mesa_HashUnlockMaybeLocked(ctx->Shared->BufferObjects,
      }
         void GLAPIENTRY
   _mesa_DeleteBuffers_no_error(GLsizei n, const GLuint *ids)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DeleteBuffers(GLsizei n, const GLuint *ids)
   {
               if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteBuffersARB(n)");
                  }
         /**
   * This is the implementation for glGenBuffers and glCreateBuffers. It is not
   * exposed to the rest of Mesa to encourage the use of nameless buffers in
   * driver internals.
   */
   static void
   create_buffers(struct gl_context *ctx, GLsizei n, GLuint *buffers, bool dsa)
   {
               if (!buffers)
            /*
   * This must be atomic (generation and allocation of buffer object IDs)
   */
   _mesa_HashLockMaybeLocked(ctx->Shared->BufferObjects,
         /* If one context only creates buffers and another context only deletes
   * buffers, buffers don't get released because it only produces zombie
   * buffers. Only the context that has created the buffers can release
   * them. Thus, when we create buffers, we prune the list of zombie
   * buffers.
   */
                     /* Insert the ID and pointer into the hash table. If non-DSA, insert a
   * DummyBufferObject.  Otherwise, create a new buffer object and insert
   * it.
   */
   for (int i = 0; i < n; i++) {
      if (dsa) {
      buf = new_gl_buffer_object(ctx, buffers[i]);
   if (!buf) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCreateBuffers");
   _mesa_HashUnlockMaybeLocked(ctx->Shared->BufferObjects,
               }
   else
                        _mesa_HashUnlockMaybeLocked(ctx->Shared->BufferObjects,
      }
         static void
   create_buffers_err(struct gl_context *ctx, GLsizei n, GLuint *buffers, bool dsa)
   {
               if (MESA_VERBOSE & VERBOSE_API)
            if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(n %d < 0)", func, n);
                  }
      /**
   * Generate a set of unique buffer object IDs and store them in \c buffers.
   *
   * \param n        Number of IDs to generate.
   * \param buffers  Array of \c n locations to store the IDs.
   */
   void GLAPIENTRY
   _mesa_GenBuffers_no_error(GLsizei n, GLuint *buffers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GenBuffers(GLsizei n, GLuint *buffers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      /**
   * Create a set of buffer objects and store their unique IDs in \c buffers.
   *
   * \param n        Number of IDs to generate.
   * \param buffers  Array of \c n locations to store the IDs.
   */
   void GLAPIENTRY
   _mesa_CreateBuffers_no_error(GLsizei n, GLuint *buffers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_CreateBuffers(GLsizei n, GLuint *buffers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * Determine if ID is the name of a buffer object.
   *
   * \param id  ID of the potential buffer object.
   * \return  \c GL_TRUE if \c id is the name of a buffer object,
   *          \c GL_FALSE otherwise.
   */
   GLboolean GLAPIENTRY
   _mesa_IsBuffer(GLuint id)
   {
      struct gl_buffer_object *bufObj;
   GET_CURRENT_CONTEXT(ctx);
                        }
         static bool
   validate_buffer_storage(struct gl_context *ctx,
               {
      if (size <= 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(size <= 0)", func);
               GLbitfield valid_flags = GL_MAP_READ_BIT |
                                    if (ctx->Extensions.ARB_sparse_buffer)
            if (flags & ~valid_flags) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(invalid flag bits set)", func);
               /* The Errors section of the GL_ARB_sparse_buffer spec says:
   *
   *    "INVALID_VALUE is generated by BufferStorage if <flags> contains
   *     SPARSE_STORAGE_BIT_ARB and <flags> also contains any combination of
   *     MAP_READ_BIT or MAP_WRITE_BIT."
   */
   if (flags & GL_SPARSE_STORAGE_BIT_ARB &&
      flags & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)) {
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(SPARSE_STORAGE and READ/WRITE)", func);
               if (flags & GL_MAP_PERSISTENT_BIT &&
      !(flags & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT))) {
   _mesa_error(ctx, GL_INVALID_VALUE,
                     if (flags & GL_MAP_COHERENT_BIT && !(flags & GL_MAP_PERSISTENT_BIT)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (bufObj->Immutable || bufObj->HandleAllocated) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(immutable)", func);
                  }
         static void
   buffer_storage(struct gl_context *ctx, struct gl_buffer_object *bufObj,
                     {
               /* Unmap the existing buffer.  We'll replace it now.  Not an error. */
                     bufObj->Immutable = GL_TRUE;
            if (memObj) {
      res = bufferobj_data_mem(ctx, target, size, memObj, offset,
      }
   else {
      res = _mesa_bufferobj_data(ctx, target, size, data, GL_DYNAMIC_DRAW,
               if (!res) {
      if (target == GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD) {
      /* Even though the interaction between AMD_pinned_memory and
   * glBufferStorage is not described in the spec, Graham Sellers
   * said that it should behave the same as glBufferData.
   */
      }
   else {
               }
         static ALWAYS_INLINE void
   inlined_buffer_storage(GLenum target, GLuint buffer, GLsizeiptr size,
                     {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
            if (mem) {
      if (!no_error) {
      if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               /* From the EXT_external_objects spec:
   *
   *   "An INVALID_VALUE error is generated by BufferStorageMemEXT and
   *   NamedBufferStorageMemEXT if <memory> is 0, or ..."
   */
   if (memory == 0) {
                     memObj = _mesa_lookup_memory_object(ctx, memory);
   if (!memObj)
            /* From the EXT_external_objects spec:
   *
   *   "An INVALID_OPERATION error is generated if <memory> names a
   *   valid memory object which has no associated memory."
   */
   if (!no_error && !memObj->Immutable) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(no associated memory)",
                        if (dsa) {
      if (no_error) {
         } else {
      bufObj = _mesa_lookup_bufferobj_err(ctx, buffer, func);
   if (!bufObj)
         } else {
      if (no_error) {
      struct gl_buffer_object **bufObjPtr =
            } else {
      bufObj = get_buffer(ctx, func, target, GL_INVALID_OPERATION);
   if (!bufObj)
                  if (no_error || validate_buffer_storage(ctx, bufObj, size, flags, func))
      }
         void GLAPIENTRY
   _mesa_BufferStorage_no_error(GLenum target, GLsizeiptr size,
         {
      inlined_buffer_storage(target, 0, size, data, flags, GL_NONE, 0,
      }
         void GLAPIENTRY
   _mesa_BufferStorage(GLenum target, GLsizeiptr size, const GLvoid *data,
         {
      inlined_buffer_storage(target, 0, size, data, flags, GL_NONE, 0,
      }
      void GLAPIENTRY
   _mesa_NamedBufferStorageEXT(GLuint buffer, GLsizeiptr size,
         {
               struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                  inlined_buffer_storage(GL_NONE, buffer, size, data, flags, GL_NONE, 0,
      }
         void GLAPIENTRY
   _mesa_BufferStorageMemEXT(GLenum target, GLsizeiptr size,
         {
      inlined_buffer_storage(target, 0, size, NULL, 0, memory, offset,
      }
         void GLAPIENTRY
   _mesa_BufferStorageMemEXT_no_error(GLenum target, GLsizeiptr size,
         {
      inlined_buffer_storage(target, 0, size, NULL, 0, memory, offset,
      }
         void GLAPIENTRY
   _mesa_NamedBufferStorage_no_error(GLuint buffer, GLsizeiptr size,
         {
      /* In direct state access, buffer objects have an unspecified target
   * since they are not required to be bound.
   */
   inlined_buffer_storage(GL_NONE, buffer, size, data, flags, GL_NONE, 0,
      }
         void GLAPIENTRY
   _mesa_NamedBufferStorage(GLuint buffer, GLsizeiptr size, const GLvoid *data,
         {
      /* In direct state access, buffer objects have an unspecified target
   * since they are not required to be bound.
   */
   inlined_buffer_storage(GL_NONE, buffer, size, data, flags, GL_NONE, 0,
      }
      void GLAPIENTRY
   _mesa_NamedBufferStorageMemEXT(GLuint buffer, GLsizeiptr size,
         {
      inlined_buffer_storage(GL_NONE, buffer, size, NULL, 0, memory, offset,
      }
         void GLAPIENTRY
   _mesa_NamedBufferStorageMemEXT_no_error(GLuint buffer, GLsizeiptr size,
         {
      inlined_buffer_storage(GL_NONE, buffer, size, NULL, 0, memory, offset,
      }
         static ALWAYS_INLINE void
   buffer_data(struct gl_context *ctx, struct gl_buffer_object *bufObj,
               {
               if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "%s(%s, %ld, %p, %s)\n",
               func,
               if (!no_error) {
      if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(size < 0)", func);
               switch (usage) {
   case GL_STREAM_DRAW_ARB:
      valid_usage = (ctx->API != API_OPENGLES);
      case GL_STATIC_DRAW_ARB:
   case GL_DYNAMIC_DRAW_ARB:
      valid_usage = true;
      case GL_STREAM_READ_ARB:
   case GL_STREAM_COPY_ARB:
   case GL_STATIC_READ_ARB:
   case GL_STATIC_COPY_ARB:
   case GL_DYNAMIC_READ_ARB:
   case GL_DYNAMIC_COPY_ARB:
      valid_usage = _mesa_is_desktop_gl(ctx) || _mesa_is_gles3(ctx);
      default:
      valid_usage = false;
               if (!valid_usage) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(invalid usage: %s)", func,
                     if (bufObj->Immutable || bufObj->HandleAllocated) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(immutable)", func);
                  /* Unmap the existing buffer.  We'll replace it now.  Not an error. */
                           #ifdef VBO_DEBUG
      printf("glBufferDataARB(%u, sz %ld, from %p, usage 0x%x)\n",
      #endif
      #ifdef BOUNDS_CHECK
         #endif
         if (!_mesa_bufferobj_data(ctx, target, size, data, usage,
                              if (target == GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD) {
      if (!no_error) {
      /* From GL_AMD_pinned_memory:
   *
   *   INVALID_OPERATION is generated by BufferData if <target> is
   *   EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, and the store cannot be
   *   mapped to the GPU address space.
   */
         } else {
               }
      static void
   buffer_data_error(struct gl_context *ctx, struct gl_buffer_object *bufObj,
               {
         }
      static void
   buffer_data_no_error(struct gl_context *ctx, struct gl_buffer_object *bufObj,
               {
         }
      void
   _mesa_buffer_data(struct gl_context *ctx, struct gl_buffer_object *bufObj,
               {
         }
      void GLAPIENTRY
   _mesa_BufferData_no_error(GLenum target, GLsizeiptr size, const GLvoid *data,
         {
               struct gl_buffer_object **bufObj = get_buffer_target(ctx, target, true);
   buffer_data_no_error(ctx, *bufObj, target, size, data, usage,
      }
      void GLAPIENTRY
   _mesa_BufferData(GLenum target, GLsizeiptr size,
         {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = get_buffer(ctx, "glBufferData", target, GL_INVALID_OPERATION);
   if (!bufObj)
            _mesa_buffer_data(ctx, bufObj, target, size, data, usage,
      }
      void GLAPIENTRY
   _mesa_NamedBufferData_no_error(GLuint buffer, GLsizeiptr size,
         {
               struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   buffer_data_no_error(ctx, bufObj, GL_NONE, size, data, usage,
      }
      void GLAPIENTRY
   _mesa_NamedBufferData(GLuint buffer, GLsizeiptr size, const GLvoid *data,
         {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = _mesa_lookup_bufferobj_err(ctx, buffer, "glNamedBufferData");
   if (!bufObj)
            /* In direct state access, buffer objects have an unspecified target since
   * they are not required to be bound.
   */
   _mesa_buffer_data(ctx, bufObj, GL_NONE, size, data, usage,
      }
      void GLAPIENTRY
   _mesa_NamedBufferDataEXT(GLuint buffer, GLsizeiptr size, const GLvoid *data,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (!buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                  _mesa_buffer_data(ctx, bufObj, GL_NONE, size, data, usage,
      }
      static bool
   validate_buffer_sub_data(struct gl_context *ctx,
                     {
      if (!buffer_object_subdata_range_good(ctx, bufObj, offset, size,
            /* error already recorded */
               if (bufObj->Immutable &&
      !(bufObj->StorageFlags & GL_DYNAMIC_STORAGE_BIT)) {
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s", func);
               if ((bufObj->Usage == GL_STATIC_DRAW ||
      bufObj->Usage == GL_STATIC_COPY) &&
   bufObj->NumSubDataCalls >= BUFFER_WARNING_CALL_COUNT - 1) {
   /* If the application declared the buffer as static draw/copy or stream
   * draw, it should not be frequently modified with glBufferSubData.
   */
   BUFFER_USAGE_WARNING(ctx,
                                    }
         /**
   * Implementation for glBufferSubData and glNamedBufferSubData.
   *
   * \param ctx     GL context.
   * \param bufObj  The buffer object.
   * \param offset  Offset of the first byte of the subdata range.
   * \param size    Size, in bytes, of the subdata range.
   * \param data    The data store.
   * \param func  Name of calling function for recording errors.
   *
   */
   void
   _mesa_buffer_sub_data(struct gl_context *ctx, struct gl_buffer_object *bufObj,
         {
      if (size == 0)
            bufObj->NumSubDataCalls++;
               }
         static ALWAYS_INLINE void
   buffer_sub_data(GLenum target, GLuint buffer, GLintptr offset,
               {
      GET_CURRENT_CONTEXT(ctx);
            if (dsa) {
      if (no_error) {
         } else {
      bufObj = _mesa_lookup_bufferobj_err(ctx, buffer, func);
   if (!bufObj)
         } else {
      if (no_error) {
      struct gl_buffer_object **bufObjPtr = get_buffer_target(ctx, target, true);
      } else {
      bufObj = get_buffer(ctx, func, target, GL_INVALID_OPERATION);
   if (!bufObj)
                  if (no_error || validate_buffer_sub_data(ctx, bufObj, offset, size, func))
      }
         void GLAPIENTRY
   _mesa_BufferSubData_no_error(GLenum target, GLintptr offset,
         {
      buffer_sub_data(target, 0, offset, size, data, false, true,
      }
         void GLAPIENTRY
   _mesa_BufferSubData(GLenum target, GLintptr offset,
         {
      buffer_sub_data(target, 0, offset, size, data, false, false,
      }
      void GLAPIENTRY
   _mesa_NamedBufferSubData_no_error(GLuint buffer, GLintptr offset,
         {
      buffer_sub_data(0, buffer, offset, size, data, true, true,
      }
      void GLAPIENTRY
   _mesa_NamedBufferSubData(GLuint buffer, GLintptr offset,
         {
      buffer_sub_data(0, buffer, offset, size, data, true, false,
      }
      void GLAPIENTRY
   _mesa_NamedBufferSubDataEXT(GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (!buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                  if (validate_buffer_sub_data(ctx, bufObj, offset, size,
                  }
         void GLAPIENTRY
   _mesa_GetBufferSubData(GLenum target, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = get_buffer(ctx, "glGetBufferSubData", target,
         if (!bufObj)
            if (!buffer_object_subdata_range_good(ctx, bufObj, offset, size, false,
                           }
      void GLAPIENTRY
   _mesa_GetNamedBufferSubData(GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = _mesa_lookup_bufferobj_err(ctx, buffer,
         if (!bufObj)
            if (!buffer_object_subdata_range_good(ctx, bufObj, offset, size, false,
                           }
         void GLAPIENTRY
   _mesa_GetNamedBufferSubDataEXT(GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (!buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                  if (!buffer_object_subdata_range_good(ctx, bufObj, offset, size, false,
                           }
      /**
   * \param subdata   true if caller is *SubData, false if *Data
   */
   static ALWAYS_INLINE void
   clear_buffer_sub_data(struct gl_context *ctx, struct gl_buffer_object *bufObj,
                     {
      mesa_format mesaFormat;
   GLubyte clearValue[MAX_PIXEL_BYTES];
            /* This checks for disallowed mappings. */
   if (!no_error && !buffer_object_subdata_range_good(ctx, bufObj, offset, size,
                        if (no_error) {
         } else {
      mesaFormat = validate_clear_buffer_format(ctx, internalformat,
               if (mesaFormat == MESA_FORMAT_NONE)
            clearValueSize = _mesa_get_format_bytes(mesaFormat);
   if (!no_error &&
      (offset % clearValueSize != 0 || size % clearValueSize != 0)) {
   _mesa_error(ctx, GL_INVALID_VALUE,
                           /* Bail early. Negative size has already been checked. */
   if (size == 0)
                     if (!ctx->pipe->clear_buffer) {
      clear_buffer_subdata_sw(ctx, offset, size,
                     if (!data)
         else if (!convert_clear_buffer_data(ctx, mesaFormat, clearValue,
                        ctx->pipe->clear_buffer(ctx->pipe, bufObj->buffer, offset, size,
      }
      static void
   clear_buffer_sub_data_error(struct gl_context *ctx,
                           {
      clear_buffer_sub_data(ctx, bufObj, internalformat, offset, size, format,
      }
         static void
   clear_buffer_sub_data_no_error(struct gl_context *ctx,
                                 {
      clear_buffer_sub_data(ctx, bufObj, internalformat, offset, size, format,
      }
         void GLAPIENTRY
   _mesa_ClearBufferData_no_error(GLenum target, GLenum internalformat,
         {
               struct gl_buffer_object **bufObj = get_buffer_target(ctx, target, true);
   clear_buffer_sub_data_no_error(ctx, *bufObj, internalformat, 0,
            }
         void GLAPIENTRY
   _mesa_ClearBufferData(GLenum target, GLenum internalformat, GLenum format,
         {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = get_buffer(ctx, "glClearBufferData", target, GL_INVALID_VALUE);
   if (!bufObj)
            clear_buffer_sub_data_error(ctx, bufObj, internalformat, 0, bufObj->Size,
      }
         void GLAPIENTRY
   _mesa_ClearNamedBufferData_no_error(GLuint buffer, GLenum internalformat,
               {
               struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   clear_buffer_sub_data_no_error(ctx, bufObj, internalformat, 0, bufObj->Size,
            }
         void GLAPIENTRY
   _mesa_ClearNamedBufferData(GLuint buffer, GLenum internalformat,
         {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = _mesa_lookup_bufferobj_err(ctx, buffer, "glClearNamedBufferData");
   if (!bufObj)
            clear_buffer_sub_data_error(ctx, bufObj, internalformat, 0, bufObj->Size,
            }
         void GLAPIENTRY
   _mesa_ClearNamedBufferDataEXT(GLuint buffer, GLenum internalformat,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                  clear_buffer_sub_data_error(ctx, bufObj, internalformat, 0, bufObj->Size,
            }
         void GLAPIENTRY
   _mesa_ClearBufferSubData_no_error(GLenum target, GLenum internalformat,
                     {
               struct gl_buffer_object **bufObj = get_buffer_target(ctx, target, true);
   clear_buffer_sub_data_no_error(ctx, *bufObj, internalformat, offset, size,
            }
         void GLAPIENTRY
   _mesa_ClearBufferSubData(GLenum target, GLenum internalformat,
                     {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = get_buffer(ctx, "glClearBufferSubData", target, GL_INVALID_VALUE);
   if (!bufObj)
            clear_buffer_sub_data_error(ctx, bufObj, internalformat, offset, size,
            }
         void GLAPIENTRY
   _mesa_ClearNamedBufferSubData_no_error(GLuint buffer, GLenum internalformat,
                     {
               struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   clear_buffer_sub_data_no_error(ctx, bufObj, internalformat, offset, size,
            }
         void GLAPIENTRY
   _mesa_ClearNamedBufferSubData(GLuint buffer, GLenum internalformat,
                     {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = _mesa_lookup_bufferobj_err(ctx, buffer,
         if (!bufObj)
            clear_buffer_sub_data_error(ctx, bufObj, internalformat, offset, size,
            }
      void GLAPIENTRY
   _mesa_ClearNamedBufferSubDataEXT(GLuint buffer, GLenum internalformat,
                     {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                  clear_buffer_sub_data_error(ctx, bufObj, internalformat, offset, size,
            }
      static GLboolean
   unmap_buffer(struct gl_context *ctx, struct gl_buffer_object *bufObj)
   {
      GLboolean status = _mesa_bufferobj_unmap(ctx, bufObj, MAP_USER);
   bufObj->Mappings[MAP_USER].AccessFlags = 0;
   assert(bufObj->Mappings[MAP_USER].Pointer == NULL);
   assert(bufObj->Mappings[MAP_USER].Offset == 0);
               }
      static GLboolean
   validate_and_unmap_buffer(struct gl_context *ctx,
               {
               if (!_mesa_bufferobj_mapped(bufObj, MAP_USER)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  #ifdef BOUNDS_CHECK
      if (bufObj->Mappings[MAP_USER].AccessFlags != GL_READ_ONLY_ARB) {
      GLubyte *buf = (GLubyte *) bufObj->Mappings[MAP_USER].Pointer;
   GLuint i;
   /* check that last 100 bytes are still = magic value */
   for (i = 0; i < 100; i++) {
      GLuint pos = bufObj->Size - i - 1;
   if (buf[pos] != 123) {
      _mesa_warning(ctx, "Out of bounds buffer object write detected"
                     #endif
      #ifdef VBO_DEBUG
      if (bufObj->Mappings[MAP_USER].AccessFlags & GL_MAP_WRITE_BIT) {
      GLuint i, unchanged = 0;
   GLubyte *b = (GLubyte *) bufObj->Mappings[MAP_USER].Pointer;
   GLint pos = -1;
   /* check which bytes changed */
   for (i = 0; i < bufObj->Size - 1; i++) {
      if (b[i] == (i & 0xff) && b[i+1] == ((i+1) & 0xff)) {
      unchanged++;
   if (pos == -1)
         }
   if (unchanged) {
      printf("glUnmapBufferARB(%u): %u of %ld unchanged, starting at %d\n",
            #endif
            }
      GLboolean GLAPIENTRY
   _mesa_UnmapBuffer_no_error(GLenum target)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object **bufObjPtr = get_buffer_target(ctx, target, true);
               }
      GLboolean GLAPIENTRY
   _mesa_UnmapBuffer(GLenum target)
   {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = get_buffer(ctx, "glUnmapBuffer", target, GL_INVALID_OPERATION);
   if (!bufObj)
               }
      GLboolean GLAPIENTRY
   _mesa_UnmapNamedBufferEXT_no_error(GLuint buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
               }
      GLboolean GLAPIENTRY
   _mesa_UnmapNamedBufferEXT(GLuint buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (!buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     bufObj = _mesa_lookup_bufferobj_err(ctx, buffer, "glUnmapNamedBuffer");
   if (!bufObj)
               }
         static bool
   get_buffer_parameter(struct gl_context *ctx,
               {
      switch (pname) {
   case GL_BUFFER_SIZE_ARB:
      *params = bufObj->Size;
      case GL_BUFFER_USAGE_ARB:
      *params = bufObj->Usage;
      case GL_BUFFER_ACCESS_ARB:
      *params = simplified_access_mode(ctx,
            case GL_BUFFER_MAPPED_ARB:
      *params = _mesa_bufferobj_mapped(bufObj, MAP_USER);
      case GL_BUFFER_ACCESS_FLAGS:
      if (!ctx->Extensions.ARB_map_buffer_range)
         *params = bufObj->Mappings[MAP_USER].AccessFlags;
      case GL_BUFFER_MAP_OFFSET:
      if (!ctx->Extensions.ARB_map_buffer_range)
         *params = bufObj->Mappings[MAP_USER].Offset;
      case GL_BUFFER_MAP_LENGTH:
      if (!ctx->Extensions.ARB_map_buffer_range)
         *params = bufObj->Mappings[MAP_USER].Length;
      case GL_BUFFER_IMMUTABLE_STORAGE:
      if (!ctx->Extensions.ARB_buffer_storage)
         *params = bufObj->Immutable;
      case GL_BUFFER_STORAGE_FLAGS:
      if (!ctx->Extensions.ARB_buffer_storage)
         *params = bufObj->StorageFlags;
      default:
                        invalid_pname:
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(invalid pname: %s)", func,
            }
      void GLAPIENTRY
   _mesa_GetBufferParameteriv(GLenum target, GLenum pname, GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
            bufObj = get_buffer(ctx, "glGetBufferParameteriv", target,
         if (!bufObj)
            if (!get_buffer_parameter(ctx, bufObj, pname, &parameter,
                     }
      void GLAPIENTRY
   _mesa_GetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
            bufObj = get_buffer(ctx, "glGetBufferParameteri64v", target,
         if (!bufObj)
            if (!get_buffer_parameter(ctx, bufObj, pname, &parameter,
                     }
      void GLAPIENTRY
   _mesa_GetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
            bufObj = _mesa_lookup_bufferobj_err(ctx, buffer,
         if (!bufObj)
            if (!get_buffer_parameter(ctx, bufObj, pname, &parameter,
                     }
      void GLAPIENTRY
   _mesa_GetNamedBufferParameterivEXT(GLuint buffer, GLenum pname, GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
            if (!buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                  if (!get_buffer_parameter(ctx, bufObj, pname, &parameter,
                     }
      void GLAPIENTRY
   _mesa_GetNamedBufferParameteri64v(GLuint buffer, GLenum pname,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
            bufObj = _mesa_lookup_bufferobj_err(ctx, buffer,
         if (!bufObj)
            if (!get_buffer_parameter(ctx, bufObj, pname, &parameter,
                     }
         void GLAPIENTRY
   _mesa_GetBufferPointerv(GLenum target, GLenum pname, GLvoid **params)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (pname != GL_BUFFER_MAP_POINTER) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferPointerv(pname != "
                     bufObj = get_buffer(ctx, "glGetBufferPointerv", target,
         if (!bufObj)
               }
      void GLAPIENTRY
   _mesa_GetNamedBufferPointerv(GLuint buffer, GLenum pname, GLvoid **params)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (pname != GL_BUFFER_MAP_POINTER) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetNamedBufferPointerv(pname != "
                     bufObj = _mesa_lookup_bufferobj_err(ctx, buffer,
         if (!bufObj)
               }
      void GLAPIENTRY
   _mesa_GetNamedBufferPointervEXT(GLuint buffer, GLenum pname, GLvoid **params)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (!buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   if (pname != GL_BUFFER_MAP_POINTER) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetNamedBufferPointervEXT(pname != "
                     bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                     }
      static void
   copy_buffer_sub_data(struct gl_context *ctx, struct gl_buffer_object *src,
               {
      if (_mesa_check_disallowed_mapping(src)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (_mesa_check_disallowed_mapping(dst)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (readOffset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (writeOffset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (size > src->Size || readOffset > src->Size - size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           if (size > dst->Size || writeOffset > dst->Size - size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           if (src == dst) {
      if (readOffset + size <= writeOffset) {
         }
   else if (writeOffset + size <= readOffset) {
         }
   else {
      /* overlapping src/dst is illegal */
   _mesa_error(ctx, GL_INVALID_VALUE,
                           }
      void GLAPIENTRY
   _mesa_CopyBufferSubData_no_error(GLenum readTarget, GLenum writeTarget,
               {
               struct gl_buffer_object **src_ptr = get_buffer_target(ctx, readTarget, true);
            struct gl_buffer_object **dst_ptr = get_buffer_target(ctx, writeTarget, true);
            bufferobj_copy_subdata(ctx, src, dst, readOffset, writeOffset,
      }
      void GLAPIENTRY
   _mesa_CopyBufferSubData(GLenum readTarget, GLenum writeTarget,
               {
      GET_CURRENT_CONTEXT(ctx);
            src = get_buffer(ctx, "glCopyBufferSubData", readTarget,
         if (!src)
            dst = get_buffer(ctx, "glCopyBufferSubData", writeTarget,
         if (!dst)
            copy_buffer_sub_data(ctx, src, dst, readOffset, writeOffset, size,
      }
      void GLAPIENTRY
   _mesa_NamedCopyBufferSubDataEXT(GLuint readBuffer, GLuint writeBuffer,
               {
      GET_CURRENT_CONTEXT(ctx);
            src = _mesa_lookup_bufferobj(ctx, readBuffer);
   if (!handle_bind_buffer_gen(ctx, readBuffer,
                        dst = _mesa_lookup_bufferobj(ctx, writeBuffer);
   if (!handle_bind_buffer_gen(ctx, writeBuffer,
                  copy_buffer_sub_data(ctx, src, dst, readOffset, writeOffset, size,
      }
      void GLAPIENTRY
   _mesa_CopyNamedBufferSubData_no_error(GLuint readBuffer, GLuint writeBuffer,
               {
               struct gl_buffer_object *src = _mesa_lookup_bufferobj(ctx, readBuffer);
            bufferobj_copy_subdata(ctx, src, dst, readOffset, writeOffset,
      }
      void GLAPIENTRY
   _mesa_CopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer,
               {
      GET_CURRENT_CONTEXT(ctx);
            src = _mesa_lookup_bufferobj_err(ctx, readBuffer,
         if (!src)
            dst = _mesa_lookup_bufferobj_err(ctx, writeBuffer,
         if (!dst)
            copy_buffer_sub_data(ctx, src, dst, readOffset, writeOffset, size,
      }
      void GLAPIENTRY
   _mesa_InternalBufferSubDataCopyMESA(GLintptr srcBuffer, GLuint srcOffset,
                     {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *src = (struct gl_buffer_object *)srcBuffer;
   struct gl_buffer_object *dst;
            /* Handle behavior for all 3 variants. */
   if (named && ext_dsa) {
      func = "glNamedBufferSubDataEXT";
   dst = _mesa_lookup_bufferobj(ctx, dstTargetOrName);
   if (!handle_bind_buffer_gen(ctx, dstTargetOrName, &dst, func, false))
      } else if (named) {
      func = "glNamedBufferSubData";
   dst = _mesa_lookup_bufferobj_err(ctx, dstTargetOrName, func);
   if (!dst)
      } else {
      assert(!ext_dsa);
   func = "glBufferSubData";
   dst = get_buffer(ctx, func, dstTargetOrName, GL_INVALID_OPERATION);
   if (!dst)
               if (!validate_buffer_sub_data(ctx, dst, dstOffset, size, func))
                  done:
      /* The caller passes the reference to this function, so unreference it. */
      }
      static bool
   validate_map_buffer_range(struct gl_context *ctx,
                     {
                        if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* Page 38 of the PDF of the OpenGL ES 3.0 spec says:
   *
   *     "An INVALID_OPERATION error is generated for any of the following
   *     conditions:
   *
   *     * <length> is zero."
   *
   * Additionally, page 94 of the PDF of the OpenGL 4.5 core spec
   * (30.10.2014) also says this, so it's no longer allowed for desktop GL,
   * either.
   */
   if (length == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(length = 0)", func);
               allowed_access = GL_MAP_READ_BIT |
                  GL_MAP_WRITE_BIT |
   GL_MAP_INVALIDATE_RANGE_BIT |
         if (ctx->Extensions.ARB_buffer_storage) {
         allowed_access |= GL_MAP_PERSISTENT_BIT |
            if (access & ~allowed_access) {
      /* generate an error if any bits other than those allowed are set */
   _mesa_error(ctx, GL_INVALID_VALUE,
                     if ((access & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if ((access & GL_MAP_READ_BIT) &&
      (access & (GL_MAP_INVALIDATE_RANGE_BIT |
               _mesa_error(ctx, GL_INVALID_OPERATION,
                     if ((access & GL_MAP_FLUSH_EXPLICIT_BIT) &&
      ((access & GL_MAP_WRITE_BIT) == 0)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (access & GL_MAP_READ_BIT &&
      !(bufObj->StorageFlags & GL_MAP_READ_BIT)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (access & GL_MAP_WRITE_BIT &&
      !(bufObj->StorageFlags & GL_MAP_WRITE_BIT)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (access & GL_MAP_COHERENT_BIT &&
      !(bufObj->StorageFlags & GL_MAP_COHERENT_BIT)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (access & GL_MAP_PERSISTENT_BIT &&
      !(bufObj->StorageFlags & GL_MAP_PERSISTENT_BIT)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (offset + length > bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
               "%s(offset %lu + length %lu > buffer_size %lu)", func,
               if (_mesa_bufferobj_mapped(bufObj, MAP_USER)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (access & GL_MAP_WRITE_BIT) {
      bufObj->NumMapBufferWriteCalls++;
   if ((bufObj->Usage == GL_STATIC_DRAW ||
      bufObj->Usage == GL_STATIC_COPY) &&
   bufObj->NumMapBufferWriteCalls >= BUFFER_WARNING_CALL_COUNT) {
   BUFFER_USAGE_WARNING(ctx,
                                       }
      static void *
   map_buffer_range(struct gl_context *ctx, struct gl_buffer_object *bufObj,
               {
      if (!bufObj->Size) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s(buffer size = 0)", func);
               void *map = _mesa_bufferobj_map_range(ctx, offset, length, access, bufObj,
         if (!map) {
         }
   else {
      /* The driver callback should have set all these fields.
   * This is important because other modules (like VBO) might call
   * the driver function directly.
   */
   assert(bufObj->Mappings[MAP_USER].Pointer == map);
   assert(bufObj->Mappings[MAP_USER].Length == length);
   assert(bufObj->Mappings[MAP_USER].Offset == offset);
               if (access & GL_MAP_WRITE_BIT) {
               #ifdef VBO_DEBUG
      if (strstr(func, "Range") == NULL) { /* If not MapRange */
      printf("glMapBuffer(%u, sz %ld, access 0x%x)\n",
         /* Access must be write only */
   if ((access & GL_MAP_WRITE_BIT) && (!(access & ~GL_MAP_WRITE_BIT))) {
      GLuint i;
   GLubyte *b = (GLubyte *) bufObj->Mappings[MAP_USER].Pointer;
   for (i = 0; i < bufObj->Size; i++)
            #endif
      #ifdef BOUNDS_CHECK
      if (strstr(func, "Range") == NULL) { /* If not MapRange */
      GLubyte *buf = (GLubyte *) bufObj->Mappings[MAP_USER].Pointer;
   GLuint i;
   /* buffer is 100 bytes larger than requested, fill with magic value */
   for (i = 0; i < 100; i++) {
               #endif
            }
      void * GLAPIENTRY
   _mesa_MapBufferRange_no_error(GLenum target, GLintptr offset,
         {
               struct gl_buffer_object **bufObjPtr = get_buffer_target(ctx, target, true);
            return map_buffer_range(ctx, bufObj, offset, length, access,
      }
      void * GLAPIENTRY
   _mesa_MapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (!ctx->Extensions.ARB_map_buffer_range) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     bufObj = get_buffer(ctx, "glMapBufferRange", target, GL_INVALID_OPERATION);
   if (!bufObj)
            if (!validate_map_buffer_range(ctx, bufObj, offset, length, access,
                  return map_buffer_range(ctx, bufObj, offset, length, access,
      }
      void * GLAPIENTRY
   _mesa_MapNamedBufferRange_no_error(GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            return map_buffer_range(ctx, bufObj, offset, length, access,
      }
      static void *
   map_named_buffer_range(GLuint buffer, GLintptr offset, GLsizeiptr length,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (!ctx->Extensions.ARB_map_buffer_range) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (dsa_ext) {
      bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer, &bufObj, func, false))
      } else {
      bufObj = _mesa_lookup_bufferobj_err(ctx, buffer, func);
   if (!bufObj)
               if (!validate_map_buffer_range(ctx, bufObj, offset, length, access, func))
               }
      void * GLAPIENTRY
   _mesa_MapNamedBufferRangeEXT(GLuint buffer, GLintptr offset, GLsizeiptr length,
         {
      GET_CURRENT_CONTEXT(ctx);
   if (!buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   return map_named_buffer_range(buffer, offset, length, access, true,
      }
      void * GLAPIENTRY
   _mesa_MapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length,
         {
      return map_named_buffer_range(buffer, offset, length, access, false,
      }
      /**
   * Converts GLenum access from MapBuffer and MapNamedBuffer into
   * flags for input to map_buffer_range.
   *
   * \return true if the type of requested access is permissible.
   */
   static bool
   get_map_buffer_access_flags(struct gl_context *ctx, GLenum access,
         {
      switch (access) {
   case GL_READ_ONLY_ARB:
      *flags = GL_MAP_READ_BIT;
      case GL_WRITE_ONLY_ARB:
      *flags = GL_MAP_WRITE_BIT;
      case GL_READ_WRITE_ARB:
      *flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
      default:
      *flags = 0;
         }
      void * GLAPIENTRY
   _mesa_MapBuffer_no_error(GLenum target, GLenum access)
   {
               GLbitfield accessFlags;
            struct gl_buffer_object **bufObjPtr = get_buffer_target(ctx, target, true);
            return map_buffer_range(ctx, bufObj, 0, bufObj->Size, accessFlags,
      }
      void * GLAPIENTRY
   _mesa_MapBuffer(GLenum target, GLenum access)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
            if (!get_map_buffer_access_flags(ctx, access, &accessFlags)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMapBuffer(invalid access)");
               bufObj = get_buffer(ctx, "glMapBuffer", target, GL_INVALID_OPERATION);
   if (!bufObj)
            if (!validate_map_buffer_range(ctx, bufObj, 0, bufObj->Size, accessFlags,
                  return map_buffer_range(ctx, bufObj, 0, bufObj->Size, accessFlags,
      }
      void * GLAPIENTRY
   _mesa_MapNamedBuffer_no_error(GLuint buffer, GLenum access)
   {
               GLbitfield accessFlags;
                     return map_buffer_range(ctx, bufObj, 0, bufObj->Size, accessFlags,
      }
      void * GLAPIENTRY
   _mesa_MapNamedBuffer(GLuint buffer, GLenum access)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
            if (!get_map_buffer_access_flags(ctx, access, &accessFlags)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMapNamedBuffer(invalid access)");
               bufObj = _mesa_lookup_bufferobj_err(ctx, buffer, "glMapNamedBuffer");
   if (!bufObj)
            if (!validate_map_buffer_range(ctx, bufObj, 0, bufObj->Size, accessFlags,
                  return map_buffer_range(ctx, bufObj, 0, bufObj->Size, accessFlags,
      }
      void * GLAPIENTRY
   _mesa_MapNamedBufferEXT(GLuint buffer, GLenum access)
   {
               GLbitfield accessFlags;
   if (!buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   if (!get_map_buffer_access_flags(ctx, access, &accessFlags)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMapNamedBufferEXT(invalid access)");
               struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                  if (!validate_map_buffer_range(ctx, bufObj, 0, bufObj->Size, accessFlags,
                  return map_buffer_range(ctx, bufObj, 0, bufObj->Size, accessFlags,
      }
      static void
   flush_mapped_buffer_range(struct gl_context *ctx,
                     {
      if (!ctx->Extensions.ARB_map_buffer_range) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (!_mesa_bufferobj_mapped(bufObj, MAP_USER)) {
      /* buffer is not mapped */
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if ((bufObj->Mappings[MAP_USER].AccessFlags &
      GL_MAP_FLUSH_EXPLICIT_BIT) == 0) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (offset + length > bufObj->Mappings[MAP_USER].Length) {
      _mesa_error(ctx, GL_INVALID_VALUE,
               "%s(offset %ld + length %ld > mapped length %ld)", func,
                        _mesa_bufferobj_flush_mapped_range(ctx, offset, length, bufObj,
      }
      void GLAPIENTRY
   _mesa_FlushMappedBufferRange_no_error(GLenum target, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object **bufObjPtr = get_buffer_target(ctx, target, true);
            _mesa_bufferobj_flush_mapped_range(ctx, offset, length, bufObj,
      }
      void GLAPIENTRY
   _mesa_FlushMappedBufferRange(GLenum target, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = get_buffer(ctx, "glFlushMappedBufferRange", target,
         if (!bufObj)
            flush_mapped_buffer_range(ctx, bufObj, offset, length,
      }
      void GLAPIENTRY
   _mesa_FlushMappedNamedBufferRange_no_error(GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_bufferobj_flush_mapped_range(ctx, offset, length, bufObj,
      }
      void GLAPIENTRY
   _mesa_FlushMappedNamedBufferRange(GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            bufObj = _mesa_lookup_bufferobj_err(ctx, buffer,
         if (!bufObj)
            flush_mapped_buffer_range(ctx, bufObj, offset, length,
      }
      void GLAPIENTRY
   _mesa_FlushMappedNamedBufferRangeEXT(GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (!buffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                  flush_mapped_buffer_range(ctx, bufObj, offset, length,
      }
      static void
   bind_buffer_range_uniform_buffer(struct gl_context *ctx, GLuint index,
               {
      if (!bufObj) {
      offset = -1;
               _mesa_reference_buffer_object(ctx, &ctx->UniformBuffer, bufObj);
      }
      /**
   * Bind a region of a buffer object to a uniform block binding point.
   * \param index  the uniform buffer binding point index
   * \param bufObj  the buffer object
   * \param offset  offset to the start of buffer object region
   * \param size  size of the buffer object region
   */
   static void
   bind_buffer_range_uniform_buffer_err(struct gl_context *ctx, GLuint index,
               {
      if (index >= ctx->Const.MaxUniformBufferBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferRange(index=%d)", index);
               if (offset & (ctx->Const.UniformBufferOffsetAlignment - 1)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
      ctx->Const.UniformBufferOffsetAlignment);
                     }
      static void
   bind_buffer_range_shader_storage_buffer(struct gl_context *ctx,
                           {
      if (!bufObj) {
      offset = -1;
               _mesa_reference_buffer_object(ctx, &ctx->ShaderStorageBuffer, bufObj);
      }
      /**
   * Bind a region of a buffer object to a shader storage block binding point.
   * \param index  the shader storage buffer binding point index
   * \param bufObj  the buffer object
   * \param offset  offset to the start of buffer object region
   * \param size  size of the buffer object region
   */
   static void
   bind_buffer_range_shader_storage_buffer_err(struct gl_context *ctx,
                     {
      if (index >= ctx->Const.MaxShaderStorageBufferBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferRange(index=%d)", index);
               if (offset & (ctx->Const.ShaderStorageBufferOffsetAlignment - 1)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                              }
      static void
   bind_buffer_range_atomic_buffer(struct gl_context *ctx, GLuint index,
               {
      if (!bufObj) {
      offset = -1;
               _mesa_reference_buffer_object(ctx, &ctx->AtomicBuffer, bufObj);
      }
      /**
   * Bind a region of a buffer object to an atomic storage block binding point.
   * \param index  the shader storage buffer binding point index
   * \param bufObj  the buffer object
   * \param offset  offset to the start of buffer object region
   * \param size  size of the buffer object region
   */
   static void
   bind_buffer_range_atomic_buffer_err(struct gl_context *ctx,
                     {
      if (index >= ctx->Const.MaxAtomicBufferBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferRange(index=%d)", index);
               if (offset & (ATOMIC_COUNTER_SIZE - 1)) {
         "glBindBufferRange(offset misaligned %d/%d)", (int) offset,
   ATOMIC_COUNTER_SIZE);
                     }
      static inline bool
   bind_buffers_check_offset_and_size(struct gl_context *ctx,
                     {
      if (offsets[index] < 0) {
      /* The ARB_multi_bind spec says:
   *
   *    "An INVALID_VALUE error is generated by BindBuffersRange if any
   *     value in <offsets> is less than zero (per binding)."
   */
   _mesa_error(ctx, GL_INVALID_VALUE,
                           if (sizes[index] <= 0) {
      /* The ARB_multi_bind spec says:
   *
   *     "An INVALID_VALUE error is generated by BindBuffersRange if any
   *      value in <sizes> is less than or equal to zero (per binding)."
   */
   _mesa_error(ctx, GL_INVALID_VALUE,
                              }
      static bool
   error_check_bind_uniform_buffers(struct gl_context *ctx,
               {
      if (!ctx->Extensions.ARB_uniform_buffer_object) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                     /* The ARB_multi_bind_spec says:
   *
   *     "An INVALID_OPERATION error is generated if <first> + <count> is
   *      greater than the number of target-specific indexed binding points,
   *      as described in section 6.7.1."
   */
   if (first + count > ctx->Const.MaxUniformBufferBindings) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               "%s(first=%u + count=%d > the value of "
   "GL_MAX_UNIFORM_BUFFER_BINDINGS=%u)",
                  }
      static bool
   error_check_bind_shader_storage_buffers(struct gl_context *ctx,
               {
      if (!ctx->Extensions.ARB_shader_storage_buffer_object) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                     /* The ARB_multi_bind_spec says:
   *
   *     "An INVALID_OPERATION error is generated if <first> + <count> is
   *      greater than the number of target-specific indexed binding points,
   *      as described in section 6.7.1."
   */
   if (first + count > ctx->Const.MaxShaderStorageBufferBindings) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               "%s(first=%u + count=%d > the value of "
   "GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS=%u)",
                  }
      /**
   * Unbind all uniform buffers in the range
   * <first> through <first>+<count>-1
   */
   static void
   unbind_uniform_buffers(struct gl_context *ctx, GLuint first, GLsizei count)
   {
      for (int i = 0; i < count; i++)
      set_buffer_binding(ctx, &ctx->UniformBufferBindings[first + i],
   }
      /**
   * Unbind all shader storage buffers in the range
   * <first> through <first>+<count>-1
   */
   static void
   unbind_shader_storage_buffers(struct gl_context *ctx, GLuint first,
         {
      for (int i = 0; i < count; i++)
      set_buffer_binding(ctx, &ctx->ShaderStorageBufferBindings[first + i],
   }
      static void
   bind_uniform_buffers(struct gl_context *ctx, GLuint first, GLsizei count,
                           {
      if (!error_check_bind_uniform_buffers(ctx, first, count, caller))
            /* Assume that at least one binding will be changed */
   FLUSH_VERTICES(ctx, 0, 0);
            if (!buffers) {
      /* The ARB_multi_bind spec says:
   *
   *    "If <buffers> is NULL, all bindings from <first> through
   *     <first>+<count>-1 are reset to their unbound (zero) state.
   *     In this case, the offsets and sizes associated with the
   *     binding points are set to default values, ignoring
   *     <offsets> and <sizes>."
   */
   unbind_uniform_buffers(ctx, first, count);
               /* Note that the error semantics for multi-bind commands differ from
   * those of other GL commands.
   *
   * The Issues section in the ARB_multi_bind spec says:
   *
   *    "(11) Typically, OpenGL specifies that if an error is generated by a
   *          command, that command has no effect.  This is somewhat
   *          unfortunate for multi-bind commands, because it would require a
   *          first pass to scan the entire list of bound objects for errors
   *          and then a second pass to actually perform the bindings.
   *          Should we have different error semantics?
   *
   *       RESOLVED:  Yes.  In this specification, when the parameters for
   *       one of the <count> binding points are invalid, that binding point
   *       is not updated and an error will be generated.  However, other
   *       binding points in the same command will be updated if their
   *       parameters are valid and no other error occurs."
            _mesa_HashLockMaybeLocked(ctx->Shared->BufferObjects,
            for (int i = 0; i < count; i++) {
      struct gl_buffer_binding *binding =
         GLintptr offset = 0;
            if (range) {
                     /* The ARB_multi_bind spec says:
   *
   *     "An INVALID_VALUE error is generated by BindBuffersRange if any
   *      pair of values in <offsets> and <sizes> does not respectively
   *      satisfy the constraints described for those parameters for the
   *      specified target, as described in section 6.7.1 (per binding)."
   *
   * Section 6.7.1 refers to table 6.5, which says:
   *
   *     "┌───────────────────────────────────────────────────────────────┐
   *      │ Uniform buffer array bindings (see sec. 7.6)                  │
   *      ├─────────────────────┬─────────────────────────────────────────┤
   *      │  ...                │  ...                                    │
   *      │  offset restriction │  multiple of value of UNIFORM_BUFFER_-  │
   *      │                     │  OFFSET_ALIGNMENT                       │
   *      │  ...                │  ...                                    │
   *      │  size restriction   │  none                                   │
   *      └─────────────────────┴─────────────────────────────────────────┘"
   */
   if (offsets[i] & (ctx->Const.UniformBufferOffsetAlignment - 1)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
               "glBindBuffersRange(offsets[%u]=%" PRId64
   " is misaligned; it must be a multiple of the value of "
   "GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT=%u when "
   "target=GL_UNIFORM_BUFFER)",
               offset = offsets[i];
               set_buffer_multi_binding(ctx, buffers, i, caller,
                     _mesa_HashUnlockMaybeLocked(ctx->Shared->BufferObjects,
      }
      static void
   bind_shader_storage_buffers(struct gl_context *ctx, GLuint first,
                                 {
      if (!error_check_bind_shader_storage_buffers(ctx, first, count, caller))
            /* Assume that at least one binding will be changed */
   FLUSH_VERTICES(ctx, 0, 0);
            if (!buffers) {
      /* The ARB_multi_bind spec says:
   *
   *    "If <buffers> is NULL, all bindings from <first> through
   *     <first>+<count>-1 are reset to their unbound (zero) state.
   *     In this case, the offsets and sizes associated with the
   *     binding points are set to default values, ignoring
   *     <offsets> and <sizes>."
   */
   unbind_shader_storage_buffers(ctx, first, count);
               /* Note that the error semantics for multi-bind commands differ from
   * those of other GL commands.
   *
   * The Issues section in the ARB_multi_bind spec says:
   *
   *    "(11) Typically, OpenGL specifies that if an error is generated by a
   *          command, that command has no effect.  This is somewhat
   *          unfortunate for multi-bind commands, because it would require a
   *          first pass to scan the entire list of bound objects for errors
   *          and then a second pass to actually perform the bindings.
   *          Should we have different error semantics?
   *
   *       RESOLVED:  Yes.  In this specification, when the parameters for
   *       one of the <count> binding points are invalid, that binding point
   *       is not updated and an error will be generated.  However, other
   *       binding points in the same command will be updated if their
   *       parameters are valid and no other error occurs."
            _mesa_HashLockMaybeLocked(ctx->Shared->BufferObjects,
            for (int i = 0; i < count; i++) {
      struct gl_buffer_binding *binding =
         GLintptr offset = 0;
            if (range) {
                     /* The ARB_multi_bind spec says:
   *
   *     "An INVALID_VALUE error is generated by BindBuffersRange if any
   *      pair of values in <offsets> and <sizes> does not respectively
   *      satisfy the constraints described for those parameters for the
   *      specified target, as described in section 6.7.1 (per binding)."
   *
   * Section 6.7.1 refers to table 6.5, which says:
   *
   *     "┌───────────────────────────────────────────────────────────────┐
   *      │ Shader storage buffer array bindings (see sec. 7.8)           │
   *      ├─────────────────────┬─────────────────────────────────────────┤
   *      │  ...                │  ...                                    │
   *      │  offset restriction │  multiple of value of SHADER_STORAGE_-  │
   *      │                     │  BUFFER_OFFSET_ALIGNMENT                │
   *      │  ...                │  ...                                    │
   *      │  size restriction   │  none                                   │
   *      └─────────────────────┴─────────────────────────────────────────┘"
   */
   if (offsets[i] & (ctx->Const.ShaderStorageBufferOffsetAlignment - 1)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
               "glBindBuffersRange(offsets[%u]=%" PRId64
   " is misaligned; it must be a multiple of the value of "
   "GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT=%u when "
   "target=GL_SHADER_STORAGE_BUFFER)",
               offset = offsets[i];
               set_buffer_multi_binding(ctx, buffers, i, caller,
                     _mesa_HashUnlockMaybeLocked(ctx->Shared->BufferObjects,
      }
      static bool
   error_check_bind_xfb_buffers(struct gl_context *ctx,
               {
      if (!ctx->Extensions.EXT_transform_feedback) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                     /* Page 398 of the PDF of the OpenGL 4.4 (Core Profile) spec says:
   *
   *     "An INVALID_OPERATION error is generated :
   *
   *     ...
   *     • by BindBufferRange or BindBufferBase if target is TRANSFORM_-
   *       FEEDBACK_BUFFER and transform feedback is currently active."
   *
   * We assume that this is also meant to apply to BindBuffersRange
   * and BindBuffersBase.
   */
   if (tfObj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           /* The ARB_multi_bind_spec says:
   *
   *     "An INVALID_OPERATION error is generated if <first> + <count> is
   *      greater than the number of target-specific indexed binding points,
   *      as described in section 6.7.1."
   */
   if (first + count > ctx->Const.MaxTransformFeedbackBuffers) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               "%s(first=%u + count=%d > the value of "
   "GL_MAX_TRANSFORM_FEEDBACK_BUFFERS=%u)",
                  }
      /**
   * Unbind all transform feedback buffers in the range
   * <first> through <first>+<count>-1
   */
   static void
   unbind_xfb_buffers(struct gl_context *ctx,
               {
      for (int i = 0; i < count; i++)
      _mesa_set_transform_feedback_binding(ctx, tfObj, first + i,
   }
      static void
   bind_xfb_buffers(struct gl_context *ctx,
                  GLuint first, GLsizei count,
   const GLuint *buffers,
   bool range,
      {
      struct gl_transform_feedback_object *tfObj =
            if (!error_check_bind_xfb_buffers(ctx, tfObj, first, count, caller))
            /* Assume that at least one binding will be changed */
            if (!buffers) {
      /* The ARB_multi_bind spec says:
   *
   *    "If <buffers> is NULL, all bindings from <first> through
   *     <first>+<count>-1 are reset to their unbound (zero) state.
   *     In this case, the offsets and sizes associated with the
   *     binding points are set to default values, ignoring
   *     <offsets> and <sizes>."
   */
   unbind_xfb_buffers(ctx, tfObj, first, count);
               /* Note that the error semantics for multi-bind commands differ from
   * those of other GL commands.
   *
   * The Issues section in the ARB_multi_bind spec says:
   *
   *    "(11) Typically, OpenGL specifies that if an error is generated by a
   *          command, that command has no effect.  This is somewhat
   *          unfortunate for multi-bind commands, because it would require a
   *          first pass to scan the entire list of bound objects for errors
   *          and then a second pass to actually perform the bindings.
   *          Should we have different error semantics?
   *
   *       RESOLVED:  Yes.  In this specification, when the parameters for
   *       one of the <count> binding points are invalid, that binding point
   *       is not updated and an error will be generated.  However, other
   *       binding points in the same command will be updated if their
   *       parameters are valid and no other error occurs."
            _mesa_HashLockMaybeLocked(ctx->Shared->BufferObjects,
            for (int i = 0; i < count; i++) {
      const GLuint index = first + i;
   struct gl_buffer_object * const boundBufObj = tfObj->Buffers[index];
   struct gl_buffer_object *bufObj;
   GLintptr offset = 0;
            if (range) {
                     /* The ARB_multi_bind spec says:
   *
   *     "An INVALID_VALUE error is generated by BindBuffersRange if any
   *      pair of values in <offsets> and <sizes> does not respectively
   *      satisfy the constraints described for those parameters for the
   *      specified target, as described in section 6.7.1 (per binding)."
   *
   * Section 6.7.1 refers to table 6.5, which says:
   *
   *     "┌───────────────────────────────────────────────────────────────┐
   *      │ Transform feedback array bindings (see sec. 13.2.2)           │
   *      ├───────────────────────┬───────────────────────────────────────┤
   *      │    ...                │    ...                                │
   *      │    offset restriction │    multiple of 4                      │
   *      │    ...                │    ...                                │
   *      │    size restriction   │    multiple of 4                      │
   *      └───────────────────────┴───────────────────────────────────────┘"
   */
   if (offsets[i] & 0x3) {
      _mesa_error(ctx, GL_INVALID_VALUE,
               "glBindBuffersRange(offsets[%u]=%" PRId64
   " is misaligned; it must be a multiple of 4 when "
               if (sizes[i] & 0x3) {
      _mesa_error(ctx, GL_INVALID_VALUE,
               "glBindBuffersRange(sizes[%u]=%" PRId64
   " is misaligned; it must be a multiple of 4 when "
               offset = offsets[i];
               if (boundBufObj && boundBufObj->Name == buffers[i])
         else {
      bool error;
   bufObj = _mesa_multi_bind_lookup_bufferobj(ctx, buffers, i, caller,
         if (error)
               _mesa_set_transform_feedback_binding(ctx, tfObj, index, bufObj,
               _mesa_HashUnlockMaybeLocked(ctx->Shared->BufferObjects,
      }
      static bool
   error_check_bind_atomic_buffers(struct gl_context *ctx,
               {
      if (!ctx->Extensions.ARB_shader_atomic_counters) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                     /* The ARB_multi_bind_spec says:
   *
   *     "An INVALID_OPERATION error is generated if <first> + <count> is
   *      greater than the number of target-specific indexed binding points,
   *      as described in section 6.7.1."
   */
   if (first + count > ctx->Const.MaxAtomicBufferBindings) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               "%s(first=%u + count=%d > the value of "
                  }
      /**
   * Unbind all atomic counter buffers in the range
   * <first> through <first>+<count>-1
   */
   static void
   unbind_atomic_buffers(struct gl_context *ctx, GLuint first, GLsizei count)
   {
      for (int i = 0; i < count; i++)
      set_buffer_binding(ctx, &ctx->AtomicBufferBindings[first + i],
   }
      static void
   bind_atomic_buffers(struct gl_context *ctx,
                     GLuint first,
   GLsizei count,
   const GLuint *buffers,
   bool range,
   {
      if (!error_check_bind_atomic_buffers(ctx, first, count, caller))
            /* Assume that at least one binding will be changed */
   FLUSH_VERTICES(ctx, 0, 0);
            if (!buffers) {
      /* The ARB_multi_bind spec says:
   *
   *    "If <buffers> is NULL, all bindings from <first> through
   *     <first>+<count>-1 are reset to their unbound (zero) state.
   *     In this case, the offsets and sizes associated with the
   *     binding points are set to default values, ignoring
   *     <offsets> and <sizes>."
   */
   unbind_atomic_buffers(ctx, first, count);
               /* Note that the error semantics for multi-bind commands differ from
   * those of other GL commands.
   *
   * The Issues section in the ARB_multi_bind spec says:
   *
   *    "(11) Typically, OpenGL specifies that if an error is generated by a
   *          command, that command has no effect.  This is somewhat
   *          unfortunate for multi-bind commands, because it would require a
   *          first pass to scan the entire list of bound objects for errors
   *          and then a second pass to actually perform the bindings.
   *          Should we have different error semantics?
   *
   *       RESOLVED:  Yes.  In this specification, when the parameters for
   *       one of the <count> binding points are invalid, that binding point
   *       is not updated and an error will be generated.  However, other
   *       binding points in the same command will be updated if their
   *       parameters are valid and no other error occurs."
            _mesa_HashLockMaybeLocked(ctx->Shared->BufferObjects,
            for (int i = 0; i < count; i++) {
      struct gl_buffer_binding *binding =
         GLintptr offset = 0;
            if (range) {
                     /* The ARB_multi_bind spec says:
   *
   *     "An INVALID_VALUE error is generated by BindBuffersRange if any
   *      pair of values in <offsets> and <sizes> does not respectively
   *      satisfy the constraints described for those parameters for the
   *      specified target, as described in section 6.7.1 (per binding)."
   *
   * Section 6.7.1 refers to table 6.5, which says:
   *
   *     "┌───────────────────────────────────────────────────────────────┐
   *      │ Atomic counter array bindings (see sec. 7.7.2)                │
   *      ├───────────────────────┬───────────────────────────────────────┤
   *      │    ...                │    ...                                │
   *      │    offset restriction │    multiple of 4                      │
   *      │    ...                │    ...                                │
   *      │    size restriction   │    none                               │
   *      └───────────────────────┴───────────────────────────────────────┘"
   */
   if (offsets[i] & (ATOMIC_COUNTER_SIZE - 1)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
               "glBindBuffersRange(offsets[%u]=%" PRId64
   " is misaligned; it must be a multiple of %d when "
               offset = offsets[i];
               set_buffer_multi_binding(ctx, buffers, i, caller,
                     _mesa_HashUnlockMaybeLocked(ctx->Shared->BufferObjects,
      }
      static ALWAYS_INLINE void
   bind_buffer_range(GLenum target, GLuint index, GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glBindBufferRange(%s, %u, %u, %lu, %lu)\n",
                     if (buffer == 0) {
         } else {
      bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                     if (no_error) {
      switch (target) {
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      _mesa_bind_buffer_range_xfb(ctx, ctx->TransformFeedback.CurrentObject,
            case GL_UNIFORM_BUFFER:
      bind_buffer_range_uniform_buffer(ctx, index, bufObj, offset, size);
      case GL_SHADER_STORAGE_BUFFER:
      bind_buffer_range_shader_storage_buffer(ctx, index, bufObj, offset,
            case GL_ATOMIC_COUNTER_BUFFER:
      bind_buffer_range_atomic_buffer(ctx, index, bufObj, offset, size);
      default:
            } else {
      if (buffer != 0) {
      if (size <= 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferRange(size=%d)",
                        switch (target) {
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      if (!_mesa_validate_buffer_range_xfb(ctx,
                              _mesa_bind_buffer_range_xfb(ctx, ctx->TransformFeedback.CurrentObject,
            case GL_UNIFORM_BUFFER:
      bind_buffer_range_uniform_buffer_err(ctx, index, bufObj, offset,
            case GL_SHADER_STORAGE_BUFFER:
      bind_buffer_range_shader_storage_buffer_err(ctx, index, bufObj,
            case GL_ATOMIC_COUNTER_BUFFER:
      bind_buffer_range_atomic_buffer_err(ctx, index, bufObj,
            default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferRange(target)");
            }
      void GLAPIENTRY
   _mesa_BindBufferRange_no_error(GLenum target, GLuint index, GLuint buffer,
         {
         }
      void GLAPIENTRY
   _mesa_BindBufferRange(GLenum target, GLuint index,
         {
         }
      void GLAPIENTRY
   _mesa_BindBufferBase(GLenum target, GLuint index, GLuint buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glBindBufferBase(%s, %u, %u)\n",
               if (buffer == 0) {
         } else {
      bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer,
                     /* Note that there's some oddness in the GL 3.1-GL 3.3 specifications with
   * regards to BindBufferBase.  It says (GL 3.1 core spec, page 63):
   *
   *     "BindBufferBase is equivalent to calling BindBufferRange with offset
   *      zero and size equal to the size of buffer."
   *
   * but it says for glGetIntegeri_v (GL 3.1 core spec, page 230):
   *
   *     "If the parameter (starting offset or size) was not specified when the
   *      buffer object was bound, zero is returned."
   *
   * What happens if the size of the buffer changes?  Does the size of the
   * buffer at the moment glBindBufferBase was called still play a role, like
   * the first quote would imply, or is the size meaningless in the
   * glBindBufferBase case like the second quote would suggest?  The GL 4.1
   * core spec page 45 says:
   *
   *     "It is equivalent to calling BindBufferRange with offset zero, while
   *      size is determined by the size of the bound buffer at the time the
   *      binding is used."
   *
   * My interpretation is that the GL 4.1 spec was a clarification of the
   * behavior, not a change.  In particular, this choice will only make
   * rendering work in cases where it would have had undefined results.
            switch (target) {
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      _mesa_bind_buffer_base_transform_feedback(ctx,
                  case GL_UNIFORM_BUFFER:
      bind_buffer_base_uniform_buffer(ctx, index, bufObj);
      case GL_SHADER_STORAGE_BUFFER:
      bind_buffer_base_shader_storage_buffer(ctx, index, bufObj);
      case GL_ATOMIC_COUNTER_BUFFER:
      bind_buffer_base_atomic_buffer(ctx, index, bufObj);
      default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferBase(target)");
         }
      void GLAPIENTRY
   _mesa_BindBuffersRange(GLenum target, GLuint first, GLsizei count,
               {
               if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glBindBuffersRange(%s, %u, %d, %p, %p, %p)\n",
                     switch (target) {
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      bind_xfb_buffers(ctx, first, count, buffers, true, offsets, sizes,
            case GL_UNIFORM_BUFFER:
      bind_uniform_buffers(ctx, first, count, buffers, true, offsets, sizes,
            case GL_SHADER_STORAGE_BUFFER:
      bind_shader_storage_buffers(ctx, first, count, buffers, true, offsets, sizes,
            case GL_ATOMIC_COUNTER_BUFFER:
      bind_atomic_buffers(ctx, first, count, buffers, true, offsets, sizes,
            default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBuffersRange(target=%s)",
               }
      void GLAPIENTRY
   _mesa_BindBuffersBase(GLenum target, GLuint first, GLsizei count,
         {
               if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glBindBuffersBase(%s, %u, %d, %p)\n",
               switch (target) {
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      bind_xfb_buffers(ctx, first, count, buffers, false, NULL, NULL,
            case GL_UNIFORM_BUFFER:
      bind_uniform_buffers(ctx, first, count, buffers, false, NULL, NULL,
            case GL_SHADER_STORAGE_BUFFER:
      bind_shader_storage_buffers(ctx, first, count, buffers, false, NULL, NULL,
            case GL_ATOMIC_COUNTER_BUFFER:
      bind_atomic_buffers(ctx, first, count, buffers, false, NULL, NULL,
            default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBuffersBase(target=%s)",
               }
      /**
   * Called via glInvalidateBuffer(Sub)Data.
   */
   static void
   bufferobj_invalidate(struct gl_context *ctx,
                     {
               /* We ignore partial invalidates. */
   if (offset != 0 || size != obj->Size)
            /* If the buffer is mapped, we can't invalidate it. */
   if (!obj->buffer || _mesa_bufferobj_mapped(obj, MAP_USER))
               }
      static ALWAYS_INLINE void
   invalidate_buffer_subdata(struct gl_context *ctx,
               {
      if (ctx->has_invalidate_buffer)
      }
      void GLAPIENTRY
   _mesa_InvalidateBufferSubData_no_error(GLuint buffer, GLintptr offset,
         {
               struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, buffer);
      }
      void GLAPIENTRY
   _mesa_InvalidateBufferSubData(GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
            /* Section 6.5 (Invalidating Buffer Data) of the OpenGL 4.5 (Compatibility
   * Profile) spec says:
   *
   *     "An INVALID_VALUE error is generated if buffer is zero or is not the
   *     name of an existing buffer object."
   */
   bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufObj || bufObj == &DummyBufferObject) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           /* The GL_ARB_invalidate_subdata spec says:
   *
   *     "An INVALID_VALUE error is generated if <offset> or <length> is
   *     negative, or if <offset> + <length> is greater than the value of
   *     BUFFER_SIZE."
   */
   if (offset < 0 || length < 0 || end > bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* The OpenGL 4.4 (Core Profile) spec says:
   *
   *     "An INVALID_OPERATION error is generated if buffer is currently
   *     mapped by MapBuffer or if the invalidate range intersects the range
   *     currently mapped by MapBufferRange, unless it was mapped
   *     with MAP_PERSISTENT_BIT set in the MapBufferRange access flags."
   */
   if (!(bufObj->Mappings[MAP_USER].AccessFlags & GL_MAP_PERSISTENT_BIT) &&
      bufferobj_range_mapped(bufObj, offset, length)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                              }
      void GLAPIENTRY
   _mesa_InvalidateBufferData_no_error(GLuint buffer)
   {
               struct gl_buffer_object *bufObj =_mesa_lookup_bufferobj(ctx, buffer);
      }
      void GLAPIENTRY
   _mesa_InvalidateBufferData(GLuint buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
            /* Section 6.5 (Invalidating Buffer Data) of the OpenGL 4.5 (Compatibility
   * Profile) spec says:
   *
   *     "An INVALID_VALUE error is generated if buffer is zero or is not the
   *     name of an existing buffer object."
   */
   bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufObj || bufObj == &DummyBufferObject) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           /* The OpenGL 4.4 (Core Profile) spec says:
   *
   *     "An INVALID_OPERATION error is generated if buffer is currently
   *     mapped by MapBuffer or if the invalidate range intersects the range
   *     currently mapped by MapBufferRange, unless it was mapped
   *     with MAP_PERSISTENT_BIT set in the MapBufferRange access flags."
   */
   if (_mesa_check_disallowed_mapping(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                              }
      static void
   buffer_page_commitment(struct gl_context *ctx,
                     {
      if (!(bufferObj->StorageFlags & GL_SPARSE_STORAGE_BIT_ARB)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(not a sparse buffer object)",
                     if (size < 0 || size > bufferObj->Size ||
      offset < 0 || offset > bufferObj->Size - size) {
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(out of bounds)",
                     /* The GL_ARB_sparse_buffer extension specification says:
   *
   *     "INVALID_VALUE is generated by BufferPageCommitmentARB if <offset> is
   *     not an integer multiple of SPARSE_BUFFER_PAGE_SIZE_ARB, or if <size>
   *     is not an integer multiple of SPARSE_BUFFER_PAGE_SIZE_ARB and does
   *     not extend to the end of the buffer's data store."
   */
   if (offset % ctx->Const.SparseBufferPageSize != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(offset not aligned to page size)",
                     if (size % ctx->Const.SparseBufferPageSize != 0 &&
      offset + size != bufferObj->Size) {
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(size not aligned to page size)",
                     struct pipe_context *pipe = ctx->pipe;
                     if (!pipe->resource_commit(pipe, bufferObj->buffer, 0, &box, commit)) {
            }
      void GLAPIENTRY
   _mesa_BufferPageCommitmentARB(GLenum target, GLintptr offset, GLsizeiptr size,
         {
      GET_CURRENT_CONTEXT(ctx);
            bufferObj = get_buffer(ctx, "glBufferPageCommitmentARB", target,
         if (!bufferObj)
            buffer_page_commitment(ctx, bufferObj, offset, size, commit,
      }
      void GLAPIENTRY
   _mesa_NamedBufferPageCommitmentARB(GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            bufferObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufferObj || bufferObj == &DummyBufferObject) {
      /* Note: the extension spec is not clear about the excpected error value. */
   _mesa_error(ctx, GL_INVALID_VALUE,
                           buffer_page_commitment(ctx, bufferObj, offset, size, commit,
      }
      void GLAPIENTRY
   _mesa_NamedBufferPageCommitmentEXT(GLuint buffer, GLintptr offset,
         {
      GET_CURRENT_CONTEXT(ctx);
            /* Use NamedBuffer* functions logic from EXT_direct_state_access */
   if (buffer != 0) {
      bufferObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!handle_bind_buffer_gen(ctx, buffer, &bufferObj,
            } else {
      /* GL_EXT_direct_state_access says about NamedBuffer* functions:
   *
   *   There is no buffer corresponding to the name zero, these commands
   *   generate the INVALID_OPERATION error if the buffer parameter is
   *   zero.
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   buffer_page_commitment(ctx, bufferObj, offset, size, commit,
      }
