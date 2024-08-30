   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "rogue.h"
   #include "util/macros.h"
   #include "util/ralloc.h"
   #include "util/register_allocate.h"
      #include <stdbool.h>
   #include <stdlib.h>
      /**
   * \file rogue_regalloc.c
   *
   * \brief Contains the rogue_regalloc pass.
   */
      /* TODO: Internal register support for high register pressure regs. */
      typedef struct rogue_live_range {
      unsigned start;
      } rogue_live_range;
      static void rogue_regarray_liveness(rogue_regarray *regarray,
         {
      assert(list_is_singular(&regarray->writes) ||
         if (!list_is_empty(&regarray->writes)) {
      rogue_regarray_write *write =
                     rogue_foreach_regarray_use (use, regarray) {
            }
      static void rogue_reg_liveness(rogue_reg *reg, rogue_live_range *live_range)
   {
      assert(list_is_singular(&reg->writes) || list_is_empty(&reg->writes));
   if (!list_is_empty(&reg->writes)) {
      rogue_reg_write *write =
                     rogue_foreach_reg_use (use, reg) {
            }
      PUBLIC
   bool rogue_regalloc(rogue_shader *shader)
   {
      if (shader->is_grouped)
                     unsigned num_ssa_regs = rogue_count_used_regs(shader, ROGUE_REG_CLASS_SSA);
   if (!num_ssa_regs)
            assert(list_is_empty(&shader->regs[ROGUE_REG_CLASS_TEMP]));
            /* Setup regset and register classes. */
            for (enum rogue_regalloc_class c = 0; c < ROGUE_REGALLOC_CLASS_COUNT; ++c) {
      ASSERTED struct ra_class *ra_class =
                  for (unsigned t = 0; t < hw_temps; ++t)
      if (!(t % regalloc_info[c].stride))
                     /* TODO: Consider tracking this in the shader itself, i.e. one list for child
   * regarrays, one for parents. Or, since children are already in a list in
   * the parent, only have parent regarrays in the shader.
            /* Count the parent regarrays. */
   unsigned num_parent_regarrays = 0;
   rogue_foreach_regarray (regarray, shader) {
      if (regarray->parent || regarray->regs[0]->class != ROGUE_REG_CLASS_SSA)
                        /* Construct list of parent regarrays. */
   rogue_regarray **parent_regarrays =
      rzalloc_array_size(ra_regs,
               unsigned ra = 0;
   rogue_foreach_regarray (regarray, shader) {
      if (regarray->parent || regarray->regs[0]->class != ROGUE_REG_CLASS_SSA)
                        /* Prepare live ranges. */
   rogue_live_range *ssa_live_range =
         for (unsigned u = 0; u < num_ssa_regs; ++u)
            /* Populate live ranges for register arrays. */
   for (unsigned u = 0; u < num_parent_regarrays; ++u) {
      rogue_regarray *regarray = parent_regarrays[u];
   unsigned base_index = regarray->regs[0]->index;
                     rogue_foreach_subarray (subarray, regarray) {
                     /* Populate live ranges for registers. */
   rogue_foreach_reg (reg, shader, ROGUE_REG_CLASS_SSA) {
      if (reg->regarray)
            rogue_live_range *live_range = &ssa_live_range[reg->index];
               struct ra_graph *ra_graph =
                  /* Set register class for regarrays/vectors. */
   for (unsigned u = 0; u < num_parent_regarrays; ++u) {
      rogue_regarray *regarray = parent_regarrays[u];
                     if (regarray->size == 2)
         else if (regarray->size == 4)
         else
            ra_set_node_class(ra_graph,
                     /* Set register class for "standalone" registers. */
   rogue_foreach_reg (reg, shader, ROGUE_REG_CLASS_SSA) {
      if (reg->regarray)
            ra_set_node_class(ra_graph,
                           /* Build interference graph from overlapping live ranges. */
   for (unsigned index0 = 0; index0 < num_ssa_regs; ++index0) {
               for (unsigned index1 = 0; index1 < num_ssa_regs; ++index1) {
                              /* If the live ranges overlap, those register nodes interfere. */
   if (!(live_range0->start >= live_range1->end ||
                        /* TODO: Spilling support. */
   if (!ra_allocate(ra_graph))
            /* Replace SSA regarray registers with allocated physical registers. */
   for (unsigned u = 0; u < num_parent_regarrays; ++u) {
               unsigned base_index = regarray->regs[0]->index;
   unsigned hw_base_index = ra_get_node_reg(ra_graph, base_index);
   enum rogue_regalloc_class ra_class =
                  bool used = false;
   for (unsigned r = 0; r < regarray->size; ++r) {
               if (used)
               /* First time using new regarray, modify in place. */
   if (!used) {
      progress |=
      } else {
               /* Replace parent regarray first. */
   rogue_regarray *new_regarray = rogue_regarray_cached(shader,
                                    /* Replace remaining standalone SSA registers with allocated physical
   * registers. */
   rogue_foreach_reg_safe (reg, shader, ROGUE_REG_CLASS_SSA) {
      assert(!reg->regarray);
            enum rogue_regalloc_class ra_class =
                  /* First time using new register, modify in place. */
   if (!rogue_reg_is_used(shader, new_class, hw_index)) {
         } else {
      /* Register has already been used, replace references and delete. */
   assert(list_is_singular(&reg->writes)); /* SSA reg. */
   rogue_reg *new_reg = rogue_temp_reg(shader, hw_index);
               #ifndef NDEBUG
      /* Ensure that temp regs are continuous from zero, and have no gaps. */
   unsigned num_temp_regs = list_length(&shader->regs[ROGUE_REG_CLASS_TEMP]);
   rogue_foreach_reg (reg, shader, ROGUE_REG_CLASS_TEMP) {
            #endif /* NDEBUG */
         ralloc_free(ra_regs);
      }
