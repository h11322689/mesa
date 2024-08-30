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
   #include "tgsi_info.h"
   #include "tgsi_parse.h"
   #include "tgsi_util.h"
   #include "tgsi_exec.h"
   #include "util/bitscan.h"
      union pointer_hack
   {
      void *pointer;
      };
      unsigned
   tgsi_util_get_src_register_swizzle(const struct tgsi_src_register *reg,
         {
      switch (component) {
   case TGSI_CHAN_X:
         case TGSI_CHAN_Y:
         case TGSI_CHAN_Z:
         case TGSI_CHAN_W:
         default:
         }
      }
         unsigned
   tgsi_util_get_full_src_register_swizzle(
      const struct tgsi_full_src_register *reg,
      {
         }
         /**
   * Determine which channels of the specificed src register are effectively
   * used by this instruction.
   */
   unsigned
   tgsi_util_get_src_usage_mask(enum tgsi_opcode opcode,
                              unsigned src_idx,
   uint8_t write_mask,
   uint8_t swizzle_x,
      {
      unsigned read_mask;
            switch (opcode) {
   case TGSI_OPCODE_IF:
   case TGSI_OPCODE_UIF:
   case TGSI_OPCODE_EMIT:
   case TGSI_OPCODE_ENDPRIM:
   case TGSI_OPCODE_RCP:
   case TGSI_OPCODE_RSQ:
   case TGSI_OPCODE_SQRT:
   case TGSI_OPCODE_EX2:
   case TGSI_OPCODE_LG2:
   case TGSI_OPCODE_SIN:
   case TGSI_OPCODE_COS:
   case TGSI_OPCODE_POW: /* reads src0.x and src1.x */
   case TGSI_OPCODE_UP2H:
   case TGSI_OPCODE_UP2US:
   case TGSI_OPCODE_UP4B:
   case TGSI_OPCODE_UP4UB:
   case TGSI_OPCODE_MEMBAR:
   case TGSI_OPCODE_BALLOT:
      read_mask = TGSI_WRITEMASK_X;
         case TGSI_OPCODE_DP2:
   case TGSI_OPCODE_PK2H:
   case TGSI_OPCODE_PK2US:
   case TGSI_OPCODE_F2D:
   case TGSI_OPCODE_I2D:
   case TGSI_OPCODE_U2D:
   case TGSI_OPCODE_F2U64:
   case TGSI_OPCODE_F2I64:
   case TGSI_OPCODE_U2I64:
   case TGSI_OPCODE_I2I64:
   case TGSI_OPCODE_TXQS: /* bindless handle possible */
   case TGSI_OPCODE_RESQ: /* bindless handle possible */
      read_mask = TGSI_WRITEMASK_XY;
         case TGSI_OPCODE_TXQ:
      if (src_idx == 0)
         else
               case TGSI_OPCODE_DP3:
      read_mask = TGSI_WRITEMASK_XYZ;
         case TGSI_OPCODE_DSEQ:
   case TGSI_OPCODE_DSNE:
   case TGSI_OPCODE_DSLT:
   case TGSI_OPCODE_DSGE:
   case TGSI_OPCODE_DP4:
   case TGSI_OPCODE_PK4B:
   case TGSI_OPCODE_PK4UB:
   case TGSI_OPCODE_D2F:
   case TGSI_OPCODE_D2I:
   case TGSI_OPCODE_D2U:
   case TGSI_OPCODE_I2F:
   case TGSI_OPCODE_U2F:
   case TGSI_OPCODE_U64SEQ:
   case TGSI_OPCODE_U64SNE:
   case TGSI_OPCODE_U64SLT:
   case TGSI_OPCODE_U64SGE:
   case TGSI_OPCODE_U642F:
   case TGSI_OPCODE_I64SLT:
   case TGSI_OPCODE_I64SGE:
   case TGSI_OPCODE_I642F:
      read_mask = TGSI_WRITEMASK_XYZW;
         case TGSI_OPCODE_LIT:
      read_mask = write_mask & TGSI_WRITEMASK_YZ ?
               case TGSI_OPCODE_EXP:
   case TGSI_OPCODE_LOG:
      read_mask = write_mask & TGSI_WRITEMASK_XYZ ? TGSI_WRITEMASK_X : 0;
         case TGSI_OPCODE_DST:
      if (src_idx == 0)
         else
               case TGSI_OPCODE_DLDEXP:
      if (src_idx == 0) {
         } else {
      read_mask =
      (write_mask & TGSI_WRITEMASK_XY ? TGSI_WRITEMASK_X : 0) |
   }
         case TGSI_OPCODE_READ_INVOC:
      if (src_idx == 0)
         else
               case TGSI_OPCODE_FBFETCH:
      read_mask = 0; /* not a real register read */
         case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TEX_LZ:
   case TGSI_OPCODE_TXF_LZ:
   case TGSI_OPCODE_TXF:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXL:
   case TGSI_OPCODE_TXP:
   case TGSI_OPCODE_TXD:
   case TGSI_OPCODE_TEX2:
   case TGSI_OPCODE_TXB2:
   case TGSI_OPCODE_TXL2:
   case TGSI_OPCODE_LODQ:
   case TGSI_OPCODE_TG4: {
      unsigned dim_layer =
                  /* Add shadow. */
   if (tgsi_is_shadow_target(tex_target)) {
      dim_layer_shadow = dim_layer + 1;
   if (tex_target == TGSI_TEXTURE_SHADOW1D)
      } else {
                  /* Remove layer. */
   if (tgsi_is_array_sampler(tex_target))
         else
                     switch (src_idx) {
   case 0:
      if (opcode == TGSI_OPCODE_LODQ)
                                       if (opcode == TGSI_OPCODE_TXF ||
      opcode == TGSI_OPCODE_TXB ||
   opcode == TGSI_OPCODE_TXL ||
   opcode == TGSI_OPCODE_TXP)
            case 1:
      if (opcode == TGSI_OPCODE_TXD)
         else if (opcode == TGSI_OPCODE_TEX2 ||
            opcode == TGSI_OPCODE_TXB2 ||
   opcode == TGSI_OPCODE_TXL2 ||
            case 2:
      if (opcode == TGSI_OPCODE_TXD)
            }
               case TGSI_OPCODE_LOAD:
      if (src_idx == 0) {
         } else {
      unsigned dim = tgsi_util_get_texture_coord_dim(mem_target);
      }
         case TGSI_OPCODE_STORE:
      if (src_idx == 0) {
      unsigned dim = tgsi_util_get_texture_coord_dim(mem_target);
      } else {
         }
         case TGSI_OPCODE_ATOMUADD:
   case TGSI_OPCODE_ATOMXCHG:
   case TGSI_OPCODE_ATOMCAS:
   case TGSI_OPCODE_ATOMAND:
   case TGSI_OPCODE_ATOMOR:
   case TGSI_OPCODE_ATOMXOR:
   case TGSI_OPCODE_ATOMUMIN:
   case TGSI_OPCODE_ATOMUMAX:
   case TGSI_OPCODE_ATOMIMIN:
   case TGSI_OPCODE_ATOMIMAX:
   case TGSI_OPCODE_ATOMFADD:
      if (src_idx == 0) {
         } else if (src_idx == 1) {
      unsigned dim = tgsi_util_get_texture_coord_dim(mem_target);
      } else {
         }
         case TGSI_OPCODE_INTERP_CENTROID:
   case TGSI_OPCODE_INTERP_SAMPLE:
   case TGSI_OPCODE_INTERP_OFFSET:
      if (src_idx == 0)
         else if (opcode == TGSI_OPCODE_INTERP_OFFSET)
         else
               default:
      if (tgsi_get_opcode_info(opcode)->output_mode ==
      TGSI_OUTPUT_COMPONENTWISE)
      else
                     usage_mask = 0;
   if (read_mask & TGSI_WRITEMASK_X)
         if (read_mask & TGSI_WRITEMASK_Y)
         if (read_mask & TGSI_WRITEMASK_Z)
         if (read_mask & TGSI_WRITEMASK_W)
               }
      unsigned
   tgsi_util_get_inst_usage_mask(const struct tgsi_full_instruction *inst,
         {
      return tgsi_util_get_src_usage_mask(inst->Instruction.Opcode, src_idx,
                                          }
      /**
   * Return the dimension of the texture coordinates (layer included for array
   * textures), as well as the location of the shadow reference value or the
   * sample index.
   */
   int
   tgsi_util_get_texture_coord_dim(enum tgsi_texture_type tgsi_tex)
   {
      /*
   * Depending on the texture target, (src0.xyzw, src1.x) is interpreted
   * differently:
   *
   *   (s, X, X, X, X),               for BUFFER
   *   (s, X, X, X, X),               for 1D
   *   (s, t, X, X, X),               for 2D, RECT
   *   (s, t, r, X, X),               for 3D, CUBE
   *
   *   (s, layer, X, X, X),           for 1D_ARRAY
   *   (s, t, layer, X, X),           for 2D_ARRAY
   *   (s, t, r, layer, X),           for CUBE_ARRAY
   *
   *   (s, X, shadow, X, X),          for SHADOW1D
   *   (s, t, shadow, X, X),          for SHADOW2D, SHADOWRECT
   *   (s, t, r, shadow, X),          for SHADOWCUBE
   *
   *   (s, layer, shadow, X, X),      for SHADOW1D_ARRAY
   *   (s, t, layer, shadow, X),      for SHADOW2D_ARRAY
   *   (s, t, r, layer, shadow),      for SHADOWCUBE_ARRAY
   *
   *   (s, t, sample, X, X),          for 2D_MSAA
   *   (s, t, layer, sample, X),      for 2D_ARRAY_MSAA
   */
   switch (tgsi_tex) {
   case TGSI_TEXTURE_BUFFER:
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_SHADOW1D:
         case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
   case TGSI_TEXTURE_2D_MSAA:
         case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
         case TGSI_TEXTURE_CUBE_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
         default:
      assert(!"unknown texture target");
         }
         /**
   * Given a TGSI_TEXTURE_x target, return register component where the
   * shadow reference/distance coordinate is found.  Typically, components
   * 0 and 1 are the (s,t) texcoords and component 2 or 3 hold the shadow
   * reference value.  But if we return 4, it means the reference value is
   * found in the 0th component of the second coordinate argument to the
   * TEX2 instruction.
   */
   int
   tgsi_util_get_shadow_ref_src_index(enum tgsi_texture_type tgsi_tex)
   {
      switch (tgsi_tex) {
   case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
         case TGSI_TEXTURE_SHADOWCUBE:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_2D_MSAA:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
         case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
         default:
      /* no shadow nor sample */
         }
         bool
   tgsi_is_shadow_target(enum tgsi_texture_type target)
   {
      switch (target) {
   case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE:
   case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
         default:
            }
