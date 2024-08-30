   /*
   * Copyright © 2014 Broadcom
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
      #include "util/ralloc.h"
   #include "util/register_allocate.h"
   #include "vc4_context.h"
   #include "vc4_qir.h"
   #include "vc4_qpu.h"
      #define QPU_R(file, index) { QPU_MUX_##file, index }
      static const struct qpu_reg vc4_regs[] = {
         { QPU_MUX_R0, 0},
   { QPU_MUX_R1, 0},
   { QPU_MUX_R2, 0},
   { QPU_MUX_R3, 0},
   { QPU_MUX_R4, 0},
   QPU_R(A, 0),
   QPU_R(B, 0),
   QPU_R(A, 1),
   QPU_R(B, 1),
   QPU_R(A, 2),
   QPU_R(B, 2),
   QPU_R(A, 3),
   QPU_R(B, 3),
   QPU_R(A, 4),
   QPU_R(B, 4),
   QPU_R(A, 5),
   QPU_R(B, 5),
   QPU_R(A, 6),
   QPU_R(B, 6),
   QPU_R(A, 7),
   QPU_R(B, 7),
   QPU_R(A, 8),
   QPU_R(B, 8),
   QPU_R(A, 9),
   QPU_R(B, 9),
   QPU_R(A, 10),
   QPU_R(B, 10),
   QPU_R(A, 11),
   QPU_R(B, 11),
   QPU_R(A, 12),
   QPU_R(B, 12),
   QPU_R(A, 13),
   QPU_R(B, 13),
   QPU_R(A, 14),
   QPU_R(B, 14),
   QPU_R(A, 15),
   QPU_R(B, 15),
   QPU_R(A, 16),
   QPU_R(B, 16),
   QPU_R(A, 17),
   QPU_R(B, 17),
   QPU_R(A, 18),
   QPU_R(B, 18),
   QPU_R(A, 19),
   QPU_R(B, 19),
   QPU_R(A, 20),
   QPU_R(B, 20),
   QPU_R(A, 21),
   QPU_R(B, 21),
   QPU_R(A, 22),
   QPU_R(B, 22),
   QPU_R(A, 23),
   QPU_R(B, 23),
   QPU_R(A, 24),
   QPU_R(B, 24),
   QPU_R(A, 25),
   QPU_R(B, 25),
   QPU_R(A, 26),
   QPU_R(B, 26),
   QPU_R(A, 27),
   QPU_R(B, 27),
   QPU_R(A, 28),
   QPU_R(B, 28),
   QPU_R(A, 29),
   QPU_R(B, 29),
   QPU_R(A, 30),
   QPU_R(B, 30),
   QPU_R(A, 31),
   };
   #define ACC_INDEX     0
   #define ACC_COUNT     5
   #define AB_INDEX      (ACC_INDEX + ACC_COUNT)
   #define AB_COUNT      64
      static void
   vc4_alloc_reg_set(struct vc4_context *vc4)
   {
         assert(vc4_regs[AB_INDEX].addr == 0);
   assert(vc4_regs[AB_INDEX + 1].addr == 0);
            if (vc4->regs)
                     /* The physical regfiles split us into two classes, with [0] being the
      * whole space and [1] being the bottom half (for threaded fragment
   * shaders).
      for (int i = 0; i < 2; i++) {
            vc4->reg_class_any[i] = ra_alloc_contig_reg_class(vc4->regs, 1);
   vc4->reg_class_a_or_b[i] = ra_alloc_contig_reg_class(vc4->regs, 1);
   vc4->reg_class_a_or_b_or_acc[i] = ra_alloc_contig_reg_class(vc4->regs, 1);
      }
            /* r0-r3 */
   for (uint32_t i = ACC_INDEX; i < ACC_INDEX + 4; i++) {
            ra_class_add_reg(vc4->reg_class_r0_r3, i);
               /* R4 gets a special class because it can't be written as a general
      * purpose register. (it's TMU_NOSWAP as a write address).
      for (int i = 0; i < 2; i++) {
                        /* A/B */
   for (uint32_t i = AB_INDEX; i < AB_INDEX + 64; i ++) {
            /* Reserve ra14/rb14 for spilling fixup_raddr_conflict() in
   * vc4_qpu_emit.c
                                             if (vc4_regs[i].addr < 16) {
                                 /* A only */
                                 if (vc4_regs[i].addr < 16) {
                  }
      struct node_to_temp_map {
         uint32_t temp;
   };
      static int
   node_to_temp_priority(const void *in_a, const void *in_b)
   {
         const struct node_to_temp_map *a = in_a;
            }
      #define CLASS_BIT_A			(1 << 0)
   #define CLASS_BIT_B			(1 << 1)
   #define CLASS_BIT_R4			(1 << 2)
   #define CLASS_BIT_R0_R3			(1 << 4)
      struct vc4_ra_select_callback_data {
         uint32_t next_acc;
   };
      static unsigned int
   vc4_ra_select_callback(unsigned int n, BITSET_WORD *regs, void *data)
   {
                  /* If r4 is available, always choose it -- few other things can go
      * there, and choosing anything else means inserting a mov.
      if (BITSET_TEST(regs, ACC_INDEX + 4))
            /* Choose an accumulator if possible (no delay between write and
      * read), but round-robin through them to give post-RA instruction
   * selection more options.
      for (int i = 0; i < ACC_COUNT; i++) {
                           if (BITSET_TEST(regs, acc)) {
                     for (int i = 0; i < AB_COUNT; i++) {
                           if (BITSET_TEST(regs, ab)) {
                     }
      /**
   * Returns a mapping from QFILE_TEMP indices to struct qpu_regs.
   *
   * The return value should be freed by the caller.
   */
   struct qpu_reg *
   vc4_register_allocate(struct vc4_context *vc4, struct vc4_compile *c)
   {
         struct node_to_temp_map map[c->num_temps];
   uint32_t temp_to_node[c->num_temps];
   uint8_t class_bits[c->num_temps];
   struct qpu_reg *temp_registers = calloc(c->num_temps,
         struct vc4_ra_select_callback_data callback_data = {
                        /* If things aren't ever written (undefined values), just read from
      * r0.
      for (uint32_t i = 0; i < c->num_temps; i++)
                     struct ra_graph *g = ra_alloc_interference_graph(vc4->regs,
            /* Compute the live ranges so we can figure out interference. */
                     for (uint32_t i = 0; i < c->num_temps; i++) {
               }
   qsort(map, c->num_temps, sizeof(map[0]), node_to_temp_priority);
   for (uint32_t i = 0; i < c->num_temps; i++) {
                  /* Figure out our register classes and preallocated registers.  We
      * start with any temp being able to be in any file, then instructions
   * incrementally remove bits that the temp definitely can't be in.
      memset(class_bits,
                  int ip = 0;
   qir_for_each_inst_inorder(inst, c) {
            if (qir_writes_r4(inst)) {
            /* This instruction writes r4 (and optionally moves
   * its result to a temp), so nothing else can be
   * stored in r4 across it.
   */
                              /* If we're doing a conditional write of something
   * writing R4 (math, tex results), then make sure that
   * we store in a temp so that we actually
   * conditionally move the result.
   */
      } else {
            /* R4 can't be written as a general purpose
   * register. (it's TMU_NOSWAP as a write address).
                     switch (inst->op) {
   case QOP_FRAG_Z:
                        case QOP_FRAG_W:
                        case QOP_ROT_MUL:
                        case QOP_THRSW:
            /* All accumulators are invalidated across a thread
   * switch.
   */
   for (int i = 0; i < c->num_temps; i++) {
                                                if (inst->dst.pack && !qir_is_mul(inst)) {
            /* The non-MUL pack flags require an A-file dst
                     /* Apply restrictions for src unpacks.  The integer unpacks
   * can only be done from regfile A, while float unpacks can be
   * either A or R4.
   */
   for (int i = 0; i < qir_get_nsrc(inst); i++) {
            if (inst->src[i].file == QFILE_TEMP &&
      inst->src[i].pack) {
      if (qir_is_float_input(inst)) {
               } else {
                           for (uint32_t i = 0; i < c->num_temps; i++) {
                     switch (class_bits[i]) {
   case CLASS_BIT_A | CLASS_BIT_B | CLASS_BIT_R4 | CLASS_BIT_R0_R3:
            ra_set_node_class(g, node,
      case CLASS_BIT_A | CLASS_BIT_B:
            ra_set_node_class(g, node,
      case CLASS_BIT_A | CLASS_BIT_B | CLASS_BIT_R0_R3:
            ra_set_node_class(g, node,
      case CLASS_BIT_A | CLASS_BIT_R4:
            ra_set_node_class(g, node,
      case CLASS_BIT_A:
            ra_set_node_class(g, node,
                           default:
            /* DDX/DDY used across thread switched might get us
   * here.
   */
   if (c->fs_threaded) {
                              fprintf(stderr, "temp %d: bad class bits: 0x%x\n",
                  for (uint32_t i = 0; i < c->num_temps; i++) {
            for (uint32_t j = i + 1; j < c->num_temps; j++) {
            if (!(c->temp_start[i] >= c->temp_end[j] ||
         c->temp_start[j] >= c->temp_end[i])) {
   ra_add_node_interference(g,
            bool ok = ra_allocate(g);
   if (!ok) {
            if (!c->fs_threaded) {
                        c->failed = true;
               for (uint32_t i = 0; i < c->num_temps; i++) {
                     /* If the value's never used, just write to the NOP register
   * for clarity in debug output.
   */
                        }
