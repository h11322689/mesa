   /*
   * Copyright 2022 Alyssa Rosenzweig
   * Copyright 2021 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir_builder.h"
   #include "agx_compiler.h"
      static void
   def_size(nir_def *def, unsigned *size, unsigned *align)
   {
               *size = (bit_size * def->num_components) / 16;
      }
      static float
   instr_cost(nir_instr *instr, const void *data)
   {
      switch (instr->type) {
   case nir_instr_type_intrinsic:
      switch (nir_instr_as_intrinsic(instr)->intrinsic) {
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_agx:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_constant_agx:
   case nir_intrinsic_load_ubo:
         default:
      /* Assume it's a sysval or something */
            case nir_instr_type_tex:
      /* Texturing involes lots of memory bandwidth */
         case nir_instr_type_alu:
      /* We optimistically assume that moves get coalesced */
   if (nir_op_is_vec_or_mov(nir_instr_as_alu(instr)->op))
         else
         default:
            }
      static float
   rewrite_cost(nir_def *def, const void *data)
   {
      bool mov_needed = false;
   nir_foreach_use(use, def) {
      nir_instr *parent_instr = nir_src_parent_instr(use);
   if (parent_instr->type != nir_instr_type_alu) {
      mov_needed = true;
      } else {
      nir_alu_instr *alu = nir_instr_as_alu(parent_instr);
   if (alu->op == nir_op_vec2 || alu->op == nir_op_vec3 ||
      alu->op == nir_op_vec4 || alu->op == nir_op_mov) {
   mov_needed = true;
      } else {
                           }
      static bool
   avoid_instr(const nir_instr *instr, const void *data)
   {
               /* Do not move bindless handles, since we need those to retain their constant
   * base index.
   */
   if (def) {
      nir_foreach_use(use, def) {
      if (nir_src_parent_instr(use)->type == nir_instr_type_tex) {
      /* Check if used as a bindless texture handle */
   nir_tex_instr *tex = nir_instr_as_tex(nir_src_parent_instr(use));
                  if (handle_idx >= 0 && tex->src[handle_idx].src.ssa == def)
      } else if (nir_src_parent_instr(use)->type ==
            /* Check if used as a bindless image handle */
                  switch (intr->intrinsic) {
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_store:
      if (intr->src[0].ssa == def)
            default:
                              }
      static const nir_opt_preamble_options preamble_options = {
      .drawid_uniform = true,
   .subgroup_size_uniform = true,
   .def_size = def_size,
   .instr_cost_cb = instr_cost,
   .rewrite_cost_cb = rewrite_cost,
   .avoid_instr_cb = avoid_instr,
      };
      bool
   agx_nir_opt_preamble(nir_shader *nir, unsigned *preamble_size)
   {
         }
