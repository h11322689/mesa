   /*
   * Copyright © 2016 Red Hat.
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "macros.h"
   #include "mtypes.h"
   #include "bufferobj.h"
   #include "context.h"
   #include "enums.h"
   #include "externalobjects.h"
   #include "teximage.h"
   #include "texobj.h"
   #include "glformats.h"
   #include "texstorage.h"
   #include "util/u_memory.h"
      #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_bitmap.h"
   #include "state_tracker/st_texture.h"
      struct st_context;
      #include "frontend/drm_driver.h"
   #ifdef HAVE_LIBDRM
   #include "drm-uapi/drm_fourcc.h"
   #endif
      static struct gl_memory_object *
   memoryobj_alloc(struct gl_context *ctx, GLuint name)
   {
      struct gl_memory_object *obj = CALLOC_STRUCT(gl_memory_object);
   if (!obj)
            obj->Name = name;
   obj->Dedicated = GL_FALSE;
      }
      static void
   import_memoryobj_fd(struct gl_context *ctx,
                     {
   #if !defined(_WIN32)
      struct pipe_screen *screen = ctx->pipe->screen;
   struct winsys_handle whandle = {
      .type = WINSYS_HANDLE_TYPE_FD,
   #ifdef HAVE_LIBDRM
         #endif
               obj->memory = screen->memobj_create_from_handle(screen,
                  /* We own fd, but we no longer need it. So get rid of it */
      #endif
   }
      static void
   import_memoryobj_win32(struct gl_context *ctx,
                           {
      struct pipe_screen *screen = ctx->pipe->screen;
   struct winsys_handle whandle = {
      #ifdef _WIN32
         #else
         #endif
   #ifdef HAVE_LIBDRM
         #endif
                     obj->memory = screen->memobj_create_from_handle(screen,
            }
      /**
   * Delete a memory object.
   * Not removed from hash table here.
   */
   void
   _mesa_delete_memory_object(struct gl_context *ctx,
         {
      struct pipe_screen *screen = ctx->pipe->screen;
   if (memObj->memory)
            }
      void GLAPIENTRY
   _mesa_DeleteMemoryObjectsEXT(GLsizei n, const GLuint *memoryObjects)
   {
               if (MESA_VERBOSE & (VERBOSE_API)) {
      _mesa_debug(ctx, "glDeleteMemoryObjectsEXT(%d, %p)\n", n,
               if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteMemoryObjectsEXT(n < 0)");
               if (!memoryObjects)
            _mesa_HashLockMutex(ctx->Shared->MemoryObjects);
   for (GLint i = 0; i < n; i++) {
      if (memoryObjects[i] > 0) {
                     if (delObj) {
      _mesa_HashRemoveLocked(ctx->Shared->MemoryObjects,
                  }
      }
      GLboolean GLAPIENTRY
   _mesa_IsMemoryObjectEXT(GLuint memoryObject)
   {
               if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     struct gl_memory_object *obj =
               }
      void GLAPIENTRY
   _mesa_CreateMemoryObjectsEXT(GLsizei n, GLuint *memoryObjects)
   {
                        if (MESA_VERBOSE & (VERBOSE_API))
            if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(n < 0)", func);
               if (!memoryObjects)
            _mesa_HashLockMutex(ctx->Shared->MemoryObjects);
   if (_mesa_HashFindFreeKeys(ctx->Shared->MemoryObjects, memoryObjects, n)) {
      for (GLsizei i = 0; i < n; i++) {
               /* allocate memory object */
   memObj = memoryobj_alloc(ctx, memoryObjects[i]);
   if (!memObj) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s()", func);
   _mesa_HashUnlockMutex(ctx->Shared->MemoryObjects);
               /* insert into hash table */
   _mesa_HashInsertLocked(ctx->Shared->MemoryObjects,
                           }
      void GLAPIENTRY
   _mesa_MemoryObjectParameterivEXT(GLuint memoryObject,
               {
      GET_CURRENT_CONTEXT(ctx);
                     if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               memObj = _mesa_lookup_memory_object(ctx, memoryObject);
   if (!memObj)
            if (memObj->Immutable) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(memoryObject is immutable", func);
               switch (pname) {
   case GL_DEDICATED_MEMORY_OBJECT_EXT:
      memObj->Dedicated = (GLboolean) params[0];
      case GL_PROTECTED_MEMORY_OBJECT_EXT:
      /* EXT_protected_textures not supported */
      default:
         }
         invalid_pname:
         }
      void GLAPIENTRY
   _mesa_GetMemoryObjectParameterivEXT(GLuint memoryObject,
               {
      GET_CURRENT_CONTEXT(ctx);
                     if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               memObj = _mesa_lookup_memory_object(ctx, memoryObject);
   if (!memObj)
            switch (pname) {
      case GL_DEDICATED_MEMORY_OBJECT_EXT:
      *params = (GLint) memObj->Dedicated;
      case GL_PROTECTED_MEMORY_OBJECT_EXT:
      /* EXT_protected_textures not supported */
      default:
      }
         invalid_pname:
         }
      static struct gl_memory_object *
   lookup_memory_object_err(struct gl_context *ctx, unsigned memory,
         {
      if (memory == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(memory=0)", func);
               struct gl_memory_object *memObj = _mesa_lookup_memory_object(ctx, memory);
   if (!memObj)
            if (!memObj->Immutable) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(no associated memory)",
                        }
      /**
   * Helper used by _mesa_TexStorageMem1/2/3DEXT().
   */
   static void
   texstorage_memory(GLuint dims, GLenum target, GLsizei levels,
                     {
      struct gl_texture_object *texObj;
                     if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (!_mesa_is_legal_tex_storage_target(ctx, dims, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           /* Check the format to make sure it is sized. */
   if (!_mesa_is_legal_tex_storage_format(ctx, internalFormat)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           texObj = _mesa_get_current_tex_object(ctx, target);
   if (!texObj)
            memObj = lookup_memory_object_err(ctx, memory, func);
   if (!memObj)
            _mesa_texture_storage_memory(ctx, dims, texObj, memObj, target,
            }
      static void
   texstorage_memory_ms(GLuint dims, GLenum target, GLsizei samples,
                     {
      struct gl_texture_object *texObj;
                     if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               texObj = _mesa_get_current_tex_object(ctx, target);
   if (!texObj)
            memObj = lookup_memory_object_err(ctx, memory, func);
   if (!memObj)
            _mesa_texture_storage_ms_memory(ctx, dims, texObj, memObj, target, samples,
            }
      /**
   * Helper used by _mesa_TextureStorageMem1/2/3DEXT().
   */
   static void
   texturestorage_memory(GLuint dims, GLuint texture, GLsizei levels,
                     {
      struct gl_texture_object *texObj;
                     if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               /* Check the format to make sure it is sized. */
   if (!_mesa_is_legal_tex_storage_format(ctx, internalFormat)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           texObj = _mesa_lookup_texture(ctx, texture);
   if (!texObj)
            if (!_mesa_is_legal_tex_storage_target(ctx, dims, texObj->Target)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           memObj = lookup_memory_object_err(ctx, memory, func);
   if (!memObj)
            _mesa_texture_storage_memory(ctx, dims, texObj, memObj, texObj->Target,
            }
      static void
   texturestorage_memory_ms(GLuint dims, GLuint texture, GLsizei samples,
                     {
      struct gl_texture_object *texObj;
                     if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               texObj = _mesa_lookup_texture(ctx, texture);
   if (!texObj)
            memObj = lookup_memory_object_err(ctx, memory, func);
   if (!memObj)
            _mesa_texture_storage_ms_memory(ctx, dims, texObj, memObj, texObj->Target,
            }
      void GLAPIENTRY
   _mesa_TexStorageMem2DEXT(GLenum target,
                           GLsizei levels,
   GLenum internalFormat,
   {
      texstorage_memory(2, target, levels, internalFormat, width, height, 1,
      }
      void GLAPIENTRY
   _mesa_TexStorageMem2DMultisampleEXT(GLenum target,
                                       GLsizei samples,
   {
      texstorage_memory_ms(2, target, samples, internalFormat, width, height, 1,
            }
      void GLAPIENTRY
   _mesa_TexStorageMem3DEXT(GLenum target,
                           GLsizei levels,
   GLenum internalFormat,
   GLsizei width,
   {
      texstorage_memory(3, target, levels, internalFormat, width, height, depth,
      }
      void GLAPIENTRY
   _mesa_TexStorageMem3DMultisampleEXT(GLenum target,
                                       GLsizei samples,
   GLenum internalFormat,
   {
      texstorage_memory_ms(3, target, samples, internalFormat, width, height,
            }
      void GLAPIENTRY
   _mesa_TextureStorageMem2DEXT(GLuint texture,
                              GLsizei levels,
      {
      texturestorage_memory(2, texture, levels, internalFormat, width, height, 1,
      }
      void GLAPIENTRY
   _mesa_TextureStorageMem2DMultisampleEXT(GLuint texture,
                                             {
      texturestorage_memory_ms(2, texture, samples, internalFormat, width, height,
            }
      void GLAPIENTRY
   _mesa_TextureStorageMem3DEXT(GLuint texture,
                              GLsizei levels,
   GLenum internalFormat,
      {
      texturestorage_memory(3, texture, levels, internalFormat, width, height,
      }
      void GLAPIENTRY
   _mesa_TextureStorageMem3DMultisampleEXT(GLuint texture,
                                          GLsizei samples,
      {
      texturestorage_memory_ms(3, texture, samples, internalFormat, width, height,
            }
      void GLAPIENTRY
   _mesa_TexStorageMem1DEXT(GLenum target,
                           GLsizei levels,
   {
      texstorage_memory(1, target, levels, internalFormat, width, 1, 1, memory,
      }
      void GLAPIENTRY
   _mesa_TextureStorageMem1DEXT(GLuint texture,
                                 {
      texturestorage_memory(1, texture, levels, internalFormat, width, 1, 1,
      }
      static struct gl_semaphore_object *
   semaphoreobj_alloc(struct gl_context *ctx, GLuint name)
   {
      struct gl_semaphore_object *obj = CALLOC_STRUCT(gl_semaphore_object);
   if (!obj)
            obj->Name = name;
      }
      static void
   import_semaphoreobj_fd(struct gl_context *ctx,
               {
                     #if !defined(_WIN32)
      /* We own fd, but we no longer need it. So get rid of it */
      #endif
   }
      static void
   import_semaphoreobj_win32(struct gl_context *ctx,
                           {
      struct pipe_context *pipe = ctx->pipe;
               }
      static void
   server_wait_semaphore(struct gl_context *ctx,
                        struct gl_semaphore_object *semObj,
   GLuint numBufferBarriers,
      {
      struct st_context *st = ctx->st;
   struct pipe_context *pipe = ctx->pipe;
   struct gl_buffer_object *bufObj;
            /* The driver is allowed to flush during fence_server_sync, be prepared */
   st_flush_bitmap_cache(st);
            /**
   * According to the EXT_external_objects spec, the memory operations must
   * follow the wait. This is to make sure the flush is executed after the
   * other party is done modifying the memory.
   *
   * Relevant excerpt from section "4.2.3 Waiting for Semaphores":
   *
   * Following completion of the semaphore wait operation, memory will also be
   * made visible in the specified buffer and texture objects.
   *
   */
   for (unsigned i = 0; i < numBufferBarriers; i++) {
      if (!bufObjs[i])
            bufObj = bufObjs[i];
   if (bufObj->buffer)
               for (unsigned i = 0; i < numTextureBarriers; i++) {
      if (!texObjs[i])
            texObj = texObjs[i];
   if (texObj->pt)
         }
      static void
   server_signal_semaphore(struct gl_context *ctx,
                           struct gl_semaphore_object *semObj,
   GLuint numBufferBarriers,
   {
      struct st_context *st = ctx->st;
   struct pipe_context *pipe = ctx->pipe;
   struct gl_buffer_object *bufObj;
            for (unsigned i = 0; i < numBufferBarriers; i++) {
      if (!bufObjs[i])
            bufObj = bufObjs[i];
   if (bufObj->buffer)
               for (unsigned i = 0; i < numTextureBarriers; i++) {
      if (!texObjs[i])
            texObj = texObjs[i];
   if (texObj->pt)
               /* The driver must flush during fence_server_signal, be prepared */
   st_flush_bitmap_cache(st);
      }
      /**
   * Used as a placeholder for semaphore objects between glGenSemaphoresEXT()
   * and glImportSemaphoreFdEXT(), so that glIsSemaphoreEXT() can work correctly.
   */
   static struct gl_semaphore_object DummySemaphoreObject;
      /**
   * Delete a semaphore object.
   * Not removed from hash table here.
   */
   void
   _mesa_delete_semaphore_object(struct gl_context *ctx,
         {
      if (semObj != &DummySemaphoreObject) {
      struct pipe_context *pipe = ctx->pipe;
   pipe->screen->fence_reference(ctx->screen, &semObj->fence, NULL);
         }
      void GLAPIENTRY
   _mesa_GenSemaphoresEXT(GLsizei n, GLuint *semaphores)
   {
                        if (MESA_VERBOSE & (VERBOSE_API))
            if (!ctx->Extensions.EXT_semaphore) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(n < 0)", func);
               if (!semaphores)
            _mesa_HashLockMutex(ctx->Shared->SemaphoreObjects);
   if (_mesa_HashFindFreeKeys(ctx->Shared->SemaphoreObjects, semaphores, n)) {
      for (GLsizei i = 0; i < n; i++) {
      _mesa_HashInsertLocked(ctx->Shared->SemaphoreObjects,
                     }
      void GLAPIENTRY
   _mesa_DeleteSemaphoresEXT(GLsizei n, const GLuint *semaphores)
   {
                        if (MESA_VERBOSE & (VERBOSE_API)) {
                  if (!ctx->Extensions.EXT_semaphore) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(n < 0)", func);
               if (!semaphores)
            _mesa_HashLockMutex(ctx->Shared->SemaphoreObjects);
   for (GLint i = 0; i < n; i++) {
      if (semaphores[i] > 0) {
                     if (delObj) {
      _mesa_HashRemoveLocked(ctx->Shared->SemaphoreObjects,
                  }
      }
      GLboolean GLAPIENTRY
   _mesa_IsSemaphoreEXT(GLuint semaphore)
   {
               if (!ctx->Extensions.EXT_semaphore) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glIsSemaphoreEXT(unsupported)");
               struct gl_semaphore_object *obj =
               }
      /**
   * Helper that outputs the correct error status for parameter
   * calls where no pnames are defined
   */
   static void
   semaphore_parameter_stub(const char* func, GLenum pname)
   {
   }
      void GLAPIENTRY
   _mesa_SemaphoreParameterui64vEXT(GLuint semaphore,
               {
      GET_CURRENT_CONTEXT(ctx);
            if (!ctx->Extensions.EXT_semaphore) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (pname != GL_D3D12_FENCE_VALUE_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=0x%x)", func, pname);
               struct gl_semaphore_object *semObj = _mesa_lookup_semaphore_object(ctx,
         if (!semObj)
            if (semObj->type != PIPE_FD_TYPE_TIMELINE_SEMAPHORE) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(Not a D3D12 fence)", func);
               semObj->timeline_value = params[0];
      }
      void GLAPIENTRY
   _mesa_GetSemaphoreParameterui64vEXT(GLuint semaphore,
               {
      GET_CURRENT_CONTEXT(ctx);
            if (!ctx->Extensions.EXT_semaphore) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (pname != GL_D3D12_FENCE_VALUE_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=0x%x)", func, pname);
               struct gl_semaphore_object *semObj = _mesa_lookup_semaphore_object(ctx,
         if (!semObj)
            if (semObj->type != PIPE_FD_TYPE_TIMELINE_SEMAPHORE) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(Not a D3D12 fence)", func);
                  }
      void GLAPIENTRY
   _mesa_WaitSemaphoreEXT(GLuint semaphore,
                        GLuint numBufferBarriers,
      {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_semaphore_object *semObj = NULL;
   struct gl_buffer_object **bufObjs = NULL;
                     if (!ctx->Extensions.EXT_semaphore) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
                        semObj = _mesa_lookup_semaphore_object(ctx, semaphore);
   if (!semObj)
                     bufObjs = malloc(sizeof(struct gl_buffer_object *) * numBufferBarriers);
   if (!bufObjs) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s(numBufferBarriers=%u)",
                     for (unsigned i = 0; i < numBufferBarriers; i++) {
                  texObjs = malloc(sizeof(struct gl_texture_object *) * numTextureBarriers);
   if (!texObjs) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s(numTextureBarriers=%u)",
                     for (unsigned i = 0; i < numTextureBarriers; i++) {
                  server_wait_semaphore(ctx, semObj,
                     end:
      free(bufObjs);
      }
      void GLAPIENTRY
   _mesa_SignalSemaphoreEXT(GLuint semaphore,
                           GLuint numBufferBarriers,
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_semaphore_object *semObj = NULL;
   struct gl_buffer_object **bufObjs = NULL;
                     if (!ctx->Extensions.EXT_semaphore) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
                        semObj = _mesa_lookup_semaphore_object(ctx, semaphore);
   if (!semObj)
                     bufObjs = malloc(sizeof(struct gl_buffer_object *) * numBufferBarriers);
   if (!bufObjs) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s(numBufferBarriers=%u)",
                     for (unsigned i = 0; i < numBufferBarriers; i++) {
                  texObjs = malloc(sizeof(struct gl_texture_object *) * numTextureBarriers);
   if (!texObjs) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s(numTextureBarriers=%u)",
                     for (unsigned i = 0; i < numTextureBarriers; i++) {
                  server_signal_semaphore(ctx, semObj,
                     end:
      free(bufObjs);
      }
      void GLAPIENTRY
   _mesa_ImportMemoryFdEXT(GLuint memory,
                     {
                        if (!ctx->Extensions.EXT_memory_object_fd) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (handleType != GL_HANDLE_TYPE_OPAQUE_FD_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(handleType=%u)", func, handleType);
               struct gl_memory_object *memObj = _mesa_lookup_memory_object(ctx, memory);
   if (!memObj)
            import_memoryobj_fd(ctx, memObj, size, fd);
      }
      void GLAPIENTRY
   _mesa_ImportMemoryWin32HandleEXT(GLuint memory,
                     {
                        if (!ctx->Extensions.EXT_memory_object_win32) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (handleType != GL_HANDLE_TYPE_OPAQUE_WIN32_EXT &&
      handleType != GL_HANDLE_TYPE_D3D11_IMAGE_EXT &&
   handleType != GL_HANDLE_TYPE_D3D12_RESOURCE_EXT &&
   handleType != GL_HANDLE_TYPE_D3D12_TILEPOOL_EXT) {
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(handleType=%u)", func, handleType);
               struct gl_memory_object *memObj = _mesa_lookup_memory_object(ctx, memory);
   if (!memObj)
            import_memoryobj_win32(ctx, memObj, size, handle, NULL);
      }
      void GLAPIENTRY
   _mesa_ImportMemoryWin32NameEXT(GLuint memory,
                     {
                        if (!ctx->Extensions.EXT_memory_object_win32) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (handleType != GL_HANDLE_TYPE_OPAQUE_WIN32_EXT &&
      handleType != GL_HANDLE_TYPE_D3D11_IMAGE_EXT &&
   handleType != GL_HANDLE_TYPE_D3D12_RESOURCE_EXT &&
   handleType != GL_HANDLE_TYPE_D3D12_TILEPOOL_EXT) {
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(handleType=%u)", func, handleType);
               struct gl_memory_object *memObj = _mesa_lookup_memory_object(ctx, memory);
   if (!memObj)
            import_memoryobj_win32(ctx, memObj, size, NULL, name);
      }
      void GLAPIENTRY
   _mesa_ImportSemaphoreFdEXT(GLuint semaphore,
               {
                        if (!ctx->Extensions.EXT_semaphore_fd) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (handleType != GL_HANDLE_TYPE_OPAQUE_FD_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(handleType=%u)", func, handleType);
               struct gl_semaphore_object *semObj = _mesa_lookup_semaphore_object(ctx,
         if (!semObj)
            if (semObj == &DummySemaphoreObject) {
      semObj = semaphoreobj_alloc(ctx, semaphore);
   if (!semObj) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", func);
      }
                  }
      void GLAPIENTRY
   _mesa_ImportSemaphoreWin32HandleEXT(GLuint semaphore,
               {
                        if (!ctx->Extensions.EXT_semaphore_win32) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (handleType != GL_HANDLE_TYPE_OPAQUE_WIN32_EXT &&
      handleType != GL_HANDLE_TYPE_D3D12_FENCE_EXT) {
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(handleType=%u)", func, handleType);
               if (handleType == GL_HANDLE_TYPE_D3D12_FENCE_EXT &&
      !ctx->screen->get_param(ctx->screen, PIPE_CAP_TIMELINE_SEMAPHORE_IMPORT)) {
               struct gl_semaphore_object *semObj = _mesa_lookup_semaphore_object(ctx,
         if (!semObj)
            if (semObj == &DummySemaphoreObject) {
      semObj = semaphoreobj_alloc(ctx, semaphore);
   if (!semObj) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", func);
      }
               enum pipe_fd_type type = handleType == GL_HANDLE_TYPE_D3D12_FENCE_EXT ?
            }
      void GLAPIENTRY
   _mesa_ImportSemaphoreWin32NameEXT(GLuint semaphore,
               {
                        if (!ctx->Extensions.EXT_semaphore_win32) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               if (handleType != GL_HANDLE_TYPE_OPAQUE_WIN32_EXT &&
      handleType != GL_HANDLE_TYPE_D3D12_FENCE_EXT) {
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(handleType=%u)", func, handleType);
               if (handleType == GL_HANDLE_TYPE_D3D12_FENCE_EXT &&
      !ctx->screen->get_param(ctx->screen, PIPE_CAP_TIMELINE_SEMAPHORE_IMPORT)) {
               struct gl_semaphore_object *semObj = _mesa_lookup_semaphore_object(ctx,
         if (!semObj)
            if (semObj == &DummySemaphoreObject) {
      semObj = semaphoreobj_alloc(ctx, semaphore);
   if (!semObj) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", func);
      }
               enum pipe_fd_type type = handleType == GL_HANDLE_TYPE_D3D12_FENCE_EXT ?
            }
