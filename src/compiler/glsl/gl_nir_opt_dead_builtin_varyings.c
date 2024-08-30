   /*
   * Copyright © 2013 Marek Olšák <maraeo@gmail.com>
   * Copyright © 2022 Valve Corporation
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
   * This eliminates the built-in shader outputs which are either not written
   * at all or not used by the next stage. It also eliminates unused elements
   * of gl_TexCoord inputs, which reduces the overall varying usage.
   * The varyings handled here are the primary and secondary color, the fog,
   * and the texture coordinates (gl_TexCoord).
   *
   * This pass is necessary, because the Mesa GLSL linker cannot eliminate
   * built-in varyings like it eliminates user-defined varyings, because
   * the built-in varyings have pre-assigned locations. Also, the elimination
   * of unused gl_TexCoord elements requires its own lowering pass anyway.
   *
   * It's implemented by replacing all occurrences of dead varyings with
   * temporary variables, which creates dead code. It is recommended to run
   * a dead-code elimination pass after this.
   *
   * If any texture coordinate slots can be eliminated, the gl_TexCoord array is
   * broken down into separate vec4 variables with locations equal to
   * VARYING_SLOT_TEX0 + i.
   */
      #include "gl_nir_link_varyings.h"
   #include "gl_nir_linker.h"
   #include "linker_util.h"
   #include "nir_builder.h"
   #include "nir_gl_types.h"
   #include "nir_types.h"
      #include "compiler/glsl_types.h"
   #include "main/consts_exts.h"
   #include "main/shader_types.h"
   #include "util/u_string.h"
      struct varying_info {
      bool lower_texcoord_array;
   nir_variable *texcoord_array;
                     nir_variable *color[2];
   nir_variable *backcolor[2];
   unsigned color_usage; /* bitmask */
            nir_variable *fog;
   bool has_fog;
               };
      static void
   initialise_varying_info(struct varying_info *info, nir_variable_mode mode,
         {
      info->find_frag_outputs = find_frag_outputs;
   info->lower_texcoord_array = true;
   info->texcoord_array = NULL;
   info->texcoord_usage = 0;
   info->color_usage = 0;
   info->tfeedback_color_usage = 0;
   info->fog = NULL;
   info->has_fog = false;
   info->tfeedback_has_fog = false;
            memset(info->color, 0, sizeof(info->color));
      }
      static void
   gather_info_on_varying_deref(struct varying_info *info, nir_deref_instr *deref)
   {
               if (!glsl_type_is_array(var->type) || !is_gl_identifier(var->name))
            if (!info->find_frag_outputs && var->data.location == VARYING_SLOT_TEX0) {
               assert(deref->deref_type == nir_deref_type_array);
   if (nir_src_is_const(deref->arr.index)) {
         } else {
      /* There is variable indexing, we can't lower the texcoord array. */
   info->texcoord_usage |= (1 << glsl_array_size(var->type)) - 1;
                     }
      /**
   * This obtains detailed information about built-in varyings from shader code.
   */
   static void
   get_varying_info(struct varying_info *info, nir_shader *shader,
         {
      /* Handle the transform feedback varyings. */
   for (unsigned i = 0; i < num_tfeedback_decls; i++) {
      if (!xfb_decl_is_varying(&tfeedback_decls[i]))
                     switch (location) {
   case VARYING_SLOT_COL0:
   case VARYING_SLOT_BFC0:
      info->tfeedback_color_usage |= 1;
      case VARYING_SLOT_COL1:
   case VARYING_SLOT_BFC1:
      info->tfeedback_color_usage |= 2;
      case VARYING_SLOT_FOGC:
      info->tfeedback_has_fog = true;
      default:
      if (location >= VARYING_SLOT_TEX0 &&
      location <= VARYING_SLOT_TEX7) {
                     /* Process frag shader vars */
   nir_foreach_variable_with_modes(var, shader, info->mode) {
      /* Nothing to do here for fragment outputs. */
   if (info->find_frag_outputs)
            /* Handle colors and fog. */
   switch (var->data.location) {
   case VARYING_SLOT_COL0:
      info->color[0] = var;
   info->color_usage |= 1;
      case VARYING_SLOT_COL1:
      info->color[1] = var;
   info->color_usage |= 2;
      case VARYING_SLOT_BFC0:
      info->backcolor[0] = var;
   info->color_usage |= 1;
      case VARYING_SLOT_BFC1:
      info->backcolor[1] = var;
   info->color_usage |= 2;
      case VARYING_SLOT_FOGC:
      info->fog = var;
   info->has_fog = true;
                  /* Process the shader. */
   assert(shader->info.stage != MESA_SHADER_COMPUTE);
            /* assert that functions have been inlined before packing is called */
   nir_foreach_function(f, shader) {
                  nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                              /* Copies should have been lowered by nir_split_var_copies() before
   * calling this pass.
                  if (intrin->intrinsic != nir_intrinsic_load_deref &&
                  nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                                 if (!info->texcoord_array) {
            }
      struct replace_varyings_data {
               struct gl_shader_program *prog;
   struct gl_linked_shader *shader;
            nir_variable *new_texcoord[MAX_TEXTURE_COORD_UNITS];
   nir_variable *new_color[2];
   nir_variable *new_backcolor[2];
      };
      static nir_variable *
   create_new_var(nir_shader *shader, char *name, nir_variable_mode mode,
         {
      nir_variable *var = rzalloc(shader, nir_variable);
   var->name = ralloc_strdup(var, name);
   var->data.mode = mode;
            nir_shader_add_variable(shader, var);
      }
      static void
   replace_varying(struct replace_varyings_data *rv_data, nir_variable *var)
   {
      /* Remove the gl_TexCoord array. */
   if (rv_data->info->lower_texcoord_array &&
      var == rv_data->info->texcoord_array) {
               /* Lower set-but-unused color and fog outputs to shader temps. */
   for (int i = 0; i < 2; i++) {
      if (var == rv_data->info->color[i] && rv_data->new_color[i]) {
                  if (var == rv_data->info->backcolor[i] && rv_data->new_backcolor[i]) {
                     if (var == rv_data->info->fog && rv_data->new_fog) {
            }
      static void
   rewrite_varying_deref(nir_builder *b, struct replace_varyings_data *rv_data,
         {
      if (deref->deref_type != nir_deref_type_array)
            nir_variable *var = nir_deref_instr_get_variable(deref);
   const struct varying_info *info = rv_data->info;
            /* Replace an array dereference gl_TexCoord[i] with a single
   * variable dereference representing gl_TexCoord[i].
   */
   if (info->lower_texcoord_array && info->texcoord_array == var) {
      /* gl_TexCoord[i] occurrence */
   unsigned i = nir_src_as_uint(deref->arr.index);
   nir_deref_instr *new_deref =
         nir_def_rewrite_uses(&deref->def, &new_deref->def);
         }
      static void
   prepare_array(struct replace_varyings_data *rv_data,
               nir_shader *shader, nir_variable **new_var,
   int max_elements, unsigned start_location,
   {
      for (int i = max_elements - 1; i >= 0; i--) {
      if (usage & (1 << i)) {
               if (!(external_usage & (1 << i))) {
      /* This varying is unused in the next stage. Declare
   * a temporary instead of an output. */
   snprintf(name, 32, "gl_%s_%s%i_dummy", mode_str, var_name, i);
   new_var[i] = create_new_var(shader, name, nir_var_shader_temp,
      } else {
      snprintf(name, 32, "gl_%s_%s%i", mode_str, var_name, i);
   new_var[i] = create_new_var(shader, name, rv_data->info->mode,
         new_var[i]->data.location = start_location + i;
               }
      /**
   * This replaces unused varyings with temporary variables.
   *
   * If "ir" is the producer, the "external" usage should come from
   * the consumer. It also works the other way around. If either one is
   * missing, set the "external" usage to a full mask.
   */
   static void
   replace_varyings(const struct gl_constants *consts,
                  struct gl_linked_shader *shader,
   struct gl_shader_program *prog,
      {
      struct replace_varyings_data rv_data;
   rv_data.shader = shader;
   rv_data.info = info;
   rv_data.consts = consts;
            memset(rv_data.new_texcoord, 0, sizeof(rv_data.new_texcoord));
   memset(rv_data.new_color, 0, sizeof(rv_data.new_color));
   memset(rv_data.new_backcolor, 0, sizeof(rv_data.new_backcolor));
                     /* Handle texcoord outputs.
   *
   * We're going to break down the gl_TexCoord array into separate
   * variables. First, add declarations of the new variables all
   * occurrences of gl_TexCoord will be replaced with.
   */
   if (info->lower_texcoord_array) {
      prepare_array(&rv_data, shader->Program->nir, rv_data.new_texcoord,
                           /* Create dummy variables which will replace set-but-unused color and
   * fog outputs.
   */
            for (int i = 0; i < 2; i++) {
               if (!(external_color_usage & (1 << i))) {
      if (info->color[i]) {
      snprintf(name, 32, "gl_%s_FrontColor%i_dummy", mode_str, i);
   rv_data.new_color[i] =
                     if (info->backcolor[i]) {
      snprintf(name, 32, "gl_%s_BackColor%i_dummy", mode_str, i);
   rv_data.new_backcolor[i] =
      create_new_var(shader->Program->nir, name, nir_var_shader_temp,
                  if (!external_has_fog && !info->tfeedback_has_fog && info->fog) {
               snprintf(name, 32, "gl_%s_FogFragCoord_dummy", mode_str);
   rv_data.new_fog =
      create_new_var(shader->Program->nir, name, nir_var_shader_temp,
            /* Now do the replacing. */
   nir_foreach_variable_with_modes_safe(var, shader->Program->nir, info->mode) {
                  nir_function_impl *impl = nir_shader_get_entrypoint(shader->Program->nir);
            /* assert that functions have been inlined before packing is called */
   nir_foreach_function(f, shader->Program->nir) {
                  /* Rewrite the derefs to use the new vars */
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                              /* Copies should have been lowered by nir_split_var_copies() before
   * calling this pass.
                  if (intrin->intrinsic != nir_intrinsic_load_deref &&
                  nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                           }
      static void
   lower_texcoord_array(const struct gl_constants *consts,
                     {
      replace_varyings(consts, shader, prog, info,
      }
      void
   gl_nir_opt_dead_builtin_varyings(const struct gl_constants *consts, gl_api api,
                                 {
      /* Lowering of built-in varyings has no effect with the core context and
   * GLES2, because they are not available there.
   */
   if (api == API_OPENGL_CORE ||
      api == API_OPENGLES2) {
               /* Information about built-in varyings. */
   struct varying_info producer_info;
   struct varying_info consumer_info;
   initialise_varying_info(&producer_info, nir_var_shader_out, false);
            if (producer) {
      get_varying_info(&producer_info, producer->Program->nir,
            if (producer->Stage == MESA_SHADER_TESS_CTRL)
            if (!consumer) {
      /* At least eliminate unused gl_TexCoord elements. */
   if (producer_info.lower_texcoord_array) {
         }
                  if (consumer) {
      get_varying_info(&consumer_info, consumer->Program->nir,
            if (consumer->Stage != MESA_SHADER_FRAGMENT)
            if (!producer) {
      /* At least eliminate unused gl_TexCoord elements. */
   if (consumer_info.lower_texcoord_array) {
         }
                  /* Eliminate the outputs unused by the consumer. */
   if (producer_info.lower_texcoord_array ||
      producer_info.color_usage ||
   producer_info.has_fog) {
   replace_varyings(consts, producer, prog, &producer_info,
                           /* The gl_TexCoord fragment shader inputs can be initialized
   * by GL_COORD_REPLACE, so we can't eliminate them.
   *
   * This doesn't prevent elimination of the gl_TexCoord elements which
   * are not read by the fragment shader. We want to eliminate those anyway.
   */
   if (consumer->Stage == MESA_SHADER_FRAGMENT) {
                  /* Eliminate the inputs uninitialized by the producer. */
   if (consumer_info.lower_texcoord_array ||
      consumer_info.color_usage ||
   consumer_info.has_fog) {
   replace_varyings(consts, consumer, prog, &consumer_info,
                        done:
      if (producer)
            if (consumer)
      }
