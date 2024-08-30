   /*
   * Copyright 2014, 2015 Red Hat.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include <libsync.h>
   #include "pipe/p_shader_tokens.h"
      #include "compiler/nir/nir.h"
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
   #include "nir/nir_to_tgsi.h"
   #include "util/format/u_format.h"
   #include "indices/u_primconvert.h"
   #include "util/u_draw.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_surface.h"
   #include "util/u_transfer.h"
   #include "util/u_helpers.h"
   #include "util/slab.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_blitter.h"
      #include "virgl_encode.h"
   #include "virgl_context.h"
   #include "virtio-gpu/virgl_protocol.h"
   #include "virgl_resource.h"
   #include "virgl_screen.h"
   #include "virgl_staging_mgr.h"
   #include "virgl_video.h"
      static uint32_t next_handle;
   uint32_t virgl_object_assign_handle(void)
   {
         }
      bool
   virgl_can_rebind_resource(struct virgl_context *vctx,
         {
      /* We cannot rebind resources that are referenced by host objects, which
   * are
   *
   *  - VIRGL_OBJECT_SURFACE
   *  - VIRGL_OBJECT_SAMPLER_VIEW
   *  - VIRGL_OBJECT_STREAMOUT_TARGET
   *
   * Because surfaces cannot be created from buffers, we require the resource
   * to be a buffer instead (and avoid tracking VIRGL_OBJECT_SURFACE binds).
   */
   const unsigned unsupported_bind = (PIPE_BIND_SAMPLER_VIEW |
         const unsigned bind_history = virgl_resource(res)->bind_history;
      }
      void
   virgl_rebind_resource(struct virgl_context *vctx,
         {
      /* Queries use internally created buffers and do not go through transfers.
   * Index buffers are not bindable.  They are not tracked.
   */
   ASSERTED const unsigned tracked_bind = (PIPE_BIND_VERTEX_BUFFER |
                     const unsigned bind_history = virgl_resource(res)->bind_history;
            assert(virgl_can_rebind_resource(vctx, res) &&
            if (bind_history & PIPE_BIND_VERTEX_BUFFER) {
      for (i = 0; i < vctx->num_vertex_buffers; i++) {
      if (vctx->vertex_buffer[i].buffer.resource == res) {
      vctx->vertex_array_dirty = true;
                     if (bind_history & PIPE_BIND_SHADER_BUFFER) {
      uint32_t remaining_mask = vctx->atomic_buffer_enabled_mask;
   while (remaining_mask) {
      int i = u_bit_scan(&remaining_mask);
   if (vctx->atomic_buffers[i].buffer == res) {
      const struct pipe_shader_buffer *abo = &vctx->atomic_buffers[i];
                     /* check per-stage shader bindings */
   if (bind_history & (PIPE_BIND_CONSTANT_BUFFER |
                  enum pipe_shader_type shader_type;
   for (shader_type = 0; shader_type < PIPE_SHADER_TYPES; shader_type++) {
                     if (bind_history & PIPE_BIND_CONSTANT_BUFFER) {
      uint32_t remaining_mask = binding->ubo_enabled_mask;
   while (remaining_mask) {
      int i = u_bit_scan(&remaining_mask);
   if (binding->ubos[i].buffer == res) {
      const struct pipe_constant_buffer *ubo = &binding->ubos[i];
   virgl_encoder_set_uniform_buffer(vctx, shader_type, i,
                                 if (bind_history & PIPE_BIND_SHADER_BUFFER) {
      uint32_t remaining_mask = binding->ssbo_enabled_mask;
   while (remaining_mask) {
      int i = u_bit_scan(&remaining_mask);
   if (binding->ssbos[i].buffer == res) {
      const struct pipe_shader_buffer *ssbo = &binding->ssbos[i];
   virgl_encode_set_shader_buffers(vctx, shader_type, i, 1,
                     if (bind_history & PIPE_BIND_SHADER_IMAGE) {
      uint32_t remaining_mask = binding->image_enabled_mask;
   while (remaining_mask) {
      int i = u_bit_scan(&remaining_mask);
   if (binding->images[i].resource == res) {
      const struct pipe_image_view *image = &binding->images[i];
   virgl_encode_set_shader_images(vctx, shader_type, i, 1,
                     }
      static void virgl_attach_res_framebuffer(struct virgl_context *vctx)
   {
      struct virgl_winsys *vws = virgl_screen(vctx->base.screen)->vws;
   struct pipe_surface *surf;
   struct virgl_resource *res;
            surf = vctx->framebuffer.zsbuf;
   if (surf) {
      res = virgl_resource(surf->texture);
   if (res) {
      vws->emit_res(vws, vctx->cbuf, res->hw_res, false);
         }
   for (i = 0; i < vctx->framebuffer.nr_cbufs; i++) {
      surf = vctx->framebuffer.cbufs[i];
   if (surf) {
      res = virgl_resource(surf->texture);
   if (res) {
      vws->emit_res(vws, vctx->cbuf, res->hw_res, false);
               }
      static void virgl_attach_res_sampler_views(struct virgl_context *vctx,
         {
      struct virgl_winsys *vws = virgl_screen(vctx->base.screen)->vws;
   const struct virgl_shader_binding_state *binding =
            for (int i = 0; i < PIPE_MAX_SHADER_SAMPLER_VIEWS; ++i) {
      if (binding->views[i] && binding->views[i]->texture) {
      struct virgl_resource *res = virgl_resource(binding->views[i]->texture);
            }
      static void virgl_attach_res_vertex_buffers(struct virgl_context *vctx)
   {
      struct virgl_winsys *vws = virgl_screen(vctx->base.screen)->vws;
   struct virgl_resource *res;
            for (i = 0; i < vctx->num_vertex_buffers; i++) {
      res = virgl_resource(vctx->vertex_buffer[i].buffer.resource);
   if (res)
         }
      static void virgl_attach_res_index_buffer(struct virgl_context *vctx,
         {
      struct virgl_winsys *vws = virgl_screen(vctx->base.screen)->vws;
            res = virgl_resource(ib->buffer);
   if (res)
      }
      static void virgl_attach_res_so_targets(struct virgl_context *vctx)
   {
      struct virgl_winsys *vws = virgl_screen(vctx->base.screen)->vws;
   struct virgl_resource *res;
            for (i = 0; i < vctx->num_so_targets; i++) {
      res = virgl_resource(vctx->so_targets[i].base.buffer);
   if (res)
         }
      static void virgl_attach_res_uniform_buffers(struct virgl_context *vctx,
         {
      struct virgl_winsys *vws = virgl_screen(vctx->base.screen)->vws;
   const struct virgl_shader_binding_state *binding =
         uint32_t remaining_mask = binding->ubo_enabled_mask;
            while (remaining_mask) {
      int i = u_bit_scan(&remaining_mask);
   res = virgl_resource(binding->ubos[i].buffer);
   assert(res);
         }
      static void virgl_attach_res_shader_buffers(struct virgl_context *vctx,
         {
      struct virgl_winsys *vws = virgl_screen(vctx->base.screen)->vws;
   const struct virgl_shader_binding_state *binding =
         uint32_t remaining_mask = binding->ssbo_enabled_mask;
            while (remaining_mask) {
      int i = u_bit_scan(&remaining_mask);
   res = virgl_resource(binding->ssbos[i].buffer);
   assert(res);
         }
      static void virgl_attach_res_shader_images(struct virgl_context *vctx,
         {
      struct virgl_winsys *vws = virgl_screen(vctx->base.screen)->vws;
   const struct virgl_shader_binding_state *binding =
         uint32_t remaining_mask = binding->image_enabled_mask;
            while (remaining_mask) {
      int i = u_bit_scan(&remaining_mask);
   res = virgl_resource(binding->images[i].resource);
   assert(res);
         }
      static void virgl_attach_res_atomic_buffers(struct virgl_context *vctx)
   {
      struct virgl_winsys *vws = virgl_screen(vctx->base.screen)->vws;
   uint32_t remaining_mask = vctx->atomic_buffer_enabled_mask;
            while (remaining_mask) {
      int i = u_bit_scan(&remaining_mask);
   res = virgl_resource(vctx->atomic_buffers[i].buffer);
   assert(res);
         }
      /*
   * after flushing, the hw context still has a bunch of
   * resources bound, so we need to rebind those here.
   */
   static void virgl_reemit_draw_resources(struct virgl_context *vctx)
   {
               /* reattach any flushed resources */
   /* framebuffer, sampler views, vertex/index/uniform/stream buffers */
            for (shader_type = 0; shader_type < PIPE_SHADER_COMPUTE; shader_type++) {
      virgl_attach_res_sampler_views(vctx, shader_type);
   virgl_attach_res_uniform_buffers(vctx, shader_type);
   virgl_attach_res_shader_buffers(vctx, shader_type);
      }
   virgl_attach_res_atomic_buffers(vctx);
   virgl_attach_res_vertex_buffers(vctx);
      }
      static void virgl_reemit_compute_resources(struct virgl_context *vctx)
   {
      virgl_attach_res_sampler_views(vctx, PIPE_SHADER_COMPUTE);
   virgl_attach_res_uniform_buffers(vctx, PIPE_SHADER_COMPUTE);
   virgl_attach_res_shader_buffers(vctx, PIPE_SHADER_COMPUTE);
               }
      static struct pipe_surface *virgl_create_surface(struct pipe_context *ctx,
               {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_surface *surf;
   struct virgl_resource *res = virgl_resource(resource);
            /* no support for buffer surfaces */
   if (resource->target == PIPE_BUFFER)
            surf = CALLOC_STRUCT(virgl_surface);
   if (!surf)
            assert(ctx->screen->get_param(ctx->screen,
                        virgl_resource_dirty(res, 0);
   handle = virgl_object_assign_handle();
   pipe_reference_init(&surf->base.reference, 1);
   pipe_resource_reference(&surf->base.texture, resource);
   surf->base.context = ctx;
            surf->base.width = u_minify(resource->width0, templ->u.tex.level);
   surf->base.height = u_minify(resource->height0, templ->u.tex.level);
   surf->base.u.tex.level = templ->u.tex.level;
   surf->base.u.tex.first_layer = templ->u.tex.first_layer;
   surf->base.u.tex.last_layer = templ->u.tex.last_layer;
            virgl_encoder_create_surface(vctx, handle, res, &surf->base);
   surf->handle = handle;
      }
      static void virgl_surface_destroy(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
            pipe_resource_reference(&surf->base.texture, NULL);
   virgl_encode_delete_object(vctx, surf->handle, VIRGL_OBJECT_SURFACE);
      }
      static void *virgl_create_blend_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   uint32_t handle;
            virgl_encode_blend_state(vctx, handle, blend_state);
         }
      static void virgl_bind_blend_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   uint32_t handle = (unsigned long)blend_state;
      }
      static void virgl_delete_blend_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   uint32_t handle = (unsigned long)blend_state;
      }
      static void *virgl_create_depth_stencil_alpha_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   uint32_t handle;
            virgl_encode_dsa_state(vctx, handle, blend_state);
      }
      static void virgl_bind_depth_stencil_alpha_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   uint32_t handle = (unsigned long)blend_state;
      }
      static void virgl_delete_depth_stencil_alpha_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   uint32_t handle = (unsigned long)dsa_state;
      }
      static void *virgl_create_rasterizer_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
            if (!vrs)
         vrs->rs = *rs_state;
            assert(rs_state->depth_clip_near ||
            virgl_encode_rasterizer_state(vctx, vrs->handle, rs_state);
      }
      static void virgl_bind_rasterizer_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   uint32_t handle = 0;
   if (rs_state) {
      struct virgl_rasterizer_state *vrs = rs_state;
   vctx->rs_state = *vrs;
      }
      }
      static void virgl_delete_rasterizer_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_rasterizer_state *vrs = rs_state;
   virgl_encode_delete_object(vctx, vrs->handle, VIRGL_OBJECT_RASTERIZER);
      }
      static void virgl_set_framebuffer_state(struct pipe_context *ctx,
         {
               vctx->framebuffer = *state;
   virgl_encoder_set_framebuffer_state(vctx, state);
      }
      static void virgl_set_viewport_states(struct pipe_context *ctx,
                     {
      struct virgl_context *vctx = virgl_context(ctx);
      }
      static void *virgl_create_vertex_elements_state(struct pipe_context *ctx,
               {
      struct pipe_vertex_element new_elements[PIPE_MAX_ATTRIBS];
   struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_vertex_elements_state *state =
            for (int i = 0; i < num_elements; ++i) {
      if (elements[i].instance_divisor) {
      /* Virglrenderer doesn't deal with instance_divisor correctly if
   * there isn't a 1:1 relationship between elements and bindings.
   * So let's make sure there is, by duplicating bindings.
   */
   for (int j = 0; j < num_elements; ++j) {
      new_elements[j] = elements[j];
   new_elements[j].vertex_buffer_index = j;
      }
   elements = new_elements;
   state->num_bindings = num_elements;
         }
   for (int i = 0; i < num_elements; ++i)
            state->handle = virgl_object_assign_handle();
   virgl_encoder_create_vertex_elements(vctx, state->handle,
            }
      static void virgl_delete_vertex_elements_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_vertex_elements_state *state =
         virgl_encode_delete_object(vctx, state->handle, VIRGL_OBJECT_VERTEX_ELEMENTS);
      }
      static void virgl_bind_vertex_elements_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_vertex_elements_state *state =
         vctx->vertex_elements = state;
   virgl_encode_bind_object(vctx, state ? state->handle : 0,
            }
      static void virgl_set_vertex_buffers(struct pipe_context *ctx,
                           {
               util_set_vertex_buffers_count(vctx->vertex_buffer,
                              if (buffers) {
      for (unsigned i = 0; i < num_buffers; i++) {
      struct virgl_resource *res =
         if (res && !buffers[i].is_user_buffer)
                     }
      static void virgl_hw_set_vertex_buffers(struct virgl_context *vctx)
   {
      if (vctx->vertex_array_dirty) {
               if (ve->num_bindings) {
      struct pipe_vertex_buffer vertex_buffers[PIPE_MAX_ATTRIBS];
                     } else
                           }
      static void virgl_set_stencil_ref(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
      }
      static void virgl_set_blend_color(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
      }
      static void virgl_hw_set_index_buffer(struct virgl_context *vctx,
         {
      virgl_encoder_set_index_buffer(vctx, ib);
      }
      static void virgl_set_constant_buffer(struct pipe_context *ctx,
                     {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_shader_binding_state *binding =
            if (buf && buf->buffer) {
      struct virgl_resource *res = virgl_resource(buf->buffer);
            virgl_encoder_set_uniform_buffer(vctx, shader, index,
                  if (take_ownership) {
      pipe_resource_reference(&binding->ubos[index].buffer, NULL);
      } else {
         }
   binding->ubos[index] = *buf;
      } else {
      static const struct pipe_constant_buffer dummy_ubo;
   if (!buf)
         virgl_encoder_write_constant_buffer(vctx, shader, index,
                  pipe_resource_reference(&binding->ubos[index].buffer, NULL);
         }
      static bool
   lower_gles_arrayshadow_offset_filter(const nir_instr *instr,
         {
      if (instr->type != nir_instr_type_tex)
                     if (!tex->is_shadow || !tex->is_array)
            // textureGradOffset can be used directly
   int grad_index = nir_tex_instr_src_index(tex, nir_tex_src_ddx);
   int proj_index = nir_tex_instr_src_index(tex, nir_tex_src_projector);
   if (grad_index >= 0 && proj_index < 0)
            int offset_index = nir_tex_instr_src_index(tex, nir_tex_src_offset);
   if (offset_index >= 0)
               }
      static void *virgl_shader_encoder(struct pipe_context *ctx,
               {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_screen *rs = virgl_screen(ctx->screen);
   uint32_t handle;
   const struct tgsi_token *tokens;
   const struct tgsi_token *ntt_tokens = NULL;
   struct tgsi_token *new_tokens;
   int ret;
            if (shader->type == PIPE_SHADER_IR_NIR) {
      struct nir_to_tgsi_options options = {
      .unoptimized_ra = true,
   .lower_fabs = true,
   .lower_ssbo_bindings =
                     if (!(rs->caps.caps.v2.capability_bits_v2 & VIRGL_CAP_V2_TEXTURE_SHADOW_LOD) &&
      rs->caps.caps.v2.capability_bits & VIRGL_CAP_HOST_IS_GLES) {
   nir_lower_tex_options lower_tex_options = {
                                       /* The host can't handle certain IO slots as separable, because we can't assign
   * more than 32 IO locations explicitly, and with varyings and patches we already
   * exhaust the possible ways of handling this for the varyings with generic names,
   * so drop the flag in these cases */
   const uint64_t drop_slots_for_separable_io = 0xffull << VARYING_SLOT_TEX0 |
                                 bool keep_separable_flags = true;
   if (s->info.stage != MESA_SHADER_VERTEX)
         if (s->info.stage != MESA_SHADER_FRAGMENT)
            /* Propagare the separable shader property to the host, unless
   * it is an internal shader - these are marked separable even though they are not. */
   is_separable = s->info.separate_shader && !s->info.internal && keep_separable_flags;
      } else {
                  new_tokens = virgl_tgsi_transform(rs, tokens, is_separable);
   if (!new_tokens)
            handle = virgl_object_assign_handle();
   /* encode VS state */
   ret = virgl_encode_shader_state(vctx, handle, type,
               if (ret) {
      FREE((void *)ntt_tokens);
               FREE((void *)ntt_tokens);
   FREE(new_tokens);
         }
   static void *virgl_create_vs_state(struct pipe_context *ctx,
         {
         }
      static void *virgl_create_tcs_state(struct pipe_context *ctx,
         {
         }
      static void *virgl_create_tes_state(struct pipe_context *ctx,
         {
         }
      static void *virgl_create_gs_state(struct pipe_context *ctx,
         {
         }
      static void *virgl_create_fs_state(struct pipe_context *ctx,
         {
         }
      static void
   virgl_delete_fs_state(struct pipe_context *ctx,
         {
      uint32_t handle = (unsigned long)fs;
               }
      static void
   virgl_delete_gs_state(struct pipe_context *ctx,
         {
      uint32_t handle = (unsigned long)gs;
               }
      static void
   virgl_delete_vs_state(struct pipe_context *ctx,
         {
      uint32_t handle = (unsigned long)vs;
               }
      static void
   virgl_delete_tcs_state(struct pipe_context *ctx,
         {
      uint32_t handle = (unsigned long)tcs;
               }
      static void
   virgl_delete_tes_state(struct pipe_context *ctx,
         {
      uint32_t handle = (unsigned long)tes;
               }
      static void virgl_bind_vs_state(struct pipe_context *ctx,
         {
      uint32_t handle = (unsigned long)vss;
               }
      static void virgl_bind_tcs_state(struct pipe_context *ctx,
         {
      uint32_t handle = (unsigned long)vss;
               }
      static void virgl_bind_tes_state(struct pipe_context *ctx,
         {
      uint32_t handle = (unsigned long)vss;
               }
      static void virgl_bind_gs_state(struct pipe_context *ctx,
         {
      uint32_t handle = (unsigned long)vss;
               }
         static void virgl_bind_fs_state(struct pipe_context *ctx,
         {
      uint32_t handle = (unsigned long)vss;
               }
      static void virgl_clear(struct pipe_context *ctx,
                           {
               if (!vctx->num_draws)
                     }
      static void virgl_clear_render_target(struct pipe_context *ctx,
                                 {
      if (virgl_debug & VIRGL_DEBUG_VERBOSE)
      }
      static void virgl_clear_texture(struct pipe_context *ctx,
                           {
      struct virgl_screen *rs = virgl_screen(ctx->screen);
            if (rs->caps.caps.v2.capability_bits & VIRGL_CAP_CLEAR_TEXTURE) {
      struct virgl_context *vctx = virgl_context(ctx);
      } else {
         }
   /* Mark as dirty, since we are updating the host side resource
   * without going through the corresponding guest side resource, and
   * hence the two will diverge.
   */
      }
      static void virgl_draw_vbo(struct pipe_context *ctx,
                                 {
      if (num_draws > 1) {
      util_draw_multi(ctx, dinfo, drawid_offset, indirect, draws, num_draws);
               if (!indirect && (!draws[0].count || !dinfo->instance_count))
            struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_screen *rs = virgl_screen(ctx->screen);
   struct virgl_indexbuf ib = {};
            if (!indirect &&
      !dinfo->primitive_restart &&
   !u_trim_pipe_prim(dinfo->mode, (unsigned*)&draws[0].count))
         if (!(rs->caps.caps.v1.prim_mask & (1 << dinfo->mode))) {
      util_primconvert_save_rasterizer_state(vctx->primconvert, &vctx->rs_state.rs);
   util_primconvert_draw_vbo(vctx->primconvert, dinfo, drawid_offset, indirect, draws, num_draws);
      }
   if (info.index_size) {
         pipe_resource_reference(&ib.buffer, info.has_user_indices ? NULL : info.index.resource);
   ib.user_buffer = info.has_user_indices ? info.index.user : NULL;
                  if (ib.user_buffer) {
            unsigned start_offset = draws[0].start * ib.index_size;
   u_upload_data(vctx->uploader, 0,
                  }
            if (!vctx->num_draws)
                                          }
      static void virgl_submit_cmd(struct virgl_winsys *vws,
               {
      if (unlikely(virgl_debug & VIRGL_DEBUG_SYNC)) {
                        vws->fence_wait(vws, sync_fence, OS_TIMEOUT_INFINITE);
      } else {
            }
      void virgl_flush_eq(struct virgl_context *ctx, void *closure,
         {
               /* skip empty cbuf */
   if (ctx->cbuf->cdw == ctx->cbuf_initial_cdw &&
      ctx->queue.num_dwords == 0 &&
   !fence)
         if (ctx->num_draws)
            /* send the buffer to the remote side for decoding */
                              /* Reserve some space for transfers. */
   if (ctx->encoded_transfers)
                              /* We have flushed the command queue, including any pending copy transfers
   * involving staging resources.
   */
      }
      static void virgl_flush_from_st(struct pipe_context *ctx,
               {
                  }
      static struct pipe_sampler_view *virgl_create_sampler_view(struct pipe_context *ctx,
               {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_sampler_view *grview;
   uint32_t handle;
            if (!state)
            grview = CALLOC_STRUCT(virgl_sampler_view);
   if (!grview)
            res = virgl_resource(texture);
   handle = virgl_object_assign_handle();
            grview->base = *state;
            grview->base.texture = NULL;
   grview->base.context = ctx;
   pipe_resource_reference(&grview->base.texture, texture);
   grview->handle = handle;
      }
      static void virgl_set_sampler_views(struct pipe_context *ctx,
                                       {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_shader_binding_state *binding =
            for (unsigned i = 0; i < num_views; i++) {
      unsigned idx = start_slot + i;
   if (views && views[i]) {
                     if (take_ownership) {
      pipe_sampler_view_reference(&binding->views[idx], NULL);
      } else {
            } else {
                     virgl_encode_set_sampler_views(vctx, shader_type,
                  if (unbind_num_trailing_slots) {
      virgl_set_sampler_views(ctx, shader_type, start_slot + num_views,
         }
      static void
   virgl_texture_barrier(struct pipe_context *ctx, unsigned flags)
   {
      struct virgl_context *vctx = virgl_context(ctx);
            if (!(rs->caps.caps.v2.capability_bits & VIRGL_CAP_TEXTURE_BARRIER) &&
      !(rs->caps.caps.v2.capability_bits_v2 & VIRGL_CAP_V2_BLEND_EQUATION))
         }
      static void virgl_destroy_sampler_view(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
            virgl_encode_delete_object(vctx, grview->handle, VIRGL_OBJECT_SAMPLER_VIEW);
   pipe_resource_reference(&view->texture, NULL);
      }
      static void *virgl_create_sampler_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
                     virgl_encode_sampler_state(vctx, handle, state);
      }
      static void virgl_delete_sampler_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
               }
      static void virgl_bind_sampler_states(struct pipe_context *ctx,
                           {
      struct virgl_context *vctx = virgl_context(ctx);
   uint32_t handles[PIPE_MAX_SHADER_SAMPLER_VIEWS];
   int i;
   for (i = 0; i < num_samplers; i++) {
         }
      }
      static void virgl_set_polygon_stipple(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
      }
      static void virgl_set_scissor_states(struct pipe_context *ctx,
                     {
      struct virgl_context *vctx = virgl_context(ctx);
      }
      static void virgl_set_sample_mask(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
      }
      static void virgl_set_min_samples(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
            if (!(rs->caps.caps.v2.capability_bits & VIRGL_CAP_SET_MIN_SAMPLES))
            }
      static void virgl_set_clip_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
      }
      static void virgl_set_tess_state(struct pipe_context *ctx,
               {
      struct virgl_context *vctx = virgl_context(ctx);
            if (!rs->caps.caps.v1.bset.has_tessellation_shaders)
            }
      static void virgl_set_patch_vertices(struct pipe_context *ctx, uint8_t patch_vertices)
   {
                  }
      static void virgl_resource_copy_region(struct pipe_context *ctx,
                                       {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_resource *dres = virgl_resource(dst);
            if (dres->b.target == PIPE_BUFFER)
                  virgl_encode_resource_copy_region(vctx, dres,
                  }
      static void
   virgl_flush_resource(struct pipe_context *pipe,
         {
   }
      static void virgl_blit(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_resource *dres = virgl_resource(blit->dst.resource);
            assert(ctx->screen->get_param(ctx->screen,
                        virgl_resource_dirty(dres, blit->dst.level);
   virgl_encode_blit(vctx, dres, sres,
      }
      static void virgl_set_hw_atomic_buffers(struct pipe_context *ctx,
                     {
               vctx->atomic_buffer_enabled_mask &= ~u_bit_consecutive(start_slot, count);
   for (unsigned i = 0; i < count; i++) {
      unsigned idx = start_slot + i;
   if (buffers && buffers[i].buffer) {
                     pipe_resource_reference(&vctx->atomic_buffers[idx].buffer,
         vctx->atomic_buffers[idx] = buffers[i];
      } else {
                        }
      static void virgl_set_shader_buffers(struct pipe_context *ctx,
                           {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_screen *rs = virgl_screen(ctx->screen);
   struct virgl_shader_binding_state *binding =
            binding->ssbo_enabled_mask &= ~u_bit_consecutive(start_slot, count);
   for (unsigned i = 0; i < count; i++) {
      unsigned idx = start_slot + i;
   if (buffers && buffers[i].buffer) {
                     pipe_resource_reference(&binding->ssbos[idx].buffer, buffers[i].buffer);
   binding->ssbos[idx] = buffers[i];
      } else {
                     uint32_t max_shader_buffer = (shader == PIPE_SHADER_FRAGMENT || shader == PIPE_SHADER_COMPUTE) ?
      rs->caps.caps.v2.max_shader_buffer_frag_compute :
      if (!max_shader_buffer)
            }
      static void virgl_create_fence_fd(struct pipe_context *ctx,
                     {
      assert(type == PIPE_FD_TYPE_NATIVE_SYNC);
            if (rs->vws->cs_create_fence)
      }
      static void virgl_fence_server_sync(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
            if (rs->vws->fence_server_sync)
      }
      static void virgl_set_shader_images(struct pipe_context *ctx,
                           {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_screen *rs = virgl_screen(ctx->screen);
   struct virgl_shader_binding_state *binding =
            binding->image_enabled_mask &= ~u_bit_consecutive(start_slot, count);
   for (unsigned i = 0; i < count; i++) {
      unsigned idx = start_slot + i;
   if (images && images[i].resource) {
                     pipe_resource_reference(&binding->images[idx].resource,
         binding->images[idx] = images[i];
      } else {
                     uint32_t max_shader_images = (shader == PIPE_SHADER_FRAGMENT || shader == PIPE_SHADER_COMPUTE) ?
   rs->caps.caps.v2.max_shader_image_frag_compute :
   rs->caps.caps.v2.max_shader_image_other_stages;
   if (!max_shader_images)
                  if (unbind_num_trailing_slots) {
      virgl_set_shader_images(ctx, shader, start_slot + count,
         }
      static void virgl_memory_barrier(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
            if (!(rs->caps.caps.v2.capability_bits & VIRGL_CAP_MEMORY_BARRIER))
            }
      static void *virgl_create_compute_state(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   uint32_t handle;
   const struct tgsi_token *ntt_tokens = NULL;
   const struct tgsi_token *tokens;
   struct pipe_stream_output_info so_info = {};
            if (state->ir_type == PIPE_SHADER_IR_NIR) {
      struct nir_to_tgsi_options options = {
      .unoptimized_ra = true,
      };
   nir_shader *s = nir_shader_clone(NULL, state->prog);
      } else {
                  void *new_tokens = virgl_tgsi_transform((struct virgl_screen *)vctx->base.screen, tokens, false);
   if (!new_tokens)
            handle = virgl_object_assign_handle();
   ret = virgl_encode_shader_state(vctx, handle, PIPE_SHADER_COMPUTE,
                     if (ret) {
      FREE((void *)ntt_tokens);
               FREE((void *)ntt_tokens);
               }
      static void virgl_bind_compute_state(struct pipe_context *ctx, void *state)
   {
      uint32_t handle = (unsigned long)state;
               }
      static void virgl_delete_compute_state(struct pipe_context *ctx, void *state)
   {
      uint32_t handle = (unsigned long)state;
               }
      static void virgl_launch_grid(struct pipe_context *ctx,
         {
               if (!vctx->num_compute)
                     }
      static void
   virgl_release_shader_binding(struct virgl_context *vctx,
         {
      struct virgl_shader_binding_state *binding =
            for (int i = 0; i < PIPE_MAX_SHADER_SAMPLER_VIEWS; ++i) {
      if (binding->views[i]) {
      pipe_sampler_view_reference(
                  while (binding->ubo_enabled_mask) {
      int i = u_bit_scan(&binding->ubo_enabled_mask);
               while (binding->ssbo_enabled_mask) {
      int i = u_bit_scan(&binding->ssbo_enabled_mask);
               while (binding->image_enabled_mask) {
      int i = u_bit_scan(&binding->image_enabled_mask);
         }
      static void
   virgl_emit_string_marker(struct pipe_context *ctx, const char *message,  int len)
   {
      struct virgl_context *vctx = virgl_context(ctx);
      }
      static void
   virgl_context_destroy( struct pipe_context *ctx )
   {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_screen *rs = virgl_screen(ctx->screen);
            vctx->framebuffer.zsbuf = NULL;
   vctx->framebuffer.nr_cbufs = 0;
   virgl_encoder_destroy_sub_ctx(vctx, vctx->hw_sub_ctx_id);
            for (shader_type = 0; shader_type < PIPE_SHADER_TYPES; shader_type++)
            while (vctx->atomic_buffer_enabled_mask) {
      int i = u_bit_scan(&vctx->atomic_buffer_enabled_mask);
               rs->vws->cmd_buf_destroy(vctx->cbuf);
   if (vctx->uploader)
         if (vctx->supports_staging)
         util_primconvert_destroy(vctx->primconvert);
            slab_destroy_child(&vctx->transfer_pool);
      }
      static void virgl_get_sample_position(struct pipe_context *ctx,
            unsigned sample_count,
      {
      struct virgl_context *vctx = virgl_context(ctx);
            if (sample_count > vs->caps.caps.v1.max_samples) {
         sample_count, vs->caps.caps.v1.max_samples);
                  /* The following is basically copied from dri/i965gen6_get_sample_position
   * The only addition is that we hold the msaa positions for all sample
   * counts in a flat array. */
   uint32_t bits = 0;
   if (sample_count == 1) {
      out_value[0] = out_value[1] = 0.5f;
      } else if (sample_count == 2) {
         } else if (sample_count <= 4) {
         } else if (sample_count <= 8) {
         } else if (sample_count <= 16) {
         }
   out_value[0] = ((bits >> 4) & 0xf) / 16.0f;
            if (virgl_debug & VIRGL_DEBUG_VERBOSE)
      debug_printf("VIRGL: sample postion [%2d/%2d] = (%f, %f)\n",
   }
      static void virgl_send_tweaks(struct virgl_context *vctx, struct virgl_screen *rs)
   {
      if (rs->tweak_gles_emulate_bgra)
            if (rs->tweak_gles_apply_bgra_dest_swizzle)
            if (rs->tweak_gles_tf3_value > 0)
      virgl_encode_tweak(vctx, virgl_tweak_gles_tf3_samples_passes_multiplier,
   }
      static void virgl_link_shader(struct pipe_context *ctx, void **handles)
   {
      struct virgl_context *vctx = virgl_context(ctx);
            uint32_t shader_handles[PIPE_SHADER_TYPES];
   for (uint32_t i = 0; i < PIPE_SHADER_TYPES; ++i)
                  /* block until shader linking is finished on host */
   if (rs->shader_sync && !unlikely(virgl_debug & VIRGL_DEBUG_SYNC)) {
      struct virgl_winsys *vws = rs->vws;
   struct pipe_fence_handle *sync_fence;
   virgl_flush_eq(vctx, vctx, &sync_fence);
   vws->fence_wait(vws, sync_fence, OS_TIMEOUT_INFINITE);
         }
      struct pipe_context *virgl_context_create(struct pipe_screen *pscreen,
               {
      struct virgl_context *vctx;
   struct virgl_screen *rs = virgl_screen(pscreen);
   vctx = CALLOC_STRUCT(virgl_context);
            vctx->cbuf = rs->vws->cmd_buf_create(rs->vws, VIRGL_MAX_CMDBUF_DWORDS);
   if (!vctx->cbuf) {
      FREE(vctx);
               vctx->base.destroy = virgl_context_destroy;
   vctx->base.create_surface = virgl_create_surface;
   vctx->base.surface_destroy = virgl_surface_destroy;
   vctx->base.set_framebuffer_state = virgl_set_framebuffer_state;
   vctx->base.create_blend_state = virgl_create_blend_state;
   vctx->base.bind_blend_state = virgl_bind_blend_state;
   vctx->base.delete_blend_state = virgl_delete_blend_state;
   vctx->base.create_depth_stencil_alpha_state = virgl_create_depth_stencil_alpha_state;
   vctx->base.bind_depth_stencil_alpha_state = virgl_bind_depth_stencil_alpha_state;
   vctx->base.delete_depth_stencil_alpha_state = virgl_delete_depth_stencil_alpha_state;
   vctx->base.create_rasterizer_state = virgl_create_rasterizer_state;
   vctx->base.bind_rasterizer_state = virgl_bind_rasterizer_state;
            vctx->base.set_viewport_states = virgl_set_viewport_states;
   vctx->base.create_vertex_elements_state = virgl_create_vertex_elements_state;
   vctx->base.bind_vertex_elements_state = virgl_bind_vertex_elements_state;
   vctx->base.delete_vertex_elements_state = virgl_delete_vertex_elements_state;
   vctx->base.set_vertex_buffers = virgl_set_vertex_buffers;
            vctx->base.set_tess_state = virgl_set_tess_state;
   vctx->base.set_patch_vertices = virgl_set_patch_vertices;
   vctx->base.create_vs_state = virgl_create_vs_state;
   vctx->base.create_tcs_state = virgl_create_tcs_state;
   vctx->base.create_tes_state = virgl_create_tes_state;
   vctx->base.create_gs_state = virgl_create_gs_state;
            vctx->base.bind_vs_state = virgl_bind_vs_state;
   vctx->base.bind_tcs_state = virgl_bind_tcs_state;
   vctx->base.bind_tes_state = virgl_bind_tes_state;
   vctx->base.bind_gs_state = virgl_bind_gs_state;
            vctx->base.delete_vs_state = virgl_delete_vs_state;
   vctx->base.delete_tcs_state = virgl_delete_tcs_state;
   vctx->base.delete_tes_state = virgl_delete_tes_state;
   vctx->base.delete_gs_state = virgl_delete_gs_state;
            vctx->base.create_compute_state = virgl_create_compute_state;
   vctx->base.bind_compute_state = virgl_bind_compute_state;
   vctx->base.delete_compute_state = virgl_delete_compute_state;
            vctx->base.clear = virgl_clear;
   vctx->base.clear_render_target = virgl_clear_render_target;
   vctx->base.clear_texture = virgl_clear_texture;
   vctx->base.draw_vbo = virgl_draw_vbo;
   vctx->base.flush = virgl_flush_from_st;
   vctx->base.screen = pscreen;
   vctx->base.create_sampler_view = virgl_create_sampler_view;
   vctx->base.sampler_view_destroy = virgl_destroy_sampler_view;
   vctx->base.set_sampler_views = virgl_set_sampler_views;
            vctx->base.create_sampler_state = virgl_create_sampler_state;
   vctx->base.delete_sampler_state = virgl_delete_sampler_state;
            vctx->base.set_polygon_stipple = virgl_set_polygon_stipple;
   vctx->base.set_scissor_states = virgl_set_scissor_states;
   vctx->base.set_sample_mask = virgl_set_sample_mask;
   vctx->base.set_min_samples = virgl_set_min_samples;
   vctx->base.set_stencil_ref = virgl_set_stencil_ref;
                              vctx->base.resource_copy_region = virgl_resource_copy_region;
   vctx->base.flush_resource = virgl_flush_resource;
   vctx->base.blit =  virgl_blit;
   vctx->base.create_fence_fd = virgl_create_fence_fd;
            vctx->base.set_shader_buffers = virgl_set_shader_buffers;
   vctx->base.set_hw_atomic_buffers = virgl_set_hw_atomic_buffers;
   vctx->base.set_shader_images = virgl_set_shader_images;
   vctx->base.memory_barrier = virgl_memory_barrier;
            vctx->base.create_video_codec = virgl_video_create_codec;
            if (rs->caps.caps.v2.host_feature_check_version >= 7)
            virgl_init_context_resource_functions(&vctx->base);
   virgl_init_query_functions(vctx);
            slab_create_child(&vctx->transfer_pool, &rs->transfer_pool);
   virgl_transfer_queue_init(&vctx->queue, vctx);
   vctx->encoded_transfers = (rs->vws->supports_encoded_transfers &&
            /* Reserve some space for transfers. */
   if (vctx->encoded_transfers)
            vctx->primconvert = util_primconvert_create(&vctx->base, rs->caps.caps.v1.prim_mask);
   vctx->uploader = u_upload_create(&vctx->base, 1024 * 1024,
         if (!vctx->uploader)
         vctx->base.stream_uploader = vctx->uploader;
            /* We use a special staging buffer as the source of copy transfers. */
   if ((rs->caps.caps.v2.capability_bits & VIRGL_CAP_COPY_TRANSFER) &&
      vctx->encoded_transfers) {
   virgl_staging_init(&vctx->staging, &vctx->base, 1024 * 1024);
               vctx->hw_sub_ctx_id = p_atomic_inc_return(&rs->sub_ctx_id);
                     if (rs->caps.caps.v2.capability_bits & VIRGL_CAP_GUEST_MAY_INIT_LOG) {
      host_debug_flagstring = getenv("VIRGL_HOST_DEBUG");
   if (host_debug_flagstring)
               if (rs->caps.caps.v2.capability_bits & VIRGL_CAP_APP_TWEAK_SUPPORT)
            /* On Android, a virgl_screen is generally created first by the HWUI
   * service, followed by the application's no-op attempt to do the same with
   * eglInitialize(). To retain the ability for apps to set their own driver
   * config procedurally right before context creation, we must check the
   * envvar again.
      #if DETECT_OS_ANDROID
      if (!rs->shader_sync) {
      uint64_t debug_options = debug_get_flags_option("VIRGL_DEBUG",
               #endif
            fail:
      virgl_context_destroy(&vctx->base);
      }
