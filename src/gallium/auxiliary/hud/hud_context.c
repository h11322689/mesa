   /**************************************************************************
   *
   * Copyright 2013 Marek Olšák <maraeo@gmail.com>
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /* This head-up display module can draw transparent graphs on top of what
   * the app is rendering, visualizing various data like framerate, cpu load,
   * performance counters, etc. It can be hook up into any gallium frontend.
   *
   * The HUD is controlled with the GALLIUM_HUD environment variable.
   * Set GALLIUM_HUD=help for more info.
   */
      #include <inttypes.h>
   #include <signal.h>
   #include <stdio.h>
      #include "util/detect_os.h"
      #if DETECT_OS_WINDOWS
   #include <io.h>
      /**
   * Access flags W_OK are defined by mingw, but not defined by MSVC, we defined it according to
   * https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/access-waccess
   */
   #ifndef W_OK
   #define W_OK 02
   #endif
   #endif /* DETECT_OS_WINDOWS */
      #include "hud/hud_context.h"
   #include "hud/hud_private.h"
      #include "frontend/api.h"
   #include "cso_cache/cso_context.h"
   #include "util/u_draw_quad.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "util/u_sampler.h"
   #include "util/u_simple_shaders.h"
   #include "util/u_string.h"
   #include "util/u_upload_mgr.h"
   #include "tgsi/tgsi_text.h"
   #include "tgsi/tgsi_dump.h"
      #define HUD_DEFAULT_VISIBILITY true
   #define HUD_DEFAULT_SCALE 1
   #define HUD_DEFAULT_ROTATION 0
   #define HUD_DEFAULT_OPACITY 66
      /* Control the visibility of all HUD contexts */
   static bool huds_visible = HUD_DEFAULT_VISIBILITY;
   static int hud_scale = HUD_DEFAULT_SCALE;
   static int hud_rotate = HUD_DEFAULT_ROTATION;
   static float hud_opacity = HUD_DEFAULT_OPACITY / 100.0f;
      #if DETECT_OS_UNIX
   static void
   signal_visible_handler(int sig, siginfo_t *siginfo, void *context)
   {
         }
   #endif
      static void
   hud_draw_colored_prims(struct hud_context *hud, unsigned prim,
                     {
      struct cso_context *cso = hud->cso;
   struct pipe_context *pipe = hud->pipe;
            hud->constants.color[0] = r;
   hud->constants.color[1] = g;
   hud->constants.color[2] = b;
   hud->constants.color[3] = a;
   hud->constants.translate[0] = (float) (xoffset * hud_scale);
   hud->constants.translate[1] = (float) (yoffset * hud_scale);
   hud->constants.scale[0] = hud_scale;
   hud->constants.scale[1] = yscale * hud_scale;
            u_upload_data(hud->pipe->stream_uploader, 0,
                        cso_set_vertex_buffers(cso, 1, 0, false, &vbuffer);
   pipe_resource_reference(&vbuffer.buffer.resource, NULL);
   cso_set_fragment_shader_handle(hud->cso, hud->fs_color);
      }
      static void
   hud_draw_colored_quad(struct hud_context *hud, unsigned prim,
               {
      float buffer[] = {
      (float) x1, (float) y1,
   (float) x1, (float) y2,
   (float) x2, (float) y2,
                  }
      static void
   hud_draw_background_quad(struct hud_context *hud,
         {
      float *vertices = hud->bg.vertices + hud->bg.num_vertices*2;
                     vertices[num++] = (float) x1;
            vertices[num++] = (float) x1;
            vertices[num++] = (float) x2;
            vertices[num++] = (float) x2;
               }
      static void
   hud_draw_string(struct hud_context *hud, unsigned x, unsigned y,
         {
      char buf[256];
   char *s = buf;
   float *vertices = hud->text.vertices + hud->text.num_vertices*4;
            va_list ap;
   va_start(ap, str);
   vsnprintf(buf, sizeof(buf), str, ap);
            if (!*s)
            hud_draw_background_quad(hud,
                        while (*s) {
      unsigned x1 = x;
   unsigned y1 = y;
   unsigned x2 = x + hud->font.glyph_width;
   unsigned y2 = y + hud->font.glyph_height;
   unsigned tx1 = (*s % 16) * hud->font.glyph_width;
   unsigned ty1 = (*s / 16) * hud->font.glyph_height;
   unsigned tx2 = tx1 + hud->font.glyph_width;
            if (*s == ' ') {
      x += hud->font.glyph_width;
   s++;
                        vertices[num++] = (float) x1;
   vertices[num++] = (float) y1;
   vertices[num++] = (float) tx1;
            vertices[num++] = (float) x1;
   vertices[num++] = (float) y2;
   vertices[num++] = (float) tx1;
            vertices[num++] = (float) x2;
   vertices[num++] = (float) y2;
   vertices[num++] = (float) tx2;
            vertices[num++] = (float) x2;
   vertices[num++] = (float) y1;
   vertices[num++] = (float) tx2;
            x += hud->font.glyph_width;
                  }
      static const char *
   get_float_modifier(double d)
   {
      /* Round to 3 decimal places so as not to print trailing zeros. */
   if (d*1000 != (int)(d*1000))
            /* Show at least 4 digits with at most 3 decimal places, but not zeros. */
   if (d >= 1000 || d == (int)d)
         else if (d >= 100 || d*10 == (int)(d*10))
         else if (d >= 10 || d*100 == (int)(d*100))
         else
      }
      static void
   number_to_human_readable(double num, enum pipe_driver_query_type type,
         {
      static const char *byte_units[] =
         static const char *metric_units[] =
         static const char *time_units[] =
         static const char *hz_units[] =
         static const char *percent_units[] = {"%"};
   static const char *dbm_units[] = {" (-dBm)"};
   static const char *temperature_units[] = {" C"};
   static const char *volt_units[] = {" mV", " V"};
   static const char *amp_units[] = {" mA", " A"};
   static const char *watt_units[] = {" mW", " W"};
            const char **units;
   unsigned max_unit;
   double divisor = (type == PIPE_DRIVER_QUERY_TYPE_BYTES) ? 1024 : 1000;
   unsigned unit = 0;
            switch (type) {
   case PIPE_DRIVER_QUERY_TYPE_MICROSECONDS:
      max_unit = ARRAY_SIZE(time_units)-1;
   units = time_units;
      case PIPE_DRIVER_QUERY_TYPE_VOLTS:
      max_unit = ARRAY_SIZE(volt_units)-1;
   units = volt_units;
      case PIPE_DRIVER_QUERY_TYPE_AMPS:
      max_unit = ARRAY_SIZE(amp_units)-1;
   units = amp_units;
      case PIPE_DRIVER_QUERY_TYPE_DBM:
      max_unit = ARRAY_SIZE(dbm_units)-1;
   units = dbm_units;
      case PIPE_DRIVER_QUERY_TYPE_TEMPERATURE:
      max_unit = ARRAY_SIZE(temperature_units)-1;
   units = temperature_units;
      case PIPE_DRIVER_QUERY_TYPE_FLOAT:
      max_unit = ARRAY_SIZE(float_units)-1;
   units = float_units;
      case PIPE_DRIVER_QUERY_TYPE_PERCENTAGE:
      max_unit = ARRAY_SIZE(percent_units)-1;
   units = percent_units;
      case PIPE_DRIVER_QUERY_TYPE_BYTES:
      max_unit = ARRAY_SIZE(byte_units)-1;
   units = byte_units;
      case PIPE_DRIVER_QUERY_TYPE_HZ:
      max_unit = ARRAY_SIZE(hz_units)-1;
   units = hz_units;
      case PIPE_DRIVER_QUERY_TYPE_WATTS:
      max_unit = ARRAY_SIZE(watt_units)-1;
   units = watt_units;
      default:
      max_unit = ARRAY_SIZE(metric_units)-1;
               while (d > divisor && unit < max_unit) {
      d /= divisor;
      }
   int n = sprintf(out, get_float_modifier(d), d);
   if (n > 0)
      }
      static void
   hud_draw_graph_line_strip(struct hud_context *hud, const struct hud_graph *gr,
         {
      if (gr->num_vertices <= 1)
                     hud_draw_colored_prims(hud, MESA_PRIM_LINE_STRIP,
                              if (gr->num_vertices <= gr->index)
            hud_draw_colored_prims(hud, MESA_PRIM_LINE_STRIP,
                        }
      static void
   hud_pane_accumulate_vertices(struct hud_context *hud,
         {
      struct hud_graph *gr;
   float *line_verts = hud->whitelines.vertices + hud->whitelines.num_vertices*2;
   unsigned i, num = 0;
   char str[32];
            /* draw background */
   hud_draw_background_quad(hud,
                  /* draw numbers on the right-hand side */
   for (i = 0; i <= last_line; i++) {
      unsigned x = pane->x2 + 2;
   unsigned y = pane->inner_y1 +
                  number_to_human_readable(pane->max_value * i / last_line,
                     /* draw info below the pane */
   i = 0;
   LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
      unsigned x = pane->x1 + 2;
            number_to_human_readable(gr->current_value, pane->type, str);
   hud_draw_string(hud, x, y, "  %s: %s", gr->name, str);
               /* draw border */
   assert(hud->whitelines.num_vertices + num/2 + 8 <= hud->whitelines.max_num_vertices);
   line_verts[num++] = (float) pane->x1;
   line_verts[num++] = (float) pane->y1;
   line_verts[num++] = (float) pane->x2;
            line_verts[num++] = (float) pane->x2;
   line_verts[num++] = (float) pane->y1;
   line_verts[num++] = (float) pane->x2;
            line_verts[num++] = (float) pane->x1;
   line_verts[num++] = (float) pane->y2;
   line_verts[num++] = (float) pane->x2;
            line_verts[num++] = (float) pane->x1;
   line_verts[num++] = (float) pane->y1;
   line_verts[num++] = (float) pane->x1;
            /* draw horizontal lines inside the graph */
   for (i = 0; i <= last_line; i++) {
      float y = round((pane->max_value * i / (double)last_line) *
            assert(hud->whitelines.num_vertices + num/2 + 2 <= hud->whitelines.max_num_vertices);
   line_verts[num++] = pane->x1;
   line_verts[num++] = y;
   line_verts[num++] = pane->x2;
                  }
      static void
   hud_pane_accumulate_vertices_simple(struct hud_context *hud,
         {
      struct hud_graph *gr;
   unsigned i;
            /* draw info below the pane */
   i = 0;
   LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
      unsigned x = pane->x1;
            number_to_human_readable(gr->current_value, pane->type, str);
   hud_draw_string(hud, x, y, "%s: %s", gr->name, str);
         }
      static void
   hud_pane_draw_colored_objects(struct hud_context *hud,
         {
      struct hud_graph *gr;
            /* draw colored quads below the pane */
   i = 0;
   LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
      unsigned x = pane->x1 + 2;
            hud_draw_colored_quad(hud, MESA_PRIM_QUADS, x + 1, y + 1, x + 12, y + 13,
                     /* draw the line strips */
   LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
            }
      static void
   hud_prepare_vertices(struct hud_context *hud, struct vertex_queue *v,
         {
      v->num_vertices = 0;
   v->max_num_vertices = num_vertices;
      }
      /**
   * Draw the HUD to the texture \p tex.
   * The texture is usually the back buffer being displayed.
   */
   static void
   hud_draw_results(struct hud_context *hud, struct pipe_resource *tex)
   {
      struct cso_context *cso = hud->cso;
   struct pipe_context *pipe = hud->pipe;
   struct pipe_framebuffer_state fb;
   struct pipe_surface surf_templ, *surf;
   struct pipe_viewport_state viewport;
   const struct pipe_sampler_state *sampler_states[] =
                  if (!huds_visible)
            hud->fb_width = tex->width0;
   hud->fb_height = tex->height0;
   float th = hud_rotate * (M_PI / 180.0f);
   hud->constants.rotate[0] = cos(th);
   hud->constants.rotate[1] = -sin(th);
   hud->constants.rotate[2] = sin(th);
            /* invert the aspect ratio when we rotate the hud */
   if (hud_rotate % 180 == 90) {
      hud->constants.two_div_fb_height = 2.0f / hud->fb_width;
      } else {
      assert(hud_rotate % 180 == 0);
   hud->constants.two_div_fb_width = 2.0f / hud->fb_width;
               cso_save_state(cso, (CSO_BIT_FRAMEBUFFER |
                        CSO_BIT_SAMPLE_MASK |
   CSO_BIT_MIN_SAMPLES |
   CSO_BIT_BLEND |
   CSO_BIT_DEPTH_STENCIL_ALPHA |
   CSO_BIT_FRAGMENT_SHADER |
   CSO_BIT_FRAGMENT_SAMPLERS |
   CSO_BIT_RASTERIZER |
   CSO_BIT_VIEWPORT |
   CSO_BIT_STREAM_OUTPUTS |
   CSO_BIT_GEOMETRY_SHADER |
   CSO_BIT_TESSCTRL_SHADER |
   CSO_BIT_TESSEVAL_SHADER |
         /* set states */
   memset(&surf_templ, 0, sizeof(surf_templ));
            /* Without this, AA lines look thinner if they are between 2 pixels
   * because the alpha is 0.5 on both pixels. (it's ugly)
   *
   * sRGB makes the width of all AA lines look the same.
   */
   if (hud->has_srgb) {
               if (srgb_format != PIPE_FORMAT_NONE)
      }
            memset(&fb, 0, sizeof(fb));
   fb.nr_cbufs = 1;
   fb.cbufs[0] = surf;
   fb.zsbuf = NULL;
   fb.width = hud->fb_width;
   fb.height = hud->fb_height;
            viewport.scale[0] = 0.5f * hud->fb_width;
   viewport.scale[1] = 0.5f * hud->fb_height;
   viewport.scale[2] = 0.0f;
   viewport.translate[0] = 0.5f * hud->fb_width;
   viewport.translate[1] = 0.5f * hud->fb_height;
   viewport.translate[2] = 0.0f;
   viewport.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   viewport.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   viewport.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
            cso_set_framebuffer(cso, &fb);
   cso_set_sample_mask(cso, ~0);
   cso_set_min_samples(cso, 1);
   cso_set_depth_stencil_alpha(cso, &hud->dsa);
   cso_set_rasterizer(cso, &hud->rasterizer);
   cso_set_viewport(cso, &viewport);
   cso_set_stream_outputs(cso, 0, NULL, NULL);
   cso_set_tessctrl_shader_handle(cso, NULL);
   cso_set_tesseval_shader_handle(cso, NULL);
   cso_set_geometry_shader_handle(cso, NULL);
   cso_set_vertex_shader_handle(cso, hud->vs_color);
   cso_set_vertex_elements(cso, &hud->velems);
   cso_set_render_condition(cso, NULL, false, 0);
   pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, 1, 0, false,
         cso_set_samplers(cso, PIPE_SHADER_FRAGMENT, 1, sampler_states);
            /* draw accumulated vertices for background quads */
   cso_set_blend(cso, &hud->alpha_blend);
            if (hud->bg.num_vertices) {
      hud->constants.color[0] = 0;
   hud->constants.color[1] = 0;
   hud->constants.color[2] = 0;
   hud->constants.color[3] = hud_opacity;
   hud->constants.translate[0] = 0;
   hud->constants.translate[1] = 0;
   hud->constants.scale[0] = hud_scale;
                     cso_set_vertex_buffers(cso, 1, 0, false, &hud->bg.vbuf);
      }
            /* draw accumulated vertices for text */
   if (hud->text.num_vertices) {
      cso_set_vertex_shader_handle(cso, hud->vs_text);
   cso_set_vertex_elements(cso, &hud->text_velems);
   cso_set_vertex_buffers(cso, 1, 0, false, &hud->text.vbuf);
   cso_set_fragment_shader_handle(hud->cso, hud->fs_text);
   cso_draw_arrays(cso, MESA_PRIM_QUADS, 0, hud->text.num_vertices);
      }
            if (hud->simple)
            /* draw accumulated vertices for white lines */
            hud->constants.color[0] = 1;
   hud->constants.color[1] = 1;
   hud->constants.color[2] = 1;
   hud->constants.color[3] = 1;
   hud->constants.translate[0] = 0;
   hud->constants.translate[1] = 0;
   hud->constants.scale[0] = hud_scale;
   hud->constants.scale[1] = hud_scale;
            if (hud->whitelines.num_vertices) {
      cso_set_vertex_shader_handle(cso, hud->vs_color);
   cso_set_vertex_buffers(cso, 1, 0, false, &hud->whitelines.vbuf);
   cso_set_fragment_shader_handle(hud->cso, hud->fs_color);
      }
            /* draw the rest */
   cso_set_blend(cso, &hud->alpha_blend);
   cso_set_rasterizer(cso, &hud->rasterizer_aa_lines);
   LIST_FOR_EACH_ENTRY(pane, &hud->pane_list, head) {
      if (pane)
            done:
               /* restore states not restored by cso */
   if (hud->st) {
      hud->st_invalidate_state(hud->st,
                              }
      static void
   hud_start_queries(struct hud_context *hud, struct pipe_context *pipe)
   {
      struct hud_pane *pane;
            /* Start queries. */
            LIST_FOR_EACH_ENTRY(pane, &hud->pane_list, head) {
      LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
      if (gr->begin_query)
            }
      /* Stop queries, query results, and record vertices for charts. */
   static void
   hud_stop_queries(struct hud_context *hud, struct pipe_context *pipe)
   {
      struct hud_pane *pane;
            /* prepare vertex buffers */
   hud_prepare_vertices(hud, &hud->bg, 16 * 256, 2 * sizeof(float));
   hud_prepare_vertices(hud, &hud->whitelines, 4 * 256, 2 * sizeof(float));
            /* Allocate everything once and divide the storage into 3 portions
   * manually, because u_upload_alloc can unmap memory from previous calls.
   */
   u_upload_alloc(pipe->stream_uploader, 0,
                  hud->bg.buffer_size +
   hud->whitelines.buffer_size +
      if (!hud->bg.vertices)
            pipe_resource_reference(&hud->whitelines.vbuf.buffer.resource, hud->bg.vbuf.buffer.resource);
            hud->whitelines.vbuf.buffer_offset = hud->bg.vbuf.buffer_offset +
         hud->whitelines.vertices = hud->bg.vertices +
            hud->text.vbuf.buffer_offset = hud->whitelines.vbuf.buffer_offset +
         hud->text.vertices = hud->whitelines.vertices +
            /* prepare all graphs */
            LIST_FOR_EACH_ENTRY(pane, &hud->pane_list, head) {
      LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
                  if (pane->sort_items) {
      LIST_FOR_EACH_ENTRY_SAFE(gr, next, &pane->graph_list, head) {
      /* ignore the last one */
                  /* This is an incremental bubble sort, because we only do one pass
   * per frame. It will eventually reach an equilibrium.
   */
   if (gr->current_value <
      list_entry(next, struct hud_graph, head)->current_value) {
   list_del(&gr->head);
                     if (hud->simple)
         else
               /* unmap the uploader's vertex buffer before drawing */
      }
      /**
   * Record queries and draw the HUD. The "cso" parameter acts as a filter.
   * If "cso" is not the recording context, recording is skipped.
   * If "cso" is not the drawing context, drawing is skipped.
   * cso == NULL ignores the filter.
   */
   void
   hud_run(struct hud_context *hud, struct cso_context *cso,
         {
               /* If "cso" is the recording or drawing context or NULL, execute
   * the operation. Otherwise, don't do anything.
   */
   if (hud->record_pipe && (!pipe || pipe == hud->record_pipe))
            if (hud->cso && (!cso || cso == hud->cso))
            if (hud->record_pipe && (!pipe || pipe == hud->record_pipe))
      }
      /**
   * Record query results and assemble vertices if "pipe" is a recording but
   * not drawing context.
   */
   void
   hud_record_only(struct hud_context *hud, struct pipe_context *pipe)
   {
               /* If it's a drawing context, only hud_run() records query results. */
   if (pipe == hud->pipe || pipe != hud->record_pipe)
            hud_stop_queries(hud, hud->record_pipe);
      }
      static void
   fixup_bytes(enum pipe_driver_query_type type, int position, uint64_t *exp10)
   {
      if (type == PIPE_DRIVER_QUERY_TYPE_BYTES && position % 3 == 0)
      }
      /**
   * Set the maximum value for the Y axis of the graph.
   * This scales the graph accordingly.
   */
   void
   hud_pane_set_max_value(struct hud_pane *pane, uint64_t value)
   {
      double leftmost_digit;
   uint64_t exp10;
            /* The following code determines the max_value in the graph as well as
   * how many describing lines are drawn. The max_value is rounded up,
   * so that all drawn numbers are rounded for readability.
   * We want to print multiples of a simple number instead of multiples of
   * hard-to-read numbers like 1.753.
            /* Find the left-most digit. Make sure exp10 * 10 and fixup_bytes doesn't
   * overflow. (11 is safe) */
   exp10 = 1;
   for (i = 0; exp10 <= UINT64_MAX / 11 && exp10 * 9 < value; i++) {
      exp10 *= 10;
                        /* Round 9 to 10. */
   if (leftmost_digit == 9) {
      leftmost_digit = 1;
   exp10 *= 10;
               switch ((unsigned)leftmost_digit) {
   case 1:
      pane->last_line = 5; /* lines in +1/5 increments */
      case 2:
      pane->last_line = 8; /* lines in +1/4 increments. */
      case 3:
   case 4:
      pane->last_line = leftmost_digit * 2; /* lines in +1/2 increments */
      case 5:
   case 6:
   case 7:
   case 8:
      pane->last_line = leftmost_digit; /* lines in +1 increments */
      default:
                  /* Truncate {3,4} to {2.5, 3.5} if possible. */
   for (i = 3; i <= 4; i++) {
      if (leftmost_digit == i && value <= (i - 0.5) * exp10) {
      leftmost_digit = i - 0.5;
                  /* Truncate 2 to a multiple of 0.2 in (1, 1.6] if possible. */
   if (leftmost_digit == 2) {
      for (i = 1; i <= 3; i++) {
      if (value <= (1 + i*0.2) * exp10) {
      leftmost_digit = 1 + i*0.2;
   pane->last_line = 5 + i; /* lines in +1/5 increments. */
                     pane->max_value = leftmost_digit * exp10;
      }
      static void
   hud_pane_update_dyn_ceiling(struct hud_graph *gr, struct hud_pane *pane)
   {
      unsigned i;
            if (pane->dyn_ceil_last_ran != gr->index) {
      LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
      for (i = 0; i <  gr->num_vertices; ++i) {
      tmp = gr->vertices[i * 2 + 1] > tmp ?
                  /* Avoid setting it lower than the initial starting height. */
   tmp = tmp > pane->initial_max_value ? tmp : pane->initial_max_value;
               /*
   * Mark this adjustment run so we could avoid repeating a full update
   * again needlessly in case the pane has more than one graph.
   */
      }
      static struct hud_pane *
   hud_pane_create(struct hud_context *hud,
                  unsigned x1, unsigned y1, unsigned x2, unsigned y2,
      {
               if (!pane)
            pane->hud = hud;
   pane->x1 = x1;
   pane->y1 = y1;
   pane->x2 = x2;
   pane->y2 = y2;
   pane->y_simple = y_simple;
   pane->inner_x1 = x1 + 1;
   pane->inner_x2 = x2 - 1;
   pane->inner_y1 = y1 + 1;
   pane->inner_y2 = y2 - 1;
   pane->inner_width = pane->inner_x2 - pane->inner_x1;
   pane->inner_height = pane->inner_y2 - pane->inner_y1;
   pane->period = period;
   pane->max_num_vertices = (x2 - x1 + 2) / 2;
   pane->ceiling = ceiling;
   pane->dyn_ceiling = dyn_ceiling;
   pane->dyn_ceil_last_ran = 0;
   pane->sort_items = sort_items;
   pane->initial_max_value = max_value;
   hud_pane_set_max_value(pane, max_value);
   list_inithead(&pane->graph_list);
      }
      /* replace '-' with a space */
   static void
   strip_hyphens(char *s)
   {
      while (*s) {
      if (*s == '-')
               }
      /**
   * Add a graph to an existing pane.
   * One pane can contain multiple graphs over each other.
   */
   void
   hud_pane_add_graph(struct hud_pane *pane, struct hud_graph *gr)
   {
      static const float colors[][3] = {
      {0, 1, 0},
   {1, 0, 0},
   {0, 1, 1},
   {1, 0, 1},
   {1, 1, 0},
   {0.5, 1, 0.5},
   {1, 0.5, 0.5},
   {0.5, 1, 1},
   {1, 0.5, 1},
   {1, 1, 0.5},
   {0, 0.5, 0},
   {0.5, 0, 0},
   {0, 0.5, 0.5},
   {0.5, 0, 0.5},
      };
                     gr->vertices = MALLOC(pane->max_num_vertices * sizeof(float) * 2);
   gr->color[0] = colors[color][0];
   gr->color[1] = colors[color][1];
   gr->color[2] = colors[color][2];
   gr->pane = pane;
   list_addtail(&gr->head, &pane->graph_list);
   pane->num_graphs++;
      }
      void
   hud_graph_add_value(struct hud_graph *gr, double value)
   {
      gr->current_value = value;
            if (gr->fd) {
      if (gr->fd == stdout) {
         }
   if (fabs(value - lround(value)) > FLT_EPSILON) {
      fprintf(gr->fd, get_float_modifier(value), value);
      }
   else {
                     if (gr->index == gr->pane->max_num_vertices) {
      gr->vertices[0] = 0;
   gr->vertices[1] = gr->vertices[(gr->index-1)*2+1];
      }
   gr->vertices[(gr->index)*2+0] = (float) (gr->index * 2);
   gr->vertices[(gr->index)*2+1] = (float) value;
            if (gr->num_vertices < gr->pane->max_num_vertices) {
                  if (gr->pane->dyn_ceiling == true) {
         }
   if (value > gr->pane->max_value) {
            }
      static void
   hud_graph_destroy(struct hud_graph *graph, struct pipe_context *pipe)
   {
      FREE(graph->vertices);
   if (graph->free_query_data)
         if (graph->fd)
            }
      static void strcat_without_spaces(char *dst, const char *src)
   {
      dst += strlen(dst);
   while (*src) {
      if (*src == ' ')
         else
            }
      }
         #if DETECT_OS_WINDOWS
      #define PATH_SEP "\\"
      #else
      #define PATH_SEP "/"
      #endif
         /**
   * If the GALLIUM_HUD_DUMP_DIR env var is set, we'll write the raw
   * HUD values to files at ${GALLIUM_HUD_DUMP_DIR}/<stat> where <stat>
   * is a HUD variable such as "fps", or "cpu"
   */
   static void
   hud_graph_set_dump_file(struct hud_graph *gr, const char *hud_dump_dir, bool to_stdout)
   {
      if (hud_dump_dir) {
      char *dump_file = malloc(strlen(hud_dump_dir) + sizeof(PATH_SEP)
         if (dump_file) {
      strcpy(dump_file, hud_dump_dir);
   strcat(dump_file, PATH_SEP);
   strcat_without_spaces(dump_file, gr->name);
   gr->fd = fopen(dump_file, "a+");
         } else if (to_stdout) {
                  if (gr->fd) {
      /* flush output after each line is written */
         }
      /**
   * Read a string from the environment variable.
   * The separators "+", ",", ":", and ";" terminate the string.
   * Return the number of read characters.
   */
   static int
   parse_string(const char *s, char *out)
   {
               for (i = 0; *s && *s != '+' && *s != ',' && *s != ':' && *s != ';' && *s != '=';
      s++, out++, i++)
                  if (*s && !i) {
      fprintf(stderr, "gallium_hud: syntax error: unexpected '%c' (%i) while "
                        }
      static char *
   read_pane_settings(char *str, unsigned * const x, unsigned * const y,
                     {
      char *ret = str;
            while (*str == '.') {
      ++str;
   switch (*str) {
   case 'x':
      ++str;
   *x = strtoul(str, &ret, 10);
               case 'y':
      ++str;
   *y = strtoul(str, &ret, 10);
               case 'w':
      ++str;
   tmp = strtoul(str, &ret, 10);
   *width = tmp > 80 ? tmp : 80; /* 80 is chosen arbitrarily */
               /*
   * Prevent setting height to less than 50. If the height is set to less,
   * the text of the Y axis labels on the graph will start overlapping.
   */
   case 'h':
      ++str;
   tmp = strtoul(str, &ret, 10);
   *height = tmp > 50 ? tmp : 50;
               case 'c':
      ++str;
   tmp = strtoul(str, &ret, 10);
   *ceiling = tmp > 10 ? tmp : 10;
               case 'd':
      ++str;
   ret = str;
               case 'r':
      ++str;
   ret = str;
               case 's':
      ++str;
   ret = str;
               default:
      fprintf(stderr, "gallium_hud: syntax error: unexpected '%c'\n", *str);
                        }
      static bool
   has_occlusion_query(struct pipe_screen *screen)
   {
         }
      static bool
   has_streamout(struct pipe_screen *screen)
   {
         }
      static bool
   has_pipeline_stats_query(struct pipe_screen *screen)
   {
         }
      static void
   hud_parse_env_var(struct hud_context *hud, struct pipe_screen *screen,
         {
      unsigned num, i;
   char name_a[256], s[256];
   char *name;
   struct hud_pane *pane = NULL;
   unsigned x = 10, y = 10, y_simple = 10;
   unsigned width = 251, height = 100;
   unsigned period = period_ms * 1000;
   uint64_t ceiling = UINT64_MAX;
   unsigned column_width = 251;
   bool dyn_ceiling = false;
   bool reset_colors = false;
   bool sort_items = false;
   bool to_stdout = false;
            if (strncmp(env, "simple,", 7) == 0) {
      hud->simple = true;
               /*
   * The GALLIUM_HUD_PERIOD env var sets the graph update rate.
   * The env var is in seconds (a float).
   * Zero means update after every frame.
   */
   period_env = getenv("GALLIUM_HUD_PERIOD");
   if (period_env) {
      float p = (float) atof(period_env);
   if (p >= 0.0f) {
                     while ((num = parse_string(env, name_a)) != 0) {
                        /* check for explicit location, size and etc. settings */
   name = read_pane_settings(name_a, &x, &y, &width, &height, &ceiling,
         /*
      * Keep track of overall column width to avoid pane overlapping in case
   * later we create a new column while the bottom pane in the current
   * column is less wide than the rest of the panes in it.
                  if (!pane) {
      pane = hud_pane_create(hud, x, y, x + width, y + height, y_simple,
         if (!pane)
               if (reset_colors) {
      pane->next_color = 0;
               #if defined(HAVE_GALLIUM_EXTRA_HUD) || defined(HAVE_LIBSENSORS)
         #endif
         /* IF YOU CHANGE THIS, UPDATE print_help! */
   if (strcmp(name, "fps") == 0) {
         }
   else if (strcmp(name, "frametime") == 0) {
         }
   else if (strcmp(name, "cpu") == 0) {
         }
   else if (sscanf(name, "cpu%u%s", &i, s) == 1) {
         }
   else if (strcmp(name, "API-thread-busy") == 0) {
         }
   else if (strcmp(name, "API-thread-offloaded-slots") == 0) {
         }
   else if (strcmp(name, "API-thread-direct-slots") == 0) {
         }
   else if (strcmp(name, "API-thread-num-syncs") == 0) {
         }
   else if (strcmp(name, "API-thread-num-batches") == 0) {
         }
   else if (strcmp(name, "main-thread-busy") == 0) {
         #ifdef HAVE_GALLIUM_EXTRA_HUD
         else if (sscanf(name, "nic-rx-%s", arg_name) == 1) {
         }
   else if (sscanf(name, "nic-tx-%s", arg_name) == 1) {
         }
   else if (sscanf(name, "nic-rssi-%s", arg_name) == 1) {
      hud_nic_graph_install(pane, arg_name, NIC_RSSI_DBM);
      }
   else if (sscanf(name, "diskstat-rd-%s", arg_name) == 1) {
      hud_diskstat_graph_install(pane, arg_name, DISKSTAT_RD);
      }
   else if (sscanf(name, "diskstat-wr-%s", arg_name) == 1) {
      hud_diskstat_graph_install(pane, arg_name, DISKSTAT_WR);
      }
   else if (sscanf(name, "cpufreq-min-cpu%u", &i) == 1) {
      hud_cpufreq_graph_install(pane, i, CPUFREQ_MINIMUM);
      }
   else if (sscanf(name, "cpufreq-cur-cpu%u", &i) == 1) {
      hud_cpufreq_graph_install(pane, i, CPUFREQ_CURRENT);
      }
   else if (sscanf(name, "cpufreq-max-cpu%u", &i) == 1) {
      hud_cpufreq_graph_install(pane, i, CPUFREQ_MAXIMUM);
      #endif
   #ifdef HAVE_LIBSENSORS
         else if (sscanf(name, "sensors_temp_cu-%s", arg_name) == 1) {
      hud_sensors_temp_graph_install(pane, arg_name,
            }
   else if (sscanf(name, "sensors_temp_cr-%s", arg_name) == 1) {
      hud_sensors_temp_graph_install(pane, arg_name,
            }
   else if (sscanf(name, "sensors_volt_cu-%s", arg_name) == 1) {
      hud_sensors_temp_graph_install(pane, arg_name,
            }
   else if (sscanf(name, "sensors_curr_cu-%s", arg_name) == 1) {
      hud_sensors_temp_graph_install(pane, arg_name,
            }
   else if (sscanf(name, "sensors_pow_cu-%s", arg_name) == 1) {
      hud_sensors_temp_graph_install(pane, arg_name,
            #endif
         else if (strcmp(name, "samples-passed") == 0 &&
            hud_pipe_query_install(&hud->batch_query, pane,
                        "samples-passed",
   }
   else if (strcmp(name, "primitives-generated") == 0 &&
            hud_pipe_query_install(&hud->batch_query, pane,
                        "primitives-generated",
   }
   else if (strcmp(name, "stdout") == 0) {
         }
   else {
               /* pipeline statistics queries */
   if (has_pipeline_stats_query(screen)) {
      static const char *pipeline_statistics_names[] =
   {
      "ia-vertices",
   "ia-primitives",
   "vs-invocations",
   "gs-invocations",
   "gs-primitives",
   "clipper-invocations",
   "clipper-primitives-generated",
   "ps-invocations",
   "hs-invocations",
   "ds-invocations",
      };
   for (i = 0; i < ARRAY_SIZE(pipeline_statistics_names); ++i)
      if (strcmp(name, pipeline_statistics_names[i]) == 0)
      if (i < ARRAY_SIZE(pipeline_statistics_names)) {
      hud_pipe_query_install(&hud->batch_query, pane, name,
                                          /* driver queries */
   if (!processed) {
      if (!hud_driver_query_install(&hud->batch_query, pane,
            fprintf(stderr, "gallium_hud: unknown driver query '%s'\n", name);
   fflush(stderr);
                     if (*env == ':') {
               if (!pane) {
      fprintf(stderr, "gallium_hud: syntax error: unexpected ':', "
         fflush(stderr);
                              if (num && sscanf(s, "%u", &i) == 1) {
      hud_pane_set_max_value(pane, i);
      }
   else {
      fprintf(stderr, "gallium_hud: syntax error: unexpected '%c' (%i) "
                        if (*env == '=') {
               if (!pane) {
      fprintf(stderr, "gallium_hud: syntax error: unexpected '=', "
         fflush(stderr);
                              strip_hyphens(s);
   if (added && !list_is_empty(&pane->graph_list)) {
      struct hud_graph *graph;
   graph = list_entry(pane->graph_list.prev, struct hud_graph, head);
                  if (*env == 0)
            /* parse a separator */
   switch (*env) {
   case '+':
                  case ',':
      env++;
                  y += height + hud->font.glyph_height * (pane->num_graphs + 2);
                  if (pane && pane->num_graphs) {
      list_addtail(&pane->head, &hud->pane_list);
                  case ';':
      env++;
   y = 10;
   y_simple = 10;
                  if (pane && pane->num_graphs) {
      list_addtail(&pane->head, &hud->pane_list);
               /* Starting a new column; reset column width. */
               default:
      fprintf(stderr, "gallium_hud: syntax error: unexpected '%c'\n", *env);
               /* Reset to defaults for the next pane in case these were modified. */
   width = 251;
   ceiling = UINT64_MAX;
   dyn_ceiling = false;
                  if (pane) {
      if (pane->num_graphs) {
         }
   else {
                     const char *hud_dump_dir = getenv("GALLIUM_HUD_DUMP_DIR");
   if ((hud_dump_dir && access(hud_dump_dir, W_OK) == 0) || to_stdout) {
      LIST_FOR_EACH_ENTRY(pane, &hud->pane_list, head) {
               LIST_FOR_EACH_ENTRY(gr, &pane->graph_list, head) {
                  }
      static void
   print_help(struct pipe_screen *screen)
   {
               puts("Syntax: GALLIUM_HUD=name1[+name2][...][:value1][,nameI...][;nameJ...]");
   puts("");
   puts("  Names are identifiers of data sources which will be drawn as graphs");
   puts("  in panes. Multiple graphs can be drawn in the same pane.");
   puts("  There can be multiple panes placed in rows and columns.");
   puts("");
   puts("  '+' separates names which will share a pane.");
   puts("  ':[value]' specifies the initial maximum value of the Y axis");
   puts("             for the given pane.");
   puts("  ',' creates a new pane below the last one.");
   puts("  ';' creates a new pane at the top of the next column.");
   puts("  '=' followed by a string, changes the name of the last data source");
   puts("      to that string");
   puts("");
   puts("  Example: GALLIUM_HUD=\"cpu,fps;primitives-generated\"");
   puts("");
   puts("  Additionally, by prepending '.[identifier][value]' modifiers to");
   puts("  a name, it is possible to explicitly set the location and size");
   puts("  of a pane, along with limiting overall maximum value of the");
   puts("  Y axis and activating dynamic readjustment of the Y axis.");
   puts("  Several modifiers may be applied to the same pane simultaneously.");
   puts("");
   puts("  'x[value]' sets the location of the pane on the x axis relative");
   puts("             to the upper-left corner of the viewport, in pixels.");
   puts("  'y[value]' sets the location of the pane on the y axis relative");
   puts("             to the upper-left corner of the viewport, in pixels.");
   puts("  'w[value]' sets width of the graph pixels.");
   puts("  'h[value]' sets height of the graph in pixels.");
   puts("  'c[value]' sets the ceiling of the value of the Y axis.");
   puts("             If the graph needs to draw values higher than");
   puts("             the ceiling allows, the value is clamped.");
   puts("  'd' activates dynamic Y axis readjustment to set the value of");
   puts("      the Y axis to match the highest value still visible in the graph.");
   puts("  'r' resets the color counter (the next color will be green)");
   puts("  's' sort items below graphs in descending order");
   puts("");
   puts("  If 'c' and 'd' modifiers are used simultaneously, both are in effect:");
   puts("  the Y axis does not go above the restriction imposed by 'c' while");
   puts("  still adjusting the value of the Y axis down when appropriate.");
   puts("");
   puts("  You can change behavior of the whole HUD by adding these options at");
   puts("  the beginning of the environment variable:");
   puts("  'simple,' disables all the fancy stuff and only draws text.");
   puts("");
   puts("  Example: GALLIUM_HUD=\".w256.h64.x1600.y520.d.c1000fps+cpu,.datom-count\"");
   puts("");
   puts("  Available names:");
   puts("    stdout (prints the counters value to stdout)");
   puts("    fps");
   puts("    frametime");
            for (i = 0; i < num_cpus; i++)
            if (has_occlusion_query(screen))
         if (has_streamout(screen))
            if (has_pipeline_stats_query(screen)) {
      puts("    ia-vertices");
   puts("    ia-primitives");
   puts("    vs-invocations");
   puts("    gs-invocations");
   puts("    gs-primitives");
   puts("    clipper-invocations");
   puts("    clipper-primitives-generated");
   puts("    ps-invocations");
   puts("    hs-invocations");
   puts("    ds-invocations");
            #ifdef HAVE_GALLIUM_EXTRA_HUD
      hud_get_num_disks(1);
   hud_get_num_nics(1);
      #endif
   #ifdef HAVE_LIBSENSORS
         #endif
         if (screen->get_driver_query_info){
      bool skipping = false;
   struct pipe_driver_query_info info;
            for (i = 0; i < num_queries; i++){
      screen->get_driver_query_info(screen, i, &info);
   if (info.flags & PIPE_DRIVER_QUERY_FLAG_DONT_LIST) {
      if (!skipping)
            } else {
      printf("    %s\n", info.name);
                     puts("");
      }
      static void
   hud_unset_draw_context(struct hud_context *hud)
   {
               if (!pipe)
                     if (hud->fs_color) {
      pipe->delete_fs_state(pipe, hud->fs_color);
      }
   if (hud->fs_text) {
      pipe->delete_fs_state(pipe, hud->fs_text);
      }
   if (hud->vs_color) {
      pipe->delete_vs_state(pipe, hud->vs_color);
      }
   if (hud->vs_text) {
      pipe->delete_vs_state(pipe, hud->vs_text);
               hud->cso = NULL;
      }
      static bool
   hud_set_draw_context(struct hud_context *hud, struct cso_context *cso,
               {
               assert(!hud->pipe);
   hud->pipe = pipe;
   hud->cso = cso;
   hud->st = st;
            struct pipe_sampler_view view_templ;
   u_sampler_view_default_template(
         hud->font_sampler_view = pipe->create_sampler_view(pipe, hud->font.texture,
         if (!hud->font_sampler_view)
            /* color fragment shader */
   hud->fs_color =
         util_make_fragment_passthrough_shader(pipe,
                  /* text fragment shader */
   {
      /* Read a texture and do .xxxx swizzling. */
   static const char *fragment_shader_text = {
      "FRAG\n"
   "DCL IN[0], GENERIC[0], LINEAR\n"
   "DCL SAMP[0]\n"
   "DCL SVIEW[0], 2D, FLOAT\n"
                  "TEX TEMP[0], IN[0], SAMP[0], 2D\n"
   "MOV OUT[0], TEMP[0].xxxx\n"
               struct tgsi_token tokens[1000];
            if (!tgsi_text_translate(fragment_shader_text, tokens, ARRAY_SIZE(tokens))) {
      assert(0);
      }
   pipe_shader_state_from_tgsi(&state, tokens);
               /* color vertex shader */
   {
      static const char *vertex_shader_text = {
      "VERT\n"
   "DCL IN[0..1]\n"
   "DCL OUT[0], POSITION\n"
   "DCL OUT[1], COLOR[0]\n" /* color */
   "DCL OUT[2], GENERIC[0]\n" /* texcoord */
   /* [0] = color,
   * [1] = (2/fb_width, 2/fb_height, xoffset, yoffset)
   * [2] = (xscale, yscale, 0, 0)
   * [3] = rotation_matrix */
   "DCL CONST[0][0..3]\n"
                  /* v = in * (xscale, yscale) + (xoffset, yoffset) */
   "MAD TEMP[0].xy, IN[0], CONST[0][2].xyyy, CONST[0][1].zwww\n"
                  /* pos = rotation_matrix * v */
   "MUL TEMP[2].xyzw, TEMP[1].xyxy, CONST[0][3].xyzw\n"
                  "MOV OUT[1], CONST[0][0]\n"
   "MOV OUT[2], IN[1]\n"
               struct tgsi_token tokens[1000];
   struct pipe_shader_state state = {0};
   if (!tgsi_text_translate(vertex_shader_text, tokens, ARRAY_SIZE(tokens))) {
      assert(0);
      }
   pipe_shader_state_from_tgsi(&state, tokens);
               /* text vertex shader */
   {
      /* similar to the above, without the color component
   * to match the varyings in fs_text */
   static const char *vertex_shader_text = {
      "VERT\n"
   "DCL IN[0..1]\n"
   "DCL OUT[0], POSITION\n"
   "DCL OUT[1], GENERIC[0]\n" /* texcoord */
   /* [0] = color,
   * [1] = (2/fb_width, 2/fb_height, xoffset, yoffset)
   * [2] = (xscale, yscale, 0, 0)
   * [3] = rotation_matrix */
   "DCL CONST[0][0..3]\n"
   "DCL TEMP[0..2]\n"
                  /* v = in * (xscale, yscale) + (xoffset, yoffset) */
   "MAD TEMP[0].xy, IN[0], CONST[0][2].xyyy, CONST[0][1].zwww\n"
                  /* pos = rotation_matrix * v */
   "MUL TEMP[2].xyzw, TEMP[1].xyxy, CONST[0][3].xyzw\n"
                  "MUL OUT[1], IN[1], IMM[1]\n"
               struct tgsi_token tokens[1000];
   struct pipe_shader_state state = {0};
   if (!tgsi_text_translate(vertex_shader_text, tokens, ARRAY_SIZE(tokens))) {
      assert(0);
      }
   pipe_shader_state_from_tgsi(&state, tokens);
                     fail:
      hud_unset_draw_context(hud);
   fprintf(stderr, "hud: failed to set a draw context");
      }
      static void
   hud_unset_record_context(struct hud_context *hud)
   {
      struct pipe_context *pipe = hud->record_pipe;
   struct hud_pane *pane, *pane_tmp;
            if (!pipe)
            LIST_FOR_EACH_ENTRY_SAFE(pane, pane_tmp, &hud->pane_list, head) {
      LIST_FOR_EACH_ENTRY_SAFE(graph, graph_tmp, &pane->graph_list, head) {
      list_del(&graph->head);
      }
   list_del(&pane->head);
               hud_batch_query_cleanup(&hud->batch_query, pipe);
      }
      static void
   hud_set_record_context(struct hud_context *hud, struct pipe_context *pipe)
   {
         }
      static void
   hud_init_velems(struct cso_velems_state *velems, unsigned stride)
   {
      velems->count = 2;
   for (unsigned i = 0; i < 2; i++) {
      velems->velems[i].src_offset = i * 2 * sizeof(float);
   velems->velems[i].src_format = PIPE_FORMAT_R32G32_FLOAT;
   velems->velems[i].vertex_buffer_index = 0;
         }
      /**
   * Create the HUD.
   *
   * If "share" is non-NULL and GALLIUM_HUD_SHARE=x,y is set, increment the
   * reference counter of "share", set "cso" as the recording or drawing context
   * according to the environment variable, and return "share".
   * This allows sharing the HUD instance within a multi-context share group,
   * record queries in one context and draw them in another.
   */
   struct hud_context *
   hud_create(struct cso_context *cso, struct hud_context *share,
               {
      const char *share_env = debug_get_option("GALLIUM_HUD_SHARE", NULL);
            if (share_env && sscanf(share_env, "%u,%u", &record_ctx, &draw_ctx) != 2)
            if (share && share_env) {
      /* All contexts in a share group share the HUD instance.
   * Only one context can record queries and only one context
   * can draw the HUD.
   *
   * GALLIUM_HUD_SHARE=x,y determines the context indices.
   */
            if (context_id == record_ctx) {
      assert(!share->record_pipe);
               if (context_id == draw_ctx) {
      assert(!share->pipe);
                           struct pipe_screen *screen = cso_get_pipe_context(cso)->screen;
   struct hud_context *hud;
   unsigned i;
   unsigned default_period_ms = 500;/* default period (1/2 second) */
   const char *show_fps = getenv("LIBGL_SHOW_FPS");
   bool emulate_libgl_show_fps = false;
   if (show_fps) {
      default_period_ms = atoi(show_fps) * 1000;
   if (default_period_ms)
         else
      }
   const char *env = debug_get_option("GALLIUM_HUD",
      #if DETECT_OS_UNIX
      unsigned signo = debug_get_num_option("GALLIUM_HUD_TOGGLE_SIGNAL", 0);
   static bool sig_handled = false;
               #endif
      huds_visible = debug_get_bool_option("GALLIUM_HUD_VISIBLE", !emulate_libgl_show_fps);
   hud_opacity = debug_get_num_option("GALLIUM_HUD_OPACITY", HUD_DEFAULT_OPACITY) / 100.0f;
   hud_scale = debug_get_num_option("GALLIUM_HUD_SCALE", HUD_DEFAULT_SCALE);
   hud_rotate = debug_get_num_option("GALLIUM_HUD_ROTATION", HUD_DEFAULT_ROTATION) % 360;
   if (hud_rotate < 0) {
         }
   if (hud_rotate % 90 != 0) {
      fprintf(stderr, "gallium_hud: rotation must be a multiple of 90. Falling back to 0.\n");
               if (!env || !*env)
            if (strcmp(env, "help") == 0) {
      print_help(screen);
               hud = CALLOC_STRUCT(hud_context);
   if (!hud)
            /* font (the context is only used for the texture upload) */
   if (!util_font_create(cso_get_pipe_context(cso),
            FREE(hud);
                        static const enum pipe_format srgb_formats[] = {
      PIPE_FORMAT_B8G8R8A8_SRGB,
      };
   for (i = 0; i < ARRAY_SIZE(srgb_formats); i++) {
      if (!screen->is_format_supported(screen, srgb_formats[i],
                                    /* blend state */
            hud->alpha_blend.rt[0].colormask = PIPE_MASK_RGBA;
   hud->alpha_blend.rt[0].blend_enable = 1;
   hud->alpha_blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   hud->alpha_blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
   hud->alpha_blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
   hud->alpha_blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   hud->alpha_blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ZERO;
            /* rasterizer */
   hud->rasterizer.half_pixel_center = 1;
   hud->rasterizer.bottom_edge_rule = 1;
   hud->rasterizer.depth_clip_near = 1;
   hud->rasterizer.depth_clip_far = 1;
   hud->rasterizer.line_width = 1;
            hud->rasterizer_aa_lines = hud->rasterizer;
            /* vertex elements */
   hud_init_velems(&hud->velems, 2 * sizeof(float));
            /* sampler state (for font drawing) */
   hud->font_sampler_state.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   hud->font_sampler_state.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
            /* constants */
   hud->constbuf.buffer_size = sizeof(hud->constants);
                        #if DETECT_OS_UNIX
      if (!sig_handled && signo != 0) {
      action.sa_sigaction = &signal_visible_handler;
            if (signo >= NSIG)
         else if (sigaction(signo, &action, NULL) < 0)
                        #endif
         if (record_ctx == 0)
         if (draw_ctx == 0)
            hud_parse_env_var(hud, screen, env, default_period_ms);
      }
      /**
   * Destroy a HUD. If the HUD has several users, decrease the reference counter
   * and detach the context from the HUD.
   */
   void
   hud_destroy(struct hud_context *hud, struct cso_context *cso)
   {
      if (!cso || hud->record_pipe == cso_get_pipe_context(cso))
            if (!cso || hud->cso == cso)
            if (p_atomic_dec_zero(&hud->refcount)) {
      pipe_resource_reference(&hud->font.texture, NULL);
         }
      void
   hud_add_queue_for_monitoring(struct hud_context *hud,
         {
      assert(!hud->monitored_queue);
      }
