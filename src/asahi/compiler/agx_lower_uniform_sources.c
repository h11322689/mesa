   /*
   * Copyright 2023 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
   #include "agx_builder.h"
   #include "agx_compiler.h"
      /*
   * Not all instructions can take uniforms. Memory instructions can take
   * uniforms, but only for their base (first) source and only in the
   * low-half of the uniform file.
   *
   * This pass lowers invalid uniforms.
   */
   static bool
   should_lower(enum agx_opcode op, agx_index uniform, unsigned src_index)
   {
      if (uniform.type != AGX_INDEX_UNIFORM)
            /* Some instructions only seem able to access uniforms in the low half */
            switch (op) {
   case AGX_OPCODE_IMAGE_LOAD:
   case AGX_OPCODE_TEXTURE_LOAD:
   case AGX_OPCODE_TEXTURE_SAMPLE:
         case AGX_OPCODE_DEVICE_LOAD:
         case AGX_OPCODE_DEVICE_STORE:
   case AGX_OPCODE_ATOMIC:
         case AGX_OPCODE_LOCAL_LOAD:
         case AGX_OPCODE_LOCAL_STORE:
         case AGX_OPCODE_IMAGE_WRITE:
         case AGX_OPCODE_ZS_EMIT:
   case AGX_OPCODE_ST_TILE:
   case AGX_OPCODE_LD_TILE:
   case AGX_OPCODE_BLOCK_IMAGE_STORE:
   case AGX_OPCODE_UNIFORM_STORE:
   case AGX_OPCODE_ST_VARY:
   case AGX_OPCODE_LOCAL_ATOMIC:
   case AGX_OPCODE_SAMPLE_MASK:
   case AGX_OPCODE_ITER:
   case AGX_OPCODE_ITERPROJ:
         default:
            }
      void
   agx_lower_uniform_sources(agx_context *ctx)
   {
      agx_foreach_instr_global_safe(ctx, I) {
               agx_foreach_src(I, s) {
      if (should_lower(I->op, I->src[s], s))
            }
