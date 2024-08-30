   /*
   * Copyright (C) 2017-2019 Alyssa Rosenzweig
   * Copyright (C) 2017-2019 Connor Abbott
   * Copyright (C) 2019 Collabora, Ltd.
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
      #include "decode.h"
   #include <ctype.h>
   #include <errno.h>
   #include <memory.h>
   #include <stdarg.h>
   #include <stdbool.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <genxml/gen_macros.h>
   #include <sys/mman.h>
      #include "compiler/bifrost/disassemble.h"
   #include "compiler/valhall/disassemble.h"
   #include "midgard/disassemble.h"
   #include "util/set.h"
   #include "pan_format.h"
      #if PAN_ARCH <= 5
   /* Midgard's tiler descriptor is embedded within the
   * larger FBD */
      static void
   pandecode_midgard_tiler_descriptor(struct pandecode_context *ctx,
               {
      pan_unpack(tp, TILER_CONTEXT, t);
            /* We've never seen weights used in practice, but they exist */
   pan_unpack(wp, TILER_WEIGHTS, w);
            nonzero_weights |= w.weight0 != 0x0;
   nonzero_weights |= w.weight1 != 0x0;
   nonzero_weights |= w.weight2 != 0x0;
   nonzero_weights |= w.weight3 != 0x0;
   nonzero_weights |= w.weight4 != 0x0;
   nonzero_weights |= w.weight5 != 0x0;
   nonzero_weights |= w.weight6 != 0x0;
            if (nonzero_weights)
      }
   #endif
      #if PAN_ARCH >= 5
   static void
   pandecode_render_target(struct pandecode_context *ctx, uint64_t gpu_va,
               {
      pandecode_log(ctx, "Color Render Targets @%" PRIx64 ":\n", gpu_va);
            for (int i = 0; i < (fb->render_target_count); i++) {
      mali_ptr rt_va = gpu_va + i * pan_size(RENDER_TARGET);
   const struct mali_render_target_packed *PANDECODE_PTR_VAR(
                     ctx->indent--;
      }
   #endif
      #if PAN_ARCH >= 6
   static void
   pandecode_sample_locations(struct pandecode_context *ctx, const void *fb)
   {
                        pandecode_log(ctx, "Sample locations @%" PRIx64 ":\n",
         for (int i = 0; i < 33; i++) {
      pandecode_log(ctx, "  (%d, %d),\n", samples[2 * i] - 128,
         }
   #endif
      struct pandecode_fbd
   GENX(pandecode_fbd)(struct pandecode_context *ctx, uint64_t gpu_va,
         {
      const void *PANDECODE_PTR_VAR(ctx, fb, (mali_ptr)gpu_va);
   pan_section_unpack(fb, FRAMEBUFFER, PARAMETERS, params);
         #if PAN_ARCH >= 6
               unsigned dcd_size = pan_size(DRAW);
         #if PAN_ARCH <= 9
         #endif
         if (params.pre_frame_0 != MALI_PRE_POST_FRAME_SHADER_MODE_NEVER) {
      const void *PANDECODE_PTR_VAR(ctx, dcd,
         pan_unpack(dcd, DRAW, draw);
   pandecode_log(ctx, "Pre frame 0 @%" PRIx64 " (mode=%d):\n",
                     if (params.pre_frame_1 != MALI_PRE_POST_FRAME_SHADER_MODE_NEVER) {
      const void *PANDECODE_PTR_VAR(ctx, dcd,
         pan_unpack(dcd, DRAW, draw);
   pandecode_log(ctx, "Pre frame 1 @%" PRIx64 ":\n",
                     if (params.post_frame != MALI_PRE_POST_FRAME_SHADER_MODE_NEVER) {
      const void *PANDECODE_PTR_VAR(ctx, dcd,
         pan_unpack(dcd, DRAW, draw);
   pandecode_log(ctx, "Post frame:\n");
         #else
               const void *t = pan_section_ptr(fb, FRAMEBUFFER, TILER);
   const void *w = pan_section_ptr(fb, FRAMEBUFFER, TILER_WEIGHTS);
      #endif
         pandecode_log(ctx, "Framebuffer @%" PRIx64 ":\n", gpu_va);
               #if PAN_ARCH >= 6
      if (params.tiler)
      #endif
         ctx->indent--;
         #if PAN_ARCH >= 5
               if (params.has_zs_crc_extension) {
      const struct mali_zs_crc_extension_packed *PANDECODE_PTR_VAR(
         DUMP_CL(ctx, ZS_CRC_EXTENSION, zs_crc, "ZS CRC Extension:\n");
                        if (is_fragment)
            return (struct pandecode_fbd){
      .rt_count = params.render_target_count,
         #else
      /* Dummy unpack of the padding section to make sure all words are 0.
   * No need to call print here since the section is supposed to be empty.
   */
   pan_section_unpack(fb, FRAMEBUFFER, PADDING_1, padding1);
            return (struct pandecode_fbd){
            #endif
   }
      #if PAN_ARCH >= 5
   mali_ptr
   GENX(pandecode_blend)(struct pandecode_context *ctx, void *descs, int rt_no,
         {
      pan_unpack(descs + (rt_no * pan_size(BLEND)), BLEND, b);
      #if PAN_ARCH >= 6
      if (b.internal.mode != MALI_BLEND_MODE_SHADER)
               #else
         #endif
   }
   #endif
      #if PAN_ARCH <= 7
   static bool
   panfrost_is_yuv_format(uint32_t packed)
   {
   #if PAN_ARCH == 7
      enum mali_format mali_fmt = packed >> 12;
      #else
      /* Currently only supported by panfrost on v7 */
   assert(0);
      #endif
   }
      static void
   pandecode_texture_payload(struct pandecode_context *ctx, mali_ptr payload,
         {
      unsigned nr_samples =
            /* A bunch of bitmap pointers follow.
   * We work out the correct number,
   * based on the mipmap/cubemap
   * properties, but dump extra
                     /* Miptree for each face */
   if (tex->dimension == MALI_TEXTURE_DIMENSION_CUBE)
            /* Array of layers */
            /* Array of textures */
         #define PANDECODE_EMIT_TEX_PAYLOAD_DESC(T, msg)                                \
      for (int i = 0; i < bitmap_count; ++i) {                                    \
      uint64_t addr = payload + pan_size(T) * i;                               \
   pan_unpack(PANDECODE_PTR(ctx, addr, void), T, s);                        \
            #if PAN_ARCH <= 5
      switch (tex->surface_type) {
   case MALI_SURFACE_TYPE_32:
      PANDECODE_EMIT_TEX_PAYLOAD_DESC(SURFACE_32, "Surface 32");
      case MALI_SURFACE_TYPE_64:
      PANDECODE_EMIT_TEX_PAYLOAD_DESC(SURFACE, "Surface");
      case MALI_SURFACE_TYPE_32_WITH_ROW_STRIDE:
      PANDECODE_EMIT_TEX_PAYLOAD_DESC(SURFACE_32, "Surface 32 With Row Stride");
      case MALI_SURFACE_TYPE_64_WITH_STRIDES:
      PANDECODE_EMIT_TEX_PAYLOAD_DESC(SURFACE_WITH_STRIDE,
            default:
      fprintf(ctx->dump_stream, "Unknown surface descriptor type %X\n",
               #elif PAN_ARCH == 6
         #else
      STATIC_ASSERT(PAN_ARCH == 7);
   if (panfrost_is_yuv_format(tex->format)) {
         } else {
      PANDECODE_EMIT_TEX_PAYLOAD_DESC(SURFACE_WITH_STRIDE,
         #endif
      #undef PANDECODE_EMIT_TEX_PAYLOAD_DESC
   }
   #endif
      #if PAN_ARCH <= 5
   void
   GENX(pandecode_texture)(struct pandecode_context *ctx, mali_ptr u, unsigned tex)
   {
               pan_unpack(cl, TEXTURE, temp);
            ctx->indent++;
   pandecode_texture_payload(ctx, u + pan_size(TEXTURE), &temp);
      }
   #else
   void
   GENX(pandecode_texture)(struct pandecode_context *ctx, const void *cl,
         {
      pan_unpack(cl, TEXTURE, temp);
                  #if PAN_ARCH >= 9
               /* Miptree for each face */
   if (temp.dimension == MALI_TEXTURE_DIMENSION_CUBE)
            for (unsigned i = 0; i < plane_count; ++i)
      DUMP_ADDR(ctx, PLANE, temp.surfaces + i * pan_size(PLANE), "Plane %u:\n",
   #else
         #endif
         }
   #endif
      #if PAN_ARCH >= 6
   void
   GENX(pandecode_tiler)(struct pandecode_context *ctx, mali_ptr gpu_va,
         {
               if (t.heap) {
      pan_unpack(PANDECODE_PTR(ctx, t.heap, void), TILER_HEAP, h);
               DUMP_UNPACKED(ctx, TILER_CONTEXT, t, "Tiler Context @%" PRIx64 ":\n",
      }
   #endif
      #if PAN_ARCH >= 9
   void
   GENX(pandecode_fau)(struct pandecode_context *ctx, mali_ptr addr,
         {
      if (count == 0)
                              fprintf(ctx->dump_stream, "%s @%" PRIx64 ":\n", name, addr);
   for (unsigned i = 0; i < count; ++i) {
         }
      }
      mali_ptr
   GENX(pandecode_shader)(struct pandecode_context *ctx, mali_ptr addr,
         {
      MAP_ADDR(ctx, SHADER_PROGRAM, addr, cl);
                     DUMP_UNPACKED(ctx, SHADER_PROGRAM, desc, "%s Shader @%" PRIx64 ":\n", label,
         pandecode_shader_disassemble(ctx, desc.binary, gpu_id);
      }
      static void
   pandecode_resources(struct pandecode_context *ctx, mali_ptr addr, unsigned size)
   {
      const uint8_t *cl = pandecode_fetch_gpu_mem(ctx, addr, size);
            for (unsigned i = 0; i < size; i += 0x20) {
               switch (type) {
   case MALI_DESCRIPTOR_TYPE_SAMPLER:
      DUMP_CL(ctx, SAMPLER, cl + i, "Sampler @%" PRIx64 ":\n", addr + i);
      case MALI_DESCRIPTOR_TYPE_TEXTURE:
      pandecode_log(ctx, "Texture @%" PRIx64 "\n", addr + i);
   GENX(pandecode_texture)(ctx, cl + i, i);
      case MALI_DESCRIPTOR_TYPE_ATTRIBUTE:
      DUMP_CL(ctx, ATTRIBUTE, cl + i, "Attribute @%" PRIx64 ":\n", addr + i);
      case MALI_DESCRIPTOR_TYPE_BUFFER:
      DUMP_CL(ctx, BUFFER, cl + i, "Buffer @%" PRIx64 ":\n", addr + i);
      default:
      fprintf(ctx->dump_stream, "Unknown descriptor type %X\n", type);
            }
      void
   GENX(pandecode_resource_tables)(struct pandecode_context *ctx, mali_ptr addr,
         {
      unsigned count = addr & 0x3F;
            const uint8_t *cl =
            for (unsigned i = 0; i < count; ++i) {
      pan_unpack(cl + i * MALI_RESOURCE_LENGTH, RESOURCE, entry);
   DUMP_UNPACKED(ctx, RESOURCE, entry, "Entry %u @%" PRIx64 ":\n", i,
            ctx->indent += 2;
   if (entry.address)
               }
      void
   GENX(pandecode_depth_stencil)(struct pandecode_context *ctx, mali_ptr addr)
   {
      MAP_ADDR(ctx, DEPTH_STENCIL, addr, cl);
   pan_unpack(cl, DEPTH_STENCIL, desc);
      }
      void
   GENX(pandecode_shader_environment)(struct pandecode_context *ctx,
               {
      if (p->shader)
            if (p->resources)
            if (p->thread_storage)
            if (p->fau)
      }
      void
   GENX(pandecode_blend_descs)(struct pandecode_context *ctx, mali_ptr blend,
               {
      for (unsigned i = 0; i < count; ++i) {
               mali_ptr blend_shader =
         if (blend_shader) {
      fprintf(ctx->dump_stream, "Blend shader %u @%" PRIx64 "", i,
                  }
      void
   GENX(pandecode_dcd)(struct pandecode_context *ctx, const struct MALI_DRAW *p,
         {
               GENX(pandecode_depth_stencil)(ctx, p->depth_stencil);
   GENX(pandecode_blend_descs)
   (ctx, p->blend, p->blend_count, frag_shader, gpu_id);
   GENX(pandecode_shader_environment)(ctx, &p->shader, gpu_id);
      }
   #endif
