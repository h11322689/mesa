   /*
   * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "freedreno_program.h"
      #include "fd3_emit.h"
   #include "fd3_format.h"
   #include "fd3_program.h"
   #include "fd3_texture.h"
      bool
   fd3_needs_manual_clipping(const struct ir3_shader *shader,
         {
               return (!rast->depth_clip_near ||
         util_bitcount(rast->clip_plane_enable) > 6 ||
   outputs & ((1ULL << VARYING_SLOT_CLIP_VERTEX) |
      }
      static void
   emit_shader(struct fd_ringbuffer *ring, const struct ir3_shader_variant *so)
   {
      const struct ir3_info *si = &so->info;
   enum adreno_state_block sb;
   enum adreno_state_src src;
            if (so->type == MESA_SHADER_VERTEX) {
         } else {
                  if (FD_DBG(DIRECT)) {
      sz = si->sizedwords;
   src = SS_DIRECT;
      } else {
      sz = 0;
   src = SS_INDIRECT;
               OUT_PKT3(ring, CP_LOAD_STATE, 2 + sz);
   OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(0) | CP_LOAD_STATE_0_STATE_SRC(src) |
               if (bin) {
      OUT_RING(ring, CP_LOAD_STATE_1_EXT_SRC_ADDR(0) |
      } else {
         }
   for (i = 0; i < sz; i++) {
            }
      void
   fd3_program_emit(struct fd_ringbuffer *ring, struct fd3_emit *emit, int nr,
         {
      const struct ir3_shader_variant *vp, *fp;
   const struct ir3_info *vsi, *fsi;
   enum a3xx_instrbuffermode fpbuffer, vpbuffer;
   uint32_t fpbuffersz, vpbuffersz, fsoff;
   uint32_t pos_regid, posz_regid, psize_regid;
   uint32_t ij_regid[4], face_regid, coord_regid, zwcoord_regid;
   uint32_t color_regid[4] = {0};
   int constmode;
                     vp = fd3_emit_get_vp(emit);
            vsi = &vp->info;
            fpbuffer = BUFFER;
   vpbuffer = BUFFER;
   fpbuffersz = fp->instrlen;
            /*
   * Decide whether to use BUFFER or CACHE mode for VS and FS.  It
   * appears like 256 is the hard limit, but when the combined size
   * exceeds 128 then blob will try to keep FS in BUFFER mode and
   * switch to CACHE for VS until VS is too large.  The blob seems
   * to switch FS out of BUFFER mode at slightly under 128.  But
   * a bit fuzzy on the decision tree, so use slightly conservative
   * limits.
   *
   * TODO check if these thresholds for BUFFER vs CACHE mode are the
   *      same for all a3xx or whether we need to consider the gpuid
            if ((fpbuffersz + vpbuffersz) > 128) {
      if (fpbuffersz < 112) {
      /* FP:BUFFER   VP:CACHE  */
   vpbuffer = CACHE;
      } else if (vpbuffersz < 112) {
      /* FP:CACHE    VP:BUFFER */
   fpbuffer = CACHE;
      } else {
      /* FP:CACHE    VP:CACHE  */
   vpbuffer = fpbuffer = CACHE;
                  if (fpbuffer == BUFFER) {
         } else {
                  /* seems like vs->constlen + fs->constlen > 256, then CONSTMODE=1 */
            pos_regid = ir3_find_output_regid(vp, VARYING_SLOT_POS);
   posz_regid = ir3_find_output_regid(fp, FRAG_RESULT_DEPTH);
   psize_regid = ir3_find_output_regid(vp, VARYING_SLOT_PSIZ);
   if (fp->color0_mrt) {
      color_regid[0] = color_regid[1] = color_regid[2] = color_regid[3] =
      } else {
      color_regid[0] = ir3_find_output_regid(fp, FRAG_RESULT_DATA0);
   color_regid[1] = ir3_find_output_regid(fp, FRAG_RESULT_DATA1);
   color_regid[2] = ir3_find_output_regid(fp, FRAG_RESULT_DATA2);
               face_regid = ir3_find_sysval_regid(fp, SYSTEM_VALUE_FRONT_FACE);
   coord_regid = ir3_find_sysval_regid(fp, SYSTEM_VALUE_FRAG_COORD);
   zwcoord_regid =
         ij_regid[0] =
         ij_regid[1] =
         ij_regid[2] =
         ij_regid[3] =
            /* adjust regids for alpha output formats. there is no alpha render
   * format, so it's just treated like red
   */
   for (i = 0; i < nr; i++)
      if (util_format_is_alpha(pipe_surface_format(bufs[i])))
         /* we could probably divide this up into things that need to be
   * emitted if frag-prog is dirty vs if vert-prog is dirty..
            OUT_PKT0(ring, REG_A3XX_HLSQ_CONTROL_0_REG, 6);
   OUT_RING(ring, A3XX_HLSQ_CONTROL_0_REG_FSTHREADSIZE(FOUR_QUADS) |
                     A3XX_HLSQ_CONTROL_0_REG_FSSUPERTHREADENABLE |
   A3XX_HLSQ_CONTROL_0_REG_CONSTMODE(constmode) |
   /* NOTE:  I guess SHADERRESTART and CONSTFULLUPDATE maybe
   * flush some caches? I think we only need to set those
   * bits if we have updated const or shader..
   OUT_RING(ring, A3XX_HLSQ_CONTROL_1_REG_VSTHREADSIZE(TWO_QUADS) |
                     OUT_RING(ring, A3XX_HLSQ_CONTROL_2_REG_PRIMALLOCTHRESHOLD(31) |
         OUT_RING(ring,
            A3XX_HLSQ_CONTROL_3_REG_IJPERSPCENTERREGID(ij_regid[0]) |
      A3XX_HLSQ_CONTROL_3_REG_IJNONPERSPCENTERREGID(ij_regid[1]) |
   OUT_RING(ring, A3XX_HLSQ_VS_CONTROL_REG_CONSTLENGTH(vp->constlen) |
               OUT_RING(ring, A3XX_HLSQ_FS_CONTROL_REG_CONSTLENGTH(fp->constlen) |
                  OUT_PKT0(ring, REG_A3XX_SP_SP_CTRL_REG, 1);
   OUT_RING(ring, A3XX_SP_SP_CTRL_REG_CONSTMODE(constmode) |
                        OUT_PKT0(ring, REG_A3XX_SP_VS_LENGTH_REG, 1);
            OUT_PKT0(ring, REG_A3XX_SP_VS_CTRL_REG0, 3);
   OUT_RING(ring,
            A3XX_SP_VS_CTRL_REG0_THREADMODE(MULTI) |
      A3XX_SP_VS_CTRL_REG0_INSTRBUFFERMODE(vpbuffer) |
   COND(vpbuffer == CACHE, A3XX_SP_VS_CTRL_REG0_CACHEINVALID) |
   A3XX_SP_VS_CTRL_REG0_HALFREGFOOTPRINT(vsi->max_half_reg + 1) |
   A3XX_SP_VS_CTRL_REG0_FULLREGFOOTPRINT(vsi->max_reg + 1) |
   A3XX_SP_VS_CTRL_REG0_THREADSIZE(TWO_QUADS) |
   OUT_RING(ring,
            A3XX_SP_VS_CTRL_REG1_CONSTLENGTH(vp->constlen) |
      OUT_RING(ring, A3XX_SP_VS_PARAM_REG_POSREGID(pos_regid) |
                  struct ir3_shader_linkage l = {0};
            for (i = 0, j = 0; (i < 16) && (j < l.cnt); i++) {
                        reg |= A3XX_SP_VS_OUT_REG_A_REGID(l.var[j].regid);
   reg |= A3XX_SP_VS_OUT_REG_A_COMPMASK(l.var[j].compmask);
            reg |= A3XX_SP_VS_OUT_REG_B_REGID(l.var[j].regid);
   reg |= A3XX_SP_VS_OUT_REG_B_COMPMASK(l.var[j].compmask);
                        for (i = 0, j = 0; (i < 8) && (j < l.cnt); i++) {
                        reg |= A3XX_SP_VS_VPC_DST_REG_OUTLOC0(l.var[j++].loc + 8);
   reg |= A3XX_SP_VS_VPC_DST_REG_OUTLOC1(l.var[j++].loc + 8);
   reg |= A3XX_SP_VS_VPC_DST_REG_OUTLOC2(l.var[j++].loc + 8);
                        OUT_PKT0(ring, REG_A3XX_SP_VS_OBJ_OFFSET_REG, 2);
   OUT_RING(ring, A3XX_SP_VS_OBJ_OFFSET_REG_CONSTOBJECTOFFSET(0) |
                  if (emit->binning_pass) {
      OUT_PKT0(ring, REG_A3XX_SP_FS_LENGTH_REG, 1);
            OUT_PKT0(ring, REG_A3XX_SP_FS_CTRL_REG0, 2);
   OUT_RING(ring, A3XX_SP_FS_CTRL_REG0_THREADMODE(MULTI) |
                  OUT_PKT0(ring, REG_A3XX_SP_FS_OBJ_OFFSET_REG, 1);
   OUT_RING(ring, A3XX_SP_FS_OBJ_OFFSET_REG_CONSTOBJECTOFFSET(128) |
      } else {
      OUT_PKT0(ring, REG_A3XX_SP_FS_LENGTH_REG, 1);
            OUT_PKT0(ring, REG_A3XX_SP_FS_CTRL_REG0, 2);
   OUT_RING(ring,
            A3XX_SP_FS_CTRL_REG0_THREADMODE(MULTI) |
      A3XX_SP_FS_CTRL_REG0_INSTRBUFFERMODE(fpbuffer) |
   COND(fpbuffer == CACHE, A3XX_SP_FS_CTRL_REG0_CACHEINVALID) |
   A3XX_SP_FS_CTRL_REG0_HALFREGFOOTPRINT(fsi->max_half_reg + 1) |
   A3XX_SP_FS_CTRL_REG0_FULLREGFOOTPRINT(fsi->max_reg + 1) |
   A3XX_SP_FS_CTRL_REG0_INOUTREGOVERLAP |
   A3XX_SP_FS_CTRL_REG0_THREADSIZE(FOUR_QUADS) |
   A3XX_SP_FS_CTRL_REG0_SUPERTHREADMODE |
   OUT_RING(ring, A3XX_SP_FS_CTRL_REG1_CONSTLENGTH(fp->constlen) |
                              OUT_PKT0(ring, REG_A3XX_SP_FS_OBJ_OFFSET_REG, 2);
   OUT_RING(ring, A3XX_SP_FS_OBJ_OFFSET_REG_CONSTOBJECTOFFSET(
                           OUT_PKT0(ring, REG_A3XX_SP_FS_OUTPUT_REG, 1);
   OUT_RING(ring, COND(fp->writes_pos, A3XX_SP_FS_OUTPUT_REG_DEPTH_ENABLE) |
                  OUT_PKT0(ring, REG_A3XX_SP_FS_MRT_REG(0), 4);
   for (i = 0; i < 4; i++) {
      uint32_t mrt_reg =
                  if (i < nr) {
      enum pipe_format fmt = pipe_surface_format(bufs[i]);
   mrt_reg |=
      COND(util_format_is_pure_uint(fmt), A3XX_SP_FS_MRT_REG_UINT) |
   }
               if (emit->binning_pass) {
      OUT_PKT0(ring, REG_A3XX_VPC_ATTR, 2);
   OUT_RING(ring, A3XX_VPC_ATTR_THRDASSIGN(1) | A3XX_VPC_ATTR_LMSIZE(1) |
            } else {
               memset(vinterp, 0, sizeof(vinterp));
   memset(flatshade, 0, sizeof(flatshade));
            /* figure out VARYING_INTERP / FLAT_SHAD register values: */
   for (j = -1; (j = ir3_next_varying(fp, j)) < (int)fp->inputs_count;) {
      /* NOTE: varyings are packed, so if compmask is 0xb
   * then first, third, and fourth component occupy
   * three consecutive varying slots:
                           if (fp->inputs[j].flat ||
                     for (i = 0; i < 4; i++) {
      if (compmask & (1 << i)) {
      vinterp[loc / 16] |= FLAT << ((loc % 16) * 2);
   flatshade[loc / 32] |= 1 << (loc % 32);
                     bool coord_mode = emit->sprite_coord_mode;
   if (ir3_point_sprite(fp, j, emit->sprite_coord_enable, &coord_mode)) {
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
                     OUT_PKT0(ring, REG_A3XX_VPC_ATTR, 2);
   OUT_RING(ring, A3XX_VPC_ATTR_TOTALATTR(fp->total_in) |
               OUT_RING(ring, A3XX_VPC_PACK_NUMFPNONPOSVAR(fp->total_in) |
            OUT_PKT0(ring, REG_A3XX_VPC_VARYING_INTERP_MODE(0), 4);
   OUT_RING(ring, vinterp[0]); /* VPC_VARYING_INTERP[0].MODE */
   OUT_RING(ring, vinterp[1]); /* VPC_VARYING_INTERP[1].MODE */
   OUT_RING(ring, vinterp[2]); /* VPC_VARYING_INTERP[2].MODE */
            OUT_PKT0(ring, REG_A3XX_VPC_VARYING_PS_REPL_MODE(0), 4);
   OUT_RING(ring, vpsrepl[0]); /* VPC_VARYING_PS_REPL[0].MODE */
   OUT_RING(ring, vpsrepl[1]); /* VPC_VARYING_PS_REPL[1].MODE */
   OUT_RING(ring, vpsrepl[2]); /* VPC_VARYING_PS_REPL[2].MODE */
            OUT_PKT0(ring, REG_A3XX_SP_FS_FLAT_SHAD_MODE_REG_0, 2);
   OUT_RING(ring, flatshade[0]); /* SP_FS_FLAT_SHAD_MODE_REG_0 */
               if (vpbuffer == BUFFER)
            OUT_PKT0(ring, REG_A3XX_VFD_PERFCOUNTER0_SELECT, 1);
            if (!emit->binning_pass) {
      if (fpbuffer == BUFFER)
            OUT_PKT0(ring, REG_A3XX_VFD_PERFCOUNTER0_SELECT, 1);
         }
      static struct ir3_program_state *
   fd3_program_create(void *data, const struct ir3_shader_variant *bs,
                     const struct ir3_shader_variant *vs, 
   const struct ir3_shader_variant *hs,
   const struct ir3_shader_variant *ds,
   {
      struct fd_context *ctx = fd_context(data);
                     state->bs = bs;
   state->vs = vs;
               }
      static void
   fd3_program_destroy(void *data, struct ir3_program_state *state)
   {
      struct fd3_program_state *so = fd3_program_state(state);
      }
      static const struct ir3_cache_funcs cache_funcs = {
      .create_state = fd3_program_create,
      };
      void
   fd3_prog_init(struct pipe_context *pctx)
   {
               ctx->shader_cache = ir3_cache_create(&cache_funcs, ctx);
   ir3_prog_init(pctx);
      }
