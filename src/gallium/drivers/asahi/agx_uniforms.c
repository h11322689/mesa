   /*
   * Copyright 2021 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
   #include <stdio.h>
   #include "asahi/lib/agx_pack.h"
   #include "agx_state.h"
      static uint64_t
   agx_const_buffer_ptr(struct agx_batch *batch, struct pipe_constant_buffer *cb)
   {
      if (cb->buffer) {
      struct agx_resource *rsrc = agx_resource(cb->buffer);
               } else {
            }
      static uint64_t
   agx_shader_buffer_ptr(struct agx_batch *batch, struct pipe_shader_buffer *sb)
   {
      if (sb->buffer) {
               /* Assume SSBOs are written. TODO: Optimize read-only SSBOs */
               } else {
            }
      static uint64_t
   agx_vertex_buffer_ptr(struct agx_batch *batch, unsigned vbo)
   {
      struct pipe_vertex_buffer vb = batch->ctx->vertex_buffers[vbo];
            if (vb.buffer.resource) {
      struct agx_resource *rsrc = agx_resource(vb.buffer.resource);
               } else {
            }
      void
   agx_upload_vbos(struct agx_batch *batch)
   {
      u_foreach_bit(vbo, batch->ctx->vb_mask) {
            }
      void
   agx_upload_uniforms(struct agx_batch *batch)
   {
               struct agx_ptr root_ptr = agx_pool_alloc_aligned(
            batch->uniforms.tables[AGX_SYSVAL_TABLE_ROOT] = root_ptr.gpu;
            if (ctx->streamout.key.active) {
               for (unsigned i = 0; i < batch->ctx->streamout.num_targets; ++i) {
      uint32_t size = 0;
   batch->uniforms.xfb.base[i] =
                           }
      uint64_t
   agx_upload_stage_uniforms(struct agx_batch *batch, uint64_t textures,
         {
      struct agx_context *ctx = batch->ctx;
            struct agx_ptr root_ptr = agx_pool_alloc_aligned(
            struct agx_stage_uniforms uniforms = {
                  u_foreach_bit(s, st->valid_samplers) {
                  u_foreach_bit(cb, st->cb_mask) {
                  u_foreach_bit(cb, st->ssbo_mask) {
      uniforms.ssbo_base[cb] = agx_shader_buffer_ptr(batch, &st->ssbo[cb]);
               memcpy(root_ptr.cpu, &uniforms, sizeof(uniforms));
      }
