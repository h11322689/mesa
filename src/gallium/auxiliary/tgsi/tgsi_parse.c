   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc.
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
      #include "util/u_debug.h"
   #include "pipe/p_shader_tokens.h"
   #include "tgsi_parse.h"
   #include "util/u_memory.h"
      unsigned
   tgsi_parse_init(
      struct tgsi_parse_context *ctx,
      {
      ctx->FullHeader.Header = *(struct tgsi_header *) &tokens[0];
   if (ctx->FullHeader.Header.HeaderSize >= 2) {
         }
   else {
                  ctx->Tokens = tokens;
               }
      void
   tgsi_parse_free(
         {
   }
      bool
   tgsi_parse_end_of_tokens(
         {
      /* All values involved are unsigned, but the sum will be promoted to
   * a signed value (at least on 64 bit). To capture a possible overflow
   * make it a signed comparison.
   */
      ctx->FullHeader.Header.HeaderSize + ctx->FullHeader.Header.BodySize;
   }
         /**
   * This function is used to avoid and work-around type punning/aliasing
   * warnings.  The warnings seem harmless on x86 but on PPC they cause
   * real failures.
   */
   static inline void
   copy_token(void *dst, const void *src)
   {
         }
         /**
   * Get next 4-byte token, return it at address specified by 'token'
   */
   static void
   next_token(
      struct tgsi_parse_context *ctx,
      {
      assert( !tgsi_parse_end_of_tokens( ctx ) );
   copy_token(token, &ctx->Tokens[ctx->Position]);
      }
         void
   tgsi_parse_token(
         {
      struct tgsi_token token;
                     switch( token.Type ) {
   case TGSI_TOKEN_TYPE_DECLARATION:
   {
               memset(decl, 0, sizeof *decl);
                     if (decl->Declaration.Dimension) {
                  if (decl->Declaration.Interpolate) {
                  if (decl->Declaration.Semantic) {
                  if (decl->Declaration.File == TGSI_FILE_IMAGE) {
                  if (decl->Declaration.File == TGSI_FILE_SAMPLER_VIEW) {
                  if (decl->Declaration.Array) {
                              case TGSI_TOKEN_TYPE_IMMEDIATE:
   {
      struct tgsi_full_immediate *imm = &ctx->FullToken.FullImmediate;
            memset(imm, 0, sizeof *imm);
                     switch (imm->Immediate.DataType) {
   case TGSI_IMM_FLOAT32:
   case TGSI_IMM_FLOAT64:
      for (i = 0; i < imm_count; i++) {
                     case TGSI_IMM_UINT32:
   case TGSI_IMM_UINT64:
      for (i = 0; i < imm_count; i++) {
                     case TGSI_IMM_INT32:
   case TGSI_IMM_INT64:
      for (i = 0; i < imm_count; i++) {
                     default:
                              case TGSI_TOKEN_TYPE_INSTRUCTION:
   {
               memset(inst, 0, sizeof *inst);
            if (inst->Instruction.Label) {
                  if (inst->Instruction.Texture) {
      next_token( ctx, &inst->Texture);
   for (i = 0; i < inst->Texture.NumOffsets; i++) {
                     if (inst->Instruction.Memory) {
                                                                                 /*
   * No support for multi-dimensional addressing.
                  if (inst->Dst[i].Dimension.Indirect)
                                                                                 /*
   * No support for multi-dimensional addressing.
                  if (inst->Src[i].Dimension.Indirect)
                              case TGSI_TOKEN_TYPE_PROPERTY:
   {
      struct tgsi_full_property *prop = &ctx->FullToken.FullProperty;
            memset(prop, 0, sizeof *prop);
            prop_count = prop->Property.NrTokens - 1;
   for (i = 0; i < prop_count; i++) {
                              default:
            }
               /**
   * Make a new copy of a token array.
   */
   struct tgsi_token *
   tgsi_dup_tokens(const struct tgsi_token *tokens)
   {
      unsigned n = tgsi_num_tokens(tokens);
   unsigned bytes = n * sizeof(struct tgsi_token);
   struct tgsi_token *new_tokens = (struct tgsi_token *) MALLOC(bytes);
   if (new_tokens)
            }
         /**
   * Allocate memory for num_tokens tokens.
   */
   struct tgsi_token *
   tgsi_alloc_tokens(unsigned num_tokens)
   {
      unsigned bytes = num_tokens * sizeof(struct tgsi_token);
      }
         /**
   * Free tokens allocated by tgsi_alloc_tokens() or tgsi_dup_tokens()
   */
   void
   tgsi_free_tokens(const struct tgsi_token *tokens)
   {
         }
         unsigned
   tgsi_get_processor_type(const struct tgsi_token *tokens)
   {
               if (tgsi_parse_init( &parse, tokens ) != TGSI_PARSE_OK) {
      debug_printf("tgsi_parse_init() failed in %s:%i!\n", __func__, __LINE__);
      }
      }
