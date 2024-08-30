   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      /**
   * @file
   * SVGA Shader Dump Facilities
   * 
   * @author Michal Krol <michal@vmware.com>
   */
      #include <assert.h>
   #include <string.h>
      #include "svga_shader.h"
   #include "svga_shader_dump.h"
   #include "svga_shader_op.h"
   #include "util/u_debug.h"
      #include "../svga_hw_reg.h"
   #include "svga3d_shaderdefs.h"
      struct dump_info
   {
      uint32 version;
   bool is_ps;
      };
      #define DUMP_MAX_OP_SRC 4
      struct dump_op
   {
      struct sh_op op;
   struct sh_dstreg dst;
   struct sh_srcreg dstind;
   struct sh_srcreg src[DUMP_MAX_OP_SRC];
   struct sh_srcreg srcind[DUMP_MAX_OP_SRC];
      };
      static void
   dump_indent(int indent)
   {
               for (i = 0; i < indent; ++i) {
            }
      static void dump_op( struct sh_op op, const char *mnemonic )
   {
               if (op.predicated) {
         }
   if (op.coissue)
                  switch (op.opcode) {
   case SVGA3DOP_TEX:
      switch (op.control) {
   case 0:
         case 1 /* PROJECT */:
      _debug_printf("p");
      case 2 /* BIAS */:
      _debug_printf("b");
      default:
         }
         case SVGA3DOP_IFC:
   case SVGA3DOP_BREAKC:
   case SVGA3DOP_SETP:
      switch (op.control) {
   case SVGA3DOPCOMP_GT:
      _debug_printf("_gt");
      case SVGA3DOPCOMP_EQ:
      _debug_printf("_eq");
      case SVGA3DOPCOMP_GE:
      _debug_printf("_ge");
      case SVGA3DOPCOMP_LT:
      _debug_printf("_lt");
      case SVGA3DOPCOMPC_NE:
      _debug_printf("_ne");
      case SVGA3DOPCOMP_LE:
      _debug_printf("_le");
      default:
         }
         default:
            }
      static void
   format_reg(const char *name,
               {
      if (reg.relative) {
               if (sh_srcreg_type(*indreg) == SVGA3DREG_LOOP) {
         } else {
            } else {
            }
      static void dump_reg( struct sh_reg reg, struct sh_srcreg *indreg, const struct dump_info *di )
   {
               switch (sh_reg_type( reg )) {
   case SVGA3DREG_TEMP:
      format_reg("r", reg, NULL);
         case SVGA3DREG_INPUT:
      format_reg("v", reg, indreg);
         case SVGA3DREG_CONST:
      format_reg("c", reg, indreg);
         case SVGA3DREG_ADDR:    /* VS */
   /* SVGA3DREG_TEXTURE */ /* PS */
      assert(!reg.relative);
   if (di->is_ps) {
         } else {
         }
         case SVGA3DREG_RASTOUT:
      assert(!reg.relative);
   switch (reg.number) {
   case 0 /*POSITION*/:
      _debug_printf( "oPos" );
      case 1 /*FOG*/:
      _debug_printf( "oFog" );
      case 2 /*POINT_SIZE*/:
      _debug_printf( "oPts" );
      default:
      assert( 0 );
      }
         case SVGA3DREG_ATTROUT:
      assert( reg.number < 2 );
   format_reg("oD", reg, NULL);
         case SVGA3DREG_TEXCRDOUT:  /* VS */
   /* SVGA3DREG_OUTPUT */     /* VS3.0+ */
      if (!di->is_ps && di->version >= SVGA3D_VS_30) {
         } else {
         }
         case SVGA3DREG_COLOROUT:
      format_reg("oC", reg, NULL);
         case SVGA3DREG_DEPTHOUT:
      assert(!reg.relative);
   assert(reg.number == 0);
   _debug_printf("oDepth");
         case SVGA3DREG_SAMPLER:
      format_reg("s", reg, NULL);
         case SVGA3DREG_CONSTBOOL:
      format_reg("b", reg, NULL);
         case SVGA3DREG_CONSTINT:
      format_reg("i", reg, NULL);
         case SVGA3DREG_LOOP:
      assert(!reg.relative);
   assert( reg.number == 0 );
   _debug_printf( "aL" );
         case SVGA3DREG_MISCTYPE:
      assert(!reg.relative);
   switch (reg.number) {
   case SVGA3DMISCREG_POSITION:
      _debug_printf("vPos");
      case SVGA3DMISCREG_FACE:
      _debug_printf("vFace");
      default:
      assert(0);
      }
         case SVGA3DREG_LABEL:
      format_reg("l", reg, NULL);
         case SVGA3DREG_PREDICATE:
      format_reg("p", reg, NULL);
         default:
      assert( 0 );
         }
      static void dump_cdata( struct sh_cdata cdata )
   {
         }
      static void dump_idata( struct sh_idata idata )
   {
         }
      static void dump_bdata( bool bdata )
   {
         }
      static void
   dump_sampleinfo(struct sh_sampleinfo sampleinfo)
   {
               switch (sampleinfo.texture_type) {
   case SVGA3DSAMP_2D:
      _debug_printf( "_2d" );
      case SVGA3DSAMP_2D_SHADOW:
      _debug_printf( "_2dshadow" );
      case SVGA3DSAMP_CUBE:
      _debug_printf( "_cube" );
      case SVGA3DSAMP_VOLUME:
      _debug_printf( "_volume" );
      default:
            }
      static void
   dump_semantic(uint usage,
         {
      switch (usage) {
   case SVGA3D_DECLUSAGE_POSITION:
      _debug_printf("_position");
      case SVGA3D_DECLUSAGE_BLENDWEIGHT:
      _debug_printf("_blendweight");
      case SVGA3D_DECLUSAGE_BLENDINDICES:
      _debug_printf("_blendindices");
      case SVGA3D_DECLUSAGE_NORMAL:
      _debug_printf("_normal");
      case SVGA3D_DECLUSAGE_PSIZE:
      _debug_printf("_psize");
      case SVGA3D_DECLUSAGE_TEXCOORD:
      _debug_printf("_texcoord");
      case SVGA3D_DECLUSAGE_TANGENT:
      _debug_printf("_tangent");
      case SVGA3D_DECLUSAGE_BINORMAL:
      _debug_printf("_binormal");
      case SVGA3D_DECLUSAGE_TESSFACTOR:
      _debug_printf("_tessfactor");
      case SVGA3D_DECLUSAGE_POSITIONT:
      _debug_printf("_positiont");
      case SVGA3D_DECLUSAGE_COLOR:
      _debug_printf("_color");
      case SVGA3D_DECLUSAGE_FOG:
      _debug_printf("_fog");
      case SVGA3D_DECLUSAGE_DEPTH:
      _debug_printf("_depth");
      case SVGA3D_DECLUSAGE_SAMPLE:
      _debug_printf("_sample");
      default:
      assert(!"Unknown usage");
               if (usage_index) {
            }
      static void
   dump_dstreg(struct sh_dstreg dstreg,
               {
      union {
      struct sh_reg reg;
                                 if (dstreg.modifier & SVGA3DDSTMOD_SATURATE)
         if (dstreg.modifier & SVGA3DDSTMOD_PARTIALPRECISION)
         switch (dstreg.shift_scale) {
   case 0:
         case 1:
      _debug_printf( "_x2" );
      case 2:
      _debug_printf( "_x4" );
      case 3:
      _debug_printf( "_x8" );
      case 13:
      _debug_printf( "_d8" );
      case 14:
      _debug_printf( "_d4" );
      case 15:
      _debug_printf( "_d2" );
      default:
         }
            u.dstreg = dstreg;
   dump_reg( u.reg, indreg, di);
   if (dstreg.write_mask != SVGA3DWRITEMASK_ALL) {
      _debug_printf( "." );
   if (dstreg.write_mask & SVGA3DWRITEMASK_0)
         if (dstreg.write_mask & SVGA3DWRITEMASK_1)
         if (dstreg.write_mask & SVGA3DWRITEMASK_2)
         if (dstreg.write_mask & SVGA3DWRITEMASK_3)
         }
      static void dump_srcreg( struct sh_srcreg srcreg, struct sh_srcreg *indreg, const struct dump_info *di )
   {
      struct sh_reg srcreg_sh = {0};
   /* bit-fields carefully aligned, ensure they stay that way. */
   STATIC_ASSERT(sizeof(struct sh_reg) == sizeof(struct sh_srcreg));
            switch (srcreg.modifier) {
   case SVGA3DSRCMOD_NEG:
   case SVGA3DSRCMOD_BIASNEG:
   case SVGA3DSRCMOD_SIGNNEG:
   case SVGA3DSRCMOD_X2NEG:
   case SVGA3DSRCMOD_ABSNEG:
      _debug_printf( "-" );
      case SVGA3DSRCMOD_COMP:
      _debug_printf( "1-" );
      case SVGA3DSRCMOD_NOT:
         }
   dump_reg(srcreg_sh, indreg, di );
   switch (srcreg.modifier) {
   case SVGA3DSRCMOD_NONE:
   case SVGA3DSRCMOD_NEG:
   case SVGA3DSRCMOD_COMP:
   case SVGA3DSRCMOD_NOT:
         case SVGA3DSRCMOD_BIAS:
   case SVGA3DSRCMOD_BIASNEG:
      _debug_printf( "_bias" );
      case SVGA3DSRCMOD_SIGN:
   case SVGA3DSRCMOD_SIGNNEG:
      _debug_printf( "_bx2" );
      case SVGA3DSRCMOD_X2:
   case SVGA3DSRCMOD_X2NEG:
      _debug_printf( "_x2" );
      case SVGA3DSRCMOD_DZ:
      _debug_printf( "_dz" );
      case SVGA3DSRCMOD_DW:
      _debug_printf( "_dw" );
      case SVGA3DSRCMOD_ABS:
   case SVGA3DSRCMOD_ABSNEG:
      _debug_printf("_abs");
      default:
         }
   if (srcreg.swizzle_x != 0 || srcreg.swizzle_y != 1 || srcreg.swizzle_z != 2 || srcreg.swizzle_w != 3) {
      _debug_printf( "." );
   if (srcreg.swizzle_x == srcreg.swizzle_y && srcreg.swizzle_y == srcreg.swizzle_z && srcreg.swizzle_z == srcreg.swizzle_w) {
         }
   else {
      _debug_printf( "%c", "xyzw"[srcreg.swizzle_x] );
   _debug_printf( "%c", "xyzw"[srcreg.swizzle_y] );
   _debug_printf( "%c", "xyzw"[srcreg.swizzle_z] );
            }
      static void
   parse_op(struct dump_info *di,
            const uint **token,
   struct dump_op *op,
      {
               assert(num_dst <= 1);
            op->op = *(struct sh_op *)*token;
            if (num_dst >= 1) {
      op->dst = *(struct sh_dstreg *)*token;
   *token += sizeof(struct sh_dstreg) / sizeof(uint);
   if (op->dst.relative &&
      (!di->is_ps && di->version >= SVGA3D_VS_30)) {
   op->dstind = *(struct sh_srcreg *)*token;
                  if (op->op.predicated) {
      op->p0 = *(struct sh_srcreg *)*token;
               for (i = 0; i < num_src; ++i) {
      op->src[i] = *(struct sh_srcreg *)*token;
   *token += sizeof(struct sh_srcreg) / sizeof(uint);
   if (op->src[i].relative &&
      ((!di->is_ps && di->version >= SVGA3D_VS_20) ||
   (di->is_ps && di->version >= SVGA3D_PS_30))) {
   op->srcind[i] = *(struct sh_srcreg *)*token;
            }
      static void
   dump_inst(struct dump_info *di,
            const unsigned **assem,
      {
      struct dump_op dop;
   bool not_first_arg = false;
                     di->indent -= info->pre_dedent;
   dump_indent(di->indent);
                     parse_op(di, assem, &dop, info->num_dst, info->num_src);
   if (info->num_dst > 0) {
      dump_dstreg(dop.dst, &dop.dstind, di);
               for (i = 0; i < info->num_src; i++) {
      if (not_first_arg) {
         } else {
         }
   dump_srcreg(dop.src[i], &dop.srcind[i], di);
                  }
      void
   svga_shader_dump(
      const unsigned *assem,
   unsigned dwords,
      {
      bool finished = false;
            di.version = *assem++;
   di.is_ps = (di.version & 0xFFFF0000) == 0xFFFF0000;
            _debug_printf(
      "%s_%u_%u\n",
   di.is_ps ? "ps" : "vs",
   (di.version >> 8) & 0xff,
         while (!finished) {
               switch (op.opcode) {
   case SVGA3DOP_DCL:
                        _debug_printf( "dcl" );
   switch (sh_dstreg_type(dcl.reg)) {
   case SVGA3DREG_INPUT:
      if ((di.is_ps && di.version >= SVGA3D_PS_30) ||
      (!di.is_ps && di.version >= SVGA3D_VS_30)) {
   dump_semantic(dcl.u.semantic.usage,
      }
      case SVGA3DREG_TEXCRDOUT:
      if (!di.is_ps && di.version >= SVGA3D_VS_30) {
      dump_semantic(dcl.u.semantic.usage,
      }
      case SVGA3DREG_SAMPLER:
      dump_sampleinfo( dcl.u.sampleinfo );
      }
   dump_dstreg(dcl.reg, NULL, &di);
   _debug_printf( "\n" );
                  case SVGA3DOP_DEFB:
                        _debug_printf( "defb " );
   dump_reg( defb.reg, NULL, &di );
   _debug_printf( ", " );
   dump_bdata( defb.data );
   _debug_printf( "\n" );
                  case SVGA3DOP_DEFI:
                        _debug_printf( "defi " );
   dump_reg( defi.reg, NULL, &di );
   _debug_printf( ", " );
   dump_idata( defi.idata );
   _debug_printf( "\n" );
                  case SVGA3DOP_TEXCOORD:
                        assert(di.is_ps);
                                                case SVGA3DOP_TEX:
                        assert(di.is_ps);
                     if (di.version > SVGA3D_PS_14) {
      info.num_src = 2;
      } else {
                                    case SVGA3DOP_DEF:
                        _debug_printf( "def " );
   dump_reg( def.reg, NULL, &di );
   _debug_printf( ", " );
   dump_cdata( def.cdata );
   _debug_printf( "\n" );
                  case SVGA3DOP_SINCOS:
                        if ((di.is_ps && di.version >= SVGA3D_PS_30) ||
                                                case SVGA3DOP_PHASE:
      _debug_printf( "phase\n" );
               case SVGA3DOP_COMMENT:
                        /* Ignore comment contents. */
                  case SVGA3DOP_END:
                  default:
                                    }
