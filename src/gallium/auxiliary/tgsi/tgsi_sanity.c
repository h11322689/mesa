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
      #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "cso_cache/cso_hash.h"
   #include "tgsi_sanity.h"
   #include "tgsi_info.h"
   #include "tgsi_iterate.h"
      #include <stdint.h>
      DEBUG_GET_ONCE_BOOL_OPTION(print_sanity, "TGSI_PRINT_SANITY", false)
         typedef struct {
      uint32_t file : 28;
   /* max 2 dimensions */
   uint32_t dimensions : 4;
      } scan_register;
      struct sanity_check_ctx
   {
      struct tgsi_iterate_context iter;
   struct cso_hash regs_decl;
   struct cso_hash regs_used;
            unsigned num_imms;
   unsigned num_instructions;
            unsigned errors;
   unsigned warnings;
   unsigned implied_array_size;
               };
      static inline unsigned
   scan_register_key(const scan_register *reg)
   {
      unsigned key = reg->file;
   key |= (reg->indices[0] << 4);
               }
      static void
   fill_scan_register1d(scan_register *reg,
         {
      reg->file = file;
   reg->dimensions = 1;
   reg->indices[0] = index;
      }
      static void
   fill_scan_register2d(scan_register *reg,
               {
      reg->file = file;
   reg->dimensions = 2;
   reg->indices[0] = index1;
      }
      static void
   scan_register_dst(scan_register *reg,
         {
      if (dst->Register.Dimension) {
      /*FIXME: right now we don't support indirect
   * multidimensional addressing */
   fill_scan_register2d(reg,
                  } else {
      fill_scan_register1d(reg,
               }
      static void
   scan_register_src(scan_register *reg,
         {
      if (src->Register.Dimension) {
      /*FIXME: right now we don't support indirect
   * multidimensional addressing */
   fill_scan_register2d(reg,
                  } else {
      fill_scan_register1d(reg,
               }
      static scan_register *
   create_scan_register_src(struct tgsi_full_src_register *src)
   {
      scan_register *reg = MALLOC(sizeof(scan_register));
               }
      static scan_register *
   create_scan_register_dst(struct tgsi_full_dst_register *dst)
   {
      scan_register *reg = MALLOC(sizeof(scan_register));
               }
      static void
   report_error(
      struct sanity_check_ctx *ctx,
   const char *format,
      {
               if (!ctx->print)
            debug_printf( "Error  : " );
   va_start( args, format );
   _debug_vprintf( format, args );
   va_end( args );
   debug_printf( "\n" );
      }
      static void
   report_warning(
      struct sanity_check_ctx *ctx,
   const char *format,
      {
               if (!ctx->print)
            debug_printf( "Warning: " );
   va_start( args, format );
   _debug_vprintf( format, args );
   va_end( args );
   debug_printf( "\n" );
      }
      static bool
   check_file_name(
      struct sanity_check_ctx *ctx,
      {
      if (file <= TGSI_FILE_NULL || file >= TGSI_FILE_COUNT) {
      report_error( ctx, "(%u): Invalid register file name", file );
      }
      }
      static bool
   is_register_declared(
      struct sanity_check_ctx *ctx,
      {
      void *data = cso_hash_find_data_from_template(
      &ctx->regs_decl, scan_register_key(reg),
         }
      static bool
   is_any_register_declared(
      struct sanity_check_ctx *ctx,
      {
      struct cso_hash_iter iter =
            while (!cso_hash_iter_is_null(iter)) {
      scan_register *reg = (scan_register *)cso_hash_iter_data(iter);
   if (reg->file == file)
                        }
      static bool
   is_register_used(
      struct sanity_check_ctx *ctx,
      {
      void *data = cso_hash_find_data_from_template(
      &ctx->regs_used, scan_register_key(reg),
         }
         static bool
   is_ind_register_used(
      struct sanity_check_ctx *ctx,
      {
         }
      static const char *file_names[TGSI_FILE_COUNT] =
   {
      "NULL",
   "CONST",
   "IN",
   "OUT",
   "TEMP",
   "SAMP",
   "ADDR",
   "IMM",
   "SV",
      };
      static bool
   check_register_usage(
      struct sanity_check_ctx *ctx,
   scan_register *reg,
   const char *name,
      {
      if (!check_file_name( ctx, reg->file )) {
      FREE(reg);
               if (indirect_access) {
      /* Note that 'index' is an offset relative to the value of the
   * address register.  No range checking done here.*/
   reg->indices[0] = 0;
   reg->indices[1] = 0;
   if (!is_any_register_declared( ctx, reg->file ))
         if (!is_ind_register_used(ctx, reg))
         else
      }
   else {
      if (!is_register_declared( ctx, reg )) {
      if (reg->dimensions == 2) {
      report_error( ctx, "%s[%d][%d]: Undeclared %s register", file_names[reg->file],
      }
   else {
      report_error( ctx, "%s[%d]: Undeclared %s register", file_names[reg->file],
         }
   if (!is_register_used( ctx, reg ))
         else
      }
      }
      static bool
   iter_instruction(
      struct tgsi_iterate_context *iter,
      {
      struct sanity_check_ctx *ctx = (struct sanity_check_ctx *) iter;
   const struct tgsi_opcode_info *info;
            if (inst->Instruction.Opcode == TGSI_OPCODE_END) {
      if (ctx->index_of_END != ~0u) {
         }
               info = tgsi_get_opcode_info( inst->Instruction.Opcode );
   if (!info) {
      report_error( ctx, "(%u): Invalid instruction opcode", inst->Instruction.Opcode );
               if (info->num_dst != inst->Instruction.NumDstRegs) {
      report_error( ctx, "%s: Invalid number of destination operands, should be %u",
      }
   if (info->num_src != inst->Instruction.NumSrcRegs) {
      report_error( ctx, "%s: Invalid number of source operands, should be %u",
               /* Check destination and source registers' validity.
   * Mark the registers as used.
   */
   for (i = 0; i < inst->Instruction.NumDstRegs; i++) {
      scan_register *reg = create_scan_register_dst(&inst->Dst[i]);
   check_register_usage(
      ctx,
   reg,
   "destination",
      if (!inst->Dst[i].Register.WriteMask) {
            }
   for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
      scan_register *reg = create_scan_register_src(&inst->Src[i]);
   check_register_usage(
      ctx,
   reg,
   "source",
      if (inst->Src[i].Register.Indirect) {
               fill_scan_register1d(ind_reg,
               check_register_usage(
      ctx,
   ind_reg,
   "indirect",
                           }
      static void
   check_and_declare(struct sanity_check_ctx *ctx,
         {
      if (is_register_declared( ctx, reg))
      report_error( ctx, "%s[%u]: The same register declared more than once",
      cso_hash_insert(&ctx->regs_decl,
            }
         static bool
   iter_declaration(
      struct tgsi_iterate_context *iter,
      {
      struct sanity_check_ctx *ctx = (struct sanity_check_ctx *) iter;
   enum tgsi_file_type file;
            /* No declarations allowed after the first instruction.
   */
   if (ctx->num_instructions > 0)
            /* Check registers' validity.
   * Mark the registers as declared.
   */
   file = decl->Declaration.File;
   if (!check_file_name( ctx, file ))
         for (i = decl->Range.First; i <= decl->Range.Last; i++) {
      /* declared TGSI_FILE_INPUT's for geometry and tessellation
   * have an implied second dimension */
   unsigned processor = ctx->iter.processor.Processor;
   unsigned patch = decl->Semantic.Name == TGSI_SEMANTIC_PATCH ||
      decl->Semantic.Name == TGSI_SEMANTIC_TESSOUTER ||
      if (file == TGSI_FILE_INPUT && !patch && (
            processor == PIPE_SHADER_GEOMETRY ||
   processor == PIPE_SHADER_TESS_CTRL ||
   unsigned vert;
   for (vert = 0; vert < ctx->implied_array_size; ++vert) {
      scan_register *reg = MALLOC(sizeof(scan_register));
   fill_scan_register2d(reg, file, i, vert);
         } else if (file == TGSI_FILE_OUTPUT && !patch &&
            unsigned vert;
   for (vert = 0; vert < ctx->implied_out_array_size; ++vert) {
      scan_register *reg = MALLOC(sizeof(scan_register));
   fill_scan_register2d(reg, file, i, vert);
         } else {
      scan_register *reg = MALLOC(sizeof(scan_register));
   if (decl->Declaration.Dimension) {
         } else {
         }
                     }
      static bool
   iter_immediate(
      struct tgsi_iterate_context *iter,
      {
      struct sanity_check_ctx *ctx = (struct sanity_check_ctx *) iter;
            /* No immediates allowed after the first instruction.
   */
   if (ctx->num_instructions > 0)
            /* Mark the register as declared.
   */
   reg = MALLOC(sizeof(scan_register));
   fill_scan_register1d(reg, TGSI_FILE_IMMEDIATE, ctx->num_imms);
   cso_hash_insert(&ctx->regs_decl, scan_register_key(reg), reg);
            /* Check data type validity.
   */
   if (imm->Immediate.DataType != TGSI_IMM_FLOAT32 &&
      imm->Immediate.DataType != TGSI_IMM_UINT32 &&
   imm->Immediate.DataType != TGSI_IMM_INT32) {
   report_error( ctx, "(%u): Invalid immediate data type", imm->Immediate.DataType );
                  }
         static bool
   iter_property(
      struct tgsi_iterate_context *iter,
      {
               if (iter->processor.Processor == PIPE_SHADER_GEOMETRY &&
      prop->Property.PropertyName == TGSI_PROPERTY_GS_INPUT_PRIM) {
      }
   if (iter->processor.Processor == PIPE_SHADER_TESS_CTRL &&
      prop->Property.PropertyName == TGSI_PROPERTY_TCS_VERTICES_OUT)
         }
      static bool
   prolog(struct tgsi_iterate_context *iter)
   {
      struct sanity_check_ctx *ctx = (struct sanity_check_ctx *) iter;
   if (iter->processor.Processor == PIPE_SHADER_TESS_CTRL ||
      iter->processor.Processor == PIPE_SHADER_TESS_EVAL)
         }
      static bool
   epilog(
         {
               /* There must be an END instruction somewhere.
   */
   if (ctx->index_of_END == ~0u) {
                  /* Check if all declared registers were used.
   */
   {
      struct cso_hash_iter iter =
            while (!cso_hash_iter_is_null(iter)) {
      scan_register *reg = (scan_register *)cso_hash_iter_data(iter);
   if (!is_register_used(ctx, reg) && !is_ind_register_used(ctx, reg)) {
      report_warning( ctx, "%s[%u]: Register never used",
      }
                  /* Print totals, if any.
   */
   if (ctx->errors || ctx->warnings)
               }
      static void
   regs_hash_destroy(struct cso_hash *hash)
   {
      struct cso_hash_iter iter = cso_hash_first_node(hash);
   while (!cso_hash_iter_is_null(iter)) {
      scan_register *reg = (scan_register *)cso_hash_iter_data(iter);
   iter = cso_hash_erase(hash, iter);
   assert(reg->file < TGSI_FILE_COUNT);
      }
      }
      bool
   tgsi_sanity_check(
         {
      struct sanity_check_ctx ctx;
            ctx.iter.prolog = prolog;
   ctx.iter.iterate_instruction = iter_instruction;
   ctx.iter.iterate_declaration = iter_declaration;
   ctx.iter.iterate_immediate = iter_immediate;
   ctx.iter.iterate_property = iter_property;
            cso_hash_init(&ctx.regs_decl);
   cso_hash_init(&ctx.regs_used);
            ctx.num_imms = 0;
   ctx.num_instructions = 0;
            ctx.errors = 0;
   ctx.warnings = 0;
   ctx.implied_array_size = 0;
            retval = tgsi_iterate_shader( tokens, &ctx.iter );
   regs_hash_destroy(&ctx.regs_decl);
   regs_hash_destroy(&ctx.regs_used);
   regs_hash_destroy(&ctx.regs_ind_used);
   if (retval == false)
               }
