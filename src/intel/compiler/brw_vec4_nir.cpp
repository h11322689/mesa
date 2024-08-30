   /*
   * Copyright © 2015 Intel Corporation
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
      #include "brw_nir.h"
   #include "brw_vec4.h"
   #include "brw_vec4_builder.h"
   #include "brw_vec4_surface_builder.h"
   #include "brw_eu.h"
   #include "nir.h"
   #include "nir_intrinsics.h"
   #include "nir_intrinsics_indices.h"
      using namespace brw;
   using namespace brw::surface_access;
      namespace brw {
      void
   vec4_visitor::emit_nir_code()
   {
      /* Globally set the rounding mode based on the float controls.  gen7 doesn't
   * support 16-bit floats, and gen8 switches to scalar VS.  So we don't need
   * to do any per-instruction mode switching the way the scalar FS handles.
   */
   emit_shader_float_controls_execution_mode();
   if (nir->num_uniforms > 0)
               }
      void
   vec4_visitor::nir_setup_uniforms()
   {
         }
      void
   vec4_visitor::nir_emit_impl(nir_function_impl *impl)
   {
                  }
      void
   vec4_visitor::nir_emit_cf_list(exec_list *list)
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
   vec4_visitor::nir_emit_if(nir_if *if_stmt)
   {
      /* First, put the condition in f0 */
   src_reg condition = get_nir_src(if_stmt->condition, BRW_REGISTER_TYPE_D, 1);
   vec4_instruction *inst = emit(MOV(dst_null_d(), condition));
            /* We can just predicate based on the X channel, as the condition only
   * goes on its own line */
                     /* note: if the else is empty, dead CF elimination will remove it */
                        }
      void
   vec4_visitor::nir_emit_loop(nir_loop *loop)
   {
      assert(!nir_loop_has_continue_construct(loop));
                        }
      void
   vec4_visitor::nir_emit_block(nir_block *block)
   {
      nir_foreach_instr(instr, block) {
            }
      void
   vec4_visitor::nir_emit_instr(nir_instr *instr)
   {
               switch (instr->type) {
   case nir_instr_type_load_const:
      nir_emit_load_const(nir_instr_as_load_const(instr));
         case nir_instr_type_intrinsic:
      nir_emit_intrinsic(nir_instr_as_intrinsic(instr));
         case nir_instr_type_alu:
      nir_emit_alu(nir_instr_as_alu(instr));
         case nir_instr_type_jump:
      nir_emit_jump(nir_instr_as_jump(instr));
         case nir_instr_type_tex:
      nir_emit_texture(nir_instr_as_tex(instr));
         case nir_instr_type_undef:
      nir_emit_undef(nir_instr_as_undef(instr));
         default:
            }
      static dst_reg
   dst_reg_for_nir_reg(vec4_visitor *v, nir_def *handle,
         {
      nir_intrinsic_instr *decl = nir_reg_get_decl(handle);
   dst_reg reg = v->nir_ssa_values[handle->index];
   if (nir_intrinsic_bit_size(decl) == 64)
            reg = offset(reg, 8, base_offset);
   if (indirect) {
      reg.reladdr =
      new(v->mem_ctx) src_reg(v->get_nir_src(*indirect,
         }
      }
      dst_reg
   vec4_visitor::get_nir_def(const nir_def &def)
   {
      nir_intrinsic_instr *store_reg = nir_store_reg_for_def(&def);
   if (!store_reg) {
      dst_reg dst =
         if (def.bit_size == 64)
         nir_ssa_values[def.index] = dst;
      } else {
      nir_src *indirect =
                  dst_reg dst = dst_reg_for_nir_reg(this, store_reg->src[1].ssa,
               dst.writemask = nir_intrinsic_write_mask(store_reg);
         }
      dst_reg
   vec4_visitor::get_nir_def(const nir_def &def, enum brw_reg_type type)
   {
         }
      dst_reg
   vec4_visitor::get_nir_def(const nir_def &def, nir_alu_type type)
   {
         }
      src_reg
   vec4_visitor::get_nir_src(const nir_src &src, enum brw_reg_type type,
         {
               dst_reg reg;
   if (load_reg) {
      nir_src *indirect =
                  reg = dst_reg_for_nir_reg(this, load_reg->src[0].ssa,
            } else {
                           src_reg reg_as_src = src_reg(reg);
   reg_as_src.swizzle = brw_swizzle_for_size(num_components);
      }
      src_reg
   vec4_visitor::get_nir_src(const nir_src &src, nir_alu_type type,
         {
      return get_nir_src(src, brw_type_for_nir_type(devinfo, type),
      }
      src_reg
   vec4_visitor::get_nir_src(const nir_src &src, unsigned num_components)
   {
      /* if type is not specified, default to signed int */
      }
      src_reg
   vec4_visitor::get_nir_src_imm(const nir_src &src)
   {
      assert(nir_src_num_components(src) == 1);
   assert(nir_src_bit_size(src) == 32);
   return nir_src_is_const(src) ? src_reg(brw_imm_d(nir_src_as_int(src))) :
      }
      src_reg
   vec4_visitor::get_indirect_offset(nir_intrinsic_instr *instr)
   {
               if (nir_src_is_const(*offset_src)) {
      /* The only constant offset we should find is 0.  brw_nir.c's
   * add_const_offset_to_base() will fold other constant offsets
   * into the base index.
   */
   assert(nir_src_as_uint(*offset_src) == 0);
                  }
      static src_reg
   setup_imm_df(const vec4_builder &bld, double v)
   {
      const intel_device_info *devinfo = bld.shader->devinfo;
            /* gfx7.5 does not support DF immediates straightforward but the DIM
   * instruction allows to set the 64-bit immediate value.
   */
   if (devinfo->verx10 == 75) {
      const vec4_builder ubld = bld.exec_all();
   const dst_reg dst = bld.vgrf(BRW_REGISTER_TYPE_DF);
   ubld.DIM(dst, brw_imm_df(v));
               /* gfx7 does not support DF immediates */
   union {
      double d;
   struct {
      uint32_t i1;
                           /* Write the low 32-bit of the constant to the X:UD channel and the
   * high 32-bit to the Y:UD channel to build the constant in a VGRF.
   * We have to do this twice (offset 0 and offset 1), since a DF VGRF takes
   * two SIMD8 registers in SIMD4x2 execution. Finally, return a swizzle
   * XXXX so any access to the VGRF only reads the constant data in these
   * channels.
   */
   const dst_reg tmp = bld.vgrf(BRW_REGISTER_TYPE_UD, 2);
   for (unsigned n = 0; n < 2; n++) {
      const vec4_builder ubld = bld.exec_all().group(4, n);
   ubld.MOV(writemask(offset(tmp, 8, n), WRITEMASK_X), brw_imm_ud(di.i1));
                  }
      void
   vec4_visitor::nir_emit_load_const(nir_load_const_instr *instr)
   {
               if (instr->def.bit_size == 64) {
      reg = dst_reg(VGRF, alloc.allocate(2));
      } else {
      reg = dst_reg(VGRF, alloc.allocate(1));
               const vec4_builder ibld = vec4_builder(this).at_end();
            /* @FIXME: consider emitting vector operations to save some MOVs in
   * cases where the components are representable in 8 bits.
   * For now, we emit a MOV for each distinct value.
   */
   for (unsigned i = 0; i < instr->def.num_components; i++) {
               if ((remaining & writemask) == 0)
            for (unsigned j = i; j < instr->def.num_components; j++) {
      if ((instr->def.bit_size == 32 &&
      instr->value[i].u32 == instr->value[j].u32) ||
   (instr->def.bit_size == 64 &&
   instr->value[i].f64 == instr->value[j].f64)) {
                  reg.writemask = writemask;
   if (instr->def.bit_size == 64) {
         } else {
                              /* Set final writemask */
               }
      src_reg
   vec4_visitor::get_nir_ssbo_intrinsic_index(nir_intrinsic_instr *instr)
   {
      /* SSBO stores are weird in that their index is in src[1] */
            if (nir_src_is_const(instr->src[src])) {
         } else {
            }
      void
   vec4_visitor::nir_emit_intrinsic(nir_intrinsic_instr *instr)
   {
      dst_reg dest;
            switch (instr->intrinsic) {
   case nir_intrinsic_decl_reg: {
      unsigned bit_size = nir_intrinsic_bit_size(instr);
   unsigned array_elems = nir_intrinsic_num_array_elems(instr);
   if (array_elems == 0)
            const unsigned num_regs = array_elems * DIV_ROUND_UP(bit_size, 32);
   dst_reg reg(VGRF, alloc.allocate(num_regs));
   if (bit_size == 64)
            nir_ssa_values[instr->def.index] = reg;
               case nir_intrinsic_load_reg:
   case nir_intrinsic_load_reg_indirect:
   case nir_intrinsic_store_reg:
   case nir_intrinsic_store_reg_indirect:
      /* Nothing to do with these. */
         case nir_intrinsic_load_input: {
      assert(instr->def.bit_size == 32);
   /* We set EmitNoIndirectInput for VS */
                     src = src_reg(ATTR, nir_intrinsic_base(instr) + load_offset,
                  /* Swizzle source based on component layout qualifier */
   src.swizzle = BRW_SWZ_COMP_INPUT(nir_intrinsic_component(instr));
   emit(MOV(dest, src));
               case nir_intrinsic_store_output: {
      assert(nir_src_bit_size(instr->src[0]) == 32);
   unsigned store_offset = nir_src_as_uint(instr->src[1]);
   int varying = nir_intrinsic_base(instr) + store_offset;
   src = get_nir_src(instr->src[0], BRW_REGISTER_TYPE_F,
            unsigned c = nir_intrinsic_component(instr);
   output_reg[varying][c] = dst_reg(src);
   output_num_components[varying][c] = instr->num_components;
               case nir_intrinsic_get_ssbo_size: {
      assert(nir_src_num_components(instr->src[0]) == 1);
   unsigned ssbo_index = nir_src_is_const(instr->src[0]) ?
            dst_reg result_dst = get_nir_def(instr->def);
   vec4_instruction *inst = new(mem_ctx)
            inst->base_mrf = 2;
   inst->mlen = 1; /* always at least one */
            /* MRF for the first parameter */
   src_reg lod = brw_imm_d(0);
   int param_base = inst->base_mrf;
   int writemask = WRITEMASK_X;
            emit(inst);
               case nir_intrinsic_store_ssbo: {
               /* brw_nir_lower_mem_access_bit_sizes takes care of this */
   assert(nir_src_bit_size(instr->src[0]) == 32);
   assert(nir_intrinsic_write_mask(instr) ==
            src_reg surf_index = get_nir_ssbo_intrinsic_index(instr);
   src_reg offset_reg = retype(get_nir_src_imm(instr->src[2]),
            /* Value */
            /* IvyBridge does not have a native SIMD4x2 untyped write message so untyped
   * writes will use SIMD8 mode. In order to hide this and keep symmetry across
   * typed and untyped messages and across hardware platforms, the
   * current implementation of the untyped messages will transparently convert
   * the SIMD4x2 payload into an equivalent SIMD8 payload by transposing it
   * and enabling only channel X on the SEND instruction.
   *
   * The above, works well for full vector writes, but not for partial writes
   * where we want to write some channels and not others, like when we have
   * code such as v.xyw = vec3(1,2,4). Because the untyped write messages are
   * quite restrictive with regards to the channel enables we can configure in
   * the message descriptor (not all combinations are allowed) we cannot simply
   * implement these scenarios with a single message while keeping the
   * aforementioned symmetry in the implementation. For now we de decided that
   * it is better to keep the symmetry to reduce complexity, so in situations
   * such as the one described we end up emitting two untyped write messages
   * (one for xy and another for w).
   *
   * The code below packs consecutive channels into a single write message,
   * detects gaps in the vector write and if needed, sends a second message
   * with the remaining channels. If in the future we decide that we want to
   * emit a single message at the expense of losing the symmetry in the
   * implementation we can:
   *
   * 1) For IvyBridge: Only use the red channel of the untyped write SIMD8
   *    message payload. In this mode we can write up to 8 offsets and dwords
   *    to the red channel only (for the two vec4s in the SIMD4x2 execution)
   *    and select which of the 8 channels carry data to write by setting the
   *    appropriate writemask in the dst register of the SEND instruction.
   *    It would require to write a new generator opcode specifically for
   *    IvyBridge since we would need to prepare a SIMD8 payload that could
   *    use any channel, not just X.
   *
   * 2) For Haswell+: Simply send a single write message but set the writemask
   *    on the dst of the SEND instruction to select the channels we want to
   *    write. It would require to modify the current messages to receive
   *    and honor the writemask provided.
   */
   const vec4_builder bld = vec4_builder(this).at_end()
            emit_untyped_write(bld, surf_index, offset_reg, val_reg,
                           case nir_intrinsic_load_ssbo: {
               /* brw_nir_lower_mem_access_bit_sizes takes care of this */
            src_reg surf_index = get_nir_ssbo_intrinsic_index(instr);
   src_reg offset_reg = retype(get_nir_src_imm(instr->src[1]),
            /* Read the vector */
   const vec4_builder bld = vec4_builder(this).at_end()
            src_reg read_result = emit_untyped_read(bld, surf_index, offset_reg,
               dst_reg dest = get_nir_def(instr->def);
   read_result.type = dest.type;
   read_result.swizzle = brw_swizzle_for_size(instr->num_components);
   emit(MOV(dest, read_result));
               case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
      nir_emit_ssbo_atomic(lsc_op_to_legacy_atomic(lsc_aop_for_nir_intrinsic(instr)), instr);
         case nir_intrinsic_load_vertex_id:
            case nir_intrinsic_load_vertex_id_zero_base:
   case nir_intrinsic_load_base_vertex:
   case nir_intrinsic_load_instance_id:
   case nir_intrinsic_load_base_instance:
   case nir_intrinsic_load_draw_id:
   case nir_intrinsic_load_invocation_id:
            case nir_intrinsic_load_uniform: {
      /* Offsets are in bytes but they should always be multiples of 4 */
                     src = src_reg(dst_reg(UNIFORM, nir_intrinsic_base(instr) / 16));
            /* Uniforms don't actually have to be vec4 aligned.  In the case that
   * it isn't, we have to use a swizzle to shift things around.  They
   * do still have the std140 alignment requirement that vec2's have to
   * be vec2-aligned and vec3's and vec4's have to be vec4-aligned.
   *
   * The swizzle also works in the indirect case as the generator adds
   * the swizzle to the offset for us.
   */
   const int type_size = type_sz(src.type);
   unsigned shift = (nir_intrinsic_base(instr) % 16) / type_size;
            if (nir_src_is_const(instr->src[0])) {
      const unsigned load_offset = nir_src_as_uint(instr->src[0]);
                  src.swizzle = brw_swizzle_for_size(instr->num_components);
   dest.writemask = brw_writemask_for_size(instr->num_components);
   unsigned offset = load_offset + shift * type_size;
   src.offset = ROUND_DOWN_TO(offset, 16);
   shift = (offset % 16) / type_size;
                     } else {
      /* Uniform arrays are vec4 aligned, because of std140 alignment
   * rules.
                                          emit(SHADER_OPCODE_MOV_INDIRECT, dest, src,
      }
               case nir_intrinsic_load_ubo: {
                        if (nir_src_is_const(instr->src[0])) {
      /* The block index is a constant, so just emit the binding table entry
   * as an immediate.
   */
   const unsigned index = nir_src_as_uint(instr->src[0]);
      } else {
      /* The block index is not a constant. Evaluate the index expression
   * per-channel and add the base UBO index; we have to select a value
   * from any live channel.
   */
   surf_index = src_reg(this, glsl_type::uint_type);
   emit(MOV(dst_reg(surf_index), get_nir_src(instr->src[0], nir_type_int32,
                     src_reg push_reg;
   src_reg offset_reg;
   if (nir_src_is_const(instr->src[1])) {
      unsigned load_offset = nir_src_as_uint(instr->src[1]);
                  /* See if we've selected this as a push constant candidate */
   if (nir_src_is_const(instr->src[0])) {
                     for (int i = 0; i < 4; i++) {
      const struct brw_ubo_range *range = &prog_data->base.ubo_ranges[i];
                           push_reg = src_reg(dst_reg(UNIFORM, UBO_START + i));
   push_reg.type = dest.type;
   push_reg.offset = aligned_offset - 32 * range->start;
               } else {
      offset_reg = src_reg(this, glsl_type::uint_type);
   emit(MOV(dst_reg(offset_reg),
               src_reg packed_consts;
   if (push_reg.file != BAD_FILE) {
         } else if (instr->def.bit_size == 32) {
      packed_consts = src_reg(this, glsl_type::vec4_type);
   emit_pull_constant_load_reg(dst_reg(packed_consts),
                        } else {
                     emit_pull_constant_load_reg(dst_reg(temp_float),
         if (offset_reg.file == IMM)
         else
         emit_pull_constant_load_reg(dst_reg(byte_offset(temp_float, REG_SIZE)),
                  packed_consts = src_reg(this, glsl_type::dvec4_type);
               packed_consts.swizzle = brw_swizzle_for_size(instr->num_components);
   if (nir_src_is_const(instr->src[1])) {
      unsigned load_offset = nir_src_as_uint(instr->src[1]);
   unsigned type_size = type_sz(dest.type);
   packed_consts.swizzle +=
      BRW_SWIZZLE4(load_offset % 16 / type_size,
                                             case nir_intrinsic_barrier: {
      if (nir_intrinsic_memory_scope(instr) == SCOPE_NONE)
         const vec4_builder bld =
         const dst_reg tmp = bld.vgrf(BRW_REGISTER_TYPE_UD);
   vec4_instruction *fence =
         fence->sfid = GFX7_SFID_DATAPORT_DATA_CACHE;
               case nir_intrinsic_shader_clock: {
      /* We cannot do anything if there is an event, so ignore it for now */
   const src_reg shader_clock = get_timestamp();
            dest = get_nir_def(instr->def, type);
   emit(MOV(dest, shader_clock));
               default:
            }
      void
   vec4_visitor::nir_emit_ssbo_atomic(int op, nir_intrinsic_instr *instr)
   {
      dst_reg dest;
   if (nir_intrinsic_infos[instr->intrinsic].has_dest)
            src_reg surface = get_nir_ssbo_intrinsic_index(instr);
   src_reg offset = get_nir_src(instr->src[1], 1);
   src_reg data1;
   if (op != BRW_AOP_INC && op != BRW_AOP_DEC && op != BRW_AOP_PREDEC)
         src_reg data2;
   if (op == BRW_AOP_CMPWR)
            /* Emit the actual atomic operation operation */
   const vec4_builder bld =
            src_reg atomic_result = emit_untyped_atomic(bld, surface, offset,
                           dest.type = atomic_result.type;
      }
      static unsigned
   brw_swizzle_for_nir_swizzle(uint8_t swizzle[4])
   {
         }
      bool
   vec4_visitor::optimize_predicate(nir_alu_instr *instr,
         {
      if (instr->src[0].src.ssa->parent_instr->type != nir_instr_type_alu)
            nir_alu_instr *cmp_instr =
            switch (cmp_instr->op) {
   case nir_op_b32any_fnequal2:
   case nir_op_b32any_inequal2:
   case nir_op_b32any_fnequal3:
   case nir_op_b32any_inequal3:
   case nir_op_b32any_fnequal4:
   case nir_op_b32any_inequal4:
      *predicate = BRW_PREDICATE_ALIGN16_ANY4H;
      case nir_op_b32all_fequal2:
   case nir_op_b32all_iequal2:
   case nir_op_b32all_fequal3:
   case nir_op_b32all_iequal3:
   case nir_op_b32all_fequal4:
   case nir_op_b32all_iequal4:
      *predicate = BRW_PREDICATE_ALIGN16_ALL4H;
      default:
                  unsigned size_swizzle =
            src_reg op[2];
   assert(nir_op_infos[cmp_instr->op].num_inputs == 2);
   for (unsigned i = 0; i < 2; i++) {
      nir_alu_type type = nir_op_infos[cmp_instr->op].input_types[i];
   unsigned bit_size = nir_src_bit_size(cmp_instr->src[i].src);
   type = (nir_alu_type) (((unsigned) type) | bit_size);
   op[i] = get_nir_src(cmp_instr->src[i].src, type, 4);
   unsigned base_swizzle =
                     emit(CMP(dst_null_d(), op[0], op[1],
               }
      void
   vec4_visitor::emit_conversion_from_double(dst_reg dst, src_reg src)
   {
      enum opcode op;
   switch (dst.type) {
   case BRW_REGISTER_TYPE_D:
      op = VEC4_OPCODE_DOUBLE_TO_D32;
      case BRW_REGISTER_TYPE_UD:
      op = VEC4_OPCODE_DOUBLE_TO_U32;
      case BRW_REGISTER_TYPE_F:
      op = VEC4_OPCODE_DOUBLE_TO_F32;
      default:
                  dst_reg temp = dst_reg(this, glsl_type::dvec4_type);
   emit(MOV(temp, src));
   dst_reg temp2 = dst_reg(this, glsl_type::dvec4_type);
            emit(VEC4_OPCODE_PICK_LOW_32BIT, retype(temp2, dst.type), src_reg(temp2));
      }
      void
   vec4_visitor::emit_conversion_to_double(dst_reg dst, src_reg src)
   {
      dst_reg tmp_dst = dst_reg(src_reg(this, glsl_type::dvec4_type));
   src_reg tmp_src = retype(src_reg(this, glsl_type::vec4_type), src.type);
   emit(MOV(dst_reg(tmp_src), src));
   emit(VEC4_OPCODE_TO_DOUBLE, tmp_dst, tmp_src);
      }
      /**
   * Try to use an immediate value for a source
   *
   * In cases of flow control, constant propagation is sometimes unable to
   * determine that a register contains a constant value.  To work around this,
   * try to emit a literal as one of the sources.  If \c try_src0_also is set,
   * \c op[0] will also be tried for an immediate value.
   *
   * If \c op[0] is modified, the operands will be exchanged so that \c op[1]
   * will always be the immediate value.
   *
   * \return The index of the source that was modified, 0 or 1, if successful.
   * Otherwise, -1.
   *
   * \param op - Operands to the instruction
   * \param try_src0_also - True if \c op[0] should also be a candidate for
   *                        getting an immediate value.  This should only be set
   *                        for commutative operations.
   */
   static int
   try_immediate_source(const nir_alu_instr *instr, src_reg *op,
         {
               /* MOV should be the only single-source instruction passed to this
   * function.  Any other unary instruction with a constant source should
   * have been constant-folded away!
   */
   assert(nir_op_infos[instr->op].num_inputs > 1 ||
            if (instr->op != nir_op_mov &&
      nir_src_bit_size(instr->src[1].src) == 32 &&
   nir_src_is_const(instr->src[1].src)) {
      } else if (try_src0_also &&
         nir_src_bit_size(instr->src[0].src) == 32 &&
         } else {
                           switch (old_type) {
   case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_UD: {
      int first_comp = -1;
            for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
      if (nir_alu_instr_channel_used(instr, idx, i)) {
      if (first_comp < 0) {
      first_comp = i;
   d = nir_src_comp_as_int(instr->src[idx].src,
      } else if (d != nir_src_comp_as_int(instr->src[idx].src,
                                       if (op[idx].abs)
            if (op[idx].negate)
            op[idx] = retype(src_reg(brw_imm_d(d)), old_type);
               case BRW_REGISTER_TYPE_F: {
      int first_comp = -1;
   float f[NIR_MAX_VEC_COMPONENTS] = { 0.0f };
            for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
      if (nir_alu_instr_channel_used(instr, idx, i)) {
      f[i] = nir_src_comp_as_float(instr->src[idx].src,
         if (first_comp < 0) {
         } else if (f[first_comp] != f[i]) {
                        if (is_scalar) {
                                    op[idx] = src_reg(brw_imm_f(f[first_comp]));
      } else {
                                                         const int vf = brw_float_to_vf(f[i]);
                              op[idx] = src_reg(brw_imm_vf4(vf_values[0], vf_values[1],
      }
               default:
                  /* If the instruction has more than one source, the instruction format only
   * allows source 1 to be an immediate value.  If the immediate value was
   * source 0, then the sources must be exchanged.
   */
   if (idx == 0 && instr->op != nir_op_mov) {
      src_reg tmp = op[0];
   op[0] = op[1];
                  }
      void
   vec4_visitor::fix_float_operands(src_reg op[3], nir_alu_instr *instr)
   {
               for (unsigned i = 0; i < 2; i++) {
      if (!nir_src_is_const(instr->src[i].src))
            for (unsigned j = i + 1; j < 3; j++) {
                                    if (nir_alu_srcs_equal(instr, instr, i, j)) {
                              fixed[i] = true;
      } else if (nir_alu_srcs_negative_equal(instr, instr, i, j)) {
                                    fixed[i] = true;
                     for (unsigned i = 0; i < 3; i++) {
      if (!fixed[i])
         }
      static bool
   const_src_fits_in_16_bits(const nir_src &src, brw_reg_type type)
   {
      assert(nir_src_is_const(src));
   if (brw_reg_type_is_unsigned_integer(type)) {
         } else {
      const int64_t c = nir_src_comp_as_int(src, 0);
         }
      void
   vec4_visitor::nir_emit_alu(nir_alu_instr *instr)
   {
               nir_alu_type dst_type = (nir_alu_type) (nir_op_infos[instr->op].output_type |
         dst_reg dst = get_nir_def(instr->def, dst_type);
            src_reg op[4];
   for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      nir_alu_type src_type = (nir_alu_type)
      (nir_op_infos[instr->op].input_types[i] |
      op[i] = get_nir_src(instr->src[i].src, src_type, 4);
            #ifndef NDEBUG
      /* On Gen7 and earlier, no functionality is exposed that should allow 8-bit
   * integer types to ever exist.
   */
   for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++)
      #endif
         switch (instr->op) {
   case nir_op_mov:
      try_immediate_source(instr, &op[0], true);
   inst = emit(MOV(dst, op[0]));
         case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
            case nir_op_i2f32:
   case nir_op_u2f32:
      inst = emit(MOV(dst, op[0]));
         case nir_op_f2f32:
   case nir_op_f2i32:
   case nir_op_f2u32:
      if (nir_src_bit_size(instr->src[0].src) == 64)
         else
               case nir_op_f2f64:
   case nir_op_i2f64:
   case nir_op_u2f64:
      emit_conversion_to_double(dst, op[0]);
         case nir_op_fsat:
      inst = emit(MOV(dst, op[0]));
   inst->saturate = true;
         case nir_op_fneg:
   case nir_op_ineg:
      op[0].negate = true;
   inst = emit(MOV(dst, op[0]));
         case nir_op_fabs:
   case nir_op_iabs:
      op[0].negate = false;
   op[0].abs = true;
   inst = emit(MOV(dst, op[0]));
         case nir_op_iadd:
      assert(instr->def.bit_size < 64);
      case nir_op_fadd:
      try_immediate_source(instr, op, true);
   inst = emit(ADD(dst, op[0], op[1]));
         case nir_op_uadd_sat:
      assert(instr->def.bit_size < 64);
   inst = emit(ADD(dst, op[0], op[1]));
   inst->saturate = true;
         case nir_op_fmul:
      try_immediate_source(instr, op, true);
   inst = emit(MUL(dst, op[0], op[1]));
         case nir_op_imul: {
               /* For integer multiplication, the MUL uses the low 16 bits of one of
   * the operands (src0 through SNB, src1 on IVB and later). The MACH
   * accumulates in the contribution of the upper 16 bits of that
   * operand. If we can determine that one of the args is in the low
   * 16 bits, though, we can just emit a single MUL.
   */
   if (nir_src_is_const(instr->src[0].src) &&
      nir_alu_instr_src_read_mask(instr, 0) == 1 &&
   const_src_fits_in_16_bits(instr->src[0].src, op[0].type)) {
   if (devinfo->ver < 7)
         else
      } else if (nir_src_is_const(instr->src[1].src) &&
            nir_alu_instr_src_read_mask(instr, 1) == 1 &&
   if (devinfo->ver < 7)
         else
      } else {
               emit(MUL(acc, op[0], op[1]));
   emit(MACH(dst_null_d(), op[0], op[1]));
      }
               case nir_op_imul_high:
   case nir_op_umul_high: {
      assert(instr->def.bit_size < 64);
            emit(MUL(acc, op[0], op[1]));
   emit(MACH(dst, op[0], op[1]));
               case nir_op_frcp:
      inst = emit_math(SHADER_OPCODE_RCP, dst, op[0]);
         case nir_op_fexp2:
      inst = emit_math(SHADER_OPCODE_EXP2, dst, op[0]);
         case nir_op_flog2:
      inst = emit_math(SHADER_OPCODE_LOG2, dst, op[0]);
         case nir_op_fsin:
      inst = emit_math(SHADER_OPCODE_SIN, dst, op[0]);
         case nir_op_fcos:
      inst = emit_math(SHADER_OPCODE_COS, dst, op[0]);
         case nir_op_idiv:
   case nir_op_udiv:
      assert(instr->def.bit_size < 64);
   emit_math(SHADER_OPCODE_INT_QUOTIENT, dst, op[0], op[1]);
         case nir_op_umod:
   case nir_op_irem:
      /* According to the sign table for INT DIV in the Ivy Bridge PRM, it
   * appears that our hardware just does the right thing for signed
   * remainder.
   */
   assert(instr->def.bit_size < 64);
   emit_math(SHADER_OPCODE_INT_REMAINDER, dst, op[0], op[1]);
         case nir_op_imod: {
      /* Get a regular C-style remainder.  If a % b == 0, set the predicate. */
            /* Math instructions don't support conditional mod */
   inst = emit(MOV(dst_null_d(), src_reg(dst)));
            /* Now, we need to determine if signs of the sources are different.
   * When we XOR the sources, the top bit is 0 if they are the same and 1
   * if they are different.  We can then use a conditional modifier to
   * turn that into a predicate.  This leads us to an XOR.l instruction.
   *
   * Technically, according to the PRM, you're not allowed to use .l on a
   * XOR instruction.  However, empirical experiments and Curro's reading
   * of the simulator source both indicate that it's safe.
   */
   src_reg tmp = src_reg(this, glsl_type::ivec4_type);
   inst = emit(XOR(dst_reg(tmp), op[0], op[1]));
   inst->predicate = BRW_PREDICATE_NORMAL;
            /* If the result of the initial remainder operation is non-zero and the
   * two sources have different signs, add in a copy of op[1] to get the
   * final integer modulus value.
   */
   inst = emit(ADD(dst, src_reg(dst), op[1]));
   inst->predicate = BRW_PREDICATE_NORMAL;
               case nir_op_ldexp:
            case nir_op_fsqrt:
      inst = emit_math(SHADER_OPCODE_SQRT, dst, op[0]);
         case nir_op_frsq:
      inst = emit_math(SHADER_OPCODE_RSQ, dst, op[0]);
         case nir_op_fpow:
      inst = emit_math(SHADER_OPCODE_POW, dst, op[0], op[1]);
         case nir_op_uadd_carry: {
      assert(instr->def.bit_size < 64);
            emit(ADDC(dst_null_ud(), op[0], op[1]));
   emit(MOV(dst, src_reg(acc)));
               case nir_op_usub_borrow: {
      assert(instr->def.bit_size < 64);
            emit(SUBB(dst_null_ud(), op[0], op[1]));
   emit(MOV(dst, src_reg(acc)));
               case nir_op_ftrunc:
      inst = emit(RNDZ(dst, op[0]));
   if (devinfo->ver < 6) {
      inst->conditional_mod = BRW_CONDITIONAL_R;
   inst = emit(ADD(dst, src_reg(dst), brw_imm_f(1.0f)));
   inst->predicate = BRW_PREDICATE_NORMAL;
      }
         case nir_op_fceil: {
      src_reg tmp = src_reg(this, glsl_type::float_type);
            op[0].negate = !op[0].negate;
   emit(RNDD(dst_reg(tmp), op[0]));
   tmp.negate = true;
   inst = emit(MOV(dst, tmp));
               case nir_op_ffloor:
      inst = emit(RNDD(dst, op[0]));
         case nir_op_ffract:
      inst = emit(FRC(dst, op[0]));
         case nir_op_fround_even:
      inst = emit(RNDE(dst, op[0]));
   if (devinfo->ver < 6) {
      inst->conditional_mod = BRW_CONDITIONAL_R;
   inst = emit(ADD(dst, src_reg(dst), brw_imm_f(1.0f)));
   inst->predicate = BRW_PREDICATE_NORMAL;
      }
         case nir_op_fquantize2f16: {
      /* See also vec4_visitor::emit_pack_half_2x16() */
   src_reg tmp16 = src_reg(this, glsl_type::uvec4_type);
   src_reg tmp32 = src_reg(this, glsl_type::vec4_type);
            /* Check for denormal */
   src_reg abs_src0 = op[0];
   abs_src0.abs = true;
   emit(CMP(dst_null_f(), abs_src0, brw_imm_f(ldexpf(1.0, -14)),
         /* Get the appropriately signed zero */
   emit(AND(retype(dst_reg(zero), BRW_REGISTER_TYPE_UD),
               /* Do the actual F32 -> F16 -> F32 conversion */
   emit(F32TO16(dst_reg(tmp16), op[0]));
   emit(F16TO32(dst_reg(tmp32), tmp16));
   /* Select that or zero based on normal status */
   inst = emit(BRW_OPCODE_SEL, dst, zero, tmp32);
   inst->predicate = BRW_PREDICATE_NORMAL;
               case nir_op_imin:
   case nir_op_umin:
      assert(instr->def.bit_size < 64);
      case nir_op_fmin:
      try_immediate_source(instr, op, true);
   inst = emit_minmax(BRW_CONDITIONAL_L, dst, op[0], op[1]);
         case nir_op_imax:
   case nir_op_umax:
      assert(instr->def.bit_size < 64);
      case nir_op_fmax:
      try_immediate_source(instr, op, true);
   inst = emit_minmax(BRW_CONDITIONAL_GE, dst, op[0], op[1]);
         case nir_op_fddx:
   case nir_op_fddx_coarse:
   case nir_op_fddx_fine:
   case nir_op_fddy:
   case nir_op_fddy_coarse:
   case nir_op_fddy_fine:
            case nir_op_ilt32:
   case nir_op_ult32:
   case nir_op_ige32:
   case nir_op_uge32:
   case nir_op_ieq32:
   case nir_op_ine32:
      assert(instr->def.bit_size < 64);
      case nir_op_flt32:
   case nir_op_fge32:
   case nir_op_feq32:
   case nir_op_fneu32: {
      enum brw_conditional_mod conditional_mod =
            if (nir_src_bit_size(instr->src[0].src) < 64) {
      /* If the order of the sources is changed due to an immediate value,
   * then the condition must also be changed.
   */
                     } else {
      /* Produce a 32-bit boolean result from the DF comparison by selecting
   * only the low 32-bit in each DF produced. Do this in a temporary
   * so we can then move from there to the result using align16 again
   * to honor the original writemask.
   */
   dst_reg temp = dst_reg(this, glsl_type::dvec4_type);
   emit(CMP(temp, op[0], op[1], conditional_mod));
   dst_reg result = dst_reg(this, glsl_type::bvec4_type);
   emit(VEC4_OPCODE_PICK_LOW_32BIT, result, src_reg(temp));
      }
               case nir_op_b32all_iequal2:
   case nir_op_b32all_iequal3:
   case nir_op_b32all_iequal4:
      assert(instr->def.bit_size < 64);
      case nir_op_b32all_fequal2:
   case nir_op_b32all_fequal3:
   case nir_op_b32all_fequal4: {
      unsigned swiz =
            emit(CMP(dst_null_d(), swizzle(op[0], swiz), swizzle(op[1], swiz),
         emit(MOV(dst, brw_imm_d(0)));
   inst = emit(MOV(dst, brw_imm_d(~0)));
   inst->predicate = BRW_PREDICATE_ALIGN16_ALL4H;
               case nir_op_b32any_inequal2:
   case nir_op_b32any_inequal3:
   case nir_op_b32any_inequal4:
      assert(instr->def.bit_size < 64);
      case nir_op_b32any_fnequal2:
   case nir_op_b32any_fnequal3:
   case nir_op_b32any_fnequal4: {
      unsigned swiz =
            emit(CMP(dst_null_d(), swizzle(op[0], swiz), swizzle(op[1], swiz),
            emit(MOV(dst, brw_imm_d(0)));
   inst = emit(MOV(dst, brw_imm_d(~0)));
   inst->predicate = BRW_PREDICATE_ALIGN16_ANY4H;
               case nir_op_inot:
      assert(instr->def.bit_size < 64);
   emit(NOT(dst, op[0]));
         case nir_op_ixor:
      assert(instr->def.bit_size < 64);
   try_immediate_source(instr, op, true);
   emit(XOR(dst, op[0], op[1]));
         case nir_op_ior:
      assert(instr->def.bit_size < 64);
   try_immediate_source(instr, op, true);
   emit(OR(dst, op[0], op[1]));
         case nir_op_iand:
      assert(instr->def.bit_size < 64);
   try_immediate_source(instr, op, true);
   emit(AND(dst, op[0], op[1]));
         case nir_op_b2i32:
   case nir_op_b2f32:
   case nir_op_b2f64:
      if (instr->def.bit_size > 32) {
      assert(dst.type == BRW_REGISTER_TYPE_DF);
      } else {
         }
         case nir_op_unpack_half_2x16_split_x:
   case nir_op_unpack_half_2x16_split_y:
   case nir_op_pack_half_2x16_split:
            case nir_op_unpack_snorm_2x16:
   case nir_op_unpack_unorm_2x16:
   case nir_op_pack_snorm_2x16:
   case nir_op_pack_unorm_2x16:
            case nir_op_pack_uvec4_to_uint:
            case nir_op_pack_uvec2_to_uint: {
      dst_reg tmp1 = dst_reg(this, glsl_type::uint_type);
   tmp1.writemask = WRITEMASK_X;
   op[0].swizzle = BRW_SWIZZLE_YYYY;
            dst_reg tmp2 = dst_reg(this, glsl_type::uint_type);
   tmp2.writemask = WRITEMASK_X;
   op[0].swizzle = BRW_SWIZZLE_XXXX;
            emit(OR(dst, src_reg(tmp1), src_reg(tmp2)));
               case nir_op_pack_64_2x32_split: {
      dst_reg result = dst_reg(this, glsl_type::dvec4_type);
   dst_reg tmp = dst_reg(this, glsl_type::uvec4_type);
   emit(MOV(tmp, retype(op[0], BRW_REGISTER_TYPE_UD)));
   emit(VEC4_OPCODE_SET_LOW_32BIT, result, src_reg(tmp));
   emit(MOV(tmp, retype(op[1], BRW_REGISTER_TYPE_UD)));
   emit(VEC4_OPCODE_SET_HIGH_32BIT, result, src_reg(tmp));
   emit(MOV(dst, src_reg(result)));
               case nir_op_unpack_64_2x32_split_x:
   case nir_op_unpack_64_2x32_split_y: {
      enum opcode oper = (instr->op == nir_op_unpack_64_2x32_split_x) ?
         dst_reg tmp = dst_reg(this, glsl_type::dvec4_type);
   emit(MOV(tmp, op[0]));
   dst_reg tmp2 = dst_reg(this, glsl_type::uvec4_type);
   emit(oper, tmp2, src_reg(tmp));
   emit(MOV(dst, src_reg(tmp2)));
               case nir_op_unpack_half_2x16:
      /* As NIR does not guarantee that we have a correct swizzle outside the
   * boundaries of a vector, and the implementation of emit_unpack_half_2x16
   * uses the source operand in an operation with WRITEMASK_Y while our
   * source operand has only size 1, it accessed incorrect data producing
   * regressions in Piglit. We repeat the swizzle of the first component on the
   * rest of components to avoid regressions. In the vec4_visitor IR code path
   * this is not needed because the operand has already the correct swizzle.
   */
   op[0].swizzle = brw_compose_swizzle(BRW_SWIZZLE_XXXX, op[0].swizzle);
   emit_unpack_half_2x16(dst, op[0]);
         case nir_op_pack_half_2x16:
      emit_pack_half_2x16(dst, op[0]);
         case nir_op_unpack_unorm_4x8:
      assert(instr->def.bit_size < 64);
   emit_unpack_unorm_4x8(dst, op[0]);
         case nir_op_pack_unorm_4x8:
      assert(instr->def.bit_size < 64);
   emit_pack_unorm_4x8(dst, op[0]);
         case nir_op_unpack_snorm_4x8:
      assert(instr->def.bit_size < 64);
   emit_unpack_snorm_4x8(dst, op[0]);
         case nir_op_pack_snorm_4x8:
      assert(instr->def.bit_size < 64);
   emit_pack_snorm_4x8(dst, op[0]);
         case nir_op_bitfield_reverse:
      assert(instr->def.bit_size == 32);
   assert(nir_src_bit_size(instr->src[0].src) == 32);
   emit(BFREV(dst, op[0]));
         case nir_op_bit_count:
      assert(instr->def.bit_size == 32);
   assert(nir_src_bit_size(instr->src[0].src) < 64);
   emit(CBIT(dst, op[0]));
         case nir_op_ifind_msb: {
      assert(instr->def.bit_size == 32);
   assert(nir_src_bit_size(instr->src[0].src) == 32);
            vec4_builder bld = vec4_builder(this).at_end();
                     /* FBH counts from the MSB side, while GLSL's findMSB() wants the count
   * from the LSB side. If FBH didn't return an error (0xFFFFFFFF), then
   * subtract the result from 31 to convert the MSB count into an LSB
   * count.
   */
            inst = bld.ADD(dst, src, brw_imm_d(31));
   inst->predicate = BRW_PREDICATE_NORMAL;
   inst->src[0].negate = true;
               case nir_op_uclz:
      assert(instr->def.bit_size == 32);
   assert(nir_src_bit_size(instr->src[0].src) == 32);
   emit(LZD(dst, op[0]));
         case nir_op_find_lsb:
      assert(instr->def.bit_size == 32);
   assert(nir_src_bit_size(instr->src[0].src) == 32);
   assert(devinfo->ver >= 7);
   emit(FBL(dst, op[0]));
         case nir_op_ubitfield_extract:
   case nir_op_ibitfield_extract:
         case nir_op_ubfe:
   case nir_op_ibfe:
      assert(instr->def.bit_size < 64);
   op[0] = fix_3src_operand(op[0]);
   op[1] = fix_3src_operand(op[1]);
            emit(BFE(dst, op[2], op[1], op[0]));
         case nir_op_bfm:
      assert(instr->def.bit_size < 64);
   emit(BFI1(dst, op[0], op[1]));
         case nir_op_bfi:
      assert(instr->def.bit_size < 64);
   op[0] = fix_3src_operand(op[0]);
   op[1] = fix_3src_operand(op[1]);
            emit(BFI2(dst, op[0], op[1], op[2]));
         case nir_op_bitfield_insert:
            case nir_op_fsign:
      if (type_sz(op[0].type) < 8) {
      /* AND(val, 0x80000000) gives the sign bit.
   *
   * Predicated OR ORs 1.0 (0x3f800000) with the sign bit if val is not
   * zero.
                  op[0].type = BRW_REGISTER_TYPE_UD;
                  inst = emit(OR(dst, src_reg(dst), brw_imm_ud(0x3f800000u)));
   inst->predicate = BRW_PREDICATE_NORMAL;
      } else {
      /* For doubles we do the same but we need to consider:
   *
   * - We use a MOV with conditional_mod instead of a CMP so that we can
   *   skip loading a 0.0 immediate. We use a source modifier on the
   *   source of the MOV so that we flush denormalized values to 0.
   *   Since we want to compare against 0, this won't alter the result.
   * - We need to extract the high 32-bit of each DF where the sign
   *   is stored.
                  /* Check for zero */
   src_reg value = op[0];
   value.abs = true;
                  /* AND each high 32-bit channel with 0x80000000u */
   dst_reg tmp = dst_reg(this, glsl_type::uvec4_type);
                  /* Add 1.0 to each channel, predicated to skip the cases where the
   * channel's value was 0
   */
                  /* Now convert the result from float to double */
   emit_conversion_to_double(dst, retype(src_reg(tmp),
      }
         case nir_op_ishl:
      assert(instr->def.bit_size < 64);
   try_immediate_source(instr, op, false);
   emit(SHL(dst, op[0], op[1]));
         case nir_op_ishr:
      assert(instr->def.bit_size < 64);
   try_immediate_source(instr, op, false);
   emit(ASR(dst, op[0], op[1]));
         case nir_op_ushr:
      assert(instr->def.bit_size < 64);
   try_immediate_source(instr, op, false);
   emit(SHR(dst, op[0], op[1]));
         case nir_op_ffma:
      if (type_sz(dst.type) == 8) {
      dst_reg mul_dst = dst_reg(this, glsl_type::dvec4_type);
   emit(MUL(mul_dst, op[1], op[0]));
      } else {
      fix_float_operands(op, instr);
      }
         case nir_op_flrp:
      fix_float_operands(op, instr);
   inst = emit(LRP(dst, op[2], op[1], op[0]));
         case nir_op_b32csel:
      enum brw_predicate predicate;
   if (!optimize_predicate(instr, &predicate)) {
      emit(CMP(dst_null_d(), op[0], brw_imm_d(0), BRW_CONDITIONAL_NZ));
   switch (dst.writemask) {
   case WRITEMASK_X:
      predicate = BRW_PREDICATE_ALIGN16_REPLICATE_X;
      case WRITEMASK_Y:
      predicate = BRW_PREDICATE_ALIGN16_REPLICATE_Y;
      case WRITEMASK_Z:
      predicate = BRW_PREDICATE_ALIGN16_REPLICATE_Z;
      case WRITEMASK_W:
      predicate = BRW_PREDICATE_ALIGN16_REPLICATE_W;
      default:
      predicate = BRW_PREDICATE_NORMAL;
         }
   inst = emit(BRW_OPCODE_SEL, dst, op[1], op[2]);
   inst->predicate = predicate;
         case nir_op_fdot2_replicated:
      try_immediate_source(instr, op, true);
   inst = emit(BRW_OPCODE_DP2, dst, op[0], op[1]);
         case nir_op_fdot3_replicated:
      try_immediate_source(instr, op, true);
   inst = emit(BRW_OPCODE_DP3, dst, op[0], op[1]);
         case nir_op_fdot4_replicated:
      try_immediate_source(instr, op, true);
   inst = emit(BRW_OPCODE_DP4, dst, op[0], op[1]);
         case nir_op_fdph_replicated:
      try_immediate_source(instr, op, false);
   inst = emit(BRW_OPCODE_DPH, dst, op[0], op[1]);
         case nir_op_fdiv:
            case nir_op_fmod:
            case nir_op_fsub:
   case nir_op_isub:
            default:
                  /* If we need to do a boolean resolve, replace the result with -(x & 1)
   * to sign extend the low bit to 0/~0
   */
   if (devinfo->ver <= 5 &&
      (instr->instr.pass_flags & BRW_NIR_BOOLEAN_MASK) ==
   BRW_NIR_BOOLEAN_NEEDS_RESOLVE) {
   dst_reg masked = dst_reg(this, glsl_type::int_type);
   masked.writemask = dst.writemask;
   emit(AND(masked, src_reg(dst), brw_imm_d(1)));
   src_reg masked_neg = src_reg(masked);
   masked_neg.negate = true;
         }
      void
   vec4_visitor::nir_emit_jump(nir_jump_instr *instr)
   {
      switch (instr->type) {
   case nir_jump_break:
      emit(BRW_OPCODE_BREAK);
         case nir_jump_continue:
      emit(BRW_OPCODE_CONTINUE);
         case nir_jump_return:
         default:
            }
      static bool
   is_high_sampler(const struct intel_device_info *devinfo, src_reg sampler)
   {
      if (devinfo->verx10 != 75)
               }
      void
   vec4_visitor::nir_emit_texture(nir_tex_instr *instr)
   {
      unsigned texture = instr->texture_index;
   unsigned sampler = instr->sampler_index;
   src_reg texture_reg = brw_imm_ud(texture);
   src_reg sampler_reg = brw_imm_ud(sampler);
   src_reg coordinate;
   const glsl_type *coord_type = NULL;
   src_reg shadow_comparator;
   src_reg offset_value;
   src_reg lod, lod2;
   src_reg sample_index;
                     /* The hardware requires a LOD for buffer textures */
   if (instr->sampler_dim == GLSL_SAMPLER_DIM_BUF)
            /* Load the texture operation sources */
   uint32_t constant_offset = 0;
   for (unsigned i = 0; i < instr->num_srcs; i++) {
      switch (instr->src[i].src_type) {
   case nir_tex_src_comparator:
      shadow_comparator = get_nir_src(instr->src[i].src,
               case nir_tex_src_coord: {
               switch (instr->op) {
   case nir_texop_txf:
   case nir_texop_txf_ms:
   case nir_texop_samples_identical:
      coordinate = get_nir_src(instr->src[i].src, BRW_REGISTER_TYPE_D,
                     default:
      coordinate = get_nir_src(instr->src[i].src, BRW_REGISTER_TYPE_F,
         coord_type = glsl_type::vec(src_size);
      }
               case nir_tex_src_ddx:
      lod = get_nir_src(instr->src[i].src, BRW_REGISTER_TYPE_F,
               case nir_tex_src_ddy:
      lod2 = get_nir_src(instr->src[i].src, BRW_REGISTER_TYPE_F,
               case nir_tex_src_lod:
      switch (instr->op) {
   case nir_texop_txs:
   case nir_texop_txf:
                  default:
      lod = get_nir_src(instr->src[i].src, BRW_REGISTER_TYPE_F, 1);
                  case nir_tex_src_ms_index: {
      sample_index = get_nir_src(instr->src[i].src, BRW_REGISTER_TYPE_D, 1);
               case nir_tex_src_offset:
      if (!brw_texture_offset(instr, i, &constant_offset)) {
      offset_value =
                  case nir_tex_src_texture_offset: {
      assert(texture_reg.is_zero());
   texture_reg = emit_uniformize(get_nir_src(instr->src[i].src,
                     case nir_tex_src_sampler_offset: {
      assert(sampler_reg.is_zero());
   sampler_reg = emit_uniformize(get_nir_src(instr->src[i].src,
                     case nir_tex_src_projector:
            case nir_tex_src_bias:
            default:
                     if (instr->op == nir_texop_txf_ms ||
      instr->op == nir_texop_samples_identical) {
   assert(coord_type != NULL);
   if (devinfo->ver >= 7) {
         } else {
                     /* Stuff the channel select bits in the top of the texture offset */
   if (instr->op == nir_texop_tg4) {
      if (instr->component == 1 &&
      (key_tex->gather_channel_quirk_mask & (1 << texture))) {
   /* gather4 sampler is broken for green channel on RG32F --
   * we must ask for blue instead.
   */
      } else {
                     enum opcode opcode;
   switch (instr->op) {
   case nir_texop_tex:             opcode = SHADER_OPCODE_TXL;        break;
   case nir_texop_txl:             opcode = SHADER_OPCODE_TXL;        break;
   case nir_texop_txd:             opcode = SHADER_OPCODE_TXD;        break;
   case nir_texop_txf:             opcode = SHADER_OPCODE_TXF;        break;
   case nir_texop_txf_ms:          opcode = SHADER_OPCODE_TXF_CMS;    break;
   case nir_texop_txs:             opcode = SHADER_OPCODE_TXS;        break;
   case nir_texop_query_levels:    opcode = SHADER_OPCODE_TXS;        break;
   case nir_texop_texture_samples: opcode = SHADER_OPCODE_SAMPLEINFO; break;
   case nir_texop_tg4:
      opcode = offset_value.file != BAD_FILE ? SHADER_OPCODE_TG4_OFFSET
            case nir_texop_samples_identical: {
      /* There are some challenges implementing this for vec4, and it seems
   * unlikely to be used anyway.  For now, just return false ways.
   */
   emit(MOV(dest, brw_imm_ud(0u)));
      }
   case nir_texop_txb:
   case nir_texop_lod:
         default:
                                    /* The message header is necessary for:
   * - Gfx4 (always)
   * - Texel offsets
   * - Gather channel selection
   * - Sampler indices too large to fit in a 4-bit value.
   * - Sampleinfo message - takes no parameters, but mlen = 0 is illegal
   */
   inst->header_size =
      (devinfo->ver < 5 ||
   inst->offset != 0 ||
   opcode == SHADER_OPCODE_TG4 ||
   opcode == SHADER_OPCODE_TG4_OFFSET ||
   opcode == SHADER_OPCODE_SAMPLEINFO ||
      inst->base_mrf = 2;
   inst->mlen = inst->header_size;
   inst->dst.writemask = WRITEMASK_XYZW;
            inst->src[1] = texture_reg;
            /* MRF for the first parameter */
            if (opcode == SHADER_OPCODE_TXS) {
      int writemask = devinfo->ver == 4 ? WRITEMASK_W : WRITEMASK_X;
   emit(MOV(dst_reg(MRF, param_base, lod.type, writemask), lod));
      } else if (opcode == SHADER_OPCODE_SAMPLEINFO) {
         } else {
      /* Load the coordinate */
   /* FINISHME: gl_clamp_mask and saturate */
   int coord_mask = (1 << instr->coord_components) - 1;
            emit(MOV(dst_reg(MRF, param_base, coordinate.type, coord_mask),
                  if (zero_mask != 0) {
      emit(MOV(dst_reg(MRF, param_base, coordinate.type, zero_mask),
      }
   /* Load the shadow comparator */
   if (shadow_comparator.file != BAD_FILE &&
         emit(MOV(dst_reg(MRF, param_base + 1, shadow_comparator.type,
      WRITEMASK_X),
      inst->mlen++;
                  /* Load the LOD info */
   switch (opcode) {
   int mrf, writemask;
   if (devinfo->ver >= 5) {
      mrf = param_base + 1;
   if (shadow_comparator.file != BAD_FILE) {
      writemask = WRITEMASK_Y;
      } else {
      writemask = WRITEMASK_X;
         } else /* devinfo->ver == 4 */ {
      mrf = param_base;
      }
   emit(MOV(dst_reg(MRF, mrf, lod.type, writemask), lod));
                        case SHADER_OPCODE_TXF:
                  case SHADER_OPCODE_TXF_CMS:
      emit(MOV(dst_reg(MRF, param_base + 1, sample_index.type, WRITEMASK_X),
         if (devinfo->ver >= 7) {
      /* MCS data is in the first channel of `mcs`, but we need to get it into
   * the .y channel of the second vec4 of params, so replicate .x across
   * the whole vec4 and then mask off everything except .y
   */
   mcs.swizzle = BRW_SWIZZLE_XXXX;
   emit(MOV(dst_reg(MRF, param_base + 1, glsl_type::uint_type, WRITEMASK_Y),
      }
               case SHADER_OPCODE_TXD: {
      if (devinfo->ver >= 5) {
      lod.swizzle = BRW_SWIZZLE4(BRW_SWIZZLE_X,BRW_SWIZZLE_X,BRW_SWIZZLE_Y,BRW_SWIZZLE_Y);
   lod2.swizzle = BRW_SWIZZLE4(BRW_SWIZZLE_X,BRW_SWIZZLE_X,BRW_SWIZZLE_Y,BRW_SWIZZLE_Y);
   emit(MOV(dst_reg(MRF, param_base + 1, type, WRITEMASK_XZ), lod));
   emit(MOV(dst_reg(MRF, param_base + 1, type, WRITEMASK_YW), lod2));
            if (nir_tex_instr_dest_size(instr) == 3 ||
            lod.swizzle = BRW_SWIZZLE_ZZZZ;
   lod2.swizzle = BRW_SWIZZLE_ZZZZ;
   emit(MOV(dst_reg(MRF, param_base + 2, type, WRITEMASK_X), lod));
   emit(MOV(dst_reg(MRF, param_base + 2, type, WRITEMASK_Y), lod2));
                     if (shadow_comparator.file != BAD_FILE) {
      emit(MOV(dst_reg(MRF, param_base + 2,
         } else /* devinfo->ver == 4 */ {
      emit(MOV(dst_reg(MRF, param_base + 1, type, WRITEMASK_XYZ), lod));
   emit(MOV(dst_reg(MRF, param_base + 2, type, WRITEMASK_XYZ), lod2));
      }
                        case SHADER_OPCODE_TG4_OFFSET:
      if (shadow_comparator.file != BAD_FILE) {
      emit(MOV(dst_reg(MRF, param_base, shadow_comparator.type, WRITEMASK_W),
               emit(MOV(dst_reg(MRF, param_base + 1, glsl_type::ivec2_type, WRITEMASK_XY),
                     default:
                              /* fixup num layers (z) for cube arrays: hardware returns faces * layers;
   * spec requires layers.
   */
   if (instr->op == nir_texop_txs && devinfo->ver < 7) {
      /* Gfx4-6 return 0 instead of 1 for single layer surfaces. */
   emit_minmax(BRW_CONDITIONAL_GE, writemask(inst->dst, WRITEMASK_Z),
               if (instr->op == nir_texop_query_levels) {
      /* # levels is in .w */
   src_reg swizzled(dest);
   swizzled.swizzle = BRW_SWIZZLE4(BRW_SWIZZLE_W, BRW_SWIZZLE_W,
               }
      src_reg
   vec4_visitor::emit_mcs_fetch(const glsl_type *coordinate_type,
         {
      vec4_instruction *inst =
      new(mem_ctx) vec4_instruction(SHADER_OPCODE_TXF_MCS,
      inst->base_mrf = 2;
   inst->src[1] = surface;
   inst->src[2] = brw_imm_ud(0); /* sampler */
                     /* parameters are: u, v, r, lod; lod will always be zero due to api restrictions */
   int coord_mask = (1 << coordinate_type->vector_elements) - 1;
            emit(MOV(dst_reg(MRF, param_base, coordinate_type, coord_mask),
            emit(MOV(dst_reg(MRF, param_base, coordinate_type, zero_mask),
            emit(inst);
      }
      void
   vec4_visitor::nir_emit_undef(nir_undef_instr *instr)
   {
      nir_ssa_values[instr->def.index] =
      }
      /* SIMD4x2 64bit data is stored in register space like this:
   *
   * r0.0:DF  x0 y0 z0 w0
   * r1.0:DF  x1 y1 z1 w1
   *
   * When we need to write data such as this to memory using 32-bit write
   * messages we need to shuffle it in this fashion:
   *
   * r0.0:DF  x0 y0 x1 y1 (to be written at base offset)
   * r0.0:DF  z0 w0 z1 w1 (to be written at base offset + 16)
   *
   * We need to do the inverse operation when we read using 32-bit messages,
   * which we can do by applying the same exact shuffling on the 64-bit data
   * read, only that because the data for each vertex is positioned differently
   * we need to apply different channel enables.
   *
   * This function takes 64bit data and shuffles it as explained above.
   *
   * The @for_write parameter is used to specify if the shuffling is being done
   * for proper SIMD4x2 64-bit data that needs to be shuffled prior to a 32-bit
   * write message (for_write = true), or instead we are doing the inverse
   * operation and we have just read 64-bit data using a 32-bit messages that we
   * need to shuffle to create valid SIMD4x2 64-bit data (for_write = false).
   *
   * If @block and @ref are non-NULL, then the shuffling is done after @ref,
   * otherwise the instructions are emitted normally at the end. The function
   * returns the last instruction inserted.
   *
   * Notice that @src and @dst cannot be the same register.
   */
   vec4_instruction *
   vec4_visitor::shuffle_64bit_data(dst_reg dst, src_reg src, bool for_write,
               {
      assert(type_sz(src.type) == 8);
   assert(type_sz(dst.type) == 8);
   assert(!regions_overlap(dst, 2 * REG_SIZE, src, 2 * REG_SIZE));
                     const vec4_builder bld = !ref ? vec4_builder(this).at_end() :
            /* Resolve swizzle in src */
   if (src.swizzle != BRW_SWIZZLE_XYZW) {
      dst_reg data = dst_reg(this, glsl_type::dvec4_type);
   bld.emit(mov_op, data, src);
               /* dst+0.XY = src+0.XY */
            /* dst+0.ZW = src+1.XY */
   bld.group(4, for_write ? 1 : 0)
                  /* dst+1.XY = src+0.ZW */
   bld.group(4, for_write ? 0 : 1)
                  /* dst+1.ZW = src+1.ZW */
   return bld.group(4, 1)
            }
      }
