   /*
   * Copyright 2021 Alyssa Rosenzweig
   * Copyright 2020 Collabora Ltd.
   * Copyright 2016 Broadcom
   * SPDX-License-Identifier: MIT
   */
      #include "agx_compile.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir_types.h"
   #include "util/glheader.h"
   #include "util/u_debug.h"
   #include "agx_builder.h"
   #include "agx_compiler.h"
   #include "agx_debug.h"
   #include "agx_internal_formats.h"
   #include "agx_nir.h"
   #include "nir_intrinsics.h"
      /* Alignment for shader programs. I'm not sure what the optimal value is. */
   #define AGX_CODE_ALIGN 0x100
      /* clang-format off */
   static const struct debug_named_value agx_debug_options[] = {
      {"msgs",      AGX_DBG_MSGS,		"Print debug messages"},
   {"shaders",   AGX_DBG_SHADERS,	"Dump shaders in NIR and AIR"},
   {"shaderdb",  AGX_DBG_SHADERDB,	"Print statistics"},
   {"verbose",   AGX_DBG_VERBOSE,	"Disassemble verbosely"},
   {"internal",  AGX_DBG_INTERNAL,	"Dump even internal shaders"},
   {"novalidate",AGX_DBG_NOVALIDATE,"Skip IR validation in debug builds"},
   {"noopt",     AGX_DBG_NOOPT,     "Disable backend optimizations"},
   {"wait",      AGX_DBG_WAIT,      "Wait after all async instructions"},
   {"nopreamble",AGX_DBG_NOPREAMBLE,"Do not use shader preambles"},
   {"demand",    AGX_DBG_DEMAND,    "Bound tightly to register demand"},
   {"nosched",   AGX_DBG_NOSCHED,   "Do not schedule the shader"},
      };
   /* clang-format on */
      DEBUG_GET_ONCE_FLAGS_OPTION(agx_compiler_debug, "AGX_MESA_DEBUG",
            int agx_compiler_debug = 0;
      uint64_t
   agx_get_compiler_debug(void)
   {
         }
      #define DBG(fmt, ...)                                                          \
      do {                                                                        \
      if (agx_compiler_debug & AGX_DBG_MSGS)                                   \
            static agx_index
   agx_cached_preload(agx_context *ctx, agx_index *cache, unsigned base,
         {
      if (agx_is_null(*cache)) {
      agx_block *block = agx_start_block(ctx);
   agx_builder b = agx_init_builder(ctx, agx_before_block(block));
                  }
      static agx_index
   agx_vertex_id(agx_builder *b)
   {
         }
      static agx_index
   agx_instance_id(agx_builder *b)
   {
      return agx_cached_preload(b->shader, &b->shader->instance_id, 12,
      }
      static agx_index
   agx_get_cf(agx_context *ctx, bool smooth, bool perspective,
         {
      struct agx_varyings_fs *varyings = &ctx->out->varyings.fs;
            if (slot == VARYING_SLOT_POS) {
      assert(offset == 2 || offset == 3);
               /* Forcibly vectorize pointcoord reads, since there's no (known) way to index
   * Y alone.
   */
   bool is_pntc = (slot == VARYING_SLOT_PNTC);
            if (is_pntc) {
      cf_offset = offset;
   offset = 0;
               /* First, search for an appropriate binding. This is O(n) to the number of
   * bindings, which isn't great, but n should be small in practice.
   */
   for (unsigned b = 0; b < varyings->nr_bindings; ++b) {
      if ((varyings->bindings[b].slot == slot) &&
      (varyings->bindings[b].offset == offset) &&
   (varyings->bindings[b].count == count) &&
                                 /* If we didn't find one, make one */
   unsigned b = varyings->nr_bindings++;
   varyings->bindings[b].cf_base = varyings->nr_cf;
   varyings->bindings[b].slot = slot;
   varyings->bindings[b].offset = offset;
   varyings->bindings[b].count = count;
   varyings->bindings[b].smooth = smooth;
   varyings->bindings[b].perspective = perspective;
               }
      /* Builds a 64-bit hash table key for an index */
   static uint64_t
   agx_index_to_key(agx_index idx)
   {
               uint64_t key = 0;
   memcpy(&key, &idx, sizeof(idx));
      }
      /*
   * Extract a single channel out of a vector source. We split vectors with
   * p_split so we can use the split components directly, without emitting a
   * machine instruction. This has advantages of RA, as the split can usually be
   * optimized away.
   */
   static agx_index
   agx_emit_extract(agx_builder *b, agx_index vec, unsigned channel)
   {
      agx_index *components = _mesa_hash_table_u64_search(b->shader->allocated_vec,
                        }
      static agx_index
   agx_extract_nir_src(agx_builder *b, nir_src src, unsigned channel)
   {
               /* We only deal with scalars, extract a single scalar if needed */
   if (nir_src_num_components(src) > 1)
         else
      }
      static void
   agx_cache_collect(agx_builder *b, agx_index dst, unsigned nr_srcs,
         {
      /* Lifetime of a hash table entry has to be at least as long as the table */
            for (unsigned i = 0; i < nr_srcs; ++i)
            _mesa_hash_table_u64_insert(b->shader->allocated_vec, agx_index_to_key(dst),
      }
      /*
   * Combine multiple scalars into a vector destination. This corresponds to
   * collect, lowered to moves (a shuffle in general) after register allocation.
   *
   * To optimize vector extractions, we record the individual channels
   */
   static agx_instr *
   agx_emit_collect_to(agx_builder *b, agx_index dst, unsigned nr_srcs,
         {
               if (nr_srcs == 1)
                     agx_foreach_src(I, s)
               }
      static agx_index
   agx_emit_collect(agx_builder *b, unsigned nr_srcs, agx_index *srcs)
   {
      agx_index dst = agx_temp(b->shader, srcs[0].size);
   agx_emit_collect_to(b, dst, nr_srcs, srcs);
      }
      static agx_index
   agx_vec2(agx_builder *b, agx_index s0, agx_index s1)
   {
         }
      static agx_index
   agx_recollect_vector(agx_builder *b, nir_src vec)
   {
      agx_index comps[4];
            for (unsigned i = 0; i < nr; ++i)
               }
      /*
   * Extract the lower or upper N-bits from a (2*N)-bit quantity. We use a split
   * without null destinations to let us CSE (and coalesce) the splits when both x
   * and y are split.
   */
   static agx_instr *
   agx_subdivide_to(agx_builder *b, agx_index dst, agx_index s0, unsigned comp)
   {
      assert((s0.size == (dst.size + 1)) && "only 2x subdivide handled");
            agx_instr *split = agx_split(b, 2, s0);
   split->dest[comp] = dst;
   split->dest[1 - comp] = agx_temp(b->shader, dst.size);
      }
      static void
   agx_block_add_successor(agx_block *block, agx_block *successor)
   {
               /* Cull impossible edges */
   if (block->unconditional_jumps)
            for (unsigned i = 0; i < ARRAY_SIZE(block->successors); ++i) {
      if (block->successors[i]) {
      if (block->successors[i] == successor)
         else
               block->successors[i] = successor;
   util_dynarray_append(&successor->predecessors, agx_block *, block);
                  }
      /*
   * Splits an n-component vector (vec) into n scalar destinations (dests) using a
   * split pseudo-instruction.
   *
   * Pre-condition: dests is filled with agx_null().
   */
   static void
   agx_emit_split(agx_builder *b, agx_index *dests, agx_index vec, unsigned n)
   {
               agx_foreach_dest(I, d) {
      dests[d] = agx_temp(b->shader, vec.size);
         }
      static void
   agx_emit_cached_split(agx_builder *b, agx_index vec, unsigned n)
   {
      agx_index dests[4] = {agx_null(), agx_null(), agx_null(), agx_null()};
   agx_emit_split(b, dests, vec, n);
      }
      static void
   agx_emit_load_const(agx_builder *b, nir_load_const_instr *instr)
   {
      /* Ensure we've been scalarized and bit size lowered */
   unsigned bit_size = instr->def.bit_size;
            /* Emit move, later passes can inline/push if useful */
   agx_mov_imm_to(b, agx_def_index(&instr->def),
      }
      /*
   * Implement mul_high of 32-bit sources by doing a 32x32->64-bit multiply and
   * extracting only the high word.
   */
   static agx_instr *
   agx_mul_high_to(agx_builder *b, agx_index dst, agx_index P, agx_index Q,
         {
      assert(P.size == Q.size && "source sizes must match");
   assert(P.size == dst.size && "dest size must match");
            static_assert(AGX_SIZE_64 == (AGX_SIZE_32 + 1), "enum wrong");
            if (!is_signed) {
      P = agx_abs(P);
               agx_index product = agx_temp(b->shader, P.size + 1);
               }
      static enum agx_format
   agx_format_for_pipe(enum pipe_format format)
   {
   #define CASE(x)                                                                \
      if (format == (enum pipe_format)AGX_INTERNAL_FORMAT_##x)                    \
            CASE(I8);
   CASE(I16);
   CASE(I32);
   CASE(F16);
   CASE(U8NORM);
   CASE(S8NORM);
   CASE(U16NORM);
   CASE(S16NORM);
   CASE(RGB10A2);
   CASE(SRGBA8);
   CASE(RG11B10F);
         #undef CASE
         }
      static void
   agx_emit_load_coefficients(agx_builder *b, agx_index dest,
         {
      enum glsl_interp_mode mode = nir_intrinsic_interp_mode(instr);
   bool smooth = (mode != INTERP_MODE_FLAT);
            agx_index cf = agx_get_cf(b->shader, smooth, perspective,
                  agx_ldcf_to(b, dest, cf, 1);
      }
      static enum agx_interpolation
   agx_interp_for_bary(nir_intrinsic_instr *bary, agx_index *sample_index)
   {
      switch (bary->intrinsic) {
   case nir_intrinsic_load_barycentric_pixel:
            case nir_intrinsic_load_barycentric_centroid:
            case nir_intrinsic_load_barycentric_at_sample:
      *sample_index = agx_src_index(&bary->src[0]);
         default:
            }
      static void
   agx_emit_load_vary(agx_builder *b, agx_index dest, nir_intrinsic_instr *instr)
   {
      ASSERTED unsigned components = instr->num_components;
                     agx_index sample_index = agx_zero();
            bool perspective =
            nir_io_semantics sem = nir_intrinsic_io_semantics(instr);
   nir_src *offset = nir_get_io_offset_src(instr);
            assert(nir_def_components_read(&instr->def) ==
                  agx_index I = agx_get_cf(b->shader, true, perspective,
                  /* For perspective interpolation, we project (multiply by 1/W) */
   if (perspective) {
      agx_index J = agx_get_cf(b->shader, true, false, VARYING_SLOT_POS, 3, 1);
      } else {
                     }
      static agx_instr *
   agx_emit_store_vary(agx_builder *b, nir_intrinsic_instr *instr)
   {
      nir_io_semantics sem = nir_intrinsic_io_semantics(instr);
   nir_src *offset = nir_get_io_offset_src(instr);
                     if (sem.location == VARYING_SLOT_LAYER) {
      /* Separate slots used for the sysval vs the varying. The default slot
   * above is for the varying. Change for the sysval.
   */
            if (sem.no_varying)
               assert(imm_index < ~0);
            /* nir_lower_io_to_scalar */
            return agx_st_vary(b, agx_immediate(imm_index),
      }
      static agx_instr *
   agx_emit_local_store_pixel(agx_builder *b, nir_intrinsic_instr *instr)
   {
      /* TODO: Reverse-engineer interactions with MRT */
   if (b->shader->key->fs.ignore_tib_dependencies) {
         } else if (b->shader->did_writeout) {
         } else {
                  /* Compact the registers according to the mask */
            unsigned compact_count = 0;
   u_foreach_bit(i, nir_intrinsic_write_mask(instr)) {
                           b->shader->did_writeout = true;
   b->shader->out->tag_write_disable = false;
   return agx_st_tile(b, collected, agx_src_index(&instr->src[1]),
                  }
      static agx_instr *
   agx_emit_store_zs(agx_builder *b, nir_intrinsic_instr *instr)
   {
      unsigned base = nir_intrinsic_base(instr);
   bool write_z = base & 1;
            /* TODO: Handle better */
   assert(!b->shader->key->fs.ignore_tib_dependencies && "not used");
            agx_index z = agx_src_index(&instr->src[1]);
            assert(!write_z || z.size == AGX_SIZE_32);
            if (write_z && write_s) {
      agx_index u2u32 = agx_temp(b->shader, AGX_SIZE_32);
   agx_mov_to(b, u2u32, s);
                        /* Not necessarily a sample mask but overlapping hw mechanism... Should
   * maybe rename this flag to something more general.
   */
               }
      static void
   agx_emit_local_load_pixel(agx_builder *b, agx_index dest,
         {
      /* TODO: Reverse-engineer interactions with MRT */
   assert(!b->shader->key->fs.ignore_tib_dependencies && "invalid usage");
   agx_wait_pix(b, 0x0008);
   b->shader->did_writeout = true;
            unsigned nr_comps = instr->def.num_components;
   agx_ld_tile_to(b, dest, agx_src_index(&instr->src[0]),
                  }
      static void
   agx_emit_load(agx_builder *b, agx_index dest, nir_intrinsic_instr *instr)
   {
      agx_index addr = agx_src_index(&instr->src[0]);
   agx_index offset = agx_src_index(&instr->src[1]);
   enum agx_format fmt = agx_format_for_pipe(nir_intrinsic_format(instr));
            /* Zero-extend offset if we're not sign-extending */
   if (!nir_intrinsic_sign_extend(instr))
            agx_device_load_to(b, dest, addr, offset, fmt,
            }
      static void
   agx_emit_store(agx_builder *b, nir_intrinsic_instr *instr)
   {
      agx_index addr = agx_src_index(&instr->src[1]);
   agx_index offset = agx_src_index(&instr->src[2]);
   enum agx_format fmt = agx_format_for_pipe(nir_intrinsic_format(instr));
            /* Zero-extend offset if we're not sign-extending */
   if (!nir_intrinsic_sign_extend(instr))
            agx_device_store(b, agx_recollect_vector(b, instr->src[0]), addr, offset,
            }
      /* Preambles write directly to uniform registers, so move from uniform to GPR */
   static agx_instr *
   agx_emit_load_preamble(agx_builder *b, agx_index dst,
         {
      agx_index srcs[4] = {agx_null()};
   unsigned dim = instr->def.num_components;
            unsigned base = nir_intrinsic_base(instr);
            for (unsigned i = 0; i < dim; ++i)
               }
      static agx_instr *
   agx_emit_store_preamble(agx_builder *b, nir_intrinsic_instr *instr)
   {
      agx_index vec = agx_src_index(&instr->src[0]);
   unsigned base = nir_intrinsic_base(instr);
            for (unsigned i = 0; i < nir_src_num_components(instr->src[0]); ++i) {
      agx_uniform_store(b, agx_extract_nir_src(b, instr->src[0], i),
                  }
      static enum agx_dim
   agx_tex_dim(enum glsl_sampler_dim dim, bool array)
   {
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
            case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_RECT:
   case GLSL_SAMPLER_DIM_EXTERNAL:
            case GLSL_SAMPLER_DIM_MS:
            case GLSL_SAMPLER_DIM_3D:
      assert(!array && "3D arrays unsupported");
         case GLSL_SAMPLER_DIM_CUBE:
            case GLSL_SAMPLER_DIM_BUF:
            default:
            }
      static agx_instr *
   agx_emit_block_image_store(agx_builder *b, nir_intrinsic_instr *instr)
   {
      unsigned image = nir_src_as_uint(instr->src[0]);
   agx_index offset = agx_src_index(&instr->src[1]);
   agx_index layer = agx_src_index(&instr->src[2]);
            bool ms = nir_intrinsic_image_dim(instr) == GLSL_SAMPLER_DIM_MS;
   bool array = nir_intrinsic_image_array(instr);
            /* Modified coordinate descriptor */
   agx_index coords;
   if (array) {
      coords = agx_temp(b->shader, AGX_SIZE_32);
   agx_emit_collect_to(
      b, coords, 2,
   (agx_index[]){
      ms ? agx_mov_imm(b, 16, 0) : layer,
      } else {
                  // XXX: how does this possibly work
   if (format == AGX_FORMAT_F16)
            return agx_block_image_store(b, agx_immediate(image), offset, coords, format,
      }
      static agx_instr *
   agx_load_compute_dimension(agx_builder *b, agx_index dst,
         {
      unsigned dim = instr->def.num_components;
   unsigned size = instr->def.bit_size;
            agx_index srcs[] = {
      agx_get_sr(b, size, base + 0),
   agx_get_sr(b, size, base + 1),
                  }
      static enum agx_atomic_opc
   translate_atomic_opcode(nir_atomic_op op)
   {
      /* clang-format off */
   switch (op) {
   case nir_atomic_op_iadd:    return AGX_ATOMIC_OPC_ADD;
   case nir_atomic_op_imin:    return AGX_ATOMIC_OPC_IMIN;
   case nir_atomic_op_umin:    return AGX_ATOMIC_OPC_UMIN;
   case nir_atomic_op_imax:    return AGX_ATOMIC_OPC_IMAX;
   case nir_atomic_op_umax:    return AGX_ATOMIC_OPC_UMAX;
   case nir_atomic_op_iand:    return AGX_ATOMIC_OPC_AND;
   case nir_atomic_op_ior:     return AGX_ATOMIC_OPC_OR;
   case nir_atomic_op_ixor:    return AGX_ATOMIC_OPC_XOR;
   case nir_atomic_op_xchg:    return AGX_ATOMIC_OPC_XCHG;
   case nir_atomic_op_cmpxchg: return AGX_ATOMIC_OPC_CMPXCHG;
   default: unreachable("unknown atomic opcode");
   }
      }
      /*
   * The "base" of a local load/store/atomic can be zero but no other immediates.
   * This would be a little silly to handle when inlining immediates, so we
   * instead exclude these ops from immediate inlining and just handle 0 specially
   * when translating.
   */
   static agx_index
   agx_local_base(nir_src src)
   {
      if (nir_src_is_const(src) && nir_src_as_uint(src) == 0)
         else
      }
      static void
   agx_emit_atomic(agx_builder *b, agx_index dst, nir_intrinsic_instr *instr,
         {
      enum agx_atomic_opc op =
         agx_index base =
         agx_index value = agx_src_index(&instr->src[local ? 1 : 2]);
            /* cmpxchg (only) takes 2 sources, passed in consecutive registers */
   if (op == AGX_ATOMIC_OPC_CMPXCHG) {
      agx_index value2 = agx_src_index(&instr->src[local ? 2 : 3]);
               if (local) {
      assert(base.size == AGX_SIZE_16);
      } else {
      assert(base.size == AGX_SIZE_64);
         }
      static enum agx_format
   format_for_bitsize(unsigned bitsize)
   {
      switch (bitsize) {
   case 8:
         case 16:
         case 32:
         default:
            }
      static void
   agx_emit_local_load(agx_builder *b, agx_index dst, nir_intrinsic_instr *instr)
   {
      agx_index base = agx_local_base(instr->src[0]);
   agx_index index = agx_zero(); /* TODO: optimize address arithmetic */
            enum agx_format format = format_for_bitsize(instr->def.bit_size);
   unsigned nr = instr->def.num_components;
            agx_local_load_to(b, dst, base, index, format, mask);
      }
      static void
   agx_emit_local_store(agx_builder *b, nir_intrinsic_instr *instr)
   {
      agx_index value = agx_src_index(&instr->src[0]);
   agx_index base = agx_local_base(instr->src[1]);
   agx_index index = agx_zero(); /* TODO: optimize address arithmetic */
            enum agx_format format = format_for_bitsize(nir_src_bit_size(instr->src[0]));
   unsigned mask = BITFIELD_MASK(
               }
      /*
   * In the hardware, bindless texture sources are specified as a 64-bit uniform
   * base address summed with a 32-bit register index. In NIR, we model this as a
   * vec2, where the first source is the (constant) uniform register number and
   * the second source is the (dynamic) byte offset.
   */
   static agx_index
   agx_translate_bindless_handle(agx_builder *b, nir_src *handle, agx_index *base)
   {
      nir_scalar base_scalar = nir_scalar_resolved(handle->ssa, 0);
            unsigned base_uint = nir_scalar_as_uint(base_scalar);
               }
      /*
   * Contrary to NIR, in the hardware txf requires a special sampler. The sampler
   * cannot be arbitrary, since the hardware honours the clamps so particular
   * configuration is required for correct out-of-bounds behaviour for txf. This
   * helper gets the shader's txf sampler, allocating one if needed.
   */
   static agx_index
   agx_txf_sampler(agx_context *ctx)
   {
      if (!ctx->out->uses_txf) {
      ctx->out->txf_sampler = BITSET_LAST_BIT(ctx->nir->info.samplers_used);
                  }
      static unsigned
   agx_expand_tex_to(agx_builder *b, nir_def *def, agx_index src, bool masked)
   {
      unsigned nr_channels = def->num_components;
            if (!masked)
            agx_index packed_channels[4] = {agx_null()};
            /* Hardware writes the masked components contiguously, expand out for NIR */
            for (unsigned i = 0; i < nr_channels; ++i) {
      unpacked_channels[i] =
      (mask & BITFIELD_BIT(i))
                  agx_emit_collect_to(b, agx_def_index(def), nr_channels, unpacked_channels);
      }
      static agx_instr *
   agx_emit_image_load(agx_builder *b, agx_index dst, nir_intrinsic_instr *intr)
   {
      agx_index ms_index = agx_src_index(&intr->src[2]);
   agx_index lod = agx_src_index(&intr->src[3]);
            agx_index bindless = agx_immediate(0), texture;
   if (intr->intrinsic == nir_intrinsic_bindless_image_load)
         else if (nir_src_is_const(intr->src[0]) &&
               else
            assert(nir_src_num_components(intr->src[1]) == 4);
   agx_index coord[4] = {
      agx_extract_nir_src(b, intr->src[1], 0),
   agx_extract_nir_src(b, intr->src[1], 1),
   agx_extract_nir_src(b, intr->src[1], 2),
               enum glsl_sampler_dim dim = nir_intrinsic_image_dim(intr);
   bool is_array = nir_intrinsic_image_array(intr);
   bool is_ms = dim == GLSL_SAMPLER_DIM_MS;
   unsigned coord_comps = glsl_get_sampler_dim_coordinate_components(dim);
   if (is_array && is_ms) {
      agx_index layer = agx_temp(b->shader, AGX_SIZE_16);
   agx_subdivide_to(b, layer, coord[coord_comps], 0);
      } else if (is_ms) {
      agx_index tmp = agx_temp(b->shader, AGX_SIZE_32);
   agx_mov_to(b, tmp, ms_index);
      } else if (is_array) {
                  /* Multisampled images do not support mipmapping */
   if (is_ms) {
      lod_mode = AGX_LOD_MODE_AUTO_LOD;
               agx_index coords = agx_emit_collect(b, coord_comps, coord);
            agx_instr *I = agx_image_load_to(
      b, tmp, coords, lod, bindless, texture, agx_txf_sampler(b->shader),
      I->mask = agx_expand_tex_to(b, &intr->def, tmp, true);
      }
      static agx_instr *
   agx_emit_image_store(agx_builder *b, nir_intrinsic_instr *instr)
   {
      enum glsl_sampler_dim glsl_dim = nir_intrinsic_image_dim(instr);
   enum agx_dim dim = agx_tex_dim(glsl_dim, nir_intrinsic_image_array(instr));
            agx_index base, index;
   if (instr->intrinsic == nir_intrinsic_bindless_image_store) {
               assert(base.size == AGX_SIZE_64);
      } else {
      base = agx_zero();
                        agx_index coords4 = agx_src_index(&instr->src[1]);
   agx_index lod = agx_src_index(&instr->src[4]);
            int coord_components = glsl_get_sampler_dim_coordinate_components(glsl_dim);
   if (nir_intrinsic_image_array(instr))
            agx_index coord_comps[4] = {};
   for (unsigned i = 0; i < coord_components; ++i)
            agx_index coords = agx_emit_collect(b, coord_components, coord_comps);
            /* If the image format has less than 4 components, nir_opt_shrink_stores can
   * shrink the store. But the IR still expects 4 components: pad with undef.
   */
   if (nir_src_num_components(instr->src[3]) < 4) {
               for (unsigned i = 0; i < 4; ++i) {
      if (i < nir_src_num_components(instr->src[3]))
         else
                              }
      static agx_instr *
   agx_emit_intrinsic(agx_builder *b, nir_intrinsic_instr *instr)
   {
      agx_index dst = nir_intrinsic_infos[instr->intrinsic].has_dest
                        switch (instr->intrinsic) {
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_at_offset:
      /* handled later via load_vary */
      case nir_intrinsic_load_interpolated_input:
      assert(stage == MESA_SHADER_FRAGMENT);
   agx_emit_load_vary(b, dst, instr);
         case nir_intrinsic_load_coefficients_agx:
      assert(stage == MESA_SHADER_FRAGMENT);
   agx_emit_load_coefficients(b, dst, instr);
         case nir_intrinsic_load_agx:
   case nir_intrinsic_load_constant_agx:
      agx_emit_load(b, dst, instr);
         case nir_intrinsic_store_output:
      assert(stage == MESA_SHADER_VERTEX);
         case nir_intrinsic_store_agx:
      agx_emit_store(b, instr);
         case nir_intrinsic_store_shared:
      agx_emit_local_store(b, instr);
         case nir_intrinsic_load_shared:
      agx_emit_local_load(b, dst, instr);
         case nir_intrinsic_global_atomic_agx:
   case nir_intrinsic_global_atomic_swap_agx:
      agx_emit_atomic(b, dst, instr, false);
         case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
      agx_emit_atomic(b, dst, instr, true);
         case nir_intrinsic_store_zs_agx:
      assert(stage == MESA_SHADER_FRAGMENT);
         case nir_intrinsic_store_local_pixel_agx:
      assert(stage == MESA_SHADER_FRAGMENT);
         case nir_intrinsic_load_local_pixel_agx:
      assert(stage == MESA_SHADER_FRAGMENT);
   agx_emit_local_load_pixel(b, dst, instr);
         case nir_intrinsic_load_pixel_coord:
      return agx_emit_collect_to(
      b, dst, 2,
   (agx_index[2]){
      agx_get_sr(b, 16, AGX_SR_THREAD_POSITION_IN_GRID_X),
         case nir_intrinsic_load_frag_coord_zw: {
      agx_index cf = agx_get_cf(b->shader, true, false, VARYING_SLOT_POS,
                        case nir_intrinsic_sample_mask_agx: {
      assert(stage == MESA_SHADER_FRAGMENT);
            agx_wait_pix(b, 0x0001);
   return agx_sample_mask(b, agx_src_index(&instr->src[0]),
               case nir_intrinsic_load_back_face_agx:
            case nir_intrinsic_load_sample_mask_in:
            case nir_intrinsic_load_sample_mask:
            case nir_intrinsic_load_helper_invocation:
      /* Compare special register to zero. We could lower this in NIR (letting
   * us fold in an inot) but meh?
   */
   return agx_icmp_to(b, dst,
               case nir_intrinsic_load_vertex_id:
      assert(b->shader->stage == MESA_SHADER_VERTEX);
         case nir_intrinsic_load_instance_id:
      assert(b->shader->stage == MESA_SHADER_VERTEX);
         case nir_intrinsic_load_preamble:
            case nir_intrinsic_store_preamble:
            case nir_intrinsic_image_load:
   case nir_intrinsic_bindless_image_load:
            case nir_intrinsic_image_store:
   case nir_intrinsic_bindless_image_store:
            case nir_intrinsic_block_image_store_agx:
            case nir_intrinsic_load_workgroup_id:
      return agx_load_compute_dimension(b, dst, instr,
         case nir_intrinsic_load_workgroup_size:
      return agx_load_compute_dimension(b, dst, instr,
         case nir_intrinsic_load_global_invocation_id:
   case nir_intrinsic_load_global_invocation_id_zero_base:
      return agx_load_compute_dimension(b, dst, instr,
         case nir_intrinsic_load_local_invocation_id:
      return agx_load_compute_dimension(
         case nir_intrinsic_load_local_invocation_index:
            case nir_intrinsic_barrier: {
                        if (nir_intrinsic_memory_scope(instr) != SCOPE_NONE) {
                              if (modes & nir_var_image) {
      agx_image_barrier_1(b);
   agx_image_barrier_2(b);
                  if (nir_intrinsic_execution_scope(instr) != SCOPE_NONE) {
      assert(nir_intrinsic_execution_scope(instr) > SCOPE_SUBGROUP &&
                              if (needs_image_barriers) {
      agx_image_barrier_3(b);
                           case nir_intrinsic_fence_pbe_to_tex_agx: {
      agx_image_barrier_1(b);
   agx_image_barrier_2(b);
   agx_image_barrier_3(b);
   agx_image_barrier_4(b);
               case nir_intrinsic_fence_mem_to_tex_agx: {
      /* Flush out the atomic to main memory */
            /* TODO: Which ones do we actually need? */
   agx_image_barrier_1(b);
   agx_image_barrier_2(b);
   agx_image_barrier_3(b);
            /* Flush out the texture cache */
   agx_flush_memory_to_texture(b);
               case nir_intrinsic_fence_pbe_to_tex_pixel_agx: {
      agx_image_barrier_1(b);
   agx_image_barrier_2(b);
   agx_flush_memory_to_texture(b);
   agx_image_barrier_3(b);
               case nir_intrinsic_begin_invocation_interlock: {
      if (!b->shader->did_writeout &&
                  b->shader->did_writeout = true;
               case nir_intrinsic_load_barycentric_sample:
   case nir_intrinsic_load_sample_id:
   case nir_intrinsic_load_sample_pos:
            default:
      fprintf(stderr, "Unhandled intrinsic %s\n",
               }
      static agx_index
   agx_alu_src_index(agx_builder *b, nir_alu_src src)
   {
      /* Check well-formedness of the input NIR */
   ASSERTED unsigned bitsize = nir_src_bit_size(src.src);
   unsigned comps = nir_src_num_components(src.src);
            assert(bitsize == 1 || bitsize == 8 || bitsize == 16 || bitsize == 32 ||
                     }
      /*
   * Emit an instruction translating (s0 * s1) + (s2 << s3). Assuming s3 is
   * constant, this is an imad instruction. If s1 == 1, then this is optimized to
   * an iadd instruction, which is faster.
   */
   static agx_instr *
   agx_emit_imadshl_agx(agx_builder *b, nir_alu_instr *alu, agx_index dst,
         {
      /* If the shift is not constant, use a variable shift. This should never
   * happen in practice but we don't want to constrain the NIR.
   */
   unsigned shift;
   if (!nir_src_is_const(alu->src[3].src)) {
      s2 = agx_bfi(b, agx_immediate(0), s2, s3, 0);
      } else {
                           /* Emit iadd if possible, else imad */
   if (nir_src_is_const(alu->src[1].src) &&
                  } else {
            }
      static bool
   is_conversion_to_8bit(nir_op op)
   {
      switch (op) {
   case nir_op_i2i8:
   case nir_op_u2u8:
   case nir_op_f2i8:
   case nir_op_f2u8:
   case nir_op_b2i8:
         default:
            }
      static agx_instr *
   agx_emit_alu(agx_builder *b, nir_alu_instr *instr)
   {
      unsigned srcs = nir_op_infos[instr->op].num_inputs;
   unsigned sz = instr->def.bit_size;
   unsigned src_sz = srcs ? nir_src_bit_size(instr->src[0].src) : 0;
            assert(comps == 1 || nir_op_is_vec_or_mov(instr->op));
   assert(
      sz == 1 ||
   ((nir_op_is_vec_or_mov(instr->op) || is_conversion_to_8bit(instr->op)) &&
   sz == 8) ||
         agx_index dst = agx_def_index(&instr->def);
   agx_index s0 = srcs > 0 ? agx_alu_src_index(b, instr->src[0]) : agx_null();
   agx_index s1 = srcs > 1 ? agx_alu_src_index(b, instr->src[1]) : agx_null();
   agx_index s2 = srcs > 2 ? agx_alu_src_index(b, instr->src[2]) : agx_null();
            agx_index i0 = agx_immediate(0);
         #define UNOP(nop, aop)                                                         \
      case nir_op_##nop:                                                          \
      #define BINOP(nop, aop)                                                        \
      case nir_op_##nop:                                                          \
      #define TRIOP(nop, aop)                                                        \
      case nir_op_##nop:                                                          \
            switch (instr->op) {
      BINOP(fadd, fadd);
   BINOP(fmul, fmul);
            UNOP(f2f16, fmov);
   UNOP(f2f16_rtne, fmov);
   UNOP(f2f32, fmov);
   UNOP(fround_even, roundeven);
   UNOP(ftrunc, trunc);
   UNOP(ffloor, floor);
   UNOP(fceil, ceil);
   UNOP(frcp, rcp);
   UNOP(frsq, rsqrt);
   UNOP(flog2, log2);
            UNOP(fddx, dfdx);
   UNOP(fddx_coarse, dfdx);
            UNOP(fddy, dfdy);
   UNOP(fddy_coarse, dfdy);
            UNOP(mov, mov);
   UNOP(u2u32, mov);
   UNOP(bitfield_reverse, bitrev);
   UNOP(bit_count, popcount);
   UNOP(ufind_msb, ffs);
   BINOP(iand, and);
   BINOP(ior, or);
   BINOP(ixor, xor);
         case nir_op_feq:
         case nir_op_flt:
         case nir_op_fge:
         case nir_op_fneu:
            case nir_op_ieq:
         case nir_op_ine:
         case nir_op_ilt:
         case nir_op_ige:
         case nir_op_ult:
         case nir_op_uge:
            case nir_op_inot:
      if (sz == 1)
         else
         case nir_op_b2b1:
            case nir_op_fsqrt:
         case nir_op_fabs:
         case nir_op_fneg:
            case nir_op_fmin: {
      agx_index tmp = agx_fcmpsel(b, s0, s1, s0, s1, AGX_FCOND_LTN);
   /* flush denorms */
      }
   case nir_op_fmax: {
      agx_index tmp = agx_fcmpsel(b, s0, s1, s0, s1, AGX_FCOND_GTN);
   /* flush denorms */
      }
   case nir_op_imin:
         case nir_op_imax:
         case nir_op_umin:
         case nir_op_umax:
            case nir_op_iadd:
         case nir_op_imadshl_agx:
         case nir_op_imsubshl_agx:
         case nir_op_isub:
         case nir_op_ineg:
         case nir_op_imul:
         case nir_op_umul_2x32_64:
         case nir_op_imul_2x32_64:
         case nir_op_umul_high:
         case nir_op_imul_high:
            case nir_op_ishl:
         case nir_op_ushr:
         case nir_op_ishr:
            case nir_op_extr_agx:
      return agx_extr_to(b, dst, s0, s1, s2,
         case nir_op_ubitfield_extract: {
      unsigned m = nir_alu_src_as_uint(instr->src[2]);
            /* Disable masking if the whole thing is used */
   if (m >= 32)
                        case nir_op_bcsel:
            case nir_op_b2i32:
   case nir_op_b2i16:
   case nir_op_b2i8:
            case nir_op_b2b32:
      return agx_icmpsel_to(b, dst, s0, i0, i0, agx_mov_imm(b, 32, 0xFFFFFFFF),
         case nir_op_b2f16:
   case nir_op_b2f32: {
      /* At this point, boolean is just zero/nonzero, so compare with zero */
   agx_index f1 = (sz == 16) ? agx_mov_imm(b, 16, _mesa_float_to_half(1.0))
                        case nir_op_i2i32: {
      if (src_sz == 8) {
      /* Sign extend in software, NIR likes 8-bit conversions */
   agx_index ishl16 = agx_bfi(b, i0, s0, agx_immediate(8), 0);
      } else {
      assert(s0.size == AGX_SIZE_16 && "other conversions lowered");
                  case nir_op_i2i16: {
      if (src_sz == 8) {
      /* Sign extend in software, NIR likes 8-bit conversions */
   agx_index ishl16 = agx_bfi(b, i0, s0, agx_immediate(8), 0);
      } else {
      assert(s0.size == AGX_SIZE_32 && "other conversions lowered");
                  case nir_op_u2u16: {
      if (s0.size == AGX_SIZE_32)
         else
               /* It will be put into a 16-bit register, but zero out the garbage. We could
   * optimize this in the future but it ensures correctness for u2u16(u2u8(x))
   * sequences.
   */
   case nir_op_u2u8:
   case nir_op_i2i8:
            case nir_op_iadd_sat: {
      agx_instr *I = agx_iadd_to(b, dst, s0, s1, 0);
   I->saturate = true;
               case nir_op_isub_sat: {
      agx_instr *I = agx_iadd_to(b, dst, s0, agx_neg(s1), 0);
   I->saturate = true;
               case nir_op_uadd_sat: {
      agx_instr *I = agx_iadd_to(b, dst, agx_abs(s0), agx_abs(s1), 0);
   I->saturate = true;
               case nir_op_usub_sat: {
      agx_instr *I = agx_iadd_to(b, dst, agx_abs(s0), agx_neg(agx_abs(s1)), 0);
   I->saturate = true;
               case nir_op_fsat: {
      agx_instr *I = agx_fadd_to(b, dst, s0, agx_negzero());
   I->saturate = true;
               case nir_op_fsin_agx: {
      agx_index fixup = agx_sin_pt_1(b, s0);
   agx_index sinc = agx_sin_pt_2(b, fixup);
               case nir_op_f2i16:
      return agx_convert_to(b, dst, agx_immediate(AGX_CONVERT_F_TO_S16), s0,
         case nir_op_f2i32:
      return agx_convert_to(b, dst, agx_immediate(AGX_CONVERT_F_TO_S32), s0,
         case nir_op_f2u16:
      return agx_convert_to(b, dst, agx_immediate(AGX_CONVERT_F_TO_U16), s0,
         case nir_op_f2u32:
      return agx_convert_to(b, dst, agx_immediate(AGX_CONVERT_F_TO_U32), s0,
         case nir_op_u2f16:
   case nir_op_u2f32: {
      if (src_sz == 64)
            enum agx_convert mode = (src_sz == 32)   ? AGX_CONVERT_U32_TO_F
                              case nir_op_i2f16:
   case nir_op_i2f32: {
      if (src_sz == 64)
            enum agx_convert mode = (src_sz == 32)   ? AGX_CONVERT_S32_TO_F
                              case nir_op_pack_32_2x16_split:
   case nir_op_pack_64_2x32_split: {
      agx_index idx[] = {s0, s1};
               case nir_op_unpack_64_2x32_split_x:
   case nir_op_unpack_32_2x16_split_x:
            case nir_op_unpack_64_2x32_split_y:
   case nir_op_unpack_32_2x16_split_y:
            case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4: {
      agx_index idx[] = {s0, s1, s2, s3};
               case nir_op_vec8:
   case nir_op_vec16:
            default:
      fprintf(stderr, "Unhandled ALU op %s\n", nir_op_infos[instr->op].name);
         }
      static enum agx_lod_mode
   agx_lod_mode_for_nir(nir_texop op)
   {
      switch (op) {
   case nir_texop_tex:
   case nir_texop_tg4:
         case nir_texop_txb:
         case nir_texop_txd:
         case nir_texop_txl:
         case nir_texop_txf:
         case nir_texop_txf_ms:
         default:
            }
      static enum agx_gather
   agx_gather_for_nir(nir_tex_instr *tex)
   {
      if (tex->op == nir_texop_tg4) {
      enum agx_gather components[] = {
      AGX_GATHER_R,
   AGX_GATHER_G,
   AGX_GATHER_B,
               assert(tex->component < ARRAY_SIZE(components));
      } else {
            }
      static void
   agx_emit_tex(agx_builder *b, nir_tex_instr *instr)
   {
      agx_index coords = agx_null(), bindless = agx_immediate(0),
            texture = agx_immediate(instr->texture_index),
   sampler = agx_immediate(instr->sampler_index),
                  if (txf)
            for (unsigned i = 0; i < instr->num_srcs; ++i) {
               switch (instr->src[i].src_type) {
   case nir_tex_src_backend1:
                  case nir_tex_src_backend2:
                  case nir_tex_src_lod:
   case nir_tex_src_bias:
                  case nir_tex_src_comparator:
      assert(index.size == AGX_SIZE_32);
               case nir_tex_src_texture_offset:
      texture = index;
      case nir_tex_src_sampler_offset:
                  case nir_tex_src_texture_handle:
      texture =
               case nir_tex_src_ddx: {
                                             /* We explicitly don't cache about the split cache for this */
                  for (unsigned i = 0; i < n; ++i) {
      I->src[(2 * i) + 0] = agx_emit_extract(b, index, i);
                           case nir_tex_src_ddy:
                  default:
                              /* Pack shadow reference value (compare) and packed offset together */
            if (!agx_is_null(compare) && !agx_is_null(packed_offset))
         else if (!agx_is_null(packed_offset))
         else if (!agx_is_null(compare))
            agx_index tmp = agx_temp(b->shader, dst.size);
   agx_instr *I = agx_texture_sample_to(
      b, tmp, coords, lod, bindless, texture, sampler, compare_offset,
   agx_tex_dim(instr->sampler_dim, instr->is_array),
   agx_lod_mode_for_nir(instr->op), 0, 0, !agx_is_null(packed_offset),
         if (txf)
            /* Destination masking doesn't seem to work properly for gathers (because
   * it's mostly pointless), but it does show up in the lowering of
   * textureGatherOffsets. Don't try to mask the destination for gathers.
   */
   bool masked = (instr->op != nir_texop_tg4);
      }
      /*
   * Determine if a NIR loop (CF list) uses a continue jump, including within
   * if-else statements but not including nested loops.
   */
   static bool
   cf_list_uses_continue(struct exec_list *list)
   {
      foreach_list_typed(nir_cf_node, node, node, list) {
      if (node->type == nir_cf_node_block) {
               nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_jump &&
      nir_instr_as_jump(instr)->type == nir_jump_continue)
      } else if (node->type == nir_cf_node_if) {
               if (cf_list_uses_continue(&nif->then_list) ||
      cf_list_uses_continue(&nif->else_list))
   } else {
                        }
      static bool
   loop_uses_continue(nir_loop *loop)
   {
         }
      /*
   * NIR loops are treated as a pair of AGX loops:
   *
   *    do {
   *       do {
   *          ...
   *       } while (0);
   *    } while (cond);
   *
   * By manipulating the nesting counter, we may break out of nested loops, so
   * under the model, both break and continue may be implemented as breaks, where
   * break breaks out of the outer loop (2 layers) and continue breaks out of the
   * inner loop (1 layer).
   *
   * After manipulating the nesting counter directly, pop_exec #0 must be used to
   * flush the update to the execution mask.
   */
   static void
   agx_emit_jump(agx_builder *b, nir_jump_instr *instr)
   {
      agx_context *ctx = b->shader;
            /* Break out of either one or two loops */
            if (instr->type == nir_jump_continue) {
      nestings += 1;
      } else if (instr->type == nir_jump_break) {
      nestings += ctx->loop_continues ? 2 : 1;
               agx_break(b, nestings, ctx->break_block);
      }
      static void
   agx_emit_phi(agx_builder *b, nir_phi_instr *instr)
   {
      agx_instr *I =
            /* Deferred */
      }
      /* Look up the AGX block corresponding to a given NIR block. Used when
   * translating phi nodes after emitting all blocks.
   */
   static agx_block *
   agx_from_nir_block(agx_context *ctx, nir_block *block)
   {
         }
      static void
   agx_emit_phi_deferred(agx_context *ctx, agx_block *block, agx_instr *I)
   {
               /* Guaranteed by lower_phis_to_scalar */
            nir_foreach_phi_src(src, phi) {
      agx_block *pred = agx_from_nir_block(ctx, src->pred);
   unsigned i = agx_predecessor_index(block, pred);
                  }
      static void
   agx_emit_phis_deferred(agx_context *ctx)
   {
      agx_foreach_block(ctx, block) {
      agx_foreach_phi_in_block(block, I)
         }
      static void
   agx_emit_undef(agx_builder *b, nir_undef_instr *instr)
   {
      /* For now, just lower undefs to zero. This doesn't matter too much, since
   * the lowering happens in NIR and this just allows for late lowering passes
   * to result in undefs.
   */
      }
      static void
   agx_emit_instr(agx_builder *b, struct nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_load_const:
      agx_emit_load_const(b, nir_instr_as_load_const(instr));
         case nir_instr_type_intrinsic:
      agx_emit_intrinsic(b, nir_instr_as_intrinsic(instr));
         case nir_instr_type_alu:
      agx_emit_alu(b, nir_instr_as_alu(instr));
         case nir_instr_type_tex:
      agx_emit_tex(b, nir_instr_as_tex(instr));
         case nir_instr_type_jump:
      agx_emit_jump(b, nir_instr_as_jump(instr));
         case nir_instr_type_phi:
      agx_emit_phi(b, nir_instr_as_phi(instr));
         case nir_instr_type_undef:
      agx_emit_undef(b, nir_instr_as_undef(instr));
         default:
            }
      static agx_block *
   agx_create_block(agx_context *ctx)
   {
                           }
      static agx_block *
   emit_block(agx_context *ctx, nir_block *block)
   {
      if (ctx->after_block) {
      ctx->current_block = ctx->after_block;
      } else {
                  agx_block *blk = ctx->current_block;
   list_addtail(&blk->link, &ctx->blocks);
                              nir_foreach_instr(instr, block) {
                     }
      static agx_block *emit_cf_list(agx_context *ctx, struct exec_list *list);
      /* Emit if-else as
   *
   *    if_icmp cond != 0
   *       ...
   *    else_icmp cond == 0
   *       ...
   *    pop_exec
   *
   * If the else is empty, we can omit the else_icmp. This happens elsewhere, as
   * an empty else block can become nonempty after RA due to phi lowering. This is
   * not usually optimal, but it's a start.
   */
      static void
   emit_if(agx_context *ctx, nir_if *nif)
   {
      agx_block *first_block = ctx->current_block;
   agx_builder _b = agx_init_builder(ctx, agx_after_block(first_block));
            agx_instr *if_ = agx_if_icmp(&_b, cond, agx_zero(), 1, AGX_ICOND_UEQ, true,
         ctx->loop_nesting++;
            /* Emit the two subblocks. */
   agx_block *if_block = emit_cf_list(ctx, &nif->then_list);
                     agx_block *else_block = emit_cf_list(ctx, &nif->else_list);
            /* If the "if" fails, we fallthrough to the else */
            /* Insert an else instruction at the beginning of the else block. We use
   * "else_fcmp 0.0, 0.0, eq" as unconditional else, matching the blob.
   *
   * If it fails, we fall through to the logical end of the last else block.
   */
   _b.cursor = agx_before_block(else_block);
                     agx_block_add_successor(first_block, if_block);
   agx_block_add_successor(first_block, else_block);
   agx_block_add_successor(end_then, ctx->after_block);
            _b.cursor = agx_after_block(ctx->current_block);
   agx_pop_exec(&_b, 1);
   ctx->loop_nesting--;
      }
      static void
   emit_loop(agx_context *ctx, nir_loop *nloop)
   {
      assert(!nir_loop_has_continue_construct(nloop));
   /* We only track nesting within the innermost loop, so push and reset */
   unsigned pushed_nesting = ctx->loop_nesting;
   ctx->loop_nesting = 0;
            bool old_continues = ctx->loop_continues;
            agx_block *popped_break = ctx->break_block;
            ctx->break_block = agx_create_block(ctx);
            /* If we are emitting a loop inside other control flow, there might be
   * threads masked off (TODO: divergence analysis), so push_exec them so
   * we get the lower nesting count values to ourselves.
   */
   agx_builder _b = agx_init_builder(ctx, agx_after_block(ctx->current_block));
   if (ctx->total_nesting > 1)
            /* Fallthrough to body */
            /* Emit the body */
   ctx->after_block = ctx->continue_block;
   ctx->after_block->loop_header = true;
            /* If we used any continue jumps, we need to reactivate the continued
   * threads. We do this with an always true while_icmp, which behaves like:
   *
   *    if (r0l == 1) {
   *       r0l = 0;
   *    }
   *    update_exec
   *
   * If we did not use continue, this would be a no-op so it is omitted.
   */
            if (ctx->loop_continues) {
      agx_while_icmp(
      &_b, agx_zero(), agx_zero(), 2, AGX_ICOND_UEQ, false,
            agx_jmp_exec_any(&_b, start_block);
   agx_pop_exec(&_b, ctx->loop_continues ? 2 : 1);
            /* Pop off */
   ctx->after_block = ctx->break_block;
   ctx->break_block = popped_break;
            /* Update shader-db stats */
            /* All nested control flow must have finished */
            /* Restore loop nesting (we might be inside an if inside an outer loop) */
   ctx->loop_nesting = pushed_nesting;
   ctx->total_nesting--;
      }
      /* Before the first control flow structure, the nesting counter needs to be
   * zeroed for correct operation. This only happens at most once, since by
   * definition this occurs at the end of the first block, which dominates the
   * rest of the program. */
      static void
   emit_first_cf(agx_context *ctx)
   {
      if (ctx->any_cf)
            agx_builder _b = agx_init_builder(ctx, agx_after_block(ctx->current_block));
   agx_begin_cf(&_b);
      }
      static agx_block *
   emit_cf_list(agx_context *ctx, struct exec_list *list)
   {
               foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block: {
                                          case nir_cf_node_if:
      emit_first_cf(ctx);
               case nir_cf_node_loop:
      emit_first_cf(ctx);
               default:
                        }
      static void
   agx_set_st_vary_final(agx_context *ctx)
   {
      agx_foreach_instr_global_rev(ctx, I) {
      if (I->op == AGX_OPCODE_ST_VARY) {
      I->last = true;
                  /* If we got here, there was no varying written. We need to mark that. */
   agx_block *last_block = list_last_entry(&ctx->blocks, agx_block, link);
   agx_builder _b = agx_init_builder(ctx, agx_after_block_logical(last_block));
      }
      static int
   agx_dump_stats(agx_context *ctx, unsigned size, char **out)
   {
               /* Count instructions */
   agx_foreach_instr_global(ctx, I)
            unsigned nr_threads =
            return asprintf(out,
                  "%s shader: %u inst, %u bytes, %u halfregs, %u threads, "
   }
      static int
   glsl_type_size(const struct glsl_type *type, bool bindless)
   {
         }
      static bool
   agx_lower_sincos_filter(const nir_instr *instr, UNUSED const void *_)
   {
      if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(instr);
      }
      /* Sine and cosine are implemented via the sin_pt_1 and sin_pt_2 opcodes for
   * heavy lifting. sin_pt_2 implements sinc in the first quadrant, expressed in
   * turns (sin (tau x) / x), while sin_pt_1 implements a piecewise sign/offset
   * fixup to transform a quadrant angle [0, 4] to [-1, 1]. The NIR opcode
   * fsin_agx models the fixup, sinc, and multiply to obtain sine, so we just
   * need to change units from radians to quadrants modulo turns. Cosine is
   * implemented by shifting by one quadrant: cos(x) = sin(x + tau/4).
   */
      static nir_def *
   agx_lower_sincos_impl(struct nir_builder *b, nir_instr *instr, UNUSED void *_)
   {
      nir_alu_instr *alu = nir_instr_as_alu(instr);
   nir_def *x = nir_mov_alu(b, alu->src[0], 1);
            if (alu->op == nir_op_fcos)
            nir_def *quadrants = nir_fmul_imm(b, nir_ffract(b, turns), 4.0);
      }
      static bool
   agx_lower_sincos(nir_shader *shader)
   {
      return nir_shader_lower_instructions(shader, agx_lower_sincos_filter,
      }
      static bool
   agx_lower_front_face(struct nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_load_front_face)
            nir_def *def = &intr->def;
            b->cursor = nir_before_instr(&intr->instr);
   nir_def_rewrite_uses(def, nir_inot(b, nir_load_back_face_agx(b, 1)));
      }
      /*
   * Standard NIR optimization loop. This is run in agx_preprocess_nir, then once
   * again at shader variant compile time. Unless there was a complex shader key,
   * the latter run should be almost a no-op.
   */
   static void
   agx_optimize_loop_nir(nir_shader *nir)
   {
               do {
               NIR_PASS(progress, nir, nir_lower_var_copies);
            NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_remove_phis);
   NIR_PASS(progress, nir, nir_lower_phis_to_scalar, true);
   NIR_PASS(progress, nir, nir_opt_dce);
   NIR_PASS(progress, nir, nir_opt_dead_cf);
   NIR_PASS(progress, nir, nir_opt_cse);
   NIR_PASS(progress, nir, nir_opt_peephole_select, 64, false, true);
   NIR_PASS(progress, nir, nir_opt_phi_precision);
   NIR_PASS(progress, nir, nir_opt_algebraic);
            NIR_PASS(progress, nir, nir_opt_undef);
            NIR_PASS(progress, nir, nir_opt_shrink_vectors);
         }
      static void
   agx_optimize_nir(nir_shader *nir, unsigned *preamble_size)
   {
      /* This runs only once up front since other optimizations don't affect it */
                     bool progress = false;
            /* If address lowering made progress, clean up before forming preambles.
   * Otherwise the optimized preambles might just be constants! Do it before
   * lowering int64 too, to avoid lowering constant int64 arithmetic.
   */
   if (progress) {
      NIR_PASS_V(nir, nir_opt_constant_folding);
               /* Only lower int64 after optimizing address arithmetic, so that u2u64/i2i64
   * conversions remain.
   */
   progress = false;
            /* If we lowered actual int64 arithmetic (not folded into the address
   * calculations), then clean up after the lowering.
   */
   if (progress) {
      do {
               NIR_PASS(progress, nir, nir_opt_algebraic);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
                  if (likely(!(agx_compiler_debug & AGX_DBG_NOPREAMBLE)))
            /* Forming preambles may dramatically reduce the instruction count
   * in certain blocks, causing some if-else statements to become
   * trivial. We want to peephole select those, given that control flow
   * prediction instructions are costly.
   */
                     /* Fuse add/sub/multiplies/shifts after running opt_algebraic_late to fuse
   * isub but before shifts are lowered.
   */
   do {
               NIR_PASS(progress, nir, nir_opt_dce);
   NIR_PASS(progress, nir, nir_opt_cse);
               /* Do remaining lowering late, since this inserts &s for shifts so we want to
   * do it after fusing constant shifts. Constant folding will clean up.
   */
   NIR_PASS_V(nir, agx_nir_lower_algebraic_late);
   NIR_PASS_V(nir, nir_opt_constant_folding);
            /* Must run after uses are fixed but before a last round of copyprop + DCE */
   if (nir->info.stage == MESA_SHADER_FRAGMENT)
            NIR_PASS_V(nir, nir_copy_prop);
   NIR_PASS_V(nir, nir_opt_dce);
   NIR_PASS_V(nir, nir_opt_cse);
   NIR_PASS_V(nir, nir_lower_alu_to_scalar, NULL, NULL);
            /* Cleanup optimizations */
   nir_move_options move_all = nir_move_const_undef | nir_move_load_ubo |
                        NIR_PASS_V(nir, nir_opt_sink, move_all);
   NIR_PASS_V(nir, nir_opt_move, move_all);
      }
      /* ABI: position first, then user, then psiz */
   static void
   agx_remap_varyings_vs(nir_shader *nir, struct agx_varyings_vs *varyings,
         {
               /* Initialize to "nothing is written" */
   for (unsigned i = 0; i < ARRAY_SIZE(varyings->slots); ++i)
            /* gl_Position is implicitly written, although it may validly be absent in
   * vertex programs run only for transform feedback. Those ignore their
   * varyings so it doesn't matter what we do here as long as we don't fail.
   */
   varyings->slots[VARYING_SLOT_POS] = base;
            /* These are always flat-shaded from the FS perspective */
                     /* Smooth 32-bit user bindings go next */
   u_foreach_bit64(loc, nir->info.outputs_written &
                  if (loc == VARYING_SLOT_POS || loc == VARYING_SLOT_PSIZ)
            varyings->slots[loc] = base;
   base += 4;
               /* Flat 32-bit user bindings go next */
   u_foreach_bit64(loc,
            if (loc == VARYING_SLOT_POS || loc == VARYING_SLOT_PSIZ)
            varyings->slots[loc] = base;
   base += 4;
               /* Linear 32-bit user bindings go next */
   u_foreach_bit64(loc,
            if (loc == VARYING_SLOT_POS || loc == VARYING_SLOT_PSIZ)
            varyings->slots[loc] = base;
   base += 4;
               /* TODO: Link FP16 varyings */
   varyings->base_index_fp16 = base;
   varyings->num_16_smooth = 0;
   varyings->num_16_flat = 0;
            if (nir->info.outputs_written & VARYING_BIT_PSIZ) {
      varyings->slots[VARYING_SLOT_PSIZ] = base;
               if (nir->info.outputs_written & VARYING_BIT_LAYER) {
      varyings->layer_viewport_slot = base;
               /* All varyings linked now */
      }
      /*
   * Varyings that are used as texture coordinates should be kept at fp32, because
   * fp16 does not have enough precision for large textures. It's technically
   * conformant not to, but every app gets this wrong.
   */
   static bool
   agx_gather_texcoords(nir_builder *b, nir_instr *instr, void *data)
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
      struct interp_masks {
      uint64_t flat;
      };
      static bool
   agx_gather_interp(nir_builder *b, nir_instr *instr, void *data)
   {
      struct interp_masks *masks = data;
   if (instr->type != nir_instr_type_intrinsic)
                     if (intr->intrinsic == nir_intrinsic_load_input) {
      nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
      } else if (intr->intrinsic == nir_intrinsic_load_interpolated_input &&
            nir_intrinsic_interp_mode(nir_src_as_intrinsic(intr->src[0])) ==
   nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
                  }
      /*
   * Build a bit mask of varyings (by location) that are flatshaded and linear
   * shaded. This information is needed by lower_mediump_io and
   * agx_uncompiled_shader_info.
   */
   static struct interp_masks
   agx_interp_masks(nir_shader *nir)
   {
               struct interp_masks masks = {0};
   nir_shader_instructions_pass(nir, agx_gather_interp, nir_metadata_all,
               }
      /*
   * Build a bit mask of varyings (by location) that are used as texture
   * coordinates. This information is needed by lower_mediump_io.
   */
   static uint64_t
   agx_texcoord_mask(nir_shader *nir)
   {
               uint64_t mask = 0;
   nir_shader_instructions_pass(nir, agx_gather_texcoords, nir_metadata_all,
            }
      static nir_mem_access_size_align
   mem_access_size_align_cb(nir_intrinsic_op intrin, uint8_t bytes,
                     {
                        if ((bytes & 1) || (align == 1))
         else if ((bytes & 2) || (align == 2))
         else if (bit_size >= 32)
            return (nir_mem_access_size_align){
      .num_components = MIN2(bytes / (bit_size / 8), 4),
   .bit_size = bit_size,
         }
      static unsigned
   lower_bit_size_callback(const nir_instr *instr, UNUSED void *_)
   {
      if (instr->type != nir_instr_type_alu)
            /* Lower 8-bit ALU to 16-bit. We check the destination, as we do not want to
   * lower conversions from 8-bit to larger types. Those conversions get
   * implemented natively.
   */
   nir_alu_instr *alu = nir_instr_as_alu(instr);
   if (alu->def.bit_size == 8 && !is_conversion_to_8bit(alu->op))
         else if (alu->def.bit_size == 1 && alu->src[0].src.ssa->bit_size == 8)
         else
      }
      static bool
   agx_should_dump(nir_shader *nir, unsigned agx_dbg_bit)
   {
      return (agx_compiler_debug & agx_dbg_bit) &&
      }
      static unsigned
   agx_compile_function_nir(nir_shader *nir, nir_function_impl *impl,
                           {
               agx_context *ctx = rzalloc(NULL, agx_context);
   ctx->nir = nir;
   ctx->is_preamble = impl->function->is_preamble;
   ctx->out = out;
   ctx->key = key;
   ctx->stage = nir->info.stage;
   ctx->allocated_vec = _mesa_hash_table_u64_create(ctx);
   ctx->indexed_nir_blocks = rzalloc_array(ctx, agx_block *, impl->num_blocks);
            ctx->alloc = impl->ssa_alloc;
   emit_cf_list(ctx, &impl->body);
            /* Stop the main shader or preamble shader after the exit block. For real
   * functions, we would return here.
   */
   agx_block *last_block = list_last_entry(&ctx->blocks, agx_block, link);
   agx_builder _b = agx_init_builder(ctx, agx_after_block(last_block));
            /* Index blocks now that we're done emitting so the order is consistent */
   agx_foreach_block(ctx, block)
                     if (likely(!(agx_compiler_debug & AGX_DBG_NOOPT))) {
      /* Eliminate dead instructions before CSE to avoid silly scheduling */
            /* CSE before eliminating dead destinations so that subdivision is
   * optimized properly.
   */
            /* After DCE, use counts are right so we can run the optimizer. */
               /* For correctness, lower uniform sources after copyprop (for correctness,
   * as copyprop creates uniform sources). To keep register pressure in
   * check, lower after CSE, since moves are cheaper than registers.
   */
            /* RA correctness depends on DCE */
   agx_dce(ctx, true);
            if (agx_should_dump(nir, AGX_DBG_SHADERS))
            if (likely(!(agx_compiler_debug & AGX_DBG_NOSCHED))) {
      agx_pressure_schedule(ctx);
               if (agx_should_dump(nir, AGX_DBG_SHADERS))
            agx_ra(ctx);
            if (ctx->stage == MESA_SHADER_VERTEX && !impl->function->is_preamble)
            agx_insert_waits(ctx);
   agx_opt_empty_else(ctx);
   agx_opt_break_if(ctx);
   agx_opt_jmp_none(ctx);
            if (agx_should_dump(nir, AGX_DBG_SHADERS))
            /* Pad binary */
   if (binary->size % AGX_CODE_ALIGN) {
      unsigned ngrow = AGX_CODE_ALIGN - (binary->size % AGX_CODE_ALIGN);
               unsigned offset = binary->size;
                              if (impl->function->is_preamble)
         else
            /* Don't dump statistics for preambles, since they're not worth optimizing */
   if (!impl->function->is_preamble) {
      char *stats;
            if (ret >= 0) {
      if (agx_should_dump(nir, AGX_DBG_SHADERDB)) {
      fprintf(stderr, "SHADER-DB: %s - %s\n", nir->info.label ?: "",
                                                         }
      /*
   * Preprocess NIR. In particular, this lowers I/O. Drivers should call this
   * as soon as they don't need unlowered I/O.
   *
   * This also lowers as much as possible. After preprocessing NIR, the following
   * NIR passes are called by the GL driver:
   *
   *    - nir_lower_blend
   *    - nir_lower_texcoord_replace_late
   *    - agx_nir_lower_vbo
   *    - agx_nir_lower_tilebuffer
   *
   * Unless an instruction is constructed by one of the above passes, it should be
   * lowered here to avoid duplicate work with shader variants.
   */
   void
   agx_preprocess_nir(nir_shader *nir, bool support_lod_bias, bool allow_mediump,
         {
      if (out)
                     if (nir->info.stage == MESA_SHADER_VERTEX) {
                  /* Lower large arrays to scratch and small arrays to csel */
   NIR_PASS_V(nir, nir_lower_vars_to_scratch, nir_var_function_temp, 16,
         NIR_PASS_V(nir, nir_lower_indirect_derefs, nir_var_function_temp, ~0);
   NIR_PASS_V(nir, nir_split_var_copies);
   NIR_PASS_V(nir, nir_lower_global_vars_to_local);
   NIR_PASS_V(nir, nir_lower_var_copies);
   NIR_PASS_V(nir, nir_lower_vars_to_ssa);
   NIR_PASS_V(nir, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
         NIR_PASS_V(nir, nir_lower_ssbo);
   if (nir->info.stage == MESA_SHADER_FRAGMENT) {
                        /* Interpolate varyings at fp16 and write to the tilebuffer at fp16. As an
   * exception, interpolate flat shaded at fp32. This works around a
   * hardware limitation. The resulting code (with an extra f2f16 at the end
   * if needed) matches what Metal produces.
   */
   if (likely(allow_mediump)) {
               NIR_PASS_V(nir, nir_lower_mediump_io,
                     if (out) {
      out->inputs_flat_shaded = masks.flat;
                  /* Clean up deref gunk after lowering I/O */
   NIR_PASS_V(nir, nir_opt_dce);
            /* Runs before we lower away idiv, to work at all. But runs after lowering
   * textures, since the cube map array lowering generates division by 6.
   */
            nir_lower_idiv_options idiv_options = {
                  NIR_PASS_V(nir, nir_lower_idiv, &idiv_options);
   NIR_PASS_V(nir, nir_lower_frexp);
   NIR_PASS_V(nir, nir_lower_alu_to_scalar, NULL, NULL);
   NIR_PASS_V(nir, nir_lower_load_const_to_scalar);
   NIR_PASS_V(nir, nir_lower_flrp, 16 | 32 | 64, false);
   NIR_PASS_V(nir, agx_lower_sincos);
   NIR_PASS_V(nir, nir_shader_intrinsics_pass, agx_lower_front_face,
                  /* After lowering, run through the standard suite of NIR optimizations. We
   * will run through the loop later, once we have the shader key, but if we
   * run now, that run will ideally be almost a no-op.
   */
            /* We're lowered away all variables. Remove them all for smaller shaders. */
   NIR_PASS_V(nir, nir_remove_dead_variables, nir_var_all, NULL);
            /* Move before lowering */
   nir_move_options move_all = nir_move_const_undef | nir_move_load_ubo |
                  NIR_PASS_V(nir, nir_opt_sink, move_all);
   NIR_PASS_V(nir, nir_opt_move, move_all);
      }
      void
   agx_compile_shader_nir(nir_shader *nir, struct agx_shader_key *key,
                     {
                        assert(nir->info.io_lowered &&
                  out->nr_bindful_textures = BITSET_LAST_BIT(nir->info.textures_used);
            /* If required, tag writes will be enabled by instruction selection */
   if (nir->info.stage == MESA_SHADER_FRAGMENT)
            /* Late tilebuffer lowering creates multisampled image stores */
            /* Late sysval lowering creates large loads. Load lowering creates unpacks */
   nir_lower_mem_access_bit_sizes_options lower_mem_access_options = {
      .modes = nir_var_mem_ssbo | nir_var_mem_constant |
                  };
   NIR_PASS_V(nir, nir_lower_mem_access_bit_sizes, &lower_mem_access_options);
   NIR_PASS_V(nir, nir_lower_bit_size, lower_bit_size_callback, NULL);
            /* Late blend lowering creates vectors */
   NIR_PASS_V(nir, nir_lower_alu_to_scalar, NULL, NULL);
            if (nir->info.stage == MESA_SHADER_FRAGMENT)
            /* Late VBO lowering creates constant udiv instructions */
            /* Varying output is scalar, other I/O is vector. Lowered late because
   * transform feedback programs will use vector output.
   */
   if (nir->info.stage == MESA_SHADER_VERTEX) {
      NIR_PASS_V(nir, nir_lower_io_to_scalar, nir_var_shader_out, NULL, NULL);
               out->push_count = key->reserved_preamble;
            /* Create sample_mask instructions late, since NIR's scheduling is not aware
   * of the ordering requirements between sample_mask and pixel stores.
   *
   * Note: when epilogs are used, special handling is required since the sample
   * count is dynamic when the main fragment shader is compiled.
   */
   if (nir->info.stage == MESA_SHADER_FRAGMENT && key->fs.nr_samples) {
      if (agx_nir_lower_sample_mask(nir, key->fs.nr_samples)) {
      /* Clean up ixor(bcsel) patterns created from sample mask lowering.
   * If this succeeds, we'll have expressions to constant fold to get the
   * benefit. We need to rescalarize after folding constants.
   */
   if (agx_nir_opt_ixor_bcsel(nir)) {
      NIR_PASS_V(nir, nir_opt_constant_folding);
   NIR_PASS_V(nir, nir_lower_load_const_to_scalar);
                     /* Must be last since NIR passes can remap driver_location freely */
   if (nir->info.stage == MESA_SHADER_VERTEX)
            if (agx_should_dump(nir, AGX_DBG_SHADERS))
                     nir_foreach_function_with_impl(func, impl, nir) {
      unsigned offset =
            if (func->is_preamble) {
      out->preamble_offset = offset;
      } else if (func->is_entrypoint) {
         } else {
                     if (nir->info.stage == MESA_SHADER_VERTEX) {
      out->writes_psiz =
            out->writes_layer_viewport =
         } else if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      out->disable_tri_merging = nir->info.fs.needs_all_helper_invocations ||
                  /* Writing the sample mask requires tag writes */
            /* Report a canonical depth layout. This happens at the end because the
   * sample mask lowering affects it.
   */
            if (!(nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH)))
         else if (layout == FRAG_DEPTH_LAYOUT_NONE)
         else
         }
