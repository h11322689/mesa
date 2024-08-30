   /*
   * Copyright © 2009 Intel Corporation
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
      #include <stdio.h>
   #include <string.h>
   #include "glsl_types.h"
   #include "util/compiler.h"
   #include "util/glheader.h"
   #include "util/hash_table.h"
   #include "util/macros.h"
   #include "util/ralloc.h"
   #include "util/u_math.h"
   #include "util/u_string.h"
   #include "util/simple_mtx.h"
      static simple_mtx_t glsl_type_cache_mutex = SIMPLE_MTX_INITIALIZER;
      static struct {
               /* Use a linear (arena) allocator for all the new types, since
   * they are not meant to be deallocated individually.
   */
            /* There might be multiple users for types (e.g. application using OpenGL
   * and Vulkan simultaneously or app using multiple Vulkan instances). Counter
   * is used to make sure we don't release the types if a user is still present.
   */
            struct hash_table *explicit_matrix_types;
   struct hash_table *array_types;
   struct hash_table *cmat_types;
   struct hash_table *struct_types;
   struct hash_table *interface_types;
      } glsl_type_cache;
      static const struct glsl_type *
   make_vector_matrix_type(linear_ctx *lin_ctx, uint32_t gl_type,
                           {
      assert(lin_ctx != NULL);
   assert(name != NULL);
            /* Neither dimension is zero or both dimensions are zero. */
            struct glsl_type *t = linear_zalloc(lin_ctx, struct glsl_type);
   t->gl_type = gl_type;
   t->base_type = base_type;
   t->sampled_type = GLSL_TYPE_VOID;
   t->interface_row_major = row_major;
   t->vector_elements = vector_elements;
   t->matrix_columns = matrix_columns;
   t->explicit_stride = explicit_stride;
   t->explicit_alignment = explicit_alignment;
               }
      static void
   fill_struct_type(struct glsl_type *t, const struct glsl_struct_field *fields, unsigned num_fields,
         {
      assert(util_is_power_of_two_or_zero(explicit_alignment));
   t->base_type = GLSL_TYPE_STRUCT;
   t->sampled_type = GLSL_TYPE_VOID;
   t->packed = packed;
   t->length = num_fields;
   t->name_id = (uintptr_t)name;
   t->explicit_alignment = explicit_alignment;
      }
      static const struct glsl_type *
   make_struct_type(linear_ctx *lin_ctx, const struct glsl_struct_field *fields, unsigned num_fields,
               {
      assert(lin_ctx != NULL);
            struct glsl_type *t = linear_zalloc(lin_ctx, struct glsl_type);
            struct glsl_struct_field *copied_fields =
            for (unsigned i = 0; i < num_fields; i++) {
      copied_fields[i] = fields[i];
                           }
      static void
   fill_interface_type(struct glsl_type *t, const struct glsl_struct_field *fields, unsigned num_fields,
               {
      t->base_type = GLSL_TYPE_INTERFACE;
   t->sampled_type = GLSL_TYPE_VOID;
   t->interface_packing = (unsigned)packing;
   t->interface_row_major = (unsigned)row_major;
   t->length = num_fields;
   t->name_id = (uintptr_t)name;
      }
      static const struct glsl_type *
   make_interface_type(linear_ctx *lin_ctx, const struct glsl_struct_field *fields, unsigned num_fields,
               {
      assert(lin_ctx != NULL);
            struct glsl_type *t = linear_zalloc(lin_ctx, struct glsl_type);
            struct glsl_struct_field *copied_fields =
            for (unsigned i = 0; i < num_fields; i++) {
      copied_fields[i] = fields[i];
                           }
      static const struct glsl_type *
   make_subroutine_type(linear_ctx *lin_ctx, const char *subroutine_name)
   {
      assert(lin_ctx != NULL);
            struct glsl_type *t = linear_zalloc(lin_ctx, struct glsl_type);
   t->base_type = GLSL_TYPE_SUBROUTINE;
   t->sampled_type = GLSL_TYPE_VOID;
   t->vector_elements = 1;
   t->matrix_columns = 1;
               }
      bool
   glsl_contains_sampler(const struct glsl_type *t)
   {
      if (glsl_type_is_array(t)) {
         } else if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      for (unsigned int i = 0; i < t->length; i++) {
      if (glsl_contains_sampler(t->fields.structure[i].type))
      }
      } else {
            }
      bool
   glsl_contains_array(const struct glsl_type *t)
   {
      if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      for (unsigned int i = 0; i < t->length; i++) {
      if (glsl_contains_array(t->fields.structure[i].type))
      }
      } else {
            }
      bool
   glsl_contains_integer(const struct glsl_type *t)
   {
      if (glsl_type_is_array(t)) {
         } else if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      for (unsigned int i = 0; i < t->length; i++) {
      if (glsl_contains_integer(t->fields.structure[i].type))
      }
      } else {
            }
      bool
   glsl_contains_double(const struct glsl_type *t)
   {
      if (glsl_type_is_array(t)) {
         } else if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      for (unsigned int i = 0; i < t->length; i++) {
      if (glsl_contains_double(t->fields.structure[i].type))
      }
      } else {
            }
      bool
   glsl_type_contains_64bit(const struct glsl_type *t)
   {
      if (glsl_type_is_array(t)) {
         } else if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      for (unsigned int i = 0; i < t->length; i++) {
      if (glsl_type_contains_64bit(t->fields.structure[i].type))
      }
      } else {
            }
      bool
   glsl_contains_opaque(const struct glsl_type *t)
   {
      switch (t->base_type) {
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_ATOMIC_UINT:
         case GLSL_TYPE_ARRAY:
         case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE:
      for (unsigned int i = 0; i < t->length; i++) {
      if (glsl_contains_opaque(t->fields.structure[i].type))
      }
      default:
            }
      bool
   glsl_contains_subroutine(const struct glsl_type *t)
   {
      if (glsl_type_is_array(t)) {
         } else if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      for (unsigned int i = 0; i < t->length; i++) {
      if (glsl_contains_subroutine(t->fields.structure[i].type))
      }
      } else {
            }
      bool
   glsl_type_contains_image(const struct glsl_type *t)
   {
      if (glsl_type_is_array(t)) {
         } else if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      for (unsigned int i = 0; i < t->length; i++) {
      if (glsl_type_contains_image(t->fields.structure[i].type))
      }
      } else {
            }
      const struct glsl_type *
   glsl_get_base_glsl_type(const struct glsl_type *t)
   {
      switch (t->base_type) {
   case GLSL_TYPE_UINT:
         case GLSL_TYPE_UINT16:
         case GLSL_TYPE_UINT8:
         case GLSL_TYPE_INT:
         case GLSL_TYPE_INT16:
         case GLSL_TYPE_INT8:
         case GLSL_TYPE_FLOAT:
         case GLSL_TYPE_FLOAT16:
         case GLSL_TYPE_DOUBLE:
         case GLSL_TYPE_BOOL:
         case GLSL_TYPE_UINT64:
         case GLSL_TYPE_INT64:
         default:
            }
      const struct glsl_type *
   glsl_get_scalar_type(const struct glsl_type *t)
   {
               /* Handle arrays */
   while (type->base_type == GLSL_TYPE_ARRAY)
            const struct glsl_type *scalar_type = glsl_get_base_glsl_type(type);
   if (scalar_type == &glsl_type_builtin_error)
               }
         const struct glsl_type *
   glsl_get_bare_type(const struct glsl_type *t)
   {
      switch (t->base_type) {
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
      return glsl_simple_type(t->base_type, t->vector_elements,
         case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE: {
      struct glsl_struct_field *bare_fields = (struct glsl_struct_field *)
         for (unsigned i = 0; i < t->length; i++) {
      bare_fields[i].type = glsl_get_bare_type(t->fields.structure[i].type);
      }
   const struct glsl_type *bare_type =
         free(bare_fields);
               case GLSL_TYPE_ARRAY:
      return glsl_array_type(glsl_get_bare_type(t->fields.array), t->length,
         case GLSL_TYPE_COOPERATIVE_MATRIX:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_ATOMIC_UINT:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_SUBROUTINE:
   case GLSL_TYPE_ERROR:
                     }
      const struct glsl_type *
   glsl_float16_type(const struct glsl_type *t)
   {
               return glsl_simple_explicit_type(GLSL_TYPE_FLOAT16, t->vector_elements,
            }
      const struct glsl_type *
   glsl_int16_type(const struct glsl_type *t)
   {
               return glsl_simple_explicit_type(GLSL_TYPE_INT16, t->vector_elements,
            }
      const struct glsl_type *
   glsl_uint16_type(const struct glsl_type *t)
   {
               return glsl_simple_explicit_type(GLSL_TYPE_UINT16, t->vector_elements,
            }
      void
   glsl_type_singleton_init_or_ref()
   {
      /* Values of these types must fit in the two bits of
   * glsl_type::sampled_type.
   */
   STATIC_ASSERT((((unsigned)GLSL_TYPE_UINT)  & 3) == (unsigned)GLSL_TYPE_UINT);
   STATIC_ASSERT((((unsigned)GLSL_TYPE_INT)   & 3) == (unsigned)GLSL_TYPE_INT);
            ASSERT_BITFIELD_SIZE(struct glsl_type, base_type, GLSL_TYPE_ERROR);
   ASSERT_BITFIELD_SIZE(struct glsl_type, sampled_type, GLSL_TYPE_ERROR);
   ASSERT_BITFIELD_SIZE(struct glsl_type, sampler_dimensionality,
            simple_mtx_lock(&glsl_type_cache_mutex);
   if (glsl_type_cache.users == 0) {
      glsl_type_cache.mem_ctx = ralloc_context(NULL);
      }
   glsl_type_cache.users++;
      }
      void
   glsl_type_singleton_decref()
   {
      simple_mtx_lock(&glsl_type_cache_mutex);
            /* Do not release glsl_types if they are still used. */
   if (--glsl_type_cache.users) {
      simple_mtx_unlock(&glsl_type_cache_mutex);
               ralloc_free(glsl_type_cache.mem_ctx);
               }
      static const struct glsl_type *
   make_array_type(linear_ctx *lin_ctx, const struct glsl_type *element_type, unsigned length,
         {
               struct glsl_type *t = linear_zalloc(lin_ctx, struct glsl_type);
   t->base_type = GLSL_TYPE_ARRAY;
   t->sampled_type = GLSL_TYPE_VOID;
   t->length = length;
   t->explicit_stride = explicit_stride;
   t->explicit_alignment = element_type->explicit_alignment;
            /* Inherit the gl type of the base. The GL type is used for
   * uniform/statevar handling in Mesa and the arrayness of the type
   * is represented by the size rather than the type.
   */
            const char *element_name = glsl_get_type_name(element_type);
   char *n;
   if (length == 0)
         else
            /* Flip the dimensions for a multidimensional array.  The type of
   * an array of 4 elements of type int[...] is written as int[4][...].
   */
   const char *pos = strchr(element_name, '[');
   if (pos) {
      char *base = n + (pos - element_name);
   const unsigned element_part = strlen(pos);
            /* Move the outer array dimension to the front. */
            /* Rewrite the element array dimensions from the element name string. */
                           }
      static const char *
   glsl_cmat_use_to_string(enum glsl_cmat_use use)
   {
      switch (use) {
   case GLSL_CMAT_USE_NONE:        return "NONE";
   case GLSL_CMAT_USE_A:           return "A";
   case GLSL_CMAT_USE_B:           return "B";
   case GLSL_CMAT_USE_ACCUMULATOR: return "ACCUMULATOR";
   default:
            };
      static const struct glsl_type *
   vec(unsigned components, const struct glsl_type *const ts[])
   {
               if (components == 8)
         else if (components == 16)
            if (n == 0 || n > 7)
               }
      #define VECN(components, sname, vname)           \
   const struct glsl_type *              \
   glsl_ ## vname ## _type (unsigned components)    \
   {                                                \
      static const struct glsl_type *const ts[] = { \
      &glsl_type_builtin_ ## sname,              \
   &glsl_type_builtin_ ## vname ## 2,         \
   &glsl_type_builtin_ ## vname ## 3,         \
   &glsl_type_builtin_ ## vname ## 4,         \
   &glsl_type_builtin_ ## vname ## 5,         \
   &glsl_type_builtin_ ## vname ## 8,         \
      };                                            \
      }
      VECN(components, float, vec)
   VECN(components, float16_t, f16vec)
   VECN(components, double, dvec)
   VECN(components, int, ivec)
   VECN(components, uint, uvec)
   VECN(components, bool, bvec)
   VECN(components, int64_t, i64vec)
   VECN(components, uint64_t, u64vec)
   VECN(components, int16_t, i16vec)
   VECN(components, uint16_t, u16vec)
   VECN(components, int8_t, i8vec)
   VECN(components, uint8_t, u8vec)
      static const struct glsl_type *
   get_explicit_matrix_instance(unsigned int base_type, unsigned int rows, unsigned int columns,
            const struct glsl_type *
   glsl_simple_explicit_type(unsigned base_type, unsigned rows, unsigned columns,
               {
      if (base_type == GLSL_TYPE_VOID) {
      assert(explicit_stride == 0 && explicit_alignment == 0 && !row_major);
               /* Matrix and vector types with explicit strides or alignment have to be
   * looked up in a table so they're handled separately.
   */
   if (explicit_stride > 0 || explicit_alignment > 0) {
      return get_explicit_matrix_instance(base_type, rows, columns,
                              /* Treat GLSL vectors as Nx1 matrices.
   */
   if (columns == 1) {
      switch (base_type) {
   case GLSL_TYPE_UINT:
         case GLSL_TYPE_INT:
         case GLSL_TYPE_FLOAT:
         case GLSL_TYPE_FLOAT16:
         case GLSL_TYPE_DOUBLE:
         case GLSL_TYPE_BOOL:
         case GLSL_TYPE_UINT64:
         case GLSL_TYPE_INT64:
         case GLSL_TYPE_UINT16:
         case GLSL_TYPE_INT16:
         case GLSL_TYPE_UINT8:
         case GLSL_TYPE_INT8:
         default:
            } else {
      if ((base_type != GLSL_TYPE_FLOAT &&
      base_type != GLSL_TYPE_DOUBLE &&
               /* GLSL matrix types are named mat{COLUMNS}x{ROWS}.  Only the following
   * combinations are valid:
   *
   *   1 2 3 4
   * 1
   * 2   x x x
   * 3   x x x
   * 4   x x x
   #define IDX(c,r) (((c-1)*3) + (r-1))
            switch (base_type) {
   case GLSL_TYPE_DOUBLE: {
      switch (IDX(columns, rows)) {
   case IDX(2,2): return &glsl_type_builtin_dmat2;
   case IDX(2,3): return &glsl_type_builtin_dmat2x3;
   case IDX(2,4): return &glsl_type_builtin_dmat2x4;
   case IDX(3,2): return &glsl_type_builtin_dmat3x2;
   case IDX(3,3): return &glsl_type_builtin_dmat3;
   case IDX(3,4): return &glsl_type_builtin_dmat3x4;
   case IDX(4,2): return &glsl_type_builtin_dmat4x2;
   case IDX(4,3): return &glsl_type_builtin_dmat4x3;
   case IDX(4,4): return &glsl_type_builtin_dmat4;
   default: return &glsl_type_builtin_error;
      }
   case GLSL_TYPE_FLOAT: {
      switch (IDX(columns, rows)) {
   case IDX(2,2): return &glsl_type_builtin_mat2;
   case IDX(2,3): return &glsl_type_builtin_mat2x3;
   case IDX(2,4): return &glsl_type_builtin_mat2x4;
   case IDX(3,2): return &glsl_type_builtin_mat3x2;
   case IDX(3,3): return &glsl_type_builtin_mat3;
   case IDX(3,4): return &glsl_type_builtin_mat3x4;
   case IDX(4,2): return &glsl_type_builtin_mat4x2;
   case IDX(4,3): return &glsl_type_builtin_mat4x3;
   case IDX(4,4): return &glsl_type_builtin_mat4;
   default: return &glsl_type_builtin_error;
      }
   case GLSL_TYPE_FLOAT16: {
      switch (IDX(columns, rows)) {
   case IDX(2,2): return &glsl_type_builtin_f16mat2;
   case IDX(2,3): return &glsl_type_builtin_f16mat2x3;
   case IDX(2,4): return &glsl_type_builtin_f16mat2x4;
   case IDX(3,2): return &glsl_type_builtin_f16mat3x2;
   case IDX(3,3): return &glsl_type_builtin_f16mat3;
   case IDX(3,4): return &glsl_type_builtin_f16mat3x4;
   case IDX(4,2): return &glsl_type_builtin_f16mat4x2;
   case IDX(4,3): return &glsl_type_builtin_f16mat4x3;
   case IDX(4,4): return &glsl_type_builtin_f16mat4;
   default: return &glsl_type_builtin_error;
      }
   default: return &glsl_type_builtin_error;
               assert(!"Should not get here.");
      }
      struct PACKED explicit_matrix_key {
      /* Rows and Columns are implied in the bare type. */
   uintptr_t bare_type;
   uintptr_t explicit_stride;
   uintptr_t explicit_alignment;
      };
      static uint32_t
   hash_explicit_matrix_key(const void *a)
   {
         }
      static bool
   compare_explicit_matrix_key(const void *a, const void *b)
   {
         }
      static const struct glsl_type *
   get_explicit_matrix_instance(unsigned int base_type, unsigned int rows, unsigned int columns,
         {
      assert(explicit_stride > 0 || explicit_alignment > 0);
            if (explicit_alignment > 0) {
      assert(util_is_power_of_two_nonzero(explicit_alignment));
                                 /* Ensure there's no internal padding, to avoid multiple hashes for same key. */
            struct explicit_matrix_key key = { 0 };
   key.bare_type = (uintptr_t) bare_type;
   key.explicit_stride = explicit_stride;
   key.explicit_alignment = explicit_alignment;
                     simple_mtx_lock(&glsl_type_cache_mutex);
   assert(glsl_type_cache.users > 0);
            if (glsl_type_cache.explicit_matrix_types == NULL) {
      glsl_type_cache.explicit_matrix_types =
      }
            const struct hash_entry *entry =
                     char name[128];
   snprintf(name, sizeof(name), "%sx%ua%uB%s", glsl_get_type_name(bare_type),
            linear_ctx *lin_ctx = glsl_type_cache.lin_ctx;
   const struct glsl_type *t =
      make_vector_matrix_type(lin_ctx, bare_type->gl_type,
                           struct explicit_matrix_key *stored_key = linear_zalloc(lin_ctx, struct explicit_matrix_key);
            entry = _mesa_hash_table_insert_pre_hashed(explicit_matrix_types,
               const struct glsl_type *t = (const struct glsl_type *) entry->data;
            assert(t->base_type == base_type);
   assert(t->vector_elements == rows);
   assert(t->matrix_columns == columns);
   assert(t->explicit_stride == explicit_stride);
               }
      const struct glsl_type *
   glsl_sampler_type(enum glsl_sampler_dim dim, bool shadow,
         {
      switch (type) {
   case GLSL_TYPE_FLOAT:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
      if (shadow)
         else
      case GLSL_SAMPLER_DIM_2D:
      if (shadow)
         else
      case GLSL_SAMPLER_DIM_3D:
      if (shadow || array)
         else
      case GLSL_SAMPLER_DIM_CUBE:
      if (shadow)
         else
      case GLSL_SAMPLER_DIM_RECT:
      if (array)
         if (shadow)
         else
      case GLSL_SAMPLER_DIM_BUF:
      if (shadow || array)
         else
      case GLSL_SAMPLER_DIM_MS:
      if (shadow)
            case GLSL_SAMPLER_DIM_EXTERNAL:
      if (shadow || array)
         else
      case GLSL_SAMPLER_DIM_SUBPASS:
   case GLSL_SAMPLER_DIM_SUBPASS_MS:
         }
      case GLSL_TYPE_INT:
      if (shadow)
         switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
      if (array)
            case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
      if (array)
            case GLSL_SAMPLER_DIM_BUF:
      if (array)
            case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_EXTERNAL:
         case GLSL_SAMPLER_DIM_SUBPASS:
   case GLSL_SAMPLER_DIM_SUBPASS_MS:
         }
      case GLSL_TYPE_UINT:
      if (shadow)
         switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
      if (array)
            case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
      if (array)
            case GLSL_SAMPLER_DIM_BUF:
      if (array)
            case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_EXTERNAL:
         case GLSL_SAMPLER_DIM_SUBPASS:
   case GLSL_SAMPLER_DIM_SUBPASS_MS:
         }
      case GLSL_TYPE_VOID:
         default:
                     }
      const struct glsl_type *
   glsl_bare_sampler_type()
   {
         }
      const struct glsl_type *
   glsl_bare_shadow_sampler_type()
   {
         }
      const struct glsl_type *
   glsl_texture_type(enum glsl_sampler_dim dim, bool array, enum glsl_base_type type)
   {
      switch (type) {
   case GLSL_TYPE_FLOAT:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
         case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
      if (array)
         else
      case GLSL_SAMPLER_DIM_BUF:
      if (array)
         else
      case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_SUBPASS:
         case GLSL_SAMPLER_DIM_SUBPASS_MS:
         case GLSL_SAMPLER_DIM_EXTERNAL:
      if (array)
         else
      }
      case GLSL_TYPE_INT:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
      if (array)
            case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
      if (array)
            case GLSL_SAMPLER_DIM_BUF:
      if (array)
            case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_SUBPASS:
         case GLSL_SAMPLER_DIM_SUBPASS_MS:
         case GLSL_SAMPLER_DIM_EXTERNAL:
         }
      case GLSL_TYPE_UINT:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
      if (array)
            case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
      if (array)
            case GLSL_SAMPLER_DIM_BUF:
      if (array)
            case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_SUBPASS:
         case GLSL_SAMPLER_DIM_SUBPASS_MS:
         case GLSL_SAMPLER_DIM_EXTERNAL:
         }
      case GLSL_TYPE_VOID:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
         case GLSL_SAMPLER_DIM_BUF:
         default:
            default:
                     }
      const struct glsl_type *
   glsl_image_type(enum glsl_sampler_dim dim, bool array, enum glsl_base_type type)
   {
      switch (type) {
   case GLSL_TYPE_FLOAT:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
         case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
      if (array)
         else
      case GLSL_SAMPLER_DIM_BUF:
      if (array)
         else
      case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_SUBPASS:
         case GLSL_SAMPLER_DIM_SUBPASS_MS:
         case GLSL_SAMPLER_DIM_EXTERNAL:
         }
      case GLSL_TYPE_INT:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
      if (array)
            case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
      if (array)
            case GLSL_SAMPLER_DIM_BUF:
      if (array)
            case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_SUBPASS:
         case GLSL_SAMPLER_DIM_SUBPASS_MS:
         case GLSL_SAMPLER_DIM_EXTERNAL:
         }
      case GLSL_TYPE_UINT:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
      if (array)
            case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
      if (array)
            case GLSL_SAMPLER_DIM_BUF:
      if (array)
            case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_SUBPASS:
         case GLSL_SAMPLER_DIM_SUBPASS_MS:
         case GLSL_SAMPLER_DIM_EXTERNAL:
         }
      case GLSL_TYPE_INT64:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
      if (array)
            case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
      if (array)
            case GLSL_SAMPLER_DIM_BUF:
      if (array)
            case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_SUBPASS:
   case GLSL_SAMPLER_DIM_SUBPASS_MS:
   case GLSL_SAMPLER_DIM_EXTERNAL:
         }
      case GLSL_TYPE_UINT64:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
      if (array)
            case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
      if (array)
            case GLSL_SAMPLER_DIM_BUF:
      if (array)
            case GLSL_SAMPLER_DIM_MS:
         case GLSL_SAMPLER_DIM_SUBPASS:
   case GLSL_SAMPLER_DIM_SUBPASS_MS:
   case GLSL_SAMPLER_DIM_EXTERNAL:
         }
      case GLSL_TYPE_VOID:
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
         case GLSL_SAMPLER_DIM_BUF:
         default:
            default:
                     }
      struct PACKED array_key {
      uintptr_t element;
   uintptr_t array_size;
      };
      static uint32_t
   hash_array_key(const void *a)
   {
         }
      static bool
   compare_array_key(const void *a, const void *b)
   {
         }
      const struct glsl_type *
   glsl_array_type(const struct glsl_type *element,
               {
      /* Ensure there's no internal padding, to avoid multiple hashes for same key. */
            struct array_key key = { 0 };
   key.element = (uintptr_t)element;
   key.array_size = array_size;
                     simple_mtx_lock(&glsl_type_cache_mutex);
   assert(glsl_type_cache.users > 0);
            if (glsl_type_cache.array_types == NULL) {
      glsl_type_cache.array_types =
      }
            const struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(array_types, key_hash, &key);
   if (entry == NULL) {
      linear_ctx *lin_ctx = glsl_type_cache.lin_ctx;
   const struct glsl_type *t = make_array_type(lin_ctx, element, array_size, explicit_stride);
   struct array_key *stored_key = linear_zalloc(lin_ctx, struct array_key);
            entry = _mesa_hash_table_insert_pre_hashed(array_types, key_hash,
                     const struct glsl_type *t = (const struct glsl_type *) entry->data;
            assert(t->base_type == GLSL_TYPE_ARRAY);
   assert(t->length == array_size);
               }
      static const struct glsl_type *
   make_cmat_type(linear_ctx *lin_ctx, const struct glsl_cmat_description desc)
   {
               struct glsl_type *t = linear_zalloc(lin_ctx, struct glsl_type);
   t->base_type = GLSL_TYPE_COOPERATIVE_MATRIX;
   t->sampled_type = GLSL_TYPE_VOID;
   t->vector_elements = 1;
            const struct glsl_type *element_type = glsl_simple_type(desc.element_type, 1, 1);
   t->name_id = (uintptr_t ) linear_asprintf(lin_ctx, "coopmat<%s, %s, %u, %u, %s>",
                                 }
      const struct glsl_type *
   glsl_cmat_type(const struct glsl_cmat_description *desc)
   {
               const uint32_t key = desc->element_type | desc->scope << 5 |
                        simple_mtx_lock(&glsl_type_cache_mutex);
   assert(glsl_type_cache.users > 0);
            if (glsl_type_cache.cmat_types == NULL) {
      glsl_type_cache.cmat_types =
      }
            const struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(
         if (entry == NULL) {
      const struct glsl_type *t = make_cmat_type(glsl_type_cache.lin_ctx, *desc);
   entry = _mesa_hash_table_insert_pre_hashed(cmat_types, key_hash,
               const struct glsl_type *t = (const struct glsl_type *)entry->data;
            assert(t->base_type == GLSL_TYPE_COOPERATIVE_MATRIX);
   assert(t->cmat_desc.element_type == desc->element_type);
   assert(t->cmat_desc.scope == desc->scope);
   assert(t->cmat_desc.rows == desc->rows);
   assert(t->cmat_desc.cols == desc->cols);
               }
      bool
   glsl_type_compare_no_precision(const struct glsl_type *a, const struct glsl_type *b)
   {
      if (a == b)
            if (glsl_type_is_array(a)) {
      if (!glsl_type_is_array(b) || a->length != b->length)
                                 if (glsl_type_is_struct(a)) {
      if (!glsl_type_is_struct(b))
      } else if (glsl_type_is_interface(a)) {
      if (!glsl_type_is_interface(b))
      } else {
                  return glsl_record_compare(a, b,
                  }
      bool
   glsl_record_compare(const struct glsl_type *a, const struct glsl_type *b, bool match_name,
         {
      if (a->length != b->length)
            if (a->interface_packing != b->interface_packing)
            if (a->interface_row_major != b->interface_row_major)
            if (a->explicit_alignment != b->explicit_alignment)
            if (a->packed != b->packed)
            /* From the GLSL 4.20 specification (Sec 4.2):
   *
   *     "Structures must have the same name, sequence of type names, and
   *     type definitions, and field names to be considered the same type."
   *
   * GLSL ES behaves the same (Ver 1.00 Sec 4.2.4, Ver 3.00 Sec 4.2.5).
   *
   * Section 7.4.1 (Shader Interface Matching) of the OpenGL 4.30 spec says:
   *
   *     "Variables or block members declared as structures are considered
   *     to match in type if and only if structure members match in name,
   *     type, qualification, and declaration order."
   */
   if (match_name)
      if (strcmp(glsl_get_type_name(a), glsl_get_type_name(b)) != 0)
         for (unsigned i = 0; i < a->length; i++) {
      if (match_precision) {
      if (a->fields.structure[i].type != b->fields.structure[i].type)
      } else {
      const struct glsl_type *ta = a->fields.structure[i].type;
   const struct glsl_type *tb = b->fields.structure[i].type;
   if (!glsl_type_compare_no_precision(ta, tb))
      }
   if (strcmp(a->fields.structure[i].name,
               if (a->fields.structure[i].matrix_layout
         return false;
   if (match_locations && a->fields.structure[i].location
      != b->fields.structure[i].location)
      if (a->fields.structure[i].component
      != b->fields.structure[i].component)
      if (a->fields.structure[i].offset
      != b->fields.structure[i].offset)
      if (a->fields.structure[i].interpolation
      != b->fields.structure[i].interpolation)
      if (a->fields.structure[i].centroid
      != b->fields.structure[i].centroid)
      if (a->fields.structure[i].sample
      != b->fields.structure[i].sample)
      if (a->fields.structure[i].patch
      != b->fields.structure[i].patch)
      if (a->fields.structure[i].memory_read_only
      != b->fields.structure[i].memory_read_only)
      if (a->fields.structure[i].memory_write_only
      != b->fields.structure[i].memory_write_only)
      if (a->fields.structure[i].memory_coherent
      != b->fields.structure[i].memory_coherent)
      if (a->fields.structure[i].memory_volatile
      != b->fields.structure[i].memory_volatile)
      if (a->fields.structure[i].memory_restrict
      != b->fields.structure[i].memory_restrict)
      if (a->fields.structure[i].image_format
      != b->fields.structure[i].image_format)
      if (match_precision &&
      a->fields.structure[i].precision
   != b->fields.structure[i].precision)
      if (a->fields.structure[i].explicit_xfb_buffer
      != b->fields.structure[i].explicit_xfb_buffer)
      if (a->fields.structure[i].xfb_buffer
      != b->fields.structure[i].xfb_buffer)
      if (a->fields.structure[i].xfb_stride
      != b->fields.structure[i].xfb_stride)
               }
         static bool
   record_key_compare(const void *a, const void *b)
   {
      const struct glsl_type *const key1 = (struct glsl_type *) a;
            return strcmp(glsl_get_type_name(key1), glsl_get_type_name(key2)) == 0 &&
      }
         /**
   * Generate an integer hash value for a glsl_type structure type.
   */
   static unsigned
   record_key_hash(const void *a)
   {
      const struct glsl_type *const key = (struct glsl_type *) a;
   uintptr_t hash = key->length;
            for (unsigned i = 0; i < key->length; i++) {
      /* casting pointer to uintptr_t */
               if (sizeof(hash) == 8)
         else
               }
      const struct glsl_type *
   glsl_struct_type_with_explicit_alignment(const struct glsl_struct_field *fields,
                     {
      struct glsl_type key = {0};
   fill_struct_type(&key, fields, num_fields, name, packed, explicit_alignment);
            simple_mtx_lock(&glsl_type_cache_mutex);
   assert(glsl_type_cache.users > 0);
            if (glsl_type_cache.struct_types == NULL) {
      glsl_type_cache.struct_types =
      }
            const struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(struct_types,
         if (entry == NULL) {
      const struct glsl_type *t = make_struct_type(glsl_type_cache.lin_ctx, fields, num_fields,
                        const struct glsl_type *t = (const struct glsl_type *) entry->data;
            assert(t->base_type == GLSL_TYPE_STRUCT);
   assert(t->length == num_fields);
   assert(strcmp(glsl_get_type_name(t), name) == 0);
   assert(t->packed == packed);
               }
         const struct glsl_type *
   glsl_interface_type(const struct glsl_struct_field *fields,
                     unsigned num_fields,
   {
      struct glsl_type key = {0};
   fill_interface_type(&key, fields, num_fields, packing, row_major, block_name);
            simple_mtx_lock(&glsl_type_cache_mutex);
   assert(glsl_type_cache.users > 0);
            if (glsl_type_cache.interface_types == NULL) {
      glsl_type_cache.interface_types =
      }
            const struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(interface_types,
         if (entry == NULL) {
      const struct glsl_type *t = make_interface_type(glsl_type_cache.lin_ctx, fields, num_fields,
                        const struct glsl_type *t = (const struct glsl_type *) entry->data;
            assert(t->base_type == GLSL_TYPE_INTERFACE);
   assert(t->length == num_fields);
               }
      const struct glsl_type *
   glsl_subroutine_type(const char *subroutine_name)
   {
               simple_mtx_lock(&glsl_type_cache_mutex);
   assert(glsl_type_cache.users > 0);
            if (glsl_type_cache.subroutine_types == NULL) {
      glsl_type_cache.subroutine_types =
      }
            const struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(subroutine_types,
         if (entry == NULL) {
                           const struct glsl_type *t = (const struct glsl_type *) entry->data;
            assert(t->base_type == GLSL_TYPE_SUBROUTINE);
               }
      const struct glsl_type *
   glsl_get_mul_type(const struct glsl_type *type_a, const struct glsl_type *type_b)
   {
      if (glsl_type_is_matrix(type_a) && glsl_type_is_matrix(type_b)) {
      /* Matrix multiply.  The columns of A must match the rows of B.  Given
   * the other previously tested constraints, this means the vector type
   * of a row from A must be the same as the vector type of a column from
   * B.
   */
   if (glsl_get_row_type(type_a) == glsl_get_column_type(type_b)) {
      /* The resulting matrix has the number of columns of matrix B and
   * the number of rows of matrix A.  We get the row count of A by
   * looking at the size of a vector that makes up a column.  The
   * transpose (size of a row) is done for B.
   */
   const struct glsl_type *const type =
      glsl_simple_type(type_a->base_type,
                           } else if (type_a == type_b) {
         } else if (glsl_type_is_matrix(type_a)) {
      /* A is a matrix and B is a column vector.  Columns of A must match
   * rows of B.  Given the other previously tested constraints, this
   * means the vector type of a row from A must be the same as the
   * vector the type of B.
   */
   if (glsl_get_row_type(type_a) == type_b) {
      /* The resulting vector has a number of elements equal to
   * the number of rows of matrix A. */
   const struct glsl_type *const type =
      glsl_simple_type(type_a->base_type,
                     } else {
               /* A is a row vector and B is a matrix.  Columns of A must match rows
   * of B.  Given the other previously tested constraints, this means
   * the type of A must be the same as the vector type of a column from
   * B.
   */
   if (type_a == glsl_get_column_type(type_b)) {
      /* The resulting vector has a number of elements equal to
   * the number of columns of matrix B. */
   const struct glsl_type *const type =
      glsl_simple_type(type_a->base_type,
                                 }
      int
   glsl_get_field_index(const struct glsl_type *t, const char *name)
   {
      if (t->base_type != GLSL_TYPE_STRUCT &&
      t->base_type != GLSL_TYPE_INTERFACE)
         for (unsigned i = 0; i < t->length; i++) {
      if (strcmp(name, t->fields.structure[i].name) == 0)
                  }
         unsigned
   glsl_get_component_slots(const struct glsl_type *t)
   {
      switch (t->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_BOOL:
            case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
            case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE: {
               for (unsigned i = 0; i < t->length; i++)
                        case GLSL_TYPE_ARRAY:
            case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
            case GLSL_TYPE_SUBROUTINE:
            case GLSL_TYPE_COOPERATIVE_MATRIX:
   case GLSL_TYPE_ATOMIC_UINT:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
                     }
      unsigned
   glsl_get_component_slots_aligned(const struct glsl_type *t, unsigned offset)
   {
      /* Align 64bit type only if it crosses attribute slot boundary. */
   switch (t->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_BOOL:
            case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64: {
      unsigned size = 2 * glsl_get_components(t);
   if (offset % 2 == 1 && (offset % 4 + size) > 4) {
                              case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE: {
               for (unsigned i = 0; i < t->length; i++) {
      const struct glsl_type *member = t->fields.structure[i].type;
                           case GLSL_TYPE_ARRAY: {
               for (unsigned i = 0; i < t->length; i++) {
      size += glsl_get_component_slots_aligned(t->fields.array,
                           case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
            case GLSL_TYPE_SUBROUTINE:
            case GLSL_TYPE_COOPERATIVE_MATRIX:
   case GLSL_TYPE_ATOMIC_UINT:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
                     }
      unsigned
   glsl_get_struct_location_offset(const struct glsl_type *t, unsigned length)
   {
      unsigned offset = 0;
   t = glsl_without_array(t);
   if (glsl_type_is_struct(t)) {
               for (unsigned i = 0; i < length; i++) {
      const struct glsl_type *st = t->fields.structure[i].type;
   const struct glsl_type *wa = glsl_without_array(st);
   if (glsl_type_is_struct(wa)) {
      unsigned r_offset = glsl_get_struct_location_offset(wa, wa->length);
   offset += glsl_type_is_array(st) ?
      } else if (glsl_type_is_array(st) && glsl_type_is_array(st->fields.array)) {
                     /* For arrays of arrays the outer arrays take up a uniform
   * slot for each element. The innermost array elements share a
   * single slot so we ignore the innermost array when calculating
   * the offset.
   */
   while (glsl_type_is_array(base_type->fields.array)) {
      outer_array_size = outer_array_size * base_type->length;
      }
      } else {
      /* We dont worry about arrays here because unless the array
   * contains a structure or another array it only takes up a single
   * uniform slot.
   */
            }
      }
      unsigned
   glsl_type_uniform_locations(const struct glsl_type *t)
   {
               switch (t->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
   case GLSL_TYPE_BOOL:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_SUBROUTINE:
            case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE:
      for (unsigned i = 0; i < t->length; i++)
            case GLSL_TYPE_ARRAY:
         default:
            }
      unsigned
   glsl_varying_count(const struct glsl_type *t)
   {
               switch (t->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_BOOL:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
            case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE:
      for (unsigned i = 0; i < t->length; i++)
            case GLSL_TYPE_ARRAY:
      /* Don't count innermost array elements */
   if (glsl_type_is_struct(glsl_without_array(t)) ||
      glsl_type_is_interface(glsl_without_array(t)) ||
   glsl_type_is_array(t->fields.array))
      else
      default:
      assert(!"unsupported varying type");
         }
      unsigned
   glsl_get_std140_base_alignment(const struct glsl_type *t, bool row_major)
   {
               /* (1) If the member is a scalar consuming <N> basic machine units, the
   *     base alignment is <N>.
   *
   * (2) If the member is a two- or four-component vector with components
   *     consuming <N> basic machine units, the base alignment is 2<N> or
   *     4<N>, respectively.
   *
   * (3) If the member is a three-component vector with components consuming
   *     <N> basic machine units, the base alignment is 4<N>.
   */
   if (glsl_type_is_scalar(t) || glsl_type_is_vector(t)) {
      switch (t->vector_elements) {
   case 1:
         case 2:
         case 3:
   case 4:
                     /* (4) If the member is an array of scalars or vectors, the base alignment
   *     and array stride are set to match the base alignment of a single
   *     array element, according to rules (1), (2), and (3), and rounded up
   *     to the base alignment of a vec4. The array may have padding at the
   *     end; the base offset of the member following the array is rounded up
   *     to the next multiple of the base alignment.
   *
   * (6) If the member is an array of <S> column-major matrices with <C>
   *     columns and <R> rows, the matrix is stored identically to a row of
   *     <S>*<C> column vectors with <R> components each, according to rule
   *     (4).
   *
   * (8) If the member is an array of <S> row-major matrices with <C> columns
   *     and <R> rows, the matrix is stored identically to a row of <S>*<R>
   *     row vectors with <C> components each, according to rule (4).
   *
   * (10) If the member is an array of <S> structures, the <S> elements of
   *      the array are laid out in order, according to rule (9).
   */
   if (glsl_type_is_array(t)) {
      if (glsl_type_is_scalar(t->fields.array) ||
      glsl_type_is_vector(t->fields.array) ||
   glsl_type_is_matrix(t->fields.array)) {
   return MAX2(glsl_get_std140_base_alignment(t->fields.array, row_major),
      } else {
      assert(glsl_type_is_struct(t->fields.array) ||
                        /* (5) If the member is a column-major matrix with <C> columns and
   *     <R> rows, the matrix is stored identically to an array of
   *     <C> column vectors with <R> components each, according to
   *     rule (4).
   *
   * (7) If the member is a row-major matrix with <C> columns and <R>
   *     rows, the matrix is stored identically to an array of <R>
   *     row vectors with <C> components each, according to rule (4).
   */
   if (glsl_type_is_matrix(t)) {
      const struct glsl_type *vec_type, *array_type;
   int c = t->matrix_columns;
            if (row_major) {
      vec_type = glsl_simple_type(t->base_type, c, 1);
      } else {
      vec_type = glsl_simple_type(t->base_type, r, 1);
                           /* (9) If the member is a structure, the base alignment of the
   *     structure is <N>, where <N> is the largest base alignment
   *     value of any of its members, and rounded up to the base
   *     alignment of a vec4. The individual members of this
   *     sub-structure are then assigned offsets by applying this set
   *     of rules recursively, where the base offset of the first
   *     member of the sub-structure is equal to the aligned offset
   *     of the structure. The structure may have padding at the end;
   *     the base offset of the member following the sub-structure is
   *     rounded up to the next multiple of the base alignment of the
   *     structure.
   */
   if (glsl_type_is_struct(t)) {
      unsigned base_alignment = 16;
   for (unsigned i = 0; i < t->length; i++) {
      bool field_row_major = row_major;
   const enum glsl_matrix_layout matrix_layout =
         if (matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR) {
         } else if (matrix_layout == GLSL_MATRIX_LAYOUT_COLUMN_MAJOR) {
                  const struct glsl_type *field_type = t->fields.structure[i].type;
   base_alignment = MAX2(base_alignment,
      }
               assert(!"not reached");
      }
      unsigned
   glsl_get_std140_size(const struct glsl_type *t, bool row_major)
   {
               /* (1) If the member is a scalar consuming <N> basic machine units, the
   *     base alignment is <N>.
   *
   * (2) If the member is a two- or four-component vector with components
   *     consuming <N> basic machine units, the base alignment is 2<N> or
   *     4<N>, respectively.
   *
   * (3) If the member is a three-component vector with components consuming
   *     <N> basic machine units, the base alignment is 4<N>.
   */
   if (glsl_type_is_scalar(t) || glsl_type_is_vector(t)) {
      assert(t->explicit_stride == 0);
               /* (5) If the member is a column-major matrix with <C> columns and
   *     <R> rows, the matrix is stored identically to an array of
   *     <C> column vectors with <R> components each, according to
   *     rule (4).
   *
   * (6) If the member is an array of <S> column-major matrices with <C>
   *     columns and <R> rows, the matrix is stored identically to a row of
   *     <S>*<C> column vectors with <R> components each, according to rule
   *     (4).
   *
   * (7) If the member is a row-major matrix with <C> columns and <R>
   *     rows, the matrix is stored identically to an array of <R>
   *     row vectors with <C> components each, according to rule (4).
   *
   * (8) If the member is an array of <S> row-major matrices with <C> columns
   *     and <R> rows, the matrix is stored identically to a row of <S>*<R>
   *     row vectors with <C> components each, according to rule (4).
   */
   if (glsl_type_is_matrix(glsl_without_array(t))) {
      const struct glsl_type *element_type;
   const struct glsl_type *vec_type;
            if (glsl_type_is_array(t)) {
      element_type = glsl_without_array(t);
      } else {
      element_type = t;
               if (row_major) {
                        } else {
      vec_type = glsl_simple_type(element_type->base_type,
            }
   const struct glsl_type *array_type =
                        /* (4) If the member is an array of scalars or vectors, the base alignment
   *     and array stride are set to match the base alignment of a single
   *     array element, according to rules (1), (2), and (3), and rounded up
   *     to the base alignment of a vec4. The array may have padding at the
   *     end; the base offset of the member following the array is rounded up
   *     to the next multiple of the base alignment.
   *
   * (10) If the member is an array of <S> structures, the <S> elements of
   *      the array are laid out in order, according to rule (9).
   */
   if (glsl_type_is_array(t)) {
      unsigned stride;
   stride = glsl_get_std140_size(glsl_without_array(t), row_major);
         unsigned element_base_align =
      glsl_get_std140_base_alignment(glsl_without_array(t), row_major);
                     unsigned size = glsl_get_aoa_size(t) * stride;
   assert(t->explicit_stride == 0 ||
                     /* (9) If the member is a structure, the base alignment of the
   *     structure is <N>, where <N> is the largest base alignment
   *     value of any of its members, and rounded up to the base
   *     alignment of a vec4. The individual members of this
   *     sub-structure are then assigned offsets by applying this set
   *     of rules recursively, where the base offset of the first
   *     member of the sub-structure is equal to the aligned offset
   *     of the structure. The structure may have padding at the end;
   *     the base offset of the member following the sub-structure is
   *     rounded up to the next multiple of the base alignment of the
   *     structure.
   */
   if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      unsigned size = 0;
            for (unsigned i = 0; i < t->length; i++) {
      bool field_row_major = row_major;
   const enum glsl_matrix_layout matrix_layout =
         if (matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR) {
         } else if (matrix_layout == GLSL_MATRIX_LAYOUT_COLUMN_MAJOR) {
                  const struct glsl_type *field_type = t->fields.structure[i].type;
                  /* Ignore unsized arrays when calculating size */
                                          if (glsl_type_is_struct(field_type) && (i + 1 < t->length))
      }
   size = align(size, MAX2(max_align, 16));
               assert(!"not reached");
      }
      const struct glsl_type *
   glsl_get_explicit_std140_type(const struct glsl_type *t, bool row_major)
   {
      if (glsl_type_is_vector(t) || glsl_type_is_scalar(t)) {
         } else if (glsl_type_is_matrix(t)) {
      const struct glsl_type *vec_type;
   if (row_major)
         else
         unsigned elem_size = glsl_get_std140_size(vec_type, false);
   unsigned stride = align(elem_size, 16);
   return glsl_simple_explicit_type(t->base_type, t->vector_elements,
            } else if (glsl_type_is_array(t)) {
      unsigned elem_size = glsl_get_std140_size(t->fields.array, row_major);
   const struct glsl_type *elem_type =
         unsigned stride = align(elem_size, 16);
      } else if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      struct glsl_struct_field *fields = (struct glsl_struct_field *)
         unsigned offset = 0;
   for (unsigned i = 0; i < t->length; i++) {
               bool field_row_major = row_major;
   if (fields[i].matrix_layout == GLSL_MATRIX_LAYOUT_COLUMN_MAJOR) {
         } else if (fields[i].matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR) {
         }
                  unsigned fsize = glsl_get_std140_size(fields[i].type,
         unsigned falign = glsl_get_std140_base_alignment(fields[i].type,
         /* From the GLSL 460 spec section "Uniform and Shader Storage Block
   * Layout Qualifiers":
   *
   *    "The actual offset of a member is computed as follows: If
   *    offset was declared, start with that offset, otherwise start
   *    with the next available offset. If the resulting offset is not
   *    a multiple of the actual alignment, increase it to the first
   *    offset that is a multiple of the actual alignment. This results
   *    in the actual offset the member will have."
   */
   if (fields[i].offset >= 0) {
      assert((unsigned)fields[i].offset >= offset);
      }
   offset = align(offset, falign);
   fields[i].offset = offset;
               const struct glsl_type *type;
   if (glsl_type_is_struct(t))
         else
      type = glsl_interface_type(fields, t->length,
               free(fields);
      } else {
            }
      unsigned
   glsl_get_std430_base_alignment(const struct glsl_type *t, bool row_major)
   {
                  /* (1) If the member is a scalar consuming <N> basic machine units, the
   *     base alignment is <N>.
   *
   * (2) If the member is a two- or four-component vector with components
   *     consuming <N> basic machine units, the base alignment is 2<N> or
   *     4<N>, respectively.
   *
   * (3) If the member is a three-component vector with components consuming
   *     <N> basic machine units, the base alignment is 4<N>.
   */
   if (glsl_type_is_scalar(t) || glsl_type_is_vector(t)) {
      switch (t->vector_elements) {
   case 1:
         case 2:
         case 3:
   case 4:
                     /* OpenGL 4.30 spec, section 7.6.2.2 "Standard Uniform Block Layout":
   *
   * "When using the std430 storage layout, shader storage blocks will be
   * laid out in buffer storage identically to uniform and shader storage
   * blocks using the std140 layout, except that the base alignment and
   * stride of arrays of scalars and vectors in rule 4 and of structures
   * in rule 9 are not rounded up a multiple of the base alignment of a vec4.
            /* (1) If the member is a scalar consuming <N> basic machine units, the
   *     base alignment is <N>.
   *
   * (2) If the member is a two- or four-component vector with components
   *     consuming <N> basic machine units, the base alignment is 2<N> or
   *     4<N>, respectively.
   *
   * (3) If the member is a three-component vector with components consuming
   *     <N> basic machine units, the base alignment is 4<N>.
   */
   if (glsl_type_is_array(t))
            /* (5) If the member is a column-major matrix with <C> columns and
   *     <R> rows, the matrix is stored identically to an array of
   *     <C> column vectors with <R> components each, according to
   *     rule (4).
   *
   * (7) If the member is a row-major matrix with <C> columns and <R>
   *     rows, the matrix is stored identically to an array of <R>
   *     row vectors with <C> components each, according to rule (4).
   */
   if (glsl_type_is_matrix(t)) {
      const struct glsl_type *vec_type, *array_type;
   int c = t->matrix_columns;
            if (row_major) {
      vec_type = glsl_simple_type(t->base_type, c, 1);
      } else {
      vec_type = glsl_simple_type(t->base_type, r, 1);
                                 *     structure is <N>, where <N> is the largest base alignment
   *     value of any of its members, and rounded up to the base
   *     alignment of a vec4. The individual members of this
   *     sub-structure are then assigned offsets by applying this set
   *     of rules recursively, where the base offset of the first
   *     member of the sub-structure is equal to the aligned offset
   *     of the structure. The structure may have padding at the end;
   *     the base offset of the member following the sub-structure is
   *     rounded up to the next multiple of the base alignment of the
   *     structure.
   */
   if (glsl_type_is_struct(t)) {
      unsigned base_alignment = 0;
   for (unsigned i = 0; i < t->length; i++) {
      bool field_row_major = row_major;
   const enum glsl_matrix_layout matrix_layout =
         if (matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR) {
         } else if (matrix_layout == GLSL_MATRIX_LAYOUT_COLUMN_MAJOR) {
                  const struct glsl_type *field_type = t->fields.structure[i].type;
   base_alignment = MAX2(base_alignment,
      }
   assert(base_alignment > 0);
      }
   assert(!"not reached");
      }
      unsigned
   glsl_get_std430_array_stride(const struct glsl_type *t, bool row_major)
   {
               /* Notice that the array stride of a vec3 is not 3 * N but 4 * N.
   * See OpenGL 4.30 spec, section 7.6.2.2 "Standard Uniform Block Layout"
   *
   * (3) If the member is a three-component vector with components consuming
   *     <N> basic machine units, the base alignment is 4<N>.
   */
   if (glsl_type_is_vector(t) && t->vector_elements == 3)
            /* By default use std430_size(row_major) */
   unsigned stride = glsl_get_std430_size(t, row_major);
   assert(t->explicit_stride == 0 || t->explicit_stride == stride);
      }
      /* Note that the value returned by this method is only correct if the
   * explit offset, and stride values are set, so only with SPIR-V shaders.
   * Should not be used with GLSL shaders.
   */
      unsigned
   glsl_get_explicit_size(const struct glsl_type *t, bool align_to_stride)
   {
      if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      if (t->length > 0) {
               for (unsigned i = 0; i < t->length; i++) {
      assert(t->fields.structure[i].offset >= 0);
   unsigned last_byte = t->fields.structure[i].offset +
                        } else {
            } else if (glsl_type_is_array(t)) {
      /* From ARB_program_interface_query spec:
   *
   *   "For the property of BUFFER_DATA_SIZE, then the implementation-dependent
   *   minimum total buffer object size, in basic machine units, required to
   *   hold all active variables associated with an active uniform block, shader
   *   storage block, or atomic counter buffer is written to <params>.  If the
   *   final member of an active shader storage block is array with no declared
   *   size, the minimum buffer size is computed assuming the array was declared
   *   as an array with one element."
   *
   */
   if (glsl_type_is_unsized_array(t))
            assert(t->length > 0);
   unsigned elem_size = align_to_stride ? t->explicit_stride : glsl_get_explicit_size(t->fields.array,
                     } else if (glsl_type_is_matrix(t)) {
      const struct glsl_type *elem_type;
            if (t->interface_row_major) {
      elem_type = glsl_simple_type(t->base_type, t->matrix_columns, 1);
      } else {
      elem_type = glsl_simple_type(t->base_type, t->vector_elements, 1);
               unsigned elem_size = align_to_stride ? t->explicit_stride : glsl_get_explicit_size(elem_type,
            assert(t->explicit_stride);
                           }
      unsigned
   glsl_get_std430_size(const struct glsl_type *t, bool row_major)
   {
               /* OpenGL 4.30 spec, section 7.6.2.2 "Standard Uniform Block Layout":
   *
   * "When using the std430 storage layout, shader storage blocks will be
   * laid out in buffer storage identically to uniform and shader storage
   * blocks using the std140 layout, except that the base alignment and
   * stride of arrays of scalars and vectors in rule 4 and of structures
   * in rule 9 are not rounded up a multiple of the base alignment of a vec4.
   */
   if (glsl_type_is_scalar(t) || glsl_type_is_vector(t)) {
      assert(t->explicit_stride == 0);
               if (glsl_type_is_matrix(glsl_without_array(t))) {
      const struct glsl_type *element_type;
   const struct glsl_type *vec_type;
            if (glsl_type_is_array(t)) {
      element_type = glsl_without_array(t);
      } else {
      element_type = t;
               if (row_major) {
                        } else {
      vec_type = glsl_simple_type(element_type->base_type,
            }
   const struct glsl_type *array_type =
                        if (glsl_type_is_array(t)) {
      unsigned stride;
   if (glsl_type_is_struct(glsl_without_array(t)))
         else
                  unsigned size = glsl_get_aoa_size(t) * stride;
   assert(t->explicit_stride == 0 ||
                     if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      unsigned size = 0;
            for (unsigned i = 0; i < t->length; i++) {
      bool field_row_major = row_major;
   const enum glsl_matrix_layout matrix_layout =
         if (matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR) {
         } else if (matrix_layout == GLSL_MATRIX_LAYOUT_COLUMN_MAJOR) {
                  const struct glsl_type *field_type = t->fields.structure[i].type;
   unsigned base_alignment = glsl_get_std430_base_alignment(field_type,
                           }
   size = align(size, max_align);
               assert(!"not reached");
      }
      const struct glsl_type *
   glsl_get_explicit_std430_type(const struct glsl_type *t, bool row_major)
   {
      if (glsl_type_is_vector(t) || glsl_type_is_scalar(t)) {
         } else if (glsl_type_is_matrix(t)) {
      const struct glsl_type *vec_type;
   if (row_major)
         else
         unsigned stride = glsl_get_std430_array_stride(vec_type, false);
   return glsl_simple_explicit_type(t->base_type, t->vector_elements,
            } else if (glsl_type_is_array(t)) {
      const struct glsl_type *elem_type =
         unsigned stride = glsl_get_std430_array_stride(t->fields.array,
            } else if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      struct glsl_struct_field *fields = (struct glsl_struct_field *)
         unsigned offset = 0;
   for (unsigned i = 0; i < t->length; i++) {
               bool field_row_major = row_major;
   if (fields[i].matrix_layout == GLSL_MATRIX_LAYOUT_COLUMN_MAJOR) {
         } else if (fields[i].matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR) {
         }
                  unsigned fsize = glsl_get_std430_size(fields[i].type,
         unsigned falign = glsl_get_std430_base_alignment(fields[i].type,
         /* From the GLSL 460 spec section "Uniform and Shader Storage Block
   * Layout Qualifiers":
   *
   *    "The actual offset of a member is computed as follows: If
   *    offset was declared, start with that offset, otherwise start
   *    with the next available offset. If the resulting offset is not
   *    a multiple of the actual alignment, increase it to the first
   *    offset that is a multiple of the actual alignment. This results
   *    in the actual offset the member will have."
   */
   if (fields[i].offset >= 0) {
      assert((unsigned)fields[i].offset >= offset);
      }
   offset = align(offset, falign);
   fields[i].offset = offset;
               const struct glsl_type *type;
   if (glsl_type_is_struct(t))
         else
      type = glsl_interface_type(fields, t->length,
               free(fields);
      } else {
            }
      static unsigned
   explicit_type_scalar_byte_size(const struct glsl_type *type)
   {
      if (type->base_type == GLSL_TYPE_BOOL)
         else
      }
      /* This differs from get_explicit_std430_type() in that it:
   * - can size arrays slightly smaller ("stride * (len - 1) + elem_size" instead
   *   of "stride * len")
   * - consumes a glsl_type_size_align_func which allows 8 and 16-bit values to be
   *   packed more tightly
   * - overrides any struct field offsets but get_explicit_std430_type() tries to
   *   respect any existing ones
   */
   const struct glsl_type *
   glsl_get_explicit_type_for_size_align(const struct glsl_type *t,
               {
      if (glsl_type_is_image(t) || glsl_type_is_sampler(t)) {
      type_info(t, size, alignment);
   assert(*alignment > 0);
      } else if (glsl_type_is_cmat(t)) {
      *size = 0;
   *alignment = 0;
      } else if (glsl_type_is_scalar(t)) {
      type_info(t, size, alignment);
   assert(*size == explicit_type_scalar_byte_size(t));
   assert(*alignment == explicit_type_scalar_byte_size(t));
      } else if (glsl_type_is_vector(t)) {
      type_info(t, size, alignment);
   assert(*alignment > 0);
   assert(*alignment % explicit_type_scalar_byte_size(t) == 0);
   return glsl_simple_explicit_type(t->base_type, t->vector_elements, 1, 0,
      } else if (glsl_type_is_array(t)) {
      unsigned elem_size, elem_align;
   const struct glsl_type *explicit_element =
                           *size = stride * (t->length - 1) + elem_size;
   *alignment = elem_align;
      } else if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      struct glsl_struct_field *fields = (struct glsl_struct_field *)
            *size = 0;
   *alignment = 1;
   for (unsigned i = 0; i < t->length; i++) {
                     unsigned field_size, field_align;
   fields[i].type =
      glsl_get_explicit_type_for_size_align(fields[i].type, type_info,
                     *size = fields[i].offset + field_size;
      }
   /*
   * "The alignment of the struct is the alignment of the most-aligned
   *  field in it."
   *
   * "Finally, the size of the struct is the current offset rounded up to
   *  the nearest multiple of the struct's alignment."
   *
   * https://doc.rust-lang.org/reference/type-layout.html#reprc-structs
   */
            const struct glsl_type *type;
   if (glsl_type_is_struct(t)) {
      type = glsl_struct_type_with_explicit_alignment(fields, t->length,
            } else {
      assert(!t->packed);
   type = glsl_interface_type(fields, t->length,
            }
   free(fields);
      } else if (glsl_type_is_matrix(t)) {
      unsigned col_size, col_align;
   type_info(glsl_get_column_type(t), &col_size, &col_align);
            *size = t->matrix_columns * stride;
   /* Matrix and column alignments match. See glsl_type::column_type() */
   assert(col_align > 0);
   *alignment = col_align;
   return glsl_simple_explicit_type(t->base_type, t->vector_elements,
            } else {
            }
      const struct glsl_type *
   glsl_type_replace_vec3_with_vec4(const struct glsl_type *t)
   {
      if (glsl_type_is_scalar(t) || glsl_type_is_vector(t) || glsl_type_is_matrix(t)) {
      if (t->interface_row_major) {
      if (t->matrix_columns == 3) {
      return glsl_simple_explicit_type(t->base_type, t->vector_elements,
                  } else {
            } else {
      if (t->vector_elements == 3) {
      return glsl_simple_explicit_type(t->base_type, 4,
                        } else {
               } else if (glsl_type_is_array(t)) {
      const struct glsl_type *vec4_elem_type =
         if (vec4_elem_type == t->fields.array)
            } else if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
      struct glsl_struct_field *fields = (struct glsl_struct_field *)
            bool needs_new_type = false;
   for (unsigned i = 0; i < t->length; i++) {
      fields[i] = t->fields.structure[i];
   assert(fields[i].matrix_layout != GLSL_MATRIX_LAYOUT_ROW_MAJOR);
   fields[i].type = glsl_type_replace_vec3_with_vec4(fields[i].type);
   if (fields[i].type != t->fields.structure[i].type)
               const struct glsl_type *type;
   if (!needs_new_type) {
         } else if (glsl_type_is_struct(t)) {
      type = glsl_struct_type_with_explicit_alignment(fields, t->length,
            } else {
      assert(!t->packed);
   type = glsl_interface_type(fields, t->length,
            }
   free(fields);
      } else {
            }
      unsigned
   glsl_count_vec4_slots(const struct glsl_type *t, bool is_gl_vertex_input, bool is_bindless)
   {
      /* From page 31 (page 37 of the PDF) of the GLSL 1.50 spec:
   *
   *     "A scalar input counts the same amount against this limit as a vec4,
   *     so applications may want to consider packing groups of four
   *     unrelated float inputs together into a vector to better utilize the
   *     capabilities of the underlying hardware. A matrix input will use up
   *     multiple locations.  The number of locations used will equal the
   *     number of columns in the matrix."
   *
   * The spec does not explicitly say how arrays are counted.  However, it
   * should be safe to assume the total number of slots consumed by an array
   * is the number of entries in the array multiplied by the number of slots
   * consumed by a single element of the array.
   *
   * The spec says nothing about how structs are counted, because vertex
   * attributes are not allowed to be (or contain) structs.  However, Mesa
   * allows varying structs, the number of varying slots taken up by a
   * varying struct is simply equal to the sum of the number of slots taken
   * up by each element.
   *
   * Doubles are counted different depending on whether they are vertex
   * inputs or everything else. Vertex inputs from ARB_vertex_attrib_64bit
   * take one location no matter what size they are, otherwise dvec3/4
   * take two locations.
   */
   switch (t->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_BOOL:
         case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
      if (t->vector_elements > 2 && !is_gl_vertex_input)
         else
      case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE: {
               for (unsigned i = 0; i < t->length; i++) {
      const struct glsl_type *member_type = t->fields.structure[i].type;
   size += glsl_count_vec4_slots(member_type, is_gl_vertex_input,
                           case GLSL_TYPE_ARRAY: {
      const struct glsl_type *element = t->fields.array;
   return t->length * glsl_count_vec4_slots(element, is_gl_vertex_input,
               case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
      if (!is_bindless)
         else
         case GLSL_TYPE_SUBROUTINE:
            case GLSL_TYPE_COOPERATIVE_MATRIX:
   case GLSL_TYPE_ATOMIC_UINT:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
                              }
      unsigned
   glsl_count_dword_slots(const struct glsl_type *t, bool is_bindless)
   {
      switch (t->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
         case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_FLOAT16:
         case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
         case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
      if (!is_bindless)
            case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
         case GLSL_TYPE_ARRAY:
      return glsl_count_dword_slots(t->fields.array, is_bindless) *
         case GLSL_TYPE_INTERFACE:
   case GLSL_TYPE_STRUCT: {
      unsigned size = 0;
   for (unsigned i = 0; i < t->length; i++) {
      size += glsl_count_dword_slots(t->fields.structure[i].type,
      }
               case GLSL_TYPE_ATOMIC_UINT:
         case GLSL_TYPE_SUBROUTINE:
         case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
   default:
                     }
      int
   glsl_get_sampler_coordinate_components(const struct glsl_type *t)
   {
      assert(glsl_type_is_sampler(t) ||
                  enum glsl_sampler_dim dim = (enum glsl_sampler_dim)t->sampler_dimensionality;
            /* Array textures need an additional component for the array index, except
   * for cubemap array images that behave like a 2D array of interleaved
   * cubemap faces.
   */
   if (t->sampler_array &&
      !(glsl_type_is_image(t) && t->sampler_dimensionality == GLSL_SAMPLER_DIM_CUBE))
            }
      union packed_type {
      uint32_t u32;
   struct {
      unsigned base_type:5;
   unsigned interface_row_major:1;
   unsigned vector_elements:3;
   unsigned matrix_columns:3;
   unsigned explicit_stride:16;
      } basic;
   struct {
      unsigned base_type:5;
   unsigned dimensionality:4;
   unsigned shadow:1;
   unsigned array:1;
   unsigned sampled_type:5;
      } sampler;
   struct {
      unsigned base_type:5;
   unsigned length:13;
      } array;
   struct glsl_cmat_description cmat_desc;
   struct {
      unsigned base_type:5;
   unsigned interface_packing_or_packed:2;
   unsigned interface_row_major:1;
   unsigned length:20;
         };
      static void
   encode_glsl_struct_field(struct blob *blob, const struct glsl_struct_field *struct_field)
   {
      encode_type_to_blob(blob, struct_field->type);
   blob_write_string(blob, struct_field->name);
   blob_write_uint32(blob, struct_field->location);
   blob_write_uint32(blob, struct_field->component);
   blob_write_uint32(blob, struct_field->offset);
   blob_write_uint32(blob, struct_field->xfb_buffer);
   blob_write_uint32(blob, struct_field->xfb_stride);
   blob_write_uint32(blob, struct_field->image_format);
      }
      static void
   decode_glsl_struct_field_from_blob(struct blob_reader *blob, struct glsl_struct_field *struct_field)
   {
      struct_field->type = decode_type_from_blob(blob);
   struct_field->name = blob_read_string(blob);
   struct_field->location = blob_read_uint32(blob);
   struct_field->component = blob_read_uint32(blob);
   struct_field->offset = blob_read_uint32(blob);
   struct_field->xfb_buffer = blob_read_uint32(blob);
   struct_field->xfb_stride = blob_read_uint32(blob);
   struct_field->image_format = (enum pipe_format)blob_read_uint32(blob);
      }
      void
   encode_type_to_blob(struct blob *blob, const struct glsl_type *type)
   {
      if (!type) {
      blob_write_uint32(blob, 0);
               STATIC_ASSERT(sizeof(union packed_type) == 4);
   union packed_type encoded;
   encoded.u32 = 0;
            switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
   case GLSL_TYPE_BOOL:
      encoded.basic.interface_row_major = type->interface_row_major;
   assert(type->matrix_columns < 8);
   if (type->vector_elements <= 5)
         else if (type->vector_elements == 8)
         else if (type->vector_elements == 16)
         encoded.basic.matrix_columns = type->matrix_columns;
   encoded.basic.explicit_stride = MIN2(type->explicit_stride, 0xffff);
   encoded.basic.explicit_alignment =
         blob_write_uint32(blob, encoded.u32);
   /* If we don't have enough bits for explicit_stride, store it
   * separately.
   */
   if (encoded.basic.explicit_stride == 0xffff)
         if (encoded.basic.explicit_alignment == 0xf)
            case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
      encoded.sampler.dimensionality = type->sampler_dimensionality;
   if (type->base_type == GLSL_TYPE_SAMPLER)
         else
         encoded.sampler.array = type->sampler_array;
   encoded.sampler.sampled_type = type->sampled_type;
      case GLSL_TYPE_SUBROUTINE:
      blob_write_uint32(blob, encoded.u32);
   blob_write_string(blob, glsl_get_type_name(type));
      case GLSL_TYPE_ATOMIC_UINT:
         case GLSL_TYPE_ARRAY:
      encoded.array.length = MIN2(type->length, 0x1fff);
   encoded.array.explicit_stride = MIN2(type->explicit_stride, 0x3fff);
   blob_write_uint32(blob, encoded.u32);
   /* If we don't have enough bits for length or explicit_stride, store it
   * separately.
   */
   if (encoded.array.length == 0x1fff)
         if (encoded.array.explicit_stride == 0x3fff)
         encode_type_to_blob(blob, type->fields.array);
      case GLSL_TYPE_COOPERATIVE_MATRIX:
      encoded.cmat_desc = type->cmat_desc;
   blob_write_uint32(blob, encoded.u32);
      case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE:
      encoded.strct.length = MIN2(type->length, 0xfffff);
   encoded.strct.explicit_alignment =
         if (glsl_type_is_interface(type)) {
      encoded.strct.interface_packing_or_packed = type->interface_packing;
      } else {
         }
   blob_write_uint32(blob, encoded.u32);
            /* If we don't have enough bits for length, store it separately. */
   if (encoded.strct.length == 0xfffff)
         if (encoded.strct.explicit_alignment == 0xf)
            for (unsigned i = 0; i < type->length; i++)
            case GLSL_TYPE_VOID:
         case GLSL_TYPE_ERROR:
   default:
      assert(!"Cannot encode type!");
   encoded.u32 = 0;
                  }
      const struct glsl_type *
   decode_type_from_blob(struct blob_reader *blob)
   {
      union packed_type encoded;
            if (encoded.u32 == 0) {
                           switch (base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
   case GLSL_TYPE_BOOL: {
      unsigned explicit_stride = encoded.basic.explicit_stride;
   if (explicit_stride == 0xffff)
         unsigned explicit_alignment = encoded.basic.explicit_alignment;
   if (explicit_alignment == 0xf)
         else if (explicit_alignment > 0)
         uint32_t vector_elements = encoded.basic.vector_elements;
   if (vector_elements == 6)
         else if (vector_elements == 7)
         return glsl_simple_explicit_type(base_type, vector_elements,
                        }
   case GLSL_TYPE_SAMPLER:
      return glsl_sampler_type((enum glsl_sampler_dim)encoded.sampler.dimensionality,
                  case GLSL_TYPE_TEXTURE:
      return glsl_texture_type((enum glsl_sampler_dim)encoded.sampler.dimensionality,
            case GLSL_TYPE_SUBROUTINE:
         case GLSL_TYPE_IMAGE:
      return glsl_image_type((enum glsl_sampler_dim)encoded.sampler.dimensionality,
            case GLSL_TYPE_ATOMIC_UINT:
         case GLSL_TYPE_ARRAY: {
      unsigned length = encoded.array.length;
   if (length == 0x1fff)
         unsigned explicit_stride = encoded.array.explicit_stride;
   if (explicit_stride == 0x3fff)
         return glsl_array_type(decode_type_from_blob(blob), length,
      }
   case GLSL_TYPE_COOPERATIVE_MATRIX: {
         }
   case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE: {
      char *name = blob_read_string(blob);
   unsigned num_fields = encoded.strct.length;
   if (num_fields == 0xfffff)
         unsigned explicit_alignment = encoded.strct.explicit_alignment;
   if (explicit_alignment == 0xf)
         else if (explicit_alignment > 0)
            struct glsl_struct_field *fields = (struct glsl_struct_field *)
         for (unsigned i = 0; i < num_fields; i++)
            const struct glsl_type *t;
   if (base_type == GLSL_TYPE_INTERFACE) {
      assert(explicit_alignment == 0);
   enum glsl_interface_packing packing =
         bool row_major = encoded.strct.interface_row_major;
      } else {
      unsigned packed = encoded.strct.interface_packing_or_packed;
   t = glsl_struct_type_with_explicit_alignment(fields, num_fields,
                     free(fields);
      }
   case GLSL_TYPE_VOID:
         case GLSL_TYPE_ERROR:
   default:
      assert(!"Cannot decode type!");
         }
      unsigned
   glsl_get_cl_alignment(const struct glsl_type *t)
   {
      /* vectors unlike arrays are aligned to their size */
   if (glsl_type_is_scalar(t) || glsl_type_is_vector(t))
         else if (glsl_type_is_array(t))
         else if (glsl_type_is_struct(t)) {
      /* Packed Structs are 0x1 aligned despite their size. */
   if (t->packed)
            unsigned res = 1;
   for (unsigned i = 0; i < t->length; ++i) {
      const struct glsl_struct_field *field = &t->fields.structure[i];
      }
      }
      }
      unsigned
   glsl_get_cl_size(const struct glsl_type *t)
   {
      if (glsl_type_is_scalar(t) || glsl_type_is_vector(t)) {
      return util_next_power_of_two(t->vector_elements) *
      } else if (glsl_type_is_array(t)) {
      unsigned size = glsl_get_cl_size(t->fields.array);
      } else if (glsl_type_is_struct(t)) {
      unsigned size = 0;
   unsigned max_alignment = 1;
   for (unsigned i = 0; i < t->length; ++i) {
      const struct glsl_struct_field *field = &t->fields.structure[i];
   /* if a struct is packed, members don't get aligned */
   if (!t->packed) {
      unsigned alignment = glsl_get_cl_alignment(field->type);
   max_alignment = MAX2(max_alignment, alignment);
      }
               /* Size of C structs are aligned to the biggest alignment of its fields */
   size = align(size, max_alignment);
      }
      }
      extern const char glsl_type_builtin_names[];
      const char *
   glsl_get_type_name(const struct glsl_type *type)
   {
      if (type->has_builtin_name) {
         } else {
            }
      void
   glsl_get_cl_type_size_align(const struct glsl_type *t,
         {
      *size = glsl_get_cl_size(t);
      }
      int
   glsl_get_sampler_dim_coordinate_components(enum glsl_sampler_dim dim)
   {
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
   case GLSL_SAMPLER_DIM_BUF:
         case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_RECT:
   case GLSL_SAMPLER_DIM_MS:
   case GLSL_SAMPLER_DIM_EXTERNAL:
   case GLSL_SAMPLER_DIM_SUBPASS:
   case GLSL_SAMPLER_DIM_SUBPASS_MS:
         case GLSL_SAMPLER_DIM_3D:
   case GLSL_SAMPLER_DIM_CUBE:
         default:
            }
      bool
   glsl_type_is_vector(const struct glsl_type *t)
   {
      return t->vector_elements > 1 &&
         t->matrix_columns == 1 &&
      }
      bool
   glsl_type_is_scalar(const struct glsl_type *t)
   {
      return t->vector_elements == 1 &&
            }
      bool
   glsl_type_is_vector_or_scalar(const struct glsl_type *t)
   {
         }
      bool
   glsl_type_is_matrix(const struct glsl_type *t)
   {
      /* GLSL only has float matrices. */
   return t->matrix_columns > 1 && (t->base_type == GLSL_TYPE_FLOAT ||
            }
      bool
   glsl_type_is_array_or_matrix(const struct glsl_type *t)
   {
         }
      bool
   glsl_type_is_dual_slot(const struct glsl_type *t)
   {
         }
      const struct glsl_type *
   glsl_get_array_element(const struct glsl_type *t)
   {
      if (glsl_type_is_matrix(t))
         else if (glsl_type_is_vector(t))
            }
      bool
   glsl_type_is_leaf(const struct glsl_type *t)
   {
      if (glsl_type_is_struct_or_ifc(t) ||
      (glsl_type_is_array(t) &&
   (glsl_type_is_array(glsl_get_array_element(t)) ||
            } else {
            }
      bool
   glsl_contains_atomic(const struct glsl_type *t)
   {
         }
      const struct glsl_type *
   glsl_without_array(const struct glsl_type *t)
   {
      while (glsl_type_is_array(t))
            }
      const struct glsl_type *
   glsl_without_array_or_matrix(const struct glsl_type *t)
   {
      t = glsl_without_array(t);
   if (glsl_type_is_matrix(t))
            }
      const struct glsl_type *
   glsl_type_wrap_in_arrays(const struct glsl_type *t,
         {
      if (!glsl_type_is_array(arrays))
            const struct glsl_type *elem_type =
         return glsl_array_type(elem_type, glsl_get_length(arrays),
      }
      const struct glsl_type *
   glsl_get_cmat_element(const struct glsl_type *t)
   {
      assert(t->base_type == GLSL_TYPE_COOPERATIVE_MATRIX);
      }
      const struct glsl_cmat_description *
   glsl_get_cmat_description(const struct glsl_type *t)
   {
      assert(t->base_type == GLSL_TYPE_COOPERATIVE_MATRIX);
      }
      unsigned
   glsl_get_length(const struct glsl_type *t)
   {
         }
      unsigned
   glsl_get_aoa_size(const struct glsl_type *t)
   {
      if (!glsl_type_is_array(t))
            unsigned size = t->length;
            while (glsl_type_is_array(array_base_type)) {
      size = size * array_base_type->length;
      }
      }
      const struct glsl_type *
   glsl_get_struct_field(const struct glsl_type *t, unsigned index)
   {
      assert(glsl_type_is_struct(t) || glsl_type_is_interface(t));
   assert(index < t->length);
      }
      const struct glsl_struct_field *
   glsl_get_struct_field_data(const struct glsl_type *t, unsigned index)
   {
      assert(glsl_type_is_struct(t) || glsl_type_is_interface(t));
   assert(index < t->length);
      }
      enum glsl_interface_packing
   glsl_get_internal_ifc_packing(const struct glsl_type *t,
         {
      enum glsl_interface_packing packing = glsl_get_ifc_packing(t);
   if (packing == GLSL_INTERFACE_PACKING_STD140 ||
      (!std430_supported &&
   (packing == GLSL_INTERFACE_PACKING_SHARED ||
            } else {
      assert(packing == GLSL_INTERFACE_PACKING_STD430 ||
         (std430_supported &&
   (packing == GLSL_INTERFACE_PACKING_SHARED ||
         }
      const struct glsl_type *
   glsl_get_row_type(const struct glsl_type *t)
   {
      if (!glsl_type_is_matrix(t))
            if (t->explicit_stride && !t->interface_row_major)
      return glsl_simple_explicit_type(t->base_type, t->matrix_columns, 1,
      else
      }
      const struct glsl_type *
   glsl_get_column_type(const struct glsl_type *t)
   {
      if (!glsl_type_is_matrix(t))
            if (t->interface_row_major) {
      /* If we're row-major, the vector element stride is the same as the
   * matrix stride and we have no alignment (i.e. component-aligned).
   */
   return glsl_simple_explicit_type(t->base_type, t->vector_elements, 1,
      } else {
      /* Otherwise, the vector is tightly packed (stride=0).  For
   * alignment, we treat a matrix as an array of columns make the same
   * assumption that the alignment of the column is the same as the
   * alignment of the whole matrix.
   */
   return glsl_simple_explicit_type(t->base_type, t->vector_elements, 1, 0,
         }
      unsigned
   glsl_atomic_size(const struct glsl_type *t)
   {
      if (glsl_type_is_atomic_uint(t))
         else if (glsl_type_is_array(t))
         else
      }
      const struct glsl_type *
   glsl_type_to_16bit(const struct glsl_type *old_type)
   {
      if (glsl_type_is_array(old_type)) {
      return glsl_array_type(glsl_type_to_16bit(glsl_get_array_element(old_type)),
                     if (glsl_type_is_vector_or_scalar(old_type)) {
      switch (glsl_get_base_type(old_type)) {
   case GLSL_TYPE_FLOAT:
         case GLSL_TYPE_UINT:
         case GLSL_TYPE_INT:
         default:
                        }
      const struct glsl_type *
   glsl_replace_vector_type(const struct glsl_type *t, unsigned components)
   {
      if (glsl_type_is_array(t)) {
      return glsl_array_type(
      glsl_replace_vector_type(t->fields.array, components), t->length,
   } else if (glsl_type_is_vector_or_scalar(t)) {
         } else {
            }
      const struct glsl_type *
   glsl_channel_type(const struct glsl_type *t)
   {
      switch (t->base_type) {
   case GLSL_TYPE_ARRAY:
      return glsl_array_type(glsl_channel_type(t->fields.array), t->length,
      case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
   case GLSL_TYPE_BOOL:
         default:
            }
      static void
   glsl_size_align_handle_array_and_structs(const struct glsl_type *type,
               {
      if (type->base_type == GLSL_TYPE_ARRAY) {
      unsigned elem_size = 0, elem_align = 0;
   size_align(type->fields.array, &elem_size, &elem_align);
   *align = elem_align;
      } else {
      assert(type->base_type == GLSL_TYPE_STRUCT ||
            *size = 0;
   *align = 0;
   for (unsigned i = 0; i < type->length; i++) {
      unsigned elem_size = 0, elem_align = 0;
   size_align(type->fields.structure[i].type, &elem_size, &elem_align);
   *align = MAX2(*align, elem_align);
            }
      void
   glsl_get_natural_size_align_bytes(const struct glsl_type *type,
         {
      switch (type->base_type) {
   case GLSL_TYPE_BOOL:
      /* We special-case Booleans to 32 bits to not cause heartburn for
   * drivers that suddenly get an 8-bit load.
   */
   *size = 4 * glsl_get_components(type);
   *align = 4;
         case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64: {
      unsigned N = glsl_get_bit_size(type) / 8;
   *size = N * glsl_get_components(type);
   *align = N;
               case GLSL_TYPE_ARRAY:
   case GLSL_TYPE_INTERFACE:
   case GLSL_TYPE_STRUCT:
      glsl_size_align_handle_array_and_structs(type,
                     case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
      /* Bindless samplers and images. */
   *size = 8;
   *align = 8;
         case GLSL_TYPE_COOPERATIVE_MATRIX:
   case GLSL_TYPE_ATOMIC_UINT:
   case GLSL_TYPE_SUBROUTINE:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
            }
      /**
   * Returns a byte size/alignment for a type where each array element or struct
   * field is aligned to 16 bytes.
   */
   void
   glsl_get_vec4_size_align_bytes(const struct glsl_type *type,
         {
      switch (type->base_type) {
   case GLSL_TYPE_BOOL:
      /* We special-case Booleans to 32 bits to not cause heartburn for
   * drivers that suddenly get an 8-bit load.
   */
   *size = 4 * glsl_get_components(type);
   *align = 16;
         case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64: {
      unsigned N = glsl_get_bit_size(type) / 8;
   *size = 16 * (type->matrix_columns - 1) + N * type->vector_elements;
   *align = 16;
               case GLSL_TYPE_ARRAY:
   case GLSL_TYPE_INTERFACE:
   case GLSL_TYPE_STRUCT:
      glsl_size_align_handle_array_and_structs(type,
                     case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
   case GLSL_TYPE_COOPERATIVE_MATRIX:
   case GLSL_TYPE_ATOMIC_UINT:
   case GLSL_TYPE_SUBROUTINE:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
            }
      static unsigned
   glsl_type_count(const struct glsl_type *type, enum glsl_base_type base_type)
   {
      if (glsl_type_is_array(type)) {
      return glsl_get_length(type) *
               /* Ignore interface blocks - they can only contain bindless samplers,
   * which we shouldn't count.
   */
   if (glsl_type_is_struct(type)) {
      unsigned count = 0;
   for (unsigned i = 0; i < glsl_get_length(type); i++)
                     if (glsl_get_base_type(type) == base_type)
               }
      unsigned
   glsl_type_get_sampler_count(const struct glsl_type *type)
   {
         }
      unsigned
   glsl_type_get_texture_count(const struct glsl_type *type)
   {
         }
      unsigned
   glsl_type_get_image_count(const struct glsl_type *type)
   {
         }
