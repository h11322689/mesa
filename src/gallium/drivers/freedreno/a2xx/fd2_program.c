   /*
   * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
   *    Jonathan Marek <jonathan@marek.ca>
   */
      #include "nir/tgsi_to_nir.h"
   #include "pipe/p_state.h"
   #include "tgsi/tgsi_dump.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "freedreno_program.h"
      #include "ir2/instr-a2xx.h"
   #include "fd2_program.h"
   #include "fd2_texture.h"
   #include "fd2_util.h"
   #include "ir2.h"
      static struct fd2_shader_stateobj *
   create_shader(struct pipe_context *pctx, gl_shader_stage type)
   {
      struct fd2_shader_stateobj *so = CALLOC_STRUCT(fd2_shader_stateobj);
   if (!so)
         so->type = type;
   so->is_a20x = is_a20x(fd_context(pctx)->screen);
      }
      static void
   delete_shader(struct fd2_shader_stateobj *so)
   {
      if (!so)
         ralloc_free(so->nir);
   for (int i = 0; i < ARRAY_SIZE(so->variant); i++)
            }
      static void
   emit(struct fd_ringbuffer *ring, gl_shader_stage type,
         {
                        OUT_PKT3(ring, CP_IM_LOAD_IMMEDIATE, 2 + info->sizedwords);
   OUT_RING(ring, type == MESA_SHADER_FRAGMENT);
            if (patches)
      util_dynarray_append(patches, uint32_t *,
         for (i = 0; i < info->sizedwords; i++)
      }
      static int
   ir2_glsl_type_size(const struct glsl_type *type, bool bindless)
   {
         }
      static void *
   fd2_fp_state_create(struct pipe_context *pctx,
         {
      struct fd2_shader_stateobj *so = create_shader(pctx, MESA_SHADER_FRAGMENT);
   if (!so)
            so->nir = (cso->type == PIPE_SHADER_IR_NIR)
                  NIR_PASS_V(so->nir, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
            if (ir2_optimize_nir(so->nir, true))
                              ralloc_free(so->nir);
   so->nir = NULL;
         fail:
      delete_shader(so);
      }
      static void
   fd2_fp_state_delete(struct pipe_context *pctx, void *hwcso)
   {
      struct fd2_shader_stateobj *so = hwcso;
      }
      static void *
   fd2_vp_state_create(struct pipe_context *pctx,
         {
      struct fd2_shader_stateobj *so = create_shader(pctx, MESA_SHADER_VERTEX);
   if (!so)
            so->nir = (cso->type == PIPE_SHADER_IR_NIR)
                  NIR_PASS_V(so->nir, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
            if (ir2_optimize_nir(so->nir, true))
                     /* compile binning variant now */
                  fail:
      delete_shader(so);
      }
      static void
   fd2_vp_state_delete(struct pipe_context *pctx, void *hwcso)
   {
      struct fd2_shader_stateobj *so = hwcso;
      }
      static void
   patch_vtx_fetch(struct fd_context *ctx, struct pipe_vertex_element *elem,
         {
               instr->dst_swiz = fd2_vtx_swiz(elem->src_format, dst_swiz);
   instr->format_comp_all = fmt.sign == SQ_TEX_SIGN_SIGNED;
   instr->num_format_all = fmt.num_format;
   instr->format = fmt.format;
   instr->exp_adjust_all = fmt.exp_adjust;
   instr->stride = elem->src_stride;
      }
      static void
   patch_fetches(struct fd_context *ctx, struct ir2_shader_info *info,
               {
      for (int i = 0; i < info->num_fetch_instrs; i++) {
               instr_fetch_t *instr = (instr_fetch_t *)&info->dwords[fi->offset];
   if (instr->opc == VTX_FETCH) {
      unsigned idx =
         patch_vtx_fetch(ctx, &vtx->pipe[idx], &instr->vtx, fi->vtx.dst_swiz);
               assert(instr->opc == TEX_FETCH);
   instr->tex.const_idx = fd2_get_const_idx(ctx, tex, fi->tex.samp_id);
         }
      void
   fd2_program_emit(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
      struct fd2_shader_stateobj *fp = NULL, *vp;
   struct ir2_shader_info *fpi, *vpi;
   struct ir2_frag_linkage *f;
   uint8_t vs_gprs, fs_gprs = 0, vs_export = 0;
   enum a2xx_sq_ps_vtx_mode mode = POSITION_1_VECTOR;
   bool binning = (ctx->batch && ring == ctx->batch->binning);
                     /* find variant matching the linked fragment shader */
   if (!binning) {
      fp = prog->fs;
   for (variant = 1; variant < ARRAY_SIZE(vp->variant); variant++) {
      /* if checked all variants, compile a new variant */
   if (!vp->variant[variant].info.sizedwords) {
      ir2_compile(vp, variant, fp);
               /* check if fragment shader linkage matches */
   if (!memcmp(&vp->variant[variant].f, &fp->variant[0].f,
            }
               vpi = &vp->variant[variant].info;
   fpi = &fp->variant[0].info;
            /* clear/gmem2mem/mem2gmem need to be changed to remove this condition */
   if (prog != &ctx->solid_prog && prog != &ctx->blit_prog[0]) {
      patch_fetches(ctx, vpi, ctx->vtx.vtx, &ctx->tex[PIPE_SHADER_VERTEX]);
   if (fp)
               emit(ring, MESA_SHADER_VERTEX, vpi,
            if (fp) {
      emit(ring, MESA_SHADER_FRAGMENT, fpi, NULL);
   fs_gprs = (fpi->max_reg < 0) ? 0x80 : fpi->max_reg;
                        if (vp->writes_psize && !binning)
            /* set register to use for param (fragcoord/pointcoord/frontfacing) */
   OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_SQ_CONTEXT_MISC));
   OUT_RING(ring,
            A2XX_SQ_CONTEXT_MISC_SC_SAMPLE_CNTL(CENTERS_ONLY) |
               OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_SQ_PROGRAM_CNTL));
   OUT_RING(ring,
            A2XX_SQ_PROGRAM_CNTL_PS_EXPORT_MODE(2) |
      A2XX_SQ_PROGRAM_CNTL_VS_EXPORT_MODE(mode) |
   A2XX_SQ_PROGRAM_CNTL_VS_RESOURCE |
   A2XX_SQ_PROGRAM_CNTL_PS_RESOURCE |
   A2XX_SQ_PROGRAM_CNTL_VS_EXPORT_COUNT(vs_export) |
   A2XX_SQ_PROGRAM_CNTL_PS_REGS(fs_gprs) |
      }
      void
   fd2_prog_init(struct pipe_context *pctx)
   {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_program_stateobj *prog;
   struct fd2_shader_stateobj *so;
   struct ir2_shader_info *info;
            pctx->create_fs_state = fd2_fp_state_create;
            pctx->create_vs_state = fd2_vp_state_create;
                              prog = &ctx->solid_prog;
   so = prog->vs;
         #define IR2_FETCH_SWIZ_XY01 0xb08
   #define IR2_FETCH_SWIZ_XYZ1 0xa88
                  instr = (instr_fetch_vtx_t *)&info->dwords[info->fetch_info[0].offset];
   instr->const_index = 26;
   instr->const_index_sel = 0;
   instr->format = FMT_32_32_32_FLOAT;
   instr->format_comp_all = false;
   instr->stride = 12;
   instr->num_format_all = true;
            prog = &ctx->blit_prog[0];
   so = prog->vs;
                     instr = (instr_fetch_vtx_t *)&info->dwords[info->fetch_info[0].offset];
   instr->const_index = 26;
   instr->const_index_sel = 1;
   instr->format = FMT_32_32_FLOAT;
   instr->format_comp_all = false;
   instr->stride = 8;
   instr->num_format_all = false;
            instr = (instr_fetch_vtx_t *)&info->dwords[info->fetch_info[1].offset];
   instr->const_index = 26;
   instr->const_index_sel = 0;
   instr->format = FMT_32_32_32_FLOAT;
   instr->format_comp_all = false;
   instr->stride = 12;
   instr->num_format_all = false;
      }
