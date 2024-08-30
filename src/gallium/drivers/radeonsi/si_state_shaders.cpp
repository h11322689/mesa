   /*
   * Copyright 2012 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_llvm_util.h"
   #include "ac_nir.h"
   #include "ac_shader_util.h"
   #include "compiler/nir/nir_serialize.h"
   #include "nir/tgsi_to_nir.h"
   #include "si_build_pm4.h"
   #include "sid.h"
   #include "util/crc32.h"
   #include "util/disk_cache.h"
   #include "util/hash_table.h"
   #include "util/mesa-sha1.h"
   #include "util/u_async_debug.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "tgsi/tgsi_from_mesa.h"
      static void si_update_tess_in_out_patch_vertices(struct si_context *sctx);
      unsigned si_determine_wave_size(struct si_screen *sscreen, struct si_shader *shader)
   {
      /* There are a few uses that pass shader=NULL here, expecting the default compute wave size. */
   struct si_shader_info *info = shader ? &shader->selector->info : NULL;
            if (sscreen->info.gfx_level < GFX10)
            /* Legacy GS only supports Wave64. */
   if ((stage == MESA_SHADER_VERTEX && shader->key.ge.as_es && !shader->key.ge.as_ngg) ||
      (stage == MESA_SHADER_TESS_EVAL && shader->key.ge.as_es && !shader->key.ge.as_ngg) ||
   (stage == MESA_SHADER_GEOMETRY && !shader->key.ge.as_ngg))
         /* Workgroup sizes that are not divisible by 64 use Wave32. */
   if (stage == MESA_SHADER_COMPUTE && info && !info->base.workgroup_size_variable &&
      (info->base.workgroup_size[0] *
   info->base.workgroup_size[1] *
   info->base.workgroup_size[2]) % 64 != 0)
         /* Debug flags. */
   unsigned dbg_wave_size = 0;
   if (sscreen->debug_flags &
      (stage == MESA_SHADER_COMPUTE ? DBG(W32_CS) :
   stage == MESA_SHADER_FRAGMENT ? DBG(W32_PS) | DBG(W32_PS_DISCARD) : DBG(W32_GE)))
         if (sscreen->debug_flags &
      (stage == MESA_SHADER_COMPUTE ? DBG(W64_CS) :
   stage == MESA_SHADER_FRAGMENT ? DBG(W64_PS) : DBG(W64_GE))) {
   assert(!dbg_wave_size);
               /* Shader profiles. */
   unsigned profile_wave_size = 0;
   if (info && info->options & SI_PROFILE_WAVE32)
            if (info && info->options & SI_PROFILE_WAVE64) {
      assert(!profile_wave_size);
               if (profile_wave_size) {
      /* Only debug flags override shader profiles. */
   if (dbg_wave_size)
                        /* Debug flags except w32psdiscard don't override the discard bug workaround,
   * but they override everything else.
   */
   if (dbg_wave_size)
            /* Pixel shaders without interp instructions don't suffer from reduced interpolation
   * performance in Wave32, so use Wave32. This helps Piano and Voloplosion.
   */
   if (stage == MESA_SHADER_FRAGMENT && !info->num_inputs)
            /* There are a few very rare cases where VS is better with Wave32, and there are no known
   * cases where Wave64 is better.
   * Wave32 is disabled for GFX10 when culling is active as a workaround for #6457. I don't
   * know why this helps.
   */
   if (stage <= MESA_SHADER_GEOMETRY &&
      !(sscreen->info.gfx_level == GFX10 && shader && shader->key.ge.opt.ngg_culling))
         /* TODO: Merged shaders must use the same wave size because the driver doesn't recompile
   * individual shaders of merged shaders to match the wave size between them.
   */
   bool merged_shader = stage <= MESA_SHADER_GEOMETRY && shader && !shader->is_gs_copy_shader &&
                  /* Divergent loops in Wave64 can end up having too many iterations in one half of the wave
   * while the other half is idling but occupying VGPRs, preventing other waves from launching.
   * Wave32 eliminates the idling half to allow the next wave to start.
   */
   if (!merged_shader && info && info->has_divergent_loop)
               }
      /* SHADER_CACHE */
      /**
   * Return the IR key for the shader cache.
   */
   void si_get_ir_cache_key(struct si_shader_selector *sel, bool ngg, bool es,
         {
      struct blob blob = {};
   unsigned ir_size;
            if (sel->nir_binary) {
      ir_binary = sel->nir_binary;
      } else {
               blob_init(&blob);
   /* Keep debug info if NIR debug prints are in use. */
   nir_serialize(&blob, sel->nir, NIR_DEBUG(PRINT) == 0);
   ir_binary = blob.data;
               /* These settings affect the compilation, but they are not derived
   * from the input shader IR.
   */
            if (ngg)
         if (sel->nir)
         if (wave_size == 32)
                     /* use_ngg_culling disables NGG passthrough for non-culling shaders to reduce context
   * rolls, which can be changed with AMD_DEBUG=nonggc or AMD_DEBUG=nggc.
   */
   if (sel->screen->use_ngg_culling)
         if (sel->screen->record_llvm_ir)
         if (sel->screen->info.has_image_opcodes)
         if (sel->screen->options.no_infinite_interp)
         if (sel->screen->options.clamp_div_by_zero)
         if ((sel->stage == MESA_SHADER_VERTEX ||
      sel->stage == MESA_SHADER_TESS_EVAL ||
   sel->stage == MESA_SHADER_GEOMETRY) &&
   !es &&
   sel->screen->options.vrs2x2)
      if (sel->screen->options.inline_uniforms)
            struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);
   _mesa_sha1_update(&ctx, &shader_variant_flags, 4);
   _mesa_sha1_update(&ctx, ir_binary, ir_size);
            if (ir_binary == blob.data)
      }
      /** Copy "data" to "ptr" and return the next dword following copied data. */
   static uint32_t *write_data(uint32_t *ptr, const void *data, unsigned size)
   {
      /* data may be NULL if size == 0 */
   if (size)
         ptr += DIV_ROUND_UP(size, 4);
      }
      /** Read data from "ptr". Return the next dword following the data. */
   static uint32_t *read_data(uint32_t *ptr, void *data, unsigned size)
   {
      memcpy(data, ptr, size);
   ptr += DIV_ROUND_UP(size, 4);
      }
      /**
   * Write the size as uint followed by the data. Return the next dword
   * following the copied data.
   */
   static uint32_t *write_chunk(uint32_t *ptr, const void *data, unsigned size)
   {
      *ptr++ = size;
      }
      /**
   * Read the size as uint followed by the data. Return both via parameters.
   * Return the next dword following the data.
   */
   static uint32_t *read_chunk(uint32_t *ptr, void **data, unsigned *size)
   {
      *size = *ptr++;
   assert(*data == NULL);
   if (!*size)
         *data = malloc(*size);
      }
      struct si_shader_blob_head {
      uint32_t size;
   uint32_t type;
      };
      /**
   * Return the shader binary in a buffer.
   */
   static uint32_t *si_get_shader_binary(struct si_shader *shader)
   {
      /* There is always a size of data followed by the data itself. */
   unsigned llvm_ir_size =
            /* Refuse to allocate overly large buffers and guard against integer
   * overflow. */
   if (shader->binary.code_size > UINT_MAX / 4 || llvm_ir_size > UINT_MAX / 4 ||
      shader->binary.num_symbols > UINT_MAX / 32)
         unsigned size = sizeof(struct si_shader_blob_head) +
                  align(sizeof(shader->config), 4) +
   align(sizeof(shader->info), 4) +
      uint32_t *buffer = (uint32_t*)CALLOC(1, size);
   if (!buffer)
            struct si_shader_blob_head *head = (struct si_shader_blob_head *)buffer;
   head->type = shader->binary.type;
            uint32_t *data = buffer + sizeof(*head) / 4;
            ptr = write_data(ptr, &shader->config, sizeof(shader->config));
   ptr = write_data(ptr, &shader->info, sizeof(shader->info));
   ptr = write_data(ptr, &shader->binary.exec_size, 4);
   ptr = write_chunk(ptr, shader->binary.code_buffer, shader->binary.code_size);
   ptr = write_chunk(ptr, shader->binary.symbols, shader->binary.num_symbols * 8);
   ptr = write_chunk(ptr, shader->binary.llvm_ir_string, llvm_ir_size);
            /* Compute CRC32. */
               }
      static bool si_load_shader_binary(struct si_shader *shader, void *binary)
   {
      struct si_shader_blob_head *head = (struct si_shader_blob_head *)binary;
   unsigned chunk_size;
            uint32_t *ptr = (uint32_t *)binary + sizeof(*head) / 4;
   if (util_hash_crc32(ptr, head->size - sizeof(*head)) != head->crc32) {
      fprintf(stderr, "radeonsi: binary shader has invalid CRC32\n");
               shader->binary.type = (enum si_shader_binary_type)head->type;
   ptr = read_data(ptr, &shader->config, sizeof(shader->config));
   ptr = read_data(ptr, &shader->info, sizeof(shader->info));
   ptr = read_data(ptr, &shader->binary.exec_size, 4);
   ptr = read_chunk(ptr, (void **)&shader->binary.code_buffer, &code_size);
   shader->binary.code_size = code_size;
   ptr = read_chunk(ptr, (void **)&shader->binary.symbols, &chunk_size);
   shader->binary.num_symbols = chunk_size / 8;
            if (!shader->is_gs_copy_shader &&
      shader->selector->stage == MESA_SHADER_GEOMETRY && !shader->key.ge.as_ngg) {
   shader->gs_copy_shader = CALLOC_STRUCT(si_shader);
   if (!shader->gs_copy_shader)
                     if (!si_load_shader_binary(shader->gs_copy_shader, (uint8_t*)binary + head->size)) {
      FREE(shader->gs_copy_shader);
   shader->gs_copy_shader = NULL;
               util_queue_fence_init(&shader->gs_copy_shader->ready);
   shader->gs_copy_shader->selector = shader->selector;
   shader->gs_copy_shader->is_gs_copy_shader = true;
   shader->gs_copy_shader->wave_size =
                           }
      /**
   * Insert a shader into the cache. It's assumed the shader is not in the cache.
   * Use si_shader_cache_load_shader before calling this.
   */
   void si_shader_cache_insert_shader(struct si_screen *sscreen, unsigned char ir_sha1_cache_key[20],
         {
      uint32_t *hw_binary;
   struct hash_entry *entry;
   uint8_t key[CACHE_KEY_SIZE];
            if (!insert_into_disk_cache && memory_cache_full)
            entry = _mesa_hash_table_search(sscreen->shader_cache, ir_sha1_cache_key);
   if (entry)
            hw_binary = si_get_shader_binary(shader);
   if (!hw_binary)
                     if (shader->selector->stage == MESA_SHADER_GEOMETRY && !shader->key.ge.as_ngg) {
      uint32_t *gs_copy_binary = si_get_shader_binary(shader->gs_copy_shader);
   if (!gs_copy_binary) {
      FREE(hw_binary);
               /* Combine both binaries. */
   size += *gs_copy_binary;
   uint32_t *combined_binary = (uint32_t*)MALLOC(size);
   if (!combined_binary) {
      FREE(hw_binary);
   FREE(gs_copy_binary);
               memcpy(combined_binary, hw_binary, *hw_binary);
   memcpy(combined_binary + *hw_binary / 4, gs_copy_binary, *gs_copy_binary);
   FREE(hw_binary);
   FREE(gs_copy_binary);
               if (!memory_cache_full) {
      if (_mesa_hash_table_insert(sscreen->shader_cache,
                  FREE(hw_binary);
                           if (sscreen->disk_shader_cache && insert_into_disk_cache) {
      disk_cache_compute_key(sscreen->disk_shader_cache, ir_sha1_cache_key, 20, key);
               if (memory_cache_full)
      }
      bool si_shader_cache_load_shader(struct si_screen *sscreen, unsigned char ir_sha1_cache_key[20],
         {
               if (entry) {
      if (si_load_shader_binary(shader, entry->data)) {
      p_atomic_inc(&sscreen->num_memory_shader_cache_hits);
         }
            if (!sscreen->disk_shader_cache)
            unsigned char sha1[CACHE_KEY_SIZE];
            size_t total_size;
   uint32_t *buffer = (uint32_t*)disk_cache_get(sscreen->disk_shader_cache, sha1, &total_size);
   if (buffer) {
      unsigned size = *buffer;
            /* The GS copy shader binary is after the GS binary. */
   if (shader->selector->stage == MESA_SHADER_GEOMETRY && !shader->key.ge.as_ngg)
            if (total_size >= sizeof(uint32_t) && size + gs_copy_binary_size == total_size) {
      if (si_load_shader_binary(shader, buffer)) {
      free(buffer);
   si_shader_cache_insert_shader(sscreen, ir_sha1_cache_key, shader, false);
   p_atomic_inc(&sscreen->num_disk_shader_cache_hits);
         } else {
      /* Something has gone wrong discard the item from the cache and
   * rebuild/link from source.
   */
   assert(!"Invalid radeonsi shader disk cache item!");
                  free(buffer);
   p_atomic_inc(&sscreen->num_disk_shader_cache_misses);
      }
      static uint32_t si_shader_cache_key_hash(const void *key)
   {
      /* Take the first dword of SHA1. */
      }
      static bool si_shader_cache_key_equals(const void *a, const void *b)
   {
      /* Compare SHA1s. */
      }
      static void si_destroy_shader_cache_entry(struct hash_entry *entry)
   {
      FREE((void *)entry->key);
      }
      bool si_init_shader_cache(struct si_screen *sscreen)
   {
      (void)simple_mtx_init(&sscreen->shader_cache_mutex, mtx_plain);
   sscreen->shader_cache =
         sscreen->shader_cache_size = 0;
   /* Maximum size: 64MB on 32 bits, 1GB else */
               }
      void si_destroy_shader_cache(struct si_screen *sscreen)
   {
      if (sscreen->shader_cache)
            }
      /* SHADER STATES */
      unsigned si_shader_encode_vgprs(struct si_shader *shader)
   {
      assert(shader->selector->screen->info.gfx_level >= GFX10 || shader->wave_size == 64);
      }
      unsigned si_shader_encode_sgprs(struct si_shader *shader)
   {
      if (shader->selector->screen->info.gfx_level >= GFX10)
               }
      bool si_shader_mem_ordered(struct si_shader *shader)
   {
      if (shader->selector->screen->info.gfx_level < GFX10)
            /* Return true if both types of VMEM that return something are used. */
   return shader->info.uses_vmem_sampler_or_bvh &&
            }
      static void si_set_tesseval_regs(struct si_screen *sscreen, const struct si_shader_selector *tes,
         {
      const struct si_shader_info *info = &tes->info;
   enum tess_primitive_mode tes_prim_mode = info->base.tess._primitive_mode;
   unsigned tes_spacing = info->base.tess.spacing;
   bool tes_vertex_order_cw = !info->base.tess.ccw;
   bool tes_point_mode = info->base.tess.point_mode;
            switch (tes_prim_mode) {
   case TESS_PRIMITIVE_ISOLINES:
      type = V_028B6C_TESS_ISOLINE;
      case TESS_PRIMITIVE_TRIANGLES:
      type = V_028B6C_TESS_TRIANGLE;
      case TESS_PRIMITIVE_QUADS:
      type = V_028B6C_TESS_QUAD;
      default:
      assert(0);
               switch (tes_spacing) {
   case TESS_SPACING_FRACTIONAL_ODD:
      partitioning = V_028B6C_PART_FRAC_ODD;
      case TESS_SPACING_FRACTIONAL_EVEN:
      partitioning = V_028B6C_PART_FRAC_EVEN;
      case TESS_SPACING_EQUAL:
      partitioning = V_028B6C_PART_INTEGER;
      default:
      assert(0);
               if (tes_point_mode)
         else if (tes_prim_mode == TESS_PRIMITIVE_ISOLINES)
         else if (tes_vertex_order_cw)
      /* for some reason, this must be the other way around */
      else
            if (sscreen->info.has_distributed_tess) {
      if (sscreen->info.family == CHIP_FIJI || sscreen->info.family >= CHIP_POLARIS10)
         else
      } else
            shader->vgt_tf_param = S_028B6C_TYPE(type) | S_028B6C_PARTITIONING(partitioning) |
            }
      /* Polaris needs different VTX_REUSE_DEPTH settings depending on
   * whether the "fractional odd" tessellation spacing is used.
   *
   * Possible VGT configurations and which state should set the register:
   *
   *   Reg set in | VGT shader configuration   | Value
   * ------------------------------------------------------
   *     VS as VS | VS                         | 30
   *     VS as ES | ES -> GS -> VS             | 30
   *    TES as VS | LS -> HS -> VS             | 14 or 30
   *    TES as ES | LS -> HS -> ES -> GS -> VS | 14 or 30
   */
   static void polaris_set_vgt_vertex_reuse(struct si_screen *sscreen, struct si_shader_selector *sel,
         {
      if (sscreen->info.family < CHIP_POLARIS10 || sscreen->info.gfx_level >= GFX10)
            /* VS as VS, or VS as ES: */
   if ((sel->stage == MESA_SHADER_VERTEX &&
      (!shader->key.ge.as_ls && !shader->is_gs_copy_shader)) ||
   /* TES as VS, or TES as ES: */
   sel->stage == MESA_SHADER_TESS_EVAL) {
            if (sel->stage == MESA_SHADER_TESS_EVAL &&
                        }
      static struct si_pm4_state *
   si_get_shader_pm4_state(struct si_shader *shader,
         {
      si_pm4_clear_state(&shader->pm4, shader->selector->screen, false);
   shader->pm4.atom.emit = emit_func;
      }
      static unsigned si_get_num_vs_user_sgprs(struct si_shader *shader,
         {
      struct si_shader_selector *vs =
                  /* 1 SGPR is reserved for the vertex buffer pointer. */
            if (num_vbos_in_user_sgprs)
            /* Add the pointer to VBO descriptors. */
      }
      /* Return VGPR_COMP_CNT for the API vertex shader. This can be hw LS, LSHS, ES, ESGS, VS. */
   static unsigned si_get_vs_vgpr_comp_cnt(struct si_screen *sscreen, struct si_shader *shader,
         {
      assert(shader->selector->stage == MESA_SHADER_VERTEX ||
            /* GFX6-9   LS    (VertexID, RelAutoIndex,           InstanceID / StepRate0, InstanceID)
   * GFX6-9   ES,VS (VertexID, InstanceID / StepRate0, VSPrimID,               InstanceID)
   * GFX10-11 LS    (VertexID, RelAutoIndex,           UserVGPR1,              UserVGPR2 or InstanceID)
   * GFX10-11 ES,VS (VertexID, UserVGPR1,              UserVGPR2 or VSPrimID,  UserVGPR3 or InstanceID)
   */
   bool is_ls = shader->selector->stage == MESA_SHADER_TESS_CTRL || shader->key.ge.as_ls;
            if (shader->info.uses_instanceid) {
      if (sscreen->info.gfx_level >= GFX10)
         else if (is_ls)
         else
               if (legacy_vs_prim_id)
            /* GFX11: We prefer to compute RelAutoIndex using (WaveID * WaveSize + ThreadID).
   * Older chips didn't have WaveID in LS.
   */
   if (is_ls && sscreen->info.gfx_level <= GFX10_3)
               }
      unsigned si_get_shader_prefetch_size(struct si_shader *shader)
   {
      /* inst_pref_size is calculated in cache line size granularity */
   assert(!(shader->bo->b.b.width0 & 0x7f));
      }
      static void si_shader_ls(struct si_screen *sscreen, struct si_shader *shader)
   {
      struct si_pm4_state *pm4;
                     pm4 = si_get_shader_pm4_state(shader, NULL);
   if (!pm4)
            va = shader->bo->gpu_address;
            shader->config.rsrc1 = S_00B528_VGPRS(si_shader_encode_vgprs(shader)) |
                           shader->config.rsrc2 = S_00B52C_USER_SGPR(si_get_num_vs_user_sgprs(shader, SI_VS_NUM_USER_SGPR)) |
            }
      static void si_shader_hs(struct si_screen *sscreen, struct si_shader *shader)
   {
      struct si_pm4_state *pm4 = si_get_shader_pm4_state(shader, NULL);
   if (!pm4)
            uint64_t va = shader->bo->gpu_address;
   unsigned num_user_sgprs = sscreen->info.gfx_level >= GFX9 ?
                  if (sscreen->info.gfx_level >= GFX11) {
      si_pm4_set_reg_idx3(pm4, R_00B404_SPI_SHADER_PGM_RSRC4_HS,
                           } else if (sscreen->info.gfx_level >= GFX10) {
         } else if (sscreen->info.gfx_level >= GFX9) {
         } else {
      si_pm4_set_reg(pm4, R_00B420_SPI_SHADER_PGM_LO_HS, va >> 8);
   si_pm4_set_reg(pm4, R_00B424_SPI_SHADER_PGM_HI_HS,
               si_pm4_set_reg(pm4, R_00B428_SPI_SHADER_PGM_RSRC1_HS,
                  S_00B428_VGPRS(si_shader_encode_vgprs(shader)) |
   S_00B428_SGPRS(si_shader_encode_sgprs(shader)) |
   S_00B428_DX10_CLAMP(1) |
   S_00B428_MEM_ORDERED(si_shader_mem_ordered(shader)) |
         shader->config.rsrc2 = S_00B42C_SCRATCH_EN(shader->config.scratch_bytes_per_wave > 0) |
            if (sscreen->info.gfx_level >= GFX10)
         else if (sscreen->info.gfx_level >= GFX9)
         else
            if (sscreen->info.gfx_level <= GFX8)
               }
      static void si_emit_shader_es(struct si_context *sctx, unsigned index)
   {
               radeon_begin(&sctx->gfx_cs);
   radeon_opt_set_context_reg(sctx, R_028AAC_VGT_ESGS_RING_ITEMSIZE,
                  if (shader->selector->stage == MESA_SHADER_TESS_EVAL)
      radeon_opt_set_context_reg(sctx, R_028B6C_VGT_TF_PARAM, SI_TRACKED_VGT_TF_PARAM,
         if (shader->vgt_vertex_reuse_block_cntl)
      radeon_opt_set_context_reg(sctx, R_028C58_VGT_VERTEX_REUSE_BLOCK_CNTL,
               }
      static void si_shader_es(struct si_screen *sscreen, struct si_shader *shader)
   {
      struct si_pm4_state *pm4;
   unsigned num_user_sgprs;
   unsigned vgpr_comp_cnt;
   uint64_t va;
                     pm4 = si_get_shader_pm4_state(shader, si_emit_shader_es);
   if (!pm4)
                     if (shader->selector->stage == MESA_SHADER_VERTEX) {
      vgpr_comp_cnt = si_get_vs_vgpr_comp_cnt(sscreen, shader, false);
      } else if (shader->selector->stage == MESA_SHADER_TESS_EVAL) {
      vgpr_comp_cnt = shader->selector->info.uses_primid ? 3 : 2;
      } else
                     si_pm4_set_reg(pm4, R_00B320_SPI_SHADER_PGM_LO_ES, va >> 8);
   si_pm4_set_reg(pm4, R_00B324_SPI_SHADER_PGM_HI_ES,
         si_pm4_set_reg(pm4, R_00B328_SPI_SHADER_PGM_RSRC1_ES,
                  S_00B328_VGPRS(si_shader_encode_vgprs(shader)) |
   S_00B328_SGPRS(si_shader_encode_sgprs(shader)) |
      si_pm4_set_reg(pm4, R_00B32C_SPI_SHADER_PGM_RSRC2_ES,
                  if (shader->selector->stage == MESA_SHADER_TESS_EVAL)
            polaris_set_vgt_vertex_reuse(sscreen, shader->selector, shader);
      }
      void gfx9_get_gs_info(struct si_shader_selector *es, struct si_shader_selector *gs,
         {
      unsigned gs_num_invocations = MAX2(gs->info.base.gs.invocations, 1);
   unsigned input_prim = gs->info.base.gs.input_primitive;
   bool uses_adjacency =
            /* All these are in dwords: */
   /* We can't allow using the whole LDS, because GS waves compete with
   * other shader stages for LDS space. */
   const unsigned max_lds_size = 8 * 1024;
   const unsigned esgs_itemsize = es->info.esgs_vertex_stride / 4;
            /* All these are per subgroup: */
   const unsigned max_out_prims = 32 * 1024;
   const unsigned max_es_verts = 255;
   const unsigned ideal_gs_prims = 64;
   unsigned max_gs_prims, gs_prims;
            if (uses_adjacency || gs_num_invocations > 1)
         else
            /* MAX_PRIMS_PER_SUBGROUP = gs_prims * max_vert_out * gs_invocations.
   * Make sure we don't go over the maximum value.
   */
   if (gs->info.base.gs.vertices_out > 0) {
      max_gs_prims =
      }
            /* If the primitive has adjacency, halve the number of vertices
   * that will be reused in multiple primitives.
   */
            gs_prims = MIN2(ideal_gs_prims, max_gs_prims);
            /* Compute ESGS LDS size based on the worst case number of ES vertices
   * needed to create the target number of GS prims per subgroup.
   */
            /* If total LDS usage is too big, refactor partitions based on ratio
   * of ESGS item sizes.
   */
   if (esgs_lds_size > max_lds_size) {
      /* Our target GS Prims Per Subgroup was too large. Calculate
   * the maximum number of GS Prims Per Subgroup that will fit
   * into LDS, capped by the maximum that the hardware can support.
   */
   gs_prims = MIN2((max_lds_size / (esgs_itemsize * min_es_verts)), max_gs_prims);
   assert(gs_prims > 0);
            esgs_lds_size = esgs_itemsize * worst_case_es_verts;
               /* Now calculate remaining ESGS information. */
   if (esgs_lds_size)
         else
            /* Vertices for adjacency primitives are not always reused, so restore
   * it for ES_VERTS_PER_SUBGRP.
   */
            /* For normal primitives, the VGT only checks if they are past the ES
   * verts per subgroup after allocating a full GS primitive and if they
   * are, kick off a new subgroup.  But if those additional ES verts are
   * unique (e.g. not reused) we need to make sure there is enough LDS
   * space to account for those ES verts beyond ES_VERTS_PER_SUBGRP.
   */
            out->es_verts_per_subgroup = es_verts;
   out->gs_prims_per_subgroup = gs_prims;
   out->gs_inst_prims_in_subgroup = gs_prims * gs_num_invocations;
   out->max_prims_per_subgroup = out->gs_inst_prims_in_subgroup * gs->info.base.gs.vertices_out;
               }
      static void si_emit_shader_gs(struct si_context *sctx, unsigned index)
   {
               if (sctx->gfx_level >= GFX9) {
      SET_FIELD(sctx->current_gs_state, GS_STATE_ESGS_VERTEX_STRIDE,
                        /* R_028A60_VGT_GSVS_RING_OFFSET_1, R_028A64_VGT_GSVS_RING_OFFSET_2
   * R_028A68_VGT_GSVS_RING_OFFSET_3 */
   radeon_opt_set_context_reg3(
      sctx, R_028A60_VGT_GSVS_RING_OFFSET_1, SI_TRACKED_VGT_GSVS_RING_OFFSET_1,
   shader->gs.vgt_gsvs_ring_offset_1, shader->gs.vgt_gsvs_ring_offset_2,
         /* R_028AB0_VGT_GSVS_RING_ITEMSIZE */
   radeon_opt_set_context_reg(sctx, R_028AB0_VGT_GSVS_RING_ITEMSIZE,
                  /* R_028B38_VGT_GS_MAX_VERT_OUT */
   radeon_opt_set_context_reg(sctx, R_028B38_VGT_GS_MAX_VERT_OUT, SI_TRACKED_VGT_GS_MAX_VERT_OUT,
            /* R_028B5C_VGT_GS_VERT_ITEMSIZE, R_028B60_VGT_GS_VERT_ITEMSIZE_1
   * R_028B64_VGT_GS_VERT_ITEMSIZE_2, R_028B68_VGT_GS_VERT_ITEMSIZE_3 */
   radeon_opt_set_context_reg4(
      sctx, R_028B5C_VGT_GS_VERT_ITEMSIZE, SI_TRACKED_VGT_GS_VERT_ITEMSIZE,
   shader->gs.vgt_gs_vert_itemsize, shader->gs.vgt_gs_vert_itemsize_1,
         /* R_028B90_VGT_GS_INSTANCE_CNT */
   radeon_opt_set_context_reg(sctx, R_028B90_VGT_GS_INSTANCE_CNT, SI_TRACKED_VGT_GS_INSTANCE_CNT,
            if (sctx->gfx_level >= GFX9) {
      /* R_028A44_VGT_GS_ONCHIP_CNTL */
   radeon_opt_set_context_reg(sctx, R_028A44_VGT_GS_ONCHIP_CNTL, SI_TRACKED_VGT_GS_ONCHIP_CNTL,
         /* R_028A94_VGT_GS_MAX_PRIMS_PER_SUBGROUP */
   radeon_opt_set_context_reg(sctx, R_028A94_VGT_GS_MAX_PRIMS_PER_SUBGROUP,
                  if (shader->key.ge.part.gs.es->stage == MESA_SHADER_TESS_EVAL)
      radeon_opt_set_context_reg(sctx, R_028B6C_VGT_TF_PARAM, SI_TRACKED_VGT_TF_PARAM,
      if (shader->vgt_vertex_reuse_block_cntl)
      radeon_opt_set_context_reg(sctx, R_028C58_VGT_VERTEX_REUSE_BLOCK_CNTL,
         }
            /* These don't cause any context rolls. */
   radeon_begin_again(&sctx->gfx_cs);
   if (sctx->gfx_level >= GFX7) {
      radeon_opt_set_sh_reg_idx(sctx, R_00B21C_SPI_SHADER_PGM_RSRC3_GS,
            }
   if (sctx->gfx_level >= GFX10) {
      radeon_opt_set_sh_reg_idx(sctx, R_00B204_SPI_SHADER_PGM_RSRC4_GS,
            }
      }
      static void si_shader_gs(struct si_screen *sscreen, struct si_shader *shader)
   {
      struct si_shader_selector *sel = shader->selector;
   const uint8_t *num_components = sel->info.num_stream_output_components;
   unsigned gs_num_invocations = sel->info.base.gs.invocations;
   struct si_pm4_state *pm4;
   uint64_t va;
   unsigned max_stream = util_last_bit(sel->info.base.gs.active_stream_mask);
                     pm4 = si_get_shader_pm4_state(shader, si_emit_shader_gs);
   if (!pm4)
            offset = num_components[0] * sel->info.base.gs.vertices_out;
            if (max_stream >= 2)
                  if (max_stream >= 3)
                  if (max_stream >= 4)
                  /* The GSVS_RING_ITEMSIZE register takes 15 bits */
                     shader->gs.vgt_gs_vert_itemsize = num_components[0];
   shader->gs.vgt_gs_vert_itemsize_1 = (max_stream >= 2) ? num_components[1] : 0;
   shader->gs.vgt_gs_vert_itemsize_2 = (max_stream >= 3) ? num_components[2] : 0;
            shader->gs.vgt_gs_instance_cnt =
            /* Copy over fields from the GS copy shader to make them easily accessible from GS. */
                     if (sscreen->info.gfx_level >= GFX9) {
      unsigned input_prim = sel->info.base.gs.input_primitive;
   gl_shader_stage es_stage = shader->key.ge.part.gs.es->stage;
            if (es_stage == MESA_SHADER_VERTEX) {
         } else if (es_stage == MESA_SHADER_TESS_EVAL)
         else
            /* If offsets 4, 5 are used, GS_VGPR_COMP_CNT is ignored and
   * VGPR[0:4] are always loaded.
   */
   if (sel->info.uses_invocationid)
         else if (sel->info.uses_primid)
         else if (input_prim >= MESA_PRIM_TRIANGLES)
         else
            unsigned num_user_sgprs;
   if (es_stage == MESA_SHADER_VERTEX)
         else
            if (sscreen->info.gfx_level >= GFX10) {
         } else {
                  uint32_t rsrc1 = S_00B228_VGPRS(si_shader_encode_vgprs(shader)) |
                  S_00B228_SGPRS(si_shader_encode_sgprs(shader)) |
   S_00B228_DX10_CLAMP(1) |
      uint32_t rsrc2 = S_00B22C_USER_SGPR(num_user_sgprs) |
                              if (sscreen->info.gfx_level >= GFX10) {
         } else {
                  si_pm4_set_reg(pm4, R_00B228_SPI_SHADER_PGM_RSRC1_GS, rsrc1);
            shader->gs.spi_shader_pgm_rsrc3_gs =
      ac_apply_cu_en(S_00B21C_CU_EN(0xffff) |
            shader->gs.spi_shader_pgm_rsrc4_gs =
      ac_apply_cu_en(S_00B204_CU_EN_GFX10(0xffff) |
               shader->gs.vgt_gs_onchip_cntl =
      S_028A44_ES_VERTS_PER_SUBGRP(shader->gs_info.es_verts_per_subgroup) |
   S_028A44_GS_PRIMS_PER_SUBGRP(shader->gs_info.gs_prims_per_subgroup) |
      shader->gs.vgt_gs_max_prims_per_subgroup =
                  if (es_stage == MESA_SHADER_TESS_EVAL)
               } else {
      shader->gs.spi_shader_pgm_rsrc3_gs =
      ac_apply_cu_en(S_00B21C_CU_EN(0xffff) |
               si_pm4_set_reg(pm4, R_00B220_SPI_SHADER_PGM_LO_GS, va >> 8);
   si_pm4_set_reg(pm4, R_00B224_SPI_SHADER_PGM_HI_GS,
            si_pm4_set_reg(pm4, R_00B228_SPI_SHADER_PGM_RSRC1_GS,
                  S_00B228_VGPRS(si_shader_encode_vgprs(shader)) |
      si_pm4_set_reg(pm4, R_00B22C_SPI_SHADER_PGM_RSRC2_GS,
            }
      }
      bool gfx10_is_ngg_passthrough(struct si_shader *shader)
   {
               /* Never use NGG passthrough if culling is possible even when it's not used by this shader,
   * so that we don't get context rolls when enabling and disabling NGG passthrough.
   */
   if (sel->screen->use_ngg_culling)
            /* The definition of NGG passthrough is:
   * - user GS is turned off (no amplification, no GS instancing, and no culling)
   * - VGT_ESGS_RING_ITEMSIZE is ignored (behaving as if it was equal to 1)
   * - vertex indices are packed into 1 VGPR
   * - Navi23 and later chips can optionally skip the gs_alloc_req message
   *
   * NGG passthrough still allows the use of LDS.
   */
      }
      /* Common tail code for NGG primitive shaders. */
   static void gfx10_emit_shader_ngg(struct si_context *sctx, unsigned index)
   {
               SET_FIELD(sctx->current_gs_state, GS_STATE_ESGS_VERTEX_STRIDE,
            radeon_begin(&sctx->gfx_cs);
   radeon_opt_set_context_reg(sctx, R_0287FC_GE_MAX_OUTPUT_PER_SUBGROUP,
               radeon_opt_set_context_reg(sctx, R_028B4C_GE_NGG_SUBGRP_CNTL, SI_TRACKED_GE_NGG_SUBGRP_CNTL,
         radeon_opt_set_context_reg(sctx, R_028A84_VGT_PRIMITIVEID_EN, SI_TRACKED_VGT_PRIMITIVEID_EN,
         if (sctx->gfx_level < GFX11) {
      radeon_opt_set_context_reg(sctx, R_028A44_VGT_GS_ONCHIP_CNTL, SI_TRACKED_VGT_GS_ONCHIP_CNTL,
      }
   radeon_opt_set_context_reg(sctx, R_028B38_VGT_GS_MAX_VERT_OUT, SI_TRACKED_VGT_GS_MAX_VERT_OUT,
         radeon_opt_set_context_reg(sctx, R_028B90_VGT_GS_INSTANCE_CNT, SI_TRACKED_VGT_GS_INSTANCE_CNT,
         radeon_opt_set_context_reg(sctx, R_0286C4_SPI_VS_OUT_CONFIG, SI_TRACKED_SPI_VS_OUT_CONFIG,
         radeon_opt_set_context_reg2(sctx, R_028708_SPI_SHADER_IDX_FORMAT,
                     radeon_opt_set_context_reg(sctx, R_028818_PA_CL_VTE_CNTL, SI_TRACKED_PA_CL_VTE_CNTL,
                  /* These don't cause a context roll. */
   radeon_begin_again(&sctx->gfx_cs);
   radeon_opt_set_uconfig_reg(sctx, R_030980_GE_PC_ALLOC, SI_TRACKED_GE_PC_ALLOC,
         if (sctx->screen->info.has_set_pairs_packets) {
      assert(!sctx->screen->info.uses_kernel_cu_mask);
   radeon_opt_push_gfx_sh_reg(R_00B21C_SPI_SHADER_PGM_RSRC3_GS,
               radeon_opt_push_gfx_sh_reg(R_00B204_SPI_SHADER_PGM_RSRC4_GS,
            } else {
      radeon_opt_set_sh_reg_idx(sctx, R_00B21C_SPI_SHADER_PGM_RSRC3_GS,
               radeon_opt_set_sh_reg_idx(sctx, R_00B204_SPI_SHADER_PGM_RSRC4_GS,
            }
      }
      static void gfx10_emit_shader_ngg_tess(struct si_context *sctx, unsigned index)
   {
               radeon_begin(&sctx->gfx_cs);
   radeon_opt_set_context_reg(sctx, R_028B6C_VGT_TF_PARAM, SI_TRACKED_VGT_TF_PARAM,
                     }
      unsigned si_get_input_prim(const struct si_shader_selector *gs, const union si_shader_key *key)
   {
      if (gs->stage == MESA_SHADER_GEOMETRY)
            if (gs->stage == MESA_SHADER_TESS_EVAL) {
      if (gs->info.base.tess.point_mode)
         if (gs->info.base.tess._primitive_mode == TESS_PRIMITIVE_ISOLINES)
                     if (key->ge.opt.ngg_culling & SI_NGG_CULL_LINES)
               }
      static unsigned si_get_vs_out_cntl(const struct si_shader_selector *sel,
         {
      /* Clip distances can be killed, but cull distances can't. */
   unsigned clipcull_mask = (sel->info.clipdist_mask & ~shader->key.ge.opt.kill_clip_distances) |
         bool writes_psize = sel->info.writes_psize && !shader->key.ge.opt.kill_pointsize;
   bool misc_vec_ena = writes_psize || (sel->info.writes_edgeflag && !ngg) ||
                  return S_02881C_VS_OUT_CCDIST0_VEC_ENA((clipcull_mask & 0x0F) != 0) |
         S_02881C_VS_OUT_CCDIST1_VEC_ENA((clipcull_mask & 0xF0) != 0) |
   S_02881C_USE_VTX_POINT_SIZE(writes_psize) |
   S_02881C_USE_VTX_EDGE_FLAG(sel->info.writes_edgeflag && !ngg) |
   S_02881C_USE_VTX_VRS_RATE(sel->screen->options.vrs2x2) |
   S_02881C_USE_VTX_RENDER_TARGET_INDX(sel->info.writes_layer) |
   S_02881C_USE_VTX_VIEWPORT_INDX(sel->info.writes_viewport_index) |
   S_02881C_VS_OUT_MISC_VEC_ENA(misc_vec_ena) |
   S_02881C_VS_OUT_MISC_SIDE_BUS_ENA(misc_vec_ena ||
      }
      /**
   * Prepare the PM4 image for \p shader, which will run as a merged ESGS shader
   * in NGG mode.
   */
   static void gfx10_shader_ngg(struct si_screen *sscreen, struct si_shader *shader)
   {
      const struct si_shader_selector *gs_sel = shader->selector;
   const struct si_shader_info *gs_info = &gs_sel->info;
   const gl_shader_stage gs_stage = shader->selector->stage;
   const struct si_shader_selector *es_sel =
         const struct si_shader_info *es_info = &es_sel->info;
   const gl_shader_stage es_stage = es_sel->stage;
   unsigned num_user_sgprs;
   unsigned num_params, es_vgpr_comp_cnt, gs_vgpr_comp_cnt;
   uint64_t va;
   bool window_space = gs_sel->stage == MESA_SHADER_VERTEX ?
         bool es_enable_prim_id = shader->key.ge.mono.u.vs_export_prim_id || es_info->uses_primid;
   unsigned gs_num_invocations = gs_sel->stage == MESA_SHADER_GEOMETRY ?
         unsigned input_prim = si_get_input_prim(gs_sel, &shader->key);
            struct si_pm4_state *pm4 = si_get_shader_pm4_state(shader, NULL);
   if (!pm4)
            if (es_stage == MESA_SHADER_TESS_EVAL)
         else
                     if (es_stage == MESA_SHADER_VERTEX) {
               if (es_info->base.vs.blit_sgprs_amd) {
      num_user_sgprs =
      } else {
            } else {
      assert(es_stage == MESA_SHADER_TESS_EVAL);
   es_vgpr_comp_cnt = es_enable_prim_id ? 3 : 2;
            if (es_enable_prim_id || gs_info->uses_primid)
               /* If offsets 4, 5 are used, GS_VGPR_COMP_CNT is ignored and
   * VGPR[0:4] are always loaded.
   *
   * Vertex shaders always need to load VGPR3, because they need to
   * pass edge flags for decomposed primitives (such as quads) to the PA
   * for the GL_LINE polygon mode to skip rendering lines on inner edges.
   */
   if (gs_info->uses_invocationid ||
      (gfx10_edgeflags_have_effect(shader) && !gfx10_is_ngg_passthrough(shader)))
      else if ((gs_stage == MESA_SHADER_GEOMETRY && gs_info->uses_primid) ||
               else if (input_prim >= MESA_PRIM_TRIANGLES && !gfx10_is_ngg_passthrough(shader))
         else
            si_pm4_set_reg(pm4, R_00B320_SPI_SHADER_PGM_LO_ES, va >> 8);
   si_pm4_set_reg(pm4, R_00B228_SPI_SHADER_PGM_RSRC1_GS,
                  S_00B228_VGPRS(si_shader_encode_vgprs(shader)) |
   S_00B228_FLOAT_MODE(shader->config.float_mode) |
      si_pm4_set_reg(pm4, R_00B22C_SPI_SHADER_PGM_RSRC2_GS,
                  S_00B22C_SCRATCH_EN(shader->config.scratch_bytes_per_wave > 0) |
   S_00B22C_USER_SGPR(num_user_sgprs) |
   S_00B22C_ES_VGPR_COMP_CNT(es_vgpr_comp_cnt) |
         /* Set register values emitted conditionally in gfx10_emit_shader_ngg_*. */
   shader->ngg.spi_shader_idx_format = S_028708_IDX0_EXPORT_FORMAT(V_028708_SPI_SHADER_1COMP);
   shader->ngg.spi_shader_pos_format =
      S_02870C_POS0_EXPORT_FORMAT(V_02870C_SPI_SHADER_4COMP) |
   S_02870C_POS1_EXPORT_FORMAT(shader->info.nr_pos_exports > 1 ? V_02870C_SPI_SHADER_4COMP
         S_02870C_POS2_EXPORT_FORMAT(shader->info.nr_pos_exports > 2 ? V_02870C_SPI_SHADER_4COMP
         S_02870C_POS3_EXPORT_FORMAT(shader->info.nr_pos_exports > 3 ? V_02870C_SPI_SHADER_4COMP
      shader->ngg.ge_max_output_per_subgroup = S_0287FC_MAX_VERTS_PER_SUBGROUP(shader->ngg.max_out_verts);
   shader->ngg.ge_ngg_subgrp_cntl = S_028B4C_PRIM_AMP_FACTOR(shader->ngg.prim_amp_factor);
   shader->ngg.vgt_gs_instance_cnt =
      S_028B90_ENABLE(gs_num_invocations > 1) |
   S_028B90_CNT(gs_num_invocations) |
               if (gs_stage == MESA_SHADER_GEOMETRY) {
      shader->ngg.esgs_vertex_stride = es_sel->info.esgs_vertex_stride / 4;
      } else {
      shader->ngg.esgs_vertex_stride = 1;
               if (es_stage == MESA_SHADER_TESS_EVAL)
                     shader->ngg.vgt_primitiveid_en =
      S_028A84_NGG_DISABLE_PROVOK_REUSE(shader->key.ge.mono.u.vs_export_prim_id ||
                  ac_compute_late_alloc(&sscreen->info, true, shader->key.ge.opt.ngg_culling,
                  /* Oversubscribe PC. This improves performance when there are too many varyings. */
            if (shader->key.ge.opt.ngg_culling) {
      /* Be more aggressive with NGG culling. */
   if (shader->info.nr_param_exports > 4)
         else if (shader->info.nr_param_exports > 2)
         else
      }
   oversub_pc_lines = late_alloc_wave64 ? (sscreen->info.pc_lines / 4) * oversub_pc_factor : 0;
   shader->ngg.ge_pc_alloc = S_030980_OVERSUB_EN(oversub_pc_lines > 0) |
         shader->ngg.vgt_primitiveid_en |= S_028A84_PRIMITIVEID_EN(es_enable_prim_id);
   shader->ngg.spi_shader_pgm_rsrc3_gs =
      ac_apply_cu_en(S_00B21C_CU_EN(cu_mask) |
            shader->ngg.spi_shader_pgm_rsrc4_gs = S_00B204_SPI_SHADER_LATE_ALLOC_GS_GFX10(late_alloc_wave64);
   shader->ngg.spi_vs_out_config = S_0286C4_VS_EXPORT_COUNT(num_params - 1) |
            if (sscreen->info.gfx_level >= GFX11) {
      shader->ngg.spi_shader_pgm_rsrc4_gs |=
      ac_apply_cu_en(S_00B204_CU_EN_GFX11(0x1) |
         } else {
      shader->ngg.spi_shader_pgm_rsrc4_gs |=
      ac_apply_cu_en(S_00B204_CU_EN_GFX10(0xffff),
            if (sscreen->info.gfx_level >= GFX11) {
      shader->ge_cntl = S_03096C_PRIMS_PER_SUBGRP(shader->ngg.max_gsprims) |
                     S_03096C_VERTS_PER_SUBGRP(shader->ngg.hw_max_esverts) |
      } else {
      shader->ge_cntl = S_03096C_PRIM_GRP_SIZE_GFX10(shader->ngg.max_gsprims) |
                  shader->ngg.vgt_gs_onchip_cntl =
      S_028A44_ES_VERTS_PER_SUBGRP(shader->ngg.hw_max_esverts) |
               /* On gfx10, the GE only checks against the maximum number of ES verts after
   * allocating a full GS primitive. So we need to ensure that whenever
   * this check passes, there is enough space for a full primitive without
   * vertex reuse. VERT_GRP_SIZE=256 doesn't need this. We should always get 256
   * if we have enough LDS.
   *
   * Tessellation is unaffected because it always sets GE_CNTL.VERT_GRP_SIZE = 0.
   */
   if ((sscreen->info.gfx_level == GFX10) &&
      (es_stage == MESA_SHADER_VERTEX || gs_stage == MESA_SHADER_VERTEX) && /* = no tess */
   shader->ngg.hw_max_esverts != 256 &&
   shader->ngg.hw_max_esverts > 5) {
   /* This could be based on the input primitive type. 5 is the worst case
   * for primitive types with adjacency.
   */
   shader->ge_cntl &= C_03096C_VERT_GRP_SIZE;
                  if (window_space) {
         } else {
      shader->ngg.pa_cl_vte_cntl = S_028818_VTX_W0_FMT(1) |
                           shader->ngg.vgt_shader_stages_en =
      S_028B54_ES_EN(es_stage == MESA_SHADER_TESS_EVAL ?
         S_028B54_GS_EN(gs_stage == MESA_SHADER_GEOMETRY) |
   S_028B54_PRIMGEN_EN(1) |
   S_028B54_PRIMGEN_PASSTHRU_EN(gfx10_is_ngg_passthrough(shader)) |
   S_028B54_PRIMGEN_PASSTHRU_NO_MSG(gfx10_is_ngg_passthrough(shader) &&
         S_028B54_NGG_WAVE_ID_EN(si_shader_uses_streamout(shader)) |
   S_028B54_GS_W32_EN(shader->wave_size == 32) |
            }
      static void si_emit_shader_vs(struct si_context *sctx, unsigned index)
   {
               radeon_begin(&sctx->gfx_cs);
   radeon_opt_set_context_reg(sctx, R_028A40_VGT_GS_MODE, SI_TRACKED_VGT_GS_MODE,
         radeon_opt_set_context_reg(sctx, R_028A84_VGT_PRIMITIVEID_EN, SI_TRACKED_VGT_PRIMITIVEID_EN,
            if (sctx->gfx_level <= GFX8) {
      radeon_opt_set_context_reg(sctx, R_028AB4_VGT_REUSE_OFF, SI_TRACKED_VGT_REUSE_OFF,
               radeon_opt_set_context_reg(sctx, R_0286C4_SPI_VS_OUT_CONFIG, SI_TRACKED_SPI_VS_OUT_CONFIG,
            radeon_opt_set_context_reg(sctx, R_02870C_SPI_SHADER_POS_FORMAT,
                  radeon_opt_set_context_reg(sctx, R_028818_PA_CL_VTE_CNTL, SI_TRACKED_PA_CL_VTE_CNTL,
            if (shader->selector->stage == MESA_SHADER_TESS_EVAL)
      radeon_opt_set_context_reg(sctx, R_028B6C_VGT_TF_PARAM, SI_TRACKED_VGT_TF_PARAM,
         if (shader->vgt_vertex_reuse_block_cntl)
      radeon_opt_set_context_reg(sctx, R_028C58_VGT_VERTEX_REUSE_BLOCK_CNTL,
               /* Required programming for tessellation. (legacy pipeline only) */
   if (sctx->gfx_level >= GFX10 && shader->selector->stage == MESA_SHADER_TESS_EVAL) {
      radeon_opt_set_context_reg(sctx, R_028A44_VGT_GS_ONCHIP_CNTL,
                                          /* GE_PC_ALLOC is not a context register, so it doesn't cause a context roll. */
   if (sctx->gfx_level >= GFX10) {
      radeon_begin_again(&sctx->gfx_cs);
   radeon_opt_set_uconfig_reg(sctx, R_030980_GE_PC_ALLOC, SI_TRACKED_GE_PC_ALLOC,
               }
      /**
   * Compute the state for \p shader, which will run as a vertex shader on the
   * hardware.
   *
   * If \p gs is non-NULL, it points to the geometry shader for which this shader
   * is the copy shader.
   */
   static void si_shader_vs(struct si_screen *sscreen, struct si_shader *shader,
         {
      const struct si_shader_info *info = &shader->selector->info;
   struct si_pm4_state *pm4;
   unsigned num_user_sgprs, vgpr_comp_cnt;
   uint64_t va;
   unsigned nparams, oc_lds_en;
   bool window_space = shader->selector->stage == MESA_SHADER_VERTEX ?
                           pm4 = si_get_shader_pm4_state(shader, si_emit_shader_vs);
   if (!pm4)
            /* We always write VGT_GS_MODE in the VS state, because every switch
   * between different shader pipelines involving a different GS or no
   * GS at all involves a switch of the VS (different GS use different
   * copy shaders). On the other hand, when the API switches from a GS to
   * no GS and then back to the same GS used originally, the GS state is
   * not sent again.
   */
   if (!gs) {
               /* PrimID needs GS scenario A. */
   if (enable_prim_id)
            shader->vs.vgt_gs_mode = S_028A40_MODE(mode);
      } else {
      shader->vs.vgt_gs_mode =
                     if (sscreen->info.gfx_level <= GFX8) {
      /* Reuse needs to be set off if we write oViewport. */
                        if (gs) {
      vgpr_comp_cnt = 0; /* only VertexID is needed for GS-COPY. */
      } else if (shader->selector->stage == MESA_SHADER_VERTEX) {
               if (info->base.vs.blit_sgprs_amd) {
         } else {
            } else if (shader->selector->stage == MESA_SHADER_TESS_EVAL) {
      vgpr_comp_cnt = enable_prim_id ? 3 : 2;
      } else
            /* VS is required to export at least one param. */
   nparams = MAX2(shader->info.nr_param_exports, 1);
            if (sscreen->info.gfx_level >= GFX10) {
      shader->vs.spi_vs_out_config |=
               shader->vs.spi_shader_pos_format =
      S_02870C_POS0_EXPORT_FORMAT(V_02870C_SPI_SHADER_4COMP) |
   S_02870C_POS1_EXPORT_FORMAT(shader->info.nr_pos_exports > 1 ? V_02870C_SPI_SHADER_4COMP
         S_02870C_POS2_EXPORT_FORMAT(shader->info.nr_pos_exports > 2 ? V_02870C_SPI_SHADER_4COMP
         S_02870C_POS3_EXPORT_FORMAT(shader->info.nr_pos_exports > 3 ? V_02870C_SPI_SHADER_4COMP
      unsigned late_alloc_wave64, cu_mask;
   ac_compute_late_alloc(&sscreen->info, false, false,
                  shader->vs.ge_pc_alloc = S_030980_OVERSUB_EN(late_alloc_wave64 > 0) |
                           if (sscreen->info.gfx_level >= GFX7) {
      si_pm4_set_reg_idx3(pm4, R_00B118_SPI_SHADER_PGM_RSRC3_VS,
                                 si_pm4_set_reg(pm4, R_00B120_SPI_SHADER_PGM_LO_VS, va >> 8);
   si_pm4_set_reg(pm4, R_00B124_SPI_SHADER_PGM_HI_VS,
            uint32_t rsrc1 =
      S_00B128_VGPRS(si_shader_encode_vgprs(shader)) |
   S_00B128_SGPRS(si_shader_encode_sgprs(shader)) |
   S_00B128_VGPR_COMP_CNT(vgpr_comp_cnt) |
   S_00B128_DX10_CLAMP(1) |
   S_00B128_MEM_ORDERED(si_shader_mem_ordered(shader)) |
      uint32_t rsrc2 = S_00B12C_USER_SGPR(num_user_sgprs) | S_00B12C_OC_LDS_EN(oc_lds_en) |
            if (sscreen->info.gfx_level >= GFX10)
         else if (sscreen->info.gfx_level == GFX9)
            if (si_shader_uses_streamout(shader)) {
      rsrc2 |= S_00B12C_SO_BASE0_EN(!!shader->selector->info.base.xfb_stride[0]) |
            S_00B12C_SO_BASE1_EN(!!shader->selector->info.base.xfb_stride[1]) |
   S_00B12C_SO_BASE2_EN(!!shader->selector->info.base.xfb_stride[2]) |
            si_pm4_set_reg(pm4, R_00B128_SPI_SHADER_PGM_RSRC1_VS, rsrc1);
            if (window_space)
         else
      shader->vs.pa_cl_vte_cntl =
      S_028818_VTX_W0_FMT(1) | S_028818_VPORT_X_SCALE_ENA(1) | S_028818_VPORT_X_OFFSET_ENA(1) |
            if (shader->selector->stage == MESA_SHADER_TESS_EVAL)
            polaris_set_vgt_vertex_reuse(sscreen, shader->selector, shader);
      }
      static unsigned si_get_spi_shader_col_format(struct si_shader *shader)
   {
      unsigned spi_shader_col_format = shader->key.ps.part.epilog.spi_shader_col_format;
   unsigned value = 0, num_mrts = 0;
            /* Remove holes in spi_shader_col_format. */
   for (i = 0; i < num_targets; i++) {
               if (spi_format) {
      value |= spi_format << (num_mrts * 4);
                     }
      static void si_emit_shader_ps(struct si_context *sctx, unsigned index)
   {
               radeon_begin(&sctx->gfx_cs);
   radeon_opt_set_context_reg2(sctx, R_0286CC_SPI_PS_INPUT_ENA, SI_TRACKED_SPI_PS_INPUT_ENA,
               radeon_opt_set_context_reg(sctx, R_0286E0_SPI_BARYC_CNTL, SI_TRACKED_SPI_BARYC_CNTL,
         radeon_opt_set_context_reg(sctx, R_0286D8_SPI_PS_IN_CONTROL, SI_TRACKED_SPI_PS_IN_CONTROL,
         radeon_opt_set_context_reg2(sctx, R_028710_SPI_SHADER_Z_FORMAT, SI_TRACKED_SPI_SHADER_Z_FORMAT,
               radeon_opt_set_context_reg(sctx, R_02823C_CB_SHADER_MASK, SI_TRACKED_CB_SHADER_MASK,
            }
      static void si_shader_ps(struct si_screen *sscreen, struct si_shader *shader)
   {
      struct si_shader_info *info = &shader->selector->info;
            /* we need to enable at least one of them, otherwise we hang the GPU */
   assert(G_0286CC_PERSP_SAMPLE_ENA(input_ena) || G_0286CC_PERSP_CENTER_ENA(input_ena) ||
         G_0286CC_PERSP_CENTROID_ENA(input_ena) || G_0286CC_PERSP_PULL_MODEL_ENA(input_ena) ||
   G_0286CC_LINEAR_SAMPLE_ENA(input_ena) || G_0286CC_LINEAR_CENTER_ENA(input_ena) ||
   /* POS_W_FLOAT_ENA requires one of the perspective weights. */
   assert(!G_0286CC_POS_W_FLOAT_ENA(input_ena) || G_0286CC_PERSP_SAMPLE_ENA(input_ena) ||
                  /* Validate interpolation optimization flags (read as implications). */
   assert(!shader->key.ps.part.prolog.bc_optimize_for_persp ||
         assert(!shader->key.ps.part.prolog.bc_optimize_for_linear ||
         assert(!shader->key.ps.part.prolog.force_persp_center_interp ||
         assert(!shader->key.ps.part.prolog.force_linear_center_interp ||
         assert(!shader->key.ps.part.prolog.force_persp_sample_interp ||
         assert(!shader->key.ps.part.prolog.force_linear_sample_interp ||
            /* Validate cases when the optimizations are off (read as implications). */
   assert(shader->key.ps.part.prolog.bc_optimize_for_persp ||
         assert(shader->key.ps.part.prolog.bc_optimize_for_linear ||
            /* DB_SHADER_CONTROL */
   shader->ps.db_shader_control = S_02880C_Z_EXPORT_ENABLE(info->writes_z) |
                        switch (info->base.fs.depth_layout) {
   case FRAG_DEPTH_LAYOUT_GREATER:
      shader->ps.db_shader_control |= S_02880C_CONSERVATIVE_Z_EXPORT(V_02880C_EXPORT_GREATER_THAN_Z);
      case FRAG_DEPTH_LAYOUT_LESS:
      shader->ps.db_shader_control |= S_02880C_CONSERVATIVE_Z_EXPORT(V_02880C_EXPORT_LESS_THAN_Z);
      default:;
            /* Z_ORDER, EXEC_ON_HIER_FAIL and EXEC_ON_NOOP should be set as following:
   *
   *   | early Z/S | writes_mem | allow_ReZ? |      Z_ORDER       | EXEC_ON_HIER_FAIL | EXEC_ON_NOOP
   * --|-----------|------------|------------|--------------------|-------------------|-------------
   * 1a|   false   |   false    |   true     | EarlyZ_Then_ReZ    |         0         |     0
   * 1b|   false   |   false    |   false    | EarlyZ_Then_LateZ  |         0         |     0
   * 2 |   false   |   true     |   n/a      |       LateZ        |         1         |     0
   * 3 |   true    |   false    |   n/a      | EarlyZ_Then_LateZ  |         0         |     0
   * 4 |   true    |   true     |   n/a      | EarlyZ_Then_LateZ  |         0         |     1
   *
   * In cases 3 and 4, HW will force Z_ORDER to EarlyZ regardless of what's set in the register.
   * In case 2, NOOP_CULL is a don't care field. In case 2, 3 and 4, ReZ doesn't make sense.
   *
   * Don't use ReZ without profiling !!!
   *
   * ReZ decreases performance by 15% in DiRT: Showdown on Ultra settings, which has pretty complex
   * shaders.
   */
   if (info->base.fs.early_fragment_tests) {
      /* Cases 3, 4. */
   shader->ps.db_shader_control |= S_02880C_DEPTH_BEFORE_SHADER(1) |
            } else if (info->base.writes_memory) {
      /* Case 2. */
   shader->ps.db_shader_control |= S_02880C_Z_ORDER(V_02880C_LATE_Z) |
      } else {
      /* Case 1. */
               if (info->base.fs.post_depth_coverage)
            /* Bug workaround for smoothing (overrasterization) on GFX6. */
   if (sscreen->info.gfx_level == GFX6 && shader->key.ps.mono.poly_line_smoothing) {
      shader->ps.db_shader_control &= C_02880C_Z_ORDER;
               if (sscreen->info.has_rbplus && !sscreen->info.rbplus_allowed)
            /* SPI_BARYC_CNTL.POS_FLOAT_LOCATION
   * Possible values:
   * 0 -> Position = pixel center
   * 1 -> Position = pixel centroid
   * 2 -> Position = at sample position
   *
   * From GLSL 4.5 specification, section 7.1:
   *   "The variable gl_FragCoord is available as an input variable from
   *    within fragment shaders and it holds the window relative coordinates
   *    (x, y, z, 1/w) values for the fragment. If multi-sampling, this
   *    value can be for any location within the pixel, or one of the
   *    fragment samples. The use of centroid does not further restrict
   *    this value to be inside the current primitive."
   *
   * Meaning that centroid has no effect and we can return anything within
   * the pixel. Thus, return the value at sample position, because that's
   * the most accurate one shaders can get.
   */
   shader->ps.spi_baryc_cntl = S_0286E0_POS_FLOAT_LOCATION(2) |
               shader->ps.spi_shader_col_format = si_get_spi_shader_col_format(shader);
   shader->ps.cb_shader_mask = ac_get_cb_shader_mask(shader->key.ps.part.epilog.spi_shader_col_format);
   shader->ps.spi_ps_input_ena = shader->config.spi_ps_input_ena;
   shader->ps.spi_ps_input_addr = shader->config.spi_ps_input_addr;
   shader->ps.num_interp = si_get_ps_num_interp(shader);
   shader->ps.spi_shader_z_format =
      ac_get_spi_shader_z_format(info->writes_z, info->writes_stencil, shader->ps.writes_samplemask,
         /* Ensure that some export memory is always allocated, for two reasons:
   *
   * 1) Correctness: The hardware ignores the EXEC mask if no export
   *    memory is allocated, so KILL and alpha test do not work correctly
   *    without this.
   * 2) Performance: Every shader needs at least a NULL export, even when
   *    it writes no color/depth output. The NULL export instruction
   *    stalls without this setting.
   *
   * Don't add this to CB_SHADER_MASK.
   *
   * GFX10 supports pixel shaders without exports by setting both
   * the color and Z formats to SPI_SHADER_ZERO. The hw will skip export
   * instructions if any are present.
   *
   * RB+ depth-only rendering requires SPI_SHADER_32_R.
   */
            if (!shader->ps.spi_shader_col_format) {
      if (shader->key.ps.part.epilog.rbplus_depth_only_opt) {
         } else if (!has_mrtz) {
      if (sscreen->info.gfx_level >= GFX10) {
      if (G_02880C_KILL_ENABLE(shader->ps.db_shader_control))
      } else {
                        /* Enable PARAM_GEN for point smoothing.
   * Gfx11 workaround when there are no PS inputs but LDS is used.
   */
   bool param_gen = shader->key.ps.mono.point_smoothing ||
                  shader->ps.spi_ps_in_control = S_0286D8_NUM_INTERP(shader->ps.num_interp) |
                  struct si_pm4_state *pm4 = si_get_shader_pm4_state(shader, si_emit_shader_ps);
   if (!pm4)
            /* If multiple state sets are allowed to be in a bin, break the batch on a new PS. */
   if (sscreen->dpbb_allowed &&
      (sscreen->pbb_context_states_per_bin > 1 ||
   sscreen->pbb_persistent_states_per_bin > 1)) {
   si_pm4_cmd_add(pm4, PKT3(PKT3_EVENT_WRITE, 0, 0));
               if (sscreen->info.gfx_level >= GFX11) {
               si_pm4_set_reg_idx3(pm4, R_00B004_SPI_SHADER_PGM_RSRC4_PS,
                           uint64_t va = shader->bo->gpu_address;
   si_pm4_set_reg(pm4, R_00B020_SPI_SHADER_PGM_LO_PS, va >> 8);
   si_pm4_set_reg(pm4, R_00B024_SPI_SHADER_PGM_HI_PS,
            si_pm4_set_reg(pm4, R_00B028_SPI_SHADER_PGM_RSRC1_PS,
                  S_00B028_VGPRS(si_shader_encode_vgprs(shader)) |
   S_00B028_SGPRS(si_shader_encode_sgprs(shader)) |
      si_pm4_set_reg(pm4, R_00B02C_SPI_SHADER_PGM_RSRC2_PS,
                        }
      static void si_shader_init_pm4_state(struct si_screen *sscreen, struct si_shader *shader)
   {
               switch (shader->selector->stage) {
   case MESA_SHADER_VERTEX:
      if (shader->key.ge.as_ls)
         else if (shader->key.ge.as_es)
         else if (shader->key.ge.as_ngg)
         else
            case MESA_SHADER_TESS_CTRL:
      si_shader_hs(sscreen, shader);
      case MESA_SHADER_TESS_EVAL:
      if (shader->key.ge.as_es)
         else if (shader->key.ge.as_ngg)
         else
            case MESA_SHADER_GEOMETRY:
      if (shader->key.ge.as_ngg) {
         } else {
      /* VS must be initialized first because GS uses its fields. */
   si_shader_vs(sscreen, shader->gs_copy_shader, shader->selector);
      }
      case MESA_SHADER_FRAGMENT:
      si_shader_ps(sscreen, shader);
      default:
                     }
      static void si_clear_vs_key_inputs(struct si_context *sctx, union si_shader_key *key,
         {
      prolog_key->instance_divisor_is_one = 0;
   prolog_key->instance_divisor_is_fetched = 0;
   key->ge.mono.vs_fetch_opencode = 0;
      }
      void si_vs_key_update_inputs(struct si_context *sctx)
   {
      struct si_shader_selector *vs = sctx->shader.vs.cso;
   struct si_vertex_elements *elts = sctx->vertex_elements;
            if (!vs)
            if (vs->info.base.vs.blit_sgprs_amd) {
      si_clear_vs_key_inputs(sctx, key, &key->ge.part.vs.prolog);
   key->ge.opt.prefer_mono = 0;
   sctx->uses_nontrivial_vs_prolog = false;
                        if (elts->instance_divisor_is_one || elts->instance_divisor_is_fetched)
            key->ge.part.vs.prolog.instance_divisor_is_one = elts->instance_divisor_is_one;
   key->ge.part.vs.prolog.instance_divisor_is_fetched = elts->instance_divisor_is_fetched;
            unsigned count_mask = (1 << vs->info.num_inputs) - 1;
   unsigned fix = elts->fix_fetch_always & count_mask;
            if (sctx->vertex_buffer_unaligned & elts->vb_alignment_check_mask) {
      uint32_t mask = elts->fix_fetch_unaligned & count_mask;
   while (mask) {
      unsigned i = u_bit_scan(&mask);
   unsigned log_hw_load_size = 1 + ((elts->hw_load_is_dword >> i) & 1);
   unsigned vbidx = elts->vertex_buffer_index[i];
   struct pipe_vertex_buffer *vb = &sctx->vertex_buffer[vbidx];
   unsigned align_mask = (1 << log_hw_load_size) - 1;
   if (vb->buffer_offset & align_mask) {
      fix |= 1 << i;
                              while (fix) {
      unsigned i = u_bit_scan(&fix);
            key->ge.mono.vs_fix_fetch[i].bits = fix_fetch;
   if (fix_fetch)
      }
   key->ge.mono.vs_fetch_opencode = opencode;
   if (opencode)
                     /* draw_vertex_state (display lists) requires a trivial VS prolog that ignores
   * the current vertex buffers and vertex elements.
   *
   * We just computed the prolog key because we needed to set uses_nontrivial_vs_prolog,
   * so that we know whether the VS prolog should be updated when we switch from
   * draw_vertex_state to draw_vbo. Now clear the VS prolog for draw_vertex_state.
   * This should happen rarely because the VS prolog should be trivial in most
   * cases.
   */
   if (uses_nontrivial_vs_prolog && sctx->force_trivial_vs_prolog)
      }
      void si_get_vs_key_inputs(struct si_context *sctx, union si_shader_key *key,
         {
      prolog_key->instance_divisor_is_one = sctx->shader.vs.key.ge.part.vs.prolog.instance_divisor_is_one;
            key->ge.mono.vs_fetch_opencode = sctx->shader.vs.key.ge.mono.vs_fetch_opencode;
   memcpy(key->ge.mono.vs_fix_fetch, sctx->shader.vs.key.ge.mono.vs_fix_fetch,
      }
      void si_update_ps_inputs_read_or_disabled(struct si_context *sctx)
   {
               /* Find out if PS is disabled. */
   bool ps_disabled = true;
   if (ps) {
      bool ps_modifies_zs = ps->info.base.fs.uses_discard ||
                        ps->info.writes_z ||
   ps->info.writes_stencil ||
   ps->info.writes_samplemask ||
               ps_disabled = sctx->queued.named.rasterizer->rasterizer_discard ||
               if (ps_disabled) {
         } else {
               if (sctx->shader.ps.key.ps.part.prolog.color_two_side) {
                     if (inputs_read & BITFIELD64_BIT(SI_UNIQUE_SLOT_COL1))
                     }
      static void si_get_vs_key_outputs(struct si_context *sctx, struct si_shader_selector *vs,
         {
               /* Find out which VS outputs aren't used by the PS. */
   uint64_t outputs_written = vs->info.outputs_written_before_ps;
            key->ge.opt.kill_outputs = ~linked & outputs_written;
   key->ge.opt.ngg_culling = sctx->ngg_culling;
   key->ge.mono.u.vs_export_prim_id = vs->stage != MESA_SHADER_GEOMETRY &&
         key->ge.opt.kill_pointsize = vs->info.writes_psize &&
               key->ge.opt.remove_streamout = vs->info.enabled_streamout_buffer_mask &&
      }
      static void si_clear_vs_key_outputs(struct si_context *sctx, struct si_shader_selector *vs,
         {
      key->ge.opt.kill_clip_distances = 0;
   key->ge.opt.kill_outputs = 0;
   key->ge.opt.remove_streamout = 0;
   key->ge.opt.ngg_culling = 0;
   key->ge.mono.u.vs_export_prim_id = 0;
      }
      void si_ps_key_update_framebuffer(struct si_context *sctx)
   {
      struct si_shader_selector *sel = sctx->shader.ps.cso;
            if (!sel)
            if (sel->info.color0_writes_all_cbufs &&
      sel->info.colors_written == 0x1)
      else
            /* ps_uses_fbfetch is true only if the color buffer is bound. */
   if (sctx->ps_uses_fbfetch) {
      struct pipe_surface *cb0 = sctx->framebuffer.state.cbufs[0];
            /* 1D textures are allocated and used as 2D on GFX9. */
   key->ps.mono.fbfetch_msaa = sctx->framebuffer.nr_samples > 1;
   key->ps.mono.fbfetch_is_1D =
      sctx->gfx_level != GFX9 &&
      key->ps.mono.fbfetch_layered =
      tex->target == PIPE_TEXTURE_1D_ARRAY || tex->target == PIPE_TEXTURE_2D_ARRAY ||
   tex->target == PIPE_TEXTURE_CUBE || tex->target == PIPE_TEXTURE_CUBE_ARRAY ||
   } else {
      key->ps.mono.fbfetch_msaa = 0;
   key->ps.mono.fbfetch_is_1D = 0;
         }
      void si_ps_key_update_framebuffer_blend_rasterizer(struct si_context *sctx)
   {
      struct si_shader_selector *sel = sctx->shader.ps.cso;
   union si_shader_key *key = &sctx->shader.ps.key;
   struct si_state_blend *blend = sctx->queued.named.blend;
   struct si_state_rasterizer *rs = sctx->queued.named.rasterizer;
   bool alpha_to_coverage = blend->alpha_to_coverage && rs->multisample_enable &&
                  if (!sel)
            key->ps.part.epilog.alpha_to_one = blend->alpha_to_one && rs->multisample_enable;
   key->ps.part.epilog.alpha_to_coverage_via_mrtz =
      sctx->gfx_level >= GFX11 && alpha_to_coverage &&
         /* Remove the gl_SampleMask fragment shader output if MSAA is disabled.
   * This is required for correctness and it's also an optimization.
   */
   key->ps.part.epilog.kill_samplemask = sel->info.writes_samplemask &&
                  /* If alpha-to-coverage isn't exported via MRTZ, set that we need to export alpha
   * through MRT0.
   */
   if (alpha_to_coverage && !key->ps.part.epilog.alpha_to_coverage_via_mrtz)
            /* Select the shader color format based on whether
   * blending or alpha are needed.
   */
   key->ps.part.epilog.spi_shader_col_format =
      (blend->blend_enable_4bit & need_src_alpha_4bit &
   sctx->framebuffer.spi_shader_col_format_blend_alpha) |
   (blend->blend_enable_4bit & ~need_src_alpha_4bit &
   sctx->framebuffer.spi_shader_col_format_blend) |
   (~blend->blend_enable_4bit & need_src_alpha_4bit &
   sctx->framebuffer.spi_shader_col_format_alpha) |
   (~blend->blend_enable_4bit & ~need_src_alpha_4bit &
               key->ps.part.epilog.dual_src_blend_swizzle = sctx->gfx_level >= GFX11 &&
                  /* The output for dual source blending should have
   * the same format as the first output.
   */
   if (blend->dual_src_blend) {
      key->ps.part.epilog.spi_shader_col_format |=
               /* If alpha-to-coverage is enabled, we have to export alpha
   * even if there is no color buffer.
   *
   * Gfx11 exports alpha-to-coverage via MRTZ if MRTZ is present.
   */
   if (!(key->ps.part.epilog.spi_shader_col_format & 0xf) && alpha_to_coverage &&
      !key->ps.part.epilog.alpha_to_coverage_via_mrtz)
         /* On GFX6 and GFX7 except Hawaii, the CB doesn't clamp outputs
   * to the range supported by the type if a channel has less
   * than 16 bits and the export format is 16_ABGR.
   */
   if (sctx->gfx_level <= GFX7 && sctx->family != CHIP_HAWAII) {
      key->ps.part.epilog.color_is_int8 = sctx->framebuffer.color_is_int8;
               /* Disable unwritten outputs (if WRITE_ALL_CBUFS isn't enabled). */
   if (!key->ps.part.epilog.last_cbuf) {
      key->ps.part.epilog.spi_shader_col_format &= sel->info.colors_written_4bit;
   key->ps.part.epilog.color_is_int8 &= sel->info.colors_written;
               /* Enable RB+ for depth-only rendering. Registers must be programmed as follows:
   *    CB_COLOR_CONTROL.MODE = CB_DISABLE
   *    CB_COLOR0_INFO.FORMAT = COLOR_32
   *    CB_COLOR0_INFO.NUMBER_TYPE = NUMBER_FLOAT
   *    SPI_SHADER_COL_FORMAT.COL0_EXPORT_FORMAT = SPI_SHADER_32_R
   *    SX_PS_DOWNCONVERT.MRT0 = SX_RT_EXPORT_32_R
   *
   * Also, the following conditions must be met.
   */
   key->ps.part.epilog.rbplus_depth_only_opt =
      sctx->screen->info.rbplus_allowed &&
   blend->cb_target_enabled_4bit == 0 && /* implies CB_DISABLE */
   !alpha_to_coverage &&
   !sel->info.base.writes_memory &&
         /* Eliminate shader code computing output values that are unused.
   * This enables dead code elimination between shader parts.
   * Check if any output is eliminated.
   *
   * Dual source blending never has color buffer 1 enabled, so ignore it.
   *
   * On gfx11, pixel shaders that write memory should be compiled with an inlined epilog,
   * so that the compiler can see s_endpgm and deallocates VGPRs before memory stores return.
   */
   if (sel->info.colors_written_4bit &
      (blend->dual_src_blend ? 0xffffff0f : 0xffffffff) &
   ~(sctx->framebuffer.colorbuf_enabled_4bit & blend->cb_target_enabled_4bit))
      else if (sctx->gfx_level >= GFX11 && sel->info.base.writes_memory)
         else
      }
      void si_ps_key_update_rasterizer(struct si_context *sctx)
   {
      struct si_shader_selector *sel = sctx->shader.ps.cso;
   union si_shader_key *key = &sctx->shader.ps.key;
            if (!sel)
            key->ps.part.prolog.color_two_side = rs->two_side && sel->info.colors_read;
   key->ps.part.prolog.flatshade_colors = rs->flatshade && sel->info.uses_interp_color;
      }
      void si_ps_key_update_dsa(struct si_context *sctx)
   {
                  }
      static void si_ps_key_update_primtype_shader_rasterizer_framebuffer(struct si_context *sctx)
   {
      union si_shader_key *key = &sctx->shader.ps.key;
            bool is_poly = !util_prim_is_points_or_lines(sctx->current_rast_prim);
            key->ps.part.prolog.poly_stipple = rs->poly_stipple_enable && is_poly;
   key->ps.mono.poly_line_smoothing =
      ((is_poly && rs->poly_smooth) || (is_line && rs->line_smooth)) &&
         key->ps.mono.point_smoothing = rs->point_smooth &&
      }
      void si_ps_key_update_sample_shading(struct si_context *sctx)
   {
      struct si_shader_selector *sel = sctx->shader.ps.cso;
            if (!sel)
            if (sctx->ps_iter_samples > 1 && sel->info.reads_samplemask)
         else
      }
      void si_ps_key_update_framebuffer_rasterizer_sample_shading(struct si_context *sctx)
   {
      struct si_shader_selector *sel = sctx->shader.ps.cso;
   union si_shader_key *key = &sctx->shader.ps.key;
            if (!sel)
            bool uses_persp_center = sel->info.uses_persp_center ||
         bool uses_persp_centroid = sel->info.uses_persp_centroid ||
         bool uses_persp_sample = sel->info.uses_persp_sample ||
            if (rs->force_persample_interp && rs->multisample_enable &&
      sctx->framebuffer.nr_samples > 1 && sctx->ps_iter_samples > 1) {
   key->ps.part.prolog.force_persp_sample_interp =
            key->ps.part.prolog.force_linear_sample_interp =
            key->ps.part.prolog.force_persp_center_interp = 0;
   key->ps.part.prolog.force_linear_center_interp = 0;
   key->ps.part.prolog.bc_optimize_for_persp = 0;
   key->ps.part.prolog.bc_optimize_for_linear = 0;
      } else if (rs->multisample_enable && sctx->framebuffer.nr_samples > 1) {
      key->ps.part.prolog.force_persp_sample_interp = 0;
   key->ps.part.prolog.force_linear_sample_interp = 0;
   key->ps.part.prolog.force_persp_center_interp = 0;
   key->ps.part.prolog.force_linear_center_interp = 0;
   key->ps.part.prolog.bc_optimize_for_persp =
         key->ps.part.prolog.bc_optimize_for_linear =
            } else {
      key->ps.part.prolog.force_persp_sample_interp = 0;
            /* Make sure SPI doesn't compute more than 1 pair
   * of (i,j), which is the optimization here. */
   key->ps.part.prolog.force_persp_center_interp = uses_persp_center +
                  key->ps.part.prolog.force_linear_center_interp = sel->info.uses_linear_center +
               key->ps.part.prolog.bc_optimize_for_persp = 0;
   key->ps.part.prolog.bc_optimize_for_linear = 0;
         }
      /* Compute the key for the hw shader variant */
   static inline void si_shader_selector_key(struct pipe_context *ctx, struct si_shader_selector *sel,
         {
               switch (sel->stage) {
   case MESA_SHADER_VERTEX:
      if (!sctx->shader.tes.cso && !sctx->shader.gs.cso)
         else
            case MESA_SHADER_TESS_CTRL:
      if (sctx->gfx_level >= GFX9) {
      si_get_vs_key_inputs(sctx, key, &key->ge.part.tcs.ls_prolog);
      }
      case MESA_SHADER_TESS_EVAL:
      if (!sctx->shader.gs.cso)
         else
            case MESA_SHADER_GEOMETRY:
      if (sctx->gfx_level >= GFX9) {
      if (sctx->shader.tes.cso) {
      si_clear_vs_key_inputs(sctx, key, &key->ge.part.gs.vs_prolog);
      } else {
      si_get_vs_key_inputs(sctx, key, &key->ge.part.gs.vs_prolog);
               /* Only NGG can eliminate GS outputs, because the code is shared with VS. */
   if (sctx->ngg)
         else
      }
      case MESA_SHADER_FRAGMENT:
      si_ps_key_update_primtype_shader_rasterizer_framebuffer(sctx);
      default:
            }
      static void si_build_shader_variant(struct si_shader *shader, int thread_index, bool low_priority)
   {
      struct si_shader_selector *sel = shader->selector;
   struct si_screen *sscreen = sel->screen;
   struct ac_llvm_compiler **compiler;
            if (thread_index >= 0) {
      if (low_priority) {
      assert(thread_index < (int)ARRAY_SIZE(sscreen->compiler_lowp));
      } else {
      assert(thread_index < (int)ARRAY_SIZE(sscreen->compiler));
      }
   if (!debug->async)
      } else {
      assert(!low_priority);
               if (!*compiler) {
      *compiler = CALLOC_STRUCT(ac_llvm_compiler);
               if (unlikely(!si_create_shader_variant(sscreen, *compiler, shader, debug))) {
      PRINT_ERR("Failed to build shader variant (type=%u)\n", sel->stage);
   shader->compilation_failed = true;
               if (shader->compiler_ctx_state.is_debug_context) {
      FILE *f = open_memstream(&shader->shader_log, &shader->shader_log_size);
   if (f) {
      si_shader_dump(sscreen, shader, NULL, f, false);
                     }
      static void si_build_shader_variant_low_priority(void *job, void *gdata, int thread_index)
   {
                           }
      /* This should be const, but C++ doesn't allow implicit zero-initialization with const. */
   static union si_shader_key zeroed;
      static bool si_check_missing_main_part(struct si_screen *sscreen, struct si_shader_selector *sel,
               {
               if (!*mainp) {
               if (!main_part)
            /* We can leave the fence as permanently signaled because the
   * main part becomes visible globally only after it has been
   * compiled. */
            main_part->selector = sel;
   if (sel->stage <= MESA_SHADER_GEOMETRY) {
      main_part->key.ge.as_es = key->ge.as_es;
   main_part->key.ge.as_ls = key->ge.as_ls;
      }
   main_part->is_monolithic = false;
            if (!si_compile_shader(sscreen, compiler_state->compiler, main_part,
            FREE(main_part);
      }
      }
      }
      /* A helper to copy *key to *local_key and return local_key. */
   template<typename SHADER_KEY_TYPE>
   static ALWAYS_INLINE const SHADER_KEY_TYPE *
   use_local_key_copy(const SHADER_KEY_TYPE *key, SHADER_KEY_TYPE *local_key, unsigned key_size)
   {
      if (key != local_key)
               }
      #define NO_INLINE_UNIFORMS false
      /**
   * Select a shader variant according to the shader key.
   *
   * This uses a C++ template to compute the optimal memcmp size at compile time, which is important
   * for getting inlined memcmp. The memcmp size depends on the shader key type and whether inlined
   * uniforms are enabled.
   */
   template<bool INLINE_UNIFORMS = true, typename SHADER_KEY_TYPE>
   static int si_shader_select_with_key(struct si_context *sctx, struct si_shader_ctx_state *state,
         {
      struct si_screen *sscreen = sctx->screen;
   struct si_shader_selector *sel = state->cso;
   struct si_shader_selector *previous_stage_sel = NULL;
   struct si_shader *current = state->current;
   struct si_shader *shader = NULL;
            /* "opt" must be the last field and "inlined_uniform_values" must be the last field inside opt.
   * If there is padding, insert the padding manually before opt or inside opt.
   */
   STATIC_ASSERT(offsetof(SHADER_KEY_TYPE, opt) + sizeof(key->opt) == sizeof(*key));
   STATIC_ASSERT(offsetof(SHADER_KEY_TYPE, opt.inlined_uniform_values) +
            const unsigned key_size_no_uniforms = sizeof(*key) - sizeof(key->opt.inlined_uniform_values);
   /* Don't compare inlined_uniform_values if uniform inlining is disabled. */
   const unsigned key_size = INLINE_UNIFORMS ? sizeof(*key) : key_size_no_uniforms;
   const unsigned key_opt_size =
      INLINE_UNIFORMS ? sizeof(key->opt) :
         /* si_shader_select_with_key must not modify 'key' because it would affect future shaders.
   * If we need to modify it for this specific shader (eg: to disable optimizations), we
   * use a copy.
   */
            if (unlikely(sscreen->debug_flags & DBG(NO_OPT_VARIANT))) {
      /* Disable shader variant optimizations. */
   key = use_local_key_copy<SHADER_KEY_TYPE>(key, &local_key, key_size);
            again:
      /* Check if we don't need to change anything.
   * This path is also used for most shaders that don't need multiple
   * variants, it will cost just a computation of the key and this
   * test. */
   if (likely(current && memcmp(&current->key, key, key_size) == 0)) {
      if (unlikely(!util_queue_fence_is_signalled(&current->ready))) {
      if (current->is_optimized) {
      key = use_local_key_copy(key, &local_key, key_size);
   memset(&local_key.opt, 0, key_opt_size);
                                 current_not_ready:
         /* This must be done before the mutex is locked, because async GS
   * compilation calls this function too, and therefore must enter
   * the mutex first.
   */
                     int variant_count = 0;
            /* Find the shader variant. */
   const unsigned cnt = sel->variants_count;
   for (unsigned i = 0; i < cnt; i++) {
               if (memcmp(iter_key, key, key_size_no_uniforms) == 0) {
               /* Check the inlined uniform values separately, and count
   * the number of variants based on them.
   */
   if (key->opt.inline_uniforms &&
      memcmp(iter_key->opt.inlined_uniform_values,
         key->opt.inlined_uniform_values,
   if (variant_count++ > max_inline_uniforms_variants) {
      key = use_local_key_copy(key, &local_key, key_size);
   /* Too many variants. Disable inlining for this shader. */
   local_key.opt.inline_uniforms = 0;
   memset(local_key.opt.inlined_uniform_values, 0, MAX_INLINABLE_UNIFORMS * 4);
   simple_mtx_unlock(&sel->mutex);
      }
                        if (unlikely(!util_queue_fence_is_signalled(&iter->ready))) {
      /* If it's an optimized shader and its compilation has
   * been started but isn't done, use the unoptimized
   * shader so as not to cause a stall due to compilation.
   */
   if (iter->is_optimized) {
      key = use_local_key_copy(key, &local_key, key_size);
                                 if (iter->compilation_failed) {
                  state->current = sel->variants[i];
                  /* Build a new shader. */
   shader = CALLOC_STRUCT(si_shader);
   if (!shader) {
      simple_mtx_unlock(&sel->mutex);
                        if (!sctx->compiler) {
      sctx->compiler = CALLOC_STRUCT(ac_llvm_compiler);
               shader->selector = sel;
   *((SHADER_KEY_TYPE*)&shader->key) = *key;
   shader->wave_size = si_determine_wave_size(sscreen, shader);
   shader->compiler_ctx_state.compiler = sctx->compiler;
   shader->compiler_ctx_state.debug = sctx->debug;
            /* If this is a merged shader, get the first shader's selector. */
   if (sscreen->info.gfx_level >= GFX9) {
      if (sel->stage == MESA_SHADER_TESS_CTRL)
         else if (sel->stage == MESA_SHADER_GEOMETRY)
            /* We need to wait for the previous shader. */
   if (previous_stage_sel)
               bool is_pure_monolithic =
            /* Compile the main shader part if it doesn't exist. This can happen
   * if the initial guess was wrong.
   */
   if (!is_pure_monolithic) {
               /* Make sure the main shader part is present. This is needed
   * for shaders that can be compiled as VS, LS, or ES, and only
   * one of them is compiled at creation.
   *
   * It is also needed for GS, which can be compiled as non-NGG
   * and NGG.
   *
   * For merged shaders, check that the starting shader's main
   * part is present.
   */
   if (previous_stage_sel) {
               if (sel->stage == MESA_SHADER_TESS_CTRL) {
         } else if (sel->stage == MESA_SHADER_GEOMETRY) {
      shader1_key.ge.as_es = 1;
      } else {
                  simple_mtx_lock(&previous_stage_sel->mutex);
   ok = si_check_missing_main_part(sscreen, previous_stage_sel, &shader->compiler_ctx_state,
                     if (ok) {
      ok = si_check_missing_main_part(sscreen, sel, &shader->compiler_ctx_state,
               if (!ok) {
      FREE(shader);
   simple_mtx_unlock(&sel->mutex);
                  if (sel->variants_count == sel->variants_max_count) {
      sel->variants_max_count += 2;
   sel->variants = (struct si_shader**)
         sel->keys = (union si_shader_key*)
               /* Keep the reference to the 1st shader of merged shaders, so that
   * Gallium can't destroy it before we destroy the 2nd shader.
   *
   * Set sctx = NULL, because it's unused if we're not releasing
   * the shader, and we don't have any sctx here.
   */
            /* Monolithic-only shaders don't make a distinction between optimized
   * and unoptimized. */
   shader->is_monolithic =
            shader->is_optimized = !is_pure_monolithic &&
            /* If it's an optimized shader, compile it asynchronously. */
   if (shader->is_optimized) {
      /* Compile it asynchronously. */
   util_queue_add_job(&sscreen->shader_compiler_queue_low_priority, shader, &shader->ready,
            /* Add only after the ready fence was reset, to guard against a
   * race with si_bind_XX_shader. */
   sel->variants[sel->variants_count] = shader;
   sel->keys[sel->variants_count] = shader->key;
            /* Use the default (unoptimized) shader for now. */
   key = use_local_key_copy(key, &local_key, key_size);
   memset(&local_key.opt, 0, key_opt_size);
            if (sscreen->options.sync_compile)
                        /* Reset the fence before adding to the variant list. */
            sel->variants[sel->variants_count] = shader;
   sel->keys[sel->variants_count] = shader->key;
                     assert(!shader->is_optimized);
                     if (!shader->compilation_failed)
               }
      int si_shader_select(struct pipe_context *ctx, struct si_shader_ctx_state *state)
   {
                        if (state->cso->stage == MESA_SHADER_FRAGMENT) {
      if (state->key.ps.opt.inline_uniforms)
         else
      } else {
      if (state->key.ge.opt.inline_uniforms) {
         } else {
               }
      static void si_parse_next_shader_property(const struct si_shader_info *info,
         {
               switch (info->base.stage) {
   case MESA_SHADER_VERTEX:
      switch (next_shader) {
   case MESA_SHADER_GEOMETRY:
      key->ge.as_es = 1;
      case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
      key->ge.as_ls = 1;
      default:
      /* If POSITION isn't written, it can only be a HW VS
   * if streamout is used. If streamout isn't used,
   * assume that it's a HW LS. (the next shader is TCS)
   * This heuristic is needed for separate shader objects.
   */
   if (!info->writes_position && !info->enabled_streamout_buffer_mask)
      }
         case MESA_SHADER_TESS_EVAL:
      if (next_shader == MESA_SHADER_GEOMETRY || !info->writes_position)
               default:;
      }
      /**
   * Compile the main shader part or the monolithic shader as part of
   * si_shader_selector initialization. Since it can be done asynchronously,
   * there is no way to report compile failures to applications.
   */
   static void si_init_shader_selector_async(void *job, void *gdata, int thread_index)
   {
      struct si_shader_selector *sel = (struct si_shader_selector *)job;
   struct si_screen *sscreen = sel->screen;
   struct ac_llvm_compiler **compiler;
            assert(!debug->debug_message || debug->async);
   assert(thread_index >= 0);
   assert(thread_index < (int)ARRAY_SIZE(sscreen->compiler));
            if (!*compiler) {
      *compiler = CALLOC_STRUCT(ac_llvm_compiler);
               /* Serialize NIR to save memory. Monolithic shader variants
   * have to deserialize NIR before compilation.
   */
   if (sel->nir) {
      struct blob blob;
            blob_init(&blob);
   /* true = remove optional debugging data to increase
   * the likehood of getting more shader cache hits.
   * It also drops variable names, so we'll save more memory.
   * If NIR debug prints are used we don't strip to get more
   * useful logs.
   */
   nir_serialize(&blob, sel->nir, NIR_DEBUG(PRINT) == 0);
   blob_finish_get_buffer(&blob, &sel->nir_binary, &size);
               /* Compile the main shader part for use with a prolog and/or epilog.
   * If this fails, the driver will try to compile a monolithic shader
   * on demand.
   */
   if (!sscreen->use_monolithic_shaders) {
      struct si_shader *shader = CALLOC_STRUCT(si_shader);
            if (!shader) {
      fprintf(stderr, "radeonsi: can't allocate a main shader part\n");
               /* We can leave the fence signaled because use of the default
   * main part is guarded by the selector's ready fence. */
            shader->selector = sel;
   shader->is_monolithic = false;
            if (sel->stage <= MESA_SHADER_GEOMETRY &&
      sscreen->use_ngg && (!sel->info.enabled_streamout_buffer_mask ||
         ((sel->stage == MESA_SHADER_VERTEX && !shader->key.ge.as_ls) ||
                        if (sel->nir) {
      if (sel->stage <= MESA_SHADER_GEOMETRY) {
      si_get_ir_cache_key(sel, shader->key.ge.as_ngg, shader->key.ge.as_es,
      } else {
                     /* Try to load the shader from the shader cache. */
            if (si_shader_cache_load_shader(sscreen, ir_sha1_cache_key, shader)) {
      simple_mtx_unlock(&sscreen->shader_cache_mutex);
      } else {
               /* Compile the shader if it hasn't been loaded from the cache. */
   if (!si_compile_shader(sscreen, *compiler, shader, debug)) {
      fprintf(stderr,
      "radeonsi: can't compile a main shader part (type: %s, name: %s).\n"
   "This is probably a driver bug, please report "
   "it to https://gitlab.freedesktop.org/mesa/mesa/-/issues.\n",
   gl_shader_stage_name(shader->selector->stage),
      FREE(shader);
               simple_mtx_lock(&sscreen->shader_cache_mutex);
   si_shader_cache_insert_shader(sscreen, ir_sha1_cache_key, shader, true);
                        /* Unset "outputs_written" flags for outputs converted to
   * DEFAULT_VAL, so that later inter-shader optimizations don't
   * try to eliminate outputs that don't exist in the final
   * shader.
   *
   * This is only done if non-monolithic shaders are enabled.
   */
   if ((sel->stage == MESA_SHADER_VERTEX ||
      sel->stage == MESA_SHADER_TESS_EVAL ||
   sel->stage == MESA_SHADER_GEOMETRY) &&
                  for (i = 0; i < sel->info.num_outputs; i++) {
                     /* OFFSET=0x20 means DEFAULT_VAL, which means VS doesn't export it. */
                           /* Remove the output from the mask. */
   if ((semantic <= VARYING_SLOT_VAR31 || semantic >= VARYING_SLOT_VAR0_16BIT) &&
      semantic != VARYING_SLOT_POS &&
   semantic != VARYING_SLOT_PSIZ &&
   semantic != VARYING_SLOT_CLIP_VERTEX &&
   semantic != VARYING_SLOT_EDGE) {
   id = si_shader_io_get_unique_index(semantic);
                        /* Free NIR. We only keep serialized NIR after this point. */
   if (sel->nir) {
      ralloc_free(sel->nir);
         }
      void si_schedule_initial_compile(struct si_context *sctx, gl_shader_stage stage,
                     {
               struct util_async_debug_callback async_debug;
   bool debug = (sctx->debug.debug_message && !sctx->debug.async) || sctx->is_debug ||
            if (debug) {
      u_async_debug_init(&async_debug);
                        if (debug) {
      util_queue_fence_wait(ready_fence);
   u_async_debug_drain(&async_debug, &sctx->debug);
               if (sctx->screen->options.sync_compile)
      }
      /* Return descriptor slot usage masks from the given shader info. */
   void si_get_active_slot_masks(struct si_screen *sscreen, const struct si_shader_info *info,
         {
               num_shaderbufs = info->base.num_ssbos;
   num_constbufs = info->base.num_ubos;
   /* two 8-byte images share one 16-byte slot */
   num_images = align(info->base.num_images, 2);
   num_msaa_images = align(BITSET_LAST_BIT(info->base.msaa_images), 2);
            /* The layout is: sb[last] ... sb[0], cb[0] ... cb[last] */
   start = si_get_shaderbuf_slot(num_shaderbufs - 1);
            /* The layout is:
   *   - fmask[last] ... fmask[0]     go to [15-last .. 15]
   *   - image[last] ... image[0]     go to [31-last .. 31]
   *   - sampler[0] ... sampler[last] go to [32 .. 32+last*2]
   *
   * FMASKs for images are placed separately, because MSAA images are rare,
   * and so we can benefit from a better cache hit rate if we keep image
   * descriptors together.
   */
   if (sscreen->info.gfx_level < GFX11 && num_msaa_images)
            start = si_get_image_slot(num_images - 1) / 2;
      }
      static void *si_create_shader_selector(struct pipe_context *ctx,
         {
      struct si_screen *sscreen = (struct si_screen *)ctx->screen;
   struct si_context *sctx = (struct si_context *)ctx;
            if (!sel)
            sel->screen = sscreen;
   sel->compiler_ctx_state.debug = sctx->debug;
   sel->compiler_ctx_state.is_debug_context = sctx->is_debug;
   sel->variants_max_count = 2;
   sel->keys = (union si_shader_key *)
         sel->variants = (struct si_shader **)
            if (state->type == PIPE_SHADER_IR_TGSI) {
         } else {
      assert(state->type == PIPE_SHADER_IR_NIR);
                        sel->stage = sel->nir->info.stage;
   const enum pipe_shader_type type = pipe_shader_type_from_mesa(sel->stage);
   sel->pipe_shader_type = type;
   sel->const_and_shader_buf_descriptors_index =
         sel->sampler_and_images_descriptors_index =
            if (si_can_dump_shader(sscreen, sel->stage, SI_DUMP_INIT_NIR))
            p_atomic_inc(&sscreen->num_shaders_created);
   si_get_active_slot_masks(sscreen, &sel->info, &sel->active_const_and_shader_buffers,
            switch (sel->stage) {
   case MESA_SHADER_GEOMETRY:
      /* Only possibilities: POINTS, LINE_STRIP, TRIANGLES */
   sel->rast_prim = (enum mesa_prim)sel->info.base.gs.output_primitive;
   if (util_rast_prim_is_triangles(sel->rast_prim))
            /* EN_MAX_VERT_OUT_PER_GS_INSTANCE does not work with tessellation so
   * we can't split workgroups. Disable ngg if any of the following conditions is true:
   * - num_invocations * gs.vertices_out > 256
   * - LDS usage is too high
   */
   sel->tess_turns_off_ngg = sscreen->info.gfx_level >= GFX10 &&
                                 case MESA_SHADER_VERTEX:
   case MESA_SHADER_TESS_EVAL:
      if (sel->stage == MESA_SHADER_TESS_EVAL) {
      if (sel->info.base.tess.point_mode)
         else if (sel->info.base.tess._primitive_mode == TESS_PRIMITIVE_ISOLINES)
         else
      } else {
         }
      default:;
            bool ngg_culling_allowed =
      sscreen->info.gfx_level >= GFX10 &&
   sscreen->use_ngg_culling &&
   sel->info.writes_position &&
   !sel->info.writes_viewport_index && /* cull only against viewport 0 */
   !sel->info.base.writes_memory &&
   /* NGG GS supports culling with streamout because it culls after streamout. */
   (sel->stage == MESA_SHADER_GEOMETRY || !sel->info.enabled_streamout_buffer_mask) &&
   (sel->stage != MESA_SHADER_GEOMETRY || sel->info.num_stream_output_components[0]) &&
   (sel->stage != MESA_SHADER_VERTEX ||
   (!sel->info.base.vs.blit_sgprs_amd &&
                  if (ngg_culling_allowed) {
      if (sel->stage == MESA_SHADER_VERTEX) {
      if (sscreen->debug_flags & DBG(ALWAYS_NGG_CULLING_ALL))
         else
      } else if (sel->stage == MESA_SHADER_TESS_EVAL ||
            if (sel->rast_prim != MESA_PRIM_POINTS)
                           si_schedule_initial_compile(sctx, sel->stage, &sel->ready, &sel->compiler_ctx_state,
            }
      static void *si_create_shader(struct pipe_context *ctx, const struct pipe_shader_state *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_screen *sscreen = (struct si_screen *)ctx->screen;
   bool cache_hit;
   struct si_shader_selector *sel = (struct si_shader_selector *)util_live_shader_cache_get(
            if (sel && cache_hit && sctx->debug.debug_message) {
      if (sel->main_shader_part)
         if (sel->main_shader_part_ls)
         if (sel->main_shader_part_es)
         if (sel->main_shader_part_ngg)
         if (sel->main_shader_part_ngg_es)
      }
      }
      static void si_update_streamout_state(struct si_context *sctx)
   {
               if (!shader_with_so)
            sctx->streamout.enabled_stream_buffers_mask = shader_with_so->info.enabled_streamout_buffer_mask;
            /* GDS must be allocated when any GDS instructions are used, otherwise it hangs. */
   if (sctx->gfx_level >= GFX11 && shader_with_so->info.enabled_streamout_buffer_mask)
      }
      static void si_update_clip_regs(struct si_context *sctx, struct si_shader_selector *old_hw_vs,
                     {
      if (next_hw_vs &&
      (!old_hw_vs ||
   (old_hw_vs->stage == MESA_SHADER_VERTEX && old_hw_vs->info.base.vs.window_space_position) !=
   (next_hw_vs->stage == MESA_SHADER_VERTEX && next_hw_vs->info.base.vs.window_space_position) ||
   old_hw_vs->info.clipdist_mask != next_hw_vs->info.clipdist_mask ||
   old_hw_vs->info.culldist_mask != next_hw_vs->info.culldist_mask || !old_hw_vs_variant ||
   !next_hw_vs_variant ||
   old_hw_vs_variant->pa_cl_vs_out_cntl != next_hw_vs_variant->pa_cl_vs_out_cntl))
   }
      static void si_update_rasterized_prim(struct si_context *sctx)
   {
               if (sctx->shader.gs.cso) {
      /* Only possibilities: POINTS, LINE_STRIP, TRIANGLES */
      } else if (sctx->shader.tes.cso) {
      /* Only possibilities: POINTS, LINE_STRIP, TRIANGLES */
      } else {
                  /* This must be done unconditionally because it also depends on si_shader fields. */
      }
      static void si_update_common_shader_state(struct si_context *sctx, struct si_shader_selector *sel,
         {
               sctx->uses_bindless_samplers = si_shader_uses_bindless_samplers(sctx->shader.vs.cso) ||
                           sctx->uses_bindless_images = si_shader_uses_bindless_images(sctx->shader.vs.cso) ||
                              if (type == PIPE_SHADER_VERTEX || type == PIPE_SHADER_TESS_EVAL || type == PIPE_SHADER_GEOMETRY)
            si_invalidate_inlinable_uniforms(sctx, type);
      }
      static void si_update_last_vgt_stage_state(struct si_context *sctx,
                     {
      si_update_vs_viewport_state(sctx);
   si_update_streamout_state(sctx);
   si_update_clip_regs(sctx, old_hw_vs, old_hw_vs_variant, si_get_vs(sctx)->cso,
            }
      static void si_bind_vs_shader(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_shader_selector *old_hw_vs = si_get_vs(sctx)->cso;
   struct si_shader *old_hw_vs_variant = si_get_vs(sctx)->current;
            if (sctx->shader.vs.cso == sel)
            sctx->shader.vs.cso = sel;
   sctx->shader.vs.current = (sel && sel->variants_count) ? sel->variants[0] : NULL;
   sctx->num_vs_blit_sgprs = sel ? sel->info.base.vs.blit_sgprs_amd : 0;
            if (si_update_ngg(sctx))
            si_update_common_shader_state(sctx, sel, PIPE_SHADER_VERTEX);
   si_select_draw_vbo(sctx);
   si_update_last_vgt_stage_state(sctx, old_hw_vs, old_hw_vs_variant);
            if (sctx->screen->dpbb_allowed) {
               if (force_off != sctx->dpbb_force_off_profile_vs) {
      sctx->dpbb_force_off_profile_vs = force_off;
            }
      static void si_update_tess_uses_prim_id(struct si_context *sctx)
   {
      sctx->ia_multi_vgt_param_key.u.tess_uses_prim_id =
      (sctx->shader.tes.cso && sctx->shader.tes.cso->info.uses_primid) ||
   (sctx->shader.tcs.cso && sctx->shader.tcs.cso->info.uses_primid) ||
   (sctx->shader.gs.cso && sctx->shader.gs.cso->info.uses_primid) ||
   }
      bool si_update_ngg(struct si_context *sctx)
   {
      if (!sctx->screen->use_ngg) {
      assert(!sctx->ngg);
                        if (sctx->shader.gs.cso && sctx->shader.tes.cso && sctx->shader.gs.cso->tess_turns_off_ngg) {
         } else if (sctx->gfx_level < GFX11) {
               if ((last && last->info.enabled_streamout_buffer_mask) ||
      sctx->streamout.prims_gen_query_enabled)
            if (new_ngg != sctx->ngg) {
      /* Transitioning from NGG to legacy GS requires VGT_FLUSH on Navi10-14.
   * VGT_FLUSH is also emitted at the beginning of IBs when legacy GS ring
   * pointers are set.
   */
   if (sctx->screen->info.has_vgt_flush_ngg_legacy_bug && !new_ngg) {
                     if (sctx->gfx_level == GFX10) {
      /* Workaround for https://gitlab.freedesktop.org/mesa/mesa/-/issues/2941 */
                  sctx->ngg = new_ngg;
   si_select_draw_vbo(sctx);
      }
      }
      static void si_bind_gs_shader(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_shader_selector *old_hw_vs = si_get_vs(sctx)->cso;
   struct si_shader *old_hw_vs_variant = si_get_vs(sctx)->current;
   struct si_shader_selector *sel = (struct si_shader_selector*)state;
   bool enable_changed = !!sctx->shader.gs.cso != !!sel;
            if (sctx->shader.gs.cso == sel)
            sctx->shader.gs.cso = sel;
   sctx->shader.gs.current = (sel && sel->variants_count) ? sel->variants[0] : NULL;
            si_update_common_shader_state(sctx, sel, PIPE_SHADER_GEOMETRY);
            ngg_changed = si_update_ngg(sctx);
   if (ngg_changed || enable_changed)
         if (enable_changed) {
      if (sctx->ia_multi_vgt_param_key.u.uses_tess)
      }
      }
      static void si_bind_tcs_shader(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_shader_selector *sel = (struct si_shader_selector*)state;
            /* Note it could happen that user shader sel is same as fixed function shader,
   * so we should update this field even sctx->shader.tcs.cso == sel.
   */
            if (sctx->shader.tcs.cso == sel)
            sctx->shader.tcs.cso = sel;
   sctx->shader.tcs.current = (sel && sel->variants_count) ? sel->variants[0] : NULL;
   sctx->shader.tcs.key.ge.part.tcs.epilog.invoc0_tess_factors_are_def =
         si_update_tess_uses_prim_id(sctx);
                     if (enable_changed)
      }
      static void si_bind_tes_shader(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_shader_selector *old_hw_vs = si_get_vs(sctx)->cso;
   struct si_shader *old_hw_vs_variant = si_get_vs(sctx)->current;
   struct si_shader_selector *sel = (struct si_shader_selector*)state;
            if (sctx->shader.tes.cso == sel)
            sctx->shader.tes.cso = sel;
   sctx->shader.tes.current = (sel && sel->variants_count) ? sel->variants[0] : NULL;
   sctx->ia_multi_vgt_param_key.u.uses_tess = sel != NULL;
            sctx->shader.tcs.key.ge.part.tcs.epilog.prim_mode =
            sctx->shader.tcs.key.ge.part.tcs.epilog.tes_reads_tess_factors =
            si_update_common_shader_state(sctx, sel, PIPE_SHADER_TESS_EVAL);
            bool ngg_changed = si_update_ngg(sctx);
   if (ngg_changed || enable_changed)
         if (enable_changed)
            }
      void si_update_vrs_flat_shading(struct si_context *sctx)
   {
      if (sctx->gfx_level >= GFX10_3 && sctx->shader.ps.cso) {
      struct si_state_rasterizer *rs = sctx->queued.named.rasterizer;
   struct si_shader_info *info = &sctx->shader.ps.cso->info;
            if (allow_flat_shading &&
      (rs->line_smooth || rs->poly_smooth || rs->poly_stipple_enable ||
               if (sctx->allow_flat_shading != allow_flat_shading) {
      sctx->allow_flat_shading = allow_flat_shading;
            }
      static void si_bind_ps_shader(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_shader_selector *old_sel = sctx->shader.ps.cso;
            /* skip if supplied shader is one already in use */
   if (old_sel == sel)
            sctx->shader.ps.cso = sel;
            si_update_common_shader_state(sctx, sel, PIPE_SHADER_FRAGMENT);
   if (sel) {
      if (sctx->ia_multi_vgt_param_key.u.uses_tess)
            if (!old_sel || old_sel->info.colors_written != sel->info.colors_written)
            if (sctx->screen->info.has_out_of_order_rast &&
      (!old_sel || old_sel->info.base.writes_memory != sel->info.base.writes_memory ||
   old_sel->info.base.fs.early_fragment_tests !=
         }
            si_ps_key_update_framebuffer(sctx);
   si_ps_key_update_framebuffer_blend_rasterizer(sctx);
   si_ps_key_update_rasterizer(sctx);
   si_ps_key_update_dsa(sctx);
   si_ps_key_update_sample_shading(sctx);
   si_ps_key_update_framebuffer_rasterizer_sample_shading(sctx);
   si_update_ps_inputs_read_or_disabled(sctx);
            if (sctx->screen->dpbb_allowed) {
               if (force_off != sctx->dpbb_force_off_profile_ps) {
      sctx->dpbb_force_off_profile_ps = force_off;
            }
      static void si_delete_shader(struct si_context *sctx, struct si_shader *shader)
   {
      if (shader->is_optimized) {
                           /* If destroyed shaders were not unbound, the next compiled
   * shader variant could get the same pointer address and so
   * binding it to the same shader stage would be considered
   * a no-op, causing random behavior.
   */
            switch (shader->selector->stage) {
   case MESA_SHADER_VERTEX:
      if (shader->key.ge.as_ls) {
      if (sctx->gfx_level <= GFX8)
      } else if (shader->key.ge.as_es) {
      if (sctx->gfx_level <= GFX8)
      } else if (shader->key.ge.as_ngg) {
         } else {
         }
      case MESA_SHADER_TESS_CTRL:
      state_index = SI_STATE_IDX(hs);
      case MESA_SHADER_TESS_EVAL:
      if (shader->key.ge.as_es) {
      if (sctx->gfx_level <= GFX8)
      } else if (shader->key.ge.as_ngg) {
         } else {
         }
      case MESA_SHADER_GEOMETRY:
      if (shader->is_gs_copy_shader)
         else
            case MESA_SHADER_FRAGMENT:
      state_index = SI_STATE_IDX(ps);
      default:;
            if (shader->gs_copy_shader)
            si_shader_selector_reference(sctx, &shader->previous_stage_sel, NULL);
   si_shader_destroy(shader);
      }
      static void si_destroy_shader_selector(struct pipe_context *ctx, void *cso)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_shader_selector *sel = (struct si_shader_selector *)cso;
                     if (sctx->shaders[type].cso == sel) {
      sctx->shaders[type].cso = NULL;
               for (unsigned i = 0; i < sel->variants_count; i++) {
                  if (sel->main_shader_part)
         if (sel->main_shader_part_ls)
         if (sel->main_shader_part_es)
         if (sel->main_shader_part_ngg)
            free(sel->keys);
            util_queue_fence_destroy(&sel->ready);
   simple_mtx_destroy(&sel->mutex);
   ralloc_free(sel->nir);
   free(sel->nir_binary);
      }
      static void si_delete_shader_selector(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
               }
      /**
   * Writing CONFIG or UCONFIG VGT registers requires VGT_FLUSH before that.
   */
   static void si_cs_preamble_add_vgt_flush(struct si_context *sctx, bool tmz)
   {
      struct si_pm4_state *pm4 = tmz ? sctx->cs_preamble_state_tmz : sctx->cs_preamble_state;
   bool *has_vgt_flush = tmz ? &sctx->cs_preamble_has_vgt_flush_tmz :
            /* We shouldn't get here if registers are shadowed. */
            if (*has_vgt_flush)
            /* Done by Vulkan before VGT_FLUSH. */
   si_pm4_cmd_add(pm4, PKT3(PKT3_EVENT_WRITE, 0, 0));
            /* VGT_FLUSH is required even if VGT is idle. It resets VGT pointers. */
   si_pm4_cmd_add(pm4, PKT3(PKT3_EVENT_WRITE, 0, 0));
   si_pm4_cmd_add(pm4, EVENT_TYPE(V_028A90_VGT_FLUSH) | EVENT_INDEX(0));
               }
      /**
   * Writing CONFIG or UCONFIG VGT registers requires VGT_FLUSH before that.
   */
   static void si_emit_vgt_flush(struct radeon_cmdbuf *cs)
   {
               /* This is required before VGT_FLUSH. */
   radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
            /* VGT_FLUSH is required even if VGT is idle. It resets VGT pointers. */
   radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
   radeon_emit(EVENT_TYPE(V_028A90_VGT_FLUSH) | EVENT_INDEX(0));
      }
      /* Initialize state related to ESGS / GSVS ring buffers */
   bool si_update_gs_ring_buffers(struct si_context *sctx)
   {
               struct si_shader_selector *es =
                  /* Chip constants. */
   unsigned num_se = sctx->screen->info.max_se;
   unsigned wave_size = 64;
   unsigned max_gs_waves = 32 * num_se; /* max 32 per SE on GCN */
   /* On GFX6-GFX7, the value comes from VGT_GS_VERTEX_REUSE = 16.
   * On GFX8+, the value comes from VGT_VERTEX_REUSE_BLOCK_CNTL = 30 (+2).
   */
   unsigned gs_vertex_reuse = (sctx->gfx_level >= GFX8 ? 32 : 16) * num_se;
   unsigned alignment = 256 * num_se;
   /* The maximum size is 63.999 MB per SE. */
            /* Calculate the minimum size. */
            /* These are recommended sizes, not minimum sizes. */
   unsigned esgs_ring_size =
                  min_esgs_ring_size = align(min_esgs_ring_size, alignment);
   esgs_ring_size = align(esgs_ring_size, alignment);
            esgs_ring_size = CLAMP(esgs_ring_size, min_esgs_ring_size, max_size);
            /* Some rings don't have to be allocated if shaders don't use them.
   * (e.g. no varyings between ES and GS or GS and VS)
   *
   * GFX9 doesn't have the ESGS ring.
   */
   bool update_esgs = sctx->gfx_level <= GFX8 && esgs_ring_size &&
         bool update_gsvs =
            if (!update_esgs && !update_gsvs)
            if (update_esgs) {
      pipe_resource_reference(&sctx->esgs_ring, NULL);
   sctx->esgs_ring =
      pipe_aligned_buffer_create(sctx->b.screen,
                        if (!sctx->esgs_ring)
               if (update_gsvs) {
      pipe_resource_reference(&sctx->gsvs_ring, NULL);
   sctx->gsvs_ring =
      pipe_aligned_buffer_create(sctx->b.screen,
                        if (!sctx->gsvs_ring)
               /* Set ring bindings. */
   if (sctx->esgs_ring) {
      assert(sctx->gfx_level <= GFX8);
   si_set_ring_buffer(sctx, SI_RING_ESGS, sctx->esgs_ring, 0, sctx->esgs_ring->width0, false,
      }
   if (sctx->gsvs_ring) {
      si_set_ring_buffer(sctx, SI_RING_GSVS, sctx->gsvs_ring, 0, sctx->gsvs_ring->width0, false,
               if (sctx->shadowing.registers) {
      /* These registers will be shadowed, so set them only once. */
                                       /* Set the GS registers. */
   if (sctx->esgs_ring) {
      assert(sctx->gfx_level <= GFX8);
   radeon_set_uconfig_reg(R_030900_VGT_ESGS_RING_SIZE,
      }
   if (sctx->gsvs_ring) {
      radeon_set_uconfig_reg(R_030904_VGT_GSVS_RING_SIZE,
      }
   radeon_end();
               /* The codepath without register shadowing. */
   for (unsigned tmz = 0; tmz <= 1; tmz++) {
      struct si_pm4_state *pm4 = tmz ? sctx->cs_preamble_state_tmz : sctx->cs_preamble_state;
   uint16_t *gs_ring_state_dw_offset = tmz ? &sctx->gs_ring_state_dw_offset_tmz :
                           if (!*gs_ring_state_dw_offset) {
      /* We are here for the first time. The packets will be added. */
      } else {
      /* We have been here before. Overwrite the previous packets. */
   old_ndw = pm4->ndw;
               /* Unallocated rings are written to reserve the space in the pm4
   * (to be able to overwrite them later). */
   if (sctx->gfx_level >= GFX7) {
      if (sctx->gfx_level <= GFX8)
      si_pm4_set_reg(pm4, R_030900_VGT_ESGS_RING_SIZE,
      si_pm4_set_reg(pm4, R_030904_VGT_GSVS_RING_SIZE,
      } else {
      si_pm4_set_reg(pm4, R_0088C8_VGT_ESGS_RING_SIZE,
         si_pm4_set_reg(pm4, R_0088CC_VGT_GSVS_RING_SIZE,
      }
            if (old_ndw) {
      pm4->ndw = old_ndw;
                  /* Flush the context to re-emit both cs_preamble states. */
   sctx->initial_gfx_cs_size = 0; /* force flush */
               }
      static void si_shader_lock(struct si_shader *shader)
   {
      simple_mtx_lock(&shader->selector->mutex);
   if (shader->previous_stage_sel) {
      assert(shader->previous_stage_sel != shader->selector);
         }
      static void si_shader_unlock(struct si_shader *shader)
   {
      if (shader->previous_stage_sel)
            }
      /**
   * @returns 1 if \p sel has been updated to use a new scratch buffer
   *          0 if not
   *          < 0 if there was a failure
   */
   static int si_update_scratch_buffer(struct si_context *sctx, struct si_shader *shader)
   {
               if (!shader)
            /* This shader doesn't need a scratch buffer */
   if (shader->config.scratch_bytes_per_wave == 0)
            /* Prevent race conditions when updating:
   * - si_shader::scratch_bo
   * - si_shader::binary::code
   * - si_shader::previous_stage::binary::code.
   */
            /* This shader is already configured to use the current
   * scratch buffer. */
   if (shader->scratch_bo == sctx->scratch_buffer) {
      si_shader_unlock(shader);
                        /* Replace the shader bo with a new bo that has the relocs applied. */
   if (!si_shader_binary_upload(sctx->screen, shader, scratch_va)) {
      si_shader_unlock(shader);
               /* Update the shader state to use the new shader bo. */
                     si_shader_unlock(shader);
      }
      static bool si_update_scratch_relocs(struct si_context *sctx)
   {
               /* Update the shaders, so that they are using the latest scratch.
   * The scratch buffer may have been changed since these shaders were
   * last used, so we still need to try to update them, even if they
   * require scratch buffers smaller than the current size.
   */
   r = si_update_scratch_buffer(sctx, sctx->shader.ps.current);
   if (r < 0)
         if (r == 1)
            r = si_update_scratch_buffer(sctx, sctx->shader.gs.current);
   if (r < 0)
         if (r == 1)
            r = si_update_scratch_buffer(sctx, sctx->shader.tcs.current);
   if (r < 0)
         if (r == 1)
            /* VS can be bound as LS, ES, or VS. */
   r = si_update_scratch_buffer(sctx, sctx->shader.vs.current);
   if (r < 0)
         if (r == 1) {
      if (sctx->shader.vs.current->key.ge.as_ls)
         else if (sctx->shader.vs.current->key.ge.as_es)
         else if (sctx->shader.vs.current->key.ge.as_ngg)
         else
               /* TES can be bound as ES or VS. */
   r = si_update_scratch_buffer(sctx, sctx->shader.tes.current);
   if (r < 0)
         if (r == 1) {
      if (sctx->shader.tes.current->key.ge.as_es)
         else if (sctx->shader.tes.current->key.ge.as_ngg)
         else
                  }
      bool si_update_spi_tmpring_size(struct si_context *sctx, unsigned bytes)
   {
      unsigned spi_tmpring_size;
   ac_get_scratch_tmpring_size(&sctx->screen->info, bytes,
            unsigned scratch_needed_size = sctx->max_seen_scratch_bytes_per_wave *
            if (scratch_needed_size > 0) {
      if (!sctx->scratch_buffer || scratch_needed_size > sctx->scratch_buffer->b.b.width0) {
                     sctx->scratch_buffer = si_aligned_buffer_create(
      &sctx->screen->b,
   PIPE_RESOURCE_FLAG_UNMAPPABLE | SI_RESOURCE_FLAG_DRIVER_INTERNAL |
   SI_RESOURCE_FLAG_DISCARDABLE,
   PIPE_USAGE_DEFAULT, scratch_needed_size,
      if (!sctx->scratch_buffer)
               if (sctx->gfx_level < GFX11 && !si_update_scratch_relocs(sctx))
               if (spi_tmpring_size != sctx->spi_tmpring_size) {
      sctx->spi_tmpring_size = spi_tmpring_size;
      }
      }
      void si_init_tess_factor_ring(struct si_context *sctx)
   {
               /* The address must be aligned to 2^19, because the shader only
   * receives the high 13 bits. Align it to 2MB to match the GPU page size.
   */
   sctx->tess_rings = pipe_aligned_buffer_create(sctx->b.screen,
                                                   if (!sctx->tess_rings)
            if (sctx->screen->info.has_tmz_support) {
      sctx->tess_rings_tmz = pipe_aligned_buffer_create(sctx->b.screen,
                                                               uint64_t factor_va =
            unsigned tf_ring_size_field = sctx->screen->hs.tess_factor_ring_size / 4;
   if (sctx->gfx_level >= GFX11)
                     if (sctx->shadowing.registers) {
      /* These registers will be shadowed, so set them only once. */
   /* TODO: tmz + shadowed_regs support */
                     radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(sctx->tess_rings),
                  /* Set tessellation registers. */
   radeon_begin(cs);
   radeon_set_uconfig_reg(R_030938_VGT_TF_RING_SIZE,
         radeon_set_uconfig_reg(R_030940_VGT_TF_MEMORY_BASE, factor_va >> 8);
   if (sctx->gfx_level >= GFX10) {
      radeon_set_uconfig_reg(R_030984_VGT_TF_MEMORY_BASE_HI,
      } else if (sctx->gfx_level == GFX9) {
      radeon_set_uconfig_reg(R_030944_VGT_TF_MEMORY_BASE_HI,
      }
   radeon_set_uconfig_reg(R_03093C_VGT_HS_OFFCHIP_PARAM,
         radeon_end();
               /* The codepath without register shadowing is below. */
   /* Add these registers to cs_preamble_state. */
   for (unsigned tmz = 0; tmz <= 1; tmz++) {
      struct si_pm4_state *pm4 = tmz ? sctx->cs_preamble_state_tmz : sctx->cs_preamble_state;
            if (!tf_ring)
                              if (sctx->gfx_level >= GFX7) {
      si_pm4_set_reg(pm4, R_030938_VGT_TF_RING_SIZE, S_030938_SIZE(tf_ring_size_field));
   si_pm4_set_reg(pm4, R_03093C_VGT_HS_OFFCHIP_PARAM, sctx->screen->hs.hs_offchip_param);
   si_pm4_set_reg(pm4, R_030940_VGT_TF_MEMORY_BASE, va >> 8);
   if (sctx->gfx_level >= GFX10)
         else if (sctx->gfx_level == GFX9)
      } else {
      si_pm4_set_reg(pm4, R_008988_VGT_TF_RING_SIZE, S_008988_SIZE(tf_ring_size_field));
   si_pm4_set_reg(pm4, R_0089B8_VGT_TF_MEMORY_BASE, factor_va >> 8);
      }
               /* Flush the context to re-emit the cs_preamble state.
   * This is done only once in a lifetime of a context.
   */
   sctx->initial_gfx_cs_size = 0; /* force flush */
      }
      static void si_emit_vgt_pipeline_state(struct si_context *sctx, unsigned index)
   {
               radeon_begin(cs);
   radeon_opt_set_context_reg(sctx, R_028B54_VGT_SHADER_STAGES_EN, SI_TRACKED_VGT_SHADER_STAGES_EN,
                  if (sctx->gfx_level >= GFX10) {
               if (sctx->gfx_level < GFX11 && sctx->shader.tes.cso) {
      /* This must be a multiple of VGT_LS_HS_CONFIG.NUM_PATCHES. */
               radeon_begin_again(cs);
   radeon_opt_set_uconfig_reg(sctx, R_03096C_GE_CNTL, SI_TRACKED_GE_CNTL, ge_cntl);
         }
      static void si_emit_scratch_state(struct si_context *sctx, unsigned index)
   {
               radeon_begin(cs);
   if (sctx->gfx_level >= GFX11) {
      radeon_set_context_reg_seq(R_0286E8_SPI_TMPRING_SIZE, 3);
   radeon_emit(sctx->spi_tmpring_size);                  /* SPI_TMPRING_SIZE */
   radeon_emit(sctx->scratch_buffer->gpu_address >> 8);  /* SPI_GFX_SCRATCH_BASE_LO */
      } else {
         }
            if (sctx->scratch_buffer) {
      radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, sctx->scratch_buffer,
         }
      struct si_fixed_func_tcs_shader_key {
      uint64_t outputs_written;
      };
      static uint32_t si_fixed_func_tcs_shader_key_hash(const void *key)
   {
         }
      static bool si_fixed_func_tcs_shader_key_equals(const void *a, const void *b)
   {
         }
      bool si_set_tcs_to_fixed_func_shader(struct si_context *sctx)
   {
      if (!sctx->fixed_func_tcs_shader_cache) {
      sctx->fixed_func_tcs_shader_cache = _mesa_hash_table_create(
      NULL, si_fixed_func_tcs_shader_key_hash,
            struct si_fixed_func_tcs_shader_key key;
   key.outputs_written = sctx->shader.vs.cso->info.outputs_written;
            struct hash_entry *entry = _mesa_hash_table_search(
            struct si_shader_selector *tcs;
   if (entry)
         else {
      tcs = (struct si_shader_selector *)si_create_passthrough_tcs(sctx);
   if (!tcs)
                     sctx->shader.tcs.cso = tcs;
   sctx->shader.tcs.key.ge.part.tcs.epilog.invoc0_tess_factors_are_def =
               }
      static void si_update_tess_in_out_patch_vertices(struct si_context *sctx)
   {
      if (sctx->is_user_tcs) {
               bool same_patch_vertices =
                  if (sctx->shader.tcs.key.ge.opt.same_patch_vertices != same_patch_vertices) {
      sctx->shader.tcs.key.ge.opt.same_patch_vertices = same_patch_vertices;
               if (sctx->gfx_level == GFX9 && sctx->screen->info.has_ls_vgpr_init_bug) {
      /* Determine whether the LS VGPR fix should be applied.
   *
   * It is only required when num input CPs > num output CPs,
   * which cannot happen with the fixed function TCS.
   */
                  if (ls_vgpr_fix != sctx->shader.tcs.key.ge.part.tcs.ls_prolog.ls_vgpr_fix) {
      sctx->shader.tcs.key.ge.part.tcs.ls_prolog.ls_vgpr_fix = ls_vgpr_fix;
            } else {
      /* These fields are static for fixed function TCS. So no need to set
   * do_update_shaders between fixed-TCS draws. As fixed-TCS to user-TCS
   * or opposite, do_update_shaders should already be set by bind state.
   */
   sctx->shader.tcs.key.ge.opt.same_patch_vertices = sctx->gfx_level >= GFX9;
            /* User may only change patch vertices, needs to update fixed func TCS. */
   if (sctx->shader.tcs.cso &&
      sctx->shader.tcs.cso->info.base.tess.tcs_vertices_out != sctx->patch_vertices)
      }
      static void si_set_patch_vertices(struct pipe_context *ctx, uint8_t patch_vertices)
   {
               if (sctx->patch_vertices != patch_vertices) {
      sctx->patch_vertices = patch_vertices;
   si_update_tess_in_out_patch_vertices(sctx);
   if (sctx->shader.tcs.current) {
      /* Update the io layout now if possible,
   * otherwise make sure it's done by si_update_shaders.
   */
   if (sctx->tess_rings)
         else
               }
      /**
   * This calculates the LDS size for tessellation shaders (VS, TCS, TES).
   * LS.LDS_SIZE is shared by all 3 shader stages.
   *
   * The information about LDS and other non-compile-time parameters is then
   * written to userdata SGPRs.
   *
   * This depends on:
   * - patch_vertices
   * - VS and the currently selected shader variant (called by si_update_shaders)
   * - TCS and the currently selected shader variant (called by si_update_shaders)
   * - tess_uses_prim_id (called by si_update_shaders)
   * - sh_base[TESS_EVAL] depending on GS on/off (called by si_update_shaders)
   */
   void si_update_tess_io_layout_state(struct si_context *sctx)
   {
      struct si_shader *ls_current;
   struct si_shader_selector *ls;
   struct si_shader_selector *tcs = sctx->shader.tcs.cso;
   unsigned tess_uses_primid = sctx->ia_multi_vgt_param_key.u.tess_uses_prim_id;
   bool has_primid_instancing_bug = sctx->gfx_level == GFX6 && sctx->screen->info.max_se == 1;
   unsigned tes_sh_base = sctx->shader_pointers.sh_base[PIPE_SHADER_TESS_EVAL];
                     /* Since GFX9 has merged LS-HS in the TCS state, set LS = TCS. */
   if (sctx->gfx_level >= GFX9) {
      ls_current = sctx->shader.tcs.current;
      } else {
      ls_current = sctx->shader.vs.current;
               if (sctx->last_ls == ls_current && sctx->last_tcs == tcs &&
      sctx->last_tes_sh_base == tes_sh_base && sctx->last_num_tcs_input_cp == num_tcs_input_cp &&
   (!has_primid_instancing_bug || (sctx->last_tess_uses_primid == tess_uses_primid)))
         sctx->last_ls = ls_current;
   sctx->last_tcs = tcs;
   sctx->last_tes_sh_base = tes_sh_base;
   sctx->last_num_tcs_input_cp = num_tcs_input_cp;
            /* This calculates how shader inputs and outputs among VS, TCS, and TES
   * are laid out in LDS. */
   unsigned num_tcs_outputs = util_last_bit64(tcs->info.outputs_written);
   unsigned num_tcs_output_cp = tcs->info.base.tess.tcs_vertices_out;
            unsigned input_vertex_size = ls->info.lshs_vertex_stride;
   unsigned output_vertex_size = num_tcs_outputs * 16;
            /* Allocate LDS for TCS inputs only if it's used. */
   if (!ls_current->key.ge.opt.same_patch_vertices ||
      tcs->info.base.inputs_read & ~tcs->info.tcs_vgpr_only_inputs)
      else
            unsigned pervertex_output_patch_size = num_tcs_output_cp * output_vertex_size;
   unsigned output_patch_size = pervertex_output_patch_size + num_tcs_patch_outputs * 16;
            /* Compute the LDS size per patch.
   *
   * LDS is used to store TCS outputs if they are read, and to store tess
   * factors if they are not defined in all invocations.
   */
   if (tcs->info.base.outputs_read ||
      tcs->info.base.patch_outputs_read ||
   !tcs->info.tessfactors_are_def_in_all_invocs) {
      } else {
      /* LDS will only store TCS inputs. The offchip buffer will only store TCS outputs. */
               /* Ensure that we only need 4 waves per CU, so that we don't need to check
   * resource usage (such as whether we have enough VGPRs to fit the whole
   * threadgroup into the CU). It also ensures that the number of tcs in and out
   * vertices per threadgroup are at most 256, which is the hw limit.
   */
   unsigned max_verts_per_patch = MAX2(num_tcs_input_cp, num_tcs_output_cp);
            /* Not necessary for correctness, but higher numbers are slower.
   * The hardware can do more, but the radeonsi shader constant is
   * limited to 6 bits.
   */
            /* When distributed tessellation is unsupported, switch between SEs
   * at a higher frequency to manually balance the workload between SEs.
   */
   if (!sctx->screen->info.has_distributed_tess && sctx->screen->info.max_se > 1)
            /* Make sure the output data fits in the offchip buffer */
   num_patches =
            /* Make sure that the data fits in LDS. This assumes the shaders only
   * use LDS for the inputs and outputs.
   *
   * The maximum allowed LDS size is 32K. Higher numbers can hang.
   * Use 16K as the maximum, so that we can fit 2 workgroups on the same CU.
   */
   ASSERTED unsigned max_lds_size = 32 * 1024; /* hw limit */
   unsigned target_lds_size = 16 * 1024; /* target at least 2 workgroups per CU, 16K each */
   num_patches = MIN2(num_patches, target_lds_size / lds_per_patch);
   num_patches = MAX2(num_patches, 1);
            /* Make sure that vector lanes are fully occupied by cutting off the last wave
   * if it's only partially filled.
   */
   unsigned temp_verts_per_tg = num_patches * max_verts_per_patch;
            if (temp_verts_per_tg > wave_size &&
      (wave_size - temp_verts_per_tg % wave_size >= MAX2(max_verts_per_patch, 8)))
         if (sctx->gfx_level == GFX6) {
      /* GFX6 bug workaround, related to power management. Limit LS-HS
   * threadgroups to only one wave.
   */
   unsigned one_wave = wave_size / max_verts_per_patch;
               /* The VGT HS block increments the patch ID unconditionally
   * within a single threadgroup. This results in incorrect
   * patch IDs when instanced draws are used.
   *
   * The intended solution is to restrict threadgroups to
   * a single instance by setting SWITCH_ON_EOI, which
   * should cause IA to split instances up. However, this
   * doesn't work correctly on GFX6 when there is no other
   * SE to switch to.
   */
   if (has_primid_instancing_bug && tess_uses_primid)
            if (sctx->num_patches_per_workgroup != num_patches) {
      sctx->num_patches_per_workgroup = num_patches;
               unsigned output_patch0_offset = input_patch_size * num_patches;
            /* Compute userdata SGPRs. */
   assert(((input_vertex_size / 4) & ~0xff) == 0);
   assert(((perpatch_output_offset / 4) & ~0xffff) == 0);
   assert(num_tcs_input_cp <= 32);
   assert(num_tcs_output_cp <= 32);
   assert(num_patches <= 64);
            uint64_t ring_va = (unlikely(sctx->ws->cs_is_secure(&sctx->gfx_cs)) ?
                  sctx->tes_offchip_ring_va_sgpr = ring_va;
   sctx->tcs_offchip_layout =
      (num_patches - 1) | ((num_tcs_output_cp - 1) << 6) | ((num_tcs_input_cp - 1) << 11) |
         /* Compute the LDS size. */
            if (sctx->gfx_level >= GFX7) {
      assert(lds_size <= 65536);
      } else {
      assert(lds_size <= 32768);
               /* Set SI_SGPR_VS_STATE_BITS. */
   SET_FIELD(sctx->current_vs_state, VS_STATE_LS_OUT_VERTEX_SIZE, input_vertex_size / 4);
            /* We should be able to support in-shader LDS use with LLVM >= 9
   * by just adding the lds_sizes together, but it has never
   * been tested. */
                     if (sctx->gfx_level >= GFX9) {
               if (sctx->gfx_level >= GFX10)
         else
      } else {
               si_multiwave_lds_size_workaround(sctx->screen, &lds_size);
               sctx->ls_hs_rsrc2 = ls_hs_rsrc2;
   sctx->ls_hs_config =
         S_028B58_NUM_PATCHES(sctx->num_patches_per_workgroup) |
               }
      static void si_emit_tess_io_layout_state(struct si_context *sctx, unsigned index)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
            if (!sctx->shader.tes.cso || !sctx->shader.tcs.current)
            if (sctx->screen->info.has_set_pairs_packets) {
      radeon_opt_push_gfx_sh_reg(R_00B42C_SPI_SHADER_PGM_RSRC2_HS,
            /* Set userdata SGPRs for merged LS-HS. */
   radeon_opt_push_gfx_sh_reg(R_00B430_SPI_SHADER_USER_DATA_HS_0 +
                     radeon_opt_push_gfx_sh_reg(R_00B430_SPI_SHADER_USER_DATA_HS_0 +
                  } else if (sctx->gfx_level >= GFX9) {
      radeon_opt_set_sh_reg(sctx, R_00B42C_SPI_SHADER_PGM_RSRC2_HS,
            /* Set userdata SGPRs for merged LS-HS. */
   radeon_opt_set_sh_reg2(sctx,
                        } else {
      /* Due to a hw bug, RSRC2_LS must be written twice with another
   * LS register written in between. */
   if (sctx->gfx_level == GFX7 && sctx->family != CHIP_HAWAII)
         radeon_set_sh_reg_seq(R_00B528_SPI_SHADER_PGM_RSRC1_LS, 2);
   radeon_emit(sctx->shader.vs.current->config.rsrc1);
            /* Set userdata SGPRs for TCS. */
   radeon_opt_set_sh_reg3(sctx,
                        R_00B430_SPI_SHADER_USER_DATA_HS_0 +
            /* Set userdata SGPRs for TES. */
   unsigned tes_sh_base = sctx->shader_pointers.sh_base[PIPE_SHADER_TESS_EVAL];
            /* TES (as ES or VS) reuses the BaseVertex and DrawID user SGPRs that are used when
   * tessellation is disabled. That's because those user SGPRs are only set in LS
   * for tessellation.
   */
   if (sctx->screen->info.has_set_pairs_packets) {
      radeon_opt_push_gfx_sh_reg(tes_sh_base + SI_SGPR_TES_OFFCHIP_LAYOUT * 4,
               radeon_opt_push_gfx_sh_reg(tes_sh_base + SI_SGPR_TES_OFFCHIP_ADDR * 4,
            } else {
               radeon_opt_set_sh_reg2(sctx, tes_sh_base + SI_SGPR_TES_OFFCHIP_LAYOUT * 4,
                  }
            radeon_begin_again(cs);
   if (sctx->gfx_level >= GFX7) {
      radeon_opt_set_context_reg_idx(sctx, R_028B58_VGT_LS_HS_CONFIG,
      } else {
      radeon_opt_set_context_reg(sctx, R_028B58_VGT_LS_HS_CONFIG,
      }
      }
      void si_init_screen_live_shader_cache(struct si_screen *sscreen)
   {
      util_live_shader_cache_init(&sscreen->live_shader_cache, si_create_shader_selector,
      }
      template<int NUM_INTERP>
   static void si_emit_spi_map(struct si_context *sctx, unsigned index)
   {
      struct si_shader *ps = sctx->shader.ps.current;
   struct si_shader_info *psinfo = ps ? &ps->selector->info : NULL;
                     if (!NUM_INTERP)
            struct si_shader *vs = si_get_vs(sctx)->current;
            for (unsigned i = 0; i < NUM_INTERP; i++) {
      union si_input_info input = psinfo->input[i];
   unsigned ps_input_cntl = vs->info.vs_output_ps_input_cntl[input.semantic];
            if (non_default_val) {
      if (input.interpolate == INTERP_MODE_FLAT ||
                  if (input.fp16_lo_hi_valid) {
      ps_input_cntl |= S_028644_FP16_INTERP_MODE(1) |
                        if (input.semantic == VARYING_SLOT_PNTC ||
      (input.semantic >= VARYING_SLOT_TEX0 && input.semantic <= VARYING_SLOT_TEX7 &&
   rs->sprite_coord_enable & (1 << (input.semantic - VARYING_SLOT_TEX0)))) {
   /* Overwrite the whole value (except OFFSET) for sprite coordinates. */
   ps_input_cntl &= ~C_028644_OFFSET;
   ps_input_cntl |= S_028644_PT_SPRITE_TEX(1);
   if (input.fp16_lo_hi_valid & 0x1) {
      ps_input_cntl |= S_028644_FP16_INTERP_MODE(1) |
                              /* R_028644_SPI_PS_INPUT_CNTL_0 */
   /* Dota 2: Only ~16% of SPI map updates set different values. */
   /* Talos: Only ~9% of SPI map updates set different values. */
   radeon_begin(&sctx->gfx_cs);
   radeon_opt_set_context_regn(sctx, R_028644_SPI_PS_INPUT_CNTL_0, spi_ps_input_cntl,
            }
      void si_init_shader_functions(struct si_context *sctx)
   {
      sctx->atoms.s.vgt_pipeline_state.emit = si_emit_vgt_pipeline_state;
   sctx->atoms.s.scratch_state.emit = si_emit_scratch_state;
            sctx->b.create_vs_state = si_create_shader;
   sctx->b.create_tcs_state = si_create_shader;
   sctx->b.create_tes_state = si_create_shader;
   sctx->b.create_gs_state = si_create_shader;
            sctx->b.bind_vs_state = si_bind_vs_shader;
   sctx->b.bind_tcs_state = si_bind_tcs_shader;
   sctx->b.bind_tes_state = si_bind_tes_shader;
   sctx->b.bind_gs_state = si_bind_gs_shader;
            sctx->b.delete_vs_state = si_delete_shader_selector;
   sctx->b.delete_tcs_state = si_delete_shader_selector;
   sctx->b.delete_tes_state = si_delete_shader_selector;
   sctx->b.delete_gs_state = si_delete_shader_selector;
                     /* This unrolls the loops in si_emit_spi_map and inlines memcmp and memcpys.
   * It improves performance for viewperf/snx.
   */
   sctx->emit_spi_map[0] = si_emit_spi_map<0>;
   sctx->emit_spi_map[1] = si_emit_spi_map<1>;
   sctx->emit_spi_map[2] = si_emit_spi_map<2>;
   sctx->emit_spi_map[3] = si_emit_spi_map<3>;
   sctx->emit_spi_map[4] = si_emit_spi_map<4>;
   sctx->emit_spi_map[5] = si_emit_spi_map<5>;
   sctx->emit_spi_map[6] = si_emit_spi_map<6>;
   sctx->emit_spi_map[7] = si_emit_spi_map<7>;
   sctx->emit_spi_map[8] = si_emit_spi_map<8>;
   sctx->emit_spi_map[9] = si_emit_spi_map<9>;
   sctx->emit_spi_map[10] = si_emit_spi_map<10>;
   sctx->emit_spi_map[11] = si_emit_spi_map<11>;
   sctx->emit_spi_map[12] = si_emit_spi_map<12>;
   sctx->emit_spi_map[13] = si_emit_spi_map<13>;
   sctx->emit_spi_map[14] = si_emit_spi_map<14>;
   sctx->emit_spi_map[15] = si_emit_spi_map<15>;
   sctx->emit_spi_map[16] = si_emit_spi_map<16>;
   sctx->emit_spi_map[17] = si_emit_spi_map<17>;
   sctx->emit_spi_map[18] = si_emit_spi_map<18>;
   sctx->emit_spi_map[19] = si_emit_spi_map<19>;
   sctx->emit_spi_map[20] = si_emit_spi_map<20>;
   sctx->emit_spi_map[21] = si_emit_spi_map<21>;
   sctx->emit_spi_map[22] = si_emit_spi_map<22>;
   sctx->emit_spi_map[23] = si_emit_spi_map<23>;
   sctx->emit_spi_map[24] = si_emit_spi_map<24>;
   sctx->emit_spi_map[25] = si_emit_spi_map<25>;
   sctx->emit_spi_map[26] = si_emit_spi_map<26>;
   sctx->emit_spi_map[27] = si_emit_spi_map<27>;
   sctx->emit_spi_map[28] = si_emit_spi_map<28>;
   sctx->emit_spi_map[29] = si_emit_spi_map<29>;
   sctx->emit_spi_map[30] = si_emit_spi_map<30>;
   sctx->emit_spi_map[31] = si_emit_spi_map<31>;
      }
