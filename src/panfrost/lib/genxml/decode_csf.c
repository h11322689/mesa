   /*
   * Copyright (C) 2022-2023 Collabora, Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "genxml/gen_macros.h"
   #include "decode.h"
      #if PAN_ARCH >= 10
   /* Limit for Mali-G610. -1 because we're not including the active frame */
   #define MAX_CALL_STACK_DEPTH (8 - 1)
      struct queue_ctx {
      /* Size of CSHWIF register file in 32-bit registers */
            /* CSHWIF register file */
            /* Current instruction pointer (CPU pointer for convenience) */
            /* Current instruction end pointer */
            /* Call stack. Depth=0 means root */
   struct {
      /* Link register to return to */
            /* End pointer, there is a return (or exit) after */
      } call_stack[MAX_CALL_STACK_DEPTH];
               };
      static uint32_t
   cs_get_u32(struct queue_ctx *qctx, uint8_t reg)
   {
      assert(reg < qctx->nr_regs);
      }
      static uint64_t
   cs_get_u64(struct queue_ctx *qctx, uint8_t reg)
   {
         }
      static void
   pandecode_run_compute(struct pandecode_context *ctx, FILE *fp,
         {
               /* Print the instruction. Ignore the selects and the flags override
   * since we'll print them implicitly later.
   */
                     unsigned reg_srt = 0 + (I->srt_select * 2);
   unsigned reg_fau = 8 + (I->fau_select * 2);
   unsigned reg_spd = 16 + (I->spd_select * 2);
                              if (fau)
            GENX(pandecode_shader)
            DUMP_ADDR(ctx, LOCAL_STORAGE, cs_get_u64(qctx, reg_tsd),
            pandecode_log(ctx, "Global attribute offset: %u\n", cs_get_u32(qctx, 32));
   DUMP_CL(ctx, COMPUTE_SIZE_WORKGROUP, &qctx->regs[33], "Workgroup size\n");
   pandecode_log(ctx, "Job offset X: %u\n", cs_get_u32(qctx, 34));
   pandecode_log(ctx, "Job offset Y: %u\n", cs_get_u32(qctx, 35));
   pandecode_log(ctx, "Job offset Z: %u\n", cs_get_u32(qctx, 36));
   pandecode_log(ctx, "Job size X: %u\n", cs_get_u32(qctx, 37));
   pandecode_log(ctx, "Job size Y: %u\n", cs_get_u32(qctx, 38));
               }
      static void
   pandecode_run_idvs(struct pandecode_context *ctx, FILE *fp,
         {
      /* Print the instruction. Ignore the selects and the flags override
   * since we'll print them implicitly later.
   */
            if (I->draw_id_register_enable)
                              /* Merge flag overrides with the register flags */
   uint32_t tiler_flags_raw = cs_get_u64(qctx, 56);
   tiler_flags_raw |= I->flags_override;
            unsigned reg_position_srt = 0;
   unsigned reg_position_fau = 8;
            unsigned reg_vary_srt = I->varying_srt_select ? 2 : 0;
   unsigned reg_vary_fau = I->varying_fau_select ? 10 : 8;
            unsigned reg_frag_srt = I->fragment_srt_select ? 4 : 0;
   unsigned reg_frag_fau = 12;
            uint64_t position_srt = cs_get_u64(qctx, reg_position_srt);
   uint64_t vary_srt = cs_get_u64(qctx, reg_vary_srt);
            GENX(pandecode_resource_tables)(ctx, position_srt, "Position resources");
   GENX(pandecode_resource_tables)(ctx, vary_srt, "Varying resources");
            mali_ptr position_fau = cs_get_u64(qctx, reg_position_fau);
   mali_ptr vary_fau = cs_get_u64(qctx, reg_vary_fau);
            if (position_fau) {
      uint64_t lo = position_fau & BITFIELD64_MASK(48);
                        if (vary_fau) {
      uint64_t lo = vary_fau & BITFIELD64_MASK(48);
                        if (fragment_fau) {
      uint64_t lo = fragment_fau & BITFIELD64_MASK(48);
                        GENX(pandecode_shader)
            if (tiler_flags.secondary_shader) {
                           GENX(pandecode_shader)
            DUMP_ADDR(ctx, LOCAL_STORAGE, cs_get_u64(qctx, 24),
               DUMP_ADDR(ctx, LOCAL_STORAGE, cs_get_u64(qctx, 24),
               DUMP_ADDR(ctx, LOCAL_STORAGE, cs_get_u64(qctx, 30),
                  pandecode_log(ctx, "Global attribute offset: %u\n", cs_get_u32(qctx, 32));
   pandecode_log(ctx, "Index count: %u\n", cs_get_u32(qctx, 33));
            if (tiler_flags.index_type)
            pandecode_log(ctx, "Vertex offset: %d\n", cs_get_u32(qctx, 36));
   pandecode_log(ctx, "Instance offset: %u\n", cs_get_u32(qctx, 37));
            if (tiler_flags.index_type)
                     DUMP_CL(ctx, SCISSOR, &qctx->regs[42], "Scissor\n");
   pandecode_log(ctx, "Low depth clamp: %f\n", uif(cs_get_u32(qctx, 44)));
   pandecode_log(ctx, "High depth clamp: %f\n", uif(cs_get_u32(qctx, 45)));
            if (tiler_flags.secondary_shader)
            mali_ptr blend = cs_get_u64(qctx, 50);
                     if (tiler_flags.index_type)
            DUMP_UNPACKED(ctx, PRIMITIVE_FLAGS, tiler_flags, "Primitive flags\n");
   DUMP_CL(ctx, DCD_FLAGS_0, &qctx->regs[57], "DCD Flags 0\n");
   DUMP_CL(ctx, DCD_FLAGS_1, &qctx->regs[58], "DCD Flags 1\n");
               }
      static void
   pandecode_run_fragment(struct pandecode_context *ctx, struct queue_ctx *qctx,
         {
                        /* TODO: Tile enable map */
               }
      static void
   print_indirect(unsigned address, int16_t offset, FILE *fp)
   {
      if (offset)
         else
      }
      static void
   print_reg_tuple(unsigned base, uint16_t mask, FILE *fp)
   {
               u_foreach_bit(i, mask) {
      fprintf(fp, "%sr%u", first_reg ? "" : ":", base + i);
               if (mask == 0)
      }
      static void
   disassemble_ceu_instr(struct pandecode_context *ctx, uint64_t dword,
               {
      if (verbose) {
      fprintf(fp, " ");
   for (unsigned b = 0; b < 8; ++b)
               for (int i = 0; i < indent; ++i)
            /* Unpack the base so we get the opcode */
   uint8_t *bytes = (uint8_t *)&dword;
            switch (base.opcode) {
   case MALI_CEU_OPCODE_NOP: {
               if (I.ignored)
         else
                     case MALI_CEU_OPCODE_MOVE: {
               fprintf(fp, "MOVE d%u, #0x%" PRIX64 "\n", I.destination, I.immediate);
               case MALI_CEU_OPCODE_MOVE32: {
      pan_unpack(bytes, CEU_MOVE32, I);
   fprintf(fp, "MOVE32 r%u, #0x%X\n", I.destination, I.immediate);
               case MALI_CEU_OPCODE_WAIT: {
      bool first = true;
   pan_unpack(bytes, CEU_WAIT, I);
            u_foreach_bit(i, I.slots) {
      fprintf(fp, "%s%u", first ? "" : ",", i);
               fprintf(fp, "\n");
               case MALI_CEU_OPCODE_RUN_COMPUTE: {
      pan_unpack(bytes, CEU_RUN_COMPUTE, I);
   pandecode_run_compute(ctx, fp, qctx, &I);
               case MALI_CEU_OPCODE_RUN_IDVS: {
      pan_unpack(bytes, CEU_RUN_IDVS, I);
   pandecode_run_idvs(ctx, fp, qctx, &I);
               case MALI_CEU_OPCODE_RUN_FRAGMENT: {
      pan_unpack(bytes, CEU_RUN_FRAGMENT, I);
   fprintf(fp, "RUN_FRAGMENT%s\n",
         pandecode_run_fragment(ctx, qctx, &I);
               case MALI_CEU_OPCODE_ADD_IMMEDIATE32: {
               fprintf(fp, "ADD_IMMEDIATE32 r%u, r%u, #%d\n", I.destination, I.source,
                     case MALI_CEU_OPCODE_ADD_IMMEDIATE64: {
               fprintf(fp, "ADD_IMMEDIATE64 d%u, d%u, #%d\n", I.destination, I.source,
                     case MALI_CEU_OPCODE_LOAD_MULTIPLE: {
               fprintf(fp, "LOAD_MULTIPLE ");
   print_reg_tuple(I.base, I.mask, fp);
   fprintf(fp, ", ");
   print_indirect(I.address, I.offset, fp);
   fprintf(fp, "\n");
               case MALI_CEU_OPCODE_STORE_MULTIPLE: {
               fprintf(fp, "STORE_MULTIPLE ");
   print_indirect(I.address, I.offset, fp);
   fprintf(fp, ", ");
   print_reg_tuple(I.base, I.mask, fp);
   fprintf(fp, "\n");
               case MALI_CEU_OPCODE_SET_SB_ENTRY: {
               fprintf(fp, "SET_SB_ENTRY #%u, #%u\n", I.endpoint_entry, I.other_entry);
               case MALI_CEU_OPCODE_SYNC_ADD32: {
      pan_unpack(bytes, CEU_SYNC_ADD32, I);
   bool first = true;
   fprintf(fp, "SYNC_ADD32%s%s signal(%u), wait(",
                  u_foreach_bit(i, I.wait_mask) {
      fprintf(fp, "%s%u", first ? "" : ",", i);
               fprintf(fp, ") [d%u], r%u\n", I.address, I.data);
               case MALI_CEU_OPCODE_SYNC_ADD64: {
      pan_unpack(bytes, CEU_SYNC_ADD64, I);
   bool first = true;
   fprintf(fp, "SYNC_ADD64%s%s signal(%u), wait(",
                  u_foreach_bit(i, I.wait_mask) {
      fprintf(fp, "%s%u", first ? "" : ",", i);
               fprintf(fp, ") [d%u], d%u\n", I.address, I.data);
               case MALI_CEU_OPCODE_SYNC_SET32: {
      pan_unpack(bytes, CEU_SYNC_SET32, I);
   bool first = true;
   fprintf(fp, "SYNC_SET32.%s%s signal(%u), wait(",
                  u_foreach_bit(i, I.wait_mask) {
      fprintf(fp, "%s%u", first ? "" : ",", i);
               fprintf(fp, ") [d%u], r%u\n", I.address, I.data);
               case MALI_CEU_OPCODE_SYNC_SET64: {
      pan_unpack(bytes, CEU_SYNC_SET64, I);
   bool first = true;
   fprintf(fp, "SYNC_SET64.%s%s signal(%u), wait(",
                  u_foreach_bit(i, I.wait_mask) {
      fprintf(fp, "%s%u", first ? "" : ",", i);
               fprintf(fp, ") [d%u], d%u\n", I.address, I.data);
               case MALI_CEU_OPCODE_CALL: {
      pan_unpack(bytes, CEU_CALL, I);
   fprintf(fp, "CALL d%u, r%u\n", I.address, I.length);
               case MALI_CEU_OPCODE_JUMP: {
      pan_unpack(bytes, CEU_JUMP, I);
   fprintf(fp, "JUMP d%u, r%u\n", I.address, I.length);
               case MALI_CEU_OPCODE_REQ_RESOURCE: {
               fprintf(fp, "REQ_RESOURCE");
   if (I.compute)
         if (I.fragment)
         if (I.tiler)
         if (I.idvs)
         fprintf(fp, "\n");
               case MALI_CEU_OPCODE_SYNC_WAIT32: {
               fprintf(fp, "SYNC_WAIT32%s%s d%u, r%u\n", I.invert ? ".gt" : ".le",
                     case MALI_CEU_OPCODE_SYNC_WAIT64: {
               fprintf(fp, "SYNC_WAIT64%s%s d%u, d%u\n", I.invert ? ".gt" : ".le",
                     case MALI_CEU_OPCODE_UMIN32: {
               fprintf(fp, "UMIN32 r%u, r%u, r%u\n", I.destination, I.source_1,
                     case MALI_CEU_OPCODE_BRANCH: {
               static const char *condition[] = {
         };
   fprintf(fp, "BRANCH.%s r%u, #%d\n", condition[I.condition], I.value,
                        case MALI_CEU_OPCODE_FLUSH_CACHE2: {
      pan_unpack(bytes, CEU_FLUSH_CACHE2, I);
   static const char *mode[] = {
      "nop",
   "clean",
   "INVALID",
               fprintf(fp, "FLUSH_CACHE2.%s_l2.%s_lsc%s r%u, signal(%u), wait(",
         mode[I.l2_flush_mode], mode[I.lsc_flush_mode],
            bool first = true;
   u_foreach_bit(i, I.scoreboard_mask) {
      fprintf(fp, "%s%u", first ? "" : ",", i);
      }
   fprintf(fp, ")\n");
               case MALI_CEU_OPCODE_FINISH_TILING: {
      pan_unpack(bytes, CEU_FINISH_TILING, I);
   fprintf(fp, "FINISH_TILING\n");
               case MALI_CEU_OPCODE_FINISH_FRAGMENT: {
               bool first = true;
   fprintf(fp, "FINISH_FRAGMENT.%s, d%u, d%u, signal(%u), wait(",
                  u_foreach_bit(i, I.wait_mask) {
      fprintf(fp, "%s%u", first ? "" : ",", i);
      }
   fprintf(fp, ")\n");
               case MALI_CEU_OPCODE_HEAP_OPERATION: {
      pan_unpack(bytes, CEU_HEAP_OPERATION, I);
   const char *counter_names[] = {"vt_start", "vt_end", NULL, "frag_end"};
   bool first = true;
   fprintf(fp, "HEAP_OPERATION.%s signal(%u), wait(",
            u_foreach_bit(i, I.wait_mask) {
      fprintf(fp, "%s%u", first ? "" : ",", i);
               fprintf(fp, ")\n");
               case MALI_CEU_OPCODE_HEAP_SET: {
      pan_unpack(bytes, CEU_HEAP_SET, I);
   fprintf(fp, "HEAP_SET d%u\n", I.address);
               default: {
      fprintf(fp, "INVALID_%u 0x%" PRIX64 "\n", base.opcode, base.data);
      }
      }
      static bool
   interpret_ceu_jump(struct pandecode_context *ctx, struct queue_ctx *qctx,
         {
      uint32_t address_lo = qctx->regs[reg_address];
   uint32_t address_hi = qctx->regs[reg_address + 1];
            if (length % 8) {
      fprintf(stderr, "CS call alignment error\n");
               /* Map the entire subqueue now */
   uint64_t address = ((uint64_t)address_hi << 32) | address_lo;
            qctx->ip = cs;
            /* Skip the usual IP update */
      }
      /*
   * Interpret a single instruction of the CEU, updating the register file,
   * instruction pointer, and call stack. Memory access and GPU controls are
   * ignored for now.
   *
   * Returns true if execution should continue.
   */
   static bool
   interpret_ceu_instr(struct pandecode_context *ctx, struct queue_ctx *qctx)
   {
      /* Unpack the base so we get the opcode */
   uint8_t *bytes = (uint8_t *)qctx->ip;
                     switch (base.opcode) {
   case MALI_CEU_OPCODE_MOVE: {
               qctx->regs[I.destination + 0] = (uint32_t)I.immediate;
   qctx->regs[I.destination + 1] = (uint32_t)(I.immediate >> 32);
               case MALI_CEU_OPCODE_MOVE32: {
               qctx->regs[I.destination] = I.immediate;
               case MALI_CEU_OPCODE_ADD_IMMEDIATE32: {
               qctx->regs[I.destination] = qctx->regs[I.source] + I.immediate;
               case MALI_CEU_OPCODE_ADD_IMMEDIATE64: {
               int64_t value =
                  qctx->regs[I.destination] = value;
   qctx->regs[I.destination + 1] = value >> 32;
               case MALI_CEU_OPCODE_CALL: {
               if (qctx->call_stack_depth == MAX_CALL_STACK_DEPTH) {
      fprintf(stderr, "CS call stack overflow\n");
                                 /* Note: tail calls are not optimized in the hardware. */
                     qctx->call_stack[depth].lr = qctx->ip;
                        case MALI_CEU_OPCODE_JUMP: {
               if (qctx->call_stack_depth == 0) {
      fprintf(stderr, "Cannot jump from the entrypoint\n");
                           default:
                  /* Update IP first to point to the next instruction, so call doesn't
   * require special handling (even for tail calls).
   */
            while (qctx->ip == qctx->end) {
      /* Graceful termination */
   if (qctx->call_stack_depth == 0)
            /* Pop off the call stack */
            qctx->ip = qctx->call_stack[old_depth].lr;
                  }
      void
   GENX(pandecode_cs)(struct pandecode_context *ctx, mali_ptr queue, uint32_t size,
         {
                        /* Mali-G610 has 96 registers. Other devices not yet supported, we can make
   * this configurable later when we encounter new Malis.
   */
   struct queue_ctx qctx = {
      .nr_regs = 96,
   .regs = regs,
   .ip = cs,
   .end = cs + (size / 8),
               if (size) {
      do {
      disassemble_ceu_instr(ctx, *(qctx.ip), 1 + qctx.call_stack_depth, true,
                  fflush(ctx->dump_stream);
      }
   #endif
