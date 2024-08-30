   /*
   * Copyright © 2022 Igalia S.L.
   * SPDX-License-Identifier: MIT
   */
      #include <assert.h>
   #include <ctype.h>
   #include <err.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <getopt.h>
   #include <signal.h>
   #include <stdarg.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <sys/mman.h>
   #include <sys/stat.h>
   #include <sys/types.h>
   #include <sys/wait.h>
      #include "util/u_math.h"
      #include "adreno_common.xml.h"
   #include "adreno_pm4.xml.h"
   #include "freedreno_pm4.h"
      #include "a6xx.xml.h"
   #include "common/freedreno_dev_info.h"
      #include "util/hash_table.h"
   #include "util/os_time.h"
   #include "util/ralloc.h"
   #include "util/rb_tree.h"
   #include "util/set.h"
   #include "util/u_vector.h"
   #include "buffers.h"
   #include "cffdec.h"
   #include "disasm.h"
   #include "io.h"
   #include "rdutil.h"
   #include "redump.h"
   #include "rnnutil.h"
      /* Decompiles a single cmdstream from .rd into compilable C source.
   * C source could be compiled using rdcompiler-meson.build as an example.
   *
   * For how-to see freedreno.rst
   */
      static int handle_file(const char *filename, uint32_t submit_to_decompile);
      static const char *levels[] = {
      "\t",
   "\t\t",
   "\t\t\t",
   "\t\t\t\t",
   "\t\t\t\t\t",
   "\t\t\t\t\t\t",
   "\t\t\t\t\t\t\t",
   "\t\t\t\t\t\t\t\t",
      };
      static void
   printlvl(int lvl, const char *fmt, ...)
   {
               va_list args;
   va_start(args, fmt);
   fputs(levels[lvl], stdout);
   vprintf(fmt, args);
      }
      static void
   print_usage(const char *name)
   {
      /* clang-format off */
   fprintf(stderr, "Usage:\n\n"
         "\t%s [OPTSIONS]... FILE...\n\n"
   "Options:\n"
   "\t-s, --submit=№   - № of the submit to decompile\n"
   "\t--no-reg-bunch   - Use pkt4 for each reg in CP_CONTEXT_REG_BUNCH\n"
   "\t-h, --help       - show this message\n"
   /* clang-format on */
      }
      struct decompiler_options {
         };
      static struct decompiler_options options = {};
      /* clang-format off */
   static const struct option opts[] = {
         { "submit",       required_argument,   0, 's' },
   { "no-reg-bunch", no_argument,         &options.no_reg_bunch, 1 },
   };
   /* clang-format on */
      int
   main(int argc, char **argv)
   {
      int ret = -1;
   int c;
            while ((c = getopt_long(argc, argv, "s:h", opts, NULL)) != -1) {
      switch (c) {
   case 0:
      /* option that set a flag, nothing to do */
      case 's':
      submit_to_decompile = strtoul(optarg, NULL, 0);
      case 'h':
   default:
                     if (submit_to_decompile == -1) {
      fprintf(stderr, "Submit to decompile has to be specified\n");
               while (optind < argc) {
      ret = handle_file(argv[optind], submit_to_decompile);
   if (ret) {
      fprintf(stderr, "error reading: %s\n", argv[optind]);
      }
               if (ret)
               }
      static struct rnn *rnn;
   static struct fd_dev_id dev_id;
   static void *mem_ctx;
      static struct set decompiled_shaders;
      static void
   init_rnn(const char *gpuname)
   {
      rnn = rnn_new(true);
      }
      const char *
   pktname(unsigned opc)
   {
         }
      static uint32_t
   decompile_shader(const char *name, uint32_t regbase, uint32_t *dwords, int level)
   {
      uint64_t gpuaddr = ((uint64_t)dwords[1] << 32) | dwords[0];
            /* Shader's iova is referenced in two places, so we have to remember it. */
   if (_mesa_set_search(&decompiled_shaders, &gpuaddr)) {
         } else {
      uint64_t *key = ralloc(mem_ctx, uint64_t);
   *key = gpuaddr;
            void *buf = hostptr(gpuaddr);
                     char *stream_data = NULL;
   size_t stream_size = 0;
            try_disasm_a3xx(buf, sizedwords, 0, stream, fd_dev_gen(&dev_id) * 100);
            printlvl(level, "{\n");
   printlvl(level + 1, "const char *source = R\"(\n");
   printf("%s", stream_data);
   printlvl(level + 1, ")\";\n");
   printlvl(level + 1, "upload_shader(&ctx, 0x%" PRIx64 ", source);\n", gpuaddr);
   printlvl(level + 1, "emit_shader_iova(&ctx, cs, 0x%" PRIx64 ");\n", gpuaddr);
   printlvl(level, "}\n");
                  }
      static struct {
      uint32_t regbase;
      } reg_a6xx[] = {
      {REG_A6XX_SP_VS_OBJ_START, decompile_shader},
   {REG_A6XX_SP_HS_OBJ_START, decompile_shader},
   {REG_A6XX_SP_DS_OBJ_START, decompile_shader},
   {REG_A6XX_SP_GS_OBJ_START, decompile_shader},
   {REG_A6XX_SP_FS_OBJ_START, decompile_shader},
               }, *type0_reg;
      static uint32_t
   decompile_register(uint32_t regbase, uint32_t *dwords, uint16_t cnt, int level)
   {
               for (unsigned idx = 0; type0_reg[idx].regbase; idx++) {
      if (type0_reg[idx].regbase == regbase) {
                              if (info && info->typeinfo) {
      char *decoded = rnndec_decodeval(rnn->vc, info->typeinfo, dword);
            if (cnt == 0) {
         #if 0
            char reg_name[33];
                  /* reginfo doesn't return reg name in a compilable format, for now just
   * parse it into a compilable reg name.
   * TODO: Make RNN optionally return compilable reg name.
   */
   if (sscanf(info->name, "%32[A-Z0-6_][%32[x0-9]].%32s", reg_name,
            printlvl(level, "pkt4(cs, REG_%s_%s, (%u), %u);\n", rnn->variant,
      } else {
      printlvl(level, "pkt4(cs, REG_%s_%s_%s(%s), (%u), %u);\n",
   #else
            /* TODO: We don't have easy way to get chip generation prefix,
   * so just emit raw packet offset as a workaround.
      #endif
            } else {
      printlvl(level, "/* unknown pkt4 */\n");
                           }
      static uint32_t
   decompile_register_reg_bunch(uint32_t regbase, uint32_t *dwords, uint16_t cnt, int level)
   {
      struct rnndecaddrinfo *info = rnn_reginfo(rnn, regbase);
            if (info && info->typeinfo) {
      char *decoded = rnndec_decodeval(rnn->vc, info->typeinfo, dword);
      } else {
                  printlvl(level, "pkt(cs, 0x%04x);\n", regbase);
                        }
      static void
   decompile_registers(uint32_t regbase, uint32_t *dwords, uint32_t sizedwords,
         {
      if (!sizedwords)
         uint32_t consumed = decompile_register(regbase, dwords, sizedwords, level);
   sizedwords -= consumed;
   while (sizedwords > 0) {
      regbase += consumed;
   dwords += consumed;
   consumed = decompile_register(regbase, dwords, 0, level);
         }
      static void
   decompile_domain(uint32_t pkt, uint32_t *dwords, uint32_t sizedwords,
         {
      struct rnndomain *dom;
                              if (pkt == CP_LOAD_STATE6_FRAG || pkt == CP_LOAD_STATE6_GEOM) {
      enum a6xx_state_type state_type =
      (dwords[0] & CP_LOAD_STATE6_0_STATE_TYPE__MASK) >>
      enum a6xx_state_src state_src =
                  /* TODO: decompile all other state */
   if (state_type == ST6_SHADER && state_src == SS6_INDIRECT) {
      printlvl(level, "pkt(cs, %u);\n", dwords[0]);
   decompile_shader(NULL, 0, dwords + 1, level);
                  for (i = 0; i < sizedwords; i++) {
      struct rnndecaddrinfo *info = NULL;
   if (dom) {
                  char *decoded;
   if (!(info && info->typeinfo)) {
      printlvl(level, "pkt(cs, %u);\n", dwords[i]);
      }
   uint64_t value = dwords[i];
   bool reg64 = info->typeinfo->high >= 32 && i < sizedwords - 1;
   if (reg64) {
         }
            printlvl(level, "/* %s */\n", decoded);
   printlvl(level, "pkt(cs, %u);\n", dwords[i]);
   if (reg64) {
      printlvl(level, "pkt(cs, %u);\n", dwords[i + 1]);
               free(decoded);
   free(info->name);
         }
      static void
   decompile_commands(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      int dwords_left = sizedwords;
   uint32_t count = 0; /* dword count including packet header */
            if (!dwords) {
      fprintf(stderr, "NULL cmd buffer!\n");
               while (dwords_left > 0) {
      if (pkt_is_regwrite(dwords[0], &val, &count)) {
      assert(val < 0xffff);
      } else if (pkt_is_opcode(dwords[0], &val, &count)) {
      if (val == CP_INDIRECT_BUFFER) {
      uint64_t ibaddr;
   uint32_t ibsize;
   ibaddr = dwords[1];
                                                printlvl(level + 1, "end_ib();\n");
      } else if (val == CP_SET_DRAW_STATE) {
      for (int i = 1; i < count; i += 3) {
      uint32_t state_count = dwords[i] & 0xffff;
   if (state_count != 0) {
                                                         printlvl(level + 1, "end_draw_state(%u);\n", unchanged);
      } else {
      decompile_domain(val, dwords + i, 3, "CP_SET_DRAW_STATE",
            } else if (val == CP_CONTEXT_REG_BUNCH || val == CP_CONTEXT_REG_BUNCH2) {
                     if (val == CP_CONTEXT_REG_BUNCH2) {
      if (options.no_reg_bunch) {
      printlvl(level, "// CP_CONTEXT_REG_BUNCH2\n");
      } else {
      printlvl(level, "pkt7(cs, %s, %u);\n", "CP_CONTEXT_REG_BUNCH2", cnt);
   printlvl(level, "{\n");
                     dw += 2;
      } else {
      if (options.no_reg_bunch) {
      printlvl(level, "// CP_CONTEXT_REG_BUNCH\n");
      } else {
      printlvl(level, "pkt7(cs, %s, %u);\n", "CP_CONTEXT_REG_BUNCH", cnt);
                  for (uint32_t i = 0; i < cnt; i += 2) {
      if (options.no_reg_bunch) {
         } else {
            }
      } else {
      const char *packet_name = pktname(val);
   const char *dom_name = packet_name;
   if (packet_name) {
      /* special hack for two packets that decode the same way
   * on a6xx:
   */
   if (!strcmp(packet_name, "CP_LOAD_STATE6_FRAG") ||
      !strcmp(packet_name, "CP_LOAD_STATE6_GEOM"))
      decompile_domain(val, dwords + 1, count - 1, dom_name, packet_name,
      } else {
               } else {
                  dwords += count;
               if (dwords_left < 0)
      }
      static void
   emit_header()
   {
      if (!dev_id.gpu_id && !dev_id.chip_id)
            static bool emitted = false;
   if (emitted)
                  switch (fd_dev_gen(&dev_id)) {
   case 6:
      init_rnn("a6xx");
      case 7:
      init_rnn("a7xx");
      default:
                  printf("#include \"decode/rdcompiler-utils.h\"\n"
         "int main(int argc, char **argv)\n"
   "{\n"
   "\tstruct replay_context ctx;\n"
   "\tstruct fd_dev_id dev_id = {%u, 0x%" PRIx64 "};\n"
   "\treplay_context_init(&ctx, &dev_id, argc, argv);\n"
      }
      static inline uint32_t
   u64_hash(const void *key)
   {
         }
      static inline bool
   u64_compare(const void *key1, const void *key2)
   {
         }
      static int
   handle_file(const char *filename, uint32_t submit_to_decompile)
   {
      struct io *io;
   int submit = 0;
   bool needs_reset = false;
            if (!strcmp(filename, "-"))
         else
            if (!io) {
      fprintf(stderr, "could not open: %s\n", filename);
               type0_reg = reg_a6xx;
   mem_ctx = ralloc_context(NULL);
            struct {
      unsigned int len;
               while (parse_rd_section(io, &ps)) {
      switch (ps.type) {
   case RD_TEST:
   case RD_VERT_SHADER:
   case RD_FRAG_SHADER:
   case RD_CMD:
      /* no-op */
      case RD_GPUADDR:
      if (needs_reset) {
      reset_buffers();
               parse_addr(ps.buf, ps.sz, &gpuaddr.len, &gpuaddr.gpuaddr);
      case RD_BUFFER_CONTENTS:
      add_buffer(gpuaddr.gpuaddr, gpuaddr.len, ps.buf);
   ps.buf = NULL;
      case RD_CMDSTREAM_ADDR: {
      unsigned int sizedwords;
                  if (submit == submit_to_decompile) {
                  submit++;
      }
   case RD_GPU_ID: {
      dev_id.gpu_id = parse_gpu_id(ps.buf);
   if (fd_dev_info(&dev_id))
            }
   case RD_CHIP_ID: {
      dev_id.chip_id = parse_chip_id(ps.buf);
   if (fd_dev_info(&dev_id))
            }
   default:
                              io_close(io);
            if (ps.ret < 0) {
         }
      }
