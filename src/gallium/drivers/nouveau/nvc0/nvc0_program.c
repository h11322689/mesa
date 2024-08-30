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
   #include "compiler/nir/nir_builder.h"
   #include "util/blob.h"
      #include "nvc0/nvc0_context.h"
      #include "nv50_ir_driver.h"
   #include "nvc0/nve4_compute.h"
      /* NOTE: Using a[0x270] in FP may cause an error even if we're using less than
   * 124 scalar varying values.
   */
   static uint32_t
   nvc0_shader_input_address(unsigned sn, unsigned si)
   {
      switch (sn) {
   case TGSI_SEMANTIC_TESSOUTER:    return 0x000 + si * 0x4;
   case TGSI_SEMANTIC_TESSINNER:    return 0x010 + si * 0x4;
   case TGSI_SEMANTIC_PATCH:        return 0x020 + si * 0x10;
   case TGSI_SEMANTIC_PRIMID:       return 0x060;
   case TGSI_SEMANTIC_LAYER:        return 0x064;
   case TGSI_SEMANTIC_VIEWPORT_INDEX:return 0x068;
   case TGSI_SEMANTIC_PSIZE:        return 0x06c;
   case TGSI_SEMANTIC_POSITION:     return 0x070;
   case TGSI_SEMANTIC_GENERIC:      return 0x080 + si * 0x10;
   case TGSI_SEMANTIC_FOG:          return 0x2e8;
   case TGSI_SEMANTIC_COLOR:        return 0x280 + si * 0x10;
   case TGSI_SEMANTIC_BCOLOR:       return 0x2a0 + si * 0x10;
   case TGSI_SEMANTIC_CLIPDIST:     return 0x2c0 + si * 0x10;
   case TGSI_SEMANTIC_CLIPVERTEX:   return 0x270;
   case TGSI_SEMANTIC_PCOORD:       return 0x2e0;
   case TGSI_SEMANTIC_TESSCOORD:    return 0x2f0;
   case TGSI_SEMANTIC_INSTANCEID:   return 0x2f8;
   case TGSI_SEMANTIC_VERTEXID:     return 0x2fc;
   case TGSI_SEMANTIC_TEXCOORD:     return 0x300 + si * 0x10;
   default:
      assert(!"invalid TGSI input semantic");
         }
      static uint32_t
   nvc0_shader_output_address(unsigned sn, unsigned si)
   {
      switch (sn) {
   case TGSI_SEMANTIC_TESSOUTER:     return 0x000 + si * 0x4;
   case TGSI_SEMANTIC_TESSINNER:     return 0x010 + si * 0x4;
   case TGSI_SEMANTIC_PATCH:         return 0x020 + si * 0x10;
   case TGSI_SEMANTIC_PRIMID:        return 0x060;
   case TGSI_SEMANTIC_LAYER:         return 0x064;
   case TGSI_SEMANTIC_VIEWPORT_INDEX:return 0x068;
   case TGSI_SEMANTIC_PSIZE:         return 0x06c;
   case TGSI_SEMANTIC_POSITION:      return 0x070;
   case TGSI_SEMANTIC_GENERIC:       return 0x080 + si * 0x10;
   case TGSI_SEMANTIC_FOG:           return 0x2e8;
   case TGSI_SEMANTIC_COLOR:         return 0x280 + si * 0x10;
   case TGSI_SEMANTIC_BCOLOR:        return 0x2a0 + si * 0x10;
   case TGSI_SEMANTIC_CLIPDIST:      return 0x2c0 + si * 0x10;
   case TGSI_SEMANTIC_CLIPVERTEX:    return 0x270;
   case TGSI_SEMANTIC_TEXCOORD:      return 0x300 + si * 0x10;
   case TGSI_SEMANTIC_VIEWPORT_MASK: return 0x3a0;
   case TGSI_SEMANTIC_EDGEFLAG:      return ~0;
   default:
      assert(!"invalid TGSI output semantic");
         }
      static int
   nvc0_vp_assign_input_slots(struct nv50_ir_prog_info_out *info)
   {
               for (n = 0, i = 0; i < info->numInputs; ++i) {
      switch (info->in[i].sn) {
   case TGSI_SEMANTIC_INSTANCEID: /* for SM4 only, in TGSI they're SVs */
   case TGSI_SEMANTIC_VERTEXID:
      info->in[i].mask = 0x1;
   info->in[i].slot[0] =
            default:
         }
   for (c = 0; c < 4; ++c)
                        }
      static int
   nvc0_sp_assign_input_slots(struct nv50_ir_prog_info_out *info)
   {
      unsigned offset;
            for (i = 0; i < info->numInputs; ++i) {
               for (c = 0; c < 4; ++c)
                  }
      static int
   nvc0_fp_assign_output_slots(struct nv50_ir_prog_info_out *info)
   {
      unsigned count = info->prop.fp.numColourResults * 4;
            /* Compute the relative position of each color output, since skipped MRT
   * positions will not have registers allocated to them.
   */
   unsigned colors[8] = {0};
   for (i = 0; i < info->numOutputs; ++i)
      if (info->out[i].sn == TGSI_SEMANTIC_COLOR)
      for (i = 0, c = 0; i < 8; i++)
      if (colors[i])
      for (i = 0; i < info->numOutputs; ++i)
      if (info->out[i].sn == TGSI_SEMANTIC_COLOR)
               if (info->io.sampleMask < PIPE_MAX_SHADER_OUTPUTS)
         else
   if (info->target >= 0xe0)
            if (info->io.fragDepth < PIPE_MAX_SHADER_OUTPUTS)
               }
      static int
   nvc0_sp_assign_output_slots(struct nv50_ir_prog_info_out *info)
   {
      unsigned offset;
            for (i = 0; i < info->numOutputs; ++i) {
               for (c = 0; c < 4; ++c)
                  }
      static int
   nvc0_program_assign_varying_slots(struct nv50_ir_prog_info_out *info)
   {
               if (info->type == PIPE_SHADER_VERTEX)
         else
         if (ret)
            if (info->type == PIPE_SHADER_FRAGMENT)
         else
            }
      static inline void
   nvc0_vtgp_hdr_update_oread(struct nvc0_program *vp, uint8_t slot)
   {
      uint8_t min = (vp->hdr[4] >> 12) & 0xff;
            min = MIN2(min, slot);
               }
      /* Common part of header generation for VP, TCP, TEP and GP. */
   static int
   nvc0_vtgp_gen_header(struct nvc0_program *vp, struct nv50_ir_prog_info_out *info)
   {
               for (i = 0; i < info->numInputs; ++i) {
      if (info->in[i].patch)
         for (c = 0; c < 4; ++c) {
      a = info->in[i].slot[c];
   if (info->in[i].mask & (1 << c))
                  for (i = 0; i < info->numOutputs; ++i) {
      if (info->out[i].patch)
         for (c = 0; c < 4; ++c) {
      if (!(info->out[i].mask & (1 << c)))
         assert(info->out[i].slot[c] >= 0x40 / 4);
   a = info->out[i].slot[c] - 0x40 / 4;
   vp->hdr[13 + a / 32] |= 1 << (a % 32);
   if (info->out[i].oread)
                  for (i = 0; i < info->numSysVals; ++i) {
      switch (info->sv[i].sn) {
   case SYSTEM_VALUE_PRIMITIVE_ID:
      vp->hdr[5] |= 1 << 24;
      case SYSTEM_VALUE_INSTANCE_ID:
      vp->hdr[10] |= 1 << 30;
      case SYSTEM_VALUE_VERTEX_ID:
      vp->hdr[10] |= 1 << 31;
      case SYSTEM_VALUE_TESS_COORD:
      /* We don't have the mask, nor the slots populated. While this could
   * be achieved, the vast majority of the time if either of the coords
   * are read, then both will be read.
   */
   nvc0_vtgp_hdr_update_oread(vp, 0x2f0 / 4);
   nvc0_vtgp_hdr_update_oread(vp, 0x2f4 / 4);
      default:
                     vp->vp.clip_enable = (1 << info->io.clipDistances) - 1;
   vp->vp.cull_enable =
         for (i = 0; i < info->io.cullDistances; ++i)
            if (info->io.genUserClip < 0)
                        }
      static int
   nvc0_vp_gen_header(struct nvc0_program *vp, struct nv50_ir_prog_info_out *info)
   {
      vp->hdr[0] = 0x20061 | (1 << 10);
               }
      static void
   nvc0_tp_get_tess_mode(struct nvc0_program *tp, struct nv50_ir_prog_info_out *info)
   {
      if (info->prop.tp.outputPrim == MESA_PRIM_COUNT) {
      tp->tp.tess_mode = ~0;
      }
   switch (info->prop.tp.domain) {
   case MESA_PRIM_LINES:
      tp->tp.tess_mode = NVC0_3D_TESS_MODE_PRIM_ISOLINES;
      case MESA_PRIM_TRIANGLES:
      tp->tp.tess_mode = NVC0_3D_TESS_MODE_PRIM_TRIANGLES;
      case MESA_PRIM_QUADS:
      tp->tp.tess_mode = NVC0_3D_TESS_MODE_PRIM_QUADS;
      default:
      tp->tp.tess_mode = ~0;
               /* It seems like lines want the "CW" bit to indicate they're connected, and
   * spit out errors in dmesg when the "CONNECTED" bit is set.
   */
   if (info->prop.tp.outputPrim != MESA_PRIM_POINTS) {
      if (info->prop.tp.domain == MESA_PRIM_LINES)
         else
               /* Winding only matters for triangles/quads, not lines. */
   if (info->prop.tp.domain != MESA_PRIM_LINES &&
      info->prop.tp.outputPrim != MESA_PRIM_POINTS &&
   info->prop.tp.winding > 0)
         switch (info->prop.tp.partitioning) {
   case PIPE_TESS_SPACING_EQUAL:
      tp->tp.tess_mode |= NVC0_3D_TESS_MODE_SPACING_EQUAL;
      case PIPE_TESS_SPACING_FRACTIONAL_ODD:
      tp->tp.tess_mode |= NVC0_3D_TESS_MODE_SPACING_FRACTIONAL_ODD;
      case PIPE_TESS_SPACING_FRACTIONAL_EVEN:
      tp->tp.tess_mode |= NVC0_3D_TESS_MODE_SPACING_FRACTIONAL_EVEN;
      default:
      assert(!"invalid tessellator partitioning");
         }
      static int
   nvc0_tcp_gen_header(struct nvc0_program *tcp, struct nv50_ir_prog_info_out *info)
   {
               if (info->numPatchConstants)
                     tcp->hdr[1] = opcs << 24;
                              if (info->target >= NVISA_GM107_CHIPSET) {
      /* On GM107+, the number of output patch components has moved in the TCP
   * header, but it seems like blob still also uses the old position.
   * Also, the high 8-bits are located in between the min/max parallel
   * field and has to be set after updating the outputs. */
   tcp->hdr[3] = (opcs & 0x0f) << 28;
                           }
      static int
   nvc0_tep_gen_header(struct nvc0_program *tep, struct nv50_ir_prog_info_out *info)
   {
      tep->hdr[0] = 0x20061 | (3 << 10);
                                          }
      static int
   nvc0_gp_gen_header(struct nvc0_program *gp, struct nv50_ir_prog_info_out *info)
   {
                        switch (info->prop.gp.outputPrim) {
   case MESA_PRIM_POINTS:
      gp->hdr[3] = 0x01000000;
   gp->hdr[0] |= 0xf0000000;
      case MESA_PRIM_LINE_STRIP:
      gp->hdr[3] = 0x06000000;
   gp->hdr[0] |= 0x10000000;
      case MESA_PRIM_TRIANGLE_STRIP:
      gp->hdr[3] = 0x07000000;
   gp->hdr[0] |= 0x10000000;
      default:
      assert(0);
                           }
      #define NVC0_INTERP_FLAT          (1 << 0)
   #define NVC0_INTERP_PERSPECTIVE   (2 << 0)
   #define NVC0_INTERP_LINEAR        (3 << 0)
   #define NVC0_INTERP_CENTROID      (1 << 2)
      static uint8_t
   nvc0_hdr_interp_mode(const struct nv50_ir_varying *var)
   {
      if (var->linear)
         if (var->flat)
            }
      static int
   nvc0_fp_gen_header(struct nvc0_program *fp, struct nv50_ir_prog_info_out *info)
   {
               /* just 00062 on Kepler */
   fp->hdr[0] = 0x20062 | (5 << 10);
            if (info->prop.fp.usesDiscard)
         if (!info->prop.fp.separateFragData)
         if (info->io.sampleMask < PIPE_MAX_SHADER_OUTPUTS)
         if (info->prop.fp.writesDepth) {
      fp->hdr[19] |= 0x2;
               for (i = 0; i < info->numInputs; ++i) {
      m = nvc0_hdr_interp_mode(&info->in[i]);
   if (info->in[i].sn == TGSI_SEMANTIC_COLOR) {
      fp->fp.colors |= 1 << info->in[i].si;
   if (info->in[i].sc)
      }
   for (c = 0; c < 4; ++c) {
      if (!(info->in[i].mask & (1 << c)))
         a = info->in[i].slot[c];
   if (info->in[i].slot[0] >= (0x060 / 4) &&
      info->in[i].slot[0] <= (0x07c / 4)) {
      } else
   if (info->in[i].slot[0] >= (0x2c0 / 4) &&
      info->in[i].slot[0] <= (0x2fc / 4)) {
      } else {
      if (info->in[i].slot[c] < (0x040 / 4) ||
      info->in[i].slot[c] > (0x380 / 4))
      a *= 2;
   if (info->in[i].slot[0] >= (0x300 / 4))
                  }
   /* GM20x+ needs TGSI_SEMANTIC_POSITION to access sample locations */
   if (info->prop.fp.readsSampleLocations && info->target >= NVISA_GM200_CHIPSET)
            for (i = 0; i < info->numOutputs; ++i) {
      if (info->out[i].sn == TGSI_SEMANTIC_COLOR)
               /* There are no "regular" attachments, but the shader still needs to be
   * executed. It seems like it wants to think that it has some color
   * outputs in order to actually run.
   */
   if (info->prop.fp.numColourResults == 0 && !info->prop.fp.writesDepth)
            fp->fp.early_z = info->prop.fp.earlyFragTests;
   fp->fp.sample_mask_in = info->prop.fp.usesSampleMaskIn;
   fp->fp.reads_framebuffer = info->prop.fp.readsFramebuffer;
            /* Mark position xy and layer as read */
   if (fp->fp.reads_framebuffer)
               }
      static struct nvc0_transform_feedback_state *
   nvc0_program_create_tfb_state(const struct nv50_ir_prog_info_out *info,
         {
      struct nvc0_transform_feedback_state *tfb;
            tfb = MALLOC_STRUCT(nvc0_transform_feedback_state);
   if (!tfb)
         for (b = 0; b < 4; ++b) {
      tfb->stride[b] = pso->stride[b] * 4;
      }
            for (i = 0; i < pso->num_outputs; ++i) {
      unsigned s = pso->output[i].start_component;
   unsigned p = pso->output[i].dst_offset;
   const unsigned r = pso->output[i].register_index;
            if (r >= info->numOutputs)
            for (c = 0; c < pso->output[i].num_components; ++c)
            tfb->varying_count[b] = MAX2(tfb->varying_count[b], p);
      }
   for (b = 0; b < 4; ++b) // zero unused indices (looks nicer)
      for (c = tfb->varying_count[b]; c & 3; ++c)
            }
      #ifndef NDEBUG
   static void
   nvc0_program_dump(struct nvc0_program *prog)
   {
               if (prog->type != PIPE_SHADER_COMPUTE) {
      _debug_printf("dumping HDR for type %i\n", prog->type);
   for (pos = 0; pos < ARRAY_SIZE(prog->hdr); ++pos)
      _debug_printf("HDR[%02"PRIxPTR"] = 0x%08x\n",
   }
   _debug_printf("shader binary code (0x%x bytes):", prog->code_size);
   for (pos = 0; pos < prog->code_size / 4; ++pos) {
      if ((pos % 8) == 0)
            }
      }
   #endif
      bool
   nvc0_program_translate(struct nvc0_program *prog, uint16_t chipset,
               {
      struct blob blob;
   size_t cache_size;
   struct nv50_ir_prog_info *info;
            int ret = 0;
   cache_key key;
            info = CALLOC_STRUCT(nv50_ir_prog_info);
   if (!info)
            info->type = prog->type;
                  #ifndef NDEBUG
      info->target = debug_get_num_option("NV50_PROG_CHIPSET", chipset);
   info->optLevel = debug_get_num_option("NV50_PROG_OPTIMIZE", 4);
   info->dbgFlags = debug_get_num_option("NV50_PROG_DEBUG", 0);
      #else
         #endif
         info->bin.smemSize = prog->cp.smem_size;
   info->io.genUserClip = prog->vp.num_ucps;
   info->io.auxCBSlot = 15;
   info->io.msInfoCBSlot = 15;
   info->io.ucpBase = NVC0_CB_AUX_UCP_INFO;
   info->io.drawInfoBase = NVC0_CB_AUX_DRAW_INFO;
   info->io.msInfoBase = NVC0_CB_AUX_MS_INFO;
   info->io.bufInfoBase = NVC0_CB_AUX_BUF_INFO(0);
   info->io.suInfoBase = NVC0_CB_AUX_SU_INFO(0);
   if (info->target >= NVISA_GK104_CHIPSET) {
      info->io.texBindBase = NVC0_CB_AUX_TEX_INFO(0);
   info->io.fbtexBindBase = NVC0_CB_AUX_FB_TEX_INFO;
               if (prog->type == PIPE_SHADER_COMPUTE) {
      if (info->target >= NVISA_GK104_CHIPSET) {
      info->io.auxCBSlot = 7;
   info->io.msInfoCBSlot = 7;
      }
      } else {
                                    if (disk_shader_cache) {
      if (nv50_ir_prog_info_serialize(&blob, info)) {
                              if (cached_data && cache_size >= blob.size) { // blob.size is the size of serialized "info"
      /* Blob contains only "info". In disk cache, "info_out" comes right after it */
   size_t offset = blob.size;
   if (nv50_ir_prog_info_out_deserialize(cached_data, cache_size, offset, &info_out))
         else
      }
      } else {
            }
   if (!shader_loaded) {
      cache_size = 0;
   ret = nv50_ir_generate_code(info, &info_out);
   if (ret) {
      NOUVEAU_ERR("shader translation failed: %i\n", ret);
      }
   if (disk_shader_cache) {
      if (nv50_ir_prog_info_out_serialize(&blob, &info_out)) {
      disk_cache_put(disk_shader_cache, key, blob.data, blob.size, NULL);
      } else {
               }
            prog->code = info_out.bin.code;
   prog->code_size = info_out.bin.codeSize;
   prog->relocs = info_out.bin.relocData;
   prog->fixups = info_out.bin.fixupData;
   if (info_out.target >= NVISA_GV100_CHIPSET)
         else
         prog->cp.smem_size = info_out.bin.smemSize;
            prog->vp.need_vertex_id = info_out.io.vertexId < PIPE_MAX_SHADER_INPUTS;
            if (info_out.io.edgeFlagOut < PIPE_MAX_ATTRIBS)
                  switch (prog->type) {
   case PIPE_SHADER_VERTEX:
      ret = nvc0_vp_gen_header(prog, &info_out);
      case PIPE_SHADER_TESS_CTRL:
      ret = nvc0_tcp_gen_header(prog, &info_out);
      case PIPE_SHADER_TESS_EVAL:
      ret = nvc0_tep_gen_header(prog, &info_out);
      case PIPE_SHADER_GEOMETRY:
      ret = nvc0_gp_gen_header(prog, &info_out);
      case PIPE_SHADER_FRAGMENT:
      ret = nvc0_fp_gen_header(prog, &info_out);
      case PIPE_SHADER_COMPUTE:
         default:
      ret = -1;
   NOUVEAU_ERR("unknown program type: %u\n", prog->type);
      }
   if (ret)
            if (info_out.bin.tlsSpace) {
      assert(info_out.bin.tlsSpace < (1 << 24));
   prog->hdr[0] |= 1 << 26;
   prog->hdr[1] |= align(info_out.bin.tlsSpace, 0x10); /* l[] size */
      }
   /* TODO: factor 2 only needed where joinat/precont is used,
   *       and we only have to count non-uniform branches
   */
   /*
   if ((info->maxCFDepth * 2) > 16) {
      prog->hdr[2] |= (((info->maxCFDepth * 2) + 47) / 48) * 0x200;
      }
   */
   if (info_out.io.globalAccess)
         if (info_out.io.globalAccess & 0x2)
         if (info_out.io.fp64)
            if (prog->stream_output.num_outputs)
      prog->tfb = nvc0_program_create_tfb_state(&info_out,
         util_debug_message(debug, SHADER_INFO,
                           #ifndef NDEBUG
      if (debug_get_option("NV50_PROG_CHIPSET", NULL) && info->dbgFlags)
      #endif
      out:
      ralloc_free((void *)info->bin.nir);
   FREE(info);
      }
      static inline int
   nvc0_program_alloc_code(struct nvc0_context *nvc0, struct nvc0_program *prog)
   {
      struct nvc0_screen *screen = nvc0->screen;
   const bool is_cp = prog->type == PIPE_SHADER_COMPUTE;
   int ret;
            if (!is_cp) {
      if (screen->eng3d->oclass < TU102_3D_CLASS)
         else
               /* On Fermi, SP_START_ID must be aligned to 0x40.
   * On Kepler, the first instruction must be aligned to 0x80 because
   * latency information is expected only at certain positions.
   */
   if (screen->base.class_3d >= NVE4_3D_CLASS)
                  ret = nouveau_heap_alloc(screen->text_heap, size, prog, &prog->mem);
   if (ret)
                  if (!is_cp) {
      if (screen->base.class_3d >= NVE4_3D_CLASS &&
      screen->base.class_3d < TU102_3D_CLASS) {
   switch (prog->mem->start & 0xff) {
   case 0x40: prog->code_base += 0x70; break;
   case 0x80: prog->code_base += 0x30; break;
   case 0xc0: prog->code_base += 0x70; break;
   default:
      prog->code_base += 0x30;
   assert((prog->mem->start & 0xff) == 0x00);
            } else {
      if (screen->base.class_3d >= NVE4_3D_CLASS) {
      if (prog->mem->start & 0x40)
                           }
      static inline void
   nvc0_program_upload_code(struct nvc0_context *nvc0, struct nvc0_program *prog)
   {
      struct nvc0_screen *screen = nvc0->screen;
   const bool is_cp = prog->type == PIPE_SHADER_COMPUTE;
   uint32_t code_pos = prog->code_base;
            if (!is_cp) {
      if (screen->eng3d->oclass < TU102_3D_CLASS)
         else
      }
            if (prog->relocs)
      nv50_ir_relocate_code(prog->relocs, prog->code, code_pos,
      if (prog->fixups) {
      nv50_ir_apply_fixups(prog->fixups, prog->code,
                           for (int i = 0; i < 2; i++) {
      unsigned mask = prog->fp.color_interp[i] >> 4;
   unsigned interp = prog->fp.color_interp[i] & 3;
   if (!mask)
         prog->hdr[14] &= ~(0xff << (8 * i));
   if (prog->fp.flatshade)
         for (int c = 0; c < 4; c++)
      if (mask & (1 << c))
               if (!is_cp)
      nvc0->base.push_data(&nvc0->base, screen->text, prog->code_base,
         nvc0->base.push_data(&nvc0->base, screen->text, code_pos,
            }
      bool
   nvc0_program_upload(struct nvc0_context *nvc0, struct nvc0_program *prog)
   {
      struct nvc0_screen *screen = nvc0->screen;
   const bool is_cp = prog->type == PIPE_SHADER_COMPUTE;
   int ret;
            if (!is_cp) {
      if (screen->eng3d->oclass < TU102_3D_CLASS)
         else
               simple_mtx_assert_locked(&nvc0->screen->state_lock);
   ret = nvc0_program_alloc_code(nvc0, prog);
   if (ret) {
      struct nouveau_heap *heap = screen->text_heap;
   struct nvc0_program *progs[] = { /* Sorted accordingly to SP_START_ID */
      nvc0->compprog, nvc0->vertprog, nvc0->tctlprog,
               /* Note that the code library, which is allocated before anything else,
   * does not have a priv pointer. We can stop once we hit it.
   */
   while (heap->next && heap->next->priv) {
      struct nvc0_program *evict = heap->next->priv;
      }
            /* Make sure to synchronize before deleting the code segment. */
            if ((screen->text->size << 1) <= (1 << 23)) {
      ret = nvc0_screen_resize_text_area(screen, nvc0->base.pushbuf, screen->text->size << 1);
   if (ret) {
      NOUVEAU_ERR("Error allocating TEXT area: %d\n", ret);
               /* Re-upload the builtin function into the new code segment. */
               ret = nvc0_program_alloc_code(nvc0, prog);
   if (ret) {
      NOUVEAU_ERR("shader too large (0x%x) to fit in code space ?\n", size);
               /* All currently bound shaders have to be reuploaded. */
   for (int i = 0; i < ARRAY_SIZE(progs); i++) {
                     ret = nvc0_program_alloc_code(nvc0, progs[i]);
   if (ret) {
      NOUVEAU_ERR("failed to re-upload a shader after code eviction.\n");
                     if (progs[i]->type == PIPE_SHADER_COMPUTE) {
      /* Caches have to be invalidated but the CP_START_ID will be
   * updated in the launch_grid functions. */
   BEGIN_NVC0(nvc0->base.pushbuf, NVC0_CP(FLUSH), 1);
      } else {
                              #ifndef NDEBUG
      if (debug_get_num_option("NV50_PROG_DEBUG", 0))
      #endif
         BEGIN_NVC0(nvc0->base.pushbuf, NVC0_3D(MEM_BARRIER), 1);
               }
      /* Upload code for builtin functions like integer division emulation. */
   void
   nvc0_program_library_upload(struct nvc0_context *nvc0)
   {
      struct nvc0_screen *screen = nvc0->screen;
   int ret;
   uint32_t size;
            if (screen->lib_code)
            nv50_ir_get_target_library(screen->base.device->chipset, &code, &size);
   if (!size)
            ret = nouveau_heap_alloc(screen->text_heap, align(size, 0x100), NULL,
         if (ret)
            nvc0->base.push_data(&nvc0->base,
                  }
      void
   nvc0_program_destroy(struct nvc0_context *nvc0, struct nvc0_program *prog)
   {
      struct nir_shader *nir = prog->nir;
            if (prog->mem) {
      if (nvc0)
            }
   FREE(prog->code); /* may be 0 for hardcoded shaders */
   FREE(prog->relocs);
   FREE(prog->fixups);
   if (prog->tfb) {
      if (nvc0->state.tfb == prog->tfb)
                              prog->nir = nir;
      }
      void
   nvc0_program_init_tcp_empty(struct nvc0_context *nvc0)
   {
      const nir_shader_compiler_options *options =
      nv50_ir_nir_shader_compiler_options(nvc0->screen->base.device->chipset,
         struct nir_builder b =
      nir_builder_init_simple_shader(MESA_SHADER_TESS_CTRL, options,
                        struct pipe_shader_state state;
   pipe_shader_state_from_nir(&state, b.shader);
      }
