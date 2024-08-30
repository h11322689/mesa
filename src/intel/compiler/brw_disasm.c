   /*
   * Copyright © 2008 Keith Packard
   *
   * Permission to use, copy, modify, distribute, and sell this software and its
   * documentation for any purpose is hereby granted without fee, provided that
   * the above copyright notice appear in all copies and that both that copyright
   * notice and this permission notice appear in supporting documentation, and
   * that the name of the copyright holders not be used in advertising or
   * publicity pertaining to distribution of the software without specific,
   * written prior permission.  The copyright holders make no representations
   * about the suitability of this software for any purpose.  It is provided "as
   * is" without express or implied warranty.
   *
   * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
   * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
   * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
   * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
   * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   * OF THIS SOFTWARE.
   */
      #include <stdio.h>
   #include <string.h>
   #include <stdarg.h>
      #include "brw_eu_defines.h"
   #include "brw_inst.h"
   #include "brw_shader.h"
   #include "brw_reg.h"
   #include "brw_inst.h"
   #include "brw_eu.h"
   #include "util/half_float.h"
      bool
   brw_has_jip(const struct intel_device_info *devinfo, enum opcode opcode)
   {
      if (devinfo->ver < 6)
            return opcode == BRW_OPCODE_IF ||
         opcode == BRW_OPCODE_ELSE ||
   opcode == BRW_OPCODE_ENDIF ||
   opcode == BRW_OPCODE_WHILE ||
   opcode == BRW_OPCODE_BREAK ||
      }
      bool
   brw_has_uip(const struct intel_device_info *devinfo, enum opcode opcode)
   {
      if (devinfo->ver < 6)
            return (devinfo->ver >= 7 && opcode == BRW_OPCODE_IF) ||
         (devinfo->ver >= 8 && opcode == BRW_OPCODE_ELSE) ||
   opcode == BRW_OPCODE_BREAK ||
      }
      static bool
   has_branch_ctrl(const struct intel_device_info *devinfo, enum opcode opcode)
   {
      if (devinfo->ver < 8)
            return opcode == BRW_OPCODE_IF ||
            }
      static bool
   is_logic_instruction(unsigned opcode)
   {
      return opcode == BRW_OPCODE_AND ||
         opcode == BRW_OPCODE_NOT ||
      }
      static bool
   is_send(unsigned opcode)
   {
      return opcode == BRW_OPCODE_SEND ||
         opcode == BRW_OPCODE_SENDC ||
      }
      static bool
   is_split_send(UNUSED const struct intel_device_info *devinfo, unsigned opcode)
   {
      if (devinfo->ver >= 12)
         else
      return opcode == BRW_OPCODE_SENDS ||
   }
      const char *const conditional_modifier[16] = {
      [BRW_CONDITIONAL_NONE] = "",
   [BRW_CONDITIONAL_Z]    = ".z",
   [BRW_CONDITIONAL_NZ]   = ".nz",
   [BRW_CONDITIONAL_G]    = ".g",
   [BRW_CONDITIONAL_GE]   = ".ge",
   [BRW_CONDITIONAL_L]    = ".l",
   [BRW_CONDITIONAL_LE]   = ".le",
   [BRW_CONDITIONAL_R]    = ".r",
   [BRW_CONDITIONAL_O]    = ".o",
      };
      static const char *const m_negate[2] = {
      [0] = "",
      };
      static const char *const _abs[2] = {
      [0] = "",
      };
      static const char *const m_bitnot[2] = { "", "~" };
      static const char *const vert_stride[16] = {
      [0] = "0",
   [1] = "1",
   [2] = "2",
   [3] = "4",
   [4] = "8",
   [5] = "16",
   [6] = "32",
      };
      static const char *const width[8] = {
      [0] = "1",
   [1] = "2",
   [2] = "4",
   [3] = "8",
      };
      static const char *const horiz_stride[4] = {
      [0] = "0",
   [1] = "1",
   [2] = "2",
      };
      static const char *const chan_sel[4] = {
      [0] = "x",
   [1] = "y",
   [2] = "z",
      };
      static const char *const debug_ctrl[2] = {
      [0] = "",
      };
      static const char *const saturate[2] = {
      [0] = "",
      };
      static const char *const cmpt_ctrl[2] = {
      [0] = "",
      };
      static const char *const accwr[2] = {
      [0] = "",
      };
      static const char *const branch_ctrl[2] = {
      [0] = "",
      };
      static const char *const wectrl[2] = {
      [0] = "",
      };
      static const char *const exec_size[8] = {
      [0] = "1",
   [1] = "2",
   [2] = "4",
   [3] = "8",
   [4] = "16",
      };
      static const char *const pred_inv[2] = {
      [0] = "+",
      };
      const char *const pred_ctrl_align16[16] = {
      [1] = "",
   [2] = ".x",
   [3] = ".y",
   [4] = ".z",
   [5] = ".w",
   [6] = ".any4h",
      };
      static const char *const pred_ctrl_align1[16] = {
      [BRW_PREDICATE_NORMAL]        = "",
   [BRW_PREDICATE_ALIGN1_ANYV]   = ".anyv",
   [BRW_PREDICATE_ALIGN1_ALLV]   = ".allv",
   [BRW_PREDICATE_ALIGN1_ANY2H]  = ".any2h",
   [BRW_PREDICATE_ALIGN1_ALL2H]  = ".all2h",
   [BRW_PREDICATE_ALIGN1_ANY4H]  = ".any4h",
   [BRW_PREDICATE_ALIGN1_ALL4H]  = ".all4h",
   [BRW_PREDICATE_ALIGN1_ANY8H]  = ".any8h",
   [BRW_PREDICATE_ALIGN1_ALL8H]  = ".all8h",
   [BRW_PREDICATE_ALIGN1_ANY16H] = ".any16h",
   [BRW_PREDICATE_ALIGN1_ALL16H] = ".all16h",
   [BRW_PREDICATE_ALIGN1_ANY32H] = ".any32h",
      };
      static const char *const thread_ctrl[4] = {
      [BRW_THREAD_NORMAL] = "",
   [BRW_THREAD_ATOMIC] = "atomic",
      };
      static const char *const compr_ctrl[4] = {
      [0] = "",
   [1] = "sechalf",
   [2] = "compr",
      };
      static const char *const dep_ctrl[4] = {
      [0] = "",
   [1] = "NoDDClr",
   [2] = "NoDDChk",
      };
      static const char *const mask_ctrl[4] = {
      [0] = "",
      };
      static const char *const access_mode[2] = {
      [0] = "align1",
      };
      static const char *const reg_file[4] = {
      [0] = "A",
   [1] = "g",
   [2] = "m",
      };
      static const char *const writemask[16] = {
      [0x0] = ".",
   [0x1] = ".x",
   [0x2] = ".y",
   [0x3] = ".xy",
   [0x4] = ".z",
   [0x5] = ".xz",
   [0x6] = ".yz",
   [0x7] = ".xyz",
   [0x8] = ".w",
   [0x9] = ".xw",
   [0xa] = ".yw",
   [0xb] = ".xyw",
   [0xc] = ".zw",
   [0xd] = ".xzw",
   [0xe] = ".yzw",
      };
      static const char *const end_of_thread[2] = {
      [0] = "",
      };
      /* SFIDs on Gfx4-5 */
   static const char *const gfx4_sfid[16] = {
      [BRW_SFID_NULL]            = "null",
   [BRW_SFID_MATH]            = "math",
   [BRW_SFID_SAMPLER]         = "sampler",
   [BRW_SFID_MESSAGE_GATEWAY] = "gateway",
   [BRW_SFID_DATAPORT_READ]   = "read",
   [BRW_SFID_DATAPORT_WRITE]  = "write",
   [BRW_SFID_URB]             = "urb",
   [BRW_SFID_THREAD_SPAWNER]  = "thread_spawner",
      };
      static const char *const gfx6_sfid[16] = {
      [BRW_SFID_NULL]                     = "null",
   [BRW_SFID_MATH]                     = "math",
   [BRW_SFID_SAMPLER]                  = "sampler",
   [BRW_SFID_MESSAGE_GATEWAY]          = "gateway",
   [BRW_SFID_URB]                      = "urb",
   [BRW_SFID_THREAD_SPAWNER]           = "thread_spawner",
   [GFX6_SFID_DATAPORT_SAMPLER_CACHE]  = "dp_sampler",
   [GFX6_SFID_DATAPORT_RENDER_CACHE]   = "render",
   [GFX6_SFID_DATAPORT_CONSTANT_CACHE] = "const",
   [GFX7_SFID_DATAPORT_DATA_CACHE]     = "data",
   [GFX7_SFID_PIXEL_INTERPOLATOR]      = "pixel interp",
   [HSW_SFID_DATAPORT_DATA_CACHE_1]    = "dp data 1",
   [HSW_SFID_CRE]                      = "cre",
   [GEN_RT_SFID_RAY_TRACE_ACCELERATOR] = "rt accel",
   [GFX12_SFID_SLM]                    = "slm",
   [GFX12_SFID_TGM]                    = "tgm",
      };
      static const char *const gfx7_gateway_subfuncid[8] = {
      [BRW_MESSAGE_GATEWAY_SFID_OPEN_GATEWAY] = "open",
   [BRW_MESSAGE_GATEWAY_SFID_CLOSE_GATEWAY] = "close",
   [BRW_MESSAGE_GATEWAY_SFID_FORWARD_MSG] = "forward msg",
   [BRW_MESSAGE_GATEWAY_SFID_GET_TIMESTAMP] = "get timestamp",
   [BRW_MESSAGE_GATEWAY_SFID_BARRIER_MSG] = "barrier msg",
   [BRW_MESSAGE_GATEWAY_SFID_UPDATE_GATEWAY_STATE] = "update state",
      };
      static const char *const gfx4_dp_read_port_msg_type[4] = {
      [0b00] = "OWord Block Read",
   [0b01] = "OWord Dual Block Read",
   [0b10] = "Media Block Read",
      };
      static const char *const g45_dp_read_port_msg_type[8] = {
      [0b000] = "OWord Block Read",
   [0b010] = "OWord Dual Block Read",
   [0b100] = "Media Block Read",
   [0b110] = "DWord Scattered Read",
   [0b001] = "Render Target UNORM Read",
      };
      static const char *const dp_write_port_msg_type[8] = {
      [0b000] = "OWord block write",
   [0b001] = "OWord dual block write",
   [0b010] = "media block write",
   [0b011] = "DWord scattered write",
   [0b100] = "RT write",
   [0b101] = "streamed VB write",
   [0b110] = "RT UNORM write", /* G45+ */
      };
      static const char *const dp_rc_msg_type_gfx6[16] = {
      [BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ] = "OWORD block read",
   [GFX6_DATAPORT_READ_MESSAGE_RENDER_UNORM_READ] = "RT UNORM read",
   [GFX6_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ] = "OWORD dual block read",
   [GFX6_DATAPORT_READ_MESSAGE_MEDIA_BLOCK_READ] = "media block read",
   [GFX6_DATAPORT_READ_MESSAGE_OWORD_UNALIGN_BLOCK_READ] =
         [GFX6_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ] = "DWORD scattered read",
   [GFX6_DATAPORT_WRITE_MESSAGE_DWORD_ATOMIC_WRITE] = "DWORD atomic write",
   [GFX6_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE] = "OWORD block write",
   [GFX6_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE] =
         [GFX6_DATAPORT_WRITE_MESSAGE_MEDIA_BLOCK_WRITE] = "media block write",
   [GFX6_DATAPORT_WRITE_MESSAGE_DWORD_SCATTERED_WRITE] =
         [GFX6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE] = "RT write",
   [GFX6_DATAPORT_WRITE_MESSAGE_STREAMED_VB_WRITE] = "streamed VB write",
      };
      static const char *const dp_rc_msg_type_gfx7[16] = {
      [GFX7_DATAPORT_RC_MEDIA_BLOCK_READ] = "media block read",
   [GFX7_DATAPORT_RC_TYPED_SURFACE_READ] = "typed surface read",
   [GFX7_DATAPORT_RC_TYPED_ATOMIC_OP] = "typed atomic op",
   [GFX7_DATAPORT_RC_MEMORY_FENCE] = "memory fence",
   [GFX7_DATAPORT_RC_MEDIA_BLOCK_WRITE] = "media block write",
   [GFX7_DATAPORT_RC_RENDER_TARGET_WRITE] = "RT write",
      };
      static const char *const dp_rc_msg_type_gfx9[16] = {
      [GFX9_DATAPORT_RC_RENDER_TARGET_WRITE] = "RT write",
      };
      static const char *const *
   dp_rc_msg_type(const struct intel_device_info *devinfo)
   {
      return (devinfo->ver >= 9 ? dp_rc_msg_type_gfx9 :
         devinfo->ver >= 7 ? dp_rc_msg_type_gfx7 :
      }
      static const char *const m_rt_write_subtype[] = {
      [0b000] = "SIMD16",
   [0b001] = "SIMD16/RepData",
   [0b010] = "SIMD8/DualSrcLow",
   [0b011] = "SIMD8/DualSrcHigh",
   [0b100] = "SIMD8",
   [0b101] = "SIMD8/ImageWrite",   /* Gfx6+ */
      };
      static const char *const dp_dc0_msg_type_gfx7[16] = {
      [GFX7_DATAPORT_DC_OWORD_BLOCK_READ] = "DC OWORD block read",
   [GFX7_DATAPORT_DC_UNALIGNED_OWORD_BLOCK_READ] =
         [GFX7_DATAPORT_DC_OWORD_DUAL_BLOCK_READ] = "DC OWORD dual block read",
   [GFX7_DATAPORT_DC_DWORD_SCATTERED_READ] = "DC DWORD scattered read",
   [GFX7_DATAPORT_DC_BYTE_SCATTERED_READ] = "DC byte scattered read",
   [GFX7_DATAPORT_DC_UNTYPED_SURFACE_READ] = "DC untyped surface read",
   [GFX7_DATAPORT_DC_UNTYPED_ATOMIC_OP] = "DC untyped atomic",
   [GFX7_DATAPORT_DC_MEMORY_FENCE] = "DC mfence",
   [GFX7_DATAPORT_DC_OWORD_BLOCK_WRITE] = "DC OWORD block write",
   [GFX7_DATAPORT_DC_OWORD_DUAL_BLOCK_WRITE] = "DC OWORD dual block write",
   [GFX7_DATAPORT_DC_DWORD_SCATTERED_WRITE] = "DC DWORD scatterd write",
   [GFX7_DATAPORT_DC_BYTE_SCATTERED_WRITE] = "DC byte scattered write",
      };
      static const char *const dp_oword_block_rw[8] = {
         [BRW_DATAPORT_OWORD_BLOCK_1_OWORDLOW]  = "1-low",
   [BRW_DATAPORT_OWORD_BLOCK_1_OWORDHIGH] = "1-high",
   [BRW_DATAPORT_OWORD_BLOCK_2_OWORDS]    = "2",
   [BRW_DATAPORT_OWORD_BLOCK_4_OWORDS]    = "4",
   };
      static const char *const dp_dc1_msg_type_hsw[32] = {
      [HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_READ] = "untyped surface read",
   [HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP] = "DC untyped atomic op",
   [HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP_SIMD4X2] =
         [HSW_DATAPORT_DC_PORT1_MEDIA_BLOCK_READ] = "DC media block read",
   [HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_READ] = "DC typed surface read",
   [HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP] = "DC typed atomic",
   [HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP_SIMD4X2] = "DC typed 4x2 atomic op",
   [HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_WRITE] = "DC untyped surface write",
   [HSW_DATAPORT_DC_PORT1_MEDIA_BLOCK_WRITE] = "DC media block write",
   [HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP] = "DC atomic counter op",
   [HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP_SIMD4X2] =
         [HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_WRITE] = "DC typed surface write",
   [GFX9_DATAPORT_DC_PORT1_A64_SCATTERED_READ] = "DC A64 scattered read",
   [GFX8_DATAPORT_DC_PORT1_A64_UNTYPED_SURFACE_READ] = "DC A64 untyped surface read",
   [GFX8_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_OP] = "DC A64 untyped atomic op",
   [GFX9_DATAPORT_DC_PORT1_A64_OWORD_BLOCK_READ] = "DC A64 oword block read",
   [GFX9_DATAPORT_DC_PORT1_A64_OWORD_BLOCK_WRITE] = "DC A64 oword block write",
   [GFX8_DATAPORT_DC_PORT1_A64_UNTYPED_SURFACE_WRITE] = "DC A64 untyped surface write",
   [GFX8_DATAPORT_DC_PORT1_A64_SCATTERED_WRITE] = "DC A64 scattered write",
   [GFX9_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_FLOAT_OP] =
         [GFX9_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_FLOAT_OP] =
         [GFX12_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_HALF_INT_OP] =
         [GFX12_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_HALF_FLOAT_OP] =
      };
      static const char *const aop[16] = {
      [BRW_AOP_AND]    = "and",
   [BRW_AOP_OR]     = "or",
   [BRW_AOP_XOR]    = "xor",
   [BRW_AOP_MOV]    = "mov",
   [BRW_AOP_INC]    = "inc",
   [BRW_AOP_DEC]    = "dec",
   [BRW_AOP_ADD]    = "add",
   [BRW_AOP_SUB]    = "sub",
   [BRW_AOP_REVSUB] = "revsub",
   [BRW_AOP_IMAX]   = "imax",
   [BRW_AOP_IMIN]   = "imin",
   [BRW_AOP_UMAX]   = "umax",
   [BRW_AOP_UMIN]   = "umin",
   [BRW_AOP_CMPWR]  = "cmpwr",
      };
      static const char *const aop_float[5] = {
      [BRW_AOP_FMAX]   = "fmax",
   [BRW_AOP_FMIN]   = "fmin",
   [BRW_AOP_FCMPWR] = "fcmpwr",
      };
      static const char * const pixel_interpolator_msg_types[4] = {
      [GFX7_PIXEL_INTERPOLATOR_LOC_SHARED_OFFSET] = "per_message_offset",
   [GFX7_PIXEL_INTERPOLATOR_LOC_SAMPLE] = "sample_position",
   [GFX7_PIXEL_INTERPOLATOR_LOC_CENTROID] = "centroid",
      };
      static const char *const math_function[16] = {
      [BRW_MATH_FUNCTION_INV]    = "inv",
   [BRW_MATH_FUNCTION_LOG]    = "log",
   [BRW_MATH_FUNCTION_EXP]    = "exp",
   [BRW_MATH_FUNCTION_SQRT]   = "sqrt",
   [BRW_MATH_FUNCTION_RSQ]    = "rsq",
   [BRW_MATH_FUNCTION_SIN]    = "sin",
   [BRW_MATH_FUNCTION_COS]    = "cos",
   [BRW_MATH_FUNCTION_SINCOS] = "sincos",
   [BRW_MATH_FUNCTION_FDIV]   = "fdiv",
   [BRW_MATH_FUNCTION_POW]    = "pow",
   [BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER] = "intdivmod",
   [BRW_MATH_FUNCTION_INT_DIV_QUOTIENT]  = "intdiv",
   [BRW_MATH_FUNCTION_INT_DIV_REMAINDER] = "intmod",
   [GFX8_MATH_FUNCTION_INVM]  = "invm",
      };
      static const char *const sync_function[16] = {
      [TGL_SYNC_NOP] = "nop",
   [TGL_SYNC_ALLRD] = "allrd",
   [TGL_SYNC_ALLWR] = "allwr",
   [TGL_SYNC_BAR] = "bar",
      };
      static const char *const math_saturate[2] = {
      [0] = "",
      };
      static const char *const math_signed[2] = {
      [0] = "",
      };
      static const char *const math_scalar[2] = {
      [0] = "",
      };
      static const char *const math_precision[2] = {
      [0] = "",
      };
      static const char *const gfx5_urb_opcode[] = {
      [0] = "urb_write",
      };
      static const char *const gfx7_urb_opcode[] = {
      [BRW_URB_OPCODE_WRITE_HWORD] = "write HWord",
   [BRW_URB_OPCODE_WRITE_OWORD] = "write OWord",
   [BRW_URB_OPCODE_READ_HWORD] = "read HWord",
   [BRW_URB_OPCODE_READ_OWORD] = "read OWord",
   [GFX7_URB_OPCODE_ATOMIC_MOV] = "atomic mov",  /* Gfx7+ */
   [GFX7_URB_OPCODE_ATOMIC_INC] = "atomic inc",  /* Gfx7+ */
   [GFX8_URB_OPCODE_ATOMIC_ADD] = "atomic add",  /* Gfx8+ */
   [GFX8_URB_OPCODE_SIMD8_WRITE] = "SIMD8 write", /* Gfx8+ */
   [GFX8_URB_OPCODE_SIMD8_READ] = "SIMD8 read",  /* Gfx8+ */
   [GFX125_URB_OPCODE_FENCE] = "fence",  /* Gfx12.5+ */
      };
      static const char *const urb_swizzle[4] = {
      [BRW_URB_SWIZZLE_NONE]       = "",
   [BRW_URB_SWIZZLE_INTERLEAVE] = "interleave",
      };
      static const char *const urb_allocate[2] = {
      [0] = "",
      };
      static const char *const urb_used[2] = {
      [0] = "",
      };
      static const char *const urb_complete[2] = {
      [0] = "",
      };
      static const char *const gfx5_sampler_msg_type[] = {
      [GFX5_SAMPLER_MESSAGE_SAMPLE]              = "sample",
   [GFX5_SAMPLER_MESSAGE_SAMPLE_BIAS]         = "sample_b",
   [GFX5_SAMPLER_MESSAGE_SAMPLE_LOD]          = "sample_l",
   [GFX5_SAMPLER_MESSAGE_SAMPLE_COMPARE]      = "sample_c",
   [GFX5_SAMPLER_MESSAGE_SAMPLE_DERIVS]       = "sample_d",
   [GFX5_SAMPLER_MESSAGE_SAMPLE_BIAS_COMPARE] = "sample_b_c",
   [GFX5_SAMPLER_MESSAGE_SAMPLE_LOD_COMPARE]  = "sample_l_c",
   [GFX5_SAMPLER_MESSAGE_SAMPLE_LD]           = "ld",
   [GFX7_SAMPLER_MESSAGE_SAMPLE_GATHER4]      = "gather4",
   [GFX5_SAMPLER_MESSAGE_LOD]                 = "lod",
   [GFX5_SAMPLER_MESSAGE_SAMPLE_RESINFO]      = "resinfo",
   [GFX6_SAMPLER_MESSAGE_SAMPLE_SAMPLEINFO]   = "sampleinfo",
   [GFX7_SAMPLER_MESSAGE_SAMPLE_GATHER4_C]    = "gather4_c",
   [GFX7_SAMPLER_MESSAGE_SAMPLE_GATHER4_PO]   = "gather4_po",
   [GFX7_SAMPLER_MESSAGE_SAMPLE_GATHER4_PO_C] = "gather4_po_c",
   [HSW_SAMPLER_MESSAGE_SAMPLE_DERIV_COMPARE] = "sample_d_c",
   [GFX9_SAMPLER_MESSAGE_SAMPLE_LZ]           = "sample_lz",
   [GFX9_SAMPLER_MESSAGE_SAMPLE_C_LZ]         = "sample_c_lz",
   [GFX9_SAMPLER_MESSAGE_SAMPLE_LD_LZ]        = "ld_lz",
   [GFX9_SAMPLER_MESSAGE_SAMPLE_LD2DMS_W]     = "ld2dms_w",
   [GFX7_SAMPLER_MESSAGE_SAMPLE_LD_MCS]       = "ld_mcs",
   [GFX7_SAMPLER_MESSAGE_SAMPLE_LD2DMS]       = "ld2dms",
      };
      static const char *const gfx5_sampler_simd_mode[7] = {
      [BRW_SAMPLER_SIMD_MODE_SIMD4X2]   = "SIMD4x2",
   [BRW_SAMPLER_SIMD_MODE_SIMD8]     = "SIMD8",
   [BRW_SAMPLER_SIMD_MODE_SIMD16]    = "SIMD16",
   [BRW_SAMPLER_SIMD_MODE_SIMD32_64] = "SIMD32/64",
   [GFX10_SAMPLER_SIMD_MODE_SIMD8H]  = "SIMD8H",
      };
      static const char *const sampler_target_format[4] = {
      [0] = "F",
   [2] = "UD",
      };
      static const char *const lsc_operation[] = {
      [LSC_OP_LOAD]            = "load",
   [LSC_OP_LOAD_CMASK]      = "load_cmask",
   [LSC_OP_STORE]           = "store",
   [LSC_OP_STORE_CMASK]     = "store_cmask",
   [LSC_OP_FENCE]           = "fence",
   [LSC_OP_ATOMIC_INC]      = "atomic_inc",
   [LSC_OP_ATOMIC_DEC]      = "atomic_dec",
   [LSC_OP_ATOMIC_LOAD]     = "atomic_load",
   [LSC_OP_ATOMIC_STORE]    = "atomic_store",
   [LSC_OP_ATOMIC_ADD]      = "atomic_add",
   [LSC_OP_ATOMIC_SUB]      = "atomic_sub",
   [LSC_OP_ATOMIC_MIN]      = "atomic_min",
   [LSC_OP_ATOMIC_MAX]      = "atomic_max",
   [LSC_OP_ATOMIC_UMIN]     = "atomic_umin",
   [LSC_OP_ATOMIC_UMAX]     = "atomic_umax",
   [LSC_OP_ATOMIC_CMPXCHG]  = "atomic_cmpxchg",
   [LSC_OP_ATOMIC_FADD]     = "atomic_fadd",
   [LSC_OP_ATOMIC_FSUB]     = "atomic_fsub",
   [LSC_OP_ATOMIC_FMIN]     = "atomic_fmin",
   [LSC_OP_ATOMIC_FMAX]     = "atomic_fmax",
   [LSC_OP_ATOMIC_FCMPXCHG] = "atomic_fcmpxchg",
   [LSC_OP_ATOMIC_AND]      = "atomic_and",
   [LSC_OP_ATOMIC_OR]       = "atomic_or",
      };
      static const char *const lsc_addr_surface_type[] = {
      [LSC_ADDR_SURFTYPE_FLAT] = "flat",
   [LSC_ADDR_SURFTYPE_BSS]  = "bss",
   [LSC_ADDR_SURFTYPE_SS]   = "ss",
      };
      static const char* const lsc_fence_scope[] = {
      [LSC_FENCE_THREADGROUP]     = "threadgroup",
   [LSC_FENCE_LOCAL]           = "local",
   [LSC_FENCE_TILE]            = "tile",
   [LSC_FENCE_GPU]             = "gpu",
   [LSC_FENCE_ALL_GPU]         = "all_gpu",
   [LSC_FENCE_SYSTEM_RELEASE]  = "system_release",
      };
      static const char* const lsc_flush_type[] = {
      [LSC_FLUSH_TYPE_NONE]       = "none",
   [LSC_FLUSH_TYPE_EVICT]      = "evict",
   [LSC_FLUSH_TYPE_INVALIDATE] = "invalidate",
   [LSC_FLUSH_TYPE_DISCARD]    = "discard",
   [LSC_FLUSH_TYPE_CLEAN]      = "clean",
   [LSC_FLUSH_TYPE_L3ONLY]     = "l3only",
      };
      static const char* const lsc_addr_size[] = {
      [LSC_ADDR_SIZE_A16] = "a16",
   [LSC_ADDR_SIZE_A32] = "a32",
      };
      static const char* const lsc_backup_fence_routing[] = {
      [LSC_NORMAL_ROUTING]  = "normal_routing",
      };
      static const char* const lsc_data_size[] = {
      [LSC_DATA_SIZE_D8]      = "d8",
   [LSC_DATA_SIZE_D16]     = "d16",
   [LSC_DATA_SIZE_D32]     = "d32",
   [LSC_DATA_SIZE_D64]     = "d64",
   [LSC_DATA_SIZE_D8U32]   = "d8u32",
   [LSC_DATA_SIZE_D16U32]  = "d16u32",
      };
      static const char* const lsc_vect_size_str[] = {
      [LSC_VECT_SIZE_V1] = "V1",
   [LSC_VECT_SIZE_V2] = "V2",
   [LSC_VECT_SIZE_V3] = "V3",
   [LSC_VECT_SIZE_V4] = "V4",
   [LSC_VECT_SIZE_V8] = "V8",
   [LSC_VECT_SIZE_V16] = "V16",
   [LSC_VECT_SIZE_V32] = "V32",
      };
      static const char* const lsc_cmask_str[] = {
      [LSC_CMASK_X]      = "x",
   [LSC_CMASK_Y]      = "y",
   [LSC_CMASK_XY]     = "xy",
   [LSC_CMASK_Z]      = "z",
   [LSC_CMASK_XZ]     = "xz",
   [LSC_CMASK_YZ]     = "yz",
   [LSC_CMASK_XYZ]    = "xyz",
   [LSC_CMASK_W]      = "w",
   [LSC_CMASK_XW]     = "xw",
   [LSC_CMASK_YW]     = "yw",
   [LSC_CMASK_XYW]    = "xyw",
   [LSC_CMASK_ZW]     = "zw",
   [LSC_CMASK_XZW]    = "xzw",
   [LSC_CMASK_YZW]    = "yzw",
      };
      static const char* const lsc_cache_load[] = {
      [LSC_CACHE_LOAD_L1STATE_L3MOCS]   = "L1STATE_L3MOCS",
   [LSC_CACHE_LOAD_L1UC_L3UC]        = "L1UC_L3UC",
   [LSC_CACHE_LOAD_L1UC_L3C]         = "L1UC_L3C",
   [LSC_CACHE_LOAD_L1C_L3UC]         = "L1C_L3UC",
   [LSC_CACHE_LOAD_L1C_L3C]          = "L1C_L3C",
   [LSC_CACHE_LOAD_L1S_L3UC]         = "L1S_L3UC",
   [LSC_CACHE_LOAD_L1S_L3C]          = "L1S_L3C",
      };
      static const char* const lsc_cache_store[] = {
      [LSC_CACHE_STORE_L1STATE_L3MOCS]  = "L1STATE_L3MOCS",
   [LSC_CACHE_STORE_L1UC_L3UC]       = "L1UC_L3UC",
   [LSC_CACHE_STORE_L1UC_L3WB]       = "L1UC_L3WB",
   [LSC_CACHE_STORE_L1WT_L3UC]       = "L1WT_L3UC",
   [LSC_CACHE_STORE_L1WT_L3WB]       = "L1WT_L3WB",
   [LSC_CACHE_STORE_L1S_L3UC]        = "L1S_L3UC",
   [LSC_CACHE_STORE_L1S_L3WB]        = "L1S_L3WB",
      };
      static int column;
      static int
   string(FILE *file, const char *string)
   {
      fputs(string, file);
   column += strlen(string);
      }
      static int
   format(FILE *f, const char *format, ...) PRINTFLIKE(2, 3);
      static int
   format(FILE *f, const char *format, ...)
   {
      char buf[1024];
   va_list args;
            vsnprintf(buf, sizeof(buf) - 1, format, args);
   va_end(args);
   string(f, buf);
      }
      static int
   newline(FILE *f)
   {
      putc('\n', f);
   column = 0;
      }
      static int
   pad(FILE *f, int c)
   {
      do
         while (column < c);
      }
      static int
   control(FILE *file, const char *name, const char *const ctrl[],
         {
      if (!ctrl[id]) {
      fprintf(file, "*** invalid %s value %d ", name, id);
      }
   if (ctrl[id][0]) {
      if (space && *space)
         string(file, ctrl[id]);
   if (space)
      }
      }
      static int
   print_opcode(FILE *file, const struct brw_isa_info *isa,
         {
      const struct opcode_desc *desc = brw_opcode_desc(isa, id);
   if (!desc) {
      format(file, "*** invalid opcode value %d ", id);
      }
   string(file, desc->name);
      }
      static int
   reg(FILE *file, unsigned _reg_file, unsigned _reg_nr)
   {
               /* Clear the Compr4 instruction compression bit. */
   if (_reg_file == BRW_MESSAGE_REGISTER_FILE)
            if (_reg_file == BRW_ARCHITECTURE_REGISTER_FILE) {
      switch (_reg_nr & 0xf0) {
   case BRW_ARF_NULL:
      string(file, "null");
      case BRW_ARF_ADDRESS:
      format(file, "a%d", _reg_nr & 0x0f);
      case BRW_ARF_ACCUMULATOR:
      format(file, "acc%d", _reg_nr & 0x0f);
      case BRW_ARF_FLAG:
      format(file, "f%d", _reg_nr & 0x0f);
      case BRW_ARF_MASK:
      format(file, "mask%d", _reg_nr & 0x0f);
      case BRW_ARF_MASK_STACK:
      format(file, "ms%d", _reg_nr & 0x0f);
      case BRW_ARF_MASK_STACK_DEPTH:
      format(file, "msd%d", _reg_nr & 0x0f);
      case BRW_ARF_STATE:
      format(file, "sr%d", _reg_nr & 0x0f);
      case BRW_ARF_CONTROL:
      format(file, "cr%d", _reg_nr & 0x0f);
      case BRW_ARF_NOTIFICATION_COUNT:
      format(file, "n%d", _reg_nr & 0x0f);
      case BRW_ARF_IP:
      string(file, "ip");
   return -1;
      case BRW_ARF_TDR:
      format(file, "tdr0");
      case BRW_ARF_TIMESTAMP:
      format(file, "tm%d", _reg_nr & 0x0f);
      default:
      format(file, "ARF%d", _reg_nr);
         } else {
      err |= control(file, "src reg file", reg_file, _reg_file, NULL);
      }
      }
      static int
   dest(FILE *file, const struct brw_isa_info *isa, const brw_inst *inst)
   {
      const struct intel_device_info *devinfo = isa->devinfo;
   enum brw_reg_type type = brw_inst_dst_type(devinfo, inst);
   unsigned elem_size = brw_reg_type_to_size(type);
            if (is_split_send(devinfo, brw_inst_opcode(isa, inst))) {
      /* These are fixed for split sends */
   type = BRW_REGISTER_TYPE_UD;
   elem_size = 4;
   if (devinfo->ver >= 12) {
      err |= reg(file, brw_inst_send_dst_reg_file(devinfo, inst),
            } else if (brw_inst_dst_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT) {
      err |= reg(file, brw_inst_send_dst_reg_file(devinfo, inst),
         unsigned subreg_nr = brw_inst_dst_da16_subreg_nr(devinfo, inst);
   if (subreg_nr)
            } else {
      string(file, "g[a0");
   if (brw_inst_dst_ia_subreg_nr(devinfo, inst))
      format(file, ".%"PRIu64, brw_inst_dst_ia_subreg_nr(devinfo, inst) /
      if (brw_inst_send_dst_ia16_addr_imm(devinfo, inst))
         string(file, "]<");
         } else if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      if (brw_inst_dst_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT) {
      err |= reg(file, brw_inst_dst_reg_file(devinfo, inst),
         if (err == -1)
         if (brw_inst_dst_da1_subreg_nr(devinfo, inst))
      format(file, ".%"PRIu64, brw_inst_dst_da1_subreg_nr(devinfo, inst) /
      string(file, "<");
   err |= control(file, "horiz stride", horiz_stride,
         string(file, ">");
      } else {
      string(file, "g[a0");
   if (brw_inst_dst_ia_subreg_nr(devinfo, inst))
      format(file, ".%"PRIu64, brw_inst_dst_ia_subreg_nr(devinfo, inst) /
      if (brw_inst_dst_ia1_addr_imm(devinfo, inst))
         string(file, "]<");
   err |= control(file, "horiz stride", horiz_stride,
         string(file, ">");
         } else {
      if (brw_inst_dst_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT) {
      err |= reg(file, brw_inst_dst_reg_file(devinfo, inst),
         if (err == -1)
         if (brw_inst_dst_da16_subreg_nr(devinfo, inst))
         string(file, "<1>");
   err |= control(file, "writemask", writemask,
            } else {
      err = 1;
                     }
      static int
   dest_3src(FILE *file, const struct intel_device_info *devinfo,
         {
      bool is_align1 = brw_inst_3src_access_mode(devinfo, inst) == BRW_ALIGN_1;
   int err = 0;
   uint32_t reg_file;
   unsigned subreg_nr;
            if (devinfo->ver < 10 && is_align1)
            if (devinfo->ver == 6 && brw_inst_3src_a16_dst_reg_file(devinfo, inst))
         else if (devinfo->ver >= 12)
         else if (is_align1 && brw_inst_3src_a1_dst_reg_file(devinfo, inst))
         else
            err |= reg(file, reg_file, brw_inst_3src_dst_reg_nr(devinfo, inst));
   if (err == -1)
            if (is_align1) {
      type = brw_inst_3src_a1_dst_type(devinfo, inst);
      } else {
      type = brw_inst_3src_a16_dst_type(devinfo, inst);
      }
            if (subreg_nr)
                  if (!is_align1) {
      err |= control(file, "writemask", writemask,
      }
               }
      static int
   src_align1_region(FILE *file,
               {
      int err = 0;
   string(file, "<");
   err |= control(file, "vert stride", vert_stride, _vert_stride, NULL);
   string(file, ",");
   err |= control(file, "width", width, _width, NULL);
   string(file, ",");
   err |= control(file, "horiz_stride", horiz_stride, _horiz_stride, NULL);
   string(file, ">");
      }
      static int
   src_da1(FILE *file,
         const struct intel_device_info *devinfo,
   unsigned opcode,
   enum brw_reg_type type, unsigned _reg_file,
   unsigned _vert_stride, unsigned _width, unsigned _horiz_stride,
   unsigned reg_num, unsigned sub_reg_num, unsigned __abs,
   {
               if (devinfo->ver >= 8 && is_logic_instruction(opcode))
         else
                     err |= reg(file, _reg_file, reg_num);
   if (err == -1)
         if (sub_reg_num) {
      unsigned elem_size = brw_reg_type_to_size(type);
      }
   src_align1_region(file, _vert_stride, _width, _horiz_stride);
   string(file, brw_reg_type_to_letters(type));
      }
      static int
   src_ia1(FILE *file,
         const struct intel_device_info *devinfo,
   unsigned opcode,
   enum brw_reg_type type,
   int _addr_imm,
   unsigned _addr_subreg_nr,
   unsigned _negate,
   unsigned __abs,
   {
               if (devinfo->ver >= 8 && is_logic_instruction(opcode))
         else
                     string(file, "g[a0");
   if (_addr_subreg_nr)
         if (_addr_imm)
         string(file, "]");
   src_align1_region(file, _vert_stride, _width, _horiz_stride);
   string(file, brw_reg_type_to_letters(type));
      }
      static int
   src_swizzle(FILE *file, unsigned swiz)
   {
      unsigned x = BRW_GET_SWZ(swiz, BRW_CHANNEL_X);
   unsigned y = BRW_GET_SWZ(swiz, BRW_CHANNEL_Y);
   unsigned z = BRW_GET_SWZ(swiz, BRW_CHANNEL_Z);
   unsigned w = BRW_GET_SWZ(swiz, BRW_CHANNEL_W);
            if (x == y && x == z && x == w) {
      string(file, ".");
      } else if (swiz != BRW_SWIZZLE_XYZW) {
      string(file, ".");
   err |= control(file, "channel select", chan_sel, x, NULL);
   err |= control(file, "channel select", chan_sel, y, NULL);
   err |= control(file, "channel select", chan_sel, z, NULL);
      }
      }
      static int
   src_da16(FILE *file,
            const struct intel_device_info *devinfo,
   unsigned opcode,
   enum brw_reg_type type,
   unsigned _reg_file,
   unsigned _vert_stride,
   unsigned _reg_nr,
   unsigned _subreg_nr,
   unsigned __abs,
      {
               if (devinfo->ver >= 8 && is_logic_instruction(opcode))
         else
                     err |= reg(file, _reg_file, _reg_nr);
   if (err == -1)
         if (_subreg_nr) {
               /* bit4 for subreg number byte addressing. Make this same meaning as
            }
   string(file, "<");
   err |= control(file, "vert stride", vert_stride, _vert_stride, NULL);
   string(file, ">");
   err |= src_swizzle(file, BRW_SWIZZLE4(swz_x, swz_y, swz_z, swz_w));
   string(file, brw_reg_type_to_letters(type));
      }
      static enum brw_vertical_stride
   vstride_from_align1_3src_vstride(const struct intel_device_info *devinfo,
         {
      switch (vstride) {
   case BRW_ALIGN1_3SRC_VERTICAL_STRIDE_0: return BRW_VERTICAL_STRIDE_0;
   case BRW_ALIGN1_3SRC_VERTICAL_STRIDE_2:
      if (devinfo->ver >= 12)
         else
      case BRW_ALIGN1_3SRC_VERTICAL_STRIDE_4: return BRW_VERTICAL_STRIDE_4;
   case BRW_ALIGN1_3SRC_VERTICAL_STRIDE_8: return BRW_VERTICAL_STRIDE_8;
   default:
            }
      static enum brw_horizontal_stride
   hstride_from_align1_3src_hstride(enum gfx10_align1_3src_src_horizontal_stride hstride)
   {
      switch (hstride) {
   case BRW_ALIGN1_3SRC_SRC_HORIZONTAL_STRIDE_0: return BRW_HORIZONTAL_STRIDE_0;
   case BRW_ALIGN1_3SRC_SRC_HORIZONTAL_STRIDE_1: return BRW_HORIZONTAL_STRIDE_1;
   case BRW_ALIGN1_3SRC_SRC_HORIZONTAL_STRIDE_2: return BRW_HORIZONTAL_STRIDE_2;
   case BRW_ALIGN1_3SRC_SRC_HORIZONTAL_STRIDE_4: return BRW_HORIZONTAL_STRIDE_4;
   default:
            }
      static enum brw_vertical_stride
   vstride_from_align1_3src_hstride(enum gfx10_align1_3src_src_horizontal_stride hstride)
   {
      switch (hstride) {
   case BRW_ALIGN1_3SRC_SRC_HORIZONTAL_STRIDE_0: return BRW_VERTICAL_STRIDE_0;
   case BRW_ALIGN1_3SRC_SRC_HORIZONTAL_STRIDE_1: return BRW_VERTICAL_STRIDE_1;
   case BRW_ALIGN1_3SRC_SRC_HORIZONTAL_STRIDE_2: return BRW_VERTICAL_STRIDE_2;
   case BRW_ALIGN1_3SRC_SRC_HORIZONTAL_STRIDE_4: return BRW_VERTICAL_STRIDE_4;
   default:
            }
      /* From "GFX10 Regioning Rules for Align1 Ternary Operations" in the
   * "Register Region Restrictions" documentation
   */
   static enum brw_width
   implied_width(enum brw_vertical_stride _vert_stride,
         {
      /* "1. Width is 1 when Vertical and Horizontal Strides are both zero." */
   if (_vert_stride == BRW_VERTICAL_STRIDE_0 &&
      _horiz_stride == BRW_HORIZONTAL_STRIDE_0) {
         /* "2. Width is equal to vertical stride when Horizontal Stride is zero." */
   } else if (_horiz_stride == BRW_HORIZONTAL_STRIDE_0) {
      switch (_vert_stride) {
   case BRW_VERTICAL_STRIDE_1: return BRW_WIDTH_1;
   case BRW_VERTICAL_STRIDE_2: return BRW_WIDTH_2;
   case BRW_VERTICAL_STRIDE_4: return BRW_WIDTH_4;
   case BRW_VERTICAL_STRIDE_8: return BRW_WIDTH_8;
   case BRW_VERTICAL_STRIDE_0:
   default:
               } else {
               /* "3. Width is equal to Vertical Stride/Horizontal Stride when both
   *     Strides are non-zero.
   *
   *  4. Vertical Stride must not be zero if Horizontal Stride is non-zero.
   *     This implies Vertical Stride is always greater than Horizontal
   *     Stride."
   *
   * Given these statements and the knowledge that the stride and width
   * values are encoded in logarithmic form, we can perform the division
   * by just subtracting.
   */
         }
      static int
   src0_3src(FILE *file, const struct intel_device_info *devinfo,
         {
      int err = 0;
   unsigned reg_nr, subreg_nr;
   enum brw_reg_file _file;
   enum brw_reg_type type;
   enum brw_vertical_stride _vert_stride;
   enum brw_width _width;
   enum brw_horizontal_stride _horiz_stride;
   bool is_scalar_region;
            if (devinfo->ver < 10 && is_align1)
            if (is_align1) {
      if (devinfo->ver >= 12 && !brw_inst_3src_a1_src0_is_imm(devinfo, inst)) {
         } else if (brw_inst_3src_a1_src0_reg_file(devinfo, inst) ==
               } else if (brw_inst_3src_a1_src0_type(devinfo, inst) ==
               } else {
      _file = BRW_IMMEDIATE_VALUE;
                  if (type == BRW_REGISTER_TYPE_W) {
         } else if (type == BRW_REGISTER_TYPE_UW) {
         } else if (type == BRW_REGISTER_TYPE_HF) {
         }
               reg_nr = brw_inst_3src_src0_reg_nr(devinfo, inst);
   subreg_nr = brw_inst_3src_a1_src0_subreg_nr(devinfo, inst);
   type = brw_inst_3src_a1_src0_type(devinfo, inst);
   _vert_stride = vstride_from_align1_3src_vstride(
         _horiz_stride = hstride_from_align1_3src_hstride(
            } else {
      _file = BRW_GENERAL_REGISTER_FILE;
   reg_nr = brw_inst_3src_src0_reg_nr(devinfo, inst);
   subreg_nr = brw_inst_3src_a16_src0_subreg_nr(devinfo, inst) * 4;
            if (brw_inst_3src_a16_src0_rep_ctrl(devinfo, inst)) {
      _vert_stride = BRW_VERTICAL_STRIDE_0;
   _width = BRW_WIDTH_1;
      } else {
      _vert_stride = BRW_VERTICAL_STRIDE_4;
   _width = BRW_WIDTH_4;
         }
   is_scalar_region = _vert_stride == BRW_VERTICAL_STRIDE_0 &&
                           err |= control(file, "negate", m_negate,
                  err |= reg(file, _file, reg_nr);
   if (err == -1)
         if (subreg_nr || is_scalar_region)
         src_align1_region(file, _vert_stride, _width, _horiz_stride);
   if (!is_scalar_region && !is_align1)
         string(file, brw_reg_type_to_letters(type));
      }
      static int
   src1_3src(FILE *file, const struct intel_device_info *devinfo,
         {
      int err = 0;
   unsigned reg_nr, subreg_nr;
   enum brw_reg_file _file;
   enum brw_reg_type type;
   enum brw_vertical_stride _vert_stride;
   enum brw_width _width;
   enum brw_horizontal_stride _horiz_stride;
   bool is_scalar_region;
            if (devinfo->ver < 10 && is_align1)
            if (is_align1) {
      if (devinfo->ver >= 12) {
         } else if (brw_inst_3src_a1_src1_reg_file(devinfo, inst) ==
               } else {
                  reg_nr = brw_inst_3src_src1_reg_nr(devinfo, inst);
   subreg_nr = brw_inst_3src_a1_src1_subreg_nr(devinfo, inst);
            _vert_stride = vstride_from_align1_3src_vstride(
         _horiz_stride = hstride_from_align1_3src_hstride(
            } else {
      _file = BRW_GENERAL_REGISTER_FILE;
   reg_nr = brw_inst_3src_src1_reg_nr(devinfo, inst);
   subreg_nr = brw_inst_3src_a16_src1_subreg_nr(devinfo, inst) * 4;
            if (brw_inst_3src_a16_src1_rep_ctrl(devinfo, inst)) {
      _vert_stride = BRW_VERTICAL_STRIDE_0;
   _width = BRW_WIDTH_1;
      } else {
      _vert_stride = BRW_VERTICAL_STRIDE_4;
   _width = BRW_WIDTH_4;
         }
   is_scalar_region = _vert_stride == BRW_VERTICAL_STRIDE_0 &&
                           err |= control(file, "negate", m_negate,
                  err |= reg(file, _file, reg_nr);
   if (err == -1)
         if (subreg_nr || is_scalar_region)
         src_align1_region(file, _vert_stride, _width, _horiz_stride);
   if (!is_scalar_region && !is_align1)
         string(file, brw_reg_type_to_letters(type));
      }
      static int
   src2_3src(FILE *file, const struct intel_device_info *devinfo,
         {
      int err = 0;
   unsigned reg_nr, subreg_nr;
   enum brw_reg_file _file;
   enum brw_reg_type type;
   enum brw_vertical_stride _vert_stride;
   enum brw_width _width;
   enum brw_horizontal_stride _horiz_stride;
   bool is_scalar_region;
            if (devinfo->ver < 10 && is_align1)
            if (is_align1) {
      if (devinfo->ver >= 12 && !brw_inst_3src_a1_src2_is_imm(devinfo, inst)) {
         } else if (brw_inst_3src_a1_src2_reg_file(devinfo, inst) ==
               } else {
      _file = BRW_IMMEDIATE_VALUE;
                  if (type == BRW_REGISTER_TYPE_W) {
         } else if (type == BRW_REGISTER_TYPE_UW) {
         } else if (type == BRW_REGISTER_TYPE_HF) {
         }
               reg_nr = brw_inst_3src_src2_reg_nr(devinfo, inst);
   subreg_nr = brw_inst_3src_a1_src2_subreg_nr(devinfo, inst);
   type = brw_inst_3src_a1_src2_type(devinfo, inst);
   /* FINISHME: No vertical stride on src2. Is using the hstride in place
   *           correct? Doesn't seem like it, since there's hstride=1 but
   *           no vstride=1.
   */
   _vert_stride = vstride_from_align1_3src_hstride(
         _horiz_stride = hstride_from_align1_3src_hstride(
            } else {
      _file = BRW_GENERAL_REGISTER_FILE;
   reg_nr = brw_inst_3src_src2_reg_nr(devinfo, inst);
   subreg_nr = brw_inst_3src_a16_src2_subreg_nr(devinfo, inst) * 4;
            if (brw_inst_3src_a16_src2_rep_ctrl(devinfo, inst)) {
      _vert_stride = BRW_VERTICAL_STRIDE_0;
   _width = BRW_WIDTH_1;
      } else {
      _vert_stride = BRW_VERTICAL_STRIDE_4;
   _width = BRW_WIDTH_4;
         }
   is_scalar_region = _vert_stride == BRW_VERTICAL_STRIDE_0 &&
                           err |= control(file, "negate", m_negate,
                  err |= reg(file, _file, reg_nr);
   if (err == -1)
         if (subreg_nr || is_scalar_region)
         src_align1_region(file, _vert_stride, _width, _horiz_stride);
   if (!is_scalar_region && !is_align1)
         string(file, brw_reg_type_to_letters(type));
      }
      static int
   imm(FILE *file, const struct brw_isa_info *isa, enum brw_reg_type type,
         {
               switch (type) {
   case BRW_REGISTER_TYPE_UQ:
      format(file, "0x%016"PRIx64"UQ", brw_inst_imm_uq(devinfo, inst));
      case BRW_REGISTER_TYPE_Q:
      format(file, "0x%016"PRIx64"Q", brw_inst_imm_uq(devinfo, inst));
      case BRW_REGISTER_TYPE_UD:
      format(file, "0x%08xUD", brw_inst_imm_ud(devinfo, inst));
      case BRW_REGISTER_TYPE_D:
      format(file, "%dD", brw_inst_imm_d(devinfo, inst));
      case BRW_REGISTER_TYPE_UW:
      format(file, "0x%04xUW", (uint16_t) brw_inst_imm_ud(devinfo, inst));
      case BRW_REGISTER_TYPE_W:
      format(file, "%dW", (int16_t) brw_inst_imm_d(devinfo, inst));
      case BRW_REGISTER_TYPE_UV:
      format(file, "0x%08xUV", brw_inst_imm_ud(devinfo, inst));
      case BRW_REGISTER_TYPE_VF:
      format(file, "0x%"PRIx64"VF", brw_inst_bits(inst, 127, 96));
   pad(file, 48);
   format(file, "/* [%-gF, %-gF, %-gF, %-gF]VF */",
         brw_vf_to_float(brw_inst_imm_ud(devinfo, inst)),
   brw_vf_to_float(brw_inst_imm_ud(devinfo, inst) >> 8),
   brw_vf_to_float(brw_inst_imm_ud(devinfo, inst) >> 16),
      case BRW_REGISTER_TYPE_V:
      format(file, "0x%08xV", brw_inst_imm_ud(devinfo, inst));
      case BRW_REGISTER_TYPE_F:
      /* The DIM instruction's src0 uses an F type but contains a
   * 64-bit immediate
   */
   if (brw_inst_opcode(isa, inst) == BRW_OPCODE_DIM) {
      format(file, "0x%"PRIx64"F", brw_inst_bits(inst, 127, 64));
   pad(file, 48);
      } else {
      format(file, "0x%"PRIx64"F", brw_inst_bits(inst, 127, 96));
   pad(file, 48);
      }
      case BRW_REGISTER_TYPE_DF:
      format(file, "0x%016"PRIx64"DF", brw_inst_imm_uq(devinfo, inst));
   pad(file, 48);
   format(file, "/* %-gDF */", brw_inst_imm_df(devinfo, inst));
      case BRW_REGISTER_TYPE_HF:
      string(file, "Half Float IMM");
      case BRW_REGISTER_TYPE_NF:
   case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_B:
         }
      }
      static int
   src_sends_da(FILE *file,
               const struct intel_device_info *devinfo,
   enum brw_reg_type type,
   enum brw_reg_file _reg_file,
   {
               err |= reg(file, _reg_file, _reg_nr);
   if (err == -1)
         if (_reg_subnr)
                     }
      static int
   src_sends_ia(FILE *file,
               const struct intel_device_info *devinfo,
   enum brw_reg_type type,
   {
      string(file, "g[a0");
   if (_addr_subreg_nr)
         if (_addr_imm)
         string(file, "]");
               }
      static int
   src_send_desc_ia(FILE *file,
               {
      string(file, "a0");
   if (_addr_subreg_nr)
                     }
      static int
   src0(FILE *file, const struct brw_isa_info *isa, const brw_inst *inst)
   {
               if (is_split_send(devinfo, brw_inst_opcode(isa, inst))) {
      if (devinfo->ver >= 12) {
      return src_sends_da(file,
                     devinfo,
      } else if (brw_inst_send_src0_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT) {
      return src_sends_da(file,
                     devinfo,
      } else {
      return src_sends_ia(file,
                           } else if (brw_inst_src0_reg_file(devinfo, inst) == BRW_IMMEDIATE_VALUE) {
         } else if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      if (brw_inst_src0_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT) {
      return src_da1(file,
                  devinfo,
   brw_inst_opcode(isa, inst),
   brw_inst_src0_type(devinfo, inst),
   brw_inst_src0_reg_file(devinfo, inst),
   brw_inst_src0_vstride(devinfo, inst),
   brw_inst_src0_width(devinfo, inst),
   brw_inst_src0_hstride(devinfo, inst),
   brw_inst_src0_da_reg_nr(devinfo, inst),
   } else {
      return src_ia1(file,
                  devinfo,
   brw_inst_opcode(isa, inst),
   brw_inst_src0_type(devinfo, inst),
   brw_inst_src0_ia1_addr_imm(devinfo, inst),
   brw_inst_src0_ia_subreg_nr(devinfo, inst),
   brw_inst_src0_negate(devinfo, inst),
   brw_inst_src0_abs(devinfo, inst),
      } else {
      if (brw_inst_src0_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT) {
      return src_da16(file,
                  devinfo,
   brw_inst_opcode(isa, inst),
   brw_inst_src0_type(devinfo, inst),
   brw_inst_src0_reg_file(devinfo, inst),
   brw_inst_src0_vstride(devinfo, inst),
   brw_inst_src0_da_reg_nr(devinfo, inst),
   brw_inst_src0_da16_subreg_nr(devinfo, inst),
   brw_inst_src0_abs(devinfo, inst),
   brw_inst_src0_negate(devinfo, inst),
   brw_inst_src0_da16_swiz_x(devinfo, inst),
   } else {
      string(file, "Indirect align16 address mode not supported");
            }
      static int
   src1(FILE *file, const struct brw_isa_info *isa, const brw_inst *inst)
   {
               if (is_split_send(devinfo, brw_inst_opcode(isa, inst))) {
      return src_sends_da(file,
                     devinfo,
      } else if (brw_inst_src1_reg_file(devinfo, inst) == BRW_IMMEDIATE_VALUE) {
         } else if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      if (brw_inst_src1_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT) {
      return src_da1(file,
                  devinfo,
   brw_inst_opcode(isa, inst),
   brw_inst_src1_type(devinfo, inst),
   brw_inst_src1_reg_file(devinfo, inst),
   brw_inst_src1_vstride(devinfo, inst),
   brw_inst_src1_width(devinfo, inst),
   brw_inst_src1_hstride(devinfo, inst),
   brw_inst_src1_da_reg_nr(devinfo, inst),
   } else {
      return src_ia1(file,
                  devinfo,
   brw_inst_opcode(isa, inst),
   brw_inst_src1_type(devinfo, inst),
   brw_inst_src1_ia1_addr_imm(devinfo, inst),
   brw_inst_src1_ia_subreg_nr(devinfo, inst),
   brw_inst_src1_negate(devinfo, inst),
   brw_inst_src1_abs(devinfo, inst),
      } else {
      if (brw_inst_src1_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT) {
      return src_da16(file,
                  devinfo,
   brw_inst_opcode(isa, inst),
   brw_inst_src1_type(devinfo, inst),
   brw_inst_src1_reg_file(devinfo, inst),
   brw_inst_src1_vstride(devinfo, inst),
   brw_inst_src1_da_reg_nr(devinfo, inst),
   brw_inst_src1_da16_subreg_nr(devinfo, inst),
   brw_inst_src1_abs(devinfo, inst),
   brw_inst_src1_negate(devinfo, inst),
   brw_inst_src1_da16_swiz_x(devinfo, inst),
   } else {
      string(file, "Indirect align16 address mode not supported");
            }
      static int
   qtr_ctrl(FILE *file, const struct intel_device_info *devinfo,
         {
      int qtr_ctl = brw_inst_qtr_control(devinfo, inst);
   int exec_size = 1 << brw_inst_exec_size(devinfo, inst);
   const unsigned nib_ctl = devinfo->ver < 7 ? 0 :
            if (exec_size < 8 || nib_ctl) {
         } else if (exec_size == 8) {
      switch (qtr_ctl) {
   case 0:
      string(file, " 1Q");
      case 1:
      string(file, " 2Q");
      case 2:
      string(file, " 3Q");
      case 3:
      string(file, " 4Q");
         } else if (exec_size == 16) {
      if (qtr_ctl < 2)
         else
      }
      }
      static bool
   inst_has_type(const struct brw_isa_info *isa,
               {
      const struct intel_device_info *devinfo = isa->devinfo;
            if (brw_inst_dst_type(devinfo, inst) == type)
            if (num_sources >= 3) {
      if (brw_inst_3src_access_mode(devinfo, inst) == BRW_ALIGN_1)
      return brw_inst_3src_a1_src0_type(devinfo, inst) == type ||
            else
      } else if (num_sources == 2) {
      return brw_inst_src0_type(devinfo, inst) == type ||
      } else {
            }
      static int
   swsb(FILE *file, const struct brw_isa_info *isa, const brw_inst *inst)
   {
      const struct intel_device_info *devinfo = isa->devinfo;
   const enum opcode opcode = brw_inst_opcode(isa, inst);
   const uint8_t x = brw_inst_swsb(devinfo, inst);
   const bool is_unordered =
      opcode == BRW_OPCODE_SEND || opcode == BRW_OPCODE_SENDC ||
   opcode == BRW_OPCODE_MATH ||
   (devinfo->has_64bit_float_via_math_pipe &&
      const struct tgl_swsb swsb = tgl_swsb_decode(devinfo, is_unordered, x);
   if (swsb.regdist)
      format(file, " %s@%d",
         (swsb.pipe == TGL_PIPE_FLOAT ? "F" :
   swsb.pipe == TGL_PIPE_INT ? "I" :
   swsb.pipe == TGL_PIPE_LONG ? "L" :
      if (swsb.mode)
      format(file, " $%d%s", swsb.sbid,
               }
      #ifdef DEBUG
   static __attribute__((__unused__)) int
   brw_disassemble_imm(const struct brw_isa_info *isa,
         {
      brw_inst inst;
   inst.data[0] = (((uint64_t) dw1) << 32) | ((uint64_t) dw0);
   inst.data[1] = (((uint64_t) dw3) << 32) | ((uint64_t) dw2);
      }
   #endif
      static void
   write_label(FILE *file, const struct intel_device_info *devinfo,
               {
      if (root_label != NULL) {
      int to_bytes_scale = sizeof(brw_inst) / brw_jump_scale(devinfo);
   const struct brw_label *label =
         if (label != NULL) {
               }
      static void
   lsc_disassemble_ex_desc(const struct intel_device_info *devinfo,
                     {
      const unsigned addr_type = lsc_msg_desc_addr_type(devinfo, imm_desc);
   switch (addr_type) {
   case LSC_ADDR_SURFTYPE_FLAT:
      format(file, " base_offset %u ",
            case LSC_ADDR_SURFTYPE_BSS:
   case LSC_ADDR_SURFTYPE_SS:
      format(file, " surface_state_index %u ",
            case LSC_ADDR_SURFTYPE_BTI:
      format(file, " BTI %u ",
         format(file, " base_offset %u ",
            default:
      format(file, "unsupported address surface type %d", addr_type);
         }
      static inline bool
   brw_sfid_is_lsc(unsigned sfid)
   {
      switch (sfid) {
   case GFX12_SFID_UGM:
   case GFX12_SFID_SLM:
   case GFX12_SFID_TGM:
         default:
                     }
      int
   brw_disassemble_inst(FILE *file, const struct brw_isa_info *isa,
               {
               int err = 0;
            const enum opcode opcode = brw_inst_opcode(isa, inst);
            if (brw_inst_pred_control(devinfo, inst)) {
      string(file, "(");
   err |= control(file, "predicate inverse", pred_inv,
         format(file, "f%"PRIu64".%"PRIu64,
         devinfo->ver >= 7 ? brw_inst_flag_reg_nr(devinfo, inst) : 0,
   if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      err |= control(file, "predicate control align1", pred_ctrl_align1,
      } else {
      err |= control(file, "predicate control align16", pred_ctrl_align16,
      }
                        if (!is_send(opcode))
      err |= control(file, "saturate", saturate, brw_inst_saturate(devinfo, inst),
         err |= control(file, "debug control", debug_ctrl,
            if (opcode == BRW_OPCODE_MATH) {
      string(file, " ");
   err |= control(file, "function", math_function,
         } else if (opcode == BRW_OPCODE_SYNC) {
      string(file, " ");
   err |= control(file, "function", sync_function,
         } else if (!is_send(opcode) &&
            (devinfo->ver < 12 ||
         err |= control(file, "conditional modifier", conditional_modifier,
            /* If we're using the conditional modifier, print which flags reg is
   * used for it.  Note that on gfx6+, the embedded-condition SEL and
   * control flow doesn't update flags.
   */
   if (brw_inst_cond_modifier(devinfo, inst) &&
      (devinfo->ver < 6 || (opcode != BRW_OPCODE_SEL &&
                     format(file, ".f%"PRIu64".%"PRIu64,
                        if (opcode != BRW_OPCODE_NOP && opcode != BRW_OPCODE_NENOP) {
      string(file, "(");
   err |= control(file, "execution size", exec_size,
                     if (opcode == BRW_OPCODE_SEND && devinfo->ver < 6)
            if (brw_has_uip(devinfo, opcode)) {
      /* Instructions that have UIP also have JIP. */
   pad(file, 16);
   string(file, "JIP: ");
            pad(file, 38);
   string(file, "UIP: ");
      } else if (brw_has_jip(devinfo, opcode)) {
      int jip;
   if (devinfo->ver >= 7) {
         } else {
                  pad(file, 16);
   string(file, "JIP: ");
      } else if (devinfo->ver < 6 && (opcode == BRW_OPCODE_BREAK ||
                  pad(file, 16);
   format(file, "Jump: %d", brw_inst_gfx4_jump_count(devinfo, inst));
   pad(file, 32);
      } else if (devinfo->ver < 6 && (opcode == BRW_OPCODE_IF ||
                        pad(file, 16);
      } else if (devinfo->ver < 6 && opcode == BRW_OPCODE_ENDIF) {
      pad(file, 16);
      } else if (opcode == BRW_OPCODE_JMPI) {
      pad(file, 16);
      } else if (desc && desc->nsrc == 3) {
      pad(file, 16);
            pad(file, 32);
            pad(file, 48);
            pad(file, 64);
      } else if (desc) {
      if (desc->ndst > 0) {
      pad(file, 16);
               if (desc->nsrc > 0) {
      pad(file, 32);
               if (desc->nsrc > 1) {
      pad(file, 48);
                  if (is_send(opcode)) {
               bool has_imm_desc = false, has_imm_ex_desc = false;
   uint32_t imm_desc = 0, imm_ex_desc = 0;
   if (is_split_send(devinfo, opcode)) {
      pad(file, 64);
   if (brw_inst_send_sel_reg32_desc(devinfo, inst)) {
      /* show the indirect descriptor source */
      } else {
      has_imm_desc = true;
   imm_desc = brw_inst_send_desc(devinfo, inst);
               pad(file, 80);
   if (brw_inst_send_sel_reg32_ex_desc(devinfo, inst)) {
      /* show the indirect descriptor source */
   err |= src_send_desc_ia(file, devinfo,
      } else {
      has_imm_ex_desc = true;
   imm_ex_desc = brw_inst_sends_ex_desc(devinfo, inst);
         } else {
      if (brw_inst_src1_reg_file(devinfo, inst) != BRW_IMMEDIATE_VALUE) {
      /* show the indirect descriptor source */
   pad(file, 48);
   err |= src1(file, isa, inst);
      } else {
      has_imm_desc = true;
   imm_desc = brw_inst_send_desc(devinfo, inst);
               /* Print message descriptor as immediate source */
               newline(file);
   pad(file, 16);
            fprintf(file, "            ");
   err |= control(file, "SFID", devinfo->ver >= 6 ? gfx6_sfid : gfx4_sfid,
                  if (!has_imm_desc) {
         } else {
      bool unsupported = false;
   switch (sfid) {
   case BRW_SFID_MATH:
      err |= control(file, "math function", math_function,
         err |= control(file, "math saturate", math_saturate,
         err |= control(file, "math signed", math_signed,
         err |= control(file, "math scalar", math_scalar,
         err |= control(file, "math precision", math_precision,
            case BRW_SFID_SAMPLER:
      if (devinfo->ver >= 5) {
      err |= control(file, "sampler message", gfx5_sampler_msg_type,
               err |= control(file, "sampler simd mode", gfx5_sampler_simd_mode,
               if (devinfo->ver >= 8 &&
      brw_sampler_desc_return_format(devinfo, imm_desc)) {
      }
   format(file, " Surface = %u Sampler = %u",
            } else {
      format(file, " (bti %u, sampler %u, msg_type %u, ",
         brw_sampler_desc_binding_table_index(devinfo, imm_desc),
   brw_sampler_desc_sampler(devinfo, imm_desc),
   if (devinfo->verx10 != 45) {
      err |= control(file, "sampler target format",
                  }
      }
      case GFX6_SFID_DATAPORT_SAMPLER_CACHE:
   case GFX6_SFID_DATAPORT_CONSTANT_CACHE:
      /* aka BRW_SFID_DATAPORT_READ on Gfx4-5 */
   if (devinfo->ver >= 6) {
      format(file, " (bti %u, msg_ctrl %u, msg_type %u, write_commit %u)",
         brw_dp_desc_binding_table_index(devinfo, imm_desc),
   brw_dp_desc_msg_control(devinfo, imm_desc),
   brw_dp_desc_msg_type(devinfo, imm_desc),
      } else {
      bool is_965 = devinfo->verx10 == 40;
   err |= control(file, "DP read message type",
                                             format(file, " Surface = %u",
                  case GFX6_SFID_DATAPORT_RENDER_CACHE: {
                                    bool is_rt_write = msg_type ==
                  if (is_rt_write) {
      err |= control(file, "RT message type", m_rt_write_subtype,
         if (devinfo->ver >= 6 && brw_inst_rt_slot_group(devinfo, inst))
         if (brw_fb_write_desc_last_render_target(devinfo, imm_desc))
         if (devinfo->ver >= 10 &&
      brw_fb_write_desc_coarse_write(devinfo, imm_desc))
      if (devinfo->ver < 7 &&
      brw_fb_write_desc_write_commit(devinfo, imm_desc))
   } else {
                        format(file, " Surface = %u",
                     case BRW_SFID_URB: {
      if (devinfo->ver >= 20) {
      format(file, " (");
   const enum lsc_opcode op = lsc_msg_desc_opcode(devinfo, imm_desc);
   err |= control(file, "operation", lsc_operation,
         format(file, ",");
                        format(file, ",");
   err |= control(file, "data_size", lsc_data_size,
               format(file, ",");
   if (lsc_opcode_has_cmask(op)) {
      err |= control(file, "component_mask",
                  } else {
      err |= control(file, "vector_size",
                     if (lsc_msg_desc_transpose(devinfo, imm_desc))
      }
   switch(op) {
   case LSC_OP_LOAD_CMASK:
   case LSC_OP_LOAD:
      format(file, ",");
   err |= control(file, "cache_load",
                        default:
      format(file, ",");
   err |= control(file, "cache_store",
                                 format(file, " dst_len = %u,", lsc_msg_desc_dest_len(devinfo, imm_desc));
   format(file, " src0_len = %u,", lsc_msg_desc_src0_len(devinfo, imm_desc));
   format(file, " src1_len = %d", brw_message_ex_desc_ex_mlen(devinfo, imm_ex_desc));
   err |= control(file, "address_type", lsc_addr_surface_type,
                                                err |= control(file, "urb opcode",
                        if (devinfo->ver >= 7 &&
                        if (urb_opcode == GFX8_URB_OPCODE_SIMD8_WRITE ||
      urb_opcode == GFX8_URB_OPCODE_SIMD8_READ) {
   if (brw_inst_urb_channel_mask_present(devinfo, inst))
      } else if (urb_opcode != GFX125_URB_OPCODE_FENCE) {
      err |= control(file, "urb swizzle", urb_swizzle,
                     if (devinfo->ver < 7) {
      err |= control(file, "urb allocate", urb_allocate,
         err |= control(file, "urb used", urb_used,
      }
   if (devinfo->ver < 8) {
      err |= control(file, "urb complete", urb_complete,
         }
      }
                  case BRW_SFID_MESSAGE_GATEWAY:
      format(file, " (%s)",
               case GFX12_SFID_SLM:
   case GFX12_SFID_TGM:
   case GFX12_SFID_UGM: {
      assert(devinfo->has_lsc);
   format(file, " (");
   const enum lsc_opcode op = lsc_msg_desc_opcode(devinfo, imm_desc);
   err |= control(file, "operation", lsc_operation,
         format(file, ",");
   err |= control(file, "addr_size", lsc_addr_size,
                  if (op == LSC_OP_FENCE) {
      format(file, ",");
   err |= control(file, "scope", lsc_fence_scope,
               format(file, ",");
   err |= control(file, "flush_type", lsc_flush_type,
               format(file, ",");
   err |= control(file, "backup_mode_fence_routing",
                  } else {
      format(file, ",");
   err |= control(file, "data_size", lsc_data_size,
               format(file, ",");
   if (lsc_opcode_has_cmask(op)) {
      err |= control(file, "component_mask",
                  } else {
      err |= control(file, "vector_size",
                     if (lsc_msg_desc_transpose(devinfo, imm_desc))
      }
   switch(op) {
   case LSC_OP_LOAD_CMASK:
   case LSC_OP_LOAD:
      format(file, ",");
   err |= control(file, "cache_load",
                        default:
      format(file, ",");
   err |= control(file, "cache_store",
                           }
   format(file, " dst_len = %u,", lsc_msg_desc_dest_len(devinfo, imm_desc));
   format(file, " src0_len = %u,", lsc_msg_desc_src0_len(devinfo, imm_desc));
   format(file, " src1_len = %d", brw_message_ex_desc_ex_mlen(devinfo, imm_ex_desc));
   err |= control(file, "address_type", lsc_addr_surface_type,
         format(file, " )");
               case GFX7_SFID_DATAPORT_DATA_CACHE:
      if (devinfo->ver >= 7) {
                                                         switch (brw_inst_dp_msg_type(devinfo, inst)) {
   case GFX7_DATAPORT_DC_UNTYPED_ATOMIC_OP:
      control(file, "atomic op", aop,
         brw_dp_desc_msg_control(devinfo, imm_desc) & 0xf,
      case GFX7_DATAPORT_DC_OWORD_BLOCK_READ:
   case GFX7_DATAPORT_DC_OWORD_BLOCK_WRITE: {
      unsigned msg_ctrl = brw_dp_desc_msg_control(devinfo, imm_desc);
   assert(dp_oword_block_rw[msg_ctrl & 7]);
   format(file, "owords = %s, aligned = %d",
            }
   default:
      format(file, "%u",
      }
      } else {
                     case HSW_SFID_DATAPORT_DATA_CACHE_1: {
      if (devinfo->ver >= 7) {
                                                                  switch (brw_inst_dp_msg_type(devinfo, inst)) {
   case HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP:
   case HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP:
   case HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP:
      format(file, "SIMD%d,", (msg_ctrl & (1 << 4)) ? 8 : 16);
      case HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP_SIMD4X2:
   case HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP_SIMD4X2:
   case HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP_SIMD4X2:
   case GFX8_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_OP:
   case GFX12_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_HALF_INT_OP:
      control(file, "atomic op", aop, msg_ctrl & 0xf, &space);
      case HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_READ:
   case HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_WRITE:
   case HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_READ:
   case HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_WRITE:
   case GFX8_DATAPORT_DC_PORT1_A64_UNTYPED_SURFACE_WRITE:
   case GFX8_DATAPORT_DC_PORT1_A64_UNTYPED_SURFACE_READ: {
      static const char *simd_modes[] = { "4x2", "16", "8" };
   format(file, "SIMD%s, Mask = 0x%x",
            }
   case GFX9_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_FLOAT_OP:
   case GFX9_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_FLOAT_OP:
   case GFX12_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_HALF_FLOAT_OP:
      format(file, "SIMD%d,", (msg_ctrl & (1 << 4)) ? 8 : 16);
   control(file, "atomic float op", aop_float, msg_ctrl & 0xf,
            case GFX9_DATAPORT_DC_PORT1_A64_OWORD_BLOCK_WRITE:
   case GFX9_DATAPORT_DC_PORT1_A64_OWORD_BLOCK_READ:
      assert(dp_oword_block_rw[msg_ctrl & 7]);
   format(file, "owords = %s, aligned = %d",
            default:
         }
      } else {
         }
               case GFX7_SFID_PIXEL_INTERPOLATOR:
      if (devinfo->ver >= 7) {
      format(file, " (%s, %s, 0x%02"PRIx64")",
         brw_inst_pi_nopersp(devinfo, inst) ? "linear" : "persp",
      } else {
                     case GEN_RT_SFID_RAY_TRACE_ACCELERATOR:
      if (devinfo->has_ray_tracing) {
      format(file, " SIMD%d,",
      } else {
                     default:
      unsupported = true;
                              if (space)
      }
   if (devinfo->verx10 >= 125 && brw_inst_send_ex_bso(devinfo, inst))
         if (brw_sfid_is_lsc(sfid)) {
         } else {
      if (has_imm_desc)
         if (has_imm_ex_desc) {
      format(file, " ex_mlen %u",
      }
   if (has_imm_desc)
         }
   pad(file, 64);
   if (opcode != BRW_OPCODE_NOP && opcode != BRW_OPCODE_NENOP) {
      string(file, "{");
   space = 1;
   err |= control(file, "access mode", access_mode,
         if (devinfo->ver >= 6) {
      err |= control(file, "write enable control", wectrl,
      } else {
      err |= control(file, "mask control", mask_ctrl,
               if (devinfo->ver < 12) {
      err |= control(file, "dependency control", dep_ctrl,
                     if (devinfo->ver >= 6)
         else {
      if (brw_inst_qtr_control(devinfo, inst) == BRW_COMPRESSION_COMPRESSED &&
      desc && desc->ndst > 0 &&
   brw_inst_dst_reg_file(devinfo, inst) == BRW_MESSAGE_REGISTER_FILE &&
   brw_inst_dst_da_reg_nr(devinfo, inst) & BRW_MRF_COMPR4) {
      } else {
      err |= control(file, "compression control", compr_ctrl,
                  if (devinfo->ver >= 12)
            err |= control(file, "compaction", cmpt_ctrl, is_compacted, &space);
   err |= control(file, "thread control", thread_ctrl,
                     if (has_branch_ctrl(devinfo, opcode)) {
      err |= control(file, "branch ctrl", branch_ctrl,
      } else if (devinfo->ver >= 6) {
      err |= control(file, "acc write control", accwr,
      }
   if (is_send(opcode))
      err |= control(file, "end of thread", end_of_thread,
      if (space)
            }
   string(file, ";");
   newline(file);
      }
