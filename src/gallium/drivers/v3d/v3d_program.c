   /*
   * Copyright © 2014-2017 Broadcom
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
      #include <inttypes.h>
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/ralloc.h"
   #include "util/hash_table.h"
   #include "util/u_upload_mgr.h"
   #include "tgsi/tgsi_dump.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_serialize.h"
   #include "nir/tgsi_to_nir.h"
   #include "compiler/v3d_compiler.h"
   #include "v3d_context.h"
   #include "broadcom/cle/v3d_packet_v33_pack.h"
      static struct v3d_compiled_shader *
   v3d_get_compiled_shader(struct v3d_context *v3d,
                  static void
   v3d_setup_shared_precompile_key(struct v3d_uncompiled_shader *uncompiled,
            static gl_varying_slot
   v3d_get_slot_for_driver_location(nir_shader *s, uint32_t driver_location)
   {
         nir_foreach_shader_out_variable(var, s) {
                                 /* For compact arrays, we have more than one location to
   * check.
   */
   if (var->data.compact) {
            assert(glsl_type_is_array(var->type));
   for (int i = 0; i < DIV_ROUND_UP(glsl_array_size(var->type), 4); i++) {
         if ((var->data.driver_location + i) == driver_location) {
            }
      /**
   * Precomputes the TRANSFORM_FEEDBACK_OUTPUT_DATA_SPEC array for the shader.
   *
   * A shader can have 16 of these specs, and each one of them can write up to
   * 16 dwords.  Since we allow a total of 64 transform feedback output
   * components (not 16 vectors), we have to group the writes of multiple
   * varyings together in a single data spec.
   */
   static void
   v3d_set_transform_feedback_outputs(struct v3d_uncompiled_shader *so,
         {
         if (!stream_output->num_outputs)
            struct v3d_varying_slot slots[PIPE_MAX_SO_OUTPUTS * 4];
            for (int buffer = 0; buffer < PIPE_MAX_SO_BUFFERS; buffer++) {
                                                                                                   /* Pad any undefined slots in the output */
   for (int j = buffer_offset; j < output->dst_offset; j++) {
                                    /* Set the coordinate shader up to output the
   * components of this varying.
                                    slots[slot_count] =
                                                                                 struct V3D33_TRANSFORM_FEEDBACK_OUTPUT_DATA_SPEC unpacked = {
         /* We need the offset from the coordinate shader's VPM
      * output block, which has the [X, Y, Z, W, Xs, Ys]
                                                                                 /* If point size is being written by the shader, then
   * all the VPM start offsets are shifted up by one.
                                                   V3D33_TRANSFORM_FEEDBACK_OUTPUT_DATA_SPEC_pack(NULL,
               so->num_tf_specs++;
      }
               so->num_tf_outputs = slot_count;
   so->tf_outputs = ralloc_array(so->base.ir.nir, struct v3d_varying_slot,
         }
      static int
   type_size(const struct glsl_type *type, bool bindless)
   {
         }
      static void
   precompile_all_outputs(nir_shader *s,
               {
         nir_foreach_shader_out_variable(var, s) {
            const int array_len = MAX2(glsl_get_length(var->type), 1);
   for (int j = 0; j < array_len; j++) {
            const int slot = var->data.location + j;
   const int num_components =
         for (int i = 0; i < num_components; i++) {
         const int swiz = var->data.location_frac + i;
   outputs[(*num_outputs)++] =
   }
      /**
   * Precompiles a shader variant at shader state creation time if
   * V3D_DEBUG=precompile is set.  Used for shader-db
   * (https://gitlab.freedesktop.org/mesa/shader-db)
   */
   static void
   v3d_shader_precompile(struct v3d_context *v3d,
         {
                  if (s->info.stage == MESA_SHADER_FRAGMENT) {
                           nir_foreach_shader_out_variable(var, s) {
            if (var->data.location == FRAG_RESULT_COLOR) {
         } else if (var->data.location >= FRAG_RESULT_DATA0) {
                                 } else if (s->info.stage == MESA_SHADER_GEOMETRY) {
                                                                        /* Compile GS bin shader: only position (XXX: include TF) */
   key.is_coord = true;
   key.num_used_outputs = 0;
   for (int i = 0; i < 4; i++) {
            key.used_outputs[key.num_used_outputs++] =
         } else {
            assert(s->info.stage == MESA_SHADER_VERTEX);
   struct v3d_vs_key key = {
                                                               /* Compile VS bin shader: only position (XXX: include TF) */
   key.is_coord = true;
   key.num_used_outputs = 0;
   for (int i = 0; i < 4; i++) {
            key.used_outputs[key.num_used_outputs++] =
         }
      static bool
   lower_uniform_offset_to_bytes_cb(nir_builder *b, nir_intrinsic_instr *intr,
         {
         if (intr->intrinsic != nir_intrinsic_load_uniform)
            b->cursor = nir_before_instr(&intr->instr);
   nir_intrinsic_set_base(intr, nir_intrinsic_base(intr) * 16);
   nir_src_rewrite(&intr->src[0], nir_ishl_imm(b, intr->src[0].ssa, 4));
   }
      static bool
   lower_textures_cb(nir_builder *b, nir_instr *instr, void *_state)
   {
         if (instr->type != nir_instr_type_tex)
            nir_tex_instr *tex = nir_instr_as_tex(instr);
   if (nir_tex_instr_need_sampler(tex))
            /* Use the texture index as sampler index for the purposes of
      * lower_tex_packing, since in GL we currently make packing
   * decisions based on texture format.
      tex->backend_flags = tex->texture_index;
   }
      static bool
   v3d_nir_lower_uniform_offset_to_bytes(nir_shader *s)
   {
         return nir_shader_intrinsics_pass(s, lower_uniform_offset_to_bytes_cb,
         }
      static bool
   v3d_nir_lower_textures(nir_shader *s)
   {
         return nir_shader_instructions_pass(s, lower_textures_cb,
         }
      static void *
   v3d_uncompiled_shader_create(struct pipe_context *pctx,
         {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_uncompiled_shader *so = CALLOC_STRUCT(v3d_uncompiled_shader);
   if (!so)
                              if (type == PIPE_SHADER_IR_NIR) {
            /* The backend takes ownership of the NIR shader on state
   * creation.
      } else {
                     if (V3D_DBG(TGSI)) {
            fprintf(stderr, "prog %d TGSI:\n",
                        if (s->info.stage != MESA_SHADER_VERTEX &&
         s->info.stage != MESA_SHADER_GEOMETRY) {
      NIR_PASS(_, s, nir_lower_io,
                                                   /* Get rid of split copies */
                              /* Since we can't expose PIPE_CAP_PACKED_UNIFORMS the state tracker
      * will produce uniform intrinsics with offsets in vec4 units but
   * our compiler expects to work in units of bytes.
                        /* Garbage collect dead instructions */
            so->base.type = PIPE_SHADER_IR_NIR;
            /* Generate sha1 from NIR for caching */
   struct blob blob;
   blob_init(&blob);
   nir_serialize(&blob, s, true);
   assert(!blob.out_of_memory);
   _mesa_sha1_compute(blob.data, blob.size, so->sha1);
            if (V3D_DBG(NIR) || v3d_debug_flag_for_shader_stage(s->info.stage)) {
            fprintf(stderr, "%s prog %d NIR:\n",
                           if (V3D_DBG(PRECOMPILE))
            }
      static void
   v3d_shader_debug_output(const char *message, void *data)
   {
                  }
      static void *
   v3d_shader_state_create(struct pipe_context *pctx,
         {
         struct v3d_uncompiled_shader *so =
            v3d_uncompiled_shader_create(pctx,
                              }
      /* Key ued with the RAM cache */
   struct v3d_cache_key {
         struct v3d_key *key;
   };
      struct v3d_compiled_shader *
   v3d_get_compiled_shader(struct v3d_context *v3d,
                     {
         nir_shader *s = uncompiled->base.ir.nir;
   struct hash_table *ht = v3d->prog.cache[s->info.stage];
   struct v3d_cache_key cache_key;
   cache_key.key = key;
   memcpy(cache_key.sha1, uncompiled->sha1, sizeof(cache_key.sha1));
   struct hash_entry *entry = _mesa_hash_table_search(ht, &cache_key);
   if (entry)
            int variant_id =
               #ifdef ENABLE_SHADER_CACHE
         #endif
         if (!shader) {
                                          qpu_insts = v3d_compile(v3d->screen->compiler, key,
                              /* qpu_insts being NULL can happen if the register allocation
   * failed. At this point we can't really trigger an OpenGL API
   * error, as the final compilation could happen on the draw
   * call. So let's at least assert, so debug builds finish at
   * this point.
                        if (shader_size) {
         #ifdef ENABLE_SHADER_CACHE
               #endif
                                    if (ht) {
            struct v3d_cache_key *dup_cache_key =
         dup_cache_key->key = ralloc_size(shader, key_size);
   memcpy(dup_cache_key->key, cache_key.key, key_size);
               if (shader->prog_data.base->spill_size >
         v3d->prog.spill_size_per_thread) {
      /* The TIDX register we use for choosing the area to access
   * for scratch space is: (core << 6) | (qpu << 2) | thread.
   * Even at minimum threadcount in a particular shader, that
   * means we still multiply by qpus by 4.
                        v3d_bo_unreference(&v3d->prog.spill_bo);
   v3d->prog.spill_bo = v3d_bo_alloc(v3d->screen,
                     }
      static void
   v3d_free_compiled_shader(struct v3d_compiled_shader *shader)
   {
         pipe_resource_reference(&shader->resource, NULL);
   }
      static void
   v3d_setup_shared_key(struct v3d_context *v3d, struct v3d_key *key,
         {
                  key->num_tex_used = texstate->num_textures;
   key->num_samplers_used = texstate->num_textures;
   assert(key->num_tex_used == key->num_samplers_used);
   for (int i = 0; i < texstate->num_textures; i++) {
                                                         /* For 16-bit, we set up the sampler to always return 2
   * channels (meaning no recompiles for most statechanges),
   * while for 32 we actually scale the returns with channels.
   */
   if (key->sampler[i].return_size == 16) {
         } else if (devinfo->ver > 40) {
         } else {
                              if (key->sampler[i].return_size == 32 && devinfo->ver < 40) {
            memcpy(key->tex[i].swizzle,
      } else {
            /* For 16-bit returns, we let the sampler state handle
   * the swizzle.
   */
   key->tex[i].swizzle[0] = PIPE_SWIZZLE_X;
   key->tex[i].swizzle[1] = PIPE_SWIZZLE_Y;
   }
      static void
   v3d_setup_shared_precompile_key(struct v3d_uncompiled_shader *uncompiled,
         {
                  /* Note that below we access they key's texture and sampler fields
      * using the same index. On OpenGL they are the same (they are
   * combined)
      key->num_tex_used = s->info.num_textures;
   key->num_samplers_used = s->info.num_textures;
   for (int i = 0; i < s->info.num_textures; i++) {
                           key->tex[i].swizzle[0] = PIPE_SWIZZLE_X;
   key->tex[i].swizzle[1] = PIPE_SWIZZLE_Y;
      }
      static void
   v3d_update_compiled_fs(struct v3d_context *v3d, uint8_t prim_mode)
   {
         struct v3d_job *job = v3d->job;
   struct v3d_fs_key local_key;
   struct v3d_fs_key *key = &local_key;
            if (!(v3d->dirty & (V3D_DIRTY_PRIM_MODE |
                        V3D_DIRTY_BLEND |
   V3D_DIRTY_FRAMEBUFFER |
   V3D_DIRTY_ZSA |
   V3D_DIRTY_RASTERIZER |
               memset(key, 0, sizeof(*key));
   v3d_setup_shared_key(v3d, &key->base, &v3d->tex[PIPE_SHADER_FRAGMENT]);
   key->base.ucp_enables = v3d->rasterizer->base.clip_plane_enable;
   key->is_points = (prim_mode == MESA_PRIM_POINTS);
   key->is_lines = (prim_mode >= MESA_PRIM_LINES &&
         key->line_smoothing = (key->is_lines &&
         key->has_gs = v3d->prog.bind_gs != NULL;
   if (v3d->blend->base.logicop_enable) {
         } else {
         }
   if (job->msaa) {
            key->msaa = v3d->rasterizer->base.multisample;
                        for (int i = 0; i < v3d->framebuffer.nr_cbufs; i++) {
                                 /* gl_FragColor's propagation to however many bound color
   * buffers there are means that the shader compile needs to
                        /* If logic operations are enabled then we might emit color
   * reads and we need to know the color buffer format and
   * swizzle for that.
   */
   if (key->logicop_func != PIPE_LOGICOP_COPY) {
            key->color_fmt[i].format = cbuf->format;
   memcpy(key->color_fmt[i].swizzle,
                                    if (desc->channel[0].type == UTIL_FORMAT_TYPE_FLOAT &&
                        if (s->info.fs.untyped_color_outputs) {
            if (util_format_is_pure_uint(cbuf->format))
                  if (key->is_points) {
            key->point_sprite_mask =
                     struct v3d_compiled_shader *old_fs = v3d->prog.fs;
   v3d->prog.fs = v3d_get_compiled_shader(v3d, &key->base, sizeof(*key),
         if (v3d->prog.fs == old_fs)
                     if (old_fs) {
            if (v3d->prog.fs->prog_data.fs->flat_shade_flags !=
                        if (v3d->prog.fs->prog_data.fs->noperspective_flags !=
                        if (v3d->prog.fs->prog_data.fs->centroid_flags !=
      old_fs->prog_data.fs->centroid_flags) {
            if (old_fs && memcmp(v3d->prog.fs->prog_data.fs->input_slots,
                     }
      static void
   v3d_update_compiled_gs(struct v3d_context *v3d, uint8_t prim_mode)
   {
         struct v3d_gs_key local_key;
            if (!(v3d->dirty & (V3D_DIRTY_GEOMTEX |
                        V3D_DIRTY_RASTERIZER |
               if (!v3d->prog.bind_gs) {
            v3d->prog.gs = NULL;
               memset(key, 0, sizeof(*key));
   v3d_setup_shared_key(v3d, &key->base, &v3d->tex[PIPE_SHADER_GEOMETRY]);
   key->base.ucp_enables = v3d->rasterizer->base.clip_plane_enable;
   key->base.is_last_geometry_stage = true;
   key->num_used_outputs = v3d->prog.fs->prog_data.fs->num_inputs;
   STATIC_ASSERT(sizeof(key->used_outputs) ==
         memcpy(key->used_outputs, v3d->prog.fs->prog_data.fs->input_slots,
            key->per_vertex_point_size =
                  struct v3d_uncompiled_shader *uncompiled = v3d->prog.bind_gs;
   struct v3d_compiled_shader *gs =
               if (gs != v3d->prog.gs) {
                                 /* The last bin-mode shader in the geometry pipeline only outputs
      * varyings used by transform feedback.
      memcpy(key->used_outputs, uncompiled->tf_outputs,
         if (uncompiled->num_tf_outputs < key->num_used_outputs) {
            uint32_t size = sizeof(*key->used_outputs) *
                  }
            struct v3d_compiled_shader *old_gs = v3d->prog.gs;
   struct v3d_compiled_shader *gs_bin =
               if (gs_bin != old_gs) {
                        if (old_gs && memcmp(v3d->prog.gs->prog_data.gs->input_slots,
                     }
      static void
   v3d_update_compiled_vs(struct v3d_context *v3d, uint8_t prim_mode)
   {
         struct v3d_vs_key local_key;
            if (!(v3d->dirty & (V3D_DIRTY_VERTTEX |
                        V3D_DIRTY_VTXSTATE |
   V3D_DIRTY_UNCOMPILED_VS |
   (v3d->prog.bind_gs ? 0 : V3D_DIRTY_RASTERIZER) |
               memset(key, 0, sizeof(*key));
   v3d_setup_shared_key(v3d, &key->base, &v3d->tex[PIPE_SHADER_VERTEX]);
   key->base.ucp_enables = v3d->rasterizer->base.clip_plane_enable;
            if (!v3d->prog.bind_gs) {
         key->num_used_outputs = v3d->prog.fs->prog_data.fs->num_inputs;
   STATIC_ASSERT(sizeof(key->used_outputs) ==
         memcpy(key->used_outputs, v3d->prog.fs->prog_data.fs->input_slots,
   } else {
         key->num_used_outputs = v3d->prog.gs->prog_data.gs->num_inputs;
   STATIC_ASSERT(sizeof(key->used_outputs) ==
         memcpy(key->used_outputs, v3d->prog.gs->prog_data.gs->input_slots,
            key->per_vertex_point_size =
                  nir_shader *s = v3d->prog.bind_vs->base.ir.nir;
   uint64_t inputs_read = s->info.inputs_read;
            while (inputs_read) {
            int location = u_bit_scan64(&inputs_read);
   nir_variable *var =
         assert (var != NULL);
   int driver_location = var->data.driver_location;
   switch (v3d->vtx->pipe[driver_location].src_format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B10G10R10A2_UNORM:
   case PIPE_FORMAT_B10G10R10A2_SNORM:
   case PIPE_FORMAT_B10G10R10A2_USCALED:
   case PIPE_FORMAT_B10G10R10A2_SSCALED:
               default:
               struct v3d_uncompiled_shader *shader_state = v3d->prog.bind_vs;
   struct v3d_compiled_shader *vs =
               if (vs != v3d->prog.vs) {
                                 /* Coord shaders only output varyings used by transform feedback,
      * unless they are linked to other shaders in the geometry side
   * of the pipeline, since in that case any of the output varyings
   * could be required in later geometry stages to compute
   * gl_Position or TF outputs.
      if (!v3d->prog.bind_gs) {
            memcpy(key->used_outputs, shader_state->tf_outputs,
         sizeof(*key->used_outputs) *
   if (shader_state->num_tf_outputs < key->num_used_outputs) {
            uint32_t tail_bytes =
         sizeof(*key->used_outputs) *
   (key->num_used_outputs -
         } else {
            key->num_used_outputs = v3d->prog.gs_bin->prog_data.gs->num_inputs;
   STATIC_ASSERT(sizeof(key->used_outputs) ==
                     struct v3d_compiled_shader *cs =
               if (cs != v3d->prog.cs) {
               }
      void
   v3d_update_compiled_shaders(struct v3d_context *v3d, uint8_t prim_mode)
   {
         v3d_update_compiled_fs(v3d, prim_mode);
   v3d_update_compiled_gs(v3d, prim_mode);
   }
      void
   v3d_update_compiled_cs(struct v3d_context *v3d)
   {
         struct v3d_key local_key;
            if (!(v3d->dirty & (V3D_DIRTY_UNCOMPILED_CS |
                        memset(key, 0, sizeof(*key));
            struct v3d_compiled_shader *cs =
               if (cs != v3d->prog.compute) {
               }
      static inline uint32_t
   cache_hash(const void *_key, uint32_t key_size)
   {
                  struct mesa_sha1 ctx;
   unsigned char sha1[20];
   _mesa_sha1_init(&ctx);
   _mesa_sha1_update(&ctx, key->key, key_size);
   _mesa_sha1_update(&ctx, key->sha1, 20);
   _mesa_sha1_final(&ctx, sha1);
   }
      static inline bool
   cache_compare(const void *_key1, const void *_key2, uint32_t key_size)
   {
         const struct v3d_cache_key *key1 = (struct v3d_cache_key *) _key1;
            if (memcmp(key1->key, key2->key, key_size) != 0)
            }
      static uint32_t
   fs_cache_hash(const void *key)
   {
         }
      static uint32_t
   gs_cache_hash(const void *key)
   {
         }
      static uint32_t
   vs_cache_hash(const void *key)
   {
         }
      static uint32_t
   cs_cache_hash(const void *key)
   {
         }
      static bool
   fs_cache_compare(const void *key1, const void *key2)
   {
         }
      static bool
   gs_cache_compare(const void *key1, const void *key2)
   {
         }
      static bool
   vs_cache_compare(const void *key1, const void *key2)
   {
         }
      static bool
   cs_cache_compare(const void *key1, const void *key2)
   {
         }
      static void
   v3d_shader_state_delete(struct pipe_context *pctx, void *hwcso)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_uncompiled_shader *so = hwcso;
            hash_table_foreach(v3d->prog.cache[s->info.stage], entry) {
                                          if (v3d->prog.fs == shader)
         if (v3d->prog.vs == shader)
         if (v3d->prog.cs == shader)
                                    ralloc_free(so->base.ir.nir);
   }
      static void
   v3d_fp_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->prog.bind_fs = hwcso;
   }
      static void
   v3d_gp_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->prog.bind_gs = hwcso;
   }
      static void
   v3d_vp_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->prog.bind_vs = hwcso;
   }
      static void
   v3d_compute_state_bind(struct pipe_context *pctx, void *state)
   {
                  v3d->prog.bind_compute = state;
   }
      static void *
   v3d_create_compute_state(struct pipe_context *pctx,
         {
         return v3d_uncompiled_shader_create(pctx, cso->ir_type,
   }
      void
   v3d_program_init(struct pipe_context *pctx)
   {
                  pctx->create_vs_state = v3d_shader_state_create;
            pctx->create_gs_state = v3d_shader_state_create;
            pctx->create_fs_state = v3d_shader_state_create;
            pctx->bind_fs_state = v3d_fp_state_bind;
   pctx->bind_gs_state = v3d_gp_state_bind;
            if (v3d->screen->has_csd) {
            pctx->create_compute_state = v3d_create_compute_state;
               v3d->prog.cache[MESA_SHADER_VERTEX] =
         v3d->prog.cache[MESA_SHADER_GEOMETRY] =
         v3d->prog.cache[MESA_SHADER_FRAGMENT] =
         v3d->prog.cache[MESA_SHADER_COMPUTE] =
   }
      void
   v3d_program_fini(struct pipe_context *pctx)
   {
                  for (int i = 0; i < MESA_SHADER_STAGES; i++) {
                                 hash_table_foreach(cache, entry) {
            struct v3d_compiled_shader *shader = entry->data;
            }
