   /*
   * Copyright © 2012 Intel Corporation
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
   *
   * Authors:
   *    Eric Anholt <eric@anholt.net>
   *
   */
      #include "brw_vec4.h"
   #include "brw_vec4_live_variables.h"
      using namespace brw;
      #define MAX_INSTRUCTION (1 << 30)
      /** @file brw_vec4_live_variables.cpp
   *
   * Support for computing at the basic block level which variables
   * (virtual GRFs in our case) are live at entry and exit.
   *
   * See Muchnick's Advanced Compiler Design and Implementation, section
   * 14.1 (p444).
   */
      /**
   * Sets up the use/def arrays and block-local approximation of the live ranges.
   *
   * The basic-block-level live variable analysis needs to know which
   * variables get used before they're completely defined, and which
   * variables are completely defined before they're used.
   *
   * We independently track each channel of a vec4.  This is because we need to
   * be able to recognize a sequence like:
   *
   * ...
   * DP4 tmp.x a b;
   * DP4 tmp.y c d;
   * MUL result.xy tmp.xy e.xy
   * ...
   *
   * as having tmp live only across that sequence (assuming it's used nowhere
   * else), because it's a common pattern.  A more conservative approach that
   * doesn't get tmp marked a deffed in this block will tend to result in
   * spilling.
   */
   void
   vec4_live_variables::setup_def_use()
   {
               foreach_block (block, cfg) {
      assert(ip == block->start_ip);
   assert(cfg->blocks[block->num - 1]->end_ip == ip - 1);
            foreach_inst_in_block(vec4_instruction, inst, block) {
            for (unsigned int i = 0; i < 3; i++) {
      if (inst->src[i].file == VGRF) {
                                                         if (!BITSET_TEST(bd->def, v))
      }
            for (unsigned c = 0; c < 4; c++) {
      if (inst->reads_flag(c) &&
      !BITSET_TEST(bd->flag_def, c)) {
                  /* Set up the instruction defs. */
   if (inst->dst.file == VGRF) {
      for (unsigned i = 0; i < DIV_ROUND_UP(inst->size_written, 16); i++) {
                                                /* Check for unconditional register writes, these are the
   * things that screen off preceding definitions of a
   * variable, and thus qualify for being in def[].
   */
   if ((!inst->predicate || inst->opcode == BRW_OPCODE_SEL) &&
      !BITSET_TEST(bd->use, v))
            }
   if (inst->writes_flag(devinfo)) {
      for (unsigned c = 0; c < 4; c++) {
      if ((inst->dst.writemask & (1 << c)) &&
      !BITSET_TEST(bd->flag_use, c)) {
            ip++;
               }
      /**
   * The algorithm incrementally sets bits in liveout and livein,
   * propagating it through control flow.  It will eventually terminate
   * because it only ever adds bits, and stops when no bits are added in
   * a pass.
   */
   void
   vec4_live_variables::compute_live_variables()
   {
               while (cont) {
               foreach_block_reverse (block, cfg) {
      /* Update liveout */
   foreach_list_typed(bblock_link, child_link, link, &block->children) {
               for (int i = 0; i < bitset_words; i++) {
               BITSET_WORD new_liveout = (child_bd->livein[i] &
         cont = true;
         }
            BITSET_WORD new_liveout = (child_bd->flag_livein[0] &
         if (new_liveout) {
         }
               /* Update livein */
   for (int i = 0; i < bitset_words; i++) {
      BITSET_WORD new_livein = (bd->use[i] |
               if (new_livein & ~bd->livein[i]) {
      bd->livein[i] |= new_livein;
         }
   BITSET_WORD new_livein = (bd->flag_use[0] |
               if (new_livein & ~bd->flag_livein[0]) {
      bd->flag_livein[0] |= new_livein;
               }
      /**
   * Extend the start/end ranges for each variable to account for the
   * new information calculated from control flow.
   */
   void
   vec4_live_variables::compute_start_end()
   {
      foreach_block (block, cfg) {
               for (int i = 0; i < num_vars; i++) {
      if (BITSET_TEST(bd.livein, i)) {
      start[i] = MIN2(start[i], block->start_ip);
               if (BITSET_TEST(bd.liveout, i)) {
      start[i] = MIN2(start[i], block->end_ip);
               }
      vec4_live_variables::vec4_live_variables(const backend_shader *s)
         {
               num_vars = alloc.total_size * 8;
   start = ralloc_array(mem_ctx, int, num_vars);
            for (int i = 0; i < num_vars; i++) {
      start[i] = MAX_INSTRUCTION;
                                 bitset_words = BITSET_WORDS(num_vars);
   for (int i = 0; i < cfg->num_blocks; i++) {
      block_data[i].def = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
   block_data[i].use = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
   block_data[i].livein = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
            block_data[i].flag_def[0] = 0;
   block_data[i].flag_use[0] = 0;
   block_data[i].flag_livein[0] = 0;
               setup_def_use();
   compute_live_variables();
      }
      vec4_live_variables::~vec4_live_variables()
   {
         }
      static bool
   check_register_live_range(const vec4_live_variables *live, int ip,
         {
      for (unsigned j = 0; j < n; j += 4) {
      if (var + j >= unsigned(live->num_vars) ||
      live->start[var + j] > ip || live->end[var + j] < ip)
               }
      bool
   vec4_live_variables::validate(const backend_shader *s) const
   {
               foreach_block_and_inst(block, vec4_instruction, inst, s->cfg) {
      for (unsigned c = 0; c < 4; c++) {
      if (inst->dst.writemask & (1 << c)) {
      for (unsigned i = 0; i < 3; i++) {
      if (inst->src[i].file == VGRF &&
      !check_register_live_range(this, ip,
                        if (inst->dst.file == VGRF &&
      !check_register_live_range(this, ip,
                                          }
      int
   vec4_live_variables::var_range_start(unsigned v, unsigned n) const
   {
               for (unsigned i = 0; i < n; i++)
               }
      int
   vec4_live_variables::var_range_end(unsigned v, unsigned n) const
   {
               for (unsigned i = 0; i < n; i++)
               }
      bool
   vec4_live_variables::vgrfs_interfere(int a, int b) const
   {
      return !((var_range_end(8 * alloc.offsets[a], 8 * alloc.sizes[a]) <=
            var_range_start(8 * alloc.offsets[b], 8 * alloc.sizes[b])) ||
   }
