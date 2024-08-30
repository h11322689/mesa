   #include "CL/cl.h"
      #include "nir.h"
   #include "nir_builder.h"
      #include "rusticl_nir.h"
      static bool
   rusticl_lower_intrinsics_filter(const nir_instr* instr, const void* state)
   {
         }
      static nir_def*
   rusticl_lower_intrinsics_instr(
      nir_builder *b,
   nir_instr *instr,
      ) {
      nir_intrinsic_instr *intrins = nir_instr_as_intrinsic(instr);
            switch (intrins->intrinsic) {
   case nir_intrinsic_image_deref_format:
   case nir_intrinsic_image_deref_order: {
      int32_t offset;
   nir_deref_instr *deref;
   nir_def *val;
            if (intrins->intrinsic == nir_intrinsic_image_deref_format) {
         offset = CL_SNORM_INT8;
   } else {
         offset = CL_R;
                     if (val->parent_instr->type == nir_instr_type_deref) {
         nir_deref_instr *deref = nir_instr_as_deref(val->parent_instr);
   nir_variable *var = nir_deref_instr_get_variable(deref);
   assert(var);
            // we put write images after read images
   if (glsl_type_is_image(var->type)) {
                  deref = nir_build_deref_var(b, var);
   deref = nir_build_deref_array(b, deref, val);
            // we have to fix up the value base
               }
   case nir_intrinsic_load_global_invocation_id_zero_base:
      if (intrins->def.bit_size == 64)
            case nir_intrinsic_load_base_global_invocation_id:
         case nir_intrinsic_load_constant_base_ptr:
         case nir_intrinsic_load_printf_buffer_address:
         case nir_intrinsic_load_work_dim:
      assert(nir_find_variable_with_location(b->shader, nir_var_uniform, state->work_dim_loc));
   return nir_u2uN(b, nir_load_var(b, nir_find_variable_with_location(b->shader, nir_var_uniform, state->work_dim_loc)),
      default:
            }
      bool
   rusticl_lower_intrinsics(nir_shader *nir, struct rusticl_lower_state* state)
   {
      return nir_shader_lower_instructions(
      nir,
   rusticl_lower_intrinsics_filter,
   rusticl_lower_intrinsics_instr,
         }
      static nir_def*
   rusticl_lower_input_instr(struct nir_builder *b, nir_instr *instr, void *_)
   {
      nir_intrinsic_instr *intrins = nir_instr_as_intrinsic(instr);
   if (intrins->intrinsic != nir_intrinsic_load_kernel_input)
            nir_def *ubo_idx = nir_imm_int(b, 0);
            assert(intrins->def.bit_size >= 8);
   nir_def *load_result =
      nir_load_ubo(b, intrins->num_components, intrins->def.bit_size,
                  nir_intrinsic_set_align_mul(load, nir_intrinsic_align_mul(intrins));
   nir_intrinsic_set_align_offset(load, nir_intrinsic_align_offset(intrins));
   nir_intrinsic_set_range_base(load, nir_intrinsic_base(intrins));
               }
      bool
   rusticl_lower_inputs(nir_shader *shader)
   {
                        progress = nir_shader_lower_instructions(
      shader,
   rusticl_lower_intrinsics_filter,
   rusticl_lower_input_instr,
               nir_foreach_variable_with_modes(var, shader, nir_var_mem_ubo) {
      var->data.binding++;
      }
            if (shader->num_uniforms > 0) {
      const struct glsl_type *type = glsl_array_type(glsl_uint8_t_type(), shader->num_uniforms, 1);
   nir_variable *ubo = nir_variable_create(shader, nir_var_mem_ubo, type, "kernel_input");
   ubo->data.binding = 0;
               shader->info.first_ubo_is_default_ubo = true;
      }
