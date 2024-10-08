   /*
   * Copyright 2018 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_pipe.h"
   #include "tgsi/tgsi_text.h"
   #include "tgsi/tgsi_ureg.h"
      void *si_get_blitter_vs(struct si_context *sctx, enum blitter_attrib_type type, unsigned num_layers)
   {
      unsigned vs_blit_property;
            switch (type) {
   case UTIL_BLITTER_ATTRIB_NONE:
      vs = num_layers > 1 ? &sctx->vs_blit_pos_layered : &sctx->vs_blit_pos;
   vs_blit_property = SI_VS_BLIT_SGPRS_POS;
      case UTIL_BLITTER_ATTRIB_COLOR:
      vs = num_layers > 1 ? &sctx->vs_blit_color_layered : &sctx->vs_blit_color;
   vs_blit_property = SI_VS_BLIT_SGPRS_POS_COLOR;
      case UTIL_BLITTER_ATTRIB_TEXCOORD_XY:
   case UTIL_BLITTER_ATTRIB_TEXCOORD_XYZW:
      assert(num_layers == 1);
   vs = &sctx->vs_blit_texcoord;
   vs_blit_property = SI_VS_BLIT_SGPRS_POS_TEXCOORD;
      default:
      assert(0);
      }
   if (*vs)
            struct ureg_program *ureg = ureg_create(PIPE_SHADER_VERTEX);
   if (!ureg)
            /* Add 1 for the attribute ring address. */
   if (sctx->gfx_level >= GFX11 && type != UTIL_BLITTER_ATTRIB_NONE)
            /* Tell the shader to load VS inputs from SGPRs: */
   ureg_property(ureg, TGSI_PROPERTY_VS_BLIT_SGPRS_AMD, vs_blit_property);
            /* This is just a pass-through shader with 1-3 MOV instructions. */
            if (type != UTIL_BLITTER_ATTRIB_NONE) {
                  if (num_layers > 1) {
      struct ureg_src instance_id = ureg_DECL_system_value(ureg, TGSI_SEMANTIC_INSTANCEID, 0);
            ureg_MOV(ureg, ureg_writemask(layer, TGSI_WRITEMASK_X),
      }
            *vs = ureg_create_shader_and_destroy(ureg, &sctx->b);
      }
      /* Create a compute shader implementing clear_buffer or copy_buffer. */
   void *si_create_dma_compute_shader(struct pipe_context *ctx, unsigned num_dwords_per_thread,
         {
      struct si_screen *sscreen = (struct si_screen *)ctx->screen;
            unsigned store_qualifier = TGSI_MEMORY_COHERENT | TGSI_MEMORY_RESTRICT;
   if (dst_stream_cache_policy)
            /* Don't cache loads, because there is no reuse. */
            unsigned num_mem_ops = MAX2(1, num_dwords_per_thread / 4);
            for (unsigned i = 0; i < num_mem_ops; i++) {
      if (i * 4 < num_dwords_per_thread)
               struct ureg_program *ureg = ureg_create(PIPE_SHADER_COMPUTE);
   if (!ureg)
                     ureg_property(ureg, TGSI_PROPERTY_CS_FIXED_BLOCK_WIDTH, default_wave_size);
   ureg_property(ureg, TGSI_PROPERTY_CS_FIXED_BLOCK_HEIGHT, 1);
            struct ureg_src value;
   if (!is_copy) {
      ureg_property(ureg, TGSI_PROPERTY_CS_USER_DATA_COMPONENTS_AMD, inst_dwords[0]);
               struct ureg_src tid = ureg_DECL_system_value(ureg, TGSI_SEMANTIC_THREAD_ID, 0);
   struct ureg_src blk = ureg_DECL_system_value(ureg, TGSI_SEMANTIC_BLOCK_ID, 0);
   struct ureg_dst store_addr = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_X);
   struct ureg_dst load_addr = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_X);
   struct ureg_dst dstbuf = ureg_dst(ureg_DECL_buffer(ureg, 0, false));
   struct ureg_src srcbuf;
            if (is_copy) {
      srcbuf = ureg_DECL_buffer(ureg, 1, false);
               /* If there are multiple stores, the first store writes into 0*wavesize+tid,
   * the 2nd store writes into 1*wavesize+tid, the 3rd store writes into 2*wavesize+tid, etc.
   */
   ureg_UMAD(ureg, store_addr, blk, ureg_imm1u(ureg, default_wave_size * num_mem_ops),
         /* Convert from a "store size unit" into bytes. */
   ureg_UMUL(ureg, store_addr, ureg_src(store_addr), ureg_imm1u(ureg, 4 * inst_dwords[0]));
            /* Distance between a load and a store for latency hiding. */
            for (unsigned i = 0; i < num_mem_ops + load_store_distance; i++) {
               if (is_copy && i < num_mem_ops) {
      if (i) {
      ureg_UADD(ureg, load_addr, ureg_src(load_addr),
               values[i] = ureg_src(ureg_DECL_temporary(ureg));
   struct ureg_dst dst =
         struct ureg_src srcs[] = {srcbuf, ureg_src(load_addr)};
   ureg_memory_insn(ureg, TGSI_OPCODE_LOAD, &dst, 1, srcs, 2, load_qualifier,
               if (d >= 0) {
      if (d) {
      ureg_UADD(ureg, store_addr, ureg_src(store_addr),
               struct ureg_dst dst = ureg_writemask(dstbuf, u_bit_consecutive(0, inst_dwords[d]));
   struct ureg_src srcs[] = {ureg_src(store_addr), is_copy ? values[d] : value};
   ureg_memory_insn(ureg, TGSI_OPCODE_STORE, &dst, 1, srcs, 2, store_qualifier,
         }
            struct pipe_compute_state state = {};
   state.ir_type = PIPE_SHADER_IR_TGSI;
            void *cs = ctx->create_compute_state(ctx, &state);
   ureg_destroy(ureg);
            free(values);
      }
      /* Create the compute shader that is used to collect the results.
   *
   * One compute grid with a single thread is launched for every query result
   * buffer. The thread (optionally) reads a previous summary buffer, then
   * accumulates data from the query result buffer, and writes the result either
   * to a summary buffer to be consumed by the next grid invocation or to the
   * user-supplied buffer.
   *
   * Data layout:
   *
   * CONST
   *  0.x = end_offset
   *  0.y = result_stride
   *  0.z = result_count
   *  0.w = bit field:
   *          1: read previously accumulated values
   *          2: write accumulated values for chaining
   *          4: write result available
   *          8: convert result to boolean (0/1)
   *         16: only read one dword and use that as result
   *         32: apply timestamp conversion
   *         64: store full 64 bits result
   *        128: store signed 32 bits result
   *        256: SO_OVERFLOW mode: take the difference of two successive half-pairs
   *  1.x = fence_offset
   *  1.y = pair_stride
   *  1.z = pair_count
   *
   * BUFFER[0] = query result buffer
   * BUFFER[1] = previous summary buffer
   * BUFFER[2] = next summary buffer or user-supplied buffer
   */
   void *si_create_query_result_cs(struct si_context *sctx)
   {
      /* TEMP[0].xy = accumulated result so far
   * TEMP[0].z = result not available
   *
   * TEMP[1].x = current result index
   * TEMP[1].y = current pair index
   */
   static const char text_tmpl[] =
      "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 1\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 1\n"
   "PROPERTY CS_FIXED_BLOCK_DEPTH 1\n"
   "DCL BUFFER[0]\n"
   "DCL BUFFER[1]\n"
   "DCL BUFFER[2]\n"
   "DCL CONST[0][0..1]\n"
   "DCL TEMP[0..5]\n"
   "IMM[0] UINT32 {0, 31, 2147483647, 4294967295}\n"
   "IMM[1] UINT32 {1, 2, 4, 8}\n"
   "IMM[2] UINT32 {16, 32, 64, 128}\n"
   "IMM[3] UINT32 {1000000, 0, %u, 0}\n" /* for timestamp conversion */
            "AND TEMP[5], CONST[0][0].wwww, IMM[2].xxxx\n"
   "UIF TEMP[5]\n"
   /* Check result availability. */
   "LOAD TEMP[1].x, BUFFER[0], CONST[0][1].xxxx\n"
   "ISHR TEMP[0].z, TEMP[1].xxxx, IMM[0].yyyy\n"
   "MOV TEMP[1], TEMP[0].zzzz\n"
            /* Load result if available. */
   "UIF TEMP[1]\n"
   "LOAD TEMP[0].xy, BUFFER[0], IMM[0].xxxx\n"
   "ENDIF\n"
   "ELSE\n"
   /* Load previously accumulated result if requested. */
   "MOV TEMP[0], IMM[0].xxxx\n"
   "AND TEMP[4], CONST[0][0].wwww, IMM[1].xxxx\n"
   "UIF TEMP[4]\n"
   "LOAD TEMP[0].xyz, BUFFER[1], IMM[0].xxxx\n"
            "MOV TEMP[1].x, IMM[0].xxxx\n"
   "BGNLOOP\n"
   /* Break if accumulated result so far is not available. */
   "UIF TEMP[0].zzzz\n"
   "BRK\n"
            /* Break if result_index >= result_count. */
   "USGE TEMP[5], TEMP[1].xxxx, CONST[0][0].zzzz\n"
   "UIF TEMP[5]\n"
   "BRK\n"
            /* Load fence and check result availability */
   "UMAD TEMP[5].x, TEMP[1].xxxx, CONST[0][0].yyyy, CONST[0][1].xxxx\n"
   "LOAD TEMP[5].x, BUFFER[0], TEMP[5].xxxx\n"
   "ISHR TEMP[0].z, TEMP[5].xxxx, IMM[0].yyyy\n"
   "NOT TEMP[0].z, TEMP[0].zzzz\n"
   "UIF TEMP[0].zzzz\n"
   "BRK\n"
            "MOV TEMP[1].y, IMM[0].xxxx\n"
   "BGNLOOP\n"
   /* Load start and end. */
   "UMUL TEMP[5].x, TEMP[1].xxxx, CONST[0][0].yyyy\n"
   "UMAD TEMP[5].x, TEMP[1].yyyy, CONST[0][1].yyyy, TEMP[5].xxxx\n"
            "UADD TEMP[5].y, TEMP[5].xxxx, CONST[0][0].xxxx\n"
                     "AND TEMP[5].z, CONST[0][0].wwww, IMM[4].xxxx\n"
   "UIF TEMP[5].zzzz\n"
   /* Load second start/end half-pair and
   * take the difference
   */
   "UADD TEMP[5].xy, TEMP[5], IMM[1].wwww\n"
   "LOAD TEMP[2].xy, BUFFER[0], TEMP[5].xxxx\n"
            "U64ADD TEMP[3].xy, TEMP[3], -TEMP[2]\n"
   "U64ADD TEMP[4].xy, TEMP[4], -TEMP[3]\n"
                     /* Increment pair index */
   "UADD TEMP[1].y, TEMP[1].yyyy, IMM[1].xxxx\n"
   "USGE TEMP[5], TEMP[1].yyyy, CONST[0][1].zzzz\n"
   "UIF TEMP[5]\n"
   "BRK\n"
   "ENDIF\n"
            /* Increment result index */
   "UADD TEMP[1].x, TEMP[1].xxxx, IMM[1].xxxx\n"
   "ENDLOOP\n"
            "AND TEMP[4], CONST[0][0].wwww, IMM[1].yyyy\n"
   "UIF TEMP[4]\n"
   /* Store accumulated data for chaining. */
   "STORE BUFFER[2].xyz, IMM[0].xxxx, TEMP[0]\n"
   "ELSE\n"
   "AND TEMP[4], CONST[0][0].wwww, IMM[1].zzzz\n"
   "UIF TEMP[4]\n"
   /* Store result availability. */
   "NOT TEMP[0].z, TEMP[0]\n"
   "AND TEMP[0].z, TEMP[0].zzzz, IMM[1].xxxx\n"
            "AND TEMP[4], CONST[0][0].wwww, IMM[2].zzzz\n"
   "UIF TEMP[4]\n"
   "STORE BUFFER[2].y, IMM[0].xxxx, IMM[0].xxxx\n"
   "ENDIF\n"
   "ELSE\n"
   /* Store result if it is available. */
   "NOT TEMP[4], TEMP[0].zzzz\n"
   "UIF TEMP[4]\n"
   /* Apply timestamp conversion */
   "AND TEMP[4], CONST[0][0].wwww, IMM[2].yyyy\n"
   "UIF TEMP[4]\n"
   "U64MUL TEMP[0].xy, TEMP[0], IMM[3].xyxy\n"
   "U64DIV TEMP[0].xy, TEMP[0], IMM[3].zwzw\n"
            /* Convert to boolean */
   "AND TEMP[4], CONST[0][0].wwww, IMM[1].wwww\n"
   "UIF TEMP[4]\n"
   "U64SNE TEMP[0].x, TEMP[0].xyxy, IMM[4].zwzw\n"
   "AND TEMP[0].x, TEMP[0].xxxx, IMM[1].xxxx\n"
   "MOV TEMP[0].y, IMM[0].xxxx\n"
            "AND TEMP[4], CONST[0][0].wwww, IMM[2].zzzz\n"
   "UIF TEMP[4]\n"
   "STORE BUFFER[2].xy, IMM[0].xxxx, TEMP[0].xyxy\n"
   "ELSE\n"
   /* Clamping */
   "UIF TEMP[0].yyyy\n"
   "MOV TEMP[0].x, IMM[0].wwww\n"
            "AND TEMP[4], CONST[0][0].wwww, IMM[2].wwww\n"
   "UIF TEMP[4]\n"
   "UMIN TEMP[0].x, TEMP[0].xxxx, IMM[0].zzzz\n"
            "STORE BUFFER[2].x, IMM[0].xxxx, TEMP[0].xxxx\n"
   "ENDIF\n"
   "ENDIF\n"
   "ENDIF\n"
                  char text[sizeof(text_tmpl) + 32];
   struct tgsi_token tokens[1024];
            /* Hard code the frequency into the shader so that the backend can
   * use the full range of optimizations for divide-by-constant.
   */
            if (!tgsi_text_translate(text, tokens, ARRAY_SIZE(tokens))) {
      assert(false);
               state.ir_type = PIPE_SHADER_IR_TGSI;
               }
      /* Load samples from the image, and copy them to the same image. This looks like
   * a no-op, but it's not. Loads use FMASK, while stores don't, so samples are
   * reordered to match expanded FMASK.
   *
   * After the shader finishes, FMASK should be cleared to identity.
   */
   void *si_create_fmask_expand_cs(struct pipe_context *ctx, unsigned num_samples, bool is_array)
   {
      enum tgsi_texture_type target = is_array ? TGSI_TEXTURE_2D_ARRAY_MSAA : TGSI_TEXTURE_2D_MSAA;
   struct ureg_program *ureg = ureg_create(PIPE_SHADER_COMPUTE);
   if (!ureg)
            ureg_property(ureg, TGSI_PROPERTY_CS_FIXED_BLOCK_WIDTH, 8);
   ureg_property(ureg, TGSI_PROPERTY_CS_FIXED_BLOCK_HEIGHT, 8);
            /* Compute the image coordinates. */
   struct ureg_src image = ureg_DECL_image(ureg, 0, target, 0, true, false);
   struct ureg_src tid = ureg_DECL_system_value(ureg, TGSI_SEMANTIC_THREAD_ID, 0);
   struct ureg_src blk = ureg_DECL_system_value(ureg, TGSI_SEMANTIC_BLOCK_ID, 0);
   struct ureg_dst coord = ureg_writemask(ureg_DECL_temporary(ureg), TGSI_WRITEMASK_XYZW);
   ureg_UMAD(ureg, ureg_writemask(coord, TGSI_WRITEMASK_XY), ureg_swizzle(blk, 0, 1, 1, 1),
         if (is_array) {
                  /* Load samples, resolving FMASK. */
   struct ureg_dst sample[8];
            for (unsigned i = 0; i < num_samples; i++) {
                        struct ureg_src srcs[] = {image, ureg_src(coord)};
   ureg_memory_insn(ureg, TGSI_OPCODE_LOAD, &sample[i], 1, srcs, 2, TGSI_MEMORY_RESTRICT, target,
               /* Store samples, ignoring FMASK. */
   for (unsigned i = 0; i < num_samples; i++) {
               struct ureg_dst dst_image = ureg_dst(image);
   struct ureg_src srcs[] = {ureg_src(coord), ureg_src(sample[i])};
   ureg_memory_insn(ureg, TGSI_OPCODE_STORE, &dst_image, 1, srcs, 2, TGSI_MEMORY_RESTRICT,
      }
            struct pipe_compute_state state = {};
   state.ir_type = PIPE_SHADER_IR_TGSI;
            void *cs = ctx->create_compute_state(ctx, &state);
   ureg_destroy(ureg);
               }
      /* Create the compute shader that is used to collect the results of gfx10+
   * shader queries.
   *
   * One compute grid with a single thread is launched for every query result
   * buffer. The thread (optionally) reads a previous summary buffer, then
   * accumulates data from the query result buffer, and writes the result either
   * to a summary buffer to be consumed by the next grid invocation or to the
   * user-supplied buffer.
   *
   * Data layout:
   *
   * BUFFER[0] = query result buffer (layout is defined by gfx10_sh_query_buffer_mem)
   * BUFFER[1] = previous summary buffer
   * BUFFER[2] = next summary buffer or user-supplied buffer
   *
   * CONST
   *  0.x = config; the low 3 bits indicate the mode:
   *          0: sum up counts
   *          1: determine result availability and write it as a boolean
   *          2: SO_OVERFLOW
   *          3: SO_ANY_OVERFLOW
   *        the remaining bits form a bitfield:
   *          8: write result as a 64-bit value
   *  0.y = offset in bytes to counts or stream for SO_OVERFLOW mode
   *  0.z = chain bit field:
   *          1: have previous summary buffer
   *          2: write next summary buffer
   *  0.w = result_count
   */
   void *gfx11_create_sh_query_result_cs(struct si_context *sctx)
   {
      /* TEMP[0].x = accumulated result so far
   * TEMP[0].y = result missing
   * TEMP[0].z = whether we're in overflow mode
   */
   static const char text_tmpl[] =
         "COMP\n"
   "PROPERTY CS_FIXED_BLOCK_WIDTH 1\n"
   "PROPERTY CS_FIXED_BLOCK_HEIGHT 1\n"
   "PROPERTY CS_FIXED_BLOCK_DEPTH 1\n"
   "DCL BUFFER[0]\n"
   "DCL BUFFER[1]\n"
   "DCL BUFFER[2]\n"
   "DCL CONST[0][0..0]\n"
   "DCL TEMP[0..5]\n"
   "IMM[0] UINT32 {0, 7, 256, 4294967295}\n"
                  /* acc_result = 0;
   * acc_missing = 0;
                  /* if (chain & 1) {
   *    acc_result = buffer[1][0];
   *    acc_missing = buffer[1][1];
   * }
   */
   "AND TEMP[5], CONST[0][0].zzzz, IMM[1].xxxx\n"
   "UIF TEMP[5]\n"
                  /* is_overflow (TEMP[0].z) = (config & 7) >= 2; */
                  /* result_remaining (TEMP[1].x) = (is_overflow && acc_result) ? 0 : result_count; */
                                 /* for (;;) {
   *    if (!result_remaining) {
   *       break;
   *    }
   *    result_remaining--;
   */
   "BGNLOOP\n"
   "  USEQ TEMP[5], TEMP[1].xxxx, IMM[0].xxxx\n"
   "  UIF TEMP[5]\n"
   "     BRK\n"
                  /*    fence = buffer[0]@(base_offset + sizeof(gfx10_sh_query_buffer_mem.stream)); */
                  /*    if (!fence) {
   *       acc_missing = ~0u;
   *       break;
   *    }
   */
   "  USEQ TEMP[5], TEMP[5].xxxx, IMM[0].xxxx\n"
   "  UIF TEMP[5]\n"
   "     MOV TEMP[0].y, TEMP[5].xxxx\n"
                                 /*    if (!(config & 7)) {
   *       acc_result += buffer[0]@stream_offset;
   *    }
   */
   "  AND TEMP[5].x, CONST[0][0].xxxx, IMM[0].yyyy\n"
   "  USEQ TEMP[5], TEMP[5].xxxx, IMM[0].xxxx\n"
   "  UIF TEMP[5]\n"
   "     LOAD TEMP[5].x, BUFFER[0], TEMP[2].xxxx\n"
                  /*    if ((config & 7) >= 2) {
   *       count (TEMP[2].y) = (config & 1) ? 4 : 1;
   */
   "  AND TEMP[5].x, CONST[0][0].xxxx, IMM[0].yyyy\n"
   "  USGE TEMP[5], TEMP[5].xxxx, IMM[1].yyyy\n"
   "  UIF TEMP[5]\n"
                  /*       do {
   *          generated = buffer[0]@(stream_offset + 2 * sizeof(uint64_t));
   *          emitted = buffer[0]@(stream_offset + 3 * sizeof(uint64_t));
   *          if (generated != emitted) {
   *             acc_result = 1;
   *             result_remaining = 0;
   *             break;
   *          }
   *
   *          stream_offset += sizeof(gfx10_sh_query_buffer_mem.stream[0]);
   *       } while (--count);
   *    }
   */
   "     BGNLOOP\n"
   "        UADD TEMP[5].x, TEMP[2].xxxx, IMM[2].xxxx\n"
   "        LOAD TEMP[4].xyzw, BUFFER[0], TEMP[5].xxxx\n"
   "        USNE TEMP[5], TEMP[4].xyxy, TEMP[4].zwzw\n"
   "        UIF TEMP[5]\n"
   "           MOV TEMP[0].x, IMM[1].xxxx\n"
   "           MOV TEMP[1].y, IMM[0].xxxx\n"
                  "        UADD TEMP[2].y, TEMP[2].yyyy, IMM[0].wwww\n"
   "        USEQ TEMP[5], TEMP[2].yyyy, IMM[0].xxxx\n"
   "        UIF TEMP[5]\n"
   "           BRK\n"
   "        ENDIF\n"
   "        UADD TEMP[2].x, TEMP[2].xxxx, IMM[2].yyyy\n"
                  /*    base_offset += sizeof(gfx10_sh_query_buffer_mem);
   * } // end outer loop
   */
                  /* if (chain & 2) {
   *    buffer[2][0] = acc_result;
   *    buffer[2][1] = acc_missing;
   * } else {
   */
   "AND TEMP[5], CONST[0][0].zzzz, IMM[1].yyyy\n"
   "UIF TEMP[5]\n"
                  /*    if ((config & 7) == 1) {
   *       acc_result = acc_missing ? 0 : 1;
   *       acc_missing = 0;
   *    }
   */
   "  AND TEMP[5], CONST[0][0].xxxx, IMM[0].yyyy\n"
   "  USEQ TEMP[5], TEMP[5].xxxx, IMM[1].xxxx\n"
   "  UIF TEMP[5]\n"
   "     UCMP TEMP[0].x, TEMP[0].yyyy, IMM[0].xxxx, IMM[1].xxxx\n"
                  /*    if (!acc_missing) {
   *       buffer[2][0] = acc_result;
   *       if (config & 8) {
   *          buffer[2][1] = 0;
   *       }
   *    }
   * }
   */
   "  USEQ TEMP[5], TEMP[0].yyyy, IMM[0].xxxx\n"
   "  UIF TEMP[5]\n"
   "     STORE BUFFER[2].x, IMM[0].xxxx, TEMP[0].xxxx\n"
   "     AND TEMP[5], CONST[0][0].xxxx, IMM[1].wwww\n"
   "     UIF TEMP[5]\n"
   "        STORE BUFFER[2].x, IMM[1].zzzz, TEMP[0].yyyy\n"
   "     ENDIF\n"
   "  ENDIF\n"
            struct tgsi_token tokens[1024];
            if (!tgsi_text_translate(text_tmpl, tokens, ARRAY_SIZE(tokens))) {
      assert(false);
               state.ir_type = PIPE_SHADER_IR_TGSI;
               }
