   /*
   * Copyright 2003 VMware, Inc.
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   * VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Keith Whitwell <keithw@vmware.com>
   */
         #include "util/detect.h"
   #include "util/compiler.h"
   #include "util/u_memory.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_math.h"
   #include "util/format/u_format.h"
      #include "translate.h"
         #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
      #include "rtasm/rtasm_x86sse.h"
         #define X    0
   #define Y    1
   #define Z    2
   #define W    3
         struct translate_buffer
   {
      const void *base_ptr;
   uintptr_t stride;
      };
      struct translate_buffer_variant
   {
      unsigned buffer_index;
   unsigned instance_divisor;
      };
         #define ELEMENT_BUFFER_INSTANCE_ID  1001
      #define NUM_FLOAT_CONSTS 9
   #define NUM_UNSIGNED_CONSTS 1
      enum
   {
      CONST_IDENTITY,
   CONST_INV_127,
   CONST_INV_255,
   CONST_INV_32767,
   CONST_INV_65535,
   CONST_INV_2147483647,
   CONST_INV_4294967295,
   CONST_255,
   CONST_2147483648,
   /* float consts end */
      };
      #define C(v) {(float)(v), (float)(v), (float)(v), (float)(v)}
   static float consts[NUM_FLOAT_CONSTS][4] = {
      {0, 0, 0, 1},
   C(1.0 / 127.0),
   C(1.0 / 255.0),
   C(1.0 / 32767.0),
   C(1.0 / 65535.0),
   C(1.0 / 2147483647.0),
   C(1.0 / 4294967295.0),
   C(255.0),
      };
      #undef C
      static unsigned uconsts[NUM_UNSIGNED_CONSTS][4] = {
         };
      struct translate_sse
   {
               struct x86_function linear_func;
   struct x86_function elt_func;
   struct x86_function elt16_func;
   struct x86_function elt8_func;
            alignas(16) float consts[NUM_FLOAT_CONSTS][4];
   alignas(16) float uconsts[NUM_UNSIGNED_CONSTS][4];
   int8_t reg_to_const[16];
            struct translate_buffer buffer[TRANSLATE_MAX_ATTRIBS];
            /* Multiple buffer variants can map to a single buffer. */
   struct translate_buffer_variant buffer_variant[TRANSLATE_MAX_ATTRIBS];
            /* Multiple elements can map to a single buffer variant. */
            bool use_instancing;
   unsigned instance_id;
            /* these are actually known values, but putting them in a struct
   * like this is helpful to keep them in sync across the file.
   */
   struct x86_reg tmp_EAX;
   struct x86_reg tmp2_EDX;
   struct x86_reg src_ECX;
   struct x86_reg idx_ESI;      /* either start+i or &elt[i] */
   struct x86_reg machine_EDI;
   struct x86_reg outbuf_EBX;
      };
         static int
   get_offset(const void *a, const void *b)
   {
         }
         static struct x86_reg
   get_const(struct translate_sse *p, unsigned id)
   {
      struct x86_reg reg;
            if (p->const_to_reg[id] >= 0)
            for (i = 2; i < 8; ++i) {
      if (p->reg_to_const[i] < 0)
               /* TODO: be smarter here */
   if (i == 8)
                     if (p->reg_to_const[i] >= 0)
            p->reg_to_const[i] = id;
            /* TODO: this should happen outside the loop, if possible */
   const void *c;
   if (id < NUM_FLOAT_CONSTS)
         else
         sse_movaps(p->func, reg,
               }
         /* load the data in a SSE2 register, padding with zeros */
   static bool
   emit_load_sse2(struct translate_sse *p,
         {
      struct x86_reg tmpXMM = x86_make_reg(file_XMM, 1);
   struct x86_reg tmp = p->tmp_EAX;
   switch (size) {
   case 1:
      x86_movzx8(p->func, tmp, src);
   sse2_movd(p->func, data, tmp);
      case 2:
      x86_movzx16(p->func, tmp, src);
   sse2_movd(p->func, data, tmp);
      case 3:
      x86_movzx8(p->func, tmp, x86_make_disp(src, 2));
   x86_shl_imm(p->func, tmp, 16);
   x86_mov16(p->func, tmp, src);
   sse2_movd(p->func, data, tmp);
      case 4:
      sse2_movd(p->func, data, src);
      case 6:
      sse2_movd(p->func, data, src);
   x86_movzx16(p->func, tmp, x86_make_disp(src, 4));
   sse2_movd(p->func, tmpXMM, tmp);
   sse2_punpckldq(p->func, data, tmpXMM);
      case 8:
      sse2_movq(p->func, data, src);
      case 12:
      sse2_movq(p->func, data, src);
   sse2_movd(p->func, tmpXMM, x86_make_disp(src, 8));
   sse2_punpcklqdq(p->func, data, tmpXMM);
      case 16:
      sse2_movdqu(p->func, data, src);
      default:
         }
      }
         /* this value can be passed for the out_chans argument */
   #define CHANNELS_0001 5
         /* this function will load #chans float values, and will
   * pad the register with zeroes at least up to out_chans.
   *
   * If out_chans is set to CHANNELS_0001, then the fourth
   * value will be padded with 1. Only pass this value if
   * chans < 4 or results are undefined.
   */
   static void
   emit_load_float32(struct translate_sse *p, struct x86_reg data,
         {
      switch (chans) {
   case 1:
      /* a 0 0 0
   * a 0 0 1
   */
   sse_movss(p->func, data, arg0);
   if (out_chans == CHANNELS_0001)
            case 2:
      /* 0 0 0 1
   * a b 0 1
   */
   if (out_chans == CHANNELS_0001)
      sse_shufps(p->func, data, get_const(p, CONST_IDENTITY),
      else if (out_chans > 2)
         sse_movlps(p->func, data, arg0);
      case 3:
      /* Have to jump through some hoops:
   *
   * c 0 0 0
   * c 0 0 1 if out_chans == CHANNELS_0001
   * 0 0 c 0/1
   * a b c 0/1
   */
   sse_movss(p->func, data, x86_make_disp(arg0, 8));
   if (out_chans == CHANNELS_0001)
      sse_shufps(p->func, data, get_const(p, CONST_IDENTITY),
      sse_shufps(p->func, data, data, SHUF(Y, Z, X, W));
   sse_movlps(p->func, data, arg0);
      case 4:
      sse_movups(p->func, data, arg0);
         }
      /* this function behaves like emit_load_float32, but loads
         ones */
   static void
   emit_load_float64to32(struct translate_sse *p, struct x86_reg data,
         {
      struct x86_reg tmpXMM = x86_make_reg(file_XMM, 1);
   switch (chans) {
   case 1:
      sse2_movsd(p->func, data, arg0);
   if (out_chans > 1)
         else
         if (out_chans == CHANNELS_0001)
      sse_shufps(p->func, data, get_const(p, CONST_IDENTITY),
         case 2:
      sse2_movupd(p->func, data, arg0);
   sse2_cvtpd2ps(p->func, data, data);
   if (out_chans == CHANNELS_0001)
      sse_shufps(p->func, data, get_const(p, CONST_IDENTITY),
      else if (out_chans > 2)
            case 3:
      sse2_movupd(p->func, data, arg0);
   sse2_cvtpd2ps(p->func, data, data);
   sse2_movsd(p->func, tmpXMM, x86_make_disp(arg0, 16));
   if (out_chans > 3)
         else
         sse_movlhps(p->func, data, tmpXMM);
   if (out_chans == CHANNELS_0001)
            case 4:
      sse2_movupd(p->func, data, arg0);
   sse2_cvtpd2ps(p->func, data, data);
   sse2_movupd(p->func, tmpXMM, x86_make_disp(arg0, 16));
   sse2_cvtpd2ps(p->func, tmpXMM, tmpXMM);
   sse_movlhps(p->func, data, tmpXMM);
         }
         static void
   emit_mov64(struct translate_sse *p, struct x86_reg dst_gpr,
               {
      if (x86_target(p->func) != X86_32)
         else {
      /* TODO: when/on which CPUs is SSE2 actually better than SSE? */
   if (x86_target_caps(p->func) & X86_SSE2)
         else
         }
         static void
   emit_load64(struct translate_sse *p, struct x86_reg dst_gpr,
         {
         }
         static void
   emit_store64(struct translate_sse *p, struct x86_reg dst,
         {
         }
         static void
   emit_mov128(struct translate_sse *p, struct x86_reg dst, struct x86_reg src)
   {
      if (x86_target_caps(p->func) & X86_SSE2)
         else
      }
         /* TODO: this uses unaligned accesses liberally, which is great on Nehalem,
   * but may or may not be good on older processors
   * TODO: may perhaps want to use non-temporal stores here if possible
   */
   static void
   emit_memcpy(struct translate_sse *p, struct x86_reg dst, struct x86_reg src,
         {
      struct x86_reg dataXMM = x86_make_reg(file_XMM, 0);
   struct x86_reg dataXMM2 = x86_make_reg(file_XMM, 1);
   struct x86_reg dataGPR = p->tmp_EAX;
            if (size < 8) {
      switch (size) {
   case 1:
      x86_mov8(p->func, dataGPR, src);
   x86_mov8(p->func, dst, dataGPR);
      case 2:
      x86_mov16(p->func, dataGPR, src);
   x86_mov16(p->func, dst, dataGPR);
      case 3:
      x86_mov16(p->func, dataGPR, src);
   x86_mov8(p->func, dataGPR2, x86_make_disp(src, 2));
   x86_mov16(p->func, dst, dataGPR);
   x86_mov8(p->func, x86_make_disp(dst, 2), dataGPR2);
      case 4:
      x86_mov(p->func, dataGPR, src);
   x86_mov(p->func, dst, dataGPR);
      case 6:
      x86_mov(p->func, dataGPR, src);
   x86_mov16(p->func, dataGPR2, x86_make_disp(src, 4));
   x86_mov(p->func, dst, dataGPR);
   x86_mov16(p->func, x86_make_disp(dst, 4), dataGPR2);
         }
   else if (!(x86_target_caps(p->func) & X86_SSE)) {
      unsigned i = 0;
   assert((size & 3) == 0);
   for (i = 0; i < size; i += 4) {
      x86_mov(p->func, dataGPR, x86_make_disp(src, i));
         }
   else {
      switch (size) {
   case 8:
      emit_load64(p, dataGPR, dataXMM, src);
   emit_store64(p, dst, dataGPR, dataXMM);
      case 12:
      emit_load64(p, dataGPR2, dataXMM, src);
   x86_mov(p->func, dataGPR, x86_make_disp(src, 8));
   emit_store64(p, dst, dataGPR2, dataXMM);
   x86_mov(p->func, x86_make_disp(dst, 8), dataGPR);
      case 16:
      emit_mov128(p, dataXMM, src);
   emit_mov128(p, dst, dataXMM);
      case 24:
      emit_mov128(p, dataXMM, src);
   emit_load64(p, dataGPR, dataXMM2, x86_make_disp(src, 16));
   emit_mov128(p, dst, dataXMM);
   emit_store64(p, x86_make_disp(dst, 16), dataGPR, dataXMM2);
      case 32:
      emit_mov128(p, dataXMM, src);
   emit_mov128(p, dataXMM2, x86_make_disp(src, 16));
   emit_mov128(p, dst, dataXMM);
   emit_mov128(p, x86_make_disp(dst, 16), dataXMM2);
      default:
               }
      static bool
   translate_attr_convert(struct translate_sse *p,
               {
      const struct util_format_description *input_desc =
         const struct util_format_description *output_desc =
         unsigned i;
   bool id_swizzle = true;
   unsigned swizzle[4] =
      { PIPE_SWIZZLE_NONE, PIPE_SWIZZLE_NONE,
      unsigned needed_chans = 0;
            if (a->output_format == PIPE_FORMAT_NONE
      || a->input_format == PIPE_FORMAT_NONE)
         if (input_desc->channel[0].size & 7)
            if (input_desc->colorspace != output_desc->colorspace)
            for (i = 1; i < input_desc->nr_channels; ++i) {
      if (memcmp
      (&input_desc->channel[i], &input_desc->channel[0],
   sizeof(input_desc->channel[0])))
            for (i = 1; i < output_desc->nr_channels; ++i) {
      if (memcmp
      (&output_desc->channel[i], &output_desc->channel[0],
   sizeof(output_desc->channel[0]))) {
                  for (i = 0; i < output_desc->nr_channels; ++i) {
      if (output_desc->swizzle[i] < 4)
               if ((x86_target_caps(p->func) & X86_SSE) &&
      (0 || a->output_format == PIPE_FORMAT_R32_FLOAT
   || a->output_format == PIPE_FORMAT_R32G32_FLOAT
   || a->output_format == PIPE_FORMAT_R32G32B32_FLOAT
   || a->output_format == PIPE_FORMAT_R32G32B32A32_FLOAT)) {
   struct x86_reg dataXMM = x86_make_reg(file_XMM, 0);
            for (i = 0; i < output_desc->nr_channels; ++i) {
      if (swizzle[i] == PIPE_SWIZZLE_0
      && i >= input_desc->nr_channels)
            for (i = 0; i < output_desc->nr_channels; ++i) {
      if (swizzle[i] < 4)
         if (swizzle[i] < PIPE_SWIZZLE_0 && swizzle[i] != i)
               if (needed_chans > 0) {
      switch (input_desc->channel[0].type) {
   case UTIL_FORMAT_TYPE_UNSIGNED:
      if (!(x86_target_caps(p->func) & X86_SSE2))
         emit_load_sse2(p, dataXMM, src,
                  /* TODO: add support for SSE4.1 pmovzx */
   switch (input_desc->channel[0].size) {
   case 8:
      /* TODO: this may be inefficient due to get_identity() being
   *  used both as a float and integer register.
   */
   sse2_punpcklbw(p->func, dataXMM, get_const(p, CONST_IDENTITY));
   sse2_punpcklbw(p->func, dataXMM, get_const(p, CONST_IDENTITY));
      case 16:
      sse2_punpcklwd(p->func, dataXMM, get_const(p, CONST_IDENTITY));
      case 32:           /* we lose precision here */
      /* No unsigned conversion (except in AVX512F), so we check if
   * it's negative, and stick the high bit as a separate float
   * value in an aux register: */
   auxXMM = x86_make_reg(file_XMM, 1);
   /* aux = 0 */
   sse_xorps(p->func, auxXMM, auxXMM);
   /* aux = aux > data ? 0xffffffff : 0 */
   sse2_pcmpgtd(p->func, auxXMM, dataXMM);
   /* data = data & 0x7fffffff */
   sse_andps(p->func, dataXMM, get_const(p, CONST_2147483647_INT));
   /* aux = aux & 2147483648.0 */
   sse_andps(p->func, auxXMM, get_const(p, CONST_2147483648));
      default:
         }
   sse2_cvtdq2ps(p->func, dataXMM, dataXMM);
   if (input_desc->channel[0].size == 32)
      /* add in the high bit's worth of float that we AND'd away */
      if (input_desc->channel[0].normalized) {
      struct x86_reg factor;
   switch (input_desc->channel[0].size) {
   case 8:
      factor = get_const(p, CONST_INV_255);
      case 16:
      factor = get_const(p, CONST_INV_65535);
      case 32:
      factor = get_const(p, CONST_INV_4294967295);
      default:
      assert(0);
   factor.disp = 0;
   factor.file = 0;
   factor.idx = 0;
   factor.mod = 0;
      }
      }
      case UTIL_FORMAT_TYPE_SIGNED:
      if (!(x86_target_caps(p->func) & X86_SSE2))
         emit_load_sse2(p, dataXMM, src,
                  /* TODO: add support for SSE4.1 pmovsx */
   switch (input_desc->channel[0].size) {
   case 8:
      sse2_punpcklbw(p->func, dataXMM, dataXMM);
   sse2_punpcklbw(p->func, dataXMM, dataXMM);
   sse2_psrad_imm(p->func, dataXMM, 24);
      case 16:
      sse2_punpcklwd(p->func, dataXMM, dataXMM);
   sse2_psrad_imm(p->func, dataXMM, 16);
      case 32:           /* we lose precision here */
         default:
         }
   sse2_cvtdq2ps(p->func, dataXMM, dataXMM);
   if (input_desc->channel[0].normalized) {
      struct x86_reg factor;
   switch (input_desc->channel[0].size) {
   case 8:
      factor = get_const(p, CONST_INV_127);
      case 16:
      factor = get_const(p, CONST_INV_32767);
      case 32:
      factor = get_const(p, CONST_INV_2147483647);
      default:
      assert(0);
   factor.disp = 0;
   factor.file = 0;
   factor.idx = 0;
   factor.mod = 0;
      }
                        case UTIL_FORMAT_TYPE_FLOAT:
      if (input_desc->channel[0].size != 32
      && input_desc->channel[0].size != 64) {
      }
   if (swizzle[3] == PIPE_SWIZZLE_1
      && input_desc->nr_channels <= 3) {
   swizzle[3] = PIPE_SWIZZLE_W;
      }
   switch (input_desc->channel[0].size) {
   case 32:
      emit_load_float32(p, dataXMM, src, needed_chans,
            case 64:           /* we lose precision here */
      if (!(x86_target_caps(p->func) & X86_SSE2))
         emit_load_float64to32(p, dataXMM, src, needed_chans,
            default:
         }
      default:
                  if (!id_swizzle) {
      sse_shufps(p->func, dataXMM, dataXMM,
                  if (output_desc->nr_channels >= 4
      && swizzle[0] < PIPE_SWIZZLE_0
   && swizzle[1] < PIPE_SWIZZLE_0
   && swizzle[2] < PIPE_SWIZZLE_0
   && swizzle[3] < PIPE_SWIZZLE_0) {
      }
   else {
      if (output_desc->nr_channels >= 2
      && swizzle[0] < PIPE_SWIZZLE_0
   && swizzle[1] < PIPE_SWIZZLE_0) {
      }
   else {
      if (swizzle[0] < PIPE_SWIZZLE_0) {
         }
   else {
                        if (output_desc->nr_channels >= 2) {
      if (swizzle[1] < PIPE_SWIZZLE_0) {
      sse_shufps(p->func, dataXMM, dataXMM, SHUF(1, 1, 2, 3));
      }
   else {
      x86_mov_imm(p->func, x86_make_disp(dst, 4),
                     if (output_desc->nr_channels >= 3) {
      if (output_desc->nr_channels >= 4
      && swizzle[2] < PIPE_SWIZZLE_0
   && swizzle[3] < PIPE_SWIZZLE_0) {
      }
   else {
      if (swizzle[2] < PIPE_SWIZZLE_0) {
      sse_shufps(p->func, dataXMM, dataXMM, SHUF(2, 2, 2, 3));
      }
   else {
                        if (output_desc->nr_channels >= 4) {
      if (swizzle[3] < PIPE_SWIZZLE_0) {
      sse_shufps(p->func, dataXMM, dataXMM, SHUF(3, 3, 3, 3));
      }
   else {
      x86_mov_imm(p->func, x86_make_disp(dst, 12),
                  }
      }
   else if ((x86_target_caps(p->func) & X86_SSE2)
            && input_desc->channel[0].size == 8
   && output_desc->channel[0].size == 16
   && output_desc->channel[0].normalized ==
   input_desc->channel[0].normalized &&
   (0 || (input_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED
         || (input_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED
         || (input_desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED
   struct x86_reg dataXMM = x86_make_reg(file_XMM, 0);
   struct x86_reg tmpXMM = x86_make_reg(file_XMM, 1);
   struct x86_reg tmp = p->tmp_EAX;
            for (i = 0; i < output_desc->nr_channels; ++i) {
      if (swizzle[i] == PIPE_SWIZZLE_0
      && i >= input_desc->nr_channels) {
                  for (i = 0; i < output_desc->nr_channels; ++i) {
      if (swizzle[i] < 4)
         if (swizzle[i] < PIPE_SWIZZLE_0 && swizzle[i] != i)
               if (needed_chans > 0) {
      emit_load_sse2(p, dataXMM, src,
                  switch (input_desc->channel[0].type) {
   case UTIL_FORMAT_TYPE_UNSIGNED:
      if (input_desc->channel[0].normalized) {
      sse2_punpcklbw(p->func, dataXMM, dataXMM);
   if (output_desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED)
      }
   else
            case UTIL_FORMAT_TYPE_SIGNED:
      if (input_desc->channel[0].normalized) {
      sse2_movq(p->func, tmpXMM, get_const(p, CONST_IDENTITY));
   sse2_punpcklbw(p->func, tmpXMM, dataXMM);
   sse2_psllw_imm(p->func, dataXMM, 9);
   sse2_psrlw_imm(p->func, dataXMM, 8);
   sse2_por(p->func, tmpXMM, dataXMM);
   sse2_psrlw_imm(p->func, dataXMM, 7);
   sse2_por(p->func, tmpXMM, dataXMM);
   {
      struct x86_reg t = dataXMM;
   dataXMM = tmpXMM;
         }
   else {
      sse2_punpcklbw(p->func, dataXMM, dataXMM);
      }
      default:
                  if (output_desc->channel[0].normalized)
      imms[1] =
               if (!id_swizzle)
      sse2_pshuflw(p->func, dataXMM, dataXMM,
                  if (output_desc->nr_channels >= 4
      && swizzle[0] < PIPE_SWIZZLE_0
   && swizzle[1] < PIPE_SWIZZLE_0
   && swizzle[2] < PIPE_SWIZZLE_0
   && swizzle[3] < PIPE_SWIZZLE_0) {
      }
   else {
      if (swizzle[0] < PIPE_SWIZZLE_0) {
      if (output_desc->nr_channels >= 2
      && swizzle[1] < PIPE_SWIZZLE_0) {
      }
   else {
      sse2_movd(p->func, tmp, dataXMM);
   x86_mov16(p->func, dst, tmp);
   if (output_desc->nr_channels >= 2)
      x86_mov16_imm(p->func, x86_make_disp(dst, 2),
      }
   else {
      if (output_desc->nr_channels >= 2
      && swizzle[1] >= PIPE_SWIZZLE_0) {
   x86_mov_imm(p->func, dst,
            }
   else {
      x86_mov16_imm(p->func, dst,
         if (output_desc->nr_channels >= 2) {
      sse2_movd(p->func, tmp, dataXMM);
   x86_shr_imm(p->func, tmp, 16);
                     if (output_desc->nr_channels >= 3) {
      if (swizzle[2] < PIPE_SWIZZLE_0) {
      if (output_desc->nr_channels >= 4
      && swizzle[3] < PIPE_SWIZZLE_0) {
   sse2_psrlq_imm(p->func, dataXMM, 32);
      }
   else {
      sse2_psrlq_imm(p->func, dataXMM, 32);
   sse2_movd(p->func, tmp, dataXMM);
   x86_mov16(p->func, x86_make_disp(dst, 4), tmp);
   if (output_desc->nr_channels >= 4) {
      x86_mov16_imm(p->func, x86_make_disp(dst, 6),
            }
   else {
      if (output_desc->nr_channels >= 4
      && swizzle[3] >= PIPE_SWIZZLE_0) {
   x86_mov_imm(p->func, x86_make_disp(dst, 4),
            }
                           if (output_desc->nr_channels >= 4) {
      sse2_psrlq_imm(p->func, dataXMM, 48);
   sse2_movd(p->func, tmp, dataXMM);
                  }
      }
   else if (!memcmp(&output_desc->channel[0], &input_desc->channel[0],
            struct x86_reg tmp = p->tmp_EAX;
            if (input_desc->channel[0].size == 8 && input_desc->nr_channels == 4
      && output_desc->nr_channels == 4
   && swizzle[0] == PIPE_SWIZZLE_W
   && swizzle[1] == PIPE_SWIZZLE_Z
   && swizzle[2] == PIPE_SWIZZLE_Y
   && swizzle[3] == PIPE_SWIZZLE_X) {
   /* TODO: support movbe */
   x86_mov(p->func, tmp, src);
   x86_bswap(p->func, tmp);
   x86_mov(p->func, dst, tmp);
               for (i = 0; i < output_desc->nr_channels; ++i) {
      switch (output_desc->channel[0].size) {
   case 8:
      if (swizzle[i] >= PIPE_SWIZZLE_0) {
      unsigned v = 0;
   if (swizzle[i] == PIPE_SWIZZLE_1) {
      switch (output_desc->channel[0].type) {
   case UTIL_FORMAT_TYPE_UNSIGNED:
      v = output_desc->channel[0].normalized ? 0xff : 1;
      case UTIL_FORMAT_TYPE_SIGNED:
      v = output_desc->channel[0].normalized ? 0x7f : 1;
      default:
            }
      }
   else {
      x86_mov8(p->func, tmp, x86_make_disp(src, swizzle[i] * 1));
      }
      case 16:
      if (swizzle[i] >= PIPE_SWIZZLE_0) {
      unsigned v = 0;
   if (swizzle[i] == PIPE_SWIZZLE_1) {
      switch (output_desc->channel[1].type) {
   case UTIL_FORMAT_TYPE_UNSIGNED:
      v = output_desc->channel[1].normalized ? 0xffff : 1;
      case UTIL_FORMAT_TYPE_SIGNED:
      v = output_desc->channel[1].normalized ? 0x7fff : 1;
      case UTIL_FORMAT_TYPE_FLOAT:
      v = 0x3c00;
      default:
            }
      }
   else if (swizzle[i] == PIPE_SWIZZLE_0) {
         }
   else {
      x86_mov16(p->func, tmp, x86_make_disp(src, swizzle[i] * 2));
      }
      case 32:
      if (swizzle[i] >= PIPE_SWIZZLE_0) {
      unsigned v = 0;
   if (swizzle[i] == PIPE_SWIZZLE_1) {
      switch (output_desc->channel[1].type) {
   case UTIL_FORMAT_TYPE_UNSIGNED:
      v = output_desc->channel[1].normalized ? 0xffffffff : 1;
      case UTIL_FORMAT_TYPE_SIGNED:
      v = output_desc->channel[1].normalized ? 0x7fffffff : 1;
      case UTIL_FORMAT_TYPE_FLOAT:
      v = 0x3f800000;
      default:
            }
      }
   else {
      x86_mov(p->func, tmp, x86_make_disp(src, swizzle[i] * 4));
      }
      case 64:
      if (swizzle[i] >= PIPE_SWIZZLE_0) {
      unsigned l = 0;
   unsigned h = 0;
   if (swizzle[i] == PIPE_SWIZZLE_1) {
      switch (output_desc->channel[1].type) {
   case UTIL_FORMAT_TYPE_UNSIGNED:
      h = output_desc->channel[1].normalized ? 0xffffffff : 0;
   l = output_desc->channel[1].normalized ? 0xffffffff : 1;
      case UTIL_FORMAT_TYPE_SIGNED:
      h = output_desc->channel[1].normalized ? 0x7fffffff : 0;
   l = output_desc->channel[1].normalized ? 0xffffffff : 1;
      case UTIL_FORMAT_TYPE_FLOAT:
      h = 0x3ff00000;
   l = 0;
      default:
            }
   x86_mov_imm(p->func, x86_make_disp(dst, i * 8), l);
      }
   else {
      if (x86_target_caps(p->func) & X86_SSE) {
      struct x86_reg tmpXMM = x86_make_reg(file_XMM, 0);
   emit_load64(p, tmp, tmpXMM,
            }
   else {
      x86_mov(p->func, tmp, x86_make_disp(src, swizzle[i] * 8));
   x86_mov(p->func, x86_make_disp(dst, i * 8), tmp);
   x86_mov(p->func, tmp,
               }
      default:
            }
      }
   /* special case for draw's EMIT_4UB (RGBA) and EMIT_4UB_BGRA */
   else if ((x86_target_caps(p->func) & X86_SSE2) &&
            a->input_format == PIPE_FORMAT_R32G32B32A32_FLOAT &&
   (0 || a->output_format == PIPE_FORMAT_B8G8R8A8_UNORM
            /* load */
            if (a->output_format == PIPE_FORMAT_B8G8R8A8_UNORM) {
                  /* scale by 255.0 */
            /* pack and emit */
   sse2_cvtps2dq(p->func, dataXMM, dataXMM);
   sse2_packssdw(p->func, dataXMM, dataXMM);
   sse2_packuswb(p->func, dataXMM, dataXMM);
                           }
         static bool
   translate_attr(struct translate_sse *p,
               {
      if (a->input_format == a->output_format) {
      emit_memcpy(p, dst, src, util_format_get_stride(a->input_format, 1));
                  }
         static bool
   init_inputs(struct translate_sse *p, unsigned index_size)
   {
      unsigned i;
   struct x86_reg instance_id =
         struct x86_reg start_instance =
            for (i = 0; i < p->nr_buffer_variants; i++) {
      struct translate_buffer_variant *variant = &p->buffer_variant[i];
            if (!index_size || variant->instance_divisor) {
      struct x86_reg buf_max_index =
         struct x86_reg buf_stride =
         struct x86_reg buf_ptr =
         struct x86_reg buf_base_ptr =
                        /* Calculate pointer to first attrib:
   *   base_ptr + stride * index, where index depends on instance divisor
   */
                     /* Start with instance = instance_id
   * which is true if divisor is 1.
                                    /* TODO: Add x86_shr() to rtasm and use it whenever
   *       instance divisor is power of two.
   */
   x86_xor(p->func, tmp_EDX, tmp_EDX);
                     /* instance = (instance_id / divisor) + start_instance
   */
                  /* XXX we need to clamp the index here too, but to a
   * per-array max value, not the draw->pt.max_index value
   * that's being given to us via translate->set_buffer().
      }
                     /* Clamp to max_index
   */
   x86_cmp(p->func, tmp_EAX, buf_max_index);
               x86_mov(p->func, p->tmp2_EDX, buf_stride);
   x64_rexw(p->func);
   x86_imul(p->func, tmp_EAX, p->tmp2_EDX);
                           /* In the linear case, keep the buffer pointer instead of the
   * index number.
   */
   if (!index_size && p->nr_buffer_variants == 1) {
      x64_rexw(p->func);
      }
   else {
      x64_rexw(p->func);
                        }
         static struct x86_reg
   get_buffer_ptr(struct translate_sse *p,
         {
      if (var_idx == ELEMENT_BUFFER_INSTANCE_ID) {
         }
   if (!index_size && p->nr_buffer_variants == 1) {
         }
   else if (!index_size || p->buffer_variant[var_idx].instance_divisor) {
      struct x86_reg ptr = p->src_ECX;
   struct x86_reg buf_ptr =
                  x64_rexw(p->func);
   x86_mov(p->func, ptr, buf_ptr);
      }
   else {
      struct x86_reg ptr = p->src_ECX;
   const struct translate_buffer_variant *variant =
         struct x86_reg buf_stride =
      x86_make_disp(p->machine_EDI,
      struct x86_reg buf_base_ptr =
      x86_make_disp(p->machine_EDI,
      struct x86_reg buf_max_index =
                  /* Calculate pointer to current attrib:
   */
   switch (index_size) {
   case 1:
      x86_movzx8(p->func, ptr, elt);
      case 2:
      x86_movzx16(p->func, ptr, elt);
      case 4:
      x86_mov(p->func, ptr, elt);
               /* Clamp to max_index
   */
   x86_cmp(p->func, ptr, buf_max_index);
            x86_mov(p->func, p->tmp2_EDX, buf_stride);
   x64_rexw(p->func);
   x86_imul(p->func, ptr, p->tmp2_EDX);
   x64_rexw(p->func);
   x86_add(p->func, ptr, buf_base_ptr);
         }
         static bool
   incr_inputs(struct translate_sse *p, unsigned index_size)
   {
      if (!index_size && p->nr_buffer_variants == 1) {
      const unsigned buffer_index = p->buffer_variant[0].buffer_index;
   struct x86_reg stride =
                  if (p->buffer_variant[0].instance_divisor == 0) {
      x64_rexw(p->func);
   x86_add(p->func, p->idx_ESI, stride);
         }
   else if (!index_size) {
               /* Is this worthwhile??
   */
   for (i = 0; i < p->nr_buffer_variants; i++) {
      struct translate_buffer_variant *variant = &p->buffer_variant[i];
   struct x86_reg buf_ptr = x86_make_disp(p->machine_EDI,
      struct x86_reg buf_stride =
                     if (variant->instance_divisor == 0) {
      x86_mov(p->func, p->tmp_EAX, buf_stride);
   x64_rexw(p->func);
   x86_add(p->func, p->tmp_EAX, buf_ptr);
   if (i == 0)
         x64_rexw(p->func);
            }
   else {
      x64_rexw(p->func);
                  }
         /* Build run( struct translate *machine,
   *            unsigned start,
   *            unsigned count,
   *            void *output_buffer )
   * or
   *  run_elts( struct translate *machine,
   *            unsigned *elts,
   *            unsigned count,
   *            void *output_buffer )
   *
   *  Lots of hardcoding
   *
   * EAX -- pointer to current output vertex
   * ECX -- pointer to current attribute 
   * 
   */
   static bool
   build_vertex_emit(struct translate_sse *p,
         {
      int fixup, label;
            memset(p->reg_to_const, 0xff, sizeof(p->reg_to_const));
            p->tmp_EAX = x86_make_reg(file_REG32, reg_AX);
   p->idx_ESI = x86_make_reg(file_REG32, reg_SI);
   p->outbuf_EBX = x86_make_reg(file_REG32, reg_BX);
   p->machine_EDI = x86_make_reg(file_REG32, reg_DI);
   p->count_EBP = x86_make_reg(file_REG32, reg_BP);
   p->tmp2_EDX = x86_make_reg(file_REG32, reg_DX);
                              if (x86_target(p->func) == X86_64_WIN64_ABI) {
      /* the ABI guarantees a 16-byte aligned 32-byte "shadow space"
   * above the return address
   */
   sse2_movdqa(p->func, x86_make_disp(x86_make_reg(file_REG32, reg_SP), 8),
         sse2_movdqa(p->func,
                     x86_push(p->func, p->outbuf_EBX);
            /* on non-Win64 x86-64, these are already in the right registers */
   if (x86_target(p->func) != X86_64_STD_ABI) {
      x86_push(p->func, p->machine_EDI);
            if (x86_target(p->func) != X86_32) {
      x64_mov64(p->func, p->machine_EDI, x86_fn_arg(p->func, 1));
      }
   else {
      x86_mov(p->func, p->machine_EDI, x86_fn_arg(p->func, 1));
                           if (x86_target(p->func) != X86_32)
         else
            /* Load instance ID.
   */
   if (p->use_instancing) {
      x86_mov(p->func, p->tmp2_EDX, x86_fn_arg(p->func, 4));
   x86_mov(p->func,
                  x86_mov(p->func, p->tmp_EAX, x86_fn_arg(p->func, 5));
   x86_mov(p->func,
                     /* Get vertex count, compare to zero
   */
   x86_xor(p->func, p->tmp_EAX, p->tmp_EAX);
   x86_cmp(p->func, p->count_EBP, p->tmp_EAX);
            /* always load, needed or not:
   */
            /* Note address for loop jump
   */
   label = x86_get_label(p->func);
   {
      struct x86_reg elt = !index_size ? p->idx_ESI : x86_deref(p->idx_ESI);
   int last_variant = -1;
            for (j = 0; j < p->translate.key.nr_elements; j++) {
                     /* Figure out source pointer address:
   */
   if (variant != last_variant) {
      last_variant = variant;
               if (!translate_attr(p, a,
                           /* Next output vertex:
   */
   x64_rexw(p->func);
   x86_lea(p->func, p->outbuf_EBX,
            /* Incr index
   */
               /* decr count, loop if not zero
   */
   x86_dec(p->func, p->count_EBP);
            /* Exit mmx state?
   */
   if (p->func->need_emms)
            /* Land forward jump here:
   */
            /* Pop regs and return
   */
   if (x86_target(p->func) != X86_64_STD_ABI) {
      x86_pop(p->func, p->idx_ESI);
               x86_pop(p->func, p->count_EBP);
            if (x86_target(p->func) == X86_64_WIN64_ABI) {
      sse2_movdqa(p->func, x86_make_reg(file_XMM, 6),
         sse2_movdqa(p->func, x86_make_reg(file_XMM, 7),
      }
               }
         static void
   translate_sse_set_buffer(struct translate *translate,
               {
               if (buf < p->nr_buffers) {
      p->buffer[buf].base_ptr = (char *) ptr;
   p->buffer[buf].stride = stride;
               if (0)
      debug_printf("%s %d/%d: %p %d\n",
   }
         static void
   translate_sse_release(struct translate *translate)
   {
               x86_release_func(&p->elt8_func);
   x86_release_func(&p->elt16_func);
   x86_release_func(&p->elt_func);
               }
         struct translate *
   translate_sse2_create(const struct translate_key *key)
   {
      struct translate_sse *p = NULL;
            if (!util_get_cpu_caps()->has_sse)
            p = os_malloc_aligned(sizeof(struct translate_sse), 16);
   if (!p)
            memset(p, 0, sizeof(*p));
   memcpy(p->consts, consts, sizeof(consts));
            p->translate.key = *key;
   p->translate.release = translate_sse_release;
                     for (i = 0; i < key->nr_elements; i++) {
      if (key->element[i].type == TRANSLATE_ELEMENT_NORMAL) {
                              if (key->element[i].instance_divisor) {
                  /*
   * Map vertex element to vertex buffer variant.
   */
   for (j = 0; j < p->nr_buffer_variants; j++) {
      if (p->buffer_variant[j].buffer_index ==
      key->element[i].input_buffer
   && p->buffer_variant[j].instance_divisor ==
   key->element[i].instance_divisor) {
         }
   if (j == p->nr_buffer_variants) {
      p->buffer_variant[j].buffer_index = key->element[i].input_buffer;
   p->buffer_variant[j].instance_divisor =
            }
      }
   else {
                              if (0)
            if (!build_vertex_emit(p, &p->linear_func, 0))
            if (!build_vertex_emit(p, &p->elt_func, 4))
            if (!build_vertex_emit(p, &p->elt16_func, 2))
            if (!build_vertex_emit(p, &p->elt8_func, 1))
            p->translate.run = (run_func) x86_get_func(&p->linear_func);
   if (p->translate.run == NULL)
            p->translate.run_elts = (run_elts_func) x86_get_func(&p->elt_func);
   if (p->translate.run_elts == NULL)
            p->translate.run_elts16 = (run_elts16_func) x86_get_func(&p->elt16_func);
   if (p->translate.run_elts16 == NULL)
            p->translate.run_elts8 = (run_elts8_func) x86_get_func(&p->elt8_func);
   if (p->translate.run_elts8 == NULL)
                  fail:
      if (p)
               }
         #else
      struct translate *
   translate_sse2_create(const struct translate_key *key)
   {
         }
      #endif
