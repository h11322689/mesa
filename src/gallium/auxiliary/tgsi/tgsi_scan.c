   /**************************************************************************
   * 
   * Copyright 2008 VMware, Inc.
   * All Rights Reserved.
   * Copyright 2008 VMware, Inc.  All rights Reserved.
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
   * TGSI program scan utility.
   * Used to determine which registers and instructions are used by a shader.
   *
   * Authors:  Brian Paul
   */
         #include "util/u_debug.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "tgsi/tgsi_info.h"
   #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_util.h"
   #include "tgsi/tgsi_scan.h"
         static bool
   is_memory_file(enum tgsi_file_type file)
   {
      return file == TGSI_FILE_SAMPLER ||
         file == TGSI_FILE_SAMPLER_VIEW ||
   file == TGSI_FILE_IMAGE ||
      }
         static bool
   is_mem_query_inst(enum tgsi_opcode opcode)
   {
      return opcode == TGSI_OPCODE_RESQ ||
         opcode == TGSI_OPCODE_TXQ ||
      }
      /**
   * Is the opcode a "true" texture instruction which samples from a
   * texture map?
   */
   static bool
   is_texture_inst(enum tgsi_opcode opcode)
   {
      return (!is_mem_query_inst(opcode) &&
      }
         static void
   scan_src_operand(struct tgsi_shader_info *info,
                  const struct tgsi_full_instruction *fullinst,
   const struct tgsi_full_src_register *src,
      {
               if (info->processor == PIPE_SHADER_COMPUTE &&
      src->Register.File == TGSI_FILE_SYSTEM_VALUE) {
                     switch (name) {
   case TGSI_SEMANTIC_GRID_SIZE:
      info->uses_grid_size = true;
                  /* Mark which inputs are effectively used */
   if (src->Register.File == TGSI_FILE_INPUT) {
      if (src->Register.Indirect) {
      for (ind = 0; ind < info->num_inputs; ++ind) {
            } else {
      assert(ind >= 0);
   assert(ind < PIPE_MAX_SHADER_INPUTS);
               if (info->processor == PIPE_SHADER_FRAGMENT) {
               if (src->Register.Indirect && src->Indirect.ArrayID)
                                 if (name == TGSI_SEMANTIC_POSITION &&
      usage_mask_after_swizzle & TGSI_WRITEMASK_Z)
               if (info->processor == PIPE_SHADER_TESS_CTRL &&
      src->Register.File == TGSI_FILE_OUTPUT) {
            if (src->Register.Indirect && src->Indirect.ArrayID)
         else
            switch (info->output_semantic_name[input]) {
   case TGSI_SEMANTIC_PATCH:
      info->reads_perpatch_outputs = true;
      case TGSI_SEMANTIC_TESSINNER:
   case TGSI_SEMANTIC_TESSOUTER:
      info->reads_tessfactor_outputs = true;
      default:
                     /* check for indirect register reads */
   if (src->Register.Indirect)
            if (src->Register.Dimension && src->Dimension.Indirect)
            /* Texture samplers */
   if (src->Register.File == TGSI_FILE_SAMPLER) {
               assert(fullinst->Instruction.Texture);
            if (is_texture_inst(fullinst->Instruction.Opcode)) {
      const unsigned target = fullinst->Texture.Texture;
   assert(target < TGSI_TEXTURE_UNKNOWN);
   /* for texture instructions, check that the texture instruction
   * target matches the previous sampler view declaration (if there
   * was one.)
   */
   if (info->sampler_targets[index] == TGSI_TEXTURE_UNKNOWN) {
      /* probably no sampler view declaration */
      } else {
      /* Make sure the texture instruction's sampler/target info
   * agrees with the sampler view declaration.
   */
                     if (is_memory_file(src->Register.File) &&
      !is_mem_query_inst(fullinst->Instruction.Opcode)) {
            if (src->Register.File == TGSI_FILE_IMAGE &&
      (fullinst->Memory.Texture == TGSI_TEXTURE_2D_MSAA ||
   fullinst->Memory.Texture == TGSI_TEXTURE_2D_ARRAY_MSAA)) {
   if (src->Register.Indirect)
         else
               if (tgsi_get_opcode_info(fullinst->Instruction.Opcode)->is_store) {
               if (src->Register.File == TGSI_FILE_BUFFER) {
      if (src->Register.Indirect)
         else
         } else {
      if (src->Register.File == TGSI_FILE_BUFFER) {
      if (src->Register.Indirect)
         else
               }
         static void
   scan_instruction(struct tgsi_shader_info *info,
               {
      unsigned i;
            assert(fullinst->Instruction.Opcode < TGSI_OPCODE_LAST);
            switch (fullinst->Instruction.Opcode) {
   case TGSI_OPCODE_IF:
   case TGSI_OPCODE_UIF:
   case TGSI_OPCODE_BGNLOOP:
      (*current_depth)++;
      case TGSI_OPCODE_ENDIF:
   case TGSI_OPCODE_ENDLOOP:
      (*current_depth)--;
      case TGSI_OPCODE_FBFETCH:
      info->uses_fbfetch = true;
      default:
                  for (i = 0; i < fullinst->Instruction.NumSrcRegs; i++) {
      scan_src_operand(info, fullinst, &fullinst->Src[i], i,
                  if (fullinst->Src[i].Register.Indirect) {
                              scan_src_operand(info, fullinst, &src, -1,
                     if (fullinst->Src[i].Register.Dimension &&
                                    scan_src_operand(info, fullinst, &src, -1,
                        if (fullinst->Instruction.Texture) {
      for (i = 0; i < fullinst->Texture.NumOffsets; i++) {
                              /* The usage mask is suboptimal but should be safe. */
   scan_src_operand(info, fullinst, &src, -1,
                  (1 << fullinst->TexOffsets[i].SwizzleX) |
               /* check for indirect register writes */
   for (i = 0; i < fullinst->Instruction.NumDstRegs; i++) {
               if (dst->Register.Indirect) {
                                                         if (dst->Register.Dimension && dst->Dimension.Indirect) {
                                                         if (is_memory_file(dst->Register.File)) {
                              if (dst->Register.File == TGSI_FILE_IMAGE) {
      if (fullinst->Memory.Texture == TGSI_TEXTURE_2D_MSAA ||
      fullinst->Memory.Texture == TGSI_TEXTURE_2D_ARRAY_MSAA) {
   if (dst->Register.Indirect)
         else
         } else if (dst->Register.File == TGSI_FILE_BUFFER) {
      if (dst->Register.Indirect)
         else
                        }
            static void
   scan_declaration(struct tgsi_shader_info *info,
         {
      enum tgsi_file_type file = fulldecl->Declaration.File;
   const unsigned procType = info->processor;
            if (fulldecl->Declaration.Array) {
               switch (file) {
   case TGSI_FILE_INPUT:
      assert(array_id < ARRAY_SIZE(info->input_array_first));
   info->input_array_first[array_id] = fulldecl->Range.First;
      case TGSI_FILE_OUTPUT:
      assert(array_id < ARRAY_SIZE(info->output_array_first));
               case TGSI_FILE_NULL:
            default:
                     for (reg = fulldecl->Range.First; reg <= fulldecl->Range.Last; reg++) {
      unsigned semName = fulldecl->Semantic.Name;
   unsigned semIndex = fulldecl->Semantic.Index +
         int buffer;
            /*
   * only first 32 regs will appear in this bitfield, if larger
   * bits will wrap around.
   */
   info->file_mask[file] |= (1u << (reg & 31));
   info->file_count[file]++;
            switch (file) {
   case TGSI_FILE_CONSTANT:
                              info->const_file_max[buffer] =
                     case TGSI_FILE_IMAGE:
      info->images_declared |= 1u << reg;
   if (fulldecl->Image.Resource == TGSI_TEXTURE_BUFFER)
               case TGSI_FILE_BUFFER:
                  case TGSI_FILE_HW_ATOMIC:
                  case TGSI_FILE_INPUT:
      info->input_semantic_name[reg] = (uint8_t) semName;
   info->input_semantic_index[reg] = (uint8_t) semIndex;
                                 switch (semName) {
   case TGSI_SEMANTIC_PRIMID:
      info->uses_primid = true;
      case TGSI_SEMANTIC_FACE:
      info->uses_frontface = true;
                  case TGSI_FILE_SYSTEM_VALUE:
                              switch (semName) {
   case TGSI_SEMANTIC_INSTANCEID:
      info->uses_instanceid = true;
      case TGSI_SEMANTIC_VERTEXID:
      info->uses_vertexid = true;
      case TGSI_SEMANTIC_VERTEXID_NOBASE:
      info->uses_vertexid_nobase = true;
      case TGSI_SEMANTIC_BASEVERTEX:
      info->uses_basevertex = true;
      case TGSI_SEMANTIC_PRIMID:
      info->uses_primid = true;
      case TGSI_SEMANTIC_INVOCATIONID:
      info->uses_invocationid = true;
      case TGSI_SEMANTIC_FACE:
      info->uses_frontface = true;
                  case TGSI_FILE_OUTPUT:
      info->output_semantic_name[reg] = (uint8_t) semName;
   info->output_semantic_index[reg] = (uint8_t) semIndex;
                  if (fulldecl->Declaration.UsageMask & TGSI_WRITEMASK_X) {
      info->output_streams[reg] |= (uint8_t)fulldecl->Semantic.StreamX;
      }
   if (fulldecl->Declaration.UsageMask & TGSI_WRITEMASK_Y) {
      info->output_streams[reg] |= (uint8_t)fulldecl->Semantic.StreamY << 2;
      }
   if (fulldecl->Declaration.UsageMask & TGSI_WRITEMASK_Z) {
      info->output_streams[reg] |= (uint8_t)fulldecl->Semantic.StreamZ << 4;
      }
   if (fulldecl->Declaration.UsageMask & TGSI_WRITEMASK_W) {
      info->output_streams[reg] |= (uint8_t)fulldecl->Semantic.StreamW << 6;
               switch (semName) {
   case TGSI_SEMANTIC_VIEWPORT_INDEX:
      info->writes_viewport_index = true;
      case TGSI_SEMANTIC_LAYER:
      info->writes_layer = true;
      case TGSI_SEMANTIC_PSIZE:
      info->writes_psize = true;
      case TGSI_SEMANTIC_CLIPVERTEX:
      info->writes_clipvertex = true;
      case TGSI_SEMANTIC_STENCIL:
      info->writes_stencil = true;
      case TGSI_SEMANTIC_SAMPLEMASK:
      info->writes_samplemask = true;
      case TGSI_SEMANTIC_EDGEFLAG:
      info->writes_edgeflag = true;
      case TGSI_SEMANTIC_POSITION:
      if (procType == PIPE_SHADER_FRAGMENT)
         else
                        case TGSI_FILE_SAMPLER:
      STATIC_ASSERT(sizeof(info->samplers_declared) * 8 >= PIPE_MAX_SAMPLERS);
               case TGSI_FILE_SAMPLER_VIEW:
                     assert(target < TGSI_TEXTURE_UNKNOWN);
   if (info->sampler_targets[reg] == TGSI_TEXTURE_UNKNOWN) {
      /* Save sampler target for this sampler index */
   info->sampler_targets[reg] = target;
      } else {
      /* if previously declared, make sure targets agree */
   assert(info->sampler_targets[reg] == target);
                  case TGSI_FILE_NULL:
            default:
               }
         static void
   scan_immediate(struct tgsi_shader_info *info)
   {
      unsigned reg = info->immediate_count++;
            info->file_mask[file] |= (1 << reg);
   info->file_count[file]++;
      }
         static void
   scan_property(struct tgsi_shader_info *info,
         {
      unsigned name = fullprop->Property.PropertyName;
            assert(name < ARRAY_SIZE(info->properties));
            switch (name) {
   case TGSI_PROPERTY_NUM_CLIPDIST_ENABLED:
      info->num_written_clipdistance = value;
      case TGSI_PROPERTY_NUM_CULLDIST_ENABLED:
      info->num_written_culldistance = value;
         }
         /**
   * Scan the given TGSI shader to collect information such as number of
   * registers used, special instructions used, etc.
   * \return info  the result of the scan
   */
   void
   tgsi_scan_shader(const struct tgsi_token *tokens,
         {
      unsigned procType, i;
   struct tgsi_parse_context parse;
            memset(info, 0, sizeof(*info));
   for (i = 0; i < TGSI_FILE_COUNT; i++)
         for (i = 0; i < ARRAY_SIZE(info->const_file_max); i++)
         for (i = 0; i < ARRAY_SIZE(info->sampler_targets); i++)
            /**
   ** Setup to begin parsing input shader
   **/
   if (tgsi_parse_init( &parse, tokens ) != TGSI_PARSE_OK) {
      debug_printf("tgsi_parse_init() failed in tgsi_scan_shader()!\n");
      }
   procType = parse.FullHeader.Processor.Processor;
   assert(procType == PIPE_SHADER_FRAGMENT ||
         procType == PIPE_SHADER_VERTEX ||
   procType == PIPE_SHADER_GEOMETRY ||
   procType == PIPE_SHADER_TESS_CTRL ||
   procType == PIPE_SHADER_TESS_EVAL ||
            if (procType == PIPE_SHADER_GEOMETRY)
            /**
   ** Loop over incoming program tokens/instructions
   */
   while (!tgsi_parse_end_of_tokens(&parse)) {
               switch( parse.FullToken.Token.Type ) {
   case TGSI_TOKEN_TYPE_INSTRUCTION:
      scan_instruction(info, &parse.FullToken.FullInstruction,
            case TGSI_TOKEN_TYPE_DECLARATION:
      scan_declaration(info, &parse.FullToken.FullDeclaration);
      case TGSI_TOKEN_TYPE_IMMEDIATE:
      scan_immediate(info);
      case TGSI_TOKEN_TYPE_PROPERTY:
      scan_property(info, &parse.FullToken.FullProperty);
      default:
                     info->uses_kill = (info->opcode_count[TGSI_OPCODE_KILL_IF] ||
            /* The dimensions of the IN decleration in geometry shader have
   * to be deduced from the type of the input primitive.
   */
   if (procType == PIPE_SHADER_GEOMETRY) {
      unsigned input_primitive =
         int num_verts = u_vertices_per_prim(input_primitive);
   int j;
   info->file_count[TGSI_FILE_INPUT] = num_verts;
   info->file_max[TGSI_FILE_INPUT] =
         for (j = 0; j < num_verts; ++j) {
                        }
   