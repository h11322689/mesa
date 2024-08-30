   /**************************************************************************
   *
   * Copyright 2012-2021 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
      /*
   * ShaderTGSI.c --
   *    Functions for translating shaders.
   */
      #include "Debug.h"
   #include "ShaderParse.h"
      #include "pipe/p_state.h"
   #include "tgsi/tgsi_ureg.h"
   #include "tgsi/tgsi_dump.h"
   #include "util/u_memory.h"
      #include "ShaderDump.h"
         enum dx10_opcode_format {
      OF_FLOAT,
   OF_INT,
      };
      struct dx10_opcode_xlate {
      D3D10_SB_OPCODE_TYPE type;
   enum dx10_opcode_format format;
      };
      /* Opcodes that we have not even attempted to implement:
   */
   #define TGSI_LOG_UNSUPPORTED TGSI_OPCODE_LAST
      /* Opcodes which do not translate directly to a TGSI opcode, but which
   * have at least a partial implemention coded below:
   */
   #define TGSI_EXPAND          (TGSI_OPCODE_LAST+1)
      static struct dx10_opcode_xlate opcode_xlate[D3D10_SB_NUM_OPCODES] = {
      {D3D10_SB_OPCODE_ADD,                              OF_FLOAT, TGSI_OPCODE_ADD},
   {D3D10_SB_OPCODE_AND,                              OF_UINT,  TGSI_OPCODE_AND},
   {D3D10_SB_OPCODE_BREAK,                            OF_FLOAT, TGSI_OPCODE_BRK},
   {D3D10_SB_OPCODE_BREAKC,                           OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_CALL,                             OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_CALLC,                            OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_CASE,                             OF_UINT,  TGSI_OPCODE_CASE},
   {D3D10_SB_OPCODE_CONTINUE,                         OF_FLOAT, TGSI_OPCODE_CONT},
   {D3D10_SB_OPCODE_CONTINUEC,                        OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_CUT,                              OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DEFAULT,                          OF_FLOAT, TGSI_OPCODE_DEFAULT},
   {D3D10_SB_OPCODE_DERIV_RTX,                        OF_FLOAT, TGSI_OPCODE_DDX},
   {D3D10_SB_OPCODE_DERIV_RTY,                        OF_FLOAT, TGSI_OPCODE_DDY},
   {D3D10_SB_OPCODE_DISCARD,                          OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_DIV,                              OF_FLOAT, TGSI_OPCODE_DIV},
   {D3D10_SB_OPCODE_DP2,                              OF_FLOAT, TGSI_OPCODE_DP2},
   {D3D10_SB_OPCODE_DP3,                              OF_FLOAT, TGSI_OPCODE_DP3},
   {D3D10_SB_OPCODE_DP4,                              OF_FLOAT, TGSI_OPCODE_DP4},
   {D3D10_SB_OPCODE_ELSE,                             OF_FLOAT, TGSI_OPCODE_ELSE},
   {D3D10_SB_OPCODE_EMIT,                             OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_EMITTHENCUT,                      OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_ENDIF,                            OF_FLOAT, TGSI_OPCODE_ENDIF},
   {D3D10_SB_OPCODE_ENDLOOP,                          OF_FLOAT, TGSI_OPCODE_ENDLOOP},
   {D3D10_SB_OPCODE_ENDSWITCH,                        OF_FLOAT, TGSI_OPCODE_ENDSWITCH},
   {D3D10_SB_OPCODE_EQ,                               OF_FLOAT, TGSI_OPCODE_FSEQ},
   {D3D10_SB_OPCODE_EXP,                              OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_FRC,                              OF_FLOAT, TGSI_OPCODE_FRC},
   {D3D10_SB_OPCODE_FTOI,                             OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_FTOU,                             OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_GE,                               OF_FLOAT, TGSI_OPCODE_FSGE},
   {D3D10_SB_OPCODE_IADD,                             OF_INT,   TGSI_OPCODE_UADD},
   {D3D10_SB_OPCODE_IF,                               OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_IEQ,                              OF_INT,   TGSI_OPCODE_USEQ},
   {D3D10_SB_OPCODE_IGE,                              OF_INT,   TGSI_OPCODE_ISGE},
   {D3D10_SB_OPCODE_ILT,                              OF_INT,   TGSI_OPCODE_ISLT},
   {D3D10_SB_OPCODE_IMAD,                             OF_INT,   TGSI_OPCODE_UMAD},
   {D3D10_SB_OPCODE_IMAX,                             OF_INT,   TGSI_OPCODE_IMAX},
   {D3D10_SB_OPCODE_IMIN,                             OF_INT,   TGSI_OPCODE_IMIN},
   {D3D10_SB_OPCODE_IMUL,                             OF_INT,   TGSI_EXPAND},
   {D3D10_SB_OPCODE_INE,                              OF_INT,   TGSI_OPCODE_USNE},
   {D3D10_SB_OPCODE_INEG,                             OF_INT,   TGSI_OPCODE_INEG},
   {D3D10_SB_OPCODE_ISHL,                             OF_INT,   TGSI_OPCODE_SHL},
   {D3D10_SB_OPCODE_ISHR,                             OF_INT,   TGSI_OPCODE_ISHR},
   {D3D10_SB_OPCODE_ITOF,                             OF_INT,   TGSI_OPCODE_I2F},
   {D3D10_SB_OPCODE_LABEL,                            OF_INT,   TGSI_EXPAND},
   {D3D10_SB_OPCODE_LD,                               OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_LD_MS,                            OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_LOG,                              OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_LOOP,                             OF_FLOAT, TGSI_OPCODE_BGNLOOP},
   {D3D10_SB_OPCODE_LT,                               OF_FLOAT, TGSI_OPCODE_FSLT},
   {D3D10_SB_OPCODE_MAD,                              OF_FLOAT, TGSI_OPCODE_MAD},
   {D3D10_SB_OPCODE_MIN,                              OF_FLOAT, TGSI_OPCODE_MIN},
   {D3D10_SB_OPCODE_MAX,                              OF_FLOAT, TGSI_OPCODE_MAX},
   {D3D10_SB_OPCODE_CUSTOMDATA,                       OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_MOV,                              OF_UINT,  TGSI_OPCODE_MOV},
   {D3D10_SB_OPCODE_MOVC,                             OF_UINT,  TGSI_OPCODE_UCMP},
   {D3D10_SB_OPCODE_MUL,                              OF_FLOAT, TGSI_OPCODE_MUL},
   {D3D10_SB_OPCODE_NE,                               OF_FLOAT, TGSI_OPCODE_FSNE},
   {D3D10_SB_OPCODE_NOP,                              OF_FLOAT, TGSI_OPCODE_NOP},
   {D3D10_SB_OPCODE_NOT,                              OF_UINT,  TGSI_OPCODE_NOT},
   {D3D10_SB_OPCODE_OR,                               OF_UINT,  TGSI_OPCODE_OR},
   {D3D10_SB_OPCODE_RESINFO,                          OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_RET,                              OF_FLOAT, TGSI_OPCODE_RET},
   {D3D10_SB_OPCODE_RETC,                             OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_ROUND_NE,                         OF_FLOAT, TGSI_OPCODE_ROUND},
   {D3D10_SB_OPCODE_ROUND_NI,                         OF_FLOAT, TGSI_OPCODE_FLR},
   {D3D10_SB_OPCODE_ROUND_PI,                         OF_FLOAT, TGSI_OPCODE_CEIL},
   {D3D10_SB_OPCODE_ROUND_Z,                          OF_FLOAT, TGSI_OPCODE_TRUNC},
   {D3D10_SB_OPCODE_RSQ,                              OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_SAMPLE,                           OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_SAMPLE_C,                         OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_SAMPLE_C_LZ,                      OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_SAMPLE_L,                         OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_SAMPLE_D,                         OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_SAMPLE_B,                         OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_SQRT,                             OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_SWITCH,                           OF_UINT,  TGSI_OPCODE_SWITCH},
   {D3D10_SB_OPCODE_SINCOS,                           OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_UDIV,                             OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_ULT,                              OF_UINT,  TGSI_OPCODE_USLT},
   {D3D10_SB_OPCODE_UGE,                              OF_UINT,  TGSI_OPCODE_USGE},
   {D3D10_SB_OPCODE_UMUL,                             OF_UINT,  TGSI_EXPAND},
   {D3D10_SB_OPCODE_UMAD,                             OF_UINT,  TGSI_OPCODE_UMAD},
   {D3D10_SB_OPCODE_UMAX,                             OF_UINT,  TGSI_OPCODE_UMAX},
   {D3D10_SB_OPCODE_UMIN,                             OF_UINT,  TGSI_OPCODE_UMIN},
   {D3D10_SB_OPCODE_USHR,                             OF_UINT,  TGSI_OPCODE_USHR},
   {D3D10_SB_OPCODE_UTOF,                             OF_UINT,  TGSI_OPCODE_U2F},
   {D3D10_SB_OPCODE_XOR,                              OF_UINT,  TGSI_OPCODE_XOR},
   {D3D10_SB_OPCODE_DCL_RESOURCE,                     OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_CONSTANT_BUFFER,              OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_SAMPLER,                      OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_INDEX_RANGE,                  OF_FLOAT, TGSI_LOG_UNSUPPORTED},
   {D3D10_SB_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY, OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_GS_INPUT_PRIMITIVE,           OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT,      OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_INPUT,                        OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_INPUT_SGV,                    OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_INPUT_SIV,                    OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_INPUT_PS,                     OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_INPUT_PS_SGV,                 OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_INPUT_PS_SIV,                 OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_OUTPUT,                       OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_OUTPUT_SGV,                   OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_OUTPUT_SIV,                   OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_TEMPS,                        OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_INDEXABLE_TEMP,               OF_FLOAT, TGSI_EXPAND},
   {D3D10_SB_OPCODE_DCL_GLOBAL_FLAGS,                 OF_FLOAT, TGSI_LOG_UNSUPPORTED},
   {D3D10_SB_OPCODE_RESERVED0,                        OF_FLOAT, TGSI_LOG_UNSUPPORTED},
   {D3D10_1_SB_OPCODE_LOD,                            OF_FLOAT, TGSI_LOG_UNSUPPORTED},
   {D3D10_1_SB_OPCODE_GATHER4,                        OF_FLOAT, TGSI_LOG_UNSUPPORTED},
   {D3D10_1_SB_OPCODE_SAMPLE_POS,                     OF_FLOAT, TGSI_LOG_UNSUPPORTED},
      };
      #define SHADER_MAX_TEMPS 4096
   #define SHADER_MAX_INPUTS 32
   #define SHADER_MAX_OUTPUTS 32
   #define SHADER_MAX_CONSTS 4096
   #define SHADER_MAX_RESOURCES PIPE_MAX_SHADER_SAMPLER_VIEWS
   #define SHADER_MAX_SAMPLERS PIPE_MAX_SAMPLERS
   #define SHADER_MAX_INDEXABLE_TEMPS 4096
      struct Shader_call {
      unsigned d3d_label;
      };
      struct Shader_label {
      unsigned d3d_label;
      };
      struct Shader_resource {
         };
      struct Shader_xlate {
               uint vertices_in;
            struct ureg_dst temps[SHADER_MAX_TEMPS];
   struct ureg_dst output_depth;
   struct Shader_resource resources[SHADER_MAX_RESOURCES];
   struct ureg_src sv[SHADER_MAX_RESOURCES];
   struct ureg_src samplers[SHADER_MAX_SAMPLERS];
   struct ureg_src imms;
            uint temp_offset;
            struct {
      bool declared;
   uint writemask;
   uint siv_name;
   bool overloaded;
               struct {
                  struct {
      uint d3d;
      } clip_distance_mapping[2], cull_distance_mapping[2];
   uint num_clip_distances_declared;
            struct Shader_call *calls;
   uint num_calls;
   uint max_calls;
   struct Shader_label *labels;
   uint num_labels;
      };
      static uint
   translate_interpolation(D3D10_SB_INTERPOLATION_MODE interpolation)
   {
      switch (interpolation) {
   case D3D10_SB_INTERPOLATION_UNDEFINED:
      assert(0);
         case D3D10_SB_INTERPOLATION_CONSTANT:
         case D3D10_SB_INTERPOLATION_LINEAR:
         case D3D10_SB_INTERPOLATION_LINEAR_NOPERSPECTIVE:
            case D3D10_SB_INTERPOLATION_LINEAR_CENTROID:
   case D3D10_SB_INTERPOLATION_LINEAR_SAMPLE: // DX10.1
      LOG_UNSUPPORTED(true);
         case D3D10_SB_INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID:
   case D3D10_SB_INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE: // DX10.1
      LOG_UNSUPPORTED(true);
               assert(0);
      }
      static uint
   translate_system_name(D3D10_SB_NAME name)
   {
      switch (name) {
   case D3D10_SB_NAME_UNDEFINED:
      assert(0);                /* should not happen */
      case D3D10_SB_NAME_POSITION:
         case D3D10_SB_NAME_CLIP_DISTANCE:
   case D3D10_SB_NAME_CULL_DISTANCE:
         case D3D10_SB_NAME_PRIMITIVE_ID:
         case D3D10_SB_NAME_INSTANCE_ID:
         case D3D10_SB_NAME_VERTEX_ID:
         case D3D10_SB_NAME_VIEWPORT_ARRAY_INDEX:
         case D3D10_SB_NAME_RENDER_TARGET_ARRAY_INDEX:
         case D3D10_SB_NAME_IS_FRONT_FACE:
         case D3D10_SB_NAME_SAMPLE_INDEX:
      LOG_UNSUPPORTED(true);
               assert(0);
      }
      static uint
   translate_semantic_index(struct Shader_xlate *sx,
               {
      unsigned idx;
   switch (name) {
   case D3D10_SB_NAME_CLIP_DISTANCE:
   case D3D10_SB_NAME_CULL_DISTANCE:
      if (sx->clip_distance_mapping[0].d3d == operand->base.index[0].imm) {
         } else {
      assert(sx->clip_distance_mapping[1].d3d == operand->base.index[0].imm);
      }
   /*   case D3D10_SB_NAME_CULL_DISTANCE:
         if (sx->cull_distance_mapping[0].d3d == operand->base.index[0].imm) {
         } else {
      assert(sx->cull_distance_mapping[1].d3d == operand->base.index[0].imm);
      }
      default:
         }
      }
      static enum tgsi_return_type
   trans_dcl_ret_type(D3D10_SB_RESOURCE_RETURN_TYPE d3drettype) {
      switch (d3drettype) {
   case D3D10_SB_RETURN_TYPE_UNORM:
         case D3D10_SB_RETURN_TYPE_SNORM:
         case D3D10_SB_RETURN_TYPE_SINT:
         case D3D10_SB_RETURN_TYPE_UINT:
         case D3D10_SB_RETURN_TYPE_FLOAT:
         case D3D10_SB_RETURN_TYPE_MIXED:
   default:
      LOG_UNSUPPORTED(true);
         }
      static void
   declare_vertices_in(struct Shader_xlate *sx,
         {
      /* Make sure vertices_in is consistent with input primitive
   * and other input declarations.
   */
   if (sx->vertices_in) {
         } else {
            }
      struct swizzle_mapping {
      unsigned x;
   unsigned y;
   unsigned z;
      };
      /* mapping of writmask to swizzles */
   static const struct swizzle_mapping writemask_to_swizzle[] = {
      { TGSI_SWIZZLE_X, TGSI_SWIZZLE_X, TGSI_SWIZZLE_X, TGSI_SWIZZLE_X }, //TGSI_WRITEMASK_NONE
   { TGSI_SWIZZLE_X, TGSI_SWIZZLE_X, TGSI_SWIZZLE_X, TGSI_SWIZZLE_X }, //TGSI_WRITEMASK_X
   { TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Y }, //TGSI_WRITEMASK_Y
   { TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y }, //TGSI_WRITEMASK_XY
   { TGSI_SWIZZLE_Z, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_Z }, //TGSI_WRITEMASK_Z
   { TGSI_SWIZZLE_X, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_X, TGSI_SWIZZLE_Z }, //TGSI_WRITEMASK_XZ
   { TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Z }, //TGSI_WRITEMASK_YZ
   { TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_X }, //TGSI_WRITEMASK_XYZ
   { TGSI_SWIZZLE_W, TGSI_SWIZZLE_W, TGSI_SWIZZLE_W, TGSI_SWIZZLE_W }, //TGSI_WRITEMASK_W
   { TGSI_SWIZZLE_X, TGSI_SWIZZLE_W, TGSI_SWIZZLE_X, TGSI_SWIZZLE_W }, //TGSI_WRITEMASK_XW
   { TGSI_SWIZZLE_Y, TGSI_SWIZZLE_W, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_W }, //TGSI_WRITEMASK_YW
   { TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_W, TGSI_SWIZZLE_W }, //TGSI_WRITEMASK_XYW
   { TGSI_SWIZZLE_Z, TGSI_SWIZZLE_W, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_W }, //TGSI_WRITEMASK_ZW
   { TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_W }, //TGSI_WRITEMASK_XZW
   { TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_W }, //TGSI_WRITEMASK_YZW
      };
      static struct ureg_src
   swizzle_reg(struct ureg_src src, uint writemask,
         {
      switch (siv_name) {
   case D3D10_SB_NAME_PRIMITIVE_ID:
   case D3D10_SB_NAME_INSTANCE_ID:
   case D3D10_SB_NAME_VERTEX_ID:
   case D3D10_SB_NAME_VIEWPORT_ARRAY_INDEX:
   case D3D10_SB_NAME_RENDER_TARGET_ARRAY_INDEX:
   case D3D10_SB_NAME_IS_FRONT_FACE:
         default: {
      const struct swizzle_mapping *swizzle =
         return ureg_swizzle(src, swizzle->x, swizzle->y,
      }
      }
      static void
   dcl_base_output(struct Shader_xlate *sx,
                     {
      unsigned writemask =
         unsigned idx = operand->base.index[0].imm;
            if (!writemask) {
      sx->outputs[idx].reg[0] = reg;
   sx->outputs[idx].reg[1] = reg;
   sx->outputs[idx].reg[2] = reg;
   sx->outputs[idx].reg[3] = reg;
               for (i = 0; i < 4; ++i) {
      unsigned mask = 1 << i;
   if ((writemask & mask)) {
               }
      static void
   dcl_base_input(struct Shader_xlate *sx,
                  struct ureg_program *ureg,
   const struct Shader_dst_operand *operand,
      {
      unsigned writemask =
            if (sx->inputs[index].declared && !sx->inputs[index].overloaded) {
               ureg_MOV(ureg,
            ureg_writemask(temp, sx->inputs[index].writemask),
      ureg_MOV(ureg, ureg_writemask(temp, writemask),
         sx->inputs[index].reg = ureg_src(temp);
   sx->inputs[index].overloaded = true;
      } else if (sx->inputs[index].overloaded) {
      struct ureg_dst temp = ureg_dst(sx->inputs[index].reg);
   ureg_MOV(ureg, ureg_writemask(temp, writemask),
            } else {
               sx->inputs[index].reg = dcl_reg;
   sx->inputs[index].declared = true;
   sx->inputs[index].writemask = writemask;
         }
      static void
   dcl_vs_input(struct Shader_xlate *sx,
               {
      struct ureg_src reg;
   assert(dst->base.index_dim == 1);
                     dcl_base_input(sx, ureg, dst, reg, dst->base.index[0].imm,
      }
      static void
   dcl_gs_input(struct Shader_xlate *sx,
               {
      if (dst->base.index_dim == 2) {
                        /* XXX: Implement declaration masks in gallium.
   */
   if (!sx->inputs[dst->base.index[1].imm].reg.File) {
      struct ureg_src reg =
      ureg_DECL_input(ureg,
                  dcl_base_input(sx, ureg, dst, reg, dst->base.index[1].imm,
         } else {
      assert(dst->base.type == D3D10_SB_OPERAND_TYPE_INPUT_PRIMITIVEID);
                  }
      static void
   dcl_sgv_input(struct Shader_xlate *sx,
               struct ureg_program *ureg,
   {
      struct ureg_src reg;
   assert(dst->base.index_dim == 1);
                     dcl_base_input(sx, ureg, dst, reg, dst->base.index[0].imm,
      }
      static void
   dcl_siv_input(struct Shader_xlate *sx,
               struct ureg_program *ureg,
   {
      struct ureg_src reg;
   assert(dst->base.index_dim == 2);
                     reg = ureg_DECL_input(ureg,
                  dcl_base_input(sx, ureg, dst, reg, dst->base.index[1].imm,
      }
      static void
   dcl_ps_input(struct Shader_xlate *sx,
               struct ureg_program *ureg,
   {
      struct ureg_src reg;
   assert(dst->base.index_dim == 1);
            reg = ureg_DECL_fs_input(ureg,
                        dcl_base_input(sx, ureg, dst, reg, dst->base.index[0].imm,
      }
      static void
   dcl_ps_sgv_input(struct Shader_xlate *sx,
                     {
      struct ureg_src reg;
   assert(dst->base.index_dim == 1);
            if (dcl_siv_name == D3D10_SB_NAME_POSITION) {
      ureg_property(ureg,
               ureg_property(ureg,
                     reg = ureg_DECL_fs_input(ureg,
                        if (dcl_siv_name == D3D10_SB_NAME_IS_FRONT_FACE) {
      /* We need to map gallium's front_face to the one expected
   * by D3D10 */
                     ureg_CMP(ureg, tmp, reg,
                        dcl_base_input(sx, ureg, dst, reg, dst->base.index[0].imm,
      }
      static void
   dcl_ps_siv_input(struct Shader_xlate *sx,
                     {
      struct ureg_src reg;
   assert(dst->base.index_dim == 1);
            reg = ureg_DECL_fs_input(ureg,
                        if (dcl_siv_name == D3D10_SB_NAME_POSITION) {
      /* D3D10 expects reciprocal of interpolated 1/w as 4th component,
   * gallium/GL just interpolated 1/w */
            ureg_MOV(ureg, tmp, reg);
   ureg_RCP(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_W),
                     dcl_base_input(sx, ureg, dst, reg, dst->base.index[0].imm,
      }
      static struct ureg_src
   translate_relative_operand(struct Shader_xlate *sx,
         {
               switch (operand->type) {
   case D3D10_SB_OPERAND_TYPE_TEMP:
               reg = ureg_src(sx->temps[sx->temp_offset + operand->index[0].imm]);
         case D3D10_SB_OPERAND_TYPE_INPUT_PRIMITIVEID:
      reg = sx->prim_id;
         case D3D10_SB_OPERAND_TYPE_INDEXABLE_TEMP:
               reg = ureg_src(sx->temps[sx->indexable_temp_offsets[operand->index[0].imm] +
               case D3D10_SB_OPERAND_TYPE_INPUT:
   case D3D10_SB_OPERAND_TYPE_OUTPUT:
   case D3D10_SB_OPERAND_TYPE_IMMEDIATE32:
   case D3D10_SB_OPERAND_TYPE_IMMEDIATE64:
   case D3D10_SB_OPERAND_TYPE_SAMPLER:
   case D3D10_SB_OPERAND_TYPE_RESOURCE:
   case D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER:
   case D3D10_SB_OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER:
   case D3D10_SB_OPERAND_TYPE_LABEL:
   case D3D10_SB_OPERAND_TYPE_OUTPUT_DEPTH:
   case D3D10_SB_OPERAND_TYPE_NULL:
   case D3D10_SB_OPERAND_TYPE_RASTERIZER:
   case D3D10_SB_OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
      LOG_UNSUPPORTED(true);
   reg = ureg_src(ureg_DECL_temporary(sx->ureg));
         default:
      assert(0);                /* should never happen */
               reg = ureg_scalar(reg, operand->comp);
      }
      static struct ureg_dst
   translate_operand(struct Shader_xlate *sx,
               {
               switch (operand->type) {
   case D3D10_SB_OPERAND_TYPE_TEMP:
      assert(operand->index_dim == 1);
   assert(operand->index[0].index_rep == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            reg = sx->temps[sx->temp_offset + operand->index[0].imm];
         case D3D10_SB_OPERAND_TYPE_OUTPUT:
      assert(operand->index_dim == 1);
            if (operand->index[0].index_rep == D3D10_SB_OPERAND_INDEX_IMMEDIATE32) {
      if (!writemask) {
         } else {
      unsigned i;
   for (i = 0; i < 4; ++i) {
      unsigned mask = 1 << i;
   if ((writemask & mask)) {
      reg = sx->outputs[operand->index[0].imm].reg[i];
               } else {
      struct ureg_src addr =
         assert(operand->index[0].index_rep == D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE);
      }
         case D3D10_SB_OPERAND_TYPE_OUTPUT_DEPTH:
               reg = sx->output_depth;
         case D3D10_SB_OPERAND_TYPE_INPUT_PRIMITIVEID:
               reg = ureg_dst(sx->prim_id);
         case D3D10_SB_OPERAND_TYPE_INPUT:
   case D3D10_SB_OPERAND_TYPE_INDEXABLE_TEMP:
   case D3D10_SB_OPERAND_TYPE_IMMEDIATE32:
   case D3D10_SB_OPERAND_TYPE_IMMEDIATE64:
   case D3D10_SB_OPERAND_TYPE_SAMPLER:
   case D3D10_SB_OPERAND_TYPE_RESOURCE:
   case D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER:
   case D3D10_SB_OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER:
   case D3D10_SB_OPERAND_TYPE_LABEL:
   case D3D10_SB_OPERAND_TYPE_NULL:
   case D3D10_SB_OPERAND_TYPE_RASTERIZER:
   case D3D10_SB_OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
      /* XXX: Translate more operands types.
   */
   LOG_UNSUPPORTED(true);
                  }
      static struct ureg_src
   translate_indexable_temp(struct Shader_xlate *sx,
         {
      struct ureg_src reg;
   switch (operand->index[1].index_rep) {
   case D3D10_SB_OPERAND_INDEX_IMMEDIATE32:
      reg = ureg_src(
      sx->temps[sx->indexable_temp_offsets[operand->index[0].imm] +
         case D3D10_SB_OPERAND_INDEX_RELATIVE:
      reg = ureg_src_indirect(
      ureg_src(sx->temps[
         translate_relative_operand(sx,
         case D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
      reg = ureg_src_indirect(
      ureg_src(sx->temps[
               translate_relative_operand(sx,
         default:
      /* XXX: Other index representations.
   */
   LOG_UNSUPPORTED(true);
      }
      }
      static struct ureg_dst
   translate_dst_operand(struct Shader_xlate *sx,
               {
      struct ureg_dst reg;
   unsigned writemask =
            assert((D3D10_SB_OPERAND_4_COMPONENT_MASK_SHIFT) == 4);
   assert((D3D10_SB_OPERAND_4_COMPONENT_MASK_X >> 4) == TGSI_WRITEMASK_X);
   assert((D3D10_SB_OPERAND_4_COMPONENT_MASK_Y >> 4) == TGSI_WRITEMASK_Y);
   assert((D3D10_SB_OPERAND_4_COMPONENT_MASK_Z >> 4) == TGSI_WRITEMASK_Z);
            switch (operand->base.type) {
   case D3D10_SB_OPERAND_TYPE_INDEXABLE_TEMP:
      assert(operand->base.index_dim == 2);
   assert(operand->base.index[0].index_rep == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            reg = ureg_dst(translate_indexable_temp(sx, &operand->base));
         default:
                  /* oDepth often has an empty writemask */
   if (operand->base.type != D3D10_SB_OPERAND_TYPE_OUTPUT_DEPTH) {
                  if (saturate) {
                     }
      static struct ureg_src
   translate_src_operand(struct Shader_xlate *sx,
               {
               switch (operand->base.type) {
   case D3D10_SB_OPERAND_TYPE_INPUT:
      if (operand->base.index_dim == 1) {
      switch (operand->base.index[0].index_rep) {
   case D3D10_SB_OPERAND_INDEX_IMMEDIATE32:
      assert(operand->base.index[0].imm < SHADER_MAX_INPUTS);
   reg = sx->inputs[operand->base.index[0].imm].reg;
      case D3D10_SB_OPERAND_INDEX_RELATIVE: {
      struct ureg_src tmp =
            }
         case D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE: {
      struct ureg_src tmp =
            }
         default:
      /* XXX: Other index representations.
                  } else {
                     switch (operand->base.index[1].index_rep) {
   case D3D10_SB_OPERAND_INDEX_IMMEDIATE32:
      reg = sx->inputs[operand->base.index[1].imm].reg;
      case D3D10_SB_OPERAND_INDEX_RELATIVE: {
      struct ureg_src tmp =
            }
         case D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE: {
      struct ureg_src tmp =
            }
         default:
      /* XXX: Other index representations.
   */
               switch (operand->base.index[0].index_rep) {
   case D3D10_SB_OPERAND_INDEX_IMMEDIATE32:
      reg = ureg_src_dimension(reg, operand->base.index[0].imm);
      case D3D10_SB_OPERAND_INDEX_RELATIVE:{
      struct ureg_src tmp =
            }
         case D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE: {
      struct ureg_src tmp =
            }
         default:
      /* XXX: Other index representations.
   */
         }
         case D3D10_SB_OPERAND_TYPE_INDEXABLE_TEMP:
      assert(operand->base.index_dim == 2);
   assert(operand->base.index[0].index_rep == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            reg = translate_indexable_temp(sx, &operand->base);
         case D3D10_SB_OPERAND_TYPE_IMMEDIATE32:
      switch (format) {
   case OF_FLOAT:
      reg = ureg_imm4f(sx->ureg,
                  operand->imm[0].f32,
         case OF_INT:
      reg = ureg_imm4i(sx->ureg,
                  operand->imm[0].i32,
         case OF_UINT:
      reg = ureg_imm4u(sx->ureg,
                  operand->imm[0].u32,
         default:
      assert(0);
      }
         case D3D10_SB_OPERAND_TYPE_SAMPLER:
      assert(operand->base.index_dim == 1);
   assert(operand->base.index[0].index_rep == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            reg = sx->samplers[operand->base.index[0].imm];
         case D3D10_SB_OPERAND_TYPE_RESOURCE:
      assert(operand->base.index_dim == 1);
   assert(operand->base.index[0].index_rep == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            reg = sx->sv[operand->base.index[0].imm];
         case D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER:
               assert(operand->base.index[0].index_rep == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            switch (operand->base.index[1].index_rep) {
   case D3D10_SB_OPERAND_INDEX_IMMEDIATE32:
               reg = ureg_src_register(TGSI_FILE_CONSTANT, operand->base.index[1].imm);
   reg = ureg_src_dimension(reg, operand->base.index[0].imm);
      case D3D10_SB_OPERAND_INDEX_RELATIVE:
   case D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
      reg = ureg_src_register(TGSI_FILE_CONSTANT, operand->base.index[1].imm);
   reg = ureg_src_indirect(
      reg,
      reg = ureg_src_dimension(reg, operand->base.index[0].imm);
      default:
      /* XXX: Other index representations.
   */
                     case D3D10_SB_OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER:
               switch (operand->base.index[0].index_rep) {
   case D3D10_SB_OPERAND_INDEX_IMMEDIATE32:
      reg = sx->imms;
   reg.Index += operand->base.index[0].imm;
      case D3D10_SB_OPERAND_INDEX_RELATIVE:
   case D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
      reg = sx->imms;
   reg.Index += operand->base.index[0].imm;
   reg = ureg_src_indirect(
      sx->imms,
         default:
      /* XXX: Other index representations.
   */
      }
         case D3D10_SB_OPERAND_TYPE_INPUT_PRIMITIVEID:
      reg = sx->prim_id;
         default:
                  reg = ureg_swizzle(reg,
                              switch (operand->modifier) {
   case D3D10_SB_OPERAND_MODIFIER_NONE:
         case D3D10_SB_OPERAND_MODIFIER_NEG:
      reg = ureg_negate(reg);
      case D3D10_SB_OPERAND_MODIFIER_ABS:
      reg = ureg_abs(reg);
      case D3D10_SB_OPERAND_MODIFIER_ABSNEG:
      reg = ureg_negate(ureg_abs(reg));
      default:
                     }
      static uint
   translate_resource_dimension(D3D10_SB_RESOURCE_DIMENSION dim)
   {
      switch (dim) {
   case D3D10_SB_RESOURCE_DIMENSION_UNKNOWN:
         case D3D10_SB_RESOURCE_DIMENSION_BUFFER:
         case D3D10_SB_RESOURCE_DIMENSION_TEXTURE1D:
         case D3D10_SB_RESOURCE_DIMENSION_TEXTURE2D:
         case D3D10_SB_RESOURCE_DIMENSION_TEXTURE2DMS:
         case D3D10_SB_RESOURCE_DIMENSION_TEXTURE3D:
         case D3D10_SB_RESOURCE_DIMENSION_TEXTURECUBE:
         case D3D10_SB_RESOURCE_DIMENSION_TEXTURE1DARRAY:
         case D3D10_SB_RESOURCE_DIMENSION_TEXTURE2DARRAY:
         case D3D10_SB_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
         case D3D10_SB_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
         default:
      assert(0);
         }
      static uint
   texture_dim_from_tgsi_target(unsigned tgsi_target)
   {
      switch (tgsi_target) {
   case TGSI_TEXTURE_BUFFER:
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_1D_ARRAY:
         case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_2D_MSAA:
   case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
         case TGSI_TEXTURE_3D:
         case TGSI_TEXTURE_UNKNOWN:
   default:
      assert(0);
         }
      static bool
   operand_is_scalar(const struct Shader_src_operand *operand)
   {
      return operand->swizzle[0] == operand->swizzle[1] &&
            }
      static void
   Shader_add_call(struct Shader_xlate *sx,
               {
               sx->calls[sx->num_calls].d3d_label = d3d_label;
   sx->calls[sx->num_calls].tgsi_label_token = tgsi_label_token;
      }
      static void
   Shader_add_label(struct Shader_xlate *sx,
               {
               sx->labels[sx->num_labels].d3d_label = d3d_label;
   sx->labels[sx->num_labels].tgsi_insn_no = tgsi_insn_no;
      }
         static void
   sample_ureg_emit(struct ureg_program *ureg,
                  unsigned tgsi_opcode,
   unsigned num_src,
      {
      unsigned num_offsets = 0;
                     if (opcode->imm_texel_offset.u ||
      opcode->imm_texel_offset.v ||
   opcode->imm_texel_offset.w) {
   struct ureg_src offsetreg;
   num_offsets = 1;
   /* don't actually always need all 3 values */
   offsetreg = ureg_imm3i(ureg,
                     texoffsets.File = offsetreg.File;
   texoffsets.Index = offsetreg.Index;
   texoffsets.SwizzleX = offsetreg.SwizzleX;
   texoffsets.SwizzleY = offsetreg.SwizzleY;
               ureg_tex_insn(ureg,
               tgsi_opcode,
   &dst, 1,
   TGSI_TEXTURE_UNKNOWN,
      }
      typedef void (*unary_ureg_func)(struct ureg_program *ureg, struct ureg_dst dst,
         static void
   expand_unary_to_scalarf(struct ureg_program *ureg, unary_ureg_func func,
         {
      struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   struct ureg_dst dst = translate_dst_operand(sx, &opcode->dst[0],
         struct ureg_src src = translate_src_operand(sx, &opcode->src[0], OF_FLOAT);
   struct ureg_dst scalar_dst;
   ureg_MOV(ureg, tmp, src);
            scalar_dst = ureg_writemask(dst, TGSI_WRITEMASK_X);
   if (scalar_dst.WriteMask != TGSI_WRITEMASK_NONE) {
      func(ureg, scalar_dst,
      }
   scalar_dst = ureg_writemask(dst, TGSI_WRITEMASK_Y);
   if (scalar_dst.WriteMask != TGSI_WRITEMASK_NONE) {
      func(ureg, scalar_dst,
      }
   scalar_dst = ureg_writemask(dst, TGSI_WRITEMASK_Z);
   if (scalar_dst.WriteMask != TGSI_WRITEMASK_NONE) {
      func(ureg, scalar_dst,
      }
   scalar_dst = ureg_writemask(dst, TGSI_WRITEMASK_W);
   if (scalar_dst.WriteMask != TGSI_WRITEMASK_NONE) {
      func(ureg, scalar_dst,
      }
      }
      const struct tgsi_token *
   Shader_tgsi_translate(const unsigned *code,
         {
      struct Shader_xlate sx;
   struct Shader_parser parser;
   struct ureg_program *ureg = NULL;
   struct Shader_opcode opcode;
   const struct tgsi_token *tokens = NULL;
   uint nr_tokens;
   bool shader_dumped = false;
   bool inside_sub = false;
                              if (st_debug & ST_DEBUG_TGSI) {
      dx10_shader_dump_tokens(code);
               sx.max_calls = 64;
   sx.calls = (struct Shader_call *)MALLOC(sx.max_calls *
                  sx.max_labels = 64;
   sx.labels = (struct Shader_label *)MALLOC(sx.max_labels *
                        /* Header. */
   switch (parser.header.type) {
   case D3D10_SB_PIXEL_SHADER:
      ureg = ureg_create(PIPE_SHADER_FRAGMENT);
      case D3D10_SB_VERTEX_SHADER:
      ureg = ureg_create(PIPE_SHADER_VERTEX);
      case D3D10_SB_GEOMETRY_SHADER:
      ureg = ureg_create(PIPE_SHADER_GEOMETRY);
               assert(ureg);
            while (Shader_parse_opcode(&parser, &opcode)) {
               assert(opcode.type < D3D10_SB_NUM_OPCODES);
            switch (opcode.type) {
   case D3D10_SB_OPCODE_EXP:
      expand_unary_to_scalarf(ureg, ureg_EX2, &sx, &opcode);
      case D3D10_SB_OPCODE_SQRT:
      expand_unary_to_scalarf(ureg, ureg_SQRT, &sx, &opcode);
      case D3D10_SB_OPCODE_RSQ:
      expand_unary_to_scalarf(ureg, ureg_RSQ, &sx, &opcode);
      case D3D10_SB_OPCODE_LOG:
      expand_unary_to_scalarf(ureg, ureg_LG2, &sx, &opcode);
      case D3D10_SB_OPCODE_IMUL:
      if (opcode.dst[0].base.type != D3D10_SB_OPERAND_TYPE_NULL) {
      ureg_IMUL_HI(ureg,
                           if (opcode.dst[1].base.type != D3D10_SB_OPERAND_TYPE_NULL) {
      ureg_UMUL(ureg,
                                 case D3D10_SB_OPCODE_FTOI: {
      /* XXX: tgsi (and just about everybody else, c, opencl, glsl) has
   * out-of-range (and NaN) values undefined for f2i/f2u, but d3d10
   * requires clamping to min and max representable value (as well as 0
   * for NaNs) (this applies to both ftoi and ftou). At least the online
   * docs state that - this is consistent with generic d3d10 conversion
   * rules.
   * For FTOI, we cheat a bit here - in particular depending on noone
   * caring about NaNs, and depending on the (undefined!) behavior of
   * F2I returning 0x80000000 for too negative values (which works with
   * x86 sse). Hence only need to clamp too positive values.
   * Note that it is impossible to clamp using a float, since 2^31 - 1
   * is not exactly representable with a float.
   */
   struct ureg_dst too_large = ureg_DECL_temporary(ureg);
   struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   ureg_FSGE(ureg, too_large,
               ureg_F2I(ureg, tmp,
         ureg_UCMP(ureg,
            translate_dst_operand(&sx, &opcode.dst[0], opcode.saturate),
   ureg_src(too_large),
      ureg_release_temporary(ureg, too_large);
      }
            case D3D10_SB_OPCODE_FTOU: {
      /* For ftou, we need to do both clamps, which as a bonus also
   * gets us correct NaN behavior.
   * Note that it is impossible to clamp using a float against the upper
   * limit, since 2^32 - 1 is not exactly representable with a float,
   * but the clamp against 0.0 certainly works just fine.
   */
   struct ureg_dst too_large = ureg_DECL_temporary(ureg);
   struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   ureg_FSGE(ureg, too_large,
               /* clamp negative values + NaN to zero.
   * (Could be done slightly more efficient in llvmpipe due to
   * MAX NaN behavior handling.)
   */
   ureg_MAX(ureg, tmp,
               ureg_F2U(ureg, tmp,
         ureg_UCMP(ureg,
            translate_dst_operand(&sx, &opcode.dst[0], opcode.saturate),
   ureg_src(too_large),
      ureg_release_temporary(ureg, too_large);
      }
            case D3D10_SB_OPCODE_LD_MS:
      /* XXX: We don't support multi-sampling yet, but we need to parse
   * this opcode regardless, so we just ignore sample index operand
      case D3D10_SB_OPCODE_LD:
      if (st_debug & ST_DEBUG_OLD_TEX_OPS) {
      unsigned resource = opcode.src[1].base.index[0].imm;
                  if (ureg_src_is_undef(sx.samplers[resource])) {
                        ureg_TXF(ureg,
            translate_dst_operand(&sx, &opcode.dst[0], opcode.saturate),
   sx.resources[resource].target,
   }
   else {
      struct ureg_src srcreg[2];
                  sample_ureg_emit(ureg, TGSI_OPCODE_SAMPLE_I, 2, &opcode,
                              case D3D10_SB_OPCODE_CUSTOMDATA:
      if (opcode.customdata._class ==
      D3D10_SB_CUSTOMDATA_DCL_IMMEDIATE_CONSTANT_BUFFER) {
   sx.imms =
      ureg_DECL_immediate_block_uint(ureg,
         } else {
                     case D3D10_SB_OPCODE_RESINFO:
      if (st_debug & ST_DEBUG_OLD_TEX_OPS) {
      unsigned resource = opcode.src[1].base.index[0].imm;
                  if (ureg_src_is_undef(sx.samplers[resource])) {
      sx.samplers[resource] =
      }
   /* don't bother with swizzle, ret type etc. */
   ureg_TXQ(ureg,
            translate_dst_operand(&sx, &opcode.dst[0],
         sx.resources[resource].target,
   }
   else {
      struct ureg_dst r0 = ureg_DECL_temporary(ureg);
   struct ureg_src tsrc = translate_src_operand(&sx, &opcode.src[1], OF_UINT);
                  /* while specs say swizzle is ignored better safe than sorry */
   tsrc.SwizzleX = TGSI_SWIZZLE_X;
   tsrc.SwizzleY = TGSI_SWIZZLE_Y;
                  ureg_SVIEWINFO(ureg, r0,
                  tsrc = ureg_src(r0);
   tsrc.SwizzleX = opcode.src[1].swizzle[0];
   tsrc.SwizzleY = opcode.src[1].swizzle[1];
                  if (opcode.specific.resinfo_ret_type ==
      D3D10_SB_RESINFO_INSTRUCTION_RETURN_UINT) {
      }
   else if (opcode.specific.resinfo_ret_type ==
               }
   else { /* D3D10_SB_RESINFO_INSTRUCTION_RETURN_RCPFLOAT */
      unsigned i;
   /*
   * Must apply rcp only to parts determined by dims,
   * (width/height/depth) but NOT to array size nor mip levels
   * hence need to figure that out here.
   * This is one sick modifier if you ask me!
   */
                        ureg_I2F(ureg, r0, ureg_src(r0));
   tsrc = ureg_src(r0);
   for (i = 0; i < 4; i++) {
      unsigned dst_swizzle = opcode.src[1].swizzle[i];
   struct ureg_dst dstregmasked = ureg_writemask(dstreg, 1 << i);
   /*
   * could do one mov with multiple write mask bits set
   * but rcp is scalar anyway.
   */
   if (dst_swizzle < dims) {
         }
   else {
               }
                  case D3D10_SB_OPCODE_SAMPLE:
      if (st_debug & ST_DEBUG_OLD_TEX_OPS) {
                              ureg_TEX(ureg,
            translate_dst_operand(&sx, &opcode.dst[0],
         sx.resources[opcode.src[1].base.index[0].imm].target,
   }
   else {
      struct ureg_src srcreg[3];
   srcreg[0] = translate_src_operand(&sx, &opcode.src[0], OF_FLOAT);
                  sample_ureg_emit(ureg, TGSI_OPCODE_SAMPLE, 3, &opcode,
                              case D3D10_SB_OPCODE_SAMPLE_C:
                        /* XXX: Support only 2D texture targets for now.
   *      Need to figure out how to pack the compare value
   *      for other dimensions and if there is enough space
   *      in a single operand for all possible cases.
   */
                                 /* Insert the compare value into .z component.
   */
   ureg_MOV(ureg,
               ureg_MOV(ureg,
                                 ureg_TEX(ureg,
            translate_dst_operand(&sx, &opcode.dst[0],
                        }
   else {
      struct ureg_src srcreg[4];
   srcreg[0] = translate_src_operand(&sx, &opcode.src[0], OF_FLOAT);
   srcreg[1] = translate_src_operand(&sx, &opcode.src[1], OF_UINT);
                  sample_ureg_emit(ureg, TGSI_OPCODE_SAMPLE_C, 4, &opcode,
                              case D3D10_SB_OPCODE_SAMPLE_C_LZ:
                                       /* XXX: Support only 2D texture targets for now.
   *      Need to figure out how to pack the compare value
   *      for other dimensions and if there is enough space
   *      in a single operand for all possible cases.
   */
                  /* Insert the compare value into .z component.
   * Insert 0 into .w component.
   */
   ureg_MOV(ureg,
               ureg_MOV(ureg,
               ureg_MOV(ureg,
                  ureg_TXL(ureg,
            translate_dst_operand(&sx, &opcode.dst[0],
                        }
   else {
      struct ureg_src srcreg[4];
   srcreg[0] = translate_src_operand(&sx, &opcode.src[0], OF_FLOAT);
   srcreg[1] = translate_src_operand(&sx, &opcode.src[1], OF_UINT);
                  sample_ureg_emit(ureg, TGSI_OPCODE_SAMPLE_C_LZ, 4, &opcode,
                              case D3D10_SB_OPCODE_SAMPLE_L:
                                       /* Insert LOD into .w component.
   */
   ureg_MOV(ureg,
               ureg_MOV(ureg,
                  ureg_TXL(ureg,
            translate_dst_operand(&sx, &opcode.dst[0],
                        }
   else {
      struct ureg_src srcreg[4];
   srcreg[0] = translate_src_operand(&sx, &opcode.src[0], OF_FLOAT);
   srcreg[1] = translate_src_operand(&sx, &opcode.src[1], OF_UINT);
                  sample_ureg_emit(ureg, TGSI_OPCODE_SAMPLE_L, 4, &opcode,
                              case D3D10_SB_OPCODE_SAMPLE_D:
      if (st_debug & ST_DEBUG_OLD_TEX_OPS) {
                     ureg_TXD(ureg,
            translate_dst_operand(&sx, &opcode.dst[0],
         sx.resources[opcode.src[1].base.index[0].imm].target,
   translate_src_operand(&sx, &opcode.src[0], OF_FLOAT),
   translate_src_operand(&sx, &opcode.src[3], OF_FLOAT),
   }
   else {
      struct ureg_src srcreg[5];
   srcreg[0] = translate_src_operand(&sx, &opcode.src[0], OF_FLOAT);
   srcreg[1] = translate_src_operand(&sx, &opcode.src[1], OF_UINT);
   srcreg[2] = translate_src_operand(&sx, &opcode.src[2], OF_UINT);
                  sample_ureg_emit(ureg, TGSI_OPCODE_SAMPLE_D, 5, &opcode,
                              case D3D10_SB_OPCODE_SAMPLE_B:
                                       /* Insert LOD bias into .w component.
   */
   ureg_MOV(ureg,
               ureg_MOV(ureg,
                  ureg_TXB(ureg,
            translate_dst_operand(&sx, &opcode.dst[0],
                        }
   else {
      struct ureg_src srcreg[4];
   srcreg[0] = translate_src_operand(&sx, &opcode.src[0], OF_FLOAT);
   srcreg[1] = translate_src_operand(&sx, &opcode.src[1], OF_UINT);
                  sample_ureg_emit(ureg, TGSI_OPCODE_SAMPLE_B, 4, &opcode,
                              case D3D10_SB_OPCODE_SINCOS: {
      struct ureg_dst src0 = ureg_DECL_temporary(ureg);
   ureg_MOV(ureg, src0, translate_src_operand(&sx, &opcode.src[0], OF_FLOAT));
   if (opcode.dst[0].base.type != D3D10_SB_OPERAND_TYPE_NULL) {
      struct ureg_dst dst = translate_dst_operand(&sx, &opcode.dst[0],
         struct ureg_src src = ureg_src(src0);
   ureg_SIN(ureg, ureg_writemask(dst, TGSI_WRITEMASK_X),
         ureg_SIN(ureg, ureg_writemask(dst, TGSI_WRITEMASK_Y),
         ureg_SIN(ureg, ureg_writemask(dst, TGSI_WRITEMASK_Z),
         ureg_SIN(ureg, ureg_writemask(dst, TGSI_WRITEMASK_W),
      }
   if (opcode.dst[1].base.type != D3D10_SB_OPERAND_TYPE_NULL) {
      struct ureg_dst dst = translate_dst_operand(&sx, &opcode.dst[1],
         struct ureg_src src = ureg_src(src0);
   ureg_COS(ureg, ureg_writemask(dst, TGSI_WRITEMASK_X),
         ureg_COS(ureg, ureg_writemask(dst, TGSI_WRITEMASK_Y),
         ureg_COS(ureg, ureg_writemask(dst, TGSI_WRITEMASK_Z),
         ureg_COS(ureg, ureg_writemask(dst, TGSI_WRITEMASK_W),
      }
      }
            case D3D10_SB_OPCODE_UDIV: {
      struct ureg_dst src0 = ureg_DECL_temporary(ureg);
   struct ureg_dst src1 = ureg_DECL_temporary(ureg);
   ureg_MOV(ureg, src0, translate_src_operand(&sx, &opcode.src[0], OF_UINT));
   ureg_MOV(ureg, src1, translate_src_operand(&sx, &opcode.src[1], OF_UINT));
   if (opcode.dst[0].base.type != D3D10_SB_OPERAND_TYPE_NULL) {
      ureg_UDIV(ureg,
            translate_dst_operand(&sx, &opcode.dst[0],
   }
   if (opcode.dst[1].base.type != D3D10_SB_OPERAND_TYPE_NULL) {
      ureg_UMOD(ureg,
            translate_dst_operand(&sx, &opcode.dst[1],
   }
   ureg_release_temporary(ureg, src0);
      }
         case D3D10_SB_OPCODE_UMUL: {
      if (opcode.dst[0].base.type != D3D10_SB_OPERAND_TYPE_NULL) {
      ureg_UMUL_HI(ureg,
               translate_dst_operand(&sx, &opcode.dst[0],
      }
   if (opcode.dst[1].base.type != D3D10_SB_OPERAND_TYPE_NULL) {
      ureg_UMUL(ureg,
            translate_dst_operand(&sx, &opcode.dst[1],
            }
            case D3D10_SB_OPCODE_DCL_RESOURCE:
   {
      unsigned target;
   unsigned res_index = opcode.dst[0].base.index[0].imm;
                  target = translate_resource_dimension(opcode.specific.dcl_resource_dimension);
   sx.resources[res_index].target = target;
   if (!(st_debug & ST_DEBUG_OLD_TEX_OPS)) {
      sx.sv[res_index] =
      ureg_DECL_sampler_view(ureg, res_index, target,
                     }
               case D3D10_SB_OPCODE_DCL_CONSTANT_BUFFER: {
                        if (num_constants == 0) {
         } else {
                  ureg_DECL_constant2D(ureg,
                                 case D3D10_SB_OPCODE_DCL_SAMPLER:
                     sx.samplers[opcode.dst[0].base.index[0].imm] =
      ureg_DECL_sampler(ureg,
            case D3D10_SB_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
               switch (opcode.specific.dcl_gs_output_primitive_topology) {
   case D3D10_SB_PRIMITIVE_TOPOLOGY_POINTLIST:
      ureg_property(sx.ureg,
                     case D3D10_SB_PRIMITIVE_TOPOLOGY_LINESTRIP:
      ureg_property(sx.ureg,
                     case D3D10_SB_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
      ureg_property(sx.ureg,
                     default:
                     case D3D10_SB_OPCODE_DCL_GS_INPUT_PRIMITIVE:
               /* Figure out the second dimension of GS inputs.
   */
   switch (opcode.specific.dcl_gs_input_primitive) {
   case D3D10_SB_PRIMITIVE_POINT:
      declare_vertices_in(&sx, 1);
   ureg_property(sx.ureg,
                     case D3D10_SB_PRIMITIVE_LINE:
      declare_vertices_in(&sx, 2);
   ureg_property(sx.ureg,
                     case D3D10_SB_PRIMITIVE_TRIANGLE:
      declare_vertices_in(&sx, 3);
   ureg_property(sx.ureg,
                     case D3D10_SB_PRIMITIVE_LINE_ADJ:
      declare_vertices_in(&sx, 4);
   ureg_property(sx.ureg,
                     case D3D10_SB_PRIMITIVE_TRIANGLE_ADJ:
      declare_vertices_in(&sx, 6);
   ureg_property(sx.ureg,
                     default:
                     case D3D10_SB_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
               ureg_property(sx.ureg,
                     case D3D10_SB_OPCODE_DCL_INPUT:
      if (parser.header.type == D3D10_SB_VERTEX_SHADER) {
         } else {
      assert(parser.header.type == D3D10_SB_GEOMETRY_SHADER);
                  case D3D10_SB_OPCODE_DCL_INPUT_SGV:
      assert(parser.header.type == D3D10_SB_VERTEX_SHADER);
               case D3D10_SB_OPCODE_DCL_INPUT_SIV:
      assert(parser.header.type == D3D10_SB_GEOMETRY_SHADER);
               case D3D10_SB_OPCODE_DCL_INPUT_PS:
      assert(parser.header.type == D3D10_SB_PIXEL_SHADER);
   dcl_ps_input(&sx, ureg, &opcode.dst[0],
               case D3D10_SB_OPCODE_DCL_INPUT_PS_SGV:
      assert(parser.header.type == D3D10_SB_PIXEL_SHADER);
   dcl_ps_sgv_input(&sx, ureg, &opcode.dst[0],
               case D3D10_SB_OPCODE_DCL_INPUT_PS_SIV:
      assert(parser.header.type == D3D10_SB_PIXEL_SHADER);
   dcl_ps_siv_input(&sx, ureg, &opcode.dst[0],
                     case D3D10_SB_OPCODE_DCL_OUTPUT:
      if (parser.header.type == D3D10_SB_PIXEL_SHADER) {
      /* Pixel shader outputs. */
   if (opcode.dst[0].base.type == D3D10_SB_OPERAND_TYPE_OUTPUT_DEPTH) {
                     sx.output_depth = ureg_DECL_output_masked(ureg, TGSI_SEMANTIC_POSITION, 0, TGSI_WRITEMASK_Z, 0, 1);
      } else {
                           dcl_base_output(&sx, ureg,
                  ureg_DECL_output(ureg,
      } else {
                     if (output_mapping) {
      unsigned nr_outputs = ureg_get_nr_outputs(ureg);
   output_mapping[nr_outputs]
      }
   dcl_base_output(&sx, ureg,
                  ureg_DECL_output(ureg,
               case D3D10_SB_OPCODE_DCL_OUTPUT_SIV:
                     if (output_mapping) {
      unsigned nr_outputs = ureg_get_nr_outputs(ureg);
   output_mapping[nr_outputs]
      }
   if (opcode.dcl_siv_name == D3D10_SB_NAME_CLIP_DISTANCE ||
      opcode.dcl_siv_name == D3D10_SB_NAME_CULL_DISTANCE) {
   /*
   * FIXME: this is quite broken. gallium no longer has separate
   * clip/cull dists, using (max 2) combined clipdist/culldist regs
   * instead. Unlike d3d10 though, which is clip and which cull is
   * simply determined by by number of clip/cull dists (that is,
   * all clip dists must come first).
   */
   unsigned numcliporcull = sx.num_clip_distances_declared +
         sx.clip_distance_mapping[numcliporcull].d3d =
         sx.clip_distance_mapping[numcliporcull].tgsi = numcliporcull;
   if (opcode.dcl_siv_name == D3D10_SB_NAME_CLIP_DISTANCE) {
      ++sx.num_clip_distances_declared;
   /* re-emit should be safe... */
   ureg_property(ureg, TGSI_PROPERTY_NUM_CLIPDIST_ENABLED,
      } else {
      ++sx.num_cull_distances_declared;
   ureg_property(ureg, TGSI_PROPERTY_NUM_CULLDIST_ENABLED,
         } else if (0 && opcode.dcl_siv_name == D3D10_SB_NAME_CULL_DISTANCE) {
      sx.cull_distance_mapping[sx.num_cull_distances_declared].d3d =
         sx.cull_distance_mapping[sx.num_cull_distances_declared].tgsi =
         ++sx.num_cull_distances_declared;
   ureg_property(ureg, TGSI_PROPERTY_NUM_CULLDIST_ENABLED,
               dcl_base_output(&sx, ureg,
                  ureg_DECL_output_masked(
      ureg,
   translate_system_name(opcode.dcl_siv_name),
   translate_semantic_index(&sx, opcode.dcl_siv_name,
               case D3D10_SB_OPCODE_DCL_OUTPUT_SGV:
                     if (output_mapping) {
      unsigned nr_outputs = ureg_get_nr_outputs(ureg);
   output_mapping[nr_outputs]
      }
   dcl_base_output(&sx, ureg,
                  ureg_DECL_output(ureg,
            case D3D10_SB_OPCODE_DCL_TEMPS:
                                                for (i = 0; i < opcode.specific.dcl_num_temps; i++) {
         }
                  case D3D10_SB_OPCODE_DCL_INDEXABLE_TEMP:
                                       assert(opcode.specific.dcl_indexable_temp.index <
                                       for (i = 0; i < opcode.specific.dcl_indexable_temp.count; i++) {
         }
      }
      case D3D10_SB_OPCODE_IF: {
      unsigned label = 0;
   if (opcode.specific.test_boolean == D3D10_SB_INSTRUCTION_TEST_ZERO) {
      struct ureg_src src =
         struct ureg_dst src_nz = ureg_DECL_temporary(ureg);
   ureg_USEQ(ureg, src_nz, src, ureg_imm1u(ureg, 0));
   ureg_UIF(ureg, ureg_src(src_nz), &label);
      } else {
            }
         case D3D10_SB_OPCODE_RETC:
   case D3D10_SB_OPCODE_CONTINUEC:
   case D3D10_SB_OPCODE_CALLC:
   case D3D10_SB_OPCODE_DISCARD:
   case D3D10_SB_OPCODE_BREAKC:
   {
      unsigned label = 0;
   assert(operand_is_scalar(&opcode.src[0]));
   if (opcode.specific.test_boolean == D3D10_SB_INSTRUCTION_TEST_ZERO) {
      struct ureg_src src =
         struct ureg_dst src_nz = ureg_DECL_temporary(ureg);
   ureg_USEQ(ureg, src_nz, src, ureg_imm1u(ureg, 0));
   ureg_UIF(ureg, ureg_src(src_nz), &label);
      }
   else {
         }
   switch (opcode.type) {
   case D3D10_SB_OPCODE_RETC:
      ureg_RET(ureg);
      case D3D10_SB_OPCODE_CONTINUEC:
      ureg_CONT(ureg);
      case D3D10_SB_OPCODE_CALLC: {
      unsigned label = opcode.src[1].base.index[0].imm;
   unsigned tgsi_token_label = 0;
   ureg_CAL(ureg, &tgsi_token_label);
      }
         case D3D10_SB_OPCODE_DISCARD:
      ureg_KILL(ureg);
      case D3D10_SB_OPCODE_BREAKC:
      ureg_BRK(ureg);
      default:
      assert(0);
      }
      }
         case D3D10_SB_OPCODE_LABEL: {
      unsigned label = opcode.src[0].base.index[0].imm;
   unsigned tgsi_inst_no = 0;
   if (inside_sub) {
         }
   tgsi_inst_no = ureg_get_instruction_number(ureg);
   ureg_BGNSUB(ureg);
   inside_sub = true;
      }
         case D3D10_SB_OPCODE_CALL: {
      unsigned label = opcode.src[0].base.index[0].imm;
   unsigned tgsi_token_label = 0;
   ureg_CAL(ureg, &tgsi_token_label);
      }
         case D3D10_SB_OPCODE_EMIT:
      ureg_EMIT(ureg, ureg_imm1u(ureg, 0));
      case D3D10_SB_OPCODE_CUT:
      ureg_ENDPRIM(ureg, ureg_imm1u(ureg, 0));
      case D3D10_SB_OPCODE_EMITTHENCUT:
      ureg_EMIT(ureg, ureg_imm1u(ureg, 0));
   ureg_ENDPRIM(ureg, ureg_imm1u(ureg, 0));
      case D3D10_SB_OPCODE_DCL_INDEX_RANGE:
   case D3D10_SB_OPCODE_DCL_GLOBAL_FLAGS:
      /* Ignore */
      default:
      {
      uint i;
                           if (ox->tgsi_opcode == TGSI_LOG_UNSUPPORTED) {
      if (!shader_dumped) {
      dx10_shader_dump_tokens(code);
      }
   debug_printf("%s: unsupported opcode %i\n",
                     /* Destination operands. */
   for (i = 0; i < opcode.num_dst; i++) {
                        /* Source operands. */
   for (i = 0; i < opcode.num_src; i++) {
                  /* Try to re-route output depth to Z channel. */
   if (opcode.dst[0].base.type == D3D10_SB_OPERAND_TYPE_OUTPUT_DEPTH) {
      LOG_UNSUPPORTED(opcode.type != D3D10_SB_OPCODE_MOV);
                     ureg_insn(ureg,
            ox->tgsi_opcode,
   dst,
   opcode.num_dst,
                           if (inside_sub) {
                           for (i = 0; i < sx.num_calls; ++i) {
      for (j = 0; j < sx.num_labels; ++j) {
      if (sx.calls[i].d3d_label == sx.labels[j].d3d_label) {
      ureg_fixup_label(sx.ureg,
                     }
      }
   FREE(sx.labels);
            tokens = ureg_get_tokens(ureg, &nr_tokens);
   assert(tokens);
            if (st_debug & ST_DEBUG_TGSI) {
                     }
