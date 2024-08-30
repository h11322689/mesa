   /*
   * Copyright (C) 2016 Rob Clark <robclark@freedesktop.org>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "pipe/p_state.h"
   #include "util/bitset.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "freedreno_program.h"
      #include "fd5_emit.h"
   #include "fd5_format.h"
   #include "fd5_program.h"
   #include "fd5_texture.h"
      #include "ir3_cache.h"
      void
   fd5_emit_shader(struct fd_ringbuffer *ring, const struct ir3_shader_variant *so)
   {
      const struct ir3_info *si = &so->info;
   enum a4xx_state_block sb = fd4_stage2shadersb(so->type);
   enum a4xx_state_src src;
            if (FD_DBG(DIRECT)) {
      sz = si->sizedwords;
   src = SS4_DIRECT;
      } else {
      sz = 0;
   src = SS4_INDIRECT;
               OUT_PKT7(ring, CP_LOAD_STATE4, 3 + sz);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
                     if (bin) {
      OUT_RING(ring, CP_LOAD_STATE4_1_EXT_SRC_ADDR(0) |
            } else {
                  /* for how clever coverity is, it is sometimes rather dull, and
   * doesn't realize that the only case where bin==NULL, sz==0:
   */
            for (i = 0; i < sz; i++) {
            }
      void
   fd5_emit_shader_obj(struct fd_context *ctx, struct fd_ringbuffer *ring,
               {
               OUT_PKT4(ring, shader_obj_reg, 6);
            uint32_t per_sp_size = ctx->pvtmem[so->pvtmem_per_wave].per_sp_size;
   OUT_RING(ring, A5XX_SP_VS_PVT_MEM_PARAM_MEMSIZEPERITEM(
               if (so->pvtmem_size > 0) { /* SP_xS_PVT_MEM_ADDR */
      OUT_RELOC(ring, ctx->pvtmem[so->pvtmem_per_wave].bo, 0, 0, 0);
      } else {
      OUT_RING(ring, 0);
      }
      }
      /* TODO maybe some of this we could pre-compute once rather than having
   * so much draw-time logic?
   */
   static void
   emit_stream_out(struct fd_ringbuffer *ring, const struct ir3_shader_variant *v,
         {
      const struct ir3_stream_output_info *strmout = &v->stream_output;
   unsigned ncomp[PIPE_MAX_SO_BUFFERS] = {0};
                     for (unsigned i = 0; i < strmout->num_outputs; i++) {
      const struct ir3_stream_output *out = &strmout->output[i];
   unsigned k = out->register_index;
                     /* linkage map sorted by order frag shader wants things, so
   * a bit less ideal here..
   */
   for (idx = 0; idx < l->cnt; idx++)
                           for (unsigned j = 0; j < out->num_components; j++) {
      unsigned c = j + out->start_component;
                  if (loc & 1) {
      prog[loc / 2] |= A5XX_VPC_SO_PROG_B_EN |
            } else {
      prog[loc / 2] |= A5XX_VPC_SO_PROG_A_EN |
                           OUT_PKT7(ring, CP_CONTEXT_REG_BUNCH, 12 + (2 * ARRAY_SIZE(prog)));
   OUT_RING(ring, REG_A5XX_VPC_SO_BUF_CNTL);
   OUT_RING(ring, A5XX_VPC_SO_BUF_CNTL_ENABLE |
                     COND(ncomp[0] > 0, A5XX_VPC_SO_BUF_CNTL_BUF0) |
   OUT_RING(ring, REG_A5XX_VPC_SO_NCOMP(0));
   OUT_RING(ring, ncomp[0]);
   OUT_RING(ring, REG_A5XX_VPC_SO_NCOMP(1));
   OUT_RING(ring, ncomp[1]);
   OUT_RING(ring, REG_A5XX_VPC_SO_NCOMP(2));
   OUT_RING(ring, ncomp[2]);
   OUT_RING(ring, REG_A5XX_VPC_SO_NCOMP(3));
   OUT_RING(ring, ncomp[3]);
   OUT_RING(ring, REG_A5XX_VPC_SO_CNTL);
   OUT_RING(ring, A5XX_VPC_SO_CNTL_ENABLE);
   for (unsigned i = 0; i < ARRAY_SIZE(prog); i++) {
      OUT_RING(ring, REG_A5XX_VPC_SO_PROG);
         }
      struct stage {
      const struct ir3_shader_variant *v;
   const struct ir3_info *i;
   /* const sizes are in units of 4 * vec4 */
   uint8_t constoff;
   uint8_t constlen;
   /* instr sizes are in units of 16 instructions */
   uint8_t instroff;
      };
      enum { VS = 0, FS = 1, HS = 2, DS = 3, GS = 4, MAX_STAGES };
      static void
   setup_stages(struct fd5_emit *emit, struct stage *s)
   {
               s[VS].v = fd5_emit_get_vp(emit);
                     for (i = 0; i < MAX_STAGES; i++) {
      if (s[i].v) {
      s[i].i = &s[i].v->info;
   /* constlen is in units of 4 * vec4: */
   assert(s[i].v->constlen % 4 == 0);
   s[i].constlen = s[i].v->constlen / 4;
   /* instrlen is already in units of 16 instr.. although
   * probably we should ditch that and not make the compiler
   * care about instruction group size of a3xx vs a5xx
   */
      } else {
      s[i].i = NULL;
   s[i].constlen = 0;
                  /* NOTE: at least for gles2, blob partitions VS at bottom of const
   * space and FS taking entire remaining space.  We probably don't
   * need to do that the same way, but for now mimic what the blob
   * does to make it easier to diff against register values from blob
   *
   * NOTE: if VS.instrlen + FS.instrlen > 64, then one or both shaders
   * is run from external memory.
   */
   if ((s[VS].instrlen + s[FS].instrlen) > 64) {
      /* prioritize FS for internal memory: */
   if (s[FS].instrlen < 64) {
      /* if FS can fit, kick VS out to external memory: */
      } else if (s[VS].instrlen < 64) {
      /* otherwise if VS can fit, kick out FS: */
      } else {
      /* neither can fit, run both from external memory: */
   s[VS].instrlen = 0;
                  unsigned constoff = 0;
   for (i = 0; i < MAX_STAGES; i++) {
      s[i].constoff = constoff;
               s[VS].instroff = 0;
   s[FS].instroff = 64 - s[FS].instrlen;
      }
      static inline uint32_t
   next_regid(uint32_t reg, uint32_t increment)
   {
      if (VALIDREG(reg))
         else
      }
   void
   fd5_program_emit(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
      struct stage s[MAX_STAGES];
   uint32_t pos_regid, psize_regid, color_regid[8];
   uint32_t face_regid, coord_regid, zwcoord_regid, samp_id_regid,
         uint32_t ij_regid[IJ_COUNT], vertex_regid, instance_regid, clip0_regid,
         enum a3xx_threadsize fssz;
   uint8_t psize_loc = ~0;
                     bool do_streamout = (s[VS].v->stream_output.num_outputs > 0);
   uint8_t clip_mask = s[VS].v->clip_mask,
                  /* Unlike a6xx, we don't factor the rasterizer's clip enables in here.  It's
   * already handled by the frontend by storing 0.0 to the clipdist in the
   * shader variant (using either nir_lower_clip_disable for clip distances
   * from the source shader, or nir_lower_clip_vs for user clip planes).
   * Masking the disabled clipdists off causes GPU hangs in tests like
   * spec@glsl-1.20@execution@clipping@vs-clip-vertex-enables.
                     pos_regid = ir3_find_output_regid(s[VS].v, VARYING_SLOT_POS);
   psize_regid = ir3_find_output_regid(s[VS].v, VARYING_SLOT_PSIZ);
   clip0_regid = ir3_find_output_regid(s[VS].v, VARYING_SLOT_CLIP_DIST0);
   clip1_regid = ir3_find_output_regid(s[VS].v, VARYING_SLOT_CLIP_DIST1);
   vertex_regid =
                  if (s[FS].v->color0_mrt) {
      color_regid[0] = color_regid[1] = color_regid[2] = color_regid[3] =
      color_regid[4] = color_regid[5] = color_regid[6] = color_regid[7] =
   } else {
      color_regid[0] = ir3_find_output_regid(s[FS].v, FRAG_RESULT_DATA0);
   color_regid[1] = ir3_find_output_regid(s[FS].v, FRAG_RESULT_DATA1);
   color_regid[2] = ir3_find_output_regid(s[FS].v, FRAG_RESULT_DATA2);
   color_regid[3] = ir3_find_output_regid(s[FS].v, FRAG_RESULT_DATA3);
   color_regid[4] = ir3_find_output_regid(s[FS].v, FRAG_RESULT_DATA4);
   color_regid[5] = ir3_find_output_regid(s[FS].v, FRAG_RESULT_DATA5);
   color_regid[6] = ir3_find_output_regid(s[FS].v, FRAG_RESULT_DATA6);
               samp_id_regid = ir3_find_sysval_regid(s[FS].v, SYSTEM_VALUE_SAMPLE_ID);
   samp_mask_regid =
         face_regid = ir3_find_sysval_regid(s[FS].v, SYSTEM_VALUE_FRONT_FACE);
   coord_regid = ir3_find_sysval_regid(s[FS].v, SYSTEM_VALUE_FRAG_COORD);
   zwcoord_regid = next_regid(coord_regid, 2);
   for (unsigned i = 0; i < ARRAY_SIZE(ij_regid); i++)
      ij_regid[i] = ir3_find_sysval_regid(
         /* we could probably divide this up into things that need to be
   * emitted if frag-prog is dirty vs if vert-prog is dirty..
            OUT_PKT4(ring, REG_A5XX_HLSQ_VS_CONFIG, 5);
   OUT_RING(ring, A5XX_HLSQ_VS_CONFIG_CONSTOBJECTOFFSET(s[VS].constoff) |
               OUT_RING(ring, A5XX_HLSQ_FS_CONFIG_CONSTOBJECTOFFSET(s[FS].constoff) |
               OUT_RING(ring, A5XX_HLSQ_HS_CONFIG_CONSTOBJECTOFFSET(s[HS].constoff) |
               OUT_RING(ring, A5XX_HLSQ_DS_CONFIG_CONSTOBJECTOFFSET(s[DS].constoff) |
               OUT_RING(ring, A5XX_HLSQ_GS_CONFIG_CONSTOBJECTOFFSET(s[GS].constoff) |
                  OUT_PKT4(ring, REG_A5XX_HLSQ_CS_CONFIG, 1);
            OUT_PKT4(ring, REG_A5XX_HLSQ_VS_CNTL, 5);
   OUT_RING(ring, A5XX_HLSQ_VS_CNTL_INSTRLEN(s[VS].instrlen) |
               OUT_RING(ring, A5XX_HLSQ_FS_CNTL_INSTRLEN(s[FS].instrlen) |
               OUT_RING(ring, A5XX_HLSQ_HS_CNTL_INSTRLEN(s[HS].instrlen) |
               OUT_RING(ring, A5XX_HLSQ_DS_CNTL_INSTRLEN(s[DS].instrlen) |
               OUT_RING(ring, A5XX_HLSQ_GS_CNTL_INSTRLEN(s[GS].instrlen) |
                  OUT_PKT4(ring, REG_A5XX_SP_VS_CONFIG, 5);
   OUT_RING(ring, A5XX_SP_VS_CONFIG_CONSTOBJECTOFFSET(s[VS].constoff) |
               OUT_RING(ring, A5XX_SP_FS_CONFIG_CONSTOBJECTOFFSET(s[FS].constoff) |
               OUT_RING(ring, A5XX_SP_HS_CONFIG_CONSTOBJECTOFFSET(s[HS].constoff) |
               OUT_RING(ring, A5XX_SP_DS_CONFIG_CONSTOBJECTOFFSET(s[DS].constoff) |
               OUT_RING(ring, A5XX_SP_GS_CONFIG_CONSTOBJECTOFFSET(s[GS].constoff) |
                  OUT_PKT4(ring, REG_A5XX_SP_CS_CONFIG, 1);
            OUT_PKT4(ring, REG_A5XX_HLSQ_VS_CONSTLEN, 2);
   OUT_RING(ring, s[VS].constlen); /* HLSQ_VS_CONSTLEN */
            OUT_PKT4(ring, REG_A5XX_HLSQ_FS_CONSTLEN, 2);
   OUT_RING(ring, s[FS].constlen); /* HLSQ_FS_CONSTLEN */
            OUT_PKT4(ring, REG_A5XX_HLSQ_HS_CONSTLEN, 2);
   OUT_RING(ring, s[HS].constlen); /* HLSQ_HS_CONSTLEN */
            OUT_PKT4(ring, REG_A5XX_HLSQ_DS_CONSTLEN, 2);
   OUT_RING(ring, s[DS].constlen); /* HLSQ_DS_CONSTLEN */
            OUT_PKT4(ring, REG_A5XX_HLSQ_GS_CONSTLEN, 2);
   OUT_RING(ring, s[GS].constlen); /* HLSQ_GS_CONSTLEN */
            OUT_PKT4(ring, REG_A5XX_HLSQ_CS_CONSTLEN, 2);
   OUT_RING(ring, 0x00000000); /* HLSQ_CS_CONSTLEN */
            OUT_PKT4(ring, REG_A5XX_SP_VS_CTRL_REG0, 1);
   OUT_RING(
      ring,
   A5XX_SP_VS_CTRL_REG0_HALFREGFOOTPRINT(s[VS].i->max_half_reg + 1) |
      A5XX_SP_VS_CTRL_REG0_FULLREGFOOTPRINT(s[VS].i->max_reg + 1) |
   COND(s[VS].instrlen != 0, A5XX_SP_VS_CTRL_REG0_BUFFER) |
   /* XXX: 0x2 is only unset in
   * dEQP-GLES3.functional.ubo.single_nested_struct_array.single_buffer.packed_instance_array_vertex
   * on a collection of blob traces.  That shader is 1091 instrs, 0
   * half, 3 full, 108 constlen.  Other >1091 instr non-VS shaders don't
   * unset it, so that's not the trick.
   */
   0x2 |
            /* If we have streamout, link against the real FS in the binning program,
   * rather than the dummy FS used for binning pass state, to ensure the
   * OUTLOC's match.  Depending on whether we end up doing sysmem or gmem, the
   * actual streamout could happen with either the binning pass or draw pass
   * program, but the same streamout stateobj is used in either case:
   */
   const struct ir3_shader_variant *link_fs = s[FS].v;
   if (do_streamout && emit->binning_pass)
         struct ir3_shader_linkage l = {0};
            uint8_t clip0_loc = l.clip0_loc;
            OUT_PKT4(ring, REG_A5XX_VPC_VAR_DISABLE(0), 4);
   OUT_RING(ring, ~l.varmask[0]); /* VPC_VAR[0].DISABLE */
   OUT_RING(ring, ~l.varmask[1]); /* VPC_VAR[1].DISABLE */
   OUT_RING(ring, ~l.varmask[2]); /* VPC_VAR[2].DISABLE */
            /* Add stream out outputs after computing the VPC_VAR_DISABLE bitmask. */
            /* a5xx appends pos/psize to end of the linkage map: */
   if (VALIDREG(pos_regid))
            if (VALIDREG(psize_regid)) {
      psize_loc = l.max_loc;
               /* Handle the case where clip/cull distances aren't read by the FS. Make
   * sure to avoid adding an output with an empty writemask if the user
   * disables all the clip distances in the API so that the slot is unused.
   */
   if (clip0_loc == 0xff && VALIDREG(clip0_regid) &&
      (clip_cull_mask & 0xf) != 0) {
   clip0_loc = l.max_loc;
   ir3_link_add(&l, VARYING_SLOT_CLIP_DIST0, clip0_regid,
               if (clip1_loc == 0xff && VALIDREG(clip1_regid) &&
      (clip_cull_mask >> 4) != 0) {
   clip1_loc = l.max_loc;
   ir3_link_add(&l, VARYING_SLOT_CLIP_DIST1, clip1_regid,
               /* If we have stream-out, we use the full shader for binning
   * pass, rather than the optimized binning pass one, so that we
   * have all the varying outputs available for xfb.  So streamout
   * state should always be derived from the non-binning pass
   * program:
   */
   if (do_streamout && !emit->binning_pass)
            for (i = 0, j = 0; (i < 16) && (j < l.cnt); i++) {
                        reg |= A5XX_SP_VS_OUT_REG_A_REGID(l.var[j].regid);
   reg |= A5XX_SP_VS_OUT_REG_A_COMPMASK(l.var[j].compmask);
            reg |= A5XX_SP_VS_OUT_REG_B_REGID(l.var[j].regid);
   reg |= A5XX_SP_VS_OUT_REG_B_COMPMASK(l.var[j].compmask);
                        for (i = 0, j = 0; (i < 8) && (j < l.cnt); i++) {
                        reg |= A5XX_SP_VS_VPC_DST_REG_OUTLOC0(l.var[j++].loc);
   reg |= A5XX_SP_VS_VPC_DST_REG_OUTLOC1(l.var[j++].loc);
   reg |= A5XX_SP_VS_VPC_DST_REG_OUTLOC2(l.var[j++].loc);
                                 if (s[VS].instrlen)
            // TODO depending on other bits in this reg (if any) set somewhere else?
   OUT_PKT4(ring, REG_A5XX_PC_PRIM_VTX_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_SP_PRIMITIVE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_VPC_CNTL_0, 1);
   OUT_RING(ring, A5XX_VPC_CNTL_0_STRIDE_IN_VPC(l.max_loc) |
                           if (emit->binning_pass) {
      OUT_PKT4(ring, REG_A5XX_SP_FS_OBJ_START_LO, 2);
   OUT_RING(ring, 0x00000000); /* SP_FS_OBJ_START_LO */
      } else {
                  OUT_PKT4(ring, REG_A5XX_HLSQ_CONTROL_0_REG, 5);
   OUT_RING(ring, A5XX_HLSQ_CONTROL_0_REG_FSTHREADSIZE(fssz) |
               OUT_RING(ring, A5XX_HLSQ_CONTROL_1_REG_PRIMALLOCTHRESHOLD(63));
   OUT_RING(ring, A5XX_HLSQ_CONTROL_2_REG_FACEREGID(face_regid) |
                     OUT_RING(
      ring,
   A5XX_HLSQ_CONTROL_3_REG_IJ_PERSP_PIXEL(ij_regid[IJ_PERSP_PIXEL]) |
      A5XX_HLSQ_CONTROL_3_REG_IJ_LINEAR_PIXEL(ij_regid[IJ_LINEAR_PIXEL]) |
   A5XX_HLSQ_CONTROL_3_REG_IJ_PERSP_CENTROID(
         A5XX_HLSQ_CONTROL_3_REG_IJ_PERSP_CENTROID(
   OUT_RING(
      ring,
   A5XX_HLSQ_CONTROL_4_REG_XYCOORDREGID(coord_regid) |
      A5XX_HLSQ_CONTROL_4_REG_ZWCOORDREGID(zwcoord_regid) |
            OUT_PKT4(ring, REG_A5XX_SP_FS_CTRL_REG0, 1);
   OUT_RING(
      ring,
   COND(s[FS].v->total_in > 0, A5XX_SP_FS_CTRL_REG0_VARYING) |
      0x40002 | /* XXX set pretty much everywhere */
   COND(s[FS].instrlen != 0, A5XX_SP_FS_CTRL_REG0_BUFFER) |
   A5XX_SP_FS_CTRL_REG0_THREADSIZE(fssz) |
   A5XX_SP_FS_CTRL_REG0_HALFREGFOOTPRINT(s[FS].i->max_half_reg + 1) |
   A5XX_SP_FS_CTRL_REG0_FULLREGFOOTPRINT(s[FS].i->max_reg + 1) |
            OUT_PKT4(ring, REG_A5XX_HLSQ_UPDATE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_VPC_GS_SIV_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_SP_SP_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_CNTL, 1);
   OUT_RING(ring,
            CONDREG(ij_regid[IJ_PERSP_PIXEL], A5XX_GRAS_CNTL_IJ_PERSP_PIXEL) |
      CONDREG(ij_regid[IJ_PERSP_CENTROID],
         CONDREG(ij_regid[IJ_PERSP_SAMPLE],
         CONDREG(ij_regid[IJ_LINEAR_PIXEL], A5XX_GRAS_CNTL_IJ_LINEAR_PIXEL) |
   CONDREG(ij_regid[IJ_LINEAR_CENTROID],
         CONDREG(ij_regid[IJ_LINEAR_SAMPLE],
         COND(s[FS].v->fragcoord_compmask != 0,
      A5XX_GRAS_CNTL_COORD_MASK(s[FS].v->fragcoord_compmask) |
         OUT_PKT4(ring, REG_A5XX_RB_RENDER_CONTROL0, 2);
   OUT_RING(
      ring,
   CONDREG(ij_regid[IJ_PERSP_PIXEL],
            CONDREG(ij_regid[IJ_PERSP_CENTROID],
         CONDREG(ij_regid[IJ_PERSP_SAMPLE],
         CONDREG(ij_regid[IJ_LINEAR_PIXEL],
         CONDREG(ij_regid[IJ_LINEAR_CENTROID],
         CONDREG(ij_regid[IJ_LINEAR_SAMPLE],
         COND(s[FS].v->fragcoord_compmask != 0,
      A5XX_RB_RENDER_CONTROL0_COORD_MASK(s[FS].v->fragcoord_compmask) |
      COND(s[FS].v->frag_face, A5XX_RB_RENDER_CONTROL0_IJ_LINEAR_PIXEL) |
   OUT_RING(ring,
            CONDREG(samp_mask_regid, A5XX_RB_RENDER_CONTROL1_SAMPLEMASK) |
         OUT_PKT4(ring, REG_A5XX_SP_FS_OUTPUT_REG(0), 8);
   for (i = 0; i < 8; i++) {
      OUT_RING(ring, A5XX_SP_FS_OUTPUT_REG_REGID(color_regid[i]) |
                     OUT_PKT4(ring, REG_A5XX_VPC_PACK, 1);
   OUT_RING(ring, A5XX_VPC_PACK_NUMNONPOSVAR(s[FS].v->total_in) |
            if (!emit->binning_pass) {
               memset(vinterp, 0, sizeof(vinterp));
            /* looks like we need to do int varyings in the frag
   * shader on a5xx (no flatshad reg?  or a420.0 bug?):
   *
   *    (sy)(ss)nop
   *    (sy)ldlv.u32 r0.x,l[r0.x], 1
   *    ldlv.u32 r0.y,l[r0.x+1], 1
   *    (ss)bary.f (ei)r63.x, 0, r0.x
   *    (ss)(rpt1)cov.s32f16 hr0.x, (r)r0.x
   *    (rpt5)nop
   *    sam (f16)(xyzw)hr0.x, hr0.x, s#0, t#0
   *
   * Possibly on later a5xx variants we'll be able to use
   * something like the code below instead of workaround
   * in the shader:
   */
   /* figure out VARYING_INTERP / VARYING_PS_REPL register values: */
   for (j = -1;
      (j = ir3_next_varying(s[FS].v, j)) < (int)s[FS].v->inputs_count;) {
   /* NOTE: varyings are packed, so if compmask is 0xb
   * then first, third, and fourth component occupy
   * three consecutive varying slots:
                           if (s[FS].v->inputs[j].flat ||
                     for (i = 0; i < 4; i++) {
      if (compmask & (1 << i)) {
      vinterp[loc / 16] |= 1 << ((loc % 16) * 2);
   // flatshade[loc / 32] |= 1 << (loc % 32);
                     bool coord_mode = emit->sprite_coord_mode;
   if (ir3_point_sprite(s[FS].v, j, emit->sprite_coord_enable,
            /* mask is two 2-bit fields, where:
   *   '01' -> S
   *   '10' -> T
   *   '11' -> 1 - T  (flip mode)
   */
   unsigned mask = coord_mode ? 0b1101 : 0b1001;
   uint32_t loc = inloc;
   if (compmask & 0x1) {
      vpsrepl[loc / 16] |= ((mask >> 0) & 0x3) << ((loc % 16) * 2);
      }
   if (compmask & 0x2) {
      vpsrepl[loc / 16] |= ((mask >> 2) & 0x3) << ((loc % 16) * 2);
      }
   if (compmask & 0x4) {
      /* .z <- 0.0f */
   vinterp[loc / 16] |= 0b10 << ((loc % 16) * 2);
      }
   if (compmask & 0x8) {
      /* .w <- 1.0f */
   vinterp[loc / 16] |= 0b11 << ((loc % 16) * 2);
                     OUT_PKT4(ring, REG_A5XX_VPC_VARYING_INTERP_MODE(0), 8);
   for (i = 0; i < 8; i++)
            OUT_PKT4(ring, REG_A5XX_VPC_VARYING_PS_REPL_MODE(0), 8);
   for (i = 0; i < 8; i++)
               OUT_PKT4(ring, REG_A5XX_GRAS_VS_CL_CNTL, 1);
   OUT_RING(ring, A5XX_GRAS_VS_CL_CNTL_CLIP_MASK(clip_mask) |
            OUT_PKT4(ring, REG_A5XX_VPC_CLIP_CNTL, 1);
   OUT_RING(ring, A5XX_VPC_CLIP_CNTL_CLIP_MASK(clip_cull_mask) |
                  OUT_PKT4(ring, REG_A5XX_PC_CLIP_CNTL, 1);
            if (!emit->binning_pass)
      if (s[FS].instrlen)
         OUT_PKT4(ring, REG_A5XX_VFD_CONTROL_1, 5);
   OUT_RING(ring, A5XX_VFD_CONTROL_1_REGID4VTX(vertex_regid) |
         OUT_RING(ring, 0x0000fcfc); /* VFD_CONTROL_2 */
   OUT_RING(ring, 0x0000fcfc); /* VFD_CONTROL_3 */
   OUT_RING(ring, 0x000000fc); /* VFD_CONTROL_4 */
      }
      static struct ir3_program_state *
   fd5_program_create(void *data, const struct ir3_shader_variant *bs,
                     const struct ir3_shader_variant *vs,
   const struct ir3_shader_variant *hs,
   const struct ir3_shader_variant *ds,
   {
      struct fd_context *ctx = fd_context(data);
                     state->bs = bs;
   state->vs = vs;
               }
      static void
   fd5_program_destroy(void *data, struct ir3_program_state *state)
   {
      struct fd5_program_state *so = fd5_program_state(state);
      }
      static const struct ir3_cache_funcs cache_funcs = {
      .create_state = fd5_program_create,
      };
      void
   fd5_prog_init(struct pipe_context *pctx)
   {
               ctx->shader_cache = ir3_cache_create(&cache_funcs, ctx);
   ir3_prog_init(pctx);
      }
