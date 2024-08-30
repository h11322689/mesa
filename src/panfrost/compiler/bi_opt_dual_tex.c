   /*
   * Copyright (C) 2021 Collabora, Ltd.
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
      #include "bi_builder.h"
   #include "compiler.h"
      #define XXH_INLINE_ALL
   #include "util/xxhash.h"
      /* Fuse pairs of TEXS_2D instructions into a single dual texture TEXC, if both
   * sample at the same coordinate with the default LOD mode for the shader stage
   * (computed LOD in fragment shaders, else zero LOD) and immediate
   * texture/sampler indices 0...3.
   *
   * Fusing across basic block boundaries is not expected to be useful, as it
   * increases register pressure and causes redundant memory traffic. As such, we
   * use a local optimization pass.
   *
   * To pair ops efficiently, we maintain a set (backed by a hash table) using
   * only the coordinate sources for hashing and equality. Hence the pass runs in
   * O(n) worst case expected time for n insturctions in a block. We reject
   * invalid texture instructions quickly to reduce the constant factor.
   *
   * Dual texture instructions have skip flags, like normal texture instructions.
   * Adding a skip flag to an instruction that doesn't have it is illegal, but
   * removing a skip flag from one that has it is legal. Accordingly, set the
   * fused TEXC's skip to the logical AND of the unfused TEXS flags. We run the
   * optimization pass to run after bi_analyze_helper_requirements.
   */
      static inline bool
   bi_can_fuse_dual_tex(bi_instr *I, bool fuse_zero_lod)
   {
      return (I->op == BI_OPCODE_TEXS_2D_F32 || I->op == BI_OPCODE_TEXS_2D_F16) &&
            }
      static enum bifrost_texture_format
   bi_format_for_texs_2d(enum bi_opcode op)
   {
      switch (op) {
   case BI_OPCODE_TEXS_2D_F32:
         case BI_OPCODE_TEXS_2D_F16:
         default:
            }
      static void
   bi_fuse_dual(bi_context *ctx, bi_instr *I1, bi_instr *I2)
   {
      /* Construct a texture operation descriptor for the dual texture */
   struct bifrost_dual_texture_operation desc = {
               .primary_texture_index = I1->texture_index,
   .primary_sampler_index = I1->sampler_index,
   .primary_format = bi_format_for_texs_2d(I1->op),
            .secondary_texture_index = I2->texture_index,
   .secondary_sampler_index = I2->sampler_index,
   .secondary_format = bi_format_for_texs_2d(I2->op),
               /* LOD mode is implied in a shader stage */
            /* Insert before the earlier instruction in case its result is consumed
   * before the later instruction
   */
            bi_instr *I = bi_texc_dual_to(
      &b, I1->dest[0], I2->dest[0], bi_null(), /* staging */
   I1->src[0], I1->src[1],                  /* coordinates */
   bi_imm_u32(bi_dual_tex_as_u32(desc)), I1->lod_mode,
                  bi_remove_instruction(I1);
      }
      #define HASH(hash, data) XXH32(&(data), sizeof(data), hash)
      static uint32_t
   coord_hash(const void *key)
   {
                  }
      static bool
   coord_equal(const void *key1, const void *key2)
   {
      const bi_instr *I = key1;
            return memcmp(&I->src[0], &J->src[0],
      }
      static void
   bi_opt_fuse_dual_texture_block(bi_context *ctx, bi_block *block)
   {
      struct set *set = _mesa_set_create(ctx, coord_hash, coord_equal);
   bool fuse_zero_lod = (ctx->stage != MESA_SHADER_FRAGMENT);
            bi_foreach_instr_in_block_safe(block, I) {
      if (!bi_can_fuse_dual_tex(I, fuse_zero_lod))
                     if (found) {
      bi_fuse_dual(ctx, (bi_instr *)ent->key, I);
            }
      void
   bi_opt_fuse_dual_texture(bi_context *ctx)
   {
      bi_foreach_block(ctx, block) {
            }
