   /*
   * Copyright © 2006 - 2017 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "brw_compiler.h"
   #include "brw_eu.h"
   #include "brw_prim.h"
      #include "dev/intel_debug.h"
      struct brw_sf_compile {
      struct brw_codegen func;
   struct brw_sf_prog_key key;
            struct brw_reg pv;
   struct brw_reg det;
   struct brw_reg dx0;
   struct brw_reg dx2;
   struct brw_reg dy0;
            /* z and 1/w passed in separately:
   */
   struct brw_reg z[3];
            /* The vertices:
   */
            /* Temporaries, allocated after last vertex reg.
   */
   struct brw_reg inv_det;
   struct brw_reg a1_sub_a0;
   struct brw_reg a2_sub_a0;
            struct brw_reg m1Cx;
   struct brw_reg m2Cy;
            GLuint nr_verts;
   GLuint nr_attr_regs;
   GLuint nr_setup_regs;
            /** The last known value of the f0.0 flag register. */
               };
      /**
   * Determine the vue slot corresponding to the given half of the given register.
   */
   static inline int vert_reg_to_vue_slot(struct brw_sf_compile *c, GLuint reg,
         {
         }
      /**
   * Determine the varying corresponding to the given half of the given
   * register.  half=0 means the first half of a register, half=1 means the
   * second half.
   */
   static inline int vert_reg_to_varying(struct brw_sf_compile *c, GLuint reg,
         {
      int vue_slot = vert_reg_to_vue_slot(c, reg, half);
      }
      /**
   * Determine the register corresponding to the given vue slot
   */
   static struct brw_reg get_vue_slot(struct brw_sf_compile *c,
               {
      GLuint off = vue_slot / 2 - c->urb_entry_read_offset;
               }
      /**
   * Determine the register corresponding to the given varying.
   */
   static struct brw_reg get_varying(struct brw_sf_compile *c,
               {
      int vue_slot = c->vue_map.varying_to_slot[varying];
   assert (vue_slot >= c->urb_entry_read_offset);
      }
      static bool
   have_attr(struct brw_sf_compile *c, GLuint attr)
   {
         }
      /***********************************************************************
   * Twoside lighting
   */
   static void copy_bfc( struct brw_sf_compile *c,
         {
      struct brw_codegen *p = &c->func;
            for (i = 0; i < 2; i++) {
            brw_MOV(p,
      get_varying(c, vert, VARYING_SLOT_COL0+i),
   get_varying(c, vert, VARYING_SLOT_BFC0+i));
      }
         static void do_twoside_color( struct brw_sf_compile *c )
   {
      struct brw_codegen *p = &c->func;
            /* Already done in clip program:
   */
   if (c->key.primitive == BRW_SF_PRIM_UNFILLED_TRIS)
            /* If the vertex shader provides backface color, do the selection. The VS
   * promises to set up the front color if the backface color is provided, but
   * it may contain junk if never written to.
   */
   if (!(have_attr(c, VARYING_SLOT_COL0) && have_attr(c, VARYING_SLOT_BFC0)) &&
      !(have_attr(c, VARYING_SLOT_COL1) && have_attr(c, VARYING_SLOT_BFC1)))
         /* Need to use BRW_EXECUTE_4 and also do an 4-wide compare in order
   * to get all channels active inside the IF.  In the clipping code
   * we run with NoMask, so it's not an option and we can use
   * BRW_EXECUTE_1 for all comparisons.
   */
   brw_CMP(p, vec4(brw_null_reg()), backface_conditional, c->det, brw_imm_f(0));
   brw_IF(p, BRW_EXECUTE_4);
   {
      switch (c->nr_verts) {
   case 3: copy_bfc(c, c->vert[2]); FALLTHROUGH;
   case 2: copy_bfc(c, c->vert[1]); FALLTHROUGH;
   case 1: copy_bfc(c, c->vert[0]);
      }
      }
            /***********************************************************************
   * Flat shading
   */
      static void copy_flatshaded_attributes(struct brw_sf_compile *c,
               {
      struct brw_codegen *p = &c->func;
            for (i = 0; i < c->vue_map.num_slots; i++) {
      if (c->key.interp_mode[i] == INTERP_MODE_FLAT) {
      brw_MOV(p,
                  }
      static int count_flatshaded_attributes(struct brw_sf_compile *c)
   {
      int i;
            for (i = 0; i < c->vue_map.num_slots; i++)
      if (c->key.interp_mode[i] == INTERP_MODE_FLAT)
            }
            /* Need to use a computed jump to copy flatshaded attributes as the
   * vertices are ordered according to y-coordinate before reaching this
   * point, so the PV could be anywhere.
   */
   static void do_flatshade_triangle( struct brw_sf_compile *c )
   {
      struct brw_codegen *p = &c->func;
   GLuint nr;
            /* Already done in clip program:
   */
   if (c->key.primitive == BRW_SF_PRIM_UNFILLED_TRIS)
            if (p->devinfo->ver == 5)
                     brw_MUL(p, c->pv, c->pv, brw_imm_d(jmpi*(nr*2+1)));
            copy_flatshaded_attributes(c, c->vert[1], c->vert[0]);
   copy_flatshaded_attributes(c, c->vert[2], c->vert[0]);
            copy_flatshaded_attributes(c, c->vert[0], c->vert[1]);
   copy_flatshaded_attributes(c, c->vert[2], c->vert[1]);
            copy_flatshaded_attributes(c, c->vert[0], c->vert[2]);
      }
         static void do_flatshade_line( struct brw_sf_compile *c )
   {
      struct brw_codegen *p = &c->func;
   GLuint nr;
            /* Already done in clip program:
   */
   if (c->key.primitive == BRW_SF_PRIM_UNFILLED_TRIS)
            if (p->devinfo->ver == 5)
                     brw_MUL(p, c->pv, c->pv, brw_imm_d(jmpi*(nr+1)));
   brw_JMPI(p, c->pv, BRW_PREDICATE_NONE);
            brw_JMPI(p, brw_imm_ud(jmpi*nr), BRW_PREDICATE_NONE);
      }
         /***********************************************************************
   * Triangle setup.
   */
         static void alloc_regs( struct brw_sf_compile *c )
   {
               /* Values computed by fixed function unit:
   */
   c->pv  = retype(brw_vec1_grf(1, 1), BRW_REGISTER_TYPE_D);
   c->det = brw_vec1_grf(1, 2);
   c->dx0 = brw_vec1_grf(1, 3);
   c->dx2 = brw_vec1_grf(1, 4);
   c->dy0 = brw_vec1_grf(1, 5);
            /* z and 1/w passed in separately:
   */
   c->z[0]     = brw_vec1_grf(2, 0);
   c->inv_w[0] = brw_vec1_grf(2, 1);
   c->z[1]     = brw_vec1_grf(2, 2);
   c->inv_w[1] = brw_vec1_grf(2, 3);
   c->z[2]     = brw_vec1_grf(2, 4);
            /* The vertices:
   */
   reg = 3;
   for (i = 0; i < c->nr_verts; i++) {
      c->vert[i] = brw_vec8_grf(reg, 0);
               /* Temporaries, allocated after last vertex reg.
   */
   c->inv_det = brw_vec1_grf(reg, 0);  reg++;
   c->a1_sub_a0 = brw_vec8_grf(reg, 0);  reg++;
   c->a2_sub_a0 = brw_vec8_grf(reg, 0);  reg++;
            /* Note grf allocation:
   */
               /* Outputs of this program - interpolation coefficients for
   * rasterization:
   */
   c->m1Cx = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 1, 0);
   c->m2Cy = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 2, 0);
      }
         static void copy_z_inv_w( struct brw_sf_compile *c )
   {
      struct brw_codegen *p = &c->func;
            /* Copy both scalars with a single MOV:
   */
   for (i = 0; i < c->nr_verts; i++)
      }
         static void invert_det( struct brw_sf_compile *c)
   {
      /* Looks like we invert all 8 elements just to get 1/det in
   * position 2 !?!
   */
   gfx4_math(&c->func,
      c->inv_det,
   BRW_MATH_FUNCTION_INV,
   0,
   c->det,
      }
         static bool
   calculate_masks(struct brw_sf_compile *c,
                  GLuint reg,
      {
      bool is_last_attr = (reg == c->nr_setup_regs - 1);
            *pc_persp = 0;
   *pc_linear = 0;
            interp = c->key.interp_mode[vert_reg_to_vue_slot(c, reg, 0)];
   if (interp == INTERP_MODE_SMOOTH) {
      *pc_linear = 0xf;
      } else if (interp == INTERP_MODE_NOPERSPECTIVE)
            /* Maybe only process one attribute on the final round:
   */
   if (vert_reg_to_varying(c, reg, 1) != BRW_VARYING_SLOT_COUNT) {
               interp = c->key.interp_mode[vert_reg_to_vue_slot(c, reg, 1)];
   if (interp == INTERP_MODE_SMOOTH) {
      *pc_linear |= 0xf0;
      } else if (interp == INTERP_MODE_NOPERSPECTIVE)
                  }
      /* Calculates the predicate control for which channels of a reg
   * (containing 2 attrs) to do point sprite coordinate replacement on.
   */
   static uint16_t
   calculate_point_sprite_mask(struct brw_sf_compile *c, GLuint reg)
   {
      int varying1, varying2;
            varying1 = vert_reg_to_varying(c, reg, 0);
   if (varying1 >= VARYING_SLOT_TEX0 && varying1 <= VARYING_SLOT_TEX7) {
      pc |= 0x0f;
      }
   if (varying1 == BRW_VARYING_SLOT_PNTC)
            varying2 = vert_reg_to_varying(c, reg, 1);
   if (varying2 >= VARYING_SLOT_TEX0 && varying2 <= VARYING_SLOT_TEX7) {
      if (c->key.point_sprite_coord_replace & (1 << (varying2 -
            }
   if (varying2 == BRW_VARYING_SLOT_PNTC)
               }
      static void
   set_predicate_control_flag_value(struct brw_codegen *p,
               {
               if (value != 0xff) {
      if (value != c->flag_value) {
      brw_MOV(p, brw_flag_reg(0, 0), brw_imm_uw(value));
                     }
      static void brw_emit_tri_setup(struct brw_sf_compile *c, bool allocate)
   {
      struct brw_codegen *p = &c->func;
            c->flag_value = 0xff;
            if (allocate)
            invert_det(c);
            if (c->key.do_twoside_color)
            if (c->key.contains_flat_varying)
               for (i = 0; i < c->nr_setup_regs; i++)
   {
      /* Pair of incoming attributes:
   */
   struct brw_reg a0 = offset(c->vert[0], i);
   struct brw_reg a1 = offset(c->vert[1], i);
   struct brw_reg a2 = offset(c->vert[2], i);
   GLushort pc, pc_persp, pc_linear;
            if (pc_persp)
   set_predicate_control_flag_value(p, c, pc_persp);
   brw_MUL(p, a0, a0, c->inv_w[0]);
   brw_MUL(p, a1, a1, c->inv_w[1]);
   brw_MUL(p, a2, a2, c->inv_w[2]);
                     /* Calculate coefficients for interpolated values:
   */
   if (pc_linear)
   set_predicate_control_flag_value(p, c, pc_linear);
      brw_ADD(p, c->a1_sub_a0, a1, negate(a0));
   brw_ADD(p, c->a2_sub_a0, a2, negate(a0));
      /* calculate dA/dx
         brw_MUL(p, brw_null_reg(), c->a1_sub_a0, c->dy2);
   brw_MAC(p, c->tmp, c->a2_sub_a0, negate(c->dy0));
   brw_MUL(p, c->m1Cx, c->tmp, c->inv_det);
      /* calculate dA/dy
         brw_MUL(p, brw_null_reg(), c->a2_sub_a0, c->dx0);
   brw_MAC(p, c->tmp, c->a1_sub_a0, negate(c->dx2));
   brw_MUL(p, c->m2Cy, c->tmp, c->inv_det);
                  set_predicate_control_flag_value(p, c, pc);
   /* start point for interpolation
         brw_MOV(p, c->m3C0, a0);
      /* Copy m0..m3 to URB.  m0 is implicitly copied from r0 in
      * the send instruction:
      brw_urb_WRITE(p,
            brw_null_reg(),
   0,
   brw_vec8_grf(0, 0), /* r0, will be copied to m0 */
               4, 	/* msg len */
   0,	/* response len */
   i*4,	/* offset */
                     }
            static void brw_emit_line_setup(struct brw_sf_compile *c, bool allocate)
   {
      struct brw_codegen *p = &c->func;
            c->flag_value = 0xff;
            if (allocate)
            invert_det(c);
            if (c->key.contains_flat_varying)
            for (i = 0; i < c->nr_setup_regs; i++)
   {
      /* Pair of incoming attributes:
   */
   struct brw_reg a0 = offset(c->vert[0], i);
   struct brw_reg a1 = offset(c->vert[1], i);
   GLushort pc, pc_persp, pc_linear;
            if (pc_persp)
   set_predicate_control_flag_value(p, c, pc_persp);
   brw_MUL(p, a0, a0, c->inv_w[0]);
   brw_MUL(p, a1, a1, c->inv_w[1]);
                  /* Calculate coefficients for position, color:
   */
   set_predicate_control_flag_value(p, c, pc_linear);
      brw_ADD(p, c->a1_sub_a0, a1, negate(a0));
      brw_MUL(p, c->tmp, c->a1_sub_a0, c->dx0);
   brw_MUL(p, c->m1Cx, c->tmp, c->inv_det);
      brw_MUL(p, c->tmp, c->a1_sub_a0, c->dy0);
   brw_MUL(p, c->m2Cy, c->tmp, c->inv_det);
                  set_predicate_control_flag_value(p, c, pc);
      /* start point for interpolation
         brw_MOV(p, c->m3C0, a0);
      /* Copy m0..m3 to URB.
         brw_urb_WRITE(p,
            brw_null_reg(),
   0,
   brw_vec8_grf(0, 0),
               4, 	/* msg len */
   0,	/* response len */
   i*4,	/* urb destination offset */
                     }
      static void brw_emit_point_sprite_setup(struct brw_sf_compile *c, bool allocate)
   {
      struct brw_codegen *p = &c->func;
            c->flag_value = 0xff;
            if (allocate)
            copy_z_inv_w(c);
   for (i = 0; i < c->nr_setup_regs; i++)
   {
      struct brw_reg a0 = offset(c->vert[0], i);
   GLushort pc, pc_persp, pc_linear, pc_coord_replace;
            pc_coord_replace = calculate_point_sprite_mask(c, i);
            set_predicate_control_flag_value(p, c, pc_persp);
   brw_MUL(p, a0, a0, c->inv_w[0]);
                  /* Point sprite coordinate replacement: A texcoord with this
   * enabled gets replaced with the value (x, y, 0, 1) where x and
   * y vary from 0 to 1 across the horizontal and vertical of the
   * point.
   */
   set_predicate_control_flag_value(p, c, pc_coord_replace);
   /* Calculate 1.0/PointWidth */
   gfx4_math(&c->func,
      c->tmp,
   BRW_MATH_FUNCTION_INV,
   0,
   c->dx0,
         brw_set_default_access_mode(p, BRW_ALIGN_16);
      /* dA/dx, dA/dy */
   brw_MOV(p, c->m1Cx, brw_imm_f(0.0));
   brw_MOV(p, c->m2Cy, brw_imm_f(0.0));
   brw_MOV(p, brw_writemask(c->m1Cx, WRITEMASK_X), c->tmp);
   if (c->key.sprite_origin_lower_left) {
         } else {
         }
      /* attribute constant offset */
   brw_MOV(p, c->m3C0, brw_imm_f(0.0));
   if (c->key.sprite_origin_lower_left) {
         } else {
         }
      brw_set_default_access_mode(p, BRW_ALIGN_1);
                  set_predicate_control_flag_value(p, c, pc & ~pc_coord_replace);
   brw_MOV(p, c->m1Cx, brw_imm_ud(0));
   brw_MOV(p, c->m2Cy, brw_imm_ud(0));
   brw_MOV(p, c->m3C0, a0); /* constant value */
                     set_predicate_control_flag_value(p, c, pc);
   /* Copy m0..m3 to URB. */
   brw_urb_WRITE(p,
   brw_null_reg(),
   0,
   brw_vec8_grf(0, 0),
               4, 	/* msg len */
   0,	/* response len */
   i*4,	/* urb destination offset */
                  }
      /* Points setup - several simplifications as all attributes are
   * constant across the face of the point (point sprites excluded!)
   */
   static void brw_emit_point_setup(struct brw_sf_compile *c, bool allocate)
   {
      struct brw_codegen *p = &c->func;
            c->flag_value = 0xff;
            if (allocate)
                     brw_MOV(p, c->m1Cx, brw_imm_ud(0)); /* zero - move out of loop */
            for (i = 0; i < c->nr_setup_regs; i++)
   {
      struct brw_reg a0 = offset(c->vert[0], i);
   GLushort pc, pc_persp, pc_linear;
            if (pc_persp)
   /* This seems odd as the values are all constant, but the
      * fragment shader will be expecting it:
      set_predicate_control_flag_value(p, c, pc_persp);
   brw_MUL(p, a0, a0, c->inv_w[0]);
                     /* The delta values are always zero, just send the starting
   * coordinate.  Again, this is to fit in with the interpolation
   * code in the fragment shader.
   */
   set_predicate_control_flag_value(p, c, pc);
      brw_MOV(p, c->m3C0, a0); /* constant value */
      /* Copy m0..m3 to URB.
         brw_urb_WRITE(p,
            brw_null_reg(),
   0,
   brw_vec8_grf(0, 0),
               4, 	/* msg len */
   0,	/* response len */
   i*4,	/* urb destination offset */
                     }
      static void brw_emit_anyprim_setup( struct brw_sf_compile *c )
   {
      struct brw_codegen *p = &c->func;
   struct brw_reg payload_prim = brw_uw1_reg(BRW_GENERAL_REGISTER_FILE, 1, 0);
   struct brw_reg payload_attr = get_element_ud(brw_vec1_reg(BRW_GENERAL_REGISTER_FILE, 1, 0), 0);
   struct brw_reg primmask;
   int jmp;
            c->nr_verts = 3;
                     brw_MOV(p, primmask, brw_imm_ud(1));
            brw_AND(p, v1_null_ud, primmask, brw_imm_ud((1<<_3DPRIM_TRILIST) |
            (1<<_3DPRIM_TRISTRIP) |
   (1<<_3DPRIM_TRIFAN) |
   (1<<_3DPRIM_TRISTRIP_REVERSE) |
   (1<<_3DPRIM_POLYGON) |
      brw_inst_set_cond_modifier(p->devinfo, brw_last_inst, BRW_CONDITIONAL_Z);
   jmp = brw_JMPI(p, brw_imm_d(0), BRW_PREDICATE_NORMAL) - p->store;
   brw_emit_tri_setup(c, false);
            brw_AND(p, v1_null_ud, primmask, brw_imm_ud((1<<_3DPRIM_LINELIST) |
            (1<<_3DPRIM_LINESTRIP) |
   (1<<_3DPRIM_LINELOOP) |
   (1<<_3DPRIM_LINESTRIP_CONT) |
      brw_inst_set_cond_modifier(p->devinfo, brw_last_inst, BRW_CONDITIONAL_Z);
   jmp = brw_JMPI(p, brw_imm_d(0), BRW_PREDICATE_NORMAL) - p->store;
   brw_emit_line_setup(c, false);
            brw_AND(p, v1_null_ud, payload_attr, brw_imm_ud(1<<BRW_SPRITE_POINT_ENABLE));
   brw_inst_set_cond_modifier(p->devinfo, brw_last_inst, BRW_CONDITIONAL_Z);
   jmp = brw_JMPI(p, brw_imm_d(0), BRW_PREDICATE_NORMAL) - p->store;
   brw_emit_point_sprite_setup(c, false);
               }
      const unsigned *
   brw_compile_sf(const struct brw_compiler *compiler,
                  void *mem_ctx,
   const struct brw_sf_prog_key *key,
      {
      struct brw_sf_compile c;
            /* Begin the compilation:
   */
            c.key = *key;
   c.vue_map = *vue_map;
   if (c.key.do_point_coord) {
      /*
   * gl_PointCoord is a FS instead of VS builtin variable, thus it's
   * not included in c.vue_map generated in VS stage. Here we add
   * it manually to let SF shader generate the needed interpolation
   * coefficient for FS shader.
   */
   c.vue_map.varying_to_slot[BRW_VARYING_SLOT_PNTC] = c.vue_map.num_slots;
      }
   c.urb_entry_read_offset = BRW_SF_URB_ENTRY_READ_OFFSET;
   c.nr_attr_regs = (c.vue_map.num_slots + 1)/2 - c.urb_entry_read_offset;
            c.prog_data.urb_read_length = c.nr_attr_regs;
            /* Which primitive?  Or all three?
   */
   switch (key->primitive) {
   case BRW_SF_PRIM_TRIANGLES:
      c.nr_verts = 3;
   brw_emit_tri_setup( &c, true );
      case BRW_SF_PRIM_LINES:
      c.nr_verts = 2;
   brw_emit_line_setup( &c, true );
      case BRW_SF_PRIM_POINTS:
      c.nr_verts = 1;
      brw_emit_point_sprite_setup( &c, true );
         brw_emit_point_setup( &c, true );
         case BRW_SF_PRIM_UNFILLED_TRIS:
      c.nr_verts = 3;
   brw_emit_anyprim_setup( &c );
      default:
                  /* FINISHME: SF programs use calculated jumps (i.e., JMPI with a register
   * source). Compacting would be difficult.
   */
                              if (INTEL_DEBUG(DEBUG_SF)) {
      fprintf(stderr, "sf:\n");
   brw_disassemble_with_labels(&compiler->isa,
                        }
