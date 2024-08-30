   /*
   * Copyright 2015 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_debug.h"
   #include "ac_rtld.h"
   #include "driver_ddebug/dd_util.h"
   #include "si_pipe.h"
   #include "sid.h"
   #include "sid_tables.h"
   #include "tgsi/tgsi_from_mesa.h"
   #include "util/u_dump.h"
   #include "util/u_log.h"
   #include "util/u_memory.h"
   #include "util/u_process.h"
   #include "util/u_string.h"
      static void si_dump_bo_list(struct si_context *sctx, const struct radeon_saved_cs *saved, FILE *f);
      DEBUG_GET_ONCE_OPTION(replace_shaders, "RADEON_REPLACE_SHADERS", NULL)
      /**
   * Store a linearized copy of all chunks of \p cs together with the buffer
   * list in \p saved.
   */
   void si_save_cs(struct radeon_winsys *ws, struct radeon_cmdbuf *cs, struct radeon_saved_cs *saved,
         {
      uint32_t *buf;
            /* Save the IB chunks. */
   saved->num_dw = cs->prev_dw + cs->current.cdw;
   saved->ib = MALLOC(4 * saved->num_dw);
   if (!saved->ib)
            buf = saved->ib;
   for (i = 0; i < cs->num_prev; ++i) {
      memcpy(buf, cs->prev[i].buf, cs->prev[i].cdw * 4);
      }
            if (!get_buffer_list)
            /* Save the buffer list. */
   saved->bo_count = ws->cs_get_buffer_list(cs, NULL);
   saved->bo_list = CALLOC(saved->bo_count, sizeof(saved->bo_list[0]));
   if (!saved->bo_list) {
      FREE(saved->ib);
      }
                  oom:
      fprintf(stderr, "%s: out of memory\n", __func__);
      }
      void si_clear_saved_cs(struct radeon_saved_cs *saved)
   {
      FREE(saved->ib);
               }
      void si_destroy_saved_cs(struct si_saved_cs *scs)
   {
      si_clear_saved_cs(&scs->gfx);
   si_resource_reference(&scs->trace_buf, NULL);
      }
      static void si_dump_shader(struct si_screen *sscreen, struct si_shader *shader, FILE *f)
   {
      if (shader->shader_log)
         else
            if (shader->bo && sscreen->options.dump_shader_binary) {
      unsigned size = shader->bo->b.b.width0;
            const char *mapped = sscreen->ws->buffer_map(sscreen->ws,
                  for (unsigned i = 0; i < size; i += 4) {
                                 }
      struct si_log_chunk_shader {
      /* The shader destroy code assumes a current context for unlinking of
   * PM4 packets etc.
   *
   * While we should be able to destroy shaders without a context, doing
   * so would happen only very rarely and be therefore likely to fail
   * just when you're trying to debug something. Let's just remember the
   * current context in the chunk.
   */
   struct si_context *ctx;
            /* For keep-alive reference counts */
   struct si_shader_selector *sel;
      };
      static void si_log_chunk_shader_destroy(void *data)
   {
      struct si_log_chunk_shader *chunk = data;
   si_shader_selector_reference(chunk->ctx, &chunk->sel, NULL);
   si_compute_reference(&chunk->program, NULL);
      }
      static void si_log_chunk_shader_print(void *data, FILE *f)
   {
      struct si_log_chunk_shader *chunk = data;
   struct si_screen *sscreen = chunk->ctx->screen;
      }
      static struct u_log_chunk_type si_log_chunk_type_shader = {
      .destroy = si_log_chunk_shader_destroy,
      };
      static void si_dump_gfx_shader(struct si_context *ctx, const struct si_shader_ctx_state *state,
         {
               if (!state->cso || !current)
            struct si_log_chunk_shader *chunk = CALLOC_STRUCT(si_log_chunk_shader);
   chunk->ctx = ctx;
   chunk->shader = current;
   si_shader_selector_reference(ctx, &chunk->sel, current->selector);
      }
      static void si_dump_compute_shader(struct si_context *ctx, struct u_log_context *log)
   {
               if (!state->program)
            struct si_log_chunk_shader *chunk = CALLOC_STRUCT(si_log_chunk_shader);
   chunk->ctx = ctx;
   chunk->shader = &state->program->shader;
   si_compute_reference(&chunk->program, state->program);
      }
      /**
   * Shader compiles can be overridden with arbitrary ELF objects by setting
   * the environment variable RADEON_REPLACE_SHADERS=num1:filename1[;num2:filename2]
   *
   * TODO: key this off some hash
   */
   bool si_replace_shader(unsigned num, struct si_shader_binary *binary)
   {
      const char *p = debug_get_option_replace_shaders();
   const char *semicolon;
   char *copy = NULL;
   FILE *f;
   long filesize, nread;
            if (!p)
            while (*p) {
      unsigned long i;
   char *endp;
            p = endp;
   if (*p != ':') {
      fprintf(stderr, "RADEON_REPLACE_SHADERS formatted badly.\n");
      }
            if (i == num)
            p = strchr(p, ';');
   if (!p)
            }
   if (!*p)
            semicolon = strchr(p, ';');
   if (semicolon) {
      p = copy = strndup(p, semicolon - p);
   if (!copy) {
      fprintf(stderr, "out of memory\n");
                           f = fopen(p, "r");
   if (!f) {
      perror("radeonsi: failed to open file");
               if (fseek(f, 0, SEEK_END) != 0)
            filesize = ftell(f);
   if (filesize < 0)
            if (fseek(f, 0, SEEK_SET) != 0)
            binary->code_buffer = MALLOC(filesize);
   if (!binary->code_buffer) {
      fprintf(stderr, "out of memory\n");
               nread = fread((void *)binary->code_buffer, 1, filesize, f);
   if (nread != filesize) {
      FREE((void *)binary->code_buffer);
   binary->code_buffer = NULL;
               binary->type = SI_SHADER_BINARY_ELF;
   binary->code_size = nread;
         out_close:
         out_free:
      free(copy);
         file_error:
      perror("radeonsi: reading shader");
      }
      /* Parsed IBs are difficult to read without colors. Use "less -R file" to
   * read them, or use "aha -b -f file" to convert them to html.
   */
   #define COLOR_RESET  "\033[0m"
   #define COLOR_RED    "\033[31m"
   #define COLOR_GREEN  "\033[1;32m"
   #define COLOR_YELLOW "\033[1;33m"
   #define COLOR_CYAN   "\033[1;36m"
      static void si_dump_mmapped_reg(struct si_context *sctx, FILE *f, unsigned offset)
   {
      struct radeon_winsys *ws = sctx->ws;
            if (ws->read_registers(ws, offset, 1, &value))
      }
      static void si_dump_debug_registers(struct si_context *sctx, FILE *f)
   {
      fprintf(f, "Memory-mapped registers:\n");
            /* No other registers can be read on radeon. */
   if (!sctx->screen->info.is_amdgpu) {
      fprintf(f, "\n");
               si_dump_mmapped_reg(sctx, f, R_008008_GRBM_STATUS2);
   si_dump_mmapped_reg(sctx, f, R_008014_GRBM_STATUS_SE0);
   si_dump_mmapped_reg(sctx, f, R_008018_GRBM_STATUS_SE1);
   si_dump_mmapped_reg(sctx, f, R_008038_GRBM_STATUS_SE2);
   si_dump_mmapped_reg(sctx, f, R_00803C_GRBM_STATUS_SE3);
   si_dump_mmapped_reg(sctx, f, R_00D034_SDMA0_STATUS_REG);
   si_dump_mmapped_reg(sctx, f, R_00D834_SDMA1_STATUS_REG);
   if (sctx->gfx_level <= GFX8) {
      si_dump_mmapped_reg(sctx, f, R_000E50_SRBM_STATUS);
   si_dump_mmapped_reg(sctx, f, R_000E4C_SRBM_STATUS2);
      }
   si_dump_mmapped_reg(sctx, f, R_008680_CP_STAT);
   si_dump_mmapped_reg(sctx, f, R_008674_CP_STALLED_STAT1);
   si_dump_mmapped_reg(sctx, f, R_008678_CP_STALLED_STAT2);
   si_dump_mmapped_reg(sctx, f, R_008670_CP_STALLED_STAT3);
   si_dump_mmapped_reg(sctx, f, R_008210_CP_CPC_STATUS);
   si_dump_mmapped_reg(sctx, f, R_008214_CP_CPC_BUSY_STAT);
   si_dump_mmapped_reg(sctx, f, R_008218_CP_CPC_STALLED_STAT1);
   si_dump_mmapped_reg(sctx, f, R_00821C_CP_CPF_STATUS);
   si_dump_mmapped_reg(sctx, f, R_008220_CP_CPF_BUSY_STAT);
   si_dump_mmapped_reg(sctx, f, R_008224_CP_CPF_STALLED_STAT1);
      }
      struct si_log_chunk_cs {
      struct si_context *ctx;
   struct si_saved_cs *cs;
   bool dump_bo_list;
      };
      static void si_log_chunk_type_cs_destroy(void *data)
   {
      struct si_log_chunk_cs *chunk = data;
   si_saved_cs_reference(&chunk->cs, NULL);
      }
      static void si_parse_current_ib(FILE *f, struct radeon_cmdbuf *cs, unsigned begin, unsigned end,
               {
                                 for (unsigned prev_idx = 0; prev_idx < cs->num_prev; ++prev_idx) {
               if (begin < chunk->cdw) {
      ac_parse_ib_chunk(f, chunk->buf + begin, MIN2(end, chunk->cdw) - begin, last_trace_id,
               if (end <= chunk->cdw)
            if (begin < chunk->cdw)
            begin -= MIN2(begin, chunk->cdw);
                        ac_parse_ib_chunk(f, cs->current.buf + begin, end - begin, last_trace_id, trace_id_count,
               }
      void si_print_current_ib(struct si_context *sctx, FILE *f)
   {
      si_parse_current_ib(f, &sctx->gfx_cs, 0, sctx->gfx_cs.prev_dw + sctx->gfx_cs.current.cdw,
      }
      static void si_log_chunk_type_cs_print(void *data, FILE *f)
   {
      struct si_log_chunk_cs *chunk = data;
   struct si_context *ctx = chunk->ctx;
   struct si_saved_cs *scs = chunk->cs;
            /* We are expecting that the ddebug pipe has already
   * waited for the context, so this buffer should be idle.
   * If the GPU is hung, there is no point in waiting for it.
   */
   uint32_t *map = ctx->ws->buffer_map(ctx->ws, scs->trace_buf->buf, NULL,
         if (map)
            if (chunk->gfx_end != chunk->gfx_begin) {
      if (chunk->gfx_begin == 0) {
      if (ctx->cs_preamble_state)
      ac_parse_ib(f, ctx->cs_preamble_state->pm4, ctx->cs_preamble_state->ndw, NULL, 0,
            if (scs->flushed) {
      ac_parse_ib(f, scs->gfx.ib + chunk->gfx_begin, chunk->gfx_end - chunk->gfx_begin,
      } else {
      si_parse_current_ib(f, &ctx->gfx_cs, chunk->gfx_begin, chunk->gfx_end, &last_trace_id,
                  if (chunk->dump_bo_list) {
      fprintf(f, "Flushing. Time: ");
   util_dump_ns(f, scs->time_flush);
   fprintf(f, "\n\n");
         }
      static const struct u_log_chunk_type si_log_chunk_type_cs = {
      .destroy = si_log_chunk_type_cs_destroy,
      };
      static void si_log_cs(struct si_context *ctx, struct u_log_context *log, bool dump_bo_list)
   {
               struct si_saved_cs *scs = ctx->current_saved_cs;
            if (!dump_bo_list && gfx_cur == scs->gfx_last_dw)
                     chunk->ctx = ctx;
   si_saved_cs_reference(&chunk->cs, scs);
            chunk->gfx_begin = scs->gfx_last_dw;
   chunk->gfx_end = gfx_cur;
               }
      void si_auto_log_cs(void *data, struct u_log_context *log)
   {
      struct si_context *ctx = (struct si_context *)data;
      }
      void si_log_hw_flush(struct si_context *sctx)
   {
      if (!sctx->log)
                     if (sctx->context_flags & SI_CONTEXT_FLAG_AUX) {
      /* The aux context isn't captured by the ddebug wrapper,
   * so we dump it on a flush-by-flush basis here.
   */
   FILE *f = dd_get_debug_file(false);
   if (!f) {
         } else {
                                       }
      static const char *priority_to_string(unsigned priority)
   {
   #define ITEM(x) if (priority == RADEON_PRIO_##x) return #x
      ITEM(FENCE_TRACE);
   ITEM(SO_FILLED_SIZE);
   ITEM(QUERY);
   ITEM(IB);
   ITEM(DRAW_INDIRECT);
   ITEM(INDEX_BUFFER);
   ITEM(CP_DMA);
   ITEM(BORDER_COLORS);
   ITEM(CONST_BUFFER);
   ITEM(DESCRIPTORS);
   ITEM(SAMPLER_BUFFER);
   ITEM(VERTEX_BUFFER);
   ITEM(SHADER_RW_BUFFER);
   ITEM(SAMPLER_TEXTURE);
   ITEM(SHADER_RW_IMAGE);
   ITEM(SAMPLER_TEXTURE_MSAA);
   ITEM(COLOR_BUFFER);
   ITEM(DEPTH_BUFFER);
   ITEM(COLOR_BUFFER_MSAA);
   ITEM(DEPTH_BUFFER_MSAA);
   ITEM(SEPARATE_META);
   ITEM(SHADER_BINARY);
   ITEM(SHADER_RINGS);
      #undef ITEM
            }
      static int bo_list_compare_va(const struct radeon_bo_list_item *a,
         {
         }
      static void si_dump_bo_list(struct si_context *sctx, const struct radeon_saved_cs *saved, FILE *f)
   {
               if (!saved->bo_list)
            /* Sort the list according to VM addresses first. */
            fprintf(f, "Buffer list (in units of pages = 4kB):\n" COLOR_YELLOW
                  for (i = 0; i < saved->bo_count; i++) {
      /* Note: Buffer sizes are expected to be aligned to 4k by the winsys. */
   const unsigned page_size = sctx->screen->info.gart_page_size;
   uint64_t va = saved->bo_list[i].vm_address;
   uint64_t size = saved->bo_list[i].bo_size;
            /* If there's unused virtual memory between 2 buffers, print it. */
   if (i) {
                     if (va > previous_va_end) {
                     /* Print the buffer. */
   fprintf(f, "  %10" PRIu64 "    0x%013" PRIX64 "       0x%013" PRIX64 "       ",
            /* Print the usage. */
   for (j = 0; j < 32; j++) {
                     fprintf(f, "%s%s", !hit ? "" : ", ", priority_to_string(1u << j));
      }
      }
   fprintf(f, "\nNote: The holes represent memory not used by the IB.\n"
      }
      static void si_dump_framebuffer(struct si_context *sctx, struct u_log_context *log)
   {
      struct pipe_framebuffer_state *state = &sctx->framebuffer.state;
   struct si_texture *tex;
            for (i = 0; i < state->nr_cbufs; i++) {
      if (!state->cbufs[i])
            tex = (struct si_texture *)state->cbufs[i]->texture;
   u_log_printf(log, COLOR_YELLOW "Color buffer %i:" COLOR_RESET "\n", i);
   si_print_texture_info(sctx->screen, tex, log);
               if (state->zsbuf) {
      tex = (struct si_texture *)state->zsbuf->texture;
   u_log_printf(log, COLOR_YELLOW "Depth-stencil buffer:" COLOR_RESET "\n");
   si_print_texture_info(sctx->screen, tex, log);
         }
      typedef unsigned (*slot_remap_func)(unsigned);
      struct si_log_chunk_desc_list {
      /** Pointer to memory map of buffer where the list is uploader */
   uint32_t *gpu_list;
   /** Reference of buffer where the list is uploaded, so that gpu_list
   * is kept live. */
            const char *shader_name;
   const char *elem_name;
   slot_remap_func slot_remap;
   enum amd_gfx_level gfx_level;
   enum radeon_family family;
   unsigned element_dw_size;
               };
      static void si_log_chunk_desc_list_destroy(void *data)
   {
      struct si_log_chunk_desc_list *chunk = data;
   si_resource_reference(&chunk->buf, NULL);
      }
      static void si_log_chunk_desc_list_print(void *data, FILE *f)
   {
      struct si_log_chunk_desc_list *chunk = data;
   unsigned sq_img_rsrc_word0 =
            for (unsigned i = 0; i < chunk->num_elements; i++) {
      unsigned cpu_dw_offset = i * chunk->element_dw_size;
   unsigned gpu_dw_offset = chunk->slot_remap(i) * chunk->element_dw_size;
   const char *list_note = chunk->gpu_list ? "GPU list" : "CPU list";
   uint32_t *cpu_list = chunk->list + cpu_dw_offset;
            fprintf(f, COLOR_GREEN "%s%s slot %u (%s):" COLOR_RESET "\n", chunk->shader_name,
            switch (chunk->element_dw_size) {
   case 4:
      for (unsigned j = 0; j < 4; j++)
      ac_dump_reg(f, chunk->gfx_level, chunk->family,
         case 8:
      for (unsigned j = 0; j < 8; j++)
                  fprintf(f, COLOR_CYAN "    Buffer:" COLOR_RESET "\n");
   for (unsigned j = 0; j < 4; j++)
      ac_dump_reg(f, chunk->gfx_level, chunk->family,
         case 16:
      for (unsigned j = 0; j < 8; j++)
                  fprintf(f, COLOR_CYAN "    Buffer:" COLOR_RESET "\n");
   for (unsigned j = 0; j < 4; j++)
                  fprintf(f, COLOR_CYAN "    FMASK:" COLOR_RESET "\n");
   for (unsigned j = 0; j < 8; j++)
                  fprintf(f, COLOR_CYAN "    Sampler state:" COLOR_RESET "\n");
   for (unsigned j = 0; j < 4; j++)
      ac_dump_reg(f, chunk->gfx_level, chunk->family,
                  if (memcmp(gpu_list, cpu_list, chunk->element_dw_size * 4) != 0) {
                        }
      static const struct u_log_chunk_type si_log_chunk_type_descriptor_list = {
      .destroy = si_log_chunk_desc_list_destroy,
      };
      static void si_dump_descriptor_list(struct si_screen *screen, struct si_descriptors *desc,
                     {
      if (!desc->list)
            /* In some cases, the caller doesn't know how many elements are really
   * uploaded. Reduce num_elements to fit in the range of active slots. */
   unsigned active_range_dw_begin = desc->first_active_slot * desc->element_dw_size;
   unsigned active_range_dw_end =
            while (num_elements > 0) {
      int i = slot_remap(num_elements - 1);
   unsigned dw_begin = i * element_dw_size;
            if (dw_begin >= active_range_dw_begin && dw_end <= active_range_dw_end)
                        struct si_log_chunk_desc_list *chunk =
         chunk->shader_name = shader_name;
   chunk->elem_name = elem_name;
   chunk->element_dw_size = element_dw_size;
   chunk->num_elements = num_elements;
   chunk->slot_remap = slot_remap;
   chunk->gfx_level = screen->info.gfx_level;
            si_resource_reference(&chunk->buf, desc->buffer);
            for (unsigned i = 0; i < num_elements; ++i) {
      memcpy(&chunk->list[i * element_dw_size], &desc->list[slot_remap(i) * element_dw_size],
                  }
      static unsigned si_identity(unsigned slot)
   {
         }
      static void si_dump_descriptors(struct si_context *sctx, gl_shader_stage stage,
         {
      enum pipe_shader_type processor = pipe_shader_type_from_mesa(stage);
   struct si_descriptors *descs =
         static const char *shader_name[] = {"VS", "PS", "GS", "TCS", "TES", "CS"};
   const char *name = shader_name[processor];
   unsigned enabled_constbuf, enabled_shaderbuf, enabled_samplers;
            if (info) {
      enabled_constbuf = u_bit_consecutive(0, info->base.num_ubos);
   enabled_shaderbuf = u_bit_consecutive(0, info->base.num_ssbos);
   enabled_samplers = info->base.textures_used[0];
      } else {
      enabled_constbuf =
         enabled_shaderbuf = 0;
   for (int i = 0; i < SI_NUM_SHADER_BUFFERS; i++) {
      enabled_shaderbuf |=
      (sctx->const_and_shader_buffers[processor].enabled_mask &
   }
   enabled_samplers = sctx->samplers[processor].enabled_mask;
               si_dump_descriptor_list(sctx->screen, &descs[SI_SHADER_DESCS_CONST_AND_SHADER_BUFFERS], name,
               si_dump_descriptor_list(sctx->screen, &descs[SI_SHADER_DESCS_CONST_AND_SHADER_BUFFERS], name,
               si_dump_descriptor_list(sctx->screen, &descs[SI_SHADER_DESCS_SAMPLERS_AND_IMAGES], name,
               si_dump_descriptor_list(sctx->screen, &descs[SI_SHADER_DESCS_SAMPLERS_AND_IMAGES], name,
      }
      static void si_dump_gfx_descriptors(struct si_context *sctx,
               {
      if (!state->cso || !state->current)
               }
      static void si_dump_compute_descriptors(struct si_context *sctx, struct u_log_context *log)
   {
      if (!sctx->cs_shader_state.program)
               }
      struct si_shader_inst {
      const char *text; /* start of disassembly for this instruction */
   unsigned textlen;
   unsigned size; /* instruction size = 4 or 8 */
      };
      /**
   * Open the given \p binary as \p rtld_binary and split the contained
   * disassembly string into instructions and add them to the array
   * pointed to by \p instructions, which must be sufficiently large.
   *
   * Labels are considered to be part of the following instruction.
   *
   * The caller must keep \p rtld_binary alive as long as \p instructions are
   * used and then close it afterwards.
   */
   static void si_add_split_disasm(struct si_screen *screen, struct ac_rtld_binary *rtld_binary,
                     {
      if (!ac_rtld_open(rtld_binary, (struct ac_rtld_open_info){
                                    .info = &screen->info,
         const char *disasm;
   size_t nbytes;
   if (!ac_rtld_get_section_by_name(rtld_binary, ".AMDGPU.disasm", &disasm, &nbytes))
            const char *end = disasm + nbytes;
   while (disasm < end) {
      const char *semicolon = memchr(disasm, ';', end - disasm);
   if (!semicolon)
            struct si_shader_inst *inst = &instructions[(*num)++];
   const char *inst_end = memchr(semicolon + 1, '\n', end - semicolon - 1);
   if (!inst_end)
            inst->text = disasm;
            inst->addr = *addr;
   /* More than 16 chars after ";" means the instruction is 8 bytes long. */
   inst->size = inst_end - semicolon > 16 ? 8 : 4;
            if (inst_end == end)
               }
      /* If the shader is being executed, print its asm instructions, and annotate
   * those that are being executed right now with information about waves that
   * execute them. This is most useful during a GPU hang.
   */
   static void si_print_annotated_shader(struct si_shader *shader, struct ac_wave_info *waves,
         {
      if (!shader)
            struct si_screen *screen = shader->selector->screen;
   gl_shader_stage stage = shader->selector->stage;
   uint64_t start_addr = shader->bo->gpu_address;
   uint64_t end_addr = start_addr + shader->bo->b.b.width0;
            /* See if any wave executes the shader. */
   for (i = 0; i < num_waves; i++) {
      if (start_addr <= waves[i].pc && waves[i].pc <= end_addr)
      }
   if (i == num_waves)
            /* Remember the first found wave. The waves are sorted according to PC. */
   waves = &waves[i];
            /* Get the list of instructions.
   * Buffer size / 4 is the upper bound of the instruction count.
   */
   unsigned num_inst = 0;
   uint64_t inst_addr = start_addr;
   struct ac_rtld_binary rtld_binaries[5] = {};
   struct si_shader_inst *instructions =
            if (shader->prolog) {
      si_add_split_disasm(screen, &rtld_binaries[0], &shader->prolog->binary, &inst_addr, &num_inst,
      }
   if (shader->previous_stage) {
      si_add_split_disasm(screen, &rtld_binaries[1], &shader->previous_stage->binary, &inst_addr,
      }
   si_add_split_disasm(screen, &rtld_binaries[3], &shader->binary, &inst_addr, &num_inst,
         if (shader->epilog) {
      si_add_split_disasm(screen, &rtld_binaries[4], &shader->epilog->binary, &inst_addr, &num_inst,
               fprintf(f, COLOR_YELLOW "%s - annotated disassembly:" COLOR_RESET "\n",
            /* Print instructions with annotations. */
   for (i = 0; i < num_inst; i++) {
               fprintf(f, "%.*s [PC=0x%" PRIx64 ", size=%u]\n", inst->textlen, inst->text, inst->addr,
            /* Print which waves execute the instruction right now. */
   while (num_waves && inst->addr == waves->pc) {
      fprintf(f,
                        if (inst->size == 4) {
         } else {
                  waves->matched = true;
   waves = &waves[1];
                  fprintf(f, "\n\n");
   free(instructions);
   for (unsigned i = 0; i < ARRAY_SIZE(rtld_binaries); ++i)
      }
      static void si_dump_annotated_shaders(struct si_context *sctx, FILE *f)
   {
      struct ac_wave_info waves[AC_MAX_WAVES_PER_CHIP];
                     si_print_annotated_shader(sctx->shader.vs.current, waves, num_waves, f);
   si_print_annotated_shader(sctx->shader.tcs.current, waves, num_waves, f);
   si_print_annotated_shader(sctx->shader.tes.current, waves, num_waves, f);
   si_print_annotated_shader(sctx->shader.gs.current, waves, num_waves, f);
            /* Print waves executing shaders that are not currently bound. */
   unsigned i;
   bool found = false;
   for (i = 0; i < num_waves; i++) {
      if (waves[i].matched)
            if (!found) {
      fprintf(f, COLOR_CYAN "Waves not executing currently-bound shaders:" COLOR_RESET "\n");
      }
   fprintf(f,
         "    SE%u SH%u CU%u SIMD%u WAVE%u  EXEC=%016" PRIx64 "  INST=%08X %08X  PC=%" PRIx64
   "\n",
      }
   if (found)
      }
      static void si_dump_command(const char *title, const char *command, FILE *f)
   {
               FILE *p = popen(command, "r");
   if (!p)
            fprintf(f, COLOR_YELLOW "%s: " COLOR_RESET "\n", title);
   while (fgets(line, sizeof(line), p))
         fprintf(f, "\n\n");
      }
      static void si_dump_debug_state(struct pipe_context *ctx, FILE *f, unsigned flags)
   {
               if (sctx->log)
            if (flags & PIPE_DUMP_DEVICE_STATUS_REGISTERS) {
               si_dump_annotated_shaders(sctx, f);
   si_dump_command("Active waves (raw data)", "umr -O halt_waves -wa | column -t", f);
         }
      void si_log_draw_state(struct si_context *sctx, struct u_log_context *log)
   {
      if (!log)
                     si_dump_gfx_shader(sctx, &sctx->shader.vs, log);
   si_dump_gfx_shader(sctx, &sctx->shader.tcs, log);
   si_dump_gfx_shader(sctx, &sctx->shader.tes, log);
   si_dump_gfx_shader(sctx, &sctx->shader.gs, log);
            si_dump_descriptor_list(sctx->screen, &sctx->descriptors[SI_DESCS_INTERNAL], "", "RW buffers",
               si_dump_gfx_descriptors(sctx, &sctx->shader.vs, log);
   si_dump_gfx_descriptors(sctx, &sctx->shader.tcs, log);
   si_dump_gfx_descriptors(sctx, &sctx->shader.tes, log);
   si_dump_gfx_descriptors(sctx, &sctx->shader.gs, log);
      }
      void si_log_compute_state(struct si_context *sctx, struct u_log_context *log)
   {
      if (!log)
            si_dump_compute_shader(sctx, log);
      }
      void si_check_vm_faults(struct si_context *sctx, struct radeon_saved_cs *saved, enum amd_ip_type ring)
   {
      struct pipe_screen *screen = sctx->b.screen;
   FILE *f;
   uint64_t addr;
            if (!ac_vm_fault_occurred(sctx->gfx_level, &sctx->dmesg_timestamp, &addr))
            f = dd_get_debug_file(false);
   if (!f)
            fprintf(f, "VM fault report.\n\n");
   if (util_get_command_line(cmd_line, sizeof(cmd_line)))
         fprintf(f, "Driver vendor: %s\n", screen->get_vendor(screen));
   fprintf(f, "Device vendor: %s\n", screen->get_device_vendor(screen));
   fprintf(f, "Device name: %s\n\n", screen->get_name(screen));
            if (sctx->apitrace_call_number)
            switch (ring) {
   case AMD_IP_GFX: {
      struct u_log_context log;
            si_log_draw_state(sctx, &log);
   si_log_compute_state(sctx, &log);
            u_log_new_page_print(&log, f);
   u_log_context_destroy(&log);
               default:
                           fprintf(stderr, "Detected a VM fault, exiting...\n");
      }
      void si_init_debug_functions(struct si_context *sctx)
   {
               /* Set the initial dmesg timestamp for this context, so that
   * only new messages will be checked for VM faults.
   */
   if (sctx->screen->debug_flags & DBG(CHECK_VM))
      }
