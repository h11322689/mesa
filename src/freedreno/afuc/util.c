   /*
   * Copyright © 2021 Google, Inc.
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
      #include <err.h>
   #include <stdarg.h>
   #include <stdio.h>
   #include <string.h>
      #include "rnn.h"
   #include "rnndec.h"
      #include "afuc.h"
   #include "util.h"
      static struct rnndeccontext *ctx;
   static struct rnndb *db;
   static struct rnndomain *control_regs;
   static struct rnndomain *pipe_regs;
   struct rnndomain *dom[2];
   static struct rnnenum *pm4_packets;
      static int
   find_reg(struct rnndomain *dom, const char *name)
   {
      for (int i = 0; i < dom->subelemsnum; i++)
      if (!strcmp(name, dom->subelems[i]->name))
            }
      static unsigned
   reg(struct rnndomain *dom, const char *type, const char *name)
   {
      int val = find_reg(dom, name);
   if (val < 0) {
      char *endptr = NULL;
   val = strtol(name, &endptr, 0);
   if (endptr && *endptr) {
      printf("invalid %s reg: %s\n", type, name);
         }
      }
      static char *
   reg_name(struct rnndomain *dom, unsigned id)
   {
      if (rnndec_checkaddr(ctx, dom, id, 0)) {
      struct rnndecaddrinfo *info = rnndec_decodeaddr(ctx, dom, id, 0);
   char *name = info->name;
   free(info);
      } else {
            }
      /**
   * Map control reg name to offset.
   */
   unsigned
   afuc_control_reg(const char *name)
   {
         }
      /**
   * Map offset to control reg name (or NULL), caller frees
   */
   char *
   afuc_control_reg_name(unsigned id)
   {
         }
      /**
   * Map pipe reg name to offset.
   */
   unsigned
   afuc_pipe_reg(const char *name)
   {
         }
      /**
   * "void" pipe regs don't have a value written, the $addr right is
   * enough to trigger what they do
   */
   bool
   afuc_pipe_reg_is_void(unsigned id)
   {
      if (rnndec_checkaddr(ctx, pipe_regs, id, 0)) {
      struct rnndecaddrinfo *info = rnndec_decodeaddr(ctx, pipe_regs, id, 0);
   free(info->name);
   bool ret = !strcmp(info->typeinfo->name, "void");
   free(info);
      } else {
            }
      /**
   * Map offset to pipe reg name (or NULL), caller frees
   */
   char *
   afuc_pipe_reg_name(unsigned id)
   {
         }
      /**
   * Map GPU reg name to offset.
   */
   unsigned
   afuc_gpu_reg(const char *name)
   {
      int val = find_reg(dom[0], name);
   if (val < 0)
         if (val < 0) {
      char *endptr = NULL;
   val = strtol(name, &endptr, 0);
   if (endptr && *endptr) {
      printf("invalid control reg: %s\n", name);
         }
      }
      /**
   * Map offset to gpu reg name (or NULL), caller frees
   */
   char *
   afuc_gpu_reg_name(unsigned id)
   {
               if (rnndec_checkaddr(ctx, dom[0], id, 0)) {
         } else if (rnndec_checkaddr(ctx, dom[1], id, 0)) {
                  if (d) {
      struct rnndecaddrinfo *info = rnndec_decodeaddr(ctx, d, id, 0);
   if (info) {
      char *name = info->name;
   free(info);
                     }
      unsigned
   afuc_gpr_reg(const char *name)
   {
      /* If it starts with '$' just swallow it: */
   if (name[0] == '$')
            /* handle aliases: */
   if (!strcmp(name, "rem")) {
         } else if (!strcmp(name, "memdata")) {
         } else if (!strcmp(name, "addr")) {
         } else if (!strcmp(name, "regdata")) {
         } else if (!strcmp(name, "usraddr")) {
         } else if (!strcmp(name, "data")) {
         } else {
      char *endptr = NULL;
   unsigned val = strtol(name, &endptr, 16);
   if (endptr && *endptr) {
      printf("invalid gpr reg: %s\n", name);
      }
         }
      static int
   find_enum_val(struct rnnenum *en, const char *name)
   {
               for (i = 0; i < en->valsnum; i++)
      if (en->vals[i]->valvalid && !strcmp(name, en->vals[i]->name))
            }
      /**
   * Map pm4 packet name to id
   */
   int
   afuc_pm4_id(const char *name)
   {
         }
      const char *
   afuc_pm_id_name(unsigned id)
   {
         }
      void
   afuc_printc(enum afuc_color c, const char *fmt, ...)
   {
      va_list args;
   if (c == AFUC_ERR) {
         } else if (c == AFUC_LBL) {
         }
   va_start(args, fmt);
   vprintf(fmt, args);
   va_end(args);
      }
      int afuc_util_init(int gpuver, bool colors)
   {
      char *name, *control_reg_name, *variant;
            switch (gpuver) {
   case 7:
      name = "A6XX";
   variant = "A7XX";
   control_reg_name = "A7XX_CONTROL_REG";
   pipe_reg_name = "A7XX_PIPE_REG";
      case 6:
      name = "A6XX";
   variant = "A6XX";
   control_reg_name = "A6XX_CONTROL_REG";
   pipe_reg_name = "A6XX_PIPE_REG";
      case 5:
      name = "A5XX";
   variant = "A5XX";
   control_reg_name = "A5XX_CONTROL_REG";
   pipe_reg_name = "A5XX_PIPE_REG";
      default:
      fprintf(stderr, "unknown GPU version!\n");
               rnn_init();
            ctx = rnndec_newcontext(db);
            rnn_parsefile(db, "adreno.xml");
   rnn_prepdb(db);
   if (db->estatus)
         dom[0] = rnn_finddomain(db, name);
   dom[1] = rnn_finddomain(db, "AXXX");
   control_regs = rnn_finddomain(db, control_reg_name);
                                 }
   