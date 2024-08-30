   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009-2010  VMware, Inc.  All Rights Reserved.
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
   * \file shaderobj.c
   * \author Brian Paul
   *
   */
         #include "compiler/glsl/string_to_uint_map.h"
   #include "util/glheader.h"
   #include "main/context.h"
   #include "main/glspirv.h"
   #include "main/hash.h"
   #include "main/mtypes.h"
   #include "main/shaderapi.h"
   #include "main/shaderobj.h"
   #include "main/uniforms.h"
   #include "program/program.h"
   #include "program/prog_parameter.h"
   #include "util/ralloc.h"
   #include "util/u_atomic.h"
      /**********************************************************************/
   /*** Shader object functions                                        ***/
   /**********************************************************************/
         /**
   * Set ptr to point to sh.
   * If ptr is pointing to another shader, decrement its refcount (and delete
   * if refcount hits zero).
   * Then set ptr to point to sh, incrementing its refcount.
   */
   static void
   _reference_shader(struct gl_context *ctx, struct gl_shader **ptr,
         {
      assert(ptr);
   if (*ptr == sh) {
      /* no-op */
      }
   if (*ptr) {
      /* Unreference the old shader */
                     if (p_atomic_dec_zero(&old->RefCount)) {
      if (old->Name != 0) {
      if (skip_locking)
         else
      }
                  }
            if (sh) {
      /* reference new */
   p_atomic_inc(&sh->RefCount);
         }
      void
   _mesa_reference_shader(struct gl_context *ctx, struct gl_shader **ptr,
         {
         }
      static void
   _mesa_init_shader(struct gl_shader *shader)
   {
      shader->RefCount = 1;
   shader->info.Geom.VerticesOut = -1;
   shader->info.Geom.InputType = MESA_PRIM_TRIANGLES;
      }
      /**
   * Allocate a new gl_shader object, initialize it.
   */
   struct gl_shader *
   _mesa_new_shader(GLuint name, gl_shader_stage stage)
   {
      struct gl_shader *shader;
   shader = rzalloc(NULL, struct gl_shader);
   if (shader) {
      shader->Stage = stage;
   shader->Name = name;
      }
      }
         /**
   * Delete a shader object.
   */
   void
   _mesa_delete_shader(struct gl_context *ctx, struct gl_shader *sh)
   {
      _mesa_shader_spirv_data_reference(&sh->spirv_data, NULL);
   free((void *)sh->Source);
   free((void *)sh->FallbackSource);
   free(sh->Label);
      }
         /**
   * Delete a shader object.
   */
   void
   _mesa_delete_linked_shader(struct gl_context *ctx,
         {
      _mesa_shader_spirv_data_reference(&sh->spirv_data, NULL);
   _mesa_reference_program(ctx, &sh->Program, NULL);
      }
         /**
   * Lookup a GLSL shader object.
   */
   struct gl_shader *
   _mesa_lookup_shader(struct gl_context *ctx, GLuint name)
   {
      if (name) {
      struct gl_shader *sh = (struct gl_shader *)
         /* Note that both gl_shader and gl_shader_program objects are kept
   * in the same hash table.  Check the object's type to be sure it's
   * what we're expecting.
   */
   if (sh && sh->Type == GL_SHADER_PROGRAM_MESA) {
         }
      }
      }
         /**
   * As above, but record an error if shader is not found.
   */
   struct gl_shader *
   _mesa_lookup_shader_err(struct gl_context *ctx, GLuint name, const char *caller)
   {
      if (!name) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s", caller);
      }
   else {
      struct gl_shader *sh = (struct gl_shader *)
         if (!sh) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s", caller);
      }
   if (sh->Type == GL_SHADER_PROGRAM_MESA) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
      }
         }
            /**********************************************************************/
   /*** Shader Program object functions                                ***/
   /**********************************************************************/
      void
   _mesa_reference_shader_program_data(struct gl_shader_program_data **ptr,
         {
      if (*ptr == data)
            if (*ptr) {
                        if (p_atomic_dec_zero(&oldData->RefCount)) {
                                                            if (data)
               }
      /**
   * Set ptr to point to shProg.
   * If ptr is pointing to another object, decrement its refcount (and delete
   * if refcount hits zero).
   * Then set ptr to point to shProg, incrementing its refcount.
   */
   void
   _mesa_reference_shader_program_(struct gl_context *ctx,
               {
      assert(ptr);
   if (*ptr == shProg) {
      /* no-op */
      }
   if (*ptr) {
      /* Unreference the old shader program */
                     if (p_atomic_dec_zero(&old->RefCount)) {
      _mesa_HashLockMutex(ctx->Shared->ShaderObjects);
   if (old->Name != 0)
   _mesa_HashRemoveLocked(ctx->Shared->ShaderObjects, old->Name);
   _mesa_delete_shader_program(ctx, old);
                  }
            if (shProg) {
      p_atomic_inc(&shProg->RefCount);
         }
      struct gl_shader_program_data *
   _mesa_create_shader_program_data()
   {
      struct gl_shader_program_data *data;
   data = rzalloc(NULL, struct gl_shader_program_data);
   if (data) {
      data->RefCount = 1;
                  }
      static void
   init_shader_program(struct gl_shader_program *prog)
   {
      prog->Type = GL_SHADER_PROGRAM_MESA;
            prog->AttributeBindings = string_to_uint_map_ctor();
   prog->FragDataBindings = string_to_uint_map_ctor();
            prog->Geom.UsesEndPrimitive = false;
                        }
      /**
   * Allocate a new gl_shader_program object, initialize it.
   */
   struct gl_shader_program *
   _mesa_new_shader_program(GLuint name)
   {
      struct gl_shader_program *shProg;
   shProg = rzalloc(NULL, struct gl_shader_program);
   if (shProg) {
      shProg->Name = name;
   shProg->data = _mesa_create_shader_program_data();
   if (!shProg->data) {
      ralloc_free(shProg);
      }
      }
      }
         /**
   * Clear (free) the shader program state that gets produced by linking.
   */
   void
   _mesa_clear_shader_program_data(struct gl_context *ctx,
         {
      for (gl_shader_stage sh = 0; sh < MESA_SHADER_STAGES; sh++) {
      if (shProg->_LinkedShaders[sh] != NULL) {
      _mesa_delete_linked_shader(ctx, shProg->_LinkedShaders[sh]);
                  if (shProg->UniformRemapTable) {
      ralloc_free(shProg->UniformRemapTable);
   shProg->NumUniformRemapTable = 0;
               if (shProg->UniformHash) {
      string_to_uint_map_dtor(shProg->UniformHash);
               if (shProg->data)
               }
         /**
   * Free all the data that hangs off a shader program object, but not the
   * object itself.
   * Must be called with shared->ShaderObjects locked.
   */
   void
   _mesa_free_shader_program_data(struct gl_context *ctx,
         {
                                 if (shProg->AttributeBindings) {
      string_to_uint_map_dtor(shProg->AttributeBindings);
               if (shProg->FragDataBindings) {
      string_to_uint_map_dtor(shProg->FragDataBindings);
               if (shProg->FragDataIndexBindings) {
      string_to_uint_map_dtor(shProg->FragDataIndexBindings);
               /* detach shaders */
   for (i = 0; i < shProg->NumShaders; i++) {
         }
            free(shProg->Shaders);
            /* Transform feedback varying vars */
   for (i = 0; i < shProg->TransformFeedback.NumVarying; i++) {
         }
   free(shProg->TransformFeedback.VaryingNames);
   shProg->TransformFeedback.VaryingNames = NULL;
            free(shProg->Label);
      }
         /**
   * Free/delete a shader program object.
   */
   void
   _mesa_delete_shader_program(struct gl_context *ctx,
         {
      _mesa_free_shader_program_data(ctx, shProg);
      }
         /**
   * Lookup a GLSL program object.
   */
   struct gl_shader_program *
   _mesa_lookup_shader_program(struct gl_context *ctx, GLuint name)
   {
      struct gl_shader_program *shProg;
   if (name) {
      shProg = (struct gl_shader_program *)
         /* Note that both gl_shader and gl_shader_program objects are kept
   * in the same hash table.  Check the object's type to be sure it's
   * what we're expecting.
   */
   if (shProg && shProg->Type != GL_SHADER_PROGRAM_MESA) {
         }
      }
      }
         /**
   * As above, but record an error if program is not found.
   */
   struct gl_shader_program *
   _mesa_lookup_shader_program_err_glthread(struct gl_context *ctx, GLuint name,
         {
      if (!name) {
      _mesa_error_glthread_safe(ctx, GL_INVALID_VALUE, glthread, "%s", caller);
      }
   else {
      struct gl_shader_program *shProg = (struct gl_shader_program *)
         if (!shProg) {
      _mesa_error_glthread_safe(ctx, GL_INVALID_VALUE, glthread,
            }
   if (shProg->Type != GL_SHADER_PROGRAM_MESA) {
      _mesa_error_glthread_safe(ctx, GL_INVALID_OPERATION, glthread,
            }
         }
         struct gl_shader_program *
   _mesa_lookup_shader_program_err(struct gl_context *ctx, GLuint name,
         {
         }
