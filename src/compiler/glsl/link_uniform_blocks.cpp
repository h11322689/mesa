   /*
   * Copyright © 2012 Intel Corporation
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
      #include "ir.h"
   #include "linker.h"
   #include "ir_uniform.h"
   #include "link_uniform_block_active_visitor.h"
   #include "util/hash_table.h"
   #include "util/u_math.h"
   #include "program.h"
   #include "main/errors.h"
   #include "main/shader_types.h"
   #include "main/consts_exts.h"
      namespace {
      class ubo_visitor : public program_resource_visitor {
   public:
      ubo_visitor(void *mem_ctx, gl_uniform_buffer_variable *variables,
                  : index(0), offset(0), buffer_size(0), variables(variables),
   num_variables(num_variables), mem_ctx(mem_ctx),
   is_array_instance(false), prog(prog),
      {
                  void process(const glsl_type *type, const char *name)
   {
      this->offset = 0;
   this->buffer_size = 0;
   this->is_array_instance = strchr(name, ']') != NULL;
   this->program_resource_visitor::process(type, name,
               unsigned index;
   unsigned offset;
   unsigned buffer_size;
   gl_uniform_buffer_variable *variables;
   unsigned num_variables;
   void *mem_ctx;
   bool is_array_instance;
         private:
      virtual void enter_record(const glsl_type *type, const char *,
               {
      assert(type->is_struct());
   if (packing == GLSL_INTERFACE_PACKING_STD430)
      this->offset = align(
      else
      this->offset = align(
            virtual void leave_record(const glsl_type *type, const char *,
               {
               /* If this is the last field of a structure, apply rule #9.  The
   * ARB_uniform_buffer_object spec says:
   *
   *    The structure may have padding at the end; the base offset of the
   *    member following the sub-structure is rounded up to the next
   *    multiple of the base alignment of the structure.
   */
   if (packing == GLSL_INTERFACE_PACKING_STD430)
      this->offset = align(
      else
      this->offset = align(
            virtual void set_buffer_offset(unsigned offset)
   {
                  virtual void visit_field(const glsl_type *type, const char *name,
                     {
                        v->Name = ralloc_strdup(mem_ctx, name);
   v->Type = type;
            if (this->is_array_instance) {
                                             /* Length of the tail without the ']' but with the NUL.
                     } else {
                  unsigned alignment = 0;
            /* The ARB_program_interface_query spec says:
   *
   *    If the final member of an active shader storage block is array
   *    with no declared size, the minimum buffer size is computed
   *    assuming the array was declared as an array with one element.
   *
   * For that reason, we use the base type of the unsized array to
   * calculate its size. We don't need to check if the unsized array is
   * the last member of a shader storage block (that check was already
   * done by the parser).
   */
   const glsl_type *type_for_size = type;
   if (type->is_unsized_array()) {
      if (!last_field) {
      linker_error(prog, "unsized array `%s' definition: "
                                       if (packing == GLSL_INTERFACE_PACKING_STD430) {
      alignment = type->std430_base_alignment(v->RowMajor);
      } else {
      alignment = type->std140_base_alignment(v->RowMajor);
               this->offset = align(this->offset, alignment);
                     /* The ARB_uniform_buffer_object spec says:
   *
   *    For uniform blocks laid out according to [std140] rules, the
   *    minimum buffer object size returned by the UNIFORM_BLOCK_DATA_SIZE
   *    query is derived by taking the offset of the last basic machine
   *    unit consumed by the last uniform of the uniform block (including
   *    any end-of-array or end-of-structure padding), adding one, and
   *    rounding up to the next multiple of the base alignment required
   *    for a vec4.
   */
                  };
      class count_block_size : public program_resource_visitor {
   public:
      count_block_size() : num_active_uniforms(0)
   {
                        private:
      virtual void visit_field(const glsl_type * /* type */,
                           const char * /* name */,
   {
            };
      } /* anonymous namespace */
      struct block {
      const glsl_type *type;
      };
      static void process_block_array_leaf(const char *name, gl_uniform_block *blocks,
                                       ubo_visitor *parcel,
   gl_uniform_buffer_variable *variables,
      /**
   *
   * \param first_index Value of \c block_index for the first element of the
   *                    array.
   */
   static void
   process_block_array(struct uniform_block_array_elements *ub_array, char **name,
                     size_t name_length, gl_uniform_block *blocks,
   ubo_visitor *parcel, gl_uniform_buffer_variable *variables,
   const struct link_uniform_block_active *const b,
   unsigned *block_index, unsigned binding_offset,
   {
      for (unsigned j = 0; j < ub_array->num_array_elements; j++) {
               unsigned int element_idx = ub_array->array_elements[j];
   /* Append the subscript to the current variable name */
            if (ub_array->array) {
      unsigned binding_stride = binding_offset + (element_idx *
         process_block_array(ub_array->array, name, new_length, blocks,
            } else {
      process_block_array_leaf(*name, blocks,
                        }
      static void
   process_block_array_leaf(const char *name,
                           gl_uniform_block *blocks,
   ubo_visitor *parcel, gl_uniform_buffer_variable *variables,
   const struct link_uniform_block_active *const b,
   {
      unsigned i = *block_index;
            blocks[i].name.string = ralloc_strdup(blocks, name);
   resource_name_updated(&blocks[i].name);
            /* The ARB_shading_language_420pack spec says:
   *
   *    If the binding identifier is used with a uniform block instanced as
   *    an array then the first element of the array takes the specified
   *    block binding and each subsequent element takes the next consecutive
   *    uniform block binding point.
   */
            blocks[i].UniformBufferSize = 0;
   blocks[i]._Packing = glsl_interface_packing(type->interface_packing);
   blocks[i]._RowMajor = type->get_interface_row_major();
                              /* Check SSBO size is lower than maximum supported size for SSBO */
   if (b->is_shader_storage &&
      parcel->buffer_size > consts->MaxShaderStorageBlockSize) {
   linker_error(prog, "shader storage block `%s' has size %d, "
               "which is larger than the maximum allowed (%d)",
      }
   blocks[i].NumUniforms =
               }
      /* This function resizes the array types of the block so that later we can use
   * this new size to correctly calculate the offest for indirect indexing.
   */
   static const glsl_type *
   resize_block_array(const glsl_type *type,
         {
      if (type->is_array()) {
      struct uniform_block_array_elements *child_array =
         const glsl_type *new_child_type =
            const glsl_type *new_type =
      glsl_type::get_array_instance(new_child_type,
      ub_array->ir->array->type = new_type;
      } else {
            }
      static void
   create_buffer_blocks(void *mem_ctx, const struct gl_constants *consts,
                           {
      if (num_blocks == 0) {
      assert(num_variables == 0);
                        /* Allocate storage to hold all of the information related to uniform
   * blocks that can be queried through the API.
   */
   struct gl_uniform_block *blocks =
         gl_uniform_buffer_variable *variables =
            /* Add each variable from each uniform block to the API tracking
   * structures.
   */
   ubo_visitor parcel(blocks, variables, num_variables, prog,
            unsigned i = 0;
   hash_table_foreach (block_hash, entry) {
      const struct link_uniform_block_active *const b =
                  if ((create_ubo_blocks && !b->is_shader_storage) ||
               if (b->array != NULL) {
      char *name = ralloc_strdup(NULL,
                  assert(b->has_instance_name);
   process_block_array(b->array, &name, name_length, blocks, &parcel,
                  } else {
      process_block_array_leaf(glsl_get_type_name(block_type), blocks, &parcel,
                                       }
      void
   link_uniform_blocks(void *mem_ctx,
                     const struct gl_constants *consts,
   struct gl_shader_program *prog,
   struct gl_linked_shader *shader,
   struct gl_uniform_block **ubo_blocks,
   {
      /* This hash table will track all of the uniform blocks that have been
   * encountered.  Since blocks with the same block-name must be the same,
   * the hash is organized by block-name.
   */
   struct hash_table *block_hash =
      _mesa_hash_table_create(mem_ctx, _mesa_hash_string,
         if (block_hash == NULL) {
      _mesa_error_no_memory(__func__);
   linker_error(prog, "out of memory\n");
               /* Determine which uniform blocks are active. */
   link_uniform_block_active_visitor v(mem_ctx, block_hash, prog);
            /* Count the number of active uniform blocks.  Count the total number of
   * active slots in those uniform blocks.
   */
   unsigned num_ubo_variables = 0;
   unsigned num_ssbo_variables = 0;
            hash_table_foreach (block_hash, entry) {
      struct link_uniform_block_active *const b =
                     if (b->array != NULL &&
      (b->type->without_array()->interface_packing ==
   GLSL_INTERFACE_PACKING_PACKED)) {
   b->type = resize_block_array(b->type, b->array);
   b->var->type = b->type;
               block_size.num_active_uniforms = 0;
   block_size.process(b->type->without_array(), "",
            if (b->array != NULL) {
      unsigned aoa_size = b->type->arrays_of_arrays_size();
   if (b->is_shader_storage) {
      *num_ssbo_blocks += aoa_size;
      } else {
      *num_ubo_blocks += aoa_size;
         } else {
      if (b->is_shader_storage) {
      (*num_ssbo_blocks)++;
      } else {
      (*num_ubo_blocks)++;
                        create_buffer_blocks(mem_ctx, consts, prog, ubo_blocks, *num_ubo_blocks,
         create_buffer_blocks(mem_ctx, consts, prog, ssbo_blocks, *num_ssbo_blocks,
               }
      static bool
   link_uniform_blocks_are_compatible(const gl_uniform_block *a,
         {
               /* Page 35 (page 42 of the PDF) in section 4.3.7 of the GLSL 1.50 spec says:
   *
   *    Matched block names within an interface (as defined above) must match
   *    in terms of having the same number of declarations with the same
   *    sequence of types and the same sequence of member names, as well as
   *    having the same member-wise layout qualification....if a matching
   *    block is declared as an array, then the array sizes must also
   *    match... Any mismatch will generate a link error.
   *
   * Arrays are not yet supported, so there is no check for that.
   */
   if (a->NumUniforms != b->NumUniforms)
            if (a->_Packing != b->_Packing)
            if (a->_RowMajor != b->_RowMajor)
            if (a->Binding != b->Binding)
            for (unsigned i = 0; i < a->NumUniforms; i++) {
      if (strcmp(a->Uniforms[i].Name, b->Uniforms[i].Name) != 0)
            if (a->Uniforms[i].Type != b->Uniforms[i].Type)
            if (a->Uniforms[i].RowMajor != b->Uniforms[i].RowMajor)
                  }
      /**
   * Merges a uniform block into an array of uniform blocks that may or
   * may not already contain a copy of it.
   *
   * Returns the index of the new block in the array.
   */
   int
   link_cross_validate_uniform_block(void *mem_ctx,
                     {
      for (unsigned int i = 0; i < *num_linked_blocks; i++) {
               if (strcmp(old_block->name.string, new_block->name.string) == 0)
      return link_uniform_blocks_are_compatible(old_block, new_block)
            *linked_blocks = reralloc(mem_ctx, *linked_blocks,
               int linked_block_index = (*num_linked_blocks)++;
            memcpy(linked_block, new_block, sizeof(*new_block));
   linked_block->Uniforms = ralloc_array(*linked_blocks,
                  memcpy(linked_block->Uniforms,
                  linked_block->name.string = ralloc_strdup(*linked_blocks, linked_block->name.string);
            for (unsigned int i = 0; i < linked_block->NumUniforms; i++) {
      struct gl_uniform_buffer_variable *ubo_var =
            if (ubo_var->Name == ubo_var->IndexName) {
      ubo_var->Name = ralloc_strdup(*linked_blocks, ubo_var->Name);
      } else {
      ubo_var->Name = ralloc_strdup(*linked_blocks, ubo_var->Name);
                     }
