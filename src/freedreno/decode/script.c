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
      #define LUA_COMPAT_APIINTCASTS
      #include <assert.h>
   #include <lauxlib.h>
   #include <lua.h>
   #include <lualib.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include "util/u_math.h"
      #include "cffdec.h"
   #include "rnnutil.h"
   #include "script.h"
      static lua_State *L;
      #if 0
   #define DBG(fmt, ...)                                                          \
      do {                                                                        \
            #else
   #define DBG(fmt, ...)                                                          \
      do {                                                                        \
      #endif
      /* An rnn based decoder, which can either be decoding current register
   * values, or domain based decoding of a pm4 packet.
   *
   */
   struct rnndec {
               /* for pm4 packet decoding: */
   uint32_t sizedwords;
      };
      static inline struct rnndec *
   to_rnndec(struct rnn *rnn)
   {
         }
      static uint32_t
   rnn_val(struct rnn *rnn, uint32_t regbase)
   {
               if (!rnndec->sizedwords) {
         } else if (regbase < rnndec->sizedwords) {
         } else {
      // XXX throw an error
         }
      /* does not return */
   static void
   error(const char *fmt)
   {
      fprintf(stderr, fmt, lua_tostring(L, -1));
      }
      /*
   * An enum type that can be used as string or number:
   */
      struct rnndenum {
      const char *str;
      };
      static int
   l_meta_rnn_enum_tostring(lua_State *L)
   {
      struct rnndenum *e = lua_touserdata(L, 1);
   if (e->str) {
         } else {
      char buf[32];
   sprintf(buf, "%u", e->val);
      }
      }
      /* so, this doesn't actually seem to be implemented yet, but hopefully
   * some day lua comes to it's senses
   */
   static int
   l_meta_rnn_enum_tonumber(lua_State *L)
   {
      struct rnndenum *e = lua_touserdata(L, 1);
   lua_pushinteger(L, e->val);
      }
      static const struct luaL_Reg l_meta_rnn_enum[] = {
      {"__tostring", l_meta_rnn_enum_tostring},
   {"__tonumber", l_meta_rnn_enum_tonumber},
      };
      static void
   pushenum(struct lua_State *L, int val, struct rnnenum *info)
   {
               e->val = val;
            for (int i = 0; i < info->valsnum; i++) {
      if (info->vals[i]->valvalid && (info->vals[i]->value == val)) {
      e->str = info->vals[i]->name;
                  luaL_newmetatable(L, "rnnmetaenum");
   luaL_setfuncs(L, l_meta_rnn_enum, 0);
               }
      /* Expose rnn decode to script environment as "rnn" library:
   */
      struct rnndoff {
      struct rnn *rnn;
   struct rnndelem *elem;
      };
      static void
   push_rnndoff(lua_State *L, struct rnn *rnn, struct rnndelem *elem,
         {
      struct rnndoff *rnndoff = lua_newuserdata(L, sizeof(*rnndoff));
   rnndoff->rnn = rnn;
   rnndoff->elem = elem;
      }
      static int l_rnn_etype_array(lua_State *L, struct rnn *rnn,
         static int l_rnn_etype_reg(lua_State *L, struct rnn *rnn, struct rnndelem *elem,
            static int
   pushdecval(struct lua_State *L, struct rnn *rnn, uint64_t regval,
         {
      union rnndecval val;
   switch (rnn_decodelem(rnn, info, regval, &val)) {
   case RNN_TTYPE_ENUM:
   case RNN_TTYPE_INLINE_ENUM:
      pushenum(L, val.i, info->eenum);
      case RNN_TTYPE_INT:
      lua_pushinteger(L, val.i);
      case RNN_TTYPE_UINT:
   case RNN_TTYPE_HEX:
      lua_pushunsigned(L, val.u);
      case RNN_TTYPE_FLOAT:
      lua_pushnumber(L, uif(val.u));
      case RNN_TTYPE_BOOLEAN:
      lua_pushboolean(L, val.u);
      case RNN_TTYPE_INVALID:
   default:
            }
      static int
   l_rnn_etype(lua_State *L, struct rnn *rnn, struct rnndelem *elem,
         {
      int ret;
   uint64_t regval;
   DBG("elem=%p (%d), offset=%lu", elem, elem->type, offset);
   switch (elem->type) {
   case RNN_ETYPE_REG:
      /* if a register has no bitfields, just return
   * the raw value:
   */
   regval = rnn_val(rnn, offset);
   if (elem->width == 64)
         regval <<= elem->typeinfo.shr;
   ret = pushdecval(L, rnn, regval, &elem->typeinfo);
   if (ret)
            case RNN_ETYPE_ARRAY:
         default:
      /* hmm.. */
   printf("unhandled type: %d\n", elem->type);
         }
      /*
   * Struct Object:
   * To implement stuff like 'RB_MRT[n].CONTROL' we need a struct-object
   * to represent the current array index (ie. 'RB_MRT[n]')
   */
      static int
   l_rnn_struct_meta_index(lua_State *L)
   {
      struct rnndoff *rnndoff = lua_touserdata(L, 1);
   const char *name = lua_tostring(L, 2);
   struct rnndelem *elem = rnndoff->elem;
            for (i = 0; i < elem->subelemsnum; i++) {
      struct rnndelem *subelem = elem->subelems[i];
   if (!strcmp(name, subelem->name)) {
      return l_rnn_etype(L, rnndoff->rnn, subelem,
                     }
      static const struct luaL_Reg l_meta_rnn_struct[] = {
         };
      static int
   l_rnn_etype_struct(lua_State *L, struct rnn *rnn, struct rnndelem *elem,
         {
               luaL_newmetatable(L, "rnnmetastruct");
   luaL_setfuncs(L, l_meta_rnn_struct, 0);
                        }
      /*
   * Array Object:
   */
      static int
   l_rnn_array_meta_index(lua_State *L)
   {
      struct rnndoff *rnndoff = lua_touserdata(L, 1);
   int idx = lua_tointeger(L, 2);
   struct rnndelem *elem = rnndoff->elem;
            DBG("rnndoff=%p, idx=%d, numsubelems=%d", rnndoff, idx,
            /* if just a single sub-element, it is directly a register,
   * otherwise we need to accumulate the array index while
   * we wait for the register name within the array..
   */
   if (elem->subelemsnum == 1) {
         } else {
                     }
      static const struct luaL_Reg l_meta_rnn_array[] = {
         };
      static int
   l_rnn_etype_array(lua_State *L, struct rnn *rnn, struct rnndelem *elem,
         {
               luaL_newmetatable(L, "rnnmetaarray");
   luaL_setfuncs(L, l_meta_rnn_array, 0);
                        }
      /*
   * Register element:
   */
      static int
   l_rnn_reg_meta_index(lua_State *L)
   {
      struct rnndoff *rnndoff = lua_touserdata(L, 1);
   const char *name = lua_tostring(L, 2);
   struct rnndelem *elem = rnndoff->elem;
   struct rnntypeinfo *info = &elem->typeinfo;
   struct rnnbitfield **bitfields;
   int bitfieldsnum;
            switch (info->type) {
   case RNN_TTYPE_BITSET:
      bitfields = info->ebitset->bitfields;
   bitfieldsnum = info->ebitset->bitfieldsnum;
      case RNN_TTYPE_INLINE_BITSET:
      bitfields = info->bitfields;
   bitfieldsnum = info->bitfieldsnum;
      default:
      printf("invalid register type: %d\n", info->type);
               for (i = 0; i < bitfieldsnum; i++) {
      struct rnnbitfield *bf = bitfields[i];
   if (!strcmp(name, bf->name)) {
               regval &= typeinfo_mask(&bf->typeinfo);
                                                printf("invalid member: %s\n", name);
      }
      static int
   l_rnn_reg_meta_tostring(lua_State *L)
   {
      struct rnndoff *rnndoff = lua_touserdata(L, 1);
   uint32_t regval = rnn_val(rnndoff->rnn, rnndoff->offset);
   struct rnndecaddrinfo *info = rnn_reginfo(rnndoff->rnn, rnndoff->offset);
   char *decoded;
   if (info && info->typeinfo) {
         } else {
         }
   lua_pushstring(L, decoded);
   free(decoded);
   rnn_reginfo_free(info);
      }
      static int
   l_rnn_reg_meta_tonumber(lua_State *L)
   {
      struct rnndoff *rnndoff = lua_touserdata(L, 1);
                     lua_pushnumber(L, regval);
      }
      static const struct luaL_Reg l_meta_rnn_reg[] = {
      {"__index", l_rnn_reg_meta_index},
   {"__tostring", l_rnn_reg_meta_tostring},
   {"__tonumber", l_rnn_reg_meta_tonumber},
      };
      static int
   l_rnn_etype_reg(lua_State *L, struct rnn *rnn, struct rnndelem *elem,
         {
               luaL_newmetatable(L, "rnnmetareg");
   luaL_setfuncs(L, l_meta_rnn_reg, 0);
                        }
      /*
   *
   */
      static int
   l_rnn_meta_index(lua_State *L)
   {
      struct rnn *rnn = lua_touserdata(L, 1);
   const char *name = lua_tostring(L, 2);
            elem = rnn_regelem(rnn, name);
   if (!elem)
               }
      static int
   l_rnn_meta_gc(lua_State *L)
   {
      // TODO
   // struct rnn *rnn = lua_touserdata(L, 1);
   // rnn_deinit(rnn);
      }
      static const struct luaL_Reg l_meta_rnn[] = {
      {"__index", l_rnn_meta_index},
   {"__gc", l_rnn_meta_gc},
      };
      static int
   l_rnn_init(lua_State *L)
   {
      const char *gpuname = lua_tostring(L, 1);
   struct rnndec *rnndec = lua_newuserdata(L, sizeof(*rnndec));
   _rnn_init(&rnndec->base, 0);
   rnn_load(&rnndec->base, gpuname);
            luaL_newmetatable(L, "rnnmeta");
   luaL_setfuncs(L, l_meta_rnn, 0);
                        }
      static int
   l_rnn_enumname(lua_State *L)
   {
      struct rnn *rnn = lua_touserdata(L, 1);
   const char *name = lua_tostring(L, 2);
   uint32_t val = (uint32_t)lua_tonumber(L, 3);
   lua_pushstring(L, rnn_enumname(rnn, name, val));
      }
      static int
   l_rnn_regname(lua_State *L)
   {
      struct rnn *rnn = lua_touserdata(L, 1);
   uint32_t regbase = (uint32_t)lua_tonumber(L, 2);
   lua_pushstring(L, rnn_regname(rnn, regbase, 1));
      }
      static int
   l_rnn_regval(lua_State *L)
   {
      struct rnn *rnn = lua_touserdata(L, 1);
   uint32_t regbase = (uint32_t)lua_tonumber(L, 2);
   uint32_t regval = (uint32_t)lua_tonumber(L, 3);
   struct rnndecaddrinfo *info = rnn_reginfo(rnn, regbase);
   char *decoded;
   if (info && info->typeinfo) {
         } else {
         }
   lua_pushstring(L, decoded);
   free(decoded);
   rnn_reginfo_free(info);
      }
      static const struct luaL_Reg l_rnn[] = {
      {"init", l_rnn_init},
   {"enumname", l_rnn_enumname},
   {"regname", l_rnn_regname},
   {"regval", l_rnn_regval},
      };
      /* Expose the register state to script enviroment as a "regs" library:
   */
      static int
   l_reg_written(lua_State *L)
   {
      uint32_t regbase = (uint32_t)lua_tonumber(L, 1);
   lua_pushnumber(L, reg_written(regbase));
      }
      static int
   l_reg_lastval(lua_State *L)
   {
      uint32_t regbase = (uint32_t)lua_tonumber(L, 1);
   lua_pushnumber(L, reg_lastval(regbase));
      }
      static int
   l_reg_val(lua_State *L)
   {
      uint32_t regbase = (uint32_t)lua_tonumber(L, 1);
   lua_pushnumber(L, reg_val(regbase));
      }
      static const struct luaL_Reg l_regs[] = {
      {"written", l_reg_written},
   {"lastval", l_reg_lastval},
   {"val", l_reg_val},
      };
      /* Expose API to lookup snapshot buffers:
   */
      uint64_t gpubaseaddr(uint64_t gpuaddr);
   unsigned hostlen(uint64_t gpuaddr);
      /* given address, return base-address of buffer: */
   static int
   l_bo_base(lua_State *L)
   {
      uint64_t addr = (uint64_t)lua_tonumber(L, 1);
   lua_pushnumber(L, gpubaseaddr(addr));
      }
      /* given address, return the remaining size of the buffer: */
   static int
   l_bo_size(lua_State *L)
   {
      uint64_t addr = (uint64_t)lua_tonumber(L, 1);
   lua_pushnumber(L, hostlen(addr));
      }
      static const struct luaL_Reg l_bos[] = {
         };
      static void
   openlib(const char *lib, const luaL_Reg *reg)
   {
      lua_newtable(L);
   luaL_setfuncs(L, reg, 0);
      }
      /* called at start to load the script: */
   int
   script_load(const char *file)
   {
                        L = luaL_newstate();
   luaL_openlibs(L);
   openlib("bos", l_bos);
   openlib("regs", l_regs);
            ret = luaL_loadfile(L, file);
   if (ret)
            ret = lua_pcall(L, 0, LUA_MULTRET, 0);
   if (ret)
               }
      /* called at start of each cmdstream file: */
   void
   script_start_cmdstream(const char *name)
   {
      if (!L)
                     /* if no handler just ignore it: */
   if (!lua_isfunction(L, -1)) {
      lua_pop(L, 1);
                        /* do the call (1 arguments, 0 result) */
   if (lua_pcall(L, 1, 0, 0) != 0)
      }
      /* called at each DRAW_INDX, calls script drawidx fxn to process
   * the current state
   */
   void
   script_draw(const char *primtype, uint32_t nindx)
   {
      if (!L)
                     /* if no handler just ignore it: */
   if (!lua_isfunction(L, -1)) {
      lua_pop(L, 1);
               lua_pushstring(L, primtype);
            /* do the call (2 arguments, 0 result) */
   if (lua_pcall(L, 2, 0, 0) != 0)
      }
      static int
   l_rnn_meta_dom_index(lua_State *L)
   {
      struct rnn *rnn = lua_touserdata(L, 1);
   uint32_t offset = (uint32_t)lua_tonumber(L, 2);
            /* TODO might be nicer if the arg isn't a number, to search the domain
   * for matching bitfields.. so that the script could do something like
   * 'pkt.WIDTH' insteadl of 'pkt[1].WIDTH', ie. not have to remember the
   * offset of the dword containing the bitfield..
            elem = rnn_regoff(rnn, offset);
   if (!elem)
               }
      /*
   * A wrapper object for rnndomain based decoding of an array of dwords
   * (ie. for pm4 packet decoding).  Mostly re-uses the register-value
   * decoding for the individual dwords and bitfields.
   */
      static int
   l_rnn_meta_dom_gc(lua_State *L)
   {
      // TODO
   // struct rnn *rnn = lua_touserdata(L, 1);
   // rnn_deinit(rnn);
      }
      static const struct luaL_Reg l_meta_rnn_dom[] = {
      {"__index", l_rnn_meta_dom_index},
   {"__gc", l_rnn_meta_dom_gc},
      };
      /* called to general pm4 packet decoding, such as texture/sampler state
   */
   void
   script_packet(uint32_t *dwords, uint32_t sizedwords, struct rnn *rnn,
         {
      if (!L)
                     /* if no handler for the packet, just ignore it: */
   if (!lua_isfunction(L, -1)) {
      lua_pop(L, 1);
                        rnndec->base = *rnn;
   rnndec->base.dom[0] = dom;
   rnndec->base.dom[1] = NULL;
   rnndec->dwords = dwords;
            luaL_newmetatable(L, "rnnmetadom");
   luaL_setfuncs(L, l_meta_rnn_dom, 0);
                              if (lua_pcall(L, 2, 0, 0) != 0)
      }
      /* helper to call fxn that takes and returns void: */
   static void
   simple_call(const char *name)
   {
      if (!L)
                     /* if no handler just ignore it: */
   if (!lua_isfunction(L, -1)) {
      lua_pop(L, 1);
               /* do the call (0 arguments, 0 result) */
   if (lua_pcall(L, 0, 0, 0) != 0)
      }
      /* called at end of each cmdstream file: */
   void
   script_end_cmdstream(void)
   {
         }
      /* called at start of submit/issueibcmds: */
   void
   script_start_submit(void)
   {
         }
      /* called at end of submit/issueibcmds: */
   void
   script_end_submit(void)
   {
         }
      /* called after last cmdstream file: */
   void
   script_finish(void)
   {
      if (!L)
                     lua_close(L);
      }
