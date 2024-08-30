   /*
   * Copyright © 2016-2018 Broadcom
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
      #include "v3d_compiler.h"
      /* We don't do any address packing. */
   #define __gen_user_data void
   #define __gen_address_type uint32_t
   #define __gen_address_offset(reloc) (*reloc)
   #define __gen_emit_reloc(cl, reloc)
   #include "cle/v3d_packet_v41_pack.h"
      static inline struct qinst *
   vir_TMU_WRITE(struct v3d_compile *c, enum v3d_qpu_waddr waddr, struct qreg val)
   {
         /* XXX perf: We should figure out how to merge ALU operations
      * producing the val with this MOV, when possible.
      }
      static inline struct qinst *
   vir_TMU_WRITE_or_count(struct v3d_compile *c,
                     {
         if (tmu_writes) {
               } else {
         }
      static void
   vir_WRTMUC(struct v3d_compile *c, enum quniform_contents contents, uint32_t data)
   {
         struct qinst *inst = vir_NOP(c);
   inst->qpu.sig.wrtmuc = true;
   }
      static const struct V3D41_TMU_CONFIG_PARAMETER_1 p1_unpacked_default = {
         };
      static const struct V3D41_TMU_CONFIG_PARAMETER_2 p2_unpacked_default = {
         };
      /**
   * If 'tmu_writes' is not NULL, then it just counts required register writes,
   * otherwise, it emits the actual register writes.
   *
   * It is important to notice that emitting register writes for the current
   * TMU operation may trigger a TMU flush, since it is possible that any
   * of the inputs required for the register writes is the result of a pending
   * TMU operation. If that happens we need to make sure that it doesn't happen
   * in the middle of the TMU register writes for the current TMU operation,
   * which is why we always call ntq_get_src() even if we are only interested in
   * register write counts.
   */
   static void
   handle_tex_src(struct v3d_compile *c,
                  nir_tex_instr *instr,
   unsigned src_idx,
   unsigned non_array_components,
      {
         /* Either we are calling this just to count required TMU writes, or we
      * are calling this to emit the actual TMU writes.
               struct qreg s;
   switch (instr->src[src_idx].src_type) {
   case nir_tex_src_coord:
            /* S triggers the lookup, so save it for the end. */
   s = ntq_get_src(c, instr->src[src_idx].src, 0);
   if (tmu_writes)
                        if (non_array_components > 1) {
            struct qreg src =
                     if (non_array_components > 2) {
            struct qreg src =
                     if (instr->is_array) {
            struct qreg src =
         ntq_get_src(c, instr->src[src_idx].src,
            case nir_tex_src_bias: {
            struct qreg src = ntq_get_src(c, instr->src[src_idx].src, 0);
               case nir_tex_src_lod: {
            struct qreg src = ntq_get_src(c, instr->src[src_idx].src, 0);
   vir_TMU_WRITE_or_count(c, V3D_QPU_WADDR_TMUB, src, tmu_writes);
   if (!tmu_writes) {
            /* With texel fetch automatic LOD is already disabled,
   * and disable_autolod must not be enabled. For
   * non-cubes we can use the register TMUSLOD, that
   * implicitly sets disable_autolod.
   */
   assert(p2_unpacked);
   if (instr->op != nir_texop_txf &&
      instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
               case nir_tex_src_comparator: {
            struct qreg src = ntq_get_src(c, instr->src[src_idx].src, 0);
               case nir_tex_src_offset: {
            bool is_const_offset = nir_src_is_const(instr->src[src_idx].src);
   if (is_const_offset) {
            if (!tmu_writes) {
         p2_unpacked->offset_s =
         if (non_array_components >= 2)
               if (non_array_components >= 3)
      } else {
            struct qreg src_0 =
         struct qreg src_1 =
                                                               } else {
                  default:
         }
      static void
   vir_tex_handle_srcs(struct v3d_compile *c,
                     nir_tex_instr *instr,
   {
         unsigned non_array_components = instr->op != nir_texop_lod ?
                  for (unsigned i = 0; i < instr->num_srcs; i++) {
               }
      static unsigned
   get_required_tex_tmu_writes(struct v3d_compile *c, nir_tex_instr *instr)
   {
         unsigned tmu_writes = 0;
   vir_tex_handle_srcs(c, instr, NULL, NULL, &tmu_writes);
   }
      void
   v3d40_vir_emit_tex(struct v3d_compile *c, nir_tex_instr *instr)
   {
                           /* For instructions that don't have a sampler (i.e. txf) we bind
      * default sampler state via the backend_flags to handle precision.
      unsigned sampler_idx = nir_tex_instr_need_sampler(instr) ?
            /* Even if the texture operation doesn't need a sampler by
      * itself, we still need to add the sampler configuration
   * parameter if the output is 32 bit
      assert(sampler_idx < c->key->num_samplers_used);
   bool output_type_32_bit =
            struct V3D41_TMU_CONFIG_PARAMETER_0 p0_unpacked = {
            /* Limit the number of channels returned to both how many the NIR
      * instruction writes and how many the instruction could produce.
      nir_intrinsic_instr *store = nir_store_reg_for_def(&instr->def);
   if (store == NULL) {
               } else {
            nir_def *reg = store->src[1].ssa;
                        /* For the non-ssa case we don't have a full equivalent to
   * nir_def_components_read. This is a problem for the 16
   * bit case. nir_lower_tex will not change the destination as
   * nir_tex_instr_dest_size will still return 4. The driver is
   * just expected to not store on other channels, so we
   * manually ensure that here.
   */
                     }
            struct V3D41_TMU_CONFIG_PARAMETER_2 p2_unpacked = {
            .op = V3D_TMU_OP_REGULAR,
   .gather_mode = instr->op == nir_texop_tg4,
   .gather_component = instr->component,
                        /* The input FIFO has 16 slots across all threads so if we require
      * more than that we need to lower thread count.
      while (tmu_writes > 16 / c->threads)
            /* If pipelining this TMU operation would overflow TMU fifos, we need
   * to flush any outstanding TMU operations.
   */
   const unsigned dest_components =
         if (ntq_tmu_fifo_overflow(c, dest_components))
            /* Process tex sources emitting corresponding TMU writes */
   struct qreg s = { };
            uint32_t p0_packed;
   V3D41_TMU_CONFIG_PARAMETER_0_pack(NULL,
                  uint32_t p2_packed;
   V3D41_TMU_CONFIG_PARAMETER_2_pack(NULL,
                  /* We manually set the LOD Query bit (see
      * V3D42_TMU_CONFIG_PARAMETER_2) as right now is the only V42 specific
   * feature over V41 we are using
      if (instr->op == nir_texop_lod)
            /* Load texture_idx number into the high bits of the texture address field,
      * which will be be used by the driver to decide which texture to put
   * in the actual address field.
                        /* p1 is optional, but we can skip it only if p2 can be skipped too */
   bool needs_p2_config =
                        /* To handle the cases were we can't just use p1_unpacked_default */
   bool non_default_p1_config = nir_tex_instr_need_sampler(instr) ||
            if (non_default_p1_config) {
                                                /* Word enables can't ask for more channels than the
   * output type could provide (2 for f16, 4 for
   * 32-bit).
   */
   assert(!p1_unpacked.output_type_32_bit ||
                        uint32_t p1_packed;
                        if (nir_tex_instr_need_sampler(instr)) {
            /* Load sampler_idx number into the high bits of the
   * sampler address field, which will be be used by the
                           } else {
            /* In this case, we don't need to merge in any
   * sampler state from the API and can just use
   } else if (needs_p2_config) {
            /* Configuration parameters need to be set up in
   * order, and if P2 is needed, you need to set up P1
   * too even if sampler info is not needed by the
   * texture operation. But we can set up default info,
   * and avoid asking the driver for the sampler state
   * address
   */
   uint32_t p1_packed_default;
   V3D41_TMU_CONFIG_PARAMETER_1_pack(NULL,
                     if (needs_p2_config)
            /* Emit retiring TMU write */
   struct qinst *retiring;
   if (instr->op == nir_texop_txf) {
               } else if (instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
         } else if (instr->op == nir_texop_txl) {
         } else {
                  retiring->ldtmu_count = p0_unpacked.return_words_of_texture_data;
   ntq_add_pending_tmu_flush(c, &instr->def,
   }
      static uint32_t
   v3d40_image_atomic_tmu_op(nir_intrinsic_instr *instr)
   {
         nir_atomic_op atomic_op = nir_intrinsic_atomic_op(instr);
   switch (atomic_op) {
   case nir_atomic_op_iadd:    return v3d_get_op_for_atomic_add(instr, 3);
   case nir_atomic_op_imin:    return V3D_TMU_OP_WRITE_SMIN;
   case nir_atomic_op_umin:    return V3D_TMU_OP_WRITE_UMIN_FULL_L1_CLEAR;
   case nir_atomic_op_imax:    return V3D_TMU_OP_WRITE_SMAX;
   case nir_atomic_op_umax:    return V3D_TMU_OP_WRITE_UMAX;
   case nir_atomic_op_iand:    return V3D_TMU_OP_WRITE_AND_READ_INC;
   case nir_atomic_op_ior:     return V3D_TMU_OP_WRITE_OR_READ_DEC;
   case nir_atomic_op_ixor:    return V3D_TMU_OP_WRITE_XOR_READ_NOT;
   case nir_atomic_op_xchg:    return V3D_TMU_OP_WRITE_XCHG_READ_FLUSH;
   case nir_atomic_op_cmpxchg: return V3D_TMU_OP_WRITE_CMPXCHG_READ_FLUSH;
   default:                    unreachable("unknown atomic op");
   }
      static uint32_t
   v3d40_image_load_store_tmu_op(nir_intrinsic_instr *instr)
   {
         switch (instr->intrinsic) {
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
            case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
            default:
         }
      /**
   * If 'tmu_writes' is not NULL, then it just counts required register writes,
   * otherwise, it emits the actual register writes.
   *
   * It is important to notice that emitting register writes for the current
   * TMU operation may trigger a TMU flush, since it is possible that any
   * of the inputs required for the register writes is the result of a pending
   * TMU operation. If that happens we need to make sure that it doesn't happen
   * in the middle of the TMU register writes for the current TMU operation,
   * which is why we always call ntq_get_src() even if we are only interested in
   * register write counts.
   */
   static struct qinst *
   vir_image_emit_register_writes(struct v3d_compile *c,
                     {
         if (tmu_writes)
            bool is_1d = false;
   switch (nir_intrinsic_image_dim(instr)) {
   case GLSL_SAMPLER_DIM_1D:
               case GLSL_SAMPLER_DIM_BUF:
         case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_RECT:
   case GLSL_SAMPLER_DIM_CUBE: {
            struct qreg src = ntq_get_src(c, instr->src[1], 1);
      }
   case GLSL_SAMPLER_DIM_3D: {
            struct qreg src_1_1 = ntq_get_src(c, instr->src[1], 1);
   struct qreg src_1_2 = ntq_get_src(c, instr->src[1], 2);
   vir_TMU_WRITE_or_count(c, V3D_QPU_WADDR_TMUT, src_1_1, tmu_writes);
      }
   default:
                  /* In order to fetch on a cube map, we need to interpret it as
      * 2D arrays, where the third coord would be the face index.
      if (nir_intrinsic_image_dim(instr) == GLSL_SAMPLER_DIM_CUBE ||
         nir_intrinsic_image_array(instr)) {
                  /* Emit the data writes for atomics or image store. */
   if (instr->intrinsic != nir_intrinsic_image_load &&
         !atomic_add_replaced) {
      for (int i = 0; i < nir_intrinsic_src_components(instr, 3); i++) {
                              /* Second atomic argument */
   if (instr->intrinsic == nir_intrinsic_image_atomic_swap &&
      nir_intrinsic_atomic_op(instr) == nir_atomic_op_cmpxchg) {
         struct qreg src_4_0 = ntq_get_src(c, instr->src[4], 0);
            struct qreg src_1_0 = ntq_get_src(c, instr->src[1], 0);
   if (!tmu_writes && vir_in_nonuniform_control_flow(c) &&
         instr->intrinsic != nir_intrinsic_image_load) {
                  struct qinst *retiring =
            if (!tmu_writes && vir_in_nonuniform_control_flow(c) &&
         instr->intrinsic != nir_intrinsic_image_load) {
      struct qinst *last_inst =
               }
      static unsigned
   get_required_image_tmu_writes(struct v3d_compile *c,
               {
         unsigned tmu_writes;
   vir_image_emit_register_writes(c, instr, atomic_add_replaced,
         }
      void
   v3d40_vir_emit_image_load_store(struct v3d_compile *c,
         {
         unsigned format = nir_intrinsic_format(instr);
            struct V3D41_TMU_CONFIG_PARAMETER_0 p0_unpacked = {
            struct V3D41_TMU_CONFIG_PARAMETER_1 p1_unpacked = {
                                 /* Limit the number of channels returned to both how many the NIR
      * instruction writes and how many the instruction could produce.
      uint32_t instr_return_channels = nir_intrinsic_dest_components(instr);
   if (!p1_unpacked.output_type_32_bit)
            p0_unpacked.return_words_of_texture_data =
                     /* If we were able to replace atomic_add for an inc/dec, then we
      * need/can to do things slightly different, like not loading the
   * amount to add/sub, as that is implicit.
      bool atomic_add_replaced =
            instr->intrinsic == nir_intrinsic_image_atomic &&
               uint32_t p0_packed;
   V3D41_TMU_CONFIG_PARAMETER_0_pack(NULL,
                  /* Load unit number into the high bits of the texture or sampler
      * address field, which will be be used by the driver to decide which
   * texture to put in the actual address field.
               uint32_t p1_packed;
   V3D41_TMU_CONFIG_PARAMETER_1_pack(NULL,
                  uint32_t p2_packed;
   V3D41_TMU_CONFIG_PARAMETER_2_pack(NULL,
                  if (instr->intrinsic != nir_intrinsic_image_load)
               const uint32_t tmu_writes =
            /* The input FIFO has 16 slots across all threads so if we require
      * more than that we need to lower thread count.
      while (tmu_writes > 16 / c->threads)
            /* If pipelining this TMU operation would overflow TMU fifos, we need
   * to flush any outstanding TMU operations.
   */
   if (ntq_tmu_fifo_overflow(c, instr_return_channels))
            vir_WRTMUC(c, QUNIFORM_IMAGE_TMU_CONFIG_P0, p0_packed);
   if (memcmp(&p1_unpacked, &p1_unpacked_default, sizeof(p1_unpacked)))
         if (memcmp(&p2_unpacked, &p2_unpacked_default, sizeof(p2_unpacked)))
            struct qinst *retiring =
         retiring->ldtmu_count = p0_unpacked.return_words_of_texture_data;
   ntq_add_pending_tmu_flush(c, &instr->def,
   }
