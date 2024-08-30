   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
         #include "pipe/p_shader_tokens.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_parse.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "util/u_pstipple.h"
      #include "svga_tgsi_emit.h"
   #include "svga_context.h"
         static bool emit_vs_postamble( struct svga_shader_emitter *emit );
   static bool emit_ps_postamble( struct svga_shader_emitter *emit );
         static SVGA3dShaderOpCodeType
   translate_opcode(enum tgsi_opcode opcode)
   {
      switch (opcode) {
   case TGSI_OPCODE_ADD:        return SVGA3DOP_ADD;
   case TGSI_OPCODE_DP3:        return SVGA3DOP_DP3;
   case TGSI_OPCODE_DP4:        return SVGA3DOP_DP4;
   case TGSI_OPCODE_FRC:        return SVGA3DOP_FRC;
   case TGSI_OPCODE_MAD:        return SVGA3DOP_MAD;
   case TGSI_OPCODE_MAX:        return SVGA3DOP_MAX;
   case TGSI_OPCODE_MIN:        return SVGA3DOP_MIN;
   case TGSI_OPCODE_MOV:        return SVGA3DOP_MOV;
   case TGSI_OPCODE_MUL:        return SVGA3DOP_MUL;
   case TGSI_OPCODE_NOP:        return SVGA3DOP_NOP;
   default:
      assert(!"svga: unexpected opcode in translate_opcode()");
         }
         static SVGA3dShaderRegType
   translate_file(enum tgsi_file_type file)
   {
      switch (file) {
   case TGSI_FILE_TEMPORARY: return SVGA3DREG_TEMP;
   case TGSI_FILE_INPUT:     return SVGA3DREG_INPUT;
   case TGSI_FILE_OUTPUT:    return SVGA3DREG_OUTPUT; /* VS3.0+ only */
   case TGSI_FILE_IMMEDIATE: return SVGA3DREG_CONST;
   case TGSI_FILE_CONSTANT:  return SVGA3DREG_CONST;
   case TGSI_FILE_SAMPLER:   return SVGA3DREG_SAMPLER;
   case TGSI_FILE_ADDRESS:   return SVGA3DREG_ADDR;
   default:
      assert(!"svga: unexpected register file in translate_file()");
         }
         /**
   * Translate a TGSI destination register to an SVGA3DShaderDestToken.
   * \param insn  the TGSI instruction
   * \param idx  which TGSI dest register to translate (usually (always?) zero)
   */
   static SVGA3dShaderDestToken
   translate_dst_register( struct svga_shader_emitter *emit,
               {
      const struct tgsi_full_dst_register *reg = &insn->Dst[idx];
            switch (reg->Register.File) {
   case TGSI_FILE_OUTPUT:
      /* Output registers encode semantic information in their name.
   * Need to lookup a table built at decl time:
   */
   dest = emit->output_map[reg->Register.Index];
   emit->num_output_writes++;
         default:
      {
      unsigned index = reg->Register.Index;
   assert(index < SVGA3D_TEMPREG_MAX);
   index = MIN2(index, SVGA3D_TEMPREG_MAX - 1);
      }
               if (reg->Register.Indirect) {
                  dest.mask = reg->Register.WriteMask;
            if (insn->Instruction.Saturate)
               }
         /**
   * Apply a swizzle to a src_register, returning a new src_register
   * Ex: swizzle(SRC.ZZYY, SWIZZLE_Z, SWIZZLE_W, SWIZZLE_X, SWIZZLE_Y)
   * would return SRC.YYZZ
   */
   static struct src_register
   swizzle(struct src_register src,
         {
      assert(x < 4);
   assert(y < 4);
   assert(z < 4);
   assert(w < 4);
   x = (src.base.swizzle >> (x * 2)) & 0x3;
   y = (src.base.swizzle >> (y * 2)) & 0x3;
   z = (src.base.swizzle >> (z * 2)) & 0x3;
                        }
         /**
   * Apply a "scalar" swizzle to a src_register returning a new
   * src_register where all the swizzle terms are the same.
   * Ex: scalar(SRC.WZYX, SWIZZLE_Y) would return SRC.ZZZZ
   */
   static struct src_register
   scalar(struct src_register src, unsigned comp)
   {
      assert(comp < 4);
      }
         static bool
   svga_arl_needs_adjustment( const struct svga_shader_emitter *emit )
   {
               for (i = 0; i < emit->num_arl_consts; ++i) {
      if (emit->arl_consts[i].arl_num == emit->current_arl)
      }
      }
         static int
   svga_arl_adjustment( const struct svga_shader_emitter *emit )
   {
               for (i = 0; i < emit->num_arl_consts; ++i) {
      if (emit->arl_consts[i].arl_num == emit->current_arl)
      }
      }
         /**
   * Translate a TGSI src register to a src_register.
   */
   static struct src_register
   translate_src_register( const struct svga_shader_emitter *emit,
         {
               switch (reg->Register.File) {
   case TGSI_FILE_INPUT:
      /* Input registers are referred to by their semantic name rather
   * than by index.  Use the mapping build up from the decls:
   */
   src = emit->input_map[reg->Register.Index];
         case TGSI_FILE_IMMEDIATE:
      /* Immediates are appended after TGSI constants in the D3D
   * constant buffer.
   */
   src = src_register( translate_file( reg->Register.File ),
               default:
      src = src_register( translate_file( reg->Register.File ),
                     /* Indirect addressing.
   */
   if (reg->Register.Indirect) {
      if (emit->unit == PIPE_SHADER_FRAGMENT) {
      /* Pixel shaders have only loop registers for relative
   * addressing into inputs. Ignore the redundant address
   * register, the contents of aL should be in sync with it.
   */
   if (reg->Register.File == TGSI_FILE_INPUT) {
      src.base.relAddr = 1;
         }
   else {
      /* Constant buffers only.
   */
   if (reg->Register.File == TGSI_FILE_CONSTANT) {
      /* we shift the offset towards the minimum */
   if (svga_arl_needs_adjustment( emit )) {
                        /* Not really sure what should go in the second token:
   */
                                    src = swizzle( src,
                  reg->Register.SwizzleX,
         /* src.mod isn't a bitfield, unfortunately */
   if (reg->Register.Absolute) {
      if (reg->Register.Negate)
         else
      }
   else {
      if (reg->Register.Negate)
         else
                  }
         /*
   * Get a temporary register.
   * Note: if we exceed the temporary register limit we just use
   * register SVGA3D_TEMPREG_MAX - 1.
   */
   static SVGA3dShaderDestToken
   get_temp( struct svga_shader_emitter *emit )
   {
      int i = emit->nr_hw_temp + emit->internal_temp_count++;
   if (i >= SVGA3D_TEMPREG_MAX) {
      debug_warn_once("svga: Too many temporary registers used in shader\n");
      }
      }
         /**
   * Release a single temp.  Currently only effective if it was the last
   * allocated temp, otherwise release will be delayed until the next
   * call to reset_temp_regs().
   */
   static void
   release_temp( struct svga_shader_emitter *emit,
         {
      if (temp.num == emit->internal_temp_count - 1)
      }
         /**
   * Release all temps.
   */
   static void
   reset_temp_regs(struct svga_shader_emitter *emit)
   {
         }
         /** Emit bytecode for a src_register */
   static bool
   emit_src(struct svga_shader_emitter *emit, const struct src_register src)
   {
      if (src.base.relAddr) {
      assert(src.base.reserved0);
   assert(src.indirect.reserved0);
   return (svga_shader_emit_dword( emit, src.base.value ) &&
      }
   else {
      assert(src.base.reserved0);
         }
         /** Emit bytecode for a dst_register */
   static bool
   emit_dst(struct svga_shader_emitter *emit, SVGA3dShaderDestToken dest)
   {
      assert(dest.reserved0);
   assert(dest.mask);
      }
         /** Emit bytecode for a 1-operand instruction */
   static bool
   emit_op1(struct svga_shader_emitter *emit,
            SVGA3dShaderInstToken inst,
      {
      return (emit_instruction(emit, inst) &&
            }
         /** Emit bytecode for a 2-operand instruction */
   static bool
   emit_op2(struct svga_shader_emitter *emit,
            SVGA3dShaderInstToken inst,
   SVGA3dShaderDestToken dest,
      {
      return (emit_instruction(emit, inst) &&
         emit_dst(emit, dest) &&
      }
         /** Emit bytecode for a 3-operand instruction */
   static bool
   emit_op3(struct svga_shader_emitter *emit,
            SVGA3dShaderInstToken inst,
   SVGA3dShaderDestToken dest,
   struct src_register src0,
      {
      return (emit_instruction(emit, inst) &&
         emit_dst(emit, dest) &&
   emit_src(emit, src0) &&
      }
         /** Emit bytecode for a 4-operand instruction */
   static bool
   emit_op4(struct svga_shader_emitter *emit,
            SVGA3dShaderInstToken inst,
   SVGA3dShaderDestToken dest,
   struct src_register src0,
   struct src_register src1,
      {
      return (emit_instruction(emit, inst) &&
         emit_dst(emit, dest) &&
   emit_src(emit, src0) &&
   emit_src(emit, src1) &&
      }
         /**
   * Apply the absolute value modifier to the given src_register, returning
   * a new src_register.
   */
   static struct src_register 
   absolute(struct src_register src)
   {
      src.base.srcMod = SVGA3DSRCMOD_ABS;
      }
         /**
   * Apply the negation modifier to the given src_register, returning
   * a new src_register.
   */
   static struct src_register 
   negate(struct src_register src)
   {
      switch (src.base.srcMod) {
   case SVGA3DSRCMOD_ABS:
      src.base.srcMod = SVGA3DSRCMOD_ABSNEG;
      case SVGA3DSRCMOD_ABSNEG:
      src.base.srcMod = SVGA3DSRCMOD_ABS;
      case SVGA3DSRCMOD_NEG:
      src.base.srcMod = SVGA3DSRCMOD_NONE;
      case SVGA3DSRCMOD_NONE:
      src.base.srcMod = SVGA3DSRCMOD_NEG;
      }
      }
            /* Replace the src with the temporary specified in the dst, but copying
   * only the necessary channels, and preserving the original swizzle (which is
   * important given that several opcodes have constraints in the allowed
   * swizzles).
   */
   static bool
   emit_repl(struct svga_shader_emitter *emit,
               {
      unsigned src0_swizzle;
                              dst.mask = 0;
   for (chan = 0; chan < 4; ++chan) {
      unsigned swizzle = (src0_swizzle >> (chan *2)) & 0x3;
      }
                     if (!emit_op1( emit, inst_token( SVGA3DOP_MOV ), dst, *src0 ))
            *src0 = src( dst );
               }
         /**
   * Submit/emit an instruction with zero operands.
   */
   static bool
   submit_op0(struct svga_shader_emitter *emit,
               {
      return (emit_instruction( emit, inst ) &&
      }
         /**
   * Submit/emit an instruction with one operand.
   */
   static bool
   submit_op1(struct svga_shader_emitter *emit,
            SVGA3dShaderInstToken inst,
      {
         }
         /**
   * Submit/emit an instruction with two operands.
   *
   * SVGA shaders may not refer to >1 constant register in a single
   * instruction.  This function checks for that usage and inserts a
   * move to temporary if detected.
   *
   * The same applies to input registers -- at most a single input
   * register may be read by any instruction.
   */
   static bool
   submit_op2(struct svga_shader_emitter *emit,
            SVGA3dShaderInstToken inst,
   SVGA3dShaderDestToken dest,
      {
      SVGA3dShaderDestToken temp;
   SVGA3dShaderRegType type0, type1;
            temp.value = 0;
   type0 = SVGA3dShaderGetRegType( src0.base.value );
            if (type0 == SVGA3DREG_CONST &&
      type1 == SVGA3DREG_CONST &&
   src0.base.num != src1.base.num)
         if (type0 == SVGA3DREG_INPUT &&
      type1 == SVGA3DREG_INPUT &&
   src0.base.num != src1.base.num)
         if (need_temp) {
               if (!emit_repl( emit, temp, &src0 ))
               if (!emit_op2( emit, inst, dest, src0, src1 ))
            if (need_temp)
               }
         /**
   * Submit/emit an instruction with three operands.
   *
   * SVGA shaders may not refer to >1 constant register in a single
   * instruction.  This function checks for that usage and inserts a
   * move to temporary if detected.
   */
   static bool
   submit_op3(struct svga_shader_emitter *emit,
            SVGA3dShaderInstToken inst,
   SVGA3dShaderDestToken dest,
   struct src_register src0,
      {
      SVGA3dShaderDestToken temp0;
   SVGA3dShaderDestToken temp1;
   bool need_temp0 = false;
   bool need_temp1 = false;
            temp0.value = 0;
   temp1.value = 0;
   type0 = SVGA3dShaderGetRegType( src0.base.value );
   type1 = SVGA3dShaderGetRegType( src1.base.value );
            if (inst.op != SVGA3DOP_SINCOS) {
      if (type0 == SVGA3DREG_CONST &&
      ((type1 == SVGA3DREG_CONST && src0.base.num != src1.base.num) ||
               if (type1 == SVGA3DREG_CONST &&
      (type2 == SVGA3DREG_CONST && src1.base.num != src2.base.num))
            if (type0 == SVGA3DREG_INPUT &&
      ((type1 == SVGA3DREG_INPUT && src0.base.num != src1.base.num) ||
   (type2 == SVGA3DREG_INPUT && src0.base.num != src2.base.num)))
         if (type1 == SVGA3DREG_INPUT &&
      (type2 == SVGA3DREG_INPUT && src1.base.num != src2.base.num))
         if (need_temp0) {
               if (!emit_repl( emit, temp0, &src0 ))
               if (need_temp1) {
               if (!emit_repl( emit, temp1, &src1 ))
               if (!emit_op3( emit, inst, dest, src0, src1, src2 ))
            if (need_temp1)
         if (need_temp0)
            }
         /**
   * Submit/emit an instruction with four operands.
   *
   * SVGA shaders may not refer to >1 constant register in a single
   * instruction.  This function checks for that usage and inserts a
   * move to temporary if detected.
   */
   static bool
   submit_op4(struct svga_shader_emitter *emit,
            SVGA3dShaderInstToken inst,
   SVGA3dShaderDestToken dest,
   struct src_register src0,
   struct src_register src1,
      {
      SVGA3dShaderDestToken temp0;
   SVGA3dShaderDestToken temp3;
   bool need_temp0 = false;
   bool need_temp3 = false;
            temp0.value = 0;
   temp3.value = 0;
   type0 = SVGA3dShaderGetRegType( src0.base.value );
   type1 = SVGA3dShaderGetRegType( src1.base.value );
   type2 = SVGA3dShaderGetRegType( src2.base.value );
            /* Make life a little easier - this is only used by the TXD
   * instruction which is guaranteed not to have a constant/input reg
   * in one slot at least:
   */
   assert(type1 == SVGA3DREG_SAMPLER);
            if (type0 == SVGA3DREG_CONST &&
      ((type3 == SVGA3DREG_CONST && src0.base.num != src3.base.num) ||
   (type2 == SVGA3DREG_CONST && src0.base.num != src2.base.num)))
         if (type3 == SVGA3DREG_CONST &&
      (type2 == SVGA3DREG_CONST && src3.base.num != src2.base.num))
         if (type0 == SVGA3DREG_INPUT &&
      ((type3 == SVGA3DREG_INPUT && src0.base.num != src3.base.num) ||
   (type2 == SVGA3DREG_INPUT && src0.base.num != src2.base.num)))
         if (type3 == SVGA3DREG_INPUT &&
      (type2 == SVGA3DREG_INPUT && src3.base.num != src2.base.num))
         if (need_temp0) {
               if (!emit_repl( emit, temp0, &src0 ))
               if (need_temp3) {
               if (!emit_repl( emit, temp3, &src3 ))
               if (!emit_op4( emit, inst, dest, src0, src1, src2, src3 ))
            if (need_temp3)
         if (need_temp0)
            }
         /**
   * Do the src and dest registers refer to the same register?
   */
   static bool
   alias_src_dst(struct src_register src,
         {
      if (src.base.num != dst.num)
            if (SVGA3dShaderGetRegType(dst.value) !=
      SVGA3dShaderGetRegType(src.base.value))
            }
         /**
   * Helper for emitting SVGA immediate values using the SVGA3DOP_DEF[I]
   * instructions.
   */
   static bool
   emit_def_const(struct svga_shader_emitter *emit,
               {
      SVGA3DOpDefArgs def;
            switch (type) {
   case SVGA3D_CONST_TYPE_FLOAT:
      opcode = inst_token( SVGA3DOP_DEF );
   def.dst = dst_register( SVGA3DREG_CONST, idx );
   def.constValues[0] = a;
   def.constValues[1] = b;
   def.constValues[2] = c;
   def.constValues[3] = d;
      case SVGA3D_CONST_TYPE_INT:
      opcode = inst_token( SVGA3DOP_DEFI );
   def.dst = dst_register( SVGA3DREG_CONSTINT, idx );
   def.constIValues[0] = (int)a;
   def.constIValues[1] = (int)b;
   def.constIValues[2] = (int)c;
   def.constIValues[3] = (int)d;
      default:
      assert(0);
   opcode = inst_token( SVGA3DOP_NOP );
               if (!emit_instruction(emit, opcode) ||
      !svga_shader_emit_dwords( emit, def.values, ARRAY_SIZE(def.values)))
            }
         static bool
   create_loop_const( struct svga_shader_emitter *emit )
   {
               if (!emit_def_const( emit, SVGA3D_CONST_TYPE_INT, idx,
                        255, /* iteration count */
         emit->loop_const_idx = idx;
               }
      static bool
   create_arl_consts( struct svga_shader_emitter *emit )
   {
               for (i = 0; i < emit->num_arl_consts; i += 4) {
      int j;
   unsigned idx = emit->nr_hw_float_const++;
   float vals[4];
   for (j = 0; j < 4 && (j + i) < emit->num_arl_consts; ++j) {
      vals[j] = (float) emit->arl_consts[i + j].number;
   emit->arl_consts[i + j].idx = idx;
   switch (j) {
   case 0:
      emit->arl_consts[i + 0].swizzle = TGSI_SWIZZLE_X;
      case 1:
      emit->arl_consts[i + 0].swizzle = TGSI_SWIZZLE_Y;
      case 2:
      emit->arl_consts[i + 0].swizzle = TGSI_SWIZZLE_Z;
      case 3:
      emit->arl_consts[i + 0].swizzle = TGSI_SWIZZLE_W;
         }
   while (j < 4)
            if (!emit_def_const( emit, SVGA3D_CONST_TYPE_FLOAT, idx,
                              }
         /**
   * Return the register which holds the pixel shaders front/back-
   * facing value.
   */
   static struct src_register
   get_vface( struct svga_shader_emitter *emit )
   {
      assert(emit->emitted_vface);
      }
         /**
   * Create/emit a "common" constant with values {0, 0.5, -1, 1}.
   * We can swizzle this to produce other useful constants such as
   * {0, 0, 0, 0}, {1, 1, 1, 1}, etc.
   */
   static bool
   create_common_immediate( struct svga_shader_emitter *emit )
   {
               /* Emit the constant (0, 0.5, -1, 1) and use swizzling to generate
   * other useful vectors.
   */
   if (!emit_def_const( emit, SVGA3D_CONST_TYPE_FLOAT,
               emit->common_immediate_idx[0] = idx;
            /* Emit constant {2, 0, 0, 0} (only the 2 is used for now) */
   if (emit->key.vs.adjust_attrib_range) {
      if (!emit_def_const( emit, SVGA3D_CONST_TYPE_FLOAT,
                  }
   else {
                              }
         /**
   * Return swizzle/position for the given value in the "common" immediate.
   */
   static inline unsigned
   common_immediate_swizzle(float value)
   {
      if (value == 0.0f)
         else if (value == 0.5f)
         else if (value == -1.0f)
         else if (value == 1.0f)
         else {
      assert(!"illegal value in common_immediate_swizzle");
         }
         /**
   * Returns an immediate reg where all the terms are either 0, 1, 2 or 0.5
   */
   static struct src_register
   get_immediate(struct svga_shader_emitter *emit,
         {
      unsigned sx = common_immediate_swizzle(x);
   unsigned sy = common_immediate_swizzle(y);
   unsigned sz = common_immediate_swizzle(z);
   unsigned sw = common_immediate_swizzle(w);
   assert(emit->created_common_immediate);
   assert(emit->common_immediate_idx[0] >= 0);
   return swizzle(src_register(SVGA3DREG_CONST, emit->common_immediate_idx[0]),
      }
         /**
   * returns {0, 0, 0, 0} immediate
   */
   static struct src_register
   get_zero_immediate( struct svga_shader_emitter *emit )
   {
      assert(emit->created_common_immediate);
   assert(emit->common_immediate_idx[0] >= 0);
   return swizzle(src_register( SVGA3DREG_CONST,
            }
         /**
   * returns {1, 1, 1, 1} immediate
   */
   static struct src_register
   get_one_immediate( struct svga_shader_emitter *emit )
   {
      assert(emit->created_common_immediate);
   assert(emit->common_immediate_idx[0] >= 0);
   return swizzle(src_register( SVGA3DREG_CONST,
            }
         /**
   * returns {0.5, 0.5, 0.5, 0.5} immediate
   */
   static struct src_register
   get_half_immediate( struct svga_shader_emitter *emit )
   {
      assert(emit->created_common_immediate);
   assert(emit->common_immediate_idx[0] >= 0);
   return swizzle(src_register(SVGA3DREG_CONST, emit->common_immediate_idx[0]),
      }
         /**
   * returns {2, 2, 2, 2} immediate
   */
   static struct src_register
   get_two_immediate( struct svga_shader_emitter *emit )
   {
      /* Note we use the second common immediate here */
   assert(emit->created_common_immediate);
   assert(emit->common_immediate_idx[1] >= 0);
   return swizzle(src_register( SVGA3DREG_CONST,
            }
         /**
   * returns the loop const
   */
   static struct src_register
   get_loop_const( struct svga_shader_emitter *emit )
   {
      assert(emit->created_loop_const);
   assert(emit->loop_const_idx >= 0);
   return src_register( SVGA3DREG_CONSTINT,
      }
         static struct src_register
   get_fake_arl_const( struct svga_shader_emitter *emit )
   {
      struct src_register reg;
            for (i = 0; i < emit->num_arl_consts; ++ i) {
      if (emit->arl_consts[i].arl_num == emit->current_arl) {
      idx = emit->arl_consts[i].idx;
                  reg = src_register( SVGA3DREG_CONST, idx );
      }
         /**
   * Return a register which holds the width and height of the texture
   * currently bound to the given sampler.
   */
   static struct src_register
   get_tex_dimensions( struct svga_shader_emitter *emit, int sampler_num )
   {
      int idx;
            /* the width/height indexes start right after constants */
   idx = emit->key.tex[sampler_num].width_height_idx +
            reg = src_register( SVGA3DREG_CONST, idx );
      }
         static bool
   emit_fake_arl(struct svga_shader_emitter *emit,
         {
      const struct src_register src0 =
         struct src_register src1 = get_fake_arl_const( emit );
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
            if (!submit_op1(emit, inst_token( SVGA3DOP_MOV ), tmp, src0))
            if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ), tmp, src( tmp ),
                  /* replicate the original swizzle */
   src1 = src(tmp);
            return submit_op1( emit, inst_token( SVGA3DOP_MOVA ),
      }
         static bool
   emit_if(struct svga_shader_emitter *emit,
         {
      struct src_register src0 =
         struct src_register zero = get_zero_immediate(emit);
                     if (SVGA3dShaderGetRegType(src0.base.value) == SVGA3DREG_CONST) {
      /*
   * Max different constant registers readable per IFC instruction is 1.
   */
            if (!submit_op1(emit, inst_token( SVGA3DOP_MOV ), tmp, src0))
                                 return (emit_instruction( emit, if_token ) &&
            }
         static bool
   emit_else(struct svga_shader_emitter *emit,
         {
         }
         static bool
   emit_endif(struct svga_shader_emitter *emit,
         {
                  }
         /**
   * Translate the following TGSI FLR instruction.
   *    FLR  DST, SRC
   * To the following SVGA3D instruction sequence.
   *    FRC  TMP, SRC
   *    SUB  DST, SRC, TMP
   */
   static bool
   emit_floor(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 =
                  /* FRC  TMP, SRC */
   if (!submit_op1( emit, inst_token( SVGA3DOP_FRC ), temp, src0 ))
            /* SUB  DST, SRC, TMP */
   if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ), dst, src0,
                     }
         /**
   * Translate the following TGSI CEIL instruction.
   *    CEIL  DST, SRC
   * To the following SVGA3D instruction sequence.
   *    FRC  TMP, -SRC
   *    ADD  DST, SRC, TMP
   */
   static bool
   emit_ceil(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register(emit, insn, 0);
   const struct src_register src0 =
                  /* FRC  TMP, -SRC */
   if (!submit_op1(emit, inst_token(SVGA3DOP_FRC), temp, negate(src0)))
            /* ADD DST, SRC, TMP */
   if (!submit_op2(emit, inst_token(SVGA3DOP_ADD), dst, src0, src(temp)))
               }
         /**
   * Translate the following TGSI DIV instruction.
   *    DIV  DST.xy, SRC0, SRC1
   * To the following SVGA3D instruction sequence.
   *    RCP  TMP.x, SRC1.xxxx
   *    RCP  TMP.y, SRC1.yyyy
   *    MUL  DST.xy, SRC0, TMP
   */
   static bool
   emit_div(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 =
         const struct src_register src1 =
         SVGA3dShaderDestToken temp = get_temp( emit );
            /* For each enabled element, perform a RCP instruction.  Note that
   * RCP is scalar in SVGA3D:
   */
   for (i = 0; i < 4; i++) {
      unsigned channel = 1 << i;
   if (dst.mask & channel) {
      /* RCP  TMP.?, SRC1.???? */
   if (!submit_op1( emit, inst_token( SVGA3DOP_RCP ),
                              /* Vector mul:
   * MUL  DST, SRC0, TMP
   */
   if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ), dst, src0,
                     }
         /**
   * Translate the following TGSI DP2 instruction.
   *    DP2  DST, SRC1, SRC2
   * To the following SVGA3D instruction sequence.
   *    MUL  TMP, SRC1, SRC2
   *    ADD  DST, TMP.xxxx, TMP.yyyy
   */
   static bool
   emit_dp2(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 =
         const struct src_register src1 =
         SVGA3dShaderDestToken temp = get_temp( emit );
            /* MUL  TMP, SRC1, SRC2 */
   if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ), temp, src0, src1 ))
            temp_src0 = scalar(src( temp ), TGSI_SWIZZLE_X);
            /* ADD  DST, TMP.xxxx, TMP.yyyy */
   if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ), dst,
                     }
         /**
   * Sine / Cosine helper function.
   */
   static bool
   do_emit_sincos(struct svga_shader_emitter *emit,
               {
      src0 = scalar(src0, TGSI_SWIZZLE_X);
      }
         /**
   * Translate TGSI SIN instruction into:
   * SCS TMP SRC
   * MOV DST TMP.yyyy
   */
   static bool
   emit_sin(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 =
                  /* SCS TMP SRC */
   if (!do_emit_sincos(emit, writemask(temp, TGSI_WRITEMASK_Y), src0))
                     /* MOV DST TMP.yyyy */
   if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), dst, src0 ))
               }
         /*
   * Translate TGSI COS instruction into:
   * SCS TMP SRC
   * MOV DST TMP.xxxx
   */
   static bool
   emit_cos(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 =
                  /* SCS TMP SRC */
   if (!do_emit_sincos( emit, writemask(temp, TGSI_WRITEMASK_X), src0 ))
                     /* MOV DST TMP.xxxx */
   if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), dst, src0 ))
               }
         /**
   * Translate/emit TGSI SSG (Set Sign: -1, 0, +1) instruction.
   */
   static bool
   emit_ssg(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 =
         SVGA3dShaderDestToken temp0 = get_temp( emit );
   SVGA3dShaderDestToken temp1 = get_temp( emit );
            if (emit->unit == PIPE_SHADER_VERTEX) {
      /* SGN  DST, SRC0, TMP0, TMP1 */
   return submit_op3( emit, inst_token( SVGA3DOP_SGN ), dst, src0,
               one = get_one_immediate(emit);
            /* CMP  TMP0, SRC0, one, zero */
   if (!submit_op3( emit, inst_token( SVGA3DOP_CMP ),
                  /* CMP  TMP1, negate(SRC0), negate(one), zero */
   if (!submit_op3( emit, inst_token( SVGA3DOP_CMP ),
                        /* ADD  DST, TMP0, TMP1 */
   return submit_op2( emit, inst_token( SVGA3DOP_ADD ), dst, src( temp0 ),
      }
         /**
   * Translate/emit the conditional discard instruction (discard if
   * any of X,Y,Z,W are negative).
   */
   static bool
   emit_cond_discard(struct svga_shader_emitter *emit,
         {
      const struct tgsi_full_src_register *reg = &insn->Src[0];
   struct src_register src0, srcIn;
   const bool special = (reg->Register.Absolute ||
                        reg->Register.Negate ||
   reg->Register.Indirect ||
                        if (special) {
      /* need a temp reg */
               if (special) {
      /* move the source into a temp register */
                        /* Do the discard by checking if any of the XYZW components are < 0.
   * Note that ps_2_0 and later take XYZW in consideration, while ps_1_x
   * only used XYZ.  The MSDN documentation about this is incorrect.
   */
   if (!submit_op0( emit, inst_token( SVGA3DOP_TEXKILL ), dst(src0) ))
               }
         /**
   * Translate/emit the unconditional discard instruction (usually found inside
   * an IF/ELSE/ENDIF block).
   */
   static bool
   emit_discard(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken temp;
   struct src_register one = get_one_immediate(emit);
            /* texkill doesn't allow negation on the operand so lets move
   * negation of {1} to a temp register */
   temp = get_temp( emit );
   if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), temp,
                     }
         /**
   * Test if r1 and r2 are the same register.
   */
   static bool
   same_register(struct src_register r1, struct src_register r2)
   {
      return (r1.base.num == r2.base.num &&
            }
            /**
   * Implement conditionals by initializing destination reg to 'fail',
   * then set predicate reg with UFOP_SETP, then move 'pass' to dest
   * based on predicate reg.
   *
   * SETP src0, cmp, src1  -- do this first to avoid aliasing problems.
   * MOV dst, fail
   * MOV dst, pass, p0
   */
   static bool
   emit_conditional(struct svga_shader_emitter *emit,
                  enum pipe_compare_func compare_func,
   SVGA3dShaderDestToken dst,
   struct src_register src0,
      {
      SVGA3dShaderDestToken pred_reg = dst_register( SVGA3DREG_PREDICATE, 0 );
            switch (compare_func) {
   case PIPE_FUNC_NEVER:
      return submit_op1( emit, inst_token( SVGA3DOP_MOV ),
            case PIPE_FUNC_LESS:
      setp_token = inst_token_setp(SVGA3DOPCOMP_LT);
      case PIPE_FUNC_EQUAL:
      setp_token = inst_token_setp(SVGA3DOPCOMP_EQ);
      case PIPE_FUNC_LEQUAL:
      setp_token = inst_token_setp(SVGA3DOPCOMP_LE);
      case PIPE_FUNC_GREATER:
      setp_token = inst_token_setp(SVGA3DOPCOMP_GT);
      case PIPE_FUNC_NOTEQUAL:
      setp_token = inst_token_setp(SVGA3DOPCOMPC_NE);
      case PIPE_FUNC_GEQUAL:
      setp_token = inst_token_setp(SVGA3DOPCOMP_GE);
      case PIPE_FUNC_ALWAYS:
      return submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                     if (same_register(src(dst), pass)) {
      /* We'll get bad results if the dst and pass registers are the same
   * so use a temp register containing pass.
   */
   SVGA3dShaderDestToken temp = get_temp(emit);
   if (!submit_op1(emit, inst_token(SVGA3DOP_MOV), temp, pass))
                     /* SETP src0, COMPOP, src1 */
   if (!submit_op2( emit, setp_token, pred_reg,
                  /* MOV dst, fail */
   if (!submit_op1(emit, inst_token(SVGA3DOP_MOV), dst, fail))
            /* MOV dst, pass (predicated)
   *
   * Note that the predicate reg (and possible modifiers) is passed
   * as the first source argument.
   */
   if (!submit_op2(emit,
                           }
         /**
   * Helper for emiting 'selection' commands.  Basically:
   * if (src0 OP src1)
   *    dst = 1.0;
   * else
   *    dst = 0.0;
   */
   static bool
   emit_select(struct svga_shader_emitter *emit,
               enum pipe_compare_func compare_func,
   SVGA3dShaderDestToken dst,
   {
      /* There are some SVGA instructions which implement some selects
   * directly, but they are only available in the vertex shader.
   */
   if (emit->unit == PIPE_SHADER_VERTEX) {
      switch (compare_func) {
   case PIPE_FUNC_GEQUAL:
         case PIPE_FUNC_LEQUAL:
         case PIPE_FUNC_GREATER:
         case PIPE_FUNC_LESS:
         default:
                     /* Otherwise, need to use the setp approach:
   */
   {
      struct src_register one, zero;
   /* zero immediate is 0,0,0,1 */
   zero = get_zero_immediate(emit);
                  }
         /**
   * Translate/emit a TGSI SEQ, SNE, SLT, SGE, etc. instruction.
   */
   static bool
   emit_select_op(struct svga_shader_emitter *emit,
               {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 = translate_src_register(
         struct src_register src1 = translate_src_register(
               }
         /**
   * Translate TGSI CMP instruction.  Component-wise:
   * dst = (src0 < 0.0) ? src1 : src2
   */
   static bool
   emit_cmp(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 =
         const struct src_register src1 =
         const struct src_register src2 =
            if (emit->unit == PIPE_SHADER_VERTEX) {
      struct src_register zero = get_zero_immediate(emit);
   /* We used to simulate CMP with SLT+LRP.  But that didn't work when
   * src1 or src2 was Inf/NaN.  In particular, GLSL sqrt(0) failed
   * because it involves a CMP to handle the 0 case.
   * Use a conditional expression instead.
   */
   return emit_conditional(emit, PIPE_FUNC_LESS, dst,
      }
   else {
               /* CMP  DST, SRC0, SRC2, SRC1 */
   return submit_op3( emit, inst_token( SVGA3DOP_CMP ), dst,
         }
         /**
   * Translate/emit 2-operand (coord, sampler) texture instructions.
   */
   static bool
   emit_tex2(struct svga_shader_emitter *emit,
               {
      SVGA3dShaderInstToken inst;
   struct src_register texcoord;
   struct src_register sampler;
                     switch (insn->Instruction.Opcode) {
   case TGSI_OPCODE_TEX:
      inst.op = SVGA3DOP_TEX;
      case TGSI_OPCODE_TXP:
      inst.op = SVGA3DOP_TEX;
   inst.control = SVGA3DOPCONT_PROJECT;
      case TGSI_OPCODE_TXB:
      inst.op = SVGA3DOP_TEX;
   inst.control = SVGA3DOPCONT_BIAS;
      case TGSI_OPCODE_TXL:
      inst.op = SVGA3DOP_TEXLDL;
      default:
      assert(0);
               texcoord = translate_src_register( emit, &insn->Src[0] );
            if (emit->key.tex[sampler.base.num].unnormalized ||
      emit->dynamic_branching_level > 0)
         /* Can't do mipmapping inside dynamic branch constructs.  Force LOD
   * zero in that case.
   */
   if (emit->dynamic_branching_level > 0 &&
      inst.op == SVGA3DOP_TEX &&
   SVGA3dShaderGetRegType(texcoord.base.value) == SVGA3DREG_TEMP) {
            /* MOV  tmp, texcoord */
   if (!submit_op1( emit,
                              /* MOV  tmp.w, zero */
   if (!submit_op1( emit,
                              texcoord = src( tmp );
               /* Explicit normalization of texcoords:
   */
   if (emit->key.tex[sampler.base.num].unnormalized) {
               /* MUL  tmp, SRC0, WH */
   if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ),
                                 }
         /**
   * Translate/emit 4-operand (coord, ddx, ddy, sampler) texture instructions.
   */
   static bool
   emit_tex4(struct svga_shader_emitter *emit,
               {
      SVGA3dShaderInstToken inst;
   struct src_register texcoord;
   struct src_register ddx;
   struct src_register ddy;
            texcoord = translate_src_register( emit, &insn->Src[0] );
   ddx      = translate_src_register( emit, &insn->Src[1] );
   ddy      = translate_src_register( emit, &insn->Src[2] );
                     switch (insn->Instruction.Opcode) {
   case TGSI_OPCODE_TXD:
      inst.op = SVGA3DOP_TEXLDD; /* 4 args! */
      default:
      assert(0);
                  }
         /**
   * Emit texture swizzle code.  We do this here since SVGA samplers don't
   * directly support swizzles.
   */
   static bool
   emit_tex_swizzle(struct svga_shader_emitter *emit,
                  SVGA3dShaderDestToken dst,
   struct src_register src,
   unsigned swizzle_x,
      {
      const unsigned swizzleIn[4] = {swizzle_x, swizzle_y, swizzle_z, swizzle_w};
   unsigned srcSwizzle[4];
   unsigned srcWritemask = 0x0, zeroWritemask = 0x0, oneWritemask = 0x0;
            /* build writemasks and srcSwizzle terms */
   for (i = 0; i < 4; i++) {
      if (swizzleIn[i] == PIPE_SWIZZLE_0) {
      srcSwizzle[i] = TGSI_SWIZZLE_X + i;
      }
   else if (swizzleIn[i] == PIPE_SWIZZLE_1) {
      srcSwizzle[i] = TGSI_SWIZZLE_X + i;
      }
   else {
      srcSwizzle[i] = swizzleIn[i];
                  /* write x/y/z/w comps */
   if (dst.mask & srcWritemask) {
      if (!submit_op1(emit,
                  inst_token(SVGA3DOP_MOV),
   writemask(dst, srcWritemask),
   swizzle(src,
                        /* write 0 comps */
   if (dst.mask & zeroWritemask) {
      if (!submit_op1(emit,
                  inst_token(SVGA3DOP_MOV),
            /* write 1 comps */
   if (dst.mask & oneWritemask) {
      if (!submit_op1(emit,
                  inst_token(SVGA3DOP_MOV),
               }
         /**
   * Translate/emit a TGSI texture sample instruction.
   */
   static bool
   emit_tex(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst =
         struct src_register src0 =
         struct src_register src1 =
            SVGA3dShaderDestToken tex_result;
            /* check for shadow samplers */
   bool compare = (emit->key.tex[unit].compare_mode ==
            /* texture swizzle */
   bool swizzle = (emit->key.tex[unit].swizzle_r != PIPE_SWIZZLE_X ||
                                 /* If doing compare processing or tex swizzle or saturation, we need to put
   * the fetched color into a temporary so it can be used as a source later on.
   */
   if (compare || swizzle || saturate) {
         }
   else {
                  switch(insn->Instruction.Opcode) {
   case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXP:
   case TGSI_OPCODE_TXL:
      if (!emit_tex2( emit, insn, tex_result ))
            case TGSI_OPCODE_TXD:
      if (!emit_tex4( emit, insn, tex_result ))
            default:
                  if (compare) {
               if (swizzle || saturate)
         else
            if (dst.mask & TGSI_WRITEMASK_XYZ) {
      SVGA3dShaderDestToken src0_zdivw = get_temp( emit );
   /* When sampling a depth texture, the result of the comparison is in
   * the Y component.
   */
                  if (insn->Instruction.Opcode == TGSI_OPCODE_TXP) {
      /* Divide texcoord R by Q */
   if (!submit_op1( emit, inst_token( SVGA3DOP_RCP ),
                        if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ),
                                 }
   else {
                  /* Compare texture sample value against R component of texcoord */
   if (!emit_select(emit,
                  emit->key.tex[unit].compare_func,
   writemask( dst2, TGSI_WRITEMASK_XYZ ),
            if (dst.mask & TGSI_WRITEMASK_W) {
            if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                                 if (saturate && !swizzle) {
      /* MOV_SAT real_dst, dst */
   if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), dst, src(tex_result) ))
      }
   else if (swizzle) {
      /* swizzle from tex_result to dst (handles saturation too, if any) */
   emit_tex_swizzle(emit,
                  dst, src(tex_result),
   emit->key.tex[unit].swizzle_r,
               }
         static bool
   emit_bgnloop(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderInstToken inst = inst_token( SVGA3DOP_LOOP );
   struct src_register loop_reg = src_register( SVGA3DREG_LOOP, 0 );
                     return (emit_instruction( emit, inst ) &&
            }
         static bool
   emit_endloop(struct svga_shader_emitter *emit,
         {
                           }
         /**
   * Translate/emit TGSI BREAK (out of loop) instruction.
   */
   static bool
   emit_brk(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderInstToken inst = inst_token( SVGA3DOP_BREAK );
      }
         /**
   * Emit simple instruction which operates on one scalar value (not
   * a vector).  Ex: LG2, RCP, RSQ.
   */
   static bool
   emit_scalar_op1(struct svga_shader_emitter *emit,
               {
      SVGA3dShaderInstToken inst;
   SVGA3dShaderDestToken dst;
            inst = inst_token( opcode );
   dst = translate_dst_register( emit, insn, 0 );
   src = translate_src_register( emit, &insn->Src[0] );
               }
         /**
   * Translate/emit a simple instruction (one which has no special-case
   * code) such as ADD, MUL, MIN, MAX.
   */
   static bool
   emit_simple_instruction(struct svga_shader_emitter *emit,
               {
      const struct tgsi_full_src_register *src = insn->Src;
   SVGA3dShaderInstToken inst;
            inst = inst_token( opcode );
            switch (insn->Instruction.NumSrcRegs) {
   case 0:
         case 1:
      return submit_op1( emit, inst, dst,
      case 2:
      return submit_op2( emit, inst, dst,
            case 3:
      return submit_op3( emit, inst, dst,
                  default:
      assert(0);
         }
         /**
   * TGSI_OPCODE_MOVE is only special-cased here to detect the
   * svga_fragment_shader::constant_color_output case.
   */
   static bool
   emit_mov(struct svga_shader_emitter *emit,
         {
      const struct tgsi_full_src_register *src = &insn->Src[0];
            if (emit->unit == PIPE_SHADER_FRAGMENT &&
      dst->Register.File == TGSI_FILE_OUTPUT &&
   dst->Register.Index == 0 &&
   src->Register.File == TGSI_FILE_CONSTANT &&
   !src->Register.Indirect) {
                  }
         /**
   * Translate TGSI SQRT instruction
   * if src1 == 0
   *    mov dst, src1
   * else
   *    rsq temp, src1
   *    rcp dst, temp
   * endif
   */
   static bool
   emit_sqrt(struct svga_shader_emitter *emit,
         {
      const struct src_register src1 = translate_src_register(emit, &insn->Src[0]);
   const struct src_register zero = get_zero_immediate(emit);
   SVGA3dShaderDestToken dst = translate_dst_register(emit, insn, 0);
   SVGA3dShaderDestToken temp = get_temp(emit);
   SVGA3dShaderInstToken if_token = inst_token(SVGA3DOP_IFC);
                     if (!(emit_instruction(emit, if_token) &&
         emit_src(emit, src1) &&
      ret = false;
               if (!submit_op1(emit,
            inst_token(SVGA3DOP_MOV),
   ret = false;
               if (!emit_instruction(emit, inst_token(SVGA3DOP_ELSE))) {
      ret = false;
               if (!submit_op1(emit,
            inst_token(SVGA3DOP_RSQ),
   ret = false;
               if (!submit_op1(emit,
            inst_token(SVGA3DOP_RCP),
   ret = false;
               if (!emit_instruction(emit, inst_token(SVGA3DOP_ENDIF))) {
      ret = false;
            cleanup:
                  }
         /**
   * Translate/emit TGSI DDX, DDY instructions.
   */
   static bool
   emit_deriv(struct svga_shader_emitter *emit,
         {
      if (emit->dynamic_branching_level > 0 &&
         {
      SVGA3dShaderDestToken dst =
            /* Deriv opcodes not valid inside dynamic branching, workaround
   * by zeroing out the destination.
   */
   if (!submit_op1(emit,
                                 }
   else {
      SVGA3dShaderOpCodeType opcode;
   const struct tgsi_full_src_register *reg = &insn->Src[0];
   SVGA3dShaderInstToken inst;
   SVGA3dShaderDestToken dst;
            switch (insn->Instruction.Opcode) {
   case TGSI_OPCODE_DDX:
      opcode = SVGA3DOP_DSX;
      case TGSI_OPCODE_DDY:
      opcode = SVGA3DOP_DSY;
      default:
                  inst = inst_token( opcode );
   dst = translate_dst_register( emit, insn, 0 );
            /* We cannot use negate or abs on source to dsx/dsy instruction.
   */
   if (reg->Register.Absolute ||
                     if (!emit_repl( emit, temp, &src0 ))
                     }
         /**
   * Translate/emit ARL (Address Register Load) instruction.  Used to
   * move a value into the special 'address' register.  Used to implement
   * indirect/variable indexing into arrays.
   */
   static bool
   emit_arl(struct svga_shader_emitter *emit,
         {
      ++emit->current_arl;
   if (emit->unit == PIPE_SHADER_FRAGMENT) {
      /* MOVA not present in pixel shader instruction set.
   * Ignore this instruction altogether since it is
   * only used for loop counters -- and for that
   * we reference aL directly.
   */
      }
   if (svga_arl_needs_adjustment( emit )) {
         } else {
      /* no need to adjust, just emit straight arl */
         }
         static bool
   emit_pow(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 = translate_src_register(
         struct src_register src1 = translate_src_register(
                  /* POW can only output to a temporary */
   if (insn->Dst[0].Register.File != TGSI_FILE_TEMPORARY)
            /* POW src1 must not be the same register as dst */
   if (alias_src_dst( src1, dst ))
            /* it's a scalar op */
   src0 = scalar( src0, TGSI_SWIZZLE_X );
            if (need_tmp) {
      SVGA3dShaderDestToken tmp =
            if (!submit_op2(emit, inst_token( SVGA3DOP_POW ), tmp, src0, src1))
            return submit_op1(emit, inst_token( SVGA3DOP_MOV ),
      }
   else {
            }
         /**
   * Emit a LRP (linear interpolation) instruction.
   */
   static bool
   submit_lrp(struct svga_shader_emitter *emit,
            SVGA3dShaderDestToken dst,
   struct src_register src0,
      {
      SVGA3dShaderDestToken tmp;
            /* The dst reg must be a temporary, and not be the same as src0 or src2 */
   if (SVGA3dShaderGetRegType(dst.value) != SVGA3DREG_TEMP ||
      alias_src_dst(src0, dst) ||
   alias_src_dst(src2, dst))
         if (need_dst_tmp) {
      tmp = get_temp( emit );
      }
   else {
                  if (!submit_op3(emit, inst_token( SVGA3DOP_LRP ), tmp, src0, src1, src2))
            if (need_dst_tmp) {
      if (!submit_op1(emit, inst_token( SVGA3DOP_MOV ), dst, src( tmp )))
                  }
         /**
   * Translate/emit LRP (Linear Interpolation) instruction.
   */
   static bool
   emit_lrp(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   const struct src_register src0 = translate_src_register(
         const struct src_register src1 = translate_src_register(
         const struct src_register src2 = translate_src_register(
               }
      /**
   * Translate/emit DST (Distance function) instruction.
   */
   static bool
   emit_dst_insn(struct svga_shader_emitter *emit,
         {
      if (emit->unit == PIPE_SHADER_VERTEX) {
      /* SVGA/DX9 has a DST instruction, but only for vertex shaders:
   */
      }
   else {
      /* result[0] = 1    * 1;
   * result[1] = a[1] * b[1];
   * result[2] = a[2] * 1;
   * result[3] = 1    * b[3];
   */
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   SVGA3dShaderDestToken tmp;
   const struct src_register src0 = translate_src_register(
         const struct src_register src1 = translate_src_register(
                  if (SVGA3dShaderGetRegType(dst.value) != SVGA3DREG_TEMP ||
      alias_src_dst(src0, dst) ||
               if (need_tmp) {
         }
   else {
                  /* tmp.xw = 1.0
   */
   if (tmp.mask & TGSI_WRITEMASK_XW) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                           /* tmp.yz = src0
   */
   if (tmp.mask & TGSI_WRITEMASK_YZ) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                           /* tmp.yw = tmp * src1
   */
   if (tmp.mask & TGSI_WRITEMASK_YW) {
      if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ),
                  writemask(tmp, TGSI_WRITEMASK_YW ),
            /* dst = tmp
   */
   if (need_tmp) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                                 }
         static bool
   emit_exp(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 =
                  if (dst.mask & TGSI_WRITEMASK_Y)
         else if (dst.mask & TGSI_WRITEMASK_X)
         else
            /* If y is being written, fill it with src0 - floor(src0).
   */
   if (dst.mask & TGSI_WRITEMASK_XY) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_FRC ),
                           /* If x is being written, fill it with 2 ^ floor(src0).
   */
   if (dst.mask & TGSI_WRITEMASK_X) {
      if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ),
                              if (!submit_op1( emit, inst_token( SVGA3DOP_EXP ),
                        if (!(dst.mask & TGSI_WRITEMASK_Y))
               /* If z is being written, fill it with 2 ^ src0 (partial precision).
   */
   if (dst.mask & TGSI_WRITEMASK_Z) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_EXPP ),
                           /* If w is being written, fill it with one.
   */
   if (dst.mask & TGSI_WRITEMASK_W) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                              }
         /**
   * Translate/emit LIT (Lighting helper) instruction.
   */
   static bool
   emit_lit(struct svga_shader_emitter *emit,
         {
      if (emit->unit == PIPE_SHADER_VERTEX) {
      /* SVGA/DX9 has a LIT instruction, but only for vertex shaders:
   */
      }
   else {
      /* D3D vs. GL semantics can be fairly easily accommodated by
   * variations on this sequence.
   *
   * GL:
   *   tmp.y = src.x
   *   tmp.z = pow(src.y,src.w)
   *   p0 = src0.xxxx > 0
   *   result = zero.wxxw
   *   (p0) result.yz = tmp
   *
   * D3D:
   *   tmp.y = src.x
   *   tmp.z = pow(src.y,src.w)
   *   p0 = src0.xxyy > 0
   *   result = zero.wxxw
   *   (p0) result.yz = tmp
   *
   * Will implement the GL version for now.
   */
   SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   SVGA3dShaderDestToken tmp = get_temp( emit );
   const struct src_register src0 = translate_src_register(
            /* tmp = pow(src.y, src.w)
   */
   if (dst.mask & TGSI_WRITEMASK_Z) {
      if (!submit_op2(emit, inst_token( SVGA3DOP_POW ),
                  tmp,
            /* tmp.y = src.x
   */
   if (dst.mask & TGSI_WRITEMASK_Y) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                           /* Can't quite do this with emit conditional due to the extra
   * writemask on the predicated mov:
   */
   {
                     /* D3D vs GL semantics:
   */
   if (0)
                        /* SETP src0.xxyy, GT, {0}.x */
   if (!submit_op2( emit,
                  inst_token_setp(SVGA3DOPCOMP_GT),
               /* MOV dst, fail */
   if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), dst,
                  /* MOV dst.yz, tmp (predicated)
   *
   * Note that the predicate reg (and possible modifiers) is passed
   * as the first source argument.
   */
   if (dst.mask & TGSI_WRITEMASK_YZ) {
      if (!submit_op2( emit,
                  inst_token_predicated(SVGA3DOP_MOV),
                     }
         static bool
   emit_ex2(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderInstToken inst;
   SVGA3dShaderDestToken dst;
            inst = inst_token( SVGA3DOP_EXP );
   dst = translate_dst_register( emit, insn, 0 );
   src0 = translate_src_register( emit, &insn->Src[0] );
            if (dst.mask != TGSI_WRITEMASK_XYZW) {
               if (!submit_op1( emit, inst, tmp, src0 ))
            return submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                        }
         static bool
   emit_log(struct svga_shader_emitter *emit,
         {
      SVGA3dShaderDestToken dst = translate_dst_register( emit, insn, 0 );
   struct src_register src0 =
         SVGA3dShaderDestToken abs_tmp;
   struct src_register abs_src0;
                     if (dst.mask & TGSI_WRITEMASK_Z)
         else if (dst.mask & TGSI_WRITEMASK_XY)
         else
            /* If z is being written, fill it with log2( abs( src0 ) ).
   */
   if (dst.mask & TGSI_WRITEMASK_XYZ) {
      if (!src0.base.srcMod || src0.base.srcMod == SVGA3DSRCMOD_ABS)
         else {
               if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                                             if (!submit_op1( emit, inst_token( SVGA3DOP_LOG ),
                           if (dst.mask & TGSI_WRITEMASK_XY) {
               if (dst.mask & TGSI_WRITEMASK_X)
         else
            /* If x is being written, fill it with floor( log2( abs( src0 ) ) ).
   */
   if (!submit_op1( emit, inst_token( SVGA3DOP_FRC ),
                        if (!submit_op2( emit, inst_token( SVGA3DOP_ADD ),
                              /* If y is being written, fill it with
   * abs ( src0 ) / ( 2 ^ floor( log2( abs( src0 ) ) ) ).
   */
   if (dst.mask & TGSI_WRITEMASK_Y) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_EXP ),
                              if (!submit_op2( emit, inst_token( SVGA3DOP_MUL ),
                  writemask( dst, TGSI_WRITEMASK_Y ),
            if (!(dst.mask & TGSI_WRITEMASK_X))
            if (!(dst.mask & TGSI_WRITEMASK_Z))
               if (dst.mask & TGSI_WRITEMASK_XYZ && src0.base.srcMod &&
      src0.base.srcMod != SVGA3DSRCMOD_ABS)
         /* If w is being written, fill it with one.
   */
   if (dst.mask & TGSI_WRITEMASK_W) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ),
                              }
         /**
   * Translate TGSI TRUNC or ROUND instruction.
   * We need to truncate toward zero. Ex: trunc(-1.9) = -1
   * Different approaches are needed for VS versus PS.
   */
   static bool
   emit_trunc_round(struct svga_shader_emitter *emit,
               {
      SVGA3dShaderDestToken dst = translate_dst_register(emit, insn, 0);
   const struct src_register src0 =
                  if (round) {
      SVGA3dShaderDestToken t0 = get_temp(emit);
            /* t0 = abs(src0) + 0.5 */
   if (!submit_op2(emit, inst_token(SVGA3DOP_ADD), t0,
                  /* t1 = fract(t0) */
   if (!submit_op1(emit, inst_token(SVGA3DOP_FRC), t1, src(t0)))
            /* t1 = t0 - t1 */
   if (!submit_op2(emit, inst_token(SVGA3DOP_ADD), t1, src(t0),
            }
   else {
               /* t1 = fract(abs(src0)) */
   if (!submit_op1(emit, inst_token(SVGA3DOP_FRC), t1, absolute(src0)))
            /* t1 = abs(src0) - t1 */
   if (!submit_op2(emit, inst_token(SVGA3DOP_ADD), t1, absolute(src0),
                     /*
   * Now we need to multiply t1 by the sign of the original value.
   */
   if (emit->unit == PIPE_SHADER_VERTEX) {
      /* For VS: use SGN instruction */
   /* Need two extra/dummy registers: */
   SVGA3dShaderDestToken t2 = get_temp(emit), t3 = get_temp(emit),
            /* t2 = sign(src0) */
   if (!submit_op3(emit, inst_token(SVGA3DOP_SGN), t2, src0,
                  /* dst = t1 * t2 */
   if (!submit_op2(emit, inst_token(SVGA3DOP_MUL), dst, src(t1), src(t2)))
      }
   else {
      /* For FS: Use CMP instruction */
   return submit_op3(emit, inst_token( SVGA3DOP_CMP ), dst,
                  }
         /**
   * Translate/emit "begin subroutine" instruction/marker/label.
   */
   static bool
   emit_bgnsub(struct svga_shader_emitter *emit,
               {
               /* Note that we've finished the main function and are now emitting
   * subroutines.  This affects how we terminate the generated
   * shader.
   */
            for (i = 0; i < emit->nr_labels; i++) {
      if (emit->label[i] == position) {
      return (emit_instruction( emit, inst_token( SVGA3DOP_RET ) ) &&
                        assert(0);
      }
         /**
   * Translate/emit subroutine call instruction.
   */
   static bool
   emit_call(struct svga_shader_emitter *emit,
         {
      unsigned position = insn->Label.Label;
            for (i = 0; i < emit->nr_labels; i++) {
      if (emit->label[i] == position)
               if (emit->nr_labels == ARRAY_SIZE(emit->label))
            if (i == emit->nr_labels) {
      emit->label[i] = position;
               return (emit_instruction( emit, inst_token( SVGA3DOP_CALL ) ) &&
      }
         /**
   * Called at the end of the shader.  Actually, emit special "fix-up"
   * code for the vertex/fragment shader.
   */
   static bool
   emit_end(struct svga_shader_emitter *emit)
   {
      if (emit->unit == PIPE_SHADER_VERTEX) {
         }
   else {
            }
         /**
   * Translate any TGSI instruction to SVGA.
   */
   static bool
   svga_emit_instruction(struct svga_shader_emitter *emit,
               {
               case TGSI_OPCODE_ARL:
            case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXP:
   case TGSI_OPCODE_TXL:
   case TGSI_OPCODE_TXD:
            case TGSI_OPCODE_DDX:
   case TGSI_OPCODE_DDY:
            case TGSI_OPCODE_BGNSUB:
            case TGSI_OPCODE_ENDSUB:
            case TGSI_OPCODE_CAL:
            case TGSI_OPCODE_FLR:
            case TGSI_OPCODE_TRUNC:
            case TGSI_OPCODE_ROUND:
            case TGSI_OPCODE_CEIL:
            case TGSI_OPCODE_CMP:
            case TGSI_OPCODE_DIV:
            case TGSI_OPCODE_DP2:
            case TGSI_OPCODE_COS:
            case TGSI_OPCODE_SIN:
            case TGSI_OPCODE_END:
      /* TGSI always finishes the main func with an END */
         case TGSI_OPCODE_KILL_IF:
               /* Selection opcodes.  The underlying language is fairly
   * non-orthogonal about these.
      case TGSI_OPCODE_SEQ:
            case TGSI_OPCODE_SNE:
            case TGSI_OPCODE_SGT:
            case TGSI_OPCODE_SGE:
            case TGSI_OPCODE_SLT:
            case TGSI_OPCODE_SLE:
            case TGSI_OPCODE_POW:
            case TGSI_OPCODE_EX2:
            case TGSI_OPCODE_EXP:
            case TGSI_OPCODE_LOG:
            case TGSI_OPCODE_LG2:
            case TGSI_OPCODE_RSQ:
            case TGSI_OPCODE_RCP:
            case TGSI_OPCODE_CONT:
      /* not expected (we return PIPE_SHADER_CAP_CONT_SUPPORTED = 0) */
         case TGSI_OPCODE_RET:
      /* This is a noop -- we tell mesa that we can't support RET
   * within a function (early return), so this will always be
   * followed by an ENDSUB.
   */
            /* These aren't actually used by any of the frontends we care
   * about:
      case TGSI_OPCODE_AND:
   case TGSI_OPCODE_OR:
   case TGSI_OPCODE_I2F:
   case TGSI_OPCODE_NOT:
   case TGSI_OPCODE_SHL:
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_XOR:
            case TGSI_OPCODE_IF:
         case TGSI_OPCODE_ELSE:
         case TGSI_OPCODE_ENDIF:
            case TGSI_OPCODE_BGNLOOP:
         case TGSI_OPCODE_ENDLOOP:
         case TGSI_OPCODE_BRK:
            case TGSI_OPCODE_KILL:
            case TGSI_OPCODE_DST:
            case TGSI_OPCODE_LIT:
            case TGSI_OPCODE_LRP:
            case TGSI_OPCODE_SSG:
            case TGSI_OPCODE_MOV:
            case TGSI_OPCODE_SQRT:
            default:
      {
                                    if (!emit_simple_instruction( emit, opcode, insn ))
                     }
         /**
   * Translate/emit a TGSI IMMEDIATE declaration.
   * An immediate vector is a constant that's hard-coded into the shader.
   */
   static bool
   svga_emit_immediate(struct svga_shader_emitter *emit,
         {
      static const float id[4] = {0,0,0,1};
   float value[4];
            assert(1 <= imm->Immediate.NrTokens && imm->Immediate.NrTokens <= 5);
   for (i = 0; i < 4 && i < imm->Immediate.NrTokens - 1; i++) {
      float f = imm->u[i].Float;
               /* If the immediate has less than four values, fill in the remaining
   * positions from id={0,0,0,1}.
   */
   for ( ; i < 4; i++ )
            return emit_def_const( emit, SVGA3D_CONST_TYPE_FLOAT,
            }
         static bool
   make_immediate(struct svga_shader_emitter *emit,
               {
               if (!emit_def_const( emit, SVGA3D_CONST_TYPE_FLOAT,
                              }
         /**
   * Emit special VS instructions at top of shader.
   */
   static bool
   emit_vs_preamble(struct svga_shader_emitter *emit)
   {
      if (!emit->key.vs.need_prescale) {
      if (!make_immediate( emit, 0, 0, .5, .5,
                        }
         /**
   * Emit special PS instructions at top of shader.
   */
   static bool
   emit_ps_preamble(struct svga_shader_emitter *emit)
   {
      if (emit->ps_reads_pos && emit->info.reads_z) {
      /*
   * Assemble the position from various bits of inputs. Depth and W are
   * passed in a texcoord this is due to D3D's vPos not hold Z or W.
   * Also fixup the perspective interpolation.
   *
   * temp_pos.xy = vPos.xy
   * temp_pos.w = rcp(texcoord1.w);
   * temp_pos.z = texcoord1.z * temp_pos.w;
   */
   if (!submit_op1( emit,
                              if (!submit_op1( emit,
                              if (!submit_op2( emit,
                  inst_token(SVGA3DOP_MUL),
   writemask( emit->ps_temp_pos, TGSI_WRITEMASK_Z ),
               }
         /**
   * Emit special PS instructions at end of shader.
   */
   static bool
   emit_ps_postamble(struct svga_shader_emitter *emit)
   {
               /* PS oDepth is incredibly fragile and it's very hard to catch the
   * types of usage that break it during shader emit.  Easier just to
   * redirect the main program to a temporary and then only touch
   * oDepth with a hand-crafted MOV below.
   */
   if (SVGA3dShaderGetRegType(emit->true_pos.value) != 0) {
      if (!submit_op1( emit,
                  inst_token(SVGA3DOP_MOV),
            for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      if (SVGA3dShaderGetRegType(emit->true_color_output[i].value) != 0) {
      /* Potentially override output colors with white for XOR
   * logicop workaround.
   */
   if (emit->unit == PIPE_SHADER_FRAGMENT &&
                     if (!submit_op1( emit,
                  inst_token(SVGA3DOP_MOV),
   }
   else if (emit->unit == PIPE_SHADER_FRAGMENT &&
            /* Write temp color output [0] to true output [i] */
   if (!submit_op1(emit, inst_token(SVGA3DOP_MOV),
                        }
   else {
      if (!submit_op1( emit,
                  inst_token(SVGA3DOP_MOV),
                     }
         /**
   * Emit special VS instructions at end of shader.
   */
   static bool
   emit_vs_postamble(struct svga_shader_emitter *emit)
   {
      /* PSIZ output is incredibly fragile and it's very hard to catch
   * the types of usage that break it during shader emit.  Easier
   * just to redirect the main program to a temporary and then only
   * touch PSIZ with a hand-crafted MOV below.
   */
   if (SVGA3dShaderGetRegType(emit->true_psiz.value) != 0) {
      if (!submit_op1( emit,
                  inst_token(SVGA3DOP_MOV),
            /* Need to perform various manipulations on vertex position to cope
   * with the different GL and D3D clip spaces.
   */
   if (emit->key.vs.need_prescale) {
      SVGA3dShaderDestToken temp_pos = emit->temp_pos;
   SVGA3dShaderDestToken depth = emit->depth_pos;
   SVGA3dShaderDestToken pos = emit->true_pos;
   unsigned offset = emit->info.file_max[TGSI_FILE_CONSTANT] + 1;
   struct src_register prescale_scale = src_register( SVGA3DREG_CONST,
         struct src_register prescale_trans = src_register( SVGA3DREG_CONST,
            if (!submit_op1( emit,
                              /* MUL temp_pos.xyz,    temp_pos,      prescale.scale
   * MAD result.position, temp_pos.wwww, prescale.trans, temp_pos
   *   --> Note that prescale.trans.w == 0
   */
   if (!submit_op2( emit,
                  inst_token(SVGA3DOP_MUL),
               if (!submit_op3( emit,
                  inst_token(SVGA3DOP_MAD),
   pos,
               /* Also write to depth value */
   if (!submit_op3( emit,
                  inst_token(SVGA3DOP_MAD),
   writemask(depth, TGSI_WRITEMASK_Z),
   swizzle(src(temp_pos), 3, 3, 3, 3),
   }
   else {
      SVGA3dShaderDestToken temp_pos = emit->temp_pos;
   SVGA3dShaderDestToken depth = emit->depth_pos;
   SVGA3dShaderDestToken pos = emit->true_pos;
            /* Adjust GL clipping coordinate space to hardware (D3D-style):
   *
   * DP4 temp_pos.z, {0,0,.5,.5}, temp_pos
   * MOV result.position, temp_pos
   */
   if (!submit_op2( emit,
                  inst_token(SVGA3DOP_DP4),
               if (!submit_op1( emit,
                              /* Move the manipulated depth into the extra texcoord reg */
   if (!submit_op1( emit,
                  inst_token(SVGA3DOP_MOV),
               }
         /**
   * For the pixel shader: emit the code which chooses the front
   * or back face color depending on triangle orientation.
   * This happens at the top of the fragment shader.
   *
   *  0: IF VFACE :4
   *  1:   COLOR = FrontColor;
   *  2: ELSE
   *  3:   COLOR = BackColor;
   *  4: ENDIF
   */
   static bool
   emit_light_twoside(struct svga_shader_emitter *emit)
   {
      struct src_register vface, zero;
   struct src_register front[2];
   struct src_register back[2];
   SVGA3dShaderDestToken color[2];
   int count = emit->internal_color_count;
   unsigned i;
            if (count == 0)
            vface = get_vface( emit );
            /* Can't use get_temp() to allocate the color reg as such
   * temporaries will be reclaimed after each instruction by the call
   * to reset_temp_regs().
   */
   for (i = 0; i < count; i++) {
      color[i] = dst_register( SVGA3DREG_TEMP, emit->nr_hw_temp++ );
            /* Back is always the next input:
   */
   back[i] = front[i];
            /* Reassign the input_map to the actual front-face color:
   */
                        if (emit->key.fs.front_ccw)
         else
            if (!(emit_instruction( emit, if_token ) &&
         emit_src( emit, vface ) &&
            for (i = 0; i < count; i++) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), color[i], front[i] ))
               if (!(emit_instruction( emit, inst_token( SVGA3DOP_ELSE))))
            for (i = 0; i < count; i++) {
      if (!submit_op1( emit, inst_token( SVGA3DOP_MOV ), color[i], back[i] ))
               if (!emit_instruction( emit, inst_token( SVGA3DOP_ENDIF ) ))
               }
         /**
   * Emit special setup code for the front/back face register in the FS.
   *  0: SETP_GT TEMP, VFACE, 0
   *  where TEMP is a fake frontface register
   */
   static bool
   emit_frontface(struct svga_shader_emitter *emit)
   {
      struct src_register vface;
   SVGA3dShaderDestToken temp;
                     /* Can't use get_temp() to allocate the fake frontface reg as such
   * temporaries will be reclaimed after each instruction by the call
   * to reset_temp_regs().
   */
   temp = dst_register( SVGA3DREG_TEMP,
            if (emit->key.fs.front_ccw) {
      pass = get_zero_immediate(emit);
      } else {
      pass = get_one_immediate(emit);
               if (!emit_conditional(emit, PIPE_FUNC_GREATER,
                        /* Reassign the input_map to the actual front-face color:
   */
               }
         /**
   * Emit code to invert the T component of the incoming texture coordinate.
   * This is used for drawing point sprites when
   * pipe_rasterizer_state::sprite_coord_mode == PIPE_SPRITE_COORD_LOWER_LEFT.
   */
   static bool
   emit_inverted_texcoords(struct svga_shader_emitter *emit)
   {
               while (inverted_texcoords) {
                                          assert(emit->ps_inverted_texcoord_input[unit]
            /* inverted = coord * (1, -1, 1, 1) + (0, 1, 0, 0) */
   if (!submit_op3(emit,
                  inst_token(SVGA3DOP_MAD),
   dst(emit->ps_inverted_texcoord[unit]),
               /* Reassign the input_map entry to the new texcoord register */
   emit->input_map[emit->ps_inverted_texcoord_input[unit]] =
                           }
         /**
   * Emit code to adjust vertex shader inputs/attributes:
   * - Change range from [0,1] to [-1,1] (for normalized byte/short attribs).
   * - Set attrib W component = 1.
   */
   static bool
   emit_adjusted_vertex_attribs(struct svga_shader_emitter *emit)
   {
      unsigned adjust_mask = (emit->key.vs.adjust_attrib_range |
            while (adjust_mask) {
      /* Adjust vertex attrib range and/or set W component = 1 */
   const unsigned index = u_bit_scan(&adjust_mask);
            /* allocate a temp reg */
   tmp = src_register(SVGA3DREG_TEMP, emit->nr_hw_temp);
            if (emit->key.vs.adjust_attrib_range & (1 << index)) {
      /* The vertex input/attribute is supposed to be a signed value in
   * the range [-1,1] but we actually fetched/converted it to the
   * range [0,1].  This most likely happens when the app specifies a
   * signed byte attribute but we interpreted it as unsigned bytes.
   * See also svga_translate_vertex_format().
   *
   * Here, we emit some extra instructions to adjust
   * the attribute values from [0,1] to [-1,1].
   *
   * The adjustment we implement is:
   *   new_attrib = attrib * 2.0;
   *   if (attrib >= 0.5)
   *      new_attrib = new_attrib - 2.0;
   * This isn't exactly right (it's off by a bit or so) but close enough.
                  /* tmp = attrib * 2.0 */
   if (!submit_op2(emit,
                  inst_token(SVGA3DOP_MUL),
               /* pred = (attrib >= 0.5) */
   if (!submit_op2(emit,
                  inst_token_setp(SVGA3DOPCOMP_GE),
               /* sub(pred) tmp, tmp, 2.0 */
   if (!submit_op3(emit,
                  inst_token_predicated(SVGA3DOP_SUB),
   dst(tmp),
   src(pred_reg),
   }
   else {
      /* just copy the vertex input attrib to the temp register */
   if (!submit_op1(emit,
                  inst_token(SVGA3DOP_MOV),
            if (emit->key.vs.adjust_attrib_w_1 & (1 << index)) {
      /* move 1 into W position of tmp */
   if (!submit_op1(emit,
                  inst_token(SVGA3DOP_MOV),
            /* Reassign the input_map entry to the new tmp register */
                  }
         /**
   * Determine if we need to create the "common" immediate value which is
   * used for generating useful vector constants such as {0,0,0,0} and
   * {1,1,1,1}.
   * We could just do this all the time except that we want to conserve
   * registers whenever possible.
   */
   static bool
   needs_to_create_common_immediate(const struct svga_shader_emitter *emit)
   {
               if (emit->unit == PIPE_SHADER_FRAGMENT) {
      if (emit->key.fs.light_twoside)
            if (emit->key.fs.white_fragments)
            if (emit->emit_frontface)
            if (emit->info.opcode_count[TGSI_OPCODE_DST] >= 1 ||
      emit->info.opcode_count[TGSI_OPCODE_SSG] >= 1 ||
               if (emit->inverted_texcoords)
            /* look for any PIPE_SWIZZLE_0/ONE terms */
   for (i = 0; i < emit->key.num_textures; i++) {
      if (emit->key.tex[i].swizzle_r > PIPE_SWIZZLE_W ||
      emit->key.tex[i].swizzle_g > PIPE_SWIZZLE_W ||
   emit->key.tex[i].swizzle_b > PIPE_SWIZZLE_W ||
   emit->key.tex[i].swizzle_a > PIPE_SWIZZLE_W)
            for (i = 0; i < emit->key.num_textures; i++) {
      if (emit->key.tex[i].compare_mode
      == PIPE_TEX_COMPARE_R_TO_TEXTURE)
      }
   else if (emit->unit == PIPE_SHADER_VERTEX) {
      if (emit->info.opcode_count[TGSI_OPCODE_CMP] >= 1)
         if (emit->key.vs.adjust_attrib_range ||
      emit->key.vs.adjust_attrib_w_1)
            if (emit->info.opcode_count[TGSI_OPCODE_IF] >= 1 ||
      emit->info.opcode_count[TGSI_OPCODE_BGNLOOP] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_DDX] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_DDY] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_ROUND] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_SGE] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_SGT] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_SLE] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_SLT] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_SNE] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_SEQ] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_EXP] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_LOG] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_KILL] >= 1 ||
   emit->info.opcode_count[TGSI_OPCODE_SQRT] >= 1)
            }
         /**
   * Do we need to create a looping constant?
   */
   static bool
   needs_to_create_loop_const(const struct svga_shader_emitter *emit)
   {
         }
         static bool
   needs_to_create_arl_consts(const struct svga_shader_emitter *emit)
   {
         }
         static bool
   pre_parse_add_indirect( struct svga_shader_emitter *emit,
         {
      unsigned i;
            for (i = 0; i < emit->num_arl_consts; ++i) {
      if (emit->arl_consts[i].arl_num == current_arl)
      }
   /* new entry */
   if (emit->num_arl_consts == i) {
         }
   emit->arl_consts[i].number = (emit->arl_consts[i].number > num) ?
               emit->arl_consts[i].arl_num = current_arl;
      }
         static bool
   pre_parse_instruction( struct svga_shader_emitter *emit,
               {
      if (insn->Src[0].Register.Indirect &&
      insn->Src[0].Indirect.File == TGSI_FILE_ADDRESS) {
   const struct tgsi_full_src_register *reg = &insn->Src[0];
   if (reg->Register.Index < 0) {
                     if (insn->Src[1].Register.Indirect &&
      insn->Src[1].Indirect.File == TGSI_FILE_ADDRESS) {
   const struct tgsi_full_src_register *reg = &insn->Src[1];
   if (reg->Register.Index < 0) {
                     if (insn->Src[2].Register.Indirect &&
      insn->Src[2].Indirect.File == TGSI_FILE_ADDRESS) {
   const struct tgsi_full_src_register *reg = &insn->Src[2];
   if (reg->Register.Index < 0) {
                        }
         static bool
   pre_parse_tokens( struct svga_shader_emitter *emit,
         {
      struct tgsi_parse_context parse;
                     while (!tgsi_parse_end_of_tokens( &parse )) {
      tgsi_parse_token( &parse );
   switch (parse.FullToken.Token.Type) {
   case TGSI_TOKEN_TYPE_IMMEDIATE:
   case TGSI_TOKEN_TYPE_DECLARATION:
         case TGSI_TOKEN_TYPE_INSTRUCTION:
      if (parse.FullToken.FullInstruction.Instruction.Opcode ==
      TGSI_OPCODE_ARL) {
      }
   if (!pre_parse_instruction( emit, &parse.FullToken.FullInstruction,
                  default:
               }
      }
         static bool
   svga_shader_emit_helpers(struct svga_shader_emitter *emit)
   {
      if (needs_to_create_common_immediate( emit )) {
         }
   if (needs_to_create_loop_const( emit )) {
         }
   if (needs_to_create_arl_consts( emit )) {
                  if (emit->unit == PIPE_SHADER_FRAGMENT) {
      if (!svga_shader_emit_samplers_decl( emit ))
            if (!emit_ps_preamble( emit ))
            if (emit->key.fs.light_twoside) {
      if (!emit_light_twoside( emit ))
      }
   if (emit->emit_frontface) {
      if (!emit_frontface( emit ))
      }
   if (emit->inverted_texcoords) {
      if (!emit_inverted_texcoords( emit ))
         }
   else {
      assert(emit->unit == PIPE_SHADER_VERTEX);
   if (emit->key.vs.adjust_attrib_range) {
      if (!emit_adjusted_vertex_attribs(emit) ||
      emit->key.vs.adjust_attrib_w_1) {
                        }
         /**
   * This is the main entrypoint into the TGSI instruction translater.
   * Translate TGSI shader tokens into an SVGA shader.
   */
   bool
   svga_shader_emit_instructions(struct svga_shader_emitter *emit,
         {
      struct tgsi_parse_context parse;
   const struct tgsi_token *new_tokens = NULL;
   bool ret = true;
   bool helpers_emitted = false;
            if (emit->unit == PIPE_SHADER_FRAGMENT && emit->key.fs.pstipple) {
               new_tokens = util_pstipple_create_fragment_shader(tokens, &unit, 0,
            if (new_tokens) {
      /* Setup texture state for stipple */
   emit->sampler_target[unit] = TGSI_TEXTURE_2D;
   emit->key.tex[unit].swizzle_r = TGSI_SWIZZLE_X;
   emit->key.tex[unit].swizzle_g = TGSI_SWIZZLE_Y;
                                          tgsi_parse_init( &parse, tokens );
            if (emit->unit == PIPE_SHADER_VERTEX) {
      ret = emit_vs_preamble( emit );
   if (!ret)
                        while (!tgsi_parse_end_of_tokens( &parse )) {
               switch (parse.FullToken.Token.Type) {
   case TGSI_TOKEN_TYPE_IMMEDIATE:
      ret = svga_emit_immediate( emit, &parse.FullToken.FullImmediate );
   if (!ret)
               case TGSI_TOKEN_TYPE_DECLARATION:
      ret = svga_translate_decl_sm30( emit, &parse.FullToken.FullDeclaration );
   if (!ret)
               case TGSI_TOKEN_TYPE_INSTRUCTION:
      if (!helpers_emitted) {
      if (!svga_shader_emit_helpers( emit ))
            }
   ret = svga_emit_instruction( emit,
               if (!ret)
            default:
                              /* Need to terminate the current subroutine.  Note that the
   * hardware doesn't tolerate shaders without sub-routines
   * terminating with RET+END.
   */
   if (!emit->in_main_func) {
      ret = emit_instruction( emit, inst_token( SVGA3DOP_RET ) );
   if (!ret)
                        /* Need to terminate the whole shader:
   */
   ret = emit_instruction( emit, inst_token( SVGA3DOP_END ) );
   if (!ret)
         done:
      tgsi_parse_free( &parse );
   if (new_tokens) {
                     }
