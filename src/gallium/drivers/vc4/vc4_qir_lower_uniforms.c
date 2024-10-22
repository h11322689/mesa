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
      /**
   * @file vc4_qir_lower_uniforms.c
   *
   * This is the pre-code-generation pass for fixing up instructions that try to
   * read from multiple uniform values.
   */
      #include "vc4_qir.h"
   #include "util/hash_table.h"
   #include "util/u_math.h"
      static inline uint32_t
   index_hash(const void *key)
   {
         }
      static inline bool
   index_compare(const void *a, const void *b)
   {
         }
      static void
   add_uniform(struct hash_table *ht, struct qreg reg)
   {
         struct hash_entry *entry;
            entry = _mesa_hash_table_search(ht, key);
   if (entry) {
         } else {
         }
      static void
   remove_uniform(struct hash_table *ht, struct qreg reg)
   {
         struct hash_entry *entry;
            entry = _mesa_hash_table_search(ht, key);
   assert(entry);
   entry->data = (void *)(((uintptr_t) entry->data) - 1);
   if (entry->data == NULL)
   }
      static bool
   is_lowerable_uniform(struct qinst *inst, int i)
   {
         if (inst->src[i].file != QFILE_UNIF)
         if (qir_is_tex(inst))
         }
      /* Returns the number of different uniform values referenced by the
   * instruction.
   */
   static uint32_t
   qir_get_instruction_uniform_count(struct qinst *inst)
   {
                  for (int i = 0; i < qir_get_nsrc(inst); i++) {
                           bool is_duplicate = false;
   for (int j = 0; j < i; j++) {
            if (inst->src[j].file == QFILE_UNIF &&
      inst->src[j].index == inst->src[i].index) {
         }
               }
      void
   qir_lower_uniforms(struct vc4_compile *c)
   {
         struct hash_table *ht =
            /* Walk the instruction list, finding which instructions have more
      * than one uniform referenced, and add those uniform values to the
   * ht.
      qir_for_each_inst_inorder(inst, c) {
                                    for (int i = 0; i < nsrc; i++) {
                     while (ht->entries) {
            /* Find the most commonly used uniform in instructions that
   * need a uniform lowered.
   */
   uint32_t max_count = 0;
   uint32_t max_index = 0;
   hash_table_foreach(ht, entry) {
            uint32_t count = (uintptr_t)entry->data;
   uint32_t index = (uintptr_t)entry->key - 1;
   if (count > max_count) {
                              /* Now, find the instructions using this uniform and make them
   * reference a temp instead.
                                                                              /* If the block doesn't have a load of the
      * uniform yet, add it.  We could potentially
   * do better and CSE MOVs from multiple blocks
   * into dominating blocks, except that may
   * cause troubles for register allocation.
      if (!mov) {
                                          bool removed = false;
   for (int i = 0; i < nsrc; i++) {
            if (is_lowerable_uniform(inst, i) &&
      inst->src[i].index == max_index) {
                                    /* If the instruction doesn't need lowering any more,
      * then drop it from the list.
      if (count <= 1) {
            for (int i = 0; i < nsrc; i++) {
               }
