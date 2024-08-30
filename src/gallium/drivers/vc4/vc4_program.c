   /*
   * Copyright (c) 2014 Scott Mansell
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
      #include <inttypes.h>
   #include "util/format/u_format.h"
   #include "util/crc32.h"
   #include "util/u_helpers.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/ralloc.h"
   #include "util/hash_table.h"
   #include "tgsi/tgsi_dump.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir_types.h"
   #include "nir/tgsi_to_nir.h"
   #include "vc4_context.h"
   #include "vc4_qpu.h"
   #include "vc4_qir.h"
      static struct qreg
   ntq_get_src(struct vc4_compile *c, nir_src src, int i);
   static void
   ntq_emit_cf_list(struct vc4_compile *c, struct exec_list *list);
      static struct vc4_compiled_shader *
   vc4_get_compiled_shader(struct vc4_context *vc4, enum qstage stage,
            static int
   type_size(const struct glsl_type *type, bool bindless)
   {
         }
      static void
   resize_qreg_array(struct vc4_compile *c,
                     {
         if (*size >= decl_size)
            uint32_t old_size = *size;
   *size = MAX2(*size * 2, decl_size);
   *regs = reralloc(c, *regs, struct qreg, *size);
   if (!*regs) {
                        for (uint32_t i = old_size; i < *size; i++)
   }
      static void
   ntq_emit_thrsw(struct vc4_compile *c)
   {
         if (!c->fs_threaded)
            /* Always thread switch after each texture operation for now.
      *
   * We could do better by batching a bunch of texture fetches up and
   * then doing one thread switch and collecting all their results
   * afterward.
      qir_emit_nondef(c, qir_inst(QOP_THRSW, c->undef,
         }
      static struct qreg
   indirect_uniform_load(struct vc4_compile *c, nir_intrinsic_instr *intr)
   {
                  /* Clamp to [0, array size).  Note that MIN/MAX are signed. */
   uint32_t range = nir_intrinsic_range(intr);
   indirect_offset = qir_MAX(c, indirect_offset, qir_uniform_ui(c, 0));
   indirect_offset = qir_MIN_NOIMM(c, indirect_offset,
            qir_ADD_dest(c, qir_reg(QFILE_TEX_S_DIRECT, 0),
                                          }
      static struct qreg
   vc4_ubo_load(struct vc4_compile *c, nir_intrinsic_instr *intr)
   {
         ASSERTED int buffer_index = nir_src_as_uint(intr->src[0]);
   assert(buffer_index == 1);
                     /* Clamp to [0, array size).  Note that MIN/MAX are signed. */
   offset = qir_MAX(c, offset, qir_uniform_ui(c, 0));
   offset = qir_MIN_NOIMM(c, offset,
            qir_ADD_dest(c, qir_reg(QFILE_TEX_S_DIRECT, 0),
                                    }
      nir_def *
   vc4_nir_get_swizzled_channel(nir_builder *b, nir_def **srcs, int swiz)
   {
         switch (swiz) {
   default:
   case PIPE_SWIZZLE_NONE:
               case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
         case PIPE_SWIZZLE_X:
   case PIPE_SWIZZLE_Y:
   case PIPE_SWIZZLE_Z:
   case PIPE_SWIZZLE_W:
         }
      static struct qreg *
   ntq_init_ssa_def(struct vc4_compile *c, nir_def *def)
   {
         struct qreg *qregs = ralloc_array(c->def_ht, struct qreg,
         _mesa_hash_table_insert(c->def_ht, def, qregs);
   }
      /**
   * This function is responsible for getting QIR results into the associated
   * storage for a NIR instruction.
   *
   * If it's a NIR SSA def, then we just set the associated hash table entry to
   * the new result.
   *
   * If it's a NIR reg, then we need to update the existing qreg assigned to the
   * NIR destination with the incoming value.  To do that without introducing
   * new MOVs, we require that the incoming qreg either be a uniform, or be
   * SSA-defined by the previous QIR instruction in the block and rewritable by
   * this function.  That lets us sneak ahead and insert the SF flag beforehand
   * (knowing that the previous instruction doesn't depend on flags) and rewrite
   * its destination to be the NIR reg's destination
   */
   static void
   ntq_store_def(struct vc4_compile *c, nir_def *def, int chan,
         {
         struct qinst *last_inst = NULL;
   if (!list_is_empty(&c->cur_block->instructions))
            assert(result.file == QFILE_UNIF ||
                  nir_intrinsic_instr *store = nir_store_reg_for_def(def);
   if (store == NULL) {
                                          if (entry)
                     } else {
            nir_def *reg = store->src[1].ssa;
   ASSERTED nir_intrinsic_instr *decl = nir_reg_get_decl(reg);
   assert(nir_intrinsic_base(store) == 0);
   assert(nir_intrinsic_num_array_elems(decl) == 0);
                        /* Insert a MOV if the source wasn't an SSA def in the
   * previous instruction.
   */
   if (result.file == QFILE_UNIF) {
                                             /* If we're in control flow, then make this update of the reg
   * conditional on the execution mask.
                                 /* Set the flags to the current exec mask.  To insert
   * the SF, we temporarily remove our SSA instruction.
   */
                              }
      static struct qreg
   ntq_get_src(struct vc4_compile *c, nir_src src, int i)
   {
                  nir_intrinsic_instr *load = nir_load_reg_for_def(src.ssa);
   if (load == NULL) {
               } else {
            nir_def *reg = load->src[0].ssa;
   ASSERTED nir_intrinsic_instr *decl = nir_reg_get_decl(reg);
   assert(nir_intrinsic_base(load) == 0);
   assert(nir_intrinsic_num_array_elems(decl) == 0);
               struct qreg *qregs = entry->data;
   }
      static struct qreg
   ntq_get_alu_src(struct vc4_compile *c, nir_alu_instr *instr,
         {
         struct qreg r = ntq_get_src(c, instr->src[src].src,
            };
      static inline struct qreg
   qir_SAT(struct vc4_compile *c, struct qreg val)
   {
         return qir_FMAX(c,
         }
      static struct qreg
   ntq_rcp(struct vc4_compile *c, struct qreg x)
   {
                  /* Apply a Newton-Raphson step to improve the accuracy. */
   r = qir_FMUL(c, r, qir_FSUB(c,
                  }
      static struct qreg
   ntq_rsq(struct vc4_compile *c, struct qreg x)
   {
                  /* Apply a Newton-Raphson step to improve the accuracy. */
   r = qir_FMUL(c, r, qir_FSUB(c,
                                    }
      static struct qreg
   ntq_umul(struct vc4_compile *c, struct qreg src0, struct qreg src1)
   {
         struct qreg src0_hi = qir_SHR(c, src0,
         struct qreg src1_hi = qir_SHR(c, src1,
            struct qreg hilo = qir_MUL24(c, src0_hi, src1);
   struct qreg lohi = qir_MUL24(c, src0, src1_hi);
            return qir_ADD(c, lolo, qir_SHL(c,
         }
      static struct qreg
   ntq_scale_depth_texture(struct vc4_compile *c, struct qreg src)
   {
         struct qreg depthf = qir_ITOF(c, qir_SHR(c, src,
         }
      /**
   * Emits a lowered TXF_MS from an MSAA texture.
   *
   * The addressing math has been lowered in NIR, and now we just need to read
   * it like a UBO.
   */
   static void
   ntq_emit_txf(struct vc4_compile *c, nir_tex_instr *instr)
   {
         uint32_t tile_width = 32;
   uint32_t tile_height = 32;
   uint32_t tile_size = (tile_height * tile_width *
            unsigned unit = instr->texture_index;
   uint32_t w = align(c->key->tex[unit].msaa_width, tile_width);
   uint32_t w_tiles = w / tile_width;
   uint32_t h = align(c->key->tex[unit].msaa_height, tile_height);
   uint32_t h_tiles = h / tile_height;
            struct qreg addr;
   assert(instr->num_srcs == 1);
   assert(instr->src[0].src_type == nir_tex_src_coord);
            /* Perform the clamping required by kernel validation. */
   addr = qir_MAX(c, addr, qir_uniform_ui(c, 0));
            qir_ADD_dest(c, qir_reg(QFILE_TEX_S_DIRECT, 0),
                     struct qreg tex = qir_TEX_RESULT(c);
            enum pipe_format format = c->key->tex[unit].format;
   if (util_format_is_depth_or_stencil(format)) {
            struct qreg scaled = ntq_scale_depth_texture(c, tex);
      } else {
            for (int i = 0; i < 4; i++)
      }
      static void
   ntq_emit_tex(struct vc4_compile *c, nir_tex_instr *instr)
   {
         struct qreg s, t, r, lod, compare;
   bool is_txb = false, is_txl = false;
            if (instr->op == nir_texop_txf) {
                        for (unsigned i = 0; i < instr->num_srcs; i++) {
            switch (instr->src[i].src_type) {
   case nir_tex_src_coord:
            s = ntq_get_src(c, instr->src[i].src, 0);
   if (instr->sampler_dim == GLSL_SAMPLER_DIM_1D)
         else
         if (instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE)
      case nir_tex_src_bias:
            lod = ntq_get_src(c, instr->src[i].src, 0);
      case nir_tex_src_lod:
            lod = ntq_get_src(c, instr->src[i].src, 0);
      case nir_tex_src_comparator:
               default:
               if (c->stage != QSTAGE_FRAG && !is_txl) {
            /* From the GLSL 1.20 spec:
   *
   *     "If it is mip-mapped and running on the vertex shader,
   *      then the base texture is used."
   */
               if (c->key->tex[unit].force_first_level) {
            lod = qir_uniform(c, QUNIFORM_TEXTURE_FIRST_LEVEL, unit);
               struct qreg texture_u[] = {
            qir_uniform(c, QUNIFORM_TEXTURE_CONFIG_P0, unit),
   qir_uniform(c, QUNIFORM_TEXTURE_CONFIG_P1, unit),
      };
            if (instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE || is_txl) {
                        struct qinst *tmu;
   if (instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
            tmu = qir_MOV_dest(c, qir_reg(QFILE_TEX_R, 0), r);
      } else if (c->key->tex[unit].wrap_s == PIPE_TEX_WRAP_CLAMP_TO_BORDER ||
               c->key->tex[unit].wrap_s == PIPE_TEX_WRAP_CLAMP ||
   c->key->tex[unit].wrap_t == PIPE_TEX_WRAP_CLAMP_TO_BORDER ||
      tmu = qir_MOV_dest(c, qir_reg(QFILE_TEX_R, 0),
                           if (c->key->tex[unit].wrap_s == PIPE_TEX_WRAP_CLAMP) {
                  if (c->key->tex[unit].wrap_t == PIPE_TEX_WRAP_CLAMP) {
                  tmu = qir_MOV_dest(c, qir_reg(QFILE_TEX_T, 0), t);
   tmu->src[qir_get_tex_uniform_src(tmu)] =
            if (is_txl || is_txb) {
            tmu = qir_MOV_dest(c, qir_reg(QFILE_TEX_B, 0), lod);
               tmu = qir_MOV_dest(c, qir_reg(QFILE_TEX_S, 0), s);
                                                if (util_format_is_depth_or_stencil(format)) {
                           struct qreg u0 = qir_uniform_f(c, 0.0f);
   struct qreg u1 = qir_uniform_f(c, 1.0f);
   if (c->key->tex[unit].compare_mode) {
            /* From the GL_ARB_shadow spec:
   *
   *     "Let Dt (D subscript t) be the depth texture
   *      value, in the range [0, 1].  Let R be the
                              switch (c->key->tex[unit].compare_func) {
   case PIPE_FUNC_NEVER:
         depth_output = qir_uniform_f(c, 0.0f);
   case PIPE_FUNC_ALWAYS:
         depth_output = u1;
   case PIPE_FUNC_EQUAL:
         qir_SF(c, qir_FSUB(c, compare, normalized));
   depth_output = qir_SEL(c, QPU_COND_ZS, u1, u0);
   case PIPE_FUNC_NOTEQUAL:
         qir_SF(c, qir_FSUB(c, compare, normalized));
   depth_output = qir_SEL(c, QPU_COND_ZC, u1, u0);
   case PIPE_FUNC_GREATER:
         qir_SF(c, qir_FSUB(c, compare, normalized));
   depth_output = qir_SEL(c, QPU_COND_NC, u1, u0);
   case PIPE_FUNC_GEQUAL:
         qir_SF(c, qir_FSUB(c, normalized, compare));
   depth_output = qir_SEL(c, QPU_COND_NS, u1, u0);
   case PIPE_FUNC_LESS:
         qir_SF(c, qir_FSUB(c, compare, normalized));
   depth_output = qir_SEL(c, QPU_COND_NS, u1, u0);
   case PIPE_FUNC_LEQUAL:
         qir_SF(c, qir_FSUB(c, normalized, compare));
                           for (int i = 0; i < 4; i++)
      } else {
            for (int i = 0; i < 4; i++)
      }
      /**
   * Computes x - floor(x), which is tricky because our FTOI truncates (rounds
   * to zero).
   */
   static struct qreg
   ntq_ffract(struct vc4_compile *c, struct qreg src)
   {
         struct qreg trunc = qir_ITOF(c, qir_FTOI(c, src));
   struct qreg diff = qir_FSUB(c, src, trunc);
            qir_FADD_dest(c, diff,
            }
      /**
   * Computes floor(x), which is tricky because our FTOI truncates (rounds to
   * zero).
   */
   static struct qreg
   ntq_ffloor(struct vc4_compile *c, struct qreg src)
   {
                  /* This will be < 0 if we truncated and the truncation was of a value
      * that was < 0 in the first place.
               struct qinst *sub = qir_FSUB_dest(c, result,
                  }
      /**
   * Computes ceil(x), which is tricky because our FTOI truncates (rounds to
   * zero).
   */
   static struct qreg
   ntq_fceil(struct vc4_compile *c, struct qreg src)
   {
                  /* This will be < 0 if we truncated and the truncation was of a value
      * that was > 0 in the first place.
               qir_FADD_dest(c, result,
            }
      static struct qreg
   ntq_shrink_sincos_input_range(struct vc4_compile *c, struct qreg x)
   {
         /* Since we're using a Taylor approximation, we want to have a small
      * number of coefficients and take advantage of sin/cos repeating
   * every 2pi.  We keep our x as close to 0 as we can, since the series
   * will be less accurate as |x| increases.  (Also, be careful of
   * shifting the input x value to be tricky with sin/cos relations,
   * because getting accurate values for x==0 is very important for SDL
   * rendering)
      struct qreg scaled_x =
               /* Note: FTOI truncates toward 0. */
   struct qreg x_frac = qir_FSUB(c, scaled_x,
         /* Map [0.5, 1] to [-0.5, 0] */
   qir_SF(c, qir_FSUB(c, x_frac, qir_uniform_f(c, 0.5)));
   qir_FSUB_dest(c, x_frac, x_frac, qir_uniform_f(c, 1.0))->cond = QPU_COND_NC;
   /* Map [-1, -0.5] to [0, 0.5] */
   qir_SF(c, qir_FADD(c, x_frac, qir_uniform_f(c, 0.5)));
            }
      static struct qreg
   ntq_fsin(struct vc4_compile *c, struct qreg src)
   {
         float coeff[] = {
            2.0 * M_PI,
   -pow(2.0 * M_PI, 3) / (3 * 2 * 1),
   pow(2.0 * M_PI, 5) / (5 * 4 * 3 * 2 * 1),
               struct qreg x = ntq_shrink_sincos_input_range(c, src);
   struct qreg x2 = qir_FMUL(c, x, x);
   struct qreg sum = qir_FMUL(c, x, qir_uniform_f(c, coeff[0]));
   for (int i = 1; i < ARRAY_SIZE(coeff); i++) {
            x = qir_FMUL(c, x, x2);
   sum = qir_FADD(c,
                  }
   }
      static struct qreg
   ntq_fcos(struct vc4_compile *c, struct qreg src)
   {
         float coeff[] = {
            1.0f,
   -pow(2.0 * M_PI, 2) / (2 * 1),
   pow(2.0 * M_PI, 4) / (4 * 3 * 2 * 1),
   -pow(2.0 * M_PI, 6) / (6 * 5 * 4 * 3 * 2 * 1),
               struct qreg x_frac = ntq_shrink_sincos_input_range(c, src);
   struct qreg sum = qir_uniform_f(c, coeff[0]);
   struct qreg x2 = qir_FMUL(c, x_frac, x_frac);
   struct qreg x = x2; /* Current x^2, x^4, or x^6 */
   for (int i = 1; i < ARRAY_SIZE(coeff); i++) {
                           sum = qir_FADD(c, qir_FMUL(c,
            }
   }
      static struct qreg
   ntq_fsign(struct vc4_compile *c, struct qreg src)
   {
                  qir_SF(c, src);
   qir_MOV_dest(c, t, qir_uniform_f(c, 0.0));
   qir_MOV_dest(c, t, qir_uniform_f(c, 1.0))->cond = QPU_COND_ZC;
   qir_MOV_dest(c, t, qir_uniform_f(c, -1.0))->cond = QPU_COND_NS;
   }
      static void
   emit_vertex_input(struct vc4_compile *c, int attr)
   {
         enum pipe_format format = c->vs_key->attr_formats[attr];
            c->vattr_sizes[attr] = align(attr_size, 4);
   for (int i = 0; i < align(attr_size, 4) / 4; i++) {
            c->inputs[attr * 4 + i] =
      }
      static void
   emit_fragcoord_input(struct vc4_compile *c, int attr)
   {
         c->inputs[attr * 4 + 0] = qir_ITOF(c, qir_reg(QFILE_FRAG_X, 0));
   c->inputs[attr * 4 + 1] = qir_ITOF(c, qir_reg(QFILE_FRAG_Y, 0));
   c->inputs[attr * 4 + 2] =
            qir_FMUL(c,
      }
      static struct qreg
   emit_fragment_varying(struct vc4_compile *c, gl_varying_slot slot,
         {
         uint32_t i = c->num_input_slots++;
   struct qreg vary = {
                        if (c->num_input_slots >= c->input_slots_array_size) {
                           c->input_slots = reralloc(c, c->input_slots,
               c->input_slots[i].slot = slot;
            }
      static void
   emit_fragment_input(struct vc4_compile *c, int attr, gl_varying_slot slot)
   {
         for (int i = 0; i < 4; i++) {
            c->inputs[attr * 4 + i] =
      }
      static void
   add_output(struct vc4_compile *c,
            uint32_t decl_offset,
      {
         uint32_t old_array_size = c->outputs_array_size;
   resize_qreg_array(c, &c->outputs, &c->outputs_array_size,
            if (old_array_size != c->outputs_array_size) {
            c->output_slots = reralloc(c,
                     c->output_slots[decl_offset].slot = slot;
   }
      static bool
   ntq_src_is_only_ssa_def_user(nir_src *src)
   {
         return list_is_singular(&src->ssa->uses) &&
   }
      /**
   * In general, emits a nir_pack_unorm_4x8 as a series of MOVs with the pack
   * bit set.
   *
   * However, as an optimization, it tries to find the instructions generating
   * the sources to be packed and just emit the pack flag there, if possible.
   */
   static void
   ntq_emit_pack_unorm_4x8(struct vc4_compile *c, nir_alu_instr *instr)
   {
         struct qreg result = qir_get_temp(c);
            /* If packing from a vec4 op (as expected), identify it so that we can
      * peek back at what generated its sources.
      if (instr->src[0].src.ssa->parent_instr->type == nir_instr_type_alu &&
         nir_instr_as_alu(instr->src[0].src.ssa->parent_instr)->op ==
   nir_op_vec4) {
            /* If the pack is replicating the same channel 4 times, use the 8888
      * pack flag.  This is common for blending using the alpha
   * channel.
      if (instr->src[0].swizzle[0] == instr->src[0].swizzle[1] &&
         instr->src[0].swizzle[0] == instr->src[0].swizzle[2] &&
   instr->src[0].swizzle[0] == instr->src[0].swizzle[3]) {
      struct qreg rep = ntq_get_src(c,
                           for (int i = 0; i < 4; i++) {
            int swiz = instr->src[0].swizzle[i];
   struct qreg src;
   if (vec4) {
                                    if (vec4 &&
      ntq_src_is_only_ssa_def_user(&vec4->src[swiz].src) &&
   src.file == QFILE_TEMP &&
   c->defs[src.index] &&
   qir_is_mul(c->defs[src.index]) &&
   !c->defs[src.index]->dst.pack) {
         struct qinst *rewrite = c->defs[src.index];
   c->defs[src.index] = NULL;
                           }
      /** Handles sign-extended bitfield extracts for 16 bits. */
   static struct qreg
   ntq_emit_ibfe(struct vc4_compile *c, struct qreg base, struct qreg offset,
         {
         assert(bits.file == QFILE_UNIF &&
                  assert(offset.file == QFILE_UNIF &&
         int offset_bit = c->uniform_data[offset.index];
            }
      /** Handles unsigned bitfield extracts for 8 bits. */
   static struct qreg
   ntq_emit_ubfe(struct vc4_compile *c, struct qreg base, struct qreg offset,
         {
         assert(bits.file == QFILE_UNIF &&
                  assert(offset.file == QFILE_UNIF &&
         int offset_bit = c->uniform_data[offset.index];
            }
      /**
   * If compare_instr is a valid comparison instruction, emits the
   * compare_instr's comparison and returns the sel_instr's return value based
   * on the compare_instr's result.
   */
   static bool
   ntq_emit_comparison(struct vc4_compile *c, struct qreg *dest,
               {
                  switch (compare_instr->op) {
   case nir_op_feq32:
   case nir_op_ieq32:
   case nir_op_seq:
               case nir_op_fneu32:
   case nir_op_ine32:
   case nir_op_sne:
               case nir_op_fge32:
   case nir_op_ige32:
   case nir_op_uge32:
   case nir_op_sge:
               case nir_op_flt32:
   case nir_op_ilt32:
   case nir_op_slt:
               default:
                  struct qreg src0 = ntq_get_alu_src(c, compare_instr, 0);
            unsigned unsized_type =
         if (unsized_type == nir_type_float)
         else
            switch (sel_instr->op) {
   case nir_op_seq:
   case nir_op_sne:
   case nir_op_sge:
   case nir_op_slt:
                        case nir_op_b32csel:
            *dest = qir_SEL(c, cond,
               default:
            *dest = qir_SEL(c, cond,
               /* Make the temporary for nir_store_def(). */
            }
      /**
   * Attempts to fold a comparison generating a boolean result into the
   * condition code for selecting between two values, instead of comparing the
   * boolean result against 0 to generate the condition code.
   */
   static struct qreg ntq_emit_bcsel(struct vc4_compile *c, nir_alu_instr *instr,
         {
         if (nir_load_reg_for_def(instr->src[0].src.ssa))
         if (instr->src[0].src.ssa->parent_instr->type != nir_instr_type_alu)
         nir_alu_instr *compare =
         if (!compare)
            struct qreg dest;
   if (ntq_emit_comparison(c, &dest, compare, instr))
      out:
         qir_SF(c, src[0]);
   }
      static struct qreg
   ntq_fddx(struct vc4_compile *c, struct qreg src)
   {
         /* Make sure that we have a bare temp to use for MUL rotation, so it
      * can be allocated to an accumulator.
      if (src.pack || src.file != QFILE_TEMP)
            struct qreg from_left = qir_ROT_MUL(c, src, 1);
            /* Distinguish left/right pixels of the quad. */
   qir_SF(c, qir_AND(c, qir_reg(QFILE_QPU_ELEMENT, 0),
            return qir_MOV(c, qir_SEL(c, QPU_COND_ZS,
         }
      static struct qreg
   ntq_fddy(struct vc4_compile *c, struct qreg src)
   {
         if (src.pack || src.file != QFILE_TEMP)
            struct qreg from_bottom = qir_ROT_MUL(c, src, 2);
            /* Distinguish top/bottom pixels of the quad. */
   qir_SF(c, qir_AND(c,
                  return qir_MOV(c, qir_SEL(c, QPU_COND_ZS,
         }
      static struct qreg
   ntq_emit_cond_to_int(struct vc4_compile *c, enum qpu_cond cond)
   {
         return qir_MOV(c, qir_SEL(c, cond,
         }
      static void
   ntq_emit_alu(struct vc4_compile *c, nir_alu_instr *instr)
   {
         /* Vectors are special in that they have non-scalarized writemasks,
      * and just take the first swizzle channel for each argument in order
   * into each writemask channel.
      if (instr->op == nir_op_vec2 ||
         instr->op == nir_op_vec3 ||
   instr->op == nir_op_vec4) {
      struct qreg srcs[4];
   for (int i = 0; i < nir_op_infos[instr->op].num_inputs; i++)
               for (int i = 0; i < nir_op_infos[instr->op].num_inputs; i++)
                     if (instr->op == nir_op_pack_unorm_4x8) {
                        if (instr->op == nir_op_unpack_unorm_4x8) {
            struct qreg src = ntq_get_src(c, instr->src[0].src,
         unsigned count = instr->def.num_components;
   for (int i = 0; i < count; i++) {
                           /* General case: We can just grab the one used channel per src. */
   struct qreg src[nir_op_infos[instr->op].num_inputs];
   for (int i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
                           switch (instr->op) {
   case nir_op_mov:
               case nir_op_fmul:
               case nir_op_fadd:
               case nir_op_fsub:
               case nir_op_fmin:
               case nir_op_fmax:
                  case nir_op_f2i32:
   case nir_op_f2u32:
               case nir_op_i2f32:
   case nir_op_u2f32:
               case nir_op_b2f32:
               case nir_op_b2i32:
                  case nir_op_iadd:
               case nir_op_ushr:
               case nir_op_isub:
               case nir_op_ishr:
               case nir_op_ishl:
               case nir_op_imin:
               case nir_op_imax:
               case nir_op_iand:
               case nir_op_ior:
               case nir_op_ixor:
               case nir_op_inot:
                  case nir_op_imul:
                  case nir_op_seq:
   case nir_op_sne:
   case nir_op_sge:
   case nir_op_slt:
   case nir_op_feq32:
   case nir_op_fneu32:
   case nir_op_fge32:
   case nir_op_flt32:
   case nir_op_ieq32:
   case nir_op_ine32:
   case nir_op_ige32:
   case nir_op_uge32:
   case nir_op_ilt32:
            if (!ntq_emit_comparison(c, &result, instr, instr)) {
               case nir_op_b32csel:
               case nir_op_fcsel:
                        case nir_op_frcp:
               case nir_op_frsq:
               case nir_op_fexp2:
               case nir_op_flog2:
                  case nir_op_ftrunc:
               case nir_op_fceil:
               case nir_op_ffract:
               case nir_op_ffloor:
                  case nir_op_fsin:
               case nir_op_fcos:
                  case nir_op_fsign:
                  case nir_op_fabs:
               case nir_op_iabs:
                        case nir_op_ibitfield_extract:
                  case nir_op_ubitfield_extract:
                  case nir_op_usadd_4x8_vc4:
                  case nir_op_ussub_4x8_vc4:
                  case nir_op_umin_4x8_vc4:
                  case nir_op_umax_4x8_vc4:
                  case nir_op_umul_unorm_4x8_vc4:
                  case nir_op_fddx:
   case nir_op_fddx_coarse:
   case nir_op_fddx_fine:
                  case nir_op_fddy:
   case nir_op_fddy_coarse:
   case nir_op_fddy_fine:
                  case nir_op_uadd_carry:
                        case nir_op_usub_borrow:
                        default:
            fprintf(stderr, "unknown NIR ALU inst: ");
   nir_print_instr(&instr->instr, stderr);
               }
      static void
   emit_frag_end(struct vc4_compile *c)
   {
         struct qreg color;
   if (c->output_color_index != -1) {
         } else {
                  uint32_t discard_cond = QPU_COND_ALWAYS;
   if (c->s->info.fs.uses_discard) {
                        if (c->fs_key->stencil_enabled) {
            qir_MOV_dest(c, qir_reg(QFILE_TLB_STENCIL_SETUP, 0),
         if (c->fs_key->stencil_twoside) {
               }
   if (c->fs_key->stencil_full_writemasks) {
                     if (c->output_sample_mask_index != -1) {
                  if (c->fs_key->depth_enabled) {
            if (c->output_position_index != -1) {
            qir_FTOI_dest(c, qir_reg(QFILE_TLB_Z_WRITE, 0),
            } else {
                     if (!c->msaa_per_sample_output) {
               } else {
            for (int i = 0; i < VC4_MAX_SAMPLES; i++) {
            }
      static void
   emit_scaled_viewport_write(struct vc4_compile *c, struct qreg rcp_w)
   {
                  for (int i = 0; i < 2; i++) {
                                          qir_FTOI_dest(c, packed_chan,
                  qir_FMUL(c,
            }
      static void
   emit_zs_write(struct vc4_compile *c, struct qreg rcp_w)
   {
         struct qreg zscale = qir_uniform(c, QUNIFORM_VIEWPORT_Z_SCALE, 0);
            qir_VPM_WRITE(c, qir_FADD(c, qir_FMUL(c, qir_FMUL(c,
                     }
      static void
   emit_rcp_wc_write(struct vc4_compile *c, struct qreg rcp_w)
   {
         }
      static void
   emit_point_size_write(struct vc4_compile *c)
   {
                  if (c->output_point_size_index != -1)
         else
            }
      /**
   * Emits a VPM read of the stub vertex attribute set up by vc4_draw.c.
   *
   * The simulator insists that there be at least one vertex attribute, so
   * vc4_draw.c will emit one if it wouldn't have otherwise.  The simulator also
   * insists that all vertex attributes loaded get read by the VS/CS, so we have
   * to consume it here.
   */
   static void
   emit_stub_vpm_read(struct vc4_compile *c)
   {
         if (c->num_inputs)
            c->vattr_sizes[0] = 4;
   (void)qir_MOV(c, qir_reg(QFILE_VPM, 0));
   }
      static void
   emit_vert_end(struct vc4_compile *c,
               {
                           emit_scaled_viewport_write(c, rcp_w);
   emit_zs_write(c, rcp_w);
   emit_rcp_wc_write(c, rcp_w);
   if (c->vs_key->per_vertex_point_size)
            for (int i = 0; i < num_fs_inputs; i++) {
                                                         if (input->slot == output->slot &&
      input->swizzle == output->swizzle) {
         }
   /* Emit padding if we didn't find a declared VS output for
   * this FS input.
   */
      }
      static void
   emit_coord_end(struct vc4_compile *c)
   {
                           for (int i = 0; i < 4; i++)
            emit_scaled_viewport_write(c, rcp_w);
   emit_zs_write(c, rcp_w);
   emit_rcp_wc_write(c, rcp_w);
   if (c->vs_key->per_vertex_point_size)
   }
      static void
   vc4_optimize_nir(struct nir_shader *s)
   {
         bool progress;
   unsigned lower_flrp =
                        do {
                     NIR_PASS_V(s, nir_lower_vars_to_ssa);
   NIR_PASS(progress, s, nir_lower_alu_to_scalar, NULL, NULL);
   NIR_PASS(progress, s, nir_lower_phis_to_scalar, false);
   NIR_PASS(progress, s, nir_copy_prop);
   NIR_PASS(progress, s, nir_opt_remove_phis);
   NIR_PASS(progress, s, nir_opt_dce);
   NIR_PASS(progress, s, nir_opt_dead_cf);
   NIR_PASS(progress, s, nir_opt_cse);
   NIR_PASS(progress, s, nir_opt_peephole_select, 8, true, true);
   NIR_PASS(progress, s, nir_opt_algebraic);
                                 NIR_PASS(lower_flrp_progress, s, nir_lower_flrp,
                                          /* Nothing should rematerialize any flrps, so we only
                        }
      static int
   driver_location_compare(const void *in_a, const void *in_b)
   {
         const nir_variable *const *a = in_a;
            }
      static void
   ntq_setup_inputs(struct vc4_compile *c)
   {
         unsigned num_entries = 0;
   nir_foreach_shader_in_variable(var, c->s)
                     unsigned i = 0;
   nir_foreach_shader_in_variable(var, c->s)
            /* Sort the variables so that we emit the input setup in
      * driver_location order.  This is required for VPM reads, whose data
   * is fetched into the VPM in driver_location (TGSI register index)
   * order.
               for (unsigned i = 0; i < num_entries; i++) {
                                 assert(array_len == 1);
                        if (c->stage == QSTAGE_FRAG) {
            if (var->data.location == VARYING_SLOT_POS) {
         } else if (util_varying_is_point_coord(var->data.location,
               c->inputs[loc * 4 + 0] = c->point_x;
   } else {
      } else {
      }
      static void
   ntq_setup_outputs(struct vc4_compile *c)
   {
         nir_foreach_shader_out_variable(var, c->s) {
                                                         if (c->stage == QSTAGE_FRAG) {
            switch (var->data.location) {
   case FRAG_RESULT_COLOR:
   case FRAG_RESULT_DATA0:
         c->output_color_index = loc;
   case FRAG_RESULT_DEPTH:
         c->output_position_index = loc;
   case FRAG_RESULT_SAMPLE_MASK:
            } else {
            switch (var->data.location) {
   case VARYING_SLOT_POS:
         c->output_position_index = loc;
   case VARYING_SLOT_PSIZ:
         }
      /**
   * Sets up the mapping from nir_register to struct qreg *.
   *
   * Each nir_register gets a struct qreg per 32-bit component being stored.
   */
   static void
   ntq_setup_registers(struct vc4_compile *c, nir_function_impl *impl)
   {
         nir_foreach_reg_decl(decl, impl) {
            unsigned num_components = nir_intrinsic_num_components(decl);
   unsigned array_len = nir_intrinsic_num_array_elems(decl);
                                          }
      static void
   ntq_emit_load_const(struct vc4_compile *c, nir_load_const_instr *instr)
   {
         struct qreg *qregs = ntq_init_ssa_def(c, &instr->def);
   for (int i = 0; i < instr->def.num_components; i++)
            }
      static void
   ntq_emit_ssa_undef(struct vc4_compile *c, nir_undef_instr *instr)
   {
                  /* QIR needs there to be *some* value, so pick 0 (same as for
      * ntq_setup_registers().
      for (int i = 0; i < instr->def.num_components; i++)
   }
      static void
   ntq_emit_color_read(struct vc4_compile *c, nir_intrinsic_instr *instr)
   {
                  /* Reads of the per-sample color need to be done in
      * order.
      int sample_index = (nir_intrinsic_base(instr) -
         for (int i = 0; i <= sample_index; i++) {
            if (c->color_reads[i].file == QFILE_NULL) {
            }
   ntq_store_def(c, &instr->def, 0,
   }
      static void
   ntq_emit_load_input(struct vc4_compile *c, nir_intrinsic_instr *instr)
   {
         assert(instr->num_components == 1);
   assert(nir_src_is_const(instr->src[0]) &&
            if (c->stage == QSTAGE_FRAG &&
         nir_intrinsic_base(instr) >= VC4_NIR_TLB_COLOR_READ_INPUT) {
                  uint32_t offset = nir_intrinsic_base(instr) +
         int comp = nir_intrinsic_component(instr);
   ntq_store_def(c, &instr->def, 0,
   }
      static void
   ntq_emit_intrinsic(struct vc4_compile *c, nir_intrinsic_instr *instr)
   {
                  switch (instr->intrinsic) {
   case nir_intrinsic_decl_reg:
   case nir_intrinsic_load_reg:
   case nir_intrinsic_store_reg:
            case nir_intrinsic_load_uniform:
            assert(instr->num_components == 1);
   if (nir_src_is_const(instr->src[0])) {
            offset = nir_intrinsic_base(instr) +
         assert(offset % 4 == 0);
   /* We need dwords */
   offset = offset / 4;
   ntq_store_def(c, &instr->def, 0,
      } else {
                     case nir_intrinsic_load_ubo:
                        case nir_intrinsic_load_user_clip_plane:
            for (int i = 0; i < nir_intrinsic_dest_components(instr); i++) {
            ntq_store_def(c, &instr->def, i,
                  case nir_intrinsic_load_blend_const_color_r_float:
   case nir_intrinsic_load_blend_const_color_g_float:
   case nir_intrinsic_load_blend_const_color_b_float:
   case nir_intrinsic_load_blend_const_color_a_float:
            ntq_store_def(c, &instr->def, 0,
                           case nir_intrinsic_load_blend_const_color_rgba8888_unorm:
            ntq_store_def(c, &instr->def, 0,
               case nir_intrinsic_load_blend_const_color_aaaa8888_unorm:
            ntq_store_def(c, &instr->def, 0,
               case nir_intrinsic_load_sample_mask_in:
                        case nir_intrinsic_load_front_face:
            /* The register contains 0 (front) or 1 (back), and we need to
   * turn it into a NIR bool where true means front.
   */
   ntq_store_def(c, &instr->def, 0,
                     case nir_intrinsic_load_input:
                  case nir_intrinsic_store_output:
            assert(nir_src_is_const(instr->src[1]) &&
                        /* MSAA color outputs are the only case where we have an
   * output that's not lowered to being a store of a single 32
   * bit value.
   */
   if (c->stage == QSTAGE_FRAG && instr->num_components == 4) {
            assert(offset == c->output_color_index);
   for (int i = 0; i < 4; i++) {
         c->sample_colors[i] =
      } else {
            offset = offset * 4 + nir_intrinsic_component(instr);
   assert(instr->num_components == 1);
   c->outputs[offset] =
            case nir_intrinsic_discard:
            if (c->execute.file != QFILE_NULL) {
            qir_SF(c, c->execute);
      } else {
               case nir_intrinsic_discard_if: {
                           if (c->execute.file != QFILE_NULL) {
            /* execute == 0 means the channel is active.  Invert
   * the condition so that we can use zero as "executing
   * and discarding."
   */
      } else {
                              case nir_intrinsic_load_texture_scale: {
                           ntq_store_def(c, &instr->def, 0,
         ntq_store_def(c, &instr->def, 1,
               default:
            fprintf(stderr, "Unknown intrinsic: ");
   nir_print_instr(&instr->instr, stderr);
      }
      /* Clears (activates) the execute flags for any channels whose jump target
   * matches this block.
   */
   static void
   ntq_activate_execute_for_block(struct vc4_compile *c)
   {
         qir_SF(c, qir_SUB(c,
               }
      static void
   ntq_emit_if(struct vc4_compile *c, nir_if *if_stmt)
   {
         if (!c->vc4->screen->has_control_flow) {
            fprintf(stderr,
               nir_block *nir_else_block = nir_if_first_else_block(if_stmt);
   bool empty_else_block =
                  struct qblock *then_block = qir_new_block(c);
   struct qblock *after_block = qir_new_block(c);
   struct qblock *else_block;
   if (empty_else_block)
         else
            bool was_top_level = false;
   if (c->execute.file == QFILE_NULL) {
                        /* Set ZS for executing (execute == 0) and jumping (if->condition ==
      * 0) channels, and then update execute flags for those to point to
   * the ELSE block.
      qir_SF(c, qir_OR(c,
               qir_MOV_cond(c, QPU_COND_ZS, c->execute,
            /* Jump to ELSE if nothing is active for THEN, otherwise fall
      * through.
      qir_SF(c, c->execute);
   qir_BRANCH(c, QPU_COND_BRANCH_ALL_ZC);
   qir_link_blocks(c->cur_block, else_block);
            /* Process the THEN block. */
   qir_set_emit_block(c, then_block);
            if (!empty_else_block) {
            /* Handle the end of the THEN block.  First, all currently
   * active channels update their execute flags to point to
   * ENDIF
   */
                        /* If everything points at ENDIF, then jump there immediately. */
   qir_SF(c, qir_SUB(c, c->execute, qir_uniform_ui(c, after_block->index)));
                        qir_set_emit_block(c, else_block);
                        qir_set_emit_block(c, after_block);
   if (was_top_level) {
               } else {
         }
      static void
   ntq_emit_jump(struct vc4_compile *c, nir_jump_instr *jump)
   {
         struct qblock *jump_block;
   switch (jump->type) {
   case nir_jump_break:
               case nir_jump_continue:
               default:
                  qir_SF(c, c->execute);
   qir_MOV_cond(c, QPU_COND_ZS, c->execute,
            /* Jump to the destination block if everyone has taken the jump. */
   qir_SF(c, qir_SUB(c, c->execute, qir_uniform_ui(c, jump_block->index)));
   qir_BRANCH(c, QPU_COND_BRANCH_ALL_ZS);
   struct qblock *new_block = qir_new_block(c);
   qir_link_blocks(c->cur_block, jump_block);
   qir_link_blocks(c->cur_block, new_block);
   }
      static void
   ntq_emit_instr(struct vc4_compile *c, nir_instr *instr)
   {
         switch (instr->type) {
   case nir_instr_type_alu:
                  case nir_instr_type_intrinsic:
                  case nir_instr_type_load_const:
                  case nir_instr_type_undef:
                  case nir_instr_type_tex:
                  case nir_instr_type_jump:
                  default:
            fprintf(stderr, "Unknown NIR instr type: ");
   nir_print_instr(instr, stderr);
      }
      static void
   ntq_emit_block(struct vc4_compile *c, nir_block *block)
   {
         nir_foreach_instr(instr, block) {
         }
      static void ntq_emit_cf_list(struct vc4_compile *c, struct exec_list *list);
      static void
   ntq_emit_loop(struct vc4_compile *c, nir_loop *loop)
   {
         assert(!nir_loop_has_continue_construct(loop));
   if (!c->vc4->screen->has_control_flow) {
            fprintf(stderr,
                     bool was_top_level = false;
   if (c->execute.file == QFILE_NULL) {
                        struct qblock *save_loop_cont_block = c->loop_cont_block;
            c->loop_cont_block = qir_new_block(c);
            qir_link_blocks(c->cur_block, c->loop_cont_block);
   qir_set_emit_block(c, c->loop_cont_block);
                     /* If anything had explicitly continued, or is here at the end of the
      * loop, then we need to loop again.  SF updates are masked by the
   * instruction's condition, so we can do the OR of the two conditions
   * within SF.
      qir_SF(c, c->execute);
   struct qinst *cont_check =
            qir_SUB_dest(c,
            cont_check->cond = QPU_COND_ZC;
            qir_BRANCH(c, QPU_COND_BRANCH_ANY_ZS);
   qir_link_blocks(c->cur_block, c->loop_cont_block);
            qir_set_emit_block(c, c->loop_break_block);
   if (was_top_level) {
               } else {
                  c->loop_break_block = save_loop_break_block;
   }
      static void
   ntq_emit_function(struct vc4_compile *c, nir_function_impl *func)
   {
         fprintf(stderr, "FUNCTIONS not handled.\n");
   }
      static void
   ntq_emit_cf_list(struct vc4_compile *c, struct exec_list *list)
   {
         foreach_list_typed(nir_cf_node, node, node, list) {
            switch (node->type) {
                                                                                       default:
            }
      static void
   ntq_emit_impl(struct vc4_compile *c, nir_function_impl *impl)
   {
         ntq_setup_registers(c, impl);
   }
      static void
   nir_to_qir(struct vc4_compile *c)
   {
         if (c->stage == QSTAGE_FRAG && c->s->info.fs.uses_discard)
            ntq_setup_inputs(c);
            /* Find the main function and emit the body. */
   nir_foreach_function(function, c->s) {
            assert(strcmp(function->name, "main") == 0);
      }
      static const nir_shader_compiler_options nir_options = {
         .lower_all_io_to_temps = true,
   .lower_extract_byte = true,
   .lower_extract_word = true,
   .lower_insert_byte = true,
   .lower_insert_word = true,
   .lower_fdiv = true,
   .lower_ffma16 = true,
   .lower_ffma32 = true,
   .lower_ffma64 = true,
   .lower_flrp32 = true,
   .lower_fmod = true,
   .lower_fpow = true,
   .lower_fsat = true,
   .lower_fsqrt = true,
   .lower_ldexp = true,
   .lower_fneg = true,
   .lower_ineg = true,
   .lower_to_scalar = true,
   .lower_umax = true,
   .lower_umin = true,
   .lower_isign = true,
   .has_fsub = true,
   .has_isub = true,
   .has_texture_scaling = true,
   .lower_mul_high = true,
   .max_unroll_iterations = 32,
   };
      const void *
   vc4_screen_get_compiler_options(struct pipe_screen *pscreen,
               {
         }
      static int
   count_nir_instrs(nir_shader *nir)
   {
         int count = 0;
   nir_foreach_function_impl(impl, nir) {
            nir_foreach_block(block, impl) {
            }
   }
      static struct vc4_compile *
   vc4_shader_ntq(struct vc4_context *vc4, enum qstage stage,
         {
                  c->vc4 = vc4;
   c->stage = stage;
   c->shader_state = &key->shader_state->base;
   c->program_id = key->shader_state->program_id;
   c->variant_id =
                  c->key = key;
   switch (stage) {
   case QSTAGE_FRAG:
            c->fs_key = (struct vc4_fs_key *)key;
   if (c->fs_key->is_points) {
               } else if (c->fs_key->is_lines) {
            case QSTAGE_VERT:
               case QSTAGE_COORD:
                                 if (stage == QSTAGE_FRAG) {
                  struct nir_lower_tex_options tex_options = {
                     /* Apply swizzles to all samplers. */
               /* Lower the format swizzle and ARB_texture_swizzle-style swizzle.
      * The format swizzling applies before sRGB decode, and
   * ARB_texture_swizzle is the last thing before returning the sample.
      for (int i = 0; i < ARRAY_SIZE(key->tex); i++) {
                                                                     if (arb_swiz <= 3) {
         tex_options.swizzles[i][j] =
                                          if (c->key->ucp_enables) {
            if (stage == QSTAGE_FRAG) {
               } else {
            NIR_PASS_V(c->s, nir_lower_clip_vs,
                  /* FS input scalarizing must happen after nir_lower_two_sided_color,
      * which only handles a vec4 at a time.  Similarly, VS output
   * scalarizing must happen after nir_lower_clip_vs.
      if (c->stage == QSTAGE_FRAG)
         else
            NIR_PASS_V(c->s, vc4_nir_lower_io, c);
   NIR_PASS_V(c->s, vc4_nir_lower_txf_ms, c);
   nir_lower_idiv_options idiv_options = {
         };
   NIR_PASS_V(c->s, nir_lower_idiv, &idiv_options);
                     /* Do late algebraic optimization to turn add(a, neg(b)) back into
      * subs, then the mandatory cleanup after algebraic.  Note that it may
   * produce fnegs, and if so then we need to keep running to squash
   * fneg(fneg(a)).
      bool more_late_algebraic = true;
   while (more_late_algebraic) {
            more_late_algebraic = false;
   NIR_PASS(more_late_algebraic, c->s, nir_opt_algebraic_late);
   NIR_PASS_V(c->s, nir_opt_constant_folding);
   NIR_PASS_V(c->s, nir_copy_prop);
                        NIR_PASS_V(c->s, nir_convert_from_ssa, true);
            if (VC4_DBG(NIR)) {
            fprintf(stderr, "%s prog %d/%d NIR:\n",
                              switch (stage) {
   case QSTAGE_FRAG:
            /* FS threading requires that the thread execute
   * QPU_SIG_LAST_THREAD_SWITCH exactly once before terminating
   * (with no other THRSW afterwards, obviously).  If we didn't
   * fetch a texture at a top level block, this wouldn't be
   * true.
   */
   if (c->fs_threaded && !c->last_thrsw_at_top_level) {
                           case QSTAGE_VERT:
            emit_vert_end(c,
            case QSTAGE_COORD:
                        if (VC4_DBG(QIR)) {
            fprintf(stderr, "%s prog %d/%d pre-opt QIR:\n",
                           qir_optimize(c);
            qir_schedule_instructions(c);
            if (VC4_DBG(QIR)) {
            fprintf(stderr, "%s prog %d/%d QIR:\n",
                           qir_reorder_uniforms(c);
                     }
      static void
   vc4_setup_shared_precompile_key(struct vc4_uncompiled_shader *uncompiled,
         {
                  for (int i = 0; i < s->info.num_textures; i++) {
            key->tex[i].format = PIPE_FORMAT_R8G8B8A8_UNORM;
   key->tex[i].swizzle[0] = PIPE_SWIZZLE_X;
   key->tex[i].swizzle[1] = PIPE_SWIZZLE_Y;
      }
      static inline struct vc4_varying_slot
   vc4_slot_from_slot_and_component(uint8_t slot, uint8_t component)
   {
         assume(slot < 255 / 4);
   }
      static void
   precompile_all_fs_inputs(nir_shader *s,
         {
         /* Assume all VS outputs will actually be used by the FS and output
         nir_foreach_shader_out_variable(var, s) {
            const int array_len = MAX2(glsl_get_length(var->type), 1);
   for (int j = 0; j < array_len; j++) {
            const int slot = var->data.location + j;
   const int num_components =
         for (int i = 0; i < num_components; i++) {
         const int swiz = var->data.location_frac + i;
   fs_inputs->input_slots[fs_inputs->num_inputs++] =
   }
      /**
   * Precompiles a shader variant at shader state creation time if
   * VC4_DEBUG=shaderdb is set.
   */
   static void
   vc4_shader_precompile(struct vc4_context *vc4,
         {
                  if (s->info.stage == MESA_SHADER_FRAGMENT) {
            struct vc4_fs_key key = {
            .base.shader_state = so,
   .depth_enabled = true,
   .logicop_func = PIPE_LOGICOP_COPY,
   .color_format = PIPE_FORMAT_R8G8B8A8_UNORM,
   .blend = {
                        } else {
            assert(s->info.stage == MESA_SHADER_VERTEX);
   struct vc4_varying_slot input_slots[64] = {};
   struct vc4_fs_inputs fs_inputs = {
               };
   struct vc4_vs_key key = {
                                             /* Compile VS bin shader: only position (XXX: include TF) */
   key.is_coord = true;
   fs_inputs.num_inputs = 0;
   precompile_all_fs_inputs(s, &fs_inputs);
   for (int i = 0; i < 4; i++) {
            fs_inputs.input_slots[fs_inputs.num_inputs++] =
         }
      static void *
   vc4_shader_state_create(struct pipe_context *pctx,
         {
         struct vc4_context *vc4 = vc4_context(pctx);
   struct vc4_uncompiled_shader *so = CALLOC_STRUCT(vc4_uncompiled_shader);
   if (!so)
                              if (cso->type == PIPE_SHADER_IR_NIR) {
            /* The backend takes ownership of the NIR shader on state
   * creation.
      } else {
                     if (VC4_DBG(TGSI)) {
            fprintf(stderr, "prog %d TGSI:\n",
                        if (s->info.stage == MESA_SHADER_VERTEX)
            NIR_PASS_V(s, nir_lower_io,
                                                      /* Garbage collect dead instructions */
            so->base.type = PIPE_SHADER_IR_NIR;
            if (VC4_DBG(NIR)) {
            fprintf(stderr, "%s prog %d NIR:\n",
                           if (VC4_DBG(SHADERDB)) {
                  }
      static void
   copy_uniform_state_to_shader(struct vc4_compiled_shader *shader,
         {
         int count = c->num_uniforms;
            uinfo->count = count;
   uinfo->data = ralloc_array(shader, uint32_t, count);
   memcpy(uinfo->data, c->uniform_data,
         uinfo->contents = ralloc_array(shader, enum quniform_contents, count);
   memcpy(uinfo->contents, c->uniform_contents,
                  }
      static void
   vc4_setup_compiled_fs_inputs(struct vc4_context *vc4, struct vc4_compile *c,
         {
                  memset(&inputs, 0, sizeof(inputs));
   inputs.input_slots = ralloc_array(shader,
                           memset(input_live, 0, sizeof(input_live));
   qir_for_each_inst_inorder(inst, c) {
            for (int i = 0; i < qir_get_nsrc(inst); i++) {
                     for (int i = 0; i < c->num_input_slots; i++) {
                                                         if (slot->slot == VARYING_SLOT_COL0 ||
      slot->slot == VARYING_SLOT_COL1 ||
   slot->slot == VARYING_SLOT_BFC0 ||
                        }
            /* Add our set of inputs to the set of all inputs seen.  This way, we
      * can have a single pointer that identifies an FS inputs set,
   * allowing VS to avoid recompiling when the FS is recompiled (or a
   * new one is bound using separate shader objects) but the inputs
   * don't change.
      struct set_entry *entry = _mesa_set_search(vc4->fs_inputs_set, &inputs);
   if (entry) {
               } else {
                     alloc_inputs = rzalloc(vc4->fs_inputs_set, struct vc4_fs_inputs);
                     }
      static struct vc4_compiled_shader *
   vc4_get_compiled_shader(struct vc4_context *vc4, enum qstage stage,
         {
         struct hash_table *ht;
   uint32_t key_size;
            if (stage == QSTAGE_FRAG) {
            ht = vc4->fs_cache;
      } else {
            ht = vc4->vs_cache;
               struct vc4_compiled_shader *shader;
   struct hash_entry *entry = _mesa_hash_table_search(ht, key);
   if (entry)
            struct vc4_compile *c = vc4_shader_ntq(vc4, stage, key, try_threading);
   /* If the FS failed to compile threaded, fall back to single threaded. */
   if (try_threading && c->failed) {
                                 shader->program_id = vc4->next_compiled_program_id++;
   if (stage == QSTAGE_FRAG) {
                     /* Note: the temporary clone in c->s has been freed. */
   nir_shader *orig_shader = key->shader_state->base.ir.nir;
      } else {
                     shader->vattr_offsets[0] = 0;
                                          shader->failed = c->failed;
   if (c->failed) {
         } else {
            copy_uniform_state_to_shader(shader, c);
   shader->bo = vc4_bo_alloc_shader(vc4->screen, c->qpu_insts,
                                 struct vc4_key *dup_key;
   dup_key = rzalloc_size(shader, key_size); /* TODO: don't use rzalloc */
   memcpy(dup_key, key, key_size);
            }
      static void
   vc4_setup_shared_key(struct vc4_context *vc4, struct vc4_key *key,
         {
         for (int i = 0; i < texstate->num_textures; i++) {
            struct pipe_sampler_view *sampler = texstate->textures[i];
                                       key->tex[i].format = sampler->format;
   key->tex[i].swizzle[0] = sampler->swizzle_r;
                        if (sampler->texture->nr_samples > 1) {
               } else if (sampler){
            key->tex[i].compare_mode = sampler_state->compare_mode;
   key->tex[i].compare_func = sampler_state->compare_func;
   key->tex[i].wrap_s = sampler_state->wrap_s;
   key->tex[i].wrap_t = sampler_state->wrap_t;
            }
      static void
   vc4_update_compiled_fs(struct vc4_context *vc4, uint8_t prim_mode)
   {
         struct vc4_job *job = vc4->job;
   struct vc4_fs_key local_key;
            if (!(vc4->dirty & (VC4_DIRTY_PRIM_MODE |
                        VC4_DIRTY_BLEND |
   VC4_DIRTY_FRAMEBUFFER |
   VC4_DIRTY_ZSA |
   VC4_DIRTY_RASTERIZER |
   VC4_DIRTY_SAMPLE_MASK |
               memset(key, 0, sizeof(*key));
   vc4_setup_shared_key(vc4, &key->base, &vc4->fragtex);
   key->base.shader_state = vc4->prog.bind_fs;
   key->is_points = (prim_mode == MESA_PRIM_POINTS);
   key->is_lines = (prim_mode >= MESA_PRIM_LINES &&
         key->blend = vc4->blend->rt[0];
   if (vc4->blend->logicop_enable) {
         } else {
         }
   if (job->msaa) {
            key->msaa = vc4->rasterizer->base.multisample;
   key->sample_coverage = (vc4->sample_mask != (1 << VC4_MAX_SAMPLES) - 1);
               if (vc4->framebuffer.cbufs[0])
            key->stencil_enabled = vc4->zsa->stencil_uniforms[0] != 0;
   key->stencil_twoside = vc4->zsa->stencil_uniforms[1] != 0;
   key->stencil_full_writemasks = vc4->zsa->stencil_uniforms[2] != 0;
   key->depth_enabled = (vc4->zsa->base.depth_enabled ||
            if (key->is_points) {
            key->point_sprite_mask =
         key->point_coord_upper_left =
                        struct vc4_compiled_shader *old_fs = vc4->prog.fs;
   vc4->prog.fs = vc4_get_compiled_shader(vc4, QSTAGE_FRAG, &key->base);
   if (vc4->prog.fs == old_fs)
                     if (vc4->rasterizer->base.flatshade &&
         (!old_fs || vc4->prog.fs->color_inputs != old_fs->color_inputs)) {
            if (!old_fs || vc4->prog.fs->fs_inputs != old_fs->fs_inputs)
   }
      static void
   vc4_update_compiled_vs(struct vc4_context *vc4, uint8_t prim_mode)
   {
         struct vc4_vs_key local_key;
            if (!(vc4->dirty & (VC4_DIRTY_PRIM_MODE |
                        VC4_DIRTY_RASTERIZER |
   VC4_DIRTY_VERTTEX |
               memset(key, 0, sizeof(*key));
   vc4_setup_shared_key(vc4, &key->base, &vc4->verttex);
   key->base.shader_state = vc4->prog.bind_vs;
            for (int i = 0; i < ARRAY_SIZE(key->attr_formats); i++)
            key->per_vertex_point_size =
                  struct vc4_compiled_shader *vs =
         if (vs != vc4->prog.vs) {
                        key->is_coord = true;
   /* Coord shaders don't care what the FS inputs are. */
   key->fs_inputs = NULL;
   struct vc4_compiled_shader *cs =
         if (cs != vc4->prog.cs) {
               }
      bool
   vc4_update_compiled_shaders(struct vc4_context *vc4, uint8_t prim_mode)
   {
         vc4_update_compiled_fs(vc4, prim_mode);
            return !(vc4->prog.cs->failed ||
         }
      static uint32_t
   fs_cache_hash(const void *key)
   {
         }
      static uint32_t
   vs_cache_hash(const void *key)
   {
         }
      static bool
   fs_cache_compare(const void *key1, const void *key2)
   {
         }
      static bool
   vs_cache_compare(const void *key1, const void *key2)
   {
         }
      static uint32_t
   fs_inputs_hash(const void *key)
   {
                  return _mesa_hash_data(inputs->input_slots,
         }
      static bool
   fs_inputs_compare(const void *key1, const void *key2)
   {
         const struct vc4_fs_inputs *inputs1 = key1;
            return (inputs1->num_inputs == inputs2->num_inputs &&
            memcmp(inputs1->input_slots,
      }
      static void
   delete_from_cache_if_matches(struct hash_table *ht,
                     {
                  if (key->shader_state == so) {
                                             }
      static void
   vc4_shader_state_delete(struct pipe_context *pctx, void *hwcso)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
            hash_table_foreach(vc4->fs_cache, entry) {
               }
   hash_table_foreach(vc4->vs_cache, entry) {
                        ralloc_free(so->base.ir.nir);
   }
      static void
   vc4_fp_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->prog.bind_fs = hwcso;
   }
      static void
   vc4_vp_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->prog.bind_vs = hwcso;
   }
      void
   vc4_program_init(struct pipe_context *pctx)
   {
                  pctx->create_vs_state = vc4_shader_state_create;
            pctx->create_fs_state = vc4_shader_state_create;
            pctx->bind_fs_state = vc4_fp_state_bind;
            vc4->fs_cache = _mesa_hash_table_create(pctx, fs_cache_hash,
         vc4->vs_cache = _mesa_hash_table_create(pctx, vs_cache_hash,
         vc4->fs_inputs_set = _mesa_set_create(pctx, fs_inputs_hash,
   }
      void
   vc4_program_fini(struct pipe_context *pctx)
   {
                  hash_table_foreach(vc4->fs_cache, entry) {
            struct vc4_compiled_shader *shader = entry->data;
   vc4_bo_unreference(&shader->bo);
               hash_table_foreach(vc4->vs_cache, entry) {
            struct vc4_compiled_shader *shader = entry->data;
   vc4_bo_unreference(&shader->bo);
      }
