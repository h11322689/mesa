   /*
   * Copyright © 2010 Intel Corporation
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
      #include "compiler/glsl/ir.h"
   #include "brw_fs.h"
   #include "brw_nir.h"
   #include "brw_rt.h"
   #include "brw_eu.h"
   #include "nir.h"
   #include "nir_intrinsics.h"
   #include "nir_search_helpers.h"
   #include "util/u_math.h"
   #include "util/bitscan.h"
      #include <vector>
      using namespace brw;
      void
   fs_visitor::emit_nir_code()
   {
               /* emit the arrays used for inputs and outputs - load/store intrinsics will
   * be converted to reads/writes of these arrays
   */
   nir_setup_outputs();
   nir_setup_uniforms();
   nir_emit_system_values();
                        }
      void
   fs_visitor::nir_setup_outputs()
   {
      if (stage == MESA_SHADER_TESS_CTRL ||
      stage == MESA_SHADER_TASK ||
   stage == MESA_SHADER_MESH ||
   stage == MESA_SHADER_FRAGMENT)
                  /* Calculate the size of output registers in a separate pass, before
   * allocating them.  With ARB_enhanced_layouts, multiple output variables
   * may occupy the same slot, but have different type sizes.
   */
   nir_foreach_shader_out_variable(var, nir) {
      const int loc = var->data.driver_location;
   const unsigned var_vec4s = nir_variable_count_slots(var, var->type);
               for (unsigned loc = 0; loc < ARRAY_SIZE(vec4s);) {
      if (vec4s[loc] == 0) {
      loc++;
                        /* Check if there are any ranges that start within this range and extend
   * past it. If so, include them in this allocation.
   */
   for (unsigned i = 1; i < reg_size; i++) {
      assert(i + loc < ARRAY_SIZE(vec4s));
               fs_reg reg = bld.vgrf(BRW_REGISTER_TYPE_F, 4 * reg_size);
   for (unsigned i = 0; i < reg_size; i++) {
      assert(loc + i < ARRAY_SIZE(outputs));
                     }
      void
   fs_visitor::nir_setup_uniforms()
   {
      /* Only the first compile gets to set up uniforms. */
   if (push_constant_loc)
                     if (gl_shader_stage_is_compute(stage) && devinfo->verx10 < 125) {
      /* Add uniforms for builtins after regular NIR uniforms. */
            /* Subgroup ID must be the last uniform on the list.  This will make
   * easier later to split between cross thread and per thread
   * uniforms.
   */
   uint32_t *param = brw_stage_prog_data_add_params(prog_data, 1);
   *param = BRW_PARAM_BUILTIN_SUBGROUP_ID;
         }
      static bool
   emit_system_values_block(nir_block *block, fs_visitor *v)
   {
               nir_foreach_instr(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_base_vertex:
            case nir_intrinsic_load_vertex_id_zero_base:
   case nir_intrinsic_load_is_indexed_draw:
   case nir_intrinsic_load_first_vertex:
   case nir_intrinsic_load_instance_id:
   case nir_intrinsic_load_base_instance:
                  case nir_intrinsic_load_draw_id:
      /* For Task/Mesh, draw_id will be handled later in
   * nir_emit_mesh_task_intrinsic().
   */
   if (!gl_shader_stage_is_mesh(v->stage))
               case nir_intrinsic_load_invocation_id:
      if (v->stage == MESA_SHADER_TESS_CTRL)
         assert(v->stage == MESA_SHADER_GEOMETRY);
   reg = &v->nir_system_values[SYSTEM_VALUE_INVOCATION_ID];
   if (reg->file == BAD_FILE) {
      const fs_builder abld = v->bld.annotate("gl_InvocationID", NULL);
   fs_reg g1(retype(brw_vec8_grf(1, 0), BRW_REGISTER_TYPE_UD));
   fs_reg iid = abld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   abld.SHR(iid, g1, brw_imm_ud(27u));
                  case nir_intrinsic_load_sample_pos:
   case nir_intrinsic_load_sample_pos_or_center:
      assert(v->stage == MESA_SHADER_FRAGMENT);
   reg = &v->nir_system_values[SYSTEM_VALUE_SAMPLE_POS];
   if (reg->file == BAD_FILE)
               case nir_intrinsic_load_sample_id:
      assert(v->stage == MESA_SHADER_FRAGMENT);
   reg = &v->nir_system_values[SYSTEM_VALUE_SAMPLE_ID];
   if (reg->file == BAD_FILE)
               case nir_intrinsic_load_sample_mask_in:
      assert(v->stage == MESA_SHADER_FRAGMENT);
   assert(v->devinfo->ver >= 7);
   reg = &v->nir_system_values[SYSTEM_VALUE_SAMPLE_MASK_IN];
   if (reg->file == BAD_FILE)
               case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_workgroup_id_zero_base:
      assert(gl_shader_stage_uses_workgroup(v->stage));
   reg = &v->nir_system_values[SYSTEM_VALUE_WORKGROUP_ID];
   if (reg->file == BAD_FILE)
               case nir_intrinsic_load_helper_invocation:
      assert(v->stage == MESA_SHADER_FRAGMENT);
   reg = &v->nir_system_values[SYSTEM_VALUE_HELPER_INVOCATION];
   if (reg->file == BAD_FILE) {
                     /* On Gfx6+ (gl_HelperInvocation is only exposed on Gfx7+) the
   * pixel mask is in g1.7 of the thread payload.
   *
   * We move the per-channel pixel enable bit to the low bit of each
   * channel by shifting the byte containing the pixel mask by the
   * vector immediate 0x76543210UV.
   *
   * The region of <1,8,0> reads only 1 byte (the pixel masks for
   * subspans 0 and 1) in SIMD8 and an additional byte (the pixel
   * masks for 2 and 3) in SIMD16.
                  for (unsigned i = 0; i < DIV_ROUND_UP(v->dispatch_width, 16); i++) {
      const fs_builder hbld = abld.group(MIN2(16, v->dispatch_width), i);
   hbld.SHR(offset(shifted, hbld, i),
            stride(retype(brw_vec1_grf(1 + i, 7),
                  /* A set bit in the pixel mask means the channel is enabled, but
   * that is the opposite of gl_HelperInvocation so we need to invert
   * the mask.
   *
   * The negate source-modifier bit of logical instructions on Gfx8+
   * performs 1's complement negation, so we can use that instead of
   * a NOT instruction.
   */
   fs_reg inverted = negate(shifted);
   if (v->devinfo->ver < 8) {
                        /* We then resolve the 0/1 result to 0/~0 boolean values by ANDing
   * with 1 and negating.
   */
                  fs_reg dst = abld.vgrf(BRW_REGISTER_TYPE_D, 1);
   abld.MOV(dst, negate(retype(anded, BRW_REGISTER_TYPE_D)));
                  case nir_intrinsic_load_frag_shading_rate:
      reg = &v->nir_system_values[SYSTEM_VALUE_FRAG_SHADING_RATE];
   if (reg->file == BAD_FILE)
               default:
                        }
      void
   fs_visitor::nir_emit_system_values()
   {
      nir_system_values = ralloc_array(mem_ctx, fs_reg, SYSTEM_VALUE_MAX);
   for (unsigned i = 0; i < SYSTEM_VALUE_MAX; i++) {
                  /* Always emit SUBGROUP_INVOCATION.  Dead code will clean it up if we
   * never end up using it.
   */
   {
      const fs_builder abld = bld.annotate("gl_SubgroupInvocation", NULL);
   fs_reg &reg = nir_system_values[SYSTEM_VALUE_SUBGROUP_INVOCATION];
   reg = abld.vgrf(BRW_REGISTER_TYPE_UW);
            const fs_builder allbld8 = abld.group(8, 0).exec_all();
   allbld8.MOV(reg, brw_imm_v(0x76543210));
   if (dispatch_width > 8)
         if (dispatch_width > 16) {
      const fs_builder allbld16 = abld.group(16, 0).exec_all();
                  nir_function_impl *impl = nir_shader_get_entrypoint((nir_shader *)nir);
   nir_foreach_block(block, impl)
      }
      void
   fs_visitor::nir_emit_impl(nir_function_impl *impl)
   {
      nir_ssa_values = reralloc(mem_ctx, nir_ssa_values, fs_reg,
            nir_resource_insts = reralloc(mem_ctx, nir_resource_insts, fs_inst *,
                  nir_ssa_bind_infos = reralloc(mem_ctx, nir_ssa_bind_infos,
               memset(nir_ssa_bind_infos, 0,
            nir_resource_values = reralloc(mem_ctx, nir_resource_values, fs_reg,
               }
      void
   fs_visitor::nir_emit_cf_list(exec_list *list)
   {
      exec_list_validate(list);
   foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_if:
                  case nir_cf_node_loop:
                  case nir_cf_node_block:
                  default:
               }
      void
   fs_visitor::nir_emit_if(nir_if *if_stmt)
   {
      bool invert;
            /* If the condition has the form !other_condition, use other_condition as
   * the source, but invert the predicate on the if instruction.
   */
   nir_alu_instr *cond = nir_src_as_alu_instr(if_stmt->condition);
   if (cond != NULL && cond->op == nir_op_inot) {
      invert = true;
   cond_reg = get_nir_src(cond->src[0].src);
               (cond->instr.pass_flags & BRW_NIR_BOOLEAN_MASK) == BRW_NIR_BOOLEAN_NEEDS_RESOLVE) {
         /* redo boolean resolve on gen5 */
   fs_reg masked = bld.vgrf(BRW_REGISTER_TYPE_D);
   bld.AND(masked, cond_reg, brw_imm_d(1));
   masked.negate = true;
   fs_reg tmp = bld.vgrf(cond_reg.type);
   bld.MOV(retype(tmp, BRW_REGISTER_TYPE_D), masked);
         } else {
      invert = false;
               /* first, put the condition into f0 */
   fs_inst *inst = bld.MOV(bld.null_reg_d(),
                                    if (!nir_cf_list_is_empty_block(&if_stmt->else_list)) {
      bld.emit(BRW_OPCODE_ELSE);
                        if (devinfo->ver < 7)
      limit_dispatch_width(16, "Non-uniform control flow unsupported "
   }
      void
   fs_visitor::nir_emit_loop(nir_loop *loop)
   {
      assert(!nir_loop_has_continue_construct(loop));
                              if (devinfo->ver < 7)
      limit_dispatch_width(16, "Non-uniform control flow unsupported "
   }
      void
   fs_visitor::nir_emit_block(nir_block *block)
   {
      nir_foreach_instr(instr, block) {
            }
      void
   fs_visitor::nir_emit_instr(nir_instr *instr)
   {
               switch (instr->type) {
   case nir_instr_type_alu:
      nir_emit_alu(abld, nir_instr_as_alu(instr), true);
         case nir_instr_type_deref:
      unreachable("All derefs should've been lowered");
         case nir_instr_type_intrinsic:
      switch (stage) {
   case MESA_SHADER_VERTEX:
      nir_emit_vs_intrinsic(abld, nir_instr_as_intrinsic(instr));
      case MESA_SHADER_TESS_CTRL:
      nir_emit_tcs_intrinsic(abld, nir_instr_as_intrinsic(instr));
      case MESA_SHADER_TESS_EVAL:
      nir_emit_tes_intrinsic(abld, nir_instr_as_intrinsic(instr));
      case MESA_SHADER_GEOMETRY:
      nir_emit_gs_intrinsic(abld, nir_instr_as_intrinsic(instr));
      case MESA_SHADER_FRAGMENT:
      nir_emit_fs_intrinsic(abld, nir_instr_as_intrinsic(instr));
      case MESA_SHADER_COMPUTE:
   case MESA_SHADER_KERNEL:
      nir_emit_cs_intrinsic(abld, nir_instr_as_intrinsic(instr));
      case MESA_SHADER_RAYGEN:
   case MESA_SHADER_ANY_HIT:
   case MESA_SHADER_CLOSEST_HIT:
   case MESA_SHADER_MISS:
   case MESA_SHADER_INTERSECTION:
   case MESA_SHADER_CALLABLE:
      nir_emit_bs_intrinsic(abld, nir_instr_as_intrinsic(instr));
      case MESA_SHADER_TASK:
      nir_emit_task_intrinsic(abld, nir_instr_as_intrinsic(instr));
      case MESA_SHADER_MESH:
      nir_emit_mesh_intrinsic(abld, nir_instr_as_intrinsic(instr));
      default:
         }
         case nir_instr_type_tex:
      nir_emit_texture(abld, nir_instr_as_tex(instr));
         case nir_instr_type_load_const:
      nir_emit_load_const(abld, nir_instr_as_load_const(instr));
         case nir_instr_type_undef:
      /* We create a new VGRF for undefs on every use (by handling
   * them in get_nir_src()), rather than for each definition.
   * This helps register coalescing eliminate MOVs from undef.
   */
         case nir_instr_type_jump:
      nir_emit_jump(abld, nir_instr_as_jump(instr));
         default:
            }
      /**
   * Recognizes a parent instruction of nir_op_extract_* and changes the type to
   * match instr.
   */
   bool
   fs_visitor::optimize_extract_to_float(nir_alu_instr *instr,
         {
      if (!instr->src[0].src.ssa->parent_instr)
            if (instr->src[0].src.ssa->parent_instr->type != nir_instr_type_alu)
            nir_alu_instr *src0 =
            if (src0->op != nir_op_extract_u8 && src0->op != nir_op_extract_u16 &&
      src0->op != nir_op_extract_i8 && src0->op != nir_op_extract_i16)
                  /* Element type to extract.*/
   const brw_reg_type type = brw_int_type(
      src0->op == nir_op_extract_u16 || src0->op == nir_op_extract_i16 ? 2 : 1,
         fs_reg op0 = get_nir_src(src0->src[0].src);
   op0.type = brw_type_for_nir_type(devinfo,
      (nir_alu_type)(nir_op_infos[src0->op].input_types[0] |
               bld.MOV(result, subscript(op0, type, element));
      }
      bool
   fs_visitor::optimize_frontfacing_ternary(nir_alu_instr *instr,
         {
      nir_intrinsic_instr *src0 = nir_src_as_intrinsic(instr->src[0].src);
   if (src0 == NULL || src0->intrinsic != nir_intrinsic_load_front_face)
            if (!nir_src_is_const(instr->src[1].src) ||
      !nir_src_is_const(instr->src[2].src))
         const float value1 = nir_src_as_float(instr->src[1].src);
   const float value2 = nir_src_as_float(instr->src[2].src);
   if (fabsf(value1) != 1.0f || fabsf(value2) != 1.0f)
            /* nir_opt_algebraic should have gotten rid of bcsel(b, a, a) */
                     if (devinfo->ver >= 12) {
      /* Bit 15 of g1.1 is 0 if the polygon is front facing. */
            /* For (gl_FrontFacing ? 1.0 : -1.0), emit:
   *
   *    or(8)  tmp.1<2>W  g1.1<0,1,0>W  0x00003f80W
   *    and(8) dst<1>D    tmp<8,8,1>D   0xbf800000D
   *
   * and negate g1.1<0,1,0>W for (gl_FrontFacing ? -1.0 : 1.0).
   */
   if (value1 == -1.0f)
            bld.OR(subscript(tmp, BRW_REGISTER_TYPE_W, 1),
      } else if (devinfo->ver >= 6) {
      /* Bit 15 of g0.0 is 0 if the polygon is front facing. */
            /* For (gl_FrontFacing ? 1.0 : -1.0), emit:
   *
   *    or(8)  tmp.1<2>W  g0.0<0,1,0>W  0x00003f80W
   *    and(8) dst<1>D    tmp<8,8,1>D   0xbf800000D
   *
   * and negate g0.0<0,1,0>W for (gl_FrontFacing ? -1.0 : 1.0).
   *
   * This negation looks like it's safe in practice, because bits 0:4 will
   * surely be TRIANGLES
            if (value1 == -1.0f) {
                  bld.OR(subscript(tmp, BRW_REGISTER_TYPE_W, 1),
      } else {
      /* Bit 31 of g1.6 is 0 if the polygon is front facing. */
            /* For (gl_FrontFacing ? 1.0 : -1.0), emit:
   *
   *    or(8)  tmp<1>D  g1.6<0,1,0>D  0x3f800000D
   *    and(8) dst<1>D  tmp<8,8,1>D   0xbf800000D
   *
   * and negate g1.6<0,1,0>D for (gl_FrontFacing ? -1.0 : 1.0).
   *
   * This negation looks like it's safe in practice, because bits 0:4 will
   * surely be TRIANGLES
            if (value1 == -1.0f) {
                     }
               }
      static brw_rnd_mode
   brw_rnd_mode_from_nir_op (const nir_op op) {
      switch (op) {
   case nir_op_f2f16_rtz:
         case nir_op_f2f16_rtne:
         default:
            }
      static brw_rnd_mode
   brw_rnd_mode_from_execution_mode(unsigned execution_mode)
   {
      if (nir_has_any_rounding_mode_rtne(execution_mode))
         if (nir_has_any_rounding_mode_rtz(execution_mode))
            }
      fs_reg
   fs_visitor::prepare_alu_destination_and_sources(const fs_builder &bld,
                     {
      fs_reg result =
            result.type = brw_type_for_nir_type(devinfo,
      (nir_alu_type)(nir_op_infos[instr->op].output_type |
         for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      op[i] = get_nir_src(instr->src[i].src);
   op[i].type = brw_type_for_nir_type(devinfo,
      (nir_alu_type)(nir_op_infos[instr->op].input_types[i] |
            /* Move and vecN instrutions may still be vectored.  Return the raw,
   * vectored source and destination so that fs_visitor::nir_emit_alu can
   * handle it.  Other callers should not have to handle these kinds of
   * instructions.
   */
   switch (instr->op) {
   case nir_op_mov:
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_vec8:
   case nir_op_vec16:
         default:
                  /* At this point, we have dealt with any instruction that operates on
   * more than a single channel.  Therefore, we can just adjust the source
   * and destination registers for that channel and emit the instruction.
   */
   unsigned channel = 0;
   if (nir_op_infos[instr->op].output_size == 0) {
      /* Since NIR is doing the scalarizing for us, we should only ever see
   * vectorized operations with a single channel.
   */
   nir_component_mask_t write_mask = get_nir_write_mask(instr->def);
   assert(util_bitcount(write_mask) == 1);
                        for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      assert(nir_op_infos[instr->op].input_sizes[i] < 2);
                  }
      void
   fs_visitor::resolve_inot_sources(const fs_builder &bld, nir_alu_instr *instr,
         {
      for (unsigned i = 0; i < 2; i++) {
               if (inot_instr != NULL && inot_instr->op == nir_op_inot) {
                     assert(!op[i].negate);
      } else {
               }
      bool
   fs_visitor::try_emit_b2fi_of_inot(const fs_builder &bld,
               {
      if (devinfo->ver < 6 || devinfo->verx10 >= 125)
                     if (inot_instr == NULL || inot_instr->op != nir_op_inot)
            /* HF is also possible as a destination on BDW+.  For nir_op_b2i, the set
   * of valid size-changing combinations is a bit more complex.
   *
   * The source restriction is just because I was lazy about generating the
   * constant below.
   */
   if (instr->def.bit_size != 32 ||
      nir_src_bit_size(inot_instr->src[0].src) != 32)
         /* b2[fi](inot(a)) maps a=0 => 1, a=-1 => 0.  Since a can only be 0 or -1,
   * this is float(1 + a).
   */
                     /* Ignore the saturate modifier, if there is one.  The result of the
   * arithmetic can only be 0 or 1, so the clamping will do nothing anyway.
   */
               }
      /**
   * Emit code for nir_op_fsign possibly fused with a nir_op_fmul
   *
   * If \c instr is not the \c nir_op_fsign, then \c fsign_src is the index of
   * the source of \c instr that is a \c nir_op_fsign.
   */
   void
   fs_visitor::emit_fsign(const fs_builder &bld, const nir_alu_instr *instr,
         {
               assert(instr->op == nir_op_fsign || instr->op == nir_op_fmul);
            if (instr->op != nir_op_fsign) {
      const nir_alu_instr *const fsign_instr =
            /* op[fsign_src] has the nominal result of the fsign, and op[1 -
   * fsign_src] has the other multiply source.  This must be rearranged so
   * that op[0] is the source of the fsign op[1] is the other multiply
   * source.
   */
   if (fsign_src != 0)
                     const nir_alu_type t =
                           unsigned channel = 0;
   if (nir_op_infos[instr->op].output_size == 0) {
      /* Since NIR is doing the scalarizing for us, we should only ever see
   * vectorized operations with a single channel.
   */
   nir_component_mask_t write_mask = get_nir_write_mask(instr->def);
   assert(util_bitcount(write_mask) == 1);
                           if (type_sz(op[0].type) == 2) {
      /* AND(val, 0x8000) gives the sign bit.
   *
   * Predicated OR ORs 1.0 (0x3c00) with the sign bit if val is not zero.
   */
   fs_reg zero = retype(brw_imm_uw(0), BRW_REGISTER_TYPE_HF);
            op[0].type = BRW_REGISTER_TYPE_UW;
   result.type = BRW_REGISTER_TYPE_UW;
            if (instr->op == nir_op_fsign)
         else {
      /* Use XOR here to get the result sign correct. */
                  } else if (type_sz(op[0].type) == 4) {
      /* AND(val, 0x80000000) gives the sign bit.
   *
   * Predicated OR ORs 1.0 (0x3f800000) with the sign bit if val is not
   * zero.
   */
            op[0].type = BRW_REGISTER_TYPE_UD;
   result.type = BRW_REGISTER_TYPE_UD;
            if (instr->op == nir_op_fsign)
         else {
      /* Use XOR here to get the result sign correct. */
                  } else {
      /* For doubles we do the same but we need to consider:
   *
   * - 2-src instructions can't operate with 64-bit immediates
   * - The sign is encoded in the high 32-bit of each DF
   * - We need to produce a DF result.
            fs_reg zero = vgrf(glsl_type::double_type);
   bld.MOV(zero, setup_imm_df(bld, 0.0));
                     fs_reg r = subscript(result, BRW_REGISTER_TYPE_UD, 1);
   bld.AND(r, subscript(op[0], BRW_REGISTER_TYPE_UD, 1),
            if (instr->op == nir_op_fsign) {
      set_predicate(BRW_PREDICATE_NORMAL,
      } else {
      if (devinfo->has_64bit_int) {
      /* This could be done better in some cases.  If the scale is an
   * immediate with the low 32-bits all 0, emitting a separate XOR and
   * OR would allow an algebraic optimization to remove the OR.  There
   * are currently zero instances of fsign(double(x))*IMM in shader-db
   * or any test suite, so it is hard to care at this time.
   */
   fs_reg result_int64 = retype(result, BRW_REGISTER_TYPE_UQ);
   inst = bld.XOR(result_int64, result_int64,
      } else {
      fs_reg result_int64 = retype(result, BRW_REGISTER_TYPE_UQ);
   bld.MOV(subscript(result_int64, BRW_REGISTER_TYPE_UD, 0),
         bld.XOR(subscript(result_int64, BRW_REGISTER_TYPE_UD, 1),
                     }
      /**
   * Determine whether sources of a nir_op_fmul can be fused with a nir_op_fsign
   *
   * Checks the operands of a \c nir_op_fmul to determine whether or not
   * \c emit_fsign could fuse the multiplication with the \c sign() calculation.
   *
   * \param instr  The multiplication instruction
   *
   * \param fsign_src The source of \c instr that may or may not be a
   *                  \c nir_op_fsign
   */
   static bool
   can_fuse_fmul_fsign(nir_alu_instr *instr, unsigned fsign_src)
   {
               nir_alu_instr *const fsign_instr =
            /* Rules:
   *
   * 1. instr->src[fsign_src] must be a nir_op_fsign.
   * 2. The nir_op_fsign can only be used by this multiplication.
   * 3. The source that is the nir_op_fsign does not have source modifiers.
   *    \c emit_fsign only examines the source modifiers of the source of the
   *    \c nir_op_fsign.
   *
   * The nir_op_fsign must also not have the saturate modifier, but steps
   * have already been taken (in nir_opt_algebraic) to ensure that.
   */
   return fsign_instr != NULL && fsign_instr->op == nir_op_fsign &&
      }
      static bool
   is_const_zero(const nir_src &src)
   {
         }
      void
   fs_visitor::nir_emit_alu(const fs_builder &bld, nir_alu_instr *instr,
         {
      fs_inst *inst;
   unsigned execution_mode =
            fs_reg op[NIR_MAX_VEC_COMPONENTS];
         #ifndef NDEBUG
      /* Everything except raw moves, some type conversions, iabs, and ineg
   * should have 8-bit sources lowered by nir_lower_bit_size in
   * brw_preprocess_nir or by brw_nir_lower_conversions in
   * brw_postprocess_nir.
   */
   switch (instr->op) {
   case nir_op_mov:
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_vec8:
   case nir_op_vec16:
   case nir_op_i2f16:
   case nir_op_i2f32:
   case nir_op_i2i16:
   case nir_op_i2i32:
   case nir_op_u2f16:
   case nir_op_u2f32:
   case nir_op_u2u16:
   case nir_op_u2u32:
   case nir_op_iabs:
   case nir_op_ineg:
   case nir_op_pack_32_4x8_split:
            default:
      for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
               #endif
         switch (instr->op) {
   case nir_op_mov:
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_vec8:
   case nir_op_vec16: {
      fs_reg temp = result;
            nir_intrinsic_instr *store_reg =
         if (store_reg != NULL) {
      nir_def *dest_reg = store_reg->src[1].ssa;
   for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      nir_intrinsic_instr *load_reg =
                        if (load_reg->src[0].ssa == dest_reg) {
      need_extra_copy = true;
   temp = bld.vgrf(result.type, 4);
                     nir_component_mask_t write_mask = get_nir_write_mask(instr->def);
            for (unsigned i = 0; i < last_bit; i++) {
                     if (instr->op == nir_op_mov) {
      bld.MOV(offset(temp, bld, i),
      } else {
      bld.MOV(offset(temp, bld, i),
                  /* In this case the source and destination registers were the same,
   * so we need to insert an extra set of moves in order to deal with
   * any swizzling.
   */
   if (need_extra_copy) {
      for (unsigned i = 0; i < last_bit; i++) {
                           }
               case nir_op_i2f32:
   case nir_op_u2f32:
      if (optimize_extract_to_float(instr, result))
         inst = bld.MOV(result, op[0]);
         case nir_op_f2f16_rtne:
   case nir_op_f2f16_rtz:
   case nir_op_f2f16: {
               if (nir_op_f2f16 == instr->op)
         else
            if (BRW_RND_MODE_UNSPECIFIED != rnd)
            assert(type_sz(op[0].type) < 8); /* brw_nir_lower_conversions */
   inst = bld.F32TO16(result, op[0]);
               case nir_op_b2i8:
   case nir_op_b2i16:
   case nir_op_b2i32:
   case nir_op_b2i64:
   case nir_op_b2f16:
   case nir_op_b2f32:
   case nir_op_b2f64:
      if (try_emit_b2fi_of_inot(bld, result, instr))
         op[0].type = BRW_REGISTER_TYPE_D;
   op[0].negate = !op[0].negate;
      case nir_op_i2f64:
   case nir_op_i2i64:
   case nir_op_u2f64:
   case nir_op_u2u64:
   case nir_op_f2f64:
   case nir_op_f2i64:
   case nir_op_f2u64:
   case nir_op_i2i32:
   case nir_op_u2u32:
   case nir_op_f2i32:
   case nir_op_f2u32:
   case nir_op_i2f16:
   case nir_op_u2f16:
   case nir_op_f2i16:
   case nir_op_f2u16:
   case nir_op_f2i8:
   case nir_op_f2u8:
      if (result.type == BRW_REGISTER_TYPE_B ||
      result.type == BRW_REGISTER_TYPE_UB ||
               if (op[0].type == BRW_REGISTER_TYPE_B ||
      op[0].type == BRW_REGISTER_TYPE_UB ||
               inst = bld.MOV(result, op[0]);
         case nir_op_i2i8:
   case nir_op_u2u8:
      assert(type_sz(op[0].type) < 8); /* brw_nir_lower_conversions */
      case nir_op_i2i16:
   case nir_op_u2u16: {
      /* Emit better code for u2u8(extract_u8(a, b)) and similar patterns.
   * Emitting the instructions one by one results in two MOV instructions
   * that won't be propagated.  By handling both instructions here, a
   * single MOV is emitted.
   */
   nir_alu_instr *extract_instr = nir_src_as_alu_instr(instr->src[0].src);
   if (extract_instr != NULL) {
      if (extract_instr->op == nir_op_extract_u8 ||
                     const unsigned byte = nir_src_as_uint(extract_instr->src[1].src);
                     } else if (extract_instr->op == nir_op_extract_u16 ||
                     const unsigned word = nir_src_as_uint(extract_instr->src[1].src);
                                 inst = bld.MOV(result, op[0]);
               case nir_op_fsat:
      inst = bld.MOV(result, op[0]);
   inst->saturate = true;
         case nir_op_fneg:
   case nir_op_ineg:
      op[0].negate = true;
   inst = bld.MOV(result, op[0]);
         case nir_op_fabs:
   case nir_op_iabs:
      op[0].negate = false;
   op[0].abs = true;
   inst = bld.MOV(result, op[0]);
         case nir_op_f2f32:
      if (nir_has_any_rounding_mode_enabled(execution_mode)) {
      brw_rnd_mode rnd =
         bld.exec_all().emit(SHADER_OPCODE_RND_MODE, bld.null_reg_ud(),
               if (op[0].type == BRW_REGISTER_TYPE_HF)
            inst = bld.MOV(result, op[0]);
         case nir_op_fsign:
      emit_fsign(bld, instr, result, op, 0);
         case nir_op_frcp:
      inst = bld.emit(SHADER_OPCODE_RCP, result, op[0]);
         case nir_op_fexp2:
      inst = bld.emit(SHADER_OPCODE_EXP2, result, op[0]);
         case nir_op_flog2:
      inst = bld.emit(SHADER_OPCODE_LOG2, result, op[0]);
         case nir_op_fsin:
      inst = bld.emit(SHADER_OPCODE_SIN, result, op[0]);
         case nir_op_fcos:
      inst = bld.emit(SHADER_OPCODE_COS, result, op[0]);
         case nir_op_fddx_fine:
      inst = bld.emit(FS_OPCODE_DDX_FINE, result, op[0]);
      case nir_op_fddx:
   case nir_op_fddx_coarse:
      inst = bld.emit(FS_OPCODE_DDX_COARSE, result, op[0]);
      case nir_op_fddy_fine:
      inst = bld.emit(FS_OPCODE_DDY_FINE, result, op[0]);
      case nir_op_fddy:
   case nir_op_fddy_coarse:
      inst = bld.emit(FS_OPCODE_DDY_COARSE, result, op[0]);
         case nir_op_fadd:
      if (nir_has_any_rounding_mode_enabled(execution_mode)) {
      brw_rnd_mode rnd =
         bld.exec_all().emit(SHADER_OPCODE_RND_MODE, bld.null_reg_ud(),
      }
      case nir_op_iadd:
      inst = bld.ADD(result, op[0], op[1]);
         case nir_op_iadd3:
      inst = bld.ADD3(result, op[0], op[1], op[2]);
         case nir_op_iadd_sat:
   case nir_op_uadd_sat:
      inst = bld.ADD(result, op[0], op[1]);
   inst->saturate = true;
         case nir_op_isub_sat:
      bld.emit(SHADER_OPCODE_ISUB_SAT, result, op[0], op[1]);
         case nir_op_usub_sat:
      bld.emit(SHADER_OPCODE_USUB_SAT, result, op[0], op[1]);
         case nir_op_irhadd:
   case nir_op_urhadd:
      assert(instr->def.bit_size < 64);
   inst = bld.AVG(result, op[0], op[1]);
         case nir_op_ihadd:
   case nir_op_uhadd: {
      assert(instr->def.bit_size < 64);
            if (devinfo->ver >= 8) {
      op[0] = resolve_source_modifiers(bld, op[0]);
               /* AVG(x, y) - ((x ^ y) & 1) */
   bld.XOR(tmp, op[0], op[1]);
   bld.AND(tmp, tmp, retype(brw_imm_ud(1), result.type));
   bld.AVG(result, op[0], op[1]);
   inst = bld.ADD(result, result, tmp);
   inst->src[1].negate = true;
               case nir_op_fmul:
      for (unsigned i = 0; i < 2; i++) {
      if (can_fuse_fmul_fsign(instr, i)) {
      emit_fsign(bld, instr, result, op, i);
                  /* We emit the rounding mode after the previous fsign optimization since
   * it won't result in a MUL, but will try to negate the value by other
   * means.
   */
   if (nir_has_any_rounding_mode_enabled(execution_mode)) {
      brw_rnd_mode rnd =
         bld.exec_all().emit(SHADER_OPCODE_RND_MODE, bld.null_reg_ud(),
               inst = bld.MUL(result, op[0], op[1]);
         case nir_op_imul_2x32_64:
   case nir_op_umul_2x32_64:
      bld.MUL(result, op[0], op[1]);
         case nir_op_imul_32x16:
   case nir_op_umul_32x16: {
      const bool ud = instr->op == nir_op_umul_32x16;
   const enum brw_reg_type word_type =
         const enum brw_reg_type dword_type =
                     /* Before copy propagation there are no immediate values. */
                     if (devinfo->ver >= 7)
         else
                        case nir_op_imul:
      assert(instr->def.bit_size < 64);
   bld.MUL(result, op[0], op[1]);
         case nir_op_imul_high:
   case nir_op_umul_high:
      assert(instr->def.bit_size < 64);
   if (instr->def.bit_size == 32) {
         } else {
      fs_reg tmp = bld.vgrf(brw_reg_type_from_bit_size(32, op[0].type));
   bld.MUL(tmp, op[0], op[1]);
      }
         case nir_op_idiv:
   case nir_op_udiv:
      assert(instr->def.bit_size < 64);
   bld.emit(SHADER_OPCODE_INT_QUOTIENT, result, op[0], op[1]);
         case nir_op_uadd_carry:
            case nir_op_usub_borrow:
            case nir_op_umod:
   case nir_op_irem:
      /* According to the sign table for INT DIV in the Ivy Bridge PRM, it
   * appears that our hardware just does the right thing for signed
   * remainder.
   */
   assert(instr->def.bit_size < 64);
   bld.emit(SHADER_OPCODE_INT_REMAINDER, result, op[0], op[1]);
         case nir_op_imod: {
      /* Get a regular C-style remainder.  If a % b == 0, set the predicate. */
            /* Math instructions don't support conditional mod */
   inst = bld.MOV(bld.null_reg_d(), result);
            /* Now, we need to determine if signs of the sources are different.
   * When we XOR the sources, the top bit is 0 if they are the same and 1
   * if they are different.  We can then use a conditional modifier to
   * turn that into a predicate.  This leads us to an XOR.l instruction.
   *
   * Technically, according to the PRM, you're not allowed to use .l on a
   * XOR instruction.  However, empirical experiments and Curro's reading
   * of the simulator source both indicate that it's safe.
   */
   fs_reg tmp = bld.vgrf(BRW_REGISTER_TYPE_D);
   inst = bld.XOR(tmp, op[0], op[1]);
   inst->predicate = BRW_PREDICATE_NORMAL;
            /* If the result of the initial remainder operation is non-zero and the
   * two sources have different signs, add in a copy of op[1] to get the
   * final integer modulus value.
   */
   inst = bld.ADD(result, result, op[1]);
   inst->predicate = BRW_PREDICATE_NORMAL;
               case nir_op_flt32:
   case nir_op_fge32:
   case nir_op_feq32:
   case nir_op_fneu32: {
               const uint32_t bit_size =  nir_src_bit_size(instr->src[0].src);
   if (bit_size != 32) {
      dest = bld.vgrf(op[0].type, 1);
                        if (bit_size > 32) {
         } else if(bit_size < 32) {
      /* When we convert the result to 32-bit we need to be careful and do
   * it as a signed conversion to get sign extension (for 32-bit true)
   */
                     }
               case nir_op_ilt32:
   case nir_op_ult32:
   case nir_op_ige32:
   case nir_op_uge32:
   case nir_op_ieq32:
   case nir_op_ine32: {
               const uint32_t bit_size = type_sz(op[0].type) * 8;
   if (bit_size != 32) {
      dest = bld.vgrf(op[0].type, 1);
               bld.CMP(dest, op[0], op[1],
            if (bit_size > 32) {
         } else if (bit_size < 32) {
      /* When we convert the result to 32-bit we need to be careful and do
   * it as a signed conversion to get sign extension (for 32-bit true)
   */
                     }
               case nir_op_inot:
      if (devinfo->ver >= 8) {
               if (inot_src_instr != NULL &&
      (inot_src_instr->op == nir_op_ior ||
   inot_src_instr->op == nir_op_ixor ||
   inot_src_instr->op == nir_op_iand)) {
   /* The sources of the source logical instruction are now the
   * sources of the instruction that will be generated.
   */
                  /* Smash all of the sources and destination to be signed.  This
   * doesn't matter for the operation of the instruction, but cmod
   * propagation fails on unsigned sources with negation (due to
   * fs_inst::can_do_cmod returning false).
   */
   result.type =
      brw_type_for_nir_type(devinfo,
            op[0].type =
      brw_type_for_nir_type(devinfo,
            op[1].type =
                        /* For XOR, only invert one of the sources.  Arbitrarily choose
   * the first source.
   */
   op[0].negate = !op[0].negate;
                  switch (inot_src_instr->op) {
   case nir_op_ior:
                  case nir_op_iand:
                  case nir_op_ixor:
                  default:
            }
      }
   bld.NOT(result, op[0]);
      case nir_op_ixor:
      if (devinfo->ver >= 8) {
         }
   bld.XOR(result, op[0], op[1]);
      case nir_op_ior:
      if (devinfo->ver >= 8) {
         }
   bld.OR(result, op[0], op[1]);
      case nir_op_iand:
      if (devinfo->ver >= 8) {
         }
   bld.AND(result, op[0], op[1]);
         case nir_op_fdot2:
   case nir_op_fdot3:
   case nir_op_fdot4:
   case nir_op_b32all_fequal2:
   case nir_op_b32all_iequal2:
   case nir_op_b32all_fequal3:
   case nir_op_b32all_iequal3:
   case nir_op_b32all_fequal4:
   case nir_op_b32all_iequal4:
   case nir_op_b32any_fnequal2:
   case nir_op_b32any_inequal2:
   case nir_op_b32any_fnequal3:
   case nir_op_b32any_inequal3:
   case nir_op_b32any_fnequal4:
   case nir_op_b32any_inequal4:
            case nir_op_ldexp:
            case nir_op_fsqrt:
      inst = bld.emit(SHADER_OPCODE_SQRT, result, op[0]);
         case nir_op_frsq:
      inst = bld.emit(SHADER_OPCODE_RSQ, result, op[0]);
         case nir_op_ftrunc:
      inst = bld.RNDZ(result, op[0]);
   if (devinfo->ver < 6) {
      set_condmod(BRW_CONDITIONAL_R, inst);
   set_predicate(BRW_PREDICATE_NORMAL,
            }
         case nir_op_fceil: {
      op[0].negate = !op[0].negate;
   fs_reg temp = vgrf(glsl_type::float_type);
   bld.RNDD(temp, op[0]);
   temp.negate = true;
   inst = bld.MOV(result, temp);
      }
   case nir_op_ffloor:
      inst = bld.RNDD(result, op[0]);
      case nir_op_ffract:
      inst = bld.FRC(result, op[0]);
      case nir_op_fround_even:
      inst = bld.RNDE(result, op[0]);
   if (devinfo->ver < 6) {
      set_condmod(BRW_CONDITIONAL_R, inst);
   set_predicate(BRW_PREDICATE_NORMAL,
            }
         case nir_op_fquantize2f16: {
      fs_reg tmp16 = bld.vgrf(BRW_REGISTER_TYPE_D);
   fs_reg tmp32 = bld.vgrf(BRW_REGISTER_TYPE_F);
            /* The destination stride must be at least as big as the source stride. */
            /* Check for denormal */
   fs_reg abs_src0 = op[0];
   abs_src0.abs = true;
   bld.CMP(bld.null_reg_f(), abs_src0, brw_imm_f(ldexpf(1.0, -14)),
         /* Get the appropriately signed zero */
   bld.AND(retype(zero, BRW_REGISTER_TYPE_UD),
         retype(op[0], BRW_REGISTER_TYPE_UD),
   /* Do the actual F32 -> F16 -> F32 conversion */
   bld.F32TO16(tmp16, op[0]);
   bld.F16TO32(tmp32, tmp16);
   /* Select that or zero based on normal status */
   inst = bld.SEL(result, zero, tmp32);
   inst->predicate = BRW_PREDICATE_NORMAL;
               case nir_op_imin:
   case nir_op_umin:
   case nir_op_fmin:
      inst = bld.emit_minmax(result, op[0], op[1], BRW_CONDITIONAL_L);
         case nir_op_imax:
   case nir_op_umax:
   case nir_op_fmax:
      inst = bld.emit_minmax(result, op[0], op[1], BRW_CONDITIONAL_GE);
         case nir_op_pack_snorm_2x16:
   case nir_op_pack_snorm_4x8:
   case nir_op_pack_unorm_2x16:
   case nir_op_pack_unorm_4x8:
   case nir_op_unpack_snorm_2x16:
   case nir_op_unpack_snorm_4x8:
   case nir_op_unpack_unorm_2x16:
   case nir_op_unpack_unorm_4x8:
   case nir_op_unpack_half_2x16:
   case nir_op_pack_half_2x16:
            case nir_op_unpack_half_2x16_split_x_flush_to_zero:
      assert(FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP16 & execution_mode);
      case nir_op_unpack_half_2x16_split_x:
      inst = bld.F16TO32(result, subscript(op[0], BRW_REGISTER_TYPE_HF, 0));
         case nir_op_unpack_half_2x16_split_y_flush_to_zero:
      assert(FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP16 & execution_mode);
      case nir_op_unpack_half_2x16_split_y:
      inst = bld.F16TO32(result, subscript(op[0], BRW_REGISTER_TYPE_HF, 1));
         case nir_op_pack_64_2x32_split:
   case nir_op_pack_32_2x16_split:
      bld.emit(FS_OPCODE_PACK, result, op[0], op[1]);
         case nir_op_pack_32_4x8_split:
      bld.emit(FS_OPCODE_PACK, result, op, 4);
         case nir_op_unpack_64_2x32_split_x:
   case nir_op_unpack_64_2x32_split_y: {
      if (instr->op == nir_op_unpack_64_2x32_split_x)
         else
                     case nir_op_unpack_32_2x16_split_x:
   case nir_op_unpack_32_2x16_split_y: {
      if (instr->op == nir_op_unpack_32_2x16_split_x)
         else
                     case nir_op_fpow:
      inst = bld.emit(SHADER_OPCODE_POW, result, op[0], op[1]);
         case nir_op_bitfield_reverse:
      assert(instr->def.bit_size == 32);
   assert(nir_src_bit_size(instr->src[0].src) == 32);
   bld.BFREV(result, op[0]);
         case nir_op_bit_count:
      assert(instr->def.bit_size == 32);
   assert(nir_src_bit_size(instr->src[0].src) < 64);
   bld.CBIT(result, op[0]);
         case nir_op_uclz:
      assert(instr->def.bit_size == 32);
   assert(nir_src_bit_size(instr->src[0].src) == 32);
   bld.LZD(retype(result, BRW_REGISTER_TYPE_UD), op[0]);
         case nir_op_ifind_msb: {
      assert(instr->def.bit_size == 32);
   assert(nir_src_bit_size(instr->src[0].src) == 32);
                     /* FBH counts from the MSB side, while GLSL's findMSB() wants the count
   * from the LSB side. If FBH didn't return an error (0xFFFFFFFF), then
   * subtract the result from 31 to convert the MSB count into an LSB
   * count.
   */
            inst = bld.ADD(result, result, brw_imm_d(31));
   inst->predicate = BRW_PREDICATE_NORMAL;
   inst->src[0].negate = true;
               case nir_op_find_lsb:
      assert(instr->def.bit_size == 32);
   assert(nir_src_bit_size(instr->src[0].src) == 32);
   assert(devinfo->ver >= 7);
   bld.FBL(result, op[0]);
         case nir_op_ubitfield_extract:
   case nir_op_ibitfield_extract:
         case nir_op_ubfe:
   case nir_op_ibfe:
      assert(instr->def.bit_size < 64);
   bld.BFE(result, op[2], op[1], op[0]);
      case nir_op_bfm:
      assert(instr->def.bit_size < 64);
   bld.BFI1(result, op[0], op[1]);
      case nir_op_bfi:
               /* bfi is ((...) | (~src0 & src2)). The second part is zero when src2 is
   * either 0 or src0. Replacing the 0 with another value can eliminate a
   * temporary register.
   */
   if (is_const_zero(instr->src[2].src))
         else
                  case nir_op_bitfield_insert:
            /* For all shift operations:
   *
   * Gen4 - Gen7: After application of source modifiers, the low 5-bits of
   * src1 are used an unsigned value for the shift count.
   *
   * Gen8: As with earlier platforms, but for Q and UQ types on src0, the low
   * 6-bit of src1 are used.
   *
   * Gen9+: The low bits of src1 matching the size of src0 (e.g., 4-bits for
   * W or UW src0).
   *
   * The implication is that the following instruction will produce a
   * different result on Gen9+ than on previous platforms:
   *
   *    shr(8)    g4<1>UW    g12<8,8,1>UW    0x0010UW
   *
   * where Gen9+ will shift by zero, and earlier platforms will shift by 16.
   *
   * This does not seem to be the case.  Experimentally, it has been
   * determined that shifts of 16-bit values on Gen8 behave properly.  Shifts
   * of 8-bit values on both Gen8 and Gen9 do not.  Gen11+ lowers 8-bit
   * values, so those platforms were not tested.  No features expose access
   * to 8- or 16-bit types on Gen7 or earlier, so those platforms were not
   * tested either.  See
   * https://gitlab.freedesktop.org/mesa/crucible/-/merge_requests/76.
   *
   * This is part of the reason 8-bit values are lowered to 16-bit on all
   * platforms.
   */
   case nir_op_ishl:
      bld.SHL(result, op[0], op[1]);
      case nir_op_ishr:
      bld.ASR(result, op[0], op[1]);
      case nir_op_ushr:
      bld.SHR(result, op[0], op[1]);
         case nir_op_urol:
      bld.ROL(result, op[0], op[1]);
      case nir_op_uror:
      bld.ROR(result, op[0], op[1]);
         case nir_op_pack_half_2x16_split:
      bld.emit(FS_OPCODE_PACK_HALF_2x16_SPLIT, result, op[0], op[1]);
         case nir_op_sdot_4x8_iadd:
   case nir_op_sdot_4x8_iadd_sat:
      inst = bld.DP4A(retype(result, BRW_REGISTER_TYPE_D),
                        if (instr->op == nir_op_sdot_4x8_iadd_sat)
               case nir_op_udot_4x8_uadd:
   case nir_op_udot_4x8_uadd_sat:
      inst = bld.DP4A(retype(result, BRW_REGISTER_TYPE_UD),
                        if (instr->op == nir_op_udot_4x8_uadd_sat)
               case nir_op_sudot_4x8_iadd:
   case nir_op_sudot_4x8_iadd_sat:
      inst = bld.DP4A(retype(result, BRW_REGISTER_TYPE_D),
                        if (instr->op == nir_op_sudot_4x8_iadd_sat)
               case nir_op_ffma:
      if (nir_has_any_rounding_mode_enabled(execution_mode)) {
      brw_rnd_mode rnd =
         bld.exec_all().emit(SHADER_OPCODE_RND_MODE, bld.null_reg_ud(),
               inst = bld.MAD(result, op[2], op[1], op[0]);
         case nir_op_flrp:
      if (nir_has_any_rounding_mode_enabled(execution_mode)) {
      brw_rnd_mode rnd =
         bld.exec_all().emit(SHADER_OPCODE_RND_MODE, bld.null_reg_ud(),
               inst = bld.LRP(result, op[0], op[1], op[2]);
         case nir_op_b32csel:
      if (optimize_frontfacing_ternary(instr, result))
            bld.CMP(bld.null_reg_d(), op[0], brw_imm_d(0), BRW_CONDITIONAL_NZ);
   inst = bld.SEL(result, op[1], op[2]);
   inst->predicate = BRW_PREDICATE_NORMAL;
         case nir_op_extract_u8:
   case nir_op_extract_i8: {
               /* The PRMs say:
   *
   *    BDW+
   *    There is no direct conversion from B/UB to Q/UQ or Q/UQ to B/UB.
   *    Use two instructions and a word or DWord intermediate integer type.
   */
   if (instr->def.bit_size == 64) {
               if (instr->op == nir_op_extract_i8) {
      /* If we need to sign extend, extract to a word first */
   fs_reg w_temp = bld.vgrf(BRW_REGISTER_TYPE_W);
   bld.MOV(w_temp, subscript(op[0], type, byte));
      } else if (byte & 1) {
      /* Extract the high byte from the word containing the desired byte
   * offset.
   */
   bld.SHR(result,
            } else {
      /* Otherwise use an AND with 0xff and a word type */
   bld.AND(result,
               } else {
      const brw_reg_type type = brw_int_type(1, instr->op == nir_op_extract_i8);
      }
               case nir_op_extract_u16:
   case nir_op_extract_i16: {
      const brw_reg_type type = brw_int_type(2, instr->op == nir_op_extract_i16);
   unsigned word = nir_src_as_uint(instr->src[1].src);
   bld.MOV(result, subscript(op[0], type, word));
               default:
                  /* If we need to do a boolean resolve, replace the result with -(x & 1)
   * to sign extend the low bit to 0/~0
   */
   if (devinfo->ver <= 5 &&
      !result.is_null() &&
   (instr->instr.pass_flags & BRW_NIR_BOOLEAN_MASK) == BRW_NIR_BOOLEAN_NEEDS_RESOLVE) {
   fs_reg masked = vgrf(glsl_type::int_type);
   bld.AND(masked, result, brw_imm_d(1));
   masked.negate = true;
         }
      void
   fs_visitor::nir_emit_load_const(const fs_builder &bld,
         {
      const brw_reg_type reg_type =
                  switch (instr->def.bit_size) {
   case 8:
      for (unsigned i = 0; i < instr->def.num_components; i++)
               case 16:
      for (unsigned i = 0; i < instr->def.num_components; i++)
               case 32:
      for (unsigned i = 0; i < instr->def.num_components; i++)
               case 64:
      assert(devinfo->ver >= 7);
   if (!devinfo->has_64bit_int) {
      for (unsigned i = 0; i < instr->def.num_components; i++) {
      bld.MOV(retype(offset(reg, bld, i), BRW_REGISTER_TYPE_DF),
         } else {
      for (unsigned i = 0; i < instr->def.num_components; i++)
      }
         default:
                     }
      bool
   fs_visitor::get_nir_src_bindless(const nir_src &src)
   {
         }
      unsigned
   fs_visitor::get_nir_src_block(const nir_src &src)
   {
      return nir_ssa_bind_infos[src.ssa->index].valid ?
            }
      static bool
   is_resource_src(nir_src src)
   {
      return src.ssa->parent_instr->type == nir_instr_type_intrinsic &&
      }
      fs_reg
   fs_visitor::get_resource_nir_src(const nir_src &src)
   {
      if (!is_resource_src(src))
            }
      fs_reg
   fs_visitor::get_nir_src(const nir_src &src)
   {
               fs_reg reg;
   if (!load_reg) {
      if (nir_src_is_undef(src)) {
      const brw_reg_type reg_type =
      brw_reg_type_from_bit_size(src.ssa->bit_size,
         } else {
            } else {
      nir_intrinsic_instr *decl_reg = nir_reg_get_decl(load_reg->src[0].ssa);
   /* We don't handle indirects on locals */
   assert(nir_intrinsic_base(load_reg) == 0);
   assert(load_reg->intrinsic != nir_intrinsic_load_reg_indirect);
               if (nir_src_bit_size(src) == 64 && devinfo->ver == 7) {
      /* The only 64-bit type available on gfx7 is DF, so use that. */
      } else {
      /* To avoid floating-point denorm flushing problems, set the type by
   * default to an integer type - instructions that need floating point
   * semantics will set this to F if they need to
   */
   reg.type = brw_reg_type_from_bit_size(nir_src_bit_size(src),
                  }
      /**
   * Return an IMM for constants; otherwise call get_nir_src() as normal.
   *
   * This function should not be called on any value which may be 64 bits.
   * We could theoretically support 64-bit on gfx8+ but we choose not to
   * because it wouldn't work in general (no gfx7 support) and there are
   * enough restrictions in 64-bit immediates that you can't take the return
   * value and treat it the same as the result of get_nir_src().
   */
   fs_reg
   fs_visitor::get_nir_src_imm(const nir_src &src)
   {
      assert(nir_src_bit_size(src) == 32);
   return nir_src_is_const(src) ?
      }
      fs_reg
   fs_visitor::get_nir_def(const nir_def &def)
   {
      nir_intrinsic_instr *store_reg = nir_store_reg_for_def(&def);
   if (!store_reg) {
      const brw_reg_type reg_type =
      brw_reg_type_from_bit_size(def.bit_size,
                  nir_ssa_values[def.index] =
         bld.UNDEF(nir_ssa_values[def.index]);
      } else {
      nir_intrinsic_instr *decl_reg =
         /* We don't handle indirects on locals */
   assert(nir_intrinsic_base(store_reg) == 0);
   assert(store_reg->intrinsic != nir_intrinsic_store_reg_indirect);
         }
      nir_component_mask_t
   fs_visitor::get_nir_write_mask(const nir_def &def)
   {
      nir_intrinsic_instr *store_reg = nir_store_reg_for_def(&def);
   if (!store_reg) {
         } else {
            }
      static fs_inst *
   emit_pixel_interpolater_send(const fs_builder &bld,
                              enum opcode opcode,
      {
      struct brw_wm_prog_data *wm_prog_data =
            fs_reg srcs[INTERP_NUM_SRCS];
   srcs[INTERP_SRC_OFFSET]       = src;
   srcs[INTERP_SRC_MSG_DESC]     = desc;
            fs_inst *inst = bld.emit(opcode, dst, srcs, INTERP_NUM_SRCS);
   /* 2 floats per slot returned */
   inst->size_written = 2 * dst.component_size(inst->exec_size);
   if (interpolation == INTERP_MODE_NOPERSPECTIVE) {
      inst->pi_noperspective = true;
   /* TGL BSpec says:
   *     This field cannot be set to "Linear Interpolation"
   *     unless Non-Perspective Barycentric Enable in 3DSTATE_CLIP is enabled"
   */
                           }
      /**
   * Computes 1 << x, given a D/UD register containing some value x.
   */
   static fs_reg
   intexp2(const fs_builder &bld, const fs_reg &x)
   {
               fs_reg result = bld.vgrf(x.type, 1);
            bld.MOV(one, retype(brw_imm_d(1), one.type));
   bld.SHL(result, one, x);
      }
      void
   fs_visitor::emit_gs_end_primitive(const nir_src &vertex_count_nir_src)
   {
                        if (gs_compile->control_data_header_size_bits == 0)
            /* We can only do EndPrimitive() functionality when the control data
   * consists of cut bits.  Fortunately, the only time it isn't is when the
   * output type is points, in which case EndPrimitive() is a no-op.
   */
   if (gs_prog_data->control_data_format !=
      GFX7_GS_CONTROL_DATA_FORMAT_GSCTL_CUT) {
               /* Cut bits use one bit per vertex. */
            fs_reg vertex_count = get_nir_src(vertex_count_nir_src);
            /* Cut bit n should be set to 1 if EndPrimitive() was called after emitting
   * vertex n, 0 otherwise.  So all we need to do here is mark bit
   * (vertex_count - 1) % 32 in the cut_bits register to indicate that
   * EndPrimitive() was called after emitting vertex (vertex_count - 1);
   * vec4_gs_visitor::emit_control_data_bits() will take care of the rest.
   *
   * Note that if EndPrimitive() is called before emitting any vertices, this
   * will cause us to set bit 31 of the control_data_bits register to 1.
   * That's fine because:
   *
   * - If max_vertices < 32, then vertex number 31 (zero-based) will never be
   *   output, so the hardware will ignore cut bit 31.
   *
   * - If max_vertices == 32, then vertex number 31 is guaranteed to be the
   *   last vertex, so setting cut bit 31 has no effect (since the primitive
   *   is automatically ended when the GS terminates).
   *
   * - If max_vertices > 32, then the ir_emit_vertex visitor will reset the
   *   control_data_bits register to 0 when the first vertex is emitted.
                     /* control_data_bits |= 1 << ((vertex_count - 1) % 32) */
   fs_reg prev_count = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   abld.ADD(prev_count, vertex_count, brw_imm_ud(0xffffffffu));
   fs_reg mask = intexp2(abld, prev_count);
   /* Note: we're relying on the fact that the GEN SHL instruction only pays
   * attention to the lower 5 bits of its second source argument, so on this
   * architecture, 1 << (vertex_count - 1) is equivalent to 1 <<
   * ((vertex_count - 1) % 32).
   */
      }
      void
   fs_visitor::emit_gs_control_data_bits(const fs_reg &vertex_count)
   {
      assert(stage == MESA_SHADER_GEOMETRY);
                     const fs_builder abld = bld.annotate("emit control data bits");
            /* We use a single UD register to accumulate control data bits (32 bits
   * for each of the SIMD8 channels).  So we need to write a DWord (32 bits)
   * at a time.
   *
   * Unfortunately, the URB_WRITE_SIMD8 message uses 128-bit (OWord) offsets.
   * We have select a 128-bit group via the Global and Per-Slot Offsets, then
   * use the Channel Mask phase to enable/disable which DWord within that
   * group to write.  (Remember, different SIMD8 channels may have emitted
   * different numbers of vertices, so we may need per-slot offsets.)
   *
   * Channel masking presents an annoying problem: we may have to replicate
   * the data up to 4 times:
   *
   * Msg = Handles, Per-Slot Offsets, Channel Masks, Data, Data, Data, Data.
   *
   * To avoid penalizing shaders that emit a small number of vertices, we
   * can avoid these sometimes: if the size of the control data header is
   * <= 128 bits, then there is only 1 OWord.  All SIMD8 channels will land
   * land in the same 128-bit group, so we can skip per-slot offsets.
   *
   * Similarly, if the control data header is <= 32 bits, there is only one
   * DWord, so we can skip channel masks.
   */
            if (gs_compile->control_data_header_size_bits > 32)
            if (gs_compile->control_data_header_size_bits > 128)
            /* Figure out which DWord we're trying to write to using the formula:
   *
   *    dword_index = (vertex_count - 1) * bits_per_vertex / 32
   *
   * Since bits_per_vertex is a power of two, and is known at compile
   * time, this can be optimized to:
   *
   *    dword_index = (vertex_count - 1) >> (6 - log2(bits_per_vertex))
   */
   if (channel_mask.file != BAD_FILE || per_slot_offset.file != BAD_FILE) {
      fs_reg dword_index = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   fs_reg prev_count = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   abld.ADD(prev_count, vertex_count, brw_imm_ud(0xffffffffu));
   unsigned log2_bits_per_vertex =
                  if (per_slot_offset.file != BAD_FILE) {
      /* Set the per-slot offset to dword_index / 4, so that we'll write to
   * the appropriate OWord within the control data header.
   */
               /* Set the channel masks to 1 << (dword_index % 4), so that we'll
   * write to the appropriate DWORD within the OWORD.
   */
   fs_reg channel = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   fwa_bld.AND(channel, dword_index, brw_imm_ud(3u));
   channel_mask = intexp2(fwa_bld, channel);
   /* Then the channel masks need to be in bits 23:16. */
               /* If there are channel masks, add 3 extra copies of the data. */
   const unsigned length = 1 + 3 * unsigned(channel_mask.file != BAD_FILE);
            for (unsigned i = 0; i < ARRAY_SIZE(sources); i++)
            fs_reg srcs[URB_LOGICAL_NUM_SRCS];
   srcs[URB_LOGICAL_SRC_HANDLE] = gs_payload().urb_handles;
   srcs[URB_LOGICAL_SRC_PER_SLOT_OFFSETS] = per_slot_offset;
   srcs[URB_LOGICAL_SRC_CHANNEL_MASK] = channel_mask;
   srcs[URB_LOGICAL_SRC_DATA] = bld.vgrf(BRW_REGISTER_TYPE_F, length);
   srcs[URB_LOGICAL_SRC_COMPONENTS] = brw_imm_ud(length);
            fs_inst *inst = abld.emit(SHADER_OPCODE_URB_WRITE_LOGICAL, reg_undef,
            /* We need to increment Global Offset by 256-bits to make room for
   * Broadwell's extra "Vertex Count" payload at the beginning of the
   * URB entry.  Since this is an OWord message, Global Offset is counted
   * in 128-bit units, so we must set it to 2.
   */
   if (gs_prog_data->static_vertex_count == -1)
      }
      void
   fs_visitor::set_gs_stream_control_data_bits(const fs_reg &vertex_count,
         {
               /* Note: we are calling this *before* increasing vertex_count, so
   * this->vertex_count == vertex_count - 1 in the formula above.
            /* Stream mode uses 2 bits per vertex */
            /* Must be a valid stream */
            /* Control data bits are initialized to 0 so we don't have to set any
   * bits when sending vertices to stream 0.
   */
   if (stream_id == 0)
                     /* reg::sid = stream_id */
   fs_reg sid = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
            /* reg:shift_count = 2 * (vertex_count - 1) */
   fs_reg shift_count = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
            /* Note: we're relying on the fact that the GEN SHL instruction only pays
   * attention to the lower 5 bits of its second source argument, so on this
   * architecture, stream_id << 2 * (vertex_count - 1) is equivalent to
   * stream_id << ((2 * (vertex_count - 1)) % 32).
   */
   fs_reg mask = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   abld.SHL(mask, sid, shift_count);
      }
      void
   fs_visitor::emit_gs_vertex(const nir_src &vertex_count_nir_src,
         {
                        fs_reg vertex_count = get_nir_src(vertex_count_nir_src);
            /* Haswell and later hardware ignores the "Render Stream Select" bits
   * from the 3DSTATE_STREAMOUT packet when the SOL stage is disabled,
   * and instead sends all primitives down the pipeline for rasterization.
   * If the SOL stage is enabled, "Render Stream Select" is honored and
   * primitives bound to non-zero streams are discarded after stream output.
   *
   * Since the only purpose of primives sent to non-zero streams is to
   * be recorded by transform feedback, we can simply discard all geometry
   * bound to these streams when transform feedback is disabled.
   */
   if (stream_id > 0 && !nir->info.has_transform_feedback_varyings)
            /* If we're outputting 32 control data bits or less, then we can wait
   * until the shader is over to output them all.  Otherwise we need to
   * output them as we go.  Now is the time to do it, since we're about to
   * output the vertex_count'th vertex, so it's guaranteed that the
   * control data bits associated with the (vertex_count - 1)th vertex are
   * correct.
   */
   if (gs_compile->control_data_header_size_bits > 32) {
      const fs_builder abld =
            /* Only emit control data bits if we've finished accumulating a batch
   * of 32 bits.  This is the case when:
   *
   *     (vertex_count * bits_per_vertex) % 32 == 0
   *
   * (in other words, when the last 5 bits of vertex_count *
   * bits_per_vertex are 0).  Assuming bits_per_vertex == 2^n for some
   * integer n (which is always the case, since bits_per_vertex is
   * always 1 or 2), this is equivalent to requiring that the last 5-n
   * bits of vertex_count are 0:
   *
   *     vertex_count & (2^(5-n) - 1) == 0
   *
   * 2^(5-n) == 2^5 / 2^n == 32 / bits_per_vertex, so this is
   * equivalent to:
   *
   *     vertex_count & (32 / bits_per_vertex - 1) == 0
   *
   * TODO: If vertex_count is an immediate, we could do some of this math
   *       at compile time...
   */
   fs_inst *inst =
      abld.AND(bld.null_reg_d(), vertex_count,
               abld.IF(BRW_PREDICATE_NORMAL);
   /* If vertex_count is 0, then no control data bits have been
   * accumulated yet, so we can skip emitting them.
   */
   abld.CMP(bld.null_reg_d(), vertex_count, brw_imm_ud(0u),
         abld.IF(BRW_PREDICATE_NORMAL);
   emit_gs_control_data_bits(vertex_count);
            /* Reset control_data_bits to 0 so we can start accumulating a new
   * batch.
   *
   * Note: in the case where vertex_count == 0, this neutralizes the
   * effect of any call to EndPrimitive() that the shader may have
   * made before outputting its first vertex.
   */
   inst = abld.MOV(this->control_data_bits, brw_imm_ud(0u));
   inst->force_writemask_all = true;
                        /* In stream mode we have to set control data bits for all vertices
   * unless we have disabled control data bits completely (which we do
   * do for GL_POINTS outputs that don't use streams).
   */
   if (gs_compile->control_data_header_size_bits > 0 &&
      gs_prog_data->control_data_format ==
               }
      void
   fs_visitor::emit_gs_input_load(const fs_reg &dst,
                                 {
      assert(type_sz(dst.type) == 4);
   struct brw_gs_prog_data *gs_prog_data = brw_gs_prog_data(prog_data);
            /* TODO: figure out push input layout for invocations == 1 */
   if (gs_prog_data->invocations == 1 &&
      nir_src_is_const(offset_src) && nir_src_is_const(vertex_src) &&
   4 * (base_offset + nir_src_as_uint(offset_src)) < push_reg_count) {
   int imm_offset = (base_offset + nir_src_as_uint(offset_src)) * 4 +
         for (unsigned i = 0; i < num_components; i++) {
      bld.MOV(offset(dst, bld, i),
      }
               /* Resort to the pull model.  Ensure the VUE handles are provided. */
            fs_reg start = gs_payload().icp_handle_start;
            if (gs_prog_data->invocations == 1) {
      if (nir_src_is_const(vertex_src)) {
      /* The vertex index is constant; just select the proper URB handle. */
      } else {
      /* The vertex index is non-constant.  We need to use indirect
   * addressing to fetch the proper URB handle.
   *
   * First, we start with the sequence <7, 6, 5, 4, 3, 2, 1, 0>
   * indicating that channel <n> should read the handle from
   * DWord <n>.  We convert that to bytes by multiplying by 4.
   *
   * Next, we convert the vertex index to bytes by multiplying
   * by 32 (shifting by 5), and add the two together.  This is
   * the final indirect byte offset.
   */
   fs_reg sequence =
         fs_reg channel_offsets = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
                  /* channel_offsets = 4 * sequence = <28, 24, 20, 16, 12, 8, 4, 0> */
   bld.SHL(channel_offsets, sequence, brw_imm_ud(2u));
   /* Convert vertex_index to bytes (multiply by 32) */
   bld.SHL(vertex_offset_bytes,
                        /* Use first_icp_handle as the base offset.  There is one register
   * of URB handles per vertex, so inform the register allocator that
   * we might read up to nir->info.gs.vertices_in registers.
   */
   bld.emit(SHADER_OPCODE_MOV_INDIRECT, icp_handle, start,
               } else {
               if (nir_src_is_const(vertex_src)) {
      unsigned vertex = nir_src_as_uint(vertex_src);
   assert(devinfo->ver >= 9 || vertex <= 5);
      } else {
      /* The vertex index is non-constant.  We need to use indirect
   * addressing to fetch the proper URB handle.
   *
                  /* Convert vertex_index to bytes (multiply by 4) */
   bld.SHL(icp_offset_bytes,
                  /* Use first_icp_handle as the base offset.  There is one DWord
   * of URB handles per vertex, so inform the register allocator that
   * we might read up to ceil(nir->info.gs.vertices_in / 8) registers.
   */
   bld.emit(SHADER_OPCODE_MOV_INDIRECT, icp_handle, start,
            fs_reg(icp_offset_bytes),
               fs_inst *inst;
            if (nir_src_is_const(offset_src)) {
      fs_reg srcs[URB_LOGICAL_NUM_SRCS];
            /* Constant indexing - use global offset. */
   if (first_component != 0) {
      unsigned read_components = num_components + first_component;
   fs_reg tmp = bld.vgrf(dst.type, read_components);
   inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, tmp, srcs,
         inst->size_written = read_components *
         for (unsigned i = 0; i < num_components; i++) {
      bld.MOV(offset(dst, bld, i),
         } else {
      inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, dst, srcs,
         inst->size_written = num_components *
      }
      } else {
      /* Indirect indexing - use per-slot offsets as well. */
   unsigned read_components = num_components + first_component;
            fs_reg srcs[URB_LOGICAL_NUM_SRCS];
   srcs[URB_LOGICAL_SRC_HANDLE] = icp_handle;
            if (first_component != 0) {
      inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, tmp,
         inst->size_written = read_components *
         for (unsigned i = 0; i < num_components; i++) {
      bld.MOV(offset(dst, bld, i),
         } else {
      inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, dst,
         inst->size_written = num_components *
      }
         }
      fs_reg
   fs_visitor::get_indirect_offset(nir_intrinsic_instr *instr)
   {
               if (nir_src_is_const(*offset_src)) {
      /* The only constant offset we should find is 0.  brw_nir.c's
   * add_const_offset_to_base() will fold other constant offsets
   * into the "base" index.
   */
   assert(nir_src_as_uint(*offset_src) == 0);
                  }
      void
   fs_visitor::nir_emit_vs_intrinsic(const fs_builder &bld,
         {
               fs_reg dest;
   if (nir_intrinsic_infos[instr->intrinsic].has_dest)
            switch (instr->intrinsic) {
   case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_base_vertex:
            case nir_intrinsic_load_input: {
      assert(instr->def.bit_size == 32);
   fs_reg src = fs_reg(ATTR, nir_intrinsic_base(instr) * 4, dest.type);
   src = offset(src, bld, nir_intrinsic_component(instr));
            for (unsigned i = 0; i < instr->num_components; i++)
                     case nir_intrinsic_load_vertex_id_zero_base:
   case nir_intrinsic_load_instance_id:
   case nir_intrinsic_load_base_instance:
   case nir_intrinsic_load_draw_id:
   case nir_intrinsic_load_first_vertex:
   case nir_intrinsic_load_is_indexed_draw:
            default:
      nir_emit_intrinsic(bld, instr);
         }
      fs_reg
   fs_visitor::get_tcs_single_patch_icp_handle(const fs_builder &bld,
         {
      struct brw_tcs_prog_data *tcs_prog_data = brw_tcs_prog_data(prog_data);
   const nir_src &vertex_src = instr->src[0];
                              if (nir_src_is_const(vertex_src)) {
      /* Emit a MOV to resolve <0,1,0> regioning. */
   icp_handle = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   unsigned vertex = nir_src_as_uint(vertex_src);
      } else if (tcs_prog_data->instances == 1 && vertex_intrin &&
            /* For the common case of only 1 instance, an array index of
   * gl_InvocationID means reading the handles from the start.  Skip all
   * the indirect work.
   */
      } else {
      /* The vertex index is non-constant.  We need to use indirect
   * addressing to fetch the proper URB handle.
   */
            /* Each ICP handle is a single DWord (4 bytes) */
   fs_reg vertex_offset_bytes = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   bld.SHL(vertex_offset_bytes,
                  /* We might read up to 4 registers. */
   bld.emit(SHADER_OPCODE_MOV_INDIRECT, icp_handle,
                        }
      fs_reg
   fs_visitor::get_tcs_multi_patch_icp_handle(const fs_builder &bld,
         {
      struct brw_tcs_prog_key *tcs_key = (struct brw_tcs_prog_key *) key;
   const nir_src &vertex_src = instr->src[0];
                     if (nir_src_is_const(vertex_src))
            /* The vertex index is non-constant.  We need to use indirect
   * addressing to fetch the proper URB handle.
   *
   * First, we start with the sequence indicating that channel <n>
   * should read the handle from DWord <n>.  We convert that to bytes
   * by multiplying by 4.
   *
   * Next, we convert the vertex index to bytes by multiplying
   * by the GRF size (by shifting), and add the two together.  This is
   * the final indirect byte offset.
   */
   fs_reg icp_handle = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   fs_reg sequence = nir_system_values[SYSTEM_VALUE_SUBGROUP_INVOCATION];
   fs_reg channel_offsets = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   fs_reg vertex_offset_bytes = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
            /* Offsets will be 0, 4, 8, ... */
   bld.SHL(channel_offsets, sequence, brw_imm_ud(2u));
   /* Convert vertex_index to bytes (multiply by 32) */
   assert(util_is_power_of_two_nonzero(grf_size_bytes)); /* for ffs() */
   bld.SHL(vertex_offset_bytes,
         retype(get_nir_src(vertex_src), BRW_REGISTER_TYPE_UD),
            /* Use start of ICP handles as the base offset.  There is one register
   * of URB handles per vertex, so inform the register allocator that
   * we might read up to nir->info.gs.vertices_in registers.
   */
   bld.emit(SHADER_OPCODE_MOV_INDIRECT, icp_handle, start,
            icp_offset_bytes,
            }
      void
   fs_visitor::nir_emit_tcs_intrinsic(const fs_builder &bld,
         {
      assert(stage == MESA_SHADER_TESS_CTRL);
   struct brw_tcs_prog_data *tcs_prog_data = brw_tcs_prog_data(prog_data);
            fs_reg dst;
   if (nir_intrinsic_infos[instr->intrinsic].has_dest)
            switch (instr->intrinsic) {
   case nir_intrinsic_load_primitive_id:
      bld.MOV(dst, tcs_payload().primitive_id);
      case nir_intrinsic_load_invocation_id:
      bld.MOV(retype(dst, invocation_id.type), invocation_id);
         case nir_intrinsic_barrier:
      if (nir_intrinsic_memory_scope(instr) != SCOPE_NONE)
         if (nir_intrinsic_execution_scope(instr) == SCOPE_WORKGROUP) {
      if (tcs_prog_data->instances != 1)
      }
         case nir_intrinsic_load_input:
      unreachable("nir_lower_io should never give us these.");
         case nir_intrinsic_load_per_vertex_input: {
      assert(instr->def.bit_size == 32);
   fs_reg indirect_offset = get_indirect_offset(instr);
   unsigned imm_offset = nir_intrinsic_base(instr);
            const bool multi_patch =
            fs_reg icp_handle = multi_patch ?
                  /* We can only read two double components with each URB read, so
   * we send two read messages in that case, each one loading up to
   * two double components.
   */
   unsigned num_components = instr->num_components;
            fs_reg srcs[URB_LOGICAL_NUM_SRCS];
            if (indirect_offset.file == BAD_FILE) {
      /* Constant indexing - use global offset. */
   if (first_component != 0) {
      unsigned read_components = num_components + first_component;
   fs_reg tmp = bld.vgrf(dst.type, read_components);
   inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, tmp, srcs,
         for (unsigned i = 0; i < num_components; i++) {
      bld.MOV(offset(dst, bld, i),
         } else {
      inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, dst, srcs,
      }
      } else {
                     if (first_component != 0) {
      unsigned read_components = num_components + first_component;
   fs_reg tmp = bld.vgrf(dst.type, read_components);
   inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, tmp,
         for (unsigned i = 0; i < num_components; i++) {
      bld.MOV(offset(dst, bld, i),
         } else {
      inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, dst,
      }
      }
   inst->size_written = (num_components + first_component) *
            /* Copy the temporary to the destination to deal with writemasking.
   *
   * Also attempt to deal with gl_PointSize being in the .w component.
   */
   if (inst->offset == 0 && indirect_offset.file == BAD_FILE) {
      assert(type_sz(dst.type) == 4);
   inst->dst = bld.vgrf(dst.type, 4);
   inst->size_written = 4 * REG_SIZE * reg_unit(devinfo);
      }
               case nir_intrinsic_load_output:
   case nir_intrinsic_load_per_vertex_output: {
      assert(instr->def.bit_size == 32);
   fs_reg indirect_offset = get_indirect_offset(instr);
   unsigned imm_offset = nir_intrinsic_base(instr);
            fs_inst *inst;
   if (indirect_offset.file == BAD_FILE) {
      /* This MOV replicates the output handle to all enabled channels
   * is SINGLE_PATCH mode.
   */
                  {
                     if (first_component != 0) {
      unsigned read_components =
         fs_reg tmp = bld.vgrf(dst.type, read_components);
   inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, tmp,
         inst->size_written = read_components * REG_SIZE * reg_unit(devinfo);
   for (unsigned i = 0; i < instr->num_components; i++) {
      bld.MOV(offset(dst, bld, i),
         } else {
      inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, dst,
            }
         } else {
      /* Indirect indexing - use per-slot offsets as well. */
   fs_reg srcs[URB_LOGICAL_NUM_SRCS];
                  if (first_component != 0) {
      unsigned read_components =
         fs_reg tmp = bld.vgrf(dst.type, read_components);
   inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, tmp,
         inst->size_written = read_components * REG_SIZE * reg_unit(devinfo);
   for (unsigned i = 0; i < instr->num_components; i++) {
      bld.MOV(offset(dst, bld, i),
         } else {
      inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, dst,
            }
      }
               case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output: {
      assert(nir_src_bit_size(instr->src[0]) == 32);
   fs_reg value = get_nir_src(instr->src[0]);
   fs_reg indirect_offset = get_indirect_offset(instr);
   unsigned imm_offset = nir_intrinsic_base(instr);
            if (mask == 0)
            unsigned num_components = util_last_bit(mask);
   unsigned first_component = nir_intrinsic_component(instr);
                              fs_reg mask_reg;
   if (mask != WRITEMASK_XYZW)
                     unsigned m = has_urb_lsc ? 0 : first_component;
   for (unsigned i = 0; i < num_components; i++) {
      int c = i + first_component;
   if (mask & (1 << c)) {
         } else if (devinfo->ver < 20) {
                              fs_reg srcs[URB_LOGICAL_NUM_SRCS];
   srcs[URB_LOGICAL_SRC_HANDLE] = tcs_payload().patch_urb_output;
   srcs[URB_LOGICAL_SRC_PER_SLOT_OFFSETS] = indirect_offset;
   srcs[URB_LOGICAL_SRC_CHANNEL_MASK] = mask_reg;
   srcs[URB_LOGICAL_SRC_DATA] = bld.vgrf(BRW_REGISTER_TYPE_F, m);
   srcs[URB_LOGICAL_SRC_COMPONENTS] = brw_imm_ud(m);
            fs_inst *inst = bld.emit(SHADER_OPCODE_URB_WRITE_LOGICAL, reg_undef,
         inst->offset = imm_offset;
               default:
      nir_emit_intrinsic(bld, instr);
         }
      void
   fs_visitor::nir_emit_tes_intrinsic(const fs_builder &bld,
         {
      assert(stage == MESA_SHADER_TESS_EVAL);
            fs_reg dest;
   if (nir_intrinsic_infos[instr->intrinsic].has_dest)
            switch (instr->intrinsic) {
   case nir_intrinsic_load_primitive_id:
      bld.MOV(dest, tes_payload().primitive_id);
         case nir_intrinsic_load_tess_coord:
      for (unsigned i = 0; i < 3; i++)
               case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_vertex_input: {
      assert(instr->def.bit_size == 32);
   fs_reg indirect_offset = get_indirect_offset(instr);
   unsigned imm_offset = nir_intrinsic_base(instr);
            fs_inst *inst;
   if (indirect_offset.file == BAD_FILE) {
      /* Arbitrarily only push up to 32 vec4 slots worth of data,
   * which is 16 registers (since each holds 2 vec4 slots).
   */
   const unsigned max_push_slots = 32;
   if (imm_offset < max_push_slots) {
      fs_reg src = fs_reg(ATTR, imm_offset / 2, dest.type);
   for (int i = 0; i < instr->num_components; i++) {
                        tes_prog_data->base.urb_read_length =
      MAX2(tes_prog_data->base.urb_read_length,
   } else {
      /* Replicate the patch handle to all enabled channels */
                  if (first_component != 0) {
      unsigned read_components =
         fs_reg tmp = bld.vgrf(dest.type, read_components);
   inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, tmp,
         inst->size_written = read_components * REG_SIZE * reg_unit(devinfo);
   for (unsigned i = 0; i < instr->num_components; i++) {
      bld.MOV(offset(dest, bld, i),
         } else {
      inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, dest,
            }
         } else {
               /* We can only read two double components with each URB read, so
   * we send two read messages in that case, each one loading up to
   * two double components.
                  fs_reg srcs[URB_LOGICAL_NUM_SRCS];
                  if (first_component != 0) {
      unsigned read_components =
         fs_reg tmp = bld.vgrf(dest.type, read_components);
   inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, tmp,
         for (unsigned i = 0; i < num_components; i++) {
      bld.MOV(offset(dest, bld, i),
         } else {
      inst = bld.emit(SHADER_OPCODE_URB_READ_LOGICAL, dest,
      }
   inst->offset = imm_offset;
   inst->size_written = (num_components + first_component) *
      }
      }
   default:
      nir_emit_intrinsic(bld, instr);
         }
      void
   fs_visitor::nir_emit_gs_intrinsic(const fs_builder &bld,
         {
      assert(stage == MESA_SHADER_GEOMETRY);
            fs_reg dest;
   if (nir_intrinsic_infos[instr->intrinsic].has_dest)
            switch (instr->intrinsic) {
   case nir_intrinsic_load_primitive_id:
      assert(stage == MESA_SHADER_GEOMETRY);
   assert(brw_gs_prog_data(prog_data)->include_primitive_id);
   bld.MOV(retype(dest, BRW_REGISTER_TYPE_UD), gs_payload().primitive_id);
         case nir_intrinsic_load_input:
            case nir_intrinsic_load_per_vertex_input:
      emit_gs_input_load(dest, instr->src[0], nir_intrinsic_base(instr),
                     case nir_intrinsic_emit_vertex_with_counter:
      emit_gs_vertex(instr->src[0], nir_intrinsic_stream_id(instr));
         case nir_intrinsic_end_primitive_with_counter:
      emit_gs_end_primitive(instr->src[0]);
         case nir_intrinsic_set_vertex_and_primitive_count:
      bld.MOV(this->final_gs_vertex_count, get_nir_src(instr->src[0]));
         case nir_intrinsic_load_invocation_id: {
      fs_reg val = nir_system_values[SYSTEM_VALUE_INVOCATION_ID];
   assert(val.file != BAD_FILE);
   dest.type = val.type;
   bld.MOV(dest, val);
               default:
      nir_emit_intrinsic(bld, instr);
         }
      /**
   * Fetch the current render target layer index.
   */
   static fs_reg
   fetch_render_target_array_index(const fs_builder &bld)
   {
      if (bld.shader->devinfo->ver >= 12) {
      /* The render target array index is provided in the thread payload as
   * bits 26:16 of r1.1.
   */
   const fs_reg idx = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.AND(idx, brw_uw1_reg(BRW_GENERAL_REGISTER_FILE, 1, 3),
            } else if (bld.shader->devinfo->ver >= 6) {
      /* The render target array index is provided in the thread payload as
   * bits 26:16 of r0.0.
   */
   const fs_reg idx = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.AND(idx, brw_uw1_reg(BRW_GENERAL_REGISTER_FILE, 0, 1),
            } else {
      /* Pre-SNB we only ever render into the first layer of the framebuffer
   * since layered rendering is not implemented.
   */
         }
      /**
   * Fake non-coherent framebuffer read implemented using TXF to fetch from the
   * framebuffer at the current fragment coordinates and sample index.
   */
   fs_inst *
   fs_visitor::emit_non_coherent_fb_read(const fs_builder &bld, const fs_reg &dst,
         {
               assert(bld.shader->stage == MESA_SHADER_FRAGMENT);
   const brw_wm_prog_key *wm_key =
                  /* Calculate the fragment coordinates. */
   const fs_reg coords = bld.vgrf(BRW_REGISTER_TYPE_UD, 3);
   bld.MOV(offset(coords, bld, 0), pixel_x);
   bld.MOV(offset(coords, bld, 1), pixel_y);
            /* Calculate the sample index and MCS payload when multisampling.  Luckily
   * the MCS fetch message behaves deterministically for UMS surfaces, so it
   * shouldn't be necessary to recompile based on whether the framebuffer is
   * CMS or UMS.
   */
   assert(wm_key->multisample_fbo == BRW_ALWAYS ||
         if (wm_key->multisample_fbo &&
      nir_system_values[SYSTEM_VALUE_SAMPLE_ID].file == BAD_FILE)
         const fs_reg sample = nir_system_values[SYSTEM_VALUE_SAMPLE_ID];
   const fs_reg mcs = wm_key->multisample_fbo ?
            /* Use either a normal or a CMS texel fetch message depending on whether
   * the framebuffer is single or multisample.  On SKL+ use the wide CMS
   * message just in case the framebuffer uses 16x multisampling, it should
   * be equivalent to the normal CMS fetch for lower multisampling modes.
   */
   opcode op;
   if (wm_key->multisample_fbo) {
      /* On SKL+ use the wide CMS message just in case the framebuffer uses 16x
   * multisampling, it should be equivalent to the normal CMS fetch for
   * lower multisampling modes.
   *
   * On Gfx12HP, there is only CMS_W variant available.
   */
   if (devinfo->verx10 >= 125)
         else if (devinfo->ver >= 9)
         else
      } else {
                  /* Emit the instruction. */
   fs_reg srcs[TEX_LOGICAL_NUM_SRCS];
   srcs[TEX_LOGICAL_SRC_COORDINATE]       = coords;
   srcs[TEX_LOGICAL_SRC_LOD]              = brw_imm_ud(0);
   srcs[TEX_LOGICAL_SRC_SAMPLE_INDEX]     = sample;
   srcs[TEX_LOGICAL_SRC_MCS]              = mcs;
   srcs[TEX_LOGICAL_SRC_SURFACE]          = brw_imm_ud(target);
   srcs[TEX_LOGICAL_SRC_SAMPLER]          = brw_imm_ud(0);
   srcs[TEX_LOGICAL_SRC_COORD_COMPONENTS] = brw_imm_ud(3);
   srcs[TEX_LOGICAL_SRC_GRAD_COMPONENTS]  = brw_imm_ud(0);
            fs_inst *inst = bld.emit(op, dst, srcs, ARRAY_SIZE(srcs));
               }
      /**
   * Actual coherent framebuffer read implemented using the native render target
   * read message.  Requires SKL+.
   */
   static fs_inst *
   emit_coherent_fb_read(const fs_builder &bld, const fs_reg &dst, unsigned target)
   {
      assert(bld.shader->devinfo->ver >= 9);
   fs_inst *inst = bld.emit(FS_OPCODE_FB_READ_LOGICAL, dst);
   inst->target = target;
               }
      static fs_reg
   alloc_temporary(const fs_builder &bld, unsigned size, fs_reg *regs, unsigned n)
   {
      if (n && regs[0].file != BAD_FILE) {
            } else {
               for (unsigned i = 0; i < n; i++)
                  }
      static fs_reg
   alloc_frag_output(fs_visitor *v, unsigned location)
   {
      assert(v->stage == MESA_SHADER_FRAGMENT);
   const brw_wm_prog_key *const key =
         const unsigned l = GET_FIELD(location, BRW_NIR_FRAG_OUTPUT_LOCATION);
            if (i > 0 || (key->force_dual_color_blend && l == FRAG_RESULT_DATA1))
            else if (l == FRAG_RESULT_COLOR)
      return alloc_temporary(v->bld, 4, v->outputs,
         else if (l == FRAG_RESULT_DEPTH)
            else if (l == FRAG_RESULT_STENCIL)
            else if (l == FRAG_RESULT_SAMPLE_MASK)
            else if (l >= FRAG_RESULT_DATA0 &&
            return alloc_temporary(v->bld, 4,
         else
      }
      void
   fs_visitor::nir_emit_fs_intrinsic(const fs_builder &bld,
         {
               fs_reg dest;
   if (nir_intrinsic_infos[instr->intrinsic].has_dest)
            switch (instr->intrinsic) {
   case nir_intrinsic_load_front_face:
      bld.MOV(retype(dest, BRW_REGISTER_TYPE_D),
               case nir_intrinsic_load_sample_pos:
   case nir_intrinsic_load_sample_pos_or_center: {
      fs_reg sample_pos = nir_system_values[SYSTEM_VALUE_SAMPLE_POS];
   assert(sample_pos.file != BAD_FILE);
   dest.type = sample_pos.type;
   bld.MOV(dest, sample_pos);
   bld.MOV(offset(dest, bld, 1), offset(sample_pos, bld, 1));
               case nir_intrinsic_load_layer_id:
      dest.type = BRW_REGISTER_TYPE_UD;
   bld.MOV(dest, fetch_render_target_array_index(bld));
         case nir_intrinsic_is_helper_invocation:
      emit_is_helper_invocation(dest);
         case nir_intrinsic_load_helper_invocation:
   case nir_intrinsic_load_sample_mask_in:
   case nir_intrinsic_load_sample_id:
   case nir_intrinsic_load_frag_shading_rate: {
      gl_system_value sv = nir_system_value_from_intrinsic(instr->intrinsic);
   fs_reg val = nir_system_values[sv];
   assert(val.file != BAD_FILE);
   dest.type = val.type;
   bld.MOV(dest, val);
               case nir_intrinsic_store_output: {
      const fs_reg src = get_nir_src(instr->src[0]);
   const unsigned store_offset = nir_src_as_uint(instr->src[1]);
   const unsigned location = nir_intrinsic_base(instr) +
         const fs_reg new_dest = retype(alloc_frag_output(this, location),
            for (unsigned j = 0; j < instr->num_components; j++)
                              case nir_intrinsic_load_output: {
      const unsigned l = GET_FIELD(nir_intrinsic_base(instr),
         assert(l >= FRAG_RESULT_DATA0);
   const unsigned load_offset = nir_src_as_uint(instr->src[0]);
   const unsigned target = l - FRAG_RESULT_DATA0 + load_offset;
            if (reinterpret_cast<const brw_wm_prog_key *>(key)->coherent_fb_fetch)
         else
            for (unsigned j = 0; j < instr->num_components; j++) {
      bld.MOV(offset(dest, bld, j),
                           case nir_intrinsic_demote:
   case nir_intrinsic_discard:
   case nir_intrinsic_terminate:
   case nir_intrinsic_demote_if:
   case nir_intrinsic_discard_if:
   case nir_intrinsic_terminate_if: {
      /* We track our discarded pixels in f0.1/f1.0.  By predicating on it, we
   * can update just the flag bits that aren't yet discarded.  If there's
   * no condition, we emit a CMP of g0 != g0, so all currently executing
   * channels will get turned off.
   */
   fs_inst *cmp = NULL;
   if (instr->intrinsic == nir_intrinsic_demote_if ||
      instr->intrinsic == nir_intrinsic_discard_if ||
                  if (alu != NULL &&
      alu->op != nir_op_bcsel &&
   (devinfo->ver > 5 ||
   (alu->instr.pass_flags & BRW_NIR_BOOLEAN_MASK) != BRW_NIR_BOOLEAN_NEEDS_RESOLVE ||
   alu->op == nir_op_fneu32 || alu->op == nir_op_feq32 ||
   alu->op == nir_op_flt32 || alu->op == nir_op_fge32 ||
   alu->op == nir_op_ine32 || alu->op == nir_op_ieq32 ||
   alu->op == nir_op_ilt32 || alu->op == nir_op_ige32 ||
   alu->op == nir_op_ult32 || alu->op == nir_op_uge32)) {
   /* Re-emit the instruction that generated the Boolean value, but
   * do not store it.  Since this instruction will be conditional,
   * other instructions that want to use the real Boolean value may
   * get garbage.  This was a problem for piglit's fs-discard-exit-2
   * test.
   *
   * Ideally we'd detect that the instruction cannot have a
   * conditional modifier before emitting the instructions.  Alas,
   * that is nigh impossible.  Instead, we're going to assume the
   * instruction (or last instruction) generated can have a
   * conditional modifier.  If it cannot, fallback to the old-style
   * compare, and hope dead code elimination will clean up the
   * extra instructions generated.
                  cmp = (fs_inst *) instructions.get_tail();
   if (cmp->conditional_mod == BRW_CONDITIONAL_NONE) {
      if (cmp->can_do_cmod())
         else
      } else {
      /* The old sequence that would have been generated is,
   * basically, bool_result == false.  This is equivalent to
   * !bool_result, so negate the old modifier.
   */
                  if (cmp == NULL) {
      cmp = bld.CMP(bld.null_reg_f(), get_nir_src(instr->src[0]),
         } else {
      fs_reg some_reg = fs_reg(retype(brw_vec8_grf(0, 0),
                     cmp->predicate = BRW_PREDICATE_NORMAL;
            fs_inst *jump = bld.emit(BRW_OPCODE_HALT);
   jump->flag_subreg = sample_mask_flag_subreg(this);
            if (instr->intrinsic == nir_intrinsic_terminate ||
      instr->intrinsic == nir_intrinsic_terminate_if) {
      } else {
      /* Only jump when the whole quad is demoted.  For historical
   * reasons this is also used for discard.
   */
               if (devinfo->ver < 7)
      limit_dispatch_width(
                  case nir_intrinsic_load_input: {
      /* In Fragment Shaders load_input is used either for flat inputs or
   * per-primitive inputs.
   */
   assert(instr->def.bit_size == 32);
   unsigned base = nir_intrinsic_base(instr);
   unsigned comp = nir_intrinsic_component(instr);
                     if (wm_key->mesh_input == BRW_SOMETIMES) {
      assert(devinfo->verx10 >= 125);
   /* The FS payload gives us the viewport and layer clamped to valid
   * ranges, but the spec for gl_ViewportIndex and gl_Layer includes
   * the language:
   *   the fragment stage will read the same value written by the
   *   geometry stage, even if that value is out of range.
   *
   * Which is why these are normally passed as regular attributes.
   * This isn't tested anywhere except some GL-only piglit tests
   * though, so for the case where the FS may be used against either a
   * traditional pipeline or a mesh one, where the position of these
   * will change depending on the previous stage, read them from the
   * payload to simplify things until the requisite magic is in place.
   */
   if (base == VARYING_SLOT_LAYER || base == VARYING_SLOT_VIEWPORT) {
                     unsigned mask, shift_count;
   if (base == VARYING_SLOT_LAYER) {
      shift_count = 16;
      } else {
                        fs_reg vp_or_layer = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.AND(vp_or_layer, g1, brw_imm_ud(mask));
   fs_reg shifted_value = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.SHR(shifted_value, vp_or_layer, brw_imm_ud(shift_count));
   bld.MOV(offset(dest, bld, 0), retype(shifted_value, dest.type));
                           /* Special case fields in the VUE header */
   if (base == VARYING_SLOT_LAYER)
         else if (base == VARYING_SLOT_VIEWPORT)
            if (BITFIELD64_BIT(base) & nir->info.per_primitive_inputs) {
      assert(base != VARYING_SLOT_PRIMITIVE_INDICES);
   for (unsigned int i = 0; i < num_components; i++) {
      bld.MOV(offset(dest, bld, i),
         } else {
      for (unsigned int i = 0; i < num_components; i++) {
      bld.MOV(offset(dest, bld, i),
         }
               case nir_intrinsic_load_fs_input_interp_deltas: {
      assert(stage == MESA_SHADER_FRAGMENT);
   assert(nir_src_as_uint(instr->src[0]) == 0);
   fs_reg interp = interp_reg(nir_intrinsic_base(instr),
         dest.type = BRW_REGISTER_TYPE_F;
   bld.MOV(offset(dest, bld, 0), component(interp, 3));
   bld.MOV(offset(dest, bld, 1), component(interp, 1));
   bld.MOV(offset(dest, bld, 2), component(interp, 0));
               case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_sample: {
      /* Use the delta_xy values computed from the payload */
   enum brw_barycentric_mode bary = brw_barycentric_mode(instr);
   const fs_reg srcs[] = { offset(this->delta_xy[bary], bld, 0),
         bld.LOAD_PAYLOAD(dest, srcs, ARRAY_SIZE(srcs), 0);
               case nir_intrinsic_load_barycentric_at_sample: {
      const glsl_interp_mode interpolation =
            fs_reg msg_data;
   if (nir_src_is_const(instr->src[0])) {
         } else {
      const fs_reg sample_src = retype(get_nir_src(instr->src[0]),
         const fs_reg sample_id = bld.emit_uniformize(sample_src);
   msg_data = component(bld.group(8, 0).vgrf(BRW_REGISTER_TYPE_UD), 0);
               fs_reg flag_reg;
   struct brw_wm_prog_key *wm_prog_key = (struct brw_wm_prog_key *) key;
   if (wm_prog_key->multisample_fbo == BRW_SOMETIMES) {
               check_dynamic_msaa_flag(bld.exec_all().group(8, 0),
                           emit_pixel_interpolater_send(bld,
                              FS_OPCODE_INTERPOLATE_AT_SAMPLE,
                  case nir_intrinsic_load_barycentric_at_offset: {
      const glsl_interp_mode interpolation =
                     if (const_offset) {
      assert(nir_src_bit_size(instr->src[0]) == 32);
                  emit_pixel_interpolater_send(bld,
                              FS_OPCODE_INTERPOLATE_AT_SHARED_OFFSET,
   } else {
      fs_reg src = retype(get_nir_src(instr->src[0]), BRW_REGISTER_TYPE_D);
   const enum opcode opcode = FS_OPCODE_INTERPOLATE_AT_PER_SLOT_OFFSET;
   emit_pixel_interpolater_send(bld,
                              opcode,
   }
               case nir_intrinsic_load_frag_coord:
      emit_fragcoord_interpolation(dest);
         case nir_intrinsic_load_interpolated_input: {
      assert(instr->src[0].ssa &&
         nir_intrinsic_instr *bary_intrinsic =
         nir_intrinsic_op bary_intrin = bary_intrinsic->intrinsic;
   enum glsl_interp_mode interp_mode =
                  if (bary_intrin == nir_intrinsic_load_barycentric_at_offset ||
      bary_intrin == nir_intrinsic_load_barycentric_at_sample) {
   /* Use the result of the PI message. */
      } else {
      /* Use the delta_xy values computed from the payload */
   enum brw_barycentric_mode bary = brw_barycentric_mode(bary_intrinsic);
               for (unsigned int i = 0; i < instr->num_components; i++) {
      fs_reg interp =
      component(interp_reg(nir_intrinsic_base(instr),
                     if (devinfo->ver < 6 && interp_mode == INTERP_MODE_SMOOTH) {
      fs_reg tmp = vgrf(glsl_type::float_type);
   bld.emit(FS_OPCODE_LINTERP, tmp, dst_xy, interp);
      } else {
            }
               default:
      nir_emit_intrinsic(bld, instr);
         }
      void
   fs_visitor::nir_emit_cs_intrinsic(const fs_builder &bld,
         {
      assert(gl_shader_stage_uses_workgroup(stage));
            fs_reg dest;
   if (nir_intrinsic_infos[instr->intrinsic].has_dest)
            switch (instr->intrinsic) {
   case nir_intrinsic_barrier:
      if (nir_intrinsic_memory_scope(instr) != SCOPE_NONE)
         if (nir_intrinsic_execution_scope(instr) == SCOPE_WORKGROUP) {
      /* The whole workgroup fits in a single HW thread, so all the
   * invocations are already executed lock-step.  Instead of an actual
   * barrier just emit a scheduling fence, that will generate no code.
   */
   if (!nir->info.workgroup_size_variable &&
      workgroup_size() <= dispatch_width) {
   bld.exec_all().group(1, 0).emit(FS_OPCODE_SCHEDULING_FENCE);
               emit_barrier();
      }
         case nir_intrinsic_load_subgroup_id:
      cs_payload().load_subgroup_id(bld, dest);
         case nir_intrinsic_load_local_invocation_id: {
      fs_reg val = nir_system_values[SYSTEM_VALUE_LOCAL_INVOCATION_ID];
   assert(val.file != BAD_FILE);
   dest.type = val.type;
   for (unsigned i = 0; i < 3; i++)
                     case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_workgroup_id_zero_base: {
      fs_reg val = nir_system_values[SYSTEM_VALUE_WORKGROUP_ID];
   assert(val.file != BAD_FILE);
   dest.type = val.type;
   for (unsigned i = 0; i < 3; i++)
                     case nir_intrinsic_load_num_workgroups: {
                        fs_reg srcs[SURFACE_LOGICAL_NUM_SRCS];
   srcs[SURFACE_LOGICAL_SRC_SURFACE] = brw_imm_ud(0);
   srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] = brw_imm_ud(1);
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(3); /* num components */
   srcs[SURFACE_LOGICAL_SRC_ADDRESS] = brw_imm_ud(0);
   srcs[SURFACE_LOGICAL_SRC_ALLOW_SAMPLE_MASK] = brw_imm_ud(0);
   fs_inst *inst =
      bld.emit(SHADER_OPCODE_UNTYPED_SURFACE_READ_LOGICAL,
      inst->size_written = 3 * dispatch_width * 4;
               case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
      nir_emit_surface_atomic(bld, instr, brw_imm_ud(GFX7_BTI_SLM),
               case nir_intrinsic_load_shared: {
               const unsigned bit_size = instr->def.bit_size;
   fs_reg srcs[SURFACE_LOGICAL_NUM_SRCS];
            fs_reg addr = get_nir_src(instr->src[0]);
   int base = nir_intrinsic_base(instr);
   if (base) {
      fs_reg addr_off = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   bld.ADD(addr_off, addr, brw_imm_d(base));
      } else {
                  srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] = brw_imm_ud(1);
            /* Make dest unsigned because that's what the temporary will be */
            /* Read the vector */
   assert(bit_size <= 32);
   assert(nir_intrinsic_align(instr) > 0);
   if (bit_size == 32 &&
      nir_intrinsic_align(instr) >= 4) {
   assert(instr->def.num_components <= 4);
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(instr->num_components);
   fs_inst *inst =
      bld.emit(SHADER_OPCODE_UNTYPED_SURFACE_READ_LOGICAL,
         } else {
                     fs_reg read_result = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.emit(SHADER_OPCODE_BYTE_SCATTERED_READ_LOGICAL,
            }
               case nir_intrinsic_store_shared: {
               const unsigned bit_size = nir_src_bit_size(instr->src[0]);
   fs_reg srcs[SURFACE_LOGICAL_NUM_SRCS];
            fs_reg addr = get_nir_src(instr->src[1]);
   int base = nir_intrinsic_base(instr);
   if (base) {
      fs_reg addr_off = bld.vgrf(BRW_REGISTER_TYPE_UD, 1);
   bld.ADD(addr_off, addr, brw_imm_d(base));
      } else {
                  srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] = brw_imm_ud(1);
   /* No point in masking with sample mask, here we're handling compute
   * intrinsics.
   */
            fs_reg data = get_nir_src(instr->src[0]);
            assert(bit_size <= 32);
   assert(nir_intrinsic_write_mask(instr) ==
         assert(nir_intrinsic_align(instr) > 0);
   if (bit_size == 32 &&
      nir_intrinsic_align(instr) >= 4) {
   assert(nir_src_num_components(instr->src[0]) <= 4);
   srcs[SURFACE_LOGICAL_SRC_DATA] = data;
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(instr->num_components);
   bld.emit(SHADER_OPCODE_UNTYPED_SURFACE_WRITE_LOGICAL,
      } else {
                                    bld.emit(SHADER_OPCODE_BYTE_SCATTERED_WRITE_LOGICAL,
      }
               case nir_intrinsic_load_workgroup_size: {
      /* Should have been lowered by brw_nir_lower_cs_intrinsics() or
   * crocus/iris_setup_uniforms() for the variable group size case.
   */
   unreachable("Should have been lowered");
               default:
      nir_emit_intrinsic(bld, instr);
         }
      static void
   emit_rt_lsc_fence(const fs_builder &bld,
               {
               const fs_builder ubld = bld.exec_all().group(8, 0);
   fs_reg tmp = ubld.vgrf(BRW_REGISTER_TYPE_UD);
   fs_inst *send = ubld.emit(SHADER_OPCODE_SEND, tmp,
                     send->sfid = GFX12_SFID_UGM;
   send->desc = lsc_fence_msg_desc(devinfo, scope, flush_type, true);
   send->mlen = reg_unit(devinfo); /* g0 header */
   send->ex_mlen = 0;
   /* Temp write for scheduling */
   send->size_written = REG_SIZE * reg_unit(devinfo);
               }
         void
   fs_visitor::nir_emit_bs_intrinsic(const fs_builder &bld,
         {
      assert(brw_shader_stage_is_bindless(stage));
            fs_reg dest;
   if (nir_intrinsic_infos[instr->intrinsic].has_dest)
            switch (instr->intrinsic) {
   case nir_intrinsic_load_btd_global_arg_addr_intel:
      bld.MOV(dest, retype(payload.global_arg_ptr, dest.type));
         case nir_intrinsic_load_btd_local_arg_addr_intel:
      bld.MOV(dest, retype(payload.local_arg_ptr, dest.type));
         case nir_intrinsic_load_btd_shader_type_intel:
      payload.load_shader_type(bld, dest);
         default:
      nir_emit_intrinsic(bld, instr);
         }
      static fs_reg
   brw_nir_reduction_op_identity(const fs_builder &bld,
         {
      nir_const_value value = nir_alu_binop_identity(op, type_sz(type) * 8);
   switch (type_sz(type)) {
   case 1:
      if (type == BRW_REGISTER_TYPE_UB) {
         } else {
      assert(type == BRW_REGISTER_TYPE_B);
         case 2:
         case 4:
         case 8:
      if (type == BRW_REGISTER_TYPE_DF)
         else
      default:
            }
      static opcode
   brw_op_for_nir_reduction_op(nir_op op)
   {
      switch (op) {
   case nir_op_iadd: return BRW_OPCODE_ADD;
   case nir_op_fadd: return BRW_OPCODE_ADD;
   case nir_op_imul: return BRW_OPCODE_MUL;
   case nir_op_fmul: return BRW_OPCODE_MUL;
   case nir_op_imin: return BRW_OPCODE_SEL;
   case nir_op_umin: return BRW_OPCODE_SEL;
   case nir_op_fmin: return BRW_OPCODE_SEL;
   case nir_op_imax: return BRW_OPCODE_SEL;
   case nir_op_umax: return BRW_OPCODE_SEL;
   case nir_op_fmax: return BRW_OPCODE_SEL;
   case nir_op_iand: return BRW_OPCODE_AND;
   case nir_op_ior:  return BRW_OPCODE_OR;
   case nir_op_ixor: return BRW_OPCODE_XOR;
   default:
            }
      static brw_conditional_mod
   brw_cond_mod_for_nir_reduction_op(nir_op op)
   {
      switch (op) {
   case nir_op_iadd: return BRW_CONDITIONAL_NONE;
   case nir_op_fadd: return BRW_CONDITIONAL_NONE;
   case nir_op_imul: return BRW_CONDITIONAL_NONE;
   case nir_op_fmul: return BRW_CONDITIONAL_NONE;
   case nir_op_imin: return BRW_CONDITIONAL_L;
   case nir_op_umin: return BRW_CONDITIONAL_L;
   case nir_op_fmin: return BRW_CONDITIONAL_L;
   case nir_op_imax: return BRW_CONDITIONAL_GE;
   case nir_op_umax: return BRW_CONDITIONAL_GE;
   case nir_op_fmax: return BRW_CONDITIONAL_GE;
   case nir_op_iand: return BRW_CONDITIONAL_NONE;
   case nir_op_ior:  return BRW_CONDITIONAL_NONE;
   case nir_op_ixor: return BRW_CONDITIONAL_NONE;
   default:
            }
      struct rebuild_resource {
      unsigned idx;
      };
      static bool
   add_rebuild_src(nir_src *src, void *state)
   {
               for (nir_def *def : res->array) {
      if (def == src->ssa)
               nir_foreach_src(src->ssa->parent_instr, add_rebuild_src, state);
   res->array.push_back(src->ssa);
      }
      fs_reg
   fs_visitor::try_rebuild_resource(const brw::fs_builder &bld, nir_def *resource_def)
   {
      /* Create a build at the location of the resource_intel intrinsic */
            struct rebuild_resource resources = {};
            if (!nir_foreach_src(resource_def->parent_instr,
                        if (resources.array.size() == 1) {
               if (def->parent_instr->type == nir_instr_type_load_const) {
      nir_load_const_instr *load_const =
            } else {
      assert(def->parent_instr->type == nir_instr_type_intrinsic &&
         (nir_instr_as_intrinsic(def->parent_instr)->intrinsic ==
   nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(def->parent_instr);
   unsigned base_offset = nir_intrinsic_base(intrin);
   unsigned load_offset = nir_src_as_uint(intrin->src[0]);
   fs_reg src(UNIFORM, base_offset / 4, BRW_REGISTER_TYPE_UD);
   src.offset = load_offset + base_offset % 4;
                  for (unsigned i = 0; i < resources.array.size(); i++) {
               nir_instr *instr = def->parent_instr;
   switch (instr->type) {
   case nir_instr_type_load_const: {
      nir_load_const_instr *load_const =
         fs_reg dst = ubld8.vgrf(BRW_REGISTER_TYPE_UD);
   nir_resource_insts[def->index] =
                     case nir_instr_type_alu: {
               if (nir_op_infos[alu->op].num_inputs == 2) {
      if (alu->src[0].swizzle[0] != 0 ||
      alu->src[1].swizzle[0] != 0)
   } else if (nir_op_infos[alu->op].num_inputs == 3) {
      if (alu->src[0].swizzle[0] != 0 ||
      alu->src[1].swizzle[0] != 0 ||
   alu->src[2].swizzle[0] != 0)
   } else {
      /* Not supported ALU input count */
               switch (alu->op) {
   case nir_op_iadd: {
      fs_reg dst = ubld8.vgrf(BRW_REGISTER_TYPE_UD);
   fs_reg src0 = nir_resource_insts[alu->src[0].src.ssa->index]->dst;
   fs_reg src1 = nir_resource_insts[alu->src[1].src.ssa->index]->dst;
   assert(src0.file != BAD_FILE && src1.file != BAD_FILE);
   assert(src0.type == BRW_REGISTER_TYPE_UD);
   nir_resource_insts[def->index] =
      ubld8.ADD(dst,
               }
   case nir_op_iadd3: {
      fs_reg dst = ubld8.vgrf(BRW_REGISTER_TYPE_UD);
   fs_reg src0 = nir_resource_insts[alu->src[0].src.ssa->index]->dst;
   fs_reg src1 = nir_resource_insts[alu->src[1].src.ssa->index]->dst;
   fs_reg src2 = nir_resource_insts[alu->src[2].src.ssa->index]->dst;
   assert(src0.file != BAD_FILE && src1.file != BAD_FILE && src2.file != BAD_FILE);
   assert(src0.type == BRW_REGISTER_TYPE_UD);
   nir_resource_insts[def->index] =
      ubld8.ADD3(dst,
            src1.file == IMM ? src1 : src0,
      }
   case nir_op_ushr: {
      fs_reg dst = ubld8.vgrf(BRW_REGISTER_TYPE_UD);
   fs_reg src0 = nir_resource_insts[alu->src[0].src.ssa->index]->dst;
   fs_reg src1 = nir_resource_insts[alu->src[1].src.ssa->index]->dst;
   assert(src0.file != BAD_FILE && src1.file != BAD_FILE);
   assert(src0.type == BRW_REGISTER_TYPE_UD);
   nir_resource_insts[def->index] = ubld8.SHR(dst, src0, src1);
      }
   case nir_op_ishl: {
      fs_reg dst = ubld8.vgrf(BRW_REGISTER_TYPE_UD);
   fs_reg src0 = nir_resource_insts[alu->src[0].src.ssa->index]->dst;
   fs_reg src1 = nir_resource_insts[alu->src[1].src.ssa->index]->dst;
   assert(src0.file != BAD_FILE && src1.file != BAD_FILE);
   assert(src0.type == BRW_REGISTER_TYPE_UD);
   nir_resource_insts[def->index] = ubld8.SHL(dst, src0, src1);
      }
   case nir_op_mov: {
         }
   default:
         }
               case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_resource_intel:
      nir_resource_insts[def->index] =
               case nir_intrinsic_load_uniform: {
                     unsigned base_offset = nir_intrinsic_base(intrin);
   unsigned load_offset = nir_src_as_uint(intrin->src[0]);
   fs_reg dst = ubld8.vgrf(BRW_REGISTER_TYPE_UD);
   fs_reg src(UNIFORM, base_offset / 4, BRW_REGISTER_TYPE_UD);
   src.offset = load_offset + base_offset % 4;
   nir_resource_insts[def->index] = ubld8.MOV(dst, src);
               default:
         }
               default:
                  if (nir_resource_insts[def->index] == NULL)
               assert(nir_resource_insts[resource_def->index] != NULL);
      }
      fs_reg
   fs_visitor::get_nir_image_intrinsic_image(const brw::fs_builder &bld,
         {
      if (is_resource_src(instr->src[0])) {
      fs_reg surf_index = get_resource_nir_src(instr->src[0]);
   if (surf_index.file != BAD_FILE)
               fs_reg image = retype(get_nir_src_imm(instr->src[0]), BRW_REGISTER_TYPE_UD);
               }
      fs_reg
   fs_visitor::get_nir_buffer_intrinsic_index(const brw::fs_builder &bld,
         {
      /* SSBO stores are weird in that their index is in src[1] */
   const bool is_store =
      instr->intrinsic == nir_intrinsic_store_ssbo ||
               if (nir_src_is_const(src)) {
         } else if (is_resource_src(src)) {
      fs_reg surf_index = get_resource_nir_src(src);
   if (surf_index.file != BAD_FILE)
      }
      }
      /**
   * The offsets we get from NIR act as if each SIMD channel has it's own blob
   * of contiguous space.  However, if we actually place each SIMD channel in
   * it's own space, we end up with terrible cache performance because each SIMD
   * channel accesses a different cache line even when they're all accessing the
   * same byte offset.  To deal with this problem, we swizzle the address using
   * a simple algorithm which ensures that any time a SIMD message reads or
   * writes the same address, it's all in the same cache line.  We have to keep
   * the bottom two bits fixed so that we can read/write up to a dword at a time
   * and the individual element is contiguous.  We do this by splitting the
   * address as follows:
   *
   *    31                             4-6           2          0
   *    +-------------------------------+------------+----------+
   *    |        Hi address bits        | chan index | addr low |
   *    +-------------------------------+------------+----------+
   *
   * In other words, the bottom two address bits stay, and the top 30 get
   * shifted up so that we can stick the SIMD channel index in the middle.  This
   * way, we can access 8, 16, or 32-bit elements and, when accessing a 32-bit
   * at the same logical offset, the scratch read/write instruction acts on
   * continuous elements and we get good cache locality.
   */
   fs_reg
   fs_visitor::swizzle_nir_scratch_addr(const brw::fs_builder &bld,
               {
      const fs_reg &chan_index =
                  fs_reg addr = bld.vgrf(BRW_REGISTER_TYPE_UD);
   if (in_dwords) {
      /* In this case, we know the address is aligned to a DWORD and we want
   * the final address in DWORDs.
   */
   bld.SHL(addr, nir_addr, brw_imm_ud(chan_index_bits - 2));
      } else {
      /* This case substantially more annoying because we have to pay
   * attention to those pesky two bottom bits.
   */
   fs_reg addr_hi = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.AND(addr_hi, nir_addr, brw_imm_ud(~0x3u));
   bld.SHL(addr_hi, addr_hi, brw_imm_ud(chan_index_bits));
   fs_reg chan_addr = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.SHL(chan_addr, chan_index, brw_imm_ud(2));
   bld.AND(addr, nir_addr, brw_imm_ud(0x3u));
   bld.OR(addr, addr, addr_hi);
      }
      }
      static unsigned
   choose_oword_block_size_dwords(const struct intel_device_info *devinfo,
         {
      unsigned block;
   if (devinfo->has_lsc && dwords >= 64) {
         } else if (dwords >= 32) {
         } else if (dwords >= 16) {
         } else {
         }
   assert(block <= dwords);
      }
      static void
   increment_a64_address(const fs_builder &bld, fs_reg address, uint32_t v)
   {
      if (bld.shader->devinfo->has_64bit_int) {
         } else {
      fs_reg low = retype(address, BRW_REGISTER_TYPE_UD);
            /* Add low and if that overflows, add carry to high. */
   bld.ADD(low, low, brw_imm_ud(v))->conditional_mod = BRW_CONDITIONAL_O;
         }
      static fs_reg
   emit_fence(const fs_builder &bld, enum opcode opcode,
               {
      assert(opcode == SHADER_OPCODE_INTERLOCK ||
            fs_reg dst = bld.vgrf(BRW_REGISTER_TYPE_UD);
   fs_inst *fence = bld.emit(opcode, dst, brw_vec8_grf(0, 0),
               fence->sfid = sfid;
               }
      static uint32_t
   lsc_fence_descriptor_for_intrinsic(const struct intel_device_info *devinfo,
         {
               enum lsc_fence_scope scope = LSC_FENCE_LOCAL;
            if (nir_intrinsic_has_memory_scope(instr)) {
      switch (nir_intrinsic_memory_scope(instr)) {
   case SCOPE_DEVICE:
   case SCOPE_QUEUE_FAMILY:
      scope = LSC_FENCE_TILE;
   flush_type = LSC_FLUSH_TYPE_EVICT;
      case SCOPE_WORKGROUP:
      scope = LSC_FENCE_THREADGROUP;
      case SCOPE_SHADER_CALL:
   case SCOPE_INVOCATION:
   case SCOPE_SUBGROUP:
   case SCOPE_NONE:
            } else {
      /* No scope defined. */
   scope = LSC_FENCE_TILE;
      }
      }
      void
   fs_visitor::nir_emit_intrinsic(const fs_builder &bld, nir_intrinsic_instr *instr)
   {
      /* We handle this as a special case */
   if (instr->intrinsic == nir_intrinsic_decl_reg) {
      assert(nir_intrinsic_num_array_elems(instr) == 0);
   unsigned bit_size = nir_intrinsic_bit_size(instr);
   unsigned num_components = nir_intrinsic_num_components(instr);
   const brw_reg_type reg_type =
      brw_reg_type_from_bit_size(bit_size, bit_size == 8 ?
               /* Re-use the destination's slot in the table for the register */
   nir_ssa_values[instr->def.index] =
                     fs_reg dest;
   if (nir_intrinsic_infos[instr->intrinsic].has_dest)
            switch (instr->intrinsic) {
   case nir_intrinsic_resource_intel:
      nir_ssa_bind_infos[instr->def.index].valid = true;
   nir_ssa_bind_infos[instr->def.index].bindless =
      (nir_intrinsic_resource_access_intel(instr) &
      nir_ssa_bind_infos[instr->def.index].block =
         nir_ssa_bind_infos[instr->def.index].set =
         nir_ssa_bind_infos[instr->def.index].binding =
            if (nir_intrinsic_resource_access_intel(instr) &
      nir_resource_intel_non_uniform) {
      } else {
      nir_resource_values[instr->def.index] =
      }
   nir_ssa_values[instr->def.index] =
               case nir_intrinsic_load_reg:
   case nir_intrinsic_store_reg:
      /* Nothing to do with these. */
         case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap: {
      /* Get some metadata from the image intrinsic. */
                     switch (instr->intrinsic) {
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
      srcs[SURFACE_LOGICAL_SRC_SURFACE] =
               default:
      /* Bindless */
   srcs[SURFACE_LOGICAL_SRC_SURFACE_HANDLE] =
                     srcs[SURFACE_LOGICAL_SRC_ADDRESS] = get_nir_src(instr->src[1]);
   srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] =
            /* Emit an image load, store or atomic op. */
   if (instr->intrinsic == nir_intrinsic_image_load ||
      instr->intrinsic == nir_intrinsic_bindless_image_load) {
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(instr->num_components);
   srcs[SURFACE_LOGICAL_SRC_ALLOW_SAMPLE_MASK] = brw_imm_ud(0);
   fs_inst *inst =
      bld.emit(SHADER_OPCODE_TYPED_SURFACE_READ_LOGICAL,
         } else if (instr->intrinsic == nir_intrinsic_image_store ||
            srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(instr->num_components);
   srcs[SURFACE_LOGICAL_SRC_DATA] = get_nir_src(instr->src[3]);
   srcs[SURFACE_LOGICAL_SRC_ALLOW_SAMPLE_MASK] = brw_imm_ud(1);
   bld.emit(SHADER_OPCODE_TYPED_SURFACE_WRITE_LOGICAL,
      } else {
      unsigned num_srcs = info->num_srcs;
   int op = lsc_aop_for_nir_intrinsic(instr);
   if (op == LSC_OP_ATOMIC_INC || op == LSC_OP_ATOMIC_DEC) {
      assert(num_srcs == 4);
                        fs_reg data;
   if (num_srcs >= 4)
         if (num_srcs >= 5) {
      fs_reg tmp = bld.vgrf(data.type, 2);
   fs_reg sources[2] = { data, get_nir_src(instr->src[4]) };
   bld.LOAD_PAYLOAD(tmp, sources, 2, 0);
      }
                  bld.emit(SHADER_OPCODE_TYPED_ATOMIC_LOGICAL,
      }
               case nir_intrinsic_image_size:
   case nir_intrinsic_bindless_image_size: {
      /* Cube image sizes should have previously been lowered to a 2D array */
            /* Unlike the [un]typed load and store opcodes, the TXS that this turns
   * into will handle the binding table index for us in the geneerator.
   * Incidentally, this means that we can handle bindless with exactly the
   * same code.
   */
   fs_reg image = retype(get_nir_src_imm(instr->src[0]),
                           fs_reg srcs[TEX_LOGICAL_NUM_SRCS];
   if (instr->intrinsic == nir_intrinsic_image_size)
         else
         srcs[TEX_LOGICAL_SRC_SAMPLER] = brw_imm_d(0);
   srcs[TEX_LOGICAL_SRC_COORD_COMPONENTS] = brw_imm_d(0);
   srcs[TEX_LOGICAL_SRC_GRAD_COMPONENTS] = brw_imm_d(0);
            /* Since the image size is always uniform, we can just emit a SIMD8
   * query instruction and splat the result out.
   */
            fs_reg tmp = ubld.vgrf(BRW_REGISTER_TYPE_UD, 4);
   fs_inst *inst = ubld.emit(SHADER_OPCODE_IMAGE_SIZE_LOGICAL,
                  for (unsigned c = 0; c < instr->def.num_components; ++c) {
      bld.MOV(offset(retype(dest, tmp.type), bld, c),
      }
               case nir_intrinsic_image_load_raw_intel: {
      fs_reg srcs[SURFACE_LOGICAL_NUM_SRCS];
   srcs[SURFACE_LOGICAL_SRC_SURFACE] =
         srcs[SURFACE_LOGICAL_SRC_ADDRESS] = get_nir_src(instr->src[1]);
   srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] = brw_imm_ud(1);
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(instr->num_components);
            fs_inst *inst =
      bld.emit(SHADER_OPCODE_UNTYPED_SURFACE_READ_LOGICAL,
      inst->size_written = instr->num_components * dispatch_width * 4;
               case nir_intrinsic_image_store_raw_intel: {
      fs_reg srcs[SURFACE_LOGICAL_NUM_SRCS];
   srcs[SURFACE_LOGICAL_SRC_SURFACE] =
         srcs[SURFACE_LOGICAL_SRC_ADDRESS] = get_nir_src(instr->src[1]);
   srcs[SURFACE_LOGICAL_SRC_DATA] = get_nir_src(instr->src[2]);
   srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] = brw_imm_ud(1);
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(instr->num_components);
            bld.emit(SHADER_OPCODE_UNTYPED_SURFACE_WRITE_LOGICAL,
                     case nir_intrinsic_barrier:
   case nir_intrinsic_begin_invocation_interlock:
   case nir_intrinsic_end_invocation_interlock: {
      bool ugm_fence, slm_fence, tgm_fence, urb_fence;
            /* Handling interlock intrinsics here will allow the logic for IVB
   * render cache (see below) to be reused.
            switch (instr->intrinsic) {
   case nir_intrinsic_barrier: {
      /* Note we only care about the memory part of the
   * barrier.  The execution part will be taken care
   * of by the stage specific intrinsic handler functions.
   */
   nir_variable_mode modes = nir_intrinsic_memory_modes(instr);
   ugm_fence = modes & (nir_var_mem_ssbo | nir_var_mem_global);
   slm_fence = modes & nir_var_mem_shared;
   tgm_fence = modes & nir_var_image;
   urb_fence = modes & (nir_var_shader_out | nir_var_mem_task_payload);
   if (nir_intrinsic_memory_scope(instr) != SCOPE_NONE)
                     case nir_intrinsic_begin_invocation_interlock:
      /* For beginInvocationInterlockARB(), we will generate a memory fence
   * but with a different opcode so that generator can pick SENDC
   * instead of SEND.
   */
   assert(stage == MESA_SHADER_FRAGMENT);
   ugm_fence = tgm_fence = true;
   slm_fence = urb_fence = false;
               case nir_intrinsic_end_invocation_interlock:
      /* For endInvocationInterlockARB(), we need to insert a memory fence which
   * stalls in the shader until the memory transactions prior to that
   * fence are complete.  This ensures that the shader does not end before
   * any writes from its critical section have landed.  Otherwise, you can
   * end up with a case where the next invocation on that pixel properly
   * stalls for previous FS invocation on its pixel to complete but
   * doesn't actually wait for the dataport memory transactions from that
   * thread to land before submitting its own.
   */
   assert(stage == MESA_SHADER_FRAGMENT);
   ugm_fence = tgm_fence = true;
   slm_fence = urb_fence = false;
               default:
                  if (opcode == BRW_OPCODE_NOP)
            if (nir->info.shared_size > 0) {
         } else {
                  /* If the workgroup fits in a single HW thread, the messages for SLM are
   * processed in-order and the shader itself is already synchronized so
   * the memory fence is not necessary.
   *
   * TODO: Check if applies for many HW threads sharing same Data Port.
   */
   if (!nir->info.workgroup_size_variable &&
                  switch (stage) {
      case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TASK:
   case MESA_SHADER_MESH:
         default:
      urb_fence = false;
            unsigned fence_regs_count = 0;
                     /* A memory barrier with acquire semantics requires us to
   * guarantee that memory operations of the specified storage
   * class sequenced-after the barrier aren't reordered before the
   * barrier, nor before any previous atomic operation
   * sequenced-before the barrier which may be synchronizing this
   * acquire barrier with a prior release sequence.
   *
   * In order to guarantee the latter we must make sure that any
   * such previous operation has completed execution before
   * invalidating the relevant caches, since otherwise some cache
   * could be polluted by a concurrent thread after its
   * invalidation but before the previous atomic completes, which
   * could lead to a violation of the expected memory ordering if
   * a subsequent memory read hits the polluted cacheline, which
   * would return a stale value read from memory before the
   * completion of the atomic sequenced-before the barrier.
   *
   * This ordering inversion can be avoided trivially if the
   * operations we need to order are all handled by a single
   * in-order cache, since the flush implied by the memory fence
   * occurs after any pending operations have completed, however
   * that doesn't help us when dealing with multiple caches
   * processing requests out of order, in which case we need to
   * explicitly stall the EU until any pending memory operations
   * have executed.
   *
   * Note that that might be somewhat heavy handed in some cases.
   * In particular when this memory fence was inserted by
   * spirv_to_nir() lowering an atomic with acquire semantics into
   * an atomic+barrier sequence we could do a better job by
   * synchronizing with respect to that one atomic *only*, but
   * that would require additional information not currently
   * available to the backend.
   *
   * XXX - Use an alternative workaround on IVB and ICL, since
   *       SYNC.ALLWR is only available on Gfx12+.
   */
   if (devinfo->ver >= 12 &&
      (!nir_intrinsic_has_memory_scope(instr) ||
   (nir_intrinsic_memory_semantics(instr) & NIR_MEMORY_ACQUIRE))) {
   ubld.exec_all().group(1, 0).emit(
               if (devinfo->has_lsc) {
      assert(devinfo->verx10 >= 125);
   uint32_t desc =
         if (ugm_fence) {
      fence_regs[fence_regs_count++] =
      emit_fence(ubld, opcode, GFX12_SFID_UGM, desc,
                  if (tgm_fence) {
      fence_regs[fence_regs_count++] =
      emit_fence(ubld, opcode, GFX12_SFID_TGM, desc,
                  if (slm_fence) {
      assert(opcode == SHADER_OPCODE_MEMORY_FENCE);
   if (intel_needs_workaround(devinfo, 14014063774)) {
      /* Wa_14014063774
   *
   * Before SLM fence compiler needs to insert SYNC.ALLWR in order
   * to avoid the SLM data race.
   */
   ubld.exec_all().group(1, 0).emit(
      BRW_OPCODE_SYNC, ubld.null_reg_ud(),
   }
   fence_regs[fence_regs_count++] =
      emit_fence(ubld, opcode, GFX12_SFID_SLM, desc,
                  if (urb_fence) {
      assert(opcode == SHADER_OPCODE_MEMORY_FENCE);
   fence_regs[fence_regs_count++] =
      emit_fence(ubld, opcode, BRW_SFID_URB, desc,
            } else if (devinfo->ver >= 11) {
      if (tgm_fence || ugm_fence || urb_fence) {
      fence_regs[fence_regs_count++] =
      emit_fence(ubld, opcode, GFX7_SFID_DATAPORT_DATA_CACHE, 0,
                  if (slm_fence) {
      assert(opcode == SHADER_OPCODE_MEMORY_FENCE);
   fence_regs[fence_regs_count++] =
      emit_fence(ubld, opcode, GFX7_SFID_DATAPORT_DATA_CACHE, 0,
            } else {
      /* Prior to Icelake, they're all lumped into a single cache except on
   * Ivy Bridge and Bay Trail where typed messages actually go through
   * the render cache.  There, we need both fences because we may
   * access storage images as either typed or untyped.
                  /* Simulation also complains on Gfx9 if we do not enable commit.
   */
   const bool commit_enable = render_fence ||
                  if (tgm_fence || ugm_fence || slm_fence || urb_fence) {
      fence_regs[fence_regs_count++] =
                     if (render_fence) {
      fence_regs[fence_regs_count++] =
      emit_fence(ubld, opcode, GFX6_SFID_DATAPORT_RENDER_CACHE, 0,
                        /* Be conservative in Gen11+ and always stall in a fence.  Since
   * there are two different fences, and shader might want to
   * synchronize between them.
   *
   * TODO: Use scope and visibility information for the barriers from NIR
   * to make a better decision on whether we need to stall.
   */
            /* There are four cases where we want to insert a stall:
   *
   *  1. If we're a nir_intrinsic_end_invocation_interlock.  This is
   *     required to ensure that the shader EOT doesn't happen until
   *     after the fence returns.  Otherwise, we might end up with the
   *     next shader invocation for that pixel not respecting our fence
   *     because it may happen on a different HW thread.
   *
   *  2. If we have multiple fences.  This is required to ensure that
   *     they all complete and nothing gets weirdly out-of-order.
   *
   *  3. If we have no fences.  In this case, we need at least a
   *     scheduling barrier to keep the compiler from moving things
   *     around in an invalid way.
   *
   *  4. On Gen11+ and platforms with LSC, we have multiple fence types,
   *     without further information about the fence, we need to force a
   *     stall.
   */
   if (instr->intrinsic == nir_intrinsic_end_invocation_interlock ||
      fence_regs_count != 1 || devinfo->has_lsc || force_stall) {
   ubld.exec_all().group(1, 0).emit(
      FS_OPCODE_SCHEDULING_FENCE, ubld.null_reg_ud(),
                        case nir_intrinsic_shader_clock: {
      /* We cannot do anything if there is an event, so ignore it for now */
   const fs_reg shader_clock = get_timestamp(bld);
   const fs_reg srcs[] = { component(shader_clock, 0),
         bld.LOAD_PAYLOAD(dest, srcs, ARRAY_SIZE(srcs), 0);
               case nir_intrinsic_load_reloc_const_intel: {
               /* Emit the reloc in the smallest SIMD size to limit register usage. */
   const fs_builder ubld = bld.exec_all().group(1, 0);
   fs_reg small_dest = ubld.vgrf(dest.type);
   ubld.UNDEF(small_dest);
   ubld.exec_all().group(1, 0).emit(SHADER_OPCODE_MOV_RELOC_IMM,
            /* Copy propagation will get rid of this MOV. */
   bld.MOV(dest, component(small_dest, 0));
               case nir_intrinsic_load_uniform: {
      /* Offsets are in bytes but they should always aligned to
   * the type size
   */
   unsigned base_offset = nir_intrinsic_base(instr);
                     if (nir_src_is_const(instr->src[0])) {
      unsigned load_offset = nir_src_as_uint(instr->src[0]);
   assert(load_offset % type_sz(dest.type) == 0);
   /* The base offset can only handle 32-bit units, so for 16-bit
   * data take the modulo of the offset with 4 bytes and add it to
   * the offset to read from within the source register.
                  for (unsigned j = 0; j < instr->num_components; j++) {
            } else {
                     /* We need to pass a size to the MOV_INDIRECT but we don't want it to
   * go past the end of the uniform.  In order to keep the n'th
   * component from running past, we subtract off the size of all but
   * one component of the vector.
   */
   assert(nir_intrinsic_range(instr) >=
                                       if (type_sz(dest.type) != 8 || supports_64bit_indirects) {
      for (unsigned j = 0; j < instr->num_components; j++) {
      bld.emit(SHADER_OPCODE_MOV_INDIRECT,
               } else {
      const unsigned num_mov_indirects =
         /* We read a little bit less per MOV INDIRECT, as they are now
   * 32-bits ones instead of 64-bit. Fix read_size then.
   */
   const unsigned read_size_32bit = read_size -
         for (unsigned j = 0; j < instr->num_components; j++) {
      for (unsigned i = 0; i < num_mov_indirects; i++) {
      bld.emit(SHADER_OPCODE_MOV_INDIRECT,
            subscript(offset(dest, bld, j), BRW_REGISTER_TYPE_UD, i),
            }
               case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ubo_uniform_block_intel: {
               if (get_nir_src_bindless(instr->src[0]))
         else
            if (!nir_src_is_const(instr->src[1])) {
      if (instr->intrinsic == nir_intrinsic_load_ubo) {
      /* load_ubo with non-uniform offset */
                  for (int i = 0; i < instr->num_components; i++)
      VARYING_PULL_CONSTANT_LOAD(bld, offset(dest, bld, i),
                        } else {
      /* load_ubo with uniform offset */
   const fs_builder ubld1 = bld.exec_all().group(1, 0);
                                          const nir_src load_offset = instr->src[1];
   if (nir_src_is_const(load_offset)) {
      fs_reg addr = ubld8.vgrf(BRW_REGISTER_TYPE_UD);
   ubld8.MOV(addr, brw_imm_ud(nir_src_as_uint(load_offset)));
      } else {
                                                      while (loaded_dwords < total_dwords) {
      const unsigned block =
                                 const fs_builder &ubld = block <= 8 ? ubld8 : ubld16;
                                 ubld1.ADD(srcs[SURFACE_LOGICAL_SRC_ADDRESS],
                     for (unsigned c = 0; c < instr->num_components; c++) {
                              } else {
      /* Even if we are loading doubles, a pull constant load will load
   * a 32-bit vec4, so should only reserve vgrf space for that. If we
   * need to load a full dvec4 we will have to emit 2 loads. This is
   * similar to demote_pull_constants(), except that in that case we
   * see individual accesses to each component of the vector and then
   * we let CSE deal with duplicate loads. Here we see a vector access
   * and we have to split it if necessary.
   */
   const unsigned type_size = type_sz(dest.type);
   const unsigned load_offset = nir_src_as_uint(instr->src[1]);
   const unsigned ubo_block =
         const unsigned offset_256b = load_offset / 32;
                  /* See if we've selected this as a push constant candidate */
   fs_reg push_reg;
   for (int i = 0; i < 4; i++) {
      const struct brw_ubo_range *range = &prog_data->ubo_ranges[i];
   if (range->block == ubo_block &&
                     push_reg = fs_reg(UNIFORM, UBO_START + i, dest.type);
   push_reg.offset = load_offset - 32 * range->start;
                  if (push_reg.file != BAD_FILE) {
      for (unsigned i = 0; i < instr->num_components; i++) {
      bld.MOV(offset(dest, bld, i),
      }
                                       for (unsigned c = 0; c < instr->num_components;) {
      const unsigned base = load_offset + c * type_size;
   /* Number of usable components in the next block-aligned load. */
                  const fs_reg packed_consts = ubld.vgrf(BRW_REGISTER_TYPE_UD);
   fs_reg srcs[PULL_UNIFORM_CONSTANT_SRCS];
   srcs[PULL_UNIFORM_CONSTANT_SRC_SURFACE]        = surface;
   srcs[PULL_UNIFORM_CONSTANT_SRC_SURFACE_HANDLE] = surface_handle;
                                 const fs_reg consts =
                                       }
               case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant: {
               assert(instr->def.bit_size <= 32);
   assert(nir_intrinsic_align(instr) > 0);
   fs_reg srcs[A64_LOGICAL_NUM_SRCS];
   srcs[A64_LOGICAL_ADDRESS] = get_nir_src(instr->src[0]);
   srcs[A64_LOGICAL_SRC] = fs_reg(); /* No source data */
   srcs[A64_LOGICAL_ENABLE_HELPERS] =
            if (instr->def.bit_size == 32 &&
                              fs_inst *inst =
      bld.emit(SHADER_OPCODE_A64_UNTYPED_READ_LOGICAL, dest,
      inst->size_written = instr->num_components *
      } else {
      const unsigned bit_size = instr->def.bit_size;
                           bld.emit(SHADER_OPCODE_A64_BYTE_SCATTERED_READ_LOGICAL, tmp,
            }
               case nir_intrinsic_store_global: {
               assert(nir_src_bit_size(instr->src[0]) <= 32);
   assert(nir_intrinsic_write_mask(instr) ==
                  fs_reg srcs[A64_LOGICAL_NUM_SRCS];
   srcs[A64_LOGICAL_ADDRESS] = get_nir_src(instr->src[1]);
   srcs[A64_LOGICAL_ENABLE_HELPERS] =
            if (nir_src_bit_size(instr->src[0]) == 32 &&
                                    bld.emit(SHADER_OPCODE_A64_UNTYPED_WRITE_LOGICAL, fs_reg(),
      } else {
      assert(nir_src_num_components(instr->src[0]) == 1);
   const unsigned bit_size = nir_src_bit_size(instr->src[0]);
   brw_reg_type data_type =
                                       bld.emit(SHADER_OPCODE_A64_BYTE_SCATTERED_WRITE_LOGICAL, fs_reg(),
      }
               case nir_intrinsic_global_atomic:
   case nir_intrinsic_global_atomic_swap:
      nir_emit_global_atomic(bld, instr);
         case nir_intrinsic_load_global_const_block_intel: {
      assert(instr->def.bit_size == 32);
            const fs_builder ubld = bld.exec_all().group(instr->num_components, 0);
            bool is_pred_const = nir_src_is_const(instr->src[1]);
   if (is_pred_const && nir_src_as_uint(instr->src[1]) == 0) {
      /* In this case, we don't want the UBO load at all.  We really
   * shouldn't get here but it's possible.
   */
      } else {
                              /* If the predicate is constant and we got here, then it's non-zero
   * and we don't need the predicate at all.
   */
   if (!is_pred_const) {
      /* Load the predicate */
   fs_reg pred = bld.emit_uniformize(get_nir_src(instr->src[1]));
                  /* Stomp the destination with 0 if we're OOB */
   mov = ubld.MOV(load_val, brw_imm_ud(0));
   mov->predicate = BRW_PREDICATE_NORMAL;
               fs_reg srcs[A64_LOGICAL_NUM_SRCS];
   srcs[A64_LOGICAL_ADDRESS] = addr;
   srcs[A64_LOGICAL_SRC] = fs_reg(); /* No source data */
   srcs[A64_LOGICAL_ARG] = brw_imm_ud(instr->num_components);
   /* This intrinsic loads memory from a uniform address, sometimes
   * shared across lanes. We never need to mask it.
                  fs_inst *load = ubld.emit(SHADER_OPCODE_A64_OWORD_BLOCK_READ_LOGICAL,
         if (!is_pred_const)
               /* From the HW perspective, we just did a single SIMD16 instruction
   * which loaded a dword in each SIMD channel.  From NIR's perspective,
   * this instruction returns a vec16.  Any users of this data in the
   * back-end will expect a vec16 per SIMD channel so we have to emit a
   * pile of MOVs to resolve this discrepancy.  Fortunately, copy-prop
   * will generally clean them up for us.
   */
   for (unsigned i = 0; i < instr->num_components; i++) {
      bld.MOV(retype(offset(dest, bld, i), BRW_REGISTER_TYPE_UD),
      }
               case nir_intrinsic_load_global_constant_uniform_block_intel: {
      const unsigned total_dwords = ALIGN(instr->num_components, REG_SIZE / 4);
            const fs_builder ubld1 = bld.exec_all().group(1, 0);
   const fs_builder ubld8 = bld.exec_all().group(8, 0);
            const fs_reg packed_consts =
                  while (loaded_dwords < total_dwords) {
      const unsigned block =
      choose_oword_block_size_dwords(devinfo,
                        fs_reg srcs[A64_LOGICAL_NUM_SRCS];
   srcs[A64_LOGICAL_ADDRESS] = address;
   srcs[A64_LOGICAL_SRC] = fs_reg(); /* No source data */
   srcs[A64_LOGICAL_ARG] = brw_imm_ud(block);
   srcs[A64_LOGICAL_ENABLE_HELPERS] = brw_imm_ud(0);
   ubld.emit(SHADER_OPCODE_A64_UNALIGNED_OWORD_BLOCK_READ_LOGICAL,
                  increment_a64_address(ubld1, address, block_bytes);
               for (unsigned c = 0; c < instr->num_components; c++)
                              case nir_intrinsic_load_ssbo: {
               const unsigned bit_size = instr->def.bit_size;
   fs_reg srcs[SURFACE_LOGICAL_NUM_SRCS];
   srcs[get_nir_src_bindless(instr->src[0]) ?
      SURFACE_LOGICAL_SRC_SURFACE_HANDLE :
   SURFACE_LOGICAL_SRC_SURFACE] =
      srcs[SURFACE_LOGICAL_SRC_ADDRESS] = get_nir_src(instr->src[1]);
   srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] = brw_imm_ud(1);
            /* Make dest unsigned because that's what the temporary will be */
            /* Read the vector */
   assert(bit_size <= 32);
   assert(nir_intrinsic_align(instr) > 0);
   if (bit_size == 32 &&
      nir_intrinsic_align(instr) >= 4) {
   assert(instr->def.num_components <= 4);
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(instr->num_components);
   fs_inst *inst =
      bld.emit(SHADER_OPCODE_UNTYPED_SURFACE_READ_LOGICAL,
         } else {
                     fs_reg read_result = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.emit(SHADER_OPCODE_BYTE_SCATTERED_READ_LOGICAL,
            }
               case nir_intrinsic_store_ssbo: {
               const unsigned bit_size = nir_src_bit_size(instr->src[0]);
   fs_reg srcs[SURFACE_LOGICAL_NUM_SRCS];
   srcs[get_nir_src_bindless(instr->src[1]) ?
      SURFACE_LOGICAL_SRC_SURFACE_HANDLE :
   SURFACE_LOGICAL_SRC_SURFACE] =
      srcs[SURFACE_LOGICAL_SRC_ADDRESS] = get_nir_src(instr->src[2]);
   srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] = brw_imm_ud(1);
            fs_reg data = get_nir_src(instr->src[0]);
            assert(bit_size <= 32);
   assert(nir_intrinsic_write_mask(instr) ==
         assert(nir_intrinsic_align(instr) > 0);
   if (bit_size == 32 &&
      nir_intrinsic_align(instr) >= 4) {
   assert(nir_src_num_components(instr->src[0]) <= 4);
   srcs[SURFACE_LOGICAL_SRC_DATA] = data;
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(instr->num_components);
   bld.emit(SHADER_OPCODE_UNTYPED_SURFACE_WRITE_LOGICAL,
      } else {
                                    bld.emit(SHADER_OPCODE_BYTE_SCATTERED_WRITE_LOGICAL,
      }
               case nir_intrinsic_load_ssbo_uniform_block_intel:
   case nir_intrinsic_load_shared_uniform_block_intel: {
               const bool is_ssbo =
         if (is_ssbo) {
      srcs[get_nir_src_bindless(instr->src[0]) ?
      SURFACE_LOGICAL_SRC_SURFACE_HANDLE :
   SURFACE_LOGICAL_SRC_SURFACE] =
   } else {
                  const unsigned total_dwords = ALIGN(instr->num_components,
                  const fs_builder ubld1 = bld.exec_all().group(1, 0);
   const fs_builder ubld8 = bld.exec_all().group(8, 0);
            const fs_reg packed_consts =
            const nir_src load_offset = is_ssbo ? instr->src[1] : instr->src[0];
   if (nir_src_is_const(load_offset)) {
      fs_reg addr = ubld8.vgrf(BRW_REGISTER_TYPE_UD);
   ubld8.MOV(addr, brw_imm_ud(nir_src_as_uint(load_offset)));
      } else {
      srcs[SURFACE_LOGICAL_SRC_ADDRESS] =
               while (loaded_dwords < total_dwords) {
      const unsigned block =
      choose_oword_block_size_dwords(devinfo,
                        const fs_builder &ubld = block <= 8 ? ubld8 : ubld16;
   ubld.emit(SHADER_OPCODE_UNALIGNED_OWORD_BLOCK_READ_LOGICAL,
                                 ubld1.ADD(srcs[SURFACE_LOGICAL_SRC_ADDRESS],
                     for (unsigned c = 0; c < instr->num_components; c++)
                              case nir_intrinsic_store_output: {
      assert(nir_src_bit_size(instr->src[0]) == 32);
            unsigned store_offset = nir_src_as_uint(instr->src[1]);
   unsigned num_components = instr->num_components;
            fs_reg new_dest = retype(offset(outputs[instr->const_index[0]], bld,
         for (unsigned j = 0; j < num_components; j++) {
      bld.MOV(offset(new_dest, bld, j + first_component),
      }
               case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
      nir_emit_surface_atomic(bld, instr,
                     case nir_intrinsic_get_ssbo_size: {
               /* A resinfo's sampler message is used to get the buffer size.  The
   * SIMD8's writeback message consists of four registers and SIMD16's
   * writeback message consists of 8 destination registers (two per each
   * component).  Because we are only interested on the first channel of
   * the first returned component, where resinfo returns the buffer size
   * for SURFTYPE_BUFFER, we can just use the SIMD8 variant regardless of
   * the dispatch width.
   */
   const fs_builder ubld = bld.exec_all().group(8 * reg_unit(devinfo), 0);
   fs_reg src_payload = ubld.vgrf(BRW_REGISTER_TYPE_UD);
            /* Set LOD = 0 */
            fs_reg srcs[GET_BUFFER_SIZE_SRCS];
   srcs[get_nir_src_bindless(instr->src[0]) ?
      GET_BUFFER_SIZE_SRC_SURFACE_HANDLE :
   GET_BUFFER_SIZE_SRC_SURFACE] =
      srcs[GET_BUFFER_SIZE_SRC_LOD] = src_payload;
   fs_inst *inst = ubld.emit(SHADER_OPCODE_GET_BUFFER_SIZE, ret_payload,
         inst->header_size = 0;
   inst->mlen = reg_unit(devinfo);
            /* SKL PRM, vol07, 3D Media GPGPU Engine, Bounds Checking and Faulting:
   *
   * "Out-of-bounds checking is always performed at a DWord granularity. If
   * any part of the DWord is out-of-bounds then the whole DWord is
   * considered out-of-bounds."
   *
   * This implies that types with size smaller than 4-bytes need to be
   * padded if they don't complete the last dword of the buffer. But as we
   * need to maintain the original size we need to reverse the padding
   * calculation to return the correct size to know the number of elements
   * of an unsized array. As we stored in the last two bits of the surface
   * size the needed padding for the buffer, we calculate here the
   * original buffer_size reversing the surface_size calculation:
   *
   * surface_size = isl_align(buffer_size, 4) +
   *                (isl_align(buffer_size) - buffer_size)
   *
   * buffer_size = surface_size & ~3 - surface_size & 3
            fs_reg size_aligned4 = ubld.vgrf(BRW_REGISTER_TYPE_UD);
   fs_reg size_padding = ubld.vgrf(BRW_REGISTER_TYPE_UD);
            ubld.AND(size_padding, ret_payload, brw_imm_ud(3));
   ubld.AND(size_aligned4, ret_payload, brw_imm_ud(~3));
            bld.MOV(retype(dest, ret_payload.type), component(buffer_size, 0));
               case nir_intrinsic_load_scratch: {
               assert(instr->def.num_components == 1);
   const unsigned bit_size = instr->def.bit_size;
            if (devinfo->verx10 >= 125) {
      const fs_builder ubld = bld.exec_all().group(1, 0);
   fs_reg handle = component(ubld.vgrf(BRW_REGISTER_TYPE_UD), 0);
   ubld.AND(handle, retype(brw_vec1_grf(0, 5), BRW_REGISTER_TYPE_UD),
         srcs[SURFACE_LOGICAL_SRC_SURFACE] = brw_imm_ud(GFX125_NON_BINDLESS);
      } else if (devinfo->ver >= 8) {
      srcs[SURFACE_LOGICAL_SRC_SURFACE] =
      } else {
                  srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] = brw_imm_ud(1);
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(bit_size);
   srcs[SURFACE_LOGICAL_SRC_ALLOW_SAMPLE_MASK] = brw_imm_ud(0);
            /* Make dest unsigned because that's what the temporary will be */
            /* Read the vector */
   assert(instr->def.num_components == 1);
   assert(bit_size <= 32);
   assert(nir_intrinsic_align(instr) > 0);
   if (bit_size == 32 &&
      nir_intrinsic_align(instr) >= 4) {
   if (devinfo->verx10 >= 125) {
                     srcs[SURFACE_LOGICAL_SRC_ADDRESS] =
                  bld.emit(SHADER_OPCODE_UNTYPED_SURFACE_READ_LOGICAL,
      } else {
      /* The offset for a DWORD scattered message is in dwords. */
                  bld.emit(SHADER_OPCODE_DWORD_SCATTERED_READ_LOGICAL,
         } else {
                     fs_reg read_result = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.emit(SHADER_OPCODE_BYTE_SCATTERED_READ_LOGICAL,
                     shader_stats.fill_count += DIV_ROUND_UP(dispatch_width, 16);
               case nir_intrinsic_store_scratch: {
               assert(nir_src_num_components(instr->src[0]) == 1);
   const unsigned bit_size = nir_src_bit_size(instr->src[0]);
            if (devinfo->verx10 >= 125) {
      const fs_builder ubld = bld.exec_all().group(1, 0);
   fs_reg handle = component(ubld.vgrf(BRW_REGISTER_TYPE_UD), 0);
   ubld.AND(handle, retype(brw_vec1_grf(0, 5), BRW_REGISTER_TYPE_UD),
         srcs[SURFACE_LOGICAL_SRC_SURFACE] = brw_imm_ud(GFX125_NON_BINDLESS);
      } else if (devinfo->ver >= 8) {
      srcs[SURFACE_LOGICAL_SRC_SURFACE] =
      } else {
                  srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] = brw_imm_ud(1);
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(bit_size);
   /**
   * While this instruction has side-effects, it should not be predicated
   * on sample mask, because otherwise fs helper invocations would
   * load undefined values from scratch memory. And scratch memory
   * load-stores are produced from operations without side-effects, thus
   * they should not have different behaviour in the helper invocations.
   */
   srcs[SURFACE_LOGICAL_SRC_ALLOW_SAMPLE_MASK] = brw_imm_ud(0);
            fs_reg data = get_nir_src(instr->src[0]);
            assert(nir_src_num_components(instr->src[0]) == 1);
   assert(bit_size <= 32);
   assert(nir_intrinsic_write_mask(instr) == 1);
   assert(nir_intrinsic_align(instr) > 0);
   if (bit_size == 32 &&
      nir_intrinsic_align(instr) >= 4) {
                     srcs[SURFACE_LOGICAL_SRC_ADDRESS] =
                  bld.emit(SHADER_OPCODE_UNTYPED_SURFACE_WRITE_LOGICAL,
                        /* The offset for a DWORD scattered message is in dwords. */
                  bld.emit(SHADER_OPCODE_DWORD_SCATTERED_WRITE_LOGICAL,
         } else {
                                    bld.emit(SHADER_OPCODE_BYTE_SCATTERED_WRITE_LOGICAL,
      }
   shader_stats.spill_count += DIV_ROUND_UP(dispatch_width, 16);
               case nir_intrinsic_load_subgroup_size:
      /* This should only happen for fragment shaders because every other case
   * is lowered in NIR so we can optimize on it.
   */
   assert(stage == MESA_SHADER_FRAGMENT);
   bld.MOV(retype(dest, BRW_REGISTER_TYPE_D), brw_imm_d(dispatch_width));
         case nir_intrinsic_load_subgroup_invocation:
      bld.MOV(retype(dest, BRW_REGISTER_TYPE_D),
               case nir_intrinsic_load_subgroup_eq_mask:
   case nir_intrinsic_load_subgroup_ge_mask:
   case nir_intrinsic_load_subgroup_gt_mask:
   case nir_intrinsic_load_subgroup_le_mask:
   case nir_intrinsic_load_subgroup_lt_mask:
            case nir_intrinsic_vote_any: {
               /* The any/all predicates do not consider channel enables. To prevent
   * dead channels from affecting the result, we initialize the flag with
   * with the identity value for the logical operation.
   */
   if (dispatch_width == 32) {
      /* For SIMD32, we use a UD type so we fill both f0.0 and f0.1. */
   ubld.MOV(retype(brw_flag_reg(0, 0), BRW_REGISTER_TYPE_UD),
      } else {
         }
            /* For some reason, the any/all predicates don't work properly with
   * SIMD32.  In particular, it appears that a SEL with a QtrCtrl of 2H
   * doesn't read the correct subset of the flag register and you end up
   * getting garbage in the second half.  Work around this by using a pair
   * of 1-wide MOVs and scattering the result.
   */
   fs_reg res1 = ubld.vgrf(BRW_REGISTER_TYPE_D);
   ubld.MOV(res1, brw_imm_d(0));
   set_predicate(dispatch_width == 8  ? BRW_PREDICATE_ALIGN1_ANY8H :
                        bld.MOV(retype(dest, BRW_REGISTER_TYPE_D), component(res1, 0));
      }
   case nir_intrinsic_vote_all: {
               /* The any/all predicates do not consider channel enables. To prevent
   * dead channels from affecting the result, we initialize the flag with
   * with the identity value for the logical operation.
   */
   if (dispatch_width == 32) {
      /* For SIMD32, we use a UD type so we fill both f0.0 and f0.1. */
   ubld.MOV(retype(brw_flag_reg(0, 0), BRW_REGISTER_TYPE_UD),
      } else {
         }
            /* For some reason, the any/all predicates don't work properly with
   * SIMD32.  In particular, it appears that a SEL with a QtrCtrl of 2H
   * doesn't read the correct subset of the flag register and you end up
   * getting garbage in the second half.  Work around this by using a pair
   * of 1-wide MOVs and scattering the result.
   */
   fs_reg res1 = ubld.vgrf(BRW_REGISTER_TYPE_D);
   ubld.MOV(res1, brw_imm_d(0));
   set_predicate(dispatch_width == 8  ? BRW_PREDICATE_ALIGN1_ALL8H :
                        bld.MOV(retype(dest, BRW_REGISTER_TYPE_D), component(res1, 0));
      }
   case nir_intrinsic_vote_feq:
   case nir_intrinsic_vote_ieq: {
      fs_reg value = get_nir_src(instr->src[0]);
   if (instr->intrinsic == nir_intrinsic_vote_feq) {
      const unsigned bit_size = nir_src_bit_size(instr->src[0]);
   value.type = bit_size == 8 ? BRW_REGISTER_TYPE_B :
               fs_reg uniformized = bld.emit_uniformize(value);
            /* The any/all predicates do not consider channel enables. To prevent
   * dead channels from affecting the result, we initialize the flag with
   * with the identity value for the logical operation.
   */
   if (dispatch_width == 32) {
      /* For SIMD32, we use a UD type so we fill both f0.0 and f0.1. */
   ubld.MOV(retype(brw_flag_reg(0, 0), BRW_REGISTER_TYPE_UD),
      } else {
         }
            /* For some reason, the any/all predicates don't work properly with
   * SIMD32.  In particular, it appears that a SEL with a QtrCtrl of 2H
   * doesn't read the correct subset of the flag register and you end up
   * getting garbage in the second half.  Work around this by using a pair
   * of 1-wide MOVs and scattering the result.
   */
   fs_reg res1 = ubld.vgrf(BRW_REGISTER_TYPE_D);
   ubld.MOV(res1, brw_imm_d(0));
   set_predicate(dispatch_width == 8  ? BRW_PREDICATE_ALIGN1_ALL8H :
                        bld.MOV(retype(dest, BRW_REGISTER_TYPE_D), component(res1, 0));
               case nir_intrinsic_ballot: {
      const fs_reg value = retype(get_nir_src(instr->src[0]),
         struct brw_reg flag = brw_flag_reg(0, 0);
   /* FIXME: For SIMD32 programs, this causes us to stomp on f0.1 as well
   * as f0.0.  This is a problem for fragment programs as we currently use
   * f0.1 for discards.  Fortunately, we don't support SIMD32 fragment
   * programs yet so this isn't a problem.  When we do, something will
   * have to change.
   */
   if (dispatch_width == 32)
            bld.exec_all().group(1, 0).MOV(flag, brw_imm_ud(0u));
            if (instr->def.bit_size > 32) {
         } else {
         }
   bld.MOV(dest, flag);
               case nir_intrinsic_read_invocation: {
      const fs_reg value = get_nir_src(instr->src[0]);
                     /* When for some reason the subgroup_size picked by NIR is larger than
   * the dispatch size picked by the backend (this could happen in RT,
   * FS), bound the invocation to the dispatch size.
   */
   fs_reg bound_invocation;
   if (api_subgroup_size == 0 ||
      bld.dispatch_width() < api_subgroup_size) {
   bound_invocation = bld.vgrf(BRW_REGISTER_TYPE_UD);
      } else {
         }
   bld.exec_all().emit(SHADER_OPCODE_BROADCAST, tmp, value,
            bld.MOV(retype(dest, value.type), fs_reg(component(tmp, 0)));
               case nir_intrinsic_read_first_invocation: {
      const fs_reg value = get_nir_src(instr->src[0]);
   bld.MOV(retype(dest, value.type), bld.emit_uniformize(value));
               case nir_intrinsic_shuffle: {
      const fs_reg value = get_nir_src(instr->src[0]);
            bld.emit(SHADER_OPCODE_SHUFFLE, retype(dest, value.type), value, index);
               case nir_intrinsic_first_invocation: {
      fs_reg tmp = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.exec_all().emit(SHADER_OPCODE_FIND_LIVE_CHANNEL, tmp);
   bld.MOV(retype(dest, BRW_REGISTER_TYPE_UD),
                     case nir_intrinsic_last_invocation: {
      fs_reg tmp = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.exec_all().emit(SHADER_OPCODE_FIND_LAST_LIVE_CHANNEL, tmp);
   bld.MOV(retype(dest, BRW_REGISTER_TYPE_UD),
                     case nir_intrinsic_quad_broadcast: {
      const fs_reg value = get_nir_src(instr->src[0]);
            bld.emit(SHADER_OPCODE_CLUSTER_BROADCAST, retype(dest, value.type),
                     case nir_intrinsic_quad_swap_horizontal: {
      const fs_reg value = get_nir_src(instr->src[0]);
   const fs_reg tmp = bld.vgrf(value.type);
   if (devinfo->ver <= 7) {
      /* The hardware doesn't seem to support these crazy regions with
   * compressed instructions on gfx7 and earlier so we fall back to
   * using quad swizzles.  Fortunately, we don't support 64-bit
   * anything in Vulkan on gfx7.
   */
   assert(nir_src_bit_size(instr->src[0]) == 32);
   const fs_builder ubld = bld.exec_all();
   ubld.emit(SHADER_OPCODE_QUAD_SWIZZLE, tmp, value,
            } else {
               const fs_reg src_left = horiz_stride(value, 2);
   const fs_reg src_right = horiz_stride(horiz_offset(value, 1), 2);
                              }
   bld.MOV(retype(dest, value.type), tmp);
               case nir_intrinsic_quad_swap_vertical: {
      const fs_reg value = get_nir_src(instr->src[0]);
   if (nir_src_bit_size(instr->src[0]) == 32) {
      /* For 32-bit, we can use a SIMD4x2 instruction to do this easily */
   const fs_reg tmp = bld.vgrf(value.type);
   const fs_builder ubld = bld.exec_all();
   ubld.emit(SHADER_OPCODE_QUAD_SWIZZLE, tmp, value,
            } else {
      /* For larger data types, we have to either emit dispatch_width many
   * MOVs or else fall back to doing indirects.
   */
   fs_reg idx = bld.vgrf(BRW_REGISTER_TYPE_W);
   bld.XOR(idx, nir_system_values[SYSTEM_VALUE_SUBGROUP_INVOCATION],
            }
               case nir_intrinsic_quad_swap_diagonal: {
      const fs_reg value = get_nir_src(instr->src[0]);
   if (nir_src_bit_size(instr->src[0]) == 32) {
      /* For 32-bit, we can use a SIMD4x2 instruction to do this easily */
   const fs_reg tmp = bld.vgrf(value.type);
   const fs_builder ubld = bld.exec_all();
   ubld.emit(SHADER_OPCODE_QUAD_SWIZZLE, tmp, value,
            } else {
      /* For larger data types, we have to either emit dispatch_width many
   * MOVs or else fall back to doing indirects.
   */
   fs_reg idx = bld.vgrf(BRW_REGISTER_TYPE_W);
   bld.XOR(idx, nir_system_values[SYSTEM_VALUE_SUBGROUP_INVOCATION],
            }
               case nir_intrinsic_reduce: {
      fs_reg src = get_nir_src(instr->src[0]);
   nir_op redop = (nir_op)nir_intrinsic_reduction_op(instr);
   unsigned cluster_size = nir_intrinsic_cluster_size(instr);
   if (cluster_size == 0 || cluster_size > dispatch_width)
            /* Figure out the source type */
   src.type = brw_type_for_nir_type(devinfo,
                  fs_reg identity = brw_nir_reduction_op_identity(bld, redop, src.type);
   opcode brw_op = brw_op_for_nir_reduction_op(redop);
            /* Set up a register for all of our scratching around and initialize it
   * to reduction operation's identity value.
   */
   fs_reg scan = bld.vgrf(src.type);
                     dest.type = src.type;
   if (cluster_size * type_sz(src.type) >= REG_SIZE * 2) {
      /* In this case, CLUSTER_BROADCAST instruction isn't needed because
   * the distance between clusters is at least 2 GRFs.  In this case,
   * we don't need the weird striding of the CLUSTER_BROADCAST
   * instruction and can just do regular MOVs.
   */
   assert((cluster_size * type_sz(src.type)) % (REG_SIZE * 2) == 0);
   const unsigned groups =
         const unsigned group_size = dispatch_width / groups;
   for (unsigned i = 0; i < groups; i++) {
      const unsigned cluster = (i * group_size) / cluster_size;
   const unsigned comp = cluster * cluster_size + (cluster_size - 1);
   bld.group(group_size, i).MOV(horiz_offset(dest, i * group_size),
         } else {
      bld.emit(SHADER_OPCODE_CLUSTER_BROADCAST, dest, scan,
      }
               case nir_intrinsic_inclusive_scan:
   case nir_intrinsic_exclusive_scan: {
      fs_reg src = get_nir_src(instr->src[0]);
            /* Figure out the source type */
   src.type = brw_type_for_nir_type(devinfo,
                  fs_reg identity = brw_nir_reduction_op_identity(bld, redop, src.type);
   opcode brw_op = brw_op_for_nir_reduction_op(redop);
            /* Set up a register for all of our scratching around and initialize it
   * to reduction operation's identity value.
   */
   fs_reg scan = bld.vgrf(src.type);
   const fs_builder allbld = bld.exec_all();
            if (instr->intrinsic == nir_intrinsic_exclusive_scan) {
      /* Exclusive scan is a bit harder because we have to do an annoying
   * shift of the contents before we can begin.  To make things worse,
   * we can't do this with a normal stride; we have to use indirects.
   */
   fs_reg shifted = bld.vgrf(src.type);
   fs_reg idx = bld.vgrf(BRW_REGISTER_TYPE_W);
   allbld.ADD(idx, nir_system_values[SYSTEM_VALUE_SUBGROUP_INVOCATION],
         allbld.emit(SHADER_OPCODE_SHUFFLE, shifted, scan, idx);
   allbld.group(1, 0).MOV(component(shifted, 0), identity);
                        bld.MOV(retype(dest, src.type), scan);
               case nir_intrinsic_load_global_block_intel: {
                        const fs_builder ubld1 = bld.exec_all().group(1, 0);
   const fs_builder ubld8 = bld.exec_all().group(8, 0);
            const unsigned total = instr->num_components * dispatch_width;
            while (loaded < total) {
      const unsigned block =
                           fs_reg srcs[A64_LOGICAL_NUM_SRCS];
   srcs[A64_LOGICAL_ADDRESS] = address;
   srcs[A64_LOGICAL_SRC] = fs_reg(); /* No source data */
   srcs[A64_LOGICAL_ARG] = brw_imm_ud(block);
   srcs[A64_LOGICAL_ENABLE_HELPERS] = brw_imm_ud(1);
   ubld.emit(SHADER_OPCODE_A64_UNALIGNED_OWORD_BLOCK_READ_LOGICAL,
                  increment_a64_address(ubld1, address, block_bytes);
               assert(loaded == total);
               case nir_intrinsic_store_global_block_intel: {
               fs_reg address = bld.emit_uniformize(get_nir_src(instr->src[1]));
            const fs_builder ubld1 = bld.exec_all().group(1, 0);
   const fs_builder ubld8 = bld.exec_all().group(8, 0);
            const unsigned total = instr->num_components * dispatch_width;
            while (written < total) {
                     fs_reg srcs[A64_LOGICAL_NUM_SRCS];
   srcs[A64_LOGICAL_ADDRESS] = address;
   srcs[A64_LOGICAL_SRC] = retype(byte_offset(src, written * 4),
                        const fs_builder &ubld = block == 8 ? ubld8 : ubld16;
                  const unsigned block_bytes = block * 4;
   increment_a64_address(ubld1, address, block_bytes);
               assert(written == total);
               case nir_intrinsic_load_shared_block_intel:
   case nir_intrinsic_load_ssbo_block_intel: {
               const bool is_ssbo =
                  fs_reg srcs[SURFACE_LOGICAL_NUM_SRCS];
   srcs[SURFACE_LOGICAL_SRC_SURFACE] = is_ssbo ?
      get_nir_buffer_intrinsic_index(bld, instr) :
               const fs_builder ubld1 = bld.exec_all().group(1, 0);
   const fs_builder ubld8 = bld.exec_all().group(8, 0);
            const unsigned total = instr->num_components * dispatch_width;
            while (loaded < total) {
      const unsigned block =
                           const fs_builder &ubld = block == 8 ? ubld8 : ubld16;
   ubld.emit(SHADER_OPCODE_UNALIGNED_OWORD_BLOCK_READ_LOGICAL,
                  ubld1.ADD(address, address, brw_imm_ud(block_bytes));
               assert(loaded == total);
               case nir_intrinsic_store_shared_block_intel:
   case nir_intrinsic_store_ssbo_block_intel: {
               const bool is_ssbo =
            fs_reg address = bld.emit_uniformize(get_nir_src(instr->src[is_ssbo ? 2 : 1]));
            fs_reg srcs[SURFACE_LOGICAL_NUM_SRCS];
   srcs[SURFACE_LOGICAL_SRC_SURFACE] = is_ssbo ?
      get_nir_buffer_intrinsic_index(bld, instr) :
               const fs_builder ubld1 = bld.exec_all().group(1, 0);
   const fs_builder ubld8 = bld.exec_all().group(8, 0);
            const unsigned total = instr->num_components * dispatch_width;
            while (written < total) {
                     srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(block);
                  const fs_builder &ubld = block == 8 ? ubld8 : ubld16;
                  const unsigned block_bytes = block * 4;
   ubld1.ADD(address, address, brw_imm_ud(block_bytes));
               assert(written == total);
               case nir_intrinsic_load_topology_id_intel: {
      /* These move around basically every hardware generation, so don'
   * do any >= checks and fail if the platform hasn't explicitly
   * been enabled here.
   */
            /* Here is what the layout of SR0 looks like on Gfx12 :
   *   [13:11] : Slice ID.
   *   [10:9]  : Dual-SubSlice ID
   *   [8]     : SubSlice ID
   *   [7]     : EUID[2] (aka EU Row ID)
   *   [6]     : Reserved
   *   [5:4]   : EUID[1:0]
   *   [2:0]   : Thread ID
   */
   fs_reg raw_id = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.emit(SHADER_OPCODE_READ_SR_REG, raw_id, brw_imm_ud(0));
   switch (nir_intrinsic_base(instr)) {
   case BRW_TOPOLOGY_ID_DSS:
      bld.AND(raw_id, raw_id, brw_imm_ud(0x3fff));
   /* Get rid of anything below dualsubslice */
   bld.SHR(retype(dest, BRW_REGISTER_TYPE_UD), raw_id, brw_imm_ud(9));
      case BRW_TOPOLOGY_ID_EU_THREAD_SIMD: {
      limit_dispatch_width(16, "Topology helper for Ray queries, "
                  /* EU[3:0] << 7
   *
   * The 4bit EU[3:0] we need to build for ray query memory addresses
   * computations is a bit odd :
   *
   *   EU[1:0] = raw_id[5:4] (identified as EUID[1:0])
   *   EU[2]   = raw_id[8]   (identified as SubSlice ID)
   *   EU[3]   = raw_id[7]   (identified as EUID[2] or Row ID)
   */
   {
      fs_reg tmp = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.AND(tmp, raw_id, brw_imm_ud(INTEL_MASK(7, 7)));
   bld.SHL(dst, tmp, brw_imm_ud(3));
   bld.AND(tmp, raw_id, brw_imm_ud(INTEL_MASK(8, 8)));
   bld.SHL(tmp, tmp, brw_imm_ud(1));
   bld.OR(dst, dst, tmp);
   bld.AND(tmp, raw_id, brw_imm_ud(INTEL_MASK(5, 4)));
   bld.SHL(tmp, tmp, brw_imm_ud(3));
               /* ThreadID[2:0] << 4 (ThreadID comes from raw_id[2:0]) */
   {
      bld.AND(raw_id, raw_id, brw_imm_ud(INTEL_MASK(2, 0)));
   bld.SHL(raw_id, raw_id, brw_imm_ud(4));
               /* LaneID[0:3] << 0 (Use nir SYSTEM_VALUE_SUBGROUP_INVOCATION) */
   assert(bld.dispatch_width() <= 16); /* Limit to 4 bits */
   bld.ADD(dst, dst,
            }
   default:
         }
               case nir_intrinsic_load_btd_stack_id_intel:
      if (stage == MESA_SHADER_COMPUTE) {
         } else {
         }
   /* Stack IDs are always in R1 regardless of whether we're coming from a
   * bindless shader or a regular compute shader.
   */
   bld.MOV(retype(dest, BRW_REGISTER_TYPE_UD),
               case nir_intrinsic_btd_spawn_intel:
      if (stage == MESA_SHADER_COMPUTE) {
         } else {
         }
   /* Make sure all the pointers to resume shaders have landed where other
   * threads can see them.
   */
            bld.emit(SHADER_OPCODE_BTD_SPAWN_LOGICAL, bld.null_reg_ud(),
                     case nir_intrinsic_btd_retire_intel:
      if (stage == MESA_SHADER_COMPUTE) {
         } else {
         }
   /* Make sure all the pointers to resume shaders have landed where other
   * threads can see them.
   */
   emit_rt_lsc_fence(bld, LSC_FENCE_LOCAL, LSC_FLUSH_TYPE_NONE);
   bld.emit(SHADER_OPCODE_BTD_RETIRE_LOGICAL);
         case nir_intrinsic_trace_ray_intel: {
      const bool synchronous = nir_intrinsic_synchronous(instr);
            /* Make sure all the previous RT structure writes are visible to the RT
   * fixed function within the DSS, as well as stack pointers to resume
   * shaders.
   */
                     fs_reg globals = get_nir_src(instr->src[0]);
   srcs[RT_LOGICAL_SRC_GLOBALS] = bld.emit_uniformize(globals);
   srcs[RT_LOGICAL_SRC_BVH_LEVEL] = get_nir_src(instr->src[1]);
   srcs[RT_LOGICAL_SRC_TRACE_RAY_CONTROL] = get_nir_src(instr->src[2]);
   srcs[RT_LOGICAL_SRC_SYNCHRONOUS] = brw_imm_ud(synchronous);
   bld.emit(RT_OPCODE_TRACE_RAY_LOGICAL, bld.null_reg_ud(),
            /* There is no actual value to use in the destination register of the
   * synchronous trace instruction. All of the communication with the HW
   * unit happens through memory reads/writes. So to ensure that the
   * operation has completed before we go read the results in memory, we
   * need a barrier followed by an invalidate before accessing memory.
   */
   if (synchronous) {
      bld.emit(BRW_OPCODE_SYNC, bld.null_reg_ud(), brw_imm_ud(TGL_SYNC_ALLWR));
      }
                  #ifndef NDEBUG
         assert(instr->intrinsic < nir_num_intrinsics);
   #endif
               }
      static fs_reg
   expand_to_32bit(const fs_builder &bld, const fs_reg &src)
   {
      if (type_sz(src.type) == 2) {
      fs_reg src32 = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.MOV(src32, retype(src, BRW_REGISTER_TYPE_UW));
      } else {
            }
      void
   fs_visitor::nir_emit_surface_atomic(const fs_builder &bld,
                     {
      enum lsc_opcode op = lsc_aop_for_nir_intrinsic(instr);
                     /* The BTI untyped atomic messages only support 32-bit atomics.  If you
   * just look at the big table of messages in the Vol 7 of the SKL PRM, they
   * appear to exist.  However, if you look at Vol 2a, there are no message
   * descriptors provided for Qword atomic ops except for A64 messages.
   *
   * 16-bit float atomics are supported, however.
   */
   assert(instr->def.bit_size == 32 ||
         (instr->def.bit_size == 64 && devinfo->has_lsc) ||
                     fs_reg srcs[SURFACE_LOGICAL_NUM_SRCS];
   srcs[bindless ?
      SURFACE_LOGICAL_SRC_SURFACE_HANDLE :
      srcs[SURFACE_LOGICAL_SRC_IMM_DIMS] = brw_imm_ud(1);
   srcs[SURFACE_LOGICAL_SRC_IMM_ARG] = brw_imm_ud(op);
            if (shared) {
      /* SLM - Get the offset */
   if (nir_src_is_const(instr->src[0])) {
      srcs[SURFACE_LOGICAL_SRC_ADDRESS] =
      brw_imm_ud(nir_intrinsic_base(instr) +
   } else {
      srcs[SURFACE_LOGICAL_SRC_ADDRESS] = vgrf(glsl_type::uint_type);
   bld.ADD(srcs[SURFACE_LOGICAL_SRC_ADDRESS],
               } else {
      /* SSBOs */
               fs_reg data;
   if (num_data >= 1)
            if (num_data >= 2) {
      fs_reg tmp = bld.vgrf(data.type, 2);
   fs_reg sources[2] = {
      data,
      };
   bld.LOAD_PAYLOAD(tmp, sources, 2, 0);
      }
                     switch (instr->def.bit_size) {
      case 16: {
      fs_reg dest32 = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.emit(SHADER_OPCODE_UNTYPED_ATOMIC_LOGICAL,
               bld.MOV(retype(dest, BRW_REGISTER_TYPE_UW),
                     case 32:
   case 64:
      bld.emit(SHADER_OPCODE_UNTYPED_ATOMIC_LOGICAL,
            default:
         }
      void
   fs_visitor::nir_emit_global_atomic(const fs_builder &bld,
         {
      int op = lsc_aop_for_nir_intrinsic(instr);
                              fs_reg data;
   if (num_data >= 1)
            if (num_data >= 2) {
      fs_reg tmp = bld.vgrf(data.type, 2);
   fs_reg sources[2] = {
      data,
      };
   bld.LOAD_PAYLOAD(tmp, sources, 2, 0);
               fs_reg srcs[A64_LOGICAL_NUM_SRCS];
   srcs[A64_LOGICAL_ADDRESS] = addr;
   srcs[A64_LOGICAL_SRC] = data;
   srcs[A64_LOGICAL_ARG] = brw_imm_ud(op);
            switch (instr->def.bit_size) {
   case 16: {
      fs_reg dest32 = bld.vgrf(BRW_REGISTER_TYPE_UD);
   bld.emit(SHADER_OPCODE_A64_UNTYPED_ATOMIC_LOGICAL,
               bld.MOV(retype(dest, BRW_REGISTER_TYPE_UW), dest32);
      }
   case 32:
   case 64:
      bld.emit(SHADER_OPCODE_A64_UNTYPED_ATOMIC_LOGICAL, dest,
            default:
            }
      void
   fs_visitor::nir_emit_texture(const fs_builder &bld, nir_tex_instr *instr)
   {
               /* SKL PRMs: Volume 7: 3D-Media-GPGPU:
   *
   *    "The Pixel Null Mask field, when enabled via the Pixel Null Mask
   *     Enable will be incorect for sample_c when applied to a surface with
   *     64-bit per texel format such as R16G16BA16_UNORM. Pixel Null mask
   *     Enable may incorrectly report pixels as referencing a Null surface."
   *
   * We'll take care of this in NIR.
   */
                              /* The hardware requires a LOD for buffer textures */
   if (instr->sampler_dim == GLSL_SAMPLER_DIM_BUF)
            uint32_t header_bits = 0;
   for (unsigned i = 0; i < instr->num_srcs; i++) {
      nir_src nir_src = instr->src[i].src;
   fs_reg src = get_nir_src(nir_src);
   switch (instr->src[i].src_type) {
   case nir_tex_src_bias:
      srcs[TEX_LOGICAL_SRC_LOD] =
            case nir_tex_src_comparator:
      srcs[TEX_LOGICAL_SRC_SHADOW_C] = retype(src, BRW_REGISTER_TYPE_F);
      case nir_tex_src_coord:
      switch (instr->op) {
   case nir_texop_txf:
   case nir_texop_txf_ms:
   case nir_texop_txf_ms_mcs_intel:
   case nir_texop_samples_identical:
      srcs[TEX_LOGICAL_SRC_COORDINATE] = retype(src, BRW_REGISTER_TYPE_D);
      default:
      srcs[TEX_LOGICAL_SRC_COORDINATE] = retype(src, BRW_REGISTER_TYPE_F);
               /* Wa_14012688258:
   *
   * Compiler should send U,V,R parameters even if V,R are 0.
   */
   if (instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE &&
      intel_needs_workaround(devinfo, 14012688258))
         case nir_tex_src_ddx:
      srcs[TEX_LOGICAL_SRC_LOD] = retype(src, BRW_REGISTER_TYPE_F);
   lod_components = nir_tex_instr_src_size(instr, i);
      case nir_tex_src_ddy:
      srcs[TEX_LOGICAL_SRC_LOD2] = retype(src, BRW_REGISTER_TYPE_F);
      case nir_tex_src_lod:
      switch (instr->op) {
   case nir_texop_txs:
      srcs[TEX_LOGICAL_SRC_LOD] =
            case nir_texop_txf:
      srcs[TEX_LOGICAL_SRC_LOD] =
            default:
      srcs[TEX_LOGICAL_SRC_LOD] =
            }
      case nir_tex_src_min_lod:
      srcs[TEX_LOGICAL_SRC_MIN_LOD] =
            case nir_tex_src_ms_index:
                  case nir_tex_src_offset: {
      uint32_t offset_bits = 0;
   if (brw_texture_offset(instr, i, &offset_bits)) {
         } else {
      /* On gfx12.5+, if the offsets are not both constant and in the
   * {-8,7} range, nir_lower_tex() will have already lowered the
   * source offset. So we should never reach this point.
   */
   assert(devinfo->verx10 < 125);
   srcs[TEX_LOGICAL_SRC_TG4_OFFSET] =
      }
               case nir_tex_src_projector:
            case nir_tex_src_texture_offset: {
      assert(srcs[TEX_LOGICAL_SRC_SURFACE].file == BAD_FILE);
   /* Emit code to evaluate the actual indexing expression */
   if (instr->texture_index == 0 && is_resource_src(nir_src))
         if (srcs[TEX_LOGICAL_SRC_SURFACE].file == BAD_FILE) {
      fs_reg tmp = vgrf(glsl_type::uint_type);
   bld.ADD(tmp, src, brw_imm_ud(instr->texture_index));
      }
   assert(srcs[TEX_LOGICAL_SRC_SURFACE].file != BAD_FILE);
               case nir_tex_src_sampler_offset: {
      /* Emit code to evaluate the actual indexing expression */
   if (instr->sampler_index == 0 && is_resource_src(nir_src))
         if (srcs[TEX_LOGICAL_SRC_SAMPLER].file == BAD_FILE) {
      fs_reg tmp = vgrf(glsl_type::uint_type);
   bld.ADD(tmp, src, brw_imm_ud(instr->sampler_index));
      }
               case nir_tex_src_texture_handle:
      assert(nir_tex_instr_src_index(instr, nir_tex_src_texture_offset) == -1);
   srcs[TEX_LOGICAL_SRC_SURFACE] = fs_reg();
   if (is_resource_src(nir_src))
         if (srcs[TEX_LOGICAL_SRC_SURFACE_HANDLE].file == BAD_FILE)
               case nir_tex_src_sampler_handle:
      assert(nir_tex_instr_src_index(instr, nir_tex_src_sampler_offset) == -1);
   srcs[TEX_LOGICAL_SRC_SAMPLER] = fs_reg();
   if (is_resource_src(nir_src))
         if (srcs[TEX_LOGICAL_SRC_SAMPLER_HANDLE].file == BAD_FILE)
               case nir_tex_src_ms_mcs_intel:
      assert(instr->op == nir_texop_txf_ms);
               default:
                     /* If the surface or sampler were not specified through sources, use the
   * instruction index.
   */
   if (srcs[TEX_LOGICAL_SRC_SURFACE].file == BAD_FILE &&
      srcs[TEX_LOGICAL_SRC_SURFACE_HANDLE].file == BAD_FILE)
      if (srcs[TEX_LOGICAL_SRC_SAMPLER].file == BAD_FILE &&
      srcs[TEX_LOGICAL_SRC_SAMPLER_HANDLE].file == BAD_FILE)
         if (srcs[TEX_LOGICAL_SRC_MCS].file == BAD_FILE &&
      (instr->op == nir_texop_txf_ms ||
   instr->op == nir_texop_samples_identical)) {
   if (devinfo->ver >= 7) {
      srcs[TEX_LOGICAL_SRC_MCS] =
      emit_mcs_fetch(srcs[TEX_LOGICAL_SRC_COORDINATE],
               } else {
                     srcs[TEX_LOGICAL_SRC_COORD_COMPONENTS] = brw_imm_d(instr->coord_components);
            enum opcode opcode;
   switch (instr->op) {
   case nir_texop_tex:
      opcode = SHADER_OPCODE_TEX_LOGICAL;
      case nir_texop_txb:
      opcode = FS_OPCODE_TXB_LOGICAL;
      case nir_texop_txl:
      opcode = SHADER_OPCODE_TXL_LOGICAL;
      case nir_texop_txd:
      opcode = SHADER_OPCODE_TXD_LOGICAL;
      case nir_texop_txf:
      opcode = SHADER_OPCODE_TXF_LOGICAL;
      case nir_texop_txf_ms:
      /* On Gfx12HP there is only CMS_W available. From the Bspec: Shared
   * Functions - 3D Sampler - Messages - Message Format:
   *
   *   ld2dms REMOVEDBY(GEN:HAS:1406788836)
   */
   if (devinfo->verx10 >= 125)
         else if (devinfo->ver >= 9)
         else
            case nir_texop_txf_ms_mcs_intel:
      opcode = SHADER_OPCODE_TXF_MCS_LOGICAL;
      case nir_texop_query_levels:
   case nir_texop_txs:
      opcode = SHADER_OPCODE_TXS_LOGICAL;
      case nir_texop_lod:
      opcode = SHADER_OPCODE_LOD_LOGICAL;
      case nir_texop_tg4:
      if (srcs[TEX_LOGICAL_SRC_TG4_OFFSET].file != BAD_FILE)
         else
            case nir_texop_texture_samples:
      opcode = SHADER_OPCODE_SAMPLEINFO_LOGICAL;
      case nir_texop_samples_identical: {
               /* If mcs is an immediate value, it means there is no MCS.  In that case
   * just return false.
   */
   if (srcs[TEX_LOGICAL_SRC_MCS].file == BRW_IMMEDIATE_VALUE) {
         } else if (devinfo->ver >= 9) {
      fs_reg tmp = vgrf(glsl_type::uint_type);
   bld.OR(tmp, srcs[TEX_LOGICAL_SRC_MCS],
            } else {
      bld.CMP(dst, srcs[TEX_LOGICAL_SRC_MCS], brw_imm_ud(0u),
      }
      }
   default:
                  if (instr->op == nir_texop_tg4) {
      if (instr->component == 1 &&
      key_tex->gather_channel_quirk_mask & (1 << instr->texture_index)) {
   /* gather4 sampler is broken for green channel on RG32F --
   * we must ask for blue instead.
   */
      } else {
                     fs_reg dst = bld.vgrf(brw_type_for_nir_type(devinfo, instr->dest_type), 4 + instr->is_sparse);
   fs_inst *inst = bld.emit(opcode, dst, srcs, ARRAY_SIZE(srcs));
            const unsigned dest_size = nir_tex_instr_dest_size(instr);
   if (devinfo->ver >= 9 &&
      instr->op != nir_texop_tg4 && instr->op != nir_texop_query_levels) {
   unsigned write_mask = nir_def_components_read(&instr->def);
   assert(write_mask != 0); /* dead code should have been eliminated */
   if (instr->is_sparse) {
      inst->size_written = (util_last_bit(write_mask) - 1) *
            } else {
      inst->size_written = util_last_bit(write_mask) *
         } else {
      inst->size_written = 4 * inst->dst.component_size(inst->exec_size) +
               if (srcs[TEX_LOGICAL_SRC_SHADOW_C].file != BAD_FILE)
            fs_reg nir_dest[5];
   for (unsigned i = 0; i < dest_size; i++)
            if (instr->op == nir_texop_query_levels) {
      /* # levels is in .w */
   if (devinfo->ver <= 9) {
      /**
   * Wa_1940217:
   *
   * When a surface of type SURFTYPE_NULL is accessed by resinfo, the
   * MIPCount returned is undefined instead of 0.
   */
   fs_inst *mov = bld.MOV(bld.null_reg_d(), dst);
   mov->conditional_mod = BRW_CONDITIONAL_NZ;
   nir_dest[0] = bld.vgrf(BRW_REGISTER_TYPE_D);
   fs_inst *sel = bld.SEL(nir_dest[0], offset(dst, bld, 3), brw_imm_d(0));
      } else {
            } else if (instr->op == nir_texop_txs &&
            /* Gfx4-6 return 0 instead of 1 for single layer surfaces. */
   fs_reg depth = offset(dst, bld, 2);
   nir_dest[2] = vgrf(glsl_type::int_type);
               /* The residency bits are only in the first component. */
   if (instr->is_sparse)
               }
      void
   fs_visitor::nir_emit_jump(const fs_builder &bld, nir_jump_instr *instr)
   {
      switch (instr->type) {
   case nir_jump_break:
      bld.emit(BRW_OPCODE_BREAK);
      case nir_jump_continue:
      bld.emit(BRW_OPCODE_CONTINUE);
      case nir_jump_halt:
      bld.emit(BRW_OPCODE_HALT);
      case nir_jump_return:
   default:
            }
      /*
   * This helper takes a source register and un/shuffles it into the destination
   * register.
   *
   * If source type size is smaller than destination type size the operation
   * needed is a component shuffle. The opposite case would be an unshuffle. If
   * source/destination type size is equal a shuffle is done that would be
   * equivalent to a simple MOV.
   *
   * For example, if source is a 16-bit type and destination is 32-bit. A 3
   * components .xyz 16-bit vector on SIMD8 would be.
   *
   *    |x1|x2|x3|x4|x5|x6|x7|x8|y1|y2|y3|y4|y5|y6|y7|y8|
   *    |z1|z2|z3|z4|z5|z6|z7|z8|  |  |  |  |  |  |  |  |
   *
   * This helper will return the following 2 32-bit components with the 16-bit
   * values shuffled:
   *
   *    |x1 y1|x2 y2|x3 y3|x4 y4|x5 y5|x6 y6|x7 y7|x8 y8|
   *    |z1   |z2   |z3   |z4   |z5   |z6   |z7   |z8   |
   *
   * For unshuffle, the example would be the opposite, a 64-bit type source
   * and a 32-bit destination. A 2 component .xy 64-bit vector on SIMD8
   * would be:
   *
   *    | x1l   x1h | x2l   x2h | x3l   x3h | x4l   x4h |
   *    | x5l   x5h | x6l   x6h | x7l   x7h | x8l   x8h |
   *    | y1l   y1h | y2l   y2h | y3l   y3h | y4l   y4h |
   *    | y5l   y5h | y6l   y6h | y7l   y7h | y8l   y8h |
   *
   * The returned result would be the following 4 32-bit components unshuffled:
   *
   *    | x1l | x2l | x3l | x4l | x5l | x6l | x7l | x8l |
   *    | x1h | x2h | x3h | x4h | x5h | x6h | x7h | x8h |
   *    | y1l | y2l | y3l | y4l | y5l | y6l | y7l | y8l |
   *    | y1h | y2h | y3h | y4h | y5h | y6h | y7h | y8h |
   *
   * - Source and destination register must not be overlapped.
   * - components units are measured in terms of the smaller type between
   *   source and destination because we are un/shuffling the smaller
   *   components from/into the bigger ones.
   * - first_component parameter allows skipping source components.
   */
   void
   shuffle_src_to_dst(const fs_builder &bld,
                     const fs_reg &dst,
   {
      if (type_sz(src.type) == type_sz(dst.type)) {
      assert(!regions_overlap(dst,
      type_sz(dst.type) * bld.dispatch_width() * components,
   offset(src, bld, first_component),
      for (unsigned i = 0; i < components; i++) {
      bld.MOV(retype(offset(dst, bld, i), src.type),
         } else if (type_sz(src.type) < type_sz(dst.type)) {
      /* Source is shuffled into destination */
   unsigned size_ratio = type_sz(dst.type) / type_sz(src.type);
   assert(!regions_overlap(dst,
      type_sz(dst.type) * bld.dispatch_width() *
   DIV_ROUND_UP(components, size_ratio),
               brw_reg_type shuffle_type =
      brw_reg_type_from_bit_size(8 * type_sz(src.type),
      for (unsigned i = 0; i < components; i++) {
      fs_reg shuffle_component_i =
      subscript(offset(dst, bld, i / size_ratio),
      bld.MOV(shuffle_component_i,
         } else {
      /* Source is unshuffled into destination */
   unsigned size_ratio = type_sz(src.type) / type_sz(dst.type);
   assert(!regions_overlap(dst,
      type_sz(dst.type) * bld.dispatch_width() * components,
   offset(src, bld, first_component / size_ratio),
   type_sz(src.type) * bld.dispatch_width() *
               brw_reg_type shuffle_type =
      brw_reg_type_from_bit_size(8 * type_sz(dst.type),
      for (unsigned i = 0; i < components; i++) {
      fs_reg shuffle_component_i =
      subscript(offset(src, bld, (first_component + i) / size_ratio),
      bld.MOV(retype(offset(dst, bld, i), shuffle_type),
            }
      void
   shuffle_from_32bit_read(const fs_builder &bld,
                           {
               /* This function takes components in units of the destination type while
   * shuffle_src_to_dst takes components in units of the smallest type
   */
   if (type_sz(dst.type) > 4) {
      assert(type_sz(dst.type) == 8);
   first_component *= 2;
                  }
      fs_reg
   setup_imm_df(const fs_builder &bld, double v)
   {
      const struct intel_device_info *devinfo = bld.shader->devinfo;
            if (devinfo->ver >= 8)
            /* gfx7.5 does not support DF immediates straightforward but the DIM
   * instruction allows to set the 64-bit immediate value.
   */
   if (devinfo->platform == INTEL_PLATFORM_HSW) {
      const fs_builder ubld = bld.exec_all().group(1, 0);
   fs_reg dst = ubld.vgrf(BRW_REGISTER_TYPE_DF, 1);
   ubld.DIM(dst, brw_imm_df(v));
               /* gfx7 does not support DF immediates, so we generate a 64-bit constant by
   * writing the low 32-bit of the constant to suboffset 0 of a VGRF and
   * the high 32-bit to suboffset 4 and then applying a stride of 0.
   *
   * Alternatively, we could also produce a normal VGRF (without stride 0)
   * by writing to all the channels in the VGRF, however, that would hit the
   * gfx7 bug where we have to split writes that span more than 1 register
   * into instructions with a width of 4 (otherwise the write to the second
   * register written runs into an execmask hardware bug) which isn't very
   * nice.
   */
   union {
      double d;
   struct {
      uint32_t i1;
                           const fs_builder ubld = bld.exec_all().group(1, 0);
   const fs_reg tmp = ubld.vgrf(BRW_REGISTER_TYPE_UD, 2);
   ubld.MOV(tmp, brw_imm_ud(di.i1));
               }
      fs_reg
   setup_imm_b(const fs_builder &bld, int8_t v)
   {
      const fs_reg tmp = bld.vgrf(BRW_REGISTER_TYPE_B);
   bld.MOV(tmp, brw_imm_w(v));
      }
      fs_reg
   setup_imm_ub(const fs_builder &bld, uint8_t v)
   {
      const fs_reg tmp = bld.vgrf(BRW_REGISTER_TYPE_UB);
   bld.MOV(tmp, brw_imm_uw(v));
      }
