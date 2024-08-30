   /*
   * Copyright (C) 2020 Collabora Ltd.
   * Copyright (C) 2022 Alyssa Rosenzweig <alyssa@rosenzweig.io>
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
   * Authors (Collabora):
   *      Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   */
      #include "compiler/glsl/glsl_to_nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir_types.h"
   #include "util/u_debug.h"
      #include "bifrost/disassemble.h"
   #include "valhall/disassemble.h"
   #include "valhall/va_compiler.h"
   #include "bi_builder.h"
   #include "bi_quirks.h"
   #include "bifrost_compile.h"
   #include "bifrost_nir.h"
   #include "compiler.h"
      /* clang-format off */
   static const struct debug_named_value bifrost_debug_options[] = {
      {"msgs",       BIFROST_DBG_MSGS,		   "Print debug messages"},
   {"shaders",    BIFROST_DBG_SHADERS,	   "Dump shaders in NIR and MIR"},
   {"shaderdb",   BIFROST_DBG_SHADERDB,	"Print statistics"},
   {"verbose",    BIFROST_DBG_VERBOSE,	   "Disassemble verbosely"},
   {"internal",   BIFROST_DBG_INTERNAL,	"Dump even internal shaders"},
   {"nosched",    BIFROST_DBG_NOSCHED, 	"Force trivial bundling"},
   {"nopsched",   BIFROST_DBG_NOPSCHED,   "Disable scheduling for pressure"},
   {"inorder",    BIFROST_DBG_INORDER, 	"Force in-order bundling"},
   {"novalidate", BIFROST_DBG_NOVALIDATE, "Skip IR validation"},
   {"noopt",      BIFROST_DBG_NOOPT,      "Skip optimization passes"},
   {"noidvs",     BIFROST_DBG_NOIDVS,     "Disable IDVS"},
   {"nosb",       BIFROST_DBG_NOSB,       "Disable scoreboarding"},
   {"nopreload",  BIFROST_DBG_NOPRELOAD,  "Disable message preloading"},
   {"spill",      BIFROST_DBG_SPILL,      "Test register spilling"},
      };
   /* clang-format on */
      DEBUG_GET_ONCE_FLAGS_OPTION(bifrost_debug, "BIFROST_MESA_DEBUG",
            /* How many bytes are prefetched by the Bifrost shader core. From the final
   * clause of the shader, this range must be valid instructions or zero. */
   #define BIFROST_SHADER_PREFETCH 128
      int bifrost_debug = 0;
      #define DBG(fmt, ...)                                                          \
      do {                                                                        \
      if (bifrost_debug & BIFROST_DBG_MSGS)                                    \
            static bi_block *emit_cf_list(bi_context *ctx, struct exec_list *list);
      static bi_index
   bi_preload(bi_builder *b, unsigned reg)
   {
      if (bi_is_null(b->shader->preloaded[reg])) {
      /* Insert at the beginning of the shader */
   bi_builder b_ = *b;
            /* Cache the result */
                  }
      static bi_index
   bi_coverage(bi_builder *b)
   {
      if (bi_is_null(b->shader->coverage))
               }
      /*
   * Vertex ID and Instance ID are preloaded registers. Where they are preloaded
   * changed from Bifrost to Valhall. Provide helpers that smooth over the
   * architectural difference.
   */
   static inline bi_index
   bi_vertex_id(bi_builder *b)
   {
         }
      static inline bi_index
   bi_instance_id(bi_builder *b)
   {
         }
      static void
   bi_emit_jump(bi_builder *b, nir_jump_instr *instr)
   {
               switch (instr->type) {
   case nir_jump_break:
      branch->branch_target = b->shader->break_block;
      case nir_jump_continue:
      branch->branch_target = b->shader->continue_block;
      default:
                  bi_block_add_successor(b->shader->current_block, branch->branch_target);
      }
      /* Builds a 64-bit hash table key for an index */
   static uint64_t
   bi_index_to_key(bi_index idx)
   {
               uint64_t key = 0;
   memcpy(&key, &idx, sizeof(idx));
      }
      /*
   * Extract a single channel out of a vector source. We split vectors with SPLIT
   * so we can use the split components directly, without emitting an extract.
   * This has advantages of RA, as the split can usually be optimized away.
   */
   static bi_index
   bi_extract(bi_builder *b, bi_index vec, unsigned channel)
   {
      bi_index *components = _mesa_hash_table_u64_search(b->shader->allocated_vec,
            /* No extract needed for scalars.
   *
   * This is a bit imprecise, but actual bugs (missing splits for vectors)
   * should be caught by the following assertion. It is too difficult to
   * ensure bi_extract is only called for real vectors.
   */
   if (components == NULL && channel == 0)
            assert(components != NULL && "missing bi_cache_collect()");
      }
      static void
   bi_cache_collect(bi_builder *b, bi_index dst, bi_index *s, unsigned n)
   {
      /* Lifetime of a hash table entry has to be at least as long as the table */
   bi_index *channels = ralloc_array(b->shader, bi_index, n);
            _mesa_hash_table_u64_insert(b->shader->allocated_vec, bi_index_to_key(dst),
      }
      /*
   * Splits an n-component vector (vec) into n scalar destinations (dests) using a
   * split pseudo-instruction.
   *
   * Pre-condition: dests is filled with bi_null().
   */
   static void
   bi_emit_split_i32(bi_builder *b, bi_index dests[4], bi_index vec, unsigned n)
   {
      /* Setup the destinations */
   for (unsigned i = 0; i < n; ++i) {
                  /* Emit the split */
   if (n == 1) {
         } else {
               bi_foreach_dest(I, j)
         }
      static void
   bi_emit_cached_split_i32(bi_builder *b, bi_index vec, unsigned n)
   {
      bi_index dests[4] = {bi_null(), bi_null(), bi_null(), bi_null()};
   bi_emit_split_i32(b, dests, vec, n);
      }
      /*
   * Emit and cache a split for a vector of a given bitsize. The vector may not be
   * composed of 32-bit words, but it will be split at 32-bit word boundaries.
   */
   static void
   bi_emit_cached_split(bi_builder *b, bi_index vec, unsigned bits)
   {
         }
      static void
   bi_split_def(bi_builder *b, nir_def *def)
   {
      bi_emit_cached_split(b, bi_def_index(def),
      }
      static bi_instr *
   bi_emit_collect_to(bi_builder *b, bi_index dst, bi_index *chan, unsigned n)
   {
      /* Special case: COLLECT of a single value is a scalar move */
   if (n == 1)
                     bi_foreach_src(I, i)
            bi_cache_collect(b, dst, chan, n);
      }
      static bi_instr *
   bi_collect_v2i32_to(bi_builder *b, bi_index dst, bi_index s0, bi_index s1)
   {
         }
      static bi_instr *
   bi_collect_v3i32_to(bi_builder *b, bi_index dst, bi_index s0, bi_index s1,
         {
         }
      static bi_index
   bi_collect_v2i32(bi_builder *b, bi_index s0, bi_index s1)
   {
      bi_index dst = bi_temp(b->shader);
   bi_collect_v2i32_to(b, dst, s0, s1);
      }
      static bi_index
   bi_varying_src0_for_barycentric(bi_builder *b, nir_intrinsic_instr *intr)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_sample:
            /* Need to put the sample ID in the top 16-bits */
   case nir_intrinsic_load_barycentric_at_sample:
      return bi_mkvec_v2i16(b, bi_half(bi_dontcare(b), false),
         /* Interpret as 8:8 signed fixed point positions in pixels along X and
   * Y axes respectively, relative to top-left of pixel. In NIR, (0, 0)
   * is the center of the pixel so we first fixup and then convert. For
   * fp16 input:
   *
   * f2i16(((x, y) + (0.5, 0.5)) * 2**8) =
   * f2i16((256 * (x, y)) + (128, 128)) =
   * V2F16_TO_V2S16(FMA.v2f16((x, y), #256, #128))
   *
   * For fp32 input, that lacks enough precision for MSAA 16x, but the
   * idea is the same. FIXME: still doesn't pass
   */
   case nir_intrinsic_load_barycentric_at_offset: {
      bi_index offset = bi_src_index(&intr->src[0]);
   bi_index f16 = bi_null();
            if (sz == 16) {
         } else {
      assert(sz == 32);
   bi_index f[2];
   for (unsigned i = 0; i < 2; ++i) {
      f[i] =
                                             case nir_intrinsic_load_barycentric_pixel:
   default:
            }
      static enum bi_sample
   bi_interp_for_intrinsic(nir_intrinsic_op op)
   {
      switch (op) {
   case nir_intrinsic_load_barycentric_centroid:
         case nir_intrinsic_load_barycentric_sample:
   case nir_intrinsic_load_barycentric_at_sample:
         case nir_intrinsic_load_barycentric_at_offset:
         case nir_intrinsic_load_barycentric_pixel:
   default:
            }
      /* auto, 64-bit omitted */
   static enum bi_register_format
   bi_reg_fmt_for_nir(nir_alu_type T)
   {
      switch (T) {
   case nir_type_float16:
         case nir_type_float32:
         case nir_type_int16:
         case nir_type_uint16:
         case nir_type_int32:
         case nir_type_uint32:
         default:
            }
      /* Checks if the _IMM variant of an intrinsic can be used, returning in imm the
   * immediate to be used (which applies even if _IMM can't be used) */
      static bool
   bi_is_intr_immediate(nir_intrinsic_instr *instr, unsigned *immediate,
         {
               if (!nir_src_is_const(*offset))
            *immediate = nir_intrinsic_base(instr) + nir_src_as_uint(*offset);
      }
      static void bi_make_vec_to(bi_builder *b, bi_index final_dst, bi_index *src,
            /* Bifrost's load instructions lack a component offset despite operating in
   * terms of vec4 slots. Usually I/O vectorization avoids nonzero components,
   * but they may be unavoidable with separate shaders in use. To solve this, we
   * lower to a larger load and an explicit copy of the desired components. */
      static void
   bi_copy_component(bi_builder *b, nir_intrinsic_instr *instr, bi_index tmp)
   {
      unsigned component = nir_intrinsic_component(instr);
   unsigned nr = instr->num_components;
   unsigned total = nr + component;
            assert(total <= 4 && "should be vec4");
            if (component == 0)
            bi_index srcs[] = {tmp, tmp, tmp};
            bi_make_vec_to(b, bi_def_index(&instr->def), srcs, channels, nr,
      }
      static void
   bi_emit_load_attr(bi_builder *b, nir_intrinsic_instr *instr)
   {
      /* Disregard the signedness of an integer, since loading 32-bits into a
   * 32-bit register should be bit exact so should not incur any clamping.
   *
   * If we are reading as a u32, then it must be paired with an integer (u32 or
   * s32) source, so use .auto32 to disregard.
   */
   nir_alu_type T = nir_intrinsic_dest_type(instr);
   assert(T == nir_type_uint32 || T == nir_type_int32 || T == nir_type_float32);
   enum bi_register_format regfmt =
            nir_src *offset = nir_get_io_offset_src(instr);
   unsigned component = nir_intrinsic_component(instr);
   enum bi_vecsize vecsize = (instr->num_components + component - 1);
   unsigned imm_index = 0;
   unsigned base = nir_intrinsic_base(instr);
   bool constant = nir_src_is_const(*offset);
   bool immediate = bi_is_intr_immediate(instr, &imm_index, 16);
   bi_index dest =
                  if (immediate) {
      I = bi_ld_attr_imm_to(b, dest, bi_vertex_id(b), bi_instance_id(b), regfmt,
      } else {
               if (constant)
         else if (base != 0)
            I = bi_ld_attr_to(b, dest, bi_vertex_id(b), bi_instance_id(b), idx,
               if (b->shader->arch >= 9)
               }
      /*
   * ABI: Special (desktop GL) slots come first, tightly packed. General varyings
   * come later, sparsely packed. This handles both linked and separable shaders
   * with a common code path, with minimal keying only for desktop GL. Each slot
   * consumes 16 bytes (TODO: fp16, partial vectors).
   */
   static unsigned
   bi_varying_base_bytes(bi_context *ctx, nir_intrinsic_instr *intr)
   {
      nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
            if (sem.location >= VARYING_SLOT_VAR0) {
      unsigned nr_special = util_bitcount(mask);
               } else {
            }
      /*
   * Compute the offset in bytes of a varying with an immediate offset, adding the
   * offset to the base computed above. Convenience method.
   */
   static unsigned
   bi_varying_offset(bi_context *ctx, nir_intrinsic_instr *intr)
   {
      nir_src *src = nir_get_io_offset_src(intr);
               }
      static void
   bi_emit_load_vary(bi_builder *b, nir_intrinsic_instr *instr)
   {
      enum bi_sample sample = BI_SAMPLE_CENTER;
   enum bi_update update = BI_UPDATE_STORE;
   enum bi_register_format regfmt = BI_REGISTER_FORMAT_AUTO;
   bool smooth = instr->intrinsic == nir_intrinsic_load_interpolated_input;
            unsigned component = nir_intrinsic_component(instr);
   enum bi_vecsize vecsize = (instr->num_components + component - 1);
   bi_index dest =
                     if (smooth) {
      nir_intrinsic_instr *parent = nir_src_as_intrinsic(instr->src[0]);
            sample = bi_interp_for_intrinsic(parent->intrinsic);
            assert(sz == 16 || sz == 32);
      } else {
      assert(sz == 32);
            /* Valhall can't have bi_null() here, although the source is
   * logically unused for flat varyings
   */
   if (b->shader->arch >= 9)
            /* Gather info as we go */
               enum bi_source_format source_format =
            nir_src *offset = nir_get_io_offset_src(instr);
   unsigned imm_index = 0;
   bool immediate = bi_is_intr_immediate(instr, &imm_index, 20);
            if (b->shader->malloc_idvs && immediate) {
      /* Immediate index given in bytes. */
   bi_ld_var_buf_imm_to(b, sz, dest, src0, regfmt, sample, source_format,
            } else if (immediate && smooth) {
      I = bi_ld_var_imm_to(b, dest, src0, regfmt, sample, update, vecsize,
      } else if (immediate && !smooth) {
      I = bi_ld_var_flat_imm_to(b, dest, BI_FUNCTION_NONE, regfmt, vecsize,
      } else {
      bi_index idx = bi_src_index(offset);
            if (b->shader->malloc_idvs) {
      /* Index needs to be in bytes, but NIR gives the index
   * in slots. For now assume 16 bytes per element.
   */
                                 bi_ld_var_buf_to(b, sz, dest, src0, idx_bytes, regfmt, sample,
      } else if (smooth) {
                        } else {
                                    /* Valhall usually uses machine-allocated IDVS. If this is disabled, use
   * a simple Midgard-style ABI.
   */
   if (b->shader->arch >= 9 && I != NULL)
               }
      static bi_index
   bi_make_vec8_helper(bi_builder *b, bi_index *src, unsigned *channel,
         {
                        for (unsigned i = 0; i < count; ++i) {
                           if (b->shader->arch >= 9) {
               if (count >= 3)
               } else {
            }
      static bi_index
   bi_make_vec16_helper(bi_builder *b, bi_index *src, unsigned *channel,
         {
      unsigned chan0 = channel ? channel[0] : 0;
   bi_index w0 = bi_extract(b, src[0], chan0 >> 1);
            /* Zero extend */
   if (count == 1)
            /* Else, create a vector */
            unsigned chan1 = channel ? channel[1] : 0;
   bi_index w1 = bi_extract(b, src[1], chan1 >> 1);
            if (bi_is_word_equiv(w0, w1) && (chan0 & 1) == 0 && ((chan1 & 1) == 1))
         else if (bi_is_word_equiv(w0, w1))
         else
      }
      static void
   bi_make_vec_to(bi_builder *b, bi_index dst, bi_index *src, unsigned *channel,
         {
      assert(bitsize == 8 || bitsize == 16 || bitsize == 32);
   unsigned shift = (bitsize == 32) ? 0 : (bitsize == 16) ? 1 : 2;
            assert(DIV_ROUND_UP(count * bitsize, 32) <= BI_MAX_SRCS &&
                     for (unsigned i = 0; i < count; i += chan_per_word) {
      unsigned rem = MIN2(count - i, chan_per_word);
            if (bitsize == 32)
         else if (bitsize == 16)
         else
                  }
      static inline bi_instr *
   bi_load_ubo_to(bi_builder *b, unsigned bitsize, bi_index dest0, bi_index src0,
         {
               if (b->shader->arch >= 9) {
      I = bi_ld_buffer_to(b, bitsize, dest0, src0, src1);
      } else {
                  bi_emit_cached_split(b, dest0, bitsize);
      }
      static void
   bi_load_sample_id_to(bi_builder *b, bi_index dst)
   {
      /* r61[16:23] contains the sampleID, mask it out. Upper bits
   * seem to read garbage (despite being architecturally defined
            bi_rshift_and_i32_to(b, dst, bi_preload(b, 61), bi_imm_u32(0x1f),
      }
      static bi_index
   bi_load_sample_id(bi_builder *b)
   {
      bi_index sample_id = bi_temp(b->shader);
   bi_load_sample_id_to(b, sample_id);
      }
      static bi_index
   bi_pixel_indices(bi_builder *b, unsigned rt)
   {
      /* We want to load the current pixel. */
            uint32_t indices_u32 = 0;
   memcpy(&indices_u32, &pix, sizeof(indices_u32));
            /* Sample index above is left as zero. For multisampling, we need to
            if (b->shader->inputs->blend.nr_samples > 1)
               }
      /* Source color is passed through r0-r3, or r4-r7 for the second source when
   * dual-source blending. Preload the corresponding vector.
   */
   static void
   bi_emit_load_blend_input(bi_builder *b, nir_intrinsic_instr *instr)
   {
      nir_io_semantics sem = nir_intrinsic_io_semantics(instr);
   unsigned base = (sem.location == VARYING_SLOT_VAR0) ? 4 : 0;
   unsigned size = nir_alu_type_get_type_size(nir_intrinsic_dest_type(instr));
            bi_index srcs[] = {bi_preload(b, base + 0), bi_preload(b, base + 1),
               }
      static void
   bi_emit_blend_op(bi_builder *b, bi_index rgba, nir_alu_type T, bi_index rgba2,
         {
      /* Reads 2 or 4 staging registers to cover the input */
   unsigned size = nir_alu_type_get_type_size(T);
   unsigned size_2 = nir_alu_type_get_type_size(T2);
   unsigned sr_count = (size <= 16) ? 2 : 4;
   unsigned sr_count_2 = (size_2 <= 16) ? 2 : 4;
   const struct panfrost_compile_inputs *inputs = b->shader->inputs;
   uint64_t blend_desc = inputs->blend.bifrost_blend_desc;
            /* Workaround for NIR-to-TGSI */
   if (b->shader->nir->info.fs.untyped_color_outputs)
            if (inputs->is_blend && inputs->blend.nr_samples > 1) {
      /* Conversion descriptor comes from the compile inputs, pixel
   * indices derived at run time based on sample ID */
   bi_st_tile(b, rgba, bi_pixel_indices(b, rt), bi_coverage(b),
      } else if (b->shader->inputs->is_blend) {
               /* Blend descriptor comes from the compile inputs */
            bi_blend_to(b, bi_temp(b->shader), rgba, bi_coverage(b),
            } else {
      /* Blend descriptor comes from the FAU RAM. By convention, the
   * return address on Bifrost is stored in r48 and will be used
            bi_blend_to(b, bi_temp(b->shader), rgba, bi_coverage(b),
                           assert(rt < 8);
            if (T2)
      }
      /* Blend shaders do not need to run ATEST since they are dependent on a
   * fragment shader that runs it. Blit shaders may not need to run ATEST, since
   * ATEST is not needed if early-z is forced, alpha-to-coverage is disabled, and
   * there are no writes to the coverage mask. The latter two are satisfied for
   * all blit shaders, so we just care about early-z, which blit shaders force
   * iff they do not write depth or stencil */
      static bool
   bi_skip_atest(bi_context *ctx, bool emit_zs)
   {
         }
      static void
   bi_emit_atest(bi_builder *b, bi_index alpha)
   {
      b->shader->coverage =
            }
      static void
   bi_emit_fragment_out(bi_builder *b, nir_intrinsic_instr *instr)
   {
               unsigned writeout =
            bool emit_blend = writeout & (PAN_WRITEOUT_C);
            unsigned loc = nir_intrinsic_io_semantics(instr).location;
            /* By ISA convention, the coverage mask is stored in R60. The store
   * itself will be handled by a subsequent ATEST instruction */
   if (loc == FRAG_RESULT_SAMPLE_MASK) {
      b->shader->coverage = bi_extract(b, src0, 0);
               /* Emit ATEST if we have to, note ATEST requires a floating-point alpha
   * value, but render target #0 might not be floating point. However the
   * alpha value is only used for alpha-to-coverage, a stage which is
            if (!b->shader->emitted_atest && !bi_skip_atest(b->shader, emit_zs)) {
               bi_index rgba = bi_src_index(&instr->src[0]);
   bi_index alpha = (T == nir_type_float16)
                        /* Don't read out-of-bounds */
   if (nir_src_num_components(instr->src[0]) < 4)
                        if (emit_zs) {
               if (writeout & PAN_WRITEOUT_Z)
            if (writeout & PAN_WRITEOUT_S)
            b->shader->coverage =
      bi_zs_emit(b, z, s, bi_coverage(b), writeout & PAN_WRITEOUT_S,
            if (emit_blend) {
      unsigned rt = loc ? (loc - FRAG_RESULT_DATA0) : 0;
   bool dual = (writeout & PAN_WRITEOUT_2);
   bi_index color = bi_src_index(&instr->src[0]);
   bi_index color2 = dual ? bi_src_index(&instr->src[4]) : bi_null();
            /* Explicit copy since BLEND inputs are precoloured to R0-R3,
   * TODO: maybe schedule around this or implement in RA as a
   * spill */
   bool has_mrt =
            if (has_mrt) {
      bi_index srcs[4] = {color, color, color, color};
   unsigned channels[4] = {0, 1, 2, 3};
   color = bi_temp(b->shader);
   bi_make_vec_to(
      b, color, srcs, channels, nir_src_num_components(instr->src[0]),
                        if (b->shader->inputs->is_blend) {
      /* Jump back to the fragment shader, return address is stored
   * in r48 (see above). On Valhall, only jump if the address is
   * nonzero. The check is free there and it implements the "jump
   * to 0 terminates the blend shader" that's automatic on
   * Bifrost.
   */
   if (b->shader->arch >= 8)
         else
         }
      /**
   * In a vertex shader, is the specified variable a position output? These kinds
   * of outputs are written from position shaders when IDVS is enabled. All other
   * outputs are written from the varying shader.
   */
   static bool
   bi_should_remove_store(nir_intrinsic_instr *intr, enum bi_idvs_mode idvs)
   {
               switch (sem.location) {
   case VARYING_SLOT_POS:
   case VARYING_SLOT_PSIZ:
         default:
            }
      static bool
   bifrost_nir_specialize_idvs(nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     if (intr->intrinsic != nir_intrinsic_store_output)
            if (bi_should_remove_store(intr, *idvs)) {
      nir_instr_remove(instr);
                  }
      static void
   bi_emit_store_vary(bi_builder *b, nir_intrinsic_instr *instr)
   {
      /* In principle we can do better for 16-bit. At the moment we require
   * 32-bit to permit the use of .auto, in order to force .u32 for flat
   * varyings, to handle internal TGSI shaders that set flat in the VS
            ASSERTED nir_alu_type T = nir_intrinsic_src_type(instr);
   ASSERTED unsigned T_size = nir_alu_type_get_type_size(T);
   assert(T_size == 32 || (b->shader->arch >= 9 && T_size == 16));
            unsigned imm_index = 0;
            /* Only look at the total components needed. In effect, we fill in all
   * the intermediate "holes" in the write mask, since we can't mask off
   * stores. Since nir_lower_io_to_temporaries ensures each varying is
   * written at most once, anything that's masked out is undefined, so it
   * doesn't matter what we write there. So we may as well do the
   * simplest thing possible. */
   unsigned nr = util_last_bit(nir_intrinsic_write_mask(instr));
                     /* To keep the vector dimensions consistent, we need to drop some
   * components. This should be coalesced.
   *
   * TODO: This is ugly and maybe inefficient. Would we rather
   * introduce a TRIM.i32 pseudoinstruction?
   */
   if (nr < nir_intrinsic_src_components(instr, 0)) {
               bi_index chans[4] = {bi_null(), bi_null(), bi_null(), bi_null()};
                     bi_index tmp = bi_temp(b->shader);
            bi_foreach_src(collect, w)
                        bool psiz =
                     if (b->shader->arch <= 8 && b->shader->idvs == BI_IDVS_POSITION) {
      /* Bifrost position shaders have a fast path */
   assert(T == nir_type_float16 || T == nir_type_float32);
   unsigned regfmt = (T == nir_type_float16) ? 0 : 1;
   unsigned identity = (b->shader->arch == 6) ? 0x688 : 0;
   unsigned snap4 = 0x5E;
            bi_st_cvt(b, data, bi_preload(b, 58), bi_preload(b, 59),
      } else if (b->shader->arch >= 9 && b->shader->idvs != BI_IDVS_NONE) {
               if (psiz) {
      assert(T_size == 16 && "should've been lowered");
               bi_index address = bi_lea_buf_imm(b, index);
                     bi_store(b, nr * nir_src_bit_size(instr->src[0]), data, a[0], a[1],
            } else if (immediate) {
      bi_index address = bi_lea_attr_imm(b, bi_vertex_id(b), bi_instance_id(b),
                     } else {
      bi_index idx = bi_iadd_u32(b, bi_src_index(nir_get_io_offset_src(instr)),
         bi_index address =
                        }
      static void
   bi_emit_load_ubo(bi_builder *b, nir_intrinsic_instr *instr)
   {
               bool offset_is_const = nir_src_is_const(*offset);
   bi_index dyn_offset = bi_src_index(offset);
            bi_load_ubo_to(b, instr->num_components * instr->def.bit_size,
                  }
      static void
   bi_emit_load_push_constant(bi_builder *b, nir_intrinsic_instr *instr)
   {
               nir_src *offset = &instr->src[0];
   assert(nir_src_is_const(*offset) && "no indirect push constants");
   uint32_t base = nir_intrinsic_base(instr) + nir_src_as_uint(*offset);
                     unsigned n = DIV_ROUND_UP(bits, 32);
   assert(n <= 4);
            for (unsigned i = 0; i < n; ++i) {
                              }
      static bi_index
   bi_addr_high(bi_builder *b, nir_src *src)
   {
      return (nir_src_bit_size(*src) == 64) ? bi_extract(b, bi_src_index(src), 1)
      }
      static void
   bi_handle_segment(bi_builder *b, bi_index *addr_lo, bi_index *addr_hi,
         {
      /* Not needed on Bifrost or for global accesses */
   if (b->shader->arch < 9 || seg == BI_SEG_NONE)
            /* There is no segment modifier on Valhall. Instead, we need to
   * emit the arithmetic ourselves. We do have an offset
   * available, which saves an instruction for constant offsets.
   */
   bool wls = (seg == BI_SEG_WLS);
                              if (offset && addr_lo->type == BI_INDEX_CONSTANT &&
      addr_lo->value == (int16_t)addr_lo->value) {
   *offset = addr_lo->value;
      } else {
                  /* Do not allow overflow for WLS or TLS */
      }
      static void
   bi_emit_load(bi_builder *b, nir_intrinsic_instr *instr, enum bi_seg seg)
   {
      int16_t offset = 0;
   unsigned bits = instr->num_components * instr->def.bit_size;
   bi_index dest = bi_def_index(&instr->def);
   bi_index addr_lo = bi_extract(b, bi_src_index(&instr->src[0]), 0);
                     bi_load_to(b, bits, dest, addr_lo, addr_hi, seg, offset);
      }
      static void
   bi_emit_store(bi_builder *b, nir_intrinsic_instr *instr, enum bi_seg seg)
   {
      /* Require contiguous masks, gauranteed by nir_lower_wrmasks */
   assert(nir_intrinsic_write_mask(instr) ==
            int16_t offset = 0;
   bi_index addr_lo = bi_extract(b, bi_src_index(&instr->src[1]), 0);
                     bi_store(b, instr->num_components * nir_src_bit_size(instr->src[0]),
      }
      /* Exchanges the staging register with memory */
      static void
   bi_emit_axchg_to(bi_builder *b, bi_index dst, bi_index addr, nir_src *arg,
         {
               unsigned sz = nir_src_bit_size(*arg);
                              if (b->shader->arch >= 9)
         else if (seg == BI_SEG_WLS)
               }
      /* Exchanges the second staging register with memory if comparison with first
   * staging register passes */
      static void
   bi_emit_acmpxchg_to(bi_builder *b, bi_index dst, bi_index addr, nir_src *arg_1,
         {
               /* hardware is swapped from NIR */
   bi_index src0 = bi_src_index(arg_2);
            unsigned sz = nir_src_bit_size(*arg_1);
            bi_index data_words[] = {
      bi_extract(b, src0, 0),
            /* 64-bit */
   bi_extract(b, src1, 0),
               bi_index in = bi_temp(b->shader);
   bi_emit_collect_to(b, in, data_words, 2 * (sz / 32));
            if (b->shader->arch >= 9)
         else if (seg == BI_SEG_WLS)
            bi_index out = bi_acmpxchg(b, sz, in, bi_extract(b, addr, 0), addr_hi, seg);
            bi_index inout_words[] = {bi_extract(b, out, 0),
               }
      static enum bi_atom_opc
   bi_atom_opc_for_nir(nir_atomic_op op)
   {
      /* clang-format off */
   switch (op) {
   case nir_atomic_op_iadd: return BI_ATOM_OPC_AADD;
   case nir_atomic_op_imin: return BI_ATOM_OPC_ASMIN;
   case nir_atomic_op_umin: return BI_ATOM_OPC_AUMIN;
   case nir_atomic_op_imax: return BI_ATOM_OPC_ASMAX;
   case nir_atomic_op_umax: return BI_ATOM_OPC_AUMAX;
   case nir_atomic_op_iand: return BI_ATOM_OPC_AAND;
   case nir_atomic_op_ior:  return BI_ATOM_OPC_AOR;
   case nir_atomic_op_ixor: return BI_ATOM_OPC_AXOR;
   default: unreachable("Unexpected computational atomic");
   }
      }
      /* Optimized unary atomics are available with an implied #1 argument */
      static bool
   bi_promote_atom_c1(enum bi_atom_opc op, bi_index arg, enum bi_atom_opc *out)
   {
      /* Check we have a compatible constant */
   if (arg.type != BI_INDEX_CONSTANT)
            if (!(arg.value == 1 || (arg.value == -1 && op == BI_ATOM_OPC_AADD)))
            /* Check for a compatible operation */
   switch (op) {
   case BI_ATOM_OPC_AADD:
      *out = (arg.value == 1) ? BI_ATOM_OPC_AINC : BI_ATOM_OPC_ADEC;
      case BI_ATOM_OPC_ASMAX:
      *out = BI_ATOM_OPC_ASMAX1;
      case BI_ATOM_OPC_AUMAX:
      *out = BI_ATOM_OPC_AUMAX1;
      case BI_ATOM_OPC_AOR:
      *out = BI_ATOM_OPC_AOR1;
      default:
            }
      /*
   * Coordinates are 16-bit integers in Bifrost but 32-bit in NIR. We need to
   * translate between these forms (with MKVEC.v2i16).
   *
   * Aditionally on Valhall, cube maps in the attribute pipe are treated as 2D
   * arrays.  For uniform handling, we also treat 3D textures like 2D arrays.
   *
   * Our indexing needs to reflects this.
   */
   static bi_index
   bi_emit_image_coord(bi_builder *b, bi_index coord, unsigned src_idx,
         {
               if (src_idx == 0) {
      if (coord_comps == 1 || (coord_comps == 2 && is_array))
         else
      return bi_mkvec_v2i16(b, bi_half(bi_extract(b, coord, 0), false),
   } else {
      if (coord_comps == 3 && b->shader->arch >= 9)
      return bi_mkvec_v2i16(b, bi_imm_u16(0),
      else if (coord_comps == 2 && is_array && b->shader->arch >= 9)
      return bi_mkvec_v2i16(b, bi_imm_u16(0),
      else if (coord_comps == 3)
         else if (coord_comps == 2 && is_array)
         else
         }
      static bi_index
   bi_emit_image_index(bi_builder *b, nir_intrinsic_instr *instr)
   {
      nir_src src = instr->src[0];
   bi_index index = bi_src_index(&src);
            /* Images come after vertex attributes, so handle an explicit offset */
   unsigned offset = (ctx->stage == MESA_SHADER_VERTEX)
                  if (offset == 0)
         else if (nir_src_is_const(src))
         else
      }
      static void
   bi_emit_image_load(bi_builder *b, nir_intrinsic_instr *instr)
   {
      enum glsl_sampler_dim dim = nir_intrinsic_image_dim(instr);
   unsigned coord_comps = nir_image_intrinsic_coord_components(instr);
   bool array = nir_intrinsic_image_array(instr);
            bi_index coords = bi_src_index(&instr->src[1]);
   bi_index xy = bi_emit_image_coord(b, coords, 0, coord_comps, array);
   bi_index zw = bi_emit_image_coord(b, coords, 1, coord_comps, array);
   bi_index dest = bi_def_index(&instr->def);
   enum bi_register_format regfmt =
                  /* TODO: MSAA */
            if (b->shader->arch >= 9 && nir_src_is_const(instr->src[0])) {
      bi_instr *I = bi_ld_tex_imm_to(b, dest, xy, zw, regfmt, vecsize,
               } else if (b->shader->arch >= 9) {
         } else {
      bi_ld_attr_tex_to(b, dest, xy, zw, bi_emit_image_index(b, instr), regfmt,
                  }
      static void
   bi_emit_lea_image_to(bi_builder *b, bi_index dest, nir_intrinsic_instr *instr)
   {
      enum glsl_sampler_dim dim = nir_intrinsic_image_dim(instr);
   bool array = nir_intrinsic_image_array(instr);
   ASSERTED unsigned nr_dim = glsl_get_sampler_dim_coordinate_components(dim);
            /* TODO: MSAA */
            enum bi_register_format type =
      (instr->intrinsic == nir_intrinsic_image_store)
               bi_index coords = bi_src_index(&instr->src[1]);
   bi_index xy = bi_emit_image_coord(b, coords, 0, coord_comps, array);
            if (b->shader->arch >= 9 && nir_src_is_const(instr->src[0])) {
      bi_instr *I = bi_lea_tex_imm_to(b, dest, xy, zw, false,
               } else if (b->shader->arch >= 9) {
         } else {
      bi_instr *I = bi_lea_attr_tex_to(b, dest, xy, zw,
            /* LEA_ATTR_TEX defaults to the secondary attribute table, but
   * our ABI has all images in the primary attribute table
   */
                  }
      static bi_index
   bi_emit_lea_image(bi_builder *b, nir_intrinsic_instr *instr)
   {
      bi_index dest = bi_temp(b->shader);
   bi_emit_lea_image_to(b, dest, instr);
      }
      static void
   bi_emit_image_store(bi_builder *b, nir_intrinsic_instr *instr)
   {
      bi_index a[4] = {bi_null()};
            /* Due to SPIR-V limitations, the source type is not fully reliable: it
   * reports uint32 even for write_imagei. This causes an incorrect
   * u32->s32->u32 roundtrip which incurs an unwanted clamping. Use auto32
   * instead, which will match per the OpenCL spec. Of course this does
   * not work for 16-bit stores, but those are not available in OpenCL.
   */
   nir_alu_type T = nir_intrinsic_src_type(instr);
            bi_st_cvt(b, bi_src_index(&instr->src[3]), a[0], a[1], a[2],
      }
      static void
   bi_emit_atomic_i32_to(bi_builder *b, bi_index dst, bi_index addr, bi_index arg,
         {
      enum bi_atom_opc opc = bi_atom_opc_for_nir(op);
   enum bi_atom_opc post_opc = opc;
            /* ATOM_C.i32 takes a vector with {arg, coalesced}, ATOM_C1.i32 doesn't
   * take any vector but can still output in RETURN mode */
   bi_index tmp_dest = bifrost ? bi_temp(b->shader) : dst;
            /* Generate either ATOM or ATOM1 as required */
   if (bi_promote_atom_c1(opc, arg, &opc)) {
      bi_atom1_return_i32_to(b, tmp_dest, bi_extract(b, addr, 0),
      } else {
      bi_atom_return_i32_to(b, tmp_dest, arg, bi_extract(b, addr, 0),
               if (bifrost) {
      /* Post-process it */
   bi_emit_cached_split_i32(b, tmp_dest, 2);
   bi_atom_post_i32_to(b, dst, bi_extract(b, tmp_dest, 0),
         }
      static void
   bi_emit_load_frag_coord_zw(bi_builder *b, bi_index dst, unsigned channel)
   {
      bi_ld_var_special_to(
      b, dst, bi_zero(), BI_REGISTER_FORMAT_F32, BI_SAMPLE_CENTER,
   BI_UPDATE_CLOBBER,
   (channel == 2) ? BI_VARYING_NAME_FRAG_Z : BI_VARYING_NAME_FRAG_W,
   }
      static void
   bi_emit_ld_tile(bi_builder *b, nir_intrinsic_instr *instr)
   {
      bi_index dest = bi_def_index(&instr->def);
   nir_alu_type T = nir_intrinsic_dest_type(instr);
   enum bi_register_format regfmt = bi_reg_fmt_for_nir(T);
   unsigned size = instr->def.bit_size;
            /* Get the render target */
   nir_io_semantics sem = nir_intrinsic_io_semantics(instr);
   unsigned loc = sem.location;
   assert(loc >= FRAG_RESULT_DATA0);
            bi_ld_tile_to(b, dest, bi_pixel_indices(b, rt), bi_coverage(b),
            }
      static void
   bi_emit_intrinsic(bi_builder *b, nir_intrinsic_instr *instr)
   {
      bi_index dst = nir_intrinsic_infos[instr->intrinsic].has_dest
                        switch (instr->intrinsic) {
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_sample:
   case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_at_offset:
      /* handled later via load_vary */
      case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_input:
      if (b->shader->inputs->is_blend)
         else if (stage == MESA_SHADER_FRAGMENT)
         else if (stage == MESA_SHADER_VERTEX)
         else
               case nir_intrinsic_store_output:
      if (stage == MESA_SHADER_FRAGMENT)
         else if (stage == MESA_SHADER_VERTEX)
         else
               case nir_intrinsic_store_combined_output_pan:
      assert(stage == MESA_SHADER_FRAGMENT);
   bi_emit_fragment_out(b, instr);
         case nir_intrinsic_load_ubo:
      bi_emit_load_ubo(b, instr);
         case nir_intrinsic_load_push_constant:
      bi_emit_load_push_constant(b, instr);
         case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
      bi_emit_load(b, instr, BI_SEG_NONE);
         case nir_intrinsic_store_global:
      bi_emit_store(b, instr, BI_SEG_NONE);
         case nir_intrinsic_load_scratch:
      bi_emit_load(b, instr, BI_SEG_TL);
         case nir_intrinsic_store_scratch:
      bi_emit_store(b, instr, BI_SEG_TL);
         case nir_intrinsic_load_shared:
      bi_emit_load(b, instr, BI_SEG_WLS);
         case nir_intrinsic_store_shared:
      bi_emit_store(b, instr, BI_SEG_WLS);
         case nir_intrinsic_barrier:
      if (nir_intrinsic_execution_scope(instr) != SCOPE_NONE) {
      assert(b->shader->stage != MESA_SHADER_FRAGMENT);
   assert(nir_intrinsic_execution_scope(instr) > SCOPE_SUBGROUP &&
            }
   /* Blob doesn't seem to do anything for memory barriers, so no need to
   * check nir_intrinsic_memory_scope().
   */
         case nir_intrinsic_shared_atomic: {
               if (op == nir_atomic_op_xchg) {
      bi_emit_axchg_to(b, dst, bi_src_index(&instr->src[0]), &instr->src[1],
      } else {
                              if (b->shader->arch >= 9) {
      bi_handle_segment(b, &addr, &addr_hi, BI_SEG_WLS, NULL);
      } else {
      addr = bi_seg_add_i64(b, addr, bi_zero(), false, BI_SEG_WLS);
                           bi_split_def(b, &instr->def);
               case nir_intrinsic_global_atomic: {
               if (op == nir_atomic_op_xchg) {
      bi_emit_axchg_to(b, dst, bi_src_index(&instr->src[0]), &instr->src[1],
      } else {
               bi_emit_atomic_i32_to(b, dst, bi_src_index(&instr->src[0]),
               bi_split_def(b, &instr->def);
               case nir_intrinsic_image_texel_address:
      bi_emit_lea_image_to(b, dst, instr);
         case nir_intrinsic_image_load:
      bi_emit_image_load(b, instr);
         case nir_intrinsic_image_store:
      bi_emit_image_store(b, instr);
         case nir_intrinsic_global_atomic_swap:
      bi_emit_acmpxchg_to(b, dst, bi_src_index(&instr->src[0]), &instr->src[1],
         bi_split_def(b, &instr->def);
         case nir_intrinsic_shared_atomic_swap:
      bi_emit_acmpxchg_to(b, dst, bi_src_index(&instr->src[0]), &instr->src[1],
         bi_split_def(b, &instr->def);
         case nir_intrinsic_load_pixel_coord:
      /* Vectorized load of the preloaded i16vec2 */
   bi_mov_i32_to(b, dst, bi_preload(b, 59));
         case nir_intrinsic_load_frag_coord_zw:
      bi_emit_load_frag_coord_zw(b, dst, nir_intrinsic_component(instr));
         case nir_intrinsic_load_converted_output_pan:
      bi_emit_ld_tile(b, instr);
         case nir_intrinsic_discard_if:
      bi_discard_b32(b, bi_src_index(&instr->src[0]));
         case nir_intrinsic_discard:
      bi_discard_f32(b, bi_zero(), bi_zero(), BI_CMPF_EQ);
         case nir_intrinsic_load_sample_positions_pan:
      bi_collect_v2i32_to(b, dst, bi_fau(BIR_FAU_SAMPLE_POS_ARRAY, false),
               case nir_intrinsic_load_sample_mask_in:
      /* r61[0:15] contains the coverage bitmap */
   bi_u16_to_u32_to(b, dst, bi_half(bi_preload(b, 61), false));
         case nir_intrinsic_load_sample_mask:
      bi_mov_i32_to(b, dst, bi_coverage(b));
         case nir_intrinsic_load_sample_id:
      bi_load_sample_id_to(b, dst);
         case nir_intrinsic_load_front_face:
      /* r58 == 0 means primitive is front facing */
   bi_icmp_i32_to(b, dst, bi_preload(b, 58), bi_zero(), BI_CMPF_EQ,
               case nir_intrinsic_load_point_coord:
      bi_ld_var_special_to(b, dst, bi_zero(), BI_REGISTER_FORMAT_F32,
               bi_emit_cached_split_i32(b, dst, 2);
         /* It appears vertex_id is zero-based with Bifrost geometry flows, but
   * not with Valhall's memory-allocation IDVS geometry flow. We only support
   * the new flow on Valhall so this is lowered in NIR.
   */
   case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_vertex_id_zero_base:
      assert(b->shader->malloc_idvs ==
            bi_mov_i32_to(b, dst, bi_vertex_id(b));
         case nir_intrinsic_load_instance_id:
      bi_mov_i32_to(b, dst, bi_instance_id(b));
         case nir_intrinsic_load_subgroup_invocation:
      bi_mov_i32_to(b, dst, bi_fau(BIR_FAU_LANE_ID, false));
         case nir_intrinsic_load_local_invocation_id:
      bi_collect_v3i32_to(b, dst,
                           case nir_intrinsic_load_workgroup_id:
      bi_collect_v3i32_to(b, dst, bi_preload(b, 57), bi_preload(b, 58),
               case nir_intrinsic_load_global_invocation_id:
   case nir_intrinsic_load_global_invocation_id_zero_base:
      bi_collect_v3i32_to(b, dst, bi_preload(b, 60), bi_preload(b, 61),
               case nir_intrinsic_shader_clock:
      bi_ld_gclk_u64_to(b, dst, BI_SOURCE_CYCLE_COUNTER);
   bi_split_def(b, &instr->def);
         default:
      fprintf(stderr, "Unhandled intrinsic %s\n",
               }
      static void
   bi_emit_load_const(bi_builder *b, nir_load_const_instr *instr)
   {
      /* Make sure we've been lowered */
            /* Accumulate all the channels of the constant, as if we did an
   * implicit SEL over them */
            for (unsigned i = 0; i < instr->def.num_components; ++i) {
      unsigned v =
                        }
      static bi_index
   bi_alu_src_index(bi_builder *b, nir_alu_src src, unsigned comps)
   {
               /* the bi_index carries the 32-bit (word) offset separate from the
                     assert(bitsize == 8 || bitsize == 16 || bitsize == 32);
            for (unsigned i = 0; i < comps; ++i) {
               if (i > 0)
                                 /* Compose the subword swizzle with existing (identity) swizzle */
            /* Bigger vectors should have been lowered */
            if (bitsize == 16) {
      unsigned c0 = src.swizzle[0] & 1;
   unsigned c1 = (comps > 1) ? src.swizzle[1] & 1 : c0;
      } else if (bitsize == 8) {
      /* 8-bit vectors not yet supported */
   assert(comps == 1 && "8-bit vectors not supported");
                  }
      static enum bi_round
   bi_nir_round(nir_op op)
   {
      switch (op) {
   case nir_op_fround_even:
         case nir_op_ftrunc:
         case nir_op_fceil:
         case nir_op_ffloor:
         default:
            }
      /* Convenience for lowered transcendentals */
      static bi_index
   bi_fmul_f32(bi_builder *b, bi_index s0, bi_index s1)
   {
         }
      /* Approximate with FRCP_APPROX.f32 and apply a single iteration of
   * Newton-Raphson to improve precision */
      static void
   bi_lower_frcp_32(bi_builder *b, bi_index dst, bi_index s0)
   {
      bi_index x1 = bi_frcp_approx_f32(b, s0);
   bi_index m = bi_frexpm_f32(b, s0, false, false);
   bi_index e = bi_frexpe_f32(b, bi_neg(s0), false, false);
   bi_index t1 = bi_fma_rscale_f32(b, m, bi_neg(x1), bi_imm_f32(1.0), bi_zero(),
            }
      static void
   bi_lower_frsq_32(bi_builder *b, bi_index dst, bi_index s0)
   {
      bi_index x1 = bi_frsq_approx_f32(b, s0);
   bi_index m = bi_frexpm_f32(b, s0, false, true);
   bi_index e = bi_frexpe_f32(b, bi_neg(s0), false, true);
   bi_index t1 = bi_fmul_f32(b, x1, x1);
   bi_index t2 = bi_fma_rscale_f32(b, m, bi_neg(t1), bi_imm_f32(1.0),
            }
      /* More complex transcendentals, see
   * https://gitlab.freedesktop.org/panfrost/mali-isa-docs/-/blob/master/Bifrost.adoc
   * for documentation */
      static void
   bi_lower_fexp2_32(bi_builder *b, bi_index dst, bi_index s0)
   {
      bi_index t1 = bi_temp(b->shader);
   bi_instr *t1_instr = bi_fadd_f32_to(b, t1, s0, bi_imm_u32(0x49400000));
                     bi_instr *a2 = bi_fadd_f32_to(b, bi_temp(b->shader), s0, bi_neg(t2));
            bi_index a1t = bi_fexp_table_u4(b, t1, BI_ADJ_NONE);
   bi_index t3 = bi_isub_u32(b, t1, bi_imm_u32(0x49400000), false);
   bi_index a1i = bi_arshift_i32(b, t3, bi_null(), bi_imm_u8(4));
   bi_index p1 = bi_fma_f32(b, a2->dest[0], bi_imm_u32(0x3d635635),
         bi_index p2 = bi_fma_f32(b, p1, a2->dest[0], bi_imm_u32(0x3f317218));
   bi_index p3 = bi_fmul_f32(b, a2->dest[0], p2);
   bi_instr *x = bi_fma_rscale_f32_to(b, bi_temp(b->shader), p3, a1t, a1t, a1i,
                  bi_instr *max = bi_fmax_f32_to(b, dst, x->dest[0], s0);
      }
      static void
   bi_fexp_32(bi_builder *b, bi_index dst, bi_index s0, bi_index log2_base)
   {
      /* Scale by base, Multiply by 2*24 and convert to integer to get a 8:24
   * fixed-point input */
   bi_index scale = bi_fma_rscale_f32(b, s0, log2_base, bi_negzero(),
         bi_instr *fixed_pt = bi_f32_to_s32_to(b, bi_temp(b->shader), scale);
            /* Compute the result for the fixed-point input, but pass along
   * the floating-point scale for correct NaN propagation */
      }
      static void
   bi_lower_flog2_32(bi_builder *b, bi_index dst, bi_index s0)
   {
      /* s0 = a1 * 2^e, with a1 in [0.75, 1.5) */
   bi_index a1 = bi_frexpm_f32(b, s0, true, false);
   bi_index ei = bi_frexpe_f32(b, s0, true, false);
            /* xt estimates -log(r1), a coarse approximation of log(a1) */
   bi_index r1 = bi_flog_table_f32(b, s0, BI_MODE_RED, BI_PRECISION_NONE);
            /* log(s0) = log(a1 * 2^e) = e + log(a1) = e + log(a1 * r1) -
   * log(r1), so let x1 = e - log(r1) ~= e + xt and x2 = log(a1 * r1),
   * and then log(s0) = x1 + x2 */
            /* Since a1 * r1 is close to 1, x2 = log(a1 * r1) may be computed by
   * polynomial approximation around 1. The series is expressed around
   * 1, so set y = (a1 * r1) - 1.0 */
            /* x2 = log_2(1 + y) = log_e(1 + y) * (1/log_e(2)), so approximate
   * log_e(1 + y) by the Taylor series (lower precision than the blob):
   * y - y^2/2 + O(y^3) = y(1 - y/2) + O(y^3) */
   bi_index loge =
                     /* log(s0) = x1 + x2 */
      }
      static void
   bi_flog2_32(bi_builder *b, bi_index dst, bi_index s0)
   {
      bi_index frexp = bi_frexpe_f32(b, s0, true, false);
   bi_index frexpi = bi_s32_to_f32(b, frexp);
   bi_index add = bi_fadd_lscale_f32(b, bi_imm_f32(-1.0f), s0);
      }
      static void
   bi_lower_fpow_32(bi_builder *b, bi_index dst, bi_index base, bi_index exp)
   {
               if (base.type == BI_INDEX_CONSTANT) {
         } else {
      log2_base = bi_temp(b->shader);
                  }
      static void
   bi_fpow_32(bi_builder *b, bi_index dst, bi_index base, bi_index exp)
   {
               if (base.type == BI_INDEX_CONSTANT) {
         } else {
      log2_base = bi_temp(b->shader);
                  }
      /* Bifrost has extremely coarse tables for approximating sin/cos, accessible as
   * FSIN/COS_TABLE.u6, which multiplies the bottom 6-bits by pi/32 and
   * calculates the results. We use them to calculate sin/cos via a Taylor
   * approximation:
   *
   * f(x + e) = f(x) + e f'(x) + (e^2)/2 f''(x)
   * sin(x + e) = sin(x) + e cos(x) - (e^2)/2 sin(x)
   * cos(x + e) = cos(x) - e sin(x) - (e^2)/2 cos(x)
   */
      #define TWO_OVER_PI  bi_imm_f32(2.0f / 3.14159f)
   #define MPI_OVER_TWO bi_imm_f32(-3.14159f / 2.0)
   #define SINCOS_BIAS  bi_imm_u32(0x49400000)
      static void
   bi_lower_fsincos_32(bi_builder *b, bi_index dst, bi_index s0, bool cos)
   {
      /* bottom 6-bits of result times pi/32 approximately s0 mod 2pi */
            /* Approximate domain error (small) */
   bi_index e = bi_fma_f32(b, bi_fadd_f32(b, x_u6, bi_neg(SINCOS_BIAS)),
            /* Lookup sin(x), cos(x) */
   bi_index sinx = bi_fsin_table_u6(b, x_u6, false);
            /* e^2 / 2 */
   bi_index e2_over_2 =
            /* (-e^2)/2 f''(x) */
   bi_index quadratic =
            /* e f'(x) - (e^2/2) f''(x) */
   bi_instr *I = bi_fma_f32_to(b, bi_temp(b->shader), e,
                  /* f(x) + e f'(x) - (e^2/2) f''(x) */
      }
      /*
   * The XOR lane op is useful for derivative calculations, but not all Bifrost
   * implementations have it. Add a safe helper that uses the hardware
   * functionality when available and lowers where unavailable.
   */
   static bi_index
   bi_clper_xor(bi_builder *b, bi_index s0, bi_index s1)
   {
      if (!(b->shader->quirks & BIFROST_LIMITED_CLPER)) {
      return bi_clper_i32(b, s0, s1, BI_INACTIVE_RESULT_ZERO, BI_LANE_OP_XOR,
               bi_index lane_id = bi_fau(BIR_FAU_LANE_ID, false);
   bi_index lane = bi_lshift_xor_i32(b, lane_id, s1, bi_imm_u8(0));
      }
      static enum bi_cmpf
   bi_translate_cmpf(nir_op op)
   {
      switch (op) {
   case nir_op_ieq8:
   case nir_op_ieq16:
   case nir_op_ieq32:
   case nir_op_feq16:
   case nir_op_feq32:
            case nir_op_ine8:
   case nir_op_ine16:
   case nir_op_ine32:
   case nir_op_fneu16:
   case nir_op_fneu32:
            case nir_op_ilt8:
   case nir_op_ilt16:
   case nir_op_ilt32:
   case nir_op_flt16:
   case nir_op_flt32:
   case nir_op_ult8:
   case nir_op_ult16:
   case nir_op_ult32:
            case nir_op_ige8:
   case nir_op_ige16:
   case nir_op_ige32:
   case nir_op_fge16:
   case nir_op_fge32:
   case nir_op_uge8:
   case nir_op_uge16:
   case nir_op_uge32:
            default:
            }
      static bool
   bi_nir_is_replicated(nir_alu_src *src)
   {
      for (unsigned i = 1; i < nir_src_num_components(src->src); ++i) {
      if (src->swizzle[0] == src->swizzle[i])
                  }
      static void
   bi_emit_alu(bi_builder *b, nir_alu_instr *instr)
   {
      bi_index dst = bi_def_index(&instr->def);
   unsigned srcs = nir_op_infos[instr->op].num_inputs;
   unsigned sz = instr->def.bit_size;
   unsigned comps = instr->def.num_components;
            /* Indicate scalarness */
   if (sz == 16 && comps == 1)
            /* First, match against the various moves in NIR. These are
   * special-cased because they can operate on vectors even after
   * lowering ALU to scalar. For Bifrost, bi_alu_src_index assumes the
   * instruction is no "bigger" than SIMD-within-a-register. These moves
            switch (instr->op) {
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_vec8:
   case nir_op_vec16: {
      bi_index unoffset_srcs[16] = {bi_null()};
            for (unsigned i = 0; i < srcs; ++i) {
      unoffset_srcs[i] = bi_src_index(&instr->src[i].src);
               bi_make_vec_to(b, dst, unoffset_srcs, channels, srcs, sz);
               case nir_op_unpack_32_2x16: {
      /* Should have been scalarized */
            bi_index vec = bi_src_index(&instr->src[0].src);
            bi_mov_i32_to(b, dst, bi_extract(b, vec, chan));
               case nir_op_unpack_64_2x32_split_x: {
      unsigned chan = (instr->src[0].swizzle[0] * 2) + 0;
   bi_mov_i32_to(b, dst,
                     case nir_op_unpack_64_2x32_split_y: {
      unsigned chan = (instr->src[0].swizzle[0] * 2) + 1;
   bi_mov_i32_to(b, dst,
                     case nir_op_pack_64_2x32_split:
      bi_collect_v2i32_to(b, dst,
                     bi_extract(b, bi_src_index(&instr->src[0].src),
         case nir_op_pack_64_2x32:
      bi_collect_v2i32_to(b, dst,
                     bi_extract(b, bi_src_index(&instr->src[0].src),
         case nir_op_pack_uvec2_to_uint: {
               assert(sz == 32 && src_sz == 32);
   bi_mkvec_v2i16_to(
      b, dst, bi_half(bi_extract(b, src, instr->src[0].swizzle[0]), false),
                  case nir_op_pack_uvec4_to_uint: {
               assert(sz == 32 && src_sz == 32);
   bi_mkvec_v4i8_to(
      b, dst, bi_byte(bi_extract(b, src, instr->src[0].swizzle[0]), 0),
   bi_byte(bi_extract(b, src, instr->src[0].swizzle[1]), 0),
   bi_byte(bi_extract(b, src, instr->src[0].swizzle[2]), 0),
                  case nir_op_mov: {
      bi_index idx = bi_src_index(&instr->src[0].src);
            unsigned channels[4] = {
      comps > 0 ? instr->src[0].swizzle[0] : 0,
   comps > 1 ? instr->src[0].swizzle[1] : 0,
   comps > 2 ? instr->src[0].swizzle[2] : 0,
               bi_make_vec_to(b, dst, unoffset_srcs, channels, comps, src_sz);
               case nir_op_pack_32_2x16: {
               bi_index idx = bi_src_index(&instr->src[0].src);
            unsigned channels[2] = {instr->src[0].swizzle[0],
            bi_make_vec_to(b, dst, unoffset_srcs, channels, 2, 16);
               case nir_op_f2f16:
   case nir_op_f2f16_rtz:
   case nir_op_f2f16_rtne: {
      assert(src_sz == 32);
   bi_index idx = bi_src_index(&instr->src[0].src);
   bi_index s0 = bi_extract(b, idx, instr->src[0].swizzle[0]);
   bi_index s1 =
                     /* Override rounding if explicitly requested. Otherwise, the
   * default rounding mode is selected by the builder. Depending
   * on the float controls required by the shader, the default
   * mode may not be nearest-even.
   */
   if (instr->op == nir_op_f2f16_rtz)
         else if (instr->op == nir_op_f2f16_rtne)
                        /* Vectorized downcasts */
   case nir_op_u2u16:
   case nir_op_i2i16: {
      if (!(src_sz == 32 && comps == 2))
            bi_index idx = bi_src_index(&instr->src[0].src);
   bi_index s0 = bi_extract(b, idx, instr->src[0].swizzle[0]);
            bi_mkvec_v2i16_to(b, dst, bi_half(s0, false), bi_half(s1, false));
               /* While we do not have a direct V2U32_TO_V2F16 instruction, lowering to
   * MKVEC.v2i16 + V2U16_TO_V2F16 is more efficient on Bifrost than
   * scalarizing due to scheduling (equal cost on Valhall). Additionally
   * if the source is replicated the MKVEC.v2i16 can be optimized out.
   */
   case nir_op_u2f16:
   case nir_op_i2f16: {
      if (!(src_sz == 32 && comps == 2))
            nir_alu_src *src = &instr->src[0];
   bi_index idx = bi_src_index(&src->src);
   bi_index s0 = bi_extract(b, idx, src->swizzle[0]);
            bi_index t =
      (src->swizzle[0] == src->swizzle[1])
               if (instr->op == nir_op_u2f16)
         else
                        case nir_op_i2i8:
   case nir_op_u2u8: {
      /* Acts like an 8-bit swizzle */
   bi_index idx = bi_src_index(&instr->src[0].src);
   unsigned factor = src_sz / 8;
            for (unsigned i = 0; i < comps; ++i)
            bi_make_vec_to(b, dst, &idx, chan, comps, 8);
               case nir_op_b32csel: {
      if (sz != 16)
            /* We allow vectorizing b32csel(cond, A, B) which can be
   * translated as MUX.v2i16, even though cond is a 32-bit vector.
   *
   * If the source condition vector is replicated, we can use
   * MUX.v2i16 directly, letting each component use the
   * corresponding half of the 32-bit source. NIR uses 0/~0
   * booleans so that's guaranteed to work (that is, 32-bit NIR
   * booleans are 16-bit replicated).
   *
   * If we're not replicated, we use the same trick but must
   * insert a MKVEC.v2i16 first to convert down to 16-bit.
   */
   bi_index idx = bi_src_index(&instr->src[0].src);
   bi_index s0 = bi_extract(b, idx, instr->src[0].swizzle[0]);
   bi_index s1 = bi_alu_src_index(b, instr->src[1], comps);
            if (!bi_nir_is_replicated(&instr->src[0])) {
      s0 = bi_mkvec_v2i16(
      b, bi_half(s0, false),
            bi_mux_v2i16_to(b, dst, s2, s1, s0, BI_MUX_INT_ZERO);
               default:
                  bi_index s0 =
         bi_index s1 =
         bi_index s2 =
            switch (instr->op) {
   case nir_op_ffma:
      bi_fma_to(b, sz, dst, s0, s1, s2);
         case nir_op_fmul:
      bi_fma_to(b, sz, dst, s0, s1, bi_negzero());
         case nir_op_fadd:
      bi_fadd_to(b, sz, dst, s0, s1);
         case nir_op_fsat: {
      bi_instr *I = bi_fclamp_to(b, sz, dst, s0);
   I->clamp = BI_CLAMP_CLAMP_0_1;
               case nir_op_fsat_signed_mali: {
      bi_instr *I = bi_fclamp_to(b, sz, dst, s0);
   I->clamp = BI_CLAMP_CLAMP_M1_1;
               case nir_op_fclamp_pos_mali: {
      bi_instr *I = bi_fclamp_to(b, sz, dst, s0);
   I->clamp = BI_CLAMP_CLAMP_0_INF;
               case nir_op_fneg:
      bi_fabsneg_to(b, sz, dst, bi_neg(s0));
         case nir_op_fabs:
      bi_fabsneg_to(b, sz, dst, bi_abs(s0));
         case nir_op_fsin:
      bi_lower_fsincos_32(b, dst, s0, false);
         case nir_op_fcos:
      bi_lower_fsincos_32(b, dst, s0, true);
         case nir_op_fexp2:
               if (b->shader->quirks & BIFROST_NO_FP32_TRANSCENDENTALS)
         else
                  case nir_op_flog2:
               if (b->shader->quirks & BIFROST_NO_FP32_TRANSCENDENTALS)
         else
                  case nir_op_fpow:
               if (b->shader->quirks & BIFROST_NO_FP32_TRANSCENDENTALS)
         else
                  case nir_op_frexp_exp:
      bi_frexpe_to(b, sz, dst, s0, false, false);
         case nir_op_frexp_sig:
      bi_frexpm_to(b, sz, dst, s0, false, false);
         case nir_op_ldexp:
      bi_ldexp_to(b, sz, dst, s0, s1);
         case nir_op_b8csel:
      bi_mux_v4i8_to(b, dst, s2, s1, s0, BI_MUX_INT_ZERO);
         case nir_op_b16csel:
      bi_mux_v2i16_to(b, dst, s2, s1, s0, BI_MUX_INT_ZERO);
         case nir_op_b32csel:
      bi_mux_i32_to(b, dst, s2, s1, s0, BI_MUX_INT_ZERO);
         case nir_op_extract_u8:
   case nir_op_extract_i8: {
      assert(comps == 1 && "should be scalarized");
   assert((src_sz == 16 || src_sz == 32) && "should be lowered");
            if (s0.swizzle == BI_SWIZZLE_H11) {
      assert(byte < 2);
      } else if (s0.swizzle != BI_SWIZZLE_H01) {
                                    if (instr->op == nir_op_extract_i8)
         else
                     case nir_op_extract_u16:
   case nir_op_extract_i16: {
      assert(comps == 1 && "should be scalarized");
   assert(src_sz == 32 && "should be lowered");
   unsigned half = nir_alu_src_as_uint(instr->src[1]);
            if (instr->op == nir_op_extract_i16)
         else
                     case nir_op_insert_u16: {
      assert(comps == 1 && "should be scalarized");
   unsigned half = nir_alu_src_as_uint(instr->src[1]);
            if (half == 0)
         else
                     case nir_op_ishl:
      bi_lshift_or_to(b, sz, dst, s0, bi_zero(), bi_byte(s1, 0));
      case nir_op_ushr:
      bi_rshift_or_to(b, sz, dst, s0, bi_zero(), bi_byte(s1, 0), false);
         case nir_op_ishr:
      if (b->shader->arch >= 9)
         else
               case nir_op_imin:
   case nir_op_umin:
      bi_csel_to(b, nir_op_infos[instr->op].input_types[0], sz, dst, s0, s1, s0,
               case nir_op_imax:
   case nir_op_umax:
      bi_csel_to(b, nir_op_infos[instr->op].input_types[0], sz, dst, s0, s1, s0,
               case nir_op_fddx_must_abs_mali:
   case nir_op_fddy_must_abs_mali: {
      bi_index bit = bi_imm_u32(instr->op == nir_op_fddx_must_abs_mali ? 1 : 2);
   bi_index adjacent = bi_clper_xor(b, s0, bit);
   bi_fadd_to(b, sz, dst, adjacent, bi_neg(s0));
               case nir_op_fddx:
   case nir_op_fddy:
   case nir_op_fddx_coarse:
   case nir_op_fddy_coarse:
   case nir_op_fddx_fine:
   case nir_op_fddy_fine: {
      unsigned axis;
   switch (instr->op) {
   case nir_op_fddx:
   case nir_op_fddx_coarse:
   case nir_op_fddx_fine:
      axis = 1;
      case nir_op_fddy:
   case nir_op_fddy_coarse:
   case nir_op_fddy_fine:
      axis = 2;
      default:
                  bi_index lane1, lane2;
   switch (instr->op) {
   case nir_op_fddx:
   case nir_op_fddx_fine:
   case nir_op_fddy:
   case nir_op_fddy_fine:
                     lane2 = bi_iadd_u32(b, lane1, bi_imm_u32(axis), false);
      case nir_op_fddx_coarse:
   case nir_op_fddy_coarse:
      lane1 = bi_imm_u32(0);
   lane2 = bi_imm_u32(axis);
      default:
                           if (b->shader->quirks & BIFROST_LIMITED_CLPER) {
      left = bi_clper_old_i32(b, s0, lane1);
      } else {
                     right = bi_clper_i32(b, s0, lane2, BI_INACTIVE_RESULT_ZERO,
               bi_fadd_to(b, sz, dst, right, bi_neg(left));
               case nir_op_f2f32:
      bi_f16_to_f32_to(b, dst, s0);
         case nir_op_fquantize2f16: {
      bi_instr *f16 = bi_v2f32_to_v2f16_to(b, bi_temp(b->shader), s0, s0);
            f16->ftz = f32->ftz = true;
               case nir_op_f2i32:
      if (src_sz == 32)
         else
               /* Note 32-bit sources => no vectorization, so 32-bit works */
   case nir_op_f2u16:
      if (src_sz == 32)
         else
               case nir_op_f2i16:
      if (src_sz == 32)
         else
               case nir_op_f2u32:
      if (src_sz == 32)
         else
               case nir_op_u2f16:
      if (src_sz == 32)
         else if (src_sz == 16)
         else if (src_sz == 8)
               case nir_op_u2f32:
      if (src_sz == 32)
         else if (src_sz == 16)
         else
               case nir_op_i2f16:
      if (src_sz == 32)
         else if (src_sz == 16)
         else if (src_sz == 8)
               case nir_op_i2f32:
               if (src_sz == 32)
         else if (src_sz == 16)
         else if (src_sz == 8)
               case nir_op_i2i32:
               if (src_sz == 32)
         else if (src_sz == 16)
         else if (src_sz == 8)
               case nir_op_u2u32:
               if (src_sz == 32)
         else if (src_sz == 16)
         else if (src_sz == 8)
                  case nir_op_i2i16:
               if (src_sz == 8)
         else
               case nir_op_u2u16:
               if (src_sz == 8)
         else
               case nir_op_b2i8:
   case nir_op_b2i16:
   case nir_op_b2i32:
      bi_mux_to(b, sz, dst, bi_imm_u8(0), bi_imm_uintN(1, sz), s0,
               case nir_op_ieq8:
   case nir_op_ine8:
   case nir_op_ilt8:
   case nir_op_ige8:
   case nir_op_ieq16:
   case nir_op_ine16:
   case nir_op_ilt16:
   case nir_op_ige16:
   case nir_op_ieq32:
   case nir_op_ine32:
   case nir_op_ilt32:
   case nir_op_ige32:
      bi_icmp_to(b, nir_type_int, sz, dst, s0, s1, bi_translate_cmpf(instr->op),
               case nir_op_ult8:
   case nir_op_uge8:
   case nir_op_ult16:
   case nir_op_uge16:
   case nir_op_ult32:
   case nir_op_uge32:
      bi_icmp_to(b, nir_type_uint, sz, dst, s0, s1,
               case nir_op_feq32:
   case nir_op_feq16:
   case nir_op_flt32:
   case nir_op_flt16:
   case nir_op_fge32:
   case nir_op_fge16:
   case nir_op_fneu32:
   case nir_op_fneu16:
      bi_fcmp_to(b, sz, dst, s0, s1, bi_translate_cmpf(instr->op),
               case nir_op_fround_even:
   case nir_op_fceil:
   case nir_op_ffloor:
   case nir_op_ftrunc:
      bi_fround_to(b, sz, dst, s0, bi_nir_round(instr->op));
         case nir_op_fmin:
      bi_fmin_to(b, sz, dst, s0, s1);
         case nir_op_fmax:
      bi_fmax_to(b, sz, dst, s0, s1);
         case nir_op_iadd:
      bi_iadd_to(b, nir_type_int, sz, dst, s0, s1, false);
         case nir_op_iadd_sat:
      bi_iadd_to(b, nir_type_int, sz, dst, s0, s1, true);
         case nir_op_uadd_sat:
      bi_iadd_to(b, nir_type_uint, sz, dst, s0, s1, true);
         case nir_op_ihadd:
      bi_hadd_to(b, nir_type_int, sz, dst, s0, s1, BI_ROUND_RTN);
         case nir_op_irhadd:
      bi_hadd_to(b, nir_type_int, sz, dst, s0, s1, BI_ROUND_RTP);
         case nir_op_uhadd:
      bi_hadd_to(b, nir_type_uint, sz, dst, s0, s1, BI_ROUND_RTN);
         case nir_op_urhadd:
      bi_hadd_to(b, nir_type_uint, sz, dst, s0, s1, BI_ROUND_RTP);
         case nir_op_ineg:
      bi_isub_to(b, nir_type_int, sz, dst, bi_zero(), s0, false);
         case nir_op_isub:
      bi_isub_to(b, nir_type_int, sz, dst, s0, s1, false);
         case nir_op_isub_sat:
      bi_isub_to(b, nir_type_int, sz, dst, s0, s1, true);
         case nir_op_usub_sat:
      bi_isub_to(b, nir_type_uint, sz, dst, s0, s1, true);
         case nir_op_imul:
      bi_imul_to(b, sz, dst, s0, s1);
         case nir_op_iabs:
      bi_iabs_to(b, sz, dst, s0);
         case nir_op_iand:
      bi_lshift_and_to(b, sz, dst, s0, s1, bi_imm_u8(0));
         case nir_op_ior:
      bi_lshift_or_to(b, sz, dst, s0, s1, bi_imm_u8(0));
         case nir_op_ixor:
      bi_lshift_xor_to(b, sz, dst, s0, s1, bi_imm_u8(0));
         case nir_op_inot:
      bi_lshift_or_to(b, sz, dst, bi_zero(), bi_not(s0), bi_imm_u8(0));
         case nir_op_frsq:
      if (sz == 32 && b->shader->quirks & BIFROST_NO_FP32_TRANSCENDENTALS)
         else
               case nir_op_frcp:
      if (sz == 32 && b->shader->quirks & BIFROST_NO_FP32_TRANSCENDENTALS)
         else
               case nir_op_uclz:
      bi_clz_to(b, sz, dst, s0, false);
         case nir_op_bit_count:
      assert(sz == 32 && src_sz == 32 && "should've been lowered");
   bi_popcount_i32_to(b, dst, s0);
         case nir_op_bitfield_reverse:
      assert(sz == 32 && src_sz == 32 && "should've been lowered");
   bi_bitrev_i32_to(b, dst, s0);
         case nir_op_ufind_msb: {
               if (sz == 8)
         else if (sz == 16)
            bi_isub_u32_to(b, dst, bi_imm_u32(src_sz - 1), clz, false);
               default:
      fprintf(stderr, "Unhandled ALU op %s\n", nir_op_infos[instr->op].name);
         }
      /* Returns dimension with 0 special casing cubemaps. Shamelessly copied from
   * Midgard */
   static unsigned
   bifrost_tex_format(enum glsl_sampler_dim dim)
   {
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
   case GLSL_SAMPLER_DIM_BUF:
            case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_MS:
   case GLSL_SAMPLER_DIM_EXTERNAL:
   case GLSL_SAMPLER_DIM_RECT:
            case GLSL_SAMPLER_DIM_3D:
            case GLSL_SAMPLER_DIM_CUBE:
            default:
      DBG("Unknown sampler dim type\n");
   assert(0);
         }
      static enum bi_dimension
   valhall_tex_dimension(enum glsl_sampler_dim dim)
   {
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
   case GLSL_SAMPLER_DIM_BUF:
            case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_MS:
   case GLSL_SAMPLER_DIM_EXTERNAL:
   case GLSL_SAMPLER_DIM_RECT:
            case GLSL_SAMPLER_DIM_3D:
            case GLSL_SAMPLER_DIM_CUBE:
            default:
            }
      static enum bifrost_texture_format_full
   bi_texture_format(nir_alu_type T, enum bi_clamp clamp)
   {
      switch (T) {
   case nir_type_float16:
         case nir_type_float32:
         case nir_type_uint16:
         case nir_type_int16:
         case nir_type_uint32:
         case nir_type_int32:
         default:
            }
      /* Array indices are specified as 32-bit uints, need to convert. In .z component
   * from NIR */
   static bi_index
   bi_emit_texc_array_index(bi_builder *b, bi_index idx, nir_alu_type T)
   {
      /* For (u)int we can just passthrough */
   nir_alu_type base = nir_alu_type_get_base_type(T);
   if (base == nir_type_int || base == nir_type_uint)
            /* Otherwise we convert */
            /* OpenGL ES 3.2 specification section 8.14.2 ("Coordinate Wrapping and
   * Texel Selection") defines the layer to be taken from clamp(RNE(r),
   * 0, dt - 1). So we use round RTE, clamping is handled at the data
            bi_instr *I = bi_f32_to_u32_to(b, bi_temp(b->shader), idx);
   I->round = BI_ROUND_NONE;
      }
      /* TEXC's explicit and bias LOD modes requires the LOD to be transformed to a
   * 16-bit 8:8 fixed-point format. We lower as:
   *
   * F32_TO_S32(clamp(x, -16.0, +16.0) * 256.0) & 0xFFFF =
   * MKVEC(F32_TO_S32(clamp(x * 1.0/16.0, -1.0, 1.0) * (16.0 * 256.0)), #0)
   */
      static bi_index
   bi_emit_texc_lod_88(bi_builder *b, bi_index lod, bool fp16)
   {
      /* Precompute for constant LODs to avoid general constant folding */
   if (lod.type == BI_INDEX_CONSTANT) {
      uint32_t raw = lod.value;
   float x = fp16 ? _mesa_half_to_float(raw) : uif(raw);
   int32_t s32 = CLAMP(x, -16.0f, 16.0f) * 256.0f;
               /* Sort of arbitrary. Must be less than 128.0, greater than or equal to
   * the max LOD (16 since we cap at 2^16 texture dimensions), and
   * preferably small to minimize precision loss */
            bi_instr *fsat =
      bi_fma_f32_to(b, bi_temp(b->shader), fp16 ? bi_half(lod, false) : lod,
                  bi_index fmul =
            return bi_mkvec_v2i16(b, bi_half(bi_f32_to_s32(b, fmul), false),
      }
      /* FETCH takes a 32-bit staging register containing the LOD as an integer in
   * the bottom 16-bits and (if present) the cube face index in the top 16-bits.
   * TODO: Cube face.
   */
      static bi_index
   bi_emit_texc_lod_cube(bi_builder *b, bi_index lod)
   {
         }
      /* The hardware specifies texel offsets and multisample indices together as a
   * u8vec4 <offset, ms index>. By default all are zero, so if have either a
   * nonzero texel offset or a nonzero multisample index, we build a u8vec4 with
   * the bits we need and return that to be passed as a staging register. Else we
   * return 0 to avoid allocating a data register when everything is zero. */
      static bi_index
   bi_emit_texc_offset_ms_index(bi_builder *b, nir_tex_instr *instr)
   {
               int offs_idx = nir_tex_instr_src_index(instr, nir_tex_src_offset);
   if (offs_idx >= 0 && (!nir_src_is_const(instr->src[offs_idx].src) ||
            unsigned nr = nir_src_num_components(instr->src[offs_idx].src);
   bi_index idx = bi_src_index(&instr->src[offs_idx].src);
   dest = bi_mkvec_v4i8(
      b, (nr > 0) ? bi_byte(bi_extract(b, idx, 0), 0) : bi_imm_u8(0),
   (nr > 1) ? bi_byte(bi_extract(b, idx, 1), 0) : bi_imm_u8(0),
   (nr > 2) ? bi_byte(bi_extract(b, idx, 2), 0) : bi_imm_u8(0),
            int ms_idx = nir_tex_instr_src_index(instr, nir_tex_src_ms_index);
   if (ms_idx >= 0 && (!nir_src_is_const(instr->src[ms_idx].src) ||
            dest = bi_lshift_or_i32(b, bi_src_index(&instr->src[ms_idx].src), dest,
                  }
      /*
   * Valhall specifies specifies texel offsets, multisample indices, and (for
   * fetches) LOD together as a u8vec4 <offset.xyz, LOD>, where the third
   * component is either offset.z or multisample index depending on context. Build
   * this register.
   */
   static bi_index
   bi_emit_valhall_offsets(bi_builder *b, nir_tex_instr *instr)
   {
               int offs_idx = nir_tex_instr_src_index(instr, nir_tex_src_offset);
   int ms_idx = nir_tex_instr_src_index(instr, nir_tex_src_ms_index);
            /* Components 0-2: offsets */
   if (offs_idx >= 0 && (!nir_src_is_const(instr->src[offs_idx].src) ||
            unsigned nr = nir_src_num_components(instr->src[offs_idx].src);
            /* No multisample index with 3D */
            /* Zero extend the Z byte so we can use it with MKVEC.v2i8 */
   bi_index z = (nr > 2)
                        dest = bi_mkvec_v2i8(
      b, (nr > 0) ? bi_byte(bi_extract(b, idx, 0), 0) : bi_imm_u8(0),
            /* Component 2: multisample index */
   if (ms_idx >= 0 && (!nir_src_is_const(instr->src[ms_idx].src) ||
                        /* Component 3: 8-bit LOD */
   if (lod_idx >= 0 &&
      (!nir_src_is_const(instr->src[lod_idx].src) ||
   nir_src_as_uint(instr->src[lod_idx].src) != 0) &&
   nir_tex_instr_src_type(instr, lod_idx) != nir_type_float) {
   dest = bi_lshift_or_i32(b, bi_src_index(&instr->src[lod_idx].src), dest,
                  }
      static void
   bi_emit_cube_coord(bi_builder *b, bi_index coord, bi_index *face, bi_index *s,
         {
      /* Compute max { |x|, |y|, |z| } */
   bi_index maxxyz = bi_temp(b->shader);
            bi_index cx = bi_extract(b, coord, 0), cy = bi_extract(b, coord, 1),
            /* Use a pseudo op on Bifrost due to tuple restrictions */
   if (b->shader->arch <= 8) {
         } else {
      bi_cubeface1_to(b, maxxyz, cx, cy, cz);
               /* Select coordinates */
   bi_index ssel =
         bi_index tsel =
            /* The OpenGL ES specification requires us to transform an input vector
   * (x, y, z) to the coordinate, given the selected S/T:
   *
   * (1/2 ((s / max{x,y,z}) + 1), 1/2 ((t / max{x, y, z}) + 1))
   *
   * We implement (s shown, t similar) in a form friendlier to FMA
   * instructions, and clamp coordinates at the end for correct
   * NaN/infinity handling:
   *
   * fsat(s * (0.5 * (1 / max{x, y, z})) + 0.5)
   *
   * Take the reciprocal of max{x, y, z}
   */
            /* Calculate 0.5 * (1.0 / max{x, y, z}) */
            /* Transform the coordinates */
   *s = bi_temp(b->shader);
            bi_instr *S = bi_fma_f32_to(b, *s, fma1, ssel, bi_imm_f32(0.5f));
            S->clamp = BI_CLAMP_CLAMP_0_1;
      }
      /* Emits a cube map descriptor, returning lower 32-bits and putting upper
   * 32-bits in passed pointer t. The packing of the face with the S coordinate
   * exploits the redundancy of floating points with the range restriction of
   * CUBEFACE output.
   *
   *     struct cube_map_descriptor {
   *         float s : 29;
   *         unsigned face : 3;
   *         float t : 32;
   *     }
   *
   * Since the cube face index is preshifted, this is easy to pack with a bitwise
   * MUX.i32 and a fixed mask, selecting the lower bits 29 from s and the upper 3
   * bits from face.
   */
      static bi_index
   bi_emit_texc_cube_coord(bi_builder *b, bi_index coord, bi_index *t)
   {
      bi_index face, s;
   bi_emit_cube_coord(b, coord, &face, &s, t);
   bi_index mask = bi_imm_u32(BITFIELD_MASK(29));
      }
      /* Map to the main texture op used. Some of these (txd in particular) will
   * lower to multiple texture ops with different opcodes (GRDESC_DER + TEX in
   * sequence). We assume that lowering is handled elsewhere.
   */
      static enum bifrost_tex_op
   bi_tex_op(nir_texop op)
   {
      switch (op) {
   case nir_texop_tex:
   case nir_texop_txb:
   case nir_texop_txl:
   case nir_texop_txd:
         case nir_texop_txf:
   case nir_texop_txf_ms:
   case nir_texop_tg4:
         case nir_texop_txs:
   case nir_texop_lod:
   case nir_texop_query_levels:
   case nir_texop_texture_samples:
   case nir_texop_samples_identical:
         default:
            }
      /* Data registers required by texturing in the order they appear. All are
   * optional, the texture operation descriptor determines which are present.
   * Note since 3D arrays are not permitted at an API level, Z_COORD and
   * ARRAY/SHADOW are exlusive, so TEXC in practice reads at most 8 registers */
      enum bifrost_tex_dreg {
      BIFROST_TEX_DREG_Z_COORD = 0,
   BIFROST_TEX_DREG_Y_DELTAS = 1,
   BIFROST_TEX_DREG_LOD = 2,
   BIFROST_TEX_DREG_GRDESC_HI = 3,
   BIFROST_TEX_DREG_SHADOW = 4,
   BIFROST_TEX_DREG_ARRAY = 5,
   BIFROST_TEX_DREG_OFFSETMS = 6,
   BIFROST_TEX_DREG_SAMPLER = 7,
   BIFROST_TEX_DREG_TEXTURE = 8,
      };
      static void
   bi_emit_texc(bi_builder *b, nir_tex_instr *instr)
   {
      struct bifrost_texture_operation desc = {
      .op = bi_tex_op(instr->op),
   .offset_or_bias_disable = false, /* TODO */
   .shadow_or_clamp_disable = instr->is_shadow,
   .array = instr->is_array,
   .dimension = bifrost_tex_format(instr->sampler_dim),
   .format = bi_texture_format(instr->dest_type | instr->def.bit_size,
                     switch (desc.op) {
   case BIFROST_TEX_OP_TEX:
      desc.lod_or_fetch = BIFROST_LOD_MODE_COMPUTE;
      case BIFROST_TEX_OP_FETCH:
      desc.lod_or_fetch = (enum bifrost_lod_mode)(
      instr->op == nir_texop_tg4
      ? BIFROST_TEXTURE_FETCH_GATHER4_R + instr->component
      default:
                  /* 32-bit indices to be allocated as consecutive staging registers */
   bi_index dregs[BIFROST_TEX_DREG_COUNT] = {};
            for (unsigned i = 0; i < instr->num_srcs; ++i) {
      bi_index index = bi_src_index(&instr->src[i].src);
   unsigned sz = nir_src_bit_size(instr->src[i].src);
   unsigned components = nir_src_num_components(instr->src[i].src);
   ASSERTED nir_alu_type base = nir_tex_instr_src_type(instr, i);
            switch (instr->src[i].src_type) {
   case nir_tex_src_coord:
      if (instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
         } else {
      /* Copy XY (for 2D+) or XX (for 1D) */
                           if (components == 3 && !desc.array) {
      /* 3D */
                  if (desc.array) {
      dregs[BIFROST_TEX_DREG_ARRAY] = bi_emit_texc_array_index(
                     case nir_tex_src_lod:
      if (desc.op == BIFROST_TEX_OP_TEX &&
      nir_src_is_const(instr->src[i].src) &&
   nir_src_as_uint(instr->src[i].src) == 0) {
                        assert(sz == 16 || sz == 32);
   dregs[BIFROST_TEX_DREG_LOD] =
            } else {
      assert(desc.op == BIFROST_TEX_OP_FETCH);
                                    case nir_tex_src_bias:
      /* Upper 16-bits interpreted as a clamp, leave zero */
   assert(desc.op == BIFROST_TEX_OP_TEX);
   assert(base == nir_type_float);
   assert(sz == 16 || sz == 32);
   dregs[BIFROST_TEX_DREG_LOD] = bi_emit_texc_lod_88(b, index, sz == 16);
               case nir_tex_src_ms_index:
   case nir_tex_src_offset:
                     dregs[BIFROST_TEX_DREG_OFFSETMS] =
         if (!bi_is_equiv(dregs[BIFROST_TEX_DREG_OFFSETMS], bi_zero()))
               case nir_tex_src_comparator:
                  case nir_tex_src_texture_offset:
                  case nir_tex_src_sampler_offset:
                  default:
                     if (desc.op == BIFROST_TEX_OP_FETCH &&
      bi_is_null(dregs[BIFROST_TEX_DREG_LOD])) {
                        bool direct_tex = bi_is_null(dregs[BIFROST_TEX_DREG_TEXTURE]);
   bool direct_samp = bi_is_null(dregs[BIFROST_TEX_DREG_SAMPLER]);
                     if (desc.immediate_indices) {
      desc.sampler_index_or_mode = instr->sampler_index;
      } else {
               if (direct && instr->sampler_index == instr->texture_index) {
      mode = BIFROST_INDEX_IMMEDIATE_SHARED;
      } else if (direct) {
      mode = BIFROST_INDEX_IMMEDIATE_SAMPLER;
   desc.index = instr->sampler_index;
   dregs[BIFROST_TEX_DREG_TEXTURE] =
      } else if (direct_tex) {
      assert(!direct_samp);
   mode = BIFROST_INDEX_IMMEDIATE_TEXTURE;
      } else if (direct_samp) {
      assert(!direct_tex);
   mode = BIFROST_INDEX_IMMEDIATE_SAMPLER;
      } else {
                  mode |= (BIFROST_TEXTURE_OPERATION_SINGLE << 2);
               /* Allocate staging registers contiguously by compacting the array. */
            for (unsigned i = 0; i < ARRAY_SIZE(dregs); ++i) {
      if (!bi_is_null(dregs[i]))
                        bi_index sr = sr_count ? bi_temp(b->shader) : bi_null();
            if (sr_count)
            uint32_t desc_u = 0;
   memcpy(&desc_u, &desc, sizeof(desc_u));
   bi_instr *I =
      bi_texc_to(b, dst, sr, cx, cy, bi_imm_u32(desc_u),
               bi_index w[4] = {bi_null(), bi_null(), bi_null(), bi_null()};
   bi_emit_split_i32(b, w, dst, res_size);
   bi_emit_collect_to(b, bi_def_index(&instr->def), w,
      }
      /* Staging registers required by texturing in the order they appear (Valhall) */
      enum valhall_tex_sreg {
      VALHALL_TEX_SREG_X_COORD = 0,
   VALHALL_TEX_SREG_Y_COORD = 1,
   VALHALL_TEX_SREG_Z_COORD = 2,
   VALHALL_TEX_SREG_Y_DELTAS = 3,
   VALHALL_TEX_SREG_ARRAY = 4,
   VALHALL_TEX_SREG_SHADOW = 5,
   VALHALL_TEX_SREG_OFFSETMS = 6,
   VALHALL_TEX_SREG_LOD = 7,
   VALHALL_TEX_SREG_GRDESC = 8,
      };
      static void
   bi_emit_tex_valhall(bi_builder *b, nir_tex_instr *instr)
   {
      bool explicit_offset = false;
            bool has_lod_mode = (instr->op == nir_texop_tex) ||
                  /* 32-bit indices to be allocated as consecutive staging registers */
   bi_index sregs[VALHALL_TEX_SREG_COUNT] = {};
   bi_index sampler = bi_imm_u32(instr->sampler_index);
   bi_index texture = bi_imm_u32(instr->texture_index);
            for (unsigned i = 0; i < instr->num_srcs; ++i) {
      bi_index index = bi_src_index(&instr->src[i].src);
   unsigned sz = nir_src_bit_size(instr->src[i].src);
            switch (instr->src[i].src_type) {
   case nir_tex_src_coord:
      if (instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
      sregs[VALHALL_TEX_SREG_X_COORD] = bi_emit_texc_cube_coord(
                                                      if (components == 3 && !instr->is_array) {
                     if (instr->is_array) {
      sregs[VALHALL_TEX_SREG_ARRAY] =
                     case nir_tex_src_lod:
      if (nir_src_is_const(instr->src[i].src) &&
      nir_src_as_uint(instr->src[i].src) == 0) {
                        assert(sz == 16 || sz == 32);
   sregs[VALHALL_TEX_SREG_LOD] =
                  case nir_tex_src_bias:
      /* Upper 16-bits interpreted as a clamp, leave zero */
                  lod_mode = BI_VA_LOD_MODE_COMPUTED_BIAS;
      case nir_tex_src_ms_index:
   case nir_tex_src_offset:
                  case nir_tex_src_comparator:
                  case nir_tex_src_texture_offset:
                  case nir_tex_src_sampler_offset:
                  default:
                     /* Generate packed offset + ms index + LOD register. These default to
   * zero so we only need to encode if these features are actually in use.
   */
            if (!bi_is_equiv(offsets, bi_zero())) {
      sregs[VALHALL_TEX_SREG_OFFSETMS] = offsets;
               /* Allocate staging registers contiguously by compacting the array. */
            for (unsigned i = 0; i < ARRAY_SIZE(sregs); ++i) {
      if (!bi_is_null(sregs[i]))
                        if (sr_count)
            bi_index image_src = bi_imm_u32(tables);
   image_src = bi_lshift_or_i32(b, sampler, image_src, bi_imm_u8(0));
            /* Only write the components that we actually read */
   unsigned mask = nir_def_components_read(&instr->def);
   unsigned comps_per_reg = instr->def.bit_size == 16 ? 2 : 1;
            enum bi_register_format regfmt = bi_reg_fmt_for_nir(instr->dest_type);
   enum bi_dimension dim = valhall_tex_dimension(instr->sampler_dim);
            switch (instr->op) {
   case nir_texop_tex:
   case nir_texop_txl:
   case nir_texop_txb:
      bi_tex_single_to(b, dest, idx, image_src, bi_zero(), instr->is_array, dim,
                  case nir_texop_txf:
   case nir_texop_txf_ms:
      bi_tex_fetch_to(b, dest, idx, image_src, bi_zero(), instr->is_array, dim,
            case nir_texop_tg4:
      bi_tex_gather_to(b, dest, idx, image_src, bi_zero(), instr->is_array, dim,
                  default:
                  /* The hardware will write only what we read, and it will into
   * contiguous registers without gaps (different from Bifrost). NIR
   * expects the gaps, so fill in the holes (they'll be copypropped and
   * DCE'd away later).
   */
                     /* Index into the packed component array */
   unsigned j = 0;
   unsigned comps[4] = {0};
            for (unsigned i = 0; i < nr_components; ++i) {
      if (mask & BITFIELD_BIT(i)) {
      unpacked[i] = dest;
      } else {
                     bi_make_vec_to(b, bi_def_index(&instr->def), unpacked, comps,
      }
      /* Simple textures ops correspond to NIR tex or txl with LOD = 0 on 2D/cube
   * textures with sufficiently small immediate indices. Anything else
   * needs a complete texture op. */
      static void
   bi_emit_texs(bi_builder *b, nir_tex_instr *instr)
   {
      int coord_idx = nir_tex_instr_src_index(instr, nir_tex_src_coord);
   assert(coord_idx >= 0);
            if (instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
      bi_index face, s, t;
            bi_texs_cube_to(b, instr->def.bit_size, bi_def_index(&instr->def), s, t,
      } else {
      bi_texs_2d_to(b, instr->def.bit_size, bi_def_index(&instr->def),
                              }
      static bool
   bi_is_simple_tex(nir_tex_instr *instr)
   {
      if (instr->op != nir_texop_tex && instr->op != nir_texop_txl)
            if (instr->dest_type != nir_type_float32 &&
      instr->dest_type != nir_type_float16)
         if (instr->is_shadow || instr->is_array)
            switch (instr->sampler_dim) {
   case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_EXTERNAL:
   case GLSL_SAMPLER_DIM_RECT:
            case GLSL_SAMPLER_DIM_CUBE:
      /* LOD can't be specified with TEXS_CUBE */
   if (instr->op == nir_texop_txl)
               default:
                  for (unsigned i = 0; i < instr->num_srcs; ++i) {
      if (instr->src[i].src_type != nir_tex_src_lod &&
      instr->src[i].src_type != nir_tex_src_coord)
            /* Indices need to fit in provided bits */
   unsigned idx_bits = instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE ? 2 : 3;
   if (MAX2(instr->sampler_index, instr->texture_index) >= (1 << idx_bits))
            int lod_idx = nir_tex_instr_src_index(instr, nir_tex_src_lod);
   if (lod_idx < 0)
            nir_src lod = instr->src[lod_idx].src;
      }
      static void
   bi_emit_tex(bi_builder *b, nir_tex_instr *instr)
   {
      /* If txf is used, we assume there is a valid sampler bound at index 0. Use
   * it for txf operations, since there may be no other valid samplers. This is
   * a workaround: txf does not require a sampler in NIR (so sampler_index is
   * undefined) but we need one in the hardware. This is ABI with the driver.
   */
   if (!nir_tex_instr_need_sampler(instr))
            if (b->shader->arch >= 9)
         else if (bi_is_simple_tex(instr))
         else
      }
      static void
   bi_emit_phi(bi_builder *b, nir_phi_instr *instr)
   {
      unsigned nr_srcs = exec_list_length(&instr->srcs);
            /* Deferred */
      }
      /* Look up the AGX block corresponding to a given NIR block. Used when
   * translating phi nodes after emitting all blocks.
   */
   static bi_block *
   bi_from_nir_block(bi_context *ctx, nir_block *block)
   {
         }
      static void
   bi_emit_phi_deferred(bi_context *ctx, bi_block *block, bi_instr *I)
   {
               /* Guaranteed by lower_phis_to_scalar */
            nir_foreach_phi_src(src, phi) {
      bi_block *pred = bi_from_nir_block(ctx, src->pred);
   unsigned i = bi_predecessor_index(block, pred);
                           }
      static void
   bi_emit_phis_deferred(bi_context *ctx)
   {
      bi_foreach_block(ctx, block) {
      bi_foreach_instr_in_block(block, I) {
      if (I->op == BI_OPCODE_PHI)
            }
      static void
   bi_emit_instr(bi_builder *b, struct nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_load_const:
      bi_emit_load_const(b, nir_instr_as_load_const(instr));
         case nir_instr_type_intrinsic:
      bi_emit_intrinsic(b, nir_instr_as_intrinsic(instr));
         case nir_instr_type_alu:
      bi_emit_alu(b, nir_instr_as_alu(instr));
         case nir_instr_type_tex:
      bi_emit_tex(b, nir_instr_as_tex(instr));
         case nir_instr_type_jump:
      bi_emit_jump(b, nir_instr_as_jump(instr));
         case nir_instr_type_phi:
      bi_emit_phi(b, nir_instr_as_phi(instr));
         default:
            }
      static bi_block *
   create_empty_block(bi_context *ctx)
   {
                           }
      static bi_block *
   emit_block(bi_context *ctx, nir_block *block)
   {
      if (ctx->after_block) {
      ctx->current_block = ctx->after_block;
      } else {
                  list_addtail(&ctx->current_block->link, &ctx->blocks);
                              nir_foreach_instr(instr, block) {
                     }
      static void
   emit_if(bi_context *ctx, nir_if *nif)
   {
               /* Speculatively emit the branch, but we can't fill it in until later */
   bi_builder _b = bi_init_builder(ctx, bi_after_block(ctx->current_block));
   bi_instr *then_branch =
      bi_branchz_i16(&_b, bi_half(bi_src_index(&nif->condition), false),
         /* Emit the two subblocks. */
   bi_block *then_block = emit_cf_list(ctx, &nif->then_list);
                     bi_block *else_block = emit_cf_list(ctx, &nif->else_list);
   bi_block *end_else_block = ctx->current_block;
                     assert(then_block);
                     /* Emit a jump from the end of the then block to the end of the else */
   _b.cursor = bi_after_block(end_then_block);
   bi_instr *then_exit = bi_jump(&_b, bi_zero());
            bi_block_add_successor(end_then_block, then_exit->branch_target);
            bi_block_add_successor(before_block,
            }
      static void
   emit_loop(bi_context *ctx, nir_loop *nloop)
   {
               /* Remember where we are */
            bi_block *saved_break = ctx->break_block;
            ctx->continue_block = create_empty_block(ctx);
   ctx->break_block = create_empty_block(ctx);
            /* Emit the body itself */
            /* Branch back to loop back */
   bi_builder _b = bi_init_builder(ctx, bi_after_block(ctx->current_block));
   bi_instr *I = bi_jump(&_b, bi_zero());
   I->branch_target = ctx->continue_block;
   bi_block_add_successor(start_block, ctx->continue_block);
                     /* Pop off */
   ctx->break_block = saved_break;
   ctx->continue_block = saved_continue;
      }
      static bi_block *
   emit_cf_list(bi_context *ctx, struct exec_list *list)
   {
               foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block: {
                                          case nir_cf_node_if:
                  case nir_cf_node_loop:
                  default:
                        }
      /* shader-db stuff */
      struct bi_stats {
      unsigned nr_clauses, nr_tuples, nr_ins;
      };
      static void
   bi_count_tuple_stats(bi_clause *clause, bi_tuple *tuple, struct bi_stats *stats)
   {
      /* Count instructions */
            /* Non-message passing tuples are always arithmetic */
   if (tuple->add != clause->message) {
      stats->nr_arith++;
               /* Message + FMA we'll count as arithmetic _and_ message */
   if (tuple->fma)
            switch (clause->message_type) {
   case BIFROST_MESSAGE_VARYING:
      /* Check components interpolated */
   stats->nr_varying +=
      (clause->message->vecsize + 1) *
            case BIFROST_MESSAGE_VARTEX:
      /* 2 coordinates, fp32 each */
   stats->nr_varying += (2 * 2);
      case BIFROST_MESSAGE_TEX:
      stats->nr_texture++;
         case BIFROST_MESSAGE_ATTRIBUTE:
   case BIFROST_MESSAGE_LOAD:
   case BIFROST_MESSAGE_STORE:
   case BIFROST_MESSAGE_ATOMIC:
      stats->nr_ldst++;
         case BIFROST_MESSAGE_NONE:
   case BIFROST_MESSAGE_BARRIER:
   case BIFROST_MESSAGE_BLEND:
   case BIFROST_MESSAGE_TILE:
   case BIFROST_MESSAGE_Z_STENCIL:
   case BIFROST_MESSAGE_ATEST:
   case BIFROST_MESSAGE_JOB:
   case BIFROST_MESSAGE_64BIT:
      /* Nothing to do */
         }
      /*
   * v7 allows preloading LD_VAR or VAR_TEX messages that must complete before the
   * shader completes. These costs are not accounted for in the general cycle
   * counts, so this function calculates the effective cost of these messages, as
   * if they were executed by shader code.
   */
   static unsigned
   bi_count_preload_cost(bi_context *ctx)
   {
      /* Units: 1/16 of a normalized cycle, assuming that we may interpolate
   * 16 fp16 varying components per cycle or fetch two texels per cycle.
   */
            for (unsigned i = 0; i < ARRAY_SIZE(ctx->info.bifrost->messages); ++i) {
               if (msg.enabled && msg.texture) {
      /* 2 coordinate, 2 half-words each, plus texture */
      } else if (msg.enabled) {
                        }
      static const char *
   bi_shader_stage_name(bi_context *ctx)
   {
      if (ctx->idvs == BI_IDVS_VARYING)
         else if (ctx->idvs == BI_IDVS_POSITION)
         else if (ctx->inputs->is_blend)
         else
      }
      static char *
   bi_print_stats(bi_context *ctx, unsigned size)
   {
               /* Count instructions, clauses, and tuples. Also attempt to construct
   * normalized execution engine cycle counts, using the following ratio:
   *
   * 24 arith tuples/cycle
   * 2 texture messages/cycle
   * 16 x 16-bit varying channels interpolated/cycle
   * 1 load store message/cycle
   *
   * These numbers seem to match Arm Mobile Studio's heuristic. The real
   * cycle counts are surely more complicated.
            bi_foreach_block(ctx, block) {
      bi_foreach_clause_in_block(block, clause) {
                     for (unsigned i = 0; i < clause->tuple_count; ++i)
                  float cycles_arith = ((float)stats.nr_arith) / 24.0;
   float cycles_texture = ((float)stats.nr_texture) / 2.0;
   float cycles_varying = ((float)stats.nr_varying) / 16.0;
            float cycles_message = MAX3(cycles_texture, cycles_varying, cycles_ldst);
            /* Thread count and register pressure are traded off only on v7 */
   bool full_threads = (ctx->arch == 7 && ctx->info.work_reg_count <= 32);
            /* Dump stats */
   char *str = ralloc_asprintf(
      NULL,
   "%s shader: "
   "%u inst, %u tuples, %u clauses, "
   "%f cycles, %f arith, %f texture, %f vary, %f ldst, "
   "%u quadwords, %u threads",
   bi_shader_stage_name(ctx), stats.nr_ins, stats.nr_tuples,
   stats.nr_clauses, cycles_bound, cycles_arith, cycles_texture,
         if (ctx->arch == 7) {
                  ralloc_asprintf_append(&str, ", %u loops, %u:%u spills:fills",
               }
      static char *
   va_print_stats(bi_context *ctx, unsigned size)
   {
      unsigned nr_ins = 0;
            /* Count instructions */
   bi_foreach_instr_global(ctx, I) {
      nr_ins++;
               /* Mali G78 peak performance:
   *
   * 64 FMA instructions per cycle
   * 64 CVT instructions per cycle
   * 16 SFU instructions per cycle
   * 8 x 32-bit varying channels interpolated per cycle
   * 4 texture instructions per cycle
   * 1 load/store operation per cycle
            float cycles_fma = ((float)stats.fma) / 64.0;
   float cycles_cvt = ((float)stats.cvt) / 64.0;
   float cycles_sfu = ((float)stats.sfu) / 16.0;
   float cycles_v = ((float)stats.v) / 16.0;
   float cycles_t = ((float)stats.t) / 4.0;
            /* Calculate the bound */
   float cycles = MAX2(MAX3(cycles_fma, cycles_cvt, cycles_sfu),
            /* Thread count and register pressure are traded off */
            /* Dump stats */
   return ralloc_asprintf(NULL,
                        "%s shader: "
   "%u inst, %f cycles, %f fma, %f cvt, %f sfu, %f v, "
   "%f t, %f ls, %u quadwords, %u threads, %u loops, "
   "%u:%u spills:fills",
   }
      static int
   glsl_type_size(const struct glsl_type *type, bool bindless)
   {
         }
      /* Split stores to memory. We don't split stores to vertex outputs, since
   * nir_lower_io_to_temporaries will ensure there's only a single write.
   */
      static bool
   should_split_wrmask(const nir_instr *instr, UNUSED const void *data)
   {
               switch (intr->intrinsic) {
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_scratch:
         default:
            }
      /*
   * Some operations are only available as 32-bit instructions. 64-bit floats are
   * unsupported and ints are lowered with nir_lower_int64.  Certain 8-bit and
   * 16-bit instructions, however, are lowered here.
   */
   static unsigned
   bi_lower_bit_size(const nir_instr *instr, UNUSED void *data)
   {
      if (instr->type != nir_instr_type_alu)
                     switch (alu->op) {
   case nir_op_fexp2:
   case nir_op_flog2:
   case nir_op_fpow:
   case nir_op_fsin:
   case nir_op_fcos:
   case nir_op_bit_count:
   case nir_op_bitfield_reverse:
         default:
            }
      /* Although Bifrost generally supports packed 16-bit vec2 and 8-bit vec4,
   * transcendentals are an exception. Also shifts because of lane size mismatch
   * (8-bit in Bifrost, 32-bit in NIR TODO - workaround!). Some conversions need
   * to be scalarized due to type size. */
      static uint8_t
   bi_vectorize_filter(const nir_instr *instr, const void *data)
   {
      /* Defaults work for everything else */
   if (instr->type != nir_instr_type_alu)
                     switch (alu->op) {
   case nir_op_frcp:
   case nir_op_frsq:
   case nir_op_ishl:
   case nir_op_ishr:
   case nir_op_ushr:
   case nir_op_f2i16:
   case nir_op_f2u16:
   case nir_op_extract_u8:
   case nir_op_extract_i8:
   case nir_op_extract_u16:
   case nir_op_extract_i16:
   case nir_op_insert_u16:
         default:
                  /* Vectorized instructions cannot write more than 32-bit */
   int dst_bit_size = alu->def.bit_size;
   if (dst_bit_size == 16)
         else
      }
      static bool
   bi_scalarize_filter(const nir_instr *instr, const void *data)
   {
      if (instr->type != nir_instr_type_alu)
                     switch (alu->op) {
   case nir_op_pack_uvec2_to_uint:
   case nir_op_pack_uvec4_to_uint:
         default:
            }
      /* Ensure we write exactly 4 components */
   static nir_def *
   bifrost_nir_valid_channel(nir_builder *b, nir_def *in, unsigned channel,
         {
      if (!(mask & BITFIELD_BIT(channel)))
               }
      /* Lower fragment store_output instructions to always write 4 components,
   * matching the hardware semantic. This may require additional moves. Skipping
   * these moves is possible in theory, but invokes undefined behaviour in the
   * compiler. The DDK inserts these moves, so we will as well. */
      static bool
   bifrost_nir_lower_blend_components(struct nir_builder *b,
         {
      if (intr->intrinsic != nir_intrinsic_store_output)
            nir_def *in = intr->src[0].ssa;
   unsigned first = nir_intrinsic_component(intr);
                     /* Nothing to do */
   if (mask == BITFIELD_MASK(4))
                     /* Replicate the first valid component instead */
   nir_def *replicated =
      nir_vec4(b, bifrost_nir_valid_channel(b, in, 0, first, mask),
                     /* Rewrite to use our replicated version */
   nir_src_rewrite(&intr->src[0], replicated);
   nir_intrinsic_set_component(intr, 0);
   nir_intrinsic_set_write_mask(intr, 0xF);
               }
      static nir_mem_access_size_align
   mem_access_size_align_cb(nir_intrinsic_op intrin, uint8_t bytes,
                     {
      align = nir_combined_align(align, align_offset);
            /* If the number of bytes is a multiple of 4, use 32-bit loads. Else if it's
   * a multiple of 2, use 16-bit loads. Else use 8-bit loads.
   */
            /* But if we're only aligned to 1 byte, use 8-bit loads. If we're only
   * aligned to 2 bytes, use 16-bit loads, unless we needed 8-bit loads due to
   * the size.
   */
   if (align == 1)
         else if (align == 2)
            return (nir_mem_access_size_align){
      .num_components = MIN2(bytes / (bit_size / 8), 4),
   .bit_size = bit_size,
         }
      static void
   bi_optimize_nir(nir_shader *nir, unsigned gpu_id, bool is_blend)
   {
               do {
               NIR_PASS(progress, nir, nir_lower_vars_to_ssa);
            NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_remove_phis);
   NIR_PASS(progress, nir, nir_opt_dce);
   NIR_PASS(progress, nir, nir_opt_dead_cf);
   NIR_PASS(progress, nir, nir_opt_cse);
   NIR_PASS(progress, nir, nir_opt_peephole_select, 64, false, true);
   NIR_PASS(progress, nir, nir_opt_algebraic);
            NIR_PASS(progress, nir, nir_opt_undef);
            NIR_PASS(progress, nir, nir_opt_shrink_vectors);
               /* TODO: Why is 64-bit getting rematerialized?
   * KHR-GLES31.core.shader_image_load_store.basic-allTargets-atomicFS */
            /* We need to cleanup after each iteration of late algebraic
   * optimizations, since otherwise NIR can produce weird edge cases
   * (like fneg of a constant) which we don't handle */
   bool late_algebraic = true;
   while (late_algebraic) {
      late_algebraic = false;
   NIR_PASS(late_algebraic, nir, nir_opt_algebraic_late);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_dce);
               /* This opt currently helps on Bifrost but not Valhall */
   if (gpu_id < 0x9000)
            NIR_PASS(progress, nir, nir_lower_alu_to_scalar, bi_scalarize_filter, NULL);
   NIR_PASS(progress, nir, nir_opt_vectorize, bi_vectorize_filter, NULL);
            /* Prepass to simplify instruction selection */
   late_algebraic = false;
            while (late_algebraic) {
      late_algebraic = false;
   NIR_PASS(late_algebraic, nir, nir_opt_algebraic_late);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_dce);
               NIR_PASS(progress, nir, nir_lower_load_const_to_scalar);
            if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS_V(nir, nir_shader_intrinsics_pass,
                     /* Backend scheduler is purely local, so do some global optimizations
   * to reduce register pressure. */
   nir_move_options move_all = nir_move_const_undef | nir_move_load_ubo |
                  NIR_PASS_V(nir, nir_opt_sink, move_all);
            /* We might lower attribute, varying, and image indirects. Use the
   * gathered info to skip the extra analysis in the happy path. */
   bool any_indirects = nir->info.inputs_read_indirectly ||
                              if (any_indirects) {
      nir_convert_to_lcssa(nir, true, true);
   NIR_PASS_V(nir, nir_divergence_analysis);
   NIR_PASS_V(nir, bi_lower_divergent_indirects,
         }
      static void
   bi_opt_post_ra(bi_context *ctx)
   {
      bi_foreach_instr_global_safe(ctx, ins) {
      if (ins->op == BI_OPCODE_MOV_I32 &&
      bi_is_equiv(ins->dest[0], ins->src[0]))
      }
      /* Dead code elimination for branches at the end of a block - only one branch
   * per block is legal semantically, but unreachable jumps can be generated.
   * Likewise on Bifrost we can generate jumps to the terminal block which need
   * to be lowered away to a jump to #0x0, which induces successful termination.
   * That trick doesn't work on Valhall, which needs a NOP inserted in the
   * terminal block instead.
   */
   static void
   bi_lower_branch(bi_context *ctx, bi_block *block)
   {
      bool cull_terminal = (ctx->arch <= 8);
            bi_foreach_instr_in_block_safe(block, ins) {
      if (!ins->branch_target)
            if (branched) {
      bi_remove_instruction(ins);
                        if (!bi_is_terminal_block(ins->branch_target))
            if (cull_terminal)
         else if (ins->branch_target)
         }
      static void
   bi_pack_clauses(bi_context *ctx, struct util_dynarray *binary, unsigned offset)
   {
               /* If we need to wait for ATEST or BLEND in the first clause, pass the
   * corresponding bits through to the renderer state descriptor */
   bi_block *first_block = list_first_entry(&ctx->blocks, bi_block, link);
            unsigned first_deps = first_clause ? first_clause->dependencies : 0;
   ctx->info.bifrost->wait_6 = (first_deps & (1 << 6));
            /* Pad the shader with enough zero bytes to trick the prefetcher,
   * unless we're compiling an empty shader (in which case we don't pad
   * so the size remains 0) */
            if (binary->size - offset) {
      memset(util_dynarray_grow(binary, uint8_t, prefetch_size), 0,
         }
      /*
   * Build a bit mask of varyings (by location) that are flatshaded. This
   * information is needed by lower_mediump_io, as we don't yet support 16-bit
   * flat varyings.
   *
   * Also varyings that are used as texture coordinates should be kept at fp32 so
   * the texture instruction may be promoted to VAR_TEX. In general this is a good
   * idea, as fp16 texture coordinates are not supported by the hardware and are
   * usually inappropriate. (There are both relevant CTS bugs here, even.)
   *
   * TODO: If we compacted the varyings with some fixup code in the vertex shader,
   * we could implement 16-bit flat varyings. Consider if this case matters.
   *
   * TODO: The texture coordinate handling could be less heavyhanded.
   */
   static bool
   bi_gather_texcoords(nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type != nir_instr_type_tex)
                     int coord_idx = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   if (coord_idx < 0)
            nir_src src = tex->src[coord_idx].src;
   nir_scalar x = nir_scalar_resolved(src.ssa, 0);
            if (x.def != y.def)
                     if (parent->type != nir_instr_type_intrinsic)
                     if (intr->intrinsic != nir_intrinsic_load_interpolated_input)
            nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
   *mask |= BITFIELD64_BIT(sem.location);
      }
      static uint64_t
   bi_fp32_varying_mask(nir_shader *nir)
   {
                        nir_foreach_shader_in_variable(var, nir) {
      if (var->data.interpolation == INTERP_MODE_FLAT)
               nir_shader_instructions_pass(nir, bi_gather_texcoords, nir_metadata_all,
               }
      static bool
   bi_lower_sample_mask_writes(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_store_output)
            assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   if (nir_intrinsic_io_semantics(intr).location != FRAG_RESULT_SAMPLE_MASK)
                              nir_src_rewrite(&intr->src[0],
                  }
      static bool
   bi_lower_load_output(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_load_output)
            unsigned loc = nir_intrinsic_io_semantics(intr).location;
   assert(loc >= FRAG_RESULT_DATA0);
                     nir_def *conversion = nir_load_rt_conversion_pan(
            nir_def *lowered = nir_load_converted_output_pan(
      b, intr->def.num_components, intr->def.bit_size, conversion,
   .dest_type = nir_intrinsic_dest_type(intr),
         nir_def_rewrite_uses(&intr->def, lowered);
      }
      bool
   bifrost_nir_lower_load_output(nir_shader *nir)
   {
               return nir_shader_intrinsics_pass(
      nir, bi_lower_load_output,
   }
      void
   bifrost_preprocess_nir(nir_shader *nir, unsigned gpu_id)
   {
      /* Lower gl_Position pre-optimisation, but after lowering vars to ssa
   * (so we don't accidentally duplicate the epilogue since mesa/st has
                     if (nir->info.stage == MESA_SHADER_VERTEX) {
      NIR_PASS_V(nir, nir_lower_viewport_transform);
            nir_variable *psiz = nir_find_variable_with_location(
         if (psiz != NULL)
               /* Get rid of any global vars before we lower to scratch. */
            /* Valhall introduces packed thread local storage, which improves cache
   * locality of TLS access. However, access to packed TLS cannot
   * straddle 16-byte boundaries. As such, when packed TLS is in use
   * (currently unconditional for Valhall), we force vec4 alignment for
   * scratch access.
   */
            /* Lower large arrays to scratch and small arrays to bcsel */
   NIR_PASS_V(nir, nir_lower_vars_to_scratch, nir_var_function_temp, 256,
                        NIR_PASS_V(nir, nir_split_var_copies);
   NIR_PASS_V(nir, nir_lower_var_copies);
   NIR_PASS_V(nir, nir_lower_vars_to_ssa);
   NIR_PASS_V(nir, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
            /* nir_lower[_explicit]_io is lazy and emits mul+add chains even for
   * offsets it could figure out are constant.  Do some constant folding
   * before bifrost_nir_lower_store_component below.
   */
            if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS_V(nir, nir_lower_mediump_io,
                  NIR_PASS_V(nir, nir_shader_intrinsics_pass, bi_lower_sample_mask_writes,
               } else if (nir->info.stage == MESA_SHADER_VERTEX) {
      if (gpu_id >= 0x9000) {
      NIR_PASS_V(nir, nir_lower_mediump_io, nir_var_shader_out,
                           nir_lower_mem_access_bit_sizes_options mem_size_options = {
      .modes = nir_var_mem_ubo | nir_var_mem_ssbo | nir_var_mem_constant |
                  };
            NIR_PASS_V(nir, nir_lower_ssbo);
   NIR_PASS_V(nir, pan_lower_sample_pos);
   NIR_PASS_V(nir, nir_lower_bit_size, bi_lower_bit_size, NULL);
   NIR_PASS_V(nir, nir_lower_64bit_phis);
   NIR_PASS_V(nir, pan_lower_helper_invocation);
            NIR_PASS_V(nir, nir_opt_idiv_const, 8);
   NIR_PASS_V(nir, nir_lower_idiv,
            NIR_PASS_V(nir, nir_lower_tex,
            &(nir_lower_tex_options){
      .lower_txs_lod = true,
   .lower_txp = ~0,
   .lower_tg4_broadcom_swizzle = true,
   .lower_txd = true,
            NIR_PASS_V(nir, nir_lower_image_atomics_to_global);
   NIR_PASS_V(nir, nir_lower_alu_to_scalar, bi_scalarize_filter, NULL);
   NIR_PASS_V(nir, nir_lower_load_const_to_scalar);
   NIR_PASS_V(nir, nir_lower_phis_to_scalar, true);
   NIR_PASS_V(nir, nir_lower_flrp, 16 | 32 | 64, false /* always_precise */);
   NIR_PASS_V(nir, nir_lower_var_copies);
   NIR_PASS_V(nir, nir_lower_alu);
      }
      static bi_context *
   bi_compile_variant_nir(nir_shader *nir,
                     {
               /* There may be another program in the dynarray, start at the end */
            ctx->inputs = inputs;
   ctx->nir = nir;
   ctx->stage = nir->info.stage;
   ctx->quirks = bifrost_get_quirks(inputs->gpu_id);
   ctx->arch = inputs->gpu_id >> 12;
   ctx->info = info;
   ctx->idvs = idvs;
            if (idvs != BI_IDVS_NONE) {
      /* Specializing shaders for IDVS is destructive, so we need to
   * clone. However, the last (second) IDVS shader does not need
   * to be preserved so we can skip cloning that one.
   */
   if (offset == 0)
            NIR_PASS_V(nir, nir_shader_instructions_pass, bifrost_nir_specialize_idvs,
            /* After specializing, clean up the mess */
            while (progress) {
               NIR_PASS(progress, nir, nir_opt_dce);
                  /* If nothing is pushed, all UBOs need to be uploaded */
                     bool skip_internal = nir->info.internal;
            if (bifrost_debug & BIFROST_DBG_SHADERS && !skip_internal) {
                           nir_foreach_function_impl(impl, nir) {
               ctx->indexed_nir_blocks =
                     emit_cf_list(ctx, &impl->body);
   bi_emit_phis_deferred(ctx);
               /* Index blocks now that we're done emitting */
   bi_foreach_block(ctx, block) {
                           /* If the shader doesn't write any colour or depth outputs, it may
   * still need an ATEST at the very end! */
   bool need_dummy_atest = (ctx->stage == MESA_SHADER_FRAGMENT) &&
            if (need_dummy_atest) {
      bi_block *end = list_last_entry(&ctx->blocks, bi_block, link);
   bi_builder b = bi_init_builder(ctx, bi_after_block(end));
                        /* Runs before constant folding */
   bi_lower_swizzle(ctx);
            /* Runs before copy prop */
   if (optimize && !ctx->inputs->no_ubo_to_push) {
                  if (likely(optimize)) {
               while (bi_opt_constant_fold(ctx))
            bi_opt_mod_prop_forward(ctx);
            /* Push LD_VAR_IMM/VAR_TEX instructions. Must run after
   * mod_prop_backward to fuse VAR_TEX */
   if (ctx->arch == 7 && ctx->stage == MESA_SHADER_FRAGMENT &&
      !(bifrost_debug & BIFROST_DBG_NOPRELOAD)) {
   bi_opt_dead_code_eliminate(ctx);
   bi_opt_message_preload(ctx);
               bi_opt_dead_code_eliminate(ctx);
   bi_opt_cse(ctx);
   bi_opt_dead_code_eliminate(ctx);
   if (!ctx->inputs->no_ubo_to_push)
                              if (ctx->arch >= 9) {
      va_optimize(ctx);
            bi_foreach_instr_global_safe(ctx, I) {
      /* Phis become single moves so shouldn't be affected */
                           bi_builder b = bi_init_builder(ctx, bi_before_instr(I));
               /* We need to clean up after constant lowering */
   if (likely(optimize)) {
      bi_opt_cse(ctx);
                           bi_foreach_block(ctx, block) {
                  if (bifrost_debug & BIFROST_DBG_SHADERS && !skip_internal)
            /* Analyze before register allocation to avoid false dependencies. The
   * skip bit is a function of only the data flow graph and is invariant
   * under valid scheduling. Helpers are only defined for fragment
   * shaders, so this analysis is only required in fragment shaders.
   */
   if (ctx->stage == MESA_SHADER_FRAGMENT)
            /* Fuse TEXC after analyzing helper requirements so the analysis
   * doesn't have to know about dual textures */
   if (likely(optimize)) {
                  /* Lower FAU after fusing dual texture, because fusing dual texture
   * creates new immediates that themselves may need lowering.
   */
   if (ctx->arch <= 8) {
                  /* Lowering FAU can create redundant moves. Run CSE+DCE to clean up. */
   if (likely(optimize)) {
      bi_opt_cse(ctx);
                        if (likely(!(bifrost_debug & BIFROST_DBG_NOPSCHED))) {
      bi_pressure_schedule(ctx);
                        if (likely(optimize))
            if (bifrost_debug & BIFROST_DBG_SHADERS && !skip_internal)
            if (ctx->arch >= 9) {
      va_assign_slots(ctx);
   va_insert_flow_control_nops(ctx);
   va_merge_flow(ctx);
      } else {
      bi_schedule(ctx);
            /* Analyze after scheduling since we depend on instruction
   * order. Valhall calls as part of va_insert_flow_control_nops,
   * as the handling for clauses differs from instructions.
   */
   bi_analyze_helper_terminate(ctx);
               if (bifrost_debug & BIFROST_DBG_SHADERS && !skip_internal)
            if (ctx->arch <= 8) {
         } else {
                  if (bifrost_debug & BIFROST_DBG_SHADERS && !skip_internal) {
      if (ctx->arch <= 8) {
      disassemble_bifrost(stdout, binary->data + offset,
            } else {
      disassemble_valhall(stdout, binary->data + offset,
                                 if (!skip_internal &&
      ((bifrost_debug & BIFROST_DBG_SHADERDB) || inputs->debug)) {
            if (ctx->arch >= 9) {
         } else {
                  if (bifrost_debug & BIFROST_DBG_SHADERDB)
            if (inputs->debug)
                           }
      static void
   bi_compile_variant(nir_shader *nir,
                     {
      struct bi_shader_info local_info = {
      .push = &info->push,
   .bifrost = &info->bifrost,
   .tls_size = info->tls_size,
                        /* If there is no position shader (gl_Position is not written), then
   * there is no need to build a varying shader either. This case is hit
   * for transform feedback only vertex shaders which only make sense with
   * rasterizer discard.
   */
   if ((offset == 0) && (idvs == BI_IDVS_VARYING))
            /* Software invariant: Only a secondary shader can appear at a nonzero
   * offset, to keep the ABI simple. */
            bi_context *ctx =
            /* A register is preloaded <==> it is live before the first block */
   bi_block *first_block = list_first_entry(&ctx->blocks, bi_block, link);
            /* If multisampling is used with a blend shader, the blend shader needs
   * to access the sample coverage mask in r60 and the sample ID in r61.
   * Blend shaders run in the same context as fragment shaders, so if a
   * blend shader could run, we need to preload these registers
   * conservatively. There is believed to be little cost to doing so, so
   * do so always to avoid variants of the preload descriptor.
   *
   * We only do this on Valhall, as Bifrost has to update the RSD for
   * multisampling w/ blend shader anyway, so this is handled in the
   * driver. We could unify the paths if the cost is acceptable.
   */
   if (nir->info.stage == MESA_SHADER_FRAGMENT && ctx->arch >= 9)
            info->ubo_mask |= ctx->ubo_mask;
            if (idvs == BI_IDVS_VARYING) {
      info->vs.secondary_enable = (binary->size > offset);
   info->vs.secondary_offset = offset;
   info->vs.secondary_preload = preload;
      } else {
      info->preload = preload;
               if (idvs == BI_IDVS_POSITION && !nir->info.internal &&
      nir->info.outputs_written & BITFIELD_BIT(VARYING_SLOT_PSIZ)) {
   /* Find the psiz write */
            bi_foreach_instr_global(ctx, I) {
      if (I->op == BI_OPCODE_STORE_I16 && I->seg == BI_SEG_POS) {
      write = I;
                           /* NOP it out, preserving its flow control. TODO: maybe DCE */
   if (write->flow) {
      bi_builder b = bi_init_builder(ctx, bi_before_instr(write));
   bi_instr *nop = bi_nop(&b);
                        info->vs.no_psiz_offset = binary->size;
                  }
      /* Decide if Index-Driven Vertex Shading should be used for a given shader */
   static bool
   bi_should_idvs(nir_shader *nir, const struct panfrost_compile_inputs *inputs)
   {
      /* Opt-out */
   if (inputs->no_idvs || bifrost_debug & BIFROST_DBG_NOIDVS)
            /* IDVS splits up vertex shaders, not defined on other shader stages */
   if (nir->info.stage != MESA_SHADER_VERTEX)
            /* Bifrost cannot write gl_PointSize during IDVS */
   if ((inputs->gpu_id < 0x9000) &&
      nir->info.outputs_written & BITFIELD_BIT(VARYING_SLOT_PSIZ))
         /* Otherwise, IDVS is usually better */
      }
      void
   bifrost_compile_shader_nir(nir_shader *nir,
                     {
               /* Combine stores late, to give the driver a chance to lower dual-source
   * blending as regular store_output intrinsics.
   */
                     info->tls_size = nir->scratch_size;
                     if (info->vs.idvs) {
      bi_compile_variant(nir, inputs, binary, info, BI_IDVS_POSITION);
      } else {
                  if (gl_shader_stage_is_compute(nir->info.stage)) {
      /* Workgroups may be merged if the structure of the workgroup is
   * not software visible. This is true if neither shared memory
   * nor barriers are used. The hardware may be able to optimize
   * compute shaders that set this flag.
   */
   info->cs.allow_merging_workgroups = (nir->info.shared_size == 0) &&
                        }
