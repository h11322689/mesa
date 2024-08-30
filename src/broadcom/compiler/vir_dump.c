   /*
   * Copyright © 2016-2017 Broadcom
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
      #include "broadcom/common/v3d_device_info.h"
   #include "v3d_compiler.h"
      /* Prints a human-readable description of the uniform reference. */
   void
   vir_dump_uniform(enum quniform_contents contents,
         {
         static const char *quniform_names[] = {
            [QUNIFORM_LINE_WIDTH] = "line_width",
   [QUNIFORM_AA_LINE_WIDTH] = "aa_line_width",
   [QUNIFORM_VIEWPORT_X_SCALE] = "vp_x_scale",
   [QUNIFORM_VIEWPORT_Y_SCALE] = "vp_y_scale",
   [QUNIFORM_VIEWPORT_Z_OFFSET] = "vp_z_offset",
               switch (contents) {
   case QUNIFORM_CONSTANT:
                  case QUNIFORM_UNIFORM:
                  case QUNIFORM_TEXTURE_CONFIG_P1:
                  case QUNIFORM_TMU_CONFIG_P0:
            fprintf(stderr, "tex[%d].p0 | 0x%x",
               case QUNIFORM_TMU_CONFIG_P1:
            fprintf(stderr, "tex[%d].p1 | 0x%x",
               case QUNIFORM_IMAGE_TMU_CONFIG_P0:
            fprintf(stderr, "img[%d].p0 | 0x%x",
               case QUNIFORM_TEXTURE_WIDTH:
               case QUNIFORM_TEXTURE_HEIGHT:
               case QUNIFORM_TEXTURE_DEPTH:
               case QUNIFORM_TEXTURE_ARRAY_SIZE:
               case QUNIFORM_TEXTURE_LEVELS:
                  case QUNIFORM_IMAGE_WIDTH:
               case QUNIFORM_IMAGE_HEIGHT:
               case QUNIFORM_IMAGE_DEPTH:
               case QUNIFORM_IMAGE_ARRAY_SIZE:
                  case QUNIFORM_SPILL_OFFSET:
                  case QUNIFORM_SPILL_SIZE_PER_THREAD:
                  case QUNIFORM_UBO_ADDR:
            fprintf(stderr, "ubo[%d]+0x%x",
               case QUNIFORM_SSBO_OFFSET:
                  case QUNIFORM_GET_SSBO_SIZE:
                  case QUNIFORM_GET_UBO_SIZE:
                  case QUNIFORM_NUM_WORK_GROUPS:
                  default:
            if (quniform_contents_is_texture_p0(contents)) {
            fprintf(stderr, "tex[%d].p0: 0x%08x",
      } else if (contents < ARRAY_SIZE(quniform_names) &&
                     } else {
      }
      static void
   vir_print_reg(struct v3d_compile *c, const struct qinst *inst,
         {
                  case QFILE_NULL:
                  case QFILE_LOAD_IMM:
                  case QFILE_REG:
                  case QFILE_MAGIC:
                        case QFILE_SMALL_IMM: {
            uint32_t unpacked;
   bool ok = v3d_qpu_small_imm_unpack(c->devinfo,
                        int8_t *p = (int8_t *)&inst->qpu.raddr_b;
   if (*p >= -16 && *p <= 15)
         else
               case QFILE_VPM:
                        case QFILE_TEMP:
               }
      static void
   vir_dump_sig_addr(const struct v3d_device_info *devinfo,
         {
         if (devinfo->ver < 41)
            if (!instr->sig_magic)
         else {
            const char *name =
         if (name)
            }
      static void
   vir_dump_sig(struct v3d_compile *c, struct qinst *inst)
   {
                  if (sig->thrsw)
         if (sig->ldvary) {
               }
   if (sig->ldvpm)
         if (sig->ldtmu) {
               }
   if (sig->ldtlb) {
               }
   if (sig->ldtlbu) {
               }
   if (sig->ldunif)
         if (sig->ldunifrf) {
               }
   if (sig->ldunifa)
         if (sig->ldunifarf) {
               }
   if (sig->wrtmuc)
   }
      static void
   vir_dump_alu(struct v3d_compile *c, struct qinst *inst)
   {
         struct v3d_qpu_instr *instr = &inst->qpu;
   int nsrc = vir_get_nsrc(inst);
            if (inst->qpu.alu.add.op != V3D_QPU_A_NOP) {
            fprintf(stderr, "%s", v3d_qpu_add_op_name(instr->alu.add.op));
   fprintf(stderr, "%s", v3d_qpu_cond_name(instr->flags.ac));
                                          } else {
            fprintf(stderr, "%s", v3d_qpu_mul_op_name(instr->alu.mul.op));
   fprintf(stderr, "%s", v3d_qpu_cond_name(instr->flags.mc));
                                                   for (int i = 0; i < nsrc; i++) {
            fprintf(stderr, ", ");
               }
      void
   vir_dump_inst(struct v3d_compile *c, struct qinst *inst)
   {
                  switch (inst->qpu.type) {
   case V3D_QPU_INSTR_TYPE_ALU:
               case V3D_QPU_INSTR_TYPE_BRANCH:
                                                      switch (instr->branch.bdi) {
                                                                  case V3D_QPU_BRANCH_DEST_REGFILE:
                        if (instr->branch.ub) {
                                                                                 case V3D_QPU_BRANCH_DEST_REGFILE:
                        if (vir_has_uniform(inst)) {
            fprintf(stderr, " (");
   vir_dump_uniform(c->uniform_contents[inst->uniform],
      }
      void
   vir_dump(struct v3d_compile *c)
   {
         int ip = 0;
            vir_for_each_block(block, c) {
            fprintf(stderr, "BLOCK %d:\n", block->index);
   vir_for_each_inst(inst, block) {
            if (c->live_intervals_valid) {
                                                                                    if (first) {
         } else {
                                                                                                                     if (first) {
                                                                  vir_dump_inst(c, inst);
      }
   if (block->successors[1]) {
            fprintf(stderr, "-> BLOCK %d, %d\n",
      } else if (block->successors[0]) {
            }
