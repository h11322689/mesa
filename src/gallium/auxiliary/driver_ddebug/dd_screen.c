   /**************************************************************************
   *
   * Copyright 2015 Advanced Micro Devices, Inc.
   * Copyright 2008 VMware, Inc.
   * All Rights Reserved.
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
   *
   **************************************************************************/
      #include "dd_pipe.h"
   #include "dd_public.h"
   #include "util/u_memory.h"
   #include <ctype.h>
   #include <stdio.h>
         static const char *
   dd_screen_get_name(struct pipe_screen *_screen)
   {
                  }
      static const char *
   dd_screen_get_vendor(struct pipe_screen *_screen)
   {
                  }
      static const char *
   dd_screen_get_device_vendor(struct pipe_screen *_screen)
   {
                  }
      static const void *
   dd_screen_get_compiler_options(struct pipe_screen *_screen,
               {
                  }
      static struct disk_cache *
   dd_screen_get_disk_shader_cache(struct pipe_screen *_screen)
   {
                  }
      static int
   dd_screen_get_param(struct pipe_screen *_screen,
         {
                  }
      static float
   dd_screen_get_paramf(struct pipe_screen *_screen,
         {
                  }
      static int
   dd_screen_get_compute_param(struct pipe_screen *_screen,
                     {
                  }
      static int
   dd_screen_get_shader_param(struct pipe_screen *_screen,
               {
                  }
      static uint64_t
   dd_screen_get_timestamp(struct pipe_screen *_screen)
   {
                  }
      static void dd_screen_query_memory_info(struct pipe_screen *_screen,
         {
                  }
      static struct pipe_context *
   dd_screen_context_create(struct pipe_screen *_screen, void *priv,
         {
      struct dd_screen *dscreen = dd_screen(_screen);
                     return dd_context_create(dscreen,
      }
      static bool
   dd_screen_is_format_supported(struct pipe_screen *_screen,
                                 {
               return screen->is_format_supported(screen, format, target, sample_count,
      }
      static bool
   dd_screen_can_create_resource(struct pipe_screen *_screen,
         {
                  }
      static void
   dd_screen_flush_frontbuffer(struct pipe_screen *_screen,
                                 {
      struct pipe_screen *screen = dd_screen(_screen)->screen;
            screen->flush_frontbuffer(screen, pipe, resource, level, layer, context_private,
      }
      static int
   dd_screen_get_driver_query_info(struct pipe_screen *_screen,
               {
                  }
      static int
   dd_screen_get_driver_query_group_info(struct pipe_screen *_screen,
               {
                  }
         static void
   dd_screen_get_driver_uuid(struct pipe_screen *_screen, char *uuid)
   {
                  }
      static void
   dd_screen_get_device_uuid(struct pipe_screen *_screen, char *uuid)
   {
                  }
      /********************************************************************
   * resource
   */
      static struct pipe_resource *
   dd_screen_resource_create(struct pipe_screen *_screen,
         {
      struct pipe_screen *screen = dd_screen(_screen)->screen;
            if (!res)
         res->screen = _screen;
      }
      static struct pipe_resource *
   dd_screen_resource_from_handle(struct pipe_screen *_screen,
                     {
      struct pipe_screen *screen = dd_screen(_screen)->screen;
   struct pipe_resource *res =
            if (!res)
         res->screen = _screen;
      }
      static struct pipe_resource *
   dd_screen_resource_from_user_memory(struct pipe_screen *_screen,
               {
      struct pipe_screen *screen = dd_screen(_screen)->screen;
   struct pipe_resource *res =
            if (!res)
         res->screen = _screen;
      }
      static struct pipe_resource *
   dd_screen_resource_from_memobj(struct pipe_screen *_screen,
                     {
      struct pipe_screen *screen = dd_screen(_screen)->screen;
   struct pipe_resource *res =
            if (!res)
         res->screen = _screen;
      }
      static void
   dd_screen_resource_changed(struct pipe_screen *_screen,
         {
               if (screen->resource_changed)
      }
      static void
   dd_screen_resource_destroy(struct pipe_screen *_screen,
         {
                  }
      static bool
   dd_screen_resource_get_handle(struct pipe_screen *_screen,
                           {
      struct pipe_screen *screen = dd_screen(_screen)->screen;
               }
      static bool
   dd_screen_resource_get_param(struct pipe_screen *_screen,
                              struct pipe_context *_pipe,
   struct pipe_resource *resource,
   unsigned plane,
      {
      struct pipe_screen *screen = dd_screen(_screen)->screen;
            return screen->resource_get_param(screen, pipe, resource, plane, layer,
      }
      static void
   dd_screen_resource_get_info(struct pipe_screen *_screen,
                     {
                  }
      static bool
   dd_screen_check_resource_capability(struct pipe_screen *_screen,
               {
                  }
      static int
   dd_screen_get_sparse_texture_virtual_page_size(struct pipe_screen *_screen,
                                 {
               return screen->get_sparse_texture_virtual_page_size(
      }
      /********************************************************************
   * fence
   */
      static void
   dd_screen_fence_reference(struct pipe_screen *_screen,
               {
                  }
      static bool
   dd_screen_fence_finish(struct pipe_screen *_screen,
                     {
      struct pipe_screen *screen = dd_screen(_screen)->screen;
               }
      static int
   dd_screen_fence_get_fd(struct pipe_screen *_screen,
         {
                  }
      /********************************************************************
   * vertex state
   */
      static struct pipe_vertex_state *
   dd_screen_create_vertex_state(struct pipe_screen *_screen,
                                 {
      struct pipe_screen *screen = dd_screen(_screen)->screen;
   struct pipe_vertex_state *state =
      screen->create_vertex_state(screen, buffer, elements, num_elements,
         if (!state)
         state->screen = _screen;
      }
      static void
   dd_screen_vertex_state_destroy(struct pipe_screen *_screen,
         {
                  }
      /********************************************************************
   * memobj
   */
      static struct pipe_memory_object *
   dd_screen_memobj_create_from_handle(struct pipe_screen *_screen,
               {
                  }
      static void
   dd_screen_memobj_destroy(struct pipe_screen *_screen,
         {
                  }
   /********************************************************************
   * screen
   */
      static char *
   dd_screen_finalize_nir(struct pipe_screen *_screen, void *nir)
   {
                  }
      static void
   dd_screen_destroy(struct pipe_screen *_screen)
   {
      struct dd_screen *dscreen = dd_screen(_screen);
            screen->destroy(screen);
      }
      static void
   skip_space(const char **p)
   {
      while (isspace(**p))
      }
      static bool
   match_word(const char **cur, const char *word)
   {
      size_t len = strlen(word);
   if (strncmp(*cur, word, len) != 0)
            const char *p = *cur + len;
   if (*p) {
      if (!isspace(*p))
               } else {
                     }
      static bool
   match_uint(const char **cur, unsigned *value)
   {
      char *end;
   unsigned v = strtoul(*cur, &end, 0);
   if (end == *cur || (*end && !isspace(*end)))
         *cur = end;
   *value = v;
      }
      struct pipe_screen *
   ddebug_screen_create(struct pipe_screen *screen)
   {
      struct dd_screen *dscreen;
   const char *option;
   bool flush = false;
   bool verbose = false;
   bool transfers = false;
   unsigned timeout = 1000;
   unsigned apitrace_dump_call = 0;
            option = debug_get_option("GALLIUM_DDEBUG", NULL);
   if (!option)
            if (!strcmp(option, "help")) {
      puts("Gallium driver debugger");
   puts("");
   puts("Usage:");
   puts("");
   puts("  GALLIUM_DDEBUG=\"[<timeout in ms>] [(always|apitrace <call#)] [flush] [transfers] [verbose]\"");
   puts("  GALLIUM_DDEBUG_SKIP=[count]");
   puts("");
   puts("Dump context and driver information of draw calls into");
   puts("$HOME/"DD_DIR"/. By default, watch for GPU hangs and only dump information");
   puts("about draw calls related to the hang.");
   puts("");
   puts("<timeout in ms>");
   puts("  Change the default timeout for GPU hang detection (default=1000ms).");
   puts("  Setting this to 0 will disable GPU hang detection entirely.");
   puts("");
   puts("always");
   puts("  Dump information about all draw calls.");
   puts("");
   puts("transfers");
   puts("  Also dump and do hang detection on transfers.");
   puts("");
   puts("apitrace <call#>");
   puts("  Dump information about the draw call corresponding to the given");
   puts("  apitrace call number and exit.");
   puts("");
   puts("flush");
   puts("  Flush after every draw call.");
   puts("");
   puts("verbose");
   puts("  Write additional information to stderr.");
   puts("");
   puts("GALLIUM_DDEBUG_SKIP=count");
   puts("  Skip dumping on the first count draw calls (only relevant with 'always').");
   puts("");
               for (;;) {
      skip_space(&option);
   if (!*option)
            if (match_word(&option, "always")) {
      if (mode == DD_DUMP_APITRACE_CALL) {
      printf("ddebug: both 'always' and 'apitrace' specified\n");
                  } else if (match_word(&option, "flush")) {
         } else if (match_word(&option, "transfers")) {
         } else if (match_word(&option, "verbose")) {
         } else if (match_word(&option, "apitrace")) {
      if (mode != DD_DUMP_ONLY_HANGS) {
      printf("ddebug: 'apitrace' can only appear once and not mixed with 'always'\n");
               if (!match_uint(&option, &apitrace_dump_call)) {
      printf("ddebug: expected call number after 'apitrace'\n");
                  } else if (match_uint(&option, &timeout)) {
         } else {
      printf("ddebug: bad options: %s\n", option);
                  dscreen = CALLOC_STRUCT(dd_screen);
   if (!dscreen)
         #define SCR_INIT(_member) \
               dscreen->base.destroy = dd_screen_destroy;
   dscreen->base.get_name = dd_screen_get_name;
   dscreen->base.get_vendor = dd_screen_get_vendor;
   dscreen->base.get_device_vendor = dd_screen_get_device_vendor;
   SCR_INIT(get_disk_shader_cache);
   dscreen->base.get_param = dd_screen_get_param;
   dscreen->base.get_paramf = dd_screen_get_paramf;
   dscreen->base.get_compute_param = dd_screen_get_compute_param;
   dscreen->base.get_shader_param = dd_screen_get_shader_param;
   dscreen->base.query_memory_info = dd_screen_query_memory_info;
   /* get_video_param */
   /* get_compute_param */
   SCR_INIT(get_timestamp);
   dscreen->base.context_create = dd_screen_context_create;
   dscreen->base.is_format_supported = dd_screen_is_format_supported;
   /* is_video_format_supported */
   SCR_INIT(can_create_resource);
   dscreen->base.resource_create = dd_screen_resource_create;
   dscreen->base.resource_from_handle = dd_screen_resource_from_handle;
   SCR_INIT(resource_from_memobj);
   SCR_INIT(resource_from_user_memory);
   SCR_INIT(check_resource_capability);
   dscreen->base.resource_get_handle = dd_screen_resource_get_handle;
   SCR_INIT(resource_get_param);
   SCR_INIT(resource_get_info);
   SCR_INIT(resource_changed);
   dscreen->base.resource_destroy = dd_screen_resource_destroy;
   SCR_INIT(flush_frontbuffer);
   SCR_INIT(fence_reference);
   SCR_INIT(fence_finish);
   SCR_INIT(fence_get_fd);
   SCR_INIT(memobj_create_from_handle);
   SCR_INIT(memobj_destroy);
   SCR_INIT(get_driver_query_info);
   SCR_INIT(get_driver_query_group_info);
   SCR_INIT(get_compiler_options);
   SCR_INIT(get_driver_uuid);
   SCR_INIT(get_device_uuid);
   SCR_INIT(finalize_nir);
   SCR_INIT(get_sparse_texture_virtual_page_size);
   SCR_INIT(create_vertex_state);
         #undef SCR_INIT
         dscreen->screen = screen;
   dscreen->timeout_ms = timeout;
   dscreen->dump_mode = mode;
   dscreen->flush_always = flush;
   dscreen->transfers = transfers;
   dscreen->verbose = verbose;
            switch (dscreen->dump_mode) {
   case DD_DUMP_ALL_CALLS:
      fprintf(stderr, "Gallium debugger active. Logging all calls.\n");
      case DD_DUMP_APITRACE_CALL:
      fprintf(stderr, "Gallium debugger active. Going to dump an apitrace call.\n");
      default:
      fprintf(stderr, "Gallium debugger active.\n");
               if (dscreen->timeout_ms > 0)
         else
            dscreen->skip_count = debug_get_num_option("GALLIUM_DDEBUG_SKIP", 0);
   if (dscreen->skip_count > 0) {
      fprintf(stderr, "Gallium debugger skipping the first %u draw calls.\n",
                  }
