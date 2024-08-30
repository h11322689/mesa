   /*
   * Copyright © 2020 Intel Corporation
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
      #include "nir.h"
   #include "nir_clc_helpers.h"
   #include "nir_serialize.h"
   #include "nir_spirv.h"
   #include "util/mesa-sha1.h"
      #ifdef DYNAMIC_LIBCLC_PATH
   #include <fcntl.h>
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <sys/mman.h>
   #include <unistd.h>
   #endif
      #ifdef HAVE_STATIC_LIBCLC_ZSTD
   #include <zstd.h>
   #endif
      #ifdef HAVE_STATIC_LIBCLC_SPIRV
   #include "spirv-mesa3d-.spv.h"
   #endif
      #ifdef HAVE_STATIC_LIBCLC_SPIRV64
   #include "spirv64-mesa3d-.spv.h"
   #endif
      struct clc_file {
      unsigned bit_size;
   const char *static_data;
   size_t static_data_size;
      };
      static const struct clc_file libclc_files[] = {
      {
      #ifdef HAVE_STATIC_LIBCLC_SPIRV
         .static_data = libclc_spirv_mesa3d_spv,
   #endif
   #ifdef DYNAMIC_LIBCLC_PATH
         #endif
      },
   {
      #ifdef HAVE_STATIC_LIBCLC_SPIRV64
         .static_data = libclc_spirv64_mesa3d_spv,
   #endif
   #ifdef DYNAMIC_LIBCLC_PATH
         #endif
         };
      static const struct clc_file *
   get_libclc_file(unsigned ptr_bit_size)
   {
      assert(ptr_bit_size == 32 || ptr_bit_size == 64);
      }
      struct clc_data {
                        int fd;
   const void *data;
      };
      static bool
   open_clc_data(struct clc_data *clc, unsigned ptr_bit_size)
   {
      memset(clc, 0, sizeof(*clc));
   clc->file = get_libclc_file(ptr_bit_size);
            if (clc->file->static_data) {
      snprintf((char *)clc->cache_key, sizeof(clc->cache_key),
                  #ifdef DYNAMIC_LIBCLC_PATH
      if (clc->file->sys_path != NULL) {
      int fd = open(clc->file->sys_path, O_RDONLY);
   if (fd < 0)
            struct stat stat;
   int ret = fstat(fd, &stat);
   if (ret < 0) {
      fprintf(stderr, "fstat failed on %s: %m\n", clc->file->sys_path);
   close(fd);
               struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);
   _mesa_sha1_update(&ctx, clc->file->sys_path, strlen(clc->file->sys_path));
   _mesa_sha1_update(&ctx, &stat.st_mtim, sizeof(stat.st_mtim));
                           #endif
            }
      #define SPIRV_WORD_SIZE 4
      static bool
   map_clc_data(struct clc_data *clc)
   {
         #ifdef HAVE_STATIC_LIBCLC_ZSTD
         unsigned long long cmp_size =
      ZSTD_getFrameContentSize(clc->file->static_data,
      if (cmp_size == ZSTD_CONTENTSIZE_UNKNOWN ||
      cmp_size == ZSTD_CONTENTSIZE_ERROR) {
   fprintf(stderr, "Could not determine the decompressed size of the "
                     size_t frame_size =
      ZSTD_findFrameCompressedSize(clc->file->static_data,
      if (ZSTD_isError(frame_size)) {
      fprintf(stderr, "Could not determine the size of the first ZSTD frame "
                           void *dest = malloc(cmp_size + 1);
   size_t size = ZSTD_decompress(dest, cmp_size, clc->file->static_data,
         if (ZSTD_isError(size)) {
      free(dest);
   fprintf(stderr, "Error decompressing libclc SPIR-V: %s\n",
                     clc->data = dest;
   #else
         clc->data = clc->file->static_data;
   #endif
                  #ifdef DYNAMIC_LIBCLC_PATH
      if (clc->file->sys_path != NULL) {
      off_t len = lseek(clc->fd, 0, SEEK_END);
   if (len % SPIRV_WORD_SIZE != 0) {
      fprintf(stderr, "File length isn't a multiple of the word size\n");
      }
            clc->data = mmap(NULL, len, PROT_READ, MAP_PRIVATE, clc->fd, 0);
   if (clc->data == MAP_FAILED) {
      fprintf(stderr, "Failed to mmap libclc SPIR-V: %m\n");
                     #endif
            }
      static void
   close_clc_data(struct clc_data *clc)
   {
         #ifdef HAVE_STATIC_LIBCLC_ZSTD
         #endif
                  #ifdef DYNAMIC_LIBCLC_PATH
      if (clc->file->sys_path != NULL) {
      if (clc->data)
               #endif
   }
      /** Returns true if libclc is found
   *
   * If libclc is compiled in statically, this always returns true.  If we
   * depend on a dynamic libclc, this opens and tries to stat the file.
   */
   bool
   nir_can_find_libclc(unsigned ptr_bit_size)
   {
      struct clc_data clc;
   if (open_clc_data(&clc, ptr_bit_size)) {
      close_clc_data(&clc);
      } else {
            }
      /** Adds generic pointer variants of libclc functions
   *
   * Libclc currently doesn't contain generic variants for a bunch of functions
   * like `frexp` but the OpenCL spec with generic pointers requires them.  We
   * really should fix libclc but, in the mean time, we can easily duplicate
   * every function that works on global memory and make it also work on generic
   * memory.
   */
   static void
   libclc_add_generic_variants(nir_shader *shader)
   {
      nir_foreach_function(func, shader) {
      /* These don't need generic variants */
   if (strstr(func->name, "async_work_group_strided_copy"))
            char *U3AS1 = strstr(func->name, "U3AS1");
   if (U3AS1 == NULL)
            ptrdiff_t offset_1 = U3AS1 - func->name + 4;
            char *generic_name = ralloc_strdup(shader, func->name);
   assert(generic_name[offset_1] == '1');
            if (nir_shader_get_function_for_name(shader, generic_name))
            nir_function *gfunc = nir_function_create(shader, generic_name);
   gfunc->num_params = func->num_params;
   gfunc->params = ralloc_array(shader, nir_parameter, gfunc->num_params);
   for (unsigned i = 0; i < gfunc->num_params; i++)
                     /* Rewrite any global pointers to generic */
   nir_foreach_block(block, gfunc->impl) {
      nir_foreach_instr(instr, block) {
                     nir_deref_instr *deref = nir_instr_as_deref(instr);
                                                      }
      nir_shader *
   nir_load_libclc_shader(unsigned ptr_bit_size,
                           {
      assert(ptr_bit_size ==
            struct clc_data clc;
   if (!open_clc_data(&clc, ptr_bit_size))
         #ifdef ENABLE_SHADER_CACHE
      cache_key cache_key;
   if (disk_cache) {
      disk_cache_compute_key(disk_cache, clc.cache_key,
            size_t buffer_size;
   uint8_t *buffer = disk_cache_get(disk_cache, cache_key, &buffer_size);
   if (buffer) {
      struct blob_reader blob;
   blob_reader_init(&blob, buffer, buffer_size);
   nir_shader *nir = nir_deserialize(NULL, nir_options, &blob);
   free(buffer);
   close_clc_data(&clc);
            #endif
         if (!map_clc_data(&clc)) {
      close_clc_data(&clc);
               struct spirv_to_nir_options spirv_lib_options = *spirv_options;
            assert(clc.size % SPIRV_WORD_SIZE == 0);
   nir_shader *nir = spirv_to_nir(clc.data, clc.size / SPIRV_WORD_SIZE,
                        /* nir_inline_libclc will assume that the functions in this shader are
   * already ready to lower.  This means we need to inline any function_temp
   * initializers and lower any early returns.
   */
   nir->info.internal = true;
   NIR_PASS_V(nir, nir_lower_variable_initializers, nir_var_function_temp);
                     /* Run some optimization passes. Those used here should be considered safe
   * for all use cases and drivers.
   */
   if (optimize) {
               bool progress;
   do {
      progress = false;
   NIR_PASS(progress, nir, nir_opt_copy_prop_vars);
   NIR_PASS(progress, nir, nir_lower_var_copies);
   NIR_PASS(progress, nir, nir_lower_vars_to_ssa);
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_remove_phis);
   NIR_PASS(progress, nir, nir_opt_dce);
   NIR_PASS(progress, nir, nir_opt_if, false);
   NIR_PASS(progress, nir, nir_opt_dead_cf);
   NIR_PASS(progress, nir, nir_opt_cse);
   /* drivers run this pass, so don't be too aggressive. More aggressive
   * values only increase effectiveness by <5%
   */
   NIR_PASS(progress, nir, nir_opt_peephole_select, 0, false, false);
   NIR_PASS(progress, nir, nir_opt_algebraic);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
   NIR_PASS(progress, nir, nir_opt_undef);
                        #ifdef ENABLE_SHADER_CACHE
      if (disk_cache) {
      struct blob blob;
   blob_init(&blob);
   nir_serialize(&blob, nir, false);
   disk_cache_put(disk_cache, cache_key, blob.data, blob.size, NULL);
         #endif
         close_clc_data(&clc);
      }
