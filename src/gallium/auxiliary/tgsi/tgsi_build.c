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
   #include "util/format/u_formats.h"
   #include "pipe/p_shader_tokens.h"
   #include "tgsi_build.h"
   #include "tgsi_parse.h"
         /*
   * header
   */
      struct tgsi_header
   tgsi_build_header( void )
   {
               header.HeaderSize = 1;
               }
      static void
   header_headersize_grow( struct tgsi_header *header )
   {
      assert( header->HeaderSize < 0xFF );
               }
      static void
   header_bodysize_grow( struct tgsi_header *header )
   {
                  }
      struct tgsi_processor
   tgsi_build_processor(
      unsigned type,
      {
               processor.Processor = type;
                        }
      /*
   * declaration
   */
      static void
   declaration_grow(
      struct tgsi_declaration *declaration,
      {
                           }
      static struct tgsi_declaration
   tgsi_default_declaration( void )
   {
               declaration.Type = TGSI_TOKEN_TYPE_DECLARATION;
   declaration.NrTokens = 1;
   declaration.File = TGSI_FILE_NULL;
   declaration.UsageMask = TGSI_WRITEMASK_XYZW;
   declaration.Interpolate = 0;
   declaration.Dimension = 0;
   declaration.Semantic = 0;
   declaration.Invariant = 0;
   declaration.Local = 0;
   declaration.Array = 0;
   declaration.Atomic = 0;
   declaration.MemType = TGSI_MEMORY_TYPE_GLOBAL;
               }
      static struct tgsi_declaration
   tgsi_build_declaration(
      enum tgsi_file_type file,
   unsigned usage_mask,
   unsigned interpolate,
   unsigned dimension,
   unsigned semantic,
   unsigned invariant,
   unsigned local,
   unsigned array,
   unsigned atomic,
   unsigned mem_type,
      {
               assert( file < TGSI_FILE_COUNT );
            declaration = tgsi_default_declaration();
   declaration.File = file;
   declaration.UsageMask = usage_mask;
   declaration.Interpolate = interpolate;
   declaration.Dimension = dimension;
   declaration.Semantic = semantic;
   declaration.Invariant = invariant;
   declaration.Local = local;
   declaration.Array = array;
   declaration.Atomic = atomic;
   declaration.MemType = mem_type;
               }
      static struct tgsi_declaration_range
   tgsi_default_declaration_range( void )
   {
               dr.First = 0;
               }
      static struct tgsi_declaration_dimension
   tgsi_default_declaration_dimension()
   {
               dim.Index2D = 0;
               }
      static struct tgsi_declaration_range
   tgsi_build_declaration_range(
      unsigned first,
   unsigned last,
   struct tgsi_declaration *declaration,
      {
               assert( last >= first );
            declaration_range.First = first;
                        }
      static struct tgsi_declaration_dimension
   tgsi_build_declaration_dimension(unsigned index_2d,
               {
                        dd.Index2D = index_2d;
                        }
      static struct tgsi_declaration_interp
   tgsi_default_declaration_interp( void )
   {
               di.Interpolate = TGSI_INTERPOLATE_CONSTANT;
   di.Location = TGSI_INTERPOLATE_LOC_CENTER;
               }
      static struct tgsi_declaration_interp
   tgsi_build_declaration_interp(unsigned interpolate,
                     {
               di.Interpolate = interpolate;
   di.Location = interpolate_location;
                        }
      static struct tgsi_declaration_semantic
   tgsi_default_declaration_semantic( void )
   {
               ds.Name = TGSI_SEMANTIC_POSITION;
   ds.Index = 0;
   ds.StreamX = 0;
   ds.StreamY = 0;
   ds.StreamZ = 0;
               }
      static struct tgsi_declaration_semantic
   tgsi_build_declaration_semantic(
      unsigned semantic_name,
   unsigned semantic_index,
   unsigned streamx,
   unsigned streamy,
   unsigned streamz,
   unsigned streamw,
   struct tgsi_declaration *declaration,
      {
               assert( semantic_name <= TGSI_SEMANTIC_COUNT );
            ds.Name = semantic_name;
   ds.Index = semantic_index;
   ds.StreamX = streamx;
   ds.StreamY = streamy;
   ds.StreamZ = streamz;
                        }
      static struct tgsi_declaration_image
   tgsi_default_declaration_image(void)
   {
               di.Resource = TGSI_TEXTURE_BUFFER;
   di.Raw = 0;
   di.Writable = 0;
   di.Format = 0;
               }
      static struct tgsi_declaration_image
   tgsi_build_declaration_image(unsigned texture,
                                 {
               di = tgsi_default_declaration_image();
   di.Resource = texture;
   di.Format = format;
   di.Raw = raw;
                        }
      static struct tgsi_declaration_sampler_view
   tgsi_default_declaration_sampler_view(void)
   {
               dsv.Resource = TGSI_TEXTURE_BUFFER;
   dsv.ReturnTypeX = TGSI_RETURN_TYPE_UNORM;
   dsv.ReturnTypeY = TGSI_RETURN_TYPE_UNORM;
   dsv.ReturnTypeZ = TGSI_RETURN_TYPE_UNORM;
               }
      static struct tgsi_declaration_sampler_view
   tgsi_build_declaration_sampler_view(unsigned texture,
                                       {
               dsv = tgsi_default_declaration_sampler_view();
   dsv.Resource = texture;
   dsv.ReturnTypeX = return_type_x;
   dsv.ReturnTypeY = return_type_y;
   dsv.ReturnTypeZ = return_type_z;
                        }
         static struct tgsi_declaration_array
   tgsi_default_declaration_array( void )
   {
               a.ArrayID = 0;
               }
      static struct tgsi_declaration_array
   tgsi_build_declaration_array(unsigned arrayid,
               {
               da = tgsi_default_declaration_array();
                        }
      struct tgsi_full_declaration
   tgsi_default_full_declaration( void )
   {
               full_declaration.Declaration  = tgsi_default_declaration();
   full_declaration.Range = tgsi_default_declaration_range();
   full_declaration.Dim = tgsi_default_declaration_dimension();
   full_declaration.Semantic = tgsi_default_declaration_semantic();
   full_declaration.Interp = tgsi_default_declaration_interp();
   full_declaration.Image = tgsi_default_declaration_image();
   full_declaration.SamplerView = tgsi_default_declaration_sampler_view();
               }
      unsigned
   tgsi_build_full_declaration(
      const struct tgsi_full_declaration *full_decl,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
      {
      unsigned size = 0;
   struct tgsi_declaration *declaration;
            if( maxsize <= size )
         declaration = (struct tgsi_declaration *) &tokens[size];
            *declaration = tgsi_build_declaration(
      full_decl->Declaration.File,
   full_decl->Declaration.UsageMask,
   full_decl->Declaration.Interpolate,
   full_decl->Declaration.Dimension,
   full_decl->Declaration.Semantic,
   full_decl->Declaration.Invariant,
   full_decl->Declaration.Local,
   full_decl->Declaration.Array,
   full_decl->Declaration.Atomic,
   full_decl->Declaration.MemType,
         if (maxsize <= size)
         dr = (struct tgsi_declaration_range *) &tokens[size];
            *dr = tgsi_build_declaration_range(
      full_decl->Range.First,
   full_decl->Range.Last,
   declaration,
         if (full_decl->Declaration.Dimension) {
               if (maxsize <= size) {
         }
   dd = (struct tgsi_declaration_dimension *)&tokens[size];
            *dd = tgsi_build_declaration_dimension(full_decl->Dim.Index2D,
                     if (full_decl->Declaration.Interpolate) {
               if (maxsize <= size) {
         }
   di = (struct tgsi_declaration_interp *)&tokens[size];
            *di = tgsi_build_declaration_interp(full_decl->Interp.Interpolate,
                           if( full_decl->Declaration.Semantic ) {
               if( maxsize <= size )
         ds = (struct tgsi_declaration_semantic *) &tokens[size];
            *ds = tgsi_build_declaration_semantic(
      full_decl->Semantic.Name,
   full_decl->Semantic.Index,
   full_decl->Semantic.StreamX,
   full_decl->Semantic.StreamY,
   full_decl->Semantic.StreamZ,
   full_decl->Semantic.StreamW,
   declaration,
            if (full_decl->Declaration.File == TGSI_FILE_IMAGE) {
               if (maxsize <= size) {
         }
   di = (struct tgsi_declaration_image *)&tokens[size];
            *di = tgsi_build_declaration_image(full_decl->Image.Resource,
                                       if (full_decl->Declaration.File == TGSI_FILE_SAMPLER_VIEW) {
               if (maxsize <= size) {
         }
   dsv = (struct tgsi_declaration_sampler_view *)&tokens[size];
            *dsv = tgsi_build_declaration_sampler_view(
      full_decl->SamplerView.Resource,
   full_decl->SamplerView.ReturnTypeX,
   full_decl->SamplerView.ReturnTypeY,
   full_decl->SamplerView.ReturnTypeZ,
   full_decl->SamplerView.ReturnTypeW,
   declaration,
            if (full_decl->Declaration.Array) {
               if (maxsize <= size) {
         }
   da = (struct tgsi_declaration_array *)&tokens[size];
   size++;
   *da = tgsi_build_declaration_array(
      full_decl->Array.ArrayID,
   declaration,
   }
      }
      /*
   * immediate
   */
      static struct tgsi_immediate
   tgsi_default_immediate( void )
   {
               immediate.Type = TGSI_TOKEN_TYPE_IMMEDIATE;
   immediate.NrTokens = 1;
   immediate.DataType = TGSI_IMM_FLOAT32;
               }
      static struct tgsi_immediate
   tgsi_build_immediate(
      struct tgsi_header *header,
      {
               immediate = tgsi_default_immediate();
                        }
      struct tgsi_full_immediate
   tgsi_default_full_immediate( void )
   {
               fullimm.Immediate = tgsi_default_immediate();
   fullimm.u[0].Float = 0.0f;
   fullimm.u[1].Float = 0.0f;
   fullimm.u[2].Float = 0.0f;
               }
      static void
   immediate_grow(
      struct tgsi_immediate *immediate,
      {
                           }
      unsigned
   tgsi_build_full_immediate(
      const struct tgsi_full_immediate *full_imm,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
      {
      unsigned size = 0;
   int i;
            if( maxsize <= size )
         immediate = (struct tgsi_immediate *) &tokens[size];
                              for( i = 0; i < full_imm->Immediate.NrTokens - 1; i++ ) {
               if( maxsize <= size )
            data = (union tgsi_immediate_data *) &tokens[size];
            immediate_grow( immediate, header );
                  }
      /*
   * instruction
   */
      struct tgsi_instruction
   tgsi_default_instruction( void )
   {
               instruction.Type = TGSI_TOKEN_TYPE_INSTRUCTION;
   instruction.NrTokens = 0;
   instruction.Opcode = TGSI_OPCODE_MOV;
   instruction.Saturate = 0;
   instruction.NumDstRegs = 1;
   instruction.NumSrcRegs = 1;
   instruction.Label = 0;
   instruction.Texture = 0;
   instruction.Memory = 0;
   instruction.Precise = 0;
               }
      static struct tgsi_instruction
   tgsi_build_instruction(enum tgsi_opcode opcode,
                        unsigned saturate,
      {
               assert (opcode <= TGSI_OPCODE_LAST);
   assert (saturate <= 1);
   assert (num_dst_regs <= 3);
            instruction = tgsi_default_instruction();
   instruction.Opcode = opcode;
   instruction.Saturate = saturate;
   instruction.Precise = precise;
   instruction.NumDstRegs = num_dst_regs;
                        }
      static void
   instruction_grow(
      struct tgsi_instruction *instruction,
      {
                           }
      static struct tgsi_instruction_label
   tgsi_default_instruction_label( void )
   {
               instruction_label.Label = 0;
               }
      static struct tgsi_instruction_label
   tgsi_build_instruction_label(
      unsigned label,
   struct tgsi_instruction *instruction,
      {
               instruction_label.Label = label;
   instruction_label.Padding = 0;
                        }
      static struct tgsi_instruction_texture
   tgsi_default_instruction_texture( void )
   {
               instruction_texture.Texture = TGSI_TEXTURE_UNKNOWN;
   instruction_texture.NumOffsets = 0;
   instruction_texture.ReturnType = TGSI_RETURN_TYPE_UNKNOWN;
               }
      static struct tgsi_instruction_texture
   tgsi_build_instruction_texture(
      unsigned texture,
   unsigned num_offsets,
   unsigned return_type,
   struct tgsi_instruction *instruction,
      {
               instruction_texture.Texture = texture;
   instruction_texture.NumOffsets = num_offsets;
   instruction_texture.ReturnType = return_type;
   instruction_texture.Padding = 0;
                        }
      static struct tgsi_instruction_memory
   tgsi_default_instruction_memory( void )
   {
               instruction_memory.Qualifier = 0;
   instruction_memory.Texture = 0;
   instruction_memory.Format = 0;
               }
      static struct tgsi_instruction_memory
   tgsi_build_instruction_memory(
      unsigned qualifier,
   unsigned texture,
   unsigned format,
   struct tgsi_instruction *instruction,
      {
               instruction_memory.Qualifier = qualifier;
   instruction_memory.Texture = texture;
   instruction_memory.Format = format;
   instruction_memory.Padding = 0;
                        }
      static struct tgsi_texture_offset
   tgsi_default_texture_offset( void )
   {
               texture_offset.Index = 0;
   texture_offset.File = 0;
   texture_offset.SwizzleX = 0;
   texture_offset.SwizzleY = 0;
   texture_offset.SwizzleZ = 0;
               }
      static struct tgsi_texture_offset
   tgsi_build_texture_offset(
      int index,
   enum tgsi_file_type file,
   int swizzle_x, int swizzle_y, int swizzle_z,
   struct tgsi_instruction *instruction,
      {
               texture_offset.Index = index;
   texture_offset.File = file;
   texture_offset.SwizzleX = swizzle_x;
   texture_offset.SwizzleY = swizzle_y;
   texture_offset.SwizzleZ = swizzle_z;
                        }
      static struct tgsi_src_register
   tgsi_default_src_register( void )
   {
               src_register.File = TGSI_FILE_NULL;
   src_register.SwizzleX = TGSI_SWIZZLE_X;
   src_register.SwizzleY = TGSI_SWIZZLE_Y;
   src_register.SwizzleZ = TGSI_SWIZZLE_Z;
   src_register.SwizzleW = TGSI_SWIZZLE_W;
   src_register.Negate = 0;
   src_register.Absolute = 0;
   src_register.Indirect = 0;
   src_register.Dimension = 0;
               }
      static struct tgsi_src_register
   tgsi_build_src_register(
      enum tgsi_file_type file,
   unsigned swizzle_x,
   unsigned swizzle_y,
   unsigned swizzle_z,
   unsigned swizzle_w,
   unsigned negate,
   unsigned absolute,
   unsigned indirect,
   unsigned dimension,
   int index,
   struct tgsi_instruction *instruction,
      {
               assert( file < TGSI_FILE_COUNT );
   assert( swizzle_x <= TGSI_SWIZZLE_W );
   assert( swizzle_y <= TGSI_SWIZZLE_W );
   assert( swizzle_z <= TGSI_SWIZZLE_W );
   assert( swizzle_w <= TGSI_SWIZZLE_W );
   assert( negate <= 1 );
            src_register.File = file;
   src_register.SwizzleX = swizzle_x;
   src_register.SwizzleY = swizzle_y;
   src_register.SwizzleZ = swizzle_z;
   src_register.SwizzleW = swizzle_w;
   src_register.Negate = negate;
   src_register.Absolute = absolute;
   src_register.Indirect = indirect;
   src_register.Dimension = dimension;
                        }
      static struct tgsi_ind_register
   tgsi_default_ind_register( void )
   {
               ind_register.File = TGSI_FILE_NULL;
   ind_register.Index = 0;
   ind_register.Swizzle = TGSI_SWIZZLE_X;
               }
      static struct tgsi_ind_register
   tgsi_build_ind_register(
      enum tgsi_file_type file,
   unsigned swizzle,
   int index,
   unsigned arrayid,
   struct tgsi_instruction *instruction,
      {
               assert( file < TGSI_FILE_COUNT );
   assert( swizzle <= TGSI_SWIZZLE_W );
            ind_register.File = file;
   ind_register.Swizzle = swizzle;
   ind_register.Index = index;
                        }
      static struct tgsi_dimension
   tgsi_default_dimension( void )
   {
               dimension.Indirect = 0;
   dimension.Dimension = 0;
   dimension.Padding = 0;
               }
      static struct tgsi_full_src_register
   tgsi_default_full_src_register( void )
   {
               full_src_register.Register = tgsi_default_src_register();
   full_src_register.Indirect = tgsi_default_ind_register();
   full_src_register.Dimension = tgsi_default_dimension();
               }
      static struct tgsi_dimension
   tgsi_build_dimension(
      unsigned indirect,
   unsigned index,
   struct tgsi_instruction *instruction,
      {
               dimension.Indirect = indirect;
   dimension.Dimension = 0;
   dimension.Padding = 0;
                        }
      static struct tgsi_dst_register
   tgsi_default_dst_register( void )
   {
               dst_register.File = TGSI_FILE_NULL;
   dst_register.WriteMask = TGSI_WRITEMASK_XYZW;
   dst_register.Indirect = 0;
   dst_register.Dimension = 0;
   dst_register.Index = 0;
               }
      static struct tgsi_dst_register
   tgsi_build_dst_register(
      enum tgsi_file_type file,
   unsigned mask,
   unsigned indirect,
   unsigned dimension,
   int index,
   struct tgsi_instruction *instruction,
      {
               assert( file < TGSI_FILE_COUNT );
   assert( mask <= TGSI_WRITEMASK_XYZW );
            dst_register.File = file;
   dst_register.WriteMask = mask;
   dst_register.Indirect = indirect;
   dst_register.Dimension = dimension;
   dst_register.Index = index;
                        }
      static struct tgsi_full_dst_register
   tgsi_default_full_dst_register( void )
   {
               full_dst_register.Register = tgsi_default_dst_register();
   full_dst_register.Indirect = tgsi_default_ind_register();
   full_dst_register.Dimension = tgsi_default_dimension();
               }
      struct tgsi_full_instruction
   tgsi_default_full_instruction( void )
   {
      struct tgsi_full_instruction full_instruction;
            full_instruction.Instruction = tgsi_default_instruction();
   full_instruction.Label = tgsi_default_instruction_label();
   full_instruction.Texture = tgsi_default_instruction_texture();
   full_instruction.Memory = tgsi_default_instruction_memory();
   for( i = 0;  i < TGSI_FULL_MAX_TEX_OFFSETS; i++ ) {
         }
   for( i = 0;  i < TGSI_FULL_MAX_DST_REGISTERS; i++ ) {
         }
   for( i = 0;  i < TGSI_FULL_MAX_SRC_REGISTERS; i++ ) {
                     }
      unsigned
   tgsi_build_full_instruction(
      const struct tgsi_full_instruction *full_inst,
   struct  tgsi_token *tokens,
   struct  tgsi_header *header,
      {
      unsigned size = 0;
   unsigned i;
            if( maxsize <= size )
         instruction = (struct tgsi_instruction *) &tokens[size];
            *instruction = tgsi_build_instruction(full_inst->Instruction.Opcode,
                                    if (full_inst->Instruction.Label) {
               if( maxsize <= size )
         instruction_label =
                  *instruction_label = tgsi_build_instruction_label(
         header );
               if (full_inst->Instruction.Texture) {
               if( maxsize <= size )
         instruction_texture =
                  *instruction_texture = tgsi_build_instruction_texture(
      full_inst->Texture.Texture,
   full_inst->Texture.NumOffsets,
   full_inst->Texture.ReturnType,
               for (i = 0; i < full_inst->Texture.NumOffsets; i++) {
                  texture_offset = (struct tgsi_texture_offset *)&tokens[size];
            size++;
   *texture_offset = tgsi_build_texture_offset(
      full_inst->TexOffsets[i].Index,
   full_inst->TexOffsets[i].File,
   full_inst->TexOffsets[i].SwizzleX,
   full_inst->TexOffsets[i].SwizzleY,
   full_inst->TexOffsets[i].SwizzleZ,
   instruction,
               if (full_inst->Instruction.Memory) {
               if( maxsize <= size )
         instruction_memory =
                  *instruction_memory = tgsi_build_instruction_memory(
      full_inst->Memory.Qualifier,
   full_inst->Memory.Texture,
   full_inst->Memory.Format,
   instruction,
            for( i = 0;  i <   full_inst->Instruction.NumDstRegs; i++ ) {
      const struct tgsi_full_dst_register *reg = &full_inst->Dst[i];
            if( maxsize <= size )
         dst_register = (struct tgsi_dst_register *) &tokens[size];
            *dst_register = tgsi_build_dst_register(
      reg->Register.File,
   reg->Register.WriteMask,
   reg->Register.Indirect,
   reg->Register.Dimension,
   reg->Register.Index,
               if( reg->Register.Indirect ) {
               if( maxsize <= size )
                        *ind = tgsi_build_ind_register(
      reg->Indirect.File,
   reg->Indirect.Swizzle,
   reg->Indirect.Index,
   reg->Indirect.ArrayID,
   instruction,
            if( reg->Register.Dimension ) {
                        if( maxsize <= size )
                        *dim = tgsi_build_dimension(
      reg->Dimension.Indirect,
   reg->Dimension.Index,
                                 if( maxsize <= size )
                        *ind = tgsi_build_ind_register(
      reg->DimIndirect.File,
   reg->DimIndirect.Swizzle,
   reg->DimIndirect.Index,
   reg->DimIndirect.ArrayID,
   instruction,
                  for( i = 0;  i < full_inst->Instruction.NumSrcRegs; i++ ) {
      const struct tgsi_full_src_register *reg = &full_inst->Src[i];
            if( maxsize <= size )
         src_register = (struct tgsi_src_register *)  &tokens[size];
            *src_register = tgsi_build_src_register(
      reg->Register.File,
   reg->Register.SwizzleX,
   reg->Register.SwizzleY,
   reg->Register.SwizzleZ,
   reg->Register.SwizzleW,
   reg->Register.Negate,
   reg->Register.Absolute,
   reg->Register.Indirect,
   reg->Register.Dimension,
   reg->Register.Index,
               if( reg->Register.Indirect ) {
               if( maxsize <= size )
                        *ind = tgsi_build_ind_register(
      reg->Indirect.File,
   reg->Indirect.Swizzle,
   reg->Indirect.Index,
   reg->Indirect.ArrayID,
   instruction,
            if( reg->Register.Dimension ) {
                        if( maxsize <= size )
                        *dim = tgsi_build_dimension(
      reg->Dimension.Indirect,
   reg->Dimension.Index,
                                 if( maxsize <= size )
                        *ind = tgsi_build_ind_register(
      reg->DimIndirect.File,
   reg->DimIndirect.Swizzle,
   reg->DimIndirect.Index,
   reg->DimIndirect.ArrayID,
   instruction,
                     }
      static struct tgsi_property
   tgsi_default_property( void )
   {
               property.Type = TGSI_TOKEN_TYPE_PROPERTY;
   property.NrTokens = 1;
   property.PropertyName = TGSI_PROPERTY_GS_INPUT_PRIM;
               }
      static struct tgsi_property
   tgsi_build_property(unsigned property_name,
         {
               property = tgsi_default_property();
                        }
         struct tgsi_full_property
   tgsi_default_full_property( void )
   {
               full_property.Property  = tgsi_default_property();
   memset(full_property.u, 0,
               }
      static void
   property_grow(
      struct tgsi_property *property,
      {
                           }
      static struct tgsi_property_data
   tgsi_build_property_data(
      unsigned value,
   struct tgsi_property *property,
      {
                                    }
      unsigned
   tgsi_build_full_property(
      const struct tgsi_full_property *full_prop,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
      {
      unsigned size = 0;
   int i;
            if( maxsize <= size )
         property = (struct tgsi_property *) &tokens[size];
            *property = tgsi_build_property(
      full_prop->Property.PropertyName,
                  for( i = 0; i < full_prop->Property.NrTokens - 1; i++ ) {
               if( maxsize <= size )
         data = (struct tgsi_property_data *) &tokens[size];
            *data = tgsi_build_property_data(
      full_prop->u[i].Data,
   property,
               }
