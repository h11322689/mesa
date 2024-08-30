   /*
   * Copyright © 2022 Intel Corporation
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
      #include "nir.h"
      static nir_alu_type
   nir_alu_src_type(const nir_alu_instr *instr, unsigned src)
   {
      return nir_alu_type_get_base_type(nir_op_infos[instr->op].input_types[src]) |
      }
      static nir_scalar
   nir_alu_arg(const nir_alu_instr *alu, unsigned arg, unsigned comp)
   {
      const nir_alu_src *src = &alu->src[arg];
      }
      /* Tries to determine the value of expression "val % div", assuming that val
   * is interpreted as value of type "val_type". "div" must be a power of two.
   * Returns true if it can statically tell the value of "val % div", false if not.
   * Value of *mod is undefined if this function returned false.
   *
   * Tests are in mod_analysis_tests.cpp.
   */
   bool
   nir_mod_analysis(nir_scalar val, nir_alu_type val_type, unsigned div, unsigned *mod)
   {
      if (div == 1) {
      *mod = 0;
                        switch (val.def->parent_instr->type) {
   case nir_instr_type_load_const: {
      nir_load_const_instr *load =
                  if (base_type == nir_type_uint) {
      assert(val.comp < load->def.num_components);
   uint64_t ival = nir_const_value_as_uint(load->value[val.comp],
         *mod = ival % div;
      } else if (base_type == nir_type_int) {
      assert(val.comp < load->def.num_components);
                  /* whole analysis collapses the moment we allow negative values */
                  *mod = ((uint64_t)ival) % div;
                           case nir_instr_type_alu: {
               if (alu->def.num_components != 1)
            switch (alu->op) {
   case nir_op_ishr: {
      if (nir_src_is_const(alu->src[1].src)) {
                                    nir_alu_type type0 = nir_alu_src_type(alu, 0);
                  *mod >>= shift;
      }
               case nir_op_iadd: {
      unsigned mod0;
   nir_alu_type type0 = nir_alu_src_type(alu, 0);
                  unsigned mod1;
   nir_alu_type type1 = nir_alu_src_type(alu, 1);
                  *mod = (mod0 + mod1) % div;
               case nir_op_ishl: {
      if (nir_src_is_const(alu->src[1].src)) {
                     if ((div >> shift) == 0) {
      *mod = 0;
      }
   nir_alu_type type0 = nir_alu_src_type(alu, 0);
      }
               case nir_op_imul_32x16: /* multiply 32-bits with low 16-bits */
   case nir_op_imul: {
      unsigned mod0;
                  if (s1 && (mod0 == 0)) {
      *mod = 0;
               /* if divider is larger than 2nd source max (interpreted) value
   * then modulo of multiplication is unknown
   */
                  unsigned mod1;
                  if (s2 && (mod1 == 0)) {
      *mod = 0;
                              *mod = (mod0 * mod1) % div;
               default:
         }
               default:
                     }
