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
      #include "brw_cfg.h"
   #include "brw_eu.h"
   #include "brw_fs.h"
   #include "brw_nir.h"
   #include "brw_private.h"
   #include "brw_vec4_tes.h"
   #include "dev/intel_debug.h"
   #include "util/macros.h"
   #include "util/u_debug.h"
      enum brw_reg_type
   brw_type_for_base_type(const struct glsl_type *type)
   {
      switch (type->base_type) {
   case GLSL_TYPE_FLOAT16:
         case GLSL_TYPE_FLOAT:
         case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
   case GLSL_TYPE_SUBROUTINE:
         case GLSL_TYPE_INT16:
         case GLSL_TYPE_INT8:
         case GLSL_TYPE_UINT:
         case GLSL_TYPE_UINT16:
         case GLSL_TYPE_UINT8:
         case GLSL_TYPE_ARRAY:
         case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_ATOMIC_UINT:
      /* These should be overridden with the type of the member when
   * dereferenced into.  BRW_REGISTER_TYPE_UD seems like a likely
   * way to trip up if we don't.
   */
      case GLSL_TYPE_IMAGE:
         case GLSL_TYPE_DOUBLE:
         case GLSL_TYPE_UINT64:
         case GLSL_TYPE_INT64:
         case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
   case GLSL_TYPE_COOPERATIVE_MATRIX:
                     }
      uint32_t
   brw_math_function(enum opcode op)
   {
      switch (op) {
   case SHADER_OPCODE_RCP:
         case SHADER_OPCODE_RSQ:
         case SHADER_OPCODE_SQRT:
         case SHADER_OPCODE_EXP2:
         case SHADER_OPCODE_LOG2:
         case SHADER_OPCODE_POW:
         case SHADER_OPCODE_SIN:
         case SHADER_OPCODE_COS:
         case SHADER_OPCODE_INT_QUOTIENT:
         case SHADER_OPCODE_INT_REMAINDER:
         default:
            }
      bool
   brw_texture_offset(const nir_tex_instr *tex, unsigned src,
         {
      if (!nir_src_is_const(tex->src[src].src))
                     /* Combine all three offsets into a single unsigned dword:
   *
   *    bits 11:8 - U Offset (X component)
   *    bits  7:4 - V Offset (Y component)
   *    bits  3:0 - R Offset (Z component)
   */
   uint32_t offset_bits = 0;
   for (unsigned i = 0; i < num_components; i++) {
               /* offset out of bounds; caller will handle it. */
   if (offset > 7 || offset < -8)
            const unsigned shift = 4 * (2 - i);
                           }
      const char *
   brw_instruction_name(const struct brw_isa_info *isa, enum opcode op)
   {
               switch (op) {
   case 0 ... NUM_BRW_OPCODES - 1:
      /* The DO instruction doesn't exist on Gfx6+, but we use it to mark the
   * start of a loop in the IR.
   */
   if (devinfo->ver >= 6 && op == BRW_OPCODE_DO)
            /* The following conversion opcodes doesn't exist on Gfx8+, but we use
   * then to mark that we want to do the conversion.
   */
   if (devinfo->ver > 7 && op == BRW_OPCODE_F32TO16)
            if (devinfo->ver > 7 && op == BRW_OPCODE_F16TO32)
            assert(brw_opcode_desc(isa, op)->name);
      case FS_OPCODE_FB_WRITE:
         case FS_OPCODE_FB_WRITE_LOGICAL:
         case FS_OPCODE_REP_FB_WRITE:
         case FS_OPCODE_FB_READ:
         case FS_OPCODE_FB_READ_LOGICAL:
            case SHADER_OPCODE_RCP:
         case SHADER_OPCODE_RSQ:
         case SHADER_OPCODE_SQRT:
         case SHADER_OPCODE_EXP2:
         case SHADER_OPCODE_LOG2:
         case SHADER_OPCODE_POW:
         case SHADER_OPCODE_INT_QUOTIENT:
         case SHADER_OPCODE_INT_REMAINDER:
         case SHADER_OPCODE_SIN:
         case SHADER_OPCODE_COS:
            case SHADER_OPCODE_SEND:
            case SHADER_OPCODE_UNDEF:
            case SHADER_OPCODE_TEX:
         case SHADER_OPCODE_TEX_LOGICAL:
         case SHADER_OPCODE_TXD:
         case SHADER_OPCODE_TXD_LOGICAL:
         case SHADER_OPCODE_TXF:
         case SHADER_OPCODE_TXF_LOGICAL:
         case SHADER_OPCODE_TXF_LZ:
         case SHADER_OPCODE_TXL:
         case SHADER_OPCODE_TXL_LOGICAL:
         case SHADER_OPCODE_TXL_LZ:
         case SHADER_OPCODE_TXS:
         case SHADER_OPCODE_TXS_LOGICAL:
         case FS_OPCODE_TXB:
         case FS_OPCODE_TXB_LOGICAL:
         case SHADER_OPCODE_TXF_CMS:
         case SHADER_OPCODE_TXF_CMS_LOGICAL:
         case SHADER_OPCODE_TXF_CMS_W:
         case SHADER_OPCODE_TXF_CMS_W_LOGICAL:
         case SHADER_OPCODE_TXF_CMS_W_GFX12_LOGICAL:
         case SHADER_OPCODE_TXF_UMS:
         case SHADER_OPCODE_TXF_UMS_LOGICAL:
         case SHADER_OPCODE_TXF_MCS:
         case SHADER_OPCODE_TXF_MCS_LOGICAL:
         case SHADER_OPCODE_LOD:
         case SHADER_OPCODE_LOD_LOGICAL:
         case SHADER_OPCODE_TG4:
         case SHADER_OPCODE_TG4_LOGICAL:
         case SHADER_OPCODE_TG4_OFFSET:
         case SHADER_OPCODE_TG4_OFFSET_LOGICAL:
         case SHADER_OPCODE_SAMPLEINFO:
         case SHADER_OPCODE_SAMPLEINFO_LOGICAL:
            case SHADER_OPCODE_IMAGE_SIZE_LOGICAL:
            case VEC4_OPCODE_UNTYPED_ATOMIC:
         case SHADER_OPCODE_UNTYPED_ATOMIC_LOGICAL:
         case VEC4_OPCODE_UNTYPED_SURFACE_READ:
         case SHADER_OPCODE_UNTYPED_SURFACE_READ_LOGICAL:
         case VEC4_OPCODE_UNTYPED_SURFACE_WRITE:
         case SHADER_OPCODE_UNTYPED_SURFACE_WRITE_LOGICAL:
         case SHADER_OPCODE_UNALIGNED_OWORD_BLOCK_READ_LOGICAL:
         case SHADER_OPCODE_OWORD_BLOCK_WRITE_LOGICAL:
         case SHADER_OPCODE_A64_UNTYPED_READ_LOGICAL:
         case SHADER_OPCODE_A64_OWORD_BLOCK_READ_LOGICAL:
         case SHADER_OPCODE_A64_UNALIGNED_OWORD_BLOCK_READ_LOGICAL:
         case SHADER_OPCODE_A64_OWORD_BLOCK_WRITE_LOGICAL:
         case SHADER_OPCODE_A64_UNTYPED_WRITE_LOGICAL:
         case SHADER_OPCODE_A64_BYTE_SCATTERED_READ_LOGICAL:
         case SHADER_OPCODE_A64_BYTE_SCATTERED_WRITE_LOGICAL:
         case SHADER_OPCODE_A64_UNTYPED_ATOMIC_LOGICAL:
         case SHADER_OPCODE_TYPED_ATOMIC_LOGICAL:
         case SHADER_OPCODE_TYPED_SURFACE_READ_LOGICAL:
         case SHADER_OPCODE_TYPED_SURFACE_WRITE_LOGICAL:
         case SHADER_OPCODE_MEMORY_FENCE:
         case FS_OPCODE_SCHEDULING_FENCE:
         case SHADER_OPCODE_INTERLOCK:
      /* For an interlock we actually issue a memory fence via sendc. */
         case SHADER_OPCODE_BYTE_SCATTERED_READ_LOGICAL:
         case SHADER_OPCODE_BYTE_SCATTERED_WRITE_LOGICAL:
         case SHADER_OPCODE_DWORD_SCATTERED_READ_LOGICAL:
         case SHADER_OPCODE_DWORD_SCATTERED_WRITE_LOGICAL:
            case SHADER_OPCODE_LOAD_PAYLOAD:
         case FS_OPCODE_PACK:
            case SHADER_OPCODE_GFX4_SCRATCH_READ:
         case SHADER_OPCODE_GFX4_SCRATCH_WRITE:
         case SHADER_OPCODE_GFX7_SCRATCH_READ:
         case SHADER_OPCODE_SCRATCH_HEADER:
            case SHADER_OPCODE_URB_WRITE_LOGICAL:
         case SHADER_OPCODE_URB_READ_LOGICAL:
            case SHADER_OPCODE_FIND_LIVE_CHANNEL:
         case SHADER_OPCODE_FIND_LAST_LIVE_CHANNEL:
         case FS_OPCODE_LOAD_LIVE_CHANNELS:
            case SHADER_OPCODE_BROADCAST:
         case SHADER_OPCODE_SHUFFLE:
         case SHADER_OPCODE_SEL_EXEC:
         case SHADER_OPCODE_QUAD_SWIZZLE:
         case SHADER_OPCODE_CLUSTER_BROADCAST:
            case SHADER_OPCODE_GET_BUFFER_SIZE:
            case VEC4_OPCODE_MOV_BYTES:
         case VEC4_OPCODE_PACK_BYTES:
         case VEC4_OPCODE_UNPACK_UNIFORM:
         case VEC4_OPCODE_DOUBLE_TO_F32:
         case VEC4_OPCODE_DOUBLE_TO_D32:
         case VEC4_OPCODE_DOUBLE_TO_U32:
         case VEC4_OPCODE_TO_DOUBLE:
         case VEC4_OPCODE_PICK_LOW_32BIT:
         case VEC4_OPCODE_PICK_HIGH_32BIT:
         case VEC4_OPCODE_SET_LOW_32BIT:
         case VEC4_OPCODE_SET_HIGH_32BIT:
         case VEC4_OPCODE_MOV_FOR_SCRATCH:
         case VEC4_OPCODE_ZERO_OOB_PUSH_REGS:
            case FS_OPCODE_DDX_COARSE:
         case FS_OPCODE_DDX_FINE:
         case FS_OPCODE_DDY_COARSE:
         case FS_OPCODE_DDY_FINE:
            case FS_OPCODE_LINTERP:
            case FS_OPCODE_PIXEL_X:
         case FS_OPCODE_PIXEL_Y:
            case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD:
         case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD_GFX4:
         case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD_LOGICAL:
            case FS_OPCODE_SET_SAMPLE_ID:
            case FS_OPCODE_PACK_HALF_2x16_SPLIT:
            case SHADER_OPCODE_HALT_TARGET:
            case FS_OPCODE_INTERPOLATE_AT_SAMPLE:
         case FS_OPCODE_INTERPOLATE_AT_SHARED_OFFSET:
         case FS_OPCODE_INTERPOLATE_AT_PER_SLOT_OFFSET:
            case VEC4_VS_OPCODE_URB_WRITE:
         case VS_OPCODE_PULL_CONSTANT_LOAD:
         case VS_OPCODE_PULL_CONSTANT_LOAD_GFX7:
            case VS_OPCODE_UNPACK_FLAGS_SIMD4X2:
            case VEC4_GS_OPCODE_URB_WRITE:
         case VEC4_GS_OPCODE_URB_WRITE_ALLOCATE:
         case GS_OPCODE_THREAD_END:
         case GS_OPCODE_SET_WRITE_OFFSET:
         case GS_OPCODE_SET_VERTEX_COUNT:
         case GS_OPCODE_SET_DWORD_2:
         case GS_OPCODE_PREPARE_CHANNEL_MASKS:
         case GS_OPCODE_SET_CHANNEL_MASKS:
         case GS_OPCODE_GET_INSTANCE_ID:
         case GS_OPCODE_FF_SYNC:
         case GS_OPCODE_SET_PRIMITIVE_ID:
         case GS_OPCODE_SVB_WRITE:
         case GS_OPCODE_SVB_SET_DST_INDEX:
         case GS_OPCODE_FF_SYNC_SET_PRIMITIVES:
         case CS_OPCODE_CS_TERMINATE:
         case SHADER_OPCODE_BARRIER:
         case SHADER_OPCODE_MULH:
         case SHADER_OPCODE_ISUB_SAT:
         case SHADER_OPCODE_USUB_SAT:
         case SHADER_OPCODE_MOV_INDIRECT:
         case SHADER_OPCODE_MOV_RELOC_IMM:
            case VEC4_OPCODE_URB_READ:
         case TCS_OPCODE_GET_INSTANCE_ID:
         case VEC4_TCS_OPCODE_URB_WRITE:
         case VEC4_TCS_OPCODE_SET_INPUT_URB_OFFSETS:
         case VEC4_TCS_OPCODE_SET_OUTPUT_URB_OFFSETS:
         case TCS_OPCODE_GET_PRIMITIVE_ID:
         case TCS_OPCODE_CREATE_BARRIER_HEADER:
         case TCS_OPCODE_SRC0_010_IS_ZERO:
         case TCS_OPCODE_RELEASE_INPUT:
         case TCS_OPCODE_THREAD_END:
         case TES_OPCODE_CREATE_INPUT_READ_HEADER:
         case TES_OPCODE_ADD_INDIRECT_URB_OFFSET:
         case TES_OPCODE_GET_PRIMITIVE_ID:
            case RT_OPCODE_TRACE_RAY_LOGICAL:
            case SHADER_OPCODE_RND_MODE:
         case SHADER_OPCODE_FLOAT_CONTROL_MODE:
         case SHADER_OPCODE_BTD_SPAWN_LOGICAL:
         case SHADER_OPCODE_BTD_RETIRE_LOGICAL:
         case SHADER_OPCODE_READ_SR_REG:
                     }
      bool
   brw_saturate_immediate(enum brw_reg_type type, struct brw_reg *reg)
   {
      union {
      unsigned ud;
   int d;
   float f;
                        /* We want to either do a 32-bit or 64-bit data copy, the type is otherwise
   * irrelevant, so just check the size of the type and copy from/to an
   * appropriately sized field.
   */
   if (size < 8)
         else
            switch (type) {
   case BRW_REGISTER_TYPE_UD:
   case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_UW:
   case BRW_REGISTER_TYPE_W:
   case BRW_REGISTER_TYPE_UQ:
   case BRW_REGISTER_TYPE_Q:
      /* Nothing to do. */
      case BRW_REGISTER_TYPE_F:
      sat_imm.f = SATURATE(imm.f);
      case BRW_REGISTER_TYPE_DF:
      sat_imm.df = SATURATE(imm.df);
      case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_B:
         case BRW_REGISTER_TYPE_V:
   case BRW_REGISTER_TYPE_UV:
   case BRW_REGISTER_TYPE_VF:
         case BRW_REGISTER_TYPE_HF:
         case BRW_REGISTER_TYPE_NF:
                  if (size < 8) {
      if (imm.ud != sat_imm.ud) {
      reg->ud = sat_imm.ud;
         } else {
      if (imm.df != sat_imm.df) {
      reg->df = sat_imm.df;
         }
      }
      bool
   brw_negate_immediate(enum brw_reg_type type, struct brw_reg *reg)
   {
      switch (type) {
   case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_UD:
      reg->d = -reg->d;
      case BRW_REGISTER_TYPE_W:
   case BRW_REGISTER_TYPE_UW: {
      uint16_t value = -(int16_t)reg->ud;
   reg->ud = value | (uint32_t)value << 16;
      }
   case BRW_REGISTER_TYPE_F:
      reg->f = -reg->f;
      case BRW_REGISTER_TYPE_VF:
      reg->ud ^= 0x80808080;
      case BRW_REGISTER_TYPE_DF:
      reg->df = -reg->df;
      case BRW_REGISTER_TYPE_UQ:
   case BRW_REGISTER_TYPE_Q:
      reg->d64 = -reg->d64;
      case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_B:
         case BRW_REGISTER_TYPE_UV:
   case BRW_REGISTER_TYPE_V:
         case BRW_REGISTER_TYPE_HF:
      reg->ud ^= 0x80008000;
      case BRW_REGISTER_TYPE_NF:
                     }
      bool
   brw_abs_immediate(enum brw_reg_type type, struct brw_reg *reg)
   {
      switch (type) {
   case BRW_REGISTER_TYPE_D:
      reg->d = abs(reg->d);
      case BRW_REGISTER_TYPE_W: {
      uint16_t value = abs((int16_t)reg->ud);
   reg->ud = value | (uint32_t)value << 16;
      }
   case BRW_REGISTER_TYPE_F:
      reg->f = fabsf(reg->f);
      case BRW_REGISTER_TYPE_DF:
      reg->df = fabs(reg->df);
      case BRW_REGISTER_TYPE_VF:
      reg->ud &= ~0x80808080;
      case BRW_REGISTER_TYPE_Q:
      reg->d64 = imaxabs(reg->d64);
      case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_B:
         case BRW_REGISTER_TYPE_UQ:
   case BRW_REGISTER_TYPE_UD:
   case BRW_REGISTER_TYPE_UW:
   case BRW_REGISTER_TYPE_UV:
      /* Presumably the absolute value modifier on an unsigned source is a
   * nop, but it would be nice to confirm.
   */
      case BRW_REGISTER_TYPE_V:
         case BRW_REGISTER_TYPE_HF:
      reg->ud &= ~0x80008000;
      case BRW_REGISTER_TYPE_NF:
                     }
      backend_shader::backend_shader(const struct brw_compiler *compiler,
                              : compiler(compiler),
   log_data(params->log_data),
   devinfo(compiler->devinfo),
   nir(shader),
   stage_prog_data(stage_prog_data),
   mem_ctx(params->mem_ctx),
   cfg(NULL), idom_analysis(this),
   stage(shader->info.stage),
      {
   }
      backend_shader::~backend_shader()
   {
   }
      bool
   backend_reg::equals(const backend_reg &r) const
   {
         }
      bool
   backend_reg::negative_equals(const backend_reg &r) const
   {
         }
      bool
   backend_reg::is_zero() const
   {
      if (file != IMM)
                     switch (type) {
   case BRW_REGISTER_TYPE_HF:
      assert((d & 0xffff) == ((d >> 16) & 0xffff));
      case BRW_REGISTER_TYPE_F:
         case BRW_REGISTER_TYPE_DF:
         case BRW_REGISTER_TYPE_W:
   case BRW_REGISTER_TYPE_UW:
      assert((d & 0xffff) == ((d >> 16) & 0xffff));
      case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_UD:
         case BRW_REGISTER_TYPE_UQ:
   case BRW_REGISTER_TYPE_Q:
         default:
            }
      bool
   backend_reg::is_one() const
   {
      if (file != IMM)
                     switch (type) {
   case BRW_REGISTER_TYPE_HF:
      assert((d & 0xffff) == ((d >> 16) & 0xffff));
      case BRW_REGISTER_TYPE_F:
         case BRW_REGISTER_TYPE_DF:
         case BRW_REGISTER_TYPE_W:
   case BRW_REGISTER_TYPE_UW:
      assert((d & 0xffff) == ((d >> 16) & 0xffff));
      case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_UD:
         case BRW_REGISTER_TYPE_UQ:
   case BRW_REGISTER_TYPE_Q:
         default:
            }
      bool
   backend_reg::is_negative_one() const
   {
      if (file != IMM)
                     switch (type) {
   case BRW_REGISTER_TYPE_HF:
      assert((d & 0xffff) == ((d >> 16) & 0xffff));
      case BRW_REGISTER_TYPE_F:
         case BRW_REGISTER_TYPE_DF:
         case BRW_REGISTER_TYPE_W:
      assert((d & 0xffff) == ((d >> 16) & 0xffff));
      case BRW_REGISTER_TYPE_D:
         case BRW_REGISTER_TYPE_Q:
         default:
            }
      bool
   backend_reg::is_null() const
   {
         }
         bool
   backend_reg::is_accumulator() const
   {
         }
      bool
   backend_instruction::is_commutative() const
   {
      switch (opcode) {
   case BRW_OPCODE_AND:
   case BRW_OPCODE_OR:
   case BRW_OPCODE_XOR:
   case BRW_OPCODE_ADD:
   case BRW_OPCODE_ADD3:
   case BRW_OPCODE_MUL:
   case SHADER_OPCODE_MULH:
         case BRW_OPCODE_SEL:
      /* MIN and MAX are commutative. */
   if (conditional_mod == BRW_CONDITIONAL_GE ||
      conditional_mod == BRW_CONDITIONAL_L) {
      }
      default:
            }
      bool
   backend_instruction::is_3src(const struct brw_compiler *compiler) const
   {
         }
      bool
   backend_instruction::is_tex() const
   {
      return (opcode == SHADER_OPCODE_TEX ||
         opcode == FS_OPCODE_TXB ||
   opcode == SHADER_OPCODE_TXD ||
   opcode == SHADER_OPCODE_TXF ||
   opcode == SHADER_OPCODE_TXF_LZ ||
   opcode == SHADER_OPCODE_TXF_CMS ||
   opcode == SHADER_OPCODE_TXF_CMS_W ||
   opcode == SHADER_OPCODE_TXF_UMS ||
   opcode == SHADER_OPCODE_TXF_MCS ||
   opcode == SHADER_OPCODE_TXL ||
   opcode == SHADER_OPCODE_TXL_LZ ||
   opcode == SHADER_OPCODE_TXS ||
   opcode == SHADER_OPCODE_LOD ||
   opcode == SHADER_OPCODE_TG4 ||
      }
      bool
   backend_instruction::is_math() const
   {
      return (opcode == SHADER_OPCODE_RCP ||
         opcode == SHADER_OPCODE_RSQ ||
   opcode == SHADER_OPCODE_SQRT ||
   opcode == SHADER_OPCODE_EXP2 ||
   opcode == SHADER_OPCODE_LOG2 ||
   opcode == SHADER_OPCODE_SIN ||
   opcode == SHADER_OPCODE_COS ||
   opcode == SHADER_OPCODE_INT_QUOTIENT ||
      }
      bool
   backend_instruction::is_control_flow_begin() const
   {
      switch (opcode) {
   case BRW_OPCODE_DO:
   case BRW_OPCODE_IF:
   case BRW_OPCODE_ELSE:
         default:
            }
      bool
   backend_instruction::is_control_flow_end() const
   {
      switch (opcode) {
   case BRW_OPCODE_ELSE:
   case BRW_OPCODE_WHILE:
   case BRW_OPCODE_ENDIF:
         default:
            }
      bool
   backend_instruction::is_control_flow() const
   {
      switch (opcode) {
   case BRW_OPCODE_DO:
   case BRW_OPCODE_WHILE:
   case BRW_OPCODE_IF:
   case BRW_OPCODE_ELSE:
   case BRW_OPCODE_ENDIF:
   case BRW_OPCODE_BREAK:
   case BRW_OPCODE_CONTINUE:
         default:
            }
      bool
   backend_instruction::uses_indirect_addressing() const
   {
      switch (opcode) {
   case SHADER_OPCODE_BROADCAST:
   case SHADER_OPCODE_CLUSTER_BROADCAST:
   case SHADER_OPCODE_MOV_INDIRECT:
         default:
            }
      bool
   backend_instruction::can_do_source_mods() const
   {
      switch (opcode) {
   case BRW_OPCODE_ADDC:
   case BRW_OPCODE_BFE:
   case BRW_OPCODE_BFI1:
   case BRW_OPCODE_BFI2:
   case BRW_OPCODE_BFREV:
   case BRW_OPCODE_CBIT:
   case BRW_OPCODE_FBH:
   case BRW_OPCODE_FBL:
   case BRW_OPCODE_ROL:
   case BRW_OPCODE_ROR:
   case BRW_OPCODE_SUBB:
   case BRW_OPCODE_DP4A:
   case SHADER_OPCODE_BROADCAST:
   case SHADER_OPCODE_CLUSTER_BROADCAST:
   case SHADER_OPCODE_MOV_INDIRECT:
   case SHADER_OPCODE_SHUFFLE:
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
         default:
            }
      bool
   backend_instruction::can_do_saturate() const
   {
      switch (opcode) {
   case BRW_OPCODE_ADD:
   case BRW_OPCODE_ADD3:
   case BRW_OPCODE_ASR:
   case BRW_OPCODE_AVG:
   case BRW_OPCODE_CSEL:
   case BRW_OPCODE_DP2:
   case BRW_OPCODE_DP3:
   case BRW_OPCODE_DP4:
   case BRW_OPCODE_DPH:
   case BRW_OPCODE_DP4A:
   case BRW_OPCODE_F16TO32:
   case BRW_OPCODE_F32TO16:
   case BRW_OPCODE_LINE:
   case BRW_OPCODE_LRP:
   case BRW_OPCODE_MAC:
   case BRW_OPCODE_MAD:
   case BRW_OPCODE_MATH:
   case BRW_OPCODE_MOV:
   case BRW_OPCODE_MUL:
   case SHADER_OPCODE_MULH:
   case BRW_OPCODE_PLN:
   case BRW_OPCODE_RNDD:
   case BRW_OPCODE_RNDE:
   case BRW_OPCODE_RNDU:
   case BRW_OPCODE_RNDZ:
   case BRW_OPCODE_SEL:
   case BRW_OPCODE_SHL:
   case BRW_OPCODE_SHR:
   case FS_OPCODE_LINTERP:
   case SHADER_OPCODE_COS:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_POW:
   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_SQRT:
         default:
            }
      bool
   backend_instruction::can_do_cmod() const
   {
      switch (opcode) {
   case BRW_OPCODE_ADD:
   case BRW_OPCODE_ADD3:
   case BRW_OPCODE_ADDC:
   case BRW_OPCODE_AND:
   case BRW_OPCODE_ASR:
   case BRW_OPCODE_AVG:
   case BRW_OPCODE_CMP:
   case BRW_OPCODE_CMPN:
   case BRW_OPCODE_DP2:
   case BRW_OPCODE_DP3:
   case BRW_OPCODE_DP4:
   case BRW_OPCODE_DPH:
   case BRW_OPCODE_F16TO32:
   case BRW_OPCODE_F32TO16:
   case BRW_OPCODE_FRC:
   case BRW_OPCODE_LINE:
   case BRW_OPCODE_LRP:
   case BRW_OPCODE_LZD:
   case BRW_OPCODE_MAC:
   case BRW_OPCODE_MACH:
   case BRW_OPCODE_MAD:
   case BRW_OPCODE_MOV:
   case BRW_OPCODE_MUL:
   case BRW_OPCODE_NOT:
   case BRW_OPCODE_OR:
   case BRW_OPCODE_PLN:
   case BRW_OPCODE_RNDD:
   case BRW_OPCODE_RNDE:
   case BRW_OPCODE_RNDU:
   case BRW_OPCODE_RNDZ:
   case BRW_OPCODE_SAD2:
   case BRW_OPCODE_SADA2:
   case BRW_OPCODE_SHL:
   case BRW_OPCODE_SHR:
   case BRW_OPCODE_SUBB:
   case BRW_OPCODE_XOR:
   case FS_OPCODE_LINTERP:
         default:
            }
      bool
   backend_instruction::reads_accumulator_implicitly() const
   {
      switch (opcode) {
   case BRW_OPCODE_MAC:
   case BRW_OPCODE_MACH:
   case BRW_OPCODE_SADA2:
         default:
            }
      bool
   backend_instruction::writes_accumulator_implicitly(const struct intel_device_info *devinfo) const
   {
      return writes_accumulator ||
         (devinfo->ver < 6 &&
   ((opcode >= BRW_OPCODE_ADD && opcode < BRW_OPCODE_NOP) ||
         (opcode == FS_OPCODE_LINTERP &&
      }
      bool
   backend_instruction::has_side_effects() const
   {
      switch (opcode) {
   case SHADER_OPCODE_SEND:
            case BRW_OPCODE_SYNC:
   case VEC4_OPCODE_UNTYPED_ATOMIC:
   case SHADER_OPCODE_UNTYPED_ATOMIC_LOGICAL:
   case SHADER_OPCODE_GFX4_SCRATCH_WRITE:
   case VEC4_OPCODE_UNTYPED_SURFACE_WRITE:
   case SHADER_OPCODE_UNTYPED_SURFACE_WRITE_LOGICAL:
   case SHADER_OPCODE_A64_UNTYPED_WRITE_LOGICAL:
   case SHADER_OPCODE_A64_BYTE_SCATTERED_WRITE_LOGICAL:
   case SHADER_OPCODE_A64_UNTYPED_ATOMIC_LOGICAL:
   case SHADER_OPCODE_BYTE_SCATTERED_WRITE_LOGICAL:
   case SHADER_OPCODE_DWORD_SCATTERED_WRITE_LOGICAL:
   case SHADER_OPCODE_TYPED_ATOMIC_LOGICAL:
   case SHADER_OPCODE_TYPED_SURFACE_WRITE_LOGICAL:
   case SHADER_OPCODE_MEMORY_FENCE:
   case SHADER_OPCODE_INTERLOCK:
   case SHADER_OPCODE_URB_WRITE_LOGICAL:
   case FS_OPCODE_FB_WRITE:
   case FS_OPCODE_FB_WRITE_LOGICAL:
   case FS_OPCODE_REP_FB_WRITE:
   case SHADER_OPCODE_BARRIER:
   case VEC4_TCS_OPCODE_URB_WRITE:
   case TCS_OPCODE_RELEASE_INPUT:
   case SHADER_OPCODE_RND_MODE:
   case SHADER_OPCODE_FLOAT_CONTROL_MODE:
   case FS_OPCODE_SCHEDULING_FENCE:
   case SHADER_OPCODE_OWORD_BLOCK_WRITE_LOGICAL:
   case SHADER_OPCODE_A64_OWORD_BLOCK_WRITE_LOGICAL:
   case SHADER_OPCODE_BTD_SPAWN_LOGICAL:
   case SHADER_OPCODE_BTD_RETIRE_LOGICAL:
   case RT_OPCODE_TRACE_RAY_LOGICAL:
   case VEC4_OPCODE_ZERO_OOB_PUSH_REGS:
         default:
            }
      bool
   backend_instruction::is_volatile() const
   {
      switch (opcode) {
   case SHADER_OPCODE_SEND:
            case VEC4_OPCODE_UNTYPED_SURFACE_READ:
   case SHADER_OPCODE_UNTYPED_SURFACE_READ_LOGICAL:
   case SHADER_OPCODE_TYPED_SURFACE_READ_LOGICAL:
   case SHADER_OPCODE_BYTE_SCATTERED_READ_LOGICAL:
   case SHADER_OPCODE_DWORD_SCATTERED_READ_LOGICAL:
   case SHADER_OPCODE_A64_UNTYPED_READ_LOGICAL:
   case SHADER_OPCODE_A64_BYTE_SCATTERED_READ_LOGICAL:
   case VEC4_OPCODE_URB_READ:
         default:
            }
      #ifndef NDEBUG
   static bool
   inst_is_in_block(const bblock_t *block, const backend_instruction *inst)
   {
               /* Find the tail sentinel. If the tail sentinel is the sentinel from the
   * list header in the bblock_t, then this instruction is in that basic
   * block.
   */
   while (!n->is_tail_sentinel())
               }
   #endif
      static void
   adjust_later_block_ips(bblock_t *start_block, int ip_adjustment)
   {
      for (bblock_t *block_iter = start_block->next();
      block_iter;
   block_iter = block_iter->next()) {
   block_iter->start_ip += ip_adjustment;
         }
      void
   backend_instruction::insert_after(bblock_t *block, backend_instruction *inst)
   {
      assert(this != inst);
            if (!this->is_head_sentinel())
                                 }
      void
   backend_instruction::insert_before(bblock_t *block, backend_instruction *inst)
   {
      assert(this != inst);
            if (!this->is_tail_sentinel())
                                 }
      void
   backend_instruction::remove(bblock_t *block, bool defer_later_block_ip_updates)
   {
               if (defer_later_block_ip_updates) {
         } else {
      assert(block->end_ip_delta == 0);
               if (block->start_ip == block->end_ip) {
      if (block->end_ip_delta != 0) {
      adjust_later_block_ips(block, block->end_ip_delta);
                  } else {
                     }
      void
   backend_shader::dump_instructions(const char *name) const
   {
      FILE *file = stderr;
   if (name && __normal_user()) {
      file = fopen(name, "w");
   if (!file)
                        if (file != stderr) {
            }
      void
   backend_shader::dump_instructions_to_file(FILE *file) const
   {
      if (cfg) {
      int ip = 0;
   foreach_block_and_inst(block, backend_instruction, inst, cfg) {
      if (!INTEL_DEBUG(DEBUG_OPTIMIZER))
               } else {
      int ip = 0;
   foreach_in_list(backend_instruction, inst, &instructions) {
      if (!INTEL_DEBUG(DEBUG_OPTIMIZER))
                  }
      void
   backend_shader::calculate_cfg()
   {
      if (this->cfg)
            }
      void
   backend_shader::invalidate_analysis(brw::analysis_dependency_class c)
   {
         }
      extern "C" const unsigned *
   brw_compile_tes(const struct brw_compiler *compiler,
         {
      const struct intel_device_info *devinfo = compiler->devinfo;
   nir_shader *nir = params->base.nir;
   const struct brw_tes_prog_key *key = params->key;
   const struct brw_vue_map *input_vue_map = params->input_vue_map;
            const bool is_scalar = compiler->scalar_stage[MESA_SHADER_TESS_EVAL];
   const bool debug_enabled = brw_should_print_shader(nir, DEBUG_TES);
            prog_data->base.base.stage = MESA_SHADER_TESS_EVAL;
            nir->info.inputs_read = key->inputs_read;
            brw_nir_apply_key(nir, compiler, &key->base, 8);
   brw_nir_lower_tes_inputs(nir, input_vue_map);
   brw_nir_lower_vue_outputs(nir);
   brw_postprocess_nir(nir, compiler, debug_enabled,
            brw_compute_vue_map(devinfo, &prog_data->base.vue_map,
                           assert(output_size_bytes >= 1);
   if (output_size_bytes > GFX7_MAX_DS_URB_ENTRY_SIZE_BYTES) {
      params->base.error_str = ralloc_strdup(params->base.mem_ctx,
                     prog_data->base.clip_distance_mask =
         prog_data->base.cull_distance_mask =
      ((1 << nir->info.cull_distance_array_size) - 1) <<
         prog_data->include_primitive_id =
            /* URB entry sizes are stored as a multiple of 64 bytes. */
                     STATIC_ASSERT(BRW_TESS_PARTITIONING_INTEGER == TESS_SPACING_EQUAL - 1);
   STATIC_ASSERT(BRW_TESS_PARTITIONING_ODD_FRACTIONAL ==
         STATIC_ASSERT(BRW_TESS_PARTITIONING_EVEN_FRACTIONAL ==
            prog_data->partitioning =
            switch (nir->info.tess._primitive_mode) {
   case TESS_PRIMITIVE_QUADS:
      prog_data->domain = BRW_TESS_DOMAIN_QUAD;
      case TESS_PRIMITIVE_TRIANGLES:
      prog_data->domain = BRW_TESS_DOMAIN_TRI;
      case TESS_PRIMITIVE_ISOLINES:
      prog_data->domain = BRW_TESS_DOMAIN_ISOLINE;
      default:
                  if (nir->info.tess.point_mode) {
         } else if (nir->info.tess._primitive_mode == TESS_PRIMITIVE_ISOLINES) {
         } else {
      /* Hardware winding order is backwards from OpenGL */
   prog_data->output_topology =
      nir->info.tess.ccw ? BRW_TESS_OUTPUT_TOPOLOGY_TRI_CW
            if (unlikely(debug_enabled)) {
      fprintf(stderr, "TES Input ");
   brw_print_vue_map(stderr, input_vue_map, MESA_SHADER_TESS_EVAL);
   fprintf(stderr, "TES Output ");
   brw_print_vue_map(stderr, &prog_data->base.vue_map,
               if (is_scalar) {
      fs_visitor v(compiler, &params->base, &key->base,
               if (!v.run_tes()) {
      params->base.error_str =
                     assert(v.payload().num_regs % reg_unit(devinfo) == 0);
                     fs_generator g(compiler, &params->base,
         if (unlikely(debug_enabled)) {
      g.enable_debug(ralloc_asprintf(params->base.mem_ctx,
                                 g.generate_code(v.cfg, 8, v.shader_stats,
                        } else {
      brw::vec4_tes_visitor v(compiler, &params->base, key, prog_data,
         if (!v.run()) {
         return NULL;
                  v.dump_instructions();
            assembly = brw_vec4_generate_assembly(compiler, &params->base, nir,
                              }
