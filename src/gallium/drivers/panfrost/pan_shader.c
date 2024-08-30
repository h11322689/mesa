   /*
   * Copyright (c) 2022 Amazon.com, Inc. or its affiliates.
   * Copyright (C) 2019-2022 Collabora, Ltd.
   * Copyright (C) 2019 Red Hat Inc.
   * Copyright (C) 2018 Alyssa Rosenzweig
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
   *   Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   *
   */
      #include "pan_shader.h"
   #include "nir/tgsi_to_nir.h"
   #include "util/u_memory.h"
   #include "nir_serialize.h"
   #include "pan_bo.h"
   #include "pan_context.h"
      static struct panfrost_uncompiled_shader *
   panfrost_alloc_shader(const nir_shader *nir)
   {
      struct panfrost_uncompiled_shader *so =
            simple_mtx_init(&so->lock, mtx_plain);
                     /* Serialize the NIR to a binary blob that we can hash for the disk
   * cache. Drop unnecessary information (like variable names) so the
   * serialized NIR is smaller, and also to let us detect more isomorphic
   * shaders when hashing, increasing cache hits.
   */
   struct blob blob;
   blob_init(&blob);
   nir_serialize(&blob, nir, true);
   _mesa_sha1_compute(blob.data, blob.size, so->nir_sha1);
               }
      static struct panfrost_compiled_shader *
   panfrost_alloc_variant(struct panfrost_uncompiled_shader *so)
   {
         }
      static void
   panfrost_shader_compile(struct panfrost_screen *screen, const nir_shader *ir,
                           {
                        /* While graphics shaders are preprocessed at CSO create time, compute
   * kernels are not preprocessed until they're cloned since the driver does
   * not get ownership of the NIR from compute CSOs. Do this preprocessing now.
   * Compute CSOs call this function during create time, so preprocessing
   * happens at CSO create time regardless.
   */
   if (gl_shader_stage_is_compute(s->info.stage))
            struct panfrost_compile_inputs inputs = {
      .debug = dbg,
               /* Lower this early so the backends don't have to worry about it */
   if (s->info.stage == MESA_SHADER_FRAGMENT) {
         } else if (s->info.stage == MESA_SHADER_VERTEX) {
               /* No IDVS for internal XFB shaders */
            if (s->info.has_transform_feedback_varyings) {
      NIR_PASS_V(s, nir_io_add_const_offset_to_base,
         NIR_PASS_V(s, nir_io_add_intrinsic_xfb_info);
                           if (s->info.stage == MESA_SHADER_FRAGMENT) {
      if (key->fs.nr_cbufs_for_fragcolor) {
      NIR_PASS_V(s, panfrost_nir_remove_fragcolor_stores,
               if (key->fs.sprite_coord_enable) {
      NIR_PASS_V(s, nir_lower_texcoord_replace_late,
                     if (key->fs.clip_plane_enable) {
                     if (dev->arch <= 5 && s->info.stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS_V(s, pan_lower_framebuffer, key->fs.rt_formats,
                                       assert(req_local_mem >= out->info.wls_size);
            /* In both clone and tgsi_to_nir paths, the shader is ralloc'd against
   * a NULL context
   */
      }
      static void
   panfrost_shader_get(struct pipe_screen *pscreen,
                     struct panfrost_pool *shader_pool,
   struct panfrost_pool *desc_pool,
   struct panfrost_uncompiled_shader *uncompiled,
   {
      struct panfrost_screen *screen = pan_screen(pscreen);
                     /* Try to retrieve the variant from the disk cache. If that fails,
   * compile a new variant and store in the disk cache for later reuse.
   */
   if (!panfrost_disk_cache_retrieve(screen->disk_cache, uncompiled,
            panfrost_shader_compile(screen, uncompiled->nir, dbg, &state->key,
                  panfrost_disk_cache_store(screen->disk_cache, uncompiled, &state->key,
               state->info = res.info;
            if (res.binary.size) {
      state->bin = panfrost_pool_take_ref(
      shader_pool,
   pan_pool_upload_aligned(&shader_pool->base, res.binary.data,
                     /* Don't upload RSD for fragment shaders since they need draw-time
   * merging for e.g. depth/stencil/alpha. RSDs are replaced by simpler
   * shader program descriptors on Valhall, which can be preuploaded even
   * for fragment shaders. */
   bool upload =
                     }
      static void
   panfrost_build_key(struct panfrost_context *ctx,
               {
               /* We don't currently have vertex shader variants */
   if (nir->info.stage != MESA_SHADER_FRAGMENT)
            struct panfrost_device *dev = pan_device(ctx->base.screen);
   struct pipe_framebuffer_state *fb = &ctx->pipe_framebuffer;
   struct pipe_rasterizer_state *rast = (void *)ctx->rasterizer;
            /* gl_FragColor lowering needs the number of colour buffers */
   if (uncompiled->fragcolor_lowered) {
                  /* Point sprite lowering needed on Bifrost and newer */
   if (dev->arch >= 6 && rast && ctx->active_prim == MESA_PRIM_POINTS) {
                  /* User clip plane lowering needed everywhere */
   if (rast) {
                  if (dev->arch <= 5) {
      u_foreach_bit(i, (nir->info.outputs_read >> FRAG_RESULT_DATA0)) {
                                                            /* Funny desktop GL varying lowering on Valhall */
   if (dev->arch >= 9) {
      assert(vs != NULL && "too early");
         }
      static struct panfrost_compiled_shader *
   panfrost_new_variant_locked(struct panfrost_context *ctx,
               {
               *prog = (struct panfrost_compiled_shader){
      .key = *key,
               panfrost_shader_get(ctx->base.screen, &ctx->shaders, &ctx->descs, uncompiled,
                        }
      static void
   panfrost_bind_shader_state(struct pipe_context *pctx, void *hwcso,
         {
      struct panfrost_context *ctx = pan_context(pctx);
   ctx->uncompiled[type] = hwcso;
            ctx->dirty |= PAN_DIRTY_TLS_SIZE;
            if (hwcso)
      }
      void
   panfrost_update_shader_variant(struct panfrost_context *ctx,
         {
      /* No shader variants for compute */
   if (type == PIPE_SHADER_COMPUTE)
            /* We need linking information, defer this */
   if (type == PIPE_SHADER_FRAGMENT && !ctx->uncompiled[PIPE_SHADER_VERTEX])
            /* Also defer, happens with GALLIUM_HUD */
   if (!ctx->uncompiled[type])
            /* Match the appropriate variant */
   struct panfrost_uncompiled_shader *uncompiled = ctx->uncompiled[type];
                     struct panfrost_shader_key key = {0};
            util_dynarray_foreach(&uncompiled->variants, struct panfrost_compiled_shader,
            if (memcmp(&key, &so->key, sizeof(key)) == 0) {
      compiled = so;
                  if (compiled == NULL)
                        }
      static void
   panfrost_bind_vs_state(struct pipe_context *pctx, void *hwcso)
   {
               /* Fragment shaders are linked with vertex shaders */
   struct panfrost_context *ctx = pan_context(pctx);
      }
      static void
   panfrost_bind_fs_state(struct pipe_context *pctx, void *hwcso)
   {
         }
      static void *
   panfrost_create_shader_state(struct pipe_context *pctx,
         {
      nir_shader *nir = (cso->type == PIPE_SHADER_IR_TGSI)
                           /* The driver gets ownership of the nir_shader for graphics. The NIR is
   * ralloc'd. Free the NIR when we free the uncompiled shader.
   */
            so->stream_output = cso->stream_output;
            /* Fix linkage early */
   if (so->nir->info.stage == MESA_SHADER_VERTEX) {
      so->fixed_varying_mask =
      (so->nir->info.outputs_written & BITFIELD_MASK(VARYING_SLOT_VAR0)) &
            /* gl_FragColor needs to be lowered before lowering I/O, do that now */
   if (nir->info.stage == MESA_SHADER_FRAGMENT &&
               NIR_PASS_V(nir, nir_lower_fragcolor,
                     /* Then run the suite of lowering and optimization, including I/O lowering */
   struct panfrost_device *dev = pan_device(pctx->screen);
            /* If this shader uses transform feedback, compile the transform
   * feedback program. This is a special shader variant.
   */
            if (so->nir->xfb_info) {
      nir_shader *xfb = nir_shader_clone(NULL, so->nir);
   xfb->info.name = ralloc_asprintf(xfb, "%s@xfb", xfb->info.name);
            so->xfb = calloc(1, sizeof(struct panfrost_compiled_shader));
            panfrost_shader_get(ctx->base.screen, &ctx->shaders, &ctx->descs, so,
            /* Since transform feedback is handled via the transform
   * feedback program, the original program no longer uses XFB
   */
               /* Compile the program. We don't use vertex shader keys, so there will
   * be no further vertex shader variants. We do have fragment shader
   * keys, but we can still compile with a default key that will work most
   * of the time.
   */
            /* gl_FragColor lowering needs the number of colour buffers on desktop
   * GL, where it acts as an implicit broadcast to all colour buffers.
   *
   * However, gl_FragColor is a legacy feature, so assume that if
   * gl_FragColor is used, there is only a single render target. The
   * implicit broadcast is neither especially useful nor required by GLES.
   */
   if (so->fragcolor_lowered)
            /* Creating a CSO is single-threaded, so it's ok to use the
   * locked function without explicitly taking the lock. Creating a
   * default variant acts as a precompile.
   */
               }
      static void
   panfrost_delete_shader_state(struct pipe_context *pctx, void *so)
   {
      struct panfrost_uncompiled_shader *cso =
            util_dynarray_foreach(&cso->variants, struct panfrost_compiled_shader, so) {
      panfrost_bo_unreference(so->bin.bo);
   panfrost_bo_unreference(so->state.bo);
               if (cso->xfb) {
      panfrost_bo_unreference(cso->xfb->bin.bo);
   panfrost_bo_unreference(cso->xfb->state.bo);
   panfrost_bo_unreference(cso->xfb->linkage.bo);
                           }
      /*
   * Create a compute CSO. As compute kernels do not require variants, they are
   * precompiled, creating both the uncompiled and compiled shaders now.
   */
   static void *
   panfrost_create_compute_state(struct pipe_context *pctx,
         {
      struct panfrost_context *ctx = pan_context(pctx);
   struct panfrost_uncompiled_shader *so = panfrost_alloc_shader(cso->prog);
   struct panfrost_compiled_shader *v = panfrost_alloc_variant(so);
                     panfrost_shader_get(pctx->screen, &ctx->shaders, &ctx->descs, so,
            /* The NIR becomes invalid after this. For compute kernels, we never
   * need to access it again. Don't keep a dangling pointer around.
   */
   ralloc_free((void *)so->nir);
               }
      static void
   panfrost_bind_compute_state(struct pipe_context *pipe, void *cso)
   {
      struct panfrost_context *ctx = pan_context(pipe);
                     ctx->prog[PIPE_SHADER_COMPUTE] =
      }
      static void
   panfrost_get_compute_state_info(struct pipe_context *pipe, void *cso,
         {
      struct panfrost_device *dev = pan_device(pipe->screen);
   struct panfrost_uncompiled_shader *uncompiled = cso;
   struct panfrost_compiled_shader *cs =
            info->max_threads =
         info->private_memory = cs->info.tls_size;
   info->simd_sizes = pan_subgroup_size(dev->arch);
      }
      void
   panfrost_shader_context_init(struct pipe_context *pctx)
   {
      pctx->create_vs_state = panfrost_create_shader_state;
   pctx->delete_vs_state = panfrost_delete_shader_state;
            pctx->create_fs_state = panfrost_create_shader_state;
   pctx->delete_fs_state = panfrost_delete_shader_state;
            pctx->create_compute_state = panfrost_create_compute_state;
   pctx->bind_compute_state = panfrost_bind_compute_state;
   pctx->get_compute_state_info = panfrost_get_compute_state_info;
      }
