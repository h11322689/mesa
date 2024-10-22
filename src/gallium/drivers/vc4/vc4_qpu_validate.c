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
      #include <stdlib.h>
      #include "vc4_qpu.h"
      static void
   fail_instr(uint64_t inst, const char *msg)
   {
         fprintf(stderr, "vc4_qpu_validate: %s: ", msg);
   vc4_qpu_disasm(&inst, 1);
   fprintf(stderr, "\n");
   }
      static bool
   writes_reg(uint64_t inst, uint32_t w)
   {
         return (QPU_GET_FIELD(inst, QPU_WADDR_ADD) == w ||
   }
      static bool
   _reads_reg(uint64_t inst, uint32_t r, bool ignore_a, bool ignore_b)
   {
         struct {
         } src_regs[] = {
            { QPU_GET_FIELD(inst, QPU_ADD_A) },
   { QPU_GET_FIELD(inst, QPU_ADD_B) },
               /* Branches only reference raddr_a (no mux), and we don't use that
      * feature of branching.
      if (QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_BRANCH)
            /* Load immediates don't read any registers. */
   if (QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_LOAD_IMM)
            for (int i = 0; i < ARRAY_SIZE(src_regs); i++) {
            if (!ignore_a &&
                        if (!ignore_b &&
      QPU_GET_FIELD(inst, QPU_SIG) != QPU_SIG_SMALL_IMM &&
   src_regs[i].mux == QPU_MUX_B &&
            }
      static bool
   reads_reg(uint64_t inst, uint32_t r)
   {
         }
      static bool
   reads_a_reg(uint64_t inst, uint32_t r)
   {
         }
      static bool
   reads_b_reg(uint64_t inst, uint32_t r)
   {
         }
      static bool
   writes_sfu(uint64_t inst)
   {
         return (writes_reg(inst, QPU_W_SFU_RECIP) ||
               }
      /**
   * Checks for the instruction restrictions from page 37 ("Summary of
   * Instruction Restrictions").
   */
   void
   vc4_qpu_validate(uint64_t *insts, uint32_t num_inst)
   {
         bool scoreboard_locked = false;
            /* We don't want to do validation in release builds, but we want to
      * keep compiling the validation code to make sure it doesn't get
      #ifndef DEBUG
         #endif
            for (int i = 0; i < num_inst; i++) {
                                                                                          /* "The Thread End instruction must not write to either physical
   *  regfile A or B."
   */
   if (QPU_GET_FIELD(inst, QPU_WADDR_ADD) < 32 ||
                        /* Can't trigger an implicit wait on scoreboard in the program
   * end instruction.
                                       for (int j = i; j < i + 2; j++) {
            /* "The last three instructions of any program
   *  (Thread End plus the following two delay-slot
   *  instructions) must not do varyings read, uniforms
   *  read or any kind of VPM, VDR, or VDW read or
   *  write."
   */
   if (writes_reg(insts[j], QPU_W_VPM) ||
      reads_reg(insts[j], QPU_R_VARY) ||
                                 /* "The Thread End instruction and the following two
   *  delay slot instructions must not write or read
   *  address 14 in either regfile A or B."
   */
   if (writes_reg(insts[j], 14) ||
                           /* "The final program instruction (the second delay slot
      *  instruction) must not do a TLB Z write."
      if (writes_reg(insts[i + 2], QPU_W_TLB_Z)) {
                     /* "A scoreboard wait must not occur in the first two instructions of
      *  a fragment shader. This is either the explicit Wait for Scoreboard
   *  signal or an implicit wait with the first tile-buffer read or
   *  write instruction."
      for (int i = 0; i < 2; i++) {
                                 /* "If TMU_NOSWAP is written, the write must be three instructions
      *  before the first TMU write instruction.  For example, if
   *  TMU_NOSWAP is written in the first shader instruction, the first
   *  TMU write cannot occur before the 4th shader instruction."
      int last_tmu_noswap = -10;
   for (int i = 0; i < num_inst; i++) {
                     if ((i - last_tmu_noswap) <= 3 &&
      (writes_reg(inst, QPU_W_TMU0_S) ||
                                 /* "An instruction must not read from a location in physical regfile A
      *  or B that was written to by the previous instruction."
      for (int i = 0; i < num_inst - 1; i++) {
            uint64_t inst = insts[i];
                        if (inst & QPU_WS) {
               } else {
                        if ((waddr_a < 32 && reads_a_reg(insts[i + 1], waddr_a)) ||
      (waddr_b < 32 && reads_b_reg(insts[i + 1], waddr_b))) {
                  /* "After an SFU lookup instruction, accumulator r4 must not be read
      *  in the following two instructions. Any other instruction that
   *  results in r4 being written (that is, TMU read, TLB read, SFU
   *  lookup) cannot occur in the two instructions following an SFU
   *  lookup."
      int last_sfu_inst = -10;
   for (int i = 0; i < num_inst - 1; i++) {
                           if (i - last_sfu_inst <= 2 &&
      (writes_sfu(inst) ||
      sig == QPU_SIG_LOAD_TMU0 ||
                                    for (int i = 0; i < num_inst - 1; i++) {
                     if (QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_SMALL_IMM &&
      QPU_GET_FIELD(inst, QPU_SMALL_IMM) >=
                              /* "The full horizontal vector rotate is only
   *  available when both of the mul ALU input arguments
   *  are taken from accumulators r0-r3."
   */
   if (mux_a > QPU_MUX_R3 || mux_b > QPU_MUX_R3) {
                              if (QPU_GET_FIELD(inst, QPU_SMALL_IMM) ==
      QPU_SMALL_IMM_MUL_ROT) {
      /* "An instruction that does a vector rotate
      *  by r5 must not immediately follow an
   *  instruction that writes to r5."
      if (writes_reg(insts[i - 1], QPU_W_ACC5)) {
                              /* "An instruction that does a vector rotate must not
   *  immediately follow an instruction that writes to the
   *  accumulator that is being rotated."
   */
   if (writes_reg(insts[i - 1], QPU_W_ACC0 + mux_a) ||
      writes_reg(insts[i - 1], QPU_W_ACC0 + mux_b)) {
      fail_instr(inst,
            /* "An instruction that does a vector rotate must not immediately
      *  follow an instruction that writes to the accumulator that is being
   *  rotated.
   *
               /* "After an instruction that does a TLB Z write, the multisample mask
      *  must not be read as an instruction input argument in the following
   *  two instruction. The TLB Z write instruction can, however, be
   *  followed immediately by a TLB color write."
      for (int i = 0; i < num_inst - 1; i++) {
            uint64_t inst = insts[i];
   if (writes_reg(inst, QPU_W_TLB_Z) &&
      (reads_a_reg(insts[i + 1], QPU_R_MS_REV_FLAGS) ||
                  /*
      * "A single instruction can only perform a maximum of one of the
   *  following closely coupled peripheral accesses in a single
   *  instruction: TMU write, TMU read, TLB write, TLB read, TLB
   *  combined color read and write, SFU write, Mutex read or Semaphore
   *  access."
      for (int i = 0; i < num_inst - 1; i++) {
                                 /* "The uniform base pointer can be written (from SIMD element 0) by
      *  the processor to reset the stream, there must be at least two
   *  nonuniform-accessing instructions following a pointer change
   *  before uniforms can be accessed once more."
      int last_unif_pointer_update = -3;
   for (int i = 0; i < num_inst; i++) {
                                 if (reads_reg(inst, QPU_R_UNIF) &&
      i - last_unif_pointer_update <= 2) {
                     if (waddr_add == QPU_W_UNIFORMS_ADDRESS ||
               if (threaded) {
            bool last_thrsw_found = false;
   bool scoreboard_locked = false;
                                                      if (i == thrsw_ip) {
         /* In order to get texture results back in the
      * correct order, before a new thrsw we have
   * to read all the texture results from before
   * the previous thrsw.
   *
   * FIXME: Is collecting the remaining results
   * during the delay slots OK, or should we do
   * this at THRSW signal time?
                                                                           switch (sig) {
   case QPU_SIG_THREAD_SWITCH:
   case QPU_SIG_LAST_THREAD_SWITCH:
         /* No thread switching with the scoreboard
      * locked.  Doing so means we may deadlock
   * when the other thread tries to lock
                                       /* No thread switching after lthrsw, since
      * lthrsw means that we get delayed until the
                                                      /* No THRSW while we already have a THRSW
                                                   case QPU_SIG_LOAD_TMU0:
   case QPU_SIG_LOAD_TMU1:
                                                         uint32_t waddr_add = QPU_GET_FIELD(inst, QPU_WADDR_ADD);
   uint32_t waddr_mul = QPU_GET_FIELD(inst, QPU_WADDR_MUL);
   if (waddr_add == QPU_W_TMU0_S ||
      waddr_add == QPU_W_TMU1_S ||
   waddr_mul == QPU_W_TMU0_S ||
      }
