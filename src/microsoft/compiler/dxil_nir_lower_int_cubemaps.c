   /*
   * Copyright © Microsoft Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "dxil_nir_lower_int_cubemaps.h"
      #include "nir_builder.h"
   #include "nir_builtin_builder.h"
      static bool
   type_needs_lowering(const struct glsl_type *type, bool lower_samplers)
   {
      type = glsl_without_array(type);
   if (!glsl_type_is_image(type) && !glsl_type_is_sampler(type))
         if (glsl_get_sampler_dim(type) != GLSL_SAMPLER_DIM_CUBE)
         if (glsl_type_is_image(type))
            }
      static bool
   lower_int_cubmap_to_array_filter(const nir_instr *instr,
         {
      bool lower_samplers = *(bool *)options;
   if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_size:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_store:
         default:
            } else if (instr->type == nir_instr_type_deref) {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
      } else if (instr->type == nir_instr_type_tex && lower_samplers) {
               if (tex->sampler_dim != GLSL_SAMPLER_DIM_CUBE)
            switch (tex->op) {
   case nir_texop_tex:
   case nir_texop_txb:
   case nir_texop_txd:
   case nir_texop_txl:
   case nir_texop_txs:
   case nir_texop_lod:
   case nir_texop_tg4:
         default:
                  int sampler_deref = nir_tex_instr_src_index(tex, nir_tex_src_sampler_deref);
   assert(sampler_deref >= 0);
   nir_deref_instr *deref = nir_instr_as_deref(tex->src[sampler_deref].src.ssa->parent_instr);
   nir_variable *cube = nir_deref_instr_get_variable(deref);
                  }
      typedef struct {
      bool image;
   nir_def *rx;
   nir_def *ry;
   nir_def *rz;
   nir_def *arx;
   nir_def *ary;
   nir_def *arz;
      } coord_t;
         /* This is taken from from sp_tex_sample:convert_cube */
   static nir_def *
   evaluate_face_x(nir_builder *b, coord_t *coord)
   {
      nir_def *sign = nir_fsign(b, coord->rx);
   nir_def *positive = nir_fge_imm(b, coord->rx, 0.0);
            nir_def *x = nir_fadd_imm(b, nir_fmul(b, nir_fmul(b, sign, ima), coord->rz), 0.5);
   nir_def *y = nir_fadd_imm(b, nir_fmul(b, ima, coord->ry), 0.5);
            if (coord->array)
            return coord->image ?
      nir_vec4(b, x,y, face, nir_undef(b, 1, 32)) :
   }
      static nir_def *
   evaluate_face_y(nir_builder *b, coord_t *coord)
   {
      nir_def *sign = nir_fsign(b, coord->ry);
   nir_def *positive = nir_fge_imm(b, coord->ry, 0.0);
            nir_def *x = nir_fadd_imm(b, nir_fmul(b, ima, coord->rx), 0.5);
   nir_def *y = nir_fadd_imm(b, nir_fmul(b, nir_fmul(b, sign, ima), coord->rz), 0.5);
            if (coord->array)
            return coord->image ?
      nir_vec4(b, x,y, face, nir_undef(b, 1, 32)) :
   }
      static nir_def *
   evaluate_face_z(nir_builder *b, coord_t *coord)
   {
      nir_def *sign = nir_fsign(b, coord->rz);
   nir_def *positive = nir_fge_imm(b, coord->rz, 0.0);
            nir_def *x = nir_fadd_imm(b, nir_fmul(b, nir_fmul(b, sign, ima), nir_fneg(b, coord->rx)), 0.5);
   nir_def *y = nir_fadd_imm(b, nir_fmul(b, ima, coord->ry), 0.5);
            if (coord->array)
            return coord->image ?
      nir_vec4(b, x,y, face, nir_undef(b, 1, 32)) :
   }
      static nir_def *
   create_array_tex_from_cube_tex(nir_builder *b, nir_tex_instr *tex, nir_def *coord, nir_texop op)
   {
               array_tex = nir_tex_instr_create(b->shader, tex->num_srcs);
   array_tex->op = op;
   array_tex->sampler_dim = GLSL_SAMPLER_DIM_2D;
   array_tex->is_array = true;
   array_tex->is_shadow = tex->is_shadow;
   array_tex->is_new_style_shadow = tex->is_new_style_shadow;
   array_tex->texture_index = tex->texture_index;
   array_tex->sampler_index = tex->sampler_index;
   array_tex->dest_type = tex->dest_type;
            nir_src coord_src = nir_src_for_ssa(coord);
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      nir_src *psrc = (tex->src[i].src_type == nir_tex_src_coord) ?
            array_tex->src[i].src = nir_src_for_ssa(psrc->ssa);
               nir_def_init(&array_tex->instr, &array_tex->def,
         nir_builder_instr_insert(b, &array_tex->instr);
      }
      static nir_def *
   handle_cube_edge(nir_builder *b, nir_def *x, nir_def *y, nir_def *face, nir_def *array_slice_cube_base, nir_def *tex_size)
   {
      enum cube_remap
   {
      cube_remap_zero = 0,
   cube_remap_x,
   cube_remap_y,
   cube_remap_tex_size,
   cube_remap_tex_size_minus_x,
                        struct cube_remap_table
   {
      enum cube_remap remap_x;
   enum cube_remap remap_y;
               static const struct cube_remap_table cube_remap_neg_x[6] =
   {
      {cube_remap_tex_size,         cube_remap_y,         4},
   {cube_remap_tex_size,         cube_remap_y,         5},
   {cube_remap_y,                cube_remap_zero,      1},
   {cube_remap_tex_size_minus_y, cube_remap_tex_size,  1},
   {cube_remap_tex_size,         cube_remap_y,         1},
               static const struct cube_remap_table cube_remap_pos_x[6] =
   {
      {cube_remap_zero,             cube_remap_y,         5},
   {cube_remap_zero,             cube_remap_y,         4},
   {cube_remap_tex_size_minus_y, cube_remap_zero,      0},
   {cube_remap_y,                cube_remap_tex_size,  0},
   {cube_remap_zero,             cube_remap_y,         0},
               static const struct cube_remap_table cube_remap_neg_y[6] =
   {
      {cube_remap_tex_size,         cube_remap_tex_size_minus_x, 2},
   {cube_remap_zero,             cube_remap_x,                2},
   {cube_remap_tex_size_minus_x, cube_remap_zero,             5},
   {cube_remap_x,                cube_remap_tex_size,         4},
   {cube_remap_x,                cube_remap_tex_size,         2},
               static const struct cube_remap_table cube_remap_pos_y[6] =
   {
      {cube_remap_tex_size,         cube_remap_x,                   3},
   {cube_remap_zero,             cube_remap_tex_size_minus_x,    3},
   {cube_remap_x,                cube_remap_zero,                4},
   {cube_remap_tex_size_minus_x, cube_remap_tex_size,            5},
   {cube_remap_x,                cube_remap_zero,                3},
               static const struct cube_remap_table* remap_tables[4] = {
      cube_remap_neg_x,
   cube_remap_pos_x,
   cube_remap_neg_y,
               nir_def *zero = nir_imm_int(b, 0);
      /* Doesn't matter since the texture is square */
            nir_def *x_on = nir_iand(b, nir_ige(b, x, zero), nir_ige(b, tex_size, x));
   nir_def *y_on = nir_iand(b, nir_ige(b, y, zero), nir_ige(b, tex_size, y));
            /* If the sample did not fall off the face in either dimension, then set output = input */
   nir_def *x_result = x;
   nir_def *y_result = y;
            /* otherwise, if the sample fell off the face in either the X or the Y direction, remap to the new face */
   nir_def *remap_predicates[4] =
   {
      nir_iand(b, one_on, nir_ilt(b, x, zero)),
   nir_iand(b, one_on, nir_ilt(b, tex_size, x)),
   nir_iand(b, one_on, nir_ilt(b, y, zero)),
                        remap_array[cube_remap_zero] = zero;
   remap_array[cube_remap_x] = x;
   remap_array[cube_remap_y] = y;
   remap_array[cube_remap_tex_size] = tex_size;
   remap_array[cube_remap_tex_size_minus_x] = nir_isub(b, tex_size, x);
            /* For each possible way the sample could have fallen off */
   for (unsigned i = 0; i < 4; i++) {
               /* For each possible original face */
   for (unsigned j = 0; j < 6; j++) {
               x_result = nir_bcsel(b, predicate, remap_array[remap_table[j].remap_x], x_result);
   y_result = nir_bcsel(b, predicate, remap_array[remap_table[j].remap_y], y_result);
                     }
      static nir_def *
   handle_cube_gather(nir_builder *b, nir_tex_instr *tex, nir_def *coord)
   {
               /* nir_get_texture_size puts the cursor before the tex op */
            nir_def *const_05 = nir_imm_float(b, 0.5f);
   nir_def *texel_coords = nir_fmul(b, nir_trim_vector(b, coord, 2),
            nir_def *x_orig = nir_channel(b, texel_coords, 0);
            nir_def *x_pos = nir_f2i32(b, nir_fadd(b, x_orig, const_05));
   nir_def *x_neg = nir_f2i32(b, nir_fsub(b, x_orig, const_05));
   nir_def *y_pos = nir_f2i32(b, nir_fadd(b, y_orig, const_05));
   nir_def *y_neg = nir_f2i32(b, nir_fsub(b, y_orig, const_05));
   nir_def *coords[4][2] = {
      { x_neg, y_pos },
   { x_pos, y_pos },
   { x_pos, y_neg },
               nir_def *array_slice_2d = nir_f2i32(b, nir_channel(b, coord, 2));
   nir_def *face = nir_imod_imm(b, array_slice_2d, 6);
            nir_def *channels[4];
   for (unsigned i = 0; i < 4; ++i) {
      nir_def *final_coord = handle_cube_edge(b, coords[i][0], coords[i][1], face, array_slice_cube_base, tex_size);
   nir_def *sampled_val = create_array_tex_from_cube_tex(b, tex, final_coord, nir_texop_txf);
                  }
      static nir_def *
   lower_cube_coords(nir_builder *b, nir_def *coord, bool is_array, bool is_image)
   {
      coord_t coords;
   coords.image = is_image;
   coords.rx = nir_channel(b, coord, 0);
   coords.ry = nir_channel(b, coord, 1);
   coords.rz = nir_channel(b, coord, 2);
   coords.arx = nir_fabs(b, coords.rx);
   coords.ary = nir_fabs(b, coords.ry);
   coords.arz = nir_fabs(b, coords.rz);
   coords.array = NULL;
   if (is_array)
            nir_def *use_face_x = nir_iand(b,
                  nir_if *use_face_x_if = nir_push_if(b, use_face_x);
   nir_def *face_x_coord = evaluate_face_x(b, &coords);
            nir_def *use_face_y = nir_iand(b,
                  nir_if *use_face_y_if = nir_push_if(b, use_face_y);
   nir_def *face_y_coord = evaluate_face_y(b, &coords);
                     nir_pop_if(b, use_face_y_else);
   nir_def *face_y_or_z_coord = nir_if_phi(b, face_y_coord, face_z_coord);
            // This contains in xy the normalized sample coordinates, and in z the face index
               }
      static nir_def *
   lower_cube_sample(nir_builder *b, nir_tex_instr *tex)
   {
      int coord_index = nir_tex_instr_src_index(tex, nir_tex_src_coord);
            /* Evaluate the face and the xy coordinates for a 2D tex op */
   nir_def *coord = tex->src[coord_index].src.ssa;
            if (tex->op == nir_texop_tg4)
         else
      }
      static nir_def *
   lower_cube_image_load_store_atomic(nir_builder *b, nir_intrinsic_instr *intr)
   {
      b->cursor = nir_before_instr(&intr->instr);
   nir_intrinsic_set_image_array(intr, true);
               }
      static nir_def *
   lower_cube_txs(nir_builder *b, nir_tex_instr *tex)
   {
      b->cursor = nir_after_instr(&tex->instr);
   if (!tex->is_array)
            nir_def *array_dim = nir_channel(b, &tex->def, 2);
   nir_def *cube_array_dim = nir_idiv(b, array_dim, nir_imm_int(b, 6));
   return nir_vec3(b, nir_channel(b, &tex->def, 0),
            }
      static nir_def *
   lower_cube_image_size(nir_builder *b, nir_intrinsic_instr *intr)
   {
      b->cursor = nir_after_instr(&intr->instr);
   if (!nir_intrinsic_image_array(intr))
            nir_def *array_dim = nir_channel(b, &intr->def, 2);
   nir_def *cube_array_dim = nir_idiv(b, array_dim, nir_imm_int(b, 6));
   return nir_vec3(b, nir_channel(b, &intr->def, 0),
            }
      static const struct glsl_type *
   make_2darray_sampler_from_cubemap(const struct glsl_type *type)
   {
      return  glsl_get_sampler_dim(type) == GLSL_SAMPLER_DIM_CUBE ?
            glsl_sampler_type(
         }
      static const struct glsl_type *
   make_2darray_image_from_cubemap(const struct glsl_type *type)
   {
      return  glsl_get_sampler_dim(type) == GLSL_SAMPLER_DIM_CUBE ?
            glsl_image_type(
         }
      static const struct glsl_type *
   make_2darray_from_cubemap_with_array(const struct glsl_type *type, bool is_image)
   {
      if (glsl_type_is_array(type)) {
      const struct glsl_type *new_type = glsl_without_array(type);
   return new_type != type ? glsl_array_type(make_2darray_from_cubemap_with_array(glsl_without_array(type), is_image),
      } else if (is_image)
         else
      }
      static nir_def *
   lower_int_cubemap_to_array_tex(nir_builder *b, nir_tex_instr *tex)
   {
      switch (tex->op) {
   case nir_texop_tex:
   case nir_texop_txb:
   case nir_texop_txd:
   case nir_texop_txl:
   case nir_texop_lod:
   case nir_texop_tg4:
         case nir_texop_txs:
         default:
            }
      static nir_def *
   lower_cube_image_intrinsic(nir_builder *b, nir_intrinsic_instr *intr)
   {
      if (intr->intrinsic == nir_intrinsic_image_size ||
      intr->intrinsic == nir_intrinsic_image_deref_size)
      else
      }
      static nir_def *
   lower_cube_image_deref(nir_builder *b, nir_deref_instr *deref)
   {
      deref->type = make_2darray_from_cubemap_with_array(
      deref->type,
         }
      static nir_def *
   lower_int_cubmap_to_array_impl(nir_builder *b, nir_instr *instr,
         {
      bool lower_samplers = *(bool *)options;
   if (instr->type == nir_instr_type_tex && lower_samplers)
         else if (instr->type == nir_instr_type_intrinsic)
         else if (instr->type == nir_instr_type_deref)
            }
      bool
   dxil_nir_lower_int_cubemaps(nir_shader *s, bool lower_samplers)
   {
      bool result =
         nir_shader_lower_instructions(s,
                  if (result) {
      nir_foreach_variable_with_modes_safe(var, s, nir_var_uniform | nir_var_image) {
      if (!type_needs_lowering(var->type, lower_samplers))
         bool is_image = glsl_type_is_image(glsl_without_array(var->type));
         }
         }
