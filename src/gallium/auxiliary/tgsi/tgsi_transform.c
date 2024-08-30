   /**************************************************************************
   * 
   * Copyright 2008 VMware, Inc.
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
      /**
   * TGSI program transformation utility.
   *
   * Authors:  Brian Paul
   */
      #include "util/u_debug.h"
   #include "util/log.h"
      #include "tgsi_transform.h"
      /**
   * Increments the next-token index if the tgsi_build_* succeeded, or extends the
   * token array and returns true to request a re-emit of the tgsi_build_* by the
   * caller.
   */
   static bool
   need_re_emit(struct tgsi_transform_context *ctx, uint32_t emitted, struct tgsi_header orig_header)
   {
      if (emitted > 0) {
      ctx->ti += emitted;
      } else {
      uint32_t new_len = ctx->max_tokens_out * 2;
   if (new_len < ctx->max_tokens_out) {
      ctx->fail = true;
               struct tgsi_token *new_tokens = tgsi_alloc_tokens(new_len);
   if (!new_tokens) {
      ctx->fail = true;
      }
            tgsi_free_tokens(ctx->tokens_out);
   ctx->tokens_out = new_tokens;
            /* Point the header at the resized tokens. */
   ctx->header = (struct tgsi_header *)new_tokens;
   /* The failing emit may have incremented header/body size, reset it to its state before our attempt. */
                  }
      static void
   emit_instruction(struct tgsi_transform_context *ctx,
         {
      uint32_t emitted;
            do {
      emitted = tgsi_build_full_instruction(inst,
                     }
         static void
   emit_declaration(struct tgsi_transform_context *ctx,
         {
      uint32_t emitted;
            do {
      emitted = tgsi_build_full_declaration(decl,
                     }
         static void
   emit_immediate(struct tgsi_transform_context *ctx,
         {
      uint32_t emitted;
            do {
      emitted = tgsi_build_full_immediate(imm,
                     }
         static void
   emit_property(struct tgsi_transform_context *ctx,
         {
      uint32_t emitted;
            do {
      emitted = tgsi_build_full_property(prop,
                     }
         /**
   * Apply user-defined transformations to the input shader to produce
   * the output shader.
   * For example, a register search-and-replace operation could be applied
   * by defining a transform_instruction() callback that examined and changed
   * the instruction src/dest regs.
   *
   * \return new tgsi tokens, or NULL on failure
   */
   struct tgsi_token *
   tgsi_transform_shader(const struct tgsi_token *tokens_in,
               {
      bool first_instruction = true;
   bool epilog_emitted = false;
   int cond_stack = 0;
            /* input shader */
            /* output shader */
            /* Always include space for the header. */
            /**
   ** callback context init
   **/
   ctx->emit_instruction = emit_instruction;
   ctx->emit_declaration = emit_declaration;
   ctx->emit_immediate = emit_immediate;
   ctx->emit_property = emit_property;
   ctx->tokens_out = tgsi_alloc_tokens(initial_tokens_len);
   ctx->max_tokens_out = initial_tokens_len;
            if (!ctx->tokens_out) {
      mesa_loge("failed to allocate %d tokens\n", initial_tokens_len);
               /**
   ** Setup to begin parsing input shader
   **/
   if (tgsi_parse_init( &parse, tokens_in ) != TGSI_PARSE_OK) {
      debug_printf("tgsi_parse_init() failed in tgsi_transform_shader()!\n");
      }
            /**
   **  Setup output shader
   **/
   ctx->header = (struct tgsi_header *)ctx->tokens_out;
            processor = (struct tgsi_processor *) (ctx->tokens_out + 1);
                        /**
   ** Loop over incoming program tokens/instructions
   */
                        switch( parse.FullToken.Token.Type ) {
   case TGSI_TOKEN_TYPE_INSTRUCTION:
      {
      struct tgsi_full_instruction *fullinst
                  if (first_instruction && ctx->prolog) {
                  /*
   * XXX Note: we handle the case of ret in main.
   * However, the output redirections done by transform
   * have their limits with control flow and will generally
   * not work correctly. e.g.
   * if (cond) {
   *    oColor = x;
   *    ret;
   * }
   * oColor = y;
   * end;
   * If the color output is redirected to a temp and modified
   * by a transform, this will not work (the oColor assignment
   * in the conditional will never make it to the actual output).
   */
   if ((opcode == TGSI_OPCODE_END || opcode == TGSI_OPCODE_RET) &&
      call_stack == 0 && ctx->epilog && !epilog_emitted) {
   if (opcode == TGSI_OPCODE_RET && cond_stack != 0) {
         } else {
      assert(cond_stack == 0);
   /* Emit caller's epilog */
   ctx->epilog(ctx);
      }
   /* Emit END (or RET) */
      }
   else {
      switch (opcode) {
   case TGSI_OPCODE_IF:
   case TGSI_OPCODE_UIF:
   case TGSI_OPCODE_SWITCH:
   case TGSI_OPCODE_BGNLOOP:
      cond_stack++;
      case TGSI_OPCODE_CAL:
      call_stack++;
      case TGSI_OPCODE_ENDIF:
   case TGSI_OPCODE_ENDSWITCH:
   case TGSI_OPCODE_ENDLOOP:
      assert(cond_stack > 0);
   cond_stack--;
      case TGSI_OPCODE_ENDSUB:
      assert(call_stack > 0);
   call_stack--;
      case TGSI_OPCODE_BGNSUB:
   case TGSI_OPCODE_RET:
   default:
         }
   if (ctx->transform_instruction)
                                          case TGSI_TOKEN_TYPE_DECLARATION:
      {
                     if (ctx->transform_declaration)
         else
                  case TGSI_TOKEN_TYPE_IMMEDIATE:
      {
                     if (ctx->transform_immediate)
         else
      }
      case TGSI_TOKEN_TYPE_PROPERTY:
      {
                     if (ctx->transform_property)
         else
                  default:
            }
                     if (ctx->fail) {
      tgsi_free_tokens(ctx->tokens_out);
                  }
         #include "tgsi_text.h"
      extern int tgsi_transform_foo( struct tgsi_token *tokens_out,
            /* This function exists only so that tgsi_text_translate() doesn't get
   * magic-ed out of the libtgsi.a archive by the build system.  Don't
   * remove unless you know this has been fixed - check on mingw/scons
   * builds as well.
   */
   int
   tgsi_transform_foo( struct tgsi_token *tokens_out,
         {
      const char *text = 
      "FRAG\n"
   "DCL IN[0], COLOR, CONSTANT\n"
   "DCL OUT[0], COLOR\n"
   "  0: MOV OUT[0], IN[0]\n"
   "  1: END";
      return tgsi_text_translate( text,
            }
