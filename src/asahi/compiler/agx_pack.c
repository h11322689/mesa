   /*
   * Copyright 2021 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "agx_compiler.h"
      /* Binary patches needed for branch offsets */
   struct agx_branch_fixup {
      /* Offset into the binary to patch */
            /* Value to patch with will be block->offset */
            /* If true, skips to the last instruction of the target block */
      };
      static void
   assert_register_is_aligned(agx_index reg)
   {
               switch (reg.size) {
   case AGX_SIZE_16:
         case AGX_SIZE_32:
      assert((reg.value & 1) == 0 && "unaligned reg");
      case AGX_SIZE_64:
      assert((reg.value & 3) == 0 && "unaligned reg");
                  }
      /* Texturing has its own operands */
   static unsigned
   agx_pack_sample_coords(agx_index index, bool *flag, bool *is_16)
   {
      /* TODO: Do we have a use case for 16-bit coords? */
   assert(index.size == AGX_SIZE_32);
            *is_16 = false;
   *flag = index.discard;
      }
      static unsigned
   agx_pack_texture(agx_index base, agx_index index, unsigned *packed_base,
         {
      if (base.type == AGX_INDEX_IMMEDIATE) {
               /* Texture state registers */
            if (index.type == AGX_INDEX_REGISTER) {
      assert(index.size == AGX_SIZE_16);
      } else {
      assert(index.type == AGX_INDEX_IMMEDIATE);
         } else {
      assert(base.type == AGX_INDEX_UNIFORM);
   assert(base.size == AGX_SIZE_64);
   assert((base.value & 3) == 0);
            /* Bindless */
   *packed_base = base.value >> 2;
                  }
      static unsigned
   agx_pack_sampler(agx_index index, bool *flag)
   {
      if (index.type == AGX_INDEX_REGISTER) {
      assert(index.size == AGX_SIZE_16);
      } else {
      assert(index.type == AGX_INDEX_IMMEDIATE);
                  }
      static unsigned
   agx_pack_sample_compare_offset(agx_index index)
   {
      if (index.type == AGX_INDEX_NULL)
            assert(index.size == AGX_SIZE_32);
   assert(index.value < 0x100);
   assert_register_is_aligned(index);
      }
      static unsigned
   agx_pack_lod(agx_index index, unsigned *lod_mode)
   {
      /* For automatic LOD, the LOD field is unused. Assert as much. */
   if ((*lod_mode) == AGX_LOD_MODE_AUTO_LOD) {
      assert(index.type == AGX_INDEX_IMMEDIATE);
   assert(index.value == 0);
               if (index.type == AGX_INDEX_UNIFORM) {
      /* Translate LOD mode from register mode to uniform mode */
   assert(((*lod_mode) & BITFIELD_BIT(2)) && "must start as reg mode");
   *lod_mode = (*lod_mode) & ~BITFIELD_BIT(2);
      } else {
      /* Otherwise must be registers */
   assert(index.type == AGX_INDEX_REGISTER);
                  }
      static unsigned
   agx_pack_pbe_source(agx_index index, bool *flag)
   {
      assert(index.size == AGX_SIZE_16 || index.size == AGX_SIZE_32);
            *flag = (index.size == AGX_SIZE_32);
      }
      static unsigned
   agx_pack_pbe_lod(agx_index index, bool *flag)
   {
               if (index.type == AGX_INDEX_IMMEDIATE)
         else if (index.type == AGX_INDEX_REGISTER)
         else
               }
      /* Load/stores have their own operands */
      static unsigned
   agx_pack_memory_reg(agx_index index, bool *flag)
   {
               *flag = (index.size >= AGX_SIZE_32);
      }
      static unsigned
   agx_pack_memory_base(agx_index index, bool *flag)
   {
      assert(index.size == AGX_SIZE_64);
            /* Can't seem to access high uniforms from memory instructions */
            if (index.type == AGX_INDEX_UNIFORM) {
         } else {
      assert(index.type == AGX_INDEX_REGISTER);
                  }
      static unsigned
   agx_pack_memory_index(agx_index index, bool *flag)
   {
      if (index.type == AGX_INDEX_IMMEDIATE) {
      assert(index.value < 0x10000);
               } else {
      assert(index.type == AGX_INDEX_REGISTER);
   assert(index.size == AGX_SIZE_32);
   assert((index.value & 1) == 0);
            *flag = 0;
         }
      static uint16_t
   agx_pack_local_base(agx_index index, unsigned *flags)
   {
               if (index.type == AGX_INDEX_IMMEDIATE) {
      assert(index.value == 0);
   *flags = 2;
      } else if (index.type == AGX_INDEX_UNIFORM) {
      *flags = 1 | ((index.value >> 8) << 1);
      } else {
      assert_register_is_aligned(index);
   *flags = 0;
         }
      static uint16_t
   agx_pack_local_index(agx_index index, bool *flag)
   {
               if (index.type == AGX_INDEX_IMMEDIATE) {
      assert(index.value < 0x10000);
   *flag = 1;
      } else {
      assert_register_is_aligned(index);
   *flag = 0;
         }
      static unsigned
   agx_pack_atomic_source(agx_index index)
   {
      assert(index.size == AGX_SIZE_32 && "no 64-bit atomics yet");
   assert_register_is_aligned(index);
      }
      static unsigned
   agx_pack_atomic_dest(agx_index index, bool *flag)
   {
      /* Atomic destinstions are optional (e.g. for update with no return) */
   if (index.type == AGX_INDEX_NULL) {
      *flag = 0;
               /* But are otherwise registers */
   assert(index.size == AGX_SIZE_32 && "no 64-bit atomics yet");
   assert_register_is_aligned(index);
   *flag = 1;
      }
      /* ALU goes through a common path */
      static unsigned
   agx_pack_alu_dst(agx_index dest)
   {
      assert_register_is_aligned(dest);
   unsigned reg = dest.value;
   enum agx_size size = dest.size;
            return (dest.cache ? (1 << 0) : 0) | ((size >= AGX_SIZE_32) ? (1 << 1) : 0) |
      }
      static unsigned
   agx_pack_alu_src(agx_index src)
   {
      unsigned value = src.value;
            if (src.type == AGX_INDEX_IMMEDIATE) {
      /* Flags 0 for an 8-bit immediate */
               } else if (src.type == AGX_INDEX_UNIFORM) {
      assert(size == AGX_SIZE_16 || size == AGX_SIZE_32);
            return (value & BITFIELD_MASK(6)) |
         ((value & BITFIELD_BIT(8)) ? (1 << 6) : 0) |
      } else {
      assert_register_is_aligned(src);
            unsigned hint = src.discard ? 0x3 : src.cache ? 0x2 : 0x1;
   unsigned size_flag = (size == AGX_SIZE_64)   ? 0x3
                        return (value & BITFIELD_MASK(6)) | (hint << 6) | (size_flag << 8) |
         }
      static unsigned
   agx_pack_cmpsel_src(agx_index src, enum agx_size dest_size)
   {
      unsigned value = src.value;
            if (src.type == AGX_INDEX_IMMEDIATE) {
      /* Flags 0x4 for an 8-bit immediate */
               } else if (src.type == AGX_INDEX_UNIFORM) {
      assert(size == AGX_SIZE_16 || size == AGX_SIZE_32);
   assert(size == dest_size);
            return (value & BITFIELD_MASK(6)) | ((value >> 8) << 6) | (0x3 << 7) |
      } else {
      assert(src.type == AGX_INDEX_REGISTER);
   assert(!(src.cache && src.discard));
   assert(size == AGX_SIZE_16 || size == AGX_SIZE_32);
   assert(size == dest_size);
                     return (value & BITFIELD_MASK(6)) | (hint << 6) |
         }
      static unsigned
   agx_pack_sample_mask_src(agx_index src)
   {
      unsigned value = src.value;
   unsigned packed_value =
            if (src.type == AGX_INDEX_IMMEDIATE) {
      assert(value < 0x100);
      } else {
      assert(src.type == AGX_INDEX_REGISTER);
   assert_register_is_aligned(src);
                  }
      static unsigned
   agx_pack_float_mod(agx_index src)
   {
         }
      static bool
   agx_all_16(agx_instr *I)
   {
      agx_foreach_dest(I, d) {
      if (!agx_is_null(I->dest[d]) && I->dest[d].size != AGX_SIZE_16)
               agx_foreach_src(I, s) {
      if (!agx_is_null(I->src[s]) && I->src[s].size != AGX_SIZE_16)
                  }
      /* Generic pack for ALU instructions, which are quite regular */
      static void
   agx_pack_alu(struct util_dynarray *emission, agx_instr *I)
   {
      struct agx_opcode_info info = agx_opcodes_info[I->op];
   bool is_16 = agx_all_16(I) && info.encoding_16.exact;
                     uint64_t raw = encoding.exact;
            // TODO: assert saturable
   if (I->saturate)
            if (info.nr_dests) {
      assert(info.nr_dests == 1);
   unsigned D = agx_pack_alu_dst(I->dest[0]);
            raw |= (D & BITFIELD_MASK(8)) << 7;
      } else if (info.immediates & AGX_IMMEDIATE_NEST) {
      raw |= (I->invert_cond << 8);
   raw |= (I->nest << 11);
               for (unsigned s = 0; s < info.nr_srcs; ++s) {
      bool is_cmpsel = (s >= 2) && (I->op == AGX_OPCODE_ICMPSEL ||
            unsigned src = is_cmpsel ? agx_pack_cmpsel_src(I->src[s], I->dest[0].size)
            unsigned src_short = (src & BITFIELD_MASK(10));
            /* Size bit always zero and so omitted for 16-bit */
   if (is_16 && !is_cmpsel)
            if (info.is_float || (I->op == AGX_OPCODE_FCMPSEL && !is_cmpsel)) {
      unsigned fmod = agx_pack_float_mod(I->src[s]);
   unsigned fmod_offset = is_16 ? 9 : 10;
      } else if (I->op == AGX_OPCODE_IMAD || I->op == AGX_OPCODE_IADD) {
      /* Force unsigned for immediates so uadd_sat works properly */
                           unsigned negate_src = (I->op == AGX_OPCODE_IMAD) ? 2 : 1;
   assert(!I->src[s].neg || s == negate_src);
               /* Sources come at predictable offsets */
   unsigned offset = 16 + (12 * s);
            /* Destination and each source get extended in reverse order */
   unsigned extend_offset = (sizeof(extend) * 8) - ((s + 3) * 2);
               if ((I->op == AGX_OPCODE_IMAD && I->src[2].neg) ||
      (I->op == AGX_OPCODE_IADD && I->src[1].neg))
         if (info.immediates & AGX_IMMEDIATE_TRUTH_TABLE) {
      raw |= (I->truth_table & 0x3) << 26;
      } else if (info.immediates & AGX_IMMEDIATE_SHIFT) {
      assert(I->shift <= 4);
   raw |= (uint64_t)(I->shift & 1) << 39;
      } else if (info.immediates & AGX_IMMEDIATE_BFI_MASK) {
      raw |= (uint64_t)(I->bfi_mask & 0x3) << 38;
   raw |= (uint64_t)((I->bfi_mask >> 2) & 0x3) << 50;
      } else if (info.immediates & AGX_IMMEDIATE_SR) {
      raw |= (uint64_t)(I->sr & 0x3F) << 16;
      } else if (info.immediates & AGX_IMMEDIATE_WRITEOUT)
         else if (info.immediates & AGX_IMMEDIATE_IMM)
         else if (info.immediates & AGX_IMMEDIATE_ROUND)
         else if (info.immediates & (AGX_IMMEDIATE_FCOND | AGX_IMMEDIATE_ICOND))
            /* Determine length bit */
   unsigned length = encoding.length_short;
   uint64_t short_mask = BITFIELD64_MASK(8 * length);
            if (encoding.extensible && length_bit) {
      raw |= (1 << 15);
               /* Pack! */
   if (length <= sizeof(uint64_t)) {
               /* XXX: This is a weird special case */
   if (I->op == AGX_OPCODE_IADD)
            raw |= (uint64_t)extend << extend_offset;
      } else {
      /* So far, >8 byte ALU is only to store the extend bits */
   unsigned extend_offset = (((length - sizeof(extend)) * 8) - 64);
            memcpy(util_dynarray_grow_bytes(emission, 1, 8), &raw, 8);
   memcpy(util_dynarray_grow_bytes(emission, 1, length - 8), &hi,
         }
      static void
   agx_pack_instr(struct util_dynarray *emission, struct util_dynarray *fixups,
         {
      switch (I->op) {
   case AGX_OPCODE_LD_TILE:
   case AGX_OPCODE_ST_TILE: {
      bool load = (I->op == AGX_OPCODE_LD_TILE);
   unsigned D = agx_pack_alu_dst(load ? I->dest[0] : I->src[0]);
   assert(I->mask < 0x10);
            agx_index sample_index = load ? I->src[0] : I->src[1];
   assert(sample_index.type == AGX_INDEX_REGISTER ||
         assert(sample_index.size == AGX_SIZE_16);
   unsigned St = (sample_index.type == AGX_INDEX_REGISTER) ? 1 : 0;
   unsigned S = sample_index.value;
            uint64_t raw = agx_opcodes_info[I->op].encoding.exact |
                  ((uint64_t)(D & BITFIELD_MASK(8)) << 7) | (St << 22) |
   ((uint64_t)(I->format) << 24) |
   ((uint64_t)(I->pixel_offset & BITFIELD_MASK(7)) << 28) |
               unsigned size = 8;
   memcpy(util_dynarray_grow_bytes(emission, 1, size), &raw, size);
               case AGX_OPCODE_SAMPLE_MASK: {
      unsigned S = agx_pack_sample_mask_src(I->src[1]);
   unsigned T = I->src[0].value;
   bool Tt = I->src[0].type == AGX_INDEX_IMMEDIATE;
   assert(Tt || I->src[0].type == AGX_INDEX_REGISTER);
   uint32_t raw = 0xc1 | (Tt ? BITFIELD_BIT(8) : 0) |
                  unsigned size = 4;
   memcpy(util_dynarray_grow_bytes(emission, 1, size), &raw, size);
               case AGX_OPCODE_WAIT: {
      uint64_t raw =
            unsigned size = 2;
   memcpy(util_dynarray_grow_bytes(emission, 1, size), &raw, size);
               case AGX_OPCODE_ITER:
   case AGX_OPCODE_ITERPROJ:
   case AGX_OPCODE_LDCF: {
      bool flat = (I->op == AGX_OPCODE_LDCF);
   bool perspective = (I->op == AGX_OPCODE_ITERPROJ);
   unsigned D = agx_pack_alu_dst(I->dest[0]);
            agx_index src_I = I->src[0];
            unsigned cf_I = src_I.value;
            if (perspective) {
      agx_index src_J = I->src[1];
   assert(src_J.type == AGX_INDEX_IMMEDIATE);
               assert(cf_I < 0x100);
            enum agx_interpolation interp = I->interpolation;
            /* Fix up the interpolation enum to distinguish the sample index source */
   if (interp == AGX_INTERPOLATION_SAMPLE) {
      if (sample_index.type == AGX_INDEX_REGISTER)
         else
      } else {
                  bool kill = false;    // TODO: optimize
            uint64_t raw =
      0x21 | (flat ? (1 << 7) : 0) | (perspective ? (1 << 6) : 0) |
   ((D & 0xFF) << 7) | (1ull << 15) | /* XXX */
   ((cf_I & BITFIELD_MASK(6)) << 16) | ((cf_J & BITFIELD_MASK(6)) << 24) |
   (((uint64_t)channels) << 30) | (((uint64_t)sample_index.value) << 32) |
   (forward ? (1ull << 46) : 0) | (((uint64_t)interp) << 48) |
               unsigned size = 8;
   memcpy(util_dynarray_grow_bytes(emission, 1, size), &raw, size);
               case AGX_OPCODE_ST_VARY: {
      agx_index index_src = I->src[0];
            assert(index_src.type == AGX_INDEX_IMMEDIATE);
   assert(index_src.value < BITFIELD_MASK(8));
   assert(value.type == AGX_INDEX_REGISTER);
            uint64_t raw =
      0x11 | (I->last ? (1 << 7) : 0) | ((value.value & 0x3F) << 9) |
   (((uint64_t)(index_src.value & 0x3F)) << 16) | (0x80 << 16) | /* XXX */
               unsigned size = 4;
   memcpy(util_dynarray_grow_bytes(emission, 1, size), &raw, size);
               case AGX_OPCODE_DEVICE_LOAD:
   case AGX_OPCODE_DEVICE_STORE:
   case AGX_OPCODE_UNIFORM_STORE: {
      bool is_device_store = I->op == AGX_OPCODE_DEVICE_STORE;
   bool is_uniform_store = I->op == AGX_OPCODE_UNIFORM_STORE;
   bool is_store = is_device_store || is_uniform_store;
            /* Uniform stores internally packed as 16-bit. Fix up the format, mask,
   * and size so we can use scalar 32-bit values in the IR and avoid
   * special casing earlier in the compiler.
   */
   enum agx_format format = is_uniform_store ? AGX_FORMAT_I16 : I->format;
   agx_index reg = is_store ? I->src[0] : I->dest[0];
            if (is_uniform_store) {
      mask = BITFIELD_MASK(agx_size_align_16(reg.size));
                        bool Rt, At = false, Ot;
   unsigned R = agx_pack_memory_reg(reg, &Rt);
   unsigned A =
         unsigned O = agx_pack_memory_index(I->src[offset_src], &Ot);
   unsigned u1 = is_uniform_store ? 0 : 1; // XXX
   unsigned u3 = 0;
   unsigned u4 = is_uniform_store ? 0 : 4; // XXX
   unsigned u5 = 0;
            assert(mask != 0);
            uint64_t raw =
      agx_opcodes_info[I->op].encoding.exact |
   ((format & BITFIELD_MASK(3)) << 7) | ((R & BITFIELD_MASK(6)) << 10) |
   ((A & BITFIELD_MASK(4)) << 16) | ((O & BITFIELD_MASK(4)) << 20) |
   (Ot ? (1 << 24) : 0) | (I->src[offset_src].abs ? (1 << 25) : 0) |
   (is_uniform_store ? (2 << 25) : 0) | (u1 << 26) | (At << 27) |
   (u3 << 28) | (I->scoreboard << 30) |
   (((uint64_t)((O >> 4) & BITFIELD_MASK(4))) << 32) |
   (((uint64_t)((A >> 4) & BITFIELD_MASK(4))) << 36) |
   (((uint64_t)((R >> 6) & BITFIELD_MASK(2))) << 40) |
   (((uint64_t)I->shift) << 42) | (((uint64_t)u4) << 44) |
   (L ? (1ull << 47) : 0) | (((uint64_t)(format >> 3)) << 48) |
               unsigned size = L ? 8 : 6;
   memcpy(util_dynarray_grow_bytes(emission, 1, size), &raw, size);
               case AGX_OPCODE_LOCAL_LOAD:
   case AGX_OPCODE_LOCAL_STORE: {
      bool is_load = I->op == AGX_OPCODE_LOCAL_LOAD;
   bool L = true; /* TODO: when would you want short? */
   unsigned At;
            unsigned R = agx_pack_memory_reg(is_load ? I->dest[0] : I->src[0], &Rt);
   unsigned A = agx_pack_local_base(is_load ? I->src[0] : I->src[1], &At);
            uint64_t raw =
      agx_opcodes_info[I->op].encoding.exact | (Rt ? BITFIELD64_BIT(8) : 0) |
   ((R & BITFIELD_MASK(6)) << 9) | (L ? BITFIELD64_BIT(15) : 0) |
   ((A & BITFIELD_MASK(6)) << 16) | (At << 22) | (I->format << 24) |
   ((O & BITFIELD64_MASK(6)) << 28) | (Ot ? BITFIELD64_BIT(34) : 0) |
               unsigned size = L ? 8 : 6;
   memcpy(util_dynarray_grow_bytes(emission, 1, size), &raw, size);
               case AGX_OPCODE_ATOMIC: {
      bool At, Ot, Rt;
   unsigned A = agx_pack_memory_base(I->src[1], &At);
   unsigned O = agx_pack_memory_index(I->src[2], &Ot);
   unsigned R = agx_pack_atomic_dest(I->dest[0], &Rt);
            uint64_t raw =
      agx_opcodes_info[I->op].encoding.exact |
   (((uint64_t)I->atomic_opc) << 6) | ((R & BITFIELD_MASK(6)) << 10) |
   ((A & BITFIELD_MASK(4)) << 16) | ((O & BITFIELD_MASK(4)) << 20) |
   (Ot ? (1 << 24) : 0) | (I->src[2].abs ? (1 << 25) : 0) | (At << 27) |
   (I->scoreboard << 30) |
   (((uint64_t)((O >> 4) & BITFIELD_MASK(4))) << 32) |
   (((uint64_t)((A >> 4) & BITFIELD_MASK(4))) << 36) |
   (((uint64_t)(R >> 6)) << 40) |
   (needs_g13x_coherency ? BITFIELD64_BIT(45) : 0) |
               memcpy(util_dynarray_grow_bytes(emission, 1, 8), &raw, 8);
               case AGX_OPCODE_LOCAL_ATOMIC: {
               unsigned At;
            bool Ra = I->dest[0].type != AGX_INDEX_NULL;
   unsigned R = Ra ? agx_pack_memory_reg(I->dest[0], &Rt) : 0;
   unsigned S = agx_pack_atomic_source(I->src[0]);
   unsigned A = agx_pack_local_base(I->src[1], &At);
            uint64_t raw =
      agx_opcodes_info[I->op].encoding.exact | (Rt ? BITFIELD64_BIT(8) : 0) |
   ((R & BITFIELD_MASK(6)) << 9) | (L ? BITFIELD64_BIT(15) : 0) |
   ((A & BITFIELD_MASK(6)) << 16) | (At << 22) |
   (((uint64_t)I->atomic_opc) << 24) | ((O & BITFIELD64_MASK(6)) << 28) |
   (Ot ? BITFIELD64_BIT(34) : 0) | (Ra ? BITFIELD64_BIT(38) : 0) |
                        memcpy(util_dynarray_grow_bytes(emission, 1, 8), &raw, 8);
   memcpy(util_dynarray_grow_bytes(emission, 1, 2), &raw2, 2);
               case AGX_OPCODE_TEXTURE_LOAD:
   case AGX_OPCODE_IMAGE_LOAD:
   case AGX_OPCODE_TEXTURE_SAMPLE: {
      assert(I->mask != 0);
            bool Rt, Ct, St, Cs;
   unsigned Tt;
   unsigned U;
            unsigned R = agx_pack_memory_reg(I->dest[0], &Rt);
   unsigned C = agx_pack_sample_coords(I->src[0], &Ct, &Cs);
   unsigned T = agx_pack_texture(I->src[2], I->src[3], &U, &Tt);
   unsigned S = agx_pack_sampler(I->src[4], &St);
   unsigned O = agx_pack_sample_compare_offset(I->src[5]);
            unsigned q1 = I->shadow;
   unsigned q2 = 0;   // XXX
   unsigned q3 = 12;  // XXX
            /* Set bit 43 for image loads. This seems to makes sure that image loads
   * get the value written by the latest image store, not some other image
   * store that was already in flight, fixing
   *
   *    KHR-GLES31.core.shader_image_load_store.basic-glsl-misc-fs
   *
   * Apple seems to set this bit unconditionally for read/write image loads
   * and never for readonly image loads. Some sort of cache control.
   */
   if (I->op == AGX_OPCODE_IMAGE_LOAD)
            uint32_t extend = ((U & BITFIELD_MASK(5)) << 0) | (kill << 5) |
                                       uint64_t raw =
      0x31 | ((I->op != AGX_OPCODE_TEXTURE_SAMPLE) ? (1 << 6) : 0) |
   (Rt ? (1 << 8) : 0) | ((R & BITFIELD_MASK(6)) << 9) |
   (L ? (1 << 15) : 0) | ((C & BITFIELD_MASK(6)) << 16) |
   (Ct ? (1 << 22) : 0) | (q1 << 23) | ((D & BITFIELD_MASK(6)) << 24) |
   (q2 << 30) | (((uint64_t)(T & BITFIELD_MASK(6))) << 32) |
   (((uint64_t)Tt) << 38) |
   (((uint64_t)(I->dim & BITFIELD_MASK(3))) << 40) |
   (((uint64_t)q3) << 43) | (((uint64_t)I->mask) << 48) |
   (((uint64_t)lod_mode) << 52) |
               memcpy(util_dynarray_grow_bytes(emission, 1, 8), &raw, 8);
   if (L)
                        case AGX_OPCODE_IMAGE_WRITE: {
      bool Ct, Dt, Rt, Cs;
   unsigned Tt;
            unsigned R = agx_pack_pbe_source(I->src[0], &Rt);
   unsigned C = agx_pack_sample_coords(I->src[1], &Ct, &Cs);
   unsigned D = agx_pack_pbe_lod(I->src[2], &Dt);
   unsigned T = agx_pack_texture(I->src[3], I->src[4], &U, &Tt);
            assert(U < (1 << 5));
   assert(D < (1 << 8));
   assert(R < (1 << 8));
   assert(C < (1 << 8));
   assert(T < (1 << 8));
            uint64_t raw = agx_opcodes_info[I->op].encoding.exact |
                  (Rt ? (1 << 8) : 0) | ((R & BITFIELD_MASK(6)) << 9) |
   ((C & BITFIELD_MASK(6)) << 16) | (Ct ? (1 << 22) : 0) |
   ((D & BITFIELD_MASK(6)) << 24) | (Dt ? (1u << 31) : 0) |
   (((uint64_t)(T & BITFIELD_MASK(6))) << 32) |
   (((uint64_t)Tt) << 38) |
   (((uint64_t)I->dim & BITFIELD_MASK(3)) << 40) |
   (Cs ? (1ull << 47) : 0) | (((uint64_t)U) << 48) |
               if (raw >> 48) {
      raw |= BITFIELD_BIT(15);
      } else {
                              case AGX_OPCODE_BLOCK_IMAGE_STORE: {
      enum agx_format F = I->format;
            unsigned Tt = 0;
            UNUSED unsigned U;
   unsigned T = agx_pack_texture(agx_zero(), I->src[0], &U, &Tt);
            bool Cs = false;
   bool Ct = I->src[2].discard;
            agx_index offset = I->src[1];
   assert(offset.type == AGX_INDEX_REGISTER);
   assert(offset.size == AGX_SIZE_16);
            bool unk1 = true;
            uint32_t word0 = agx_opcodes_info[I->op].encoding.exact |
                              uint32_t word1 = (T & BITFIELD_MASK(6)) | (Tt << 2) |
                                       memcpy(util_dynarray_grow_bytes(emission, 1, 4), &word0, 4);
   memcpy(util_dynarray_grow_bytes(emission, 1, 4), &word1, 4);
   memcpy(util_dynarray_grow_bytes(emission, 1, 2), &word2, 2);
               case AGX_OPCODE_ZS_EMIT: {
      agx_index S = I->src[0];
   if (S.type == AGX_INDEX_IMMEDIATE)
         else
            agx_index T = I->src[1];
                     uint32_t word0 = agx_opcodes_info[I->op].encoding.exact |
                  ((S.type == AGX_INDEX_IMMEDIATE) ? (1 << 8) : 0) |
               memcpy(util_dynarray_grow_bytes(emission, 1, 4), &word0, 4);
               case AGX_OPCODE_JMP_EXEC_ANY:
   case AGX_OPCODE_JMP_EXEC_NONE:
   case AGX_OPCODE_JMP_EXEC_NONE_AFTER: {
      /* We don't implement indirect branches */
            /* We'll fix the offset later. */
   struct agx_branch_fixup fixup = {
      .block = I->target,
   .offset = emission->size,
                        /* The rest of the instruction is fixed */
   struct agx_opcode_info info = agx_opcodes_info[I->op];
   uint64_t raw = info.encoding.exact;
   memcpy(util_dynarray_grow_bytes(emission, 1, 6), &raw, 6);
               default:
      agx_pack_alu(emission, I);
         }
      /* Relative branches may be emitted before their targets, so we patch the
   * binary to fix up the branch offsets after the main emit */
      static void
   agx_fixup_branch(struct util_dynarray *emission, struct agx_branch_fixup fix)
   {
      /* Branch offset is 2 bytes into the jump instruction */
                     /* Offsets are relative to the jump instruction */
            /* Patch the binary */
      }
      void
   agx_pack_binary(agx_context *ctx, struct util_dynarray *emission)
   {
      struct util_dynarray fixups;
            agx_foreach_block(ctx, block) {
      /* Relative to the start of the binary, the block begins at the current
   * number of bytes emitted */
            agx_foreach_instr_in_block(block, ins) {
      block->last_offset = emission->size;
                  util_dynarray_foreach(&fixups, struct agx_branch_fixup, fixup)
            /* Dougall calls the instruction in this footer "trap". Match the blob. */
   for (unsigned i = 0; i < 8; ++i) {
      uint16_t trap = agx_opcodes_info[AGX_OPCODE_TRAP].encoding.exact;
                  }
