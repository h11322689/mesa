   /*
   * Copyright 2017-2019 Alyssa Rosenzweig
   * Copyright 2017-2019 Connor Abbott
   * Copyright 2019 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include <ctype.h>
   #include <memory.h>
   #include <stdarg.h>
   #include <stdbool.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <sys/mman.h>
   #include <agx_pack.h>
      #include "util/u_hexdump.h"
   #include "decode.h"
   #ifdef __APPLE__
   #include "agx_iokit.h"
   #endif
      /* Pending UAPI */
   struct drm_asahi_params_global {
      int gpu_generation;
   int gpu_variant;
   int chip_id;
      };
      struct libagxdecode_config lib_config;
      UNUSED static const char *agx_alloc_types[AGX_NUM_ALLOC] = {"mem", "map",
            static void
   agx_disassemble(void *_code, size_t maxlen, FILE *fp)
   {
         }
      FILE *agxdecode_dump_stream;
      #define MAX_MAPPINGS 4096
      struct agx_bo mmap_array[MAX_MAPPINGS];
   unsigned mmap_count = 0;
      struct agx_bo *ro_mappings[MAX_MAPPINGS];
   unsigned ro_mapping_count = 0;
      static struct agx_bo *
   agxdecode_find_mapped_gpu_mem_containing_rw(uint64_t addr)
   {
      for (unsigned i = 0; i < mmap_count; ++i) {
      if (mmap_array[i].type == AGX_ALLOC_REGULAR &&
      addr >= mmap_array[i].ptr.gpu &&
   (addr - mmap_array[i].ptr.gpu) < mmap_array[i].size)
               }
      static struct agx_bo *
   agxdecode_find_mapped_gpu_mem_containing(uint64_t addr)
   {
               if (mem && mem->ptr.cpu && !mem->ro) {
      mprotect(mem->ptr.cpu, mem->size, PROT_READ);
   mem->ro = true;
   ro_mappings[ro_mapping_count++] = mem;
               if (mem && !mem->mapped) {
      fprintf(stderr,
         "[ERROR] access to memory not mapped (GPU %" PRIx64
                  }
      static struct agx_bo *
   agxdecode_find_handle(unsigned handle, unsigned type)
   {
      for (unsigned i = 0; i < mmap_count; ++i) {
      if (mmap_array[i].type != type)
            if (mmap_array[i].handle != handle)
                           }
      static void
   agxdecode_mark_mapped(unsigned handle)
   {
               if (!bo) {
      fprintf(stderr, "ERROR - unknown BO mapped with handle %u\n", handle);
               /* Mark mapped for future consumption */
      }
      #ifdef __APPLE__
      static void
   agxdecode_decode_segment_list(void *segment_list)
   {
               /* First, mark everything unmapped */
   for (unsigned i = 0; i < mmap_count; ++i)
            /* Check the header */
   struct agx_map_header *hdr = segment_list;
   if (hdr->resource_group_count == 0) {
      fprintf(agxdecode_dump_stream, "ERROR - empty map\n");
               if (hdr->segment_count != 1) {
      fprintf(agxdecode_dump_stream, "ERROR - can't handle segment count %u\n",
               fprintf(agxdecode_dump_stream, "Segment list:\n");
   fprintf(agxdecode_dump_stream, "  Command buffer shmem ID: %" PRIx64 "\n",
         fprintf(agxdecode_dump_stream, "  Encoder ID: %" PRIx64 "\n",
         fprintf(agxdecode_dump_stream, "  Kernel commands start offset: %u\n",
         fprintf(agxdecode_dump_stream, "  Kernel commands end offset: %u\n",
                  /* Expected structure: header followed by resource groups */
   size_t length = sizeof(struct agx_map_header);
            if (length != hdr->length) {
      fprintf(agxdecode_dump_stream, "ERROR: expected length %zu, got %u\n",
               if (hdr->padding[0] || hdr->padding[1])
            /* Check the entries */
   struct agx_map_entry *groups = ((void *)hdr) + sizeof(*hdr);
   for (unsigned i = 0; i < hdr->resource_group_count; ++i) {
      struct agx_map_entry group = groups[i];
            STATIC_ASSERT(ARRAY_SIZE(group.resource_id) == 6);
   STATIC_ASSERT(ARRAY_SIZE(group.resource_unk) == 6);
            if ((count < 1) || (count > 6)) {
      fprintf(agxdecode_dump_stream, "ERROR - invalid count %u\n", count);
               for (unsigned j = 0; j < count; ++j) {
      unsigned handle = group.resource_id[j];
                  if (!handle) {
      fprintf(agxdecode_dump_stream, "ERROR - invalid handle %u\n",
                                    fprintf(agxdecode_dump_stream, "%u (0x%X, 0x%X)\n", handle, unk,
               if (group.unka)
            /* Visual separator for resource groups */
               /* Check the handle count */
   if (nr_handles != hdr->total_resources) {
      fprintf(agxdecode_dump_stream,
               }
      #endif
      static size_t
   __agxdecode_fetch_gpu_mem(const struct agx_bo *mem, uint64_t gpu_va,
               {
      if (lib_config.read_gpu_mem)
            if (!mem)
            if (!mem) {
      fprintf(stderr, "Access to unknown memory %" PRIx64 " in %s:%d\n", gpu_va,
         fflush(agxdecode_dump_stream);
                        if (size + (gpu_va - mem->ptr.gpu) > mem->size) {
      fprintf(stderr,
         "Overflowing to unknown memory %" PRIx64
   " of size %zu (max size %zu) in %s:%d\n",
   gpu_va, size, (size_t)(mem->size - (gpu_va - mem->ptr.gpu)),
   fflush(agxdecode_dump_stream);
                           }
      #define agxdecode_fetch_gpu_mem(gpu_va, size, buf)                             \
            #define agxdecode_fetch_gpu_array(gpu_va, buf)                                 \
            static void
   agxdecode_map_read_write(void)
   {
      for (unsigned i = 0; i < ro_mapping_count; ++i) {
      ro_mappings[i]->ro = false;
   mprotect(ro_mappings[i]->ptr.cpu, ro_mappings[i]->size,
                  }
      /* Helpers for parsing the cmdstream */
      #define DUMP_UNPACKED(T, var, str)                                             \
      {                                                                           \
      agxdecode_log(str);                                                      \
            #define DUMP_CL(T, cl, str)                                                    \
      {                                                                           \
      agx_unpack(agxdecode_dump_stream, cl, T, temp);                          \
            #define agxdecode_log(str) fputs(str, agxdecode_dump_stream)
   #define agxdecode_msg(str) fprintf(agxdecode_dump_stream, "// %s", str)
      unsigned agxdecode_indent = 0;
      typedef struct drm_asahi_params_global decoder_params;
      /* Abstraction for command stream parsing */
   typedef unsigned (*decode_cmd)(const uint8_t *map, uint64_t *link, bool verbose,
            #define STATE_DONE (0xFFFFFFFFu)
   #define STATE_LINK (0xFFFFFFFEu)
   #define STATE_CALL (0xFFFFFFFDu)
   #define STATE_RET  (0xFFFFFFFCu)
      static void
   agxdecode_stateful(uint64_t va, const char *label, decode_cmd decoder,
         {
      uint64_t stack[16];
            uint8_t buf[1024];
   if (!lib_config.read_gpu_mem) {
      struct agx_bo *alloc = agxdecode_find_mapped_gpu_mem_containing(va);
   assert(alloc != NULL && "nonexistent object");
   fprintf(agxdecode_dump_stream, "%s (%" PRIx64 ", handle %u)\n", label, va,
      } else {
         }
                     int left = len;
   uint8_t *map = buf;
                     while (left) {
      if (len <= 0) {
      fprintf(agxdecode_dump_stream, "!! Failed to read GPU memory\n");
   fflush(agxdecode_dump_stream);
                        /* If we fail to decode, default to a hexdump (don't hang) */
   if (count == 0) {
      u_hexdump(agxdecode_dump_stream, map, 8, false);
               fflush(agxdecode_dump_stream);
   if (count == STATE_DONE) {
         } else if (count == STATE_LINK) {
      fprintf(agxdecode_dump_stream, "Linking to 0x%" PRIx64 "\n\n", link);
   va = link;
   left = len = agxdecode_fetch_gpu_array(va, buf);
      } else if (count == STATE_CALL) {
      fprintf(agxdecode_dump_stream,
         "Calling 0x%" PRIx64 " (return = 0x%" PRIx64 ")\n\n", link,
   assert(sp < ARRAY_SIZE(stack));
   stack[sp++] = va + 8;
   va = link;
   left = len = agxdecode_fetch_gpu_array(va, buf);
      } else if (count == STATE_RET) {
      assert(sp > 0);
   va = stack[--sp];
   fprintf(agxdecode_dump_stream, "Returning to 0x%" PRIx64 "\n\n", va);
   left = len = agxdecode_fetch_gpu_array(va, buf);
      } else {
      va += count;
                  if (left < 512 && len == sizeof(buf)) {
      left = len = agxdecode_fetch_gpu_array(va, buf);
               }
      static unsigned
   agxdecode_usc(const uint8_t *map, UNUSED uint64_t *link, UNUSED bool verbose,
         {
      enum agx_sampler_states *sampler_states = data;
   enum agx_usc_control type = map[0];
            bool extended_samplers =
      (sampler_states != NULL) &&
   (((*sampler_states) == AGX_SAMPLER_STATES_8_EXTENDED) ||
      #define USC_CASE(name, human)                                                  \
      case AGX_USC_CONTROL_##name: {                                              \
      DUMP_CL(USC_##name, map, human);                                         \
               switch (type) {
   case AGX_USC_CONTROL_NO_PRESHADER: {
      DUMP_CL(USC_NO_PRESHADER, map, "No preshader");
               case AGX_USC_CONTROL_PRESHADER: {
      agx_unpack(agxdecode_dump_stream, map, USC_PRESHADER, ctrl);
            agx_disassemble(buf, agxdecode_fetch_gpu_array(ctrl.code, buf),
                        case AGX_USC_CONTROL_SHADER: {
      agx_unpack(agxdecode_dump_stream, map, USC_SHADER, ctrl);
            agxdecode_log("\n");
   agx_disassemble(buf, agxdecode_fetch_gpu_array(ctrl.code, buf),
                              case AGX_USC_CONTROL_SAMPLER: {
      agx_unpack(agxdecode_dump_stream, map, USC_SAMPLER, temp);
            uint8_t buf[AGX_SAMPLER_LENGTH * temp.count];
                     for (unsigned i = 0; i < temp.count; ++i) {
                     if (extended_samplers) {
      DUMP_CL(BORDER, samp, "Border");
                              case AGX_USC_CONTROL_TEXTURE: {
      agx_unpack(agxdecode_dump_stream, map, USC_TEXTURE, temp);
            uint8_t buf[AGX_TEXTURE_LENGTH * temp.count];
                     /* Note: samplers only need 8 byte alignment? */
   for (unsigned i = 0; i < temp.count; ++i) {
      agx_unpack(agxdecode_dump_stream, tex, TEXTURE, t);
                                          case AGX_USC_CONTROL_UNIFORM: {
      agx_unpack(agxdecode_dump_stream, map, USC_UNIFORM, temp);
            uint8_t buf[2 * temp.size_halfs];
   agxdecode_fetch_gpu_array(temp.buffer, buf);
                           USC_CASE(FRAGMENT_PROPERTIES, "Fragment properties");
   USC_CASE(UNIFORM_HIGH, "Uniform high");
   USC_CASE(SHARED, "Shared");
         default:
      fprintf(agxdecode_dump_stream, "Unknown USC control type: %u\n", type);
   u_hexdump(agxdecode_dump_stream, map, 8, false);
            #undef USC_CASE
   }
      #define PPP_PRINT(map, header_name, struct_name, human)                        \
      if (hdr.header_name) {                                                      \
      if (((map + AGX_##struct_name##_LENGTH) > (base + size))) {              \
      fprintf(agxdecode_dump_stream, "Buffer overrun in PPP update\n");     \
      }                                                                        \
   DUMP_CL(struct_name, map, human);                                        \
   map += AGX_##struct_name##_LENGTH;                                       \
            static void
   agxdecode_record(uint64_t va, size_t size, bool verbose, decoder_params *params)
   {
      uint8_t buf[size];
   uint8_t *base = buf;
                     agx_unpack(agxdecode_dump_stream, map, PPP_HEADER, hdr);
            PPP_PRINT(map, fragment_control, FRAGMENT_CONTROL, "Fragment control");
   PPP_PRINT(map, fragment_control_2, FRAGMENT_CONTROL, "Fragment control 2");
   PPP_PRINT(map, fragment_front_face, FRAGMENT_FACE, "Front face");
   PPP_PRINT(map, fragment_front_face_2, FRAGMENT_FACE_2, "Front face 2");
   PPP_PRINT(map, fragment_front_stencil, FRAGMENT_STENCIL, "Front stencil");
   PPP_PRINT(map, fragment_back_face, FRAGMENT_FACE, "Back face");
   PPP_PRINT(map, fragment_back_face_2, FRAGMENT_FACE_2, "Back face 2");
   PPP_PRINT(map, fragment_back_stencil, FRAGMENT_STENCIL, "Back stencil");
   PPP_PRINT(map, depth_bias_scissor, DEPTH_BIAS_SCISSOR, "Depth bias/scissor");
   PPP_PRINT(map, region_clip, REGION_CLIP, "Region clip");
   PPP_PRINT(map, viewport, VIEWPORT, "Viewport");
   PPP_PRINT(map, w_clamp, W_CLAMP, "W clamp");
   PPP_PRINT(map, output_select, OUTPUT_SELECT, "Output select");
   PPP_PRINT(map, varying_counts_32, VARYING_COUNTS, "Varying counts 32");
   PPP_PRINT(map, varying_counts_16, VARYING_COUNTS, "Varying counts 16");
   PPP_PRINT(map, cull, CULL, "Cull");
            if (hdr.fragment_shader) {
      agx_unpack(agxdecode_dump_stream, map, FRAGMENT_SHADER, frag);
   agxdecode_stateful(frag.pipeline, "Fragment pipeline", agxdecode_usc,
            if (frag.cf_bindings) {
                                                   for (unsigned i = 0; i < frag.cf_binding_count; ++i) {
      DUMP_CL(CF_BINDING, cf, "Coefficient binding:");
                  DUMP_UNPACKED(FRAGMENT_SHADER, frag, "Fragment shader\n");
               PPP_PRINT(map, occlusion_query, FRAGMENT_OCCLUSION_QUERY, "Occlusion query");
   PPP_PRINT(map, occlusion_query_2, FRAGMENT_OCCLUSION_QUERY_2,
         PPP_PRINT(map, output_unknown, OUTPUT_UNKNOWN, "Output unknown");
   PPP_PRINT(map, output_size, OUTPUT_SIZE, "Output size");
            /* PPP print checks we don't read too much, now check we read enough */
      }
      static unsigned
   agxdecode_cdm(const uint8_t *map, uint64_t *link, bool verbose,
         {
      /* Bits 29-31 contain the block type */
            switch (block_type) {
   case AGX_CDM_BLOCK_TYPE_HEADER: {
         #define CDM_PRINT(STRUCT_NAME, human)                                          \
      do {                                                                        \
      DUMP_CL(CDM_##STRUCT_NAME, map, human);                                  \
   map += AGX_CDM_##STRUCT_NAME##_LENGTH;                                   \
                  agx_unpack(agxdecode_dump_stream, map, CDM_HEADER, hdr);
   agxdecode_stateful(hdr.pipeline, "Pipeline", agxdecode_usc, verbose,
         DUMP_UNPACKED(CDM_HEADER, hdr, "Compute\n");
            /* Added in G14X */
   if (params->gpu_generation >= 14 && params->num_clusters_total > 1)
            switch (hdr.mode) {
   case AGX_CDM_MODE_DIRECT:
      CDM_PRINT(GLOBAL_SIZE, "Global size");
   CDM_PRINT(LOCAL_SIZE, "Local size");
      case AGX_CDM_MODE_INDIRECT_GLOBAL:
      CDM_PRINT(INDIRECT, "Indirect buffer");
   CDM_PRINT(LOCAL_SIZE, "Local size");
      case AGX_CDM_MODE_INDIRECT_LOCAL:
      CDM_PRINT(INDIRECT, "Indirect buffer");
      default:
      fprintf(agxdecode_dump_stream, "Unknown CDM mode: %u\n", hdr.mode);
                           case AGX_CDM_BLOCK_TYPE_STREAM_LINK: {
      agx_unpack(agxdecode_dump_stream, map, CDM_STREAM_LINK, hdr);
   DUMP_UNPACKED(CDM_STREAM_LINK, hdr, "Stream Link\n");
   *link = hdr.target_lo | (((uint64_t)hdr.target_hi) << 32);
               case AGX_CDM_BLOCK_TYPE_STREAM_TERMINATE: {
      DUMP_CL(CDM_STREAM_TERMINATE, map, "Stream Terminate");
               case AGX_CDM_BLOCK_TYPE_LAUNCH: {
      DUMP_CL(CDM_LAUNCH, map, "Launch");
               default:
      fprintf(agxdecode_dump_stream, "Unknown CDM block type: %u\n",
         u_hexdump(agxdecode_dump_stream, map, 8, false);
         }
      static unsigned
   agxdecode_vdm(const uint8_t *map, uint64_t *link, bool verbose,
         {
      /* Bits 29-31 contain the block type */
            switch (block_type) {
   case AGX_VDM_BLOCK_TYPE_BARRIER: {
      agx_unpack(agxdecode_dump_stream, map, VDM_BARRIER, hdr);
   DUMP_UNPACKED(VDM_BARRIER, hdr, "Barrier\n");
               case AGX_VDM_BLOCK_TYPE_PPP_STATE_UPDATE: {
                        if (!lib_config.read_gpu_mem) {
               if (!mem) {
      DUMP_UNPACKED(PPP_STATE, cmd, "Non-existent record (XXX)\n");
                  agxdecode_record(address, cmd.size_words * 4, verbose, params);
               case AGX_VDM_BLOCK_TYPE_VDM_STATE_UPDATE: {
      size_t length = AGX_VDM_STATE_LENGTH;
   agx_unpack(agxdecode_dump_stream, map, VDM_STATE, hdr);
      #define VDM_PRINT(header_name, STRUCT_NAME, human)                             \
      if (hdr.header_name##_present) {                                            \
      DUMP_CL(VDM_STATE_##STRUCT_NAME, map, human);                            \
   map += AGX_VDM_STATE_##STRUCT_NAME##_LENGTH;                             \
                           /* If word 1 is present but word 0 is not, fallback to compact samplers */
            if (hdr.vertex_shader_word_0_present) {
      agx_unpack(agxdecode_dump_stream, map, VDM_STATE_VERTEX_SHADER_WORD_0,
                     VDM_PRINT(vertex_shader_word_0, VERTEX_SHADER_WORD_0,
            if (hdr.vertex_shader_word_1_present) {
      agx_unpack(agxdecode_dump_stream, map, VDM_STATE_VERTEX_SHADER_WORD_1,
         fprintf(agxdecode_dump_stream, "Pipeline %X\n",
         agxdecode_stateful(word_1.pipeline, "Pipeline", agxdecode_usc, verbose,
               VDM_PRINT(vertex_shader_word_1, VERTEX_SHADER_WORD_1,
         VDM_PRINT(vertex_outputs, VERTEX_OUTPUTS, "Vertex outputs");
      #undef VDM_PRINT
                     case AGX_VDM_BLOCK_TYPE_INDEX_LIST: {
      size_t length = AGX_INDEX_LIST_LENGTH;
   agx_unpack(agxdecode_dump_stream, map, INDEX_LIST, hdr);
   DUMP_UNPACKED(INDEX_LIST, hdr, "Index List\n");
      #define IDX_PRINT(header_name, STRUCT_NAME, human)                             \
      if (hdr.header_name##_present) {                                            \
      DUMP_CL(INDEX_LIST_##STRUCT_NAME, map, human);                           \
   map += AGX_INDEX_LIST_##STRUCT_NAME##_LENGTH;                            \
                  IDX_PRINT(index_buffer, BUFFER_LO, "Index buffer");
   IDX_PRINT(index_count, COUNT, "Index count");
   IDX_PRINT(instance_count, INSTANCES, "Instance count");
   IDX_PRINT(start, START, "Start");
   IDX_PRINT(indirect_buffer, INDIRECT_BUFFER, "Indirect buffer");
      #undef IDX_PRINT
                     case AGX_VDM_BLOCK_TYPE_STREAM_LINK: {
      agx_unpack(agxdecode_dump_stream, map, VDM_STREAM_LINK, hdr);
   DUMP_UNPACKED(VDM_STREAM_LINK, hdr, "Stream Link\n");
   *link = hdr.target_lo | (((uint64_t)hdr.target_hi) << 32);
               case AGX_VDM_BLOCK_TYPE_STREAM_TERMINATE: {
      DUMP_CL(VDM_STREAM_TERMINATE, map, "Stream Terminate");
               default:
      fprintf(agxdecode_dump_stream, "Unknown VDM block type: %u\n",
         u_hexdump(agxdecode_dump_stream, map, 8, false);
         }
      static void
   agxdecode_cs(uint32_t *cmdbuf, uint64_t encoder, bool verbose,
         {
      agx_unpack(agxdecode_dump_stream, cmdbuf + 16, IOGPU_COMPUTE, cs);
                     fprintf(agxdecode_dump_stream, "Context switch program:\n");
   uint8_t buf[1024];
   agx_disassemble(buf,
            }
      static void
   agxdecode_gfx(uint32_t *cmdbuf, uint64_t encoder, bool verbose,
         {
      agx_unpack(agxdecode_dump_stream, cmdbuf + 16, IOGPU_GRAPHICS, gfx);
                     if (gfx.clear_pipeline_unk) {
      fprintf(agxdecode_dump_stream, "Unk: %X\n", gfx.clear_pipeline_unk);
   agxdecode_stateful(gfx.clear_pipeline, "Clear pipeline", agxdecode_usc,
               if (gfx.store_pipeline_unk) {
      assert(gfx.store_pipeline_unk == 0x4);
   agxdecode_stateful(gfx.store_pipeline, "Store pipeline", agxdecode_usc,
               assert((gfx.partial_reload_pipeline_unk & 0xF) == 0x4);
   if (gfx.partial_reload_pipeline) {
      agxdecode_stateful(gfx.partial_reload_pipeline, "Partial reload pipeline",
               if (gfx.partial_store_pipeline) {
      agxdecode_stateful(gfx.partial_store_pipeline, "Partial store pipeline",
         }
      static void
   chip_id_to_params(decoder_params *params, uint32_t chip_id)
   {
      switch (chip_id) {
   case 0x6000 ... 0x6002:
      *params = (decoder_params){
      .gpu_generation = 13,
   .gpu_variant = "SCD"[chip_id & 15],
   .chip_id = chip_id,
      };
      case 0x6020 ... 0x6022:
      *params = (decoder_params){
      .gpu_generation = 14,
   .gpu_variant = "SCD"[chip_id & 15],
   .chip_id = chip_id,
      };
      case 0x8112:
      *params = (decoder_params){
      .gpu_generation = 14,
   .gpu_variant = 'G',
   .chip_id = chip_id,
      };
      case 0x8103:
   default:
      *params = (decoder_params){
      .gpu_generation = 13,
   .gpu_variant = 'G',
   .chip_id = chip_id,
      };
         }
      #ifdef __APPLE__
      void
   agxdecode_cmdstream(unsigned cmdbuf_handle, unsigned map_handle, bool verbose)
   {
               struct agx_bo *cmdbuf =
         struct agx_bo *map = agxdecode_find_handle(map_handle, AGX_ALLOC_MEMMAP);
   assert(cmdbuf != NULL && "nonexistent command buffer");
            /* Before decoding anything, validate the map. Set bo->mapped fields */
            /* Print the IOGPU stuff */
   agx_unpack(agxdecode_dump_stream, cmdbuf->ptr.cpu, IOGPU_HEADER, cmd);
            DUMP_CL(IOGPU_ATTACHMENT_COUNT,
                  uint32_t *attachments =
         unsigned attachment_count = attachments[3];
   for (unsigned i = 0; i < attachment_count; ++i) {
      uint32_t *ptr = attachments + 4 + (i * AGX_IOGPU_ATTACHMENT_LENGTH / 4);
                                 if (cmd.unk_5 == 3)
         else
               }
      void
   agxdecode_dump_mappings(unsigned map_handle)
   {
               struct agx_bo *map = agxdecode_find_handle(map_handle, AGX_ALLOC_MEMMAP);
   assert(map != NULL && "nonexistent mapping");
            for (unsigned i = 0; i < mmap_count; ++i) {
      if (!mmap_array[i].ptr.cpu || !mmap_array[i].size ||
                           fprintf(agxdecode_dump_stream,
         "Buffer: type %s, gpu %" PRIx64 ", handle %u.bin:\n\n",
            u_hexdump(agxdecode_dump_stream, mmap_array[i].ptr.cpu,
               }
      #endif
      void
   agxdecode_track_alloc(struct agx_bo *alloc)
   {
               for (unsigned i = 0; i < mmap_count; ++i) {
      struct agx_bo *bo = &mmap_array[i];
   bool match = (bo->handle == alloc->handle && bo->type == alloc->type);
                  }
      void
   agxdecode_track_free(struct agx_bo *bo)
   {
               for (unsigned i = 0; i < mmap_count; ++i) {
      if (mmap_array[i].handle == bo->handle &&
      (mmap_array[i].type == AGX_ALLOC_REGULAR) ==
                                          }
      static int agxdecode_dump_frame_count = 0;
      void
   agxdecode_dump_file_open(void)
   {
      if (agxdecode_dump_stream)
            /* This does a getenv every frame, so it is possible to use
   * setenv to change the base at runtime.
   */
   const char *dump_file_base =
         if (!strcmp(dump_file_base, "stderr"))
         else {
      char buffer[1024];
   snprintf(buffer, sizeof(buffer), "%s.%04d", dump_file_base,
         printf("agxdecode: dump command stream to file %s\n", buffer);
   agxdecode_dump_stream = fopen(buffer, "w");
   if (!agxdecode_dump_stream) {
      fprintf(stderr,
                  }
      static void
   agxdecode_dump_file_close(void)
   {
      if (agxdecode_dump_stream && agxdecode_dump_stream != stderr) {
      fclose(agxdecode_dump_stream);
         }
      void
   agxdecode_next_frame(void)
   {
      agxdecode_dump_file_close();
      }
      void
   agxdecode_close(void)
   {
         }
      static ssize_t
   libagxdecode_writer(void *cookie, const char *buffer, size_t size)
   {
         }
      #ifdef _GNU_SOURCE
   static cookie_io_functions_t funcs = {.write = libagxdecode_writer};
   #endif
      static decoder_params lib_params;
      void
   libagxdecode_init(struct libagxdecode_config *config)
   {
   #ifdef _GNU_SOURCE
      lib_config = *config;
               #else
      /* fopencookie is a glibc extension */
      #endif
   }
      void
   libagxdecode_vdm(uint64_t addr, const char *label, bool verbose)
   {
         }
      void
   libagxdecode_cdm(uint64_t addr, const char *label, bool verbose)
   {
         }
   void
   libagxdecode_usc(uint64_t addr, const char *label, bool verbose)
   {
         }
   void
   libagxdecode_shutdown(void)
   {
         }
