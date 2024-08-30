   /*
   * Copyright (C) 2023 Amazon.com, Inc. or its affiliates
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
   */
      #include "pan_afbc_cso.h"
   #include "nir_builder.h"
   #include "pan_context.h"
   #include "pan_resource.h"
   #include "pan_screen.h"
      #define panfrost_afbc_add_info_ubo(name, b)                                    \
      nir_variable *info_ubo = nir_variable_create(                               \
      b.shader, nir_var_mem_ubo,                                               \
   glsl_array_type(glsl_uint_type(),                                        \
                  #define panfrost_afbc_get_info_field(name, b, field)                           \
      nir_load_ubo(                                                               \
      (b), 1, sizeof(((struct panfrost_afbc_##name##_info *)0)->field) * 8,    \
   nir_imm_int(b, 0),                                                       \
   nir_imm_int(b, offsetof(struct panfrost_afbc_##name##_info, field)),     \
      static nir_def *
   read_afbc_header(nir_builder *b, nir_def *buf, nir_def *idx)
   {
      nir_def *offset = nir_imul_imm(b, idx, AFBC_HEADER_BYTES_PER_TILE);
   return nir_load_global(b, nir_iadd(b, buf, nir_u2u64(b, offset)), 16,
      }
      static void
   write_afbc_header(nir_builder *b, nir_def *buf, nir_def *idx, nir_def *hdr)
   {
      nir_def *offset = nir_imul_imm(b, idx, AFBC_HEADER_BYTES_PER_TILE);
      }
      static nir_def *
   get_morton_index(nir_builder *b, nir_def *idx, nir_def *src_stride,
         {
      nir_def *x = nir_umod(b, idx, dst_stride);
            nir_def *offset = nir_imul(b, nir_iand_imm(b, y, ~0x7), src_stride);
            x = nir_iand_imm(b, x, 0x7);
   x = nir_iand_imm(b, nir_ior(b, x, nir_ishl_imm(b, x, 2)), 0x13);
   x = nir_iand_imm(b, nir_ior(b, x, nir_ishl_imm(b, x, 1)), 0x15);
   y = nir_iand_imm(b, y, 0x7);
   y = nir_iand_imm(b, nir_ior(b, y, nir_ishl_imm(b, y, 2)), 0x13);
   y = nir_iand_imm(b, nir_ior(b, y, nir_ishl_imm(b, y, 1)), 0x15);
               }
      static nir_def *
   get_superblock_size(nir_builder *b, unsigned arch, nir_def *hdr,
         {
               unsigned body_base_ptr_len = 32;
   unsigned nr_subblocks = 16;
   unsigned sz_len = 6; /* bits */
   nir_def *words[4];
   nir_def *mask = nir_imm_int(b, (1 << sz_len) - 1);
            for (int i = 0; i < 4; i++)
            /* Sum up all of the subblock sizes */
   for (int i = 0; i < nr_subblocks; i++) {
      nir_def *subblock_size;
   unsigned bitoffset = body_base_ptr_len + (i * sz_len);
   unsigned start = bitoffset / 32;
   unsigned end = (bitoffset + (sz_len - 1)) / 32;
            /* Handle differently if the size field is split between two words
   * of the header */
   if (start != end) {
      subblock_size = nir_ior(b, nir_ushr_imm(b, words[start], offset),
            } else {
      subblock_size =
      }
   subblock_size = nir_bcsel(b, nir_ieq_imm(b, subblock_size, 1),
                  /* When the first subblock size is set to zero, the whole superblock is
   * filled with a solid color specified in the header */
   if (arch >= 7 && i == 0)
               return (arch >= 7)
            }
      static nir_def *
   get_packed_offset(nir_builder *b, nir_def *metadata, nir_def *idx,
         {
      nir_def *metadata_offset =
         nir_def *range_ptr = nir_iadd(b, metadata, metadata_offset);
   nir_def *entry = nir_load_global(b, range_ptr, 4,
         nir_def *offset =
            if (out_size)
      *out_size =
            }
      #define MAX_LINE_SIZE 16
      static void
   copy_superblock(nir_builder *b, nir_def *dst, nir_def *dst_idx, nir_def *hdr_sz,
               {
      nir_def *hdr = read_afbc_header(b, src, src_idx);
   nir_def *src_body_base_ptr = nir_u2u64(b, nir_channel(b, hdr, 0));
            nir_def *size;
   nir_def *dst_offset = get_packed_offset(b, metadata, meta_idx, &size);
   nir_def *dst_body_base_ptr = nir_iadd(b, dst_offset, hdr_sz);
            /* Replace the `base_body_ptr` field if not zero (solid color) */
   nir_def *hdr2 =
         hdr = nir_bcsel(b, nir_ieq_imm(b, src_body_base_ptr, 0), hdr, hdr2);
            nir_variable *offset_var =
         nir_store_var(b, offset_var, nir_imm_int(b, 0), 1);
   nir_loop *loop = nir_push_loop(b);
   {
      nir_def *offset = nir_load_var(b, offset_var);
   nir_if *loop_check = nir_push_if(b, nir_uge(b, offset, size));
   nir_jump(b, nir_jump_break);
   nir_push_else(b, loop_check);
   unsigned line_sz = align <= MAX_LINE_SIZE ? align : MAX_LINE_SIZE;
   for (unsigned i = 0; i < align / line_sz; ++i) {
      nir_def *src_line = nir_iadd(b, src_bodyptr, nir_u2u64(b, offset));
   nir_def *dst_line = nir_iadd(b, dst_bodyptr, nir_u2u64(b, offset));
   nir_store_global(
      b, dst_line, line_sz,
         }
   nir_store_var(b, offset_var, offset, 0x1);
      }
      }
      #define panfrost_afbc_size_get_info_field(b, field)                            \
            static nir_shader *
   panfrost_afbc_create_size_shader(struct panfrost_screen *screen, unsigned bpp,
         {
               nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_COMPUTE, screen->vtbl.get_compiler_options(),
                  nir_def *coord = nir_load_global_invocation_id(&b, 32);
   nir_def *block_idx = nir_channel(&b, coord, 0);
   nir_def *src = panfrost_afbc_size_get_info_field(&b, src);
   nir_def *metadata = panfrost_afbc_size_get_info_field(&b, metadata);
            nir_def *hdr = read_afbc_header(&b, src, block_idx);
   nir_def *size = get_superblock_size(&b, dev->arch, hdr, uncompressed_size);
   size = nir_iand(&b, nir_iadd(&b, size, nir_imm_int(&b, align - 1)),
            nir_def *offset = nir_u2u64(
      &b,
   nir_iadd(&b,
                        }
      #define panfrost_afbc_pack_get_info_field(b, field)                            \
            static nir_shader *
   panfrost_afbc_create_pack_shader(struct panfrost_screen *screen, unsigned align,
         {
      nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_COMPUTE, screen->vtbl.get_compiler_options(),
                  nir_def *coord = nir_load_global_invocation_id(&b, 32);
   nir_def *src_stride = panfrost_afbc_pack_get_info_field(&b, src_stride);
   nir_def *dst_stride = panfrost_afbc_pack_get_info_field(&b, dst_stride);
   nir_def *dst_idx = nir_channel(&b, coord, 0);
   nir_def *src_idx =
         nir_def *src = panfrost_afbc_pack_get_info_field(&b, src);
   nir_def *dst = panfrost_afbc_pack_get_info_field(&b, dst);
   nir_def *header_size =
                  copy_superblock(&b, dst, dst_idx, header_size, src, src_idx, metadata,
               }
      struct pan_afbc_shader_data *
   panfrost_afbc_get_shaders(struct panfrost_context *ctx,
         {
      struct pipe_context *pctx = &ctx->base;
   struct panfrost_screen *screen = pan_screen(ctx->base.screen);
   bool tiled = rsrc->image.layout.modifier & AFBC_FORMAT_MOD_TILED;
   struct pan_afbc_shader_key key = {
      .bpp = util_format_get_blocksizebits(rsrc->base.format),
   .align = align,
               pthread_mutex_lock(&ctx->afbc_shaders.lock);
   struct hash_entry *he =
         struct pan_afbc_shader_data *shader = he ? he->data : NULL;
            if (shader)
            shader = rzalloc(ctx->afbc_shaders.shaders, struct pan_afbc_shader_data);
   shader->key = key;
         #define COMPILE_SHADER(name, ...)                                              \
      {                                                                           \
      nir_shader *nir =                                                        \
         nir->info.num_ubos = 1;                                                  \
   struct pipe_compute_state cso = {PIPE_SHADER_IR_NIR, nir};               \
               COMPILE_SHADER(size, key.bpp, key.align);
         #undef COMPILE_SHADER
         pthread_mutex_lock(&ctx->afbc_shaders.lock);
   _mesa_hash_table_insert(ctx->afbc_shaders.shaders, &shader->key, shader);
               }
      static uint32_t
   panfrost_afbc_shader_key_hash(const void *key)
   {
         }
      static bool
   panfrost_afbc_shader_key_equal(const void *a, const void *b)
   {
         }
      void
   panfrost_afbc_context_init(struct panfrost_context *ctx)
   {
      ctx->afbc_shaders.shaders = _mesa_hash_table_create(
            }
      void
   panfrost_afbc_context_destroy(struct panfrost_context *ctx)
   {
      _mesa_hash_table_destroy(ctx->afbc_shaders.shaders, NULL);
      }
