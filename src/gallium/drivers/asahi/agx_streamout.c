   /*
   * Copyright 2023 Alyssa Rosenzweig
   * Copyright 2022 Collabora Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_xfb_info.h"
   #include "util/u_draw.h"
   #include "util/u_dump.h"
   #include "util/u_prim.h"
   #include "agx_state.h"
      static struct pipe_stream_output_target *
   agx_create_stream_output_target(struct pipe_context *pctx,
               {
                        if (!target)
            pipe_reference_init(&target->reference, 1);
            target->context = pctx;
   target->buffer_offset = buffer_offset;
               }
      static void
   agx_stream_output_target_destroy(struct pipe_context *pctx,
         {
      pipe_resource_reference(&target->buffer, NULL);
      }
      static void
   agx_set_stream_output_targets(struct pipe_context *pctx, unsigned num_targets,
               {
      struct agx_context *ctx = agx_context(pctx);
                     for (unsigned i = 0; i < num_targets; i++) {
      /* From the Gallium documentation:
   *
   *    -1 means the buffer should be appended to, and everything else sets
   *    the internal offset.
   *
   * We append regardless, so just check for != -1. Yes, using a negative
   * sentinel value with an unsigned type is bananas. But it's in the
   * Gallium contract and it will work out fine. Probably should be
   * redefined to be ~0 instead of -1 but it doesn't really matter.
   */
   if (offsets[i] != -1)
                        for (unsigned i = num_targets; i < so->num_targets; i++)
               }
      static struct pipe_stream_output_target *
   get_target(struct agx_context *ctx, unsigned buffer)
   {
      if (buffer < ctx->streamout.num_targets)
         else
      }
      /*
   * Return the address of the indexed streamout buffer. This will be
   * pushed into the streamout shader.
   */
   uint64_t
   agx_batch_get_so_address(struct agx_batch *batch, unsigned buffer,
         {
               /* If there's no target, don't write anything */
   if (!target) {
      *size = 0;
               /* Otherwise, write the target */
   struct pipe_stream_output_info *so =
            struct agx_resource *rsrc = agx_resource(target->buffer);
            /* The amount of space left depends how much we've already consumed */
   unsigned stride = so->stride[buffer] * 4;
            *size = offset < target->buffer_size ? (target->buffer_size - offset) : 0;
      }
      void
   agx_draw_vbo_from_xfb(struct pipe_context *pctx,
               {
      struct pipe_draw_start_count_bias draw = {
      .start = 0,
                  }
      static uint32_t
   xfb_prims_for_vertices(enum mesa_prim mode, unsigned verts)
   {
               /* The GL spec isn't super clear about this, but it implies that quads are
   * supposed to be tessellated into primitives and piglit
   * (ext_transform_feedback-tessellation quads) checks this.
   */
   if (u_decomposed_prim(mode) == MESA_PRIM_QUADS)
               }
      /*
   * Launch a streamout pipeline.
   */
   void
   agx_launch_so(struct pipe_context *pctx, const struct pipe_draw_info *info,
               {
               /* Break recursion from draw_vbo creating draw calls below: Do not do a
   * streamout draw for a streamout draw.
   */
   if (ctx->streamout.key.active)
            /* Configure the below draw to launch streamout rather than a regular draw */
   ctx->streamout.key.active = true;
            ctx->streamout.key.index_size = info->index_size;
   ctx->streamout.key.mode = info->mode;
   ctx->streamout.key.flatshade_first = ctx->rast->base.flatshade_first;
            /* Ignore provoking vertex for modes that don't depend on the provoking
   * vertex, to reduce shader variants.
   */
   if (info->mode != MESA_PRIM_TRIANGLE_STRIP)
            /* Determine how many vertices are XFB there will be */
   unsigned num_outputs =
         unsigned count = draw->count;
            ctx->streamout.params.base_vertex =
                  /* Streamout runs as a vertex shader with rasterizer discard */
   void *saved_rast = ctx->rast;
   pctx->bind_rasterizer_state(
            /* Dispatch a grid of points, this is compute-like */
   util_draw_arrays_instanced(pctx, MESA_PRIM_POINTS, 0, num_outputs, 0,
                  /*
   * Finally, if needed, update the counter of primitives written. The spec
   * requires:
   *
   *    If recording the vertices of a primitive to the buffer objects being
   *    used for transform feedback purposes would result in [overflow]...
   *    the counter corresponding to the asynchronous query target
   *    TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN (see section 13.4) is not
   *    incremented.
   *
   * So clamp the number of primitives generated to the number of primitives
   * we actually have space to write.
   */
   if (ctx->tf_prims_generated) {
               for (unsigned i = 0; i < ctx->streamout.num_targets; ++i) {
                              struct pipe_stream_output_info *so =
                  /* Ignore spurious targets. I don't see anything in the Gallium
   * contract specifically forbidding this.
   */
                  uint32_t offset = agx_so_target(target)->offset * stride;
   uint32_t remaining =
                              /* We now have the maximum vertices written, round down to primitives */
   uint32_t max_prims = xfb_prims_for_vertices(info->mode, min_max);
                        /* Update the offsets into the streamout buffers */
   for (unsigned i = 0; i < ctx->streamout.num_targets; ++i) {
      if (ctx->streamout.targets[i])
               ctx->dirty |= AGX_DIRTY_XFB;
      }
      /*
   * Count generated primitives on the CPU for transform feedback. This only works
   * in the absence of indirect draws, geometry shaders, or tessellation.
   */
   void
   agx_primitives_update_direct(struct agx_context *ctx,
               {
               ctx->prims_generated->value +=
      }
      /* The OpenGL spec says:
   *
   *    If recording the vertices of a primitive to the buffer objects being
   *    used for transform feedback purposes would result in either exceeding
   *    the limits of any buffer object’s size, or in exceeding the end
   *    position offset + size − 1, as set by BindBufferRange, then no vertices
   *    of that primitive are recorded in any buffer object.
   *
   * This function checks for the absence of overflow.
   *
   * The difficulty is that we are processing a single vertex at a time, so we
   * need to do some arithmetic to figure out the bounds for the whole containing
   * primitive.
   *
   * XXX: How do quads get tessellated?
   */
   static nir_def *
   primitive_fits(nir_builder *b, struct agx_xfb_key *key)
   {
      /* Get the number of vertices per primitive in the current mode, usually just
   * the base number but quads are tessellated.
   */
            if (u_decomposed_prim(key->mode) == MESA_PRIM_QUADS)
            /* Get the ID for this invocation */
            /* Figure out the ID for the first vertex of the next primitive. Since
   * transform feedback buffers are tightly packed, that's one byte after the
   * end of this primitive, which will make bounds checking convenient. That
   * will be:
   *
   *    (id - (id % prim size)) + prim size
   */
   nir_def *rem = nir_umod_imm(b, id, verts_per_prim);
            /* Figure out where that vertex will land */
   nir_def *index = nir_iadd(
      b, nir_imul(b, nir_load_instance_id(b), nir_load_num_vertices(b)),
         /* Now check for overflow in each written buffer */
            u_foreach_bit(buffer, b->shader->xfb_info->buffers_written) {
      uint16_t stride = b->shader->info.xfb_stride[buffer] * 4;
            /* For this primitive to fit, the next primitive cannot start after the
   * end of the transform feedback buffer.
   */
            /* Check whether that will remain in bounds */
   nir_def *fits =
            /* Accumulate */
                  }
      static void
   insert_overflow_check(nir_shader *nir, struct agx_xfb_key *key)
   {
               /* Extract the current transform feedback shader */
   nir_cf_list list;
            /* Get a builder for the (now empty) shader */
            /* Rebuild the shader as
   *
   *    if (!overflow) {
   *       shader();
   *    }
   */
   nir_push_if(&b, primitive_fits(&b, key));
   {
         }
      }
      static void
   lower_xfb_output(nir_builder *b, nir_intrinsic_instr *intr,
               {
      assert(buffer < MAX_XFB_BUFFERS);
            /* Transform feedback info in units of words, convert to bytes. */
   uint16_t stride = b->shader->info.xfb_stride[buffer] * 4;
                     nir_def *index = nir_iadd(
      b, nir_imul(b, nir_load_instance_id(b), nir_load_num_vertices(b)),
         nir_def *xfb_offset =
            nir_def *buf = nir_load_xfb_address(b, 64, .base = buffer);
            nir_def *value = nir_channels(
            }
      static bool
   lower_xfb(nir_builder *b, nir_intrinsic_instr *intr, UNUSED void *data)
   {
      if (intr->intrinsic != nir_intrinsic_store_output)
            /* Assume the inputs are read */
   BITSET_SET(b->shader->info.system_values_read,
                           for (unsigned i = 0; i < 2; ++i) {
      nir_io_xfb xfb =
            for (unsigned j = 0; j < 2; ++j) {
      if (xfb.out[j].num_components > 0) {
      b->cursor = nir_before_instr(&intr->instr);
   lower_xfb_output(b, intr, i * 2 + j, xfb.out[j].num_components,
                           nir_instr_remove(&intr->instr);
      }
      static bool
   lower_xfb_intrinsics(struct nir_builder *b, nir_intrinsic_instr *intr,
         {
                        switch (intr->intrinsic) {
   /* XXX: Rename to "xfb index" to avoid the clash */
   case nir_intrinsic_load_vertex_id_zero_base: {
      nir_def *id = nir_load_vertex_id(b);
   nir_def_rewrite_uses(&intr->def, id);
               case nir_intrinsic_load_vertex_id: {
      /* Get the raw invocation ID */
            /* Tessellate by primitive mode */
   if (key->mode == MESA_PRIM_LINE_STRIP ||
      key->mode == MESA_PRIM_LINE_LOOP) {
   /* The last vertex is special for a loop. Check if that's we're dealing
   * with.
   */
   nir_def *num_invocations =
                                       /* (0, 1), (1, 2), (2, 0) */
   if (key->mode == MESA_PRIM_LINE_LOOP) {
            } else if (key->mode == MESA_PRIM_TRIANGLE_STRIP) {
      /* Order depends on the provoking vertex.
   *
   * First: (0, 1, 2), (1, 3, 2), (2, 3, 4).
   * Last:  (0, 1, 2), (2, 1, 3), (2, 3, 4).
   */
                           /* Swap the two non-provoking vertices third vertex in odd triangles */
   nir_def *even = nir_ieq_imm(b, nir_iand_imm(b, prim, 1), 0);
   nir_def *is_provoking = nir_ieq_imm(b, rem, pv);
   nir_def *no_swap = nir_ior(b, is_provoking, even);
                  /* Pull the (maybe swapped) vertex from the corresponding primitive */
      } else if (key->mode == MESA_PRIM_TRIANGLE_FAN) {
      /* (0, 1, 2), (0, 2, 3) */
                  id = nir_bcsel(b, nir_ieq_imm(b, rem, 0), nir_imm_int(b, 0),
      } else if (key->mode == MESA_PRIM_QUADS ||
            /* Quads:       [(0, 1, 3), (3, 1, 2)], [(4, 5, 7), (7, 5, 6)]
   * Quad strips: [(0, 1, 3), (0, 2, 3)], [(2, 3, 5), (2, 4, 5)]
                  nir_def *prim = nir_udiv_imm(b, id, 6);
                  /* Quads:       [0, 1, 3, 3, 1, 2]
   * Quad strips: [0, 1, 3, 0, 2, 3]
   */
   uint32_t order_quads = 0x213310;
                  /* Index out of the bitpacked array */
   nir_def *offset = nir_iand_imm(
                              /* Add the "start", either an index bias or a base vertex */
            /* If drawing with an index buffer, pull the vertex ID. Otherwise, the
   * vertex ID is just the index as-is.
   */
   if (key->index_size) {
      nir_def *index_buffer = nir_load_xfb_index_buffer(b, 64);
   nir_def *offset = nir_imul_imm(b, id, key->index_size);
   nir_def *address = nir_iadd(b, index_buffer, nir_u2u64(b, offset));
                              nir_def_rewrite_uses(&intr->def, id);
               default:
            }
      void
   agx_nir_lower_xfb(nir_shader *nir, struct agx_xfb_key *key)
   {
               NIR_PASS_V(nir, nir_io_add_const_offset_to_base,
                  NIR_PASS_V(nir, insert_overflow_check, key);
   NIR_PASS_V(nir, nir_shader_intrinsics_pass, lower_xfb,
         NIR_PASS_V(nir, nir_shader_intrinsics_pass, lower_xfb_intrinsics,
            /* Lowering XFB creates piles of dead code. Eliminate now so we don't
   * push unnecessary sysvals.
   */
      }
      void
   agx_init_streamout_functions(struct pipe_context *ctx)
   {
      ctx->create_stream_output_target = agx_create_stream_output_target;
   ctx->stream_output_target_destroy = agx_stream_output_target_destroy;
      }
