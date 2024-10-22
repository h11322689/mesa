   /*
   * Copyright 2021 Alyssa Rosenzweig
   * Copyright 2020-2021 Collabora, Ltd.
   * Copyright 2014 Broadcom
   * SPDX-License-Identifier: MIT
   */
      #include "asahi/compiler/agx_compile.h"
   #include "compiler/nir/nir_builder.h"
   #include "gallium/auxiliary/util/u_blitter.h"
   #include "gallium/auxiliary/util/u_dump.h"
   #include "agx_state.h"
      void
   agx_blitter_save(struct agx_context *ctx, struct blitter_context *blitter,
         {
      util_blitter_save_vertex_buffer_slot(blitter, ctx->vertex_buffers);
   util_blitter_save_vertex_elements(blitter, ctx->attributes);
   util_blitter_save_vertex_shader(blitter,
         util_blitter_save_rasterizer(blitter, ctx->rast);
   util_blitter_save_viewport(blitter, &ctx->viewport);
   util_blitter_save_scissor(blitter, &ctx->scissor);
   util_blitter_save_fragment_shader(blitter,
         util_blitter_save_blend(blitter, ctx->blend);
   util_blitter_save_depth_stencil_alpha(blitter, ctx->zs);
   util_blitter_save_stencil_ref(blitter, &ctx->stencil_ref);
   util_blitter_save_so_targets(blitter, ctx->streamout.num_targets,
                  util_blitter_save_framebuffer(blitter, &ctx->framebuffer);
   util_blitter_save_fragment_sampler_states(
      blitter, ctx->stage[PIPE_SHADER_FRAGMENT].sampler_count,
      util_blitter_save_fragment_sampler_views(
      blitter, ctx->stage[PIPE_SHADER_FRAGMENT].texture_count,
      util_blitter_save_fragment_constant_buffer_slot(
            if (!render_cond) {
      util_blitter_save_render_condition(blitter,
               }
      void
   agx_blit(struct pipe_context *pipe, const struct pipe_blit_info *info)
   {
               if (info->render_condition_enable && !agx_render_condition_check(ctx))
            if (!util_blitter_is_blit_supported(ctx->blitter, info)) {
      fprintf(stderr, "\n");
   util_dump_blit_info(stderr, info);
   fprintf(stderr, "\n\n");
               /* Legalize compression /before/ calling into u_blitter to avoid recursion.
   * u_blitter bans recursive usage.
   */
   agx_legalize_compression(ctx, agx_resource(info->dst.resource),
            agx_legalize_compression(ctx, agx_resource(info->src.resource),
            agx_blitter_save(ctx, ctx->blitter, info->render_condition_enable);
      }
