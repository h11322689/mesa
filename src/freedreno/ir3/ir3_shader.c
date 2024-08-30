   /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
      #include "util/format/u_format.h"
   #include "util/u_atomic.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "drm/freedreno_drmif.h"
      #include "ir3_assembler.h"
   #include "ir3_compiler.h"
   #include "ir3_nir.h"
   #include "ir3_parser.h"
   #include "ir3_shader.h"
      #include "isa/isa.h"
      #include "disasm.h"
      int
   ir3_glsl_type_size(const struct glsl_type *type, bool bindless)
   {
         }
      /* wrapper for ir3_assemble() which does some info fixup based on
   * shader state.  Non-static since used by ir3_cmdline too.
   */
   void *
   ir3_shader_assemble(struct ir3_shader_variant *v)
   {
      const struct ir3_compiler *compiler = v->compiler;
   struct ir3_info *info = &v->info;
                     if (v->constant_data_size) {
      /* Make sure that where we're about to place the constant_data is safe
   * to indirectly upload from.
   */
   info->constant_data_offset =
                     /* Pad out the size so that when turnip uploads the shaders in
   * sequence, the starting offset of the next one is properly aligned.
   */
            bin = isa_assemble(v);
   if (!bin)
            /* Append the immediates after the end of the program.  This lets us emit
   * the immediates as an indirect load, while avoiding creating another BO.
   */
   if (v->constant_data_size)
      memcpy(&bin[info->constant_data_offset / 4], v->constant_data,
      ralloc_free(v->constant_data);
            /* NOTE: if relative addressing is used, we set constlen in
   * the compiler (to worst-case value) since we don't know in
   * the assembler what the max addr reg value can be:
   */
            if (v->constlen > ir3_const_state(v)->offsets.driver_param)
            /* On a4xx and newer, constlen must be a multiple of 16 dwords even though
   * uploads are in units of 4 dwords. Round it up here to make calculations
   * regarding the shared constlen simpler.
   */
   if (compiler->gen >= 4)
            /* Use the per-wave layout by default on a6xx for compute shaders. It
   * should result in better performance when loads/stores are to a uniform
   * index.
   */
   v->pvtmem_per_wave = compiler->gen >= 6 && !info->multi_dword_ldp_stp &&
                     }
      static bool
   try_override_shader_variant(struct ir3_shader_variant *v,
         {
               char *name =
                     if (!f) {
      ralloc_free(name);
               struct ir3_kernel_info info;
   info.numwg = INVALID_REG;
                     if (!v->ir) {
      fprintf(stderr, "Failed to parse %s\n", name);
               v->bin = ir3_shader_assemble(v);
   if (!v->bin) {
      fprintf(stderr, "Failed to assemble %s\n", name);
               ralloc_free(name);
      }
      static void
   assemble_variant(struct ir3_shader_variant *v, bool internal)
   {
               bool dbg_enabled = shader_debug_enabled(v->type, internal);
   if (dbg_enabled || ir3_shader_override_path || v->disasm_info.write_disasm) {
      unsigned char sha1[21];
            _mesa_sha1_compute(v->bin, v->info.size, sha1);
            bool shader_overridden =
            if (v->disasm_info.write_disasm) {
      char *stream_data = NULL;
                  fprintf(stream,
         "Native code%s for unnamed %s shader %s with sha1 %s:\n",
                           v->disasm_info.disasm = ralloc_size(v, stream_size + 1);
   memcpy(v->disasm_info.disasm, stream_data, stream_size);
   v->disasm_info.disasm[stream_size] = 0;
               if (dbg_enabled || shader_overridden) {
      char *stream_data = NULL;
                  fprintf(stream,
         "Native code%s for unnamed %s shader %s with sha1 %s:\n",
   shader_overridden ? " (overridden)" : "", ir3_shader_stage(v),
   if (v->type == MESA_SHADER_FRAGMENT)
                        mesa_log_multiline(MESA_LOG_INFO, stream_data);
                  /* no need to keep the ir around beyond this point: */
   ir3_destroy(v->ir);
      }
      static bool
   compile_variant(struct ir3_shader *shader, struct ir3_shader_variant *v)
   {
      int ret = ir3_compile_shader_nir(shader->compiler, shader, v);
   if (ret) {
      mesa_loge("compile failed! (%s:%s)", shader->nir->info.name,
                     assemble_variant(v, shader->nir->info.internal);
   if (!v->bin) {
      mesa_loge("assemble failed! (%s:%s)", shader->nir->info.name,
                        }
      /*
   * For creating normal shader variants, 'nonbinning' is NULL.  For
   * creating binning pass shader, it is link to corresponding normal
   * (non-binning) variant.
   */
   static struct ir3_shader_variant *
   alloc_variant(struct ir3_shader *shader, const struct ir3_shader_key *key,
         {
      /* hang the binning variant off it's non-binning counterpart instead
   * of the shader, to simplify the error cleanup paths
   */
   if (nonbinning)
                  if (!v)
            v->id = ++shader->variant_count;
   v->shader_id = shader->id;
   v->binning_pass = !!nonbinning;
   v->nonbinning = nonbinning;
   v->key = *key;
   v->type = shader->type;
   v->compiler = shader->compiler;
   v->mergedregs = shader->compiler->gen >= 6;
                     struct shader_info *info = &shader->nir->info;
   switch (v->type) {
   case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
      v->tess.primitive_mode = info->tess._primitive_mode;
   v->tess.tcs_vertices_out = info->tess.tcs_vertices_out;
   v->tess.spacing = info->tess.spacing;
   v->tess.ccw = info->tess.ccw;
   v->tess.point_mode = info->tess.point_mode;
         case MESA_SHADER_GEOMETRY:
      v->gs.output_primitive = info->gs.output_primitive;
   v->gs.vertices_out = info->gs.vertices_out;
   v->gs.invocations = info->gs.invocations;
   v->gs.vertices_in = info->gs.vertices_in;
         case MESA_SHADER_FRAGMENT:
      v->fs.early_fragment_tests = info->fs.early_fragment_tests;
   v->fs.color_is_dual_source = info->fs.color_is_dual_source;
   v->fs.uses_fbfetch_output  = info->fs.uses_fbfetch_output;
   v->fs.fbfetch_coherent     = info->fs.fbfetch_coherent;
         case MESA_SHADER_COMPUTE:
   case MESA_SHADER_KERNEL:
      v->cs.req_input_mem = shader->cs.req_input_mem;
   v->cs.req_local_mem = shader->cs.req_local_mem;
         default:
                  v->num_ssbos = info->num_ssbos;
   v->num_ibos = info->num_ssbos + info->num_images;
            if (!v->binning_pass) {
      v->const_state = rzalloc_size(v, sizeof(*v->const_state));
                  }
      static bool
   needs_binning_variant(struct ir3_shader_variant *v)
   {
      if ((v->type == MESA_SHADER_VERTEX) && ir3_has_binning_vs(&v->key))
            }
      static struct ir3_shader_variant *
   create_variant(struct ir3_shader *shader, const struct ir3_shader_key *key,
         {
               if (!v)
                     if (needs_binning_variant(v)) {
      v->binning = alloc_variant(shader, key, v, mem_ctx);
   if (!v->binning)
                     if (ir3_disk_cache_retrieve(shader, v))
            if (!shader->nir_finalized) {
               if (ir3_shader_debug & IR3_DBG_DISASM) {
      mesa_logi("dump nir%d: type=%d", shader->id, shader->type);
               if (v->disasm_info.write_disasm) {
                              if (!compile_variant(shader, v))
            if (needs_binning_variant(v) && !compile_variant(shader, v->binning))
                           fail:
      ralloc_free(v);
      }
      struct ir3_shader_variant *
   ir3_shader_create_variant(struct ir3_shader *shader,
               {
         }
      static inline struct ir3_shader_variant *
   shader_variant(struct ir3_shader *shader, const struct ir3_shader_key *key)
   {
               for (v = shader->variants; v; v = v->next)
      if (ir3_shader_key_equal(key, &v->key))
            }
      struct ir3_shader_variant *
   ir3_shader_get_variant(struct ir3_shader *shader,
               {
               mtx_lock(&shader->variants_lock);
            if (!v) {
      /* compile new variant if it doesn't exist already: */
   v = create_variant(shader, key, write_disasm, shader);
   if (v) {
      v->next = shader->variants;
   shader->variants = v;
                  if (v && binning_pass) {
      v = v->binning;
                           }
      struct ir3_shader *
   ir3_shader_passthrough_tcs(struct ir3_shader *vs, unsigned patch_vertices)
   {
      assert(vs->type == MESA_SHADER_VERTEX);
   assert(patch_vertices > 0);
            unsigned n = patch_vertices - 1;
   if (!vs->vs.passthrough_tcs[n]) {
      const nir_shader_compiler_options *options =
         nir_shader *tcs =
            /* Technically it is an internal shader but it is confusing to
   * not have it show up in debug output
   */
            nir_assign_io_var_locations(tcs, nir_var_shader_in,
                  nir_assign_io_var_locations(tcs, nir_var_shader_out,
                                                      vs->vs.passthrough_tcs[n] =
                           }
      void
   ir3_shader_destroy(struct ir3_shader *shader)
   {
      if (shader->type == MESA_SHADER_VERTEX) {
      u_foreach_bit (b, shader->vs.passthrough_tcs_compiled) {
            }
   ralloc_free(shader->nir);
   mtx_destroy(&shader->variants_lock);
      }
      /**
   * Creates a bitmask of the used bits of the shader key by this particular
   * shader.  Used by the gallium driver to skip state-dependent recompiles when
   * possible.
   */
   static void
   ir3_setup_used_key(struct ir3_shader *shader)
   {
      nir_shader *nir = shader->nir;
   struct shader_info *info = &nir->info;
            /* This key flag is just used to make for a cheaper ir3_shader_key_equal
   * check in the common case.
   */
                     /* When clip/cull distances are natively supported, we only use
   * ucp_enables to determine whether to lower legacy clip planes to
   * gl_ClipDistance.
   */
   if (info->stage != MESA_SHADER_COMPUTE && (info->stage != MESA_SHADER_FRAGMENT || !shader->compiler->has_clip_cull))
            if (info->stage == MESA_SHADER_FRAGMENT) {
      key->fastc_srgb = ~0;
   key->fsamples = ~0;
            if (info->inputs_read & VARYING_BITS_COLOR) {
                  /* Only used for deciding on behavior of
   * nir_intrinsic_load_barycentric_sample and the centroid demotion
   * on older HW.
   */
   key->msaa = shader->compiler->gen < 6 &&
               (info->fs.uses_sample_qualifier ||
   (BITSET_TEST(info->system_values_read,
      } else if (info->stage == MESA_SHADER_COMPUTE) {
      key->fastc_srgb = ~0;
   key->fsamples = ~0;
      } else {
      key->tessellation = ~0;
            if (info->stage == MESA_SHADER_VERTEX) {
      key->vastc_srgb = ~0;
   key->vsamples = ~0;
               if (info->stage == MESA_SHADER_TESS_CTRL)
         }
      /* Given an array of constlen's, decrease some of them so that the sum stays
   * within "combined_limit" while trying to fairly share the reduction. Returns
   * a bitfield of which stages should be trimmed.
   */
   static uint32_t
   trim_constlens(unsigned *constlens, unsigned first_stage, unsigned last_stage,
         {
      unsigned cur_total = 0;
   for (unsigned i = first_stage; i <= last_stage; i++) {
                  unsigned max_stage = 0;
   unsigned max_const = 0;
            while (cur_total > combined_limit) {
      for (unsigned i = first_stage; i <= last_stage; i++) {
      if (constlens[i] >= max_const) {
      max_stage = i;
                  assert(max_const > safe_limit);
   trimmed |= 1 << max_stage;
   cur_total = cur_total - max_const + safe_limit;
                  }
      /* Figures out which stages in the pipeline to use the "safe" constlen for, in
   * order to satisfy all shared constlen limits.
   */
   uint32_t
   ir3_trim_constlen(const struct ir3_shader_variant **variants,
         {
                        for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (variants[i]) {
      constlens[i] = variants[i]->constlen;
   shared_consts_enable =
                  uint32_t trimmed = 0;
            /* Use a hw quirk for geometry shared consts, not matched with actual
   * shared consts size (on a6xx).
   */
   uint32_t shared_consts_size_geom = shared_consts_enable ?
            uint32_t shared_consts_size = shared_consts_enable ?
            uint32_t safe_shared_consts_size = shared_consts_enable  ?
      ALIGN_POT(MAX2(DIV_ROUND_UP(shared_consts_size_geom, 4),
         /* There are two shared limits to take into account, the geometry limit on
   * a6xx and the total limit. The frag limit on a6xx only matters for a
   * single stage, so it's always satisfied with the first variant.
   */
   if (compiler->gen >= 6) {
      trimmed |=
      trim_constlens(constlens, MESA_SHADER_VERTEX, MESA_SHADER_GEOMETRY,
         }
   trimmed |=
      trim_constlens(constlens, MESA_SHADER_VERTEX, MESA_SHADER_FRAGMENT,
                  }
      struct ir3_shader *
   ir3_shader_from_nir(struct ir3_compiler *compiler, nir_shader *nir,
               {
               mtx_init(&shader->variants_lock, mtx_plain);
   shader->compiler = compiler;
   shader->id = p_atomic_inc_return(&shader->compiler->shader_count);
   shader->type = nir->info.stage;
   if (stream_output)
      memcpy(&shader->stream_output, stream_output,
      shader->options = *options;
                                 }
      static void
   dump_reg(FILE *out, const char *name, uint32_t r)
   {
      if (r != regid(63, 0)) {
      const char *reg_type = (r & HALF_REG_ID) ? "hr" : "r";
   fprintf(out, "; %s: %s%d.%c\n", name, reg_type, (r & ~HALF_REG_ID) >> 2,
         }
      static void
   dump_output(FILE *out, struct ir3_shader_variant *so, unsigned slot,
         {
      uint32_t regid;
   regid = ir3_find_output_regid(so, slot);
      }
      static const char *
   input_name(struct ir3_shader_variant *so, int i)
   {
      if (so->inputs[i].sysval) {
         } else if (so->type == MESA_SHADER_VERTEX) {
         } else {
            }
      static const char *
   output_name(struct ir3_shader_variant *so, int i)
   {
      if (so->type == MESA_SHADER_FRAGMENT) {
         } else {
      switch (so->outputs[i].slot) {
   case VARYING_SLOT_GS_HEADER_IR3:
         case VARYING_SLOT_GS_VERTEX_FLAGS_IR3:
         case VARYING_SLOT_TCS_HEADER_IR3:
         default:
               }
      static void
   dump_const_state(struct ir3_shader_variant *so, FILE *out)
   {
      const struct ir3_const_state *cs = ir3_const_state(so);
            fprintf(out, "; num_ubos:           %u\n", cs->num_ubos);
   fprintf(out, "; num_driver_params:  %u\n", cs->num_driver_params);
   fprintf(out, "; offsets:\n");
   if (cs->offsets.ubo != ~0)
         if (cs->offsets.image_dims != ~0)
         if (cs->offsets.kernel_params != ~0)
         if (cs->offsets.driver_param != ~0)
         if (cs->offsets.tfbo != ~0)
         if (cs->offsets.primitive_param != ~0)
         if (cs->offsets.primitive_map != ~0)
         fprintf(out, "; ubo_state:\n");
   fprintf(out, ";   num_enabled:      %u\n", us->num_enabled);
   for (unsigned i = 0; i < us->num_enabled; i++) {
                        fprintf(out, ";   range[%u]:\n", i);
   fprintf(out, ";     block:          %u\n", r->ubo.block);
   if (r->ubo.bindless)
                  unsigned size = r->end - r->start;
                  }
      static uint8_t
   find_input_reg_id(struct ir3_shader_variant *so, uint32_t input_idx)
   {
      uint8_t reg = so->inputs[input_idx].regid;
   if (so->type != MESA_SHADER_FRAGMENT || !so->ir || VALIDREG(reg))
                     /* In FS we don't know into which register the input is loaded
   * until the shader is scanned for the input load instructions.
   */
   foreach_block (block, &so->ir->block_list) {
      foreach_instr_safe (instr, &block->instr_list) {
      if (instr->opc == OPC_FLAT_B || instr->opc == OPC_BARY_F ||
      instr->opc == OPC_LDLV) {
   if (instr->srcs[0]->flags & IR3_REG_IMMED) {
                                                                     if (instr->dsts[0]->flags & IR3_REG_EI) {
                                 }
      void
   print_raw(FILE *out, const BITSET_WORD *data, size_t size) {
         }
      void
   ir3_shader_disasm(struct ir3_shader_variant *so, uint32_t *bin, FILE *out)
   {
      struct ir3 *ir = so->ir;
   struct ir3_register *reg;
   const char *type = ir3_shader_stage(so);
   uint8_t regid;
                     foreach_input_n (instr, i, ir) {
      reg = instr->dsts[0];
   regid = reg->num;
   fprintf(out, "@in(%sr%d.%c)\tin%d",
                  if (reg->wrmask > 0x1)
                     /* print pre-dispatch texture fetches: */
   for (i = 0; i < so->num_sampler_prefetch; i++) {
      const struct ir3_sampler_prefetch *fetch = &so->sampler_prefetch[i];
   fprintf(out,
         "@tex(%sr%d.%c)\tsrc=%u, samp=%u, tex=%u, wrmask=0x%x, opc=%s\n",
   fetch->half_precision ? "h" : "", fetch->dst >> 2,
   "xyzw"[fetch->dst & 0x3], fetch -> src, fetch -> samp_id,
               const struct ir3_const_state *const_state = ir3_const_state(so);
   for (i = 0; i < DIV_ROUND_UP(const_state->immediates_count, 4); i++) {
      fprintf(out, "@const(c%d.x)\t", const_state->offsets.immediate + i);
   fprintf(out, "0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
         const_state->immediates[i * 4 + 0],
   const_state->immediates[i * 4 + 1],
               isa_disasm(bin, so->info.sizedwords * 4, out,
            &(struct isa_decode_options){
      .gpu_id = ir->compiler->gen * 100,
   .show_errors = true,
            fprintf(out, "; %s: outputs:", type);
   for (i = 0; i < so->outputs_count; i++) {
      uint8_t regid = so->outputs[i].regid;
   const char *reg_type = so->outputs[i].half ? "hr" : "r";
   fprintf(out, " %s%d.%c (%s)", reg_type, (regid >> 2), "xyzw"[regid & 0x3],
      }
            fprintf(out, "; %s: inputs:", type);
   for (i = 0; i < so->inputs_count; i++) {
               fprintf(out, " r%d.%c (%s slot=%d cm=%x,il=%u,b=%u)", (regid >> 2),
            }
            /* print generic shader info: */
   fprintf(
      out,
   "; %s prog %d/%d: %u instr, %u nops, %u non-nops, %u mov, %u cov, %u dwords\n",
   type, so->shader_id, so->id, so->info.instrs_count, so->info.nops_count,
   so->info.instrs_count - so->info.nops_count, so->info.mov_count,
         fprintf(out,
         "; %s prog %d/%d: %u last-baryf, %u last-helper, %d half, %d full, %u constlen\n",
   type, so->shader_id, so->id, so->info.last_baryf,
            fprintf(
      out,
   "; %s prog %d/%d: %u cat0, %u cat1, %u cat2, %u cat3, %u cat4, %u cat5, %u cat6, %u cat7, \n",
   type, so->shader_id, so->id, so->info.instrs_per_cat[0],
   so->info.instrs_per_cat[1], so->info.instrs_per_cat[2],
   so->info.instrs_per_cat[3], so->info.instrs_per_cat[4],
   so->info.instrs_per_cat[5], so->info.instrs_per_cat[6],
         fprintf(
      out,
   "; %s prog %d/%d: %u sstall, %u (ss), %u systall, %u (sy), %d loops\n",
   type, so->shader_id, so->id, so->info.sstall, so->info.ss,
         /* print shader type specific info: */
   switch (so->type) {
   case MESA_SHADER_VERTEX:
      dump_output(out, so, VARYING_SLOT_POS, "pos");
   dump_output(out, so, VARYING_SLOT_PSIZ, "psize");
      case MESA_SHADER_FRAGMENT:
      dump_reg(out, "pos (ij_pixel)",
         dump_reg(
      out, "pos (ij_centroid)",
      dump_reg(out, "pos (center_rhw)",
         dump_output(out, so, FRAG_RESULT_DEPTH, "posz");
   if (so->color0_mrt) {
         } else {
      dump_output(out, so, FRAG_RESULT_DATA0, "data0");
   dump_output(out, so, FRAG_RESULT_DATA1, "data1");
   dump_output(out, so, FRAG_RESULT_DATA2, "data2");
   dump_output(out, so, FRAG_RESULT_DATA3, "data3");
   dump_output(out, so, FRAG_RESULT_DATA4, "data4");
   dump_output(out, so, FRAG_RESULT_DATA5, "data5");
   dump_output(out, so, FRAG_RESULT_DATA6, "data6");
      }
   dump_reg(out, "fragcoord",
         dump_reg(out, "fragface",
            default:
      /* TODO */
                  }
      uint64_t
   ir3_shader_outputs(const struct ir3_shader *so)
   {
         }
      /* Add any missing varyings needed for stream-out.  Otherwise varyings not
   * used by fragment shader will be stripped out.
   */
   void
   ir3_link_stream_out(struct ir3_shader_linkage *l,
         {
               /*
   * First, any stream-out varyings not already in linkage map (ie. also
   * consumed by frag shader) need to be added:
   */
   for (unsigned i = 0; i < strmout->num_outputs; i++) {
      const struct ir3_stream_output *out = &strmout->output[i];
   unsigned k = out->register_index;
   unsigned compmask =
                  /* psize/pos need to be the last entries in linkage map, and will
   * get added link_stream_out, so skip over them:
   */
   if ((v->outputs[k].slot == VARYING_SLOT_PSIZ) ||
                  for (idx = 0; idx < l->cnt; idx++) {
      if (l->var[idx].slot == v->outputs[k].slot)
                     /* add if not already in linkage map: */
   if (idx == l->cnt) {
      ir3_link_add(l, v->outputs[k].slot, v->outputs[k].regid,
               /* expand component-mask if needed, ie streaming out all components
   * but frag shader doesn't consume all components:
   */
   if (compmask & ~l->var[idx].compmask) {
      l->var[idx].compmask |= compmask;
   l->max_loc = MAX2(
            }
