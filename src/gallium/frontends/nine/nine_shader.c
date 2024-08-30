   /*
   * Copyright 2011 Joakim Sindholt <opensource@zhasha.com>
   * Copyright 2013 Christoph Bumiller
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "nine_shader.h"
      #include "device9.h"
   #include "nine_debug.h"
   #include "nine_state.h"
   #include "vertexdeclaration9.h"
      #include "util/bitscan.h"
   #include "util/macros.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "pipe/p_shader_tokens.h"
   #include "tgsi/tgsi_ureg.h"
   #include "tgsi/tgsi_dump.h"
   #include "nir/tgsi_to_nir.h"
      #define DBG_CHANNEL DBG_SHADER
      #define DUMP(args...) _nine_debug_printf(DBG_CHANNEL, NULL, args)
         struct shader_translator;
      typedef HRESULT (*translate_instruction_func)(struct shader_translator *);
      static inline const char *d3dsio_to_string(unsigned opcode);
         #define NINED3D_SM1_VS 0xfffe
   #define NINED3D_SM1_PS 0xffff
      #define NINE_MAX_COND_DEPTH 64
   #define NINE_MAX_LOOP_DEPTH 64
      #define NINED3DSP_END 0x0000ffff
      #define NINED3DSPTYPE_FLOAT4  0
   #define NINED3DSPTYPE_INT4    1
   #define NINED3DSPTYPE_BOOL    2
      #define NINED3DSPR_IMMEDIATE (D3DSPR_PREDICATE + 1)
      #define NINED3DSP_WRITEMASK_MASK  D3DSP_WRITEMASK_ALL
   #define NINED3DSP_WRITEMASK_SHIFT 16
      #define NINED3DSHADER_INST_PREDICATED (1 << 28)
      #define NINED3DSHADER_REL_OP_GT 1
   #define NINED3DSHADER_REL_OP_EQ 2
   #define NINED3DSHADER_REL_OP_GE 3
   #define NINED3DSHADER_REL_OP_LT 4
   #define NINED3DSHADER_REL_OP_NE 5
   #define NINED3DSHADER_REL_OP_LE 6
      #define NINED3DSIO_OPCODE_FLAGS_SHIFT 16
   #define NINED3DSIO_OPCODE_FLAGS_MASK  (0xff << NINED3DSIO_OPCODE_FLAGS_SHIFT)
      #define NINED3DSI_TEXLD_PROJECT 0x1
   #define NINED3DSI_TEXLD_BIAS    0x2
      #define NINED3DSP_WRITEMASK_0   0x1
   #define NINED3DSP_WRITEMASK_1   0x2
   #define NINED3DSP_WRITEMASK_2   0x4
   #define NINED3DSP_WRITEMASK_3   0x8
   #define NINED3DSP_WRITEMASK_ALL 0xf
      #define NINED3DSP_NOSWIZZLE ((0 << 0) | (1 << 2) | (2 << 4) | (3 << 6))
      #define NINE_SWIZZLE4(x,y,z,w) \
            #define NINE_APPLY_SWIZZLE(src, s) \
            #define NINED3DSPDM_SATURATE (D3DSPDM_SATURATE >> D3DSP_DSTMOD_SHIFT)
   #define NINED3DSPDM_PARTIALP (D3DSPDM_PARTIALPRECISION >> D3DSP_DSTMOD_SHIFT)
   #define NINED3DSPDM_CENTROID (D3DSPDM_MSAMPCENTROID >> D3DSP_DSTMOD_SHIFT)
      /*
   * NEG     all, not ps: m3x2, m3x3, m3x4, m4x3, m4x4
   * BIAS    <= PS 1.4 (x-0.5)
   * BIASNEG <= PS 1.4 (-(x-0.5))
   * SIGN    <= PS 1.4 (2(x-0.5))
   * SIGNNEG <= PS 1.4 (-2(x-0.5))
   * COMP    <= PS 1.4 (1-x)
   * X2       = PS 1.4 (2x)
   * X2NEG    = PS 1.4 (-2x)
   * DZ      <= PS 1.4, tex{ld,crd} (.xy/.z), z=0 => .11
   * DW      <= PS 1.4, tex{ld,crd} (.xy/.w), w=0 => .11
   * ABS     >= SM 3.0 (abs(x))
   * ABSNEG  >= SM 3.0 (-abs(x))
   * NOT     >= SM 2.0 pedication only
   */
   #define NINED3DSPSM_NONE    (D3DSPSM_NONE    >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_NEG     (D3DSPSM_NEG     >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_BIAS    (D3DSPSM_BIAS    >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_BIASNEG (D3DSPSM_BIASNEG >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_SIGN    (D3DSPSM_SIGN    >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_SIGNNEG (D3DSPSM_SIGNNEG >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_COMP    (D3DSPSM_COMP    >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_X2      (D3DSPSM_X2      >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_X2NEG   (D3DSPSM_X2NEG   >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_DZ      (D3DSPSM_DZ      >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_DW      (D3DSPSM_DW      >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_ABS     (D3DSPSM_ABS     >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_ABSNEG  (D3DSPSM_ABSNEG  >> D3DSP_SRCMOD_SHIFT)
   #define NINED3DSPSM_NOT     (D3DSPSM_NOT     >> D3DSP_SRCMOD_SHIFT)
      static const char *sm1_mod_str[] =
   {
      [NINED3DSPSM_NONE] = "",
   [NINED3DSPSM_NEG] = "-",
   [NINED3DSPSM_BIAS] = "bias",
   [NINED3DSPSM_BIASNEG] = "biasneg",
   [NINED3DSPSM_SIGN] = "sign",
   [NINED3DSPSM_SIGNNEG] = "signneg",
   [NINED3DSPSM_COMP] = "comp",
   [NINED3DSPSM_X2] = "x2",
   [NINED3DSPSM_X2NEG] = "x2neg",
   [NINED3DSPSM_DZ] = "dz",
   [NINED3DSPSM_DW] = "dw",
   [NINED3DSPSM_ABS] = "abs",
   [NINED3DSPSM_ABSNEG] = "-abs",
      };
      static void
   sm1_dump_writemask(BYTE mask)
   {
      if (mask & 1) DUMP("x"); else DUMP("_");
   if (mask & 2) DUMP("y"); else DUMP("_");
   if (mask & 4) DUMP("z"); else DUMP("_");
      }
      static void
   sm1_dump_swizzle(BYTE s)
   {
      char c[4] = { 'x', 'y', 'z', 'w' };
   DUMP("%c%c%c%c",
      }
      static const char sm1_file_char[] =
   {
      [D3DSPR_TEMP] = 'r',
   [D3DSPR_INPUT] = 'v',
   [D3DSPR_CONST] = 'c',
   [D3DSPR_ADDR] = 'A',
   [D3DSPR_RASTOUT] = 'R',
   [D3DSPR_ATTROUT] = 'D',
   [D3DSPR_OUTPUT] = 'o',
   [D3DSPR_CONSTINT] = 'I',
   [D3DSPR_COLOROUT] = 'C',
   [D3DSPR_DEPTHOUT] = 'D',
   [D3DSPR_SAMPLER] = 's',
   [D3DSPR_CONST2] = 'c',
   [D3DSPR_CONST3] = 'c',
   [D3DSPR_CONST4] = 'c',
   [D3DSPR_CONSTBOOL] = 'B',
   [D3DSPR_LOOP] = 'L',
   [D3DSPR_TEMPFLOAT16] = 'h',
   [D3DSPR_MISCTYPE] = 'M',
   [D3DSPR_LABEL] = 'X',
      };
      static void
   sm1_dump_reg(BYTE file, INT index)
   {
      switch (file) {
   case D3DSPR_LOOP:
      DUMP("aL");
      case D3DSPR_COLOROUT:
      DUMP("oC%i", index);
      case D3DSPR_DEPTHOUT:
      DUMP("oDepth");
      case D3DSPR_RASTOUT:
      DUMP("oRast%i", index);
      case D3DSPR_CONSTINT:
      DUMP("iconst[%i]", index);
      case D3DSPR_CONSTBOOL:
      DUMP("bconst[%i]", index);
      default:
      DUMP("%c%i", sm1_file_char[file], index);
         }
      struct sm1_src_param
   {
      INT idx;
   struct sm1_src_param *rel;
   BYTE file;
   BYTE swizzle;
   BYTE mod;
   BYTE type;
   union {
      DWORD d[4];
   float f[4];
   int i[4];
         };
   static void
   sm1_parse_immediate(struct shader_translator *, struct sm1_src_param *);
      struct sm1_dst_param
   {
      INT idx;
   struct sm1_src_param *rel;
   BYTE file;
   BYTE mask;
   BYTE mod;
   int8_t shift; /* sint4 */
      };
      static inline void
   assert_replicate_swizzle(const struct ureg_src *reg)
   {
      assert(reg->SwizzleY == reg->SwizzleX &&
            }
      static void
   sm1_dump_immediate(const struct sm1_src_param *param)
   {
      switch (param->type) {
   case NINED3DSPTYPE_FLOAT4:
      DUMP("{ %f %f %f %f }",
         param->imm.f[0], param->imm.f[1],
      case NINED3DSPTYPE_INT4:
      DUMP("{ %i %i %i %i }",
         param->imm.i[0], param->imm.i[1],
      case NINED3DSPTYPE_BOOL:
      DUMP("%s", param->imm.b ? "TRUE" : "FALSE");
      default:
      assert(0);
         }
      static void
   sm1_dump_src_param(const struct sm1_src_param *param)
   {
      if (param->file == NINED3DSPR_IMMEDIATE) {
      assert(!param->mod &&
               sm1_dump_immediate(param);
               if (param->mod)
         if (param->rel) {
      DUMP("%c[", sm1_file_char[param->file]);
   sm1_dump_src_param(param->rel);
      } else {
         }
   if (param->mod)
         if (param->swizzle != NINED3DSP_NOSWIZZLE) {
      DUMP(".");
         }
      static void
   sm1_dump_dst_param(const struct sm1_dst_param *param)
   {
      if (param->mod & NINED3DSPDM_SATURATE)
         if (param->mod & NINED3DSPDM_PARTIALP)
         if (param->mod & NINED3DSPDM_CENTROID)
         if (param->shift < 0)
         if (param->shift > 0)
            if (param->rel) {
      DUMP("%c[", sm1_file_char[param->file]);
   sm1_dump_src_param(param->rel);
      } else {
         }
   if (param->mask != NINED3DSP_WRITEMASK_ALL) {
      DUMP(".");
         }
      struct sm1_semantic
   {
      struct sm1_dst_param reg;
   BYTE sampler_type;
   D3DDECLUSAGE usage;
      };
      struct sm1_op_info
   {
      /* NOTE: 0 is a valid TGSI opcode, but if handler is set, this parameter
   * should be ignored completely */
   unsigned sio;
            /* versions are still set even handler is set */
   struct {
      unsigned min;
               /* number of regs parsed outside of special handler */
   unsigned ndst;
            /* some instructions don't map perfectly, so use a special handler */
      };
      struct sm1_instruction
   {
      D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode;
   BYTE flags;
   BOOL coissue;
   BOOL predicated;
   BYTE ndst;
   BYTE nsrc;
   struct sm1_src_param src[4];
   struct sm1_src_param src_rel[4];
   struct sm1_src_param pred;
   struct sm1_src_param dst_rel[1];
               };
      static void
   sm1_dump_instruction(struct sm1_instruction *insn, unsigned indent)
   {
               /* no info stored for these: */
   if (insn->opcode == D3DSIO_DCL)
         for (i = 0; i < indent; ++i)
            if (insn->predicated) {
      DUMP("@");
   sm1_dump_src_param(&insn->pred);
      }
   DUMP("%s", d3dsio_to_string(insn->opcode));
   if (insn->flags) {
      switch (insn->opcode) {
   case D3DSIO_TEX:
         DUMP(insn->flags == NINED3DSI_TEXLD_PROJECT ? "p" : "b");
   default:
         DUMP("_%x", insn->flags);
      }
   if (insn->coissue)
                  for (i = 0; i < insn->ndst && i < ARRAY_SIZE(insn->dst); ++i) {
      sm1_dump_dst_param(&insn->dst[i]);
               for (i = 0; i < insn->nsrc && i < ARRAY_SIZE(insn->src); ++i) {
      sm1_dump_src_param(&insn->src[i]);
      }
   if (insn->opcode == D3DSIO_DEF ||
      insn->opcode == D3DSIO_DEFI ||
   insn->opcode == D3DSIO_DEFB)
            }
      struct sm1_local_const
   {
      INT idx;
   struct ureg_src reg;
      };
      struct shader_translator
   {
      const DWORD *byte_code;
   const DWORD *parse;
                     /* shader version */
   struct {
      BYTE major;
      } version;
   unsigned processor; /* PIPE_SHADER_VERTEX/FRAMGENT */
   unsigned num_constf_allowed;
   unsigned num_consti_allowed;
            bool native_integers;
   bool inline_subroutines;
   bool want_texcoord;
   bool shift_wpos;
   bool wpos_is_sysval;
   bool face_is_sysval_integer;
   bool mul_zero_wins;
   bool always_output_pointsize;
   bool no_vs_window_space;
                     struct {
      struct ureg_dst *r;
   struct ureg_dst oPos;
   struct ureg_dst oPos_out; /* the real output when doing streamout or clipplane emulation */
   struct ureg_dst oFog;
   struct ureg_dst oPts;
   struct ureg_dst oCol[4];
   struct ureg_dst o[PIPE_MAX_SHADER_OUTPUTS];
   struct ureg_dst oDepth;
   struct ureg_src v[PIPE_MAX_SHADER_INPUTS];
   struct ureg_src v_consecutive; /* copy in temp array of ps inputs for rel addressing */
   struct ureg_src vPos;
   struct ureg_src vFace;
   struct ureg_src s;
   struct ureg_dst p;
   struct ureg_dst address;
   struct ureg_dst a0;
   struct ureg_dst predicate;
   struct ureg_dst predicate_tmp;
   struct ureg_dst predicate_dst;
   struct ureg_dst tS[8]; /* texture stage registers */
   struct ureg_dst tdst; /* scratch dst if we need extra modifiers */
   struct ureg_dst t[8]; /* scratch TEMPs */
   struct ureg_src vC[2]; /* PS color in */
   struct ureg_src vT[8]; /* PS texcoord in */
   struct ureg_dst rL[NINE_MAX_LOOP_DEPTH]; /* loop/rep ctr */
      } regs;
   unsigned num_temp; /* ARRAY_SIZE(regs.r) */
   unsigned num_scratch;
   unsigned loop_depth;
   unsigned loop_depth_max;
   unsigned cond_depth;
   unsigned loop_labels[NINE_MAX_LOOP_DEPTH];
   unsigned cond_labels[NINE_MAX_COND_DEPTH];
   bool loop_or_rep[NINE_MAX_LOOP_DEPTH]; /* true: loop, false: rep */
            unsigned *inst_labels; /* LABEL op */
                     struct sm1_local_const *lconstf;
   unsigned num_lconstf;
   struct sm1_local_const *lconsti;
   unsigned num_lconsti;
   struct sm1_local_const *lconstb;
            bool slots_used[NINE_MAX_CONST_ALL_VS];
   unsigned *slot_map;
            bool indirect_const_access;
            struct nine_vs_output_info output_info[16];
                        };
      #define IS_VS (tx->processor == PIPE_SHADER_VERTEX)
   #define IS_PS (tx->processor == PIPE_SHADER_FRAGMENT)
      #define FAILURE_VOID(cond) if ((cond)) {tx->failure=1;return;}
      static void
   sm1_read_semantic(struct shader_translator *, struct sm1_semantic *);
      static void
   sm1_instruction_check(const struct sm1_instruction *insn)
   {
      if (insn->opcode == D3DSIO_CRS)
   {
      if (insn->dst[0].mask & NINED3DSP_WRITEMASK_3)
   {
               }
      static void
   nine_record_outputs(struct shader_translator *tx, BYTE Usage, BYTE UsageIndex,
         {
      tx->output_info[tx->num_outputs].output_semantic = Usage;
   tx->output_info[tx->num_outputs].output_semantic_index = UsageIndex;
   tx->output_info[tx->num_outputs].mask = mask;
   tx->output_info[tx->num_outputs].output_index = output_index;
      }
      static struct ureg_src nine_float_constant_src(struct shader_translator *tx, int idx)
   {
               if (tx->slot_map)
         /* vswp constant handling: we use two buffers
   * to fit all the float constants. The special handling
   * doesn't need to be elsewhere, because all the instructions
   * accessing the constants directly are VS1, and swvp
   * is VS >= 2 */
   if (tx->info->swvp_on && idx >= 4096) {
      /* TODO: swvp rel is broken if many constants are used */
   src = ureg_src_register(TGSI_FILE_CONSTANT, idx - 4096);
      } else {
      src = ureg_src_register(TGSI_FILE_CONSTANT, idx);
               if (!tx->info->swvp_on)
         if (tx->info->const_float_slots < (idx + 1))
         if (tx->num_slots < (idx + 1))
               }
      static struct ureg_src nine_integer_constant_src(struct shader_translator *tx, int idx)
   {
               if (tx->info->swvp_on) {
      src = ureg_src_register(TGSI_FILE_CONSTANT, idx);
      } else {
      unsigned slot_idx = tx->info->const_i_base + idx;
   if (tx->slot_map)
         src = ureg_src_register(TGSI_FILE_CONSTANT, slot_idx);
   src = ureg_src_dimension(src, 0);
   tx->slots_used[slot_idx] = true;
   tx->info->int_slots_used[idx] = true;
   if (tx->num_slots < (slot_idx + 1))
               if (tx->info->const_int_slots < (idx + 1))
               }
      static struct ureg_src nine_boolean_constant_src(struct shader_translator *tx, int idx)
   {
               char r = idx / 4;
            if (tx->info->swvp_on) {
      src = ureg_src_register(TGSI_FILE_CONSTANT, r);
      } else {
      unsigned slot_idx = tx->info->const_b_base + r;
   if (tx->slot_map)
         src = ureg_src_register(TGSI_FILE_CONSTANT, slot_idx);
   src = ureg_src_dimension(src, 0);
   tx->slots_used[slot_idx] = true;
   tx->info->bool_slots_used[idx] = true;
   if (tx->num_slots < (slot_idx + 1))
      }
            if (tx->info->const_bool_slots < (idx + 1))
               }
      static struct ureg_src nine_special_constant_src(struct shader_translator *tx, int idx)
   {
               unsigned slot_idx = idx + (IS_PS ? NINE_MAX_CONST_PS_SPE_OFFSET :
            if (!tx->info->swvp_on && tx->slot_map)
         src = ureg_src_register(TGSI_FILE_CONSTANT, slot_idx);
            if (!tx->info->swvp_on)
         if (tx->num_slots < (slot_idx + 1))
               }
      static bool
   tx_lconstf(struct shader_translator *tx, struct ureg_src *src, INT index)
   {
               if (index < 0 || index >= tx->num_constf_allowed) {
      tx->failure = true;
      }
   for (i = 0; i < tx->num_lconstf; ++i) {
      if (tx->lconstf[i].idx == index) {
      *src = tx->lconstf[i].reg;
         }
      }
   static bool
   tx_lconsti(struct shader_translator *tx, struct ureg_src *src, INT index)
   {
               if (index < 0 || index >= tx->num_consti_allowed) {
      tx->failure = true;
      }
   for (i = 0; i < tx->num_lconsti; ++i) {
      if (tx->lconsti[i].idx == index) {
      *src = tx->lconsti[i].reg;
         }
      }
   static bool
   tx_lconstb(struct shader_translator *tx, struct ureg_src *src, INT index)
   {
               if (index < 0 || index >= tx->num_constb_allowed) {
      tx->failure = true;
      }
   for (i = 0; i < tx->num_lconstb; ++i) {
      if (tx->lconstb[i].idx == index) {
      *src = tx->lconstb[i].reg;
         }
      }
      static void
   tx_set_lconstf(struct shader_translator *tx, INT index, float f[4])
   {
                        for (n = 0; n < tx->num_lconstf; ++n)
      if (tx->lconstf[n].idx == index)
      if (n == tx->num_lconstf) {
      if ((n % 8) == 0) {
      tx->lconstf = REALLOC(tx->lconstf,
                  }
      }
   tx->lconstf[n].idx = index;
               }
   static void
   tx_set_lconsti(struct shader_translator *tx, INT index, int i[4])
   {
                        for (n = 0; n < tx->num_lconsti; ++n)
      if (tx->lconsti[n].idx == index)
      if (n == tx->num_lconsti) {
      if ((n % 8) == 0) {
      tx->lconsti = REALLOC(tx->lconsti,
                  }
               tx->lconsti[n].idx = index;
   tx->lconsti[n].reg = tx->native_integers ?
      ureg_imm4i(tx->ureg, i[0], i[1], i[2], i[3]) :
   }
   static void
   tx_set_lconstb(struct shader_translator *tx, INT index, BOOL b)
   {
                        for (n = 0; n < tx->num_lconstb; ++n)
      if (tx->lconstb[n].idx == index)
      if (n == tx->num_lconstb) {
      if ((n % 8) == 0) {
      tx->lconstb = REALLOC(tx->lconstb,
                  }
               tx->lconstb[n].idx = index;
   tx->lconstb[n].reg = tx->native_integers ?
      ureg_imm1u(tx->ureg, b ? 0xffffffff : 0) :
   }
      static inline struct ureg_dst
   tx_scratch(struct shader_translator *tx)
   {
      if (tx->num_scratch >= ARRAY_SIZE(tx->regs.t)) {
      tx->failure = true;
      }
   if (ureg_dst_is_undef(tx->regs.t[tx->num_scratch]))
            }
      static inline struct ureg_dst
   tx_scratch_scalar(struct shader_translator *tx)
   {
         }
      static inline struct ureg_src
   tx_src_scalar(struct ureg_dst dst)
   {
      struct ureg_src src = ureg_src(dst);
   int c = ffs(dst.WriteMask) - 1;
   if (dst.WriteMask == (1 << c))
            }
      static inline void
   tx_temp_alloc(struct shader_translator *tx, INT idx)
   {
      assert(idx >= 0);
   if (idx >= tx->num_temp) {
      unsigned k = tx->num_temp;
   unsigned n = idx + 1;
   tx->regs.r = REALLOC(tx->regs.r,
               for (; k < n; ++k)
            }
   if (ureg_dst_is_undef(tx->regs.r[idx]))
      }
      static inline void
   tx_addr_alloc(struct shader_translator *tx, INT idx)
   {
      assert(idx == 0);
   if (ureg_dst_is_undef(tx->regs.address))
         if (ureg_dst_is_undef(tx->regs.a0))
      }
      static inline bool
   TEX_if_fetch4(struct shader_translator *tx, struct ureg_dst dst,
               {
      struct ureg_dst tmp;
            if (!(tx->info->fetch4 & (1 << idx)))
                     tmp = tx_scratch(tx);
   ureg_tex_insn(tx->ureg, TGSI_OPCODE_TG4, &tmp, 1, target, TGSI_RETURN_TYPE_FLOAT,
         ureg_MOV(tx->ureg, dst, ureg_swizzle(ureg_src(tmp), NINE_SWIZZLE4(Z, X, Y, W)));
      }
      /* NOTE: It's not very clear on which ps1.1-ps1.3 instructions
   * the projection should be applied on the texture. It doesn't
   * apply on texkill.
   * The doc is very imprecise here (it says the projection is done
   * before rasterization, thus in vs, which seems wrong since ps instructions
   * are affected differently)
   * For now we only apply to the ps TEX instruction and TEXBEM.
   * Perhaps some other instructions would need it */
   static inline void
   apply_ps1x_projection(struct shader_translator *tx, struct ureg_dst dst,
         {
      struct ureg_dst tmp;
            /* no projection */
   if (dim == 1) {
         } else {
      tmp = tx_scratch_scalar(tx);
   ureg_RCP(tx->ureg, tmp, ureg_scalar(src, dim-1));
         }
      static inline void
   TEX_with_ps1x_projection(struct shader_translator *tx, struct ureg_dst dst,
               {
      unsigned dim = 1 + ((tx->info->projected >> (2 * idx)) & 3);
   struct ureg_dst tmp;
            /* dim == 1: no projection
   * Looks like must be disabled when it makes no
   * sense according the texture dimensions
   */
   if (dim == 1 || (dim <= target && !shadow)) {
         } else if (dim == 4) {
         } else {
      tmp = tx_scratch(tx);
   apply_ps1x_projection(tx, tmp, src0, idx);
         }
      static inline void
   tx_texcoord_alloc(struct shader_translator *tx, INT idx)
   {
      assert(IS_PS);
   assert(idx >= 0 && idx < ARRAY_SIZE(tx->regs.vT));
   if (ureg_src_is_undef(tx->regs.vT[idx]))
      tx->regs.vT[idx] = ureg_DECL_fs_input(tx->ureg, tx->texcoord_sn, idx,
   }
      static inline unsigned *
   tx_bgnloop(struct shader_translator *tx)
   {
      tx->loop_depth++;
   if (tx->loop_depth_max < tx->loop_depth)
         assert(tx->loop_depth < NINE_MAX_LOOP_DEPTH);
      }
      static inline unsigned *
   tx_endloop(struct shader_translator *tx)
   {
      assert(tx->loop_depth);
   tx->loop_depth--;
   ureg_fixup_label(tx->ureg, tx->loop_labels[tx->loop_depth],
            }
      static struct ureg_dst
   tx_get_loopctr(struct shader_translator *tx, bool loop_or_rep)
   {
               if (!tx->loop_depth)
   {
      DBG("loop counter requested outside of loop\n");
               if (ureg_dst_is_undef(tx->regs.rL[l])) {
      /* loop or rep ctr creation */
   tx->regs.rL[l] = ureg_DECL_local_temporary(tx->ureg);
   if (loop_or_rep)
            }
   /* loop - rep - endloop - endrep not allowed */
               }
      static struct ureg_dst
   tx_get_loopal(struct shader_translator *tx)
   {
               while (loop_level >= 0) {
      /* handle loop - rep - endrep - endloop case */
   if (tx->loop_or_rep[loop_level])
         /* the aL value is in the Y component (nine implementation) */
               DBG("aL counter requested outside of loop\n");
      }
      static inline unsigned *
   tx_cond(struct shader_translator *tx)
   {
      assert(tx->cond_depth <= NINE_MAX_COND_DEPTH);
   tx->cond_depth++;
      }
      static inline unsigned *
   tx_elsecond(struct shader_translator *tx)
   {
      assert(tx->cond_depth);
      }
      static inline void
   tx_endcond(struct shader_translator *tx)
   {
      assert(tx->cond_depth);
   tx->cond_depth--;
   ureg_fixup_label(tx->ureg, tx->cond_labels[tx->cond_depth],
      }
      static inline struct ureg_dst
   nine_ureg_dst_register(unsigned file, int index)
   {
         }
      static inline struct ureg_src
   nine_get_position_input(struct shader_translator *tx)
   {
               if (tx->wpos_is_sysval)
         else
      return ureg_DECL_fs_input(ureg, TGSI_SEMANTIC_POSITION,
   }
      static struct ureg_src
   tx_src_param(struct shader_translator *tx, const struct sm1_src_param *param)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_src src;
            assert(!param->rel || (IS_VS && param->file == D3DSPR_CONST) ||
            switch (param->file)
   {
   case D3DSPR_TEMP:
      tx_temp_alloc(tx, param->idx);
   src = ureg_src(tx->regs.r[param->idx]);
   /* case D3DSPR_TEXTURE: == D3DSPR_ADDR */
      case D3DSPR_ADDR:
      if (IS_VS) {
         assert(param->idx == 0);
   /* the address register (vs only) must be
   * assigned before use */
   assert(!ureg_dst_is_undef(tx->regs.a0));
   /* Round to lowest for vs1.1 (contrary to the doc), else
   * round to nearest */
   if (tx->version.major < 2 && tx->version.minor < 2)
         else
         } else {
         if (tx->version.major < 2 && tx->version.minor < 4) {
      /* no subroutines, so should be defined */
      } else {
      tx_texcoord_alloc(tx, param->idx);
      }
      case D3DSPR_INPUT:
      if (IS_VS) {
         } else {
         if (tx->version.major < 3) {
      src = ureg_DECL_fs_input_centroid(
      ureg, TGSI_SEMANTIC_COLOR, param->idx,
   tx->info->color_flatshade ? TGSI_INTERPOLATE_CONSTANT : TGSI_INTERPOLATE_PERSPECTIVE,
   tx->info->force_color_in_centroid ?
         } else {
      if(param->rel) {
      /* Copy all inputs (non consecutive)
      * to temp array (consecutive).
   * This is not good for performance.
   * A better way would be to have inputs
   * consecutive (would need implement alternative
   * way to match vs outputs and ps inputs).
   * However even with the better way, the temp array
   * copy would need to be used if some inputs
   * are not GENERIC or if they have different
      if (ureg_src_is_undef(tx->regs.v_consecutive)) {
         int i;
   tx->regs.v_consecutive = ureg_src(ureg_DECL_array_temporary(ureg, 10, 0));
   for (i = 0; i < 10; i++) {
      if (!ureg_src_is_undef(tx->regs.v[i]))
         else
      }
      } else {
      assert(param->idx < ARRAY_SIZE(tx->regs.v));
         }
   if (param->rel)
            case D3DSPR_PREDICATE:
      if (ureg_dst_is_undef(tx->regs.predicate)) {
         /* Forbidden to use the predicate register before being set */
   tx->failure = true;
   }
   src = ureg_src(tx->regs.predicate);
      case D3DSPR_SAMPLER:
      assert(param->mod == NINED3DSPSM_NONE);
   /* assert(param->swizzle == NINED3DSP_NOSWIZZLE); Passed by wine tests */
   src = ureg_DECL_sampler(ureg, param->idx);
      case D3DSPR_CONST:
      if (param->rel || !tx_lconstf(tx, &src, param->idx)) {
         src = nine_float_constant_src(tx, param->idx);
   if (param->rel) {
      tx->indirect_const_access = true;
      }
   if (!IS_VS && tx->version.major < 2) {
         /* ps 1.X clamps constants */
   tmp = tx_scratch(tx);
   ureg_MIN(ureg, tmp, src, ureg_imm1f(ureg, 1.0f));
   ureg_MAX(ureg, tmp, ureg_src(tmp), ureg_imm1f(ureg, -1.0f));
   }
      case D3DSPR_CONST2:
   case D3DSPR_CONST3:
   case D3DSPR_CONST4:
      DBG("CONST2/3/4 should have been collapsed into D3DSPR_CONST !\n");
   assert(!"CONST2/3/4");
   src = ureg_imm1f(ureg, 0.0f);
      case D3DSPR_CONSTINT:
      /* relative adressing only possible for float constants in vs */
   if (!tx_lconsti(tx, &src, param->idx))
            case D3DSPR_CONSTBOOL:
      if (!tx_lconstb(tx, &src, param->idx))
            case D3DSPR_LOOP:
      if (ureg_dst_is_undef(tx->regs.address))
         if (!tx->native_integers)
         ureg_ARR(ureg, tx->regs.address,
   else
         ureg_UARL(ureg, tx->regs.address,
   src = ureg_src(tx->regs.address);
      case D3DSPR_MISCTYPE:
      switch (param->idx) {
   case D3DSMO_POSITION:
      if (ureg_src_is_undef(tx->regs.vPos))
         if (tx->shift_wpos) {
         /* TODO: do this only once */
   struct ureg_dst wpos = tx_scratch(tx);
   ureg_ADD(ureg, wpos, tx->regs.vPos,
         } else {
         }
      case D3DSMO_FACE:
      if (ureg_src_is_undef(tx->regs.vFace)) {
         if (tx->face_is_sysval_integer) {
                           /* convert bool to float */
   ureg_UCMP(ureg, tmp, ureg_scalar(tx->regs.vFace, TGSI_SWIZZLE_X),
            } else {
      tx->regs.vFace = ureg_DECL_fs_input(ureg,
            }
   }
   src = tx->regs.vFace;
      default:
         assert(!"invalid src D3DSMO");
   }
      case D3DSPR_TEMPFLOAT16:
         default:
                  switch (param->mod) {
   case NINED3DSPSM_DW:
      tmp = tx_scratch(tx);
   /* NOTE: app is not allowed to read w with this modifier */
   ureg_RCP(ureg, ureg_writemask(tmp, NINED3DSP_WRITEMASK_3), ureg_scalar(src, TGSI_SWIZZLE_W));
   ureg_MUL(ureg, tmp, src, ureg_swizzle(ureg_src(tmp), NINE_SWIZZLE4(W,W,W,W)));
   src = ureg_src(tmp);
      case NINED3DSPSM_DZ:
      tmp = tx_scratch(tx);
   /* NOTE: app is not allowed to read z with this modifier */
   ureg_RCP(ureg, ureg_writemask(tmp, NINED3DSP_WRITEMASK_2), ureg_scalar(src, TGSI_SWIZZLE_Z));
   ureg_MUL(ureg, tmp, src, ureg_swizzle(ureg_src(tmp), NINE_SWIZZLE4(Z,Z,Z,Z)));
   src = ureg_src(tmp);
      default:
                  if (param->swizzle != NINED3DSP_NOSWIZZLE && param->file != D3DSPR_SAMPLER)
      src = ureg_swizzle(src,
                           switch (param->mod) {
   case NINED3DSPSM_ABS:
      src = ureg_abs(src);
      case NINED3DSPSM_ABSNEG:
      src = ureg_negate(ureg_abs(src));
      case NINED3DSPSM_NEG:
      src = ureg_negate(src);
      case NINED3DSPSM_BIAS:
      tmp = tx_scratch(tx);
   ureg_ADD(ureg, tmp, src, ureg_imm1f(ureg, -0.5f));
   src = ureg_src(tmp);
      case NINED3DSPSM_BIASNEG:
      tmp = tx_scratch(tx);
   ureg_ADD(ureg, tmp, ureg_imm1f(ureg, 0.5f), ureg_negate(src));
   src = ureg_src(tmp);
      case NINED3DSPSM_NOT:
      if (tx->native_integers && param->file == D3DSPR_CONSTBOOL) {
         tmp = tx_scratch(tx);
   ureg_NOT(ureg, tmp, src);
   src = ureg_src(tmp);
   } else { /* predicate */
         tmp = tx_scratch(tx);
   ureg_ADD(ureg, tmp, ureg_imm1f(ureg, 1.0f), ureg_negate(src));
   }
      case NINED3DSPSM_COMP:
      tmp = tx_scratch(tx);
   ureg_ADD(ureg, tmp, ureg_imm1f(ureg, 1.0f), ureg_negate(src));
   src = ureg_src(tmp);
      case NINED3DSPSM_DZ:
   case NINED3DSPSM_DW:
      /* Already handled*/
      case NINED3DSPSM_SIGN:
      tmp = tx_scratch(tx);
   ureg_MAD(ureg, tmp, src, ureg_imm1f(ureg, 2.0f), ureg_imm1f(ureg, -1.0f));
   src = ureg_src(tmp);
      case NINED3DSPSM_SIGNNEG:
      tmp = tx_scratch(tx);
   ureg_MAD(ureg, tmp, src, ureg_imm1f(ureg, -2.0f), ureg_imm1f(ureg, 1.0f));
   src = ureg_src(tmp);
      case NINED3DSPSM_X2:
      tmp = tx_scratch(tx);
   ureg_ADD(ureg, tmp, src, src);
   src = ureg_src(tmp);
      case NINED3DSPSM_X2NEG:
      tmp = tx_scratch(tx);
   ureg_ADD(ureg, tmp, src, src);
   src = ureg_negate(ureg_src(tmp));
      default:
      assert(param->mod == NINED3DSPSM_NONE);
                  }
      static struct ureg_dst
   _tx_dst_param(struct shader_translator *tx, const struct sm1_dst_param *param)
   {
               switch (param->file)
   {
   case D3DSPR_TEMP:
      assert(!param->rel);
   tx_temp_alloc(tx, param->idx);
   dst = tx->regs.r[param->idx];
   /* case D3DSPR_TEXTURE: == D3DSPR_ADDR */
      case D3DSPR_ADDR:
      assert(!param->rel);
   if (tx->version.major < 2 && !IS_VS) {
         if (ureg_dst_is_undef(tx->regs.tS[param->idx]))
         } else
   if (!IS_VS && tx->insn.opcode == D3DSIO_TEXKILL) { /* maybe others, too */
         tx_texcoord_alloc(tx, param->idx);
   } else {
         tx_addr_alloc(tx, param->idx);
   }
      case D3DSPR_RASTOUT:
      assert(!param->rel);
   switch (param->idx) {
   case 0:
         if (ureg_dst_is_undef(tx->regs.oPos)) {
      if (tx->info->clip_plane_emulation > 0) {
         } else {
            }
   dst = tx->regs.oPos;
   case 1:
         if (ureg_dst_is_undef(tx->regs.oFog))
      tx->regs.oFog =
      dst = tx->regs.oFog;
   case 2:
         if (ureg_dst_is_undef(tx->regs.oPts))
         dst = tx->regs.oPts;
   default:
         assert(0);
   }
   /* case D3DSPR_TEXCRDOUT: == D3DSPR_OUTPUT */
      case D3DSPR_OUTPUT:
      if (tx->version.major < 3) {
         assert(!param->rel);
   } else {
         assert(!param->rel); /* TODO */
   assert(param->idx < ARRAY_SIZE(tx->regs.o));
   }
      case D3DSPR_ATTROUT: /* VS */
   case D3DSPR_COLOROUT: /* PS */
      assert(param->idx >= 0 && param->idx < 4);
   assert(!param->rel);
   tx->info->rt_mask |= 1 << param->idx;
   if (ureg_dst_is_undef(tx->regs.oCol[param->idx])) {
         /* ps < 3: oCol[0] will have fog blending afterward
   * ps: oCol[0] might have alphatest afterward */
   if (!IS_VS && param->idx == 0) {
         } else {
      tx->regs.oCol[param->idx] =
      }
   dst = tx->regs.oCol[param->idx];
   if (IS_VS && tx->version.major < 3)
            case D3DSPR_DEPTHOUT:
      assert(!param->rel);
   if (ureg_dst_is_undef(tx->regs.oDepth))
      tx->regs.oDepth =
      ureg_DECL_output_masked(tx->ureg, TGSI_SEMANTIC_POSITION, 0,
   dst = tx->regs.oDepth; /* XXX: must write .z component */
      case D3DSPR_PREDICATE:
      if (ureg_dst_is_undef(tx->regs.predicate))
         dst = tx->regs.predicate;
      case D3DSPR_TEMPFLOAT16:
      DBG("unhandled D3DSPR: %u\n", param->file);
      default:
      assert(!"invalid dst D3DSPR");
      }
   if (param->rel)
            if (param->mask != NINED3DSP_WRITEMASK_ALL)
         if (param->mod & NINED3DSPDM_SATURATE)
            if (tx->predicated_activated) {
      tx->regs.predicate_dst = dst;
                  }
      static struct ureg_dst
   tx_dst_param(struct shader_translator *tx, const struct sm1_dst_param *param)
   {
      if (param->shift) {
      tx->regs.tdst = ureg_writemask(tx_scratch(tx), param->mask);
      }
      }
      static void
   tx_apply_dst0_modifiers(struct shader_translator *tx)
   {
      struct ureg_dst rdst;
            if (!tx->insn.ndst || !tx->insn.dst[0].shift || tx->insn.opcode == D3DSIO_TEXKILL)
                           if (tx->insn.dst[0].shift < 0)
         else
               }
      static struct ureg_src
   tx_dst_param_as_src(struct shader_translator *tx, const struct sm1_dst_param *param)
   {
               assert(!param->shift);
            switch (param->file) {
   case D3DSPR_INPUT:
      if (IS_VS) {
         } else {
         assert(!param->rel);
   assert(param->idx < ARRAY_SIZE(tx->regs.v));
   }
      default:
      src = ureg_src(tx_dst_param(tx, param));
      }
   if (param->rel)
            if (!param->mask)
            if (param->mask && param->mask != NINED3DSP_WRITEMASK_ALL) {
      char s[4];
   int n;
   int c;
   for (n = 0, c = 0; c < 4; ++c)
         if (param->mask & (1 << c))
   assert(n);
   for (c = n; c < 4; ++c)
            }
      }
      static HRESULT
   NineTranslateInstruction_Mkxn(struct shader_translator *tx, const unsigned k, const unsigned n)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst;
   struct ureg_src src[2];
   struct sm1_src_param *src_mat = &tx->insn.src[1];
            dst = tx_dst_param(tx, &tx->insn.dst[0]);
            for (i = 0; i < n; i++)
   {
               src[1] = tx_src_param(tx, src_mat);
            if (!(dst.WriteMask & m))
                     switch (k) {
   case 3:
         ureg_DP3(ureg, ureg_writemask(dst, m), src[0], src[1]);
   case 4:
         ureg_DP4(ureg, ureg_writemask(dst, m), src[0], src[1]);
   default:
         DBG("invalid operation: M%ux%u\n", m, n);
                  }
      #define VNOTSUPPORTED   0, 0
   #define V(maj, min)     (((maj) << 8) | (min))
      static inline const char *
   d3dsio_to_string( unsigned opcode )
   {
      static const char *names[] = {
      "NOP",
   "MOV",
   "ADD",
   "SUB",
   "MAD",
   "MUL",
   "RCP",
   "RSQ",
   "DP3",
   "DP4",
   "MIN",
   "MAX",
   "SLT",
   "SGE",
   "EXP",
   "LOG",
   "LIT",
   "DST",
   "LRP",
   "FRC",
   "M4x4",
   "M4x3",
   "M3x4",
   "M3x3",
   "M3x2",
   "CALL",
   "CALLNZ",
   "LOOP",
   "RET",
   "ENDLOOP",
   "LABEL",
   "DCL",
   "POW",
   "CRS",
   "SGN",
   "ABS",
   "NRM",
   "SINCOS",
   "REP",
   "ENDREP",
   "IF",
   "IFC",
   "ELSE",
   "ENDIF",
   "BREAK",
   "BREAKC",
   "MOVA",
   "DEFB",
   "DEFI",
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   "TEXCOORD",
   "TEXKILL",
   "TEX",
   "TEXBEM",
   "TEXBEML",
   "TEXREG2AR",
   "TEXREG2GB",
   "TEXM3x2PAD",
   "TEXM3x2TEX",
   "TEXM3x3PAD",
   "TEXM3x3TEX",
   NULL,
   "TEXM3x3SPEC",
   "TEXM3x3VSPEC",
   "EXPP",
   "LOGP",
   "CND",
   "DEF",
   "TEXREG2RGB",
   "TEXDP3TEX",
   "TEXM3x2DEPTH",
   "TEXDP3",
   "TEXM3x3",
   "TEXDEPTH",
   "CMP",
   "BEM",
   "DP2ADD",
   "DSX",
   "DSY",
   "TEXLDD",
   "SETP",
   "TEXLDL",
                        switch (opcode) {
   case D3DSIO_PHASE: return "PHASE";
   case D3DSIO_COMMENT: return "COMMENT";
   case D3DSIO_END: return "END";
   default:
            }
      #define NULL_INSTRUCTION            { 0, { 0, 0 }, { 0, 0 }, 0, 0, NULL }
   #define IS_VALID_INSTRUCTION(inst)  ((inst).vert_version.min | \
                        #define SPECIAL(name) \
            #define DECL_SPECIAL(name) \
      static HRESULT \
         static HRESULT
   NineTranslateInstruction_Generic(struct shader_translator *);
      DECL_SPECIAL(NOP)
   {
      /* Nothing to do. NOP was used to avoid hangs
   * with very old d3d drivers. */
      }
      DECL_SPECIAL(SUB)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src0 = tx_src_param(tx, &tx->insn.src[0]);
            ureg_ADD(ureg, dst, src0, ureg_negate(src1));
      }
      DECL_SPECIAL(ABS)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
            ureg_MOV(ureg, dst, ureg_abs(src));
      }
      DECL_SPECIAL(XPD)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src0 = tx_src_param(tx, &tx->insn.src[0]);
            ureg_MUL(ureg, ureg_writemask(dst, TGSI_WRITEMASK_XYZ),
            ureg_swizzle(src0, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Z,
            ureg_MAD(ureg, ureg_writemask(dst, TGSI_WRITEMASK_XYZ),
            ureg_swizzle(src0, TGSI_SWIZZLE_Z, TGSI_SWIZZLE_X,
         ureg_negate(ureg_swizzle(src1, TGSI_SWIZZLE_Y,
      ureg_MOV(ureg, ureg_writemask(dst, TGSI_WRITEMASK_W),
            }
      DECL_SPECIAL(M4x4)
   {
         }
      DECL_SPECIAL(M4x3)
   {
         }
      DECL_SPECIAL(M3x4)
   {
         }
      DECL_SPECIAL(M3x3)
   {
         }
      DECL_SPECIAL(M3x2)
   {
         }
      DECL_SPECIAL(CMP)
   {
      ureg_CMP(tx->ureg, tx_dst_param(tx, &tx->insn.dst[0]),
            tx_src_param(tx, &tx->insn.src[0]),
         }
      DECL_SPECIAL(CND)
   {
      struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_dst cgt;
            /* the coissue flag was a tip for compilers to advise to
   * execute two operations at the same time, in cases
   * the two executions had same dst with different channels.
   * It has no effect on current hw. However it seems CND
   * is affected. The handling of this very specific case
   * handled below mimick wine behaviour */
   if (tx->insn.coissue && tx->version.major == 1 && tx->version.minor < 4 && tx->insn.dst[0].mask != NINED3DSP_WRITEMASK_3) {
      ureg_MOV(tx->ureg,
                     cnd = tx_src_param(tx, &tx->insn.src[0]);
            if (tx->version.major == 1 && tx->version.minor < 4)
                     ureg_CMP(tx->ureg, dst, ureg_negate(ureg_src(cgt)),
                  }
      DECL_SPECIAL(CALL)
   {
      assert(tx->insn.src[0].idx < tx->num_inst_labels);
   ureg_CAL(tx->ureg, &tx->inst_labels[tx->insn.src[0].idx]);
      }
      DECL_SPECIAL(CALLNZ)
   {
      struct ureg_program *ureg = tx->ureg;
            if (!tx->native_integers)
         else
         ureg_CAL(ureg, &tx->inst_labels[tx->insn.src[0].idx]);
   tx_endcond(tx);
   ureg_ENDIF(ureg);
      }
      DECL_SPECIAL(LOOP)
   {
      struct ureg_program *ureg = tx->ureg;
   unsigned *label;
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[1]);
   struct ureg_dst ctr;
   struct ureg_dst aL;
   struct ureg_dst tmp;
            label = tx_bgnloop(tx);
   ctr = tx_get_loopctr(tx, true);
   aL = tx_get_loopal(tx);
            /* src: num_iterations*/
   ureg_MOV(ureg, ureg_writemask(ctr, NINED3DSP_WRITEMASK_0),
         /* al: unused - start_value of al - step for al - unused */
   ureg_MOV(ureg, aL, src);
   ureg_BGNLOOP(tx->ureg, label);
   tmp = tx_scratch_scalar(tx);
   /* Initially ctr.x contains the number of iterations.
   * ctr.y will contain the updated value of al.
   * We decrease ctr.x at the end of every iteration,
            if (!tx->native_integers) {
      /* case src and ctr contain floats */
   /* to avoid precision issue, we stop when ctr <= 0.5 */
   ureg_SGE(ureg, tmp, ureg_imm1f(ureg, 0.5f), ctrx);
      } else {
      /* case src and ctr contain integers */
   ureg_ISGE(ureg, tmp, ureg_imm1i(ureg, 0), ctrx);
      }
   ureg_BRK(ureg);
   tx_endcond(tx);
   ureg_ENDIF(ureg);
      }
      DECL_SPECIAL(RET)
   {
      /* RET as a last instruction could be safely ignored.
   * Remove it to prevent crashes/warnings in case underlying
   * driver doesn't implement arbitrary returns.
   */
   if (*(tx->parse_next) != NINED3DSP_END) {
         }
      }
      DECL_SPECIAL(ENDLOOP)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst ctr = tx_get_loopctr(tx, true);
   struct ureg_dst al = tx_get_loopal(tx);
   struct ureg_dst dst_ctrx, dst_al;
            dst_ctrx = ureg_writemask(ctr, NINED3DSP_WRITEMASK_0);
   dst_al = ureg_writemask(al, NINED3DSP_WRITEMASK_1);
   src_ctr = ureg_src(ctr);
            /* ctr.x -= 1
   * al.y (aL) += step */
   if (!tx->native_integers) {
      ureg_ADD(ureg, dst_ctrx, src_ctr, ureg_imm1f(ureg, -1.0f));
      } else {
      ureg_UADD(ureg, dst_ctrx, src_ctr, ureg_imm1i(ureg, -1));
      }
   ureg_ENDLOOP(tx->ureg, tx_endloop(tx));
      }
      DECL_SPECIAL(LABEL)
   {
      unsigned k = tx->num_inst_labels;
   unsigned n = tx->insn.src[0].idx;
   assert(n < 2048);
   if (n >= k)
      tx->inst_labels = REALLOC(tx->inst_labels,
               tx->inst_labels[n] = ureg_get_instruction_number(tx->ureg);
      }
      DECL_SPECIAL(SINCOS)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]);
                     /* Copying to a temporary register avoids src/dst aliasing.
   * src is supposed to have replicated swizzle. */
            /* z undefined, w untouched */
   ureg_COS(ureg, ureg_writemask(dst, TGSI_WRITEMASK_X),
         ureg_SIN(ureg, ureg_writemask(dst, TGSI_WRITEMASK_Y),
            }
      DECL_SPECIAL(SGN)
   {
      ureg_SSG(tx->ureg,
                  }
      DECL_SPECIAL(REP)
   {
      struct ureg_program *ureg = tx->ureg;
   unsigned *label;
   struct ureg_src rep = tx_src_param(tx, &tx->insn.src[0]);
   struct ureg_dst ctr;
   struct ureg_dst tmp;
            label = tx_bgnloop(tx);
   ctr = ureg_writemask(tx_get_loopctr(tx, false), NINED3DSP_WRITEMASK_0);
            /* NOTE: rep must be constant, so we don't have to save the count */
            /* rep: num_iterations - 0 - 0 - 0 */
   ureg_MOV(ureg, ctr, rep);
   ureg_BGNLOOP(ureg, label);
   tmp = tx_scratch_scalar(tx);
   /* Initially ctr.x contains the number of iterations.
   * We decrease ctr.x at the end of every iteration,
            if (!tx->native_integers) {
      /* case src and ctr contain floats */
   /* to avoid precision issue, we stop when ctr <= 0.5 */
   ureg_SGE(ureg, tmp, ureg_imm1f(ureg, 0.5f), ctrx);
      } else {
      /* case src and ctr contain integers */
   ureg_ISGE(ureg, tmp, ureg_imm1i(ureg, 0), ctrx);
      }
   ureg_BRK(ureg);
   tx_endcond(tx);
               }
      DECL_SPECIAL(ENDREP)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst ctr = tx_get_loopctr(tx, false);
   struct ureg_dst dst_ctrx = ureg_writemask(ctr, NINED3DSP_WRITEMASK_0);
            /* ctr.x -= 1 */
   if (!tx->native_integers)
         else
            ureg_ENDLOOP(tx->ureg, tx_endloop(tx));
      }
      DECL_SPECIAL(ENDIF)
   {
      tx_endcond(tx);
   ureg_ENDIF(tx->ureg);
      }
      DECL_SPECIAL(IF)
   {
               if (tx->native_integers && tx->insn.src[0].file == D3DSPR_CONSTBOOL)
         else
               }
      static inline unsigned
   sm1_insn_flags_to_tgsi_setop(BYTE flags)
   {
      switch (flags) {
   case NINED3DSHADER_REL_OP_GT: return TGSI_OPCODE_SGT;
   case NINED3DSHADER_REL_OP_EQ: return TGSI_OPCODE_SEQ;
   case NINED3DSHADER_REL_OP_GE: return TGSI_OPCODE_SGE;
   case NINED3DSHADER_REL_OP_LT: return TGSI_OPCODE_SLT;
   case NINED3DSHADER_REL_OP_NE: return TGSI_OPCODE_SNE;
   case NINED3DSHADER_REL_OP_LE: return TGSI_OPCODE_SLE;
   default:
      assert(!"invalid comparison flags");
         }
      DECL_SPECIAL(IFC)
   {
      const unsigned cmp_op = sm1_insn_flags_to_tgsi_setop(tx->insn.flags);
   struct ureg_src src[2];
   struct ureg_dst tmp = ureg_writemask(tx_scratch(tx), TGSI_WRITEMASK_X);
   src[0] = tx_src_param(tx, &tx->insn.src[0]);
   src[1] = tx_src_param(tx, &tx->insn.src[1]);
   ureg_insn(tx->ureg, cmp_op, &tmp, 1, src, 2, 0);
   ureg_IF(tx->ureg, ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X), tx_cond(tx));
      }
      DECL_SPECIAL(ELSE)
   {
      ureg_ELSE(tx->ureg, tx_elsecond(tx));
      }
      DECL_SPECIAL(BREAKC)
   {
      const unsigned cmp_op = sm1_insn_flags_to_tgsi_setop(tx->insn.flags);
   struct ureg_src src[2];
   struct ureg_dst tmp = ureg_writemask(tx_scratch(tx), TGSI_WRITEMASK_X);
   src[0] = tx_src_param(tx, &tx->insn.src[0]);
   src[1] = tx_src_param(tx, &tx->insn.src[1]);
   ureg_insn(tx->ureg, cmp_op, &tmp, 1, src, 2, 0);
   ureg_IF(tx->ureg, ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X), tx_cond(tx));
   ureg_BRK(tx->ureg);
   tx_endcond(tx);
   ureg_ENDIF(tx->ureg);
      }
      static const char *sm1_declusage_names[] =
   {
      [D3DDECLUSAGE_POSITION] = "POSITION",
   [D3DDECLUSAGE_BLENDWEIGHT] = "BLENDWEIGHT",
   [D3DDECLUSAGE_BLENDINDICES] = "BLENDINDICES",
   [D3DDECLUSAGE_NORMAL] = "NORMAL",
   [D3DDECLUSAGE_PSIZE] = "PSIZE",
   [D3DDECLUSAGE_TEXCOORD] = "TEXCOORD",
   [D3DDECLUSAGE_TANGENT] = "TANGENT",
   [D3DDECLUSAGE_BINORMAL] = "BINORMAL",
   [D3DDECLUSAGE_TESSFACTOR] = "TESSFACTOR",
   [D3DDECLUSAGE_POSITIONT] = "POSITIONT",
   [D3DDECLUSAGE_COLOR] = "COLOR",
   [D3DDECLUSAGE_FOG] = "FOG",
   [D3DDECLUSAGE_DEPTH] = "DEPTH",
      };
      static inline unsigned
   sm1_to_nine_declusage(struct sm1_semantic *dcl)
   {
         }
      static void
   sm1_declusage_to_tgsi(struct tgsi_declaration_semantic *sem,
               {
               /* For everything that is not matching to a TGSI_SEMANTIC_****,
   * we match to a TGSI_SEMANTIC_GENERIC with index.
   *
   * The index can be anything UINT16 and usage_idx is BYTE,
   * so we can fit everything. It doesn't matter if indices
   * are close together or low.
   *
   *
   * POSITION >= 1: 10 * index + 7
   * COLOR >= 2: 10 * (index-1) + 8
   * FOG: 16
   * TEXCOORD[0..15]: index
   * BLENDWEIGHT: 10 * index + 19
   * BLENDINDICES: 10 * index + 20
   * NORMAL: 10 * index + 21
   * TANGENT: 10 * index + 22
   * BINORMAL: 10 * index + 23
   * TESSFACTOR: 10 * index + 24
            switch (dcl->usage) {
   case D3DDECLUSAGE_POSITION:
   case D3DDECLUSAGE_POSITIONT:
   case D3DDECLUSAGE_DEPTH:
      if (index == 0) {
         sem->Name = TGSI_SEMANTIC_POSITION;
   } else {
         sem->Name = TGSI_SEMANTIC_GENERIC;
   }
      case D3DDECLUSAGE_COLOR:
      if (index < 2) {
         sem->Name = TGSI_SEMANTIC_COLOR;
   } else {
         sem->Name = TGSI_SEMANTIC_GENERIC;
   }
      case D3DDECLUSAGE_FOG:
      assert(index == 0);
   sem->Name = TGSI_SEMANTIC_GENERIC;
   sem->Index = 16;
      case D3DDECLUSAGE_PSIZE:
      assert(index == 0);
   sem->Name = TGSI_SEMANTIC_PSIZE;
   sem->Index = 0;
      case D3DDECLUSAGE_TEXCOORD:
      assert(index < 16);
   if (index < 8 && tc)
         else
         sem->Index = index;
      case D3DDECLUSAGE_BLENDWEIGHT:
      sem->Name = TGSI_SEMANTIC_GENERIC;
   sem->Index = 10 * index + 19;
      case D3DDECLUSAGE_BLENDINDICES:
      sem->Name = TGSI_SEMANTIC_GENERIC;
   sem->Index = 10 * index + 20;
      case D3DDECLUSAGE_NORMAL:
      sem->Name = TGSI_SEMANTIC_GENERIC;
   sem->Index = 10 * index + 21;
      case D3DDECLUSAGE_TANGENT:
      sem->Name = TGSI_SEMANTIC_GENERIC;
   sem->Index = 10 * index + 22;
      case D3DDECLUSAGE_BINORMAL:
      sem->Name = TGSI_SEMANTIC_GENERIC;
   sem->Index = 10 * index + 23;
      case D3DDECLUSAGE_TESSFACTOR:
      sem->Name = TGSI_SEMANTIC_GENERIC;
   sem->Index = 10 * index + 24;
      case D3DDECLUSAGE_SAMPLE:
      sem->Name = TGSI_SEMANTIC_COUNT;
   sem->Index = 0;
      default:
      unreachable("Invalid DECLUSAGE.");
         }
      #define NINED3DSTT_1D     (D3DSTT_1D >> D3DSP_TEXTURETYPE_SHIFT)
   #define NINED3DSTT_2D     (D3DSTT_2D >> D3DSP_TEXTURETYPE_SHIFT)
   #define NINED3DSTT_VOLUME (D3DSTT_VOLUME >> D3DSP_TEXTURETYPE_SHIFT)
   #define NINED3DSTT_CUBE   (D3DSTT_CUBE >> D3DSP_TEXTURETYPE_SHIFT)
   static inline unsigned
   d3dstt_to_tgsi_tex(BYTE sampler_type)
   {
      switch (sampler_type) {
   case NINED3DSTT_1D:     return TGSI_TEXTURE_1D;
   case NINED3DSTT_2D:     return TGSI_TEXTURE_2D;
   case NINED3DSTT_VOLUME: return TGSI_TEXTURE_3D;
   case NINED3DSTT_CUBE:   return TGSI_TEXTURE_CUBE;
   default:
      assert(0);
         }
   static inline unsigned
   d3dstt_to_tgsi_tex_shadow(BYTE sampler_type)
   {
      switch (sampler_type) {
   case NINED3DSTT_1D: return TGSI_TEXTURE_SHADOW1D;
   case NINED3DSTT_2D: return TGSI_TEXTURE_SHADOW2D;
   case NINED3DSTT_VOLUME:
   case NINED3DSTT_CUBE:
   default:
      assert(0);
         }
   static inline unsigned
   ps1x_sampler_type(const struct nine_shader_info *info, unsigned stage)
   {
      bool shadow = !!(info->sampler_mask_shadow & (1 << stage));
   switch ((info->sampler_ps1xtypes >> (stage * 2)) & 0x3) {
   case 1: return shadow ? TGSI_TEXTURE_SHADOW1D : TGSI_TEXTURE_1D;
   case 0: return shadow ? TGSI_TEXTURE_SHADOW2D : TGSI_TEXTURE_2D;
   case 3: return TGSI_TEXTURE_3D;
   default:
            }
      static const char *
   sm1_sampler_type_name(BYTE sampler_type)
   {
      switch (sampler_type) {
   case NINED3DSTT_1D:     return "1D";
   case NINED3DSTT_2D:     return "2D";
   case NINED3DSTT_VOLUME: return "VOLUME";
   case NINED3DSTT_CUBE:   return "CUBE";
   default:
            }
      static inline unsigned
   nine_tgsi_to_interp_mode(struct tgsi_declaration_semantic *sem)
   {
      switch (sem->Name) {
   case TGSI_SEMANTIC_POSITION:
   case TGSI_SEMANTIC_NORMAL:
         case TGSI_SEMANTIC_BCOLOR:
   case TGSI_SEMANTIC_COLOR:
         case TGSI_SEMANTIC_FOG:
   case TGSI_SEMANTIC_GENERIC:
   case TGSI_SEMANTIC_TEXCOORD:
   case TGSI_SEMANTIC_CLIPDIST:
   case TGSI_SEMANTIC_CLIPVERTEX:
         case TGSI_SEMANTIC_EDGEFLAG:
   case TGSI_SEMANTIC_FACE:
   case TGSI_SEMANTIC_INSTANCEID:
   case TGSI_SEMANTIC_PCOORD:
   case TGSI_SEMANTIC_PRIMID:
   case TGSI_SEMANTIC_PSIZE:
   case TGSI_SEMANTIC_VERTEXID:
         default:
      assert(0);
         }
      DECL_SPECIAL(DCL)
   {
      struct ureg_program *ureg = tx->ureg;
   bool is_input;
   bool is_sampler;
   struct tgsi_declaration_semantic tgsi;
   struct sm1_semantic sem;
            is_input = sem.reg.file == D3DSPR_INPUT;
   is_sampler =
            DUMP("DCL ");
   sm1_dump_dst_param(&sem.reg);
   if (is_sampler)
         else
   if (tx->version.major >= 3)
         else
   if (sem.usage | sem.usage_idx)
         else
            if (is_sampler) {
      const unsigned m = 1 << sem.reg.idx;
   ureg_DECL_sampler(ureg, sem.reg.idx);
   tx->info->sampler_mask |= m;
   tx->sampler_targets[sem.reg.idx] = (tx->info->sampler_mask_shadow & m) ?
         d3dstt_to_tgsi_tex_shadow(sem.sampler_type) :
               sm1_declusage_to_tgsi(&tgsi, tx->want_texcoord, &sem);
   if (IS_VS) {
      if (is_input) {
         /* linkage outside of shader with vertex declaration */
   ureg_DECL_vs_input(ureg, sem.reg.idx);
   assert(sem.reg.idx < ARRAY_SIZE(tx->info->input_map));
   tx->info->input_map[sem.reg.idx] = sm1_to_nine_declusage(&sem);
   tx->info->num_inputs = MAX2(tx->info->num_inputs, sem.reg.idx + 1);
   } else
   if (tx->version.major >= 3) {
         /* SM2 output semantic determined by file */
   assert(sem.reg.mask != 0);
   if (sem.usage == D3DDECLUSAGE_POSITIONT)
         assert(sem.reg.idx < ARRAY_SIZE(tx->regs.o));
   assert(ureg_dst_is_undef(tx->regs.o[sem.reg.idx]) && "Nine doesn't support yet packing");
   tx->regs.o[sem.reg.idx] = ureg_DECL_output_masked(
         nine_record_outputs(tx, sem.usage, sem.usage_idx, sem.reg.mask, sem.reg.idx);
   if ((tx->info->process_vertices || tx->info->clip_plane_emulation > 0) &&
      sem.usage == D3DDECLUSAGE_POSITION && sem.usage_idx == 0) {
   tx->regs.oPos_out = tx->regs.o[sem.reg.idx]; /* TODO: probably not good declare it twice */
                     if (tgsi.Name == TGSI_SEMANTIC_PSIZE) {
      tx->regs.o[sem.reg.idx] = ureg_DECL_temporary(ureg);
         } else {
      if (is_input && tx->version.major >= 3) {
         unsigned interp_flag;
   unsigned interp_location = 0;
   /* SM3 only, SM2 input semantic determined by file */
   assert(sem.reg.idx < ARRAY_SIZE(tx->regs.v));
   assert(ureg_src_is_undef(tx->regs.v[sem.reg.idx]) && "Nine doesn't support yet packing");
   /* PositionT and tessfactor forbidden */
                  if (tgsi.Name == TGSI_SEMANTIC_POSITION) {
      /* Position0 is forbidden (likely because vPos already does that) */
   if (sem.usage == D3DDECLUSAGE_POSITION)
         /* Following code is for depth */
                     if (sem.reg.mod & NINED3DSPDM_CENTROID ||
      (tgsi.Name == TGSI_SEMANTIC_COLOR && tx->info->force_color_in_centroid))
      interp_flag = nine_tgsi_to_interp_mode(&tgsi);
   /* We replace TGSI_INTERPOLATE_COLOR because some drivers don't support it,
   * and those who support it do the same replacement we do */
                  tx->regs.v[sem.reg.idx] = ureg_DECL_fs_input_centroid(
      ureg, tgsi.Name, tgsi.Index,
      } else
   if (!is_input && 0) { /* declare in COLOROUT/DEPTHOUT case */
         /* FragColor or FragDepth */
   assert(sem.reg.mask != 0);
   ureg_DECL_output_masked(ureg, tgsi.Name, tgsi.Index, sem.reg.mask,
      }
      }
      DECL_SPECIAL(DEF)
   {
      tx_set_lconstf(tx, tx->insn.dst[0].idx, tx->insn.src[0].imm.f);
      }
      DECL_SPECIAL(DEFB)
   {
      tx_set_lconstb(tx, tx->insn.dst[0].idx, tx->insn.src[0].imm.b);
      }
      DECL_SPECIAL(DEFI)
   {
      tx_set_lconsti(tx, tx->insn.dst[0].idx, tx->insn.src[0].imm.i);
      }
      DECL_SPECIAL(POW)
   {
      struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src[2] = {
      tx_src_param(tx, &tx->insn.src[0]),
      };
   /* Anything^0 is 1, including 0^0.
   * Assume mul_zero_wins drivers already have
   * this behaviour. Emulate for the others. */
   if (tx->mul_zero_wins) {
         } else {
      struct ureg_dst tmp = tx_scratch_scalar(tx);
   ureg_POW(tx->ureg, tmp, ureg_abs(src[0]), src[1]);
   ureg_CMP(tx->ureg, dst,
            }
      }
      /* Tests results on Win 10:
   * NV (NVIDIA GeForce GT 635M)
   * AMD (AMD Radeon HD 7730M)
   * INTEL (Intel(R) HD Graphics 4000)
   * PS2 and PS3:
   * RCP and RSQ can generate inf on NV and AMD.
   * RCP and RSQ are clamped on INTEL (+- FLT_MAX),
   * NV: log not clamped
   * AMD: log(0) is -FLT_MAX (but log(inf) is inf)
   * INTEL: log(0) is -FLT_MAX and log(inf) is 127
   * All devices have 0*anything = 0
   *
   * INTEL VS2 and VS3: same behaviour.
   * Some differences VS2 and VS3 for constants defined with inf/NaN.
   * While PS3, VS3 and PS2 keep NaN and Inf shader constants without change,
   * VS2 seems to clamp to zero (may be test failure).
   * AMD VS2: unknown, VS3: very likely behaviour of PS3
   * NV VS2 and VS3: very likely behaviour of PS3
   * For both, Inf in VS becomes NaN is PS
   * "Very likely" because the test was less extensive.
   *
   * Thus all clamping can be removed for shaders 2 and 3,
   * as long as 0*anything = 0.
   * Else clamps to enforce 0*anything = 0 (anything being then
   * neither inf or NaN, the user being unlikely to pass them
   * as constant).
   * The status for VS1 and PS1 is unknown.
   */
      DECL_SPECIAL(RCP)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]);
   struct ureg_dst tmp = tx->mul_zero_wins ? dst : tx_scratch(tx);
   ureg_RCP(ureg, tmp, src);
   if (!tx->mul_zero_wins) {
      /* FLT_MAX has issues with Rayman */
   ureg_MIN(ureg, tmp, ureg_imm1f(ureg, FLT_MAX/2.f), ureg_src(tmp));
      }
      }
      DECL_SPECIAL(RSQ)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]);
   struct ureg_dst tmp = tx->mul_zero_wins ? dst : tx_scratch(tx);
   ureg_RSQ(ureg, tmp, ureg_abs(src));
   if (!tx->mul_zero_wins)
            }
      DECL_SPECIAL(LOG)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst tmp = tx_scratch_scalar(tx);
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]);
   ureg_LG2(ureg, tmp, ureg_abs(src));
   if (tx->mul_zero_wins) {
         } else {
         }
      }
      DECL_SPECIAL(LIT)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst tmp = tx_scratch(tx);
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]);
   ureg_LIT(ureg, tmp, src);
   /* d3d9 LIT is the same than gallium LIT. One difference is that d3d9
   * states that dst.z is 0 when src.y <= 0. Gallium definition can assign
   * it 0^0 if src.w=0, which value is driver dependent. */
   ureg_CMP(ureg, ureg_writemask(dst, TGSI_WRITEMASK_Z),
               ureg_MOV(ureg, ureg_writemask(dst, TGSI_WRITEMASK_XYW), ureg_src(tmp));
      }
      DECL_SPECIAL(NRM)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst tmp = tx_scratch_scalar(tx);
   struct ureg_src nrm = tx_src_scalar(tmp);
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]);
   ureg_DP3(ureg, tmp, src, src);
   ureg_RSQ(ureg, tmp, nrm);
   if (!tx->mul_zero_wins)
         ureg_MUL(ureg, dst, src, nrm);
      }
      DECL_SPECIAL(DP2ADD)
   {
      struct ureg_dst tmp = tx_scratch_scalar(tx);
   struct ureg_src dp2 = tx_src_scalar(tmp);
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src[3];
   int i;
   for (i = 0; i < 3; ++i)
                  ureg_DP2(tx->ureg, tmp, src[0], src[1]);
               }
      DECL_SPECIAL(TEXCOORD)
   {
      struct ureg_program *ureg = tx->ureg;
   const unsigned s = tx->insn.dst[0].idx;
            tx_texcoord_alloc(tx, s);
   ureg_MOV(ureg, ureg_writemask(ureg_saturate(dst), TGSI_WRITEMASK_XYZ), tx->regs.vT[s]);
               }
      DECL_SPECIAL(TEXCOORD_ps14)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]);
                                 }
      DECL_SPECIAL(TEXKILL)
   {
               if (tx->version.major > 1 || tx->version.minor > 3) {
         } else {
      tx_texcoord_alloc(tx, tx->insn.dst[0].idx);
      }
   if (tx->version.major < 2)
                     }
      DECL_SPECIAL(TEXBEM)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]); /* t[n] */
   struct ureg_dst tmp, tmp2, texcoord;
   struct ureg_src sample, m00, m01, m10, m11, c8m, c16m2;
   struct ureg_src bumpenvlscale, bumpenvloffset;
                     sample = ureg_DECL_sampler(ureg, m);
                     tmp = tx_scratch(tx);
   tmp2 = tx_scratch(tx);
   texcoord = tx_scratch(tx);
   /*
   * Bump-env-matrix:
   * 00 is X
   * 01 is Y
   * 10 is Z
   * 11 is W
   */
   c8m = nine_special_constant_src(tx, m);
            m00 = NINE_APPLY_SWIZZLE(c8m, X);
   m01 = NINE_APPLY_SWIZZLE(c8m, Y);
   m10 = NINE_APPLY_SWIZZLE(c8m, Z);
            /* These two attributes are packed as X=scale0 Y=offset0 Z=scale1 W=offset1 etc */
   if (m % 2 == 0) {
      bumpenvlscale = NINE_APPLY_SWIZZLE(c16m2, X);
      } else {
      bumpenvlscale = NINE_APPLY_SWIZZLE(c16m2, Z);
                        /* u' = TextureCoordinates(stage m)u + D3DTSS_BUMPENVMAT00(stage m)*t(n)R  */
   ureg_MAD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), m00,
         /* u' = u' + D3DTSS_BUMPENVMAT10(stage m)*t(n)G */
   ureg_MAD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), m10,
                  /* v' = TextureCoordinates(stage m)v + D3DTSS_BUMPENVMAT01(stage m)*t(n)R */
   ureg_MAD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_Y), m01,
         /* v' = v' + D3DTSS_BUMPENVMAT11(stage m)*t(n)G*/
   ureg_MAD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_Y), m11,
                           if (tx->insn.opcode == D3DSIO_TEXBEM) {
         } else if (tx->insn.opcode == D3DSIO_TEXBEML) {
      /* t(m)RGBA = t(m)RGBA * [(t(n)B * D3DTSS_BUMPENVLSCALE(stage m)) + D3DTSS_BUMPENVLOFFSET(stage m)] */
   ureg_TEX(ureg, tmp, ps1x_sampler_type(tx->info, m), ureg_src(tmp), sample);
   ureg_MAD(ureg, tmp2, NINE_APPLY_SWIZZLE(src, Z),
                                 }
      DECL_SPECIAL(TEXREG2AR)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]); /* t[n] */
   struct ureg_src sample;
   const int m = tx->insn.dst[0].idx;
   ASSERTED const int n = tx->insn.src[0].idx;
            sample = ureg_DECL_sampler(ureg, m);
   tx->info->sampler_mask |= 1 << m;
               }
      DECL_SPECIAL(TEXREG2GB)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]); /* t[n] */
   struct ureg_src sample;
   const int m = tx->insn.dst[0].idx;
   ASSERTED const int n = tx->insn.src[0].idx;
            sample = ureg_DECL_sampler(ureg, m);
   tx->info->sampler_mask |= 1 << m;
               }
      DECL_SPECIAL(TEXM3x2PAD)
   {
         }
      DECL_SPECIAL(TEXM3x2TEX)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]); /* t[n] */
   struct ureg_src sample;
   const int m = tx->insn.dst[0].idx - 1;
   ASSERTED const int n = tx->insn.src[0].idx;
            tx_texcoord_alloc(tx, m);
            /* performs the matrix multiplication */
   ureg_DP3(ureg, ureg_writemask(dst, TGSI_WRITEMASK_X), tx->regs.vT[m], src);
            sample = ureg_DECL_sampler(ureg, m + 1);
   tx->info->sampler_mask |= 1 << (m + 1);
               }
      DECL_SPECIAL(TEXM3x3PAD)
   {
         }
      DECL_SPECIAL(TEXM3x3SPEC)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]); /* t[n] */
   struct ureg_src E = tx_src_param(tx, &tx->insn.src[1]);
   struct ureg_src sample;
   struct ureg_dst tmp;
   const int m = tx->insn.dst[0].idx - 2;
   ASSERTED const int n = tx->insn.src[0].idx;
            tx_texcoord_alloc(tx, m);
   tx_texcoord_alloc(tx, m+1);
            ureg_DP3(ureg, ureg_writemask(dst, TGSI_WRITEMASK_X), tx->regs.vT[m], src);
   ureg_DP3(ureg, ureg_writemask(dst, TGSI_WRITEMASK_Y), tx->regs.vT[m+1], src);
            sample = ureg_DECL_sampler(ureg, m + 2);
   tx->info->sampler_mask |= 1 << (m + 2);
            /* At this step, dst = N = (u', w', z').
   * We want dst to be the texture sampled at (u'', w'', z''), with
   * (u'', w'', z'') = 2 * (N.E / N.N) * N - E */
   ureg_DP3(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_src(dst), ureg_src(dst));
   ureg_RCP(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X));
   /* at this step tmp.x = 1/N.N */
   ureg_DP3(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_src(dst), E);
   /* at this step tmp.y = N.E */
   ureg_MUL(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y));
   /* at this step tmp.x = N.E/N.N */
   ureg_MUL(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X), ureg_imm1f(ureg, 2.0f));
   ureg_MUL(ureg, tmp, ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X), ureg_src(dst));
   /* at this step tmp.xyz = 2 * (N.E / N.N) * N */
   ureg_ADD(ureg, tmp, ureg_src(tmp), ureg_negate(E));
               }
      DECL_SPECIAL(TEXREG2RGB)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]); /* t[n] */
   struct ureg_src sample;
   const int m = tx->insn.dst[0].idx;
   ASSERTED const int n = tx->insn.src[0].idx;
            sample = ureg_DECL_sampler(ureg, m);
   tx->info->sampler_mask |= 1 << m;
               }
      DECL_SPECIAL(TEXDP3TEX)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]); /* t[n] */
   struct ureg_dst tmp;
   struct ureg_src sample;
   const int m = tx->insn.dst[0].idx;
   ASSERTED const int n = tx->insn.src[0].idx;
                     tmp = tx_scratch(tx);
   ureg_DP3(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), tx->regs.vT[m], src);
            sample = ureg_DECL_sampler(ureg, m);
   tx->info->sampler_mask |= 1 << m;
               }
      DECL_SPECIAL(TEXM3x2DEPTH)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]); /* t[n] */
   struct ureg_dst tmp;
   const int m = tx->insn.dst[0].idx - 1;
   ASSERTED const int n = tx->insn.src[0].idx;
            tx_texcoord_alloc(tx, m);
                     /* performs the matrix multiplication */
   ureg_DP3(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), tx->regs.vT[m], src);
            ureg_RCP(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_Z), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y));
   /* tmp.x = 'z', tmp.y = 'w', tmp.z = 1/'w'. */
   ureg_MUL(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Z));
   /* res = 'w' == 0 ? 1.0 : z/w */
   ureg_CMP(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_negate(ureg_abs(ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y))),
         /* replace the depth for depth testing with the result */
   tx->regs.oDepth = ureg_DECL_output_masked(ureg, TGSI_SEMANTIC_POSITION, 0,
         ureg_MOV(ureg, tx->regs.oDepth, ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X));
   /* note that we write nothing to the destination, since it's disallowed to use it afterward */
      }
      DECL_SPECIAL(TEXDP3)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]); /* t[n] */
   const int m = tx->insn.dst[0].idx;
   ASSERTED const int n = tx->insn.src[0].idx;
                                 }
      DECL_SPECIAL(TEXM3x3)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]); /* t[n] */
   struct ureg_src sample;
   struct ureg_dst E, tmp;
   const int m = tx->insn.dst[0].idx - 2;
   ASSERTED const int n = tx->insn.src[0].idx;
            tx_texcoord_alloc(tx, m);
   tx_texcoord_alloc(tx, m+1);
            ureg_DP3(ureg, ureg_writemask(dst, TGSI_WRITEMASK_X), tx->regs.vT[m], src);
   ureg_DP3(ureg, ureg_writemask(dst, TGSI_WRITEMASK_Y), tx->regs.vT[m+1], src);
            switch (tx->insn.opcode) {
   case D3DSIO_TEXM3x3:
      ureg_MOV(ureg, ureg_writemask(dst, TGSI_WRITEMASK_W), ureg_imm1f(ureg, 1.0f));
      case D3DSIO_TEXM3x3TEX:
      sample = ureg_DECL_sampler(ureg, m + 2);
   tx->info->sampler_mask |= 1 << (m + 2);
   ureg_TEX(ureg, dst, ps1x_sampler_type(tx->info, m + 2), ureg_src(dst), sample);
      case D3DSIO_TEXM3x3VSPEC:
      sample = ureg_DECL_sampler(ureg, m + 2);
   tx->info->sampler_mask |= 1 << (m + 2);
   E = tx_scratch(tx);
   tmp = ureg_writemask(tx_scratch(tx), TGSI_WRITEMASK_XYZ);
   ureg_MOV(ureg, ureg_writemask(E, TGSI_WRITEMASK_X), ureg_scalar(tx->regs.vT[m], TGSI_SWIZZLE_W));
   ureg_MOV(ureg, ureg_writemask(E, TGSI_WRITEMASK_Y), ureg_scalar(tx->regs.vT[m+1], TGSI_SWIZZLE_W));
   ureg_MOV(ureg, ureg_writemask(E, TGSI_WRITEMASK_Z), ureg_scalar(tx->regs.vT[m+2], TGSI_SWIZZLE_W));
   /* At this step, dst = N = (u', w', z').
      * We want dst to be the texture sampled at (u'', w'', z''), with
      ureg_DP3(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_src(dst), ureg_src(dst));
   ureg_RCP(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X));
   /* at this step tmp.x = 1/N.N */
   ureg_DP3(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_src(dst), ureg_src(E));
   /* at this step tmp.y = N.E */
   ureg_MUL(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y));
   /* at this step tmp.x = N.E/N.N */
   ureg_MUL(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X), ureg_imm1f(ureg, 2.0f));
   ureg_MUL(ureg, tmp, ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X), ureg_src(dst));
   /* at this step tmp.xyz = 2 * (N.E / N.N) * N */
   ureg_ADD(ureg, tmp, ureg_src(tmp), ureg_negate(ureg_src(E)));
   ureg_TEX(ureg, dst, ps1x_sampler_type(tx->info, m + 2), ureg_src(tmp), sample);
      default:
         }
      }
      DECL_SPECIAL(TEXDEPTH)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst r5;
                     /* we must replace the depth by r5.g == 0 ? 1.0f : r5.r/r5.g.
   * r5 won't be used afterward, thus we can use r5.ba */
   r5 = tx->regs.r[5];
   r5r = ureg_scalar(ureg_src(r5), TGSI_SWIZZLE_X);
            ureg_RCP(ureg, ureg_writemask(r5, TGSI_WRITEMASK_Z), r5g);
   ureg_MUL(ureg, ureg_writemask(r5, TGSI_WRITEMASK_X), r5r, ureg_scalar(ureg_src(r5), TGSI_SWIZZLE_Z));
   /* r5.r = r/g */
   ureg_CMP(ureg, ureg_writemask(r5, TGSI_WRITEMASK_X), ureg_negate(ureg_abs(r5g)),
         /* replace the depth for depth testing with the result */
   tx->regs.oDepth = ureg_DECL_output_masked(ureg, TGSI_SEMANTIC_POSITION, 0,
                     }
      DECL_SPECIAL(BEM)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src0 = tx_src_param(tx, &tx->insn.src[0]);
   struct ureg_src src1 = tx_src_param(tx, &tx->insn.src[1]);
   struct ureg_src m00, m01, m10, m11, c8m;
   const int m = tx->insn.dst[0].idx;
   struct ureg_dst tmp = tx_scratch(tx);
   /*
   * Bump-env-matrix:
   * 00 is X
   * 01 is Y
   * 10 is Z
   * 11 is W
   */
   c8m = nine_special_constant_src(tx, m);
   m00 = NINE_APPLY_SWIZZLE(c8m, X);
   m01 = NINE_APPLY_SWIZZLE(c8m, Y);
   m10 = NINE_APPLY_SWIZZLE(c8m, Z);
   m11 = NINE_APPLY_SWIZZLE(c8m, W);
   /* dest.r = src0.r + D3DTSS_BUMPENVMAT00(stage n) * src1.r  */
   ureg_MAD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), m00,
         /* dest.r = dest.r + D3DTSS_BUMPENVMAT10(stage n) * src1.g; */
   ureg_MAD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_X), m10,
            /* dest.g = src0.g + D3DTSS_BUMPENVMAT01(stage n) * src1.r */
   ureg_MAD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_Y), m01,
         /* dest.g = dest.g + D3DTSS_BUMPENVMAT11(stage n) * src1.g */
   ureg_MAD(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_Y), m11,
                              }
      DECL_SPECIAL(TEXLD)
   {
      struct ureg_program *ureg = tx->ureg;
   unsigned target;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src[2] = {
      tx_src_param(tx, &tx->insn.src[0]),
      };
   assert(tx->insn.src[1].idx >= 0 &&
                  if (TEX_if_fetch4(tx, dst, target, src[0], src[1], tx->insn.src[1].idx))
            switch (tx->insn.flags) {
   case 0:
      ureg_TEX(ureg, dst, target, src[0], src[1]);
      case NINED3DSI_TEXLD_PROJECT:
      ureg_TXP(ureg, dst, target, src[0], src[1]);
      case NINED3DSI_TEXLD_BIAS:
      ureg_TXB(ureg, dst, target, src[0], src[1]);
      default:
      assert(0);
      }
      }
      DECL_SPECIAL(TEXLD_14)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]);
   const unsigned s = tx->insn.dst[0].idx;
            tx->info->sampler_mask |= 1 << s;
               }
      DECL_SPECIAL(TEX)
   {
      struct ureg_program *ureg = tx->ureg;
   const unsigned s = tx->insn.dst[0].idx;
   const unsigned t = ps1x_sampler_type(tx->info, s);
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
                     src[0] = tx->regs.vT[s];
   src[1] = ureg_DECL_sampler(ureg, s);
                        }
      DECL_SPECIAL(TEXLDD)
   {
      unsigned target;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src[4] = {
      tx_src_param(tx, &tx->insn.src[0]),
   tx_src_param(tx, &tx->insn.src[1]),
   tx_src_param(tx, &tx->insn.src[2]),
      };
   assert(tx->insn.src[1].idx >= 0 &&
                  if (TEX_if_fetch4(tx, dst, target, src[0], src[1], tx->insn.src[1].idx))
            ureg_TXD(tx->ureg, dst, target, src[0], src[2], src[3], src[1]);
      }
      DECL_SPECIAL(TEXLDL)
   {
      unsigned target;
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src[2] = {
      tx_src_param(tx, &tx->insn.src[0]),
      };
   assert(tx->insn.src[1].idx >= 0 &&
                  if (TEX_if_fetch4(tx, dst, target, src[0], src[1], tx->insn.src[1].idx))
            ureg_TXL(tx->ureg, dst, target, src[0], src[1]);
      }
      DECL_SPECIAL(SETP)
   {
      const unsigned cmp_op = sm1_insn_flags_to_tgsi_setop(tx->insn.flags);
   struct ureg_dst dst = tx_dst_param(tx, &tx->insn.dst[0]);
   struct ureg_src src[2] = {
      tx_src_param(tx, &tx->insn.src[0]),
      };
   ureg_insn(tx->ureg, cmp_op, &dst, 1, src, 2, 0);
      }
      DECL_SPECIAL(BREAKP)
   {
      struct ureg_src src = tx_src_param(tx, &tx->insn.src[0]);
   ureg_IF(tx->ureg, src, tx_cond(tx));
   ureg_BRK(tx->ureg);
   tx_endcond(tx);
   ureg_ENDIF(tx->ureg);
      }
      DECL_SPECIAL(PHASE)
   {
         }
      DECL_SPECIAL(COMMENT)
   {
         }
         #define _OPI(o,t,vv1,vv2,pv1,pv2,d,s,h) \
            static const struct sm1_op_info inst_table[] =
   {
      _OPI(NOP, NOP, V(0,0), V(3,0), V(0,0), V(3,0), 0, 0, SPECIAL(NOP)), /* 0 */
   _OPI(MOV, MOV, V(0,0), V(3,0), V(0,0), V(3,0), 1, 1, NULL),
   _OPI(ADD, ADD, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, NULL), /* 2 */
   _OPI(SUB, NOP, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, SPECIAL(SUB)), /* 3 */
   _OPI(MAD, MAD, V(0,0), V(3,0), V(0,0), V(3,0), 1, 3, NULL), /* 4 */
   _OPI(MUL, MUL, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, NULL), /* 5 */
   _OPI(RCP, RCP, V(0,0), V(3,0), V(0,0), V(3,0), 1, 1, SPECIAL(RCP)), /* 6 */
   _OPI(RSQ, RSQ, V(0,0), V(3,0), V(0,0), V(3,0), 1, 1, SPECIAL(RSQ)), /* 7 */
   _OPI(DP3, DP3, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, NULL), /* 8 */
   _OPI(DP4, DP4, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, NULL), /* 9 */
   _OPI(MIN, MIN, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, NULL), /* 10 */
   _OPI(MAX, MAX, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, NULL), /* 11 */
   _OPI(SLT, SLT, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, NULL), /* 12 */
   _OPI(SGE, SGE, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, NULL), /* 13 */
   _OPI(EXP, EX2, V(0,0), V(3,0), V(0,0), V(3,0), 1, 1, NULL), /* 14 */
   _OPI(LOG, LG2, V(0,0), V(3,0), V(0,0), V(3,0), 1, 1, SPECIAL(LOG)), /* 15 */
   _OPI(LIT, LIT, V(0,0), V(3,0), V(0,0), V(0,0), 1, 1, SPECIAL(LIT)), /* 16 */
   _OPI(DST, DST, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, NULL), /* 17 */
   _OPI(LRP, LRP, V(0,0), V(3,0), V(0,0), V(3,0), 1, 3, NULL), /* 18 */
            _OPI(M4x4, NOP, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, SPECIAL(M4x4)),
   _OPI(M4x3, NOP, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, SPECIAL(M4x3)),
   _OPI(M3x4, NOP, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, SPECIAL(M3x4)),
   _OPI(M3x3, NOP, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, SPECIAL(M3x3)),
            _OPI(CALL,    CAL,     V(2,0), V(3,0), V(2,1), V(3,0), 0, 1, SPECIAL(CALL)),
   _OPI(CALLNZ,  CAL,     V(2,0), V(3,0), V(2,1), V(3,0), 0, 2, SPECIAL(CALLNZ)),
   _OPI(LOOP,    BGNLOOP, V(2,0), V(3,0), V(3,0), V(3,0), 0, 2, SPECIAL(LOOP)),
   _OPI(RET,     RET,     V(2,0), V(3,0), V(2,1), V(3,0), 0, 0, SPECIAL(RET)),
   _OPI(ENDLOOP, ENDLOOP, V(2,0), V(3,0), V(3,0), V(3,0), 0, 0, SPECIAL(ENDLOOP)),
                     _OPI(POW, POW, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, SPECIAL(POW)),
   _OPI(CRS, NOP, V(0,0), V(3,0), V(0,0), V(3,0), 1, 2, SPECIAL(XPD)), /* XXX: .w */
   _OPI(SGN, SSG, V(2,0), V(3,0), V(0,0), V(0,0), 1, 3, SPECIAL(SGN)), /* ignore src1,2 */
   _OPI(ABS, NOP, V(0,0), V(3,0), V(0,0), V(3,0), 1, 1, SPECIAL(ABS)),
            _OPI(SINCOS, NOP, V(2,0), V(2,1), V(2,0), V(2,1), 1, 3, SPECIAL(SINCOS)),
            /* More flow control */
   _OPI(REP,    NOP,    V(2,0), V(3,0), V(2,1), V(3,0), 0, 1, SPECIAL(REP)),
   _OPI(ENDREP, NOP,    V(2,0), V(3,0), V(2,1), V(3,0), 0, 0, SPECIAL(ENDREP)),
   _OPI(IF,     IF,     V(2,0), V(3,0), V(2,1), V(3,0), 0, 1, SPECIAL(IF)),
   _OPI(IFC,    IF,     V(2,1), V(3,0), V(2,1), V(3,0), 0, 2, SPECIAL(IFC)),
   _OPI(ELSE,   ELSE,   V(2,0), V(3,0), V(2,1), V(3,0), 0, 0, SPECIAL(ELSE)),
   _OPI(ENDIF,  ENDIF,  V(2,0), V(3,0), V(2,1), V(3,0), 0, 0, SPECIAL(ENDIF)),
   _OPI(BREAK,  BRK,    V(2,1), V(3,0), V(2,1), V(3,0), 0, 0, NULL),
   _OPI(BREAKC, NOP,    V(2,1), V(3,0), V(2,1), V(3,0), 0, 2, SPECIAL(BREAKC)),
   /* we don't write to the address register, but a normal register (copied
   * when needed to the address register), thus we don't use ARR */
            _OPI(DEFB, NOP, V(0,0), V(3,0) , V(0,0), V(3,0) , 1, 0, SPECIAL(DEFB)),
            _OPI(TEXCOORD,     NOP, V(0,0), V(0,0), V(0,0), V(1,3), 1, 0, SPECIAL(TEXCOORD)),
   _OPI(TEXCOORD,     MOV, V(0,0), V(0,0), V(1,4), V(1,4), 1, 1, SPECIAL(TEXCOORD_ps14)),
   _OPI(TEXKILL,      KILL_IF, V(0,0), V(0,0), V(0,0), V(3,0), 1, 0, SPECIAL(TEXKILL)),
   _OPI(TEX,          TEX, V(0,0), V(0,0), V(0,0), V(1,3), 1, 0, SPECIAL(TEX)),
   _OPI(TEX,          TEX, V(0,0), V(0,0), V(1,4), V(1,4), 1, 1, SPECIAL(TEXLD_14)),
   _OPI(TEX,          TEX, V(0,0), V(0,0), V(2,0), V(3,0), 1, 2, SPECIAL(TEXLD)),
   _OPI(TEXBEM,       TEX, V(0,0), V(0,0), V(0,0), V(1,3), 1, 1, SPECIAL(TEXBEM)),
   _OPI(TEXBEML,      TEX, V(0,0), V(0,0), V(0,0), V(1,3), 1, 1, SPECIAL(TEXBEM)),
   _OPI(TEXREG2AR,    TEX, V(0,0), V(0,0), V(0,0), V(1,3), 1, 1, SPECIAL(TEXREG2AR)),
   _OPI(TEXREG2GB,    TEX, V(0,0), V(0,0), V(0,0), V(1,3), 1, 1, SPECIAL(TEXREG2GB)),
   _OPI(TEXM3x2PAD,   TEX, V(0,0), V(0,0), V(0,0), V(1,3), 1, 1, SPECIAL(TEXM3x2PAD)),
   _OPI(TEXM3x2TEX,   TEX, V(0,0), V(0,0), V(0,0), V(1,3), 1, 1, SPECIAL(TEXM3x2TEX)),
   _OPI(TEXM3x3PAD,   TEX, V(0,0), V(0,0), V(0,0), V(1,3), 1, 1, SPECIAL(TEXM3x3PAD)),
   _OPI(TEXM3x3TEX,   TEX, V(0,0), V(0,0), V(0,0), V(1,3), 1, 1, SPECIAL(TEXM3x3)),
   _OPI(TEXM3x3SPEC,  TEX, V(0,0), V(0,0), V(0,0), V(1,3), 1, 2, SPECIAL(TEXM3x3SPEC)),
            _OPI(EXPP, EXP, V(0,0), V(1,1), V(0,0), V(0,0), 1, 1, NULL),
   _OPI(EXPP, EX2, V(2,0), V(3,0), V(0,0), V(0,0), 1, 1, NULL),
   _OPI(LOGP, LG2, V(0,0), V(3,0), V(0,0), V(0,0), 1, 1, SPECIAL(LOG)),
                     /* More tex stuff */
   _OPI(TEXREG2RGB,   TEX, V(0,0), V(0,0), V(1,2), V(1,3), 1, 1, SPECIAL(TEXREG2RGB)),
   _OPI(TEXDP3TEX,    TEX, V(0,0), V(0,0), V(1,2), V(1,3), 1, 1, SPECIAL(TEXDP3TEX)),
   _OPI(TEXM3x2DEPTH, TEX, V(0,0), V(0,0), V(1,3), V(1,3), 1, 1, SPECIAL(TEXM3x2DEPTH)),
   _OPI(TEXDP3,       TEX, V(0,0), V(0,0), V(1,2), V(1,3), 1, 1, SPECIAL(TEXDP3)),
   _OPI(TEXM3x3,      TEX, V(0,0), V(0,0), V(1,2), V(1,3), 1, 1, SPECIAL(TEXM3x3)),
            /* Misc */
   _OPI(CMP,    CMP,  V(0,0), V(0,0), V(1,2), V(3,0), 1, 3, SPECIAL(CMP)), /* reversed */
   _OPI(BEM,    NOP,  V(0,0), V(0,0), V(1,4), V(1,4), 1, 2, SPECIAL(BEM)),
   _OPI(DP2ADD, NOP,  V(0,0), V(0,0), V(2,0), V(3,0), 1, 3, SPECIAL(DP2ADD)),
   _OPI(DSX,    DDX,  V(0,0), V(0,0), V(2,1), V(3,0), 1, 1, NULL),
   _OPI(DSY,    DDY,  V(0,0), V(0,0), V(2,1), V(3,0), 1, 1, NULL),
   _OPI(TEXLDD, TXD,  V(0,0), V(0,0), V(2,1), V(3,0), 1, 4, SPECIAL(TEXLDD)),
   _OPI(SETP,   NOP,  V(0,0), V(3,0), V(2,1), V(3,0), 1, 2, SPECIAL(SETP)),
   _OPI(TEXLDL, TXL,  V(3,0), V(3,0), V(3,0), V(3,0), 1, 2, SPECIAL(TEXLDL)),
      };
      static const struct sm1_op_info inst_phase =
            static const struct sm1_op_info inst_comment =
            static void
   create_op_info_map(struct shader_translator *tx)
   {
      const unsigned version = (tx->version.major << 8) | tx->version.minor;
            for (i = 0; i < ARRAY_SIZE(tx->op_info_map); ++i)
            if (tx->processor == PIPE_SHADER_VERTEX) {
      for (i = 0; i < ARRAY_SIZE(inst_table); ++i) {
         assert(inst_table[i].sio < ARRAY_SIZE(tx->op_info_map));
   if (inst_table[i].vert_version.min <= version &&
            } else {
      for (i = 0; i < ARRAY_SIZE(inst_table); ++i) {
         assert(inst_table[i].sio < ARRAY_SIZE(tx->op_info_map));
   if (inst_table[i].frag_version.min <= version &&
               }
      static inline HRESULT
   NineTranslateInstruction_Generic(struct shader_translator *tx)
   {
      struct ureg_dst dst[1];
   struct ureg_src src[4];
            for (i = 0; i < tx->insn.ndst && i < ARRAY_SIZE(dst); ++i)
         for (i = 0; i < tx->insn.nsrc && i < ARRAY_SIZE(src); ++i)
            ureg_insn(tx->ureg, tx->insn.info->opcode,
                  }
      static inline DWORD
   TOKEN_PEEK(struct shader_translator *tx)
   {
         }
      static inline DWORD
   TOKEN_NEXT(struct shader_translator *tx)
   {
         }
      static inline void
   TOKEN_JUMP(struct shader_translator *tx)
   {
      if (tx->parse_next && tx->parse != tx->parse_next) {
      WARN("parse(%p) != parse_next(%p) !\n", tx->parse, tx->parse_next);
         }
      static inline bool
   sm1_parse_eof(struct shader_translator *tx)
   {
         }
      static void
   sm1_read_version(struct shader_translator *tx)
   {
               tx->version.major = D3DSHADER_VERSION_MAJOR(tok);
            switch (tok >> 16) {
   case NINED3D_SM1_VS: tx->processor = PIPE_SHADER_VERTEX; break;
   case NINED3D_SM1_PS: tx->processor = PIPE_SHADER_FRAGMENT; break;
   default:
      DBG("Invalid shader type: %x\n", tok);
   tx->processor = ~0;
         }
      /* This is just to check if we parsed the instruction properly. */
   static void
   sm1_parse_get_skip(struct shader_translator *tx)
   {
               if (tx->version.major >= 2) {
      tx->parse_next = tx->parse + 1 /* this */ +
      } else {
            }
      static void
   sm1_print_comment(const char *comment, UINT size)
   {
      if (!size)
            }
      static void
   sm1_parse_comments(struct shader_translator *tx, BOOL print)
   {
               while ((tok & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT)
   {
      const char *comment = "";
   UINT size = (tok & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
            if (print)
                  }
      static void
   sm1_parse_get_param(struct shader_translator *tx, DWORD *reg, DWORD *rel)
   {
               if (*reg & D3DSHADER_ADDRMODE_RELATIVE)
   {
      if (tx->version.major < 2)
         *rel = (1 << 31) |
      ((D3DSPR_ADDR << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2) |
      else
         }
      static void
   sm1_parse_dst_param(struct sm1_dst_param *dst, DWORD tok)
   {
      int8_t shift;
   dst->file =
      (tok & D3DSP_REGTYPE_MASK)  >> D3DSP_REGTYPE_SHIFT |
      dst->type = TGSI_RETURN_TYPE_FLOAT;
   dst->idx = tok & D3DSP_REGNUM_MASK;
   dst->rel = NULL;
   dst->mask = (tok & NINED3DSP_WRITEMASK_MASK) >> NINED3DSP_WRITEMASK_SHIFT;
   dst->mod = (tok & D3DSP_DSTMOD_MASK) >> D3DSP_DSTMOD_SHIFT;
   shift = (tok & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;
      }
      static void
   sm1_parse_src_param(struct sm1_src_param *src, DWORD tok)
   {
      src->file =
      ((tok & D3DSP_REGTYPE_MASK)  >> D3DSP_REGTYPE_SHIFT) |
      src->type = TGSI_RETURN_TYPE_FLOAT;
   src->idx = tok & D3DSP_REGNUM_MASK;
   src->rel = NULL;
   src->swizzle = (tok & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT;
            switch (src->file) {
   case D3DSPR_CONST2: src->file = D3DSPR_CONST; src->idx += 2048; break;
   case D3DSPR_CONST3: src->file = D3DSPR_CONST; src->idx += 4096; break;
   case D3DSPR_CONST4: src->file = D3DSPR_CONST; src->idx += 6144; break;
   default:
            }
      static void
   sm1_parse_immediate(struct shader_translator *tx,
         {
      imm->file = NINED3DSPR_IMMEDIATE;
   imm->idx = INT_MIN;
   imm->rel = NULL;
   imm->swizzle = NINED3DSP_NOSWIZZLE;
   imm->mod = 0;
   switch (tx->insn.opcode) {
   case D3DSIO_DEF:
      imm->type = NINED3DSPTYPE_FLOAT4;
   memcpy(&imm->imm.d[0], tx->parse, 4 * sizeof(DWORD));
   tx->parse += 4;
      case D3DSIO_DEFI:
      imm->type = NINED3DSPTYPE_INT4;
   memcpy(&imm->imm.d[0], tx->parse, 4 * sizeof(DWORD));
   tx->parse += 4;
      case D3DSIO_DEFB:
      imm->type = NINED3DSPTYPE_BOOL;
   memcpy(&imm->imm.d[0], tx->parse, 1 * sizeof(DWORD));
   tx->parse += 1;
      default:
      assert(0);
         }
      static void
   sm1_read_dst_param(struct shader_translator *tx,
               {
               sm1_parse_get_param(tx, &tok_dst, &tok_rel);
   sm1_parse_dst_param(dst, tok_dst);
   if (tok_dst & D3DSHADER_ADDRMODE_RELATIVE) {
      sm1_parse_src_param(rel, tok_rel);
         }
      static void
   sm1_read_src_param(struct shader_translator *tx,
               {
               sm1_parse_get_param(tx, &tok_src, &tok_rel);
   sm1_parse_src_param(src, tok_src);
   if (tok_src & D3DSHADER_ADDRMODE_RELATIVE) {
      assert(rel);
   sm1_parse_src_param(rel, tok_rel);
         }
      static void
   sm1_read_semantic(struct shader_translator *tx,
         {
      const DWORD tok_usg = TOKEN_NEXT(tx);
            sem->sampler_type = (tok_usg & D3DSP_TEXTURETYPE_MASK) >> D3DSP_TEXTURETYPE_SHIFT;
   sem->usage = (tok_usg & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT;
               }
      static void
   sm1_parse_instruction(struct shader_translator *tx)
   {
      struct sm1_instruction *insn = &tx->insn;
   HRESULT hr;
   DWORD tok;
   const struct sm1_op_info *info = NULL;
            sm1_parse_comments(tx, true);
                     insn->opcode = tok & D3DSI_OPCODE_MASK;
   insn->flags = (tok & NINED3DSIO_OPCODE_FLAGS_MASK) >> NINED3DSIO_OPCODE_FLAGS_SHIFT;
   insn->coissue = !!(tok & D3DSI_COISSUE);
            if (insn->opcode < ARRAY_SIZE(tx->op_info_map)) {
      int k = tx->op_info_map[insn->opcode];
   if (k >= 0) {
         assert(k < ARRAY_SIZE(inst_table));
      } else {
      if (insn->opcode == D3DSIO_PHASE)   info = &inst_phase;
      }
   if (!info) {
      DBG("illegal or unhandled opcode: %08x\n", insn->opcode);
   TOKEN_JUMP(tx);
      }
   insn->info = info;
   insn->ndst = info->ndst;
            /* check version */
   {
      unsigned min = IS_VS ? info->vert_version.min : info->frag_version.min;
   unsigned max = IS_VS ? info->vert_version.max : info->frag_version.max;
   unsigned ver = (tx->version.major << 8) | tx->version.minor;
   if (ver < min || ver > max) {
         DBG("opcode not supported in this shader version: %x <= %x <= %x\n",
                     for (i = 0; i < insn->ndst; ++i)
         if (insn->predicated)
         for (i = 0; i < insn->nsrc; ++i)
            /* parse here so we can dump them before processing */
   if (insn->opcode == D3DSIO_DEF ||
      insn->opcode == D3DSIO_DEFI ||
   insn->opcode == D3DSIO_DEFB)
         sm1_dump_instruction(insn, tx->cond_depth + tx->loop_depth);
            if (insn->predicated) {
      tx->predicated_activated = true;
   if (ureg_dst_is_undef(tx->regs.predicate_tmp)) {
         tx->regs.predicate_tmp = ureg_DECL_temporary(tx->ureg);
               if (info->handler)
         else
                  if (insn->predicated) {
      tx->predicated_activated = false;
   /* TODO: predicate might be allowed on outputs,
         ureg_CMP(tx->ureg, tx->regs.predicate_dst,
            ureg_negate(tx_src_param(tx, &insn->pred)),
            if (hr != D3D_OK)
                     }
      #define GET_CAP(n) screen->get_param( \
         #define GET_SHADER_CAP(n) screen->get_shader_param( \
            static HRESULT
   tx_ctor(struct shader_translator *tx, struct pipe_screen *screen, struct nine_shader_info *info)
   {
                                 tx->byte_code = info->byte_code;
            for (i = 0; i < ARRAY_SIZE(info->input_map); ++i)
                  info->position_t = false;
            memset(tx->slots_used, 0, sizeof(tx->slots_used));
   memset(info->int_slots_used, 0, sizeof(info->int_slots_used));
            tx->info->const_float_slots = 0;
   tx->info->const_int_slots = 0;
            info->sampler_mask = 0x0;
            info->lconstf.data = NULL;
                     for (i = 0; i < ARRAY_SIZE(tx->regs.rL); ++i) {
         }
   tx->regs.address = ureg_dst_undef();
   tx->regs.a0 = ureg_dst_undef();
   tx->regs.p = ureg_dst_undef();
   tx->regs.oDepth = ureg_dst_undef();
   tx->regs.vPos = ureg_src_undef();
   tx->regs.vFace = ureg_src_undef();
   for (i = 0; i < ARRAY_SIZE(tx->regs.o); ++i)
         for (i = 0; i < ARRAY_SIZE(tx->regs.oCol); ++i)
         for (i = 0; i < ARRAY_SIZE(tx->regs.vC); ++i)
         for (i = 0; i < ARRAY_SIZE(tx->regs.vT); ++i)
                                                tx->ureg = ureg_create(info->type);
   if (!tx->ureg) {
                  tx->native_integers = GET_SHADER_CAP(INTEGERS);
   tx->inline_subroutines = !GET_SHADER_CAP(SUBROUTINES);
   tx->want_texcoord = GET_CAP(TGSI_TEXCOORD);
   tx->shift_wpos = !GET_CAP(FS_COORD_PIXEL_CENTER_INTEGER);
   tx->texcoord_sn = tx->want_texcoord ?
         tx->wpos_is_sysval = GET_CAP(FS_POSITION_IS_SYSVAL);
   tx->face_is_sysval_integer = GET_CAP(FS_FACE_IS_INTEGER_SYSVAL);
   tx->no_vs_window_space = !GET_CAP(VS_WINDOW_SPACE_POSITION);
            if (info->emulate_features) {
      tx->shift_wpos = true;
   tx->no_vs_window_space = true;
               if (IS_VS) {
         } else if (tx->version.major < 2) {/* IS_PS v1 */
         } else if (tx->version.major == 2) {/* IS_PS v2 */
         } else {/* IS_PS v3 */
                  if (tx->version.major < 2) {
      tx->num_consti_allowed = 0;
      } else {
      tx->num_consti_allowed = NINE_MAX_CONST_I;
               if (info->swvp_on) {
      /* TODO: The values tx->version.major == 1 */
   tx->num_constf_allowed = 8192;
   tx->num_consti_allowed = 2048;
               /* VS must always write position. Declare it here to make it the 1st output.
   * (Some drivers like nv50 are buggy and rely on that.)
   */
   if (IS_VS) {
         } else {
      ureg_property(tx->ureg, TGSI_PROPERTY_FS_COORD_ORIGIN, TGSI_FS_COORD_ORIGIN_UPPER_LEFT);
   if (!tx->shift_wpos)
               if (tx->mul_zero_wins)
            /* Add additional definition of constants */
   if (info->add_constants_defs.c_combination) {
               assert(info->add_constants_defs.int_const_added);
   assert(info->add_constants_defs.bool_const_added);
   /* We only add constants that are used by the shader
         for (i = 0; i < NINE_MAX_CONST_I; ++i) {
         if ((*info->add_constants_defs.int_const_added)[i]) {
      DBG("Defining const i%i : { %i %i %i %i }\n", i,
      info->add_constants_defs.c_combination->const_i[i][0],
   info->add_constants_defs.c_combination->const_i[i][1],
   info->add_constants_defs.c_combination->const_i[i][2],
         }
   for (i = 0; i < NINE_MAX_CONST_B; ++i) {
         if ((*info->add_constants_defs.bool_const_added)[i]) {
      DBG("Defining const b%i : %i\n", i, (int)(info->add_constants_defs.c_combination->const_b[i] != 0));
         }
      }
      static void
   tx_dtor(struct shader_translator *tx)
   {
      if (tx->slot_map)
         if (tx->num_inst_labels)
         FREE(tx->lconstf);
   FREE(tx->regs.r);
      }
      /* CONST[0].xyz = width/2, -height/2, zmax-zmin
   * CONST[1].xyz = x+width/2, y+height/2, zmin */
   static void
   shader_add_vs_viewport_transform(struct shader_translator *tx)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_src c0 = ureg_src_register(TGSI_FILE_CONSTANT, 0);
   struct ureg_src c1 = ureg_src_register(TGSI_FILE_CONSTANT, 1);
            c0 = ureg_src_dimension(c0, 4);
   c1 = ureg_src_dimension(c1, 4);
   /* TODO: find out when we need to apply the viewport transformation or not.
   * Likely will be XYZ vs XYZRHW in vdecl_out
   * ureg_MUL(ureg, ureg_writemask(pos_tmp, TGSI_WRITEMASK_XYZ), ureg_src(tx->regs.oPos), c0);
   * ureg_ADD(ureg, ureg_writemask(tx->regs.oPos_out, TGSI_WRITEMASK_XYZ), ureg_src(pos_tmp), c1);
   */
      }
      static void
   shader_add_ps_fog_stage(struct shader_translator *tx, struct ureg_dst dst_col, struct ureg_src src_col)
   {
      struct ureg_program *ureg = tx->ureg;
   struct ureg_src fog_end, fog_coeff, fog_density, fog_params;
   struct ureg_src fog_vs, fog_color;
            if (!tx->info->fog_enable) {
      ureg_MOV(ureg, dst_col, src_col);
               if (tx->info->fog_mode != D3DFOG_NONE) {
      depth = tx_scratch_scalar(tx);
   if (tx->info->zfog)
         else /* wfog: use w. position's w contains 1/w */
               fog_color = nine_special_constant_src(tx, 12);
   fog_params = nine_special_constant_src(tx, 13);
            if (tx->info->fog_mode == D3DFOG_LINEAR) {
      fog_end = NINE_APPLY_SWIZZLE(fog_params, X);
   fog_coeff = NINE_APPLY_SWIZZLE(fog_params, Y);
   ureg_ADD(ureg, fog_factor, fog_end, ureg_negate(ureg_src(depth)));
      } else if (tx->info->fog_mode == D3DFOG_EXP) {
      fog_density = NINE_APPLY_SWIZZLE(fog_params, X);
   ureg_MUL(ureg, fog_factor, ureg_src(depth), fog_density);
   ureg_MUL(ureg, fog_factor, tx_src_scalar(fog_factor), ureg_imm1f(ureg, -1.442695f));
      } else if (tx->info->fog_mode == D3DFOG_EXP2) {
      fog_density = NINE_APPLY_SWIZZLE(fog_params, X);
   ureg_MUL(ureg, fog_factor, ureg_src(depth), fog_density);
   ureg_MUL(ureg, fog_factor, tx_src_scalar(fog_factor), tx_src_scalar(fog_factor));
   ureg_MUL(ureg, fog_factor, tx_src_scalar(fog_factor), ureg_imm1f(ureg, -1.442695f));
      } else {
      fog_vs = ureg_scalar(ureg_DECL_fs_input(ureg, TGSI_SEMANTIC_GENERIC, 16,
                           ureg_LRP(ureg, ureg_writemask(dst_col, TGSI_WRITEMASK_XYZ),
            }
      static void
   shader_add_ps_alpha_test_stage(struct shader_translator *tx, struct ureg_src src_color)
   {
      struct ureg_program *ureg = tx->ureg;
   unsigned cmp_op;
   struct ureg_src src[2];
   struct ureg_dst tmp = tx_scratch(tx);
   if (tx->info->alpha_test_emulation == PIPE_FUNC_ALWAYS)
         if (tx->info->alpha_test_emulation == PIPE_FUNC_NEVER) {
      ureg_KILL(ureg);
      }
   cmp_op = pipe_comp_to_tgsi_opposite(tx->info->alpha_test_emulation);
   src[0] = ureg_scalar(src_color, TGSI_SWIZZLE_W); /* Read color alpha channel */
   src[1] = ureg_scalar(nine_special_constant_src(tx, 14), TGSI_SWIZZLE_X); /* Read alphatest */
   ureg_insn(tx->ureg, cmp_op, &tmp, 1, src, 2, 0);
      }
      static void parse_shader(struct shader_translator *tx)
   {
               while (!sm1_parse_eof(tx) && !tx->failure)
                  if (tx->failure)
            if (IS_PS) {
      struct ureg_dst oCol0 = ureg_DECL_output(tx->ureg, TGSI_SEMANTIC_COLOR, 0);
   struct ureg_dst tmp_oCol0;
   if (tx->version.major < 3) {
         tmp_oCol0 = ureg_DECL_temporary(tx->ureg);
   if (tx->version.major < 2) {
      assert(tx->num_temp); /* there must be color output */
   info->rt_mask |= 0x1;
      } else {
         } else {
         assert(!ureg_dst_is_undef(tx->regs.oCol[0]));
   }
   shader_add_ps_alpha_test_stage(tx, ureg_src(tmp_oCol0));
               if (IS_VS && tx->version.major < 3 && ureg_dst_is_undef(tx->regs.oFog) && info->fog_enable) {
      tx->regs.oFog = ureg_DECL_output(tx->ureg, TGSI_SEMANTIC_GENERIC, 16);
               if (info->position_t) {
      if (tx->no_vs_window_space) {
         } else {
                     if (IS_VS && !ureg_dst_is_undef(tx->regs.oPts)) {
      struct ureg_dst oPts = ureg_DECL_output(tx->ureg, TGSI_SEMANTIC_PSIZE, 0);
   ureg_MAX(tx->ureg, ureg_writemask(tx->regs.oPts, TGSI_WRITEMASK_X), ureg_src(tx->regs.oPts), ureg_imm1f(tx->ureg, info->point_size_min));
   ureg_MIN(tx->ureg, ureg_writemask(oPts, TGSI_WRITEMASK_X), ureg_src(tx->regs.oPts), ureg_imm1f(tx->ureg, info->point_size_max));
      } else if (IS_VS && tx->always_output_pointsize) {
      struct ureg_dst oPts = ureg_DECL_output(tx->ureg, TGSI_SEMANTIC_PSIZE, 0);
   ureg_MOV(tx->ureg, ureg_writemask(oPts, TGSI_WRITEMASK_X), nine_special_constant_src(tx, 8));
               if (IS_VS && tx->info->clip_plane_emulation > 0) {
      struct ureg_dst clipdist[2] = {ureg_dst_undef(), ureg_dst_undef()};
   int num_clipdist = ffs(tx->info->clip_plane_emulation);
   int i;
   /* TODO: handle undefined channels of oPos (w is not always written to I think. default is 1) *
      * Note in d3d9 it's not possible to output clipvert, so we don't need to check
      clipdist[0] = ureg_DECL_output_masked(tx->ureg, TGSI_SEMANTIC_CLIPDIST, 0, ((1 << num_clipdist) - 1) & 0xf, 0, 1);
   if (num_clipdist >= 5)
         ureg_property(tx->ureg, TGSI_PROPERTY_NUM_CLIPDIST_ENABLED, num_clipdist);
   for (i = 0; i < num_clipdist; i++) {
         assert(!ureg_dst_is_undef(clipdist[i>>2]));
   if (!(tx->info->clip_plane_emulation & (1 << i)))
         else
                              if (info->process_vertices)
               }
      #define NINE_SHADER_DEBUG_OPTION_NO_NIR_VS        (1 << 2)
   #define NINE_SHADER_DEBUG_OPTION_NO_NIR_PS        (1 << 3)
   #define NINE_SHADER_DEBUG_OPTION_DUMP_NIR         (1 << 4)
   #define NINE_SHADER_DEBUG_OPTION_DUMP_TGSI        (1 << 5)
      static const struct debug_named_value nine_shader_debug_options[] = {
      { "no_nir_vs", NINE_SHADER_DEBUG_OPTION_NO_NIR_VS, "Never use NIR for vertex shaders even if the driver prefers it." },
   { "no_nir_ps", NINE_SHADER_DEBUG_OPTION_NO_NIR_PS, "Never use NIR for pixel shaders even if the driver prefers it." },
   { "dump_nir", NINE_SHADER_DEBUG_OPTION_DUMP_NIR, "Print translated NIR shaders." },
   { "dump_tgsi", NINE_SHADER_DEBUG_OPTION_DUMP_TGSI, "Print TGSI shaders." },
      };
      static inline bool
   nine_shader_get_debug_flag(uint64_t flag)
   {
      static uint64_t flags = 0;
            if (unlikely(first_run)) {
      first_run = false;
            // Check old TGSI dump envvar too
   if (debug_get_bool_option("NINE_TGSI_DUMP", false)) {
                        }
      static void
   nine_pipe_nir_shader_state_from_tgsi(struct pipe_shader_state *state, const struct tgsi_token *tgsi_tokens,
         {
               if (unlikely(nine_shader_get_debug_flag(NINE_SHADER_DEBUG_OPTION_DUMP_NIR))) {
                  state->type = PIPE_SHADER_IR_NIR;
   state->tokens = NULL;
   state->ir.nir = nir;
      }
      static void *
   nine_ureg_create_shader(struct ureg_program                  *ureg,
               {
      struct pipe_shader_state state;
   const struct tgsi_token *tgsi_tokens;
            tgsi_tokens = ureg_finalize(ureg);
   if (!tgsi_tokens)
            assert(((struct tgsi_header *) &tgsi_tokens[0])->HeaderSize >= 2);
                     /* Allow user to override preferred IR, this is very useful for debugging */
   if (unlikely(shader_type == PIPE_SHADER_VERTEX && nine_shader_get_debug_flag(NINE_SHADER_DEBUG_OPTION_NO_NIR_VS)))
         if (unlikely(shader_type == PIPE_SHADER_FRAGMENT && nine_shader_get_debug_flag(NINE_SHADER_DEBUG_OPTION_NO_NIR_PS)))
            DUMP("shader type: %s, selected IR: %s\n",
                  if (use_nir) {
         } else {
                           if (so)
            switch (shader_type) {
   case PIPE_SHADER_VERTEX:
         case PIPE_SHADER_FRAGMENT:
         default:
            }
         void *
   nine_create_shader_with_so_and_destroy(struct ureg_program                   *p,
               {
      void *result = nine_ureg_create_shader(p, pipe, so);
   ureg_destroy(p);
      }
      HRESULT
   nine_translate_shader(struct NineDevice9 *device, struct nine_shader_info *info, struct pipe_context *pipe)
   {
      struct shader_translator *tx;
   HRESULT hr = D3D_OK;
   const unsigned processor = info->type;
   struct pipe_screen *screen = info->process_vertices ? device->screen_sw : device->screen;
                     tx = MALLOC_STRUCT(shader_translator);
   if (!tx)
                     if (tx_ctor(tx, screen, info) == E_OUTOFMEMORY) {
      hr = E_OUTOFMEMORY;
      }
                     if (((tx->version.major << 16) | tx->version.minor) > 0x00030000) {
      hr = D3DERR_INVALIDCALL;
   DBG("Unsupported shader version: %u.%u !\n",
            }
   if (tx->processor != processor) {
      hr = D3DERR_INVALIDCALL;
   DBG("Shader type mismatch: %u / %u !\n", tx->processor, processor);
      }
   DUMP("%s%u.%u\n", processor == PIPE_SHADER_VERTEX ? "VS" : "PS",
                     if (tx->failure) {
      /* For VS shaders, we print the warning later,
         if (IS_PS)
         ureg_destroy(tx->ureg);
   hr = D3DERR_INVALIDCALL;
               /* Recompile after compacting constant slots if possible */
   if (!tx->indirect_const_access && !info->swvp_on && tx->num_slots > 0) {
      unsigned *slot_map;
   unsigned c;
            DBG("Recompiling shader for constant compaction\n");
            if (tx->num_inst_labels)
         FREE(tx->lconstf);
            num_ranges = 0;
   prev = -2;
   for (i = 0; i < NINE_MAX_CONST_ALL_VS; i++) {
         if (tx->slots_used[i]) {
      if (prev != i - 1)
            }
   slot_map = MALLOC(NINE_MAX_CONST_ALL_VS * sizeof(unsigned));
   const_ranges = CALLOC(num_ranges + 1, 2 * sizeof(unsigned)); /* ranges stop when last is of size 0 */
   if (!slot_map || !const_ranges) {
         hr = E_OUTOFMEMORY;
   }
   c = 0;
   j = -1;
   prev = -2;
   for (i = 0; i < NINE_MAX_CONST_ALL_VS; i++) {
         if (tx->slots_used[i]) {
      if (prev != i - 1)
         /* Initialize first slot of the range */
   if (!const_ranges[2*j+1])
         const_ranges[2*j+1]++;
   prev = i;
               if (tx_ctor(tx, screen, info) == E_OUTOFMEMORY) {
         hr = E_OUTOFMEMORY;
   }
   tx->always_output_pointsize = device->driver_caps.always_output_pointsize;
   tx->slot_map = slot_map;
   parse_shader(tx);
   #if !defined(NDEBUG)
         i = 0;
   j = 0;
   while (const_ranges[i*2+1] != 0) {
         j += const_ranges[i*2+1];
   }
   #endif
               /* record local constants */
   if (tx->num_lconstf && tx->indirect_const_access) {
      struct nine_range *ranges;
   float *data;
   int *indices;
                     data = MALLOC(tx->num_lconstf * 4 * sizeof(float));
   if (!data)
                  indices = MALLOC(tx->num_lconstf * sizeof(indices[0]));
   if (!indices)
            /* lazy sort, num_lconstf should be small */
   for (n = 0; n < tx->num_lconstf; ++n) {
         for (k = 0, i = 0; i < tx->num_lconstf; ++i) {
      if (tx->lconstf[i].idx < tx->lconstf[k].idx)
      }
   indices[n] = tx->lconstf[k].idx;
   memcpy(&data[n * 4], &tx->lconstf[k].f[0], 4 * sizeof(float));
            /* count ranges */
   for (n = 1, i = 1; i < tx->num_lconstf; ++i)
         if (indices[i] != indices[i - 1] + 1)
   ranges = MALLOC(n * sizeof(ranges[0]));
   if (!ranges) {
         FREE(indices);
   }
            k = 0;
   ranges[k].bgn = indices[0];
   for (i = 1; i < tx->num_lconstf; ++i) {
         if (indices[i] != indices[i - 1] + 1) {
      ranges[k].next = &ranges[k + 1];
   ranges[k].end = indices[i - 1] + 1;
   ++k;
      }
   ranges[k].end = indices[i - 1] + 1;
   ranges[k].next = NULL;
            FREE(indices);
               /* r500 */
   if (info->const_float_slots > device->max_vs_const_f &&
      (info->const_int_slots || info->const_bool_slots) &&
   !info->swvp_on)
            if (tx->indirect_const_access) { /* vs only */
      info->const_float_slots = device->max_vs_const_f;
               if (!info->swvp_on) {
      info->const_used_size = sizeof(float[4]) * tx->num_slots;
   if (tx->num_slots)
      } else {
         ureg_DECL_constant2D(tx->ureg, 0, 4095, 0);
   ureg_DECL_constant2D(tx->ureg, 0, 4095, 1);
   ureg_DECL_constant2D(tx->ureg, 0, 2047, 2);
            if (info->process_vertices)
            if (unlikely(nine_shader_get_debug_flag(NINE_SHADER_DEBUG_OPTION_DUMP_TGSI))) {
      const struct tgsi_token *toks = ureg_get_tokens(tx->ureg, NULL);
   tgsi_dump(toks, 0);
               if (info->process_vertices) {
      NineVertexDeclaration9_FillStreamOutputInfo(info->vdecl_out,
                        } else
         if (!info->cso) {
      hr = D3DERR_DRIVERINTERNALERROR;
   FREE(info->lconstf.data);
   FREE(info->lconstf.ranges);
               info->const_ranges = const_ranges;
   const_ranges = NULL;
      out:
      if (const_ranges)
         tx_dtor(tx);
      }
