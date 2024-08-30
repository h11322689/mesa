   /*
   * Copyright © 2018 Intel Corporation
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
   *
   */
      #include <assert.h>
   #include <getopt.h>
   #include <inttypes.h>
   #include <signal.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <stdarg.h>
   #include <zlib.h>
      #include "util/list.h"
      #include "aub_write.h"
   #include "intel_aub.h"
      #define fail_if(cond, ...) _fail_if(cond, NULL, __VA_ARGS__)
      #define fail(...) fail_if(true, __VA_ARGS__)
      static int zlib_inflate(uint32_t **ptr, int len)
   {
      struct z_stream_s zstream;
   void *out;
                     zstream.next_in = (unsigned char *)*ptr;
            if (inflateInit(&zstream) != Z_OK)
            out = malloc(out_size);
   zstream.next_out = out;
            do {
      switch (inflate(&zstream, Z_SYNC_FLUSH)) {
   case Z_STREAM_END:
         case Z_OK:
         default:
      inflateEnd(&zstream);
               if (zstream.avail_out)
            out = realloc(out, 2*zstream.total_out);
   if (out == NULL) {
      inflateEnd(&zstream);
               zstream.next_out = (unsigned char *)out + zstream.total_out;
         end:
      inflateEnd(&zstream);
   free(*ptr);
   *ptr = out;
      }
      static int ascii85_decode(const char *in, uint32_t **out, bool inflate)
   {
               *out = realloc(*out, sizeof(uint32_t)*size);
   if (*out == NULL)
            while (*in >= '!' && *in <= 'z') {
               if (len == size) {
      size *= 2;
   *out = realloc(*out, sizeof(uint32_t)*size);
   if (*out == NULL)
               if (*in == 'z') {
         } else {
      v += in[0] - 33; v *= 85;
   v += in[1] - 33; v *= 85;
   v += in[2] - 33; v *= 85;
   v += in[3] - 33; v *= 85;
   v += in[4] - 33;
      }
               if (!inflate)
               }
      static void
   print_help(const char *progname, FILE *file)
   {
      fprintf(file,
         "Usage: %s [OPTION]... [FILE]\n"
   "Convert an Intel GPU i915 error state to an aub file.\n"
   "  -h, --help          display this help and exit\n"
      }
      struct bo {
      enum address_space {
      PPGTT,
      } gtt;
   enum bo_type {
      BO_TYPE_UNKNOWN = 0,
   BO_TYPE_BATCH,
   BO_TYPE_USER,
   BO_TYPE_CONTEXT,
   BO_TYPE_RINGBUFFER,
   BO_TYPE_STATUS,
      } type;
   const char *name;
   uint64_t addr;
   uint8_t *data;
            enum intel_engine_class engine_class;
               };
      static struct bo *
   find_or_create(struct list_head *bo_list, uint64_t addr,
                     {
      list_for_each_entry(struct bo, bo_entry, bo_list, link) {
      if (bo_entry->addr == addr &&
      bo_entry->gtt == gtt &&
   bo_entry->engine_class == engine_class &&
   bo_entry->engine_instance == engine_instance)
            struct bo *new_bo = calloc(1, sizeof(*new_bo));
   new_bo->addr = addr;
   new_bo->gtt = gtt;
   new_bo->engine_class = engine_class;
   new_bo->engine_instance = engine_instance;
               }
      static void
   engine_from_name(const char *engine_name,
               {
      const struct {
      const char *match;
   enum intel_engine_class engine_class;
      } rings[] = {
      { "rcs", INTEL_ENGINE_CLASS_RENDER, true },
   { "vcs", INTEL_ENGINE_CLASS_VIDEO, true },
   { "vecs", INTEL_ENGINE_CLASS_VIDEO_ENHANCE, true },
   { "bcs", INTEL_ENGINE_CLASS_COPY, true },
   { "global", INTEL_ENGINE_CLASS_INVALID, false },
   { "render command stream", INTEL_ENGINE_CLASS_RENDER, false },
   { "blt command stream", INTEL_ENGINE_CLASS_COPY, false },
   { "bsd command stream", INTEL_ENGINE_CLASS_VIDEO, false },
   { "vebox command stream", INTEL_ENGINE_CLASS_VIDEO_ENHANCE, false },
               for (r = rings; r->match; r++) {
      if (strncasecmp(engine_name, r->match, strlen(r->match)) == 0) {
      *engine_class = r->engine_class;
   if (r->parse_instance)
         else
                           }
      int
   main(int argc, char *argv[])
   {
      int i, c;
   bool help = false, verbose = false;
   char *out_filename = NULL, *in_filename = NULL;
   const struct option aubinator_opts[] = {
      { "help",       no_argument,       NULL,     'h' },
   { "output",     required_argument, NULL,     'o' },
   { "verbose",    no_argument,       NULL,     'v' },
               i = 0;
   while ((c = getopt_long(argc, argv, "ho:v", aubinator_opts, &i)) != -1) {
      switch (c) {
   case 'h':
      help = true;
      case 'o':
      out_filename = strdup(optarg);
      case 'v':
      verbose = true;
      default:
                     if (optind < argc)
            if (help || argc == 1 || !in_filename) {
      print_help(argv[0], stderr);
               if (out_filename == NULL) {
      int out_filename_size = strlen(in_filename) + 5;
   out_filename = malloc(out_filename_size);
               FILE *err_file = fopen(in_filename, "r");
            FILE *aub_file = fopen(out_filename, "w");
                     enum intel_engine_class active_engine_class = INTEL_ENGINE_CLASS_INVALID;
            enum address_space active_gtt = PPGTT;
            struct {
      struct {
      uint32_t ring_buffer_head;
         } engines[INTEL_ENGINE_CLASS_INVALID + 1];
                     struct list_head bo_list;
                     char *line = NULL;
   size_t line_size;
   while (getline(&line, &line_size, err_file) > 0) {
      const char *pci_id_start = strstr(line, "PCI ID");
   if (pci_id_start) {
      int pci_id;
                  aub_file_init(&aub, aub_file,
         if (verbose)
         default_gtt = active_gtt = aub_use_execlists(&aub) ? PPGTT : GGTT;
               if (strstr(line, " command stream:")) {
      engine_from_name(line, &active_engine_class, &active_engine_instance);
               if (sscanf(line, "  ring->head: 0x%x\n",
            &engines[
                     if (sscanf(line, "  ring->tail: 0x%x\n",
            &engines[
                     const char *active_start = "Active (";
   if (strncmp(line, active_start, strlen(active_start)) == 0) {
                              char *count = strchr(ring, '[');
   fail_if(!count || sscanf(count, "[%d]:", &num_ring_bos) < 1,
                     const char *global_start = "Pinned (global) [";
   if (strncmp(line, global_start, strlen(global_start)) == 0) {
      active_engine_class = INTEL_ENGINE_CLASS_INVALID;
   active_engine_instance = -1;
   active_gtt = GGTT;
               if (num_ring_bos > 0) {
      unsigned hi, lo, size;
   if (sscanf(line, " %x_%x %d", &hi, &lo, &size) == 3) {
      struct bo *bo_entry = find_or_create(&bo_list, ((uint64_t)hi) << 32 | lo,
                     bo_entry->size = size;
      } else {
         }
               if (line[0] == ':' || line[0] == '~') {
                     int count = ascii85_decode(line+1, (uint32_t **) &last_bo->data, line[0] == ':');
   fail_if(count == 0, "ASCII85 decode failed.\n");
   last_bo->size = count * 4;
               char *dashes = strstr(line, " --- ");
   if (dashes) {
                        uint32_t hi, lo;
   char *bo_address_str = strchr(dashes, '=');
                  const struct {
      const char *match;
   enum bo_type type;
      } bo_types[] = {
      { "gtt_offset", BO_TYPE_BATCH,      default_gtt },
   { "batch",      BO_TYPE_BATCH,      default_gtt },
   { "user",       BO_TYPE_USER,       default_gtt },
   { "HW context", BO_TYPE_CONTEXT,    GGTT },
   { "ringbuffer", BO_TYPE_RINGBUFFER, GGTT },
   { "HW Status",  BO_TYPE_STATUS,     GGTT },
   { "WA context", BO_TYPE_CONTEXT_WA, GGTT },
               for (b = bo_types; b->type != BO_TYPE_UNKNOWN; b++) {
      if (strncasecmp(dashes, b->match, strlen(b->match)) == 0)
               last_bo = find_or_create(&bo_list, ((uint64_t) hi) << 32 | lo,
                  /* The batch buffer will appear twice as gtt_offset and user. Only
   * keep the batch type.
   */
   if (last_bo->type == BO_TYPE_UNKNOWN) {
      last_bo->type = b->type;
                              if (verbose) {
      fprintf(stdout, "BOs found:\n");
   list_for_each_entry(struct bo, bo_entry, &bo_list, link) {
      fprintf(stdout, "\t type=%i addr=0x%016" PRIx64 " size=%" PRIu64 "\n",
                  /* Find the batch that trigger the hang */
   struct bo *batch_bo = NULL;
   list_for_each_entry(struct bo, bo_entry, &bo_list, link) {
      if (bo_entry->type == BO_TYPE_BATCH) {
      batch_bo = bo_entry;
         }
            /* Add all the BOs to the aub file */
   struct bo *hwsp_bo = NULL;
   list_for_each_entry(struct bo, bo_entry, &bo_list, link) {
      switch (bo_entry->type) {
   case BO_TYPE_BATCH:
      if (bo_entry->gtt == PPGTT) {
      aub_map_ppgtt(&aub, bo_entry->addr, bo_entry->size);
   aub_write_trace_block(&aub, AUB_TRACE_TYPE_BATCH,
      } else
            case BO_TYPE_USER:
      if (bo_entry->gtt == PPGTT) {
      aub_map_ppgtt(&aub, bo_entry->addr, bo_entry->size);
   aub_write_trace_block(&aub, AUB_TRACE_TYPE_NOTYPE,
      } else
            case BO_TYPE_CONTEXT:
      if (bo_entry->engine_class == batch_bo->engine_class &&
      bo_entry->engine_instance == batch_bo->engine_instance &&
                           if (context[1] == 0) {
      fprintf(stderr,
                     /* Update the ring buffer at the last known location. */
   context[5] = engines[bo_entry->engine_class].instances[bo_entry->engine_instance].ring_buffer_head;
   context[7] = engines[bo_entry->engine_class].instances[bo_entry->engine_instance].ring_buffer_tail;
                  /* The error state doesn't provide a dump of the page tables, so
   * we have to provide our own, that's easy enough.
   */
                  fprintf(stdout, "context dump:\n");
   for (int i = 0; i < 60; i++) {
      if (i % 4 == 0)
                        }
   aub_write_ggtt(&aub, bo_entry->addr, bo_entry->size, bo_entry->data);
      case BO_TYPE_RINGBUFFER:
   case BO_TYPE_STATUS:
   case BO_TYPE_CONTEXT_WA:
      aub_write_ggtt(&aub, bo_entry->addr, bo_entry->size, bo_entry->data);
      case BO_TYPE_UNKNOWN:
      if (bo_entry->gtt == PPGTT) {
      aub_map_ppgtt(&aub, bo_entry->addr, bo_entry->size);
   if (bo_entry->data) {
      aub_write_trace_block(&aub, AUB_TRACE_TYPE_NOTYPE,
         } else {
      if (bo_entry->size > 0) {
      void *zero_data = calloc(1, bo_entry->size);
   aub_write_ggtt(&aub, bo_entry->addr, bo_entry->size, zero_data);
         }
      default:
                     if (aub_use_execlists(&aub)) {
      fail_if(!hwsp_bo, "Failed to find Context buffer.\n");
      } else {
      /* Use context id 0 -- if we are not using execlists it doesn't matter
   * anyway
   */
               /* Cleanup */
   list_for_each_entry_safe(struct bo, bo_entry, &bo_list, link) {
      list_del(&bo_entry->link);
   free(bo_entry->data);
               free(out_filename);
   free(line);
   if(err_file) {
         }
   if(aub.file) {
         } else if(aub_file) {
         }
      }
      /* vim: set ts=8 sw=8 tw=0 cino=:0,(0 noet :*/
