   /*
   * Copyright © 2014 Intel Corporation
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
      /**
   * \file serialize.cpp
   *
   * GLSL serialization
   *
   * Supports serializing and deserializing glsl programs using a blob.
   */
      #include "compiler/glsl_types.h"
   #include "compiler/shader_info.h"
   #include "ir_uniform.h"
   #include "main/mtypes.h"
   #include "main/shaderobj.h"
   #include "program/program.h"
   #include "string_to_uint_map.h"
   #include "util/bitscan.h"
         static void
   write_subroutines(struct blob *metadata, struct gl_shader_program *prog)
   {
      for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[i];
   if (!sh)
                     blob_write_uint32(metadata, glprog->sh.NumSubroutineUniforms);
   blob_write_uint32(metadata, glprog->sh.MaxSubroutineFunctionIndex);
   blob_write_uint32(metadata, glprog->sh.NumSubroutineFunctions);
   for (unsigned j = 0; j < glprog->sh.NumSubroutineFunctions; j++) {
               blob_write_string(metadata, glprog->sh.SubroutineFunctions[j].name.string);
                  for (int k = 0; k < num_types; k++) {
      encode_type_to_blob(metadata,
               }
      static void
   read_subroutines(struct blob_reader *metadata, struct gl_shader_program *prog)
   {
               for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[i];
   if (!sh)
                     glprog->sh.NumSubroutineUniforms = blob_read_uint32(metadata);
   glprog->sh.MaxSubroutineFunctionIndex = blob_read_uint32(metadata);
            subs = rzalloc_array(prog, struct gl_subroutine_function,
                  for (unsigned j = 0; j < glprog->sh.NumSubroutineFunctions; j++) {
      subs[j].name.string = ralloc_strdup(prog, blob_read_string (metadata));
   resource_name_updated(&subs[j].name);
                  subs[j].types = rzalloc_array(prog, const struct glsl_type *,
         for (int k = 0; k < subs[j].num_compat_types; k++) {
                  }
      static void
   write_buffer_block(struct blob *metadata, struct gl_uniform_block *b)
   {
      blob_write_string(metadata, b->name.string);
   blob_write_uint32(metadata, b->NumUniforms);
   blob_write_uint32(metadata, b->Binding);
   blob_write_uint32(metadata, b->UniformBufferSize);
            for (unsigned j = 0; j < b->NumUniforms; j++) {
      blob_write_string(metadata, b->Uniforms[j].Name);
   blob_write_string(metadata, b->Uniforms[j].IndexName);
   encode_type_to_blob(metadata, b->Uniforms[j].Type);
         }
      static void
   write_buffer_blocks(struct blob *metadata, struct gl_shader_program *prog)
   {
      blob_write_uint32(metadata, prog->data->NumUniformBlocks);
            for (unsigned i = 0; i < prog->data->NumUniformBlocks; i++) {
                  for (unsigned i = 0; i < prog->data->NumShaderStorageBlocks; i++) {
                  for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[i];
   if (!sh)
                     blob_write_uint32(metadata, glprog->sh.NumUniformBlocks);
            for (unsigned j = 0; j < glprog->sh.NumUniformBlocks; j++) {
      uint32_t offset =
                     for (unsigned j = 0; j < glprog->info.num_ssbos; j++) {
      uint32_t offset = glprog->sh.ShaderStorageBlocks[j] -
                  }
      static void
   read_buffer_block(struct blob_reader *metadata, struct gl_uniform_block *b,
         {
         b->name.string = ralloc_strdup(prog->data, blob_read_string (metadata));
   resource_name_updated(&b->name);
   b->NumUniforms = blob_read_uint32(metadata);
   b->Binding = blob_read_uint32(metadata);
   b->UniformBufferSize = blob_read_uint32(metadata);
            b->Uniforms =
      rzalloc_array(prog->data, struct gl_uniform_buffer_variable,
      for (unsigned j = 0; j < b->NumUniforms; j++) {
                     char *index_name = blob_read_string(metadata);
   if (strcmp(b->Uniforms[j].Name, index_name) == 0) {
         } else {
                  b->Uniforms[j].Type = decode_type_from_blob(metadata);
      }
      static void
   read_buffer_blocks(struct blob_reader *metadata,
         {
      prog->data->NumUniformBlocks = blob_read_uint32(metadata);
            prog->data->UniformBlocks =
      rzalloc_array(prog->data, struct gl_uniform_block,
         prog->data->ShaderStorageBlocks =
      rzalloc_array(prog->data, struct gl_uniform_block,
         for (unsigned i = 0; i < prog->data->NumUniformBlocks; i++) {
                  for (unsigned i = 0; i < prog->data->NumShaderStorageBlocks; i++) {
                  for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[i];
   if (!sh)
                     glprog->sh.NumUniformBlocks = blob_read_uint32(metadata);
            glprog->sh.UniformBlocks =
         glprog->sh.ShaderStorageBlocks =
            for (unsigned j = 0; j < glprog->sh.NumUniformBlocks; j++) {
      uint32_t offset = blob_read_uint32(metadata);
               for (unsigned j = 0; j < glprog->info.num_ssbos; j++) {
      uint32_t offset = blob_read_uint32(metadata);
   glprog->sh.ShaderStorageBlocks[j] =
            }
      static void
   write_atomic_buffers(struct blob *metadata, struct gl_shader_program *prog)
   {
               for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i]) {
      struct gl_program *glprog = prog->_LinkedShaders[i]->Program;
                  for (unsigned i = 0; i < prog->data->NumAtomicBuffers; i++) {
      blob_write_uint32(metadata, prog->data->AtomicBuffers[i].Binding);
   blob_write_uint32(metadata, prog->data->AtomicBuffers[i].MinimumSize);
            blob_write_bytes(metadata, prog->data->AtomicBuffers[i].StageReferences,
            for (unsigned j = 0; j < prog->data->AtomicBuffers[i].NumUniforms; j++) {
               }
      static void
   read_atomic_buffers(struct blob_reader *metadata,
         {
      prog->data->NumAtomicBuffers = blob_read_uint32(metadata);
   prog->data->AtomicBuffers =
      rzalloc_array(prog, gl_active_atomic_buffer,
         struct gl_active_atomic_buffer **stage_buff_list[MESA_SHADER_STAGES];
   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i]) {
               glprog->info.num_abos = blob_read_uint32(metadata);
   glprog->sh.AtomicBuffers =
      rzalloc_array(glprog, gl_active_atomic_buffer *,
                     for (unsigned i = 0; i < prog->data->NumAtomicBuffers; i++) {
      prog->data->AtomicBuffers[i].Binding = blob_read_uint32(metadata);
   prog->data->AtomicBuffers[i].MinimumSize = blob_read_uint32(metadata);
            blob_copy_bytes(metadata,
                  prog->data->AtomicBuffers[i].Uniforms = rzalloc_array(prog, unsigned,
            for (unsigned j = 0; j < prog->data->AtomicBuffers[i].NumUniforms; j++) {
                  for (unsigned j = 0; j < MESA_SHADER_STAGES; j++) {
      if (prog->data->AtomicBuffers[i].StageReferences[j]) {
      *stage_buff_list[j] = &prog->data->AtomicBuffers[i];
               }
      static void
   write_xfb(struct blob *metadata, struct gl_shader_program *shProg)
   {
               if (!prog) {
      blob_write_uint32(metadata, ~0u);
                                 /* Data set by glTransformFeedbackVaryings. */
   blob_write_uint32(metadata, shProg->TransformFeedback.BufferMode);
   blob_write_bytes(metadata, shProg->TransformFeedback.BufferStride,
         blob_write_uint32(metadata, shProg->TransformFeedback.NumVarying);
   for (unsigned i = 0; i < shProg->TransformFeedback.NumVarying; i++)
            blob_write_uint32(metadata, ltf->NumOutputs);
   blob_write_uint32(metadata, ltf->ActiveBuffers);
            blob_write_bytes(metadata, ltf->Outputs,
                  for (int i = 0; i < ltf->NumVarying; i++) {
      blob_write_string(metadata, ltf->Varyings[i].name.string);
   blob_write_uint32(metadata, ltf->Varyings[i].Type);
   blob_write_uint32(metadata, ltf->Varyings[i].BufferIndex);
   blob_write_uint32(metadata, ltf->Varyings[i].Size);
               blob_write_bytes(metadata, ltf->Buffers,
            }
      static void
   read_xfb(struct blob_reader *metadata, struct gl_shader_program *shProg)
   {
               if (xfb_stage == ~0u)
            if (shProg->TransformFeedback.VaryingNames)  {
      for (unsigned i = 0; i < shProg->TransformFeedback.NumVarying; ++i)
               /* Data set by glTransformFeedbackVaryings. */
   shProg->TransformFeedback.BufferMode = blob_read_uint32(metadata);
   blob_copy_bytes(metadata, &shProg->TransformFeedback.BufferStride,
                  shProg->TransformFeedback.VaryingNames = (char **)
      realloc(shProg->TransformFeedback.VaryingNames,
      /* Note, malloc used with VaryingNames. */
   for (unsigned i = 0; i < shProg->TransformFeedback.NumVarying; i++)
      shProg->TransformFeedback.VaryingNames[i] =
         struct gl_program *prog = shProg->_LinkedShaders[xfb_stage]->Program;
   struct gl_transform_feedback_info *ltf =
            prog->sh.LinkedTransformFeedback = ltf;
            ltf->NumOutputs = blob_read_uint32(metadata);
   ltf->ActiveBuffers = blob_read_uint32(metadata);
            ltf->Outputs = rzalloc_array(prog, struct gl_transform_feedback_output,
            blob_copy_bytes(metadata, (uint8_t *) ltf->Outputs,
                  ltf->Varyings = rzalloc_array(prog,
                  for (int i = 0; i < ltf->NumVarying; i++) {
      ltf->Varyings[i].name.string = ralloc_strdup(prog, blob_read_string(metadata));
   resource_name_updated(&ltf->Varyings[i].name);
   ltf->Varyings[i].Type = blob_read_uint32(metadata);
   ltf->Varyings[i].BufferIndex = blob_read_uint32(metadata);
   ltf->Varyings[i].Size = blob_read_uint32(metadata);
               blob_copy_bytes(metadata, (uint8_t *) ltf->Buffers,
            }
      static bool
   has_uniform_storage(struct gl_shader_program *prog, unsigned idx)
   {
      if (!prog->data->UniformStorage[idx].builtin &&
      !prog->data->UniformStorage[idx].is_shader_storage &&
   prog->data->UniformStorage[idx].block_index == -1)
            }
      static void
   write_uniforms(struct blob *metadata, struct gl_shader_program *prog)
   {
      blob_write_uint32(metadata, prog->SamplersValidated);
   blob_write_uint32(metadata, prog->data->NumUniformStorage);
            for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
      encode_type_to_blob(metadata, prog->data->UniformStorage[i].type);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].array_elements);
   if (prog->data->UniformStorage[i].name.string) {
         } else {
         }
   blob_write_uint32(metadata, prog->data->UniformStorage[i].builtin);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].remap_location);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].block_index);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].atomic_buffer_index);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].offset);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].array_stride);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].hidden);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].is_shader_storage);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].active_shader_mask);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].matrix_stride);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].row_major);
   blob_write_uint32(metadata, prog->data->UniformStorage[i].is_bindless);
   blob_write_uint32(metadata,
         blob_write_uint32(metadata,
         blob_write_uint32(metadata,
         if (has_uniform_storage(prog, i)) {
         blob_write_uint32(metadata, prog->data->UniformStorage[i].storage -
               blob_write_bytes(metadata, prog->data->UniformStorage[i].opaque,
               /* Here we cache all uniform values. We do this to retain values for
   * uniforms with initialisers and also hidden uniforms that may be lowered
   * constant arrays. We could possibly just store the values we need but for
   * now we just store everything.
   */
   blob_write_uint32(metadata, prog->data->NumHiddenUniforms);
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
      if (has_uniform_storage(prog, i)) {
      unsigned vec_size =
      prog->data->UniformStorage[i].type->component_slots() *
      unsigned slot =
      prog->data->UniformStorage[i].storage -
      blob_write_bytes(metadata, &prog->data->UniformDataDefaults[slot],
            }
      static void
   read_uniforms(struct blob_reader *metadata, struct gl_shader_program *prog)
   {
      struct gl_uniform_storage *uniforms;
            prog->SamplersValidated = blob_read_uint32(metadata);
   prog->data->NumUniformStorage = blob_read_uint32(metadata);
            uniforms = rzalloc_array(prog->data, struct gl_uniform_storage,
                  data = rzalloc_array(uniforms, union gl_constant_value,
         prog->data->UniformDataSlots = data;
   prog->data->UniformDataDefaults =
      rzalloc_array(uniforms, union gl_constant_value,
                  for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
      uniforms[i].type = decode_type_from_blob(metadata);
   uniforms[i].array_elements = blob_read_uint32(metadata);
   uniforms[i].name.string = ralloc_strdup(prog, blob_read_string (metadata));
   resource_name_updated(&uniforms[i].name);
   uniforms[i].builtin = blob_read_uint32(metadata);
   uniforms[i].remap_location = blob_read_uint32(metadata);
   uniforms[i].block_index = blob_read_uint32(metadata);
   uniforms[i].atomic_buffer_index = blob_read_uint32(metadata);
   uniforms[i].offset = blob_read_uint32(metadata);
   uniforms[i].array_stride = blob_read_uint32(metadata);
   uniforms[i].hidden = blob_read_uint32(metadata);
   uniforms[i].is_shader_storage = blob_read_uint32(metadata);
   uniforms[i].active_shader_mask = blob_read_uint32(metadata);
   uniforms[i].matrix_stride = blob_read_uint32(metadata);
   uniforms[i].row_major = blob_read_uint32(metadata);
   uniforms[i].is_bindless = blob_read_uint32(metadata);
   uniforms[i].num_compatible_subroutines = blob_read_uint32(metadata);
   uniforms[i].top_level_array_size = blob_read_uint32(metadata);
   uniforms[i].top_level_array_stride = blob_read_uint32(metadata);
            if (has_uniform_storage(prog, i)) {
                  memcpy(uniforms[i].opaque,
                     /* Restore uniform values. */
   prog->data->NumHiddenUniforms = blob_read_uint32(metadata);
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
      if (has_uniform_storage(prog, i)) {
      unsigned vec_size =
      prog->data->UniformStorage[i].type->component_slots() *
      unsigned slot =
      prog->data->UniformStorage[i].storage -
      blob_copy_bytes(metadata,
               assert(vec_size + prog->data->UniformStorage[i].storage <=
                     memcpy(prog->data->UniformDataDefaults, prog->data->UniformDataSlots,
      }
      enum uniform_remap_type
   {
      remap_type_inactive_explicit_location,
   remap_type_null_ptr,
   remap_type_uniform_offset,
      };
      static void
   write_uniform_remap_table(struct blob *metadata,
                     {
               for (unsigned i = 0; i < num_entries; i++) {
      gl_uniform_storage *entry = remap_table[i];
            if (entry == INACTIVE_UNIFORM_EXPLICIT_LOCATION) {
         } else if (entry == NULL) {
         } else if (i+1 < num_entries && entry == remap_table[i+1]) {
               /* If many offsets are equal, write only one offset and the number
   * of consecutive entries being equal.
   */
   unsigned count = 1;
   for (unsigned j = i + 1; j < num_entries; j++) {
                                 blob_write_uint32(metadata, offset);
   blob_write_uint32(metadata, count);
      } else {
                        }
      static void
   write_uniform_remap_tables(struct blob *metadata,
         {
      write_uniform_remap_table(metadata, prog->NumUniformRemapTable,
                  for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[i];
   if (sh) {
      write_uniform_remap_table(metadata,
                        }
      static struct gl_uniform_storage **
   read_uniform_remap_table(struct blob_reader *metadata,
                     {
      unsigned num = blob_read_uint32(metadata);
            struct gl_uniform_storage **remap_table =
            for (unsigned i = 0; i < num; i++) {
      enum uniform_remap_type type =
            if (type == remap_type_inactive_explicit_location) {
         } else if (type == remap_type_null_ptr) {
         } else if (type == remap_type_uniform_offsets_equal) {
      uint32_t uni_offset = blob_read_uint32(metadata);
                  for (unsigned j = 0; j < count; j++)
            } else {
      uint32_t uni_offset = blob_read_uint32(metadata);
         }
      }
      static void
   read_uniform_remap_tables(struct blob_reader *metadata,
         {
      prog->UniformRemapTable =
      read_uniform_remap_table(metadata, prog, &prog->NumUniformRemapTable,
         for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[i];
   if (sh) {
               glprog->sh.SubroutineUniformRemapTable =
      read_uniform_remap_table(metadata, prog,
               }
      struct whte_closure
   {
      struct blob *blob;
      };
      static void
   write_hash_table_entry(const char *key, unsigned value, void *closure)
   {
               blob_write_string(whte->blob, key);
               }
      static void
   write_hash_table(struct blob *metadata, struct string_to_uint_map *hash)
   {
      size_t offset;
            whte.blob = metadata;
                     /* Write a placeholder for the hashtable size. */
                     /* Overwrite with the computed number of entries written. */
      }
      static void
   read_hash_table(struct blob_reader *metadata, struct string_to_uint_map *hash)
   {
      size_t i, num_entries;
   const char *key;
                     for (i = 0; i < num_entries; i++) {
      key = blob_read_string(metadata);
                  }
      static void
   write_hash_tables(struct blob *metadata, struct gl_shader_program *prog)
   {
      write_hash_table(metadata, prog->AttributeBindings);
   write_hash_table(metadata, prog->FragDataBindings);
      }
      static void
   read_hash_tables(struct blob_reader *metadata, struct gl_shader_program *prog)
   {
      read_hash_table(metadata, prog->AttributeBindings);
   read_hash_table(metadata, prog->FragDataBindings);
      }
      static void
   write_shader_subroutine_index(struct blob *metadata,
               {
               for (unsigned j = 0; j < sh->Program->sh.NumSubroutineFunctions; j++) {
      if (strcmp(((gl_subroutine_function *)res->Data)->name.string,
            blob_write_uint32(metadata, j);
            }
      static void
   get_shader_var_and_pointer_sizes(size_t *s_var_size, size_t *s_var_ptrs,
         {
      *s_var_size = sizeof(gl_shader_variable);
   *s_var_ptrs =
      sizeof(var->type) +
   sizeof(var->interface_type) +
   sizeof(var->outermost_struct_type) +
   }
      enum uniform_type
   {
      uniform_remapped,
      };
      static void
   write_program_resource_data(struct blob *metadata,
               {
               switch(res->Type) {
   case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT: {
               encode_type_to_blob(metadata, var->type);
   encode_type_to_blob(metadata, var->interface_type);
            if (var->name.string) {
         } else {
                  size_t s_var_size, s_var_ptrs;
            /* Write gl_shader_variable skipping over the pointers */
   blob_write_bytes(metadata, ((char *)var) + s_var_ptrs,
            }
   case GL_UNIFORM_BLOCK:
      for (unsigned i = 0; i < prog->data->NumUniformBlocks; i++) {
      if (strcmp(((gl_uniform_block *)res->Data)->name.string,
            blob_write_uint32(metadata, i);
         }
      case GL_SHADER_STORAGE_BLOCK:
      for (unsigned i = 0; i < prog->data->NumShaderStorageBlocks; i++) {
      if (strcmp(((gl_uniform_block *)res->Data)->name.string,
            blob_write_uint32(metadata, i);
         }
      case GL_BUFFER_VARIABLE:
   case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
   case GL_UNIFORM:
      if (((gl_uniform_storage *)res->Data)->builtin ||
      res->Type != GL_UNIFORM) {
   blob_write_uint32(metadata, uniform_not_remapped);
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
      if (strcmp(((gl_uniform_storage *)res->Data)->name.string,
            blob_write_uint32(metadata, i);
            } else {
      blob_write_uint32(metadata, uniform_remapped);
      }
      case GL_ATOMIC_COUNTER_BUFFER:
      for (unsigned i = 0; i < prog->data->NumAtomicBuffers; i++) {
      if (((gl_active_atomic_buffer *)res->Data)->Binding ==
      prog->data->AtomicBuffers[i].Binding) {
   blob_write_uint32(metadata, i);
         }
      case GL_TRANSFORM_FEEDBACK_BUFFER:
      for (unsigned i = 0; i < MAX_FEEDBACK_BUFFERS; i++) {
      if (((gl_transform_feedback_buffer *)res->Data)->Binding ==
      prog->last_vert_prog->sh.LinkedTransformFeedback->Buffers[i].Binding) {
   blob_write_uint32(metadata, i);
         }
      case GL_TRANSFORM_FEEDBACK_VARYING:
      for (int i = 0; i < prog->last_vert_prog->sh.LinkedTransformFeedback->NumVarying; i++) {
      if (strcmp(((gl_transform_feedback_varying_info *)res->Data)->name.string,
            blob_write_uint32(metadata, i);
         }
      case GL_VERTEX_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE:
      sh =
         write_shader_subroutine_index(metadata, sh, res);
      default:
            }
      static void
   read_program_resource_data(struct blob_reader *metadata,
               {
               switch(res->Type) {
   case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT: {
               var->type = decode_type_from_blob(metadata);
   var->interface_type = decode_type_from_blob(metadata);
            var->name.string = ralloc_strdup(prog, blob_read_string(metadata));
            size_t s_var_size, s_var_ptrs;
            blob_copy_bytes(metadata, ((uint8_t *) var) + s_var_ptrs,
            res->Data = var;
      }
   case GL_UNIFORM_BLOCK:
      res->Data = &prog->data->UniformBlocks[blob_read_uint32(metadata)];
      case GL_SHADER_STORAGE_BLOCK:
      res->Data = &prog->data->ShaderStorageBlocks[blob_read_uint32(metadata)];
      case GL_BUFFER_VARIABLE:
   case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
   case GL_UNIFORM: {
      enum uniform_type type = (enum uniform_type) blob_read_uint32(metadata);
   if (type == uniform_not_remapped) {
         } else {
         }
      }
   case GL_ATOMIC_COUNTER_BUFFER:
      res->Data = &prog->data->AtomicBuffers[blob_read_uint32(metadata)];
      case GL_TRANSFORM_FEEDBACK_BUFFER:
      res->Data = &prog->last_vert_prog->
            case GL_TRANSFORM_FEEDBACK_VARYING:
      res->Data = &prog->last_vert_prog->
            case GL_VERTEX_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE:
      sh =
         res->Data =
            default:
            }
      static void
   write_program_resource_list(struct blob *metadata,
         {
               for (unsigned i = 0; i < prog->data->NumProgramResourceList; i++) {
      blob_write_uint32(metadata, prog->data->ProgramResourceList[i].Type);
   write_program_resource_data(metadata, prog,
         blob_write_bytes(metadata,
               }
      static void
   read_program_resource_list(struct blob_reader *metadata,
         {
               prog->data->ProgramResourceList =
      ralloc_array(prog->data, gl_program_resource,
         for (unsigned i = 0; i < prog->data->NumProgramResourceList; i++) {
      prog->data->ProgramResourceList[i].Type = blob_read_uint32(metadata);
   read_program_resource_data(metadata, prog,
         blob_copy_bytes(metadata,
               }
      static void
   write_shader_parameters(struct blob *metadata,
         {
      blob_write_uint32(metadata, params->NumParameters);
            while (i < params->NumParameters) {
      struct gl_program_parameter *param = &params->Parameters[i];
   blob_write_uint32(metadata, param->Type);
   blob_write_string(metadata, param->Name);
   blob_write_uint32(metadata, param->Size);
   blob_write_uint32(metadata, param->Padded);
   blob_write_uint32(metadata, param->DataType);
   blob_write_bytes(metadata, param->StateIndexes,
         blob_write_uint32(metadata, param->UniformStorageIndex);
                        blob_write_bytes(metadata, params->ParameterValues,
            blob_write_uint32(metadata, params->StateFlags);
   blob_write_uint32(metadata, params->UniformBytes);
   blob_write_uint32(metadata, params->FirstStateVarIndex);
      }
      static void
   read_shader_parameters(struct blob_reader *metadata,
         {
      gl_state_index16 state_indexes[STATE_LENGTH];
   uint32_t i = 0;
            _mesa_reserve_parameter_storage(params, num_parameters, num_parameters);
   while (i < num_parameters) {
      gl_register_file type = (gl_register_file) blob_read_uint32(metadata);
   const char *name = blob_read_string(metadata);
   unsigned size = blob_read_uint32(metadata);
   bool padded = blob_read_uint32(metadata);
   unsigned data_type = blob_read_uint32(metadata);
   blob_copy_bytes(metadata, (uint8_t *) state_indexes,
            _mesa_add_parameter(params, type, name, size, data_type,
            gl_program_parameter *param = &params->Parameters[i];
   param->UniformStorageIndex = blob_read_uint32(metadata);
                        blob_copy_bytes(metadata, (uint8_t *) params->ParameterValues,
            params->StateFlags = blob_read_uint32(metadata);
   params->UniformBytes = blob_read_uint32(metadata);
   params->FirstStateVarIndex = blob_read_uint32(metadata);
      }
      static void
   write_shader_metadata(struct blob *metadata, gl_linked_shader *shader)
   {
      assert(shader->Program);
   struct gl_program *glprog = shader->Program;
            blob_write_uint64(metadata, glprog->DualSlotInputs);
   blob_write_bytes(metadata, glprog->TexturesUsed,
                  blob_write_bytes(metadata, glprog->SamplerUnits,
         blob_write_bytes(metadata, glprog->sh.SamplerTargets,
         blob_write_uint32(metadata, glprog->ShadowSamplers);
   blob_write_uint32(metadata, glprog->ExternalSamplersUsed);
            blob_write_bytes(metadata, glprog->sh.image_access,
         blob_write_bytes(metadata, glprog->sh.ImageUnits,
                     blob_write_uint32(metadata, glprog->sh.NumBindlessSamplers);
   blob_write_uint32(metadata, glprog->sh.HasBoundBindlessSampler);
   for (i = 0; i < glprog->sh.NumBindlessSamplers; i++) {
      blob_write_bytes(metadata, &glprog->sh.BindlessSamplers[i],
               blob_write_uint32(metadata, glprog->sh.NumBindlessImages);
   blob_write_uint32(metadata, glprog->sh.HasBoundBindlessImage);
   for (i = 0; i < glprog->sh.NumBindlessImages; i++) {
      blob_write_bytes(metadata, &glprog->sh.BindlessImages[i],
                        assert((glprog->driver_cache_blob == NULL) ==
         blob_write_uint32(metadata, (uint32_t)glprog->driver_cache_blob_size);
   if (glprog->driver_cache_blob_size > 0) {
      blob_write_bytes(metadata, glprog->driver_cache_blob,
         }
      static void
   read_shader_metadata(struct blob_reader *metadata,
               {
               glprog->DualSlotInputs = blob_read_uint64(metadata);
   blob_copy_bytes(metadata, (uint8_t *) glprog->TexturesUsed,
                  blob_copy_bytes(metadata, (uint8_t *) glprog->SamplerUnits,
         blob_copy_bytes(metadata, (uint8_t *) glprog->sh.SamplerTargets,
         glprog->ShadowSamplers = blob_read_uint32(metadata);
   glprog->ExternalSamplersUsed = blob_read_uint32(metadata);
            blob_copy_bytes(metadata, (uint8_t *) glprog->sh.image_access,
         blob_copy_bytes(metadata, (uint8_t *) glprog->sh.ImageUnits,
                     glprog->sh.NumBindlessSamplers = blob_read_uint32(metadata);
   glprog->sh.HasBoundBindlessSampler = blob_read_uint32(metadata);
   if (glprog->sh.NumBindlessSamplers > 0) {
      glprog->sh.BindlessSamplers =
                  for (i = 0; i < glprog->sh.NumBindlessSamplers; i++) {
      blob_copy_bytes(metadata, (uint8_t *) &glprog->sh.BindlessSamplers[i],
                  glprog->sh.NumBindlessImages = blob_read_uint32(metadata);
   glprog->sh.HasBoundBindlessImage = blob_read_uint32(metadata);
   if (glprog->sh.NumBindlessImages > 0) {
      glprog->sh.BindlessImages =
                  for (i = 0; i < glprog->sh.NumBindlessImages; i++) {
      blob_copy_bytes(metadata, (uint8_t *) &glprog->sh.BindlessImages[i],
                  glprog->Parameters = _mesa_new_parameter_list();
            glprog->driver_cache_blob_size = (size_t)blob_read_uint32(metadata);
   if (glprog->driver_cache_blob_size > 0) {
      glprog->driver_cache_blob =
         blob_copy_bytes(metadata, glprog->driver_cache_blob,
         }
      static void
   get_shader_info_and_pointer_sizes(size_t *s_info_size, size_t *s_info_ptrs,
         {
      *s_info_size = sizeof(shader_info);
      }
      static void
   create_linked_shader_and_program(struct gl_context *ctx,
                     {
               struct gl_linked_shader *linked = rzalloc(NULL, struct gl_linked_shader);
            glprog = ctx->Driver.NewProgram(ctx, stage, prog->Name, false);
   glprog->info.stage = stage;
                     glprog->info.name = ralloc_strdup(glprog, blob_read_string(metadata));
            size_t s_info_size, s_info_ptrs;
   get_shader_info_and_pointer_sizes(&s_info_size, &s_info_ptrs,
            /* Restore shader info */
   blob_copy_bytes(metadata, ((uint8_t *) &glprog->info) + s_info_ptrs,
            _mesa_reference_shader_program_data(&glprog->sh.data, prog->data);
   _mesa_reference_program(ctx, &linked->Program, glprog);
      }
      extern "C" void
   serialize_glsl_program(struct blob *blob, struct gl_context *ctx,
         {
                                 blob_write_uint32(blob, prog->GLSL_Version);
   blob_write_uint32(blob, prog->IsES);
            for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[i];
   if (sh) {
               if (sh->Program->info.name)
                        if (sh->Program->info.label)
                        size_t s_info_size, s_info_ptrs;
                  /* Store shader info */
   blob_write_bytes(blob,
                                                                        }
      extern "C" bool
   deserialize_glsl_program(struct blob_reader *blob, struct gl_context *ctx,
         {
      /* Fixed function programs generated by Mesa can't be serialized. */
   if (prog->Name == 0)
                                                prog->GLSL_Version = blob_read_uint32(blob);
   prog->IsES = blob_read_uint32(blob);
            unsigned mask = prog->data->linked_stages;
   while (mask) {
      const int j = u_bit_scan(&mask);
   create_linked_shader_and_program(ctx, (gl_shader_stage) j, prog,
                                                                        }
