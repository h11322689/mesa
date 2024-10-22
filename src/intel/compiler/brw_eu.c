   /*
   Copyright (C) Intel Corp.  2006.  All Rights Reserved.
   Intel funded Tungsten Graphics to
   develop this 3D driver.
      Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:
      The above copyright notice and this permission notice (including the
   next paragraph) shall be included in all copies or substantial
   portions of the Software.
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
      **********************************************************************/
   /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   */
      #include <sys/stat.h>
   #include <fcntl.h>
      #include "brw_eu_defines.h"
   #include "brw_eu.h"
   #include "brw_shader.h"
   #include "brw_gfx_ver_enum.h"
   #include "dev/intel_debug.h"
      #include "util/ralloc.h"
      /* Returns a conditional modifier that negates the condition. */
   enum brw_conditional_mod
   brw_negate_cmod(enum brw_conditional_mod cmod)
   {
      switch (cmod) {
   case BRW_CONDITIONAL_Z:
         case BRW_CONDITIONAL_NZ:
         case BRW_CONDITIONAL_G:
         case BRW_CONDITIONAL_GE:
         case BRW_CONDITIONAL_L:
         case BRW_CONDITIONAL_LE:
         default:
            }
      /* Returns the corresponding conditional mod for swapping src0 and
   * src1 in e.g. CMP.
   */
   enum brw_conditional_mod
   brw_swap_cmod(enum brw_conditional_mod cmod)
   {
      switch (cmod) {
   case BRW_CONDITIONAL_Z:
   case BRW_CONDITIONAL_NZ:
         case BRW_CONDITIONAL_G:
         case BRW_CONDITIONAL_GE:
         case BRW_CONDITIONAL_L:
         case BRW_CONDITIONAL_LE:
         default:
            }
      /**
   * Get the least significant bit offset of the i+1-th component of immediate
   * type \p type.  For \p i equal to the two's complement of j, return the
   * offset of the j-th component starting from the end of the vector.  For
   * scalar register types return zero.
   */
   static unsigned
   imm_shift(enum brw_reg_type type, unsigned i)
   {
      assert(type != BRW_REGISTER_TYPE_UV && type != BRW_REGISTER_TYPE_V &&
            if (type == BRW_REGISTER_TYPE_VF)
         else
      }
      /**
   * Swizzle an arbitrary immediate \p x of the given type according to the
   * permutation specified as \p swz.
   */
   uint32_t
   brw_swizzle_immediate(enum brw_reg_type type, uint32_t x, unsigned swz)
   {
      if (imm_shift(type, 1)) {
      const unsigned n = 32 / imm_shift(type, 1);
            for (unsigned i = 0; i < n; i++) {
      /* Shift the specified component all the way to the right and left to
   * discard any undesired L/MSBs, then shift it right into component i.
   */
   y |= x >> imm_shift(type, (i & ~3) + BRW_GET_SWZ(swz, i & 3))
                        } else {
            }
      unsigned
   brw_get_default_exec_size(struct brw_codegen *p)
   {
         }
      unsigned
   brw_get_default_group(struct brw_codegen *p)
   {
         }
      unsigned
   brw_get_default_access_mode(struct brw_codegen *p)
   {
         }
      struct tgl_swsb
   brw_get_default_swsb(struct brw_codegen *p)
   {
         }
      void
   brw_set_default_exec_size(struct brw_codegen *p, unsigned value)
   {
         }
      void brw_set_default_predicate_control(struct brw_codegen *p, enum brw_predicate pc)
   {
         }
      void brw_set_default_predicate_inverse(struct brw_codegen *p, bool predicate_inverse)
   {
         }
      void brw_set_default_flag_reg(struct brw_codegen *p, int reg, int subreg)
   {
      assert(subreg < 2);
      }
      void brw_set_default_access_mode( struct brw_codegen *p, unsigned access_mode )
   {
         }
      void
   brw_set_default_compression_control(struct brw_codegen *p,
         {
      switch (compression_control) {
   case BRW_COMPRESSION_NONE:
      /* This is the "use the first set of bits of dmask/vmask/arf
   * according to execsize" option.
   */
   p->current->group = 0;
      case BRW_COMPRESSION_2NDHALF:
      /* For SIMD8, this is "use the second set of 8 bits." */
   p->current->group = 8;
      case BRW_COMPRESSION_COMPRESSED:
      /* For SIMD16 instruction compression, use the first set of 16 bits
   * since we don't do SIMD32 dispatch.
   */
   p->current->group = 0;
      default:
                  if (p->devinfo->ver <= 6) {
      p->current->compressed =
         }
      /**
   * Enable or disable instruction compression on the given instruction leaving
   * the currently selected channel enable group untouched.
   */
   void
   brw_inst_set_compression(const struct intel_device_info *devinfo,
         {
      if (devinfo->ver >= 6) {
      /* No-op, the EU will figure out for us whether the instruction needs to
   * be compressed.
      } else {
      /* The channel group and compression controls are non-orthogonal, there
   * are two possible representations for uncompressed instructions and we
   * may need to preserve the current one to avoid changing the selected
   * channel group inadvertently.
   */
   if (on)
         else if (brw_inst_qtr_control(devinfo, inst)
               }
      void
   brw_set_default_compression(struct brw_codegen *p, bool on)
   {
         }
      /**
   * Apply the range of channel enable signals given by
   * [group, group + exec_size) to the instruction passed as argument.
   */
   void
   brw_inst_set_group(const struct intel_device_info *devinfo,
         {
      if (devinfo->ver >= 7) {
      assert(group % 4 == 0 && group < 32);
   brw_inst_set_qtr_control(devinfo, inst, group / 8);
         } else if (devinfo->ver == 6) {
      assert(group % 8 == 0 && group < 32);
         } else {
      assert(group % 8 == 0 && group < 16);
   /* The channel group and compression controls are non-orthogonal, there
   * are two possible representations for group zero and we may need to
   * preserve the current one to avoid changing the selected compression
   * enable inadvertently.
   */
   if (group == 8)
         else if (brw_inst_qtr_control(devinfo, inst) == BRW_COMPRESSION_2NDHALF)
         }
      void
   brw_set_default_group(struct brw_codegen *p, unsigned group)
   {
         }
      void brw_set_default_mask_control( struct brw_codegen *p, unsigned value )
   {
         }
      void brw_set_default_saturate( struct brw_codegen *p, bool enable )
   {
         }
      void brw_set_default_acc_write_control(struct brw_codegen *p, unsigned value)
   {
         }
      void brw_set_default_swsb(struct brw_codegen *p, struct tgl_swsb value)
   {
         }
      void brw_push_insn_state( struct brw_codegen *p )
   {
      assert(p->current != &p->stack[BRW_EU_MAX_INSN_STACK-1]);
   *(p->current + 1) = *p->current;
      }
      void brw_pop_insn_state( struct brw_codegen *p )
   {
      assert(p->current != p->stack);
      }
         /***********************************************************************
   */
   void
   brw_init_codegen(const struct brw_isa_info *isa,
         {
               p->isa = isa;
   p->devinfo = isa->devinfo;
   p->automatic_exec_sizes = true;
   /*
   * Set the initial instruction store array size to 1024, if found that
   * isn't enough, then it will double the store size at brw_next_insn()
   * until out of memory.
   */
   p->store_size = 1024;
   p->store = rzalloc_array(mem_ctx, brw_inst, p->store_size);
   p->nr_insn = 0;
   p->current = p->stack;
                     /* Some defaults?
   */
   brw_set_default_exec_size(p, BRW_EXECUTE_8);
   brw_set_default_mask_control(p, BRW_MASK_ENABLE); /* what does this do? */
   brw_set_default_saturate(p, 0);
            /* Set up control flow stack */
   p->if_stack_depth = 0;
   p->if_stack_array_size = 16;
            p->loop_stack_depth = 0;
   p->loop_stack_array_size = 16;
   p->loop_stack = rzalloc_array(mem_ctx, int, p->loop_stack_array_size);
      }
         const unsigned *brw_get_program( struct brw_codegen *p,
         {
      *sz = p->next_insn_offset;
      }
      const struct brw_shader_reloc *
   brw_get_shader_relocs(struct brw_codegen *p, unsigned *num_relocs)
   {
      *num_relocs = p->num_relocs;
      }
      bool brw_try_override_assembly(struct brw_codegen *p, int start_offset,
         {
      const char *read_path = getenv("INTEL_SHADER_ASM_READ_PATH");
   if (!read_path) {
                           int fd = open(name, O_RDONLY);
            if (fd == -1) {
                  struct stat sb;
   if (fstat(fd, &sb) != 0 || (!S_ISREG(sb.st_mode))) {
      close(fd);
               p->nr_insn -= (p->next_insn_offset - start_offset) / sizeof(brw_inst);
            p->next_insn_offset = start_offset + sb.st_size;
   p->store_size = (start_offset + sb.st_size) / sizeof(brw_inst);
   p->store = (brw_inst *)reralloc_size(p->mem_ctx, p->store, p->next_insn_offset);
            ssize_t ret = read(fd, (char *)p->store + start_offset, sb.st_size);
   close(fd);
   if (ret != sb.st_size) {
                  ASSERTED bool valid =
      brw_validate_instructions(p->isa, p->store,
                        }
      const struct brw_label *
   brw_find_label(const struct brw_label *root, int offset)
   {
               if (curr != NULL)
   {
      do {
                                       }
      void
   brw_create_label(struct brw_label **labels, int offset, void *mem_ctx)
   {
      if (*labels != NULL) {
      struct brw_label *curr = *labels;
            do {
                                          curr = ralloc(mem_ctx, struct brw_label);
   curr->offset = offset;
   curr->number = prev->number + 1;
   curr->next = NULL;
      } else {
      struct brw_label *root = ralloc(mem_ctx, struct brw_label);
   root->number = 0;
   root->offset = offset;
   root->next = NULL;
         }
      const struct brw_label *
   brw_label_assembly(const struct brw_isa_info *isa,
         {
                                 for (int offset = start; offset < end;) {
      const brw_inst *inst = (const brw_inst *) ((const char *) assembly + offset);
                     if (is_compact) {
      brw_compact_inst *compacted = (brw_compact_inst *)inst;
   brw_uncompact_instruction(isa, &uncompacted, compacted);
               if (brw_has_uip(devinfo, brw_inst_opcode(isa, inst))) {
      /* Instructions that have UIP also have JIP. */
   brw_create_label(&root_label,
         brw_create_label(&root_label,
      } else if (brw_has_jip(devinfo, brw_inst_opcode(isa, inst))) {
      int jip;
   if (devinfo->ver >= 7) {
         } else {
                              if (is_compact) {
         } else {
                        }
      void
   brw_disassemble_with_labels(const struct brw_isa_info *isa,
         {
      void *mem_ctx = ralloc_context(NULL);
   const struct brw_label *root_label =
                        }
      void
   brw_disassemble(const struct brw_isa_info *isa,
               {
                        for (int offset = start; offset < end;) {
      const brw_inst *insn = (const brw_inst *)((char *)assembly + offset);
            if (root_label != NULL) {
   const struct brw_label *label = brw_find_label(root_label, offset);
   if (label != NULL) {
         }
            bool compacted = brw_inst_cmpt_control(devinfo, insn);
   if (0)
            if (compacted) {
      brw_compact_inst *compacted = (brw_compact_inst *)insn;
   if (dump_hex) {
      unsigned char * insn_ptr = ((unsigned char *)&insn[0]);
   const unsigned int blank_spaces = 24;
   for (int i = 0 ; i < 8; i = i + 4) {
      fprintf(out, "%02x %02x %02x %02x ",
         insn_ptr[i],
   insn_ptr[i + 1],
      }
   /* Make compacted instructions hex value output vertically aligned
   * with uncompacted instructions hex value
   */
               brw_uncompact_instruction(isa, &uncompacted, compacted);
      } else {
      if (dump_hex) {
      unsigned char * insn_ptr = ((unsigned char *)&insn[0]);
   for (int i = 0 ; i < 16; i = i + 4) {
      fprintf(out, "%02x %02x %02x %02x ",
         insn_ptr[i],
   insn_ptr[i + 1],
                              if (compacted) {
         } else {
               }
      static const struct opcode_desc opcode_descs[] = {
      /* IR,                 HW,  name,      nsrc, ndst, gfx_vers */
   { BRW_OPCODE_ILLEGAL,  0,   "illegal", 0,    0,    GFX_ALL },
   { BRW_OPCODE_SYNC,     1,   "sync",    1,    0,    GFX_GE(GFX12) },
   { BRW_OPCODE_MOV,      1,   "mov",     1,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_MOV,      97,  "mov",     1,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_SEL,      2,   "sel",     2,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_SEL,      98,  "sel",     2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_MOVI,     3,   "movi",    2,    1,    GFX_GE(GFX45) & GFX_LT(GFX12) },
   { BRW_OPCODE_MOVI,     99,  "movi",    2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_NOT,      4,   "not",     1,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_NOT,      100, "not",     1,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_AND,      5,   "and",     2,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_AND,      101, "and",     2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_OR,       6,   "or",      2,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_OR,       102, "or",      2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_XOR,      7,   "xor",     2,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_XOR,      103, "xor",     2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_SHR,      8,   "shr",     2,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_SHR,      104, "shr",     2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_SHL,      9,   "shl",     2,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_SHL,      105, "shl",     2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_DIM,      10,  "dim",     1,    1,    GFX75 },
   { BRW_OPCODE_SMOV,     10,  "smov",    0,    0,    GFX_GE(GFX8) & GFX_LT(GFX12) },
   { BRW_OPCODE_SMOV,     106, "smov",    0,    0,    GFX_GE(GFX12) },
   { BRW_OPCODE_ASR,      12,  "asr",     2,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_ASR,      108, "asr",     2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_ROR,      14,  "ror",     2,    1,    GFX11 },
   { BRW_OPCODE_ROR,      110, "ror",     2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_ROL,      15,  "rol",     2,    1,    GFX11 },
   { BRW_OPCODE_ROL,      111, "rol",     2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_CMP,      16,  "cmp",     2,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_CMP,      112, "cmp",     2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_CMPN,     17,  "cmpn",    2,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_CMPN,     113, "cmpn",    2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_CSEL,     18,  "csel",    3,    1,    GFX_GE(GFX8) & GFX_LT(GFX12) },
   { BRW_OPCODE_CSEL,     114, "csel",    3,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_F32TO16,  19,  "f32to16", 1,    1,    GFX7 | GFX75 },
   { BRW_OPCODE_F16TO32,  20,  "f16to32", 1,    1,    GFX7 | GFX75 },
   { BRW_OPCODE_BFREV,    23,  "bfrev",   1,    1,    GFX_GE(GFX7) & GFX_LT(GFX12) },
   { BRW_OPCODE_BFREV,    119, "bfrev",   1,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_BFE,      24,  "bfe",     3,    1,    GFX_GE(GFX7) & GFX_LT(GFX12) },
   { BRW_OPCODE_BFE,      120, "bfe",     3,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_BFI1,     25,  "bfi1",    2,    1,    GFX_GE(GFX7) & GFX_LT(GFX12) },
   { BRW_OPCODE_BFI1,     121, "bfi1",    2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_BFI2,     26,  "bfi2",    3,    1,    GFX_GE(GFX7) & GFX_LT(GFX12) },
   { BRW_OPCODE_BFI2,     122, "bfi2",    3,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_JMPI,     32,  "jmpi",    0,    0,    GFX_ALL },
   { BRW_OPCODE_BRD,      33,  "brd",     0,    0,    GFX_GE(GFX7) },
   { BRW_OPCODE_IF,       34,  "if",      0,    0,    GFX_ALL },
   { BRW_OPCODE_IFF,      35,  "iff",     0,    0,    GFX_LE(GFX5) },
   { BRW_OPCODE_BRC,      35,  "brc",     0,    0,    GFX_GE(GFX7) },
   { BRW_OPCODE_ELSE,     36,  "else",    0,    0,    GFX_ALL },
   { BRW_OPCODE_ENDIF,    37,  "endif",   0,    0,    GFX_ALL },
   { BRW_OPCODE_DO,       38,  "do",      0,    0,    GFX_LE(GFX5) },
   { BRW_OPCODE_CASE,     38,  "case",    0,    0,    GFX6 },
   { BRW_OPCODE_WHILE,    39,  "while",   0,    0,    GFX_ALL },
   { BRW_OPCODE_BREAK,    40,  "break",   0,    0,    GFX_ALL },
   { BRW_OPCODE_CONTINUE, 41,  "cont",    0,    0,    GFX_ALL },
   { BRW_OPCODE_HALT,     42,  "halt",    0,    0,    GFX_ALL },
   { BRW_OPCODE_CALLA,    43,  "calla",   0,    0,    GFX_GE(GFX75) },
   { BRW_OPCODE_MSAVE,    44,  "msave",   0,    0,    GFX_LE(GFX5) },
   { BRW_OPCODE_CALL,     44,  "call",    0,    0,    GFX_GE(GFX6) },
   { BRW_OPCODE_MREST,    45,  "mrest",   0,    0,    GFX_LE(GFX5) },
   { BRW_OPCODE_RET,      45,  "ret",     0,    0,    GFX_GE(GFX6) },
   { BRW_OPCODE_PUSH,     46,  "push",    0,    0,    GFX_LE(GFX5) },
   { BRW_OPCODE_FORK,     46,  "fork",    0,    0,    GFX6 },
   { BRW_OPCODE_GOTO,     46,  "goto",    0,    0,    GFX_GE(GFX8) },
   { BRW_OPCODE_POP,      47,  "pop",     2,    0,    GFX_LE(GFX5) },
   { BRW_OPCODE_WAIT,     48,  "wait",    0,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_SEND,     49,  "send",    1,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_SENDC,    50,  "sendc",   1,    1,    GFX_LT(GFX12) },
   { BRW_OPCODE_SEND,     49,  "send",    2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_SENDC,    50,  "sendc",   2,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_SENDS,    51,  "sends",   2,    1,    GFX_GE(GFX9) & GFX_LT(GFX12) },
   { BRW_OPCODE_SENDSC,   52,  "sendsc",  2,    1,    GFX_GE(GFX9) & GFX_LT(GFX12) },
   { BRW_OPCODE_MATH,     56,  "math",    2,    1,    GFX_GE(GFX6) },
   { BRW_OPCODE_ADD,      64,  "add",     2,    1,    GFX_ALL },
   { BRW_OPCODE_MUL,      65,  "mul",     2,    1,    GFX_ALL },
   { BRW_OPCODE_AVG,      66,  "avg",     2,    1,    GFX_ALL },
   { BRW_OPCODE_FRC,      67,  "frc",     1,    1,    GFX_ALL },
   { BRW_OPCODE_RNDU,     68,  "rndu",    1,    1,    GFX_ALL },
   { BRW_OPCODE_RNDD,     69,  "rndd",    1,    1,    GFX_ALL },
   { BRW_OPCODE_RNDE,     70,  "rnde",    1,    1,    GFX_ALL },
   { BRW_OPCODE_RNDZ,     71,  "rndz",    1,    1,    GFX_ALL },
   { BRW_OPCODE_MAC,      72,  "mac",     2,    1,    GFX_ALL },
   { BRW_OPCODE_MACH,     73,  "mach",    2,    1,    GFX_ALL },
   { BRW_OPCODE_LZD,      74,  "lzd",     1,    1,    GFX_ALL },
   { BRW_OPCODE_FBH,      75,  "fbh",     1,    1,    GFX_GE(GFX7) },
   { BRW_OPCODE_FBL,      76,  "fbl",     1,    1,    GFX_GE(GFX7) },
   { BRW_OPCODE_CBIT,     77,  "cbit",    1,    1,    GFX_GE(GFX7) },
   { BRW_OPCODE_ADDC,     78,  "addc",    2,    1,    GFX_GE(GFX7) },
   { BRW_OPCODE_SUBB,     79,  "subb",    2,    1,    GFX_GE(GFX7) },
   { BRW_OPCODE_SAD2,     80,  "sad2",    2,    1,    GFX_ALL },
   { BRW_OPCODE_SADA2,    81,  "sada2",   2,    1,    GFX_ALL },
   { BRW_OPCODE_ADD3,     82,  "add3",    3,    1,    GFX_GE(GFX125) },
   { BRW_OPCODE_DP4,      84,  "dp4",     2,    1,    GFX_LT(GFX11) },
   { BRW_OPCODE_DPH,      85,  "dph",     2,    1,    GFX_LT(GFX11) },
   { BRW_OPCODE_DP3,      86,  "dp3",     2,    1,    GFX_LT(GFX11) },
   { BRW_OPCODE_DP2,      87,  "dp2",     2,    1,    GFX_LT(GFX11) },
   { BRW_OPCODE_DP4A,     88,  "dp4a",    3,    1,    GFX_GE(GFX12) },
   { BRW_OPCODE_LINE,     89,  "line",    2,    1,    GFX_LE(GFX10) },
   { BRW_OPCODE_PLN,      90,  "pln",     2,    1,    GFX_GE(GFX45) & GFX_LE(GFX10) },
   { BRW_OPCODE_MAD,      91,  "mad",     3,    1,    GFX_GE(GFX6) },
   { BRW_OPCODE_LRP,      92,  "lrp",     3,    1,    GFX_GE(GFX6) & GFX_LE(GFX10) },
   { BRW_OPCODE_MADM,     93,  "madm",    3,    1,    GFX_GE(GFX8) },
   { BRW_OPCODE_NENOP,    125, "nenop",   0,    0,    GFX45 },
   { BRW_OPCODE_NOP,      126, "nop",     0,    0,    GFX_LT(GFX12) },
      };
      void
   brw_init_isa_info(struct brw_isa_info *isa,
         {
                        memset(isa->ir_to_descs, 0, sizeof(isa->ir_to_descs));
            for (unsigned i = 0; i < ARRAY_SIZE(opcode_descs); i++) {
      if (opcode_descs[i].gfx_vers & ver) {
      const unsigned e = opcode_descs[i].ir;
   const unsigned h = opcode_descs[i].hw;
   assert(e < ARRAY_SIZE(isa->ir_to_descs) && !isa->ir_to_descs[e]);
   assert(h < ARRAY_SIZE(isa->hw_to_descs) && !isa->hw_to_descs[h]);
   isa->ir_to_descs[e] = &opcode_descs[i];
            }
      /**
   * Return the matching opcode_desc for the specified IR opcode and hardware
   * generation, or NULL if the opcode is not supported by the device.
   */
   const struct opcode_desc *
   brw_opcode_desc(const struct brw_isa_info *isa, enum opcode op)
   {
         }
      /**
   * Return the matching opcode_desc for the specified HW opcode and hardware
   * generation, or NULL if the opcode is not supported by the device.
   */
   const struct opcode_desc *
   brw_opcode_desc_from_hw(const struct brw_isa_info *isa, unsigned hw)
   {
         }
      unsigned
   brw_num_sources_from_inst(const struct brw_isa_info *isa,
         {
      const struct intel_device_info *devinfo = isa->devinfo;
   const struct opcode_desc *desc =
                  if (brw_inst_opcode(isa, inst) == BRW_OPCODE_MATH) {
         } else if (devinfo->ver < 6 &&
            if (brw_inst_sfid(devinfo, inst) == BRW_SFID_MATH) {
      /* src1 must be a descriptor (including the information to determine
   * that the SEND is doing an extended math operation), but src0 can
   * actually be null since it serves as the source of the implicit GRF
   * to MRF move.
   *
   * If we stop using that functionality, we'll have to revisit this.
   */
      } else {
      /* Send instructions are allowed to have null sources since they use
   * the base_mrf field to specify which message register source.
   */
         } else {
      assert(desc->nsrc < 4);
               switch (math_function) {
   case BRW_MATH_FUNCTION_INV:
   case BRW_MATH_FUNCTION_LOG:
   case BRW_MATH_FUNCTION_EXP:
   case BRW_MATH_FUNCTION_SQRT:
   case BRW_MATH_FUNCTION_RSQ:
   case BRW_MATH_FUNCTION_SIN:
   case BRW_MATH_FUNCTION_COS:
   case BRW_MATH_FUNCTION_SINCOS:
   case GFX8_MATH_FUNCTION_INVM:
   case GFX8_MATH_FUNCTION_RSQRTM:
         case BRW_MATH_FUNCTION_FDIV:
   case BRW_MATH_FUNCTION_POW:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT:
   case BRW_MATH_FUNCTION_INT_DIV_REMAINDER:
         default:
            }
