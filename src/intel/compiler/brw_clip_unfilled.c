   /*
   Copyright (C) Intel Corp.  2006.  All Rights Reserved.
   Intel funded Tungsten Graphics to
   develop this 3D driver.
      Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:
      The above copyright notice and this permission notice (including the
   next paragraph) shall be included in all copies or substantial
   portions of the Software.
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
      **********************************************************************/
   /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   */
      #include "brw_clip.h"
   #include "brw_prim.h"
         /* This is performed against the original triangles, so no indirection
   * required:
   BZZZT!
   */
   static void compute_tri_direction( struct brw_clip_compile *c )
   {
      struct brw_codegen *p = &c->func;
   struct brw_reg e = c->reg.tmp0;
   struct brw_reg f = c->reg.tmp1;
   GLuint hpos_offset = brw_varying_to_offset(&c->vue_map, VARYING_SLOT_POS);
   struct brw_reg v0 = byte_offset(c->reg.vertex[0], hpos_offset);
   struct brw_reg v1 = byte_offset(c->reg.vertex[1], hpos_offset);
               struct brw_reg v0n = get_tmp(c);
   struct brw_reg v1n = get_tmp(c);
            /* Convert to NDC.
   * NOTE: We can't modify the original vertex coordinates,
   * as it may impact further operations.
   * So, we have to keep normalized coordinates in temp registers.
   *
   * TBD-KC
   * Try to optimize unnecessary MOV's.
   */
   brw_MOV(p, v0n, v0);
   brw_MOV(p, v1n, v1);
            brw_clip_project_position(c, v0n);
   brw_clip_project_position(c, v1n);
            /* Calculate the vectors of two edges of the triangle:
   */
   brw_ADD(p, e, v0n, negate(v2n));
            /* Take their crossproduct:
   */
   brw_set_default_access_mode(p, BRW_ALIGN_16);
   brw_MUL(p, vec4(brw_null_reg()), brw_swizzle(e, BRW_SWIZZLE_YZXW),
         brw_MAC(p, vec4(e),  negate(brw_swizzle(e, BRW_SWIZZLE_ZXYW)),
                     }
         static void cull_direction( struct brw_clip_compile *c )
   {
      struct brw_codegen *p = &c->func;
            assert (!(c->key.fill_ccw == BRW_CLIP_FILL_MODE_CULL &&
            if (c->key.fill_ccw == BRW_CLIP_FILL_MODE_CULL)
         else
            brw_CMP(p,
   vec1(brw_null_reg()),
   conditional,
   get_element(c->reg.dir, 2),
            brw_IF(p, BRW_EXECUTE_1);
   {
         }
      }
            static void copy_bfc( struct brw_clip_compile *c )
   {
      struct brw_codegen *p = &c->func;
            /* Do we have any colors to copy?
   */
   if (!(brw_clip_have_varying(c, VARYING_SLOT_COL0) &&
            !(brw_clip_have_varying(c, VARYING_SLOT_COL1) &&
               /* In some weird degenerate cases we can end up testing the
   * direction twice, once for culling and once for bfc copying.  Oh
   * well, that's what you get for setting weird GL state.
   */
   if (c->key.copy_bfc_ccw)
         else
            brw_CMP(p,
   vec1(brw_null_reg()),
   conditional,
   get_element(c->reg.dir, 2),
            brw_IF(p, BRW_EXECUTE_1);
   {
               if (brw_clip_have_varying(c, VARYING_SLOT_COL0) &&
            brw_MOV(p,
      byte_offset(c->reg.vertex[i],
               byte_offset(c->reg.vertex[i],
            if (brw_clip_have_varying(c, VARYING_SLOT_COL1) &&
            brw_MOV(p,
      byte_offset(c->reg.vertex[i],
               byte_offset(c->reg.vertex[i],
                  }
      }
               /*
   GLfloat iz	= 1.0 / dir.z;
   GLfloat ac	= dir.x * iz;
   GLfloat bc	= dir.y * iz;
   offset = ctx->Polygon.OffsetUnits * DEPTH_SCALE;
   offset += MAX2( abs(ac), abs(bc) ) * ctx->Polygon.OffsetFactor;
   if (ctx->Polygon.OffsetClamp && isfinite(ctx->Polygon.OffsetClamp)) {
      if (ctx->Polygon.OffsetClamp < 0)
         else
      }
   offset *= MRD;
   */
   static void compute_offset( struct brw_clip_compile *c )
   {
      struct brw_codegen *p = &c->func;
   struct brw_reg off = c->reg.offset;
            brw_math_invert(p, get_element(off, 2), get_element(dir, 2));
            brw_CMP(p,
   vec1(brw_null_reg()),
   BRW_CONDITIONAL_GE,
   brw_abs(get_element(off, 0)),
            brw_SEL(p, vec1(off),
                  brw_MUL(p, vec1(off), vec1(off), brw_imm_f(c->key.offset_factor));
   brw_ADD(p, vec1(off), vec1(off), brw_imm_f(c->key.offset_units));
   if (c->key.offset_clamp && isfinite(c->key.offset_clamp)) {
      brw_CMP(p,
         vec1(brw_null_reg()),
   c->key.offset_clamp < 0 ? BRW_CONDITIONAL_GE : BRW_CONDITIONAL_L,
   vec1(off),
         }
         static void merge_edgeflags( struct brw_clip_compile *c )
   {
      struct brw_codegen *p = &c->func;
            brw_AND(p, tmp0, get_element_ud(c->reg.R0, 2), brw_imm_ud(PRIM_MASK));
   brw_CMP(p,
   vec1(brw_null_reg()),
   BRW_CONDITIONAL_EQ,
   tmp0,
            /* Get away with using reg.vertex because we know that this is not
   * a _3DPRIM_TRISTRIP_REVERSE:
   */
   brw_IF(p, BRW_EXECUTE_1);
   {
      brw_AND(p, vec1(brw_null_reg()), get_element_ud(c->reg.R0, 2), brw_imm_ud(1<<8));
   brw_inst_set_cond_modifier(p->devinfo, brw_last_inst, BRW_CONDITIONAL_EQ);
   brw_MOV(p, byte_offset(c->reg.vertex[0],
                              brw_AND(p, vec1(brw_null_reg()), get_element_ud(c->reg.R0, 2), brw_imm_ud(1<<9));
   brw_inst_set_cond_modifier(p->devinfo, brw_last_inst, BRW_CONDITIONAL_EQ);
   brw_MOV(p, byte_offset(c->reg.vertex[2],
                        }
      }
            static void apply_one_offset( struct brw_clip_compile *c,
         {
      struct brw_codegen *p = &c->func;
   GLuint ndc_offset = brw_varying_to_offset(&c->vue_map,
         struct brw_reg z = deref_1f(vert, ndc_offset +
               }
            /***********************************************************************
   * Output clipped polygon as an unfilled primitive:
   */
   static void emit_lines(struct brw_clip_compile *c,
         {
      struct brw_codegen *p = &c->func;
   struct brw_indirect v0 = brw_indirect(0, 0);
   struct brw_indirect v1 = brw_indirect(1, 0);
   struct brw_indirect v0ptr = brw_indirect(2, 0);
            /* Need a separate loop for offset:
   */
   if (do_offset) {
      brw_MOV(p, c->reg.loopcount, c->reg.nr_verts);
            brw_DO(p, BRW_EXECUTE_1);
   brw_MOV(p, get_addr_reg(v0), deref_1uw(v0ptr, 0));
   brw_ADD(p, get_addr_reg(v0ptr), get_addr_reg(v0ptr), brw_imm_uw(2));
      apply_one_offset(c, v0);
      brw_ADD(p, c->reg.loopcount, c->reg.loopcount, brw_imm_d(-1));
               }
   brw_WHILE(p);
               /* v1ptr = &inlist[nr_verts]
   * *v1ptr = v0
   */
   brw_MOV(p, c->reg.loopcount, c->reg.nr_verts);
   brw_MOV(p, get_addr_reg(v0ptr), brw_address(c->reg.inlist));
   brw_ADD(p, get_addr_reg(v1ptr), get_addr_reg(v0ptr), retype(c->reg.nr_verts, BRW_REGISTER_TYPE_UW));
   brw_ADD(p, get_addr_reg(v1ptr), get_addr_reg(v1ptr), retype(c->reg.nr_verts, BRW_REGISTER_TYPE_UW));
            brw_DO(p, BRW_EXECUTE_1);
   {
      brw_MOV(p, get_addr_reg(v0), deref_1uw(v0ptr, 0));
   brw_MOV(p, get_addr_reg(v1), deref_1uw(v0ptr, 2));
            /* draw edge if edgeflag != 0 */
   brw_CMP(p,
   vec1(brw_null_reg()), BRW_CONDITIONAL_NZ,
   deref_1f(v0, brw_varying_to_offset(&c->vue_map,
         brw_imm_f(0));
   brw_IF(p, BRW_EXECUTE_1);
   brw_clip_emit_vue(c, v0, BRW_URB_WRITE_ALLOCATE_COMPLETE,
               brw_clip_emit_vue(c, v1, BRW_URB_WRITE_ALLOCATE_COMPLETE,
                     }
            brw_ADD(p, c->reg.loopcount, c->reg.loopcount, brw_imm_d(-1));
      }
   brw_WHILE(p);
      }
            static void emit_points(struct brw_clip_compile *c,
         {
               struct brw_indirect v0 = brw_indirect(0, 0);
            brw_MOV(p, c->reg.loopcount, c->reg.nr_verts);
            brw_DO(p, BRW_EXECUTE_1);
   {
      brw_MOV(p, get_addr_reg(v0), deref_1uw(v0ptr, 0));
            /* draw if edgeflag != 0
   */
   brw_CMP(p,
   vec1(brw_null_reg()), BRW_CONDITIONAL_NZ,
   deref_1f(v0, brw_varying_to_offset(&c->vue_map,
         brw_imm_f(0));
   brw_IF(p, BRW_EXECUTE_1);
   if (do_offset)
            brw_clip_emit_vue(c, v0, BRW_URB_WRITE_ALLOCATE_COMPLETE,
                     }
            brw_ADD(p, c->reg.loopcount, c->reg.loopcount, brw_imm_d(-1));
      }
   brw_WHILE(p);
      }
                        static void emit_primitives( struct brw_clip_compile *c,
         GLuint mode,
   {
      switch (mode) {
   case BRW_CLIP_FILL_MODE_FILL:
      brw_clip_tri_emit_polygon(c);
         case BRW_CLIP_FILL_MODE_LINE:
      emit_lines(c, do_offset);
         case BRW_CLIP_FILL_MODE_POINT:
      emit_points(c, do_offset);
         case BRW_CLIP_FILL_MODE_CULL:
            }
            static void emit_unfilled_primitives( struct brw_clip_compile *c )
   {
               /* Direction culling has already been done.
   */
   if (c->key.fill_ccw != c->key.fill_cw &&
      c->key.fill_ccw != BRW_CLIP_FILL_MODE_CULL &&
      {
      brw_CMP(p,
   vec1(brw_null_reg()),
   BRW_CONDITIONAL_GE,
   get_element(c->reg.dir, 2),
            brw_IF(p, BRW_EXECUTE_1);
   emit_primitives(c, c->key.fill_ccw, c->key.offset_ccw);
         }
   brw_ELSE(p);
   emit_primitives(c, c->key.fill_cw, c->key.offset_cw);
         }
      }
   else if (c->key.fill_cw != BRW_CLIP_FILL_MODE_CULL) {
         }
   else if (c->key.fill_ccw != BRW_CLIP_FILL_MODE_CULL) {
            }
               static void check_nr_verts( struct brw_clip_compile *c )
   {
               brw_CMP(p, vec1(brw_null_reg()), BRW_CONDITIONAL_L, c->reg.nr_verts, brw_imm_d(3));
   brw_IF(p, BRW_EXECUTE_1);
   {
         }
      }
         void brw_emit_unfilled_clip( struct brw_clip_compile *c )
   {
               c->need_direction = ((c->key.offset_ccw || c->key.offset_cw) ||
   (c->key.fill_ccw != c->key.fill_cw) ||
   c->key.fill_ccw == BRW_CLIP_FILL_MODE_CULL ||
   c->key.fill_cw == BRW_CLIP_FILL_MODE_CULL ||
   c->key.copy_bfc_cw ||
            brw_clip_tri_alloc_regs(c, 3 + c->key.nr_userclip + 6);
   brw_clip_tri_init_vertices(c);
                     if (c->key.fill_ccw == BRW_CLIP_FILL_MODE_CULL &&
      c->key.fill_cw == BRW_CLIP_FILL_MODE_CULL) {
   brw_clip_kill_thread(c);
                        /* Need to use the inlist indirection here:
   */
   if (c->need_direction)
            if (c->key.fill_ccw == BRW_CLIP_FILL_MODE_CULL ||
      c->key.fill_cw == BRW_CLIP_FILL_MODE_CULL)
         if (c->key.offset_ccw ||
      c->key.offset_cw)
         if (c->key.copy_bfc_ccw ||
      c->key.copy_bfc_cw)
         /* Need to do this whether we clip or not:
   */
   if (c->key.contains_flat_varying)
            brw_clip_init_clipmask(c);
   brw_CMP(p, vec1(brw_null_reg()), BRW_CONDITIONAL_NZ, c->reg.planemask, brw_imm_ud(0));
   brw_IF(p, BRW_EXECUTE_1);
   {
      brw_clip_init_planes(c);
   brw_clip_tri(c);
      }
            emit_unfilled_primitives(c);
      }
