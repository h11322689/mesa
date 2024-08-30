   /*
   * Copyright 2010 Christoph Bumiller
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "pipe/p_defines.h"
      #include "compiler/nir/nir.h"
      #include "nv50/nv50_context.h"
   #include "nv50/nv50_program.h"
      #include "nv50_ir_driver.h"
      static inline unsigned
   bitcount4(const uint32_t val)
   {
      static const uint8_t cnt[16]
   = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
      }
      static int
   nv50_vertprog_assign_slots(struct nv50_ir_prog_info_out *info)
   {
      struct nv50_program *prog = (struct nv50_program *)info->driverPriv;
            n = 0;
   for (i = 0; i < info->numInputs; ++i) {
      prog->in[i].id = i;
   prog->in[i].sn = info->in[i].sn;
   prog->in[i].si = info->in[i].si;
   prog->in[i].hw = n;
                     for (c = 0; c < 4; ++c)
                  if (info->in[i].sn == TGSI_SEMANTIC_PRIMID)
      }
            for (i = 0; i < info->numSysVals; ++i) {
      switch (info->sv[i].sn) {
   case SYSTEM_VALUE_INSTANCE_ID:
      prog->vp.attrs[2] |= NV50_3D_VP_GP_BUILTIN_ATTR_EN_INSTANCE_ID;
      case SYSTEM_VALUE_VERTEX_ID:
      prog->vp.attrs[2] |= NV50_3D_VP_GP_BUILTIN_ATTR_EN_VERTEX_ID;
   prog->vp.attrs[2] |= NV50_3D_VP_GP_BUILTIN_ATTR_EN_VERTEX_ID_DRAW_ARRAYS_ADD_START;
      case SYSTEM_VALUE_PRIMITIVE_ID:
      prog->vp.attrs[2] |= NV50_3D_VP_GP_BUILTIN_ATTR_EN_PRIMITIVE_ID;
      default:
                     /*
   * Corner case: VP has no inputs, but we will still need to submit data to
   * draw it. HW will shout at us and won't draw anything if we don't enable
   * any input, so let's just pretend it's the first one.
   */
   if (prog->vp.attrs[0] == 0 &&
      prog->vp.attrs[1] == 0 &&
   prog->vp.attrs[2] == 0)
         /* VertexID before InstanceID */
   if (info->io.vertexId < info->numSysVals)
         if (info->io.instanceId < info->numSysVals)
            n = 0;
   for (i = 0; i < info->numOutputs; ++i) {
      switch (info->out[i].sn) {
   case TGSI_SEMANTIC_PSIZE:
      prog->vp.psiz = i;
      case TGSI_SEMANTIC_CLIPDIST:
      prog->vp.clpd[info->out[i].si] = n;
      case TGSI_SEMANTIC_EDGEFLAG:
      prog->vp.edgeflag = i;
      case TGSI_SEMANTIC_BCOLOR:
      prog->vp.bfc[info->out[i].si] = i;
      case TGSI_SEMANTIC_LAYER:
      prog->gp.has_layer = true;
   prog->gp.layerid = n;
      case TGSI_SEMANTIC_VIEWPORT_INDEX:
      prog->gp.has_viewport = true;
   prog->gp.viewportid = n;
      default:
         }
   prog->out[i].id = i;
   prog->out[i].sn = info->out[i].sn;
   prog->out[i].si = info->out[i].si;
   prog->out[i].hw = n;
            for (c = 0; c < 4; ++c)
      if (info->out[i].mask & (1 << c))
   }
   prog->out_nr = info->numOutputs;
   prog->max_out = n;
   if (!prog->max_out)
            if (prog->vp.psiz < info->numOutputs)
               }
      static int
   nv50_fragprog_assign_slots(struct nv50_ir_prog_info_out *info)
   {
      struct nv50_program *prog = (struct nv50_program *)info->driverPriv;
   unsigned i, n, m, c;
   unsigned nvary;
   unsigned nflat;
            /* count recorded non-flat inputs */
   for (m = 0, i = 0; i < info->numInputs; ++i) {
      switch (info->in[i].sn) {
   case TGSI_SEMANTIC_POSITION:
         default:
      m += info->in[i].flat ? 0 : 1;
         }
            /* Fill prog->in[] so that non-flat inputs are first and
   * kick out special inputs that don't use the RESULT_MAP.
   */
   for (n = 0, i = 0; i < info->numInputs; ++i) {
      if (info->in[i].sn == TGSI_SEMANTIC_POSITION) {
      prog->fp.interp |= info->in[i].mask << 24;
   for (c = 0; c < 4; ++c)
      if (info->in[i].mask & (1 << c))
   } else {
               if (info->in[i].sn == TGSI_SEMANTIC_COLOR)
                        prog->in[j].id = i;
   prog->in[j].mask = info->in[i].mask;
   prog->in[j].sn = info->in[i].sn;
                        }
   if (!(prog->fp.interp & (8 << 24))) {
      ++nintp;
               for (i = 0; i < prog->in_nr; ++i) {
               prog->in[i].hw = nintp;
   for (c = 0; c < 4; ++c)
      if (prog->in[i].mask & (1 << c))
   }
   /* (n == m) if m never increased, i.e. no flat inputs */
   nflat = (n < m) ? (nintp - prog->in[n].hw) : 0;
   nintp -= bitcount4(prog->fp.interp >> 24); /* subtract position inputs */
            prog->fp.interp |= nvary << NV50_3D_FP_INTERPOLANT_CTRL_COUNT_NONFLAT__SHIFT;
            /* put front/back colors right after HPOS */
   prog->fp.colors = 4 << NV50_3D_SEMANTIC_COLOR_FFC0_ID__SHIFT;
   for (i = 0; i < 2; ++i)
      if (prog->vp.bfc[i] < 0xff)
                  if (info->prop.fp.numColourResults > 1)
            for (i = 0; i < info->numOutputs; ++i) {
      prog->out[i].id = i;
   prog->out[i].sn = info->out[i].sn;
   prog->out[i].si = info->out[i].si;
            if (i == info->io.fragDepth || i == info->io.sampleMask)
                  for (c = 0; c < 4; ++c)
                        if (info->io.sampleMask < PIPE_MAX_SHADER_OUTPUTS) {
      info->out[info->io.sampleMask].slot[0] = prog->max_out++;
               if (info->io.fragDepth < PIPE_MAX_SHADER_OUTPUTS)
            if (!prog->max_out)
               }
      static int
   nv50_program_assign_varying_slots(struct nv50_ir_prog_info_out *info)
   {
      switch (info->type) {
   case PIPE_SHADER_VERTEX:
         case PIPE_SHADER_GEOMETRY:
         case PIPE_SHADER_FRAGMENT:
         case PIPE_SHADER_COMPUTE:
         default:
            }
      static struct nv50_stream_output_state *
   nv50_program_create_strmout_state(const struct nv50_ir_prog_info_out *info,
         {
      struct nv50_stream_output_state *so;
   unsigned b, i, c;
            so = MALLOC_STRUCT(nv50_stream_output_state);
   if (!so)
                  for (b = 0; b < 4; ++b)
         for (i = 0; i < pso->num_outputs; ++i) {
      unsigned end =  pso->output[i].dst_offset + pso->output[i].num_components;
   b = pso->output[i].output_buffer;
   assert(b < 4);
                        so->stride[0] = pso->stride[0] * 4;
   base[0] = 0;
   for (b = 1; b < 4; ++b) {
      assert(!so->num_attribs[b] || so->num_attribs[b] == pso->stride[b]);
   so->stride[b] = so->num_attribs[b] * 4;
   if (so->num_attribs[b])
            }
   if (so->ctrl & NV50_3D_STRMOUT_BUFFERS_CTRL_INTERLEAVED) {
      assert(so->stride[0] < NV50_3D_STRMOUT_BUFFERS_CTRL_STRIDE__MAX);
                        for (i = 0; i < pso->num_outputs; ++i) {
      const unsigned s = pso->output[i].start_component;
   const unsigned p = pso->output[i].dst_offset;
   const unsigned r = pso->output[i].register_index;
            if (r >= info->numOutputs)
            for (c = 0; c < pso->output[i].num_components; ++c)
                  }
      bool
   nv50_program_translate(struct nv50_program *prog, uint16_t chipset,
         {
      struct nv50_ir_prog_info *info;
   struct nv50_ir_prog_info_out info_out = {};
   int i, ret;
            info = CALLOC_STRUCT(nv50_ir_prog_info);
   if (!info)
            info->type = prog->type;
                     info->bin.smemSize = prog->cp.smem_size;
   info->io.auxCBSlot = 15;
   info->io.ucpBase = NV50_CB_AUX_UCP_OFFSET;
   info->io.genUserClip = prog->vp.clpd_nr;
   if (prog->fp.alphatest)
            info->io.suInfoBase = NV50_CB_AUX_TEX_MS_OFFSET;
   info->io.bufInfoBase = NV50_CB_AUX_BUF_INFO(0);
   info->io.sampleInfoBase = NV50_CB_AUX_SAMPLE_OFFSET;
   info->io.msInfoCBSlot = 15;
            info->io.membarOffset = NV50_CB_AUX_MEMBAR_OFFSET;
                     prog->vp.bfc[0] = 0xff;
   prog->vp.bfc[1] = 0xff;
   prog->vp.edgeflag = 0xff;
   prog->vp.clpd[0] = map_undef;
   prog->vp.clpd[1] = map_undef;
   prog->vp.psiz = map_undef;
   prog->gp.has_layer = 0;
            if (prog->type == PIPE_SHADER_COMPUTE)
                  #ifndef NDEBUG
      info->optLevel = debug_get_num_option("NV50_PROG_OPTIMIZE", 4);
   info->dbgFlags = debug_get_num_option("NV50_PROG_DEBUG", 0);
      #else
         #endif
         ret = nv50_ir_generate_code(info, &info_out);
   if (ret) {
      NOUVEAU_ERR("shader translation failed: %i\n", ret);
               prog->code = info_out.bin.code;
   prog->code_size = info_out.bin.codeSize;
   prog->relocs = info_out.bin.relocData;
   prog->fixups = info_out.bin.fixupData;
   prog->max_gpr = MAX2(4, (info_out.bin.maxGPR >> 1) + 1);
   prog->tls_space = info_out.bin.tlsSpace;
   prog->cp.smem_size = info_out.bin.smemSize;
   prog->mul_zero_wins = info->io.mul_zero_wins;
            prog->vp.clip_enable = (1 << info_out.io.clipDistances) - 1;
   prog->vp.cull_enable =
         prog->vp.clip_mode = 0;
   for (i = 0; i < info_out.io.cullDistances; ++i)
            if (prog->type == PIPE_SHADER_FRAGMENT) {
      if (info_out.prop.fp.writesDepth) {
      prog->fp.flags[0] |= NV50_3D_FP_CONTROL_EXPORTS_Z;
      }
   if (info_out.prop.fp.usesDiscard)
      } else
   if (prog->type == PIPE_SHADER_GEOMETRY) {
      switch (info_out.prop.gp.outputPrim) {
   case MESA_PRIM_LINE_STRIP:
      prog->gp.prim_type = NV50_3D_GP_OUTPUT_PRIMITIVE_TYPE_LINE_STRIP;
      case MESA_PRIM_TRIANGLE_STRIP:
      prog->gp.prim_type = NV50_3D_GP_OUTPUT_PRIMITIVE_TYPE_TRIANGLE_STRIP;
      case MESA_PRIM_POINTS:
   default:
      assert(info_out.prop.gp.outputPrim == MESA_PRIM_POINTS);
   prog->gp.prim_type = NV50_3D_GP_OUTPUT_PRIMITIVE_TYPE_POINTS;
      }
      } else
   if (prog->type == PIPE_SHADER_COMPUTE) {
      for (i = 0; i < NV50_MAX_GLOBALS; i++) {
      prog->cp.gmem[i] = (struct nv50_gmem_state){
      .valid = info_out.prop.cp.gmem[i].valid,
   .image = info_out.prop.cp.gmem[i].image,
                     if (prog->stream_output.num_outputs)
      prog->so = nv50_program_create_strmout_state(&info_out,
         util_debug_message(debug, SHADER_INFO,
                           out:
      ralloc_free(info->bin.nir);
   FREE(info);
      }
      bool
   nv50_program_upload_code(struct nv50_context *nv50, struct nv50_program *prog)
   {
      struct nouveau_heap *heap;
   int ret;
   uint32_t size = align(prog->code_size, 0x40);
            switch (prog->type) {
   case PIPE_SHADER_VERTEX:   heap = nv50->screen->vp_code_heap; break;
   case PIPE_SHADER_GEOMETRY: heap = nv50->screen->gp_code_heap; break;
   case PIPE_SHADER_FRAGMENT: heap = nv50->screen->fp_code_heap; break;
   case PIPE_SHADER_COMPUTE:  heap = nv50->screen->fp_code_heap; break;
   default:
      assert(!"invalid program type");
               simple_mtx_assert_locked(&nv50->screen->state_lock);
   ret = nouveau_heap_alloc(heap, size, prog, &prog->mem);
   if (ret) {
      /* Out of space: evict everything to compactify the code segment, hoping
   * the working set is much smaller and drifts slowly. Improve me !
   */
   while (heap->next) {
      struct nv50_program *evict = heap->next->priv;
   if (evict)
      }
   debug_printf("WARNING: out of code space, evicting all shaders.\n");
   ret = nouveau_heap_alloc(heap, size, prog, &prog->mem);
   if (ret) {
      NOUVEAU_ERR("shader too large (0x%x) to fit in code space ?\n", size);
                  if (prog->type == PIPE_SHADER_COMPUTE) {
      /* CP code must be uploaded in FP code segment. */
      } else {
      prog->code_base = prog->mem->start;
               ret = nv50_tls_realloc(nv50->screen, prog->tls_space);
   if (ret < 0) {
      nouveau_heap_free(&prog->mem);
      }
   if (ret > 0)
            if (prog->relocs)
         if (prog->fixups)
      nv50_ir_apply_fixups(prog->fixups, prog->code,
                           nv50_sifc_linear_u8(&nv50->base, nv50->screen->code,
                  BEGIN_NV04(nv50->base.pushbuf, NV50_3D(CODE_CB_FLUSH), 1);
               }
      void
   nv50_program_destroy(struct nv50_context *nv50, struct nv50_program *p)
   {
      struct nir_shader *nir = p->nir;
            if (p->mem) {
      if (nv50)
                              FREE(p->relocs);
   FREE(p->fixups);
                     p->nir = nir;
      }
