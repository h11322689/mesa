   /*
   * Copyright © 2019 Broadcom
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
   * Implements lowering for logical operations.
   *
   * V3D doesn't have any hardware support for logic ops.  Instead, you read the
   * current contents of the destination from the tile buffer, then do math using
   * your output color and that destination value, and update the output color
   * appropriately.
   */
      #include "util/format/u_format.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_format_convert.h"
   #include "v3d_compiler.h"
         typedef nir_def *(*nir_pack_func)(nir_builder *b, nir_def *c);
   typedef nir_def *(*nir_unpack_func)(nir_builder *b, nir_def *c);
      static bool
   logicop_depends_on_dst_color(int logicop_func)
   {
         switch (logicop_func) {
   case PIPE_LOGICOP_SET:
   case PIPE_LOGICOP_CLEAR:
   case PIPE_LOGICOP_COPY:
   case PIPE_LOGICOP_COPY_INVERTED:
         default:
         }
      static nir_def *
   v3d_logicop(nir_builder *b, int logicop_func,
         {
         switch (logicop_func) {
   case PIPE_LOGICOP_CLEAR:
         case PIPE_LOGICOP_NOR:
         case PIPE_LOGICOP_AND_INVERTED:
         case PIPE_LOGICOP_COPY_INVERTED:
         case PIPE_LOGICOP_AND_REVERSE:
         case PIPE_LOGICOP_INVERT:
         case PIPE_LOGICOP_XOR:
         case PIPE_LOGICOP_NAND:
         case PIPE_LOGICOP_AND:
         case PIPE_LOGICOP_EQUIV:
         case PIPE_LOGICOP_NOOP:
         case PIPE_LOGICOP_OR_INVERTED:
         case PIPE_LOGICOP_OR_REVERSE:
         case PIPE_LOGICOP_OR:
         case PIPE_LOGICOP_SET:
         default:
               case PIPE_LOGICOP_COPY:
         }
      static nir_def *
   v3d_nir_get_swizzled_channel(nir_builder *b, nir_def **srcs, int swiz)
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
      static nir_def *
   v3d_nir_swizzle_and_pack(nir_builder *b, nir_def **chans,
         {
         nir_def *c[4];
   for (int i = 0; i < 4; i++)
            }
      static nir_def *
   v3d_nir_unpack_and_swizzle(nir_builder *b, nir_def *packed,
         {
                  nir_def *unpacked_chans[4];
   for (int i = 0; i < 4; i++)
            nir_def *c[4];
   for (int i = 0; i < 4; i++)
            }
      static nir_def *
   pack_unorm_rgb10a2(nir_builder *b, nir_def *c)
   {
         static const unsigned bits[4] = { 10, 10, 10, 2 };
            nir_def *chans[4];
   for (int i = 0; i < 4; i++)
            nir_def *result = nir_mov(b, chans[0]);
   int offset = bits[0];
   for (int i = 1; i < 4; i++) {
            nir_def *shifted_chan =
            }
   }
      static nir_def *
   unpack_unorm_rgb10a2(nir_builder *b, nir_def *c)
   {
         static const unsigned bits[4] = { 10, 10, 10, 2 };
   const unsigned masks[4] = { BITFIELD_MASK(bits[0]),
                        nir_def *chans[4];
   for (int i = 0; i < 4; i++) {
            nir_def *unorm = nir_iand_imm(b, c, masks[i]);
               }
      static const uint8_t *
   v3d_get_format_swizzle_for_rt(struct v3d_compile *c, int rt)
   {
                  /* We will automatically swap R and B channels for BGRA formats
      * on tile loads and stores (see 'swap_rb' field in v3d_resource) so
   * we want to treat these surfaces as if they were regular RGBA formats.
      if (c->fs_key->color_fmt[rt].swizzle[0] == 2 &&
         c->fs_key->color_fmt[rt].format != PIPE_FORMAT_B5G6R5_UNORM) {
   } else {
         }
      static nir_def *
   v3d_nir_get_tlb_color(nir_builder *b, struct v3d_compile *c, int rt, int sample)
   {
         uint32_t num_components =
            nir_def *color[4];
   for (int i = 0; i < 4; i++) {
            if (i < num_components) {
            color[i] =
            } else {
            }
   }
      static nir_def *
   v3d_emit_logic_op_raw(struct v3d_compile *c, nir_builder *b,
               {
                  nir_def *op_res[4];
   for (int i = 0; i < 4; i++) {
            nir_def *src = src_chans[i];
                        /* We configure our integer RTs to clamp, so we need to ignore
   * result bits that don't fit in the destination RT component
   * size.
   */
   uint32_t bits =
            util_format_get_component_bits(
      if (bits > 0 && bits < 32) {
                     nir_def *r[4];
   for (int i = 0; i < 4; i++)
            }
      static nir_def *
   v3d_emit_logic_op_unorm(struct v3d_compile *c, nir_builder *b,
                     {
         static const uint8_t src_swz[4] = { 0, 1, 2, 3 };
   nir_def *packed_src =
            const uint8_t *fmt_swz = v3d_get_format_swizzle_for_rt(c, rt);
   nir_def *packed_dst =
            nir_def *packed_result =
            }
      static nir_def *
   v3d_nir_emit_logic_op(struct v3d_compile *c, nir_builder *b,
         {
                  nir_def *src_chans[4], *dst_chans[4];
   for (unsigned i = 0; i < 4; i++) {
                        if (c->fs_key->color_fmt[rt].format == PIPE_FORMAT_R10G10B10A2_UNORM) {
            return v3d_emit_logic_op_unorm(
               if (util_format_is_unorm(c->fs_key->color_fmt[rt].format)) {
            return v3d_emit_logic_op_unorm(
               }
      static void
   v3d_emit_ms_output(nir_builder *b,
               {
         }
      static void
   v3d_nir_lower_logic_op_instr(struct v3d_compile *c,
                     {
                     const int logic_op = c->fs_key->logicop_func;
   if (c->fs_key->msaa && logicop_depends_on_dst_color(logic_op)) {
                     nir_src *offset = &intr->src[1];
   nir_alu_type type = nir_intrinsic_src_type(intr);
                                    } else {
                              }
      static bool
   v3d_nir_lower_logic_ops_block(nir_block *block, struct v3d_compile *c)
   {
                  nir_foreach_instr_safe(instr, block) {
                                                nir_foreach_shader_out_variable(var, c->s) {
                                 const int loc = var->data.location;
   if (loc != FRAG_RESULT_COLOR &&
                              /* Logic operations do not apply on floating point or
                              const enum pipe_format format =
                                                      }
      bool
   v3d_nir_lower_logic_ops(nir_shader *s, struct v3d_compile *c)
   {
                  /* Nothing to do if logic op is 'copy src to dst' or if logic ops are
      * disabled (we set the logic op to copy in that case).
      if (c->fs_key->logicop_func == PIPE_LOGICOP_COPY)
            nir_foreach_function_impl(impl, s) {
                           if (progress) {
            nir_metadata_preserve(impl,
      } else {
                     }
