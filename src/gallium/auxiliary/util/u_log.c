   /*
   * Copyright 2017 Advanced Micro Devices, Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "u_log.h"
      #include "util/u_memory.h"
   #include "util/u_string.h"
      struct page_entry {
      const struct u_log_chunk_type *type;
      };
      struct u_log_page {
      struct page_entry *entries;
   unsigned num_entries;
      };
      struct u_log_auto_logger {
      u_auto_log_fn *callback;
      };
      /**
   * Initialize the given logging context.
   */
   void
   u_log_context_init(struct u_log_context *ctx)
   {
         }
      /**
   * Free all resources associated with the given logging context.
   *
   * Pages taken from the context via \ref u_log_new_page must be destroyed
   * separately.
   */
   void
   u_log_context_destroy(struct u_log_context *ctx)
   {
      u_log_page_destroy(ctx->cur);
   FREE(ctx->auto_loggers);
      }
      /**
   * Add an auto logger.
   *
   * Auto loggers are called each time a chunk is added to the log.
   */
   void
   u_log_add_auto_logger(struct u_log_context *ctx, u_auto_log_fn *callback,
         {
      struct u_log_auto_logger *new_auto_loggers =
      REALLOC(ctx->auto_loggers,
            if (!new_auto_loggers) {
      fprintf(stderr, "Gallium u_log: out of memory\n");
               unsigned idx = ctx->num_auto_loggers++;
   ctx->auto_loggers = new_auto_loggers;
   ctx->auto_loggers[idx].callback = callback;
      }
      /**
   * Make sure that auto loggers have run.
   */
   void
   u_log_flush(struct u_log_context *ctx)
   {
      if (!ctx->num_auto_loggers)
            struct u_log_auto_logger *auto_loggers = ctx->auto_loggers;
            /* Prevent recursion. */
   ctx->num_auto_loggers = 0;
            for (unsigned i = 0; i < num_auto_loggers; ++i)
            assert(!ctx->num_auto_loggers);
   ctx->num_auto_loggers = num_auto_loggers;
      }
      static void str_print(void *data, FILE *stream)
   {
         }
      static const struct u_log_chunk_type str_chunk_type = {
      .destroy = free,
      };
      void
   u_log_printf(struct u_log_context *ctx, const char *fmt, ...)
   {
      va_list va;
            va_start(va, fmt);
   int ret = vasprintf(&str, fmt, va);
            if (ret >= 0) {
         } else {
            }
      /**
   * Add a custom chunk to the log.
   *
   * type->destroy will be called as soon as \p data is no longer needed.
   */
   void
   u_log_chunk(struct u_log_context *ctx, const struct u_log_chunk_type *type,
         {
                        if (!page) {
      ctx->cur = CALLOC_STRUCT(u_log_page);
   page = ctx->cur;
   if (!page)
               if (page->num_entries >= page->max_entries) {
      unsigned new_max_entries = MAX2(16, page->num_entries * 2);
   struct page_entry *new_entries = REALLOC(page->entries,
               if (!new_entries)
            page->entries = new_entries;
               page->entries[page->num_entries].type = type;
   page->entries[page->num_entries].data = data;
   page->num_entries++;
         out_of_memory:
         }
      /**
   * Convenience helper that starts a new page and prints the previous one.
   */
   void
   u_log_new_page_print(struct u_log_context *ctx, FILE *stream)
   {
               if (ctx->cur) {
      u_log_page_print(ctx->cur, stream);
   u_log_page_destroy(ctx->cur);
         }
      /**
   * Return the current page from the logging context and start a new one.
   *
   * The caller is responsible for destroying the returned page.
   */
   struct u_log_page *
   u_log_new_page(struct u_log_context *ctx)
   {
               struct u_log_page *page = ctx->cur;
   ctx->cur = NULL;
      }
      /**
   * Free all data associated with \p page.
   */
   void
   u_log_page_destroy(struct u_log_page *page)
   {
      if (!page)
            for (unsigned i = 0; i < page->num_entries; ++i) {
      if (page->entries[i].type->destroy)
      }
   FREE(page->entries);
      }
      /**
   * Print the given page to \p stream.
   */
   void
   u_log_page_print(struct u_log_page *page, FILE *stream)
   {
      for (unsigned i = 0; i < page->num_entries; ++i)
      }
