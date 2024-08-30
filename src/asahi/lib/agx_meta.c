   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "agx_meta.h"
   #include "agx_compile.h"
   #include "agx_device.h" /* for AGX_MEMORY_TYPE_SHADER */
   #include "agx_tilebuffer.h"
   #include "nir_builder.h"
      static struct agx_meta_shader *
   agx_compile_meta_shader(struct agx_meta_cache *cache, nir_shader *shader,
               {
      struct util_dynarray binary;
            agx_preprocess_nir(shader, false, false, NULL);
   if (tib) {
      unsigned bindless_base = 0;
   agx_nir_lower_tilebuffer(shader, tib, NULL, &bindless_base, NULL, true);
   agx_nir_lower_monolithic_msaa(
               struct agx_meta_shader *res = rzalloc(cache->ht, struct agx_meta_shader);
            res->ptr = agx_pool_upload_aligned_with_bo(&cache->pool, binary.data,
         util_dynarray_fini(&binary);
               }
      static nir_def *
   build_background_op(nir_builder *b, enum agx_meta_op op, unsigned rt,
         {
      if (op == AGX_META_OP_LOAD) {
               if (layered) {
      coord = nir_vec3(b, nir_channel(b, coord, 0), nir_channel(b, coord, 1),
               nir_tex_instr *tex = nir_tex_instr_create(b->shader, 2);
   /* The type doesn't matter as long as it matches the store */
   tex->dest_type = nir_type_uint32;
   tex->sampler_dim = msaa ? GLSL_SAMPLER_DIM_MS : GLSL_SAMPLER_DIM_2D;
   tex->is_array = layered;
   tex->op = msaa ? nir_texop_txf_ms : nir_texop_txf;
            /* Layer is necessarily already in-bounds so we do not want the compiler
   * to clamp it, which would require reading the descriptor
   */
            if (msaa) {
      tex->src[1] =
            } else {
                  tex->coord_components = layered ? 3 : 2;
   tex->texture_index = rt;
   nir_def_init(&tex->instr, &tex->def, 4, 32);
               } else {
                     }
      static struct agx_meta_shader *
   agx_build_background_shader(struct agx_meta_cache *cache,
         {
      nir_builder b = nir_builder_init_simple_shader(
                  struct agx_shader_key compiler_key = {
      .fs.ignore_tib_dependencies = true,
   .fs.nr_samples = key->tib.nr_samples,
               for (unsigned rt = 0; rt < ARRAY_SIZE(key->op); ++rt) {
      if (key->op[rt] == AGX_META_OP_NONE)
            unsigned nr = util_format_get_nr_components(key->tib.logical_format[rt]);
   bool msaa = key->tib.nr_samples > 1;
   bool layered = key->tib.layered;
            nir_store_output(
      &b, build_background_op(&b, key->op[rt], rt, nr, msaa, layered),
   nir_imm_int(&b, 0), .write_mask = BITFIELD_MASK(nr),
   .src_type = nir_type_uint32,
                              }
      static struct agx_meta_shader *
   agx_build_end_of_tile_shader(struct agx_meta_cache *cache,
         {
      nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE,
            enum glsl_sampler_dim dim =
            for (unsigned rt = 0; rt < ARRAY_SIZE(key->op); ++rt) {
      if (key->op[rt] == AGX_META_OP_NONE)
            /* The end-of-tile shader is unsuitable to handle spilled render targets.
   * Skip them. If blits are needed with spilled render targets, other parts
   * of the driver need to implement them.
   */
   if (key->tib.spilled[rt])
            assert(key->op[rt] == AGX_META_OP_STORE);
            nir_def *layer = nir_undef(&b, 1, 16);
   if (key->tib.layered)
            nir_block_image_store_agx(
      &b, nir_imm_int(&b, rt), nir_imm_intN_t(&b, offset_B, 16), layer,
   .format = agx_tilebuffer_physical_format(&key->tib, rt),
            struct agx_shader_key compiler_key = {
                     }
      struct agx_meta_shader *
   agx_get_meta_shader(struct agx_meta_cache *cache, struct agx_meta_key *key)
   {
      struct hash_entry *ent = _mesa_hash_table_search(cache->ht, key);
   if (ent)
                     for (unsigned rt = 0; rt < ARRAY_SIZE(key->op); ++rt) {
      if (key->op[rt] == AGX_META_OP_STORE) {
      ret = agx_build_end_of_tile_shader(cache, key);
                  if (!ret)
            ret->key = *key;
   _mesa_hash_table_insert(cache->ht, &ret->key, ret);
      }
      static uint32_t
   key_hash(const void *key)
   {
         }
      static bool
   key_compare(const void *a, const void *b)
   {
         }
      void
   agx_meta_init(struct agx_meta_cache *cache, struct agx_device *dev)
   {
      agx_pool_init(&cache->pool, dev, AGX_BO_EXEC | AGX_BO_LOW_VA, true);
      }
      void
   agx_meta_cleanup(struct agx_meta_cache *cache)
   {
      agx_pool_cleanup(&cache->pool);
   _mesa_hash_table_destroy(cache->ht, NULL);
      }
