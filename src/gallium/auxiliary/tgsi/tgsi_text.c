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
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/strtod.h"
   #include "tgsi_text.h"
   #include "tgsi_build.h"
   #include "tgsi_info.h"
   #include "tgsi_parse.h"
   #include "tgsi_sanity.h"
   #include "tgsi_strings.h"
   #include "tgsi_util.h"
   #include "tgsi_dump.h"
      static bool is_alpha_underscore( const char *cur )
   {
      return
      (*cur >= 'a' && *cur <= 'z') ||
   (*cur >= 'A' && *cur <= 'Z') ||
   }
      static bool is_digit( const char *cur )
   {
         }
      static bool is_digit_alpha_underscore( const char *cur )
   {
         }
      static char uprcase( char c )
   {
      if (c >= 'a' && c <= 'z')
            }
      /*
   * Ignore case of str1 and assume str1 is already uppercase.
   * Return TRUE iff str1 and str2 are equal.
   */
   static int
   streq_nocase_uprcase(const char *str1,
         {
      while (*str1 && *str2) {
      if (*str1 != uprcase(*str2))
         str1++;
      }
      }
      /* Return TRUE if both strings match.
   * The second string is terminated by zero.
   * The pointer to the first string is moved at end of the read word
   * on success.
   */
   static bool str_match_no_case( const char **pcur, const char *str )
   {
               while (*str != '\0' && *str == uprcase( *cur )) {
      str++;
      }
   if (*str == '\0') {
      *pcur = cur;
      }
      }
      /* Return TRUE if both strings match.
   * The first string is be terminated by a non-digit non-letter non-underscore
   * character, the second string is terminated by zero.
   * The pointer to the first string is moved at end of the read word
   * on success.
   */
   static bool str_match_nocase_whole( const char **pcur, const char *str )
   {
               if (str_match_no_case(&cur, str) &&
      !is_digit_alpha_underscore(cur)) {
   *pcur = cur;
      }
      }
      /* Return the array index that matches starting at *pcur, where the string at
   * *pcur is terminated by a non-digit non-letter non-underscore.
   * Returns -1 if no match is found.
   *
   * On success, the pointer to the first string is moved to the end of the read
   * word.
   */
   static int str_match_name_from_array(const char **pcur,
               {
      for (unsigned j = 0; j < array_size; ++j) {
      if (str_match_nocase_whole(pcur, array[j]))
      }
      }
      /* Return the format corresponding to the name at *pcur.
   * Returns -1 if there is no format name.
   *
   * On success, the pointer to the string is moved to the end of the read format
   * name.
   */
   static int str_match_format(const char **pcur)
   {
      for (unsigned i = 0; i < PIPE_FORMAT_COUNT; i++) {
      const struct util_format_description *desc =
         if (str_match_nocase_whole(pcur, desc->name)) {
            }
      }
      /* Eat zero or more whitespaces.
   */
   static void eat_opt_white( const char **pcur )
   {
      while (**pcur == ' ' || **pcur == '\t' || **pcur == '\n')
      }
      /* Eat one or more whitespaces.
   * Return TRUE if at least one whitespace eaten.
   */
   static bool eat_white( const char **pcur )
   {
               eat_opt_white( pcur );
      }
      /* Parse unsigned integer.
   * No checks for overflow.
   */
   static bool parse_uint( const char **pcur, unsigned *val )
   {
               if (is_digit( cur )) {
      *val = *cur++ - '0';
   while (is_digit( cur ))
         *pcur = cur;
      }
      }
      static bool parse_int( const char **pcur, int *val )
   {
      const char *cur = *pcur;
            if (*cur == '+' || *cur == '-')
            if (parse_uint(&cur, (unsigned *)val)) {
      *val *= sign;
   *pcur = cur;
                  }
      static bool parse_identifier( const char **pcur, char *ret, size_t len )
   {
      const char *cur = *pcur;
   size_t i = 0;
   if (is_alpha_underscore( cur )) {
      ret[i++] = *cur++;
   while (is_alpha_underscore( cur ) || is_digit( cur )) {
      if (i == len - 1)
            }
   ret[i++] = '\0';
   *pcur = cur;
      }
      }
      /* Parse floating point.
   */
   static bool parse_float( const char **pcur, float *val )
   {
      const char *cur = *pcur;
   *val = _mesa_strtof(cur, (char**)pcur);
   if (*pcur == cur)
            }
      static bool parse_double( const char **pcur, uint32_t *val0, uint32_t *val1)
   {
      const char *cur = *pcur;
   union {
      double dval;
               v.dval = _mesa_strtod(cur, (char**)pcur);
   if (*pcur == cur)
            *val0 = v.uval[0];
               }
      static bool parse_int64( const char **pcur, uint32_t *val0, uint32_t *val1)
   {
      const char *cur = *pcur;
   union {
      int64_t i64val;
               v.i64val = strtoll(cur, (char**)pcur, 0);
   if (*pcur == cur)
            *val0 = v.uval[0];
               }
      static bool parse_uint64( const char **pcur, uint32_t *val0, uint32_t *val1)
   {
      const char *cur = *pcur;
   union {
      uint64_t u64val;
               v.u64val = strtoull(cur, (char**)pcur, 0);
   if (*pcur == cur)
            *val0 = v.uval[0];
               }
      struct translate_ctx
   {
      const char *text;
   const char *cur;
   struct tgsi_token *tokens;
   struct tgsi_token *tokens_cur;
   struct tgsi_token *tokens_end;
   struct tgsi_header *header;
   unsigned processor : 4;
   unsigned implied_array_size : 6;
      };
      static void report_error( struct translate_ctx *ctx, const char *msg )
   {
      int line = 1;
   int column = 1;
            while (itr != ctx->cur) {
      if (*itr == '\n') {
      column = 1;
      }
   ++column;
                  }
      /* Parse shader header.
   * Return TRUE for one of the following headers.
   *    FRAG
   *    GEOM
   *    VERT
   */
   static bool parse_header( struct translate_ctx *ctx )
   {
               if (str_match_nocase_whole( &ctx->cur, "FRAG" ))
         else if (str_match_nocase_whole( &ctx->cur, "VERT" ))
         else if (str_match_nocase_whole( &ctx->cur, "GEOM" ))
         else if (str_match_nocase_whole( &ctx->cur, "TESS_CTRL" ))
         else if (str_match_nocase_whole( &ctx->cur, "TESS_EVAL" ))
         else if (str_match_nocase_whole( &ctx->cur, "COMP" ))
         else {
      report_error( ctx, "Unknown header" );
               if (ctx->tokens_cur >= ctx->tokens_end)
         ctx->header = (struct tgsi_header *) ctx->tokens_cur++;
            if (ctx->tokens_cur >= ctx->tokens_end)
         *(struct tgsi_processor *) ctx->tokens_cur++ = tgsi_build_processor( processor, ctx->header );
               }
      static bool parse_label( struct translate_ctx *ctx, unsigned *val )
   {
               if (parse_uint( &cur, val )) {
      eat_opt_white( &cur );
   if (*cur == ':') {
      cur++;
   ctx->cur = cur;
         }
      }
      static bool
   parse_file( const char **pcur, enum tgsi_file_type *file )
   {
               for (i = 0; i < TGSI_FILE_COUNT; i++) {
               if (str_match_nocase_whole( &cur, tgsi_file_name(i) )) {
      *pcur = cur;
   *file = i;
         }
      }
      static bool
   parse_opt_writemask(
      struct translate_ctx *ctx,
      {
               cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == '.') {
      cur++;
   *writemask = TGSI_WRITEMASK_NONE;
   eat_opt_white( &cur );
   if (uprcase( *cur ) == 'X') {
      cur++;
      }
   if (uprcase( *cur ) == 'Y') {
      cur++;
      }
   if (uprcase( *cur ) == 'Z') {
      cur++;
      }
   if (uprcase( *cur ) == 'W') {
      cur++;
               if (*writemask == TGSI_WRITEMASK_NONE) {
      report_error( ctx, "Writemask expected" );
                  }
   else {
         }
      }
         /* <register_file_bracket> ::= <file> `['
   */
   static bool
   parse_register_file_bracket(
      struct translate_ctx *ctx,
      {
      if (!parse_file( &ctx->cur, file )) {
      report_error( ctx, "Unknown register file" );
      }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != '[') {
      report_error( ctx, "Expected `['" );
      }
   ctx->cur++;
      }
      /* <register_file_bracket_index> ::= <register_file_bracket> <uint>
   */
   static bool
   parse_register_file_bracket_index(
      struct translate_ctx *ctx,
   enum tgsi_file_type *file,
      {
               if (!parse_register_file_bracket( ctx, file ))
         eat_opt_white( &ctx->cur );
   if (!parse_uint( &ctx->cur, &uindex )) {
      report_error( ctx, "Expected literal unsigned integer" );
      }
   *index = (int) uindex;
      }
      /* Parse simple 1d register operand.
   *    <register_dst> ::= <register_file_bracket_index> `]'
   */
   static bool
   parse_register_1d(struct translate_ctx *ctx,
               {
      if (!parse_register_file_bracket_index( ctx, file, index ))
         eat_opt_white( &ctx->cur );
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]'" );
      }
   ctx->cur++;
      }
      struct parsed_bracket {
               enum tgsi_file_type ind_file;
   int ind_index;
   unsigned ind_comp;
      };
         static bool
   parse_register_bracket(
      struct translate_ctx *ctx,
      {
      const char *cur;
                              cur = ctx->cur;
   if (parse_file( &cur, &brackets->ind_file )) {
      if (!parse_register_1d( ctx, &brackets->ind_file,
                        if (*ctx->cur == '.') {
                     switch (uprcase(*ctx->cur)) {
   case 'X':
      brackets->ind_comp = TGSI_SWIZZLE_X;
      case 'Y':
      brackets->ind_comp = TGSI_SWIZZLE_Y;
      case 'Z':
      brackets->ind_comp = TGSI_SWIZZLE_Z;
      case 'W':
      brackets->ind_comp = TGSI_SWIZZLE_W;
      default:
      report_error(ctx, "Expected indirect register swizzle component `x', `y', `z' or `w'");
      }
   ctx->cur++;
               if (*ctx->cur == '+' || *ctx->cur == '-')
         else
      }
   else {
      if (!parse_uint( &ctx->cur, &uindex )) {
      report_error( ctx, "Expected literal unsigned integer" );
      }
   brackets->index = (int) uindex;
   brackets->ind_file = TGSI_FILE_NULL;
      }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]'" );
      }
   ctx->cur++;
   if (*ctx->cur == '(') {
      ctx->cur++;
   eat_opt_white( &ctx->cur );
   if (!parse_uint( &ctx->cur, &brackets->ind_array )) {
      report_error( ctx, "Expected literal unsigned integer" );
      }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != ')') {
      report_error( ctx, "Expected `)'" );
      }
      }
      }
      static bool
   parse_opt_register_src_bracket(
      struct translate_ctx *ctx,
   struct parsed_bracket *brackets,
      {
                        eat_opt_white( &cur );
   if (cur[0] == '[') {
      ++cur;
            if (!parse_register_bracket(ctx, brackets))
                           }
         /* Parse source register operand.
   *    <register_src> ::= <register_file_bracket_index> `]' |
   *                       <register_file_bracket> <register_dst> [`.' (`x' | `y' | `z' | `w')] `]' |
   *                       <register_file_bracket> <register_dst> [`.' (`x' | `y' | `z' | `w')] `+' <uint> `]' |
   *                       <register_file_bracket> <register_dst> [`.' (`x' | `y' | `z' | `w')] `-' <uint> `]'
   */
   static bool
   parse_register_src(
      struct translate_ctx *ctx,
   enum tgsi_file_type *file,
      {
      brackets->ind_comp = TGSI_SWIZZLE_X;
   if (!parse_register_file_bracket( ctx, file ))
         if (!parse_register_bracket( ctx, brackets ))
               }
      struct parsed_dcl_bracket {
      unsigned first;
      };
      static bool
   parse_register_dcl_bracket(
      struct translate_ctx *ctx,
      {
      unsigned uindex;
                     if (!parse_uint( &ctx->cur, &uindex )) {
      /* it can be an empty bracket [] which means its range
   * is from 0 to some implied size */
   if (ctx->cur[0] == ']' && ctx->implied_array_size != 0) {
      bracket->first = 0;
   bracket->last = ctx->implied_array_size - 1;
      }
   report_error( ctx, "Expected literal unsigned integer" );
      }
                     if (ctx->cur[0] == '.' && ctx->cur[1] == '.') {
               ctx->cur += 2;
   eat_opt_white( &ctx->cur );
   if (!parse_uint( &ctx->cur, &uindex )) {
      report_error( ctx, "Expected literal integer" );
      }
   bracket->last = (int) uindex;
      }
   else {
               cleanup:
      if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]' or `..'" );
      }
   ctx->cur++;
      }
      /* Parse register declaration.
   *    <register_dcl> ::= <register_file_bracket_index> `]' |
   *                       <register_file_bracket_index> `..' <index> `]'
   */
   static bool
   parse_register_dcl(
      struct translate_ctx *ctx,
   enum tgsi_file_type *file,
   struct parsed_dcl_bracket *brackets,
      {
                        if (!parse_register_file_bracket( ctx, file ))
         if (!parse_register_dcl_bracket( ctx, &brackets[0] ))
                     cur = ctx->cur;
            if (cur[0] == '[') {
      bool is_in = *file == TGSI_FILE_INPUT;
            ++cur;
   ctx->cur = cur;
   if (!parse_register_dcl_bracket( ctx, &brackets[1] ))
         /* for geometry shader we don't really care about
   * the first brackets it's always the size of the
   * input primitive. so we want to declare just
   * the index relevant to the semantics which is in
            /* tessellation has similar constraints to geometry shader */
   if ((ctx->processor == PIPE_SHADER_GEOMETRY && is_in) ||
      (ctx->processor == PIPE_SHADER_TESS_EVAL && is_in) ||
   (ctx->processor == PIPE_SHADER_TESS_CTRL && (is_in || is_out))) {
   brackets[0] = brackets[1];
      } else {
                        }
         /* Parse destination register operand.*/
   static bool
   parse_register_dst(
      struct translate_ctx *ctx,
   enum tgsi_file_type *file,
      {
      brackets->ind_comp = TGSI_SWIZZLE_X;
   if (!parse_register_file_bracket( ctx, file ))
         if (!parse_register_bracket( ctx, brackets ))
               }
      static bool
   parse_dst_operand(
      struct translate_ctx *ctx,
      {
      enum tgsi_file_type file;
   unsigned writemask;
   const char *cur;
   struct parsed_bracket bracket[2];
            if (!parse_register_dst( ctx, &file, &bracket[0] ))
         if (!parse_opt_register_src_bracket(ctx, &bracket[1], &parsed_opt_brackets))
            cur = ctx->cur;
            if (!parse_opt_writemask( ctx, &writemask ))
            dst->Register.File = file;
   if (parsed_opt_brackets) {
      dst->Register.Dimension = 1;
   dst->Dimension.Indirect = 0;
   dst->Dimension.Dimension = 0;
            if (bracket[0].ind_file != TGSI_FILE_NULL) {
      dst->Dimension.Indirect = 1;
   dst->DimIndirect.File = bracket[0].ind_file;
   dst->DimIndirect.Index = bracket[0].ind_index;
   dst->DimIndirect.Swizzle = bracket[0].ind_comp;
      }
      }
   dst->Register.Index = bracket[0].index;
   dst->Register.WriteMask = writemask;
   if (bracket[0].ind_file != TGSI_FILE_NULL) {
      dst->Register.Indirect = 1;
   dst->Indirect.File = bracket[0].ind_file;
   dst->Indirect.Index = bracket[0].ind_index;
   dst->Indirect.Swizzle = bracket[0].ind_comp;
      }
      }
      static bool
   parse_optional_swizzle(
      struct translate_ctx *ctx,
   unsigned *swizzle,
   bool *parsed_swizzle,
      {
                        eat_opt_white( &cur );
   if (*cur == '.') {
               cur++;
   eat_opt_white( &cur );
   for (i = 0; i < components; i++) {
      if (uprcase( *cur ) == 'X')
         else if (uprcase( *cur ) == 'Y')
         else if (uprcase( *cur ) == 'Z')
         else if (uprcase( *cur ) == 'W')
         report_error( ctx, "Expected register swizzle component `x', `y', `z' or `w'" );
   return false;
         }
      }
   *parsed_swizzle = true;
      }
      }
      static bool
   parse_src_operand(
      struct translate_ctx *ctx,
      {
      enum tgsi_file_type file;
   unsigned swizzle[4];
   bool parsed_swizzle;
   struct parsed_bracket bracket[2];
            if (*ctx->cur == '-') {
      ctx->cur++;
   eat_opt_white( &ctx->cur );
               if (*ctx->cur == '|') {
      ctx->cur++;
   eat_opt_white( &ctx->cur );
               if (!parse_register_src(ctx, &file, &bracket[0]))
         if (!parse_opt_register_src_bracket(ctx, &bracket[1], &parsed_opt_brackets))
            src->Register.File = file;
   if (parsed_opt_brackets) {
      src->Register.Dimension = 1;
   src->Dimension.Indirect = 0;
   src->Dimension.Dimension = 0;
   src->Dimension.Index = bracket[0].index;
   if (bracket[0].ind_file != TGSI_FILE_NULL) {
      src->Dimension.Indirect = 1;
   src->DimIndirect.File = bracket[0].ind_file;
   src->DimIndirect.Index = bracket[0].ind_index;
   src->DimIndirect.Swizzle = bracket[0].ind_comp;
      }
      }
   src->Register.Index = bracket[0].index;
   if (bracket[0].ind_file != TGSI_FILE_NULL) {
      src->Register.Indirect = 1;
   src->Indirect.File = bracket[0].ind_file;
   src->Indirect.Index = bracket[0].ind_index;
   src->Indirect.Swizzle = bracket[0].ind_comp;
               /* Parse optional swizzle.
   */
   if (parse_optional_swizzle( ctx, swizzle, &parsed_swizzle, 4 )) {
      if (parsed_swizzle) {
      src->Register.SwizzleX = swizzle[0];
   src->Register.SwizzleY = swizzle[1];
   src->Register.SwizzleZ = swizzle[2];
                  if (src->Register.Absolute) {
      eat_opt_white( &ctx->cur );
   if (*ctx->cur != '|') {
      report_error( ctx, "Expected `|'" );
      }
                     }
      static bool
   parse_texoffset_operand(
      struct translate_ctx *ctx,
      {
      enum tgsi_file_type file;
   unsigned swizzle[3];
   bool parsed_swizzle;
            if (!parse_register_src(ctx, &file, &bracket))
            src->File = file;
            /* Parse optional swizzle.
   */
   if (parse_optional_swizzle( ctx, swizzle, &parsed_swizzle, 3 )) {
      if (parsed_swizzle) {
      src->SwizzleX = swizzle[0];
   src->SwizzleY = swizzle[1];
                     }
      static bool
   match_inst(const char **pcur,
            unsigned *saturate,
      {
      const char *cur = *pcur;
            /* simple case: the whole string matches the instruction name */
   if (str_match_nocase_whole(&cur, mnemonic)) {
      *pcur = cur;
   *saturate = 0;
   *precise = 0;
               if (str_match_no_case(&cur, mnemonic)) {
      /* the instruction has a suffix, figure it out */
   if (str_match_no_case(&cur, "_SAT")) {
      *pcur = cur;
               if (str_match_no_case(&cur, "_PRECISE")) {
      *pcur = cur;
               if (!is_digit_alpha_underscore(cur))
                  }
      static bool
   parse_instruction(
      struct translate_ctx *ctx,
      {
      int i;
   unsigned saturate = 0;
   unsigned precise = 0;
   const struct tgsi_opcode_info *info;
   struct tgsi_full_instruction inst;
   const char *cur;
                     /* Parse instruction name.
   */
   eat_opt_white( &ctx->cur );
   for (i = 0; i < TGSI_OPCODE_LAST; i++) {
               info = tgsi_get_opcode_info( i );
   if (match_inst(&cur, &saturate, &precise, info)) {
      if (info->num_dst + info->num_src + info->is_tex == 0) {
      ctx->cur = cur;
      }
   else if (*cur == '\0' || eat_white( &cur )) {
      ctx->cur = cur;
            }
   if (i == TGSI_OPCODE_LAST) {
      if (has_label)
         else
                     inst.Instruction.Opcode = i;
   inst.Instruction.Saturate = saturate;
   inst.Instruction.Precise = precise;
   inst.Instruction.NumDstRegs = info->num_dst;
            if (i >= TGSI_OPCODE_SAMPLE && i <= TGSI_OPCODE_GATHER4) {
      /*
   * These are not considered tex opcodes here (no additional
   * target argument) however we're required to set the Texture
   * bit so we can set the number of tex offsets.
   */
   inst.Instruction.Texture = 1;
               if ((i >= TGSI_OPCODE_LOAD && i <= TGSI_OPCODE_ATOMIMAX) ||
      i == TGSI_OPCODE_RESQ) {
   inst.Instruction.Memory = 1;
               assume(info->num_dst <= TGSI_FULL_MAX_DST_REGISTERS);
            /* Parse instruction operands.
   */
   for (i = 0; i < info->num_dst + info->num_src + info->is_tex; i++) {
      if (i > 0) {
      eat_opt_white( &ctx->cur );
   if (*ctx->cur != ',') {
      report_error( ctx, "Expected `,'" );
      }
   ctx->cur++;
               if (i < info->num_dst) {
      if (!parse_dst_operand( ctx, &inst.Dst[i] ))
      }
   else if (i < info->num_dst + info->num_src) {
      if (!parse_src_operand( ctx, &inst.Src[i - info->num_dst] ))
      }
   else {
               for (j = 0; j < TGSI_TEXTURE_COUNT; j++) {
      if (str_match_nocase_whole( &ctx->cur, tgsi_texture_names[j] )) {
      inst.Instruction.Texture = 1;
   inst.Texture.Texture = j;
         }
   if (j == TGSI_TEXTURE_COUNT) {
      report_error( ctx, "Expected texture target" );
                     cur = ctx->cur;
   eat_opt_white( &cur );
   for (i = 0; inst.Instruction.Texture && *cur == ',' && i < TGSI_FULL_MAX_TEX_OFFSETS; i++) {
         cur++;
   eat_opt_white( &cur );
   ctx->cur = cur;
   if (!parse_texoffset_operand( ctx, &inst.TexOffsets[i] ))
         cur = ctx->cur;
   }
            cur = ctx->cur;
            for (; inst.Instruction.Memory && *cur == ',';
      ctx->cur = cur, eat_opt_white(&cur)) {
            cur++;
            j = str_match_name_from_array(&cur, tgsi_memory_names,
         if (j >= 0) {
      inst.Memory.Qualifier |= 1U << j;
               j = str_match_name_from_array(&cur, tgsi_texture_names,
         if (j >= 0) {
      inst.Memory.Texture = j;
               j = str_match_format(&cur);
   if (j >= 0) {
      inst.Memory.Format = j;
               ctx->cur = cur;
   report_error(ctx, "Expected memory qualifier, texture target, or format\n");
               cur = ctx->cur;
   eat_opt_white( &cur );
   if (info->is_branch && *cur == ':') {
               cur++;
   eat_opt_white( &cur );
   if (!parse_uint( &cur, &target )) {
      report_error( ctx, "Expected a label" );
      }
   inst.Instruction.Label = 1;
   inst.Label.Label = target;
               advance = tgsi_build_full_instruction(
      &inst,
   ctx->tokens_cur,
   ctx->header,
      if (advance == 0)
                     }
      /* parses a 4-touple of the form {x, y, z, w}
   * where x, y, z, w are numbers */
   static bool parse_immediate_data(struct translate_ctx *ctx, unsigned type,
         {
      unsigned i;
            eat_opt_white( &ctx->cur );
   if (*ctx->cur != '{') {
      report_error( ctx, "Expected `{'" );
      }
   ctx->cur++;
   for (i = 0; i < 4; i++) {
      eat_opt_white( &ctx->cur );
   if (i > 0) {
      if (*ctx->cur != ',') {
      report_error( ctx, "Expected `,'" );
      }
   ctx->cur++;
               switch (type) {
   case TGSI_IMM_FLOAT64:
      ret = parse_double(&ctx->cur, &values[i].Uint, &values[i+1].Uint);
   i++;
      case TGSI_IMM_INT64:
      ret = parse_int64(&ctx->cur, &values[i].Uint, &values[i+1].Uint);
   i++;
      case TGSI_IMM_UINT64:
      ret = parse_uint64(&ctx->cur, &values[i].Uint, &values[i+1].Uint);
   i++;
      case TGSI_IMM_FLOAT32:
      ret = parse_float(&ctx->cur, &values[i].Float);
      case TGSI_IMM_UINT32:
      ret = parse_uint(&ctx->cur, &values[i].Uint);
      case TGSI_IMM_INT32:
      ret = parse_int(&ctx->cur, &values[i].Int);
      default:
      assert(0);
   ret = false;
               if (!ret) {
      report_error( ctx, "Expected immediate constant" );
         }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != '}') {
      report_error( ctx, "Expected `}'" );
      }
               }
      static bool parse_declaration( struct translate_ctx *ctx )
   {
      struct tgsi_full_declaration decl;
   enum tgsi_file_type file;
   struct parsed_dcl_bracket brackets[2];
   int num_brackets;
   unsigned writemask;
   const char *cur, *cur2;
   unsigned advance;
            if (!eat_white( &ctx->cur )) {
      report_error( ctx, "Syntax error" );
      }
   if (!parse_register_dcl( ctx, &file, brackets, &num_brackets))
         if (!parse_opt_writemask( ctx, &writemask ))
            decl = tgsi_default_full_declaration();
   decl.Declaration.File = file;
            if (num_brackets == 1) {
      decl.Range.First = brackets[0].first;
      } else {
      decl.Range.First = brackets[1].first;
            decl.Declaration.Dimension = 1;
               is_vs_input = (file == TGSI_FILE_INPUT &&
            cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == ',') {
      cur2 = cur;
   cur2++;
   eat_opt_white( &cur2 );
   if (str_match_nocase_whole( &cur2, "ARRAY" )) {
      int arrayid;
   if (*cur2 != '(') {
      report_error( ctx, "Expected `('" );
      }
   cur2++;
   eat_opt_white( &cur2 );
   if (!parse_int( &cur2, &arrayid )) {
      report_error( ctx, "Expected `,'" );
      }
   eat_opt_white( &cur2 );
   if (*cur2 != ')') {
      report_error( ctx, "Expected `)'" );
      }
   cur2++;
   decl.Declaration.Array = 1;
   decl.Array.ArrayID = arrayid;
                  if (*cur == ',' && !is_vs_input) {
               cur++;
   eat_opt_white( &cur );
   if (file == TGSI_FILE_IMAGE) {
      for (i = 0; i < TGSI_TEXTURE_COUNT; i++) {
      if (str_match_nocase_whole(&cur, tgsi_texture_names[i])) {
      decl.Image.Resource = i;
         }
   if (i == TGSI_TEXTURE_COUNT) {
      report_error(ctx, "Expected texture target");
               cur2 = cur;
   eat_opt_white(&cur2);
   while (*cur2 == ',') {
      cur2++;
   eat_opt_white(&cur2);
                                 } else {
                              }
   cur = cur2;
                     } else if (file == TGSI_FILE_SAMPLER_VIEW) {
      for (i = 0; i < TGSI_TEXTURE_COUNT; i++) {
      if (str_match_nocase_whole(&cur, tgsi_texture_names[i])) {
      decl.SamplerView.Resource = i;
         }
   if (i == TGSI_TEXTURE_COUNT) {
      report_error(ctx, "Expected texture target");
      }
   eat_opt_white( &cur );
   if (*cur != ',') {
      report_error( ctx, "Expected `,'" );
      }
   ++cur;
   eat_opt_white( &cur );
   for (j = 0; j < 4; ++j) {
      for (i = 0; i < TGSI_RETURN_TYPE_COUNT; ++i) {
      if (str_match_nocase_whole(&cur, tgsi_return_type_names[i])) {
      switch (j) {
   case 0:
      decl.SamplerView.ReturnTypeX = i;
      case 1:
      decl.SamplerView.ReturnTypeY = i;
      case 2:
      decl.SamplerView.ReturnTypeZ = i;
      case 3:
      decl.SamplerView.ReturnTypeW = i;
      default:
         }
         }
   if (i == TGSI_RETURN_TYPE_COUNT) {
      if (j == 0 || j >  2) {
      report_error(ctx, "Expected type name");
      }
      } else {
      cur2 = cur;
   eat_opt_white( &cur2 );
   if (*cur2 == ',') {
      cur2++;
   eat_opt_white( &cur2 );
   cur = cur2;
      } else
         }
   if (j < 4) {
      decl.SamplerView.ReturnTypeY =
      decl.SamplerView.ReturnTypeZ =
   decl.SamplerView.ReturnTypeW =
   }
      } else if (file == TGSI_FILE_BUFFER) {
      if (str_match_nocase_whole(&cur, "ATOMIC")) {
      decl.Declaration.Atomic = 1;
         } else if (file == TGSI_FILE_MEMORY) {
      if (str_match_nocase_whole(&cur, "GLOBAL")) {
      /* Note this is a no-op global is the default */
   decl.Declaration.MemType = TGSI_MEMORY_TYPE_GLOBAL;
      } else if (str_match_nocase_whole(&cur, "SHARED")) {
      decl.Declaration.MemType = TGSI_MEMORY_TYPE_SHARED;
      } else if (str_match_nocase_whole(&cur, "PRIVATE")) {
      decl.Declaration.MemType = TGSI_MEMORY_TYPE_PRIVATE;
      } else if (str_match_nocase_whole(&cur, "INPUT")) {
      decl.Declaration.MemType = TGSI_MEMORY_TYPE_INPUT;
         } else {
      if (str_match_nocase_whole(&cur, "LOCAL")) {
      decl.Declaration.Local = 1;
               cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == ',') {
                     for (i = 0; i < TGSI_SEMANTIC_COUNT; i++) {
                        cur2 = cur;
   eat_opt_white( &cur2 );
   if (*cur2 == '[') {
      cur2++;
   eat_opt_white( &cur2 );
   if (!parse_uint( &cur2, &index )) {
      report_error( ctx, "Expected literal integer" );
      }
   eat_opt_white( &cur2 );
   if (*cur2 != ']') {
                                                                  ctx->cur = cur;
                           cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == ',' &&
      file == TGSI_FILE_OUTPUT && ctx->processor == PIPE_SHADER_GEOMETRY) {
   cur++;
   eat_opt_white(&cur);
   if (str_match_nocase_whole(&cur, "STREAM")) {
               eat_opt_white(&cur);
   if (*cur != '(') {
      report_error(ctx, "Expected '('");
                     for (int i = 0; i < 4; ++i) {
      eat_opt_white(&cur);
   if (!parse_uint(&cur, &stream[i])) {
                        eat_opt_white(&cur);
   if (i < 3) {
      if (*cur != ',') {
      report_error(ctx, "Expected ','");
      }
                  if (*cur != ')') {
      report_error(ctx, "Expected ')'");
                     decl.Semantic.StreamX = stream[0];
   decl.Semantic.StreamY = stream[1];
                                 cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == ',' && !is_vs_input) {
               cur++;
   eat_opt_white( &cur );
   for (i = 0; i < TGSI_INTERPOLATE_COUNT; i++) {
      if (str_match_nocase_whole( &cur, tgsi_interpolate_names[i] )) {
                     ctx->cur = cur;
                     cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == ',' && !is_vs_input) {
               cur++;
   eat_opt_white( &cur );
   for (i = 0; i < TGSI_INTERPOLATE_LOC_COUNT; i++) {
                        ctx->cur = cur;
                     cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == ',' && !is_vs_input) {
      cur++;
   eat_opt_white( &cur );
   if (str_match_nocase_whole( &cur, tgsi_invariant_name )) {
      decl.Declaration.Invariant = 1;
      } else {
      report_error( ctx, "Expected semantic, interpolate attribute, or invariant ");
                  advance = tgsi_build_full_declaration(
      &decl,
   ctx->tokens_cur,
   ctx->header,
         if (advance == 0)
                     }
      static bool parse_immediate( struct translate_ctx *ctx )
   {
      struct tgsi_full_immediate imm;
   unsigned advance;
            if (*ctx->cur == '[') {
                        eat_opt_white( &ctx->cur );
   if (!parse_uint( &ctx->cur, &uindex )) {
      report_error( ctx, "Expected literal unsigned integer" );
               if (uindex != ctx->num_immediates) {
      report_error( ctx, "Immediates must be sorted" );
               eat_opt_white( &ctx->cur );
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]'" );
                           if (!eat_white( &ctx->cur )) {
      report_error( ctx, "Syntax error" );
      }
   for (type = 0; type < ARRAY_SIZE(tgsi_immediate_type_names); ++type) {
      if (str_match_nocase_whole(&ctx->cur, tgsi_immediate_type_names[type]))
      }
   if (type == ARRAY_SIZE(tgsi_immediate_type_names)) {
      report_error( ctx, "Expected immediate type" );
               imm = tgsi_default_full_immediate();
   imm.Immediate.NrTokens += 4;
   imm.Immediate.DataType = type;
            advance = tgsi_build_full_immediate(
      &imm,
   ctx->tokens_cur,
   ctx->header,
      if (advance == 0)
                              }
      static bool
   parse_primitive( const char **pcur, unsigned *primitive )
   {
               for (i = 0; i < MESA_PRIM_COUNT; i++) {
               if (str_match_nocase_whole( &cur, tgsi_primitive_names[i])) {
      *primitive = i;
   *pcur = cur;
         }
      }
      static bool
   parse_fs_coord_origin( const char **pcur, unsigned *fs_coord_origin )
   {
               for (i = 0; i < ARRAY_SIZE(tgsi_fs_coord_origin_names); i++) {
               if (str_match_nocase_whole( &cur, tgsi_fs_coord_origin_names[i])) {
      *fs_coord_origin = i;
   *pcur = cur;
         }
      }
      static bool
   parse_fs_coord_pixel_center( const char **pcur, unsigned *fs_coord_pixel_center )
   {
               for (i = 0; i < ARRAY_SIZE(tgsi_fs_coord_pixel_center_names); i++) {
               if (str_match_nocase_whole( &cur, tgsi_fs_coord_pixel_center_names[i])) {
      *fs_coord_pixel_center = i;
   *pcur = cur;
         }
      }
      static bool
   parse_property_next_shader( const char **pcur, unsigned *next_shader )
   {
               for (i = 0; i < ARRAY_SIZE(tgsi_processor_type_names); i++) {
               if (str_match_nocase_whole( &cur, tgsi_processor_type_names[i])) {
      *next_shader = i;
   *pcur = cur;
         }
      }
      static bool parse_property( struct translate_ctx *ctx )
   {
      struct tgsi_full_property prop;
   enum tgsi_property_name property_name;
   unsigned values[8];
   unsigned advance;
            if (!eat_white( &ctx->cur )) {
      report_error( ctx, "Syntax error" );
      }
   if (!parse_identifier( &ctx->cur, id, sizeof(id) )) {
      report_error( ctx, "Syntax error" );
      }
   for (property_name = 0; property_name < TGSI_PROPERTY_COUNT;
      ++property_name) {
   if (streq_nocase_uprcase(tgsi_property_names[property_name], id)) {
            }
   if (property_name >= TGSI_PROPERTY_COUNT) {
      debug_printf( "\nError: Unknown property : '%s'", id );
               eat_opt_white( &ctx->cur );
   switch(property_name) {
   case TGSI_PROPERTY_GS_INPUT_PRIM:
   case TGSI_PROPERTY_GS_OUTPUT_PRIM:
      if (!parse_primitive(&ctx->cur, &values[0] )) {
      report_error( ctx, "Unknown primitive name as property!" );
      }
   if (property_name == TGSI_PROPERTY_GS_INPUT_PRIM &&
      ctx->processor == PIPE_SHADER_GEOMETRY) {
      }
      case TGSI_PROPERTY_FS_COORD_ORIGIN:
      if (!parse_fs_coord_origin(&ctx->cur, &values[0] )) {
      report_error( ctx, "Unknown coord origin as property: must be UPPER_LEFT or LOWER_LEFT!" );
      }
      case TGSI_PROPERTY_FS_COORD_PIXEL_CENTER:
      if (!parse_fs_coord_pixel_center(&ctx->cur, &values[0] )) {
      report_error( ctx, "Unknown coord pixel center as property: must be HALF_INTEGER or INTEGER!" );
      }
      case TGSI_PROPERTY_NEXT_SHADER:
      if (!parse_property_next_shader(&ctx->cur, &values[0] )) {
      report_error( ctx, "Unknown next shader property value." );
      }
      case TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS:
   default:
      if (!parse_uint(&ctx->cur, &values[0] )) {
      report_error( ctx, "Expected unsigned integer as property!" );
                  prop = tgsi_default_full_property();
   prop.Property.PropertyName = property_name;
   prop.Property.NrTokens += 1;
            advance = tgsi_build_full_property(
      &prop,
   ctx->tokens_cur,
   ctx->header,
      if (advance == 0)
                     }
         static bool translate( struct translate_ctx *ctx )
   {
      eat_opt_white( &ctx->cur );
   if (!parse_header( ctx ))
            if (ctx->processor == PIPE_SHADER_TESS_CTRL ||
      ctx->processor == PIPE_SHADER_TESS_EVAL)
         while (*ctx->cur != '\0') {
      unsigned label_val = 0;
   if (!eat_white( &ctx->cur )) {
      report_error( ctx, "Syntax error" );
               if (*ctx->cur == '\0')
         if (parse_label( ctx, &label_val )) {
      if (!parse_instruction( ctx, true ))
      }
   else if (str_match_nocase_whole( &ctx->cur, "DCL" )) {
      if (!parse_declaration( ctx ))
      }
   else if (str_match_nocase_whole( &ctx->cur, "IMM" )) {
      if (!parse_immediate( ctx ))
      }
   else if (str_match_nocase_whole( &ctx->cur, "PROPERTY" )) {
      if (!parse_property( ctx ))
      }
   else if (!parse_instruction( ctx, false )) {
                        }
      bool
   tgsi_text_translate(
      const char *text,
   struct tgsi_token *tokens,
      {
               ctx.text = text;
   ctx.cur = text;
   ctx.tokens = tokens;
   ctx.tokens_cur = tokens;
            if (!translate( &ctx ))
               }
