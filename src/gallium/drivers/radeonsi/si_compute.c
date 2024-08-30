   /*
   * Copyright 2013 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_rtld.h"
   #include "amd_kernel_code_t.h"
   #include "nir/tgsi_to_nir.h"
   #include "si_build_pm4.h"
   #include "si_shader_internal.h"
   #include "util/u_async_debug.h"
   #include "util/u_memory.h"
   #include "util/u_upload_mgr.h"
   #include "si_tracepoints.h"
      #define COMPUTE_DBG(sscreen, fmt, args...)                                                         \
      do {                                                                                            \
      if ((sscreen->debug_flags & DBG(COMPUTE)))                                                   \
            struct dispatch_packet {
      uint16_t header;
   uint16_t setup;
   uint16_t workgroup_size_x;
   uint16_t workgroup_size_y;
   uint16_t workgroup_size_z;
   uint16_t reserved0;
   uint32_t grid_size_x;
   uint32_t grid_size_y;
   uint32_t grid_size_z;
   uint32_t group_segment_size;
   uint64_t kernel_object;
   uint64_t kernarg_address;
      };
      static const amd_kernel_code_t *si_compute_get_code_object(const struct si_compute *program,
         {
               if (program->ir_type != PIPE_SHADER_IR_NATIVE)
            struct ac_rtld_binary rtld;
   if (!ac_rtld_open(&rtld,
                     (struct ac_rtld_open_info){.info = &sel->screen->info,
                  const amd_kernel_code_t *result = NULL;
   const char *text;
   size_t size;
   if (!ac_rtld_get_section_by_name(&rtld, ".text", &text, &size))
            if (symbol_offset + sizeof(amd_kernel_code_t) > size)
                  out:
      ac_rtld_close(&rtld);
      }
      static void code_object_to_config(const amd_kernel_code_t *code_object,
         {
         uint32_t rsrc1 = code_object->compute_pgm_resource_registers;
   uint32_t rsrc2 = code_object->compute_pgm_resource_registers >> 32;
   out_config->num_sgprs = code_object->wavefront_sgpr_count;
   out_config->num_vgprs = code_object->workitem_vgpr_count;
   out_config->float_mode = G_00B028_FLOAT_MODE(rsrc1);
   out_config->rsrc1 = rsrc1;
   out_config->lds_size = MAX2(out_config->lds_size, G_00B84C_LDS_SIZE(rsrc2));
   out_config->rsrc2 = rsrc2;
   out_config->scratch_bytes_per_wave =
      }
      /* Asynchronous compute shader compilation. */
   static void si_create_compute_state_async(void *job, void *gdata, int thread_index)
   {
      struct si_compute *program = (struct si_compute *)job;
   struct si_shader_selector *sel = &program->sel;
   struct si_shader *shader = &program->shader;
   struct ac_llvm_compiler **compiler;
   struct util_debug_callback *debug = &sel->compiler_ctx_state.debug;
            assert(!debug->debug_message || debug->async);
   assert(thread_index >= 0);
   assert(thread_index < ARRAY_SIZE(sscreen->compiler));
            if (!*compiler) {
      *compiler = CALLOC_STRUCT(ac_llvm_compiler);
               assert(program->ir_type == PIPE_SHADER_IR_NIR);
            si_get_active_slot_masks(sscreen, &sel->info, &sel->active_const_and_shader_buffers,
            program->shader.is_monolithic = true;
            /* Variable block sizes need 10 bits (1 + log2(SI_MAX_VARIABLE_THREADS_PER_BLOCK)) per dim.
   * We pack them into a single user SGPR.
   */
   unsigned user_sgprs = SI_NUM_RESOURCE_SGPRS + (sel->info.uses_grid_size ? 3 : 0) +
                  /* Fast path for compute shaders - some descriptors passed via user SGPRs. */
   /* Shader buffers in user SGPRs. */
   for (unsigned i = 0; i < MIN2(3, sel->info.base.num_ssbos) && user_sgprs <= 12; i++) {
      user_sgprs = align(user_sgprs, 4);
   if (i == 0)
         user_sgprs += 4;
               /* Images in user SGPRs. */
            /* Remove images with FMASK from the bitmask.  We only care about the first
   * 3 anyway, so we can take msaa_images[0] and ignore the rest.
   */
   if (sscreen->info.gfx_level < GFX11)
            for (unsigned i = 0; i < 3 && non_fmask_images & (1 << i); i++) {
               if (align(user_sgprs, num_sgprs) + num_sgprs > 16)
            user_sgprs = align(user_sgprs, num_sgprs);
   if (i == 0)
         user_sgprs += num_sgprs;
      }
   sel->cs_images_num_sgprs = user_sgprs - sel->cs_images_sgpr_index;
            unsigned char ir_sha1_cache_key[20];
            /* Try to load the shader from the shader cache. */
            if (si_shader_cache_load_shader(sscreen, ir_sha1_cache_key, shader)) {
               if (!si_shader_binary_upload(sscreen, shader, 0))
            si_shader_dump_stats_for_shader_db(sscreen, shader, debug);
      } else {
               if (!si_create_shader_variant(sscreen, *compiler, &program->shader, debug)) {
      program->shader.compilation_failed = true;
               shader->config.rsrc1 = S_00B848_VGPRS((shader->config.num_vgprs - 1) /
                                    if (sscreen->info.gfx_level < GFX10) {
                  shader->config.rsrc2 = S_00B84C_USER_SGPR(user_sgprs) |
                        S_00B84C_SCRATCH_EN(shader->config.scratch_bytes_per_wave > 0) |
   S_00B84C_TGID_X_EN(sel->info.uses_block_id[0]) |
   S_00B84C_TGID_Y_EN(sel->info.uses_block_id[1]) |
   S_00B84C_TGID_Z_EN(sel->info.uses_block_id[2]) |
               simple_mtx_lock(&sscreen->shader_cache_mutex);
   si_shader_cache_insert_shader(sscreen, ir_sha1_cache_key, shader, true);
               ralloc_free(sel->nir);
      }
      static void *si_create_compute_state(struct pipe_context *ctx, const struct pipe_compute_state *cso)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_screen *sscreen = (struct si_screen *)ctx->screen;
   struct si_compute *program = CALLOC_STRUCT(si_compute);
            pipe_reference_init(&sel->base.reference, 1);
   sel->stage = MESA_SHADER_COMPUTE;
   sel->screen = sscreen;
   sel->const_and_shader_buf_descriptors_index =
         sel->sampler_and_images_descriptors_index =
         sel->info.base.shared_size = cso->static_shared_mem;
   program->shader.selector = &program->sel;
   program->ir_type = cso->ir_type;
            if (cso->ir_type != PIPE_SHADER_IR_NATIVE) {
      if (cso->ir_type == PIPE_SHADER_IR_TGSI) {
      program->ir_type = PIPE_SHADER_IR_NIR;
      } else {
      assert(cso->ir_type == PIPE_SHADER_IR_NIR);
               if (si_can_dump_shader(sscreen, sel->stage, SI_DUMP_INIT_NIR))
            sel->compiler_ctx_state.debug = sctx->debug;
   sel->compiler_ctx_state.is_debug_context = sctx->is_debug;
            si_schedule_initial_compile(sctx, MESA_SHADER_COMPUTE, &sel->ready, &sel->compiler_ctx_state,
      } else {
      const struct pipe_binary_program_header *header;
            program->shader.binary.type = SI_SHADER_BINARY_ELF;
   program->shader.binary.code_size = header->num_bytes;
   program->shader.binary.code_buffer = malloc(header->num_bytes);
   if (!program->shader.binary.code_buffer) {
      FREE(program);
      }
            const amd_kernel_code_t *code_object = si_compute_get_code_object(program, 0);
            if (AMD_HSA_BITS_GET(code_object->code_properties, AMD_CODE_PROPERTY_ENABLE_WAVEFRONT_SIZE32))
         else
            bool ok = si_shader_binary_upload(sctx->screen, &program->shader, 0);
            if (!ok) {
      fprintf(stderr, "LLVM failed to upload shader\n");
   free((void *)program->shader.binary.code_buffer);
   FREE(program);
                     }
      static void si_get_compute_state_info(struct pipe_context *ctx, void *state,
         {
      struct si_compute *program = (struct si_compute *)state;
                     /* Wait because we need the compilation to finish first */
            uint8_t wave_size = program->shader.wave_size;
   info->private_memory = DIV_ROUND_UP(program->shader.config.scratch_bytes_per_wave, wave_size);
   info->preferred_simd_size = wave_size;
   info->simd_sizes = wave_size;
      }
      static void si_bind_compute_state(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_compute *program = (struct si_compute *)state;
            sctx->cs_shader_state.program = program;
   if (!program)
            /* Wait because we need active slot usage masks. */
   if (program->ir_type != PIPE_SHADER_IR_NATIVE)
            si_set_active_descriptors(sctx,
               si_set_active_descriptors(sctx, SI_DESCS_FIRST_COMPUTE + SI_SHADER_DESCS_SAMPLERS_AND_IMAGES,
            sctx->compute_shaderbuf_sgprs_dirty = true;
            if (unlikely((sctx->screen->debug_flags & DBG(SQTT)) && sctx->sqtt)) {
      uint32_t pipeline_code_hash = _mesa_hash_data_with_seed(
      program->shader.binary.code_buffer,
               if (!si_sqtt_pipeline_is_registered(sctx->sqtt, pipeline_code_hash)) {
      /* Short lived fake pipeline: we don't need to reupload the compute shaders,
   * as we do for the gfx ones so just create a temp pipeline to be able to
   * call si_sqtt_register_pipeline, and then drop it.
   */
   struct si_sqtt_fake_pipeline pipeline = { 0 };
                                    }
      static void si_set_global_binding(struct pipe_context *ctx, unsigned first, unsigned n,
         {
      unsigned i;
   struct si_context *sctx = (struct si_context *)ctx;
            if (first + n > program->max_global_buffers) {
      unsigned old_max = program->max_global_buffers;
   program->max_global_buffers = first + n;
   program->global_buffers = realloc(
         if (!program->global_buffers) {
      fprintf(stderr, "radeonsi: failed to allocate compute global_buffers\n");
               memset(&program->global_buffers[old_max], 0,
               if (!resources) {
      for (i = 0; i < n; i++) {
         }
               for (i = 0; i < n; i++) {
      uint64_t va;
   uint32_t offset;
   pipe_resource_reference(&program->global_buffers[first + i], resources[i]);
   va = si_resource(resources[i])->gpu_address;
   offset = util_le32_to_cpu(*handles[i]);
   va += offset;
   va = util_cpu_to_le64(va);
         }
      static bool si_setup_compute_scratch_buffer(struct si_context *sctx, struct si_shader *shader)
   {
      uint64_t scratch_bo_size, scratch_needed;
   scratch_bo_size = 0;
   scratch_needed = sctx->max_seen_compute_scratch_bytes_per_wave * sctx->screen->info.max_scratch_waves;
   if (sctx->compute_scratch_buffer)
            if (scratch_bo_size < scratch_needed) {
               sctx->compute_scratch_buffer =
      si_aligned_buffer_create(&sctx->screen->b,
                           if (!sctx->compute_scratch_buffer)
               if (sctx->compute_scratch_buffer != shader->scratch_bo && scratch_needed) {
      if (sctx->gfx_level < GFX11 &&
                     if (!si_shader_binary_upload(sctx->screen, shader, scratch_va))
      }
                  }
      static bool si_switch_compute_shader(struct si_context *sctx, struct si_compute *program,
               {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
   struct ac_shader_config inline_config = {0};
   const struct ac_shader_config *config;
   unsigned rsrc2;
   uint64_t shader_va;
                     assert(variable_shared_size == 0 || stage == MESA_SHADER_KERNEL || program->ir_type == PIPE_SHADER_IR_NATIVE);
   if (sctx->cs_shader_state.emitted_program == program && sctx->cs_shader_state.offset == offset &&
      sctx->cs_shader_state.variable_shared_size == variable_shared_size)
         if (program->ir_type != PIPE_SHADER_IR_NATIVE) {
         } else {
      code_object_to_config(code_object, &inline_config);
      }
   /* copy rsrc2 so we don't have to change it inside the si_shader object */
            /* only do this for OpenCL */
   if (program->ir_type == PIPE_SHADER_IR_NATIVE || stage == MESA_SHADER_KERNEL) {
      unsigned shared_size = program->sel.info.base.shared_size + variable_shared_size;
            /* Clover uses the compute API differently than other frontends and expects drivers to parse
   * the shared_size out of the shader headers.
   */
   if (program->ir_type == PIPE_SHADER_IR_NATIVE) {
         } else {
                  /* XXX: We are over allocating LDS.  For GFX6, the shader reports
   * LDS in blocks of 256 bytes, so if there are 4 bytes lds
   * allocated in the shader and 4 bytes allocated by the state
   * tracker, then we will set LDS_SIZE to 512 bytes rather than 256.
   */
   if (sctx->gfx_level <= GFX6) {
         } else {
                  /* TODO: use si_multiwave_lds_size_workaround */
            rsrc2 &= C_00B84C_LDS_SIZE;
               unsigned tmpring_size;
   ac_get_scratch_tmpring_size(&sctx->screen->info,
                  if (!si_setup_compute_scratch_buffer(sctx, shader))
            if (shader->scratch_bo) {
      radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, shader->scratch_bo,
               shader_va = shader->bo->gpu_address + offset;
   if (program->ir_type == PIPE_SHADER_IR_NATIVE) {
      /* Shader code is placed after the amd_kernel_code_t
   * struct. */
               radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, shader->bo,
            if (sctx->screen->info.has_set_pairs_packets) {
      radeon_push_compute_sh_reg(R_00B830_COMPUTE_PGM_LO, shader_va >> 8);
   radeon_opt_push_compute_sh_reg(R_00B848_COMPUTE_PGM_RSRC1,
         radeon_opt_push_compute_sh_reg(R_00B84C_COMPUTE_PGM_RSRC2,
         radeon_opt_push_compute_sh_reg(R_00B8A0_COMPUTE_PGM_RSRC3,
               radeon_opt_push_compute_sh_reg(R_00B860_COMPUTE_TMPRING_SIZE,
         if (shader->scratch_bo) {
      radeon_opt_push_compute_sh_reg(R_00B840_COMPUTE_DISPATCH_SCRATCH_BASE_LO,
               radeon_opt_push_compute_sh_reg(R_00B844_COMPUTE_DISPATCH_SCRATCH_BASE_HI,
               } else {
      radeon_begin(cs);
   radeon_set_sh_reg(R_00B830_COMPUTE_PGM_LO, shader_va >> 8);
   radeon_opt_set_sh_reg2(sctx, R_00B848_COMPUTE_PGM_RSRC1,
               radeon_opt_set_sh_reg(sctx, R_00B860_COMPUTE_TMPRING_SIZE,
            if (shader->scratch_bo &&
      (sctx->gfx_level >= GFX11 ||
   (sctx->family >= CHIP_GFX940 && !sctx->screen->info.has_graphics))) {
   radeon_opt_set_sh_reg2(sctx, R_00B840_COMPUTE_DISPATCH_SCRATCH_BASE_LO,
                           if (sctx->gfx_level >= GFX11) {
      radeon_opt_set_sh_reg(sctx, R_00B8A0_COMPUTE_PGM_RSRC3,
            }
               COMPUTE_DBG(sctx->screen,
                        sctx->cs_shader_state.emitted_program = program;
   sctx->cs_shader_state.offset = offset;
            *prefetch = true;
      }
      static void setup_scratch_rsrc_user_sgprs(struct si_context *sctx,
         {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
            unsigned max_private_element_size =
            uint32_t scratch_dword0 = scratch_va & 0xffffffff;
            if (sctx->gfx_level >= GFX11)
         else
            /* Disable address clamping */
   uint32_t scratch_dword2 = 0xffffffff;
   uint32_t index_stride = sctx->cs_shader_state.program->shader.wave_size == 32 ? 2 : 3;
            if (sctx->gfx_level >= GFX9) {
         } else {
               if (sctx->gfx_level < GFX8) {
      /* BUF_DATA_FORMAT is ignored, but it cannot be
   * BUF_DATA_FORMAT_INVALID. */
                  radeon_begin(cs);
   radeon_set_sh_reg_seq(R_00B900_COMPUTE_USER_DATA_0 + (user_sgpr * 4), 4);
   radeon_emit(scratch_dword0);
   radeon_emit(scratch_dword1);
   radeon_emit(scratch_dword2);
   radeon_emit(scratch_dword3);
      }
      static void si_setup_user_sgprs_co_v2(struct si_context *sctx, const amd_kernel_code_t *code_object,
         {
      struct si_compute *program = sctx->cs_shader_state.program;
            static const enum amd_code_property_mask_t workgroup_count_masks[] = {
      AMD_CODE_PROPERTY_ENABLE_SGPR_GRID_WORKGROUP_COUNT_X,
   AMD_CODE_PROPERTY_ENABLE_SGPR_GRID_WORKGROUP_COUNT_Y,
         unsigned i, user_sgpr = 0;
   if (AMD_HSA_BITS_GET(code_object->code_properties,
            if (code_object->workitem_private_segment_byte_size > 0) {
         }
                        if (AMD_HSA_BITS_GET(code_object->code_properties, AMD_CODE_PROPERTY_ENABLE_SGPR_DISPATCH_PTR)) {
      struct dispatch_packet dispatch;
   unsigned dispatch_offset;
   struct si_resource *dispatch_buf = NULL;
            /* Upload dispatch ptr */
            dispatch.workgroup_size_x = util_cpu_to_le16(info->block[0]);
   dispatch.workgroup_size_y = util_cpu_to_le16(info->block[1]);
            dispatch.grid_size_x = util_cpu_to_le32(info->grid[0] * info->block[0]);
   dispatch.grid_size_y = util_cpu_to_le32(info->grid[1] * info->block[1]);
            dispatch.group_segment_size =
                     u_upload_data(sctx->b.const_uploader, 0, sizeof(dispatch), 256, &dispatch, &dispatch_offset,
            if (!dispatch_buf) {
      fprintf(stderr, "Error: Failed to allocate dispatch "
      }
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, dispatch_buf,
                     radeon_set_sh_reg_seq(R_00B900_COMPUTE_USER_DATA_0 + (user_sgpr * 4), 2);
   radeon_emit(dispatch_va);
            si_resource_reference(&dispatch_buf, NULL);
               if (AMD_HSA_BITS_GET(code_object->code_properties,
            radeon_set_sh_reg_seq(R_00B900_COMPUTE_USER_DATA_0 + (user_sgpr * 4), 2);
   radeon_emit(kernel_args_va);
   radeon_emit(S_008F04_BASE_ADDRESS_HI(kernel_args_va >> 32) | S_008F04_STRIDE(0));
               for (i = 0; i < 3 && user_sgpr < 16; i++) {
      if (code_object->code_properties & workgroup_count_masks[i]) {
      radeon_set_sh_reg_seq(R_00B900_COMPUTE_USER_DATA_0 + (user_sgpr * 4), 1);
   radeon_emit(info->grid[i]);
         }
      }
      static bool si_upload_compute_input(struct si_context *sctx, const amd_kernel_code_t *code_object,
         {
      struct si_compute *program = sctx->cs_shader_state.program;
   struct si_resource *input_buffer = NULL;
   uint32_t kernel_args_offset = 0;
   uint32_t *kernel_args;
   void *kernel_args_ptr;
            u_upload_alloc(sctx->b.const_uploader, 0, program->input_size,
                  if (unlikely(!kernel_args_ptr))
            kernel_args = (uint32_t *)kernel_args_ptr;
                     for (unsigned i = 0; i < program->input_size / 4; i++) {
                  radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, input_buffer,
            si_setup_user_sgprs_co_v2(sctx, code_object, info, kernel_args_va);
   si_resource_reference(&input_buffer, NULL);
      }
      static void si_setup_nir_user_data(struct si_context *sctx, const struct pipe_grid_info *info)
   {
      struct si_compute *program = sctx->cs_shader_state.program;
   struct si_shader_selector *sel = &program->sel;
   struct radeon_cmdbuf *cs = &sctx->gfx_cs;
   unsigned grid_size_reg = R_00B900_COMPUTE_USER_DATA_0 + 4 * SI_NUM_RESOURCE_SGPRS;
   unsigned block_size_reg = grid_size_reg +
                                 if (sel->info.uses_grid_size) {
      if (info->indirect) {
               for (unsigned i = 0; i < 3; ++i) {
      si_cp_copy_data(sctx, &sctx->gfx_cs, COPY_DATA_REG, NULL, (grid_size_reg >> 2) + i,
            }
      } else {
      if (sctx->screen->info.has_set_pairs_packets) {
      radeon_push_compute_sh_reg(grid_size_reg, info->grid[0]);
   radeon_push_compute_sh_reg(grid_size_reg + 4, info->grid[1]);
      } else {
      radeon_set_sh_reg_seq(grid_size_reg, 3);
   radeon_emit(info->grid[0]);
   radeon_emit(info->grid[1]);
                     if (sel->info.uses_variable_block_size) {
               if (sctx->screen->info.has_set_pairs_packets) {
         } else {
                     if (sel->info.base.cs.user_data_components_amd) {
               if (sctx->screen->info.has_set_pairs_packets) {
      for (unsigned i = 0; i < num; i++)
      } else {
      radeon_set_sh_reg_seq(cs_user_data_reg, num);
         }
      }
      static void si_emit_dispatch_packets(struct si_context *sctx, const struct pipe_grid_info *info)
   {
      struct si_screen *sscreen = sctx->screen;
   struct radeon_cmdbuf *cs = &sctx->gfx_cs;
   bool render_cond_bit = sctx->render_cond_enabled;
   unsigned threads_per_threadgroup = info->block[0] * info->block[1] * info->block[2];
   unsigned waves_per_threadgroup =
                  if (sctx->gfx_level >= GFX10 && waves_per_threadgroup == 1)
            if (unlikely(sctx->sqtt_enabled)) {
      si_write_event_with_dims_marker(sctx, &sctx->gfx_cs,
                     radeon_begin(cs);
   unsigned compute_resource_limits =
      ac_get_compute_resource_limits(&sscreen->info, waves_per_threadgroup,
               if (sctx->screen->info.has_set_pairs_packets) {
      radeon_opt_push_compute_sh_reg(R_00B854_COMPUTE_RESOURCE_LIMITS,
            } else {
      radeon_opt_set_sh_reg(sctx, R_00B854_COMPUTE_RESOURCE_LIMITS,
                     unsigned dispatch_initiator = S_00B800_COMPUTE_SHADER_EN(1) | S_00B800_FORCE_START_AT_000(1) |
                                 /* If the KMD allows it (there is a KMD hw register for it),
            const uint *last_block = info->last_block;
   bool partial_block_en = last_block[0] || last_block[1] || last_block[2];
            if (partial_block_en) {
               /* If no partial_block, these should be an entire block size, not 0. */
   partial[0] = last_block[0] ? last_block[0] : info->block[0];
   partial[1] = last_block[1] ? last_block[1] : info->block[1];
            num_threads[0] = S_00B81C_NUM_THREAD_FULL(info->block[0]) |
         num_threads[1] = S_00B820_NUM_THREAD_FULL(info->block[1]) |
         num_threads[2] = S_00B824_NUM_THREAD_FULL(info->block[2]) |
               } else {
      num_threads[0] = S_00B81C_NUM_THREAD_FULL(info->block[0]);
   num_threads[1] = S_00B820_NUM_THREAD_FULL(info->block[1]);
               if (sctx->screen->info.has_set_pairs_packets) {
      radeon_opt_push_compute_sh_reg(R_00B81C_COMPUTE_NUM_THREAD_X,
         radeon_opt_push_compute_sh_reg(R_00B820_COMPUTE_NUM_THREAD_Y,
         radeon_opt_push_compute_sh_reg(R_00B824_COMPUTE_NUM_THREAD_Z,
      } else {
      radeon_opt_set_sh_reg3(sctx, R_00B81C_COMPUTE_NUM_THREAD_X,
                     if (sctx->gfx_level >= GFX11) {
      radeon_end();
   gfx11_emit_buffered_compute_sh_regs(sctx);
               if (info->indirect) {
               radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(info->indirect),
            radeon_emit(PKT3(PKT3_SET_BASE, 2, 0) | PKT3_SHADER_TYPE_S(1));
   radeon_emit(1);
   radeon_emit(base_va);
            radeon_emit(PKT3(PKT3_DISPATCH_INDIRECT, 1, render_cond_bit) | PKT3_SHADER_TYPE_S(1));
   radeon_emit(info->indirect_offset);
      } else {
      radeon_emit(PKT3(PKT3_DISPATCH_DIRECT, 3, render_cond_bit) | PKT3_SHADER_TYPE_S(1));
   radeon_emit(info->grid[0]);
   radeon_emit(info->grid[1]);
   radeon_emit(info->grid[2]);
               if (unlikely(sctx->sqtt_enabled && sctx->gfx_level >= GFX9)) {
      radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
      }
      }
      static bool si_check_needs_implicit_sync(struct si_context *sctx)
   {
      /* If the compute shader is going to read from a texture/image written by a
   * previous draw, we must wait for its completion before continuing.
   * Buffers and image stores (from the draw) are not taken into consideration
   * because that's the app responsibility.
   *
   * The OpenGL 4.6 spec says:
   *
   *    buffer object and texture stores performed by shaders are not
   *    automatically synchronized
   *
   * TODO: Bindless textures are not handled, and thus are not synchronized.
   */
   struct si_shader_info *info = &sctx->cs_shader_state.program->sel.info;
   struct si_samplers *samplers = &sctx->samplers[PIPE_SHADER_COMPUTE];
            while (mask) {
      int i = u_bit_scan(&mask);
            struct si_resource *res = si_resource(sview->base.texture);
   if (sctx->ws->cs_is_buffer_referenced(&sctx->gfx_cs, res->buf,
                     struct si_images *images = &sctx->images[PIPE_SHADER_COMPUTE];
            while (mask) {
      int i = u_bit_scan(&mask);
            struct si_resource *res = si_resource(sview->resource);
   if (sctx->ws->cs_is_buffer_referenced(&sctx->gfx_cs, res->buf,
            }
      }
      static void si_launch_grid(struct pipe_context *ctx, const struct pipe_grid_info *info)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_screen *sscreen = sctx->screen;
   struct si_compute *program = sctx->cs_shader_state.program;
   const amd_kernel_code_t *code_object = si_compute_get_code_object(program, info->pc);
   int i;
   bool cs_regalloc_hang = sscreen->info.has_cs_regalloc_hang_bug &&
            if (cs_regalloc_hang) {
      sctx->flags |= SI_CONTEXT_PS_PARTIAL_FLUSH | SI_CONTEXT_CS_PARTIAL_FLUSH;
               if (program->ir_type != PIPE_SHADER_IR_NATIVE && program->shader.compilation_failed)
                     if (sctx->has_graphics) {
      if (sctx->last_num_draw_calls != sctx->num_draw_calls) {
                     if (sctx->force_cb_shader_coherent || si_check_needs_implicit_sync(sctx))
      si_make_CB_shader_coherent(sctx, 0,
                  if (sctx->gfx_level < GFX11)
         else
               if (info->indirect) {
      /* Indirect buffers use TC L2 on GFX9, but not older hw. */
   if (sctx->gfx_level <= GFX8 && si_resource(info->indirect)->TC_L2_dirty) {
      sctx->flags |= SI_CONTEXT_WB_L2;
   si_mark_atom_dirty(sctx, &sctx->atoms.s.cache_flush);
                           /* If we're using a secure context, determine if cs must be secure or not */
   if (unlikely(radeon_uses_secure_bos(sctx->ws))) {
      bool secure = si_compute_resources_check_encrypted(sctx);
   if (secure != sctx->ws->cs_is_secure(&sctx->gfx_cs)) {
      si_flush_gfx_cs(sctx, RADEON_FLUSH_ASYNC_START_NEXT_GFX_IB_NOW |
               }
      if (u_trace_perfetto_active(&sctx->ds.trace_context))
            if (sctx->bo_list_add_all_compute_resources)
            /* Skipping setting redundant registers on compute queues breaks compute. */
   if (!sctx->has_graphics)
            /* First emit registers. */
   bool prefetch;
   if (!si_switch_compute_shader(sctx, program, &program->shader, code_object, info->pc, &prefetch,
                           if (program->ir_type == PIPE_SHADER_IR_NATIVE &&
      unlikely(!si_upload_compute_input(sctx, code_object, info)))
         /* Global buffers */
   for (i = 0; i < program->max_global_buffers; i++) {
      struct si_resource *buffer = si_resource(program->global_buffers[i]);
   if (!buffer) {
         }
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, buffer,
               /* Registers that are not read from memory should be set before this: */
   if (sctx->flags)
            if (sctx->has_graphics && si_is_atom_dirty(sctx, &sctx->atoms.s.render_cond)) {
      sctx->atoms.s.render_cond.emit(sctx, -1);
               /* Prefetch the compute shader to L2. */
   if (sctx->gfx_level >= GFX7 && prefetch)
            if (program->ir_type != PIPE_SHADER_IR_NATIVE)
                     if (unlikely(sctx->current_saved_cs)) {
      si_trace_emit(sctx);
               /* Mark displayable DCC as dirty for bound images. */
   unsigned display_dcc_store_mask = sctx->images[PIPE_SHADER_COMPUTE].display_dcc_store_mask &
         while (display_dcc_store_mask) {
      struct si_texture *tex = (struct si_texture *)
                                 sctx->compute_is_busy = true;
            if (u_trace_perfetto_active(&sctx->ds.trace_context))
            if (cs_regalloc_hang) {
      sctx->flags |= SI_CONTEXT_CS_PARTIAL_FLUSH;
         }
      void si_destroy_compute(struct si_compute *program)
   {
               if (program->ir_type != PIPE_SHADER_IR_NATIVE) {
      util_queue_drop_job(&sel->screen->shader_compiler_queue, &sel->ready);
               for (unsigned i = 0; i < program->max_global_buffers; i++)
                  si_shader_destroy(&program->shader);
   ralloc_free(program->sel.nir);
      }
      static void si_delete_compute_state(struct pipe_context *ctx, void *state)
   {
      struct si_compute *program = (struct si_compute *)state;
            if (!state)
            if (program == sctx->cs_shader_state.program)
            if (program == sctx->cs_shader_state.emitted_program)
               }
      static void si_set_compute_resources(struct pipe_context *ctx_, unsigned start, unsigned count,
         {
   }
      void si_init_compute_functions(struct si_context *sctx)
   {
      sctx->b.create_compute_state = si_create_compute_state;
   sctx->b.delete_compute_state = si_delete_compute_state;
   sctx->b.bind_compute_state = si_bind_compute_state;
   sctx->b.get_compute_state_info = si_get_compute_state_info;
   sctx->b.set_compute_resources = si_set_compute_resources;
   sctx->b.set_global_binding = si_set_global_binding;
      }
