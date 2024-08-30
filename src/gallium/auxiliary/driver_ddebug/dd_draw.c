   /**************************************************************************
   *
   * Copyright 2015 Advanced Micro Devices, Inc.
   * Copyright 2008 VMware, Inc.
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "dd_pipe.h"
      #include "util/u_dump.h"
   #include "util/format/u_format.h"
   #include "util/u_framebuffer.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_process.h"
   #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_scan.h"
   #include "util/os_time.h"
   #include <inttypes.h>
   #include "util/detect.h"
      void
   dd_get_debug_filename_and_mkdir(char *buf, size_t buflen, bool verbose)
   {
      static unsigned index;
   char dir[256];
            if (!proc_name) {
      fprintf(stderr, "dd: can't get the process name\n");
                        if (mkdir(dir, 0774) && errno != EEXIST)
            snprintf(buf, buflen, "%s/%s_%u_%08u", dir, proc_name, (unsigned int)getpid(),
            if (verbose)
      }
      FILE *
   dd_get_debug_file(bool verbose)
   {
      char name[512];
            dd_get_debug_filename_and_mkdir(name, sizeof(name), verbose);
   f = fopen(name, "w");
   if (!f) {
      fprintf(stderr, "dd: can't open file %s\n", name);
                  }
      void
   dd_parse_apitrace_marker(const char *string, int len, unsigned *call_number)
   {
      unsigned num;
            if (len <= 0)
            /* Make it zero-terminated. */
   s = alloca(len + 1);
   memcpy(s, string, len);
            /* Parse the number. */
   errno = 0;
   num = strtol(s, NULL, 10);
   if (errno)
               }
      void
   dd_write_header(FILE *f, struct pipe_screen *screen, unsigned apitrace_call_number)
   {
      char cmd_line[4096];
   if (util_get_command_line(cmd_line, sizeof(cmd_line)))
         fprintf(f, "Driver vendor: %s\n", screen->get_vendor(screen));
   fprintf(f, "Device vendor: %s\n", screen->get_device_vendor(screen));
            if (apitrace_call_number)
      }
      FILE *
   dd_get_file_stream(struct dd_screen *dscreen, unsigned apitrace_call_number)
   {
               FILE *f = dd_get_debug_file(dscreen->verbose);
   if (!f)
            dd_write_header(f, screen, apitrace_call_number);
      }
      static void
   dd_dump_dmesg(FILE *f)
   {
   #if DETECT_OS_LINUX
      char line[2000];
            if (!p)
            fprintf(f, "\nLast 60 lines of dmesg:\n\n");
   while (fgets(line, sizeof(line), p))
               #endif
   }
      static unsigned
   dd_num_active_viewports(struct dd_draw_state *dstate)
   {
      struct tgsi_shader_info info;
            if (dstate->shaders[PIPE_SHADER_GEOMETRY])
         else if (dstate->shaders[PIPE_SHADER_TESS_EVAL])
         else if (dstate->shaders[PIPE_SHADER_VERTEX])
         else
            if (tokens) {
      tgsi_scan_shader(tokens, &info);
   if (info.writes_viewport_index)
                  }
      #define COLOR_RESET	"\033[0m"
   #define COLOR_SHADER	"\033[1;32m"
   #define COLOR_STATE	"\033[1;33m"
      #define DUMP(name, var) do { \
      fprintf(f, COLOR_STATE #name ": " COLOR_RESET); \
   util_dump_##name(f, var); \
      } while(0)
      #define DUMP_I(name, var, i) do { \
      fprintf(f, COLOR_STATE #name " %i: " COLOR_RESET, i); \
   util_dump_##name(f, var); \
      } while(0)
      #define DUMP_M(name, var, member) do { \
      fprintf(f, "  " #member ": "); \
   util_dump_##name(f, (var)->member); \
      } while(0)
      #define DUMP_M_ADDR(name, var, member) do { \
      fprintf(f, "  " #member ": "); \
   util_dump_##name(f, &(var)->member); \
      } while(0)
      #define PRINT_NAMED(type, name, value) \
   do { \
      fprintf(f, COLOR_STATE "%s" COLOR_RESET " = ", name); \
   util_dump_##type(f, value); \
      } while (0)
      static void
   util_dump_uint(FILE *f, unsigned i)
   {
         }
      static void
   util_dump_int(FILE *f, int i)
   {
         }
      static void
   util_dump_hex(FILE *f, unsigned i)
   {
         }
      static void
   util_dump_double(FILE *f, double d)
   {
         }
      static void
   util_dump_format(FILE *f, enum pipe_format format)
   {
         }
      static void
   util_dump_color_union(FILE *f, const union pipe_color_union *color)
   {
      fprintf(f, "{f = {%f, %f, %f, %f}, ui = {%u, %u, %u, %u}",
            }
      static void
   dd_dump_render_condition(struct dd_draw_state *dstate, FILE *f)
   {
      if (dstate->render_cond.query) {
      fprintf(f, "render condition:\n");
   DUMP_M(query_type, &dstate->render_cond, query->type);
   DUMP_M(uint, &dstate->render_cond, condition);
   DUMP_M(uint, &dstate->render_cond, mode);
         }
      static void
   dd_dump_shader(struct dd_draw_state *dstate, enum pipe_shader_type sh, FILE *f)
   {
      int i;
            shader_str[PIPE_SHADER_VERTEX] = "VERTEX";
   shader_str[PIPE_SHADER_TESS_CTRL] = "TESS_CTRL";
   shader_str[PIPE_SHADER_TESS_EVAL] = "TESS_EVAL";
   shader_str[PIPE_SHADER_GEOMETRY] = "GEOMETRY";
   shader_str[PIPE_SHADER_FRAGMENT] = "FRAGMENT";
            if (sh == PIPE_SHADER_TESS_CTRL &&
      !dstate->shaders[PIPE_SHADER_TESS_CTRL] &&
   dstate->shaders[PIPE_SHADER_TESS_EVAL])
   fprintf(f, "tess_state: {default_outer_level = {%f, %f, %f, %f}, "
         "default_inner_level = {%f, %f}}\n",
   dstate->tess_default_levels[0],
   dstate->tess_default_levels[1],
   dstate->tess_default_levels[2],
   dstate->tess_default_levels[3],
         if (sh == PIPE_SHADER_FRAGMENT)
      if (dstate->rs) {
                                             if (dstate->rs->state.rs.scissor)
                           if (dstate->rs->state.rs.poly_stipple_enable)
                  if (!dstate->shaders[sh])
            fprintf(f, COLOR_SHADER "begin shader: %s" COLOR_RESET "\n", shader_str[sh]);
            for (i = 0; i < PIPE_MAX_CONSTANT_BUFFERS; i++)
      if (dstate->constant_buffers[sh][i].buffer ||
            DUMP_I(constant_buffer, &dstate->constant_buffers[sh][i], i);
   if (dstate->constant_buffers[sh][i].buffer)
            for (i = 0; i < PIPE_MAX_SAMPLERS; i++)
      if (dstate->sampler_states[sh][i])
         for (i = 0; i < PIPE_MAX_SAMPLERS; i++)
      if (dstate->sampler_views[sh][i]) {
      DUMP_I(sampler_view, dstate->sampler_views[sh][i], i);
            for (i = 0; i < PIPE_MAX_SHADER_IMAGES; i++)
      if (dstate->shader_images[sh][i].resource) {
      DUMP_I(image_view, &dstate->shader_images[sh][i], i);
   if (dstate->shader_images[sh][i].resource)
            for (i = 0; i < PIPE_MAX_SHADER_BUFFERS; i++)
      if (dstate->shader_buffers[sh][i].buffer) {
      DUMP_I(shader_buffer, &dstate->shader_buffers[sh][i], i);
   if (dstate->shader_buffers[sh][i].buffer)
               }
      static void
   dd_dump_flush(struct dd_draw_state *dstate, struct call_flush *info, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
      }
      static void
   dd_dump_draw_vbo(struct dd_draw_state *dstate, struct pipe_draw_info *info,
                     {
               DUMP(draw_info, info);
   PRINT_NAMED(int, "drawid offset", drawid_offset);
   DUMP(draw_start_count_bias, draw);
   if (indirect) {
      if (indirect->buffer)
         if (indirect->indirect_draw_count)
         if (indirect->count_from_stream_output)
                                          for (i = 0; i < PIPE_MAX_ATTRIBS; i++)
      if (dstate->vertex_buffers[i].buffer.resource) {
      DUMP_I(vertex_buffer, &dstate->vertex_buffers[i], i);
   if (!dstate->vertex_buffers[i].is_user_buffer)
            if (dstate->velems) {
      PRINT_NAMED(uint, "num vertex elements",
         for (i = 0; i < dstate->velems->state.velems.count; i++) {
      fprintf(f, "  ");
                  PRINT_NAMED(uint, "num stream output targets", dstate->num_so_targets);
   for (i = 0; i < dstate->num_so_targets; i++)
      if (dstate->so_targets[i]) {
      DUMP_I(stream_output_target, dstate->so_targets[i], i);
   DUMP_M(resource, dstate->so_targets[i], buffer);
            fprintf(f, "\n");
   for (sh = 0; sh < PIPE_SHADER_TYPES; sh++) {
      if (sh == PIPE_SHADER_COMPUTE)
                        if (dstate->dsa)
                  if (dstate->blend)
                  PRINT_NAMED(uint, "min_samples", dstate->min_samples);
   PRINT_NAMED(hex, "sample_mask", dstate->sample_mask);
            DUMP(framebuffer_state, &dstate->framebuffer_state);
   for (i = 0; i < dstate->framebuffer_state.nr_cbufs; i++)
      if (dstate->framebuffer_state.cbufs[i]) {
      fprintf(f, "  " COLOR_STATE "cbufs[%i]:" COLOR_RESET "\n    ", i);
   DUMP(surface, dstate->framebuffer_state.cbufs[i]);
   fprintf(f, "    ");
         if (dstate->framebuffer_state.zsbuf) {
      fprintf(f, "  " COLOR_STATE "zsbuf:" COLOR_RESET "\n    ");
   DUMP(surface, dstate->framebuffer_state.zsbuf);
   fprintf(f, "    ");
      }
      }
      static void
   dd_dump_launch_grid(struct dd_draw_state *dstate, struct pipe_grid_info *info, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
   DUMP(grid_info, info);
            dd_dump_shader(dstate, PIPE_SHADER_COMPUTE, f);
      }
      static void
   dd_dump_resource_copy_region(struct dd_draw_state *dstate,
               {
      fprintf(f, "%s:\n", __func__+8);
   DUMP_M(resource, info, dst);
   DUMP_M(uint, info, dst_level);
   DUMP_M(uint, info, dstx);
   DUMP_M(uint, info, dsty);
   DUMP_M(uint, info, dstz);
   DUMP_M(resource, info, src);
   DUMP_M(uint, info, src_level);
      }
      static void
   dd_dump_blit(struct dd_draw_state *dstate, struct pipe_blit_info *info, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
   DUMP_M(resource, info, dst.resource);
   DUMP_M(uint, info, dst.level);
   DUMP_M_ADDR(box, info, dst.box);
            DUMP_M(resource, info, src.resource);
   DUMP_M(uint, info, src.level);
   DUMP_M_ADDR(box, info, src.box);
            DUMP_M(hex, info, mask);
   DUMP_M(uint, info, filter);
   DUMP_M(uint, info, scissor_enable);
   DUMP_M_ADDR(scissor_state, info, scissor);
            if (info->render_condition_enable)
      }
      static void
   dd_dump_generate_mipmap(struct dd_draw_state *dstate, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
      }
      static void
   dd_dump_get_query_result_resource(struct call_get_query_result_resource *info, FILE *f)
   {
      fprintf(f, "%s:\n", __func__ + 8);
   DUMP_M(query_type, info, query_type);
   DUMP_M(query_flags, info, flags);
   DUMP_M(query_value_type, info, result_type);
   DUMP_M(int, info, index);
   DUMP_M(resource, info, resource);
      }
      static void
   dd_dump_flush_resource(struct dd_draw_state *dstate, struct pipe_resource *res,
         {
      fprintf(f, "%s:\n", __func__+8);
      }
      static void
   dd_dump_clear(struct dd_draw_state *dstate, struct call_clear *info, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
   DUMP_M(uint, info, buffers);
   fprintf(f, "  scissor_state: %d,%d %d,%d\n",
               DUMP_M_ADDR(color_union, info, color);
   DUMP_M(double, info, depth);
      }
      static void
   dd_dump_clear_buffer(struct dd_draw_state *dstate, struct call_clear_buffer *info,
         {
      int i;
            fprintf(f, "%s:\n", __func__+8);
   DUMP_M(resource, info, res);
   DUMP_M(uint, info, offset);
   DUMP_M(uint, info, size);
            fprintf(f, "  clear_value:");
   for (i = 0; i < info->clear_value_size; i++)
            }
      static void
   dd_dump_transfer_map(struct call_transfer_map *info, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
   DUMP_M_ADDR(transfer, info, transfer);
   DUMP_M(ptr, info, transfer_ptr);
      }
      static void
   dd_dump_transfer_flush_region(struct call_transfer_flush_region *info, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
   DUMP_M_ADDR(transfer, info, transfer);
   DUMP_M(ptr, info, transfer_ptr);
      }
      static void
   dd_dump_transfer_unmap(struct call_transfer_unmap *info, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
   DUMP_M_ADDR(transfer, info, transfer);
      }
      static void
   dd_dump_buffer_subdata(struct call_buffer_subdata *info, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
   DUMP_M(resource, info, resource);
   DUMP_M(transfer_usage, info, usage);
   DUMP_M(uint, info, offset);
   DUMP_M(uint, info, size);
      }
      static void
   dd_dump_texture_subdata(struct call_texture_subdata *info, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
   DUMP_M(resource, info, resource);
   DUMP_M(uint, info, level);
   DUMP_M(transfer_usage, info, usage);
   DUMP_M_ADDR(box, info, box);
   DUMP_M(ptr, info, data);
   DUMP_M(uint, info, stride);
      }
      static void
   dd_dump_clear_texture(struct dd_draw_state *dstate, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
      }
      static void
   dd_dump_clear_render_target(struct dd_draw_state *dstate, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
      }
      static void
   dd_dump_clear_depth_stencil(struct dd_draw_state *dstate, FILE *f)
   {
      fprintf(f, "%s:\n", __func__+8);
      }
      static void
   dd_dump_driver_state(struct dd_context *dctx, FILE *f, unsigned flags)
   {
      if (dctx->pipe->dump_debug_state) {
   fprintf(f,"\n\n**************************************************"
         fprintf(f, "Driver-specific state:\n\n");
   dctx->pipe->dump_debug_state(dctx->pipe, f, flags);
      }
      static void
   dd_dump_call(FILE *f, struct dd_draw_state *state, struct dd_call *call)
   {
      switch (call->type) {
   case CALL_FLUSH:
      dd_dump_flush(state, &call->info.flush, f);
      case CALL_DRAW_VBO:
      dd_dump_draw_vbo(state, &call->info.draw_vbo.info,
                        case CALL_LAUNCH_GRID:
      dd_dump_launch_grid(state, &call->info.launch_grid, f);
      case CALL_RESOURCE_COPY_REGION:
      dd_dump_resource_copy_region(state,
            case CALL_BLIT:
      dd_dump_blit(state, &call->info.blit, f);
      case CALL_FLUSH_RESOURCE:
      dd_dump_flush_resource(state, call->info.flush_resource, f);
      case CALL_CLEAR:
      dd_dump_clear(state, &call->info.clear, f);
      case CALL_CLEAR_BUFFER:
      dd_dump_clear_buffer(state, &call->info.clear_buffer, f);
      case CALL_CLEAR_TEXTURE:
      dd_dump_clear_texture(state, f);
      case CALL_CLEAR_RENDER_TARGET:
      dd_dump_clear_render_target(state, f);
      case CALL_CLEAR_DEPTH_STENCIL:
      dd_dump_clear_depth_stencil(state, f);
      case CALL_GENERATE_MIPMAP:
      dd_dump_generate_mipmap(state, f);
      case CALL_GET_QUERY_RESULT_RESOURCE:
      dd_dump_get_query_result_resource(&call->info.get_query_result_resource, f);
      case CALL_TRANSFER_MAP:
      dd_dump_transfer_map(&call->info.transfer_map, f);
      case CALL_TRANSFER_FLUSH_REGION:
      dd_dump_transfer_flush_region(&call->info.transfer_flush_region, f);
      case CALL_TRANSFER_UNMAP:
      dd_dump_transfer_unmap(&call->info.transfer_unmap, f);
      case CALL_BUFFER_SUBDATA:
      dd_dump_buffer_subdata(&call->info.buffer_subdata, f);
      case CALL_TEXTURE_SUBDATA:
      dd_dump_texture_subdata(&call->info.texture_subdata, f);
         }
      static void
   dd_kill_process(void)
   {
   #if DETECT_OS_UNIX
         #endif
      fprintf(stderr, "dd: Aborting the process...\n");
   fflush(stdout);
   fflush(stderr);
      }
      static void
   dd_unreference_copy_of_call(struct dd_call *dst)
   {
      switch (dst->type) {
   case CALL_FLUSH:
         case CALL_DRAW_VBO:
      pipe_so_target_reference(&dst->info.draw_vbo.indirect.count_from_stream_output, NULL);
   pipe_resource_reference(&dst->info.draw_vbo.indirect.buffer, NULL);
   pipe_resource_reference(&dst->info.draw_vbo.indirect.indirect_draw_count, NULL);
   if (dst->info.draw_vbo.info.index_size &&
      !dst->info.draw_vbo.info.has_user_indices)
      else
            case CALL_LAUNCH_GRID:
      pipe_resource_reference(&dst->info.launch_grid.indirect, NULL);
      case CALL_RESOURCE_COPY_REGION:
      pipe_resource_reference(&dst->info.resource_copy_region.dst, NULL);
   pipe_resource_reference(&dst->info.resource_copy_region.src, NULL);
      case CALL_BLIT:
      pipe_resource_reference(&dst->info.blit.dst.resource, NULL);
   pipe_resource_reference(&dst->info.blit.src.resource, NULL);
      case CALL_FLUSH_RESOURCE:
      pipe_resource_reference(&dst->info.flush_resource, NULL);
      case CALL_CLEAR:
         case CALL_CLEAR_BUFFER:
      pipe_resource_reference(&dst->info.clear_buffer.res, NULL);
      case CALL_CLEAR_TEXTURE:
         case CALL_CLEAR_RENDER_TARGET:
         case CALL_CLEAR_DEPTH_STENCIL:
         case CALL_GENERATE_MIPMAP:
      pipe_resource_reference(&dst->info.generate_mipmap.res, NULL);
      case CALL_GET_QUERY_RESULT_RESOURCE:
      pipe_resource_reference(&dst->info.get_query_result_resource.resource, NULL);
      case CALL_TRANSFER_MAP:
      pipe_resource_reference(&dst->info.transfer_map.transfer.resource, NULL);
      case CALL_TRANSFER_FLUSH_REGION:
      pipe_resource_reference(&dst->info.transfer_flush_region.transfer.resource, NULL);
      case CALL_TRANSFER_UNMAP:
      pipe_resource_reference(&dst->info.transfer_unmap.transfer.resource, NULL);
      case CALL_BUFFER_SUBDATA:
      pipe_resource_reference(&dst->info.buffer_subdata.resource, NULL);
      case CALL_TEXTURE_SUBDATA:
      pipe_resource_reference(&dst->info.texture_subdata.resource, NULL);
         }
      static void
   dd_init_copy_of_draw_state(struct dd_draw_state_copy *state)
   {
               /* Just clear pointers to gallium objects. Don't clear the whole structure,
   * because it would kill performance with its size of 130 KB.
   */
   memset(state->base.vertex_buffers, 0,
         memset(state->base.so_targets, 0,
         memset(state->base.constant_buffers, 0,
         memset(state->base.sampler_views, 0,
         memset(state->base.shader_images, 0,
         memset(state->base.shader_buffers, 0,
         memset(&state->base.framebuffer_state, 0,
                              for (i = 0; i < PIPE_SHADER_TYPES; i++) {
      state->base.shaders[i] = &state->shaders[i];
   for (j = 0; j < PIPE_MAX_SAMPLERS; j++)
               state->base.velems = &state->velems;
   state->base.rs = &state->rs;
   state->base.dsa = &state->dsa;
      }
      static void
   dd_unreference_copy_of_draw_state(struct dd_draw_state_copy *state)
   {
      struct dd_draw_state *dst = &state->base;
            for (i = 0; i < ARRAY_SIZE(dst->vertex_buffers); i++)
         for (i = 0; i < ARRAY_SIZE(dst->so_targets); i++)
            for (i = 0; i < PIPE_SHADER_TYPES; i++) {
      if (dst->shaders[i])
            for (j = 0; j < PIPE_MAX_CONSTANT_BUFFERS; j++)
         for (j = 0; j < PIPE_MAX_SAMPLERS; j++)
         for (j = 0; j < PIPE_MAX_SHADER_IMAGES; j++)
         for (j = 0; j < PIPE_MAX_SHADER_BUFFERS; j++)
                  }
      static void
   dd_copy_draw_state(struct dd_draw_state *dst, struct dd_draw_state *src)
   {
               if (src->render_cond.query) {
      *dst->render_cond.query = *src->render_cond.query;
   dst->render_cond.condition = src->render_cond.condition;
      } else {
                  for (i = 0; i < ARRAY_SIZE(src->vertex_buffers); i++) {
      pipe_vertex_buffer_reference(&dst->vertex_buffers[i],
               dst->num_so_targets = src->num_so_targets;
   for (i = 0; i < src->num_so_targets; i++)
                  for (i = 0; i < PIPE_SHADER_TYPES; i++) {
      if (!src->shaders[i]) {
      dst->shaders[i] = NULL;
               if (src->shaders[i]) {
      dst->shaders[i]->state.shader = src->shaders[i]->state.shader;
   if (src->shaders[i]->state.shader.tokens) {
      dst->shaders[i]->state.shader.tokens =
      } else {
            } else {
                  for (j = 0; j < PIPE_MAX_CONSTANT_BUFFERS; j++) {
      pipe_resource_reference(&dst->constant_buffers[i][j].buffer,
         memcpy(&dst->constant_buffers[i][j], &src->constant_buffers[i][j],
               for (j = 0; j < PIPE_MAX_SAMPLERS; j++) {
      pipe_sampler_view_reference(&dst->sampler_views[i][j],
         if (src->sampler_states[i][j])
      dst->sampler_states[i][j]->state.sampler =
      else
               for (j = 0; j < PIPE_MAX_SHADER_IMAGES; j++) {
      pipe_resource_reference(&dst->shader_images[i][j].resource,
         memcpy(&dst->shader_images[i][j], &src->shader_images[i][j],
               for (j = 0; j < PIPE_MAX_SHADER_BUFFERS; j++) {
      pipe_resource_reference(&dst->shader_buffers[i][j].buffer,
         memcpy(&dst->shader_buffers[i][j], &src->shader_buffers[i][j],
                  if (src->velems)
         else
            if (src->rs)
         else
            if (src->dsa)
         else
            if (src->blend)
         else
            dst->blend_color = src->blend_color;
   dst->stencil_ref = src->stencil_ref;
   dst->sample_mask = src->sample_mask;
   dst->min_samples = src->min_samples;
   dst->clip_state = src->clip_state;
   util_copy_framebuffer_state(&dst->framebuffer_state, &src->framebuffer_state);
   memcpy(dst->scissors, src->scissors, sizeof(src->scissors));
   memcpy(dst->viewports, src->viewports, sizeof(src->viewports));
   memcpy(dst->tess_default_levels, src->tess_default_levels,
            }
      static void
   dd_free_record(struct pipe_screen *screen, struct dd_draw_record *record)
   {
      u_log_page_destroy(record->log_page);
   dd_unreference_copy_of_call(&record->call);
   dd_unreference_copy_of_draw_state(&record->draw_state);
   screen->fence_reference(screen, &record->prev_bottom_of_pipe, NULL);
   screen->fence_reference(screen, &record->top_of_pipe, NULL);
   screen->fence_reference(screen, &record->bottom_of_pipe, NULL);
   util_queue_fence_destroy(&record->driver_finished);
      }
      static void
   dd_write_record(FILE *f, struct dd_draw_record *record)
   {
      PRINT_NAMED(ptr, "pipe", record->dctx->pipe);
   PRINT_NAMED(ns, "time before (API call)", record->time_before);
   PRINT_NAMED(ns, "time after (driver done)", record->time_after);
                     if (record->log_page) {
      fprintf(f,"\n\n**************************************************"
         fprintf(f, "Context Log:\n\n");
         }
      static void
   dd_maybe_dump_record(struct dd_screen *dscreen, struct dd_draw_record *record)
   {
      if (dscreen->dump_mode == DD_DUMP_ONLY_HANGS ||
      (dscreen->dump_mode == DD_DUMP_APITRACE_CALL &&
   dscreen->apitrace_dump_call != record->draw_state.base.apitrace_call_number))
         char name[512];
   dd_get_debug_filename_and_mkdir(name, sizeof(name), dscreen->verbose);
   FILE *f = fopen(name, "w");
   if (!f) {
      fprintf(stderr, "dd: failed to open %s\n", name);
               dd_write_header(f, dscreen->screen, record->draw_state.base.apitrace_call_number);
               }
      static const char *
   dd_fence_state(struct pipe_screen *screen, struct pipe_fence_handle *fence,
         {
      if (!fence)
                     if (not_reached && !ok)
               }
      static void
   dd_report_hang(struct dd_context *dctx)
   {
      struct dd_screen *dscreen = dd_screen(dctx->base.screen);
   struct pipe_screen *screen = dscreen->screen;
   bool encountered_hang = false;
   bool stop_output = false;
                     fprintf(stderr, "Draw #   driver  prev BOP  TOP  BOP  dump file\n"
            list_for_each_entry(struct dd_draw_record, record, &dctx->records, list) {
      if (!encountered_hang &&
      screen->fence_finish(screen, NULL, record->bottom_of_pipe, 0)) {
   dd_maybe_dump_record(dscreen, record);
               if (stop_output) {
      dd_maybe_dump_record(dscreen, record);
   num_later++;
               bool driver = util_queue_fence_is_signalled(&record->driver_finished);
   bool top_not_reached = false;
   const char *prev_bop = dd_fence_state(screen, record->prev_bottom_of_pipe, NULL);
   const char *top = dd_fence_state(screen, record->top_of_pipe, &top_not_reached);
            fprintf(stderr, "%-9u %s      %s     %s  %s  ",
            char name[512];
            FILE *f = fopen(name, "w");
   if (!f) {
         } else {
                                          if (top_not_reached)
                     if (num_later)
            char name[512];
   dd_get_debug_filename_and_mkdir(name, sizeof(name), false);
   FILE *f = fopen(name, "w");
   if (!f) {
         } else {
      dd_write_header(f, dscreen->screen, 0);
   dd_dump_driver_state(dctx, f, PIPE_DUMP_DEVICE_STATUS_REGISTERS);
   dd_dump_dmesg(f);
               fprintf(stderr, "\nDone.\n");
      }
      int
   dd_thread_main(void *input)
   {
      struct dd_context *dctx = (struct dd_context *)input;
   struct dd_screen *dscreen = dd_screen(dctx->base.screen);
            const char *process_name = util_get_process_name();
   if (process_name) {
      char threadname[16];
   snprintf(threadname, sizeof(threadname), "%.*s:ddbg",
                                    for (;;) {
      struct list_head records;
   list_replace(&dctx->records, &records);
   list_inithead(&dctx->records);
            if (dctx->api_stalled)
            if (list_is_empty(&records)) {
                     cnd_wait(&dctx->cond, &dctx->mutex);
                        /* Wait for the youngest draw. This means hangs can take a bit longer
   * to detect, but it's more efficient this way.  */
   struct dd_draw_record *youngest =
            if (dscreen->timeout_ms > 0) {
                     if (!util_queue_fence_wait_timeout(&youngest->driver_finished, abs_timeout) ||
      !screen->fence_finish(screen, NULL, youngest->bottom_of_pipe,
         mtx_lock(&dctx->mutex);
   list_splice(&records, &dctx->records);
   dd_report_hang(dctx);
   /* we won't actually get here */
         } else {
                  list_for_each_entry_safe(struct dd_draw_record, record, &records, list) {
      dd_maybe_dump_record(dscreen, record);
   list_del(&record->list);
                  }
   mtx_unlock(&dctx->mutex);
      }
      static struct dd_draw_record *
   dd_create_record(struct dd_context *dctx)
   {
               record = MALLOC_STRUCT(dd_draw_record);
   if (!record)
            record->dctx = dctx;
            record->prev_bottom_of_pipe = NULL;
   record->top_of_pipe = NULL;
   record->bottom_of_pipe = NULL;
   record->log_page = NULL;
   util_queue_fence_init(&record->driver_finished);
            dd_init_copy_of_draw_state(&record->draw_state);
               }
      static void
   dd_add_record(struct dd_context *dctx, struct dd_draw_record *record)
   {
      mtx_lock(&dctx->mutex);
   if (unlikely(dctx->num_records > 10000)) {
      dctx->api_stalled = true;
   /* Since this is only a heuristic to prevent the API thread from getting
   * too far ahead, we don't need a loop here. */
   cnd_wait(&dctx->cond, &dctx->mutex);
               if (list_is_empty(&dctx->records))
            list_addtail(&record->list, &dctx->records);
   dctx->num_records++;
      }
      static void
   dd_before_draw(struct dd_context *dctx, struct dd_draw_record *record)
   {
      struct dd_screen *dscreen = dd_screen(dctx->base.screen);
   struct pipe_context *pipe = dctx->pipe;
                     if (dscreen->timeout_ms > 0) {
      if (dscreen->flush_always && dctx->num_draw_calls >= dscreen->skip_count) {
      pipe->flush(pipe, &record->prev_bottom_of_pipe, 0);
      } else {
      pipe->flush(pipe, &record->prev_bottom_of_pipe,
         pipe->flush(pipe, &record->top_of_pipe,
         } else if (dscreen->flush_always && dctx->num_draw_calls >= dscreen->skip_count) {
                     }
      static void
   dd_after_draw_async(void *data)
   {
      struct dd_draw_record *record = (struct dd_draw_record *)data;
   struct dd_context *dctx = record->dctx;
            record->log_page = u_log_new_page(&dctx->log);
                     if (dscreen->dump_mode == DD_DUMP_APITRACE_CALL &&
      dscreen->apitrace_dump_call > dctx->draw_state.apitrace_call_number) {
   dd_thread_join(dctx);
   /* No need to continue. */
         }
      static void
   dd_after_draw(struct dd_context *dctx, struct dd_draw_record *record)
   {
      struct dd_screen *dscreen = dd_screen(dctx->base.screen);
            if (dscreen->timeout_ms > 0) {
      unsigned flush_flags;
   if (dscreen->flush_always && dctx->num_draw_calls >= dscreen->skip_count)
         else
                     if (pipe->callback) {
         } else {
                  ++dctx->num_draw_calls;
   if (dscreen->skip_count && dctx->num_draw_calls % 10000 == 0)
      fprintf(stderr, "Gallium debugger reached %u draw calls.\n",
   }
      static void
   dd_context_flush(struct pipe_context *_pipe,
         {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
   struct pipe_screen *screen = pipe->screen;
            record->call.type = CALL_FLUSH;
                              pipe->flush(pipe, &record->bottom_of_pipe, flags);
   if (fence)
            if (pipe->callback) {
         } else {
            }
      static void
   dd_context_draw_vbo(struct pipe_context *_pipe,
                     const struct pipe_draw_info *info,
   unsigned drawid_offset,
   {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
            record->call.type = CALL_DRAW_VBO;
   record->call.info.draw_vbo.info = *info;
   record->call.info.draw_vbo.drawid_offset = drawid_offset;
   record->call.info.draw_vbo.draw = draws[0];
   if (info->index_size && !info->has_user_indices) {
      record->call.info.draw_vbo.info.index.resource = NULL;
   pipe_resource_reference(&record->call.info.draw_vbo.info.index.resource,
               if (indirect) {
      record->call.info.draw_vbo.indirect = *indirect;
   record->call.info.draw_vbo.indirect.buffer = NULL;
   pipe_resource_reference(&record->call.info.draw_vbo.indirect.buffer,
         record->call.info.draw_vbo.indirect.indirect_draw_count = NULL;
   pipe_resource_reference(&record->call.info.draw_vbo.indirect.indirect_draw_count,
         record->call.info.draw_vbo.indirect.count_from_stream_output = NULL;
   pipe_so_target_reference(&record->call.info.draw_vbo.indirect.count_from_stream_output,
      } else {
                  dd_before_draw(dctx, record);
   pipe->draw_vbo(pipe, info, drawid_offset, indirect, draws, num_draws);
      }
      static void
   dd_context_draw_vertex_state(struct pipe_context *_pipe,
                                 {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
            record->call.type = CALL_DRAW_VBO;
   memset(&record->call.info.draw_vbo.info, 0,
         record->call.info.draw_vbo.info.mode = info.mode;
   record->call.info.draw_vbo.info.index_size = 4;
   record->call.info.draw_vbo.info.instance_count = 1;
   record->call.info.draw_vbo.drawid_offset = 0;
   record->call.info.draw_vbo.draw = draws[0];
   record->call.info.draw_vbo.info.index.resource = NULL;
   pipe_resource_reference(&record->call.info.draw_vbo.info.index.resource,
         memset(&record->call.info.draw_vbo.indirect, 0,
            dd_before_draw(dctx, record);
   pipe->draw_vertex_state(pipe, state, partial_velem_mask, info, draws, num_draws);
      }
      static void
   dd_context_launch_grid(struct pipe_context *_pipe,
         {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
            record->call.type = CALL_LAUNCH_GRID;
   record->call.info.launch_grid = *info;
   record->call.info.launch_grid.indirect = NULL;
            dd_before_draw(dctx, record);
   pipe->launch_grid(pipe, info);
      }
      static void
   dd_context_resource_copy_region(struct pipe_context *_pipe,
                           {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
            record->call.type = CALL_RESOURCE_COPY_REGION;
   record->call.info.resource_copy_region.dst = NULL;
   pipe_resource_reference(&record->call.info.resource_copy_region.dst, dst);
   record->call.info.resource_copy_region.dst_level = dst_level;
   record->call.info.resource_copy_region.dstx = dstx;
   record->call.info.resource_copy_region.dsty = dsty;
   record->call.info.resource_copy_region.dstz = dstz;
   record->call.info.resource_copy_region.src = NULL;
   pipe_resource_reference(&record->call.info.resource_copy_region.src, src);
   record->call.info.resource_copy_region.src_level = src_level;
            dd_before_draw(dctx, record);
   pipe->resource_copy_region(pipe,
                  }
      static void
   dd_context_blit(struct pipe_context *_pipe, const struct pipe_blit_info *info)
   {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
            record->call.type = CALL_BLIT;
   record->call.info.blit = *info;
   record->call.info.blit.dst.resource = NULL;
   pipe_resource_reference(&record->call.info.blit.dst.resource, info->dst.resource);
   record->call.info.blit.src.resource = NULL;
            dd_before_draw(dctx, record);
   pipe->blit(pipe, info);
      }
      static bool
   dd_context_generate_mipmap(struct pipe_context *_pipe,
                              struct pipe_resource *res,
      {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
   struct dd_draw_record *record = dd_create_record(dctx);
            record->call.type = CALL_GENERATE_MIPMAP;
   record->call.info.generate_mipmap.res = NULL;
   pipe_resource_reference(&record->call.info.generate_mipmap.res, res);
   record->call.info.generate_mipmap.format = format;
   record->call.info.generate_mipmap.base_level = base_level;
   record->call.info.generate_mipmap.last_level = last_level;
   record->call.info.generate_mipmap.first_layer = first_layer;
            dd_before_draw(dctx, record);
   result = pipe->generate_mipmap(pipe, res, format, base_level, last_level,
         dd_after_draw(dctx, record);
      }
      static void
   dd_context_get_query_result_resource(struct pipe_context *_pipe,
                                       {
      struct dd_context *dctx = dd_context(_pipe);
   struct dd_query *dquery = dd_query(query);
   struct pipe_context *pipe = dctx->pipe;
            record->call.type = CALL_GET_QUERY_RESULT_RESOURCE;
   record->call.info.get_query_result_resource.query = query;
   record->call.info.get_query_result_resource.flags = flags;
   record->call.info.get_query_result_resource.result_type = result_type;
   record->call.info.get_query_result_resource.index = index;
   record->call.info.get_query_result_resource.resource = NULL;
   pipe_resource_reference(&record->call.info.get_query_result_resource.resource,
                  /* The query may be deleted by the time we need to print it. */
            dd_before_draw(dctx, record);
   pipe->get_query_result_resource(pipe, dquery->query, flags,
            }
      static void
   dd_context_flush_resource(struct pipe_context *_pipe,
         {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
            record->call.type = CALL_FLUSH_RESOURCE;
   record->call.info.flush_resource = NULL;
            dd_before_draw(dctx, record);
   pipe->flush_resource(pipe, resource);
      }
      static void
   dd_context_clear(struct pipe_context *_pipe, unsigned buffers, const struct pipe_scissor_state *scissor_state,
               {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
            record->call.type = CALL_CLEAR;
   record->call.info.clear.buffers = buffers;
   if (scissor_state)
         record->call.info.clear.color = *color;
   record->call.info.clear.depth = depth;
            dd_before_draw(dctx, record);
   pipe->clear(pipe, buffers, scissor_state, color, depth, stencil);
      }
      static void
   dd_context_clear_render_target(struct pipe_context *_pipe,
                                 {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
                     dd_before_draw(dctx, record);
   pipe->clear_render_target(pipe, dst, color, dstx, dsty, width, height,
            }
      static void
   dd_context_clear_depth_stencil(struct pipe_context *_pipe,
                           {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
                     dd_before_draw(dctx, record);
   pipe->clear_depth_stencil(pipe, dst, clear_flags, depth, stencil,
                  }
      static void
   dd_context_clear_buffer(struct pipe_context *_pipe, struct pipe_resource *res,
               {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
            record->call.type = CALL_CLEAR_BUFFER;
   record->call.info.clear_buffer.res = NULL;
   pipe_resource_reference(&record->call.info.clear_buffer.res, res);
   record->call.info.clear_buffer.offset = offset;
   record->call.info.clear_buffer.size = size;
   record->call.info.clear_buffer.clear_value = clear_value;
            dd_before_draw(dctx, record);
   pipe->clear_buffer(pipe, res, offset, size, clear_value, clear_value_size);
      }
      static void
   dd_context_clear_texture(struct pipe_context *_pipe,
                           {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
                     dd_before_draw(dctx, record);
   pipe->clear_texture(pipe, res, level, box, data);
      }
      /********************************************************************
   * transfer
   */
      static void *
   dd_context_buffer_map(struct pipe_context *_pipe,
                     {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
   struct dd_draw_record *record =
            if (record) {
                  }
   void *ptr = pipe->buffer_map(pipe, resource, level, usage, box, transfer);
   if (record) {
      record->call.info.transfer_map.transfer_ptr = *transfer;
   record->call.info.transfer_map.ptr = ptr;
   if (*transfer) {
      record->call.info.transfer_map.transfer = **transfer;
   record->call.info.transfer_map.transfer.resource = NULL;
   pipe_resource_reference(&record->call.info.transfer_map.transfer.resource,
      } else {
                     }
      }
      static void *
   dd_context_texture_map(struct pipe_context *_pipe,
                     {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
   struct dd_draw_record *record =
            if (record) {
                  }
   void *ptr = pipe->texture_map(pipe, resource, level, usage, box, transfer);
   if (record) {
      record->call.info.transfer_map.transfer_ptr = *transfer;
   record->call.info.transfer_map.ptr = ptr;
   if (*transfer) {
      record->call.info.transfer_map.transfer = **transfer;
   record->call.info.transfer_map.transfer.resource = NULL;
   pipe_resource_reference(&record->call.info.transfer_map.transfer.resource,
      } else {
                     }
      }
      static void
   dd_context_transfer_flush_region(struct pipe_context *_pipe,
               {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
   struct dd_draw_record *record =
            if (record) {
      record->call.type = CALL_TRANSFER_FLUSH_REGION;
   record->call.info.transfer_flush_region.transfer_ptr = transfer;
   record->call.info.transfer_flush_region.box = *box;
   record->call.info.transfer_flush_region.transfer = *transfer;
   record->call.info.transfer_flush_region.transfer.resource = NULL;
   pipe_resource_reference(
                     }
   pipe->transfer_flush_region(pipe, transfer, box);
   if (record)
      }
      static void
   dd_context_buffer_unmap(struct pipe_context *_pipe,
         {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
   struct dd_draw_record *record =
            if (record) {
      record->call.type = CALL_TRANSFER_UNMAP;
   record->call.info.transfer_unmap.transfer_ptr = transfer;
   record->call.info.transfer_unmap.transfer = *transfer;
   record->call.info.transfer_unmap.transfer.resource = NULL;
   pipe_resource_reference(
                     }
   pipe->buffer_unmap(pipe, transfer);
   if (record)
      }
      static void
   dd_context_texture_unmap(struct pipe_context *_pipe,
         {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
   struct dd_draw_record *record =
            if (record) {
      record->call.type = CALL_TRANSFER_UNMAP;
   record->call.info.transfer_unmap.transfer_ptr = transfer;
   record->call.info.transfer_unmap.transfer = *transfer;
   record->call.info.transfer_unmap.transfer.resource = NULL;
   pipe_resource_reference(
                     }
   pipe->texture_unmap(pipe, transfer);
   if (record)
      }
      static void
   dd_context_buffer_subdata(struct pipe_context *_pipe,
                     {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
   struct dd_draw_record *record =
            if (record) {
      record->call.type = CALL_BUFFER_SUBDATA;
   record->call.info.buffer_subdata.resource = NULL;
   pipe_resource_reference(&record->call.info.buffer_subdata.resource, resource);
   record->call.info.buffer_subdata.usage = usage;
   record->call.info.buffer_subdata.offset = offset;
   record->call.info.buffer_subdata.size = size;
               }
   pipe->buffer_subdata(pipe, resource, usage, offset, size, data);
   if (record)
      }
      static void
   dd_context_texture_subdata(struct pipe_context *_pipe,
                                 {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
   struct dd_draw_record *record =
            if (record) {
      record->call.type = CALL_TEXTURE_SUBDATA;
   record->call.info.texture_subdata.resource = NULL;
   pipe_resource_reference(&record->call.info.texture_subdata.resource, resource);
   record->call.info.texture_subdata.level = level;
   record->call.info.texture_subdata.usage = usage;
   record->call.info.texture_subdata.box = *box;
   record->call.info.texture_subdata.data = data;
   record->call.info.texture_subdata.stride = stride;
               }
   pipe->texture_subdata(pipe, resource, level, usage, box, data,
         if (record)
      }
      void
   dd_init_draw_functions(struct dd_context *dctx)
   {
      CTX_INIT(flush);
   CTX_INIT(draw_vbo);
   CTX_INIT(launch_grid);
   CTX_INIT(resource_copy_region);
   CTX_INIT(blit);
   CTX_INIT(clear);
   CTX_INIT(clear_render_target);
   CTX_INIT(clear_depth_stencil);
   CTX_INIT(clear_buffer);
   CTX_INIT(clear_texture);
   CTX_INIT(flush_resource);
   CTX_INIT(generate_mipmap);
   CTX_INIT(get_query_result_resource);
   CTX_INIT(buffer_map);
   CTX_INIT(texture_map);
   CTX_INIT(transfer_flush_region);
   CTX_INIT(buffer_unmap);
   CTX_INIT(texture_unmap);
   CTX_INIT(buffer_subdata);
   CTX_INIT(texture_subdata);
      }
