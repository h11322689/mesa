   /*
   * Copyright 2021 Alyssa Rosenzweig
   * Copyright 2019-2020 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "agx_compiler.h"
      static void
   agx_print_sized(char prefix, unsigned value, enum agx_size size, FILE *fp)
   {
      switch (size) {
   case AGX_SIZE_16:
      fprintf(fp, "%c%u%c", prefix, value >> 1, (value & 1) ? 'h' : 'l');
      case AGX_SIZE_32:
      assert((value & 1) == 0);
   fprintf(fp, "%c%u", prefix, value >> 1);
      case AGX_SIZE_64:
      assert((value & 1) == 0);
   fprintf(fp, "%c%u:%c%u", prefix, value >> 1, prefix, (value >> 1) + 1);
                  }
      static void
   agx_print_index(agx_index index, bool is_float, FILE *fp)
   {
      switch (index.type) {
   case AGX_INDEX_NULL:
      fprintf(fp, "_");
         case AGX_INDEX_NORMAL:
      if (index.cache)
            if (index.discard)
            if (index.kill)
            fprintf(fp, "%u", index.value);
         case AGX_INDEX_IMMEDIATE:
      if (is_float) {
      assert(index.value < 0x100);
      } else {
                        case AGX_INDEX_UNDEF:
      fprintf(fp, "undef");
         case AGX_INDEX_UNIFORM:
      agx_print_sized('u', index.value, index.size, fp);
         case AGX_INDEX_REGISTER:
      agx_print_sized('r', index.value, index.size, fp);
         default:
                  /* Print length suffixes if not implied */
   if (index.type == AGX_INDEX_NORMAL) {
      if (index.size == AGX_SIZE_16)
         else if (index.size == AGX_SIZE_64)
               if (index.abs)
            if (index.neg)
      }
      void
   agx_print_instr(const agx_instr *I, FILE *fp)
   {
      assert(I->op < AGX_NUM_OPCODES);
   struct agx_opcode_info info = agx_opcodes_info[I->op];
                     agx_foreach_dest(I, d) {
      if (print_comma)
         else
                        if (I->nr_dests) {
      fprintf(fp, " = ");
                        if (I->saturate)
            if (I->last)
                     agx_foreach_src(I, s) {
      if (print_comma)
         else
            agx_print_index(I->src[s],
                           if (I->mask) {
               for (unsigned i = 0; i < 4; ++i) {
      if (I->mask & (1 << i))
                  /* TODO: Do better for enums, truth tables, etc */
   if (info.immediates) {
      if (print_comma)
         else
                        if (info.immediates & AGX_IMMEDIATE_DIM) {
      if (print_comma)
         else
                        if (info.immediates & AGX_IMMEDIATE_SCOREBOARD) {
      if (print_comma)
         else
                        if (info.immediates & AGX_IMMEDIATE_NEST) {
      if (print_comma)
         else
                        if ((info.immediates & AGX_IMMEDIATE_INVERT_COND) && I->invert_cond) {
      if (print_comma)
         else
                           }
      void
   agx_print_block(const agx_block *block, FILE *fp)
   {
               agx_foreach_instr_in_block(block, ins)
                     if (block->successors[0]) {
               agx_foreach_successor(block, succ)
               if (block->predecessors.size) {
               agx_foreach_predecessor(block, pred)
                  }
      void
   agx_print_shader(const agx_context *ctx, FILE *fp)
   {
      agx_foreach_block(ctx, block)
      }
