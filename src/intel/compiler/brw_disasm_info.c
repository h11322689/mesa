   /*
   * Copyright © 2014 Intel Corporation
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
      #include "brw_cfg.h"
   #include "brw_eu.h"
   #include "brw_disasm_info.h"
   #include "dev/intel_debug.h"
   #include "compiler/nir/nir.h"
      __attribute__((weak)) void nir_print_instr(UNUSED const nir_instr *instr,
            void
   dump_assembly(void *assembly, int start_offset, int end_offset,
         {
      const struct brw_isa_info *isa = disasm->isa;
   const char *last_annotation_string = NULL;
            void *mem_ctx = ralloc_context(NULL);
   const struct brw_label *root_label =
            foreach_list_typed(struct inst_group, group, link, &disasm->group_list) {
      struct exec_node *next_node = exec_node_get_next(&group->link);
   if (exec_node_is_tail_sentinel(next_node))
            struct inst_group *next =
            int start_offset = group->offset;
            if (group->block_start) {
      fprintf(stderr, "   START B%d", group->block_start->num);
   foreach_list_typed(struct bblock_link, predecessor_link, link,
            struct bblock_t *predecessor_block = predecessor_link->block;
      }
   if (block_latency)
      fprintf(stderr, " (%u cycles)",
                  if (last_annotation_ir != group->ir) {
      last_annotation_ir = group->ir;
   if (last_annotation_ir) {
      fprintf(stderr, "   ");
   nir_print_instr(group->ir, stderr);
                  if (last_annotation_string != group->annotation) {
      last_annotation_string = group->annotation;
   if (last_annotation_string)
               brw_disassemble(isa, assembly, start_offset, end_offset,
            if (group->error) {
                  if (group->block_end) {
      fprintf(stderr, "   END B%d", group->block_end->num);
   foreach_list_typed(struct bblock_link, successor_link, link,
            struct bblock_t *successor_block = successor_link->block;
      }
         }
               }
      struct disasm_info *
   disasm_initialize(const struct brw_isa_info *isa,
         {
      struct disasm_info *disasm = ralloc(NULL, struct disasm_info);
   exec_list_make_empty(&disasm->group_list);
   disasm->isa = isa;
   disasm->cfg = cfg;
   disasm->cur_block = 0;
   disasm->use_tail = false;
      }
      struct inst_group *
   disasm_new_inst_group(struct disasm_info *disasm, unsigned next_inst_offset)
   {
      struct inst_group *tail = rzalloc(disasm, struct inst_group);
   tail->offset = next_inst_offset;
   exec_list_push_tail(&disasm->group_list, &tail->link);
      }
      void
   disasm_annotate(struct disasm_info *disasm,
         {
      const struct intel_device_info *devinfo = disasm->isa->devinfo;
            struct inst_group *group;
   if (!disasm->use_tail) {
         } else {
      disasm->use_tail = false;
   group = exec_node_data(struct inst_group,
               if (INTEL_DEBUG(DEBUG_ANNOTATION)) {
      group->ir = inst->ir;
               if (bblock_start(cfg->blocks[disasm->cur_block]) == inst) {
                  /* There is no hardware DO instruction on Gfx6+, so since DO always
   * starts a basic block, we need to set the .block_start of the next
   * instruction's annotation with a pointer to the bblock started by
   * the DO.
   *
   * There's also only complication from emitting an annotation without
   * a corresponding hardware instruction to disassemble.
   */
   if (devinfo->ver >= 6 && inst->opcode == BRW_OPCODE_DO) {
                  if (bblock_end(cfg->blocks[disasm->cur_block]) == inst) {
      group->block_end = cfg->blocks[disasm->cur_block];
         }
      void
   disasm_insert_error(struct disasm_info *disasm, unsigned offset,
         {
      foreach_list_typed(struct inst_group, cur, link, &disasm->group_list) {
      struct exec_node *next_node = exec_node_get_next(&cur->link);
   if (exec_node_is_tail_sentinel(next_node))
            struct inst_group *next =
            if (next->offset <= offset)
            if (offset + inst_size != next->offset) {
                     cur->error = NULL;
                                             if (cur->error)
         else
               }
