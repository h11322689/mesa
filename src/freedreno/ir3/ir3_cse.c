   /*
   * Copyright (C) 2014 Valve Corporation
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
      #include "ir3.h"
      #define XXH_INLINE_ALL
   #include "util/xxhash.h"
      /* This pass handles CSE'ing repeated expressions created in the process of
   * translating from NIR. Currently this is just collect's. Also, currently
   * this is intra-block only, to make it work over multiple block we'd need to
   * bring forward dominance calculation.
   */
      #define HASH(hash, data) XXH32(&(data), sizeof(data), hash)
      static uint32_t
   hash_instr(const void *data)
   {
      const struct ir3_instruction *instr = data;
            hash = HASH(hash, instr->opc);
   hash = HASH(hash, instr->dsts[0]->flags);
   foreach_src (src, (struct ir3_instruction *)instr) {
      if (src->flags & IR3_REG_CONST) {
      if (src->flags & IR3_REG_RELATIV)
         else
      } else if (src->flags & IR3_REG_IMMED) {
         } else {
      if (src->flags & IR3_REG_ARRAY)
                        if (opc_cat(instr->opc) == 1) {
      hash = HASH(hash, instr->cat1.dst_type);
   hash = HASH(hash, instr->cat1.src_type);
                  }
      static bool
   instrs_equal(const struct ir3_instruction *i1, const struct ir3_instruction *i2)
   {
      if (i1->opc != i2->opc)
            if (i1->dsts_count != i2->dsts_count)
            if (i1->srcs_count != i2->srcs_count)
            if (i1->dsts[0]->flags != i2->dsts[0]->flags)
            for (unsigned i = 0; i < i1->srcs_count; i++) {
               if (i1_reg->flags != i2_reg->flags)
            if (i1_reg->flags & IR3_REG_CONST) {
      if (i1_reg->flags & IR3_REG_RELATIV) {
      if (i1_reg->array.offset != i2_reg->array.offset)
      } else {
      if (i1_reg->num != i2_reg->num)
         } else if (i1_reg->flags & IR3_REG_IMMED) {
      if (i1_reg->uim_val != i2_reg->uim_val)
      } else {
      if (i1_reg->flags & IR3_REG_ARRAY) {
      if (i1_reg->array.offset != i2_reg->array.offset)
      }
   if (i1_reg->def != i2_reg->def)
                  if (opc_cat(i1->opc) == 1) {
      if (i1->cat1.dst_type != i2->cat1.dst_type ||
      i1->cat1.src_type != i2->cat1.src_type ||
   i1->cat1.round != i2->cat1.round)
               }
      static bool
   instr_can_cse(const struct ir3_instruction *instr)
   {
      if (instr->opc != OPC_META_COLLECT && instr->opc != OPC_MOV)
            if (!is_dest_gpr(instr->dsts[0]) || (instr->dsts[0]->flags & IR3_REG_ARRAY))
               }
      static bool
   cmp_func(const void *data1, const void *data2)
   {
         }
      bool
   ir3_cse(struct ir3 *ir)
   {
      struct set *instr_set = _mesa_set_create(NULL, hash_instr, cmp_func);
   foreach_block (block, &ir->block_list) {
               foreach_instr (instr, &block->instr_list) {
                              bool found;
   struct set_entry *entry =
         if (found)
                  bool progress = false;
   foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      foreach_src (src, instr) {
      if ((src->flags & IR3_REG_SSA) && src->def &&
      src->def->instr->data) {
   progress = true;
   struct ir3_instruction *instr = src->def->instr->data;
                        _mesa_set_destroy(instr_set, NULL);
      }
