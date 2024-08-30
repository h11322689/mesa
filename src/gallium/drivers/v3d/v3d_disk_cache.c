   /*
   * Copyright © 2022 Raspberry Pi Ltd
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
      /**
   * V3D on-disk shader cache.
   */
      #include "v3d_context.h"
      #include "util/blob.h"
   #include "util/u_upload_mgr.h"
      #ifdef ENABLE_SHADER_CACHE
      static uint32_t
   v3d_key_size(gl_shader_stage stage)
   {
         static const int key_size[] = {
            [MESA_SHADER_VERTEX] = sizeof(struct v3d_vs_key),
   [MESA_SHADER_GEOMETRY] = sizeof(struct v3d_gs_key),
               assert(stage >= 0 &&
                  }
      void v3d_disk_cache_init(struct v3d_screen *screen)
   {
                  ASSERTED int len =
            asprintf(&renderer, "V3D %d.%d",
               const struct build_id_note *note =
                  const uint8_t *id_sha1 = build_id_data(note);
            char timestamp[41];
                     }
      static void
   v3d_disk_cache_compute_key(struct disk_cache *cache,
                     {
                  assert(uncompiled->base.type == PIPE_SHADER_IR_NIR);
            uint32_t ckey_size = v3d_key_size(nir->info.stage);
   struct v3d_key *ckey = malloc(ckey_size);
            struct blob blob;
   blob_init(&blob);
   blob_write_bytes(&blob, ckey, ckey_size);
                     blob_finish(&blob);
   }
      struct v3d_compiled_shader *
   v3d_disk_cache_retrieve(struct v3d_context *v3d,
               {
         struct v3d_screen *screen = v3d->screen;
            if (!cache)
            assert(uncompiled->base.type == PIPE_SHADER_IR_NIR);
            cache_key cache_key;
            size_t buffer_size;
            if (V3D_DBG(CACHE)) {
            char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
   fprintf(stderr, "[v3d on-disk cache] %s %s\n",
               if (!buffer)
            /* Load data */
   struct blob_reader blob;
            uint32_t prog_data_size = v3d_prog_data_size(nir->info.stage);
   const void *prog_data = blob_read_bytes(&blob, prog_data_size);
   if (blob.overrun)
            uint32_t ulist_count = blob_read_uint32(&blob);
   uint32_t ulist_contents_size = ulist_count * sizeof(enum quniform_contents);
   const void *ulist_contents = blob_read_bytes(&blob, ulist_contents_size);
   if (blob.overrun)
            uint32_t ulist_data_size = ulist_count * sizeof(uint32_t);
   const void *ulist_data = blob_read_bytes(&blob, ulist_data_size);
   if (blob.overrun)
            uint32_t qpu_size = blob_read_uint32(&blob);
   const void *qpu_insts =
         if (blob.overrun)
            /* Assemble data */
            shader->prog_data.base = rzalloc_size(shader, prog_data_size);
                     shader->prog_data.base->uniforms.contents =
                  shader->prog_data.base->uniforms.data =
                  u_upload_data(v3d->state_uploader, 0, qpu_size, 8,
                     }
      void
   v3d_disk_cache_store(struct v3d_context *v3d,
                        const struct v3d_key *key,
      {
         struct v3d_screen *screen = v3d->screen;
            if (!cache)
            assert(uncompiled->base.type == PIPE_SHADER_IR_NIR);
            cache_key cache_key;
            if (V3D_DBG(CACHE)) {
            char sha1[41];
               struct blob blob;
            blob_write_bytes(&blob, shader->prog_data.base, v3d_prog_data_size(nir->info.stage));
   uint32_t ulist_count = shader->prog_data.base->uniforms.count;
   blob_write_uint32(&blob, ulist_count);
   blob_write_bytes(&blob,
               blob_write_bytes(&blob,
                  blob_write_uint32(&blob, qpu_size);
                     }
      #endif /* ENABLE_SHADER_CACHE */
   