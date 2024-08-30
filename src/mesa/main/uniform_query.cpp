   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009-2010  VMware, Inc.  All Rights Reserved.
   * Copyright © 2010, 2011 Intel Corporation
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
      #include <stdlib.h>
   #include <inttypes.h>  /* for PRIx64 macro */
   #include <math.h>
      #include "main/context.h"
   #include "main/draw_validate.h"
   #include "main/shaderapi.h"
   #include "main/shaderobj.h"
   #include "main/uniforms.h"
   #include "compiler/glsl/ir.h"
   #include "compiler/glsl/ir_uniform.h"
   #include "compiler/glsl/glsl_parser_extras.h"
   #include "compiler/glsl/program.h"
   #include "util/bitscan.h"
      #include "state_tracker/st_context.h"
      /* This is one of the few glGet that can be called from the app thread safely.
   * Only these conditions must be met:
   * - There are no unfinished glLinkProgram and glDeleteProgram calls
   *   for the program object. This assures that the program object is immutable.
   * - glthread=true for GL errors to be passed to the driver thread safely
   *
   * Program objects can be looked up from any thread because they are part
   * of the multi-context shared state.
   */
   extern "C" void
   _mesa_GetActiveUniform_impl(GLuint program, GLuint index,
               {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *shProg;
            if (maxLength < 0) {
      _mesa_error_glthread_safe(ctx, GL_INVALID_VALUE, glthread,
                     shProg = _mesa_lookup_shader_program_err_glthread(ctx, program, glthread,
         if (!shProg)
            res = _mesa_program_resource_find_index((struct gl_shader_program *) shProg,
            if (!res) {
      _mesa_error_glthread_safe(ctx, GL_INVALID_VALUE, glthread,
                     if (nameOut)
      _mesa_get_program_resource_name(shProg, GL_UNIFORM, index, maxLength,
            if (type)
      _mesa_program_resource_prop((struct gl_shader_program *) shProg,
            if (size)
      _mesa_program_resource_prop((struct gl_shader_program *) shProg,
         }
      extern "C" void GLAPIENTRY
   _mesa_GetActiveUniform(GLuint program, GLuint index,
               {
      _mesa_GetActiveUniform_impl(program, index, maxLength, length, size,
      }
      static GLenum
   resource_prop_from_uniform_prop(GLenum uni_prop)
   {
      switch (uni_prop) {
   case GL_UNIFORM_TYPE:
         case GL_UNIFORM_SIZE:
         case GL_UNIFORM_NAME_LENGTH:
         case GL_UNIFORM_BLOCK_INDEX:
         case GL_UNIFORM_OFFSET:
         case GL_UNIFORM_ARRAY_STRIDE:
         case GL_UNIFORM_MATRIX_STRIDE:
         case GL_UNIFORM_IS_ROW_MAJOR:
         case GL_UNIFORM_ATOMIC_COUNTER_BUFFER_INDEX:
         default:
            }
      extern "C" void GLAPIENTRY
   _mesa_GetActiveUniformsiv(GLuint program,
      GLsizei uniformCount,
   const GLuint *uniformIndices,
   GLenum pname,
      {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *shProg;
   struct gl_program_resource *res;
            if (uniformCount < 0) {
         "glGetActiveUniformsiv(uniformCount < 0)");
                  shProg = _mesa_lookup_shader_program_err(ctx, program, "glGetActiveUniform");
   if (!shProg)
                     /* We need to first verify that each entry exists as active uniform. If
   * not, generate error and do not cause any other side effects.
   *
   * In the case of and error condition, Page 16 (section 2.3.1 Errors)
   * of the OpenGL 4.5 spec says:
   *
   *     "If the generating command modifies values through a pointer argu-
   *     ment, no change is made to these values."
   */
   for (int i = 0; i < uniformCount; i++) {
      if (!_mesa_program_resource_find_index(shProg, GL_UNIFORM,
            _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniformsiv(index)");
                  for (int i = 0; i < uniformCount; i++) {
      res = _mesa_program_resource_find_index(shProg, GL_UNIFORM,
         if (!_mesa_program_resource_prop(shProg, res, uniformIndices[i],
                     }
      static struct gl_uniform_storage *
   validate_uniform_parameters(GLint location, GLsizei count,
                           {
      if (shProg == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(program not linked)", caller);
               /* From page 12 (page 26 of the PDF) of the OpenGL 2.1 spec:
   *
   *     "If a negative number is provided where an argument of type sizei or
   *     sizeiptr is specified, the error INVALID_VALUE is generated."
   */
   if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(count < 0)", caller);
               /* Check that the given location is in bounds of uniform remap table.
   * Unlinked programs will have NumUniformRemapTable == 0, so we can take
   * the shProg->data->LinkStatus check out of the main path.
   */
   if (unlikely(location >= (GLint) shProg->NumUniformRemapTable)) {
      if (!shProg->data->LinkStatus)
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(program not linked)",
      else
                              if (location == -1) {
      if (!shProg->data->LinkStatus)
                              /* Page 82 (page 96 of the PDF) of the OpenGL 2.1 spec says:
   *
   *     "If any of the following conditions occur, an INVALID_OPERATION
   *     error is generated by the Uniform* commands, and no uniform values
   *     are changed:
   *
   *     ...
   *
   *         - if no variable with a location of location exists in the
   *           program object currently in use and location is not -1,
   *         - if count is greater than one, and the uniform declared in the
   *           shader is not an array variable,
   */
   if (location < -1 || !shProg->UniformRemapTable[location]) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(location=%d)",
                     /* If the driver storage pointer in remap table is -1, we ignore silently.
   *
   * GL_ARB_explicit_uniform_location spec says:
   *     "What happens if Uniform* is called with an explicitly defined
   *     uniform location, but that uniform is deemed inactive by the
   *     linker?
   *
   *     RESOLVED: The call is ignored for inactive uniform variables and
   *     no error is generated."
   *
   */
   if (shProg->UniformRemapTable[location] ==
      INACTIVE_UNIFORM_EXPLICIT_LOCATION)
                  /* Even though no location is assigned to a built-in uniform and this
   * function should already have returned NULL, this test makes it explicit
   * that we are not allowing to update the value of a built-in.
   */
   if (uni->builtin)
            if (uni->array_elements == 0) {
      if (count > 1) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           assert((location - uni->remap_location) == 0);
      } else {
      /* The array index specified by the uniform location is just the uniform
   * location minus the base location of of the uniform.
   */
            /* If the uniform is an array, check that array_index is in bounds.
   * array_index is unsigned so no need to check for less than zero.
   */
   if (*array_index >= uni->array_elements) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(location=%d)",
               }
      }
      /**
   * Called via glGetUniform[fiui]v() to get the current value of a uniform.
   */
   extern "C" void
   _mesa_get_uniform(struct gl_context *ctx, GLuint program, GLint location,
      GLsizei bufSize, enum glsl_base_type returnType,
      {
      struct gl_shader_program *shProg =
                  struct gl_uniform_storage *const uni =
      validate_uniform_parameters(location, 1, &offset,
      if (uni == NULL) {
      /* For glGetUniform, page 264 (page 278 of the PDF) of the OpenGL 2.1
   * spec says:
   *
   *     "The error INVALID_OPERATION is generated if program has not been
   *     linked successfully, or if location is not a valid location for
   *     program."
   *
   * For glUniform, page 82 (page 96 of the PDF) of the OpenGL 2.1 spec
   * says:
   *
   *     "If the value of location is -1, the Uniform* commands will
   *     silently ignore the data passed in, and the current uniform
   *     values will not be changed."
   *
   * Allowing -1 for the location parameter of glUniform allows
   * applications to avoid error paths in the case that, for example, some
   * uniform variable is removed by the compiler / linker after
   * optimization.  In this case, the new value of the uniform is dropped
   * on the floor.  For the case of glGetUniform, there is nothing
   * sensible to do for a location of -1.
   *
   * If the location was -1, validate_unfirom_parameters will return NULL
   * without raising an error.  Raise the error here.
   */
   if (location == -1) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetUniform(location=%d)",
                           {
      unsigned elements = uni->type->components();
            const int rmul = glsl_base_type_is_64bit(returnType) ? 2 : 1;
            if ((uni->type->is_sampler() || uni->type->is_image()) &&
      !uni->is_bindless) {
   /* Non-bindless samplers/images are represented using unsigned integer
   * 32-bit, while bindless handles are 64-bit.
   */
               /* Calculate the source base address *BEFORE* modifying elements to
   * account for the size of the user's buffer.
   */
   const union gl_constant_value *src;
   if (ctx->Const.PackedDriverUniformStorage &&
                     /* 16-bit uniforms are packed. */
   if (glsl_base_type_is_16bit(uni->type->base_type)) {
      dword_elements = DIV_ROUND_UP(components, 2) *
               src = (gl_constant_value *) uni->driver_storage[0].data +
      } else {
                  assert(returnType == GLSL_TYPE_FLOAT || returnType == GLSL_TYPE_INT ||
                  /* doubles have a different size than the other 3 types */
   unsigned bytes = sizeof(src[0]) * elements * rmul;
   if (bufSize < 0 || bytes > (unsigned) bufSize) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           /* If the return type and the uniform's native type are "compatible,"
   * just memcpy the data.  If the types are not compatible, perform a
   * slower convert-and-copy process.
   */
   if (returnType == uni->type->base_type ||
      ((returnType == GLSL_TYPE_INT || returnType == GLSL_TYPE_UINT) &&
   (uni->type->is_sampler() || uni->type->is_image())) ||
   (returnType == GLSL_TYPE_UINT64 && uni->is_bindless)) {
      } else {
      union gl_constant_value *const dst =
         /* This code could be optimized by putting the loop inside the switch
   * statements.  However, this is not expected to be
   * performance-critical code.
   */
   for (unsigned i = 0; i < elements; i++) {
                     if (glsl_base_type_is_16bit(uni->type->base_type)) {
      unsigned column = i / components;
                     switch (returnType) {
   case GLSL_TYPE_FLOAT:
      switch (uni->type->base_type) {
   case GLSL_TYPE_FLOAT16:
      dst[didx].f = _mesa_half_to_float(((uint16_t*)src)[sidx]);
      case GLSL_TYPE_UINT:
      dst[didx].f = (float) src[sidx].u;
      case GLSL_TYPE_INT:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_IMAGE:
      dst[didx].f = (float) src[sidx].i;
      case GLSL_TYPE_BOOL:
      dst[didx].f = src[sidx].i ? 1.0f : 0.0f;
      case GLSL_TYPE_DOUBLE: {
      double tmp;
   memcpy(&tmp, &src[sidx].f, sizeof(tmp));
   dst[didx].f = tmp;
      }
   case GLSL_TYPE_UINT64: {
      uint64_t tmp;
   memcpy(&tmp, &src[sidx].u, sizeof(tmp));
   dst[didx].f = tmp;
      }
   case GLSL_TYPE_INT64: {
      uint64_t tmp;
   memcpy(&tmp, &src[sidx].i, sizeof(tmp));
   dst[didx].f = tmp;
      }
   default:
      assert(!"Should not get here.");
                  case GLSL_TYPE_DOUBLE:
      switch (uni->type->base_type) {
   case GLSL_TYPE_FLOAT16: {
      double f = _mesa_half_to_float(((uint16_t*)src)[sidx]);
   memcpy(&dst[didx].f, &f, sizeof(f));
      }
   case GLSL_TYPE_UINT: {
      double tmp = src[sidx].u;
   memcpy(&dst[didx].f, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_INT:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_IMAGE: {
      double tmp = src[sidx].i;
   memcpy(&dst[didx].f, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_BOOL: {
      double tmp = src[sidx].i ? 1.0 : 0.0;
   memcpy(&dst[didx].f, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_FLOAT: {
      double tmp = src[sidx].f;
   memcpy(&dst[didx].f, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_UINT64: {
      uint64_t tmpu;
   double tmp;
   memcpy(&tmpu, &src[sidx].u, sizeof(tmpu));
   tmp = tmpu;
   memcpy(&dst[didx].f, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_INT64: {
      int64_t tmpi;
   double tmp;
   memcpy(&tmpi, &src[sidx].i, sizeof(tmpi));
   tmp = tmpi;
   memcpy(&dst[didx].f, &tmp, sizeof(tmp));
      }
   default:
      assert(!"Should not get here.");
                  case GLSL_TYPE_INT:
      switch (uni->type->base_type) {
   case GLSL_TYPE_FLOAT:
      /* While the GL 3.2 core spec doesn't explicitly
   * state how conversion of float uniforms to integer
   * values works, in section 6.2 "State Tables" on
   * page 267 it says:
   *
   *     "Unless otherwise specified, when floating
   *      point state is returned as integer values or
   *      integer state is returned as floating-point
   *      values it is converted in the fashion
   *      described in section 6.1.2"
   *
   * That section, on page 248, says:
   *
   *     "If GetIntegerv or GetInteger64v are called,
   *      a floating-point value is rounded to the
   *      nearest integer..."
   */
   dst[didx].i = (int64_t) roundf(src[sidx].f);
      case GLSL_TYPE_FLOAT16:
      dst[didx].i =
            case GLSL_TYPE_BOOL:
      dst[didx].i = src[sidx].i ? 1 : 0;
      case GLSL_TYPE_UINT:
      dst[didx].i = MIN2(src[sidx].i, INT_MAX);
      case GLSL_TYPE_DOUBLE: {
      double tmp;
   memcpy(&tmp, &src[sidx].f, sizeof(tmp));
   dst[didx].i = (int64_t) round(tmp);
      }
   case GLSL_TYPE_UINT64: {
      uint64_t tmp;
   memcpy(&tmp, &src[sidx].u, sizeof(tmp));
   dst[didx].i = tmp;
      }
   case GLSL_TYPE_INT64: {
      int64_t tmp;
   memcpy(&tmp, &src[sidx].i, sizeof(tmp));
   dst[didx].i = tmp;
      }
   default:
      assert(!"Should not get here.");
                  case GLSL_TYPE_UINT:
      switch (uni->type->base_type) {
   case GLSL_TYPE_FLOAT:
      /* The spec isn't terribly clear how to handle negative
   * values with an unsigned return type.
   *
   * GL 4.5 section 2.2.2 ("Data Conversions for State
   * Query Commands") says:
   *
   * "If a value is so large in magnitude that it cannot be
   *  represented by the returned data type, then the nearest
   *  value representable using the requested type is
   *  returned."
   */
   dst[didx].u = src[sidx].f < 0.0f ?
            case GLSL_TYPE_FLOAT16: {
      float f = _mesa_half_to_float(((uint16_t*)src)[sidx]);
   dst[didx].u = f < 0.0f ? 0u : (uint32_t)roundf(f);
      }
   case GLSL_TYPE_BOOL:
      dst[didx].i = src[sidx].i ? 1 : 0;
      case GLSL_TYPE_INT:
      dst[didx].i = MAX2(src[sidx].i, 0);
      case GLSL_TYPE_DOUBLE: {
      double tmp;
   memcpy(&tmp, &src[sidx].f, sizeof(tmp));
   dst[didx].u = tmp < 0.0 ? 0u : (uint32_t) round(tmp);
      }
   case GLSL_TYPE_UINT64: {
      uint64_t tmp;
   memcpy(&tmp, &src[sidx].u, sizeof(tmp));
   dst[didx].i = MIN2(tmp, INT_MAX);
      }
   case GLSL_TYPE_INT64: {
      int64_t tmp;
   memcpy(&tmp, &src[sidx].i, sizeof(tmp));
   dst[didx].i = MAX2(tmp, 0);
      }
   default:
                     case GLSL_TYPE_INT64:
      switch (uni->type->base_type) {
   case GLSL_TYPE_UINT: {
      uint64_t tmp = src[sidx].u;
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_INT:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_IMAGE: {
      int64_t tmp = src[sidx].i;
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_BOOL: {
      int64_t tmp = src[sidx].i ? 1.0f : 0.0f;
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_UINT64: {
      uint64_t u64;
   memcpy(&u64, &src[sidx].u, sizeof(u64));
   int64_t tmp = MIN2(u64, INT_MAX);
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_FLOAT: {
      int64_t tmp = (int64_t) roundf(src[sidx].f);
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_FLOAT16: {
      float f = _mesa_half_to_float(((uint16_t*)src)[sidx]);
   int64_t tmp = (int64_t) roundf(f);
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_DOUBLE: {
      double d;
   memcpy(&d, &src[sidx].f, sizeof(d));
   int64_t tmp = (int64_t) round(d);
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   default:
      assert(!"Should not get here.");
                  case GLSL_TYPE_UINT64:
      switch (uni->type->base_type) {
   case GLSL_TYPE_UINT: {
      uint64_t tmp = src[sidx].u;
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_INT:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_IMAGE: {
      int64_t tmp = MAX2(src[sidx].i, 0);
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_BOOL: {
      int64_t tmp = src[sidx].i ? 1.0f : 0.0f;
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_INT64: {
      uint64_t i64;
   memcpy(&i64, &src[sidx].i, sizeof(i64));
   uint64_t tmp = MAX2(i64, 0);
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_FLOAT: {
      uint64_t tmp = src[sidx].f < 0.0f ?
         memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_FLOAT16: {
      float f = _mesa_half_to_float(((uint16_t*)src)[sidx]);
   uint64_t tmp = f < 0.0f ? 0ull : (uint64_t) roundf(f);
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   case GLSL_TYPE_DOUBLE: {
      double d;
   memcpy(&d, &src[sidx].f, sizeof(d));
   uint64_t tmp = (d < 0.0) ? 0ull : (uint64_t) round(d);
   memcpy(&dst[didx].u, &tmp, sizeof(tmp));
      }
   default:
      assert(!"Should not get here.");
                  default:
      assert(!"Should not get here.");
                  }
      static void
   log_uniform(const void *values, enum glsl_base_type basicType,
      unsigned rows, unsigned cols, unsigned count,
   bool transpose,
   const struct gl_shader_program *shProg,
   GLint location,
      {
         const union gl_constant_value *v = (const union gl_constant_value *) values;
   const unsigned elems = rows * cols * count;
            printf("Mesa: set program %u %s \"%s\" (loc %d, type \"%s\", "
   "transpose = %s) to: ",
   shProg->Name, extra, uni->name.string, location, glsl_get_type_name(uni->type),
   transpose ? "true" : "false");
   for (unsigned i = 0; i < elems; i++) {
      printf(", ");
            switch (basicType) {
   printf("%u ", v[i].u);
   break;
         printf("%d ", v[i].i);
   break;
         case GLSL_TYPE_UINT64: {
      uint64_t tmp;
   memcpy(&tmp, &v[i * 2].u, sizeof(tmp));
   printf("%" PRIu64 " ", tmp);
      }
   case GLSL_TYPE_INT64: {
      int64_t tmp;
   memcpy(&tmp, &v[i * 2].u, sizeof(tmp));
   printf("%" PRId64 " ", tmp);
      }
   printf("%g ", v[i].f);
   break;
         case GLSL_TYPE_DOUBLE: {
      double tmp;
   memcpy(&tmp, &v[i * 2].f, sizeof(tmp));
   printf("%g ", tmp);
      }
   assert(!"Should not get here.");
   break;
            }
   printf("\n");
      }
      #if 0
   static void
   log_program_parameters(const struct gl_shader_program *shProg)
   {
      for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      continue;
                     printf("Program %d %s shader parameters:\n",
         for (unsigned j = 0; j < prog->Parameters->NumParameters; j++) {
         prog->Parameters->Parameters[j].Name,
                  pvo,
   prog->Parameters->ParameterValues + pvo,
   prog->Parameters->ParameterValues[pvo].f,
   prog->Parameters->ParameterValues[pvo + 1].f,
         }
      }
   #endif
      /**
   * Propagate some values from uniform backing storage to driver storage
   *
   * Values propagated from uniform backing storage to driver storage
   * have all format / type conversions previously requested by the
   * driver applied.  This function is most often called by the
   * implementations of \c glUniform1f, etc. and \c glUniformMatrix2f,
   * etc.
   *
   * \param uni          Uniform whose data is to be propagated to driver storage
   * \param array_index  If \c uni is an array, this is the element of
   *                     the array to be propagated.
   * \param count        Number of array elements to propagate.
   */
   extern "C" void
   _mesa_propagate_uniforms_to_driver_storage(struct gl_uniform_storage *uni,
         unsigned array_index,
   {
               const unsigned components = uni->type->vector_elements;
   const unsigned vectors = uni->type->matrix_columns;
            /* Store the data in the driver's requested type in the driver's storage
   * areas.
   */
            for (i = 0; i < uni->num_driver_storage; i++) {
      struct gl_uniform_driver_storage *const store = &uni->driver_storage[i];
   uint8_t *dst = (uint8_t *) store->data;
   store->element_stride - (vectors * store->vector_stride);
         (uint8_t *) (&uni->storage[array_index * (dmul * components * vectors)].i);
      #if 0
         printf("%s: %p[%d] components=%u vectors=%u count=%u vector_stride=%u "
   "extra_stride=%u\n",
   __func__, dst, array_index, components,
   #endif
                     switch (store->format) {
   unsigned j;
   unsigned v;
      if (src_vector_byte_stride == store->vector_stride) {
      if (extra_stride) {
      for (j = 0; j < count; j++) {
      memcpy(dst, src, src_vector_byte_stride * vectors);
                        } else {
      /* Unigine Heaven benchmark gets here */
   memcpy(dst, src, src_vector_byte_stride * vectors * count);
   src += src_vector_byte_stride * vectors * count;
         } else {
      for (j = 0; j < count; j++) {
      for (v = 0; v < vectors; v++) {
      memcpy(dst, src, src_vector_byte_stride);
   src += src_vector_byte_stride;
                     }
   break;
                  const int *isrc = (const int *) src;
   unsigned j;
   unsigned v;
   unsigned c;
      for (j = 0; j < count; j++) {
      for (v = 0; v < vectors; v++) {
         ((float *) dst)[c] = (float) *isrc;
   isrc++;
                              }
   break;
                  assert(!"Should not get here.");
   break;
               }
         static void
   associate_uniform_storage(struct gl_context *ctx,
               {
      struct gl_program_parameter_list *params = prog->Parameters;
                     /* After adding each uniform to the parameter list, connect the storage for
   * the parameter with the tracking structure used by the API for the
   * uniform.
   */
   unsigned last_location = unsigned(~0);
   for (unsigned i = 0; i < params->NumParameters; i++) {
      if (params->Parameters[i].Type != PROGRAM_UNIFORM)
                     struct gl_uniform_storage *storage =
            /* Do not associate any uniform storage to built-in uniforms */
   if (storage->builtin)
            if (location != last_location) {
                     int dmul;
   if (ctx->Const.PackedDriverUniformStorage && !prog->info.use_legacy_math_rules) {
         } else {
                  switch (storage->type->base_type) {
   case GLSL_TYPE_UINT64:
      if (storage->type->vector_elements > 2)
            case GLSL_TYPE_UINT:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_UINT8:
      assert(ctx->Const.NativeIntegers);
   format = uniform_native;
   columns = 1;
      case GLSL_TYPE_INT64:
      if (storage->type->vector_elements > 2)
            case GLSL_TYPE_INT:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_INT8:
      format =
         columns = 1;
      case GLSL_TYPE_DOUBLE:
      if (storage->type->vector_elements > 2)
            case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
      format = uniform_native;
   columns = storage->type->matrix_columns;
      case GLSL_TYPE_BOOL:
      format = uniform_native;
   columns = 1;
      case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_SUBROUTINE:
      format = uniform_native;
   columns = 1;
      case GLSL_TYPE_ATOMIC_UINT:
   case GLSL_TYPE_ARRAY:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_ERROR:
   case GLSL_TYPE_INTERFACE:
   case GLSL_TYPE_COOPERATIVE_MATRIX:
      assert(!"Should not get here.");
               unsigned pvo = params->Parameters[i].ValueOffset;
   _mesa_uniform_attach_driver_storage(storage, dmul * columns, dmul,
                  /* When a bindless sampler/image is bound to a texture/image unit, we
   * have to overwrite the constant value by the resident handle
   * directly in the constant buffer before the next draw. One solution
   * is to keep track a pointer to the base of the data.
   */
   if (storage->is_bindless && (prog->sh.NumBindlessSamplers ||
                                       if (storage->type->without_array()->is_sampler()) {
      assert(unit >= 0 && unit < prog->sh.NumBindlessSamplers);
   prog->sh.BindlessSamplers[unit].data =
      } else if (storage->type->without_array()->is_image()) {
      assert(unit >= 0 && unit < prog->sh.NumBindlessImages);
   prog->sh.BindlessImages[unit].data =
                     /* After attaching the driver's storage to the uniform, propagate any
   * data from the linker's backing store.  This will cause values from
   * initializers in the source code to be copied over.
   */
   unsigned array_elements = MAX2(1, storage->array_elements);
   if (ctx->Const.PackedDriverUniformStorage && !prog->info.use_legacy_math_rules &&
      (storage->is_bindless || !storage->type->contains_opaque())) {
   const int dmul = storage->type->is_64bit() ? 2 : 1;
   const unsigned components =
                  for (unsigned s = 0; s < storage->num_driver_storage; s++) {
      gl_constant_value *uni_storage = (gl_constant_value *)
         memcpy(uni_storage, storage->storage,
               } else {
      _mesa_propagate_uniforms_to_driver_storage(storage, 0,
            last_location = location;
         }
         void
   _mesa_ensure_and_associate_uniform_storage(struct gl_context *ctx,
               {
      /* Avoid reallocation of the program parameter list, because the uniform
   * storage is only associated with the original parameter list.
   */
   _mesa_reserve_parameter_storage(prog->Parameters, required_space,
            /* This has to be done last.  Any operation the can cause
   * prog->ParameterValues to get reallocated (e.g., anything that adds a
   * program constant) has to happen before creating this linkage.
   */
      }
         /**
   * Return printable string for a given GLSL_TYPE_x
   */
   static const char *
   glsl_type_name(enum glsl_base_type type)
   {
      switch (type) {
   case GLSL_TYPE_UINT:
         case GLSL_TYPE_INT:
         case GLSL_TYPE_FLOAT:
         case GLSL_TYPE_DOUBLE:
         case GLSL_TYPE_UINT64:
         case GLSL_TYPE_INT64:
         case GLSL_TYPE_BOOL:
         case GLSL_TYPE_SAMPLER:
         case GLSL_TYPE_IMAGE:
         case GLSL_TYPE_ATOMIC_UINT:
         case GLSL_TYPE_STRUCT:
         case GLSL_TYPE_INTERFACE:
         case GLSL_TYPE_ARRAY:
         case GLSL_TYPE_VOID:
         case GLSL_TYPE_ERROR:
         default:
            }
         static struct gl_uniform_storage *
   validate_uniform(GLint location, GLsizei count, const GLvoid *values,
                     {
      struct gl_uniform_storage *const uni =
      validate_uniform_parameters(location, count, offset,
      if (uni == NULL)
            if (uni->type->is_matrix()) {
      /* Can't set matrix uniforms (like mat4) with glUniform */
   _mesa_error(ctx, GL_INVALID_OPERATION,
                           /* Verify that the types are compatible. */
            if (components != src_components) {
      /* glUniformN() must match float/vecN type */
   _mesa_error(ctx, GL_INVALID_OPERATION,
               "glUniform%u(\"%s\"@%u has %u components, not %u)",
               bool match;
   switch (uni->type->base_type) {
   case GLSL_TYPE_BOOL:
      match = (basicType != GLSL_TYPE_DOUBLE);
      case GLSL_TYPE_SAMPLER:
      match = (basicType == GLSL_TYPE_INT);
      case GLSL_TYPE_IMAGE:
      match = (basicType == GLSL_TYPE_INT && _mesa_is_desktop_gl(ctx));
      case GLSL_TYPE_FLOAT16:
      match = basicType == GLSL_TYPE_FLOAT;
      default:
      match = (basicType == uni->type->base_type);
               if (!match) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               "glUniform%u(\"%s\"@%d is %s, not %s)",
   src_components, uni->name.string, location,
               if (unlikely(ctx->_Shader->Flags & GLSL_UNIFORMS)) {
      log_uniform(values, basicType, components, 1, count,
               /* Page 100 (page 116 of the PDF) of the OpenGL 3.0 spec says:
   *
   *     "Setting a sampler's value to i selects texture image unit number
   *     i. The values of i range from zero to the implementation- dependent
   *     maximum supported number of texture image units."
   *
   * In addition, table 2.3, "Summary of GL errors," on page 17 (page 33 of
   * the PDF) says:
   *
   *     "Error         Description                    Offending command
   *                                                   ignored?
   *     ...
   *     INVALID_VALUE  Numeric argument out of range  Yes"
   *
   * Based on that, when an invalid sampler is specified, we generate a
   * GL_INVALID_VALUE error and ignore the command.
   */
   if (uni->type->is_sampler()) {
      for (int i = 0; i < count; i++) {
               /* check that the sampler (tex unit index) is legal */
   if (texUnit >= ctx->Const.MaxCombinedTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     }
   /* We need to reset the validate flag on changes to samplers in case
   * two different sampler types are set to the same texture unit.
   */
               if (uni->type->is_image()) {
      for (int i = 0; i < count; i++) {
               /* check that the image unit is legal */
   if (unit < 0 || unit >= (int)ctx->Const.MaxImageUnits) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                                    }
      void
   _mesa_flush_vertices_for_uniforms(struct gl_context *ctx,
         {
      /* Opaque uniforms have no storage unless they are bindless */
   if (!uni->is_bindless && uni->type->contains_opaque()) {
      /* Samplers flush on demand and ignore redundant updates. */
   if (!uni->type->is_sampler())
                     uint64_t new_driver_state = 0;
            while (mask) {
               assert(index < MESA_SHADER_STAGES);
               FLUSH_VERTICES(ctx, new_driver_state ? 0 : _NEW_PROGRAM_CONSTANTS, 0);
      }
      static bool
   copy_uniforms_to_storage(gl_constant_value *storage,
                           struct gl_uniform_storage *uni,
   {
      const gl_constant_value *src = (const gl_constant_value*)values;
   bool copy_as_uint64 = uni->is_bindless &&
                  if (!uni->type->is_boolean() && !copy_as_uint64 && !copy_to_float16) {
               if (!memcmp(storage, values, size))
            if (flush)
            memcpy(storage, values, size);
      } else if (copy_to_float16) {
      assert(ctx->Const.PackedDriverUniformStorage);
   const unsigned dst_components = align(components, 2);
            int i = 0;
            if (flush) {
      /* Find the first element that's different. */
   for (; i < count; i++) {
      for (; c < components; c++) {
      if (dst[c] != _mesa_float_to_half(src[c].f)) {
      _mesa_flush_vertices_for_uniforms(ctx, uni);
   flush = false;
         }
   c = 0;
   dst += dst_components;
         break_loops:
      if (flush)
               /* Set the remaining elements. We know that at least 1 element is
   * different and that we have flushed.
   */
   for (; i < count; i++) {
                     c = 0;
   dst += dst_components;
                  } else if (copy_as_uint64) {
      const unsigned elems = components * count;
   uint64_t *dst = (uint64_t*)storage;
            if (flush) {
      /* Find the first element that's different. */
   for (; i < elems; i++) {
      if (dst[i] != src[i].u) {
      _mesa_flush_vertices_for_uniforms(ctx, uni);
   flush = false;
         }
   if (flush)
               /* Set the remaining elements. We know that at least 1 element is
   * different and that we have flushed.
   */
   for (; i < elems; i++)
               } else {
      const unsigned elems = components * count;
            if (basicType == GLSL_TYPE_FLOAT) {
               if (flush) {
      /* Find the first element that's different. */
   for (; i < elems; i++) {
      if (dst[i].u !=
      (src[i].f != 0.0f ? ctx->Const.UniformBooleanTrue : 0)) {
   _mesa_flush_vertices_for_uniforms(ctx, uni);
   flush = false;
         }
   if (flush)
               /* Set the remaining elements. We know that at least 1 element is
   * different and that we have flushed.
   */
                     } else {
               if (flush) {
      /* Find the first element that's different. */
   for (; i < elems; i++) {
      if (dst[i].u !=
      (src[i].u ? ctx->Const.UniformBooleanTrue : 0)) {
   _mesa_flush_vertices_for_uniforms(ctx, uni);
   flush = false;
         }
   if (flush)
               /* Set the remaining elements. We know that at least 1 element is
   * different and that we have flushed.
   */
                           }
         /**
   * Called via glUniform*() functions.
   */
   extern "C" void
   _mesa_uniform(GLint location, GLsizei count, const GLvoid *values,
               {
      unsigned offset;
            struct gl_uniform_storage *uni;
   if (_mesa_is_no_error_enabled(ctx)) {
      /* From Seciton 7.6 (UNIFORM VARIABLES) of the OpenGL 4.5 spec:
   *
   *   "If the value of location is -1, the Uniform* commands will
   *   silently ignore the data passed in, and the current uniform values
   *   will not be changed.
   */
   if (location == -1)
            if (location >= (int)shProg->NumUniformRemapTable)
            uni = shProg->UniformRemapTable[location];
   if (!uni || uni == INACTIVE_UNIFORM_EXPLICIT_LOCATION)
            /* The array index specified by the uniform location is just the
   * uniform location minus the base location of of the uniform.
   */
   assert(uni->array_elements > 0 || location == (int)uni->remap_location);
      } else {
      uni = validate_uniform(location, count, values, &offset, ctx, shProg,
         if (!uni)
                        /* Page 82 (page 96 of the PDF) of the OpenGL 2.1 spec says:
   *
   *     "When loading N elements starting at an arbitrary position k in a
   *     uniform declared as an array, elements k through k + N - 1 in the
   *     array will be replaced with the new values. Values for any array
   *     element that exceeds the highest array element index used, as
   *     reported by GetActiveUniform, will be ignored by the GL."
   *
   * Clamp 'count' to a valid value.  Note that for non-arrays a count > 1
   * will have already generated an error.
   */
   if (uni->array_elements != 0) {
                  /* Store the data in the "actual type" backing storage for the uniform.
   */
   bool ctx_flushed = false;
   gl_constant_value *storage;
   if (ctx->Const.PackedDriverUniformStorage &&
      (uni->is_bindless || !uni->type->contains_opaque())) {
   for (unsigned s = 0; s < uni->num_driver_storage; s++) {
               /* 16-bit uniforms are packed. */
                                 if (copy_uniforms_to_storage(storage, uni, ctx, count, values, size_mul,
               } else {
      storage = &uni->storage[size_mul * components * offset];
   if (copy_uniforms_to_storage(storage, uni, ctx, count, values, size_mul,
            _mesa_propagate_uniforms_to_driver_storage(uni, offset, count);
         }
   /* Return early if possible. Bindless samplers need to be processed
   * because of the !sampler->bound codepath below.
   */
   if (!ctx_flushed && !(uni->type->is_sampler() && uni->is_bindless))
            /* If the uniform is a sampler, do the extra magic necessary to propagate
   * the changes through.
   */
   if (uni->type->is_sampler()) {
      /* Note that samplers are the only uniforms that don't call
   * FLUSH_VERTICES above.
   */
   bool flushed = false;
   bool any_changed = false;
                     for (int i = 0; i < MESA_SHADER_STAGES; i++) {
               /* If the shader stage doesn't use the sampler uniform, skip this. */
                  bool changed = false;
   for (int j = 0; j < count; j++) {
                     if (uni->is_bindless) {
                     /* Mark this bindless sampler as bound to a texture unit.
   */
   if (sampler->unit != value || !sampler->bound) {
      if (!flushed) {
      FLUSH_VERTICES(ctx, _NEW_TEXTURE_OBJECT, 0);
      }
   sampler->unit = value;
      }
   sampler->bound = true;
      } else {
      if (sh->Program->SamplerUnits[unit] != value) {
      if (!flushed) {
      FLUSH_VERTICES(ctx, _NEW_TEXTURE_OBJECT, 0);
      }
   sh->Program->SamplerUnits[unit] = value;
                     if (changed) {
      struct gl_program *const prog = sh->Program;
   _mesa_update_shader_textures_used(shProg, prog);
                  if (any_changed)
         else
               /* If the uniform is an image, update the mapping from image
   * uniforms to image units present in the shader data structure.
   */
   if (uni->type->is_image()) {
      for (int i = 0; i < MESA_SHADER_STAGES; i++) {
               /* If the shader stage doesn't use the image uniform, skip this. */
                  for (int j = 0; j < count; j++) {
                     if (uni->is_bindless) {
                     /* Mark this bindless image as bound to an image unit.
   */
   image->unit = value;
   image->bound = true;
      } else {
                              }
         static bool
   copy_uniform_matrix_to_storage(struct gl_context *ctx,
                                 gl_constant_value *storage,
   struct gl_uniform_storage *const uni,
   unsigned count, const void *values,
   {
      const unsigned elements = components * vectors;
            if (uni->type->base_type == GLSL_TYPE_FLOAT16) {
      assert(ctx->Const.PackedDriverUniformStorage);
   const unsigned dst_components = align(components, 2);
            if (!transpose) {
                              if (flush) {
      /* Find the first element that's different. */
   for (; i < count; i++) {
      for (; c < cols; c++) {
      for (; r < rows; r++) {
      if (dst[(c * dst_components) + r] !=
      _mesa_float_to_half(src[(c * components) + r])) {
   _mesa_flush_vertices_for_uniforms(ctx, uni);
   flush = false;
         }
      }
   c = 0;
                  break_loops_16bit:
      if (flush)
               /* Set the remaining elements. We know that at least 1 element is
   * different and that we have flushed.
   */
   for (; i < count; i++) {
      for (; c < cols; c++) {
      for (; r < rows; r++) {
      dst[(c * dst_components) + r] =
      }
      }
   c = 0;
   dst += dst_elements;
      }
      } else {
      /* Transpose the matrix. */
                           if (flush) {
      /* Find the first element that's different. */
   for (; i < count; i++) {
      for (; r < rows; r++) {
      for (; c < cols; c++) {
      if (dst[(c * dst_components) + r] !=
      _mesa_float_to_half(src[c + (r * vectors)])) {
   _mesa_flush_vertices_for_uniforms(ctx, uni);
   flush = false;
         }
      }
   r = 0;
                  break_loops_16bit_transpose:
      if (flush)
               /* Set the remaining elements. We know that at least 1 element is
   * different and that we have flushed.
   */
   for (; i < count; i++) {
      for (; r < rows; r++) {
      for (; c < cols; c++) {
      dst[(c * dst_components) + r] =
      }
      }
   r = 0;
   dst += elements;
      }
         } else if (!transpose) {
      if (!memcmp(storage, values, size))
            if (flush)
            memcpy(storage, values, size);
      } else if (basicType == GLSL_TYPE_FLOAT) {
      /* Transpose the matrix. */
   const float *src = (const float *)values;
                     if (flush) {
      /* Find the first element that's different. */
   for (; i < count; i++) {
      for (; r < rows; r++) {
      for (; c < cols; c++) {
      if (dst[(c * components) + r] != src[c + (r * vectors)]) {
      _mesa_flush_vertices_for_uniforms(ctx, uni);
   flush = false;
         }
      }
   r = 0;
   dst += elements;
            break_loops:
      if (flush)
               /* Set the remaining elements. We know that at least 1 element is
   * different and that we have flushed.
   */
   for (; i < count; i++) {
      for (; r < rows; r++) {
      for (; c < cols; c++)
            }
   r = 0;
   dst += elements;
      }
      } else {
      assert(basicType == GLSL_TYPE_DOUBLE);
   const double *src = (const double *)values;
                     if (flush) {
      /* Find the first element that's different. */
   for (; i < count; i++) {
      for (; r < rows; r++) {
      for (; c < cols; c++) {
      if (dst[(c * components) + r] != src[c + (r * vectors)]) {
      _mesa_flush_vertices_for_uniforms(ctx, uni);
   flush = false;
         }
      }
   r = 0;
   dst += elements;
            break_loops2:
      if (flush)
               /* Set the remaining elements. We know that at least 1 element is
   * different and that we have flushed.
   */
   for (; i < count; i++) {
      for (; r < rows; r++) {
      for (; c < cols; c++)
            }
   r = 0;
   dst += elements;
      }
         }
         /**
   * Called by glUniformMatrix*() functions.
   * Note: cols=2, rows=4  ==>  array[2] of vec4
   */
   extern "C" void
   _mesa_uniform_matrix(GLint location, GLsizei count,
                     {
      unsigned offset;
   struct gl_uniform_storage *const uni =
      validate_uniform_parameters(location, count, &offset,
      if (uni == NULL)
            /* GL_INVALID_VALUE is generated if `transpose' is not GL_FALSE.
   * http://www.khronos.org/opengles/sdk/docs/man/xhtml/glUniform.xml
   */
   if (transpose) {
      if (_mesa_is_gles2(ctx) && ctx->Version < 30) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                        if (!uni->type->is_matrix()) {
         "glUniformMatrix(non-matrix uniform)");
                  assert(basicType == GLSL_TYPE_FLOAT || basicType == GLSL_TYPE_DOUBLE);
            assert(!uni->type->is_sampler());
   const unsigned vectors = uni->type->matrix_columns;
            /* Verify that the types are compatible.  This is greatly simplified for
   * matrices because they can only have a float base type.
   */
   if (vectors != cols || components != rows) {
         "glUniformMatrix(matrix size mismatch)");
                  /* Section 2.11.7 (Uniform Variables) of the OpenGL 4.2 Core Profile spec
   * says:
   *
   *     "If any of the following conditions occur, an INVALID_OPERATION
   *     error is generated by the Uniform* commands, and no uniform values
   *     are changed:
   *
   *     ...
   *
   *     - if the uniform declared in the shader is not of type boolean and
   *       the type indicated in the name of the Uniform* command used does
   *       not match the type of the uniform"
   *
   * There are no Boolean matrix types, so we do not need to allow
   * GLSL_TYPE_BOOL here (as _mesa_uniform does).
   */
   if (uni->type->base_type != basicType &&
      !(uni->type->base_type == GLSL_TYPE_FLOAT16 &&
         _mesa_error(ctx, GL_INVALID_OPERATION,
               "glUniformMatrix%ux%u(\"%s\"@%d is %s, not %s)",
   cols, rows, uni->name.string, location,
               if (unlikely(ctx->_Shader->Flags & GLSL_UNIFORMS)) {
         bool(transpose), shProg, location, uni);
            /* Page 82 (page 96 of the PDF) of the OpenGL 2.1 spec says:
   *
   *     "When loading N elements starting at an arbitrary position k in a
   *     uniform declared as an array, elements k through k + N - 1 in the
   *     array will be replaced with the new values. Values for any array
   *     element that exceeds the highest array element index used, as
   *     reported by GetActiveUniform, will be ignored by the GL."
   *
   * Clamp 'count' to a valid value.  Note that for non-arrays a count > 1
   * will have already generated an error.
   */
   if (uni->array_elements != 0) {
                  /* Store the data in the "actual type" backing storage for the uniform.
   */
   gl_constant_value *storage;
   const unsigned elements = components * vectors;
   if (ctx->Const.PackedDriverUniformStorage) {
               for (unsigned s = 0; s < uni->num_driver_storage; s++) {
               /* 16-bit uniforms are packed. */
                  storage = (gl_constant_value *)
                  if (copy_uniform_matrix_to_storage(ctx, storage, uni, count, values,
                           } else {
      storage =  &uni->storage[size_mul * elements * offset];
   if (copy_uniform_matrix_to_storage(ctx, storage, uni, count, values,
                           }
      static void
   update_bound_bindless_sampler_flag(struct gl_program *prog)
   {
               if (likely(!prog->sh.HasBoundBindlessSampler))
            for (i = 0; i < prog->sh.NumBindlessSamplers; i++) {
               if (sampler->bound)
      }
      }
      static void
   update_bound_bindless_image_flag(struct gl_program *prog)
   {
               if (likely(!prog->sh.HasBoundBindlessImage))
            for (i = 0; i < prog->sh.NumBindlessImages; i++) {
               if (image->bound)
      }
      }
      /**
   * Called via glUniformHandleui64*ARB() functions.
   */
   extern "C" void
   _mesa_uniform_handle(GLint location, GLsizei count, const GLvoid *values,
         {
      unsigned offset;
            if (_mesa_is_no_error_enabled(ctx)) {
      /* From Section 7.6 (UNIFORM VARIABLES) of the OpenGL 4.5 spec:
   *
   *   "If the value of location is -1, the Uniform* commands will
   *   silently ignore the data passed in, and the current uniform values
   *   will not be changed.
   */
   if (location == -1)
            uni = shProg->UniformRemapTable[location];
   if (!uni || uni == INACTIVE_UNIFORM_EXPLICIT_LOCATION)
            /* The array index specified by the uniform location is just the
   * uniform location minus the base location of of the uniform.
   */
   assert(uni->array_elements > 0 || location == (int)uni->remap_location);
      } else {
      uni = validate_uniform_parameters(location, count, &offset,
         if (!uni)
            if (!uni->is_bindless) {
      /* From section "Errors" of the ARB_bindless_texture spec:
   *
   * "The error INVALID_OPERATION is generated by
   *  UniformHandleui64{v}ARB if the sampler or image uniform being
   *  updated has the "bound_sampler" or "bound_image" layout qualifier."
   *
   * From section 4.4.6 of the ARB_bindless_texture spec:
   *
   * "In the absence of these qualifiers, sampler and image uniforms are
   *  considered "bound". Additionally, if GL_ARB_bindless_texture is
   *  not enabled, these uniforms are considered "bound"."
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
                        const unsigned components = uni->type->vector_elements;
            if (unlikely(ctx->_Shader->Flags & GLSL_UNIFORMS)) {
      log_uniform(values, GLSL_TYPE_UINT64, components, 1, count,
               /* Page 82 (page 96 of the PDF) of the OpenGL 2.1 spec says:
   *
   *     "When loading N elements starting at an arbitrary position k in a
   *     uniform declared as an array, elements k through k + N - 1 in the
   *     array will be replaced with the new values. Values for any array
   *     element that exceeds the highest array element index used, as
   *     reported by GetActiveUniform, will be ignored by the GL."
   *
   * Clamp 'count' to a valid value.  Note that for non-arrays a count > 1
   * will have already generated an error.
   */
   if (uni->array_elements != 0) {
                     /* Store the data in the "actual type" backing storage for the uniform.
   */
   if (ctx->Const.PackedDriverUniformStorage) {
               for (unsigned s = 0; s < uni->num_driver_storage; s++) {
      void *storage = (gl_constant_value *)
                                 if (!flushed) {
      _mesa_flush_vertices_for_uniforms(ctx, uni);
      }
      }
   if (!flushed)
      } else {
      void *storage = &uni->storage[size_mul * components * offset];
            if (!memcmp(storage, values, size))
            _mesa_flush_vertices_for_uniforms(ctx, uni);
   memcpy(storage, values, size);
               if (uni->type->is_sampler()) {
      /* Mark this bindless sampler as not bound to a texture unit because
   * it refers to a texture handle.
   */
   for (int i = 0; i < MESA_SHADER_STAGES; i++) {
               /* If the shader stage doesn't use the sampler uniform, skip this. */
                  for (int j = 0; j < count; j++) {
      unsigned unit = uni->opaque[i].index + offset + j;
                                             if (uni->type->is_image()) {
      /* Mark this bindless image as not bound to an image unit because it
   * refers to a texture handle.
   */
   for (int i = 0; i < MESA_SHADER_STAGES; i++) {
               /* If the shader stage doesn't use the sampler uniform, skip this. */
                  for (int j = 0; j < count; j++) {
      unsigned unit = uni->opaque[i].index + offset + j;
                                       }
      extern "C" bool
   _mesa_sampler_uniforms_are_valid(const struct gl_shader_program *shProg,
         {
      /* Shader does not have samplers. */
   if (shProg->data->NumUniformStorage == 0)
            if (!shProg->SamplersValidated) {
      snprintf(errMsg, errMsgLength,
                  }
      }
      extern "C" bool
   _mesa_sampler_uniforms_pipeline_are_valid(struct gl_pipeline_object *pipeline)
   {
      /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
   * OpenGL 4.1 spec says:
   *
   *     "[INVALID_OPERATION] is generated by any command that transfers
   *     vertices to the GL if:
   *
   *         ...
   *
   *         - Any two active samplers in the current program object are of
   *           different types, but refer to the same texture image unit.
   *
   *         - The number of active samplers in the program exceeds the
   *           maximum number of texture image units allowed."
            GLbitfield mask;
   GLbitfield TexturesUsed[MAX_COMBINED_TEXTURE_IMAGE_UNITS];
   unsigned active_samplers = 0;
   const struct gl_program **prog =
                        for (unsigned idx = 0; idx < ARRAY_SIZE(pipeline->CurrentProgram); idx++) {
      if (!prog[idx])
            mask = prog[idx]->SamplersUsed;
   while (mask) {
      const int s = u_bit_scan(&mask);
                  /* FIXME: Samplers are initialized to 0 and Mesa doesn't do a
   * great job of eliminating unused uniforms currently so for now
   * don't throw an error if two sampler types both point to 0.
   */
                  if (TexturesUsed[unit] & ~(1 << tgt)) {
      pipeline->InfoLog =
      ralloc_asprintf(pipeline,
         "Program %d: "
                                          if (active_samplers > MAX_COMBINED_TEXTURE_IMAGE_UNITS) {
      pipeline->InfoLog =
      ralloc_asprintf(pipeline,
                                 }
