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
      #include "genxml/gen_macros.h"
   #include "util/set.h"
   #include "decode.h"
      #if PAN_ARCH <= 9
      static void
   pandecode_primitive(struct pandecode_context *ctx, const void *p)
   {
      pan_unpack(p, PRIMITIVE, primitive);
         #if PAN_ARCH <= 7
      /* Validate an index buffer is present if we need one. TODO: verify
            if (primitive.indices) {
      /* Grab the size */
   unsigned size = (primitive.index_type == MALI_INDEX_TYPE_UINT32)
                  /* Ensure we got a size, and if so, validate the index buffer
   * is large enough to hold a full set of indices of the given
            if (!size)
         else
      pandecode_validate_buffer(ctx, primitive.indices,
   } else if (primitive.index_type)
      #endif
   }
      #if PAN_ARCH <= 7
   static void
   pandecode_attributes(struct pandecode_context *ctx, mali_ptr addr, int count,
         {
      char *prefix = varying ? "Varying" : "Attribute";
            if (!count) {
      pandecode_log(ctx, "// warn: No %s records\n", prefix);
                        for (int i = 0; i < count; ++i) {
      pan_unpack(cl + i * pan_size(ATTRIBUTE_BUFFER), ATTRIBUTE_BUFFER, temp);
            switch (temp.type) {
   case MALI_ATTRIBUTE_TYPE_1D_NPOT_DIVISOR_WRITE_REDUCTION:
   case MALI_ATTRIBUTE_TYPE_1D_NPOT_DIVISOR: {
      pan_unpack(cl + (i + 1) * pan_size(ATTRIBUTE_BUFFER),
         pan_print(ctx->dump_stream, ATTRIBUTE_BUFFER_CONTINUATION_NPOT, temp2,
         i++;
      }
   case MALI_ATTRIBUTE_TYPE_3D_LINEAR:
   case MALI_ATTRIBUTE_TYPE_3D_INTERLEAVED: {
      pan_unpack(cl + (i + 1) * pan_size(ATTRIBUTE_BUFFER_CONTINUATION_3D),
         pan_print(ctx->dump_stream, ATTRIBUTE_BUFFER_CONTINUATION_3D, temp2,
         i++;
      }
   default:
            }
      }
      static unsigned
   pandecode_attribute_meta(struct pandecode_context *ctx, int count,
         {
               for (int i = 0; i < count; ++i, attribute += pan_size(ATTRIBUTE)) {
      MAP_ADDR(ctx, ATTRIBUTE, attribute, cl);
   pan_unpack(cl, ATTRIBUTE, a);
   DUMP_UNPACKED(ctx, ATTRIBUTE, a, "%s:\n",
                     pandecode_log(ctx, "\n");
      }
      /* return bits [lo, hi) of word */
   static u32
   bits(u32 word, u32 lo, u32 hi)
   {
      if (hi - lo >= 32)
            if (lo >= 32)
               }
      static void
   pandecode_invocation(struct pandecode_context *ctx, const void *i)
   {
      /* Decode invocation_count. See the comment before the definition of
   * invocation_count for an explanation.
   */
            unsigned size_x =
         unsigned size_y = bits(invocation.invocations, invocation.size_y_shift,
               unsigned size_z = bits(invocation.invocations, invocation.size_z_shift,
                  unsigned groups_x =
      bits(invocation.invocations, invocation.workgroups_x_shift,
            unsigned groups_y =
      bits(invocation.invocations, invocation.workgroups_y_shift,
            unsigned groups_z =
            pandecode_log(ctx, "Invocation (%d, %d, %d) x (%d, %d, %d)\n", size_x,
               }
      static void
   pandecode_textures(struct pandecode_context *ctx, mali_ptr textures,
         {
      if (!textures)
            pandecode_log(ctx, "Textures %" PRIx64 ":\n", textures);
         #if PAN_ARCH >= 6
      const void *cl =
            for (unsigned tex = 0; tex < texture_count; ++tex)
      #else
               for (int tex = 0; tex < texture_count; ++tex) {
      mali_ptr *PANDECODE_PTR_VAR(ctx, u, textures + tex * sizeof(mali_ptr));
   char *a = pointer_as_memory_reference(ctx, *u);
   pandecode_log(ctx, "%s,\n", a);
               /* Now, finally, descend down into the texture descriptor */
   for (unsigned tex = 0; tex < texture_count; ++tex) {
      mali_ptr *PANDECODE_PTR_VAR(ctx, u, textures + tex * sizeof(mali_ptr));
         #endif
      ctx->indent--;
      }
      static void
   pandecode_samplers(struct pandecode_context *ctx, mali_ptr samplers,
         {
      pandecode_log(ctx, "Samplers %" PRIx64 ":\n", samplers);
            for (int i = 0; i < sampler_count; ++i)
      DUMP_ADDR(ctx, SAMPLER, samplers + (pan_size(SAMPLER) * i),
         ctx->indent--;
      }
      static void
   pandecode_uniform_buffers(struct pandecode_context *ctx, mali_ptr pubufs,
         {
               for (int i = 0; i < ubufs_count; i++) {
      mali_ptr addr = (ubufs[i] >> 10) << 2;
                     char *ptr = pointer_as_memory_reference(ctx, addr);
   pandecode_log(ctx, "ubuf_%d[%u] = %s;\n", i, size, ptr);
                  }
      static void
   pandecode_uniforms(struct pandecode_context *ctx, mali_ptr uniforms,
         {
               char *ptr = pointer_as_memory_reference(ctx, uniforms);
   pandecode_log(ctx, "vec4 uniforms[%u] = %s;\n", uniform_count, ptr);
   free(ptr);
      }
      void
   GENX(pandecode_dcd)(struct pandecode_context *ctx, const struct MALI_DRAW *p,
         {
   #if PAN_ARCH >= 5
         #endif
            #if PAN_ARCH >= 5
         #endif
         #if PAN_ARCH == 5
         /* On v5 only, the actual framebuffer pointer is tagged with extra
   * metadata that we validate but do not print.
   */
            if (!ptr.type || ptr.zs_crc_extension_present ||
                           #elif PAN_ARCH == 4
         #endif
               int varying_count = 0, attribute_count = 0, uniform_count = 0,
                  if (p->state) {
      uint32_t *cl =
                     if (state.shader.shader & ~0xF)
      #if PAN_ARCH >= 6
                  if (idvs && state.secondary_shader)
   #endif
         DUMP_UNPACKED(ctx, RENDERER_STATE, state, "State:\n");
            /* Save for dumps */
   attribute_count = state.shader.attribute_count;
   varying_count = state.shader.varying_count;
   texture_count = state.shader.texture_count;
   sampler_count = state.shader.sampler_count;
      #if PAN_ARCH >= 6
         #else
         #endif
      #if PAN_ARCH == 4
         mali_ptr shader = state.blend_shader & ~0xF;
   if (state.multisample_misc.blend_shader && shader)
   #endif
         ctx->indent--;
            /* MRT blend fields are used on v5+. Technically, they are optional on v5
   * for backwards compatibility but we don't care about that.
   #if PAN_ARCH >= 5
         if ((job_type == MALI_JOB_TYPE_TILER ||
      job_type == MALI_JOB_TYPE_FRAGMENT) &&
                  for (unsigned i = 0; i < fbd_info.rt_count; i++) {
      mali_ptr shader =
         if (shader & ~0xF)
         #endif
      } else
            if (p->viewport) {
      DUMP_ADDR(ctx, VIEWPORT, p->viewport, "Viewport:\n");
                        if (p->attributes)
      max_attr_index =
         if (p->attribute_buffers)
      pandecode_attributes(ctx, p->attribute_buffers, max_attr_index, false,
         if (p->varyings) {
      varying_count =
               if (p->varying_buffers)
      pandecode_attributes(ctx, p->varying_buffers, varying_count, true,
         if (p->uniform_buffers) {
      if (uniform_buffer_count)
      pandecode_uniform_buffers(ctx, p->uniform_buffers,
      else
      } else if (uniform_buffer_count)
            /* We don't want to actually dump uniforms, but we do need to validate
            if (p->push_uniforms) {
      if (uniform_count)
         else
      } else if (uniform_count)
            if (p->textures)
            if (p->samplers)
      }
      static void
   pandecode_vertex_compute_geometry_job(struct pandecode_context *ctx,
               {
      struct mali_compute_job_packed *PANDECODE_PTR_VAR(ctx, p, job);
   pan_section_unpack(p, COMPUTE_JOB, DRAW, draw);
            pandecode_log(ctx, "Vertex Job Payload:\n");
   ctx->indent++;
   pandecode_invocation(ctx, pan_section_ptr(p, COMPUTE_JOB, INVOCATION));
   DUMP_SECTION(ctx, COMPUTE_JOB, PARAMETERS, p, "Vertex Job Parameters:\n");
   DUMP_UNPACKED(ctx, DRAW, draw, "Draw:\n");
   ctx->indent--;
      }
   #endif
      static void
   pandecode_write_value_job(struct pandecode_context *ctx, mali_ptr job)
   {
      struct mali_write_value_job_packed *PANDECODE_PTR_VAR(ctx, p, job);
   pan_section_unpack(p, WRITE_VALUE_JOB, PAYLOAD, u);
   DUMP_SECTION(ctx, WRITE_VALUE_JOB, PAYLOAD, p, "Write Value Payload:\n");
      }
      static void
   pandecode_cache_flush_job(struct pandecode_context *ctx, mali_ptr job)
   {
      struct mali_cache_flush_job_packed *PANDECODE_PTR_VAR(ctx, p, job);
   pan_section_unpack(p, CACHE_FLUSH_JOB, PAYLOAD, u);
   DUMP_SECTION(ctx, CACHE_FLUSH_JOB, PAYLOAD, p, "Cache Flush Payload:\n");
      }
      static void
   pandecode_tiler_job(struct pandecode_context *ctx,
               {
      struct mali_tiler_job_packed *PANDECODE_PTR_VAR(ctx, p, job);
   pan_section_unpack(p, TILER_JOB, DRAW, draw);
   GENX(pandecode_dcd)(ctx, &draw, h->type, gpu_id);
   pandecode_log(ctx, "Tiler Job Payload:\n");
         #if PAN_ARCH <= 7
         #endif
         pandecode_primitive(ctx, pan_section_ptr(p, TILER_JOB, PRIMITIVE));
                  #if PAN_ARCH >= 6
      pan_section_unpack(p, TILER_JOB, TILER, tiler_ptr);
         #if PAN_ARCH >= 9
      DUMP_SECTION(ctx, TILER_JOB, INSTANCE_COUNT, p, "Instance count:\n");
   DUMP_SECTION(ctx, TILER_JOB, VERTEX_COUNT, p, "Vertex count:\n");
   DUMP_SECTION(ctx, TILER_JOB, SCISSOR, p, "Scissor:\n");
      #else
         #endif
      #endif
      ctx->indent--;
      }
      static void
   pandecode_fragment_job(struct pandecode_context *ctx, mali_ptr job,
         {
      struct mali_fragment_job_packed *PANDECODE_PTR_VAR(ctx, p, job);
                  #if PAN_ARCH >= 5
      /* On v5 and newer, the actual framebuffer pointer is tagged with extra
   * metadata that we need to disregard.
   */
   pan_unpack(&s.framebuffer, FRAMEBUFFER_POINTER, ptr);
      #else
      /* On v4, the framebuffer pointer is untagged. */
      #endif
         UNUSED struct pandecode_fbd info =
         #if PAN_ARCH >= 5
      if (!ptr.type || ptr.zs_crc_extension_present != info.has_extra ||
      ptr.render_target_count != info.rt_count) {
         #endif
                     }
      #if PAN_ARCH == 6 || PAN_ARCH == 7
   static void
   pandecode_indexed_vertex_job(struct pandecode_context *ctx,
               {
               pandecode_log(ctx, "Vertex:\n");
   pan_section_unpack(p, INDEXED_VERTEX_JOB, VERTEX_DRAW, vert_draw);
   GENX(pandecode_dcd)(ctx, &vert_draw, h->type, gpu_id);
            pandecode_log(ctx, "Fragment:\n");
   pan_section_unpack(p, INDEXED_VERTEX_JOB, FRAGMENT_DRAW, frag_draw);
   GENX(pandecode_dcd)(ctx, &frag_draw, MALI_JOB_TYPE_FRAGMENT, gpu_id);
            pan_section_unpack(p, INDEXED_VERTEX_JOB, TILER, tiler_ptr);
   pandecode_log(ctx, "Tiler Job Payload:\n");
   ctx->indent++;
   GENX(pandecode_tiler)(ctx, tiler_ptr.address, gpu_id);
            pandecode_invocation(ctx,
                  DUMP_SECTION(ctx, INDEXED_VERTEX_JOB, PRIMITIVE_SIZE, p,
               }
   #endif
      #if PAN_ARCH == 9
   static void
   pandecode_malloc_vertex_job(struct pandecode_context *ctx, mali_ptr job,
         {
               DUMP_SECTION(ctx, MALLOC_VERTEX_JOB, PRIMITIVE, p, "Primitive:\n");
   DUMP_SECTION(ctx, MALLOC_VERTEX_JOB, INSTANCE_COUNT, p, "Instance count:\n");
   DUMP_SECTION(ctx, MALLOC_VERTEX_JOB, ALLOCATION, p, "Allocation:\n");
   DUMP_SECTION(ctx, MALLOC_VERTEX_JOB, TILER, p, "Tiler:\n");
   DUMP_SECTION(ctx, MALLOC_VERTEX_JOB, SCISSOR, p, "Scissor:\n");
   DUMP_SECTION(ctx, MALLOC_VERTEX_JOB, PRIMITIVE_SIZE, p, "Primitive Size:\n");
                     pan_section_unpack(p, MALLOC_VERTEX_JOB, TILER, tiler_ptr);
   pandecode_log(ctx, "Tiler Job Payload:\n");
   ctx->indent++;
   if (tiler_ptr.address)
         else
                           pan_section_unpack(p, MALLOC_VERTEX_JOB, POSITION, position);
   pan_section_unpack(p, MALLOC_VERTEX_JOB, VARYING, varying);
   GENX(pandecode_shader_environment)(ctx, &position, gpu_id);
      }
      static void
   pandecode_compute_job(struct pandecode_context *ctx, mali_ptr job,
         {
      struct mali_compute_job_packed *PANDECODE_PTR_VAR(ctx, p, job);
            GENX(pandecode_shader_environment)(ctx, &payload.compute, gpu_id);
      }
   #endif
      /*
   * Trace a job chain at a particular GPU address, interpreted for a particular
   * GPU using the job manager.
   */
   void
   GENX(pandecode_jc)(struct pandecode_context *ctx, mali_ptr jc_gpu_va,
         {
               struct set *va_set = _mesa_pointer_set_create(NULL);
                     do {
      struct mali_job_header_packed *hdr =
            entry = _mesa_set_search(va_set, hdr);
   if (entry != NULL) {
      fprintf(stdout, "Job list has a cycle\n");
               pan_unpack(hdr, JOB_HEADER, h);
            DUMP_UNPACKED(ctx, JOB_HEADER, h, "Job Header (%" PRIx64 "):\n",
                  switch (h.type) {
   case MALI_JOB_TYPE_WRITE_VALUE:
                  case MALI_JOB_TYPE_CACHE_FLUSH:
                  case MALI_JOB_TYPE_TILER:
            #if PAN_ARCH <= 7
         case MALI_JOB_TYPE_VERTEX:
   case MALI_JOB_TYPE_COMPUTE:
            #if PAN_ARCH >= 6
         case MALI_JOB_TYPE_INDEXED_VERTEX:
         #endif
   #else
         case MALI_JOB_TYPE_COMPUTE:
                  case MALI_JOB_TYPE_MALLOC_VERTEX:
         #endif
            case MALI_JOB_TYPE_FRAGMENT:
                  default:
                  /* Track the latest visited job CPU VA to detect cycles */
                        fflush(ctx->dump_stream);
      }
      void
   GENX(pandecode_abort_on_fault)(struct pandecode_context *ctx,
         {
               do {
      pan_unpack(PANDECODE_PTR(ctx, jc_gpu_va, struct mali_job_header_packed),
                  /* Ensure the job is marked COMPLETE */
   if (h.exception_status != 0x1) {
      fprintf(stderr, "Incomplete job or timeout\n");
   fflush(NULL);
                     }
      #endif
