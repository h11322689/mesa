   /*
   * Copyright (C) 2019 Alyssa Rosenzweig
   * Copyright (C) 2017-2018 Lyude Paul
   * Copyright (C) 2019 Collabora, Ltd.
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
      #include <assert.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <sys/mman.h>
      #include "util/macros.h"
   #include "util/u_debug.h"
   #include "util/u_hexdump.h"
   #include "decode.h"
      #include "compiler/bifrost/disassemble.h"
   #include "compiler/valhall/disassemble.h"
   #include "midgard/disassemble.h"
      /* Used to distiguish dumped files, otherwise we would have to print the ctx
   * pointer, which is annoying for the user since it changes with every run */
   static int num_ctxs = 0;
      #define to_mapped_memory(x)                                                    \
            /*
   * Compare a GPU VA to a node, considering a GPU VA to be equal to a node if it
   * is contained in the interval the node represents. This lets us store
   * intervals in our tree.
   */
   static int
   pandecode_cmp_key(const struct rb_node *lhs, const void *key)
   {
      struct pandecode_mapped_memory *mem = to_mapped_memory(lhs);
            if (mem->gpu_va <= *gpu_va && *gpu_va < (mem->gpu_va + mem->length))
         else
      }
      static int
   pandecode_cmp(const struct rb_node *lhs, const struct rb_node *rhs)
   {
         }
      static struct pandecode_mapped_memory *
   pandecode_find_mapped_gpu_mem_containing_rw(struct pandecode_context *ctx,
         {
               struct rb_node *node =
               }
      struct pandecode_mapped_memory *
   pandecode_find_mapped_gpu_mem_containing(struct pandecode_context *ctx,
         {
               struct pandecode_mapped_memory *mem =
            if (mem && mem->addr && !mem->ro) {
      mprotect(mem->addr, mem->length, PROT_READ);
   mem->ro = true;
   util_dynarray_append(&ctx->ro_mappings, struct pandecode_mapped_memory *,
                  }
      /*
   * To check for memory safety issues, validates that the given pointer in GPU
   * memory is valid, containing at least sz bytes. This function is a tool to
   * detect GPU-side memory bugs by validating pointers.
   */
   void
   pandecode_validate_buffer(struct pandecode_context *ctx, mali_ptr addr,
         {
      if (!addr) {
      pandecode_log(ctx, "// XXX: null pointer deref\n");
                        struct pandecode_mapped_memory *bo =
            if (!bo) {
      pandecode_log(ctx, "// XXX: invalid memory dereference\n");
                        unsigned offset = addr - bo->gpu_va;
            if (total > bo->length) {
      pandecode_log(ctx,
               "// XXX: buffer overrun. "
   "Chunk of size %zu at offset %d in buffer of size %zu. "
         }
      void
   pandecode_map_read_write(struct pandecode_context *ctx)
   {
               util_dynarray_foreach(&ctx->ro_mappings, struct pandecode_mapped_memory *,
            (*mem)->ro = false;
      }
      }
      static void
   pandecode_add_name(struct pandecode_context *ctx,
               {
               if (!name) {
                  } else {
      assert((strlen(name) + 1) < sizeof(mem->name));
         }
      void
   pandecode_inject_mmap(struct pandecode_context *ctx, uint64_t gpu_va, void *cpu,
         {
                        struct pandecode_mapped_memory *existing =
            if (existing && existing->gpu_va == gpu_va) {
      existing->length = sz;
   existing->addr = cpu;
      } else {
      /* Otherwise, add a fresh mapping */
            mapped_mem = calloc(1, sizeof(*mapped_mem));
   mapped_mem->gpu_va = gpu_va;
   mapped_mem->length = sz;
   mapped_mem->addr = cpu;
            /* Add it to the tree */
                  }
      void
   pandecode_inject_free(struct pandecode_context *ctx, uint64_t gpu_va,
         {
               struct pandecode_mapped_memory *mem =
            if (mem) {
      assert(mem->gpu_va == gpu_va);
            rb_tree_remove(&ctx->mmap_tree, &mem->node);
                  }
      char *
   pointer_as_memory_reference(struct pandecode_context *ctx, uint64_t ptr)
   {
               struct pandecode_mapped_memory *mapped;
                              if (mapped) {
      snprintf(out, 128, "%s + %d", mapped->name, (int)(ptr - mapped->gpu_va));
                        snprintf(out, 128, "0x%" PRIx64, ptr);
      }
      void
   pandecode_dump_file_open(struct pandecode_context *ctx)
   {
               /* This does a getenv every frame, so it is possible to use
   * setenv to change the base at runtime.
   */
   const char *dump_file_base =
         if (!strcmp(dump_file_base, "stderr"))
         else if (!ctx->dump_stream) {
      char buffer[1024];
   snprintf(buffer, sizeof(buffer), "%s.ctx-%d.%04d", dump_file_base,
         printf("pandecode: dump command stream to file %s\n", buffer);
   ctx->dump_stream = fopen(buffer, "w");
   if (!ctx->dump_stream)
      fprintf(stderr,
            }
      static void
   pandecode_dump_file_close(struct pandecode_context *ctx)
   {
               if (ctx->dump_stream && ctx->dump_stream != stderr) {
      if (fclose(ctx->dump_stream))
                  }
      struct pandecode_context *
   pandecode_create_context(bool to_stderr)
   {
               /* Not thread safe, but we shouldn't ever hit this, and even if we do, the
   * worst that could happen is having the files dumped with their filenames
   * in a different order. */
            /* This will be initialized later and can be changed at run time through
   * the PANDECODE_DUMP_FILE environment variable.
   */
            rb_tree_init(&ctx->mmap_tree);
            simple_mtx_t mtx_init = SIMPLE_MTX_INITIALIZER;
               }
      void
   pandecode_next_frame(struct pandecode_context *ctx)
   {
               pandecode_dump_file_close(ctx);
               }
      void
   pandecode_destroy_context(struct pandecode_context *ctx)
   {
               rb_tree_foreach_safe(struct pandecode_mapped_memory, it, &ctx->mmap_tree,
            rb_tree_remove(&ctx->mmap_tree, &it->node);
               util_dynarray_fini(&ctx->ro_mappings);
                        }
      void
   pandecode_dump_mappings(struct pandecode_context *ctx)
   {
                        rb_tree_foreach(struct pandecode_mapped_memory, it, &ctx->mmap_tree, node) {
      if (!it->addr || !it->length)
            fprintf(ctx->dump_stream, "Buffer: %s gpu %" PRIx64 "\n\n", it->name,
            u_hexdump(ctx->dump_stream, it->addr, it->length, false);
               fflush(ctx->dump_stream);
      }
      void
   pandecode_abort_on_fault(struct pandecode_context *ctx, mali_ptr jc_gpu_va,
         {
               switch (pan_arch(gpu_id)) {
   case 4:
      pandecode_abort_on_fault_v4(ctx, jc_gpu_va);
      case 5:
      pandecode_abort_on_fault_v5(ctx, jc_gpu_va);
      case 6:
      pandecode_abort_on_fault_v6(ctx, jc_gpu_va);
      case 7:
      pandecode_abort_on_fault_v7(ctx, jc_gpu_va);
      case 9:
      pandecode_abort_on_fault_v9(ctx, jc_gpu_va);
      default:
                     }
      void
   pandecode_jc(struct pandecode_context *ctx, mali_ptr jc_gpu_va, unsigned gpu_id)
   {
               switch (pan_arch(gpu_id)) {
   case 4:
      pandecode_jc_v4(ctx, jc_gpu_va, gpu_id);
      case 5:
      pandecode_jc_v5(ctx, jc_gpu_va, gpu_id);
      case 6:
      pandecode_jc_v6(ctx, jc_gpu_va, gpu_id);
      case 7:
      pandecode_jc_v7(ctx, jc_gpu_va, gpu_id);
      case 9:
      pandecode_jc_v9(ctx, jc_gpu_va, gpu_id);
      default:
                     }
      void
   pandecode_cs(struct pandecode_context *ctx, mali_ptr queue_gpu_va,
         {
               switch (pan_arch(gpu_id)) {
   case 10:
      pandecode_cs_v10(ctx, queue_gpu_va, size, gpu_id, regs);
      default:
                     }
      void
   pandecode_shader_disassemble(struct pandecode_context *ctx, mali_ptr shader_ptr,
         {
               /* Compute maximum possible size */
   struct pandecode_mapped_memory *mem =
                  /* Print some boilerplate to clearly denote the assembly (which doesn't
            pandecode_log_cont(ctx, "\nShader %p (GPU VA %" PRIx64 ") sz %" PRId64 "\n",
            if (pan_arch(gpu_id) >= 9) {
         } else if (pan_arch(gpu_id) >= 6)
         else
               }
