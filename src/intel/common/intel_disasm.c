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
      #include <stdlib.h>
      #include "compiler/brw_inst.h"
   #include "compiler/brw_eu.h"
   #include "compiler/brw_isa_info.h"
      #include "intel_disasm.h"
      static bool
   is_send(uint32_t opcode)
   {
      return (opcode == BRW_OPCODE_SEND  ||
         opcode == BRW_OPCODE_SENDC ||
      }
      int
   intel_disassemble_find_end(const struct brw_isa_info *isa,
         {
      const struct intel_device_info *devinfo = isa->devinfo;
            /* This loop exits when send-with-EOT or when opcode is 0 */
   while (true) {
               if (brw_inst_cmpt_control(devinfo, insn)) {
         } else {
                  /* Simplistic, but efficient way to terminate disasm */
   uint32_t opcode = brw_inst_opcode(isa, insn);
   if (opcode == 0 || (is_send(opcode) && brw_inst_eot(devinfo, insn))) {
                        }
      void
   intel_disassemble(const struct brw_isa_info *isa,
         {
               /* Make a dummy disasm structure that brw_validate_instructions
   * can work from.
   */
   struct disasm_info *disasm_info = disasm_initialize(isa, NULL);
   disasm_new_inst_group(disasm_info, start);
                     void *mem_ctx = ralloc_context(NULL);
   const struct brw_label *root_label =
            foreach_list_typed(struct inst_group, group, link,
            struct exec_node *next_node = exec_node_get_next(&group->link);
   if (exec_node_is_tail_sentinel(next_node))
            struct inst_group *next =
            int start_offset = group->offset;
            brw_disassemble(isa, assembly, start_offset, end_offset,
            if (group->error) {
                     ralloc_free(mem_ctx);
      }
