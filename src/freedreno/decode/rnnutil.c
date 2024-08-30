   /* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */
      /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include <assert.h>
   #include <err.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
      #include "rnnutil.h"
      static struct rnndomain *
   finddom(struct rnn *rnn, uint32_t regbase)
   {
      if (rnndec_checkaddr(rnn->vc, rnn->dom[0], regbase, 0))
            }
      void
   _rnn_init(struct rnn *rnn, int nocolor)
   {
               rnn->db = rnn_newdb();
   rnn->vc_nocolor = rnndec_newcontext(rnn->db);
   rnn->vc_nocolor->colors = &envy_null_colors;
   if (nocolor) {
         } else {
      rnn->vc = rnndec_newcontext(rnn->db);
         }
      struct rnn *
   rnn_new(int nocolor)
   {
               if (!rnn)
                        }
      static void
   init(struct rnn *rnn, char *file, char *domain, char *variant)
   {
      /* prepare rnn stuff for lookup */
   rnn_parsefile(rnn->db, file);
   rnn_prepdb(rnn->db);
   rnn->dom[0] = rnn_finddomain(rnn->db, domain);
   if ((strcmp(domain, "A2XX") == 0) || (strcmp(domain, "A3XX") == 0)) {
         } else {
         }
   if (!rnn->dom[0] && rnn->dom[1]) {
         }
            rnndec_varadd(rnn->vc, "chip", variant);
   if (rnn->vc != rnn->vc_nocolor)
         if (rnn->db->estatus)
      }
      void
   rnn_load_file(struct rnn *rnn, char *file, char *domain)
   {
         }
      void
   rnn_load(struct rnn *rnn, const char *gpuname)
   {
      if (strstr(gpuname, "a2")) {
         } else if (strstr(gpuname, "a3")) {
         } else if (strstr(gpuname, "a4")) {
         } else if (strstr(gpuname, "a5")) {
         } else if (strstr(gpuname, "a6")) {
         } else if (strstr(gpuname, "a7")) {
            }
      uint32_t
   rnn_regbase(struct rnn *rnn, const char *name)
   {
      uint32_t regbase = rnndec_decodereg(rnn->vc_nocolor, rnn->dom[0], name);
   if (!regbase)
            }
      const char *
   rnn_regname(struct rnn *rnn, uint32_t regbase, int color)
   {
      static char buf[128];
            info = rnndec_decodeaddr(color ? rnn->vc : rnn->vc_nocolor,
         if (info) {
      strcpy(buf, info->name);
   free(info->name);
   free(info);
      }
      }
      /* call rnn_reginfo_free() to free the result */
   struct rnndecaddrinfo *
   rnn_reginfo(struct rnn *rnn, uint32_t regbase)
   {
         }
      void
   rnn_reginfo_free(struct rnndecaddrinfo *info)
   {
      if (!info)
         free(info->name);
      }
      const char *
   rnn_enumname(struct rnn *rnn, const char *name, uint32_t val)
   {
         }
      static struct rnndelem *
   regelem(struct rnndomain *domain, const char *name)
   {
      int i;
   for (i = 0; i < domain->subelemsnum; i++) {
      struct rnndelem *elem = domain->subelems[i];
   if (!strcmp(elem->name, name))
      }
      }
      /* Lookup rnndelem by name: */
   struct rnndelem *
   rnn_regelem(struct rnn *rnn, const char *name)
   {
      struct rnndelem *elem = regelem(rnn->dom[0], name);
   if (elem)
            }
      static struct rnndelem *
   regoff(struct rnndomain *domain, uint32_t offset)
   {
      int i;
   for (i = 0; i < domain->subelemsnum; i++) {
      struct rnndelem *elem = domain->subelems[i];
   if (elem->offset == offset)
      }
      }
      /* Lookup rnndelem by offset: */
   struct rnndelem *
   rnn_regoff(struct rnn *rnn, uint32_t offset)
   {
      struct rnndelem *elem = regoff(rnn->dom[0], offset);
   if (elem)
            }
      enum rnnttype
   rnn_decodelem(struct rnn *rnn, struct rnntypeinfo *info, uint64_t regval,
         {
      val->u = regval;
   switch (info->type) {
   case RNN_TTYPE_INLINE_ENUM:
   case RNN_TTYPE_ENUM:
   case RNN_TTYPE_HEX:
   case RNN_TTYPE_INT:
   case RNN_TTYPE_UINT:
   case RNN_TTYPE_FLOAT:
   case RNN_TTYPE_BOOLEAN:
         case RNN_TTYPE_FIXED:
   case RNN_TTYPE_UFIXED:
         default:
            }
