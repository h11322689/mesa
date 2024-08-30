   /**************************************************************************
   *
   * Copyright 2003 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "util/u_math.h"
   #include "i915_context.h"
   #include "i915_fpc.h"
   #include "i915_reg.h"
      uint32_t
   i915_get_temp(struct i915_fp_compile *p)
   {
      int bit = ffs(~p->temp_flag);
   if (!bit) {
      i915_program_error(p, "i915_get_temp: out of temporaries");
               p->temp_flag |= 1 << (bit - 1);
      }
      static void
   i915_release_temp(struct i915_fp_compile *p, int reg)
   {
         }
      /**
   * Get unpreserved temporary, a temp whose value is not preserved between
   * PS program phases.
   */
   uint32_t
   i915_get_utemp(struct i915_fp_compile *p)
   {
      int bit = ffs(~p->utemp_flag);
   if (!bit) {
      i915_program_error(p, "i915_get_utemp: out of temporaries");
               p->utemp_flag |= 1 << (bit - 1);
      }
      void
   i915_release_utemps(struct i915_fp_compile *p)
   {
         }
      uint32_t
   i915_emit_decl(struct i915_fp_compile *p, uint32_t type, uint32_t nr,
         {
               if (type == REG_TYPE_T) {
      if (p->decl_t & (1 << nr))
               } else if (type == REG_TYPE_S) {
      if (p->decl_s & (1 << nr))
               } else
            if (p->decl < p->declarations + I915_PROGRAM_SIZE) {
      *(p->decl++) = (D0_DCL | D0_DEST(reg) | d0_flags);
   *(p->decl++) = D1_MBZ;
      } else
            p->nr_decl_insn++;
      }
      uint32_t
   i915_emit_arith(struct i915_fp_compile *p, uint32_t op, uint32_t dest,
               {
      uint32_t c[3];
            assert(GET_UREG_TYPE(dest) != REG_TYPE_CONST);
   dest = UREG(GET_UREG_TYPE(dest), GET_UREG_NR(dest));
            if (GET_UREG_TYPE(src0) == REG_TYPE_CONST)
         if (GET_UREG_TYPE(src1) == REG_TYPE_CONST)
         if (GET_UREG_TYPE(src2) == REG_TYPE_CONST)
            /* Recursively call this function to MOV additional const values
   * into temporary registers.  Use utemp registers for this -
   * currently shouldn't be possible to run out, but keep an eye on
   * this.
   */
   if (nr_const > 1) {
               s[0] = src0;
   s[1] = src1;
   s[2] = src2;
            first = GET_UREG_NR(s[c[0]]);
   for (i = 1; i < nr_const; i++) {
                        i915_emit_arith(p, A0_MOV, tmp, A0_DEST_CHANNEL_ALL, 0, s[c[i]], 0,
                        src0 = s[0];
   src1 = s[1];
   src2 = s[2];
               if (p->csr < p->program + I915_PROGRAM_SIZE) {
      *(p->csr++) = (op | A0_DEST(dest) | mask | saturate | A0_SRC0(src0));
   *(p->csr++) = (A1_SRC0(src0) | A1_SRC1(src1));
      } else
            if (GET_UREG_TYPE(dest) == REG_TYPE_R)
            p->nr_alu_insn++;
      }
      /**
   * Emit a texture load or texkill instruction.
   * \param dest  the dest i915 register
   * \param destmask  the dest register writemask
   * \param sampler  the i915 sampler register
   * \param coord  the i915 source texcoord operand
   * \param opcode  the instruction opcode
   */
   uint32_t
   i915_emit_texld(struct i915_fp_compile *p, uint32_t dest, uint32_t destmask,
               {
                        uint32_t coord_used = 0xf << UREG_CHANNEL_X_SHIFT;
   if (coord_mask & TGSI_WRITEMASK_Y)
         if (coord_mask & TGSI_WRITEMASK_Z)
         if (coord_mask & TGSI_WRITEMASK_W)
            if ((coord & coord_used) != (k & coord_used) ||
      GET_UREG_TYPE(coord) == REG_TYPE_CONST) {
   /* texcoord is swizzled or negated.  Need to allocate a new temporary
   * register (a utemp / unpreserved temp) won't do.
   */
            temp = i915_get_temp(p);          /* get temp reg index */
            i915_emit_arith(p, A0_MOV, tempReg,
                        /* new src texcoord is tempReg */
               /* Don't worry about saturate as we only support
   */
   if (destmask != A0_DEST_CHANNEL_ALL) {
      /* if not writing to XYZW... */
   uint32_t tmp = i915_get_utemp(p);
   i915_emit_texld(p, tmp, A0_DEST_CHANNEL_ALL, sampler, coord, opcode,
         i915_emit_arith(p, A0_MOV, dest, destmask, 0, tmp, 0, 0);
      } else {
      assert(GET_UREG_TYPE(dest) != REG_TYPE_CONST);
            /* Output register being oC or oD defines a phase boundary */
   if (GET_UREG_TYPE(dest) == REG_TYPE_OC ||
                  /* Reading from an r# register whose contents depend on output of the
   * current phase defines a phase boundary.
   */
   if (GET_UREG_TYPE(coord) == REG_TYPE_R &&
                  if (p->csr < p->program + I915_PROGRAM_SIZE) {
               *(p->csr++) = T1_ADDRESS_REG(coord);
      } else
            if (GET_UREG_TYPE(dest) == REG_TYPE_R)
                        if (temp >= 0)
               }
      uint32_t
   i915_emit_const1f(struct i915_fp_compile *p, float c0)
   {
      struct i915_fragment_shader *ifs = p->shader;
            if (c0 == 0.0)
         if (c0 == 1.0)
            for (reg = 0; reg < I915_MAX_CONSTANT; reg++) {
      if (ifs->constant_flags[reg] == I915_CONSTFLAG_USER)
         for (idx = 0; idx < 4; idx++) {
      if (!(ifs->constant_flags[reg] & (1 << idx)) ||
      ifs->constants[reg][idx] == c0) {
   ifs->constants[reg][idx] = c0;
   ifs->constant_flags[reg] |= 1 << idx;
   if (reg + 1 > ifs->num_constants)
                           i915_program_error(p, "i915_emit_const1f: out of constants");
      }
      uint32_t
   i915_emit_const2f(struct i915_fp_compile *p, float c0, float c1)
   {
      struct i915_fragment_shader *ifs = p->shader;
            if (c0 == 0.0)
         if (c0 == 1.0)
            if (c1 == 0.0)
         if (c1 == 1.0)
            // XXX emit swizzle here for 0, 1, -1 and any combination thereof
   // we can use swizzle + neg for that
   for (reg = 0; reg < I915_MAX_CONSTANT; reg++) {
      if (ifs->constant_flags[reg] == 0xf ||
      ifs->constant_flags[reg] == I915_CONSTFLAG_USER)
      for (idx = 0; idx < 3; idx++) {
      if (!(ifs->constant_flags[reg] & (3 << idx))) {
      ifs->constants[reg][idx + 0] = c0;
   ifs->constants[reg][idx + 1] = c1;
   ifs->constant_flags[reg] |= 3 << idx;
   if (reg + 1 > ifs->num_constants)
                           i915_program_error(p, "i915_emit_const2f: out of constants");
      }
      uint32_t
   i915_emit_const4f(struct i915_fp_compile *p, float c0, float c1, float c2,
         {
      struct i915_fragment_shader *ifs = p->shader;
            // XXX emit swizzle here for 0, 1, -1 and any combination thereof
   // we can use swizzle + neg for that
   for (reg = 0; reg < I915_MAX_CONSTANT; reg++) {
      if (ifs->constant_flags[reg] == 0xf && ifs->constants[reg][0] == c0 &&
      ifs->constants[reg][1] == c1 && ifs->constants[reg][2] == c2 &&
   ifs->constants[reg][3] == c3) {
                  ifs->constants[reg][0] = c0;
   ifs->constants[reg][1] = c1;
   ifs->constants[reg][2] = c2;
   ifs->constants[reg][3] = c3;
   ifs->constant_flags[reg] = 0xf;
   if (reg + 1 > ifs->num_constants)
                        i915_program_error(p, "i915_emit_const4f: out of constants");
      }
      uint32_t
   i915_emit_const4fv(struct i915_fp_compile *p, const float *c)
   {
         }
