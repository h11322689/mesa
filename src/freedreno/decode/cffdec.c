   /*
   * Copyright (c) 2012 Rob Clark <robdclark@gmail.com>
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
   #include <ctype.h>
   #include <err.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <inttypes.h>
   #include <signal.h>
   #include <stdarg.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <sys/stat.h>
   #include <sys/types.h>
   #include <sys/wait.h>
      #include "freedreno_pm4.h"
      #include "buffers.h"
   #include "cffdec.h"
   #include "disasm.h"
   #include "redump.h"
   #include "rnnutil.h"
   #include "script.h"
      /* ************************************************************************* */
   /* originally based on kernel recovery dump code: */
      static const struct cffdec_options *options;
      static bool needs_wfi = false;
   static bool summary = false;
   static bool in_summary = false;
   static int vertices;
      static inline unsigned
   regcnt(void)
   {
      if (options->info->chip >= 5)
         else
      }
      static int
   is_64b(void)
   {
         }
      static int draws[4];
   static struct {
      uint64_t base;
   uint32_t size; /* in dwords */
   /* Generally cmdstream consists of multiple IB calls to different
   * buffers, which are themselves often re-used for each tile.  The
   * triggered flag serves two purposes to help make it more clear
   * what part of the cmdstream is before vs after the the GPU hang:
   *
   * 1) if in IB2 we are passed the point within the IB2 buffer where
   *    the GPU hung, but IB1 is not passed the point within its
   *    buffer where the GPU had hung, then we know the GPU hang
   *    happens on a future use of that IB2 buffer.
   *
   * 2) if in an IB1 or IB2 buffer that is not the one where the GPU
   *    hung, but we've already passed the trigger point at the same
   *    IB level, we know that we are passed the point where the GPU
   *    had hung.
   *
   * So this is a one way switch, false->true.  And a higher #'d
   * IB level isn't considered triggered unless the lower #'d IB
   * level is.
   */
   bool triggered : 1;
      } ibs[4];
   static int ib;
      static int draw_count;
   static int current_draw_count;
      /* query mode.. to handle symbolic register name queries, we need to
   * defer parsing query string until after gpu_id is know and rnn db
   * loaded:
   */
   static int *queryvals;
      static bool
   quiet(int lvl)
   {
      if ((options->draw_filter != -1) &&
      (options->draw_filter != current_draw_count))
      if ((lvl >= 3) && (summary || options->querystrs || options->script))
         if ((lvl >= 2) && (options->querystrs || options->script))
            }
      void
   printl(int lvl, const char *fmt, ...)
   {
      va_list args;
   if (quiet(lvl))
         va_start(args, fmt);
   vprintf(fmt, args);
      }
      static const char *levels[] = {
      "\t",
   "\t\t",
   "\t\t\t",
   "\t\t\t\t",
   "\t\t\t\t\t",
   "\t\t\t\t\t\t",
   "\t\t\t\t\t\t\t",
   "\t\t\t\t\t\t\t\t",
   "\t\t\t\t\t\t\t\t\t",
   "x",
   "x",
   "x",
   "x",
   "x",
      };
      enum state_src_t {
      STATE_SRC_DIRECT,
   STATE_SRC_INDIRECT,
      };
      /* SDS (CP_SET_DRAW_STATE) helpers: */
   static void load_all_groups(int level);
   static void disable_all_groups(void);
      static void dump_tex_samp(uint32_t *texsamp, enum state_src_t src, int num_unit,
         static void dump_tex_const(uint32_t *texsamp, int num_unit, int level);
      static bool
   highlight_gpuaddr(uint64_t gpuaddr)
   {
      if (!options->ibs[ib].base)
            if ((ib > 0) && options->ibs[ib - 1].base &&
      !(ibs[ib - 1].triggered || ibs[ib - 1].base_seen))
         if (ibs[ib].base_seen)
            if (ibs[ib].triggered)
            if (options->ibs[ib].base != ibs[ib].base)
            uint64_t start = ibs[ib].base + 4 * (ibs[ib].size - options->ibs[ib].rem);
                     if (triggered && (ib < 2) && options->ibs[ib + 1].crash_found) {
      ibs[ib].base_seen = true;
                        if (triggered)
               }
      static void
   dump_hex(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      int i, j;
            if (quiet(2))
                     for (i = 0; i < sizedwords; i += 8) {
               /* always show first row: */
   if (i == 0)
            for (j = 0; (j < 8) && (i + j < sizedwords) && zero; j++)
                  if (zero && !lastzero)
                     if (zero)
                     if (highlight)
            if (is_64b()) {
         } else {
                  if (highlight)
                     for (j = 0; (j < 8) && (i + j < sizedwords); j++) {
                        }
      static void
   dump_float(float *dwords, uint32_t sizedwords, int level)
   {
      int i;
   for (i = 0; i < sizedwords; i++) {
      if ((i % 8) == 0) {
      if (is_64b()) {
         } else {
            } else {
         }
   printf("%8f", *(dwords++));
   if ((i % 8) == 7)
      }
   if (i % 8)
      }
      /* I believe the surface format is low bits:
   #define RB_COLOR_INFO__COLOR_FORMAT_MASK                   0x0000000fL
   comments in sys2gmem_tex_const indicate that address is [31:12], but
   looks like at least some of the bits above the format have different meaning..
   */
   static void
   parse_dword_addr(uint32_t dword, uint32_t *gpuaddr, uint32_t *flags,
         {
      assert(!is_64b()); /* this is only used on a2xx */
   *gpuaddr = dword & ~mask;
      }
      static uint32_t type0_reg_vals[0xffff + 1];
   static uint8_t type0_reg_rewritten[sizeof(type0_reg_vals) /
         static uint8_t type0_reg_written[sizeof(type0_reg_vals) / 8];
   static uint32_t lastvals[ARRAY_SIZE(type0_reg_vals)];
      static bool
   reg_rewritten(uint32_t regbase)
   {
         }
      bool
   reg_written(uint32_t regbase)
   {
         }
      static void
   clear_rewritten(void)
   {
         }
      static void
   clear_written(void)
   {
      memset(type0_reg_written, 0, sizeof(type0_reg_written));
      }
      uint32_t
   reg_lastval(uint32_t regbase)
   {
         }
      static void
   clear_lastvals(void)
   {
         }
      uint32_t
   reg_val(uint32_t regbase)
   {
         }
      void
   reg_set(uint32_t regbase, uint32_t val)
   {
      assert(regbase < regcnt());
   type0_reg_vals[regbase] = val;
   type0_reg_written[regbase / 8] |= (1 << (regbase % 8));
      }
      static void
   reg_dump_scratch(const char *name, uint32_t dword, int level)
   {
               if (quiet(3))
                     // if not, try old a2xx/a3xx version:
   if (!r)
            if (!r)
            printf("%s:%u,%u,%u,%u\n", levels[level], reg_val(r + 4), reg_val(r + 5),
      }
      static void
   dump_gpuaddr_size(uint64_t gpuaddr, int level, int sizedwords, int quietlvl)
   {
               if (quiet(quietlvl))
            buf = hostptr(gpuaddr);
   if (buf) {
            }
      static void
   dump_gpuaddr(uint64_t gpuaddr, int level)
   {
         }
      static void
   reg_dump_gpuaddr(const char *name, uint32_t dword, int level)
   {
         }
      uint32_t gpuaddr_lo;
   static void
   reg_gpuaddr_lo(const char *name, uint32_t dword, int level)
   {
         }
      static void
   reg_dump_gpuaddr_hi(const char *name, uint32_t dword, int level)
   {
         }
      static void
   reg_dump_gpuaddr64(const char *name, uint64_t qword, int level)
   {
         }
      static void
   dump_shader(const char *ext, void *buf, int bufsz)
   {
      if (options->dump_shaders) {
      static int n = 0;
   char filename[16];
   int fd;
   sprintf(filename, "%04d.%s", n++, ext);
   fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
   if (fd != -1) {
      write(fd, buf, bufsz);
            }
      static void
   disasm_gpuaddr(const char *name, uint64_t gpuaddr, int level)
   {
                        if (quiet(3))
            buf = hostptr(gpuaddr);
   if (buf) {
      uint32_t sizedwords = hostlen(gpuaddr) / 4;
            dump_hex(buf, MIN2(64, sizedwords), level + 1);
            /* this is a bit ugly way, but oh well.. */
   if (strstr(name, "SP_VS_OBJ")) {
         } else if (strstr(name, "SP_FS_OBJ")) {
         } else if (strstr(name, "SP_GS_OBJ")) {
         } else if (strstr(name, "SP_CS_OBJ")) {
         } else {
                  if (ext)
         }
      static void
   reg_disasm_gpuaddr(const char *name, uint32_t dword, int level)
   {
         }
      static void
   reg_disasm_gpuaddr_hi(const char *name, uint32_t dword, int level)
   {
         }
      static void
   reg_disasm_gpuaddr64(const char *name, uint64_t qword, int level)
   {
         }
      /* Find the value of the TEX_COUNT register that corresponds to the named
   * TEX_SAMP/TEX_CONST reg.
   *
   * Note, this kinda assumes an equal # of samplers and textures, but not
   * really sure if there is a much better option.  I suppose on a6xx we
   * could instead decode the bitfields in SP_xS_CONFIG
   */
   static int
   get_tex_count(const char *name)
   {
      char count_reg[strlen(name) + 5];
            p = strstr(name, "CONST");
   if (!p)
         if (!p)
            int n = p - name;
   strncpy(count_reg, name, n);
               }
      static void
   reg_dump_tex_samp_hi(const char *name, uint32_t dword, int level)
   {
      if (!in_summary)
            int num_unit = get_tex_count(name);
   uint64_t gpuaddr = gpuaddr_lo | (((uint64_t)dword) << 32);
            if (!buf)
               }
      static void
   reg_dump_tex_const_hi(const char *name, uint32_t dword, int level)
   {
      if (!in_summary)
            int num_unit = get_tex_count(name);
   uint64_t gpuaddr = gpuaddr_lo | (((uint64_t)dword) << 32);
            if (!buf)
               }
      /*
   * Registers with special handling (rnndec_decode() handles rest):
   */
   #define REG(x, fxn)    { #x, fxn }
   #define REG64(x, fxn)  { #x, .fxn64 = fxn, .is_reg64 = true }
   static struct {
      const char *regname;
   void (*fxn)(const char *name, uint32_t dword, int level);
   void (*fxn64)(const char *name, uint64_t qword, int level);
   uint32_t regbase;
      } reg_a2xx[] = {
         REG(CP_SCRATCH_REG0, reg_dump_scratch),
   REG(CP_SCRATCH_REG1, reg_dump_scratch),
   REG(CP_SCRATCH_REG2, reg_dump_scratch),
   REG(CP_SCRATCH_REG3, reg_dump_scratch),
   REG(CP_SCRATCH_REG4, reg_dump_scratch),
   REG(CP_SCRATCH_REG5, reg_dump_scratch),
   REG(CP_SCRATCH_REG6, reg_dump_scratch),
   REG(CP_SCRATCH_REG7, reg_dump_scratch),
   }, reg_a3xx[] = {
         REG(CP_SCRATCH_REG0, reg_dump_scratch),
   REG(CP_SCRATCH_REG1, reg_dump_scratch),
   REG(CP_SCRATCH_REG2, reg_dump_scratch),
   REG(CP_SCRATCH_REG3, reg_dump_scratch),
   REG(CP_SCRATCH_REG4, reg_dump_scratch),
   REG(CP_SCRATCH_REG5, reg_dump_scratch),
   REG(CP_SCRATCH_REG6, reg_dump_scratch),
   REG(CP_SCRATCH_REG7, reg_dump_scratch),
   REG(VSC_SIZE_ADDRESS, reg_dump_gpuaddr),
   REG(SP_VS_PVT_MEM_ADDR_REG, reg_dump_gpuaddr),
   REG(SP_FS_PVT_MEM_ADDR_REG, reg_dump_gpuaddr),
   REG(SP_VS_OBJ_START_REG, reg_disasm_gpuaddr),
   REG(SP_FS_OBJ_START_REG, reg_disasm_gpuaddr),
   REG(TPL1_TP_FS_BORDER_COLOR_BASE_ADDR, reg_dump_gpuaddr),
   }, reg_a4xx[] = {
         REG(CP_SCRATCH[0].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x1].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x2].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x3].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x4].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x5].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x6].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x7].REG, reg_dump_scratch),
   REG(SP_VS_PVT_MEM_ADDR, reg_dump_gpuaddr),
   REG(SP_FS_PVT_MEM_ADDR, reg_dump_gpuaddr),
   REG(SP_GS_PVT_MEM_ADDR, reg_dump_gpuaddr),
   REG(SP_HS_PVT_MEM_ADDR, reg_dump_gpuaddr),
   REG(SP_DS_PVT_MEM_ADDR, reg_dump_gpuaddr),
   REG(SP_CS_PVT_MEM_ADDR, reg_dump_gpuaddr),
   REG(SP_VS_OBJ_START, reg_disasm_gpuaddr),
   REG(SP_FS_OBJ_START, reg_disasm_gpuaddr),
   REG(SP_GS_OBJ_START, reg_disasm_gpuaddr),
   REG(SP_HS_OBJ_START, reg_disasm_gpuaddr),
   REG(SP_DS_OBJ_START, reg_disasm_gpuaddr),
   REG(SP_CS_OBJ_START, reg_disasm_gpuaddr),
   REG(TPL1_TP_VS_BORDER_COLOR_BASE_ADDR, reg_dump_gpuaddr),
   REG(TPL1_TP_HS_BORDER_COLOR_BASE_ADDR, reg_dump_gpuaddr),
   REG(TPL1_TP_DS_BORDER_COLOR_BASE_ADDR, reg_dump_gpuaddr),
   REG(TPL1_TP_GS_BORDER_COLOR_BASE_ADDR, reg_dump_gpuaddr),
   REG(TPL1_TP_FS_BORDER_COLOR_BASE_ADDR, reg_dump_gpuaddr),
   }, reg_a5xx[] = {
         REG(CP_SCRATCH[0x4].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x5].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x6].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x7].REG, reg_dump_scratch),
   REG(SP_VS_OBJ_START_LO, reg_gpuaddr_lo),
   REG(SP_VS_OBJ_START_HI, reg_disasm_gpuaddr_hi),
   REG(SP_HS_OBJ_START_LO, reg_gpuaddr_lo),
   REG(SP_HS_OBJ_START_HI, reg_disasm_gpuaddr_hi),
   REG(SP_DS_OBJ_START_LO, reg_gpuaddr_lo),
   REG(SP_DS_OBJ_START_HI, reg_disasm_gpuaddr_hi),
   REG(SP_GS_OBJ_START_LO, reg_gpuaddr_lo),
   REG(SP_GS_OBJ_START_HI, reg_disasm_gpuaddr_hi),
   REG(SP_FS_OBJ_START_LO, reg_gpuaddr_lo),
   REG(SP_FS_OBJ_START_HI, reg_disasm_gpuaddr_hi),
   REG(SP_CS_OBJ_START_LO, reg_gpuaddr_lo),
   REG(SP_CS_OBJ_START_HI, reg_disasm_gpuaddr_hi),
   REG(TPL1_VS_TEX_CONST_LO, reg_gpuaddr_lo),
   REG(TPL1_VS_TEX_CONST_HI, reg_dump_tex_const_hi),
   REG(TPL1_VS_TEX_SAMP_LO, reg_gpuaddr_lo),
   REG(TPL1_VS_TEX_SAMP_HI, reg_dump_tex_samp_hi),
   REG(TPL1_HS_TEX_CONST_LO, reg_gpuaddr_lo),
   REG(TPL1_HS_TEX_CONST_HI, reg_dump_tex_const_hi),
   REG(TPL1_HS_TEX_SAMP_LO, reg_gpuaddr_lo),
   REG(TPL1_HS_TEX_SAMP_HI, reg_dump_tex_samp_hi),
   REG(TPL1_DS_TEX_CONST_LO, reg_gpuaddr_lo),
   REG(TPL1_DS_TEX_CONST_HI, reg_dump_tex_const_hi),
   REG(TPL1_DS_TEX_SAMP_LO, reg_gpuaddr_lo),
   REG(TPL1_DS_TEX_SAMP_HI, reg_dump_tex_samp_hi),
   REG(TPL1_GS_TEX_CONST_LO, reg_gpuaddr_lo),
   REG(TPL1_GS_TEX_CONST_HI, reg_dump_tex_const_hi),
   REG(TPL1_GS_TEX_SAMP_LO, reg_gpuaddr_lo),
   REG(TPL1_GS_TEX_SAMP_HI, reg_dump_tex_samp_hi),
   REG(TPL1_FS_TEX_CONST_LO, reg_gpuaddr_lo),
   REG(TPL1_FS_TEX_CONST_HI, reg_dump_tex_const_hi),
   REG(TPL1_FS_TEX_SAMP_LO, reg_gpuaddr_lo),
   REG(TPL1_FS_TEX_SAMP_HI, reg_dump_tex_samp_hi),
   REG(TPL1_CS_TEX_CONST_LO, reg_gpuaddr_lo),
   REG(TPL1_CS_TEX_CONST_HI, reg_dump_tex_const_hi),
   REG(TPL1_CS_TEX_SAMP_LO, reg_gpuaddr_lo),
   REG(TPL1_CS_TEX_SAMP_HI, reg_dump_tex_samp_hi),
   REG(TPL1_TP_BORDER_COLOR_BASE_ADDR_LO, reg_gpuaddr_lo),
   //      REG(RB_MRT_FLAG_BUFFER[0].ADDR_LO, reg_gpuaddr_lo),
   //      REG(RB_MRT_FLAG_BUFFER[0].ADDR_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_MRT_FLAG_BUFFER[1].ADDR_LO, reg_gpuaddr_lo),
   //      REG(RB_MRT_FLAG_BUFFER[1].ADDR_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_MRT_FLAG_BUFFER[2].ADDR_LO, reg_gpuaddr_lo),
   //      REG(RB_MRT_FLAG_BUFFER[2].ADDR_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_MRT_FLAG_BUFFER[3].ADDR_LO, reg_gpuaddr_lo),
   //      REG(RB_MRT_FLAG_BUFFER[3].ADDR_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_MRT_FLAG_BUFFER[4].ADDR_LO, reg_gpuaddr_lo),
   //      REG(RB_MRT_FLAG_BUFFER[4].ADDR_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_MRT_FLAG_BUFFER[5].ADDR_LO, reg_gpuaddr_lo),
   //      REG(RB_MRT_FLAG_BUFFER[5].ADDR_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_MRT_FLAG_BUFFER[6].ADDR_LO, reg_gpuaddr_lo),
   //      REG(RB_MRT_FLAG_BUFFER[6].ADDR_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_MRT_FLAG_BUFFER[7].ADDR_LO, reg_gpuaddr_lo),
   //      REG(RB_MRT_FLAG_BUFFER[7].ADDR_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_BLIT_FLAG_DST_LO, reg_gpuaddr_lo),
   //      REG(RB_BLIT_FLAG_DST_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_MRT[0].BASE_LO, reg_gpuaddr_lo),
   //      REG(RB_MRT[0].BASE_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_DEPTH_BUFFER_BASE_LO, reg_gpuaddr_lo),
   //      REG(RB_DEPTH_BUFFER_BASE_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_DEPTH_FLAG_BUFFER_BASE_LO, reg_gpuaddr_lo),
   //      REG(RB_DEPTH_FLAG_BUFFER_BASE_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_BLIT_DST_LO, reg_gpuaddr_lo),
   //      REG(RB_BLIT_DST_HI, reg_dump_gpuaddr_hi),
      //      REG(RB_2D_SRC_LO, reg_gpuaddr_lo),
   //      REG(RB_2D_SRC_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_2D_SRC_FLAGS_LO, reg_gpuaddr_lo),
   //      REG(RB_2D_SRC_FLAGS_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_2D_DST_LO, reg_gpuaddr_lo),
   //      REG(RB_2D_DST_HI, reg_dump_gpuaddr_hi),
   //      REG(RB_2D_DST_FLAGS_LO, reg_gpuaddr_lo),
   //      REG(RB_2D_DST_FLAGS_HI, reg_dump_gpuaddr_hi),
            }, reg_a6xx[] = {
         REG(CP_SCRATCH[0x4].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x5].REG, reg_dump_scratch),
   REG(CP_SCRATCH[0x6].REG, reg_dump_scratch),
            REG64(SP_VS_OBJ_START, reg_disasm_gpuaddr64),
   REG64(SP_HS_OBJ_START, reg_disasm_gpuaddr64),
   REG64(SP_DS_OBJ_START, reg_disasm_gpuaddr64),
   REG64(SP_GS_OBJ_START, reg_disasm_gpuaddr64),
   REG64(SP_FS_OBJ_START, reg_disasm_gpuaddr64),
            REG64(SP_VS_TEX_CONST, reg_dump_gpuaddr64),
   REG64(SP_VS_TEX_SAMP, reg_dump_gpuaddr64),
   REG64(SP_HS_TEX_CONST, reg_dump_gpuaddr64),
   REG64(SP_HS_TEX_SAMP, reg_dump_gpuaddr64),
   REG64(SP_DS_TEX_CONST, reg_dump_gpuaddr64),
   REG64(SP_DS_TEX_SAMP, reg_dump_gpuaddr64),
   REG64(SP_GS_TEX_CONST, reg_dump_gpuaddr64),
   REG64(SP_GS_TEX_SAMP, reg_dump_gpuaddr64),
   REG64(SP_FS_TEX_CONST, reg_dump_gpuaddr64),
   REG64(SP_FS_TEX_SAMP, reg_dump_gpuaddr64),
   REG64(SP_CS_TEX_CONST, reg_dump_gpuaddr64),
            }, reg_a7xx[] = {
         REG64(SP_VS_OBJ_START, reg_disasm_gpuaddr64),
   REG64(SP_HS_OBJ_START, reg_disasm_gpuaddr64),
   REG64(SP_DS_OBJ_START, reg_disasm_gpuaddr64),
   REG64(SP_GS_OBJ_START, reg_disasm_gpuaddr64),
   REG64(SP_FS_OBJ_START, reg_disasm_gpuaddr64),
            }, *type0_reg;
      static struct rnn *rnn;
      static void
   init_rnn(const char *gpuname)
   {
                        if (options->querystrs) {
      int i;
            for (i = 0; i < options->nquery; i++) {
                              queryvals[i] = val;
                  for (unsigned idx = 0; type0_reg[idx].regname; idx++) {
      type0_reg[idx].regbase = regbase(type0_reg[idx].regname);
   if (!type0_reg[idx].regbase) {
      printf("invalid register name: %s\n", type0_reg[idx].regname);
            }
      void
   reset_regs(void)
   {
      clear_written();
   clear_lastvals();
      }
      void
   cffdec_init(const struct cffdec_options *_options)
   {
      options = _options;
            /* in case we're decoding multiple files: */
   free(queryvals);
   reset_regs();
            if (!options->info)
            switch (options->info->chip) {
   case 2:
      type0_reg = reg_a2xx;
   init_rnn("a2xx");
      case 3:
      type0_reg = reg_a3xx;
   init_rnn("a3xx");
      case 4:
      type0_reg = reg_a4xx;
   init_rnn("a4xx");
      case 5:
      type0_reg = reg_a5xx;
   init_rnn("a5xx");
      case 6:
      type0_reg = reg_a6xx;
   init_rnn("a6xx");
      case 7:
      type0_reg = reg_a7xx;
   init_rnn("a7xx");
      default:
            }
      const char *
   pktname(unsigned opc)
   {
         }
      const char *
   regname(uint32_t regbase, int color)
   {
         }
      uint32_t
   regbase(const char *name)
   {
         }
      static int
   endswith(uint32_t regbase, const char *suffix)
   {
      const char *name = regname(regbase, 0);
   const char *s = strstr(name, suffix);
   if (!s)
            }
      struct regacc
   regacc(struct rnn *r)
   {
      if (!r)
               }
      /* returns true if the complete reg value has been accumulated: */
   bool
   regacc_push(struct regacc *r, uint32_t regbase, uint32_t dword)
   {
      if (r->has_dword_lo) {
      /* Work around kernel devcore dumps which accidentially miss half of a 64b reg
   * see: https://patchwork.freedesktop.org/series/112302/
   */
   if (regbase != r->regbase + 1) {
      printf("WARNING: 64b discontinuity (%x, expected %x)\n", regbase, r->regbase + 1);
   r->has_dword_lo = false;
               r->value |= ((uint64_t)dword) << 32;
                        r->regbase = regbase;
            struct rnndecaddrinfo *info = rnn_reginfo(r->rnn, regbase);
            /* Workaround for kernel devcore dump bugs: */
   if ((info->width == 64) && endswith(regbase, "_HI")) {
      printf("WARNING: 64b discontinuity (no _LO dword for %x)\n", regbase);
                           }
      void
   dump_register_val(struct regacc *r, int level)
   {
               if (info && info->typeinfo) {
      uint64_t gpuaddr = 0;
   char *decoded = rnndec_decodeval(rnn->vc, info->typeinfo, r->value);
            /* Try and figure out if we are looking at a gpuaddr.. this
   * might be useful for other gen's too, but at least a5xx has
   * the _HI/_LO suffix we can look for.  Maybe a better approach
   * would be some special annotation in the xml..
   * for a6xx use "address" and "waddress" types
   */
   if (options->info->chip >= 6) {
      if (!strcmp(info->typeinfo->name, "address") ||
      !strcmp(info->typeinfo->name, "waddress")) {
         } else if (options->info->chip >= 5) {
      /* TODO we shouldn't rely on reg_val() since reg_set() might
   * not have been called yet for the other half of the 64b reg.
   * We can remove this hack once a5xx.xml is converted to reg64
   * and address/waddess.
   */
   if (endswith(r->regbase, "_HI") && endswith(r->regbase - 1, "_LO")) {
         } else if (endswith(r->regbase, "_LO") && endswith(r->regbase + 1, "_HI")) {
                     if (gpuaddr && hostptr(gpuaddr)) {
      printf("\t\tbase=%" PRIx64 ", offset=%" PRIu64 ", size=%u",
                                 } else if (info) {
         } else {
                     }
      static void
   dump_register(struct regacc *r, int level)
   {
      if (!quiet(3)) {
                  for (unsigned idx = 0; type0_reg[idx].regname; idx++) {
      if (type0_reg[idx].regbase == r->regbase) {
      if (type0_reg[idx].is_reg64) {
         } else {
         }
            }
      static bool
   is_banked_reg(uint32_t regbase)
   {
         }
      static void
   dump_registers(uint32_t regbase, uint32_t *dwords, uint32_t sizedwords,
         {
               while (sizedwords--) {
               /* access to non-banked registers needs a WFI:
   * TODO banked register range for a2xx??
   */
   if (needs_wfi && !is_banked_reg(regbase))
            reg_set(regbase, *dwords);
   if (regacc_push(&r, regbase, *dwords))
         regbase++;
   dwords++;
         }
      static void
   dump_domain(uint32_t *dwords, uint32_t sizedwords, int level, const char *name)
   {
      struct rnndomain *dom;
                     if (!dom)
            if (script_packet)
            if (quiet(2))
            for (i = 0; i < sizedwords; i++) {
      struct rnndecaddrinfo *info = rnndec_decodeaddr(rnn->vc, dom, i, 0);
   char *decoded;
   if (!(info && info->typeinfo))
         uint64_t value = dwords[i];
   if (info->typeinfo->high >= 32 && i < sizedwords - 1) {
      value |= (uint64_t)dwords[i + 1] << 32;
      }
   decoded = rnndec_decodeval(rnn->vc, info->typeinfo, value);
   /* Unlike the register printing path, we don't print the name
   * of the register, so if it doesn't contain other named
   * things (i.e. it isn't a bitset) then print the register
   * name as if it's a bitset with a single entry. This avoids
   * having to create a dummy register with a single entry to
   * get a name in the decoding.
   */
   if (info->typeinfo->type == RNN_TTYPE_BITSET ||
      info->typeinfo->type == RNN_TTYPE_INLINE_BITSET) {
      } else {
      printf("%s{ %s%s%s = %s }\n", levels[level], rnn->vc->colors->rname,
      }
   free(decoded);
   free(info->name);
         }
      static uint32_t bin_x1, bin_x2, bin_y1, bin_y2;
   static unsigned mode;
   static const char *render_mode;
   static const char *thread;
   static enum {
      MODE_BINNING = 0x1,
   MODE_GMEM = 0x2,
   MODE_BYPASS = 0x4,
      } enable_mask = MODE_ALL;
   static bool skip_ib2_enable_global;
   static bool skip_ib2_enable_local;
      static void
   print_mode(int level)
   {
      if ((options->info->chip >= 5) && !quiet(2)) {
      printf("%smode: %s", levels[level], render_mode);
   if (thread)
         printf("\n");
   printf("%sskip_ib2: g=%d, l=%d\n", levels[level], skip_ib2_enable_global,
         }
      static bool
   skip_query(void)
   {
      switch (options->query_mode) {
   case QUERY_ALL:
      /* never skip: */
      case QUERY_WRITTEN:
      for (int i = 0; i < options->nquery; i++) {
      uint32_t regbase = queryvals[i];
   if (!reg_written(regbase)) {
         }
   if (reg_rewritten(regbase)) {
            }
      case QUERY_DELTA:
      for (int i = 0; i < options->nquery; i++) {
      uint32_t regbase = queryvals[i];
   if (!reg_written(regbase)) {
         }
   uint32_t lastval = reg_val(regbase);
   if (lastval != lastvals[regbase]) {
            }
      }
      }
      static void
   __do_query(const char *primtype, uint32_t num_indices)
   {
               if ((5 <= options->info->chip) && (options->info->chip < 7)) {
      uint32_t scissor_tl = reg_val(regbase("GRAS_SC_WINDOW_SCISSOR_TL"));
            bin_x1 = scissor_tl & 0xffff;
   bin_y1 = scissor_tl >> 16;
   bin_x2 = scissor_br & 0xffff;
               for (int i = 0; i < options->nquery; i++) {
      uint32_t regbase = queryvals[i];
   if (!reg_written(regbase))
                     /* 64b regs require two successive 32b dwords: */
   for (int d = 0; d < 2; d++)
                  printf("%4d: %s(%u,%u-%u,%u):%u:", draw_count, primtype, bin_x1,
         if (options->info->chip >= 5)
         if (thread)
         printf("\t%08"PRIx64, r.value);
   if (r.value != lastvals[regbase]) {
         } else {
         }
   if (reg_rewritten(regbase)) {
         } else {
         }
   dump_register_val(&r, 0);
               if (n > 1)
      }
      static void
   do_query_compare(const char *primtype, uint32_t num_indices)
   {
      unsigned saved_enable_mask = enable_mask;
            /* in 'query-compare' mode, we want to see if the register is writtten
   * or changed in any mode:
   *
   * (NOTE: this could cause false-positive for 'query-delta' if the reg
   * is written with different values in binning vs sysmem/gmem mode, as
   * we don't track previous values per-mode, but I think we can live with
   * that)
   */
            clear_rewritten();
            if (!skip_query()) {
      /* dump binning pass values: */
   enable_mask = MODE_BINNING;
   render_mode = "BINNING";
   clear_rewritten();
   load_all_groups(0);
            /* dump draw pass values: */
   enable_mask = MODE_GMEM | MODE_BYPASS;
   render_mode = "DRAW";
   clear_rewritten();
   load_all_groups(0);
                        enable_mask = saved_enable_mask;
               }
      /* well, actually query and script..
   * NOTE: call this before dump_register_summary()
   */
   static void
   do_query(const char *primtype, uint32_t num_indices)
   {
      if (script_draw)
            if (options->query_compare) {
      do_query_compare(primtype, num_indices);
               if (skip_query())
               }
      static void
   cp_im_loadi(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t start = dwords[1] >> 16;
   uint32_t size = dwords[1] & 0xffff;
   const char *type = NULL, *ext = NULL;
            switch (dwords[0]) {
   case 0:
      type = "vertex";
   ext = "vo";
   disasm_type = MESA_SHADER_VERTEX;
      case 1:
      type = "fragment";
   ext = "fo";
   disasm_type = MESA_SHADER_FRAGMENT;
      default:
      type = "<unknown>";
   disasm_type = 0;
               printf("%s%s shader, start=%04x, size=%04x\n", levels[level], type, start,
                  /* dump raw shader: */
   if (ext)
      }
      static void
   cp_wide_reg_write(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t reg = dwords[0] & 0xffff;
   struct regacc r = regacc(NULL);
   for (int i = 1; i < sizedwords; i++) {
      if (regacc_push(&r, reg, dwords[i]))
         reg_set(reg, dwords[i]);
         }
      enum state_t {
      TEX_SAMP = 1,
   TEX_CONST,
   TEX_MIPADDR, /* a3xx only */
   SHADER_PROG,
            // image/ssbo state:
   SSBO_0,
   SSBO_1,
                     // unknown things, just to hexdumps:
   UNKNOWN_DWORDS,
   UNKNOWN_2DWORDS,
      };
      enum adreno_state_block {
      SB_VERT_TEX = 0,
   SB_VERT_MIPADDR = 1,
   SB_FRAG_TEX = 2,
   SB_FRAG_MIPADDR = 3,
   SB_VERT_SHADER = 4,
   SB_GEOM_SHADER = 5,
   SB_FRAG_SHADER = 6,
      };
      /* TODO there is probably a clever way to let rnndec parse things so
   * we don't have to care about packet format differences across gens
   */
      static void
   a3xx_get_state_type(uint32_t *dwords, gl_shader_stage *stage,
         {
      unsigned state_block_id = (dwords[0] >> 19) & 0x7;
   unsigned state_type = dwords[1] & 0x3;
   static const struct {
      gl_shader_stage stage;
      } lookup[0xf][0x3] = {
      [SB_VERT_TEX][0] = {MESA_SHADER_VERTEX, TEX_SAMP},
   [SB_VERT_TEX][1] = {MESA_SHADER_VERTEX, TEX_CONST},
   [SB_FRAG_TEX][0] = {MESA_SHADER_FRAGMENT, TEX_SAMP},
   [SB_FRAG_TEX][1] = {MESA_SHADER_FRAGMENT, TEX_CONST},
   [SB_VERT_SHADER][0] = {MESA_SHADER_VERTEX, SHADER_PROG},
   [SB_VERT_SHADER][1] = {MESA_SHADER_VERTEX, SHADER_CONST},
   [SB_FRAG_SHADER][0] = {MESA_SHADER_FRAGMENT, SHADER_PROG},
               *stage = lookup[state_block_id][state_type].stage;
   *state = lookup[state_block_id][state_type].state;
   unsigned state_src = (dwords[0] >> 16) & 0x7;
   if (state_src == 0 /* SS_DIRECT */)
         else
      }
      static enum state_src_t
   _get_state_src(unsigned dword0)
   {
      switch ((dword0 >> 16) & 0x3) {
   case 0: /* SS4_DIRECT / SS6_DIRECT */
         case 2: /* SS4_INDIRECT / SS6_INDIRECT */
         case 1: /* SS6_BINDLESS */
         default:
            }
      static void
   _get_state_type(unsigned state_block_id, unsigned state_type,
         {
      static const struct {
      gl_shader_stage stage;
      } lookup[0x10][0x4] = {
      // SB4_VS_TEX:
   [0x0][0] = {MESA_SHADER_VERTEX, TEX_SAMP},
   [0x0][1] = {MESA_SHADER_VERTEX, TEX_CONST},
   [0x0][2] = {MESA_SHADER_VERTEX, UBO},
   // SB4_HS_TEX:
   [0x1][0] = {MESA_SHADER_TESS_CTRL, TEX_SAMP},
   [0x1][1] = {MESA_SHADER_TESS_CTRL, TEX_CONST},
   [0x1][2] = {MESA_SHADER_TESS_CTRL, UBO},
   // SB4_DS_TEX:
   [0x2][0] = {MESA_SHADER_TESS_EVAL, TEX_SAMP},
   [0x2][1] = {MESA_SHADER_TESS_EVAL, TEX_CONST},
   [0x2][2] = {MESA_SHADER_TESS_EVAL, UBO},
   // SB4_GS_TEX:
   [0x3][0] = {MESA_SHADER_GEOMETRY, TEX_SAMP},
   [0x3][1] = {MESA_SHADER_GEOMETRY, TEX_CONST},
   [0x3][2] = {MESA_SHADER_GEOMETRY, UBO},
   // SB4_FS_TEX:
   [0x4][0] = {MESA_SHADER_FRAGMENT, TEX_SAMP},
   [0x4][1] = {MESA_SHADER_FRAGMENT, TEX_CONST},
   [0x4][2] = {MESA_SHADER_FRAGMENT, UBO},
   // SB4_CS_TEX:
   [0x5][0] = {MESA_SHADER_COMPUTE, TEX_SAMP},
   [0x5][1] = {MESA_SHADER_COMPUTE, TEX_CONST},
   [0x5][2] = {MESA_SHADER_COMPUTE, UBO},
   // SB4_VS_SHADER:
   [0x8][0] = {MESA_SHADER_VERTEX, SHADER_PROG},
   [0x8][1] = {MESA_SHADER_VERTEX, SHADER_CONST},
   [0x8][2] = {MESA_SHADER_VERTEX, UBO},
   // SB4_HS_SHADER
   [0x9][0] = {MESA_SHADER_TESS_CTRL, SHADER_PROG},
   [0x9][1] = {MESA_SHADER_TESS_CTRL, SHADER_CONST},
   [0x9][2] = {MESA_SHADER_TESS_CTRL, UBO},
   // SB4_DS_SHADER
   [0xa][0] = {MESA_SHADER_TESS_EVAL, SHADER_PROG},
   [0xa][1] = {MESA_SHADER_TESS_EVAL, SHADER_CONST},
   [0xa][2] = {MESA_SHADER_TESS_EVAL, UBO},
   // SB4_GS_SHADER
   [0xb][0] = {MESA_SHADER_GEOMETRY, SHADER_PROG},
   [0xb][1] = {MESA_SHADER_GEOMETRY, SHADER_CONST},
   [0xb][2] = {MESA_SHADER_GEOMETRY, UBO},
   // SB4_FS_SHADER:
   [0xc][0] = {MESA_SHADER_FRAGMENT, SHADER_PROG},
   [0xc][1] = {MESA_SHADER_FRAGMENT, SHADER_CONST},
   [0xc][2] = {MESA_SHADER_FRAGMENT, UBO},
   // SB4_CS_SHADER:
   [0xd][0] = {MESA_SHADER_COMPUTE, SHADER_PROG},
   [0xd][1] = {MESA_SHADER_COMPUTE, SHADER_CONST},
   [0xd][2] = {MESA_SHADER_COMPUTE, UBO},
   [0xd][3] = {MESA_SHADER_COMPUTE, SSBO_0}, /* a6xx location */
   // SB4_SSBO (shared across all stages)
   [0xe][0] = {0, SSBO_0}, /* a5xx (and a4xx?) location */
   [0xe][1] = {0, SSBO_1},
   [0xe][2] = {0, SSBO_2},
   // SB4_CS_SSBO
   [0xf][0] = {MESA_SHADER_COMPUTE, SSBO_0},
   [0xf][1] = {MESA_SHADER_COMPUTE, SSBO_1},
   [0xf][2] = {MESA_SHADER_COMPUTE, SSBO_2},
   // unknown things
   /* This looks like combined UBO state for 3d stages (a5xx and
   * before??  I think a6xx has UBO state per shader stage:
   */
   [0x6][2] = {0, UBO},
               *stage = lookup[state_block_id][state_type].stage;
      }
      static void
   a4xx_get_state_type(uint32_t *dwords, gl_shader_stage *stage,
         {
      unsigned state_block_id = (dwords[0] >> 18) & 0xf;
   unsigned state_type = dwords[1] & 0x3;
   _get_state_type(state_block_id, state_type, stage, state);
      }
      static void
   a6xx_get_state_type(uint32_t *dwords, gl_shader_stage *stage,
         {
      unsigned state_block_id = (dwords[0] >> 18) & 0xf;
   unsigned state_type = (dwords[0] >> 14) & 0x3;
   _get_state_type(state_block_id, state_type, stage, state);
      }
      static void
   dump_tex_samp(uint32_t *texsamp, enum state_src_t src, int num_unit, int level)
   {
      for (int i = 0; i < num_unit; i++) {
      /* work-around to reduce noise for opencl blob which always
   * writes the max # regardless of # of textures used
   */
   if ((num_unit == 16) && (texsamp[0] == 0) && (texsamp[1] == 0))
            if (options->info->chip == 3) {
      dump_domain(texsamp, 2, level + 2, "A3XX_TEX_SAMP");
   dump_hex(texsamp, 2, level + 1);
      } else if (options->info->chip == 4) {
      dump_domain(texsamp, 2, level + 2, "A4XX_TEX_SAMP");
   dump_hex(texsamp, 2, level + 1);
      } else if (options->info->chip == 5) {
      dump_domain(texsamp, 4, level + 2, "A5XX_TEX_SAMP");
   dump_hex(texsamp, 4, level + 1);
      } else if ((6 <= options->info->chip) && (options->info->chip < 8)) {
      dump_domain(texsamp, 4, level + 2, "A6XX_TEX_SAMP");
   dump_hex(texsamp, 4, level + 1);
            }
      static void
   dump_tex_const(uint32_t *texconst, int num_unit, int level)
   {
      for (int i = 0; i < num_unit; i++) {
      /* work-around to reduce noise for opencl blob which always
   * writes the max # regardless of # of textures used
   */
   if ((num_unit == 16) && (texconst[0] == 0) && (texconst[1] == 0) &&
                  if (options->info->chip == 3) {
      dump_domain(texconst, 4, level + 2, "A3XX_TEX_CONST");
   dump_hex(texconst, 4, level + 1);
      } else if (options->info->chip == 4) {
      dump_domain(texconst, 8, level + 2, "A4XX_TEX_CONST");
   if (options->dump_textures) {
      uint32_t addr = texconst[4] & ~0x1f;
      }
   dump_hex(texconst, 8, level + 1);
      } else if (options->info->chip == 5) {
      dump_domain(texconst, 12, level + 2, "A5XX_TEX_CONST");
   if (options->dump_textures) {
      uint64_t addr =
            }
   dump_hex(texconst, 12, level + 1);
      } else if ((6 <= options->info->chip) && (options->info->chip < 8)) {
      dump_domain(texconst, 16, level + 2, "A6XX_TEX_CONST");
   if (options->dump_textures) {
      uint64_t addr =
            }
   dump_hex(texconst, 16, level + 1);
            }
      static void
   cp_load_state(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      gl_shader_stage stage;
   enum state_t state;
   enum state_src_t src;
   uint32_t num_unit = (dwords[0] >> 22) & 0x1ff;
   uint64_t ext_src_addr;
   void *contents;
            if (quiet(2) && !options->script)
            if (options->info->chip >= 6)
         else if (options->info->chip >= 4)
         else
            switch (src) {
   case STATE_SRC_DIRECT:
      ext_src_addr = 0;
      case STATE_SRC_INDIRECT:
      if (is_64b()) {
      ext_src_addr = dwords[1] & 0xfffffffc;
      } else {
                     case STATE_SRC_BINDLESS: {
      const unsigned base_reg = stage == MESA_SHADER_COMPUTE
                  if (is_64b()) {
      const unsigned reg = base_reg + (dwords[1] >> 28) * 2;
   ext_src_addr = reg_val(reg) & 0xfffffffc;
      } else {
      const unsigned reg = base_reg + (dwords[1] >> 28);
               ext_src_addr += 4 * (dwords[1] & 0xffffff);
      }
            if (ext_src_addr)
         else
            if (!contents)
            switch (state) {
   case SHADER_PROG: {
               if (quiet(2))
            if (options->info->chip >= 4)
         else if (options->info->chip >= 3)
            /* shaders:
   *
   * note: num_unit seems to be # of instruction groups, where
   * an instruction group has 4 64bit instructions.
   */
   if (stage == MESA_SHADER_VERTEX) {
         } else if (stage == MESA_SHADER_GEOMETRY) {
         } else if (stage == MESA_SHADER_COMPUTE) {
         } else if (stage == MESA_SHADER_FRAGMENT) {
                  if (contents)
                  /* dump raw shader: */
   if (ext)
               }
   case SHADER_CONST: {
      if (quiet(2))
            /* uniforms/consts:
   *
   * note: num_unit seems to be # of pairs of dwords??
            if (options->info->chip >= 4)
            dump_float(contents, num_unit * 2, level + 1);
               }
   case TEX_MIPADDR: {
               if (quiet(2))
            /* mipmap consts block just appears to be array of num_unit gpu addr's: */
   for (i = 0; i < num_unit; i++) {
      void *ptr = hostptr(addrs[i]);
   printf("%s%2d: %08x\n", levels[level + 1], i, addrs[i]);
   if (options->dump_textures) {
      printf("base=%08x\n", (uint32_t)gpubaseaddr(addrs[i]));
         }
      }
   case TEX_SAMP: {
      dump_tex_samp(contents, src, num_unit, level);
      }
   case TEX_CONST: {
      dump_tex_const(contents, num_unit, level);
      }
   case SSBO_0: {
               for (i = 0; i < num_unit; i++) {
      int sz = 4;
   if (options->info->chip == 4) {
         } else if (options->info->chip == 5) {
         } else if ((6 <= options->info->chip) && (options->info->chip < 8)) {
      sz = 16;
      }
   dump_hex(ssboconst, sz, level + 1);
      }
      }
   case SSBO_1: {
               for (i = 0; i < num_unit; i++) {
      if (options->info->chip == 4)
         else if (options->info->chip == 5)
         dump_hex(ssboconst, 2, level + 1);
      }
      }
   case SSBO_2: {
               for (i = 0; i < num_unit; i++) {
      /* TODO a4xx and a5xx might be same: */
   if (options->info->chip == 5) {
      dump_domain(ssboconst, 2, level + 2, "A5XX_SSBO_2");
      }
   if (options->dump_textures) {
      uint64_t addr =
            }
      }
      }
   case UBO: {
               for (i = 0; i < num_unit; i++) {
      // TODO probably similar on a4xx..
   if (options->info->chip == 5)
         else if (options->info->chip == 6)
         dump_hex(uboconst, 2, level + 1);
      }
      }
   case UNKNOWN_DWORDS: {
      if (quiet(2))
         dump_hex(contents, num_unit, level + 1);
      }
   case UNKNOWN_2DWORDS: {
      if (quiet(2))
         dump_hex(contents, num_unit * 2, level + 1);
      }
   case UNKNOWN_4DWORDS: {
      if (quiet(2))
         dump_hex(contents, num_unit * 4, level + 1);
      }
   default:
      if (quiet(2))
         /* hmm.. */
   dump_hex(contents, num_unit, level + 1);
         }
      static void
   cp_set_bin(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      bin_x1 = dwords[1] & 0xffff;
   bin_y1 = dwords[1] >> 16;
   bin_x2 = dwords[2] & 0xffff;
      }
      static void
   dump_a2xx_tex_const(uint32_t *dwords, uint32_t sizedwords, uint32_t val,
         {
      uint32_t w, h, p;
   uint32_t gpuaddr, flags, mip_gpuaddr, mip_flags;
   uint32_t min, mag, swiz, clamp_x, clamp_y, clamp_z;
   static const char *filter[] = {
      "point",
   "bilinear",
      };
   static const char *clamp[] = {
      "wrap",
   "mirror",
      };
                     /* Texture, FormatXYZW=Unsigned, ClampXYZ=Wrap/Repeat,
   * RFMode=ZeroClamp-1, Dim=1:2d, pitch
   */
   p = (dwords[0] >> 22) << 5;
   clamp_x = (dwords[0] >> 10) & 0x3;
   clamp_y = (dwords[0] >> 13) & 0x3;
            /* Format=6:8888_WZYX, EndianSwap=0:None, ReqSize=0:256bit, DimHi=0,
   * NearestClamp=1:OGL Mode
   */
            /* Width, Height, EndianSwap=0:None */
   w = (dwords[2] & 0x1fff) + 1;
            /* NumFormat=0:RF, DstSelXYZW=XYZW, ExpAdj=0, MagFilt=MinFilt=0:Point,
   * Mip=2:BaseMap
   */
   mag = (dwords[3] >> 19) & 0x3;
   min = (dwords[3] >> 21) & 0x3;
            /* VolMag=VolMin=0:Point, MinMipLvl=0, MaxMipLvl=1, LodBiasH=V=0,
   * Dim3d=0
   */
            /* BorderColor=0:ABGRBlack, ForceBC=0:diable, TriJuice=0, Aniso=0,
   * Dim=1:2d, MipPacking=0
   */
            printf("%sset texture const %04x\n", levels[level], val);
   printf("%sclamp x/y/z: %s/%s/%s\n", levels[level + 1], clamp[clamp_x],
         printf("%sfilter min/mag: %s/%s\n", levels[level + 1], filter[min],
         printf("%sswizzle: %c%c%c%c\n", levels[level + 1],
         swiznames[(swiz >> 0) & 0x7], swiznames[(swiz >> 3) & 0x7],
   printf("%saddr=%08x (flags=%03x), size=%dx%d, pitch=%d, format=%s\n",
         levels[level + 1], gpuaddr, flags, w, h, p,
   printf("%smipaddr=%08x (flags=%03x)\n", levels[level + 1], mip_gpuaddr,
      }
      static void
   dump_a2xx_shader_const(uint32_t *dwords, uint32_t sizedwords, uint32_t val,
         {
      int i;
   printf("%sset shader const %04x\n", levels[level], val);
   for (i = 0; i < sizedwords;) {
      uint32_t gpuaddr, flags;
   parse_dword_addr(dwords[i++], &gpuaddr, &flags, 0xf);
   void *addr = hostptr(gpuaddr);
   if (addr) {
      const char *fmt =
         uint32_t size = dwords[i++];
   printf("%saddr=%08x, size=%d, format=%s\n", levels[level + 1], gpuaddr,
         // TODO maybe dump these as bytes instead of dwords?
   size = (size + 3) / 4; // for now convert to dwords
   dump_hex(addr, MIN2(size, 64), level + 1);
   if (size > MIN2(size, 64))
         dump_float(addr, MIN2(size, 64), level + 1);
   if (size > MIN2(size, 64))
            }
      static void
   cp_set_const(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t val = dwords[0] & 0xffff;
   switch ((dwords[0] >> 16) & 0xf) {
   case 0x0:
      dump_float((float *)(dwords + 1), sizedwords - 1, level + 1);
      case 0x1:
      /* need to figure out how const space is partitioned between
   * attributes, textures, etc..
   */
   if (val < 0x78) {
         } else {
         }
      case 0x2:
      printf("%sset bool const %04x\n", levels[level], val);
      case 0x3:
      printf("%sset loop const %04x\n", levels[level], val);
      case 0x4:
      val += 0x2000;
   if (dwords[0] & 0x80000000) {
                     /* TODO: not sure what happens w/ payload != 2.. */
                  /* note: rnn_regname uses a static buf so we can't do
   * two regname() calls for one printf..
   */
                              } else {
         }
         }
      static void dump_register_summary(int level);
      static void
   cp_event_write(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      const char *name = rnn_enumname(rnn, "vgt_event_type", dwords[0]);
            if (name && (options->info->chip > 5)) {
      char eventname[64];
   snprintf(eventname, sizeof(eventname), "EVENT:%s", name);
   if (!strcmp(name, "BLIT")) {
      do_query(eventname, 0);
   print_mode(level);
            }
      static void
   dump_register_summary(int level)
   {
      uint32_t i;
   bool saved_summary = summary;
                              /* dump current state of registers: */
            bool changed = false;
            for (i = 0; i < regcnt(); i++) {
      uint32_t regbase = i;
   uint32_t lastval = reg_val(regbase);
   /* skip registers that haven't been updated since last draw/blit: */
   if (!(options->allregs || reg_rewritten(regbase)))
         if (!reg_written(regbase))
         if (lastval != lastvals[regbase]) {
      changed |= true;
      }
   if (reg_rewritten(regbase)) {
         }
   if (!quiet(2)) {
      if (regacc_push(&r, regbase, lastval)) {
      if (changed) {
         } else {
         }
   if (written) {
         } else {
         }
                                                      draw_count++;
      }
      static uint32_t
   draw_indx_common(uint32_t *dwords, int level)
   {
      uint32_t prim_type = dwords[1] & 0x1f;
   uint32_t source_select = (dwords[1] >> 6) & 0x3;
   uint32_t num_indices = dwords[2];
                              printl(2, "%sdraw:          %d\n", levels[level], draws[ib]);
   printl(2, "%sprim_type:     %s (%d)\n", levels[level], primtype, prim_type);
   printl(2, "%ssource_select: %s (%d)\n", levels[level],
                                       }
      enum pc_di_index_size {
      INDEX_SIZE_IGN = 0,
   INDEX_SIZE_16_BIT = 0,
   INDEX_SIZE_32_BIT = 1,
   INDEX_SIZE_8_BIT = 2,
      };
      static void
   cp_draw_indx(uint32_t *dwords, uint32_t sizedwords, int level)
   {
                        /* if we have an index buffer, dump that: */
   if (sizedwords == 5) {
      void *ptr = hostptr(dwords[3]);
   printl(2, "%sgpuaddr:       %08x\n", levels[level], dwords[3]);
   printl(2, "%sidx_size:      %d\n", levels[level], dwords[4]);
   if (ptr) {
      enum pc_di_index_size size =
         if (!quiet(2)) {
      int i;
   printf("%sidxs:         ", levels[level]);
   if (size == INDEX_SIZE_8_BIT) {
      uint8_t *idx = ptr;
   for (i = 0; i < dwords[4]; i++)
      } else if (size == INDEX_SIZE_16_BIT) {
      uint16_t *idx = ptr;
   for (i = 0; i < dwords[4] / 2; i++)
      } else if (size == INDEX_SIZE_32_BIT) {
      uint32_t *idx = ptr;
   for (i = 0; i < dwords[4] / 4; i++)
      }
   printf("\n");
                     /* don't bother dumping registers for the dummy draw_indx's.. */
   if (num_indices > 0)
               }
      static void
   cp_draw_indx_2(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t num_indices = draw_indx_common(dwords, level);
   enum pc_di_index_size size =
         void *ptr = &dwords[3];
                     /* CP_DRAW_INDX_2 has embedded/inline idx buffer: */
   if (!quiet(2)) {
      int i;
   printf("%sidxs:         ", levels[level]);
   if (size == INDEX_SIZE_8_BIT) {
      uint8_t *idx = ptr;
   for (i = 0; i < num_indices; i++)
            } else if (size == INDEX_SIZE_16_BIT) {
      uint16_t *idx = ptr;
   for (i = 0; i < num_indices; i++)
            } else if (size == INDEX_SIZE_32_BIT) {
      uint32_t *idx = ptr;
   for (i = 0; i < num_indices; i++)
            }
   printf("\n");
               /* don't bother dumping registers for the dummy draw_indx's.. */
   if (num_indices > 0)
      }
      static void
   cp_draw_indx_offset(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t num_indices = dwords[2];
            do_query(rnn_enumname(rnn, "pc_di_primtype", prim_type), num_indices);
            /* don't bother dumping registers for the dummy draw_indx's.. */
   if (num_indices > 0)
      }
      static void
   cp_draw_indx_indirect(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t prim_type = dwords[0] & 0x1f;
            do_query(rnn_enumname(rnn, "pc_di_primtype", prim_type), 0);
            if (is_64b())
         else
                  if (is_64b())
         else
                     }
      static void
   cp_draw_indirect(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t prim_type = dwords[0] & 0x1f;
            do_query(rnn_enumname(rnn, "pc_di_primtype", prim_type), 0);
            addr = (((uint64_t)dwords[2] & 0x1ffff) << 32) | dwords[1];
               }
      static void
   cp_draw_indirect_multi(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t prim_type = dwords[0] & 0x1f;
            do_query(rnn_enumname(rnn, "pc_di_primtype", prim_type), 0);
            struct rnndomain *domain = rnn_finddomain(rnn->db, "CP_DRAW_INDIRECT_MULTI");
   uint32_t count_dword = rnndec_decodereg(rnn->vc, domain, "INDIRECT_COUNT");
   uint32_t addr_dword = rnndec_decodereg(rnn->vc, domain, "INDIRECT");
            if (count_dword) {
      uint64_t count_addr =
                  /* Don't print more draws than this if we don't know the indirect
   * count. It's possible the user will give ~0 or some other large
   * value, expecting the GPU to fill in the draw count, and we don't
   * want to print a gazillion draws in that case:
   */
            /* Assume the indirect count is garbage if it's larger than this
   * (quite large) value or 0. Hopefully this catches most cases.
   */
            if (buf) {
      printf("%sindirect count: %u\n", levels[level], *buf);
   if (*buf == 0 || *buf > max_indirect_draw_count) {
      /* garbage value */
      } else {
      /* not garbage */
         } else {
                     if (addr_dword && stride_dword) {
      uint64_t addr =
                  for (unsigned i = 0; i < count; i++, addr += stride) {
      printf("%sdraw %d:\n", levels[level], i);
                     }
      static void
   cp_draw_auto(uint32_t *dwords, uint32_t sizedwords, int level)
   {
               do_query(rnn_enumname(rnn, "pc_di_primtype", prim_type), 0);
               }
      static void
   cp_run_cl(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      do_query("COMPUTE", 1);
      }
      static void
   print_nop_tail_string(uint32_t *dwords, uint32_t sizedwords)
   {
      const char *buf = (void *)dwords;
   for (int i = 0; i < 4 * sizedwords; i++) {
      if (buf[i] == '\0')
         if (isascii(buf[i]))
         }
      static void
   cp_nop(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      if (quiet(3))
            /* NOP is used to encode special debug strings by Turnip.
   * See tu_cs_emit_debug_magic_strv(...)
   */
   static int scope_level = 0;
   uint32_t identifier = dwords[0];
   bool is_special = false;
   if (identifier == CP_NOP_MESG) {
      printf("### ");
      } else if (identifier == CP_NOP_BEGN) {
      printf(">>> #%d: ", ++scope_level);
      } else if (identifier == CP_NOP_END) {
      printf("<<< #%d: ", scope_level--);
               if (is_special) {
      if (sizedwords > 1) {
      print_nop_tail_string(dwords + 1, sizedwords - 1);
      }
               // blob doesn't use CP_NOP for string_marker but it does
   // use it for things that end up looking like, but aren't
   // ascii chars:
   if (!options->decode_markers)
            print_nop_tail_string(dwords, sizedwords);
      }
      uint32_t *
   parse_cp_indirect(uint32_t *dwords, uint32_t sizedwords,
         {
      if (is_64b()) {
               /* a5xx+.. high 32b of gpu addr, then size: */
   *ibaddr = dwords[0];
   *ibaddr |= ((uint64_t)dwords[1]) << 32;
               } else {
               *ibaddr = dwords[0];
                  }
      static void
   cp_indirect(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      /* traverse indirect buffers */
   uint64_t ibaddr;
   uint32_t ibsize;
                     if (!quiet(3)) {
      if (is_64b()) {
         } else {
         }
               if (options->once && has_dumped(ibaddr, enable_mask))
            /* 'query-compare' mode implies 'once' mode, although we need only to
   * process the cmdstream for *any* enable_mask mode, since we are
   * comparing binning vs draw reg values at the same time, ie. it is
   * not useful to process the same draw in both binning and draw pass.
   */
   if (options->query_compare && has_dumped(ibaddr, MODE_ALL))
            /* map gpuaddr back to hostptr: */
            if (ptr) {
      /* If the GPU hung within the target IB, the trigger point will be
   * just after the current CP_INDIRECT_BUFFER.  Because the IB is
   * executed but never returns.  Account for this by checking if
   * the IB returned:
   */
            ib++;
   ibs[ib].base = ibaddr;
            dump_commands(ptr, ibsize, level);
      } else {
            }
      static void
   cp_start_bin(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint64_t ibaddr;
   uint32_t ibsize;
   uint32_t loopcount;
            loopcount = dwords[0];
   ibaddr = dwords[1];
   ibaddr |= ((uint64_t)dwords[2]) << 32;
            /* map gpuaddr back to hostptr: */
            if (ptr) {
      /* If the GPU hung within the target IB, the trigger point will be
   * just after the current CP_START_BIN.  Because the IB is
   * executed but never returns.  Account for this by checking if
   * the IB returned:
   */
            /* TODO: we should duplicate the body of the loop after each bin, so
   * that draws get the correct state. We should also figure out if there
   * are any registers that can tell us what bin we're in when we hang so
   * that crashdec points to the right place.
   */
   ib++;
   for (uint32_t i = 0; i < loopcount; i++) {
      ibs[ib].base = ibaddr;
   ibs[ib].size = ibsize;
   printl(3, "%sbin %u\n", levels[level], i);
   dump_commands(ptr, ibsize, level);
   ibaddr += ibsize;
      }
      } else {
            }
      static void
   cp_fixed_stride_draw_table(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint64_t ibaddr;
   uint32_t ibsize;
   uint32_t loopcount;
            loopcount = dwords[3];
   ibaddr = dwords[0];
   ibaddr |= ((uint64_t)dwords[1]) << 32;
            /* map gpuaddr back to hostptr: */
            if (ptr) {
      /* If the GPU hung within the target IB, the trigger point will be
   * just after the current CP_START_BIN.  Because the IB is
   * executed but never returns.  Account for this by checking if
   * the IB returned:
   */
            ib++;
   for (uint32_t i = 0; i < loopcount; i++) {
      ibs[ib].base = ibaddr;
   ibs[ib].size = ibsize;
   printl(3, "%sdraw %u\n", levels[level], i);
   dump_commands(ptr, ibsize, level);
   ibaddr += ibsize;
      }
      } else {
            }
      static void
   cp_wfi(uint32_t *dwords, uint32_t sizedwords, int level)
   {
         }
      static void
   cp_mem_write(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      if (quiet(2))
            if (is_64b()) {
      uint64_t gpuaddr = dwords[0] | (((uint64_t)dwords[1]) << 32);
   printf("%sgpuaddr:%016" PRIx64 "\n", levels[level], gpuaddr);
            if (pkt_is_type4(dwords[2]) || pkt_is_type7(dwords[2]))
      } else {
      uint32_t gpuaddr = dwords[0];
   printf("%sgpuaddr:%08x\n", levels[level], gpuaddr);
         }
      static void
   cp_rmw(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t val = dwords[0] & 0xffff;
   uint32_t and = dwords[1];
   uint32_t or = dwords[2];
   printl(3, "%srmw (%s & 0x%08x) | 0x%08x)\n", levels[level], regname(val, 1),
         if (needs_wfi)
      printl(2, "NEEDS WFI: rmw (%s & 0x%08x) | 0x%08x)\n", regname(val, 1),
         }
      static void
   cp_reg_mem(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t val = dwords[0] & 0xffff;
            if (quiet(2))
            uint64_t gpuaddr = dwords[1] | (((uint64_t)dwords[2]) << 32);
   printf("%sgpuaddr:%016" PRIx64 "\n", levels[level], gpuaddr);
   void *ptr = hostptr(gpuaddr);
   if (ptr) {
      uint32_t cnt = (dwords[0] >> 19) & 0x3ff;
         }
      struct draw_state {
      uint16_t enable_mask;
   uint16_t flags;
   uint32_t count;
      };
      struct draw_state state[32];
      #define FLAG_DIRTY              0x1
   #define FLAG_DISABLE            0x2
   #define FLAG_DISABLE_ALL_GROUPS 0x4
   #define FLAG_LOAD_IMMED         0x8
      static int draw_mode;
      static void
   disable_group(unsigned group_id)
   {
      struct draw_state *ds = &state[group_id];
      }
      static void
   disable_all_groups(void)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(state); i++)
      }
      static void
   load_group(unsigned group_id, int level)
   {
               if (!ds->count)
            printl(2, "%sgroup_id: %u\n", levels[level], group_id);
   printl(2, "%scount: %d\n", levels[level], ds->count);
   printl(2, "%saddr: %016llx\n", levels[level], ds->addr);
            if (options->info->chip >= 6) {
               if (!(ds->enable_mask & enable_mask)) {
      printl(2, "%s\tskipped!\n\n", levels[level]);
                  void *ptr = hostptr(ds->addr);
   if (ptr) {
      if (!quiet(2))
            ib++;
   dump_commands(ptr, ds->count, level + 1);
         }
      static void
   load_all_groups(int level)
   {
      /* sanity check, we should never recursively hit recursion here, and if
   * we do bad things happen:
   */
   static bool loading_groups = false;
   if (loading_groups) {
      printf("ERROR: nothing in draw state should trigger recursively loading "
            }
   loading_groups = true;
   for (unsigned i = 0; i < ARRAY_SIZE(state); i++)
                  /* in 'query-compare' mode, defer disabling all groups until we have a
   * chance to process the query:
   */
   if (!options->query_compare)
      }
      static void
   cp_set_draw_state(uint32_t *dwords, uint32_t sizedwords, int level)
   {
               for (i = 0; i < sizedwords;) {
      struct draw_state *ds;
   uint32_t count = dwords[i] & 0xffff;
   uint32_t group_id = (dwords[i] >> 24) & 0x1f;
   uint32_t enable_mask = (dwords[i] >> 20) & 0xf;
   uint32_t flags = (dwords[i] >> 16) & 0xf;
            if (is_64b()) {
      addr = dwords[i + 1];
   addr |= ((uint64_t)dwords[i + 2]) << 32;
      } else {
      addr = dwords[i + 1];
               if (flags & FLAG_DISABLE_ALL_GROUPS) {
      disable_all_groups();
               if (flags & FLAG_DISABLE) {
      disable_group(group_id);
               assert(group_id < ARRAY_SIZE(state));
                     ds->enable_mask = enable_mask;
   ds->flags = flags;
   ds->count = count;
            if (flags & FLAG_LOAD_IMMED) {
      load_group(group_id, level);
            }
      static void
   cp_set_mode(uint32_t *dwords, uint32_t sizedwords, int level)
   {
         }
      /* execute compute shader */
   static void
   cp_exec_cs(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      do_query("compute", 0);
      }
      static void
   cp_exec_cs_indirect(uint32_t *dwords, uint32_t sizedwords, int level)
   {
               if (is_64b()) {
         } else {
                  printl(3, "%saddr: %016llx\n", levels[level], addr);
            do_query("compute", 0);
      }
      static void
   cp_set_marker(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t val = dwords[0] & 0xf;
            if (!mode) {
      static char buf[8];
   sprintf(buf, "0x%x", val);
   render_mode = buf;
                        if (!strcmp(render_mode, "RM6_BINNING")) {
         } else if (!strcmp(render_mode, "RM6_GMEM")) {
         } else if (!strcmp(render_mode, "RM6_BYPASS")) {
            }
      static void
   cp_set_thread_control(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint32_t val = dwords[0] & 0x3;
      }
      static void
   cp_set_render_mode(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint64_t addr;
                     /* TODO seems to have two ptrs, 9 dwords total (incl pkt7 hdr)..
   * not sure if this can come in different sizes.
   *
   * First ptr doesn't seem to be cmdstream, second one does.
   *
   * Comment from downstream kernel:
   *
   * SRM -- set render mode (ex binning, direct render etc)
   * SRM is set by UMD usually at start of IB to tell CP the type of
   * preemption.
   * KMD needs to set SRM to NULL to indicate CP that rendering is
   * done by IB.
   * ------------------------------------------------------------------
   *
   * Seems to always be one of these two:
   * 70ec0008 00000001 001c0000 00000000 00000010 00000003 0000000d 001c2000
   * 00000000 70ec0008 00000001 001c0000 00000000 00000000 00000003 0000000d
   * 001c2000 00000000
   *
                              if (sizedwords == 1)
            addr = dwords[1];
                              if (sizedwords == 5)
                     len = dwords[5];
   addr = dwords[6];
            printl(3, "%saddr: 0x%016lx\n", levels[level], addr);
                     if (ptr) {
      if (!quiet(2)) {
      ib++;
   dump_commands(ptr, len, level + 1);
   ib--;
            }
      static void
   cp_compute_checkpoint(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint64_t addr;
            assert(is_64b());
            if (sizedwords == 8) {
      addr = dwords[5];
   addr |= ((uint64_t)dwords[6]) << 32;
      } else {
      addr = dwords[5];
   addr |= ((uint64_t)dwords[6]) << 32;
               printl(3, "%saddr: 0x%016" PRIx64 "\n", levels[level], addr);
                     if (ptr) {
      if (!quiet(2)) {
      ib++;
   dump_commands(ptr, len, level + 1);
   ib--;
            }
      static void
   cp_blit(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      do_query(rnn_enumname(rnn, "cp_blit_cmd", dwords[0]), 0);
   print_mode(level);
      }
      static void
   cp_context_reg_bunch(uint32_t *dwords, uint32_t sizedwords, int level)
   {
               /* NOTE: seems to write same reg multiple times.. not sure if different parts
   * of these are triggered by the FLUSH_SO_n events?? (if that is what they
   * actually are?)
   */
   bool saved_summary = summary;
                     for (i = 0; i < sizedwords; i += 2) {
      if (regacc_push(&r, dwords[i + 0], dwords[i + 1]))
                        }
      /* Looks similar to CP_CONTEXT_REG_BUNCH, but not quite the same...
   * discarding first two dwords??
   *
   *   CP_CONTEXT_REG_BUNCH:
   *        0221: 9c1ff606  (rep)(xmov3)mov $usraddr, $data
   *        ; mov $data, $data
   *        ; mov $usraddr, $data
   *        ; mov $data, $data
   *        0222: d8000000  waitin
   *        0223: 981f0806  mov $01, $data
   *
   *   CP_UNK5D:
   *        0224: 981f0006  mov $00, $data
   *        0225: 981f0006  mov $00, $data
   *        0226: 9c1ff206  (rep)(xmov1)mov $usraddr, $data
   *        ; mov $data, $data
   *        0227: d8000000  waitin
   *        0228: 981f0806  mov $01, $data
   *
   */
   static void
   cp_context_reg_bunch2(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      dwords += 2;
   sizedwords -= 2;
      }
      static void
   cp_reg_write(uint32_t *dwords, uint32_t sizedwords, int level)
   {
               struct regacc r = regacc(NULL);
   if (regacc_push(&r, reg, dwords[2]))
            }
      static void
   cp_set_ctxswitch_ib(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      uint64_t addr;
   uint32_t size = dwords[2] & 0xffff;
                     if (!quiet(3)) {
                  ptr = hostptr(addr);
   if (ptr) {
            }
      static void
   cp_skip_ib2_enable_global(uint32_t *dwords, uint32_t sizedwords, int level)
   {
         }
      static void
   cp_skip_ib2_enable_local(uint32_t *dwords, uint32_t sizedwords, int level)
   {
         }
      #define CP(x, fxn, ...) { "CP_" #x, fxn, ##__VA_ARGS__ }
   static const struct type3_op {
      const char *name;
   void (*fxn)(uint32_t *dwords, uint32_t sizedwords, int level);
   struct {
            } type3_op[] = {
      CP(NOP, cp_nop),
   CP(INDIRECT_BUFFER, cp_indirect),
   CP(INDIRECT_BUFFER_PFD, cp_indirect),
   CP(WAIT_FOR_IDLE, cp_wfi),
   CP(REG_RMW, cp_rmw),
   CP(REG_TO_MEM, cp_reg_mem),
   CP(MEM_TO_REG, cp_reg_mem), /* same layout as CP_REG_TO_MEM */
   CP(MEM_WRITE, cp_mem_write),
   CP(EVENT_WRITE, cp_event_write),
   CP(RUN_OPENCL, cp_run_cl),
   CP(DRAW_INDX, cp_draw_indx, {.load_all_groups = true}),
   CP(DRAW_INDX_2, cp_draw_indx_2, {.load_all_groups = true}),
   CP(SET_CONSTANT, cp_set_const),
   CP(IM_LOAD_IMMEDIATE, cp_im_loadi),
            /* for a3xx */
   CP(LOAD_STATE, cp_load_state),
            /* for a4xx */
   CP(LOAD_STATE4, cp_load_state),
   CP(SET_DRAW_STATE, cp_set_draw_state),
   CP(DRAW_INDX_OFFSET, cp_draw_indx_offset, {.load_all_groups = true}),
   CP(EXEC_CS, cp_exec_cs, {.load_all_groups = true}),
            /* for a5xx */
   CP(SET_RENDER_MODE, cp_set_render_mode),
   CP(COMPUTE_CHECKPOINT, cp_compute_checkpoint),
   CP(BLIT, cp_blit),
   CP(CONTEXT_REG_BUNCH, cp_context_reg_bunch),
   CP(DRAW_INDIRECT, cp_draw_indirect, {.load_all_groups = true}),
   CP(DRAW_INDX_INDIRECT, cp_draw_indx_indirect, {.load_all_groups = true}),
   CP(DRAW_INDIRECT_MULTI, cp_draw_indirect_multi, {.load_all_groups = true}),
   CP(SKIP_IB2_ENABLE_GLOBAL, cp_skip_ib2_enable_global),
            /* for a6xx */
   CP(LOAD_STATE6_GEOM, cp_load_state),
   CP(LOAD_STATE6_FRAG, cp_load_state),
   CP(LOAD_STATE6, cp_load_state),
   CP(SET_MODE, cp_set_mode),
   CP(SET_MARKER, cp_set_marker),
   CP(REG_WRITE, cp_reg_write),
                                       /* for a7xx */
   CP(THREAD_CONTROL, cp_set_thread_control),
      };
      static void
   noop_fxn(uint32_t *dwords, uint32_t sizedwords, int level)
   {
   }
      static const struct type3_op *
   get_type3_op(unsigned opc)
   {
      static const struct type3_op dummy_op = {
         };
            if (!name)
            for (unsigned i = 0; i < ARRAY_SIZE(type3_op); i++)
      if (!strcmp(name, type3_op[i].name))
            }
      void
   dump_commands(uint32_t *dwords, uint32_t sizedwords, int level)
   {
      int dwords_left = sizedwords;
   uint32_t count = 0; /* dword count including packet header */
            //	assert(dwords);
   if (!dwords) {
      printf("NULL cmd buffer!\n");
               assert(ib < ARRAY_SIZE(draws));
                                 /* hack, this looks like a -1 underflow, in some versions
   * when it tries to write zero registers via pkt0
   */
   //		if ((dwords[0] >> 16) == 0xffff)
            if (pkt_is_regwrite(dwords[0], &val, &count)) {
      assert(val < regcnt());
   printl(3, "%swrite %s (%04x)\n", levels[level + 1], regname(val, 1),
         dump_registers(val, dwords + 1, count - 1, level + 2);
      #if 0
         } else if (pkt_is_type1(dwords[0])) {
      count = 3;
   val = dwords[0] & 0xfff;
   printl(3, "%swrite %s\n", levels[level+1], regname(val, 1));
   dump_registers(val, dwords+1, 1, level+2);
   val = (dwords[0] >> 12) & 0xfff;
   printl(3, "%swrite %s\n", levels[level+1], regname(val, 1));
   dump_registers(val, dwords+2, 1, level+2);
      #endif
         } else if (pkt_is_opcode(dwords[0], &val, &count)) {
      const struct type3_op *op = get_type3_op(val);
   if (op->options.load_all_groups)
         const char *name = pktname(val);
   if (!quiet(2)) {
      printf("\t%sopcode: %s%s%s (%02x) (%d dwords)\n", levels[level],
            }
   if (name) {
      /* special hack for two packets that decode the same way
   * on a6xx:
   */
   if (!strcmp(name, "CP_LOAD_STATE6_FRAG") ||
      !strcmp(name, "CP_LOAD_STATE6_GEOM"))
         }
   op->fxn(dwords + 1, count - 1, level + 1);
   if (!quiet(2))
      } else if (pkt_is_type2(dwords[0])) {
      printl(3, "%snop\n", levels[level + 1]);
      } else {
      printf("bad type! %08x\n", dwords[0]);
   /* for 5xx+ we can do a passable job of looking for start of next valid
   * packet: */
   if (options->info->chip >= 5) {
         } else {
                     dwords += count;
               if (dwords_left < 0)
      }
