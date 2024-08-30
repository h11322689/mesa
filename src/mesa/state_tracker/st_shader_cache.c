   /*
   * Copyright © 2017 Timothy Arceri
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <stdio.h>
   #include "st_debug.h"
   #include "st_program.h"
   #include "st_shader_cache.h"
   #include "st_util.h"
   #include "compiler/glsl/program.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_serialize.h"
   #include "main/uniforms.h"
   #include "pipe/p_shader_tokens.h"
   #include "util/u_memory.h"
   #include "util/perf/cpu_trace.h"
      void
   st_get_program_binary_driver_sha1(struct gl_context *ctx, uint8_t *sha1)
   {
         }
      static void
   write_stream_out_to_cache(struct blob *blob,
         {
      blob_write_uint32(blob, state->stream_output.num_outputs);
   if (state->stream_output.num_outputs) {
      blob_write_bytes(blob, &state->stream_output.stride,
         blob_write_bytes(blob, &state->stream_output.output,
         }
      static void
   copy_blob_to_driver_cache_blob(struct blob *blob, struct gl_program *prog)
   {
      prog->driver_cache_blob = ralloc_size(NULL, blob->size);
   memcpy(prog->driver_cache_blob, blob->data, blob->size);
      }
      static void
   write_nir_to_cache(struct blob *blob, struct gl_program *prog)
   {
               blob_write_intptr(blob, prog->serialized_nir_size);
               }
      void
   st_serialise_nir_program(struct gl_context *ctx, struct gl_program *prog)
   {
      if (prog->driver_cache_blob)
            struct blob blob;
            if (prog->info.stage == MESA_SHADER_VERTEX) {
               blob_write_uint32(&blob, vp->num_inputs);
   blob_write_uint32(&blob, vp->vert_attrib_mask);
   blob_write_bytes(&blob, vp->result_to_output,
               if (prog->info.stage == MESA_SHADER_VERTEX ||
      prog->info.stage == MESA_SHADER_TESS_EVAL ||
   prog->info.stage == MESA_SHADER_GEOMETRY)
                     }
      /**
   * Store NIR and any other required state in on-disk shader cache.
   */
   void
   st_store_nir_in_disk_cache(struct st_context *st, struct gl_program *prog)
   {
      if (!st->ctx->Cache)
            /* Exit early when we are dealing with a ff shader with no source file to
   * generate a source from.
   */
   static const char zero[sizeof(prog->sh.data->sha1)] = {0};
   if (memcmp(prog->sh.data->sha1, zero, sizeof(prog->sh.data->sha1)) == 0)
                     if (st->ctx->_Shader->Flags & GLSL_CACHE_INFO) {
      fprintf(stderr, "putting %s state tracker IR in cache\n",
         }
      static void
   read_stream_out_from_cache(struct blob_reader *blob_reader,
         {
      memset(&state->stream_output, 0, sizeof(state->stream_output));
   state->stream_output.num_outputs = blob_read_uint32(blob_reader);
   if (state->stream_output.num_outputs) {
      blob_copy_bytes(blob_reader, &state->stream_output.stride,
         blob_copy_bytes(blob_reader, &state->stream_output.output,
         }
      void
   st_deserialise_nir_program(struct gl_context *ctx,
               {
      struct st_context *st = st_context(ctx);
   size_t size = prog->driver_cache_blob_size;
                              /* Avoid reallocation of the program parameter list, because the uniform
   * storage is only associated with the original parameter list.
   * This should be enough for Bitmap and DrawPixels constants.
   */
                     struct blob_reader blob_reader;
                     if (prog->info.stage == MESA_SHADER_VERTEX) {
      struct gl_vertex_program *vp = (struct gl_vertex_program *)prog;
   vp->num_inputs = blob_read_uint32(&blob_reader);
   vp->vert_attrib_mask = blob_read_uint32(&blob_reader);
   blob_copy_bytes(&blob_reader, (uint8_t *) vp->result_to_output,
               if (prog->info.stage == MESA_SHADER_VERTEX ||
      prog->info.stage == MESA_SHADER_TESS_EVAL ||
   prog->info.stage == MESA_SHADER_GEOMETRY)
         assert(prog->nir == NULL);
            prog->state.type = PIPE_SHADER_IR_NIR;
   prog->serialized_nir_size = blob_read_intptr(&blob_reader);
   prog->serialized_nir = malloc(prog->serialized_nir_size);
   blob_copy_bytes(&blob_reader, prog->serialized_nir, prog->serialized_nir_size);
            /* Make sure we don't try to read more data than we wrote. This should
   * never happen in release builds but its useful to have this check to
   * catch development bugs.
   */
   if (blob_reader.current != blob_reader.end || blob_reader.overrun) {
               if (ctx->_Shader->Flags & GLSL_CACHE_INFO) {
      fprintf(stderr, "Error reading program from cache (invalid "
                     }
      bool
   st_load_nir_from_disk_cache(struct gl_context *ctx,
         {
      if (!ctx->Cache)
            /* If we didn't load the GLSL metadata from cache then we could not have
   * loaded the NIR either.
   */
   if (prog->data->LinkStatus != LINKING_SKIPPED)
            for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
            struct gl_program *glprog = prog->_LinkedShaders[i]->Program;
            /* We don't need the cached blob anymore so free it */
   ralloc_free(glprog->driver_cache_blob);
   glprog->driver_cache_blob = NULL;
            if (ctx->_Shader->Flags & GLSL_CACHE_INFO) {
      fprintf(stderr, "%s state tracker IR retrieved from cache\n",
                     }
      void
   st_serialise_nir_program_binary(struct gl_context *ctx,
               {
         }
