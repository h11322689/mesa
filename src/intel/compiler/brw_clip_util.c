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
         struct brw_reg get_tmp( struct brw_clip_compile *c )
   {
               if (++c->last_tmp > c->prog_data.total_grf)
               }
      static void release_tmp( struct brw_clip_compile *c, struct brw_reg tmp )
   {
      if (tmp.nr == c->last_tmp-1)
      }
         static struct brw_reg make_plane_ud(GLuint x, GLuint y, GLuint z, GLuint w)
   {
         }
         void brw_clip_init_planes( struct brw_clip_compile *c )
   {
               if (!c->key.nr_userclip) {
      brw_MOV(p, get_element_ud(c->reg.fixed_planes, 0), make_plane_ud( 0,    0, 0xff, 1));
   brw_MOV(p, get_element_ud(c->reg.fixed_planes, 1), make_plane_ud( 0,    0,    1, 1));
   brw_MOV(p, get_element_ud(c->reg.fixed_planes, 2), make_plane_ud( 0, 0xff,    0, 1));
   brw_MOV(p, get_element_ud(c->reg.fixed_planes, 3), make_plane_ud( 0,    1,    0, 1));
   brw_MOV(p, get_element_ud(c->reg.fixed_planes, 4), make_plane_ud(0xff,  0,    0, 1));
         }
            #define W 3
      /* Project 'pos' to screen space (or back again), overwrite with results:
   */
   void brw_clip_project_position(struct brw_clip_compile *c, struct brw_reg pos )
   {
               /* calc rhw
   */
            /* value.xyz *= value.rhw
   */
   brw_set_default_access_mode(p, BRW_ALIGN_16);
   brw_MUL(p, brw_writemask(pos, WRITEMASK_XYZ), pos,
            }
         static void brw_clip_project_vertex( struct brw_clip_compile *c,
         {
      struct brw_codegen *p = &c->func;
   struct brw_reg tmp = get_tmp(c);
   GLuint hpos_offset = brw_varying_to_offset(&c->vue_map, VARYING_SLOT_POS);
   GLuint ndc_offset = brw_varying_to_offset(&c->vue_map,
            /* Fixup position.  Extract from the original vertex and re-project
   * to screen space:
   */
   brw_MOV(p, tmp, deref_4f(vert_addr, hpos_offset));
   brw_clip_project_position(c, tmp);
               }
               /* Interpolate between two vertices and put the result into a0.0.
   * Increment a0.0 accordingly.
   *
   * Beware that dest_ptr can be equal to v0_ptr!
   */
   void brw_clip_interp_vertex( struct brw_clip_compile *c,
         struct brw_indirect dest_ptr,
   struct brw_indirect v0_ptr, /* from */
   struct brw_indirect v1_ptr, /* to */
   struct brw_reg t0,
   {
      struct brw_codegen *p = &c->func;
   struct brw_reg t_nopersp, v0_ndc_copy;
            /* Just copy the vertex header:
   */
   /*
   * After CLIP stage, only first 256 bits of the VUE are read
   * back on Ironlake, so needn't change it
   */
               /* First handle the 3D and NDC interpolation, in case we
   * need noperspective interpolation. Doing it early has no
   * performance impact in any case.
            /* Take a copy of the v0 NDC coordinates, in case dest == v0. */
   if (c->key.contains_noperspective_varying) {
      GLuint offset = brw_varying_to_offset(&c->vue_map,
         v0_ndc_copy = get_tmp(c);
               /* Compute the new 3D position
   *
   * dest_hpos = v0_hpos * (1 - t0) + v1_hpos * t0
   */
   {
      GLuint delta = brw_varying_to_offset(&c->vue_map, VARYING_SLOT_POS);
   struct brw_reg tmp = get_tmp(c);
   brw_MUL(p, vec4(brw_null_reg()), deref_4f(v1_ptr, delta), t0);
   brw_MAC(p, tmp, negate(deref_4f(v0_ptr, delta)), t0);
   brw_ADD(p, deref_4f(dest_ptr, delta), deref_4f(v0_ptr, delta), tmp);
               /* Recreate the projected (NDC) coordinate in the new vertex header */
            /* If we have noperspective attributes,
   * we need to compute the screen-space t
   */
   if (c->key.contains_noperspective_varying) {
      GLuint delta = brw_varying_to_offset(&c->vue_map,
         struct brw_reg tmp = get_tmp(c);
            /* t_nopersp = vec4(v1.xy, dest.xy) */
   brw_MOV(p, t_nopersp, deref_4f(v1_ptr, delta));
   brw_MOV(p, tmp, deref_4f(dest_ptr, delta));
   brw_set_default_access_mode(p, BRW_ALIGN_16);
   brw_MOV(p,
                  /* t_nopersp = vec4(v1.xy, dest.xy) - v0.xyxy */
   brw_ADD(p, t_nopersp, t_nopersp,
            /* Add the absolute values of the X and Y deltas so that if
   * the points aren't in the same place on the screen we get
   * nonzero values to divide.
   *
   * After that, we have vert1 - vert0 in t_nopersp.x and
   * vertnew - vert0 in t_nopersp.y
   *
   * t_nopersp = vec2(|v1.x  -v0.x| + |v1.y  -v0.y|,
   *                  |dest.x-v0.x| + |dest.y-v0.y|)
   */
   brw_ADD(p,
         brw_writemask(t_nopersp, WRITEMASK_XY),
   brw_abs(brw_swizzle(t_nopersp, BRW_SWIZZLE_XZXZ)),
            /* If the points are in the same place, just substitute a
   * value to avoid divide-by-zero
   */
   brw_CMP(p, vec1(brw_null_reg()), BRW_CONDITIONAL_EQ,
         vec1(t_nopersp),
   brw_IF(p, BRW_EXECUTE_1);
   brw_MOV(p, t_nopersp, brw_imm_vf4(brw_float_to_vf(1.0),
                              /* Now compute t_nopersp = t_nopersp.y/t_nopersp.x and broadcast it. */
   brw_math_invert(p, get_element(t_nopersp, 0), get_element(t_nopersp, 0));
   brw_MUL(p, vec1(t_nopersp), vec1(t_nopersp),
         brw_set_default_access_mode(p, BRW_ALIGN_16);
   brw_MOV(p, t_nopersp, brw_swizzle(t_nopersp, BRW_SWIZZLE_XXXX));
            release_tmp(c, tmp);
               /* Now we can iterate over each attribute
   * (could be done in pairs?)
   */
   for (slot = 0; slot < c->vue_map.num_slots; slot++) {
      int varying = c->vue_map.slot_to_varying[slot];
            /* HPOS, NDC already handled above */
   if (varying == VARYING_SLOT_POS || varying == BRW_VARYING_SLOT_NDC)
               if (force_edgeflag)
         else
      brw_MOV(p, deref_4f(dest_ptr, delta), deref_4f(v0_ptr, delta));
      } else if (varying == VARYING_SLOT_PSIZ) {
      /* PSIZ doesn't need interpolation because it isn't used by the
   * fragment shader.
      /* This is a true vertex result (and not a special value for the VUE
      * header), so interpolate:
   *
   *        New = attr0 + t*attr1 - t*attr0
         *
   * Unless the attribute is flat shaded -- in which case just copy
   */
                  if (interp != INTERP_MODE_FLAT) {
      struct brw_reg tmp = get_tmp(c);
                  brw_MUL(p,
                        brw_MAC(p,
                        brw_ADD(p,
                           }
   else {
      brw_MOV(p,
                           if (c->vue_map.num_slots % 2) {
                           if (c->key.contains_noperspective_varying)
      }
      void brw_clip_emit_vue(struct brw_clip_compile *c,
            struct brw_indirect vert,
      {
      struct brw_codegen *p = &c->func;
                     /* Any URB entry that is allocated must subsequently be used or discarded,
   * so it doesn't make sense to mark EOT and ALLOCATE at the same time.
   */
            /* Copy the vertex from vertn into m1..mN+1:
   */
            /* Overwrite PrimType and PrimStart in the message header, for
   * each vertex in turn:
   */
               /* Send each vertex as a separate write to the urb.  This
   * is different to the concept in brw_sf_emit.c, where
   * subsequent writes are used to build up a single urb
   * entry.  Each of these writes instantiates a separate
   * urb entry - (I think... what about 'allocate'?)
   */
   brw_urb_WRITE(p,
   allocate ? c->reg.R0 : retype(brw_null_reg(), BRW_REGISTER_TYPE_UD),
   0,
   c->reg.R0,
         c->nr_regs + 1, /* msg length */
   allocate ? 1 : 0, /* response_length */
   0,		/* urb offset */
      }
            void brw_clip_kill_thread(struct brw_clip_compile *c)
   {
               brw_clip_ff_sync(c);
   /* Send an empty message to kill the thread and release any
   * allocated urb entry:
   */
   brw_urb_WRITE(p,
   retype(brw_null_reg(), BRW_REGISTER_TYPE_UD),
   0,
   c->reg.R0,
         1, 		/* msg len */
   0, 		/* response len */
   0,
      }
               struct brw_reg brw_clip_plane0_address( struct brw_clip_compile *c )
   {
         }
         struct brw_reg brw_clip_plane_stride( struct brw_clip_compile *c )
   {
      if (c->key.nr_userclip) {
         }
   else {
            }
         /* Distribute flatshaded attributes from provoking vertex prior to
   * clipping.
   */
   void brw_clip_copy_flatshaded_attributes( struct brw_clip_compile *c,
         {
               for (int i = 0; i < c->vue_map.num_slots; i++) {
      if (c->key.interp_mode[i] == INTERP_MODE_FLAT) {
      brw_MOV(p,
                  }
            void brw_clip_init_clipmask( struct brw_clip_compile *c )
   {
      struct brw_codegen *p = &c->func;
            /* Shift so that lowest outcode bit is rightmost:
   */
            if (c->key.nr_userclip) {
               /* Rearrange userclip outcodes so that they come directly after
   * the fixed plane bits.
   */
   if (p->devinfo->ver == 5 || p->devinfo->verx10 == 45)
         else
            brw_SHR(p, tmp, tmp, brw_imm_ud(8));
                  }
      void brw_clip_ff_sync(struct brw_clip_compile *c)
   {
               if (p->devinfo->ver == 5) {
      brw_AND(p, brw_null_reg(), c->reg.ff_sync, brw_imm_ud(0x1));
   brw_inst_set_cond_modifier(p->devinfo, brw_last_inst, BRW_CONDITIONAL_Z);
   brw_IF(p, BRW_EXECUTE_1);
   {
            c->reg.R0,
   0,
   c->reg.R0,
   1, /* allocate */
   1, /* response length */
   0 /* eot */);
      }
   brw_ENDIF(p);
         }
      void brw_clip_init_ff_sync(struct brw_clip_compile *c)
   {
               if (p->devinfo->ver == 5) {
            }
