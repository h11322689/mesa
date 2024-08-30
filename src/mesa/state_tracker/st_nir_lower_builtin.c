   /*
   * Copyright © 2016 Red Hat
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      /* Lowering pass that lowers accesses to built-in uniform variables.
   * Built-in uniforms are not necessarily packed the same way that
   * normal uniform structs are, for example:
   *
   *    struct gl_FogParameters {
   *       vec4 color;
   *       float density;
   *       float start;
   *       float end;
   *       float scale;
   *    };
   *
   * is packed into vec4[2], whereas the same struct would be packed
   * (by gallium), as vec4[5] if it where not built-in.  Because of
   * this, we need to replace (for example) access like:
   *
   *    vec1 ssa_1 = intrinsic load_var () (gl_Fog.start) ()
   *
   * with:
   *
   *    vec4 ssa_2 = intrinsic load_var () (fog.params) ()
   *    vec1 ssa_1 = ssa_2.y
   *
   * with appropriate substitutions in the uniform variables list:
   *
   *    decl_var uniform INTERP_MODE_NONE gl_FogParameters gl_Fog (0, 0)
   *
   * would become:
   *
   *    decl_var uniform INTERP_MODE_NONE vec4 state.fog.color (0, 0)
   *    decl_var uniform INTERP_MODE_NONE vec4 state.fog.params (0, 1)
   *
   * See in particular 'struct gl_builtin_uniform_element'.
   */
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_deref.h"
   #include "st_nir.h"
   #include "compiler/glsl/ir.h"
   #include "uniforms.h"
   #include "program/prog_instruction.h"
      /** Wrapper around nir_state_variable_create to pick the name automatically. */
   nir_variable *
   st_nir_state_variable_create(nir_shader *shader,
               {
      char *name = _mesa_program_state_string(tokens);
   nir_variable *var = nir_state_variable_create(shader, type, name, tokens);
   free(name);
      }
      static const struct gl_builtin_uniform_element *
   get_element(const struct gl_builtin_uniform_desc *desc, nir_deref_path *path)
   {
                        if ((desc->num_elements == 1) && (desc->elements[0].field == NULL))
            /* we handle arrays in get_variable(): */
   if (path->path[idx]->deref_type == nir_deref_type_array)
            /* don't need to deal w/ non-struct or array of non-struct: */
   if (!path->path[idx])
            if (path->path[idx]->deref_type != nir_deref_type_struct)
                        }
      static nir_variable *
   get_variable(nir_builder *b, nir_deref_path *path,
         {
      nir_shader *shader = b->shader;
   gl_state_index16 tokens[STATE_LENGTH];
                     if (path->path[idx]->deref_type == nir_deref_type_array) {
      /* we need to fixup the array index slot: */
   switch (tokens[0]) {
   case STATE_MODELVIEW_MATRIX:
   case STATE_MODELVIEW_MATRIX_INVERSE:
   case STATE_MODELVIEW_MATRIX_TRANSPOSE:
   case STATE_MODELVIEW_MATRIX_INVTRANS:
   case STATE_PROJECTION_MATRIX:
   case STATE_PROJECTION_MATRIX_INVERSE:
   case STATE_PROJECTION_MATRIX_TRANSPOSE:
   case STATE_PROJECTION_MATRIX_INVTRANS:
   case STATE_MVP_MATRIX:
   case STATE_MVP_MATRIX_INVERSE:
   case STATE_MVP_MATRIX_TRANSPOSE:
   case STATE_MVP_MATRIX_INVTRANS:
   case STATE_TEXTURE_MATRIX:
   case STATE_TEXTURE_MATRIX_INVERSE:
   case STATE_TEXTURE_MATRIX_TRANSPOSE:
   case STATE_TEXTURE_MATRIX_INVTRANS:
   case STATE_PROGRAM_MATRIX:
   case STATE_PROGRAM_MATRIX_INVERSE:
   case STATE_PROGRAM_MATRIX_TRANSPOSE:
   case STATE_PROGRAM_MATRIX_INVTRANS:
   case STATE_LIGHT:
   case STATE_LIGHTPROD:
   case STATE_TEXGEN:
   case STATE_TEXENV_COLOR:
   case STATE_CLIPPLANE:
      tokens[1] = nir_src_as_uint(path->path[idx]->arr.index);
                  nir_variable *var = nir_find_state_variable(shader, tokens);
   if (var)
            /* variable doesn't exist yet, so create it: */
      }
      static bool
   lower_builtin_instr(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic != nir_intrinsic_load_deref)
            nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   if (!nir_deref_mode_is(deref, nir_var_uniform))
            /* built-in's will always start with "gl_" */
   nir_variable *var = nir_deref_instr_get_variable(deref);
   if (strncmp(var->name, "gl_", 3) != 0)
            const struct gl_builtin_uniform_desc *desc =
            /* if no descriptor, it isn't something we need to handle specially: */
   if (!desc)
            nir_deref_path path;
                     /* matrix elements (array_deref) do not need special handling: */
   if (!element) {
      nir_deref_path_finish(&path);
               /* remove existing var from uniform list: */
   exec_node_remove(&var->node);
   /* the _self_link() ensures we can remove multiple times, rather than
   * trying to keep track of what we have already removed:
   */
            nir_variable *new_var = get_variable(b, &path, element);
                              /* swizzle the result: */
   unsigned swiz[NIR_MAX_VEC_COMPONENTS] = {0};
   for (unsigned i = 0; i < 4; i++) {
      swiz[i] = GET_SWZ(element->swizzle, i);
      }
            /* and rewrite uses of original instruction: */
            /* at this point intrin should be unused.  We need to remove it
   * (rather than waiting for DCE pass) to avoid dangling reference
   * to remove'd var.  And we have to remove the original uniform
   * var since we don't want it to get uniform space allocated.
   */
               }
      void
   st_nir_lower_builtin(nir_shader *shader)
   {
               nir_foreach_uniform_variable(var, shader) {
      /* built-in's will always start with "gl_" */
   if (strncmp(var->name, "gl_", 3) == 0)
               if (vars->entries > 0) {
      /* at this point, array uniforms have been split into separate
   * nir_variable structs where possible. this codepath can't handle
   * dynamic array indexing, however, so all indirect uniform derefs must
   * be eliminated beforehand to avoid trying to lower one of those
   * builtins
   */
            if (nir_shader_intrinsics_pass(shader, lower_builtin_instr,
                              }
