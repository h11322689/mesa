   /*
   * Copyright © 2007-2017 Intel Corporation
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
      #include <stdbool.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <stdarg.h>
   #include <string.h>
   #include <unistd.h>
   #include <inttypes.h>
   #include <errno.h>
   #include <sys/stat.h>
   #include <sys/types.h>
   #include <sys/wait.h>
   #include <err.h>
   #include <assert.h>
   #include <getopt.h>
   #include <zlib.h>
      #include "common/intel_decoder.h"
   #include "compiler/brw_compiler.h"
   #include "dev/intel_debug.h"
   #include "util/macros.h"
      #define MIN(a, b) ((a) < (b) ? (a) : (b))
      /* options */
      static bool option_full_decode = true;
   static bool option_print_all_bb = false;
   static bool option_print_offsets = true;
   static bool option_dump_kernels = false;
   static enum { COLOR_AUTO, COLOR_ALWAYS, COLOR_NEVER } option_color;
   static char *xml_path = NULL;
      static uint32_t
   print_head(unsigned int reg)
   {
      printf("    head = 0x%08x, wraps = %d\n", reg & (0x7ffff<<2), reg >> 21);
      }
      static void
   print_register(struct intel_spec *spec, const char *name, uint32_t reg)
   {
      struct intel_group *reg_spec =
            if (reg_spec) {
      intel_print_group(stdout, reg_spec, 0, &reg, 0,
         }
      struct ring_register_mapping {
      enum intel_engine_class ring_class;
   unsigned ring_instance;
      };
      static const struct ring_register_mapping acthd_registers[] = {
      { INTEL_ENGINE_CLASS_COPY, 0, "BCS_ACTHD_UDW" },
   { INTEL_ENGINE_CLASS_VIDEO, 0, "VCS_ACTHD_UDW" },
   { INTEL_ENGINE_CLASS_VIDEO, 1, "VCS2_ACTHD_UDW" },
   { INTEL_ENGINE_CLASS_RENDER, 0, "ACTHD_UDW" },
      };
      static const struct ring_register_mapping ctl_registers[] = {
      { INTEL_ENGINE_CLASS_COPY, 0, "BCS_RING_BUFFER_CTL" },
   { INTEL_ENGINE_CLASS_VIDEO, 0, "VCS_RING_BUFFER_CTL" },
   { INTEL_ENGINE_CLASS_VIDEO, 1, "VCS2_RING_BUFFER_CTL" },
   { INTEL_ENGINE_CLASS_RENDER, 0, "RCS_RING_BUFFER_CTL" },
      };
      static const struct ring_register_mapping fault_registers[] = {
      { INTEL_ENGINE_CLASS_COPY, 0, "BCS_FAULT_REG" },
   { INTEL_ENGINE_CLASS_VIDEO, 0, "VCS_FAULT_REG" },
   { INTEL_ENGINE_CLASS_RENDER, 0, "RCS_FAULT_REG" },
      };
      static int ring_name_to_class(const char *ring_name,
         {
      static const char *class_names[] = {
      [INTEL_ENGINE_CLASS_RENDER] = "rcs",
   [INTEL_ENGINE_CLASS_COMPUTE] = "ccs",
   [INTEL_ENGINE_CLASS_COPY] = "bcs",
   [INTEL_ENGINE_CLASS_VIDEO] = "vcs",
      };
   for (size_t i = 0; i < ARRAY_SIZE(class_names); i++) {
      if (strncmp(ring_name, class_names[i], strlen(class_names[i])))
            *class = i;
               static const struct {
      const char *name;
   unsigned int class;
      } legacy_names[] = {
      { "render", INTEL_ENGINE_CLASS_RENDER, 0 },
   { "blt", INTEL_ENGINE_CLASS_COPY, 0 },
   { "bsd", INTEL_ENGINE_CLASS_VIDEO, 0 },
   { "bsd2", INTEL_ENGINE_CLASS_VIDEO, 1 },
      };
   for (size_t i = 0; i < ARRAY_SIZE(legacy_names); i++) {
      if (strcmp(ring_name, legacy_names[i].name))
            *class = legacy_names[i].class;
                  }
      static const char *
   register_name_from_ring(const struct ring_register_mapping *mapping,
               {
      enum intel_engine_class class;
            instance = ring_name_to_class(ring_name, &class);
   if (instance < 0)
            for (unsigned i = 0; i < nb_mapping; i++) {
      if (mapping[i].ring_class == class &&
      mapping[i].ring_instance == instance)
   }
      }
      static const char *
   instdone_register_for_ring(const struct intel_device_info *devinfo,
         {
      enum intel_engine_class class;
            instance = ring_name_to_class(ring_name, &class);
   if (instance < 0)
            switch (class) {
   case INTEL_ENGINE_CLASS_RENDER:
      if (devinfo->ver == 6)
         else
         case INTEL_ENGINE_CLASS_COPY:
            case INTEL_ENGINE_CLASS_VIDEO:
      switch (instance) {
   case 0:
         case 1:
         default:
               case INTEL_ENGINE_CLASS_VIDEO_ENHANCE:
            default:
                     }
      static void
   print_pgtbl_err(unsigned int reg, struct intel_device_info *devinfo)
   {
      if (reg & (1 << 26))
         if (reg & (1 << 24))
         if (reg & (1 << 23))
         if (reg & (1 << 22))
         if (reg & (1 << 21))
         if (reg & (1 << 20))
         if (reg & (1 << 19))
         if (reg & (1 << 18))
         if (reg & (1 << 17))
         if (reg & (1 << 8))
         if (reg & (1 << 4))
         if (reg & (1 << 1))
         if (reg & (1 << 0))
      }
      static void
   print_snb_fence(struct intel_device_info *devinfo, uint64_t fence)
   {
      printf("    %svalid, %c-tiled, pitch: %i, start: 0x%08x, size: %u\n",
         fence & 1 ? "" : "in",
   fence & (1<<1) ? 'y' : 'x',
   (int)(((fence>>32)&0xfff)+1)*128,
      }
      static void
   print_i965_fence(struct intel_device_info *devinfo, uint64_t fence)
   {
      printf("    %svalid, %c-tiled, pitch: %i, start: 0x%08x, size: %u\n",
         fence & 1 ? "" : "in",
   fence & (1<<1) ? 'y' : 'x',
   (int)(((fence>>2)&0x1ff)+1)*128,
      }
      static void
   print_fence(struct intel_device_info *devinfo, uint64_t fence)
   {
      if (devinfo->ver == 6 || devinfo->ver == 7) {
         } else if (devinfo->ver == 4 || devinfo->ver == 5) {
            }
      static void
   print_fault_data(struct intel_device_info *devinfo, uint32_t data1, uint32_t data0)
   {
               if (devinfo->ver < 8)
            address = ((uint64_t)(data0) << 12) | ((uint64_t)data1 & 0xf) << 44;
   printf("    Address 0x%016" PRIx64 " %s\n", address,
      }
      #define CSI "\e["
   #define NORMAL       CSI "0m"
      struct section {
      uint64_t gtt_offset;
   char *ring_name;
   const char *buffer_name;
   uint32_t *data;
   int dword_count;
      };
      #define MAX_SECTIONS 256
   static unsigned num_sections;
   static struct section sections[MAX_SECTIONS];
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
      static int qsort_hw_context_first(const void *a, const void *b)
   {
      const struct section *sa = a, *sb = b;
   if (strcmp(sa->buffer_name, "HW Context") == 0)
         if (strcmp(sb->buffer_name, "HW Context") == 0)
         else
      }
      static struct intel_batch_decode_bo
   get_intel_batch_bo(void *user_data, bool ppgtt, uint64_t address)
   {
      for (int s = 0; s < num_sections; s++) {
      if (sections[s].gtt_offset <= address &&
      address < sections[s].gtt_offset + sections[s].dword_count * 4) {
   return (struct intel_batch_decode_bo) {
      .addr = sections[s].gtt_offset,
   .map = sections[s].data,
                        }
      static void
   dump_shader_binary(void *user_data, const char *short_name,
               {
      char filename[128];
   snprintf(filename, sizeof(filename), "%s_0x%016"PRIx64".bin",
            FILE *f = fopen(filename, "w");
   if (f == NULL) {
      fprintf(stderr, "Unable to open %s\n", filename);
      }
   fwrite(data, data_length, 1, f);
      }
      static void
   read_data_file(FILE *file)
   {
      struct intel_spec *spec = NULL;
   long long unsigned fence;
   int matched;
   char *line = NULL;
   size_t line_size;
   uint32_t offset, value;
   uint32_t ring_head = UINT32_MAX, ring_tail = UINT32_MAX;
   bool ring_wraps = false;
   char *ring_name = NULL;
   struct intel_device_info devinfo;
   struct brw_isa_info isa;
            while (getline(&line, &line_size, file) > 0) {
      char *new_ring_name = NULL;
            if (sscanf(line, "%m[^ ] command stream\n", &new_ring_name) > 0) {
      free(ring_name);
               if (line[0] == ':' || line[0] == '~') {
      uint32_t *data = NULL;
   int dword_count = ascii85_decode(line+1, &data, line[0] == ':');
   if (dword_count == 0) {
      fprintf(stderr, "ASCII85 decode failed.\n");
      }
   assert(num_sections < MAX_SECTIONS);
   sections[num_sections].data = data;
   sections[num_sections].dword_count = dword_count;
   num_sections++;
               dashes = strstr(line, "---");
   if (dashes) {
      const struct {
      const char *match;
      } buffers[] = {
      { "ringbuffer", "ring buffer" },
   { "ring", "ring buffer" },
   { "gtt_offset", "batch buffer" },
   { "batch", "batch buffer" },
   { "hw context", "HW Context" },
   { "hw status", "HW status" },
   { "wa context", "WA context" },
   { "wa batchbuffer", "WA batch" },
   { "NULL context", "Kernel context" },
   { "user", "user" },
   { "semaphores", "semaphores", },
   { "guc log buffer", "GuC log", },
               free(ring_name);
   ring_name = malloc(dashes - line);
                  dashes += 4;
   for (b = buffers; b->match; b++) {
      if (strncasecmp(dashes, b->match, strlen(b->match)) == 0)
               assert(num_sections < MAX_SECTIONS);
                  uint32_t hi, lo;
   dashes = strchr(dashes, '=');
                              matched = sscanf(line, "%08x : %08x", &offset, &value);
   if (matched != 2) {
                              matched = sscanf(line, "PCI ID: 0x%04x\n", &reg);
   if (matched == 0)
         if (matched == 0) {
      const char *pci_id_start = strstr(line, "PCI ID");
   if (pci_id_start)
      }
   if (matched == 1) {
      if (!intel_get_device_info_from_pci_id(reg, &devinfo)) {
                                          if (xml_path == NULL)
         else
               matched = sscanf(line, "  CTL: 0x%08x\n", &reg);
   if (matched == 1) {
      print_register(spec,
                           matched = sscanf(line, "  HEAD: 0x%08x\n", &reg);
                                 matched = sscanf(line, "  ACTHD: 0x%08x\n", &reg);
   if (matched == 1) {
      print_register(spec,
                           matched = sscanf(line, "  ACTHD: 0x%08x %08x\n", &reg, &reg2);
                  matched = sscanf(line, "  ACTHD_LDW: 0x%08x\n", &reg);
                  matched = sscanf(line, "  ACTHD_UDW: 0x%08x\n", &reg);
                  matched = sscanf(line, "  PGTBL_ER: 0x%08x\n", &reg);
                  matched = sscanf(line, "  ERROR: 0x%08x\n", &reg);
   if (matched == 1 && reg) {
                  matched = sscanf(line, "  INSTDONE: 0x%08x\n", &reg);
   if (matched == 1) {
      const char *reg_name =
         if (reg_name)
               matched = sscanf(line, "  SC_INSTDONE: 0x%08x\n", &reg);
                  matched = sscanf(line, "  SC_INSTDONE_EXTRA: 0x%08x\n", &reg);
                  matched = sscanf(line, "  SC_INSTDONE_EXTRA2: 0x%08x\n", &reg);
                  matched = sscanf(line, "  SAMPLER_INSTDONE[%*d][%*d]: 0x%08x\n", &reg);
                  matched = sscanf(line, "  ROW_INSTDONE[%*d][%*d]: 0x%08x\n", &reg);
                  matched = sscanf(line, "  GEOM_SVGUNIT_INSTDONE[%*d][%*d]: 0x%08x\n", &reg);
                  matched = sscanf(line, "  INSTDONE1: 0x%08x\n", &reg);
                  matched = sscanf(line, "  fence[%i] = %Lx\n", &reg, &fence);
                  matched = sscanf(line, "  FAULT_REG: 0x%08x\n", &reg);
   if (matched == 1 && reg) {
      const char *reg_name =
      register_name_from_ring(fault_registers,
            if (reg_name == NULL)
                     matched = sscanf(line, "  FAULT_TLB_DATA: 0x%08x 0x%08x\n", &reg, &reg2);
                                 free(line);
            /*
   * Order sections so that the hardware context section is visited by the
   * decoder before other command buffers. This will allow the decoder to see
   * persistent state that was set before the current batch.
   */
            for (int s = 0; s < num_sections; s++) {
      if (strcmp(sections[s].buffer_name, "ring buffer") != 0)
         if (ring_head == UINT32_MAX) {
      ring_head = 0;
      }
   if (ring_tail == UINT32_MAX)
      ring_tail = (ring_head - sizeof(uint32_t)) %
      if (ring_head > ring_tail) {
      size_t total_size = sections[s].dword_count * sizeof(uint32_t) -
         size_t size1 = total_size - ring_tail;
   uint32_t *new_data = calloc(total_size, 1);
   memcpy(new_data, (uint8_t *)sections[s].data + ring_head, size1);
   memcpy((uint8_t *)new_data + size1, sections[s].data, ring_tail);
   free(sections[s].data);
   sections[s].data = new_data;
   ring_head = 0;
   ring_tail = total_size;
      }
   sections[s].data_offset = ring_head;
               for (int s = 0; s < num_sections; s++) {
      if (sections[s].dword_count * 4 > intel_debug_identifier_size() &&
      memcmp(sections[s].data, intel_debug_identifier(),
         const struct intel_debug_block_driver *driver_desc =
      intel_debug_get_identifier_block(sections[s].data,
            if (driver_desc) {
      printf("Driver identifier: %s\n",
      }
                  enum intel_batch_decode_flags batch_flags = 0;
   if (option_color == COLOR_ALWAYS)
         if (option_full_decode)
         if (option_print_offsets)
                  struct intel_batch_decode_ctx batch_ctx;
   intel_batch_decode_ctx_init(&batch_ctx, &isa, &devinfo, stdout,
                        if (option_dump_kernels)
            for (int s = 0; s < num_sections; s++) {
      enum intel_engine_class class;
            printf("--- %s (%s) at 0x%08x %08x\n",
         sections[s].buffer_name, sections[s].ring_name,
            bool is_ring_buffer = strcmp(sections[s].buffer_name, "ring buffer") == 0;
   if (option_print_all_bb || is_ring_buffer ||
      strcmp(sections[s].buffer_name, "batch buffer") == 0 ||
   strcmp(sections[s].buffer_name, "HW Context") == 0) {
   if (is_ring_buffer && ring_wraps)
         batch_ctx.engine = class;
   uint8_t *data = (uint8_t *)sections[s].data + sections[s].data_offset;
   uint64_t batch_addr = sections[s].gtt_offset + sections[s].data_offset;
   intel_print_batch(&batch_ctx, (uint32_t *)data,
                                       for (int s = 0; s < num_sections; s++) {
      free(sections[s].ring_name);
         }
      static void
   setup_pager(void)
   {
      int fds[2];
            if (!isatty(1))
            if (pipe(fds) == -1)
            pid = fork();
   if (pid == -1)
            if (pid == 0) {
      close(fds[1]);
   dup2(fds[0], 0);
               close(fds[0]);
   dup2(fds[1], 1);
      }
      static void
   print_help(const char *progname, FILE *file)
   {
      fprintf(file,
         "Usage: %s [OPTION]... [FILE]\n"
   "Parse an Intel GPU i915_error_state.\n"
   "With no FILE, debugfs-dri-directory is probed for in /debug and \n"
   "/sys/kernel/debug.  Otherwise, it may be specified. If a file is given,\n"
   "it is parsed as an GPU dump in the format of /debug/dri/0/i915_error_state.\n\n"
   "      --help          display this help and exit\n"
   "      --headers       decode only command headers\n"
   "      --color[=WHEN]  colorize the output; WHEN can be 'auto' (default\n"
   "                        if omitted), 'always', or 'never'\n"
   "      --no-pager      don't launch pager\n"
   "      --no-offsets    don't print instruction offsets\n"
   "      --xml=DIR       load hardware xml description from directory DIR\n"
   "      --all-bb        print out all batchbuffers\n"
      }
      static FILE *
   open_error_state_file(const char *path)
   {
      FILE *file;
            if (stat(path, &st))
            if (S_ISDIR(st.st_mode)) {
      ASSERTED int ret;
            ret = asprintf(&filename, "%s/i915_error_state", path);
   assert(ret > 0);
   file = fopen(filename, "r");
   free(filename);
   if (!file) {
      int minor;
   for (minor = 0; minor < 64; minor++) {
                     file = fopen(filename, "r");
   free(filename);
   if (file)
         }
   if (!file) {
      fprintf(stderr, "Failed to find i915_error_state beneath %s\n",
               } else {
      file = fopen(path, "r");
   if (!file) {
      fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
                     }
      int
   main(int argc, char *argv[])
   {
      FILE *file;
   int c, i;
   bool help = false, pager = true;
   const struct option aubinator_opts[] = {
      { "help",       no_argument,       (int *) &help,                 true },
   { "no-pager",   no_argument,       (int *) &pager,                false },
   { "no-offsets", no_argument,       (int *) &option_print_offsets, false },
   { "headers",    no_argument,       (int *) &option_full_decode,   false },
   { "color",      optional_argument, NULL,                          'c' },
   { "xml",        required_argument, NULL,                          'x' },
   { "all-bb",     no_argument,       (int *) &option_print_all_bb,  true },
   { "kernels",    no_argument,       (int *) &option_dump_kernels,  true },
               i = 0;
   while ((c = getopt_long(argc, argv, "", aubinator_opts, &i)) != -1) {
      switch (c) {
   case 'c':
      if (optarg == NULL || strcmp(optarg, "always") == 0)
         else if (strcmp(optarg, "never") == 0)
         else if (strcmp(optarg, "auto") == 0)
         else {
      fprintf(stderr, "invalid value for --color: %s", optarg);
      }
      case 'x':
      xml_path = strdup(optarg);
      case '?':
      print_help(argv[0], stderr);
      default:
                     if (help) {
      print_help(argv[0], stderr);
               if (optind >= argc) {
      if (isatty(0)) {
      file = open_error_state_file("/sys/class/drm/card0/error");
   if (!file)
                        if (file == NULL) {
      errx(1,
      "Couldn't find i915 debugfs directory.\n\n"
   "Is debugfs mounted? You might try mounting it with a command such as:\n\n"
      } else {
            } else {
      const char *path = argv[optind];
   if (strcmp(path, "-") == 0) {
         } else {
      file = open_error_state_file(path);
   if (file == NULL) {
      fprintf(stderr, "Error opening %s: %s\n", path, strerror(errno));
                     if (option_color == COLOR_AUTO)
            if (isatty(1) && pager)
            read_data_file(file);
            /* close the stdout which is opened to write the output */
   fflush(stdout);
   close(1);
            if (xml_path)
               }
      /* vim: set ts=8 sw=8 tw=0 cino=:0,(0 noet :*/
