   /*
   * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
   * Copyright 2018 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_pipe.h"
      #include "driver_ddebug/dd_util.h"
   #include "radeon_uvd.h"
   #include "si_public.h"
   #include "si_shader_internal.h"
   #include "sid.h"
   #include "ac_shadowed_regs.h"
   #include "compiler/nir/nir.h"
   #include "util/disk_cache.h"
   #include "util/hex.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_log.h"
   #include "util/u_memory.h"
   #include "util/u_suballoc.h"
   #include "util/u_tests.h"
   #include "util/u_upload_mgr.h"
   #include "util/xmlconfig.h"
   #include "vl/vl_decoder.h"
   #include "si_utrace.h"
      #include <xf86drm.h>
      static struct pipe_context *si_create_context(struct pipe_screen *screen, unsigned flags);
      static const struct debug_named_value radeonsi_debug_options[] = {
      /* Shader logging options: */
   {"vs", DBG(VS), "Print vertex shaders"},
   {"ps", DBG(PS), "Print pixel shaders"},
   {"gs", DBG(GS), "Print geometry shaders"},
   {"tcs", DBG(TCS), "Print tessellation control shaders"},
   {"tes", DBG(TES), "Print tessellation evaluation shaders"},
            {"initnir", DBG(INIT_NIR), "Print initial input NIR when shaders are created"},
   {"nir", DBG(NIR), "Print final NIR after lowering when shader variants are created"},
   {"initllvm", DBG(INIT_LLVM), "Print initial LLVM IR before optimizations"},
   {"llvm", DBG(LLVM), "Print final LLVM IR"},
   {"initaco", DBG(INIT_ACO), "Print initial ACO IR before optimizations"},
   {"aco", DBG(ACO), "Print final ACO IR"},
   {"asm", DBG(ASM), "Print final shaders in asm"},
            /* Shader compiler options the shader cache should be aware of: */
   {"w32ge", DBG(W32_GE), "Use Wave32 for vertex, tessellation, and geometry shaders."},
   {"w32ps", DBG(W32_PS), "Use Wave32 for pixel shaders."},
   {"w32psdiscard", DBG(W32_PS_DISCARD), "Use Wave32 for pixel shaders even if they contain discard and LLVM is buggy."},
   {"w32cs", DBG(W32_CS), "Use Wave32 for computes shaders."},
   {"w64ge", DBG(W64_GE), "Use Wave64 for vertex, tessellation, and geometry shaders."},
   {"w64ps", DBG(W64_PS), "Use Wave64 for pixel shaders."},
            /* Shader compiler options (with no effect on the shader cache): */
   {"checkir", DBG(CHECK_IR), "Enable additional sanity checks on shader IR"},
   {"mono", DBG(MONOLITHIC_SHADERS), "Use old-style monolithic shaders compiled on demand"},
   {"nooptvariant", DBG(NO_OPT_VARIANT), "Disable compiling optimized shader variants."},
            /* Information logging options: */
   {"info", DBG(INFO), "Print driver information"},
   {"tex", DBG(TEX), "Print texture info"},
   {"compute", DBG(COMPUTE), "Print compute info"},
   {"vm", DBG(VM), "Print virtual addresses when creating resources"},
   {"cache_stats", DBG(CACHE_STATS), "Print shader cache statistics."},
   {"ib", DBG(IB), "Print command buffers."},
            /* Driver options: */
   {"nowc", DBG(NO_WC), "Disable GTT write combining"},
   {"nowcstream", DBG(NO_WC_STREAM), "Disable GTT write combining for streaming uploads"},
   {"check_vm", DBG(CHECK_VM), "Check VM faults and dump debug info."},
   {"reserve_vmid", DBG(RESERVE_VMID), "Force VMID reservation per context."},
   {"shadowregs", DBG(SHADOW_REGS), "Enable CP register shadowing."},
   {"nofastdlist", DBG(NO_FAST_DISPLAY_LIST), "Disable fast display lists"},
            /* Multimedia options: */
            /* 3D engine options: */
   {"nogfx", DBG(NO_GFX), "Disable graphics. Only multimedia compute paths can be used."},
   {"nongg", DBG(NO_NGG), "Disable NGG and use the legacy pipeline."},
   {"nggc", DBG(ALWAYS_NGG_CULLING_ALL), "Always use NGG culling even when it can hurt."},
   {"nonggc", DBG(NO_NGG_CULLING), "Disable NGG culling."},
   {"switch_on_eop", DBG(SWITCH_ON_EOP), "Program WD/IA to switch on end-of-packet."},
   {"nooutoforder", DBG(NO_OUT_OF_ORDER), "Disable out-of-order rasterization"},
   {"nodpbb", DBG(NO_DPBB), "Disable DPBB. Overrules the dpbb enable option."},
   {"dpbb", DBG(DPBB), "Enable DPBB for gfx9 dGPU. Default enabled for gfx9 APU and >= gfx10."},
   {"nohyperz", DBG(NO_HYPERZ), "Disable Hyper-Z"},
   {"no2d", DBG(NO_2D_TILING), "Disable 2D tiling"},
   {"notiling", DBG(NO_TILING), "Disable tiling"},
   {"nodisplaytiling", DBG(NO_DISPLAY_TILING), "Disable display tiling"},
   {"nodisplaydcc", DBG(NO_DISPLAY_DCC), "Disable display DCC"},
   {"noexporteddcc", DBG(NO_EXPORTED_DCC), "Disable DCC for all exported buffers (via DMABUF, etc.)"},
   {"nodcc", DBG(NO_DCC), "Disable DCC."},
   {"nodccclear", DBG(NO_DCC_CLEAR), "Disable DCC fast clear."},
   {"nodccstore", DBG(NO_DCC_STORE), "Disable DCC stores"},
   {"dccstore", DBG(DCC_STORE), "Enable DCC stores"},
   {"nodccmsaa", DBG(NO_DCC_MSAA), "Disable DCC for MSAA"},
   {"nofmask", DBG(NO_FMASK), "Disable MSAA compression"},
                     {"tmz", DBG(TMZ), "Force allocation of scanout/depth/stencil buffer as encrypted"},
               };
      static const struct debug_named_value test_options[] = {
      /* Tests: */
   {"imagecopy", DBG(TEST_IMAGE_COPY), "Invoke resource_copy_region tests with images and exit."},
   {"cbresolve", DBG(TEST_CB_RESOLVE), "Invoke MSAA resolve tests and exit."},
   {"computeblit", DBG(TEST_COMPUTE_BLIT), "Invoke blits tests and exit."},
   {"testvmfaultcp", DBG(TEST_VMFAULT_CP), "Invoke a CP VM fault test and exit."},
   {"testvmfaultshader", DBG(TEST_VMFAULT_SHADER), "Invoke a shader VM fault test and exit."},
   {"testdmaperf", DBG(TEST_DMA_PERF), "Test DMA performance"},
   {"testgds", DBG(TEST_GDS), "Test GDS."},
   {"testgdsmm", DBG(TEST_GDS_MM), "Test GDS memory management."},
               };
      bool si_init_compiler(struct si_screen *sscreen, struct ac_llvm_compiler *compiler)
   {
      /* Only create the less-optimizing version of the compiler on APUs
   * predating Ryzen (Raven). */
   bool create_low_opt_compiler =
            enum ac_target_machine_options tm_options =
      (sscreen->debug_flags & DBG(CHECK_IR) ? AC_TM_CHECK_IR : 0) |
         if (!ac_init_llvm_compiler(compiler, sscreen->info.family, tm_options))
            compiler->passes = ac_create_llvm_passes(compiler->tm);
   if (compiler->low_opt_tm)
               }
      void si_init_aux_async_compute_ctx(struct si_screen *sscreen)
   {
      assert(!sscreen->async_compute_context);
   sscreen->async_compute_context =
      si_create_context(&sscreen->b,
                           /* Limit the numbers of waves allocated for this context. */
   if (sscreen->async_compute_context)
      }
      static void si_destroy_compiler(struct ac_llvm_compiler *compiler)
   {
         }
         static void decref_implicit_resource(struct hash_entry *entry)
   {
         }
      /*
   * pipe_context
   */
   static void si_destroy_context(struct pipe_context *context)
   {
               /* Unreference the framebuffer normally to disable related logic
   * properly.
   */
   struct pipe_framebuffer_state fb = {};
   if (context->set_framebuffer_state)
                     if (sctx->gfx_level >= GFX10 && sctx->has_graphics)
            if (sctx->sqtt) {
      struct si_screen *sscreen = sctx->screen;
   if (sscreen->info.has_stable_pstate && sscreen->b.num_contexts == 1 &&
                                       pipe_resource_reference(&sctx->esgs_ring, NULL);
   pipe_resource_reference(&sctx->gsvs_ring, NULL);
   pipe_resource_reference(&sctx->tess_rings, NULL);
   pipe_resource_reference(&sctx->tess_rings_tmz, NULL);
   pipe_resource_reference(&sctx->null_const_buf.buffer, NULL);
   pipe_resource_reference(&sctx->sample_pos_buffer, NULL);
   si_resource_reference(&sctx->border_color_buffer, NULL);
   free(sctx->border_color_table);
   si_resource_reference(&sctx->scratch_buffer, NULL);
   si_resource_reference(&sctx->compute_scratch_buffer, NULL);
   si_resource_reference(&sctx->wait_mem_scratch, NULL);
   si_resource_reference(&sctx->wait_mem_scratch_tmz, NULL);
   si_resource_reference(&sctx->small_prim_cull_info_buf, NULL);
   si_resource_reference(&sctx->pipeline_stats_query_buf, NULL);
            if (sctx->cs_preamble_state)
         if (sctx->cs_preamble_state_tmz)
            if (sctx->fixed_func_tcs_shader_cache) {
      hash_table_foreach(sctx->fixed_func_tcs_shader_cache, entry) {
         }
               if (sctx->custom_dsa_flush)
         if (sctx->custom_blend_resolve)
         if (sctx->custom_blend_fmask_decompress)
         if (sctx->custom_blend_eliminate_fastclear)
         if (sctx->custom_blend_dcc_decompress)
         if (sctx->vs_blit_pos)
         if (sctx->vs_blit_pos_layered)
         if (sctx->vs_blit_color)
         if (sctx->vs_blit_color_layered)
         if (sctx->vs_blit_texcoord)
         if (sctx->cs_clear_buffer)
         if (sctx->cs_clear_buffer_rmw)
         if (sctx->cs_copy_buffer)
         for (unsigned i = 0; i < ARRAY_SIZE(sctx->cs_copy_image); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(sctx->cs_copy_image[i]); j++) {
      for (unsigned k = 0; k < ARRAY_SIZE(sctx->cs_copy_image[i][j]); k++) {
      if (sctx->cs_copy_image[i][j][k])
            }
   if (sctx->cs_clear_render_target)
         if (sctx->cs_clear_render_target_1d_array)
         if (sctx->cs_clear_12bytes_buffer)
         for (unsigned i = 0; i < ARRAY_SIZE(sctx->cs_dcc_retile); i++) {
      if (sctx->cs_dcc_retile[i])
      }
   if (sctx->no_velems_state)
            for (unsigned i = 0; i < ARRAY_SIZE(sctx->cs_fmask_expand); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(sctx->cs_fmask_expand[i]); j++) {
      if (sctx->cs_fmask_expand[i][j]) {
                        for (unsigned i = 0; i < ARRAY_SIZE(sctx->cs_clear_dcc_msaa); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(sctx->cs_clear_dcc_msaa[i]); j++) {
      for (unsigned k = 0; k < ARRAY_SIZE(sctx->cs_clear_dcc_msaa[i][j]); k++) {
      for (unsigned l = 0; l < ARRAY_SIZE(sctx->cs_clear_dcc_msaa[i][j][k]); l++) {
      for (unsigned m = 0; m < ARRAY_SIZE(sctx->cs_clear_dcc_msaa[i][j][k][l]); m++) {
      if (sctx->cs_clear_dcc_msaa[i][j][k][l][m])
                           if (sctx->blitter)
            if (sctx->query_result_shader)
         if (sctx->sh_query_result_shader)
            sctx->ws->cs_destroy(&sctx->gfx_cs);
   if (sctx->ctx)
         if (sctx->sdma_cs) {
      sctx->ws->cs_destroy(sctx->sdma_cs);
               if (sctx->dirty_implicit_resources)
      _mesa_hash_table_destroy(sctx->dirty_implicit_resources,
         if (sctx->b.stream_uploader)
         if (sctx->b.const_uploader && sctx->b.const_uploader != sctx->b.stream_uploader)
         if (sctx->cached_gtt_allocator)
            slab_destroy_child(&sctx->pool_transfers);
                     sctx->ws->fence_reference(&sctx->last_gfx_fence, NULL);
   si_resource_reference(&sctx->eop_bug_scratch, NULL);
   si_resource_reference(&sctx->eop_bug_scratch_tmz, NULL);
   si_resource_reference(&sctx->shadowing.registers, NULL);
            if (sctx->compiler) {
      si_destroy_compiler(sctx->compiler);
                        _mesa_hash_table_destroy(sctx->tex_handles, NULL);
            util_dynarray_fini(&sctx->resident_tex_handles);
   util_dynarray_fini(&sctx->resident_img_handles);
   util_dynarray_fini(&sctx->resident_tex_needs_color_decompress);
   util_dynarray_fini(&sctx->resident_img_needs_color_decompress);
            if (!(sctx->context_flags & SI_CONTEXT_FLAG_AUX))
            if (sctx->cs_blit_shaders) {
      hash_table_foreach(sctx->cs_blit_shaders, entry) {
         }
                  }
      static enum pipe_reset_status si_get_reset_status(struct pipe_context *ctx)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   if (sctx->context_flags & SI_CONTEXT_FLAG_AUX)
            bool needs_reset, reset_completed;
   enum pipe_reset_status status = sctx->ws->ctx_query_reset_status(sctx->ctx, false,
            if (status != PIPE_NO_RESET) {
      if (sctx->has_reset_been_notified && reset_completed)
                     if (!(sctx->context_flags & SI_CONTEXT_FLAG_AUX)) {
      /* Call the gallium frontend to set a no-op API dispatch. */
   if (needs_reset && sctx->device_reset_callback.reset)
         }
      }
      static void si_set_device_reset_callback(struct pipe_context *ctx,
         {
               if (cb)
         else
      }
      /* Apitrace profiling:
   *   1) qapitrace : Tools -> Profile: Measure CPU & GPU times
   *   2) In the middle panel, zoom in (mouse wheel) on some bad draw call
   *      and remember its number.
   *   3) In Mesa, enable queries and performance counters around that draw
   *      call and print the results.
   *   4) glretrace --benchmark --markers ..
   */
   static void si_emit_string_marker(struct pipe_context *ctx, const char *string, int len)
   {
                        if (sctx->sqtt_enabled)
            if (sctx->log)
      }
      static void si_set_debug_callback(struct pipe_context *ctx, const struct util_debug_callback *cb)
   {
      struct si_context *sctx = (struct si_context *)ctx;
            util_queue_finish(&screen->shader_compiler_queue);
            if (cb)
         else
      }
      static void si_set_log_context(struct pipe_context *ctx, struct u_log_context *log)
   {
      struct si_context *sctx = (struct si_context *)ctx;
            if (log)
      }
      static void si_set_context_param(struct pipe_context *ctx, enum pipe_context_param param,
         {
               switch (param) {
   case PIPE_CONTEXT_PARAM_PIN_THREADS_TO_L3_CACHE:
      ws->pin_threads_to_L3_cache(ws, value);
      default:;
      }
      static void si_set_frontend_noop(struct pipe_context *ctx, bool enable)
   {
               ctx->flush(ctx, NULL, PIPE_FLUSH_ASYNC);
      }
      static struct pipe_context *si_create_context(struct pipe_screen *screen, unsigned flags)
   {
      struct si_screen *sscreen = (struct si_screen *)screen;
            /* Don't create a context if it's not compute-only and hw is compute-only. */
   if (!sscreen->info.has_graphics && !(flags & PIPE_CONTEXT_COMPUTE_ONLY)) {
      fprintf(stderr, "radeonsi: can't create a graphics context on a compute chip\n");
               struct si_context *sctx = CALLOC_STRUCT(si_context);
   struct radeon_winsys *ws = sscreen->ws;
   int shader, i;
            if (!sctx) {
      fprintf(stderr, "radeonsi: can't allocate a context\n");
                        if (flags & PIPE_CONTEXT_DEBUG)
            sctx->b.screen = screen; /* this must be set first */
   sctx->b.priv = NULL;
   sctx->b.destroy = si_destroy_context;
   sctx->screen = sscreen; /* Easy accessing of screen/winsys. */
   sctx->is_debug = (flags & PIPE_CONTEXT_DEBUG) != 0;
            slab_create_child(&sctx->pool_transfers, &sscreen->pool_transfers);
            sctx->ws = sscreen->ws;
   sctx->family = sscreen->info.family;
   sctx->gfx_level = sscreen->info.gfx_level;
            if (sctx->gfx_level == GFX7 || sctx->gfx_level == GFX8 || sctx->gfx_level == GFX9) {
      sctx->eop_bug_scratch = si_aligned_buffer_create(
      &sscreen->b, PIPE_RESOURCE_FLAG_UNMAPPABLE | SI_RESOURCE_FLAG_DRIVER_INTERNAL,
      if (!sctx->eop_bug_scratch) {
      fprintf(stderr, "radeonsi: can't create eop_bug_scratch\n");
                  if (flags & PIPE_CONTEXT_HIGH_PRIORITY) {
         } else if (flags & PIPE_CONTEXT_LOW_PRIORITY) {
         } else {
                           /* Initialize the context handle and the command stream. */
   sctx->ctx = sctx->ws->ctx_create(sctx->ws, priority, allow_context_lost);
   if (!sctx->ctx && priority != RADEON_CTX_PRIORITY_MEDIUM) {
      /* Context priority should be treated as a hint. If context creation
   * fails with the requested priority, for example because the caller
   * lacks CAP_SYS_NICE capability or other system resource constraints,
   * fallback to normal priority.
   */
   priority = RADEON_CTX_PRIORITY_MEDIUM;
      }
   if (!sctx->ctx) {
      fprintf(stderr, "radeonsi: can't create radeon_winsys_ctx\n");
               ws->cs_create(&sctx->gfx_cs, sctx->ctx, sctx->has_graphics ? AMD_IP_GFX : AMD_IP_COMPUTE,
            /* Initialize private allocators. */
   u_suballocator_init(&sctx->allocator_zeroed_memory, &sctx->b, 128 * 1024, 0,
                  sctx->cached_gtt_allocator = u_upload_create(&sctx->b, 16 * 1024, 0, PIPE_USAGE_STAGING, 0);
   if (!sctx->cached_gtt_allocator) {
      fprintf(stderr, "radeonsi: can't create cached_gtt_allocator\n");
               /* Initialize public allocators. Unify uploaders as follows:
   * - dGPUs: The const uploader writes to VRAM and the stream uploader writes to RAM.
   * - APUs: There is only one uploader instance writing to RAM. VRAM has the same perf on APUs.
   */
   bool is_apu = !sscreen->info.has_dedicated_vram;
   sctx->b.stream_uploader =
      u_upload_create(&sctx->b, 1024 * 1024, 0,
                  if (!sctx->b.stream_uploader) {
      fprintf(stderr, "radeonsi: can't create stream_uploader\n");
               if (is_apu) {
         } else {
      sctx->b.const_uploader =
      u_upload_create(&sctx->b, 256 * 1024, 0, PIPE_USAGE_DEFAULT,
      if (!sctx->b.const_uploader) {
      fprintf(stderr, "radeonsi: can't create const_uploader\n");
                  /* Border colors. */
   if (sscreen->info.has_3d_cube_border_color_mipmap) {
      sctx->border_color_table = malloc(SI_MAX_BORDER_COLORS * sizeof(*sctx->border_color_table));
   if (!sctx->border_color_table) {
      fprintf(stderr, "radeonsi: can't create border_color_table\n");
               sctx->border_color_buffer = si_resource(pipe_buffer_create(
         if (!sctx->border_color_buffer) {
      fprintf(stderr, "radeonsi: can't create border_color_buffer\n");
               sctx->border_color_map =
         if (!sctx->border_color_map) {
      fprintf(stderr, "radeonsi: can't map border_color_buffer\n");
                  sctx->ngg = sscreen->use_ngg;
            /* Initialize context functions used by graphics and compute. */
   if (sctx->gfx_level >= GFX10)
         else
            sctx->b.emit_string_marker = si_emit_string_marker;
   sctx->b.set_debug_callback = si_set_debug_callback;
   sctx->b.set_log_context = si_set_log_context;
   sctx->b.set_context_param = si_set_context_param;
   sctx->b.get_device_reset_status = si_get_reset_status;
   sctx->b.set_device_reset_callback = si_set_device_reset_callback;
            si_init_all_descriptors(sctx);
   si_init_buffer_functions(sctx);
   si_init_clear_functions(sctx);
   si_init_blit_functions(sctx);
   si_init_compute_functions(sctx);
   si_init_compute_blit_functions(sctx);
   si_init_debug_functions(sctx);
   si_init_fence_functions(sctx);
   si_init_query_functions(sctx);
   si_init_state_compute_functions(sctx);
            /* Initialize graphics-only context functions. */
   if (sctx->has_graphics) {
      if (sctx->gfx_level >= GFX10)
         si_init_msaa_functions(sctx);
   si_init_shader_functions(sctx);
   si_init_state_functions(sctx);
   si_init_streamout_functions(sctx);
            sctx->blitter = util_blitter_create(&sctx->b);
   if (sctx->blitter == NULL) {
      fprintf(stderr, "radeonsi: can't create blitter\n");
      }
            /* Some states are expected to be always non-NULL. */
   sctx->noop_blend = util_blitter_get_noop_blend_state(sctx->blitter);
            sctx->noop_dsa = util_blitter_get_noop_dsa_state(sctx->blitter);
            sctx->no_velems_state = sctx->b.create_vertex_elements_state(&sctx->b, 0, NULL);
            sctx->discard_rasterizer_state = util_blitter_get_discard_rasterizer_state(sctx->blitter);
            switch (sctx->gfx_level) {
   case GFX6:
      si_init_draw_functions_GFX6(sctx);
      case GFX7:
      si_init_draw_functions_GFX7(sctx);
      case GFX8:
      si_init_draw_functions_GFX8(sctx);
      case GFX9:
      si_init_draw_functions_GFX9(sctx);
      case GFX10:
      si_init_draw_functions_GFX10(sctx);
      case GFX10_3:
      si_init_draw_functions_GFX10_3(sctx);
      case GFX11:
      si_init_draw_functions_GFX11(sctx);
      case GFX11_5:
      si_init_draw_functions_GFX11_5(sctx);
      default:
                              /* Initialize multimedia functions. */
   if (sscreen->info.ip[AMD_IP_UVD].num_queues ||
      sscreen->info.ip[AMD_IP_VCN_UNIFIED].num_queues : sscreen->info.ip[AMD_IP_VCN_DEC].num_queues) ||
         sscreen->info.ip[AMD_IP_VCN_JPEG].num_queues || sscreen->info.ip[AMD_IP_VCE].num_queues ||
   sscreen->info.ip[AMD_IP_UVD_ENC].num_queues || sscreen->info.ip[AMD_IP_VCN_ENC].num_queues) {
   sctx->b.create_video_codec = si_uvd_create_decoder;
   sctx->b.create_video_buffer = si_video_buffer_create;
   if (screen->resource_create_with_modifiers)
      } else {
      sctx->b.create_video_codec = vl_create_decoder;
               /* GFX7 cannot unbind a constant buffer (S_BUFFER_LOAD doesn't skip loads
   * if NUM_RECORDS == 0). We need to use a dummy buffer instead. */
   if (sctx->gfx_level == GFX7) {
      sctx->null_const_buf.buffer =
      pipe_aligned_buffer_create(screen,
                        if (!sctx->null_const_buf.buffer) {
      fprintf(stderr, "radeonsi: can't create null_const_buf\n");
      }
            unsigned start_shader = sctx->has_graphics ? 0 : PIPE_SHADER_COMPUTE;
   for (shader = start_shader; shader < SI_NUM_SHADERS; shader++) {
      for (i = 0; i < SI_NUM_CONST_BUFFERS; i++) {
                     si_set_internal_const_buffer(sctx, SI_HS_CONST_DEFAULT_TESS_LEVELS, &sctx->null_const_buf);
   si_set_internal_const_buffer(sctx, SI_VS_CONST_INSTANCE_DIVISORS, &sctx->null_const_buf);
   si_set_internal_const_buffer(sctx, SI_VS_CONST_CLIP_PLANES, &sctx->null_const_buf);
   si_set_internal_const_buffer(sctx, SI_PS_CONST_POLY_STIPPLE, &sctx->null_const_buf);
               /* Bindless handles. */
   sctx->tex_handles = _mesa_hash_table_create(NULL, _mesa_hash_pointer, _mesa_key_pointer_equal);
            util_dynarray_init(&sctx->resident_tex_handles, NULL);
   util_dynarray_init(&sctx->resident_img_handles, NULL);
   util_dynarray_init(&sctx->resident_tex_needs_color_decompress, NULL);
   util_dynarray_init(&sctx->resident_img_needs_color_decompress, NULL);
            sctx->dirty_implicit_resources = _mesa_pointer_hash_table_create(NULL);
   if (!sctx->dirty_implicit_resources) {
      fprintf(stderr, "radeonsi: can't create dirty_implicit_resources\n");
               /* The remainder of this function initializes the gfx CS and must be last. */
                     /* Set immutable fields of shader keys. */
   if (sctx->gfx_level >= GFX9) {
      /* The LS output / HS input layout can be communicated
   * directly instead of via user SGPRs for merged LS-HS.
   * This also enables jumping over the VS prolog for HS-only waves.
   *
   * When the LS VGPR fix is needed, monolithic shaders can:
   *  - avoid initializing EXEC in both the LS prolog
   *    and the LS main part when !vs_needs_prolog
   *  - remove the fixup for unused input VGPRs
   */
            /* This enables jumping over the VS prolog for GS-only waves. */
                        si_begin_new_gfx_cs(sctx, true);
            if (sctx->gfx_level >= GFX9 && sctx->gfx_level < GFX11) {
      sctx->wait_mem_scratch =
      si_aligned_buffer_create(screen,
                        if (!sctx->wait_mem_scratch) {
      fprintf(stderr, "radeonsi: can't create wait_mem_scratch\n");
               si_cp_write_data(sctx, sctx->wait_mem_scratch, 0, 4, V_370_MEM, V_370_ME,
               if (sctx->gfx_level == GFX7) {
      /* Clear the NULL constant buffer, because loads should return zeros.
   * Note that this forces CP DMA to be used, because clover deadlocks
   * for some reason when the compute codepath is used.
   */
   uint32_t clear_value = 0;
   si_clear_buffer(sctx, sctx->null_const_buf.buffer, 0, sctx->null_const_buf.buffer->width0,
                     if (!(flags & SI_CONTEXT_FLAG_AUX)) {
               /* Check if the aux_context needs to be recreated */
   for (unsigned i = 0; i < ARRAY_SIZE(sscreen->aux_contexts); i++) {
      struct si_context *saux = si_get_aux_context(&sscreen->aux_contexts[i]);
                  if (status != PIPE_NO_RESET) {
      /* We lost the aux_context, create a new one */
   struct u_log_context *aux_log = saux->log;
                  saux = (struct si_context *)si_create_context(
      &sscreen->b, SI_CONTEXT_FLAG_AUX |
                        }
               simple_mtx_lock(&sscreen->async_compute_context_lock);
   if (sscreen->async_compute_context) {
      struct si_context *compute_ctx = (struct si_context*)sscreen->async_compute_context;
                  if (status != PIPE_NO_RESET) {
      sscreen->async_compute_context->destroy(sscreen->async_compute_context);
         }
               sctx->initial_gfx_cs_size = sctx->gfx_cs.current.cdw;
            sctx->cs_blit_shaders = _mesa_hash_table_create_u32_keys(NULL);
   if (!sctx->cs_blit_shaders)
               fail:
      fprintf(stderr, "radeonsi: Failed to create a context.\n");
   si_destroy_context(&sctx->b);
      }
      static bool si_is_resource_busy(struct pipe_screen *screen, struct pipe_resource *resource,
         {
               return !ws->buffer_wait(ws, si_resource(resource)->buf, 0,
                        }
      static struct pipe_context *si_pipe_create_context(struct pipe_screen *screen, void *priv,
         {
      struct si_screen *sscreen = (struct si_screen *)screen;
            if (sscreen->debug_flags & DBG(CHECK_VM))
                     if (ctx && sscreen->info.gfx_level >= GFX9 && sscreen->debug_flags & DBG(SQTT)) {
      /* Auto-enable stable performance profile if possible. */
   if (sscreen->info.has_stable_pstate && screen->num_contexts == 1 &&
                  if (ac_check_profile_state(&sscreen->info)) {
      fprintf(stderr, "radeonsi: Canceling RGP trace request as a hang condition has been "
                  } else if (!si_init_sqtt((struct si_context *)ctx)) {
      FREE(ctx);
                  if (!(flags & PIPE_CONTEXT_PREFER_THREADED))
            /* Clover (compute-only) is unsupported. */
   if (flags & PIPE_CONTEXT_COMPUTE_ONLY)
            /* When shaders are logged to stderr, asynchronous compilation is
   * disabled too. */
   if (sscreen->debug_flags & DBG_ALL_SHADERS)
            /* Use asynchronous flushes only on amdgpu, since the radeon
   * implementation for fence_server_sync is incomplete. */
   struct pipe_context *tc =
      threaded_context_create(ctx, &sscreen->pool_transfers,
                           si_replace_buffer_storage,
   &(struct threaded_context_options){
      .create_fence = sscreen->info.is_amdgpu ?
            if (tc && tc != ctx)
               }
      /*
   * pipe_screen
   */
   static void si_destroy_screen(struct pipe_screen *pscreen)
   {
      struct si_screen *sscreen = (struct si_screen *)pscreen;
   struct si_shader_part *parts[] = {sscreen->vs_prologs, sscreen->tcs_epilogs,
                  if (!sscreen->ws->unref(sscreen->ws))
            if (sscreen->debug_flags & DBG(CACHE_STATS)) {
      printf("live shader cache:   hits = %u, misses = %u\n", sscreen->live_shader_cache.hits,
         printf("memory shader cache: hits = %u, misses = %u\n", sscreen->num_memory_shader_cache_hits,
         printf("disk shader cache:   hits = %u, misses = %u\n", sscreen->num_disk_shader_cache_hits,
                        util_queue_destroy(&sscreen->shader_compiler_queue);
            for (unsigned i = 0; i < ARRAY_SIZE(sscreen->aux_contexts); i++) {
      if (!sscreen->aux_contexts[i].ctx)
            struct si_context *saux = si_get_aux_context(&sscreen->aux_contexts[i]);
   struct u_log_context *aux_log = saux->log;
   if (aux_log) {
      saux->b.set_log_context(&saux->b, NULL);
   u_log_context_destroy(aux_log);
               saux->b.destroy(&saux->b);
   mtx_unlock(&sscreen->aux_contexts[i].lock);
               simple_mtx_destroy(&sscreen->async_compute_context_lock);
   if (sscreen->async_compute_context) {
                  /* Release the reference on glsl types of the compiler threads. */
            for (i = 0; i < ARRAY_SIZE(sscreen->compiler); i++) {
      if (sscreen->compiler[i]) {
      si_destroy_compiler(sscreen->compiler[i]);
                  for (i = 0; i < ARRAY_SIZE(sscreen->compiler_lowp); i++) {
      if (sscreen->compiler_lowp[i]) {
      si_destroy_compiler(sscreen->compiler_lowp[i]);
                  /* Free shader parts. */
   for (i = 0; i < ARRAY_SIZE(parts); i++) {
      while (parts[i]) {
               parts[i] = part->next;
   si_shader_binary_clean(&part->binary);
         }
   simple_mtx_destroy(&sscreen->shader_parts_mutex);
            si_destroy_perfcounters(sscreen);
            simple_mtx_destroy(&sscreen->gpu_load_mutex);
                              disk_cache_destroy(sscreen->disk_shader_cache);
   util_live_shader_cache_deinit(&sscreen->live_shader_cache);
   util_idalloc_mt_fini(&sscreen->buffer_ids);
            sscreen->ws->destroy(sscreen->ws);
   FREE(sscreen->nir_options);
      }
      static void si_init_gs_info(struct si_screen *sscreen)
   {
         }
      static void si_test_vmfault(struct si_screen *sscreen, uint64_t test_flags)
   {
      struct pipe_context *ctx = sscreen->aux_context.general.ctx;
   struct si_context *sctx = (struct si_context *)ctx;
            if (!buf) {
      puts("Buffer allocation failed.");
                        if (test_flags & DBG(TEST_VMFAULT_CP)) {
      si_cp_dma_copy_buffer(sctx, buf, buf, 0, 4, 4, SI_OP_SYNC_BEFORE_AFTER,
         ctx->flush(ctx, NULL, 0);
      }
   if (test_flags & DBG(TEST_VMFAULT_SHADER)) {
      util_test_constant_buffer(ctx, buf);
      }
      }
      static void si_test_gds_memory_management(struct si_context *sctx, unsigned alloc_size,
         {
      struct radeon_winsys *ws = sctx->ws;
   struct radeon_cmdbuf cs[8];
            for (unsigned i = 0; i < ARRAY_SIZE(cs); i++) {
      ws->cs_create(&cs[i], sctx->ctx, AMD_IP_COMPUTE, NULL, NULL);
   gds_bo[i] = ws->buffer_create(ws, alloc_size, alignment, domain, 0);
               for (unsigned iterations = 0; iterations < 20000; iterations++) {
      for (unsigned i = 0; i < ARRAY_SIZE(cs); i++) {
      /* This clears GDS with CP DMA.
   *
   * We don't care if GDS is present. Just add some packet
   * to make the GPU busy for a moment.
   */
   si_cp_dma_clear_buffer(
      sctx, &cs[i], NULL, 0, alloc_size, 0,
               ws->cs_add_buffer(&cs[i], gds_bo[i], RADEON_USAGE_READWRITE, domain);
         }
      }
      static void si_disk_cache_create(struct si_screen *sscreen)
   {
      /* Don't use the cache if shader dumping is enabled. */
   if (sscreen->debug_flags & (DBG_ALL_SHADERS | DBG(USE_ACO)))
            struct mesa_sha1 ctx;
   unsigned char sha1[20];
                     if (!disk_cache_get_function_identifier(si_disk_cache_create, &ctx) ||
      !disk_cache_get_function_identifier(LLVMInitializeAMDGPUTargetInfo, &ctx))
         _mesa_sha1_final(&ctx, sha1);
            sscreen->disk_shader_cache = disk_cache_create(sscreen->info.name, cache_id,
      }
      static void si_set_max_shader_compiler_threads(struct pipe_screen *screen, unsigned max_threads)
   {
               /* This function doesn't allow a greater number of threads than
   * the queue had at its creation. */
   util_queue_adjust_num_threads(&sscreen->shader_compiler_queue, max_threads, false);
      }
      static bool si_is_parallel_shader_compilation_finished(struct pipe_screen *screen, void *shader,
         {
                  }
      static struct pipe_screen *radeonsi_screen_create_impl(struct radeon_winsys *ws,
         {
      struct si_screen *sscreen = CALLOC_STRUCT(si_screen);
   unsigned hw_threads, num_comp_hi_threads, num_comp_lo_threads;
            if (!sscreen) {
                     #define OPT_BOOL(name, dflt, description)                                                          \
         #define OPT_INT(name, dflt, description)                                                           \
         #include "si_debug_options.h"
               sscreen->ws = ws;
            if (sscreen->info.gfx_level >= GFX9) {
         } else {
      ac_get_raster_config(&sscreen->info, &sscreen->pa_sc_raster_config,
               sscreen->debug_flags = debug_get_flags_option("R600_DEBUG", radeonsi_debug_options, 0);
   sscreen->debug_flags |= debug_get_flags_option("AMD_DEBUG", radeonsi_debug_options, 0);
            if (sscreen->debug_flags & DBG(NO_DISPLAY_DCC)) {
      sscreen->info.use_display_dcc_unaligned = false;
               if (sscreen->debug_flags & DBG(SHADOW_REGS)) {
      sscreen->info.register_shadowing_required = true;
   /* Recompute has_set_pairs_packets. */
   sscreen->info.has_set_pairs_packets = sscreen->info.gfx_level >= GFX11 &&
                     if (sscreen->debug_flags & DBG(NO_GFX))
            if ((sscreen->debug_flags & DBG(TMZ)) &&
      !sscreen->info.has_tmz_support) {
   fprintf(stderr, "radeonsi: requesting TMZ features but TMZ is not supported\n");
   FREE(sscreen->nir_options);
   FREE(sscreen);
               /* Initialize just one compiler instance to check for errors. The other compiler instances are
   * initialized on demand.
   */
   sscreen->compiler[0] = CALLOC_STRUCT(ac_llvm_compiler);
   if (!si_init_compiler(sscreen, sscreen->compiler[0])) {
      /* The callee prints the error message. */
   FREE(sscreen->nir_options);
   FREE(sscreen);
                        /* Set functions first. */
   sscreen->b.context_create = si_pipe_create_context;
   sscreen->b.destroy = si_destroy_screen;
   sscreen->b.set_max_shader_compiler_threads = si_set_max_shader_compiler_threads;
   sscreen->b.is_parallel_shader_compilation_finished = si_is_parallel_shader_compilation_finished;
                     si_init_screen_get_functions(sscreen);
   si_init_screen_buffer_functions(sscreen);
   si_init_screen_fence_functions(sscreen);
   si_init_screen_state_functions(sscreen);
   si_init_screen_texture_functions(sscreen);
   si_init_screen_query_functions(sscreen);
            sscreen->max_texel_buffer_elements = sscreen->b.get_param(
            if (sscreen->debug_flags & DBG(INFO))
                     sscreen->force_aniso = MIN2(16, debug_get_num_option("R600_TEX_ANISO", -1));
   if (sscreen->force_aniso == -1) {
                  if (sscreen->force_aniso >= 0) {
      printf("radeonsi: Forcing anisotropy filter to %ix\n",
                     (void)simple_mtx_init(&sscreen->async_compute_context_lock, mtx_plain);
   (void)simple_mtx_init(&sscreen->gpu_load_mutex, mtx_plain);
            si_init_gs_info(sscreen);
   if (!si_init_shader_cache(sscreen)) {
      FREE(sscreen->nir_options);
   FREE(sscreen);
               if (sscreen->info.gfx_level < GFX10_3)
                     /* Determine the number of shader compiler threads. */
   const struct util_cpu_caps_t *caps = util_get_cpu_caps();
            if (hw_threads >= 12) {
      num_comp_hi_threads = hw_threads * 3 / 4;
      } else if (hw_threads >= 6) {
      num_comp_hi_threads = hw_threads - 2;
      } else if (hw_threads >= 2) {
      num_comp_hi_threads = hw_threads - 1;
      } else {
      num_comp_hi_threads = 1;
            #ifndef NDEBUG
               /* Use a single compilation thread if NIR printing is enabled to avoid
   * multiple shaders being printed at the same time.
   */
   if (NIR_DEBUG(PRINT)) {
      num_comp_hi_threads = 1;
         #endif
         num_comp_hi_threads = MIN2(num_comp_hi_threads, ARRAY_SIZE(sscreen->compiler));
            /* Take a reference on the glsl types for the compiler threads. */
            /* Start with a single thread and a single slot.
   * Each time we'll hit the "all slots are in use" case, the number of threads and
   * slots will be increased.
   */
   int num_slots = num_comp_hi_threads == 1 ? 64 : 1;
   if (!util_queue_init(&sscreen->shader_compiler_queue, "sh", num_slots,
                        si_destroy_shader_cache(sscreen);
   FREE(sscreen->nir_options);
   FREE(sscreen);
   glsl_type_singleton_decref();
               if (!util_queue_init(&sscreen->shader_compiler_queue_low_priority, "shlo", num_slots,
                        num_comp_lo_threads,
   si_destroy_shader_cache(sscreen);
   FREE(sscreen->nir_options);
   FREE(sscreen);
   glsl_type_singleton_decref();
               if (!debug_get_bool_option("RADEON_DISABLE_PERFCOUNTERS", false))
                     sscreen->has_draw_indirect_multi =
      (sscreen->info.family >= CHIP_POLARIS10) ||
   (sscreen->info.gfx_level == GFX8 && sscreen->info.pfp_fw_version >= 121 &&
   sscreen->info.me_fw_version >= 87) ||
   (sscreen->info.gfx_level == GFX7 && sscreen->info.pfp_fw_version >= 211 &&
   sscreen->info.me_fw_version >= 173) ||
   (sscreen->info.gfx_level == GFX6 && sscreen->info.pfp_fw_version >= 79 &&
         if (sscreen->debug_flags & DBG(NO_OUT_OF_ORDER))
            if (sscreen->info.gfx_level >= GFX11) {
      sscreen->use_ngg = true;
   sscreen->use_ngg_culling = sscreen->info.max_render_backends >= 2 &&
      } else {
      sscreen->use_ngg = !(sscreen->debug_flags & DBG(NO_NGG)) &&
                     sscreen->use_ngg_culling = sscreen->use_ngg &&
                     /* Only set this for the cases that are known to work, which are:
   * - GFX9 if bpp >= 4 (in bytes)
   */
   if (sscreen->info.gfx_level >= GFX10) {
      memset(sscreen->allow_dcc_msaa_clear_to_reg_for_bpp, true,
      } else if (sscreen->info.gfx_level == GFX9) {
      for (unsigned bpp_log2 = util_logbase2(1); bpp_log2 <= util_logbase2(16); bpp_log2++)
               /* DCC stores have 50% performance of uncompressed stores and sometimes
   * even less than that. It's risky to enable on dGPUs.
   */
   sscreen->always_allow_dcc_stores = !(sscreen->debug_flags & DBG(NO_DCC_STORE)) &&
                              sscreen->dpbb_allowed = !(sscreen->debug_flags & DBG(NO_DPBB)) &&
                              if (sscreen->dpbb_allowed) {
      /* Only bin draws that have no CONTEXT and SH register changes between them because higher
   * settings cause hangs. We've only been able to reproduce hangs on smaller chips
   * (e.g. Navi24, GFX1103), though all chips might have them. What we see may be due to
   * a driver bug.
   */
   sscreen->pbb_context_states_per_bin = 1;
            assert(sscreen->pbb_context_states_per_bin >= 1 &&
         assert(sscreen->pbb_persistent_states_per_bin >= 1 &&
               (void)simple_mtx_init(&sscreen->shader_parts_mutex, mtx_plain);
            sscreen->barrier_flags.cp_to_L2 = SI_CONTEXT_INV_SCACHE | SI_CONTEXT_INV_VCACHE;
   if (sscreen->info.gfx_level <= GFX8) {
      sscreen->barrier_flags.cp_to_L2 |= SI_CONTEXT_INV_L2;
               if (debug_get_bool_option("RADEON_DUMP_SHADERS", false))
            /* Syntax:
   *     EQAA=s,z,c
   * Example:
            * That means 8 coverage samples, 4 Z/S samples, and 2 color samples.
   * Constraints:
   *     s >= z >= c (ignoring this only wastes memory)
   *     s = [2..16]
   *     z = [2..8]
   *     c = [2..8]
   *
   * Only MSAA color and depth buffers are overridden.
   */
   if (sscreen->info.has_eqaa_surface_allocator) {
      const char *eqaa = debug_get_option("EQAA", NULL);
            if (eqaa && sscanf(eqaa, "%u,%u,%u", &s, &z, &f) == 3 && s && z && f) {
      sscreen->eqaa_force_coverage_samples = s;
   sscreen->eqaa_force_z_samples = z;
                  if (sscreen->info.gfx_level >= GFX11) {
      unsigned attr_ring_size = sscreen->info.attribute_ring_size_per_se * sscreen->info.max_se;
   sscreen->attribute_ring = si_aligned_buffer_create(&sscreen->b,
                                             /* Create the auxiliary context. This must be done last. */
   for (unsigned i = 0; i < ARRAY_SIZE(sscreen->aux_contexts); i++) {
               sscreen->aux_contexts[i].ctx =
      si_create_context(&sscreen->b,
                     if (sscreen->options.aux_debug) {
                     struct si_context *sctx = si_get_aux_context(&sscreen->aux_context.general);
   sctx->b.set_log_context(&sctx->b, log);
                  if (test_flags & DBG(TEST_IMAGE_COPY))
            if (test_flags & (DBG(TEST_CB_RESOLVE) | DBG(TEST_COMPUTE_BLIT)))
            if (test_flags & DBG(TEST_DMA_PERF)) {
                  if (test_flags & (DBG(TEST_VMFAULT_CP) | DBG(TEST_VMFAULT_SHADER)))
            if (test_flags & DBG(TEST_GDS))
            if (test_flags & DBG(TEST_GDS_MM)) {
      si_test_gds_memory_management((struct si_context *)sscreen->aux_context.general.ctx,
      }
   if (test_flags & DBG(TEST_GDS_OA_MM)) {
      si_test_gds_memory_management((struct si_context *)sscreen->aux_context.general.ctx,
                           }
      struct pipe_screen *radeonsi_screen_create(int fd, const struct pipe_screen_config *config)
   {
      struct radeon_winsys *rw = NULL;
            version = drmGetVersion(fd);
   if (!version)
            /* LLVM must be initialized before util_queue because both u_queue and LLVM call atexit,
   * and LLVM must call it first because its atexit handler executes C++ destructors,
   * which must be done after our compiler threads using LLVM in u_queue are finished
   * by their atexit handler. Since atexit handlers are called in the reverse order,
   * LLVM must be initialized first, followed by u_queue.
   */
            driParseConfigFiles(config->options, config->options_info, 0, "radeonsi",
            switch (version->version_major) {
   case 2:
      rw = radeon_drm_winsys_create(fd, config, radeonsi_screen_create_impl);
      case 3:
      rw = amdgpu_winsys_create(fd, config, radeonsi_screen_create_impl);
                        drmFreeVersion(version);
      }
      struct si_context *si_get_aux_context(struct si_aux_context *ctx)
   {
      mtx_lock(&ctx->lock);
      }
      void si_put_aux_context_flush(struct si_aux_context *ctx)
   {
      ctx->ctx->flush(ctx->ctx, NULL, 0);
      }
