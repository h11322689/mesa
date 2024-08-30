   /*
   * Copyright © 2018 Intel Corporation
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
      #include "nir.h"
   #include "gl_nir_linker.h"
   #include "compiler/glsl/ir_uniform.h" /* for gl_uniform_storage */
   #include "main/shader_types.h"
   #include "main/consts_exts.h"
      struct set_opaque_binding_closure {
      struct gl_shader_program *shader_prog;
   struct gl_program *prog;
   const nir_variable *var;
   int binding;
      };
      static void
   set_opaque_binding(struct set_opaque_binding_closure *data,
         {
      if (glsl_type_is_array(type) &&
      glsl_type_is_array(glsl_get_array_element(type))) {
            for (unsigned int i = 0; i < glsl_get_length(type); i++)
                        if (data->location < 0 ||
      data->location >= data->prog->sh.data->NumUniformStorage)
         struct gl_uniform_storage *storage =
                     for (unsigned int i = 0; i < elements; i++)
            for (int sh = 0; sh < MESA_SHADER_STAGES; sh++) {
               if (!shader)
         if (!storage->opaque[sh].active)
            if (glsl_type_is_sampler(storage->type)) {
                        if (storage->is_bindless) {
      if (index >= shader->Program->sh.NumBindlessSamplers)
         shader->Program->sh.BindlessSamplers[index].unit =
         shader->Program->sh.BindlessSamplers[index].bound = true;
      } else {
      if (index >= ARRAY_SIZE(shader->Program->SamplerUnits))
         shader->Program->SamplerUnits[index] =
            } else if (glsl_type_is_image(storage->type)) {
                        if (storage->is_bindless) {
      if (index >= shader->Program->sh.NumBindlessImages)
         shader->Program->sh.BindlessImages[index].unit =
         shader->Program->sh.BindlessImages[index].bound = true;
      } else {
      if (index >= ARRAY_SIZE(shader->Program->sh.ImageUnits))
         shader->Program->sh.ImageUnits[index] =
                  }
      static void
   copy_constant_to_storage(union gl_constant_value *storage,
                     {
      const enum glsl_base_type base_type = glsl_get_base_type(type);
   const unsigned n_columns = glsl_get_matrix_columns(type);
   const unsigned n_rows = glsl_get_vector_elements(type);
   unsigned dmul = glsl_base_type_is_64bit(base_type) ? 2 : 1;
            if (n_columns > 1) {
      const struct glsl_type *column_type = glsl_get_column_type(type);
   for (unsigned int column = 0; column < n_columns; column++) {
      copy_constant_to_storage(&storage[i], val->elements[column],
               } else {
      for (unsigned int row = 0; row < n_rows; row++) {
      switch (base_type) {
   case GLSL_TYPE_UINT:
      storage[i].u = val->values[row].u32;
      case GLSL_TYPE_INT:
   case GLSL_TYPE_SAMPLER:
      storage[i].i = val->values[row].i32;
      case GLSL_TYPE_FLOAT:
      storage[i].f = val->values[row].f32;
      case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
      /* XXX need to check on big-endian */
   memcpy(&storage[i].u, &val->values[row].f64, sizeof(double));
      case GLSL_TYPE_BOOL:
      storage[i].b = val->values[row].u32 ? boolean_true : 0;
      case GLSL_TYPE_ARRAY:
   case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_ATOMIC_UINT:
   case GLSL_TYPE_INTERFACE:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_SUBROUTINE:
   case GLSL_TYPE_ERROR:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_FLOAT16:
      /* All other types should have already been filtered by other
   * paths in the caller.
   */
   assert(!"Should not get here.");
      case GLSL_TYPE_COOPERATIVE_MATRIX:
         }
            }
      struct set_uniform_initializer_closure {
      struct gl_shader_program *shader_prog;
   struct gl_program *prog;
   const nir_variable *var;
   int location;
      };
      static void
   set_uniform_initializer(struct set_uniform_initializer_closure *data,
               {
               if (glsl_type_is_struct_or_ifc(type)) {
      for (unsigned int i = 0; i < glsl_get_length(type); i++) {
      const struct glsl_type *field_type = glsl_get_struct_field(type, i);
      }
               if (glsl_type_is_struct_or_ifc(t_without_array) ||
      (glsl_type_is_array(type) &&
   glsl_type_is_array(glsl_get_array_element(type)))) {
            for (unsigned int i = 0; i < glsl_get_length(type); i++)
                        if (data->location < 0 ||
      data->location >= data->prog->sh.data->NumUniformStorage)
         struct gl_uniform_storage *storage =
            if (glsl_type_is_array(type)) {
      const struct glsl_type *element_type = glsl_get_array_element(type);
   const enum glsl_base_type base_type = glsl_get_base_type(element_type);
   const unsigned int elements = glsl_get_components(element_type);
   unsigned int idx = 0;
            assert(glsl_get_length(type) >= storage->array_elements);
   for (unsigned int i = 0; i < storage->array_elements; i++) {
      copy_constant_to_storage(&storage->storage[idx],
                              } else {
      copy_constant_to_storage(storage->storage,
                        if (glsl_type_is_sampler(storage->type)) {
      for (int sh = 0; sh < MESA_SHADER_STAGES; sh++) {
                                                      }
      void
   gl_nir_set_uniform_initializers(const struct gl_constants *consts,
         {
      for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[i];
   if (!sh)
            nir_shader *nir = sh->Program->nir;
            nir_foreach_gl_uniform_variable(var, nir) {
      if (var->constant_initializer) {
      struct set_uniform_initializer_closure data = {
      .shader_prog = prog,
   .prog = sh->Program,
   .var = var,
   .location = var->data.location,
      };
   set_uniform_initializer(&data,
                        if (nir_variable_is_in_block(var)) {
                                       if (glsl_type_is_sampler(without_array) ||
      glsl_type_is_image(without_array)) {
   struct set_opaque_binding_closure data = {
      .shader_prog = prog,
   .prog = sh->Program,
   .var = var,
   .binding = var->data.binding,
      };
               }
   memcpy(prog->data->UniformDataDefaults, prog->data->UniformDataSlots,
         }
