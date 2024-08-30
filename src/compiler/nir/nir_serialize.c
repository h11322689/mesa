   /*
   * Copyright © 2017 Connor Abbott
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "nir_serialize.h"
   #include "util/u_dynarray.h"
   #include "util/u_math.h"
   #include "nir_control_flow.h"
   #include "nir_xfb_info.h"
      #define NIR_SERIALIZE_FUNC_HAS_IMPL ((void *)(intptr_t)1)
   #define MAX_OBJECT_IDS              (1 << 20)
      typedef struct {
      size_t blob_offset;
   nir_def *src;
      } write_phi_fixup;
      typedef struct {
                        /* maps pointer to index */
            /* the next index to assign to a NIR in-memory object */
            /* Array of write_phi_fixup structs representing phi sources that need to
   * be resolved in the second pass.
   */
            /* The last serialized type. */
   const struct glsl_type *last_type;
   const struct glsl_type *last_interface_type;
            /* For skipping equal ALU headers (typical after scalarization). */
   nir_instr_type last_instr_type;
   uintptr_t last_alu_header_offset;
            /* Don't write optional data such as variable names. */
      } write_ctx;
      typedef struct {
                        /* the next index to assign to a NIR in-memory object */
            /* The length of the index -> object table */
            /* map from index to deserialized pointer */
            /* List of phi sources. */
            /* The last deserialized type. */
   const struct glsl_type *last_type;
   const struct glsl_type *last_interface_type;
      } read_ctx;
      static void
   write_add_object(write_ctx *ctx, const void *obj)
   {
      uint32_t index = ctx->next_idx++;
   assert(index != MAX_OBJECT_IDS);
      }
      static uint32_t
   write_lookup_object(write_ctx *ctx, const void *obj)
   {
      struct hash_entry *entry = _mesa_hash_table_search(ctx->remap_table, obj);
   assert(entry);
      }
      static void
   read_add_object(read_ctx *ctx, void *obj)
   {
      assert(ctx->next_idx < ctx->idx_table_len);
      }
      static void *
   read_lookup_object(read_ctx *ctx, uint32_t idx)
   {
      assert(idx < ctx->idx_table_len);
      }
      static void *
   read_object(read_ctx *ctx)
   {
         }
      static uint32_t
   encode_bit_size_3bits(uint8_t bit_size)
   {
      /* Encode values of 0, 1, 2, 4, 8, 16, 32, 64 in 3 bits. */
   assert(bit_size <= 64 && util_is_power_of_two_or_zero(bit_size));
   if (bit_size)
            }
      static uint8_t
   decode_bit_size_3bits(uint8_t bit_size)
   {
      if (bit_size)
            }
      #define NUM_COMPONENTS_IS_SEPARATE_7 7
      static uint8_t
   encode_num_components_in_3bits(uint8_t num_components)
   {
      if (num_components <= 4)
         if (num_components == 8)
         if (num_components == 16)
            /* special value indicating that num_components is in the next uint32 */
      }
      static uint8_t
   decode_num_components_in_3bits(uint8_t value)
   {
      if (value <= 4)
         if (value == 5)
         if (value == 6)
            unreachable("invalid num_components encoding");
      }
      static void
   write_constant(write_ctx *ctx, const nir_constant *c)
   {
      blob_write_bytes(ctx->blob, c->values, sizeof(c->values));
   blob_write_uint32(ctx->blob, c->num_elements);
   for (unsigned i = 0; i < c->num_elements; i++)
      }
      static nir_constant *
   read_constant(read_ctx *ctx, nir_variable *nvar)
   {
               static const nir_const_value zero_vals[ARRAY_SIZE(c->values)] = { 0 };
   blob_copy_bytes(ctx->blob, (uint8_t *)c->values, sizeof(c->values));
   c->is_null_constant = memcmp(c->values, zero_vals, sizeof(c->values)) == 0;
   c->num_elements = blob_read_uint32(ctx->blob);
   c->elements = ralloc_array(nvar, nir_constant *, c->num_elements);
   for (unsigned i = 0; i < c->num_elements; i++) {
      c->elements[i] = read_constant(ctx, nvar);
                  }
      enum var_data_encoding {
      var_encode_full,
   var_encode_shader_temp,
   var_encode_function_temp,
      };
      union packed_var {
      uint32_t u32;
   struct {
      unsigned has_name : 1;
   unsigned has_constant_initializer : 1;
   unsigned has_pointer_initializer : 1;
   unsigned has_interface_type : 1;
   unsigned num_state_slots : 7;
   unsigned data_encoding : 2;
   unsigned type_same_as_last : 1;
   unsigned interface_type_same_as_last : 1;
   unsigned ray_query : 1;
         };
      union packed_var_data_diff {
      uint32_t u32;
   struct {
      int location : 13;
   int location_frac : 3;
         };
      static void
   write_variable(write_ctx *ctx, const nir_variable *var)
   {
                        STATIC_ASSERT(sizeof(union packed_var) == 4);
   union packed_var flags;
            flags.u.has_name = !ctx->strip && var->name;
   flags.u.has_constant_initializer = !!(var->constant_initializer);
   flags.u.has_pointer_initializer = !!(var->pointer_initializer);
   flags.u.has_interface_type = !!(var->interface_type);
   flags.u.type_same_as_last = var->type == ctx->last_type;
   flags.u.interface_type_same_as_last =
         flags.u.num_state_slots = var->num_state_slots;
                     /* When stripping, we expect that the location is no longer needed,
   * which is typically after shaders are linked.
   */
   if (ctx->strip &&
      data.mode != nir_var_system_value &&
   data.mode != nir_var_shader_in &&
   data.mode != nir_var_shader_out)
         /* Temporary variables don't serialize var->data. */
   if (data.mode == nir_var_shader_temp)
         else if (data.mode == nir_var_function_temp)
         else {
               tmp.location = ctx->last_var_data.location;
   tmp.location_frac = ctx->last_var_data.location_frac;
            /* See if we can encode only the difference in locations from the last
   * variable.
   */
   if (memcmp(&ctx->last_var_data, &tmp, sizeof(tmp)) == 0 &&
      abs((int)data.location -
         abs((int)data.driver_location -
            else
                                 if (!flags.u.type_same_as_last) {
      encode_type_to_blob(ctx->blob, var->type);
               if (var->interface_type && !flags.u.interface_type_same_as_last) {
      encode_type_to_blob(ctx->blob, var->interface_type);
               if (flags.u.has_name)
            if (flags.u.data_encoding == var_encode_full ||
      flags.u.data_encoding == var_encode_location_diff) {
   if (flags.u.data_encoding == var_encode_full) {
         } else {
      /* Serialize only the difference in locations from the last variable.
                  diff.u.location = data.location - ctx->last_var_data.location;
   diff.u.location_frac = data.location_frac -
                                                for (unsigned i = 0; i < var->num_state_slots; i++) {
      blob_write_bytes(ctx->blob, &var->state_slots[i],
      }
   if (var->constant_initializer)
         if (var->pointer_initializer)
      blob_write_uint32(ctx->blob,
      if (var->num_members > 0) {
      blob_write_bytes(ctx->blob, (uint8_t *)var->members,
         }
      static nir_variable *
   read_variable(read_ctx *ctx)
   {
      nir_variable *var = rzalloc(ctx->nir, nir_variable);
            union packed_var flags;
            if (flags.u.type_same_as_last) {
         } else {
      var->type = decode_type_from_blob(ctx->blob);
               if (flags.u.has_interface_type) {
      if (flags.u.interface_type_same_as_last) {
         } else {
      var->interface_type = decode_type_from_blob(ctx->blob);
                  if (flags.u.has_name) {
      const char *name = blob_read_string(ctx->blob);
      } else {
                  if (flags.u.data_encoding == var_encode_shader_temp)
         else if (flags.u.data_encoding == var_encode_function_temp)
         else if (flags.u.data_encoding == var_encode_full) {
      blob_copy_bytes(ctx->blob, (uint8_t *)&var->data, sizeof(var->data));
      } else { /* var_encode_location_diff */
      union packed_var_data_diff diff;
            var->data = ctx->last_var_data;
   var->data.location += diff.u.location;
   var->data.location_frac += diff.u.location_frac;
                                 var->num_state_slots = flags.u.num_state_slots;
   if (var->num_state_slots != 0) {
      var->state_slots = ralloc_array(var, nir_state_slot,
         for (unsigned i = 0; i < var->num_state_slots; i++) {
      blob_copy_bytes(ctx->blob, &var->state_slots[i],
         }
   if (flags.u.has_constant_initializer)
         else
            if (flags.u.has_pointer_initializer)
         else
            var->num_members = flags.u.num_members;
   if (var->num_members > 0) {
      var->members = ralloc_array(var, struct nir_variable_data,
         blob_copy_bytes(ctx->blob, (uint8_t *)var->members,
                  }
      static void
   write_var_list(write_ctx *ctx, const struct exec_list *src)
   {
      blob_write_uint32(ctx->blob, exec_list_length(src));
   foreach_list_typed(nir_variable, var, node, src) {
            }
      static void
   read_var_list(read_ctx *ctx, struct exec_list *dst)
   {
      exec_list_make_empty(dst);
   unsigned num_vars = blob_read_uint32(ctx->blob);
   for (unsigned i = 0; i < num_vars; i++) {
      nir_variable *var = read_variable(ctx);
         }
      union packed_src {
      uint32_t u32;
   struct {
      unsigned _pad : 2; /* <-- Header */
   unsigned object_idx : 20;
      } any;
   struct {
      unsigned _header : 22; /* <-- Header */
   unsigned _pad : 2;     /* <-- Footer */
   unsigned swizzle_x : 2;
   unsigned swizzle_y : 2;
   unsigned swizzle_z : 2;
      } alu;
   struct {
      unsigned _header : 22; /* <-- Header */
   unsigned src_type : 5; /* <-- Footer */
         };
      static void
   write_src_full(write_ctx *ctx, const nir_src *src, union packed_src header)
   {
      header.any.object_idx = write_lookup_object(ctx, src->ssa);
      }
      static void
   write_src(write_ctx *ctx, const nir_src *src)
   {
      union packed_src header = { 0 };
      }
      static union packed_src
   read_src(read_ctx *ctx, nir_src *src)
   {
      STATIC_ASSERT(sizeof(union packed_src) == 4);
   union packed_src header;
            src->ssa = read_lookup_object(ctx, header.any.object_idx);
      }
      union packed_def {
      uint8_t u8;
   struct {
      uint8_t _pad : 1;
   uint8_t num_components : 3;
   uint8_t bit_size : 3;
         };
      enum intrinsic_const_indices_encoding {
      /* Use packed_const_indices to store tightly packed indices.
   *
   * The common case for load_ubo is 0, 0, 0, which is trivially represented.
   * The common cases for load_interpolated_input also fit here, e.g.: 7, 3
   */
            const_indices_8bit,  /* 8 bits per element */
   const_indices_16bit, /* 16 bits per element */
      };
      enum load_const_packing {
      /* Constants are not packed and are stored in following dwords. */
            /* packed_value contains high 19 bits, low bits are 0,
   * good for floating-point decimals
   */
            /* packed_value contains low 19 bits, high bits are sign-extended */
      };
      union packed_instr {
      uint32_t u32;
   struct {
      unsigned instr_type : 4; /* always present */
   unsigned _pad : 20;
      } any;
   struct {
      unsigned instr_type : 4;
   unsigned exact : 1;
   unsigned no_signed_wrap : 1;
   unsigned no_unsigned_wrap : 1;
   unsigned padding : 1;
   /* Reg: writemask; SSA: swizzles for 2 srcs */
   unsigned writemask_or_two_swizzles : 4;
   unsigned op : 9;
   unsigned packed_src_ssa_16bit : 1;
   /* Scalarized ALUs always have the same header. */
   unsigned num_followup_alu_sharing_header : 2;
      } alu;
   struct {
      unsigned instr_type : 4;
   unsigned deref_type : 3;
   unsigned cast_type_same_as_last : 1;
   unsigned modes : 5; /* See (de|en)code_deref_modes() */
   unsigned _pad : 9;
   unsigned in_bounds : 1;
   unsigned packed_src_ssa_16bit : 1; /* deref_var redefines this */
      } deref;
   struct {
      unsigned instr_type : 4;
   unsigned deref_type : 3;
   unsigned _pad : 1;
   unsigned object_idx : 16; /* if 0, the object ID is a separate uint32 */
      } deref_var;
   struct {
      unsigned instr_type : 4;
   unsigned intrinsic : 10;
   unsigned const_indices_encoding : 2;
   unsigned packed_const_indices : 8;
      } intrinsic;
   struct {
      unsigned instr_type : 4;
   unsigned last_component : 4;
   unsigned bit_size : 3;
   unsigned packing : 2;       /* enum load_const_packing */
      } load_const;
   struct {
      unsigned instr_type : 4;
   unsigned last_component : 4;
   unsigned bit_size : 3;
      } undef;
   struct {
      unsigned instr_type : 4;
   unsigned num_srcs : 4;
   unsigned op : 5;
   unsigned _pad : 11;
      } tex;
   struct {
      unsigned instr_type : 4;
   unsigned num_srcs : 20;
      } phi;
   struct {
      unsigned instr_type : 4;
   unsigned type : 2;
         };
      /* Write "lo24" as low 24 bits in the first uint32. */
   static void
   write_def(write_ctx *ctx, const nir_def *def, union packed_instr header,
         {
      STATIC_ASSERT(sizeof(union packed_def) == 1);
   union packed_def pdef;
            pdef.num_components =
         pdef.bit_size = encode_bit_size_3bits(def->bit_size);
   pdef.divergent = def->divergent;
            /* Check if the current ALU instruction has the same header as the previous
   * instruction that is also ALU. If it is, we don't have to write
   * the current header. This is a typical occurence after scalarization.
   */
   if (instr_type == nir_instr_type_alu) {
               if (ctx->last_instr_type == nir_instr_type_alu) {
      assert(ctx->last_alu_header_offset);
                  /* Clear the field that counts ALUs with equal headers. */
   union packed_instr clean_header;
                  /* There can be at most 4 consecutive ALU instructions
   * sharing the same header.
   */
   if (last_header.alu.num_followup_alu_sharing_header < 3 &&
      header.u32 == clean_header.u32) {
   last_header.alu.num_followup_alu_sharing_header++;
   blob_overwrite_uint32(ctx->blob, ctx->last_alu_header_offset,
         ctx->last_alu_header = last_header.u32;
                  if (!equal_header) {
      ctx->last_alu_header_offset = blob_reserve_uint32(ctx->blob);
   blob_overwrite_uint32(ctx->blob, ctx->last_alu_header_offset, header.u32);
         } else {
                  if (pdef.num_components == NUM_COMPONENTS_IS_SEPARATE_7)
               }
      static void
   read_def(read_ctx *ctx, nir_def *def, nir_instr *instr,
         {
      union packed_def pdef;
            unsigned bit_size = decode_bit_size_3bits(pdef.bit_size);
   unsigned num_components;
   if (pdef.num_components == NUM_COMPONENTS_IS_SEPARATE_7)
         else
         nir_def_init(instr, def, num_components, bit_size);
   def->divergent = pdef.divergent;
      }
      static bool
   are_object_ids_16bit(write_ctx *ctx)
   {
      /* Check the highest object ID, because they are monotonic. */
      }
      static bool
   is_alu_src_ssa_16bit(write_ctx *ctx, const nir_alu_instr *alu)
   {
               for (unsigned i = 0; i < num_srcs; i++) {
               for (unsigned chan = 0; chan < src_components; chan++) {
      /* The swizzles for src0.x and src1.x are stored
   * in writemask_or_two_swizzles for SSA ALUs.
   */
                  if (alu->src[i].swizzle[chan] != chan)
                     }
      static void
   write_alu(write_ctx *ctx, const nir_alu_instr *alu)
   {
               /* 9 bits for nir_op */
   STATIC_ASSERT(nir_num_opcodes <= 512);
   union packed_instr header;
            header.alu.instr_type = alu->instr.type;
   header.alu.exact = alu->exact;
   header.alu.no_signed_wrap = alu->no_signed_wrap;
   header.alu.no_unsigned_wrap = alu->no_unsigned_wrap;
   header.alu.op = alu->op;
            if (header.alu.packed_src_ssa_16bit) {
      /* For packed srcs of SSA ALUs, this field stores the swizzles. */
   header.alu.writemask_or_two_swizzles = alu->src[0].swizzle[0];
   if (num_srcs > 1)
                        if (header.alu.packed_src_ssa_16bit) {
      for (unsigned i = 0; i < num_srcs; i++) {
      unsigned idx = write_lookup_object(ctx, alu->src[i].src.ssa);
   assert(idx < (1 << 16));
         } else {
      for (unsigned i = 0; i < num_srcs; i++) {
      unsigned src_channels = nir_ssa_alu_instr_src_components(alu, i);
   unsigned src_components = nir_src_num_components(alu->src[i].src);
   union packed_src src;
                  if (packed) {
      src.alu.swizzle_x = alu->src[i].swizzle[0];
   src.alu.swizzle_y = alu->src[i].swizzle[1];
   src.alu.swizzle_z = alu->src[i].swizzle[2];
                        /* Store swizzles for vec8 and vec16. */
   if (!packed) {
                                                            }
      static nir_alu_instr *
   read_alu(read_ctx *ctx, union packed_instr header)
   {
      unsigned num_srcs = nir_op_infos[header.alu.op].num_inputs;
            alu->exact = header.alu.exact;
   alu->no_signed_wrap = header.alu.no_signed_wrap;
                     if (header.alu.packed_src_ssa_16bit) {
      for (unsigned i = 0; i < num_srcs; i++) {
                                       for (unsigned chan = 0; chan < src_components; chan++)
         } else {
      for (unsigned i = 0; i < num_srcs; i++) {
      union packed_src src = read_src(ctx, &alu->src[i].src);
   unsigned src_channels = nir_ssa_alu_instr_src_components(alu, i);
                           if (packed) {
      alu->src[i].swizzle[0] = src.alu.swizzle_x;
   alu->src[i].swizzle[1] = src.alu.swizzle_y;
   alu->src[i].swizzle[2] = src.alu.swizzle_z;
      } else {
      /* Load swizzles for vec8 and vec16. */
                     for (unsigned j = 0; j < 8 && o + j < src_channels; j++) {
      alu->src[i].swizzle[o + j] =
                           if (header.alu.packed_src_ssa_16bit) {
      alu->src[0].swizzle[0] = header.alu.writemask_or_two_swizzles & 0x3;
   if (num_srcs > 1)
                  }
      #define MODE_ENC_GENERIC_BIT (1 << 4)
      static nir_variable_mode
   decode_deref_modes(unsigned modes)
   {
      if (modes & MODE_ENC_GENERIC_BIT) {
      modes &= ~MODE_ENC_GENERIC_BIT;
      } else {
            }
      static unsigned
   encode_deref_modes(nir_variable_mode modes)
   {
      /* Mode sets on derefs generally come in two forms.  For certain OpenCL
   * cases, we can have more than one of the generic modes set.  In this
   * case, we need the full bitfield.  Fortunately, there are only 4 of
   * these.  For all other modes, we can only have one mode at a time so we
   * can compress them by only storing the bit position.  This, plus one bit
   * to select encoding, lets us pack the entire bitfield in 5 bits.
   */
   STATIC_ASSERT((nir_var_all & ~nir_var_mem_generic) <
            unsigned enc;
   if (modes == 0 || (modes & nir_var_mem_generic)) {
      assert(!(modes & ~nir_var_mem_generic));
   enc = modes >> (ffs(nir_var_mem_generic) - 1);
   assert(enc < MODE_ENC_GENERIC_BIT);
      } else {
      assert(util_is_power_of_two_nonzero(modes));
   enc = ffs(modes) - 1;
      }
   assert(modes == decode_deref_modes(enc));
      }
      static void
   write_deref(write_ctx *ctx, const nir_deref_instr *deref)
   {
               union packed_instr header;
            header.deref.instr_type = deref->instr.type;
            if (deref->deref_type == nir_deref_type_cast) {
      header.deref.modes = encode_deref_modes(deref->modes);
               unsigned var_idx = 0;
   if (deref->deref_type == nir_deref_type_var) {
      var_idx = write_lookup_object(ctx, deref->var);
   if (var_idx && var_idx < (1 << 16))
               if (deref->deref_type == nir_deref_type_array ||
      deref->deref_type == nir_deref_type_ptr_as_array) {
                                 switch (deref->deref_type) {
   case nir_deref_type_var:
      if (!header.deref_var.object_idx)
               case nir_deref_type_struct:
      write_src(ctx, &deref->parent);
   blob_write_uint32(ctx->blob, deref->strct.index);
         case nir_deref_type_array:
   case nir_deref_type_ptr_as_array:
      if (header.deref.packed_src_ssa_16bit) {
      blob_write_uint16(ctx->blob,
         blob_write_uint16(ctx->blob,
      } else {
      write_src(ctx, &deref->parent);
      }
         case nir_deref_type_cast:
      write_src(ctx, &deref->parent);
   blob_write_uint32(ctx->blob, deref->cast.ptr_stride);
   blob_write_uint32(ctx->blob, deref->cast.align_mul);
   blob_write_uint32(ctx->blob, deref->cast.align_offset);
   if (!header.deref.cast_type_same_as_last) {
      encode_type_to_blob(ctx->blob, deref->type);
      }
         case nir_deref_type_array_wildcard:
      write_src(ctx, &deref->parent);
         default:
            }
      static nir_deref_instr *
   read_deref(read_ctx *ctx, union packed_instr header)
   {
      nir_deref_type deref_type = header.deref.deref_type;
                              switch (deref->deref_type) {
   case nir_deref_type_var:
      if (header.deref_var.object_idx)
         else
            deref->type = deref->var->type;
         case nir_deref_type_struct:
      read_src(ctx, &deref->parent);
   parent = nir_src_as_deref(deref->parent);
   deref->strct.index = blob_read_uint32(ctx->blob);
   deref->type = glsl_get_struct_field(parent->type, deref->strct.index);
         case nir_deref_type_array:
   case nir_deref_type_ptr_as_array:
      if (header.deref.packed_src_ssa_16bit) {
      deref->parent.ssa = read_lookup_object(ctx, blob_read_uint16(ctx->blob));
      } else {
      read_src(ctx, &deref->parent);
                        parent = nir_src_as_deref(deref->parent);
   if (deref->deref_type == nir_deref_type_array)
         else
               case nir_deref_type_cast:
      read_src(ctx, &deref->parent);
   deref->cast.ptr_stride = blob_read_uint32(ctx->blob);
   deref->cast.align_mul = blob_read_uint32(ctx->blob);
   deref->cast.align_offset = blob_read_uint32(ctx->blob);
   if (header.deref.cast_type_same_as_last) {
         } else {
      deref->type = decode_type_from_blob(ctx->blob);
      }
         case nir_deref_type_array_wildcard:
      read_src(ctx, &deref->parent);
   parent = nir_src_as_deref(deref->parent);
   deref->type = glsl_get_array_element(parent->type);
         default:
                  if (deref_type == nir_deref_type_var) {
         } else if (deref->deref_type == nir_deref_type_cast) {
         } else {
                     }
      static void
   write_intrinsic(write_ctx *ctx, const nir_intrinsic_instr *intrin)
   {
      /* 10 bits for nir_intrinsic_op */
   STATIC_ASSERT(nir_num_intrinsics <= 1024);
   unsigned num_srcs = nir_intrinsic_infos[intrin->intrinsic].num_srcs;
   unsigned num_indices = nir_intrinsic_infos[intrin->intrinsic].num_indices;
            union packed_instr header;
            header.intrinsic.instr_type = intrin->instr.type;
            /* Analyze constant indices to decide how to encode them. */
   if (num_indices) {
      unsigned max_bits = 0;
   for (unsigned i = 0; i < num_indices; i++) {
      unsigned max = util_last_bit(intrin->const_index[i]);
               if (max_bits * num_indices <= 8) {
               /* Pack all const indices into 8 bits. */
   unsigned bit_size = 8 / num_indices;
   for (unsigned i = 0; i < num_indices; i++) {
      header.intrinsic.packed_const_indices |=
         } else if (max_bits <= 8)
         else if (max_bits <= 16)
         else
               if (nir_intrinsic_infos[intrin->intrinsic].has_dest)
         else
            for (unsigned i = 0; i < num_srcs; i++)
            if (num_indices) {
      switch (header.intrinsic.const_indices_encoding) {
   case const_indices_8bit:
      for (unsigned i = 0; i < num_indices; i++)
            case const_indices_16bit:
      for (unsigned i = 0; i < num_indices; i++)
            case const_indices_32bit:
      for (unsigned i = 0; i < num_indices; i++)
                  }
      static nir_intrinsic_instr *
   read_intrinsic(read_ctx *ctx, union packed_instr header)
   {
      nir_intrinsic_op op = header.intrinsic.intrinsic;
            unsigned num_srcs = nir_intrinsic_infos[op].num_srcs;
            if (nir_intrinsic_infos[op].has_dest)
            for (unsigned i = 0; i < num_srcs; i++)
            /* Vectorized instrinsics have num_components same as dst or src that has
   * 0 components in the info. Find it.
   */
   if (nir_intrinsic_infos[op].has_dest &&
      nir_intrinsic_infos[op].dest_components == 0) {
      } else {
      for (unsigned i = 0; i < num_srcs; i++) {
      if (nir_intrinsic_infos[op].src_components[i] == 0) {
      intrin->num_components = nir_src_num_components(intrin->src[i]);
                     if (num_indices) {
      switch (header.intrinsic.const_indices_encoding) {
   case const_indices_all_combined: {
      unsigned bit_size = 8 / num_indices;
   unsigned bit_mask = u_bit_consecutive(0, bit_size);
   for (unsigned i = 0; i < num_indices; i++) {
      intrin->const_index[i] =
      (header.intrinsic.packed_const_indices >> (i * bit_size)) &
   }
      }
   case const_indices_8bit:
      for (unsigned i = 0; i < num_indices; i++)
            case const_indices_16bit:
      for (unsigned i = 0; i < num_indices; i++)
            case const_indices_32bit:
      for (unsigned i = 0; i < num_indices; i++)
                           }
      static void
   write_load_const(write_ctx *ctx, const nir_load_const_instr *lc)
   {
      assert(lc->def.num_components >= 1 && lc->def.num_components <= 16);
   union packed_instr header;
            header.load_const.instr_type = lc->instr.type;
   header.load_const.last_component = lc->def.num_components - 1;
   header.load_const.bit_size = encode_bit_size_3bits(lc->def.bit_size);
            /* Try to pack 1-component constants into the 19 free bits in the header. */
   if (lc->def.num_components == 1) {
      switch (lc->def.bit_size) {
   case 64:
      if ((lc->value[0].u64 & 0x1fffffffffffull) == 0) {
      /* packed_value contains high 19 bits, low bits are 0 */
   header.load_const.packing = load_const_scalar_hi_19bits;
      } else if (util_mask_sign_extend(lc->value[0].i64, 19) == lc->value[0].i64) {
      /* packed_value contains low 19 bits, high bits are sign-extended */
   header.load_const.packing = load_const_scalar_lo_19bits_sext;
                  case 32:
      if ((lc->value[0].u32 & 0x1fff) == 0) {
      header.load_const.packing = load_const_scalar_hi_19bits;
      } else if (util_mask_sign_extend(lc->value[0].i32, 19) == lc->value[0].i32) {
      header.load_const.packing = load_const_scalar_lo_19bits_sext;
                  case 16:
      header.load_const.packing = load_const_scalar_lo_19bits_sext;
   header.load_const.packed_value = lc->value[0].u16;
      case 8:
      header.load_const.packing = load_const_scalar_lo_19bits_sext;
   header.load_const.packed_value = lc->value[0].u8;
      case 1:
      header.load_const.packing = load_const_scalar_lo_19bits_sext;
   header.load_const.packed_value = lc->value[0].b;
      default:
                              if (header.load_const.packing == load_const_full) {
      switch (lc->def.bit_size) {
   case 64:
      blob_write_bytes(ctx->blob, lc->value,
               case 32:
      for (unsigned i = 0; i < lc->def.num_components; i++)
               case 16:
      for (unsigned i = 0; i < lc->def.num_components; i++)
               default:
      assert(lc->def.bit_size <= 8);
   for (unsigned i = 0; i < lc->def.num_components; i++)
                           }
      static nir_load_const_instr *
   read_load_const(read_ctx *ctx, union packed_instr header)
   {
      nir_load_const_instr *lc =
      nir_load_const_instr_create(ctx->nir, header.load_const.last_component + 1,
               switch (header.load_const.packing) {
   case load_const_scalar_hi_19bits:
      switch (lc->def.bit_size) {
   case 64:
      lc->value[0].u64 = (uint64_t)header.load_const.packed_value << 45;
      case 32:
      lc->value[0].u32 = (uint64_t)header.load_const.packed_value << 13;
      default:
         }
         case load_const_scalar_lo_19bits_sext:
      switch (lc->def.bit_size) {
   case 64:
      lc->value[0].u64 = header.load_const.packed_value;
   if (lc->value[0].u64 >> 18)
            case 32:
      lc->value[0].u32 = header.load_const.packed_value;
   if (lc->value[0].u32 >> 18)
            case 16:
      lc->value[0].u16 = header.load_const.packed_value;
      case 8:
      lc->value[0].u8 = header.load_const.packed_value;
      case 1:
      lc->value[0].b = header.load_const.packed_value;
      default:
         }
         case load_const_full:
      switch (lc->def.bit_size) {
   case 64:
                  case 32:
      for (unsigned i = 0; i < lc->def.num_components; i++)
               case 16:
      for (unsigned i = 0; i < lc->def.num_components; i++)
               default:
      assert(lc->def.bit_size <= 8);
   for (unsigned i = 0; i < lc->def.num_components; i++)
            }
               read_add_object(ctx, &lc->def);
      }
      static void
   write_ssa_undef(write_ctx *ctx, const nir_undef_instr *undef)
   {
               union packed_instr header;
            header.undef.instr_type = undef->instr.type;
   header.undef.last_component = undef->def.num_components - 1;
            blob_write_uint32(ctx->blob, header.u32);
      }
      static nir_undef_instr *
   read_ssa_undef(read_ctx *ctx, union packed_instr header)
   {
      nir_undef_instr *undef =
      nir_undef_instr_create(ctx->nir, header.undef.last_component + 1,
                  read_add_object(ctx, &undef->def);
      }
      union packed_tex_data {
      uint32_t u32;
   struct {
      unsigned sampler_dim : 4;
   unsigned dest_type : 8;
   unsigned coord_components : 3;
   unsigned is_array : 1;
   unsigned is_shadow : 1;
   unsigned is_new_style_shadow : 1;
   unsigned is_sparse : 1;
   unsigned component : 2;
   unsigned texture_non_uniform : 1;
   unsigned sampler_non_uniform : 1;
   unsigned array_is_lowered_cube : 1;
   unsigned is_gather_implicit_lod : 1;
         };
      static void
   write_tex(write_ctx *ctx, const nir_tex_instr *tex)
   {
      assert(tex->num_srcs < 16);
            union packed_instr header;
            header.tex.instr_type = tex->instr.type;
   header.tex.num_srcs = tex->num_srcs;
                     blob_write_uint32(ctx->blob, tex->texture_index);
   blob_write_uint32(ctx->blob, tex->sampler_index);
   blob_write_uint32(ctx->blob, tex->backend_flags);
   if (tex->op == nir_texop_tg4)
            STATIC_ASSERT(sizeof(union packed_tex_data) == sizeof(uint32_t));
   union packed_tex_data packed = {
      .u.sampler_dim = tex->sampler_dim,
   .u.dest_type = tex->dest_type,
   .u.coord_components = tex->coord_components,
   .u.is_array = tex->is_array,
   .u.is_shadow = tex->is_shadow,
   .u.is_new_style_shadow = tex->is_new_style_shadow,
   .u.is_sparse = tex->is_sparse,
   .u.component = tex->component,
   .u.texture_non_uniform = tex->texture_non_uniform,
   .u.sampler_non_uniform = tex->sampler_non_uniform,
   .u.array_is_lowered_cube = tex->array_is_lowered_cube,
      };
            for (unsigned i = 0; i < tex->num_srcs; i++) {
      union packed_src src;
   src.u32 = 0;
   src.tex.src_type = tex->src[i].src_type;
         }
      static nir_tex_instr *
   read_tex(read_ctx *ctx, union packed_instr header)
   {
                        tex->op = header.tex.op;
   tex->texture_index = blob_read_uint32(ctx->blob);
   tex->sampler_index = blob_read_uint32(ctx->blob);
   tex->backend_flags = blob_read_uint32(ctx->blob);
   if (tex->op == nir_texop_tg4)
            union packed_tex_data packed;
   packed.u32 = blob_read_uint32(ctx->blob);
   tex->sampler_dim = packed.u.sampler_dim;
   tex->dest_type = packed.u.dest_type;
   tex->coord_components = packed.u.coord_components;
   tex->is_array = packed.u.is_array;
   tex->is_shadow = packed.u.is_shadow;
   tex->is_new_style_shadow = packed.u.is_new_style_shadow;
   tex->is_sparse = packed.u.is_sparse;
   tex->component = packed.u.component;
   tex->texture_non_uniform = packed.u.texture_non_uniform;
   tex->sampler_non_uniform = packed.u.sampler_non_uniform;
   tex->array_is_lowered_cube = packed.u.array_is_lowered_cube;
            for (unsigned i = 0; i < tex->num_srcs; i++) {
      union packed_src src = read_src(ctx, &tex->src[i].src);
                  }
      static void
   write_phi(write_ctx *ctx, const nir_phi_instr *phi)
   {
      union packed_instr header;
            header.phi.instr_type = phi->instr.type;
            /* Phi nodes are special, since they may reference SSA definitions and
   * basic blocks that don't exist yet. We leave two empty uint32_t's here,
   * and then store enough information so that a later fixup pass can fill
   * them in correctly.
   */
            nir_foreach_phi_src(src, phi) {
      size_t blob_offset = blob_reserve_uint32(ctx->blob);
   ASSERTED size_t blob_offset2 = blob_reserve_uint32(ctx->blob);
   assert(blob_offset + sizeof(uint32_t) == blob_offset2);
   write_phi_fixup fixup = {
      .blob_offset = blob_offset,
   .src = src->src.ssa,
      };
         }
      static void
   write_fixup_phis(write_ctx *ctx)
   {
      util_dynarray_foreach(&ctx->phi_fixups, write_phi_fixup, fixup) {
      blob_overwrite_uint32(ctx->blob, fixup->blob_offset,
         blob_overwrite_uint32(ctx->blob, fixup->blob_offset + sizeof(uint32_t),
                  }
      static nir_phi_instr *
   read_phi(read_ctx *ctx, nir_block *blk, union packed_instr header)
   {
                        /* For similar reasons as before, we just store the index directly into the
   * pointer, and let a later pass resolve the phi sources.
   *
   * In order to ensure that the copied sources (which are just the indices
   * from the blob for now) don't get inserted into the old shader's use-def
   * lists, we have to add the phi instruction *before* we set up its
   * sources.
   */
            for (unsigned i = 0; i < header.phi.num_srcs; i++) {
      nir_def *def = (nir_def *)(uintptr_t)blob_read_uint32(ctx->blob);
   nir_block *pred = (nir_block *)(uintptr_t)blob_read_uint32(ctx->blob);
            /* Since we're not letting nir_insert_instr handle use/def stuff for us,
   * we have to set the parent_instr manually.  It doesn't really matter
   * when we do it, so we might as well do it here.
   */
            /* Stash it in the list of phi sources.  We'll walk this list and fix up
   * sources at the very end of read_function_impl.
   */
                  }
      static void
   read_fixup_phis(read_ctx *ctx)
   {
      list_for_each_entry_safe(nir_phi_src, src, &ctx->phi_srcs, src.use_link) {
      src->pred = read_lookup_object(ctx, (uintptr_t)src->pred);
            /* Remove from this list */
               }
      }
      static void
   write_jump(write_ctx *ctx, const nir_jump_instr *jmp)
   {
      /* These aren't handled because they require special block linking */
                     union packed_instr header;
            header.jump.instr_type = jmp->instr.type;
               }
      static nir_jump_instr *
   read_jump(read_ctx *ctx, union packed_instr header)
   {
      /* These aren't handled because they require special block linking */
   assert(header.jump.type != nir_jump_goto &&
            nir_jump_instr *jmp = nir_jump_instr_create(ctx->nir, header.jump.type);
      }
      static void
   write_call(write_ctx *ctx, const nir_call_instr *call)
   {
               for (unsigned i = 0; i < call->num_params; i++)
      }
      static nir_call_instr *
   read_call(read_ctx *ctx)
   {
      nir_function *callee = read_object(ctx);
            for (unsigned i = 0; i < call->num_params; i++)
               }
      static void
   write_instr(write_ctx *ctx, const nir_instr *instr)
   {
      /* We have only 4 bits for the instruction type. */
            switch (instr->type) {
   case nir_instr_type_alu:
      write_alu(ctx, nir_instr_as_alu(instr));
      case nir_instr_type_deref:
      write_deref(ctx, nir_instr_as_deref(instr));
      case nir_instr_type_intrinsic:
      write_intrinsic(ctx, nir_instr_as_intrinsic(instr));
      case nir_instr_type_load_const:
      write_load_const(ctx, nir_instr_as_load_const(instr));
      case nir_instr_type_undef:
      write_ssa_undef(ctx, nir_instr_as_undef(instr));
      case nir_instr_type_tex:
      write_tex(ctx, nir_instr_as_tex(instr));
      case nir_instr_type_phi:
      write_phi(ctx, nir_instr_as_phi(instr));
      case nir_instr_type_jump:
      write_jump(ctx, nir_instr_as_jump(instr));
      case nir_instr_type_call:
      blob_write_uint32(ctx->blob, instr->type);
   write_call(ctx, nir_instr_as_call(instr));
      case nir_instr_type_parallel_copy:
         default:
            }
      /* Return the number of instructions read. */
   static unsigned
   read_instr(read_ctx *ctx, nir_block *block)
   {
      STATIC_ASSERT(sizeof(union packed_instr) == 4);
   union packed_instr header;
   header.u32 = blob_read_uint32(ctx->blob);
            switch (header.any.instr_type) {
   case nir_instr_type_alu:
      for (unsigned i = 0; i <= header.alu.num_followup_alu_sharing_header; i++)
            case nir_instr_type_deref:
      instr = &read_deref(ctx, header)->instr;
      case nir_instr_type_intrinsic:
      instr = &read_intrinsic(ctx, header)->instr;
      case nir_instr_type_load_const:
      instr = &read_load_const(ctx, header)->instr;
      case nir_instr_type_undef:
      instr = &read_ssa_undef(ctx, header)->instr;
      case nir_instr_type_tex:
      instr = &read_tex(ctx, header)->instr;
      case nir_instr_type_phi:
      /* Phi instructions are a bit of a special case when reading because we
   * don't want inserting the instruction to automatically handle use/defs
   * for us.  Instead, we need to wait until all the blocks/instructions
   * are read so that we can set their sources up.
   */
   read_phi(ctx, block, header);
      case nir_instr_type_jump:
      instr = &read_jump(ctx, header)->instr;
      case nir_instr_type_call:
      instr = &read_call(ctx)->instr;
      case nir_instr_type_parallel_copy:
         default:
                  nir_instr_insert_after_block(block, instr);
      }
      static void
   write_block(write_ctx *ctx, const nir_block *block)
   {
      write_add_object(ctx, block);
            ctx->last_instr_type = ~0;
            nir_foreach_instr(instr, block) {
      write_instr(ctx, instr);
         }
      static void
   read_block(read_ctx *ctx, struct exec_list *cf_list)
   {
      /* Don't actually create a new block.  Just use the one from the tail of
   * the list.  NIR guarantees that the tail of the list is a block and that
   * no two blocks are side-by-side in the IR;  It should be empty.
   */
   nir_block *block =
            read_add_object(ctx, block);
   unsigned num_instrs = blob_read_uint32(ctx->blob);
   for (unsigned i = 0; i < num_instrs;) {
            }
      static void
   write_cf_list(write_ctx *ctx, const struct exec_list *cf_list);
      static void
   read_cf_list(read_ctx *ctx, struct exec_list *cf_list);
      static void
   write_if(write_ctx *ctx, nir_if *nif)
   {
      write_src(ctx, &nif->condition);
            write_cf_list(ctx, &nif->then_list);
      }
      static void
   read_if(read_ctx *ctx, struct exec_list *cf_list)
   {
               read_src(ctx, &nif->condition);
                     read_cf_list(ctx, &nif->then_list);
      }
      static void
   write_loop(write_ctx *ctx, nir_loop *loop)
   {
      blob_write_uint8(ctx->blob, loop->control);
   blob_write_uint8(ctx->blob, loop->divergent);
   bool has_continue_construct = nir_loop_has_continue_construct(loop);
            write_cf_list(ctx, &loop->body);
   if (has_continue_construct) {
            }
      static void
   read_loop(read_ctx *ctx, struct exec_list *cf_list)
   {
                        loop->control = blob_read_uint8(ctx->blob);
   loop->divergent = blob_read_uint8(ctx->blob);
            read_cf_list(ctx, &loop->body);
   if (has_continue_construct) {
      nir_loop_add_continue_construct(loop);
         }
      static void
   write_cf_node(write_ctx *ctx, nir_cf_node *cf)
   {
               switch (cf->type) {
   case nir_cf_node_block:
      write_block(ctx, nir_cf_node_as_block(cf));
      case nir_cf_node_if:
      write_if(ctx, nir_cf_node_as_if(cf));
      case nir_cf_node_loop:
      write_loop(ctx, nir_cf_node_as_loop(cf));
      default:
            }
      static void
   read_cf_node(read_ctx *ctx, struct exec_list *list)
   {
               switch (type) {
   case nir_cf_node_block:
      read_block(ctx, list);
      case nir_cf_node_if:
      read_if(ctx, list);
      case nir_cf_node_loop:
      read_loop(ctx, list);
      default:
            }
      static void
   write_cf_list(write_ctx *ctx, const struct exec_list *cf_list)
   {
      blob_write_uint32(ctx->blob, exec_list_length(cf_list));
   foreach_list_typed(nir_cf_node, cf, node, cf_list) {
            }
      static void
   read_cf_list(read_ctx *ctx, struct exec_list *cf_list)
   {
      uint32_t num_cf_nodes = blob_read_uint32(ctx->blob);
   for (unsigned i = 0; i < num_cf_nodes; i++)
      }
      static void
   write_function_impl(write_ctx *ctx, const nir_function_impl *fi)
   {
      blob_write_uint8(ctx->blob, fi->structured);
            if (fi->preamble)
                     write_cf_list(ctx, &fi->body);
      }
      static nir_function_impl *
   read_function_impl(read_ctx *ctx)
   {
               fi->structured = blob_read_uint8(ctx->blob);
            if (preamble)
                     read_cf_list(ctx, &fi->body);
                        }
      static void
   write_function(write_ctx *ctx, const nir_function *fxn)
   {
      uint32_t flags = 0;
   if (fxn->is_entrypoint)
         if (fxn->is_preamble)
         if (fxn->name)
         if (fxn->impl)
         if (fxn->should_inline)
         if (fxn->dont_inline)
         blob_write_uint32(ctx->blob, flags);
   if (fxn->name)
                     blob_write_uint32(ctx->blob, fxn->num_params);
   for (unsigned i = 0; i < fxn->num_params; i++) {
      uint32_t val =
      ((uint32_t)fxn->params[i].num_components) |
                  /* At first glance, it looks like we should write the function_impl here.
   * However, call instructions need to be able to reference at least the
   * function and those will get processed as we write the function_impls.
   * We stop here and write function_impls as a second pass.
      }
      static void
   read_function(read_ctx *ctx)
   {
      uint32_t flags = blob_read_uint32(ctx->blob);
   bool has_name = flags & 0x4;
                              fxn->num_params = blob_read_uint32(ctx->blob);
   fxn->params = ralloc_array(fxn, nir_parameter, fxn->num_params);
   for (unsigned i = 0; i < fxn->num_params; i++) {
      uint32_t val = blob_read_uint32(ctx->blob);
   fxn->params[i].num_components = val & 0xff;
               fxn->is_entrypoint = flags & 0x1;
   fxn->is_preamble = flags & 0x2;
   if (flags & 0x8)
         fxn->should_inline = flags & 0x10;
      }
      static void
   write_xfb_info(write_ctx *ctx, const nir_xfb_info *xfb)
   {
      if (xfb == NULL) {
         } else {
      size_t size = nir_xfb_info_size(xfb->output_count);
   assert(size <= UINT32_MAX);
   blob_write_uint32(ctx->blob, size);
         }
      static nir_xfb_info *
   read_xfb_info(read_ctx *ctx)
   {
      uint32_t size = blob_read_uint32(ctx->blob);
   if (size == 0)
            struct nir_xfb_info *xfb = ralloc_size(ctx->nir, size);
               }
      /**
   * Serialize NIR into a binary blob.
   *
   * \param strip  Don't serialize information only useful for debugging,
   *               such as variable names, making cache hits from similar
   *               shaders more likely.
   */
   void
   nir_serialize(struct blob *blob, const nir_shader *nir, bool strip)
   {
      write_ctx ctx = { 0 };
   ctx.remap_table = _mesa_pointer_hash_table_create(NULL);
   ctx.blob = blob;
   ctx.nir = nir;
   ctx.strip = strip;
                     struct shader_info info = nir->info;
   uint32_t strings = 0;
   if (!strip && info.name)
         if (!strip && info.label)
         blob_write_uint32(blob, strings);
   if (!strip && info.name)
         if (!strip && info.label)
         info.name = info.label = NULL;
                     blob_write_uint32(blob, nir->num_inputs);
   blob_write_uint32(blob, nir->num_uniforms);
   blob_write_uint32(blob, nir->num_outputs);
            blob_write_uint32(blob, exec_list_length(&nir->functions));
   nir_foreach_function(fxn, nir) {
                  nir_foreach_function_impl(impl, nir) {
                  blob_write_uint32(blob, nir->constant_data_size);
   if (nir->constant_data_size > 0)
                     if (nir->info.stage == MESA_SHADER_KERNEL) {
      blob_write_uint32(blob, nir->printf_info_count);
   for (int i = 0; i < nir->printf_info_count; i++) {
      u_printf_info *info = &nir->printf_info[i];
   blob_write_uint32(blob, info->num_args);
   blob_write_uint32(blob, info->string_size);
   blob_write_bytes(blob, info->arg_sizes,
         /* we can't use blob_write_string, because it contains multiple NULL
   * terminated strings */
   blob_write_bytes(blob, info->strings,
                           _mesa_hash_table_destroy(ctx.remap_table, NULL);
      }
      nir_shader *
   nir_deserialize(void *mem_ctx,
               {
      read_ctx ctx = { 0 };
   ctx.blob = blob;
   list_inithead(&ctx.phi_srcs);
   ctx.idx_table_len = blob_read_uint32(blob);
            uint32_t strings = blob_read_uint32(blob);
   char *name = (strings & 0x1) ? blob_read_string(blob) : NULL;
            struct shader_info info;
                     info.name = name ? ralloc_strdup(ctx.nir, name) : NULL;
                              ctx.nir->num_inputs = blob_read_uint32(blob);
   ctx.nir->num_uniforms = blob_read_uint32(blob);
   ctx.nir->num_outputs = blob_read_uint32(blob);
            unsigned num_functions = blob_read_uint32(blob);
   for (unsigned i = 0; i < num_functions; i++)
            nir_foreach_function(fxn, ctx.nir) {
      if (fxn->impl == NIR_SERIALIZE_FUNC_HAS_IMPL)
               ctx.nir->constant_data_size = blob_read_uint32(blob);
   if (ctx.nir->constant_data_size > 0) {
      ctx.nir->constant_data =
         blob_copy_bytes(blob, ctx.nir->constant_data,
                        if (ctx.nir->info.stage == MESA_SHADER_KERNEL) {
      ctx.nir->printf_info_count = blob_read_uint32(blob);
   ctx.nir->printf_info =
            for (int i = 0; i < ctx.nir->printf_info_count; i++) {
      u_printf_info *info = &ctx.nir->printf_info[i];
   info->num_args = blob_read_uint32(blob);
   info->string_size = blob_read_uint32(blob);
   info->arg_sizes = ralloc_array(ctx.nir, unsigned, info->num_args);
   blob_copy_bytes(blob, info->arg_sizes,
         info->strings = ralloc_array(ctx.nir, char, info->string_size);
   blob_copy_bytes(blob, info->strings,
                                       }
      void
   nir_shader_serialize_deserialize(nir_shader *shader)
   {
               struct blob writer;
   blob_init(&writer);
            /* Delete all of dest's ralloc children but leave dest alone */
   void *dead_ctx = ralloc_context(NULL);
   ralloc_adopt(dead_ctx, shader);
                     struct blob_reader reader;
   blob_reader_init(&reader, writer.data, writer.size);
                     nir_shader_replace(shader, copy);
      }
