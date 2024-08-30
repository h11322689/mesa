   /*
   * Copyright © 2016 Broadcom
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
      #include <string.h>
   #include "util/macros.h"
   #include "util/bitscan.h"
      #include "broadcom/common/v3d_device_info.h"
   #include "qpu_instr.h"
      #ifndef QPU_MASK
   #define QPU_MASK(high, low) ((((uint64_t)1<<((high)-(low)+1))-1)<<(low))
   /* Using the GNU statement expression extension */
   #define QPU_SET_FIELD(value, field)                                       \
         ({                                                                \
            uint64_t fieldval = (uint64_t)(value) << field ## _SHIFT; \
         #define QPU_GET_FIELD(word, field) ((uint32_t)(((word)  & field ## _MASK) >> field ## _SHIFT))
      #define QPU_UPDATE_FIELD(inst, value, field)                              \
         #endif /* QPU_MASK */
      #define V3D_QPU_OP_MUL_SHIFT                58
   #define V3D_QPU_OP_MUL_MASK                 QPU_MASK(63, 58)
      #define V3D_QPU_SIG_SHIFT                   53
   #define V3D_QPU_SIG_MASK                    QPU_MASK(57, 53)
      #define V3D_QPU_COND_SHIFT                  46
   #define V3D_QPU_COND_MASK                   QPU_MASK(52, 46)
   #define V3D_QPU_COND_SIG_MAGIC_ADDR         (1 << 6)
      #define V3D_QPU_MM                          QPU_MASK(45, 45)
   #define V3D_QPU_MA                          QPU_MASK(44, 44)
      #define V3D_QPU_WADDR_M_SHIFT               38
   #define V3D_QPU_WADDR_M_MASK                QPU_MASK(43, 38)
      #define V3D_QPU_BRANCH_ADDR_LOW_SHIFT       35
   #define V3D_QPU_BRANCH_ADDR_LOW_MASK        QPU_MASK(55, 35)
      #define V3D_QPU_WADDR_A_SHIFT               32
   #define V3D_QPU_WADDR_A_MASK                QPU_MASK(37, 32)
      #define V3D_QPU_BRANCH_COND_SHIFT           32
   #define V3D_QPU_BRANCH_COND_MASK            QPU_MASK(34, 32)
      #define V3D_QPU_BRANCH_ADDR_HIGH_SHIFT      24
   #define V3D_QPU_BRANCH_ADDR_HIGH_MASK       QPU_MASK(31, 24)
      #define V3D_QPU_OP_ADD_SHIFT                24
   #define V3D_QPU_OP_ADD_MASK                 QPU_MASK(31, 24)
      #define V3D_QPU_MUL_B_SHIFT                 21
   #define V3D_QPU_MUL_B_MASK                  QPU_MASK(23, 21)
      #define V3D_QPU_BRANCH_MSFIGN_SHIFT         21
   #define V3D_QPU_BRANCH_MSFIGN_MASK          QPU_MASK(22, 21)
      #define V3D_QPU_MUL_A_SHIFT                 18
   #define V3D_QPU_MUL_A_MASK                  QPU_MASK(20, 18)
      #define V3D_QPU_RADDR_C_SHIFT               18
   #define V3D_QPU_RADDR_C_MASK                QPU_MASK(23, 18)
      #define V3D_QPU_ADD_B_SHIFT                 15
   #define V3D_QPU_ADD_B_MASK                  QPU_MASK(17, 15)
      #define V3D_QPU_BRANCH_BDU_SHIFT            15
   #define V3D_QPU_BRANCH_BDU_MASK             QPU_MASK(17, 15)
      #define V3D_QPU_BRANCH_UB                   QPU_MASK(14, 14)
      #define V3D_QPU_ADD_A_SHIFT                 12
   #define V3D_QPU_ADD_A_MASK                  QPU_MASK(14, 12)
      #define V3D_QPU_BRANCH_BDI_SHIFT            12
   #define V3D_QPU_BRANCH_BDI_MASK             QPU_MASK(13, 12)
      #define V3D_QPU_RADDR_D_SHIFT               12
   #define V3D_QPU_RADDR_D_MASK                QPU_MASK(17, 12)
      #define V3D_QPU_RADDR_A_SHIFT               6
   #define V3D_QPU_RADDR_A_MASK                QPU_MASK(11, 6)
      #define V3D_QPU_RADDR_B_SHIFT               0
   #define V3D_QPU_RADDR_B_MASK                QPU_MASK(5, 0)
      #define THRSW .thrsw = true
   #define LDUNIF .ldunif = true
   #define LDUNIFRF .ldunifrf = true
   #define LDUNIFA .ldunifa = true
   #define LDUNIFARF .ldunifarf = true
   #define LDTMU .ldtmu = true
   #define LDVARY .ldvary = true
   #define LDVPM .ldvpm = true
   #define LDTLB .ldtlb = true
   #define LDTLBU .ldtlbu = true
   #define UCB .ucb = true
   #define ROT .rotate = true
   #define WRTMUC .wrtmuc = true
   #define SMIMM_A .small_imm_a = true
   #define SMIMM_B .small_imm_b = true
   #define SMIMM_C .small_imm_c = true
   #define SMIMM_D .small_imm_d = true
      static const struct v3d_qpu_sig v33_sig_map[] = {
         /*      MISC   R3       R4      R5 */
   [0]  = {                               },
   [1]  = { THRSW,                        },
   [2]  = {                        LDUNIF },
   [3]  = { THRSW,                 LDUNIF },
   [4]  = {                LDTMU,         },
   [5]  = { THRSW,         LDTMU,         },
   [6]  = {                LDTMU,  LDUNIF },
   [7]  = { THRSW,         LDTMU,  LDUNIF },
   [8]  = {        LDVARY,                },
   [9]  = { THRSW, LDVARY,                },
   [10] = {        LDVARY,         LDUNIF },
   [11] = { THRSW, LDVARY,         LDUNIF },
   [12] = {        LDVARY, LDTMU,         },
   [13] = { THRSW, LDVARY, LDTMU,         },
   [14] = { SMIMM_B, LDVARY,              },
   [15] = { SMIMM_B,                      },
   [16] = {        LDTLB,                 },
   [17] = {        LDTLBU,                },
   /* 18-21 reserved */
   [22] = { UCB,                          },
   [23] = { ROT,                          },
   [24] = {        LDVPM,                 },
   [25] = { THRSW, LDVPM,                 },
   [26] = {        LDVPM,          LDUNIF },
   [27] = { THRSW, LDVPM,          LDUNIF },
   [28] = {        LDVPM, LDTMU,          },
   [29] = { THRSW, LDVPM, LDTMU,          },
   [30] = { SMIMM_B, LDVPM,               },
   };
      static const struct v3d_qpu_sig v40_sig_map[] = {
         /*      MISC    R3      R4      R5 */
   [0]  = {                               },
   [1]  = { THRSW,                        },
   [2]  = {                        LDUNIF },
   [3]  = { THRSW,                 LDUNIF },
   [4]  = {                LDTMU,         },
   [5]  = { THRSW,         LDTMU,         },
   [6]  = {                LDTMU,  LDUNIF },
   [7]  = { THRSW,         LDTMU,  LDUNIF },
   [8]  = {        LDVARY,                },
   [9]  = { THRSW, LDVARY,                },
   [10] = {        LDVARY,         LDUNIF },
   [11] = { THRSW, LDVARY,         LDUNIF },
   /* 12-13 reserved */
   [14] = { SMIMM_B, LDVARY,              },
   [15] = { SMIMM_B,                      },
   [16] = {        LDTLB,                 },
   [17] = {        LDTLBU,                },
   [18] = {                        WRTMUC },
   [19] = { THRSW,                 WRTMUC },
   [20] = {        LDVARY,         WRTMUC },
   [21] = { THRSW, LDVARY,         WRTMUC },
   [22] = { UCB,                          },
   [23] = { ROT,                          },
   /* 24-30 reserved */
   };
      static const struct v3d_qpu_sig v41_sig_map[] = {
         /*      MISC       phys    R5 */
   [0]  = {                          },
   [1]  = { THRSW,                   },
   [2]  = {                   LDUNIF },
   [3]  = { THRSW,            LDUNIF },
   [4]  = {           LDTMU,         },
   [5]  = { THRSW,    LDTMU,         },
   [6]  = {           LDTMU,  LDUNIF },
   [7]  = { THRSW,    LDTMU,  LDUNIF },
   [8]  = {           LDVARY,        },
   [9]  = { THRSW,    LDVARY,        },
   [10] = {           LDVARY, LDUNIF },
   [11] = { THRSW,    LDVARY, LDUNIF },
   [12] = { LDUNIFRF                 },
   [13] = { THRSW,    LDUNIFRF       },
   [14] = { SMIMM_B,    LDVARY       },
   [15] = { SMIMM_B,                 },
   [16] = {           LDTLB,         },
   [17] = {           LDTLBU,        },
   [18] = {                          WRTMUC },
   [19] = { THRSW,                   WRTMUC },
   [20] = {           LDVARY,        WRTMUC },
   [21] = { THRSW,    LDVARY,        WRTMUC },
   [22] = { UCB,                     },
   [23] = { ROT,                     },
   [24] = {                   LDUNIFA},
   [25] = { LDUNIFARF                },
   /* 26-30 reserved */
   };
         static const struct v3d_qpu_sig v71_sig_map[] = {
         /*      MISC       phys    RF0 */
   [0]  = {                          },
   [1]  = { THRSW,                   },
   [2]  = {                   LDUNIF },
   [3]  = { THRSW,            LDUNIF },
   [4]  = {           LDTMU,         },
   [5]  = { THRSW,    LDTMU,         },
   [6]  = {           LDTMU,  LDUNIF },
   [7]  = { THRSW,    LDTMU,  LDUNIF },
   [8]  = {           LDVARY,        },
   [9]  = { THRSW,    LDVARY,        },
   [10] = {           LDVARY, LDUNIF },
   [11] = { THRSW,    LDVARY, LDUNIF },
   [12] = { LDUNIFRF                 },
   [13] = { THRSW,    LDUNIFRF       },
   [14] = { SMIMM_A,                 },
   [15] = { SMIMM_B,                 },
   [16] = {           LDTLB,         },
   [17] = {           LDTLBU,        },
   [18] = {                          WRTMUC },
   [19] = { THRSW,                   WRTMUC },
   [20] = {           LDVARY,        WRTMUC },
   [21] = { THRSW,    LDVARY,        WRTMUC },
   [22] = { UCB,                     },
   /* 23 reserved */
   [24] = {                   LDUNIFA},
   [25] = { LDUNIFARF                },
   /* 26-29 reserved */
   [30] = { SMIMM_C,                 },
   };
      bool
   v3d_qpu_sig_unpack(const struct v3d_device_info *devinfo,
               {
         if (packed_sig >= ARRAY_SIZE(v33_sig_map))
            if (devinfo->ver >= 71)
         else if (devinfo->ver >= 41)
         else if (devinfo->ver == 40)
         else
            /* Signals with zeroed unpacked contents after element 0 are reserved. */
   return (packed_sig == 0 ||
   }
      bool
   v3d_qpu_sig_pack(const struct v3d_device_info *devinfo,
               {
                  if (devinfo->ver >= 71)
         else if (devinfo->ver >= 41)
         else if (devinfo->ver == 40)
         else
            for (int i = 0; i < ARRAY_SIZE(v33_sig_map); i++) {
            if (memcmp(&map[i], sig, sizeof(*sig)) == 0) {
                     }
      static const uint32_t small_immediates[] = {
         0, 1, 2, 3,
   4, 5, 6, 7,
   8, 9, 10, 11,
   12, 13, 14, 15,
   -16, -15, -14, -13,
   -12, -11, -10, -9,
   -8, -7, -6, -5,
   -4, -3, -2, -1,
   0x3b800000, /* 2.0^-8 */
   0x3c000000, /* 2.0^-7 */
   0x3c800000, /* 2.0^-6 */
   0x3d000000, /* 2.0^-5 */
   0x3d800000, /* 2.0^-4 */
   0x3e000000, /* 2.0^-3 */
   0x3e800000, /* 2.0^-2 */
   0x3f000000, /* 2.0^-1 */
   0x3f800000, /* 2.0^0 */
   0x40000000, /* 2.0^1 */
   0x40800000, /* 2.0^2 */
   0x41000000, /* 2.0^3 */
   0x41800000, /* 2.0^4 */
   0x42000000, /* 2.0^5 */
   0x42800000, /* 2.0^6 */
   };
      bool
   v3d_qpu_small_imm_unpack(const struct v3d_device_info *devinfo,
               {
         if (packed_small_immediate >= ARRAY_SIZE(small_immediates))
            *small_immediate = small_immediates[packed_small_immediate];
   }
      bool
   v3d_qpu_small_imm_pack(const struct v3d_device_info *devinfo,
               {
                  for (int i = 0; i < ARRAY_SIZE(small_immediates); i++) {
            if (small_immediates[i] == value) {
                     }
      bool
   v3d_qpu_flags_unpack(const struct v3d_device_info *devinfo,
               {
         static const enum v3d_qpu_cond cond_map[4] = {
            [0] = V3D_QPU_COND_IFA,
   [1] = V3D_QPU_COND_IFB,
               cond->ac = V3D_QPU_COND_NONE;
   cond->mc = V3D_QPU_COND_NONE;
   cond->apf = V3D_QPU_PF_NONE;
   cond->mpf = V3D_QPU_PF_NONE;
   cond->auf = V3D_QPU_UF_NONE;
            if (packed_cond == 0) {
         } else if (packed_cond >> 2 == 0) {
         } else if (packed_cond >> 4 == 0) {
         } else if (packed_cond == 0x10) {
         } else if (packed_cond >> 2 == 0x4) {
         } else if (packed_cond >> 4 == 0x1) {
         } else if (packed_cond >> 4 == 0x2) {
               } else if (packed_cond >> 4 == 0x3) {
               } else if (packed_cond >> 6) {
            cond->mc = cond_map[(packed_cond >> 4) & 0x3];
   if (((packed_cond >> 2) & 0x3) == 0) {
         } else {
               }
      bool
   v3d_qpu_flags_pack(const struct v3d_device_info *devinfo,
               {
   #define AC (1 << 0)
   #define MC (1 << 1)
   #define APF (1 << 2)
   #define MPF (1 << 3)
   #define AUF (1 << 4)
   #define MUF (1 << 5)
         static const struct {
               } flags_table[] = {
            { 0,        0 },
   { APF,      0 },
   { AUF,      0 },
   { MPF,      (1 << 4) },
   { MUF,      (1 << 4) },
   { AC,       (1 << 5) },
   { AC | MPF, (1 << 5) },
   { MC,       (1 << 5) | (1 << 4) },
   { MC | APF, (1 << 5) | (1 << 4) },
               uint8_t flags_present = 0;
   if (cond->ac != V3D_QPU_COND_NONE)
         if (cond->mc != V3D_QPU_COND_NONE)
         if (cond->apf != V3D_QPU_PF_NONE)
         if (cond->mpf != V3D_QPU_PF_NONE)
         if (cond->auf != V3D_QPU_UF_NONE)
         if (cond->muf != V3D_QPU_UF_NONE)
            for (int i = 0; i < ARRAY_SIZE(flags_table); i++) {
                                                   if (flags_present & AUF)
                        if (flags_present & AC) {
            if (*packed_cond & (1 << 6))
                           if (flags_present & MC) {
            if (*packed_cond & (1 << 6))
         *packed_cond |= (cond->mc -
                           }
      /* Make a mapping of the table of opcodes in the spec.  The opcode is
   * determined by a combination of the opcode field, and in the case of 0 or
   * 1-arg opcodes, the mux (version <= 42) or raddr (version >= 71) field as
   * well.
   */
   #define OP_MASK(val) BITFIELD64_BIT(val)
   #define OP_RANGE(bot, top) BITFIELD64_RANGE(bot, top - bot + 1)
   #define ANYMUX OP_RANGE(0, 7)
   /* FIXME: right now ussing BITFIELD64_RANGE to set the last bit raises a
   * warning when building with clang using the shift-count-overflow option
   */
   #define ANYOPMASK ~0ull
      struct opcode_desc {
         uint8_t opcode_first;
            union {
            struct {
                                    /* first_ver == 0 if it's the same across all V3D versions.
      * first_ver == X, last_ver == 0 if it's the same for all V3D versions
   *   starting from X
   * first_ver == X, last_ver == Y if it's the same for all V3D versions
   *   on the range X through Y
      uint8_t first_ver;
   };
      static const struct opcode_desc add_ops_v33[] = {
         /* FADD is FADDNF depending on the order of the mux_a/mux_b. */
   { 0,   47,  .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_FADD },
   { 0,   47,  .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_FADDNF },
   { 53,  55,  .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_VFPACK },
   { 56,  56,  .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_ADD },
   { 57,  59,  .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_VFPACK },
   { 60,  60,  .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_SUB },
   { 61,  63,  .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_VFPACK },
   { 64,  111, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_FSUB },
   { 120, 120, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_MIN },
   { 121, 121, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_MAX },
   { 122, 122, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_UMIN },
   { 123, 123, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_UMAX },
   { 124, 124, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_SHL },
   { 125, 125, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_SHR },
   { 126, 126, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_ASR },
   { 127, 127, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_ROR },
   /* FMIN is instead FMAX depending on the order of the mux_a/mux_b. */
   { 128, 175, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_FMIN },
   { 128, 175, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_FMAX },
            { 181, 181, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_AND },
   { 182, 182, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_OR },
            { 184, 184, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_VADD },
   { 185, 185, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_VSUB },
   { 186, 186, .mux.b_mask = OP_MASK(0), .mux.a_mask = ANYMUX, V3D_QPU_A_NOT },
   { 186, 186, .mux.b_mask = OP_MASK(1), .mux.a_mask = ANYMUX, V3D_QPU_A_NEG },
   { 186, 186, .mux.b_mask = OP_MASK(2), .mux.a_mask = ANYMUX, V3D_QPU_A_FLAPUSH },
   { 186, 186, .mux.b_mask = OP_MASK(3), .mux.a_mask = ANYMUX, V3D_QPU_A_FLBPUSH },
   { 186, 186, .mux.b_mask = OP_MASK(4), .mux.a_mask = ANYMUX, V3D_QPU_A_FLPOP },
   { 186, 186, .mux.b_mask = OP_MASK(5), .mux.a_mask = ANYMUX, V3D_QPU_A_RECIP },
   { 186, 186, .mux.b_mask = OP_MASK(6), .mux.a_mask = ANYMUX, V3D_QPU_A_SETMSF },
   { 186, 186, .mux.b_mask = OP_MASK(7), .mux.a_mask = ANYMUX, V3D_QPU_A_SETREVF },
   { 187, 187, .mux.b_mask = OP_MASK(0), .mux.a_mask = OP_MASK(0), V3D_QPU_A_NOP, 0 },
   { 187, 187, .mux.b_mask = OP_MASK(0), .mux.a_mask = OP_MASK(1), V3D_QPU_A_TIDX },
   { 187, 187, .mux.b_mask = OP_MASK(0), .mux.a_mask = OP_MASK(2), V3D_QPU_A_EIDX },
   { 187, 187, .mux.b_mask = OP_MASK(0), .mux.a_mask = OP_MASK(3), V3D_QPU_A_LR },
   { 187, 187, .mux.b_mask = OP_MASK(0), .mux.a_mask = OP_MASK(4), V3D_QPU_A_VFLA },
   { 187, 187, .mux.b_mask = OP_MASK(0), .mux.a_mask = OP_MASK(5), V3D_QPU_A_VFLNA },
   { 187, 187, .mux.b_mask = OP_MASK(0), .mux.a_mask = OP_MASK(6), V3D_QPU_A_VFLB },
            { 187, 187, .mux.b_mask = OP_MASK(1), .mux.a_mask = OP_RANGE(0, 2), V3D_QPU_A_FXCD },
   { 187, 187, .mux.b_mask = OP_MASK(1), .mux.a_mask = OP_MASK(3), V3D_QPU_A_XCD },
   { 187, 187, .mux.b_mask = OP_MASK(1), .mux.a_mask = OP_RANGE(4, 6), V3D_QPU_A_FYCD },
            { 187, 187, .mux.b_mask = OP_MASK(2), .mux.a_mask = OP_MASK(0), V3D_QPU_A_MSF },
   { 187, 187, .mux.b_mask = OP_MASK(2), .mux.a_mask = OP_MASK(1), V3D_QPU_A_REVF },
   { 187, 187, .mux.b_mask = OP_MASK(2), .mux.a_mask = OP_MASK(2), V3D_QPU_A_VDWWT, 33 },
   { 187, 187, .mux.b_mask = OP_MASK(2), .mux.a_mask = OP_MASK(2), V3D_QPU_A_IID, 40 },
   { 187, 187, .mux.b_mask = OP_MASK(2), .mux.a_mask = OP_MASK(3), V3D_QPU_A_SAMPID, 40 },
   { 187, 187, .mux.b_mask = OP_MASK(2), .mux.a_mask = OP_MASK(4), V3D_QPU_A_BARRIERID, 40 },
   { 187, 187, .mux.b_mask = OP_MASK(2), .mux.a_mask = OP_MASK(5), V3D_QPU_A_TMUWT },
   { 187, 187, .mux.b_mask = OP_MASK(2), .mux.a_mask = OP_MASK(6), V3D_QPU_A_VPMWT },
   { 187, 187, .mux.b_mask = OP_MASK(2), .mux.a_mask = OP_MASK(7), V3D_QPU_A_FLAFIRST, 41 },
   { 187, 187, .mux.b_mask = OP_MASK(3), .mux.a_mask = OP_MASK(0), V3D_QPU_A_FLNAFIRST, 41 },
            { 188, 188, .mux.b_mask = OP_MASK(0), .mux.a_mask = ANYMUX, V3D_QPU_A_LDVPMV_IN, 40 },
   { 188, 188, .mux.b_mask = OP_MASK(0), .mux.a_mask = ANYMUX, V3D_QPU_A_LDVPMV_OUT, 40 },
   { 188, 188, .mux.b_mask = OP_MASK(1), .mux.a_mask = ANYMUX, V3D_QPU_A_LDVPMD_IN, 40 },
   { 188, 188, .mux.b_mask = OP_MASK(1), .mux.a_mask = ANYMUX, V3D_QPU_A_LDVPMD_OUT, 40 },
   { 188, 188, .mux.b_mask = OP_MASK(2), .mux.a_mask = ANYMUX, V3D_QPU_A_LDVPMP, 40 },
   { 188, 188, .mux.b_mask = OP_MASK(3), .mux.a_mask = ANYMUX, V3D_QPU_A_RSQRT, 41 },
   { 188, 188, .mux.b_mask = OP_MASK(4), .mux.a_mask = ANYMUX, V3D_QPU_A_EXP, 41 },
   { 188, 188, .mux.b_mask = OP_MASK(5), .mux.a_mask = ANYMUX, V3D_QPU_A_LOG, 41 },
   { 188, 188, .mux.b_mask = OP_MASK(6), .mux.a_mask = ANYMUX, V3D_QPU_A_SIN, 41 },
   { 188, 188, .mux.b_mask = OP_MASK(7), .mux.a_mask = ANYMUX, V3D_QPU_A_RSQRT2, 41 },
   { 189, 189, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_LDVPMG_IN, 40 },
            /* FIXME: MORE COMPLICATED */
            { 192, 239, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_FCMP },
            { 245, 245, .mux.b_mask = OP_RANGE(0, 2), .mux.a_mask = ANYMUX, V3D_QPU_A_FROUND },
   { 245, 245, .mux.b_mask = OP_MASK(3), .mux.a_mask = ANYMUX, V3D_QPU_A_FTOIN },
   { 245, 245, .mux.b_mask = OP_RANGE(4, 6), .mux.a_mask = ANYMUX, V3D_QPU_A_FTRUNC },
   { 245, 245, .mux.b_mask = OP_MASK(7), .mux.a_mask = ANYMUX, V3D_QPU_A_FTOIZ },
   { 246, 246, .mux.b_mask = OP_RANGE(0, 2), .mux.a_mask = ANYMUX, V3D_QPU_A_FFLOOR },
   { 246, 246, .mux.b_mask = OP_MASK(3), .mux.a_mask = ANYMUX, V3D_QPU_A_FTOUZ },
   { 246, 246, .mux.b_mask = OP_RANGE(4, 6), .mux.a_mask = ANYMUX, V3D_QPU_A_FCEIL },
            { 247, 247, .mux.b_mask = OP_RANGE(0, 2), .mux.a_mask = ANYMUX, V3D_QPU_A_FDX },
            /* The stvpms are distinguished by the waddr field. */
   { 248, 248, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_STVPMV },
   { 248, 248, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_A_STVPMD },
            { 252, 252, .mux.b_mask = OP_RANGE(0, 2), .mux.a_mask = ANYMUX, V3D_QPU_A_ITOF },
   { 252, 252, .mux.b_mask = OP_MASK(3), .mux.a_mask = ANYMUX, V3D_QPU_A_CLZ },
   };
      static const struct opcode_desc mul_ops_v33[] = {
         { 1, 1, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_M_ADD },
   { 2, 2, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_M_SUB },
   { 3, 3, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_M_UMUL24 },
   { 4, 8, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_M_VFMUL },
   { 9, 9, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_M_SMUL24 },
   { 10, 10, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_M_MULTOP },
   { 14, 14, .mux.b_mask = ANYMUX, .mux.a_mask = ANYMUX, V3D_QPU_M_FMOV, 33, 42 },
   { 15, 15, .mux.b_mask = OP_RANGE(0, 3), ANYMUX, V3D_QPU_M_FMOV, 33, 42},
   { 15, 15, .mux.b_mask = OP_MASK(4), .mux.a_mask = OP_MASK(0), V3D_QPU_M_NOP, 33, 42 },
            };
      /* Note that it would have been possible to define all the add/mul opcodes in
   * just one table, using the first_ver/last_ver. But taking into account that
   * for v71 there were a lot of changes, it was more tidy this way. Also right
   * now we are doing a linear search on those tables, so this maintains the
   * tables smaller.
   *
   * Just in case we merge the tables, we define the first_ver as 71 for those
   * opcodes that changed on v71
   */
   static const struct opcode_desc add_ops_v71[] = {
         /* FADD is FADDNF depending on the order of the raddr_a/raddr_b. */
   { 0,   47,  .raddr_mask = ANYOPMASK, V3D_QPU_A_FADD },
   { 0,   47,  .raddr_mask = ANYOPMASK, V3D_QPU_A_FADDNF },
   { 53,  55,  .raddr_mask = ANYOPMASK, V3D_QPU_A_VFPACK },
   { 56,  56,  .raddr_mask = ANYOPMASK, V3D_QPU_A_ADD },
   { 57,  59,  .raddr_mask = ANYOPMASK, V3D_QPU_A_VFPACK },
   { 60,  60,  .raddr_mask = ANYOPMASK, V3D_QPU_A_SUB },
   { 61,  63,  .raddr_mask = ANYOPMASK, V3D_QPU_A_VFPACK },
   { 64,  111, .raddr_mask = ANYOPMASK, V3D_QPU_A_FSUB },
   { 120, 120, .raddr_mask = ANYOPMASK, V3D_QPU_A_MIN },
   { 121, 121, .raddr_mask = ANYOPMASK, V3D_QPU_A_MAX },
   { 122, 122, .raddr_mask = ANYOPMASK, V3D_QPU_A_UMIN },
   { 123, 123, .raddr_mask = ANYOPMASK, V3D_QPU_A_UMAX },
   { 124, 124, .raddr_mask = ANYOPMASK, V3D_QPU_A_SHL },
   { 125, 125, .raddr_mask = ANYOPMASK, V3D_QPU_A_SHR },
   { 126, 126, .raddr_mask = ANYOPMASK, V3D_QPU_A_ASR },
   { 127, 127, .raddr_mask = ANYOPMASK, V3D_QPU_A_ROR },
   /* FMIN is instead FMAX depending on the raddr_a/b order. */
   { 128, 175, .raddr_mask = ANYOPMASK, V3D_QPU_A_FMIN },
   { 128, 175, .raddr_mask = ANYOPMASK, V3D_QPU_A_FMAX },
            { 181, 181, .raddr_mask = ANYOPMASK, V3D_QPU_A_AND },
   { 182, 182, .raddr_mask = ANYOPMASK, V3D_QPU_A_OR },
   { 183, 183, .raddr_mask = ANYOPMASK, V3D_QPU_A_XOR },
   { 184, 184, .raddr_mask = ANYOPMASK, V3D_QPU_A_VADD },
            { 186, 186, .raddr_mask = OP_MASK(0), V3D_QPU_A_NOT },
   { 186, 186, .raddr_mask = OP_MASK(1), V3D_QPU_A_NEG },
   { 186, 186, .raddr_mask = OP_MASK(2), V3D_QPU_A_FLAPUSH },
   { 186, 186, .raddr_mask = OP_MASK(3), V3D_QPU_A_FLBPUSH },
   { 186, 186, .raddr_mask = OP_MASK(4), V3D_QPU_A_FLPOP },
   { 186, 186, .raddr_mask = OP_MASK(5), V3D_QPU_A_CLZ },
   { 186, 186, .raddr_mask = OP_MASK(6), V3D_QPU_A_SETMSF },
            { 187, 187, .raddr_mask = OP_MASK(0), V3D_QPU_A_NOP, 0 },
   { 187, 187, .raddr_mask = OP_MASK(1), V3D_QPU_A_TIDX },
   { 187, 187, .raddr_mask = OP_MASK(2), V3D_QPU_A_EIDX },
   { 187, 187, .raddr_mask = OP_MASK(3), V3D_QPU_A_LR },
   { 187, 187, .raddr_mask = OP_MASK(4), V3D_QPU_A_VFLA },
   { 187, 187, .raddr_mask = OP_MASK(5), V3D_QPU_A_VFLNA },
   { 187, 187, .raddr_mask = OP_MASK(6), V3D_QPU_A_VFLB },
   { 187, 187, .raddr_mask = OP_MASK(7), V3D_QPU_A_VFLNB },
   { 187, 187, .raddr_mask = OP_MASK(8), V3D_QPU_A_XCD },
   { 187, 187, .raddr_mask = OP_MASK(9), V3D_QPU_A_YCD },
   { 187, 187, .raddr_mask = OP_MASK(10), V3D_QPU_A_MSF },
   { 187, 187, .raddr_mask = OP_MASK(11), V3D_QPU_A_REVF },
   { 187, 187, .raddr_mask = OP_MASK(12), V3D_QPU_A_IID },
   { 187, 187, .raddr_mask = OP_MASK(13), V3D_QPU_A_SAMPID },
   { 187, 187, .raddr_mask = OP_MASK(14), V3D_QPU_A_BARRIERID },
   { 187, 187, .raddr_mask = OP_MASK(15), V3D_QPU_A_TMUWT },
   { 187, 187, .raddr_mask = OP_MASK(16), V3D_QPU_A_VPMWT },
   { 187, 187, .raddr_mask = OP_MASK(17), V3D_QPU_A_FLAFIRST },
            { 187, 187, .raddr_mask = OP_RANGE(32, 34), V3D_QPU_A_FXCD },
            { 188, 188, .raddr_mask = OP_MASK(0), V3D_QPU_A_LDVPMV_IN, 71 },
   { 188, 188, .raddr_mask = OP_MASK(1), V3D_QPU_A_LDVPMD_IN, 71 },
            { 188, 188, .raddr_mask = OP_MASK(32), V3D_QPU_A_RECIP, 71 },
   { 188, 188, .raddr_mask = OP_MASK(33), V3D_QPU_A_RSQRT, 71 },
   { 188, 188, .raddr_mask = OP_MASK(34), V3D_QPU_A_EXP, 71 },
   { 188, 188, .raddr_mask = OP_MASK(35), V3D_QPU_A_LOG, 71 },
   { 188, 188, .raddr_mask = OP_MASK(36), V3D_QPU_A_SIN, 71 },
                     /* The stvpms are distinguished by the waddr field. */
   { 190, 190, .raddr_mask = ANYOPMASK, V3D_QPU_A_STVPMV, 71},
   { 190, 190, .raddr_mask = ANYOPMASK, V3D_QPU_A_STVPMD, 71},
                     { 245, 245, .raddr_mask = OP_RANGE(0, 2),   V3D_QPU_A_FROUND, 71 },
   { 245, 245, .raddr_mask = OP_RANGE(4, 6),   V3D_QPU_A_FROUND, 71 },
   { 245, 245, .raddr_mask = OP_RANGE(8, 10),  V3D_QPU_A_FROUND, 71 },
            { 245, 245, .raddr_mask = OP_MASK(3),  V3D_QPU_A_FTOIN, 71 },
   { 245, 245, .raddr_mask = OP_MASK(7),  V3D_QPU_A_FTOIN, 71 },
   { 245, 245, .raddr_mask = OP_MASK(11), V3D_QPU_A_FTOIN, 71 },
            { 245, 245, .raddr_mask = OP_RANGE(16, 18), V3D_QPU_A_FTRUNC, 71 },
   { 245, 245, .raddr_mask = OP_RANGE(20, 22), V3D_QPU_A_FTRUNC, 71 },
   { 245, 245, .raddr_mask = OP_RANGE(24, 26), V3D_QPU_A_FTRUNC, 71 },
            { 245, 245, .raddr_mask = OP_MASK(19), V3D_QPU_A_FTOIZ, 71 },
   { 245, 245, .raddr_mask = OP_MASK(23), V3D_QPU_A_FTOIZ, 71 },
   { 245, 245, .raddr_mask = OP_MASK(27), V3D_QPU_A_FTOIZ, 71 },
            { 245, 245, .raddr_mask = OP_RANGE(32, 34), V3D_QPU_A_FFLOOR, 71 },
   { 245, 245, .raddr_mask = OP_RANGE(36, 38), V3D_QPU_A_FFLOOR, 71 },
   { 245, 245, .raddr_mask = OP_RANGE(40, 42), V3D_QPU_A_FFLOOR, 71 },
            { 245, 245, .raddr_mask = OP_MASK(35), V3D_QPU_A_FTOUZ, 71 },
   { 245, 245, .raddr_mask = OP_MASK(39), V3D_QPU_A_FTOUZ, 71 },
   { 245, 245, .raddr_mask = OP_MASK(43), V3D_QPU_A_FTOUZ, 71 },
            { 245, 245, .raddr_mask = OP_RANGE(48, 50), V3D_QPU_A_FCEIL, 71 },
   { 245, 245, .raddr_mask = OP_RANGE(52, 54), V3D_QPU_A_FCEIL, 71 },
   { 245, 245, .raddr_mask = OP_RANGE(56, 58), V3D_QPU_A_FCEIL, 71 },
            { 245, 245, .raddr_mask = OP_MASK(51), V3D_QPU_A_FTOC },
   { 245, 245, .raddr_mask = OP_MASK(55), V3D_QPU_A_FTOC },
   { 245, 245, .raddr_mask = OP_MASK(59), V3D_QPU_A_FTOC },
            { 246, 246, .raddr_mask = OP_RANGE(0, 2),   V3D_QPU_A_FDX, 71 },
   { 246, 246, .raddr_mask = OP_RANGE(4, 6),   V3D_QPU_A_FDX, 71 },
   { 246, 246, .raddr_mask = OP_RANGE(8, 10),  V3D_QPU_A_FDX, 71 },
   { 246, 246, .raddr_mask = OP_RANGE(12, 14), V3D_QPU_A_FDX, 71 },
   { 246, 246, .raddr_mask = OP_RANGE(16, 18), V3D_QPU_A_FDY, 71 },
   { 246, 246, .raddr_mask = OP_RANGE(20, 22), V3D_QPU_A_FDY, 71 },
   { 246, 246, .raddr_mask = OP_RANGE(24, 26), V3D_QPU_A_FDY, 71 },
            { 246, 246, .raddr_mask = OP_RANGE(32, 34), V3D_QPU_A_ITOF, 71 },
            { 247, 247, .raddr_mask = ANYOPMASK, V3D_QPU_A_VPACK, 71 },
            { 249, 249, .raddr_mask = OP_RANGE(0, 2),   V3D_QPU_A_FMOV, 71 },
   { 249, 249, .raddr_mask = OP_RANGE(4, 6),   V3D_QPU_A_FMOV, 71 },
   { 249, 249, .raddr_mask = OP_RANGE(8, 10),  V3D_QPU_A_FMOV, 71 },
   { 249, 249, .raddr_mask = OP_RANGE(12, 14), V3D_QPU_A_FMOV, 71 },
   { 249, 249, .raddr_mask = OP_RANGE(16, 18), V3D_QPU_A_FMOV, 71 },
   { 249, 249, .raddr_mask = OP_RANGE(20, 22), V3D_QPU_A_FMOV, 71 },
            { 249, 249, .raddr_mask = OP_MASK(3),  V3D_QPU_A_MOV, 71 },
   { 249, 249, .raddr_mask = OP_MASK(7),  V3D_QPU_A_MOV, 71 },
   { 249, 249, .raddr_mask = OP_MASK(11), V3D_QPU_A_MOV, 71 },
   { 249, 249, .raddr_mask = OP_MASK(15), V3D_QPU_A_MOV, 71 },
            { 250, 250, .raddr_mask = ANYOPMASK, V3D_QPU_A_V10PACK, 71 },
   };
      static const struct opcode_desc mul_ops_v71[] = {
         /* For V3D 7.1, second mask field would be ignored */
   { 1, 1, .raddr_mask = ANYOPMASK, V3D_QPU_M_ADD, 71 },
   { 2, 2, .raddr_mask = ANYOPMASK, V3D_QPU_M_SUB, 71 },
   { 3, 3, .raddr_mask = ANYOPMASK, V3D_QPU_M_UMUL24, 71 },
   { 3, 3, .raddr_mask = ANYOPMASK, V3D_QPU_M_UMUL24, 71 },
   { 4, 8, .raddr_mask = ANYOPMASK, V3D_QPU_M_VFMUL, 71 },
   { 9, 9, .raddr_mask = ANYOPMASK, V3D_QPU_M_SMUL24, 71 },
            { 14, 14, .raddr_mask = OP_RANGE(0, 2),   V3D_QPU_M_FMOV, 71 },
   { 14, 14, .raddr_mask = OP_RANGE(4, 6),   V3D_QPU_M_FMOV, 71 },
   { 14, 14, .raddr_mask = OP_RANGE(8, 10),  V3D_QPU_M_FMOV, 71 },
   { 14, 14, .raddr_mask = OP_RANGE(12, 14), V3D_QPU_M_FMOV, 71 },
   { 14, 14, .raddr_mask = OP_RANGE(16, 18), V3D_QPU_M_FMOV, 71 },
            { 14, 14, .raddr_mask = OP_MASK(3),  V3D_QPU_M_MOV, 71 },
   { 14, 14, .raddr_mask = OP_MASK(7),  V3D_QPU_M_MOV, 71 },
   { 14, 14, .raddr_mask = OP_MASK(11), V3D_QPU_M_MOV, 71 },
   { 14, 14, .raddr_mask = OP_MASK(15), V3D_QPU_M_MOV, 71 },
            { 14, 14, .raddr_mask = OP_MASK(32), V3D_QPU_M_FTOUNORM16, 71 },
   { 14, 14, .raddr_mask = OP_MASK(33), V3D_QPU_M_FTOSNORM16, 71 },
   { 14, 14, .raddr_mask = OP_MASK(34), V3D_QPU_M_VFTOUNORM8, 71 },
   { 14, 14, .raddr_mask = OP_MASK(35), V3D_QPU_M_VFTOSNORM8, 71 },
   { 14, 14, .raddr_mask = OP_MASK(48), V3D_QPU_M_VFTOUNORM10LO, 71 },
                     };
      /* Returns true if op_desc should be filtered out based on devinfo->ver
   * against op_desc->first_ver and op_desc->last_ver. Check notes about
   * first_ver/last_ver on struct opcode_desc comments.
   */
   static bool
   opcode_invalid_in_version(const struct v3d_device_info *devinfo,
               {
         return (first_ver != 0 && devinfo->ver < first_ver) ||
   }
      /* Note that we pass as parameters mux_a, mux_b and raddr, even if depending
   * on the devinfo->ver some would be ignored. We do this way just to avoid
   * having two really similar lookup_opcode methods
   */
   static const struct opcode_desc *
   lookup_opcode_from_packed(const struct v3d_device_info *devinfo,
                           {
         for (int i = 0; i < num_opcodes; i++) {
                                                                                          } else {
                              }
      static bool
   v3d_qpu_float32_unpack_unpack(uint32_t packed,
         {
         switch (packed) {
   case 0:
               case 1:
               case 2:
               case 3:
               default:
         }
      static bool
   v3d_qpu_float32_unpack_pack(enum v3d_qpu_input_unpack unpacked,
         {
         switch (unpacked) {
   case V3D_QPU_UNPACK_ABS:
               case V3D_QPU_UNPACK_NONE:
               case V3D_QPU_UNPACK_L:
               case V3D_QPU_UNPACK_H:
               default:
         }
      static bool
   v3d_qpu_int32_unpack_unpack(uint32_t packed,
         {
         switch (packed) {
   case 0:
               case 1:
               case 2:
               case 3:
               case 4:
               default:
         }
      static bool
   v3d_qpu_int32_unpack_pack(enum v3d_qpu_input_unpack unpacked,
         {
         switch (unpacked) {
   case V3D_QPU_UNPACK_NONE:
               case V3D_QPU_UNPACK_UL:
               case V3D_QPU_UNPACK_UH:
               case V3D_QPU_UNPACK_IL:
               case V3D_QPU_UNPACK_IH:
               default:
         }
      static bool
   v3d_qpu_float16_unpack_unpack(uint32_t packed,
         {
         switch (packed) {
   case 0:
               case 1:
               case 2:
               case 3:
               case 4:
               default:
         }
      static bool
   v3d_qpu_float16_unpack_pack(enum v3d_qpu_input_unpack unpacked,
         {
         switch (unpacked) {
   case V3D_QPU_UNPACK_NONE:
               case V3D_QPU_UNPACK_REPLICATE_32F_16:
               case V3D_QPU_UNPACK_REPLICATE_L_16:
               case V3D_QPU_UNPACK_REPLICATE_H_16:
               case V3D_QPU_UNPACK_SWAP_16:
               default:
         }
      static bool
   v3d_qpu_float32_pack_pack(enum v3d_qpu_output_pack pack,
         {
         switch (pack) {
   case V3D_QPU_PACK_NONE:
               case V3D_QPU_PACK_L:
               case V3D_QPU_PACK_H:
               default:
         }
      static bool
   v3d33_qpu_add_unpack(const struct v3d_device_info *devinfo, uint64_t packed_inst,
         {
         uint32_t op = QPU_GET_FIELD(packed_inst, V3D_QPU_OP_ADD);
   uint32_t mux_a = QPU_GET_FIELD(packed_inst, V3D_QPU_ADD_A);
   uint32_t mux_b = QPU_GET_FIELD(packed_inst, V3D_QPU_ADD_B);
            uint32_t map_op = op;
   /* Some big clusters of opcodes are replicated with unpack
      * flags
      if (map_op >= 249 && map_op <= 251)
         if (map_op >= 253 && map_op <= 255)
            const struct opcode_desc *desc =
                        if (!desc)
                     /* FADD/FADDNF and FMIN/FMAX are determined by the orders of the
      * operands.
      if (((op >> 2) & 3) * 8 + mux_a > (op & 3) * 8 + mux_b) {
            if (instr->alu.add.op == V3D_QPU_A_FMIN)
                     /* Some QPU ops require a bit more than just basic opcode and mux a/b
      * comparisons to distinguish them.
      switch (instr->alu.add.op) {
   case V3D_QPU_A_STVPMV:
   case V3D_QPU_A_STVPMD:
   case V3D_QPU_A_STVPMP:
            switch (waddr) {
   case 0:
               case 1:
               case 2:
               default:
            default:
                  switch (instr->alu.add.op) {
   case V3D_QPU_A_FADD:
   case V3D_QPU_A_FADDNF:
   case V3D_QPU_A_FSUB:
   case V3D_QPU_A_FMIN:
   case V3D_QPU_A_FMAX:
   case V3D_QPU_A_FCMP:
   case V3D_QPU_A_VFPACK:
            if (instr->alu.add.op != V3D_QPU_A_VFPACK)
                        if (!v3d_qpu_float32_unpack_unpack((op >> 2) & 0x3,
                        if (!v3d_qpu_float32_unpack_unpack((op >> 0) & 0x3,
                     case V3D_QPU_A_FFLOOR:
   case V3D_QPU_A_FROUND:
   case V3D_QPU_A_FTRUNC:
   case V3D_QPU_A_FCEIL:
   case V3D_QPU_A_FDX:
   case V3D_QPU_A_FDY:
                     if (!v3d_qpu_float32_unpack_unpack((op >> 2) & 0x3,
                     case V3D_QPU_A_FTOIN:
   case V3D_QPU_A_FTOIZ:
   case V3D_QPU_A_FTOUZ:
   case V3D_QPU_A_FTOC:
                     if (!v3d_qpu_float32_unpack_unpack((op >> 2) & 0x3,
                     case V3D_QPU_A_VFMIN:
   case V3D_QPU_A_VFMAX:
            if (!v3d_qpu_float16_unpack_unpack(op & 0x7,
                                    default:
            instr->alu.add.output_pack = V3D_QPU_PACK_NONE;
   instr->alu.add.a.unpack = V3D_QPU_UNPACK_NONE;
               instr->alu.add.a.mux = mux_a;
   instr->alu.add.b.mux = mux_b;
            instr->alu.add.magic_write = false;
   if (packed_inst & V3D_QPU_MA) {
            switch (instr->alu.add.op) {
   case V3D_QPU_A_LDVPMV_IN:
               case V3D_QPU_A_LDVPMD_IN:
               case V3D_QPU_A_LDVPMG_IN:
               default:
                     }
      static bool
   v3d71_qpu_add_unpack(const struct v3d_device_info *devinfo, uint64_t packed_inst,
         {
         uint32_t op = QPU_GET_FIELD(packed_inst, V3D_QPU_OP_ADD);
   uint32_t raddr_a = QPU_GET_FIELD(packed_inst, V3D_QPU_RADDR_A);
   uint32_t raddr_b = QPU_GET_FIELD(packed_inst, V3D_QPU_RADDR_B);
   uint32_t waddr = QPU_GET_FIELD(packed_inst, V3D_QPU_WADDR_A);
            const struct opcode_desc *desc =
            lookup_opcode_from_packed(devinfo,
                  if (!desc)
                     /* FADD/FADDNF and FMIN/FMAX are determined by the order of the
      * operands.
      if (instr->sig.small_imm_a * 256 + ((op >> 2) & 3) * 64 + raddr_a >
         instr->sig.small_imm_b * 256 + (op & 3) * 64 + raddr_b) {
      if (instr->alu.add.op == V3D_QPU_A_FMIN)
                     /* Some QPU ops require a bit more than just basic opcode and mux a/b
      * comparisons to distinguish them.
      switch (instr->alu.add.op) {
   case V3D_QPU_A_STVPMV:
   case V3D_QPU_A_STVPMD:
   case V3D_QPU_A_STVPMP:
            switch (waddr) {
   case 0:
               case 1:
               case 2:
               default:
            default:
                  switch (instr->alu.add.op) {
   case V3D_QPU_A_FADD:
   case V3D_QPU_A_FADDNF:
   case V3D_QPU_A_FSUB:
   case V3D_QPU_A_FMIN:
   case V3D_QPU_A_FMAX:
   case V3D_QPU_A_FCMP:
   case V3D_QPU_A_VFPACK:
            if (instr->alu.add.op != V3D_QPU_A_VFPACK &&
      instr->alu.add.op != V3D_QPU_A_FCMP) {
                           if (!v3d_qpu_float32_unpack_unpack((op >> 2) & 0x3,
                        if (!v3d_qpu_float32_unpack_unpack((op >> 0) & 0x3,
                     case V3D_QPU_A_FFLOOR:
   case V3D_QPU_A_FROUND:
   case V3D_QPU_A_FTRUNC:
   case V3D_QPU_A_FCEIL:
   case V3D_QPU_A_FDX:
   case V3D_QPU_A_FDY:
                     if (!v3d_qpu_float32_unpack_unpack((op >> 2) & 0x3,
                     case V3D_QPU_A_FTOIN:
   case V3D_QPU_A_FTOIZ:
   case V3D_QPU_A_FTOUZ:
   case V3D_QPU_A_FTOC:
                     if (!v3d_qpu_float32_unpack_unpack((raddr_b >> 2) & 0x3,
                     case V3D_QPU_A_VFMIN:
   case V3D_QPU_A_VFMAX:
            unreachable("pending v71 update");
   if (!v3d_qpu_float16_unpack_unpack(op & 0x7,
                                    case V3D_QPU_A_MOV:
                     if (!v3d_qpu_int32_unpack_unpack((raddr_b >> 2) & 0x7,
                     case V3D_QPU_A_FMOV:
                     /* Mul alu FMOV has one additional variant */
                        if (!v3d_qpu_float32_unpack_unpack(unpack,
                     default:
            instr->alu.add.output_pack = V3D_QPU_PACK_NONE;
   instr->alu.add.a.unpack = V3D_QPU_UNPACK_NONE;
               instr->alu.add.a.raddr = raddr_a;
   instr->alu.add.b.raddr = raddr_b;
            instr->alu.add.magic_write = false;
   if (packed_inst & V3D_QPU_MA) {
            switch (instr->alu.add.op) {
   case V3D_QPU_A_LDVPMV_IN:
               case V3D_QPU_A_LDVPMD_IN:
               case V3D_QPU_A_LDVPMG_IN:
               default:
                     }
      static bool
   v3d_qpu_add_unpack(const struct v3d_device_info *devinfo, uint64_t packed_inst,
         {
         if (devinfo->ver < 71)
         else
   }
      static bool
   v3d33_qpu_mul_unpack(const struct v3d_device_info *devinfo, uint64_t packed_inst,
         {
         uint32_t op = QPU_GET_FIELD(packed_inst, V3D_QPU_OP_MUL);
   uint32_t mux_a = QPU_GET_FIELD(packed_inst, V3D_QPU_MUL_A);
            {
            const struct opcode_desc *desc =
            lookup_opcode_from_packed(devinfo,
                                 switch (instr->alu.mul.op) {
   case V3D_QPU_M_FMUL:
                     if (!v3d_qpu_float32_unpack_unpack((op >> 2) & 0x3,
                        if (!v3d_qpu_float32_unpack_unpack((op >> 0) & 0x3,
                        case V3D_QPU_M_FMOV:
                           if (!v3d_qpu_float32_unpack_unpack(mux_b & 0x3,
                        case V3D_QPU_M_VFMUL:
                     if (!v3d_qpu_float16_unpack_unpack(((op & 0x7) - 4) & 7,
                                 default:
            instr->alu.mul.output_pack = V3D_QPU_PACK_NONE;
   instr->alu.mul.a.unpack = V3D_QPU_UNPACK_NONE;
               instr->alu.mul.a.mux = mux_a;
   instr->alu.mul.b.mux = mux_b;
   instr->alu.mul.waddr = QPU_GET_FIELD(packed_inst, V3D_QPU_WADDR_M);
            }
      static bool
   v3d71_qpu_mul_unpack(const struct v3d_device_info *devinfo, uint64_t packed_inst,
         {
         uint32_t op = QPU_GET_FIELD(packed_inst, V3D_QPU_OP_MUL);
   uint32_t raddr_c = QPU_GET_FIELD(packed_inst, V3D_QPU_RADDR_C);
            {
            const struct opcode_desc *desc =
            lookup_opcode_from_packed(devinfo,
                                       switch (instr->alu.mul.op) {
   case V3D_QPU_M_FMUL:
                     if (!v3d_qpu_float32_unpack_unpack((op >> 2) & 0x3,
                        if (!v3d_qpu_float32_unpack_unpack((op >> 0) & 0x3,
                        case V3D_QPU_M_FMOV:
                     if (!v3d_qpu_float32_unpack_unpack((raddr_d >> 2) & 0x7,
                        case V3D_QPU_M_VFMUL:
                           if (!v3d_qpu_float16_unpack_unpack(((op & 0x7) - 4) & 7,
                                 case V3D_QPU_M_MOV:
                     if (!v3d_qpu_int32_unpack_unpack((raddr_d >> 2) & 0x7,
                     default:
            instr->alu.mul.output_pack = V3D_QPU_PACK_NONE;
   instr->alu.mul.a.unpack = V3D_QPU_UNPACK_NONE;
               instr->alu.mul.a.raddr = raddr_c;
   instr->alu.mul.b.raddr = raddr_d;
   instr->alu.mul.waddr = QPU_GET_FIELD(packed_inst, V3D_QPU_WADDR_M);
            }
      static bool
   v3d_qpu_mul_unpack(const struct v3d_device_info *devinfo, uint64_t packed_inst,
         {
         if (devinfo->ver < 71)
         else
   }
      static const struct opcode_desc *
   lookup_opcode_from_instr(const struct v3d_device_info *devinfo,
               {
         for (int i = 0; i < num_opcodes; i++) {
                                                         }
      static bool
   v3d33_qpu_add_pack(const struct v3d_device_info *devinfo,
         {
         uint32_t waddr = instr->alu.add.waddr;
   uint32_t mux_a = instr->alu.add.a.mux;
   uint32_t mux_b = instr->alu.add.b.mux;
   int nsrc = v3d_qpu_add_op_num_src(instr->alu.add.op);
   const struct opcode_desc *desc =
                        if (!desc)
                     /* If an operation doesn't use an arg, its mux values may be used to
      * identify the operation type.
      if (nsrc < 2)
            if (nsrc < 1)
                     switch (instr->alu.add.op) {
   case V3D_QPU_A_STVPMV:
            waddr = 0;
      case V3D_QPU_A_STVPMD:
            waddr = 1;
      case V3D_QPU_A_STVPMP:
                        case V3D_QPU_A_LDVPMV_IN:
   case V3D_QPU_A_LDVPMD_IN:
   case V3D_QPU_A_LDVPMP:
   case V3D_QPU_A_LDVPMG_IN:
                  case V3D_QPU_A_LDVPMV_OUT:
   case V3D_QPU_A_LDVPMD_OUT:
   case V3D_QPU_A_LDVPMG_OUT:
                        default:
                  switch (instr->alu.add.op) {
   case V3D_QPU_A_FADD:
   case V3D_QPU_A_FADDNF:
   case V3D_QPU_A_FSUB:
   case V3D_QPU_A_FMIN:
   case V3D_QPU_A_FMAX:
   case V3D_QPU_A_FCMP: {
                                 if (!v3d_qpu_float32_pack_pack(instr->alu.add.output_pack,
                              if (!v3d_qpu_float32_unpack_pack(instr->alu.add.a.unpack,
                        if (!v3d_qpu_float32_unpack_pack(instr->alu.add.b.unpack,
                        /* These operations with commutative operands are
   * distinguished by which order their operands come in.
   */
   bool ordering = a_unpack * 8 + mux_a > b_unpack * 8 + mux_b;
   if (((instr->alu.add.op == V3D_QPU_A_FMIN ||
                                                                                                   case V3D_QPU_A_VFPACK: {
                           if (instr->alu.add.a.unpack == V3D_QPU_UNPACK_ABS ||
                        if (!v3d_qpu_float32_unpack_pack(instr->alu.add.a.unpack,
                        if (!v3d_qpu_float32_unpack_pack(instr->alu.add.b.unpack,
                                             case V3D_QPU_A_FFLOOR:
   case V3D_QPU_A_FROUND:
   case V3D_QPU_A_FTRUNC:
   case V3D_QPU_A_FCEIL:
   case V3D_QPU_A_FDX:
   case V3D_QPU_A_FDY: {
                     if (!v3d_qpu_float32_pack_pack(instr->alu.add.output_pack,
                              if (!v3d_qpu_float32_unpack_pack(instr->alu.add.a.unpack,
               }
   if (packed == 0)
                     case V3D_QPU_A_FTOIN:
   case V3D_QPU_A_FTOIZ:
   case V3D_QPU_A_FTOUZ:
   case V3D_QPU_A_FTOC:
                           uint32_t packed;
   if (!v3d_qpu_float32_unpack_pack(instr->alu.add.a.unpack,
               }
                        case V3D_QPU_A_VFMIN:
   case V3D_QPU_A_VFMAX:
            if (instr->alu.add.output_pack != V3D_QPU_PACK_NONE ||
                        if (!v3d_qpu_float16_unpack_pack(instr->alu.add.a.unpack,
                           default:
            if (instr->alu.add.op != V3D_QPU_A_NOP &&
      (instr->alu.add.output_pack != V3D_QPU_PACK_NONE ||
      instr->alu.add.a.unpack != V3D_QPU_UNPACK_NONE ||
   instr->alu.add.b.unpack != V3D_QPU_UNPACK_NONE)) {
               *packed_instr |= QPU_SET_FIELD(mux_a, V3D_QPU_ADD_A);
   *packed_instr |= QPU_SET_FIELD(mux_b, V3D_QPU_ADD_B);
   *packed_instr |= QPU_SET_FIELD(opcode, V3D_QPU_OP_ADD);
   *packed_instr |= QPU_SET_FIELD(waddr, V3D_QPU_WADDR_A);
   if (instr->alu.add.magic_write && !no_magic_write)
            }
      static bool
   v3d71_qpu_add_pack(const struct v3d_device_info *devinfo,
         {
         uint32_t waddr = instr->alu.add.waddr;
   uint32_t raddr_a = instr->alu.add.a.raddr;
            int nsrc = v3d_qpu_add_op_num_src(instr->alu.add.op);
   const struct opcode_desc *desc =
            lookup_opcode_from_instr(devinfo, add_ops_v71,
      if (!desc)
                     /* If an operation doesn't use an arg, its raddr values may be used to
      * identify the operation type.
      if (nsrc < 2)
                     switch (instr->alu.add.op) {
   case V3D_QPU_A_STVPMV:
            waddr = 0;
      case V3D_QPU_A_STVPMD:
            waddr = 1;
      case V3D_QPU_A_STVPMP:
                        case V3D_QPU_A_LDVPMV_IN:
   case V3D_QPU_A_LDVPMD_IN:
   case V3D_QPU_A_LDVPMP:
   case V3D_QPU_A_LDVPMG_IN:
                  case V3D_QPU_A_LDVPMV_OUT:
   case V3D_QPU_A_LDVPMD_OUT:
   case V3D_QPU_A_LDVPMG_OUT:
                        default:
                  switch (instr->alu.add.op) {
   case V3D_QPU_A_FADD:
   case V3D_QPU_A_FADDNF:
   case V3D_QPU_A_FSUB:
   case V3D_QPU_A_FMIN:
   case V3D_QPU_A_FMAX:
   case V3D_QPU_A_FCMP: {
                                 if (instr->alu.add.op != V3D_QPU_A_FCMP) {
            if (!v3d_qpu_float32_pack_pack(instr->alu.add.output_pack,
                           if (!v3d_qpu_float32_unpack_pack(instr->alu.add.a.unpack,
                        if (!v3d_qpu_float32_unpack_pack(instr->alu.add.b.unpack,
                        /* These operations with commutative operands are
   * distinguished by the order of the operands come in.
   */
   bool ordering =
               if (((instr->alu.add.op == V3D_QPU_A_FMIN ||
                                                                                 /* If we are swapping raddr_a/b we also need to swap
   * small_imm_a/b.
   */
   if (instr->sig.small_imm_a || instr->sig.small_imm_b) {
         assert(instr->sig.small_imm_a !=
         struct v3d_qpu_sig new_sig = instr->sig;
   new_sig.small_imm_a = !instr->sig.small_imm_a;
   new_sig.small_imm_b = !instr->sig.small_imm_b;
   uint32_t sig;
   if (!v3d_qpu_sig_pack(devinfo, &new_sig, &sig))
                                          case V3D_QPU_A_VFPACK: {
                           if (instr->alu.add.a.unpack == V3D_QPU_UNPACK_ABS ||
                        if (!v3d_qpu_float32_unpack_pack(instr->alu.add.a.unpack,
                        if (!v3d_qpu_float32_unpack_pack(instr->alu.add.b.unpack,
                                             case V3D_QPU_A_FFLOOR:
   case V3D_QPU_A_FROUND:
   case V3D_QPU_A_FTRUNC:
   case V3D_QPU_A_FCEIL:
   case V3D_QPU_A_FDX:
   case V3D_QPU_A_FDY: {
                     if (!v3d_qpu_float32_pack_pack(instr->alu.add.output_pack,
                              if (!v3d_qpu_float32_unpack_pack(instr->alu.add.a.unpack,
               }
   if (packed == 0)
                     case V3D_QPU_A_FTOIN:
   case V3D_QPU_A_FTOIZ:
   case V3D_QPU_A_FTOUZ:
   case V3D_QPU_A_FTOC:
                           uint32_t packed;
   if (!v3d_qpu_float32_unpack_pack(instr->alu.add.a.unpack,
                                             case V3D_QPU_A_VFMIN:
   case V3D_QPU_A_VFMAX:
            if (instr->alu.add.output_pack != V3D_QPU_PACK_NONE ||
                        if (!v3d_qpu_float16_unpack_pack(instr->alu.add.a.unpack,
                           case V3D_QPU_A_MOV: {
                                    if (!v3d_qpu_int32_unpack_pack(instr->alu.add.a.unpack,
                                    case V3D_QPU_A_FMOV: {
                     if (!v3d_qpu_float32_pack_pack(instr->alu.add.output_pack,
                              if (!v3d_qpu_float32_unpack_pack(instr->alu.add.a.unpack,
               }
               default:
            if (instr->alu.add.op != V3D_QPU_A_NOP &&
      (instr->alu.add.output_pack != V3D_QPU_PACK_NONE ||
      instr->alu.add.a.unpack != V3D_QPU_UNPACK_NONE ||
   instr->alu.add.b.unpack != V3D_QPU_UNPACK_NONE)) {
               *packed_instr |= QPU_SET_FIELD(raddr_a, V3D_QPU_RADDR_A);
   *packed_instr |= QPU_SET_FIELD(raddr_b, V3D_QPU_RADDR_B);
   *packed_instr |= QPU_SET_FIELD(opcode, V3D_QPU_OP_ADD);
   *packed_instr |= QPU_SET_FIELD(waddr, V3D_QPU_WADDR_A);
   if (instr->alu.add.magic_write && !no_magic_write)
            }
      static bool
   v3d33_qpu_mul_pack(const struct v3d_device_info *devinfo,
         {
         uint32_t mux_a = instr->alu.mul.a.mux;
   uint32_t mux_b = instr->alu.mul.b.mux;
            const struct opcode_desc *desc =
                        if (!desc)
                     /* Some opcodes have a single valid value for their mux a/b, so set
      * that here.  If mux a/b determine packing, it will be set below.
      if (nsrc < 2)
            if (nsrc < 1)
            switch (instr->alu.mul.op) {
   case V3D_QPU_M_FMUL: {
                     if (!v3d_qpu_float32_pack_pack(instr->alu.mul.output_pack,
               }
   /* No need for a +1 because desc->opcode_first has a 1 in this
                        if (!v3d_qpu_float32_unpack_pack(instr->alu.mul.a.unpack,
                              if (!v3d_qpu_float32_unpack_pack(instr->alu.mul.b.unpack,
               }
               case V3D_QPU_M_FMOV: {
                     if (!v3d_qpu_float32_pack_pack(instr->alu.mul.output_pack,
                                    if (!v3d_qpu_float32_unpack_pack(instr->alu.mul.a.unpack,
               }
               case V3D_QPU_M_VFMUL: {
                                    if (!v3d_qpu_float16_unpack_pack(instr->alu.mul.a.unpack,
               }
   if (instr->alu.mul.a.unpack == V3D_QPU_UNPACK_SWAP_16)
                                             default:
            if (instr->alu.mul.op != V3D_QPU_M_NOP &&
      (instr->alu.mul.output_pack != V3D_QPU_PACK_NONE ||
      instr->alu.mul.a.unpack != V3D_QPU_UNPACK_NONE ||
   instr->alu.mul.b.unpack != V3D_QPU_UNPACK_NONE)) {
               *packed_instr |= QPU_SET_FIELD(mux_a, V3D_QPU_MUL_A);
            *packed_instr |= QPU_SET_FIELD(opcode, V3D_QPU_OP_MUL);
   *packed_instr |= QPU_SET_FIELD(instr->alu.mul.waddr, V3D_QPU_WADDR_M);
   if (instr->alu.mul.magic_write)
            }
      static bool
   v3d71_qpu_mul_pack(const struct v3d_device_info *devinfo,
         {
         uint32_t raddr_c = instr->alu.mul.a.raddr;
   uint32_t raddr_d = instr->alu.mul.b.raddr;
            const struct opcode_desc *desc =
            lookup_opcode_from_instr(devinfo, mul_ops_v71,
      if (!desc)
                     /* Some opcodes have a single valid value for their raddr_d, so set
      * that here.  If raddr_b determine packing, it will be set below.
      if (nsrc < 2)
            switch (instr->alu.mul.op) {
   case V3D_QPU_M_FMUL: {
                     if (!v3d_qpu_float32_pack_pack(instr->alu.mul.output_pack,
               }
   /* No need for a +1 because desc->opcode_first has a 1 in this
                        if (!v3d_qpu_float32_unpack_pack(instr->alu.mul.a.unpack,
                              if (!v3d_qpu_float32_unpack_pack(instr->alu.mul.b.unpack,
               }
               case V3D_QPU_M_FMOV: {
                     if (!v3d_qpu_float32_pack_pack(instr->alu.mul.output_pack,
                              if (!v3d_qpu_float32_unpack_pack(instr->alu.mul.a.unpack,
               }
               case V3D_QPU_M_VFMUL: {
                                          if (!v3d_qpu_float16_unpack_pack(instr->alu.mul.a.unpack,
               }
   if (instr->alu.mul.a.unpack == V3D_QPU_UNPACK_SWAP_16)
                                             case V3D_QPU_M_MOV: {
                                    if (!v3d_qpu_int32_unpack_pack(instr->alu.mul.a.unpack,
                                    default:
            if (instr->alu.mul.op != V3D_QPU_M_NOP &&
      (instr->alu.mul.output_pack != V3D_QPU_PACK_NONE ||
      instr->alu.mul.a.unpack != V3D_QPU_UNPACK_NONE ||
   instr->alu.mul.b.unpack != V3D_QPU_UNPACK_NONE)) {
               *packed_instr |= QPU_SET_FIELD(raddr_c, V3D_QPU_RADDR_C);
   *packed_instr |= QPU_SET_FIELD(raddr_d, V3D_QPU_RADDR_D);
   *packed_instr |= QPU_SET_FIELD(opcode, V3D_QPU_OP_MUL);
   *packed_instr |= QPU_SET_FIELD(instr->alu.mul.waddr, V3D_QPU_WADDR_M);
   if (instr->alu.mul.magic_write)
            }
      static bool
   v3d_qpu_add_pack(const struct v3d_device_info *devinfo,
         {
         if (devinfo->ver < 71)
         else
   }
      static bool
   v3d_qpu_mul_pack(const struct v3d_device_info *devinfo,
         {
         if (devinfo->ver < 71)
         else
   }
      static bool
   v3d_qpu_instr_unpack_alu(const struct v3d_device_info *devinfo,
               {
                  if (!v3d_qpu_sig_unpack(devinfo,
                        uint32_t packed_cond = QPU_GET_FIELD(packed_instr, V3D_QPU_COND);
   if (v3d_qpu_sig_writes_address(devinfo, &instr->sig)) {
                           instr->flags.ac = V3D_QPU_COND_NONE;
   instr->flags.mc = V3D_QPU_COND_NONE;
   instr->flags.apf = V3D_QPU_PF_NONE;
   instr->flags.mpf = V3D_QPU_PF_NONE;
      } else {
                        if (devinfo->ver <= 71) {
            /*
   * For v71 this will be set on add/mul unpack, as raddr are now
   * part of v3d_qpu_input
   */
               if (!v3d_qpu_add_unpack(devinfo, packed_instr, instr))
            if (!v3d_qpu_mul_unpack(devinfo, packed_instr, instr))
            }
      static bool
   v3d_qpu_instr_unpack_branch(const struct v3d_device_info *devinfo,
               {
                  uint32_t cond = QPU_GET_FIELD(packed_instr, V3D_QPU_BRANCH_COND);
   if (cond == 0)
         else if (V3D_QPU_BRANCH_COND_A0 + (cond - 2) <=
               else
            uint32_t msfign = QPU_GET_FIELD(packed_instr, V3D_QPU_BRANCH_MSFIGN);
   if (msfign == 3)
                           instr->branch.ub = packed_instr & V3D_QPU_BRANCH_UB;
   if (instr->branch.ub) {
                        instr->branch.raddr_a = QPU_GET_FIELD(packed_instr,
                     instr->branch.offset +=
                  instr->branch.offset +=
                  }
      bool
   v3d_qpu_instr_unpack(const struct v3d_device_info *devinfo,
               {
         if (QPU_GET_FIELD(packed_instr, V3D_QPU_OP_MUL) != 0) {
         } else {
                     if ((sig & 24) == 16) {
               } else {
      }
      static bool
   v3d_qpu_instr_pack_alu(const struct v3d_device_info *devinfo,
               {
         uint32_t sig;
   if (!v3d_qpu_sig_pack(devinfo, &instr->sig, &sig))
                  if (instr->type == V3D_QPU_INSTR_TYPE_ALU) {
            if (devinfo->ver < 71) {
            /*
   * For v71 this will be set on add/mul unpack, as raddr are now
   * part of v3d_qpu_input
                     if (!v3d_qpu_add_pack(devinfo, instr, packed_instr))
                        uint32_t flags;
   if (v3d_qpu_sig_writes_address(devinfo, &instr->sig)) {
            if (instr->flags.ac != V3D_QPU_COND_NONE ||
      instr->flags.mc != V3D_QPU_COND_NONE ||
   instr->flags.apf != V3D_QPU_PF_NONE ||
                                 flags = instr->sig_addr;
      } else {
                     } else {
                        }
      static bool
   v3d_qpu_instr_pack_branch(const struct v3d_device_info *devinfo,
               {
                  if (instr->branch.cond != V3D_QPU_BRANCH_COND_ALWAYS) {
            *packed_instr |= QPU_SET_FIELD(2 + (instr->branch.cond -
               *packed_instr |= QPU_SET_FIELD(instr->branch.msfign,
            *packed_instr |= QPU_SET_FIELD(instr->branch.bdi,
            if (instr->branch.ub) {
            *packed_instr |= V3D_QPU_BRANCH_UB;
               switch (instr->branch.bdi) {
   case V3D_QPU_BRANCH_DEST_ABS:
   case V3D_QPU_BRANCH_DEST_REL:
                                                *packed_instr |= QPU_SET_FIELD(instr->branch.offset >> 24,
      default:
                  if (instr->branch.bdi == V3D_QPU_BRANCH_DEST_REGFILE ||
         instr->branch.bdu == V3D_QPU_BRANCH_DEST_REGFILE) {
                  }
      bool
   v3d_qpu_instr_pack(const struct v3d_device_info *devinfo,
               {
                  switch (instr->type) {
   case V3D_QPU_INSTR_TYPE_ALU:
         case V3D_QPU_INSTR_TYPE_BRANCH:
         default:
         }
