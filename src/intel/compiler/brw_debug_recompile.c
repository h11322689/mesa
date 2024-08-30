   /*
   * Copyright © 2019 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * @file brw_debug_recompiles.c
   */
      #include <stdio.h>
      #include "brw_compiler.h"
      static bool
   key_debug(const struct brw_compiler *c, void *log,
         {
      if (a != b) {
      brw_shader_perf_log(c, log, "  %s %d->%d\n", name, a, b);
      }
      }
      static bool
   key_debug_float(const struct brw_compiler *c, void *log,
         {
      if (a != b) {
      brw_shader_perf_log(c, log, "  %s %f->%f\n", name, a, b);
      }
      }
      #define check(name, field) \
         #define check_float(name, field) \
            static bool
   debug_sampler_recompile(const struct brw_compiler *c, void *log,
               {
                        for (unsigned i = 0; i < BRW_MAX_SAMPLERS; i++) {
      found |= check("EXT_texture_swizzle or DEPTH_TEXTURE_MODE", swizzles[i]);
               for (unsigned i = 0; i < 3; i++) {
                     }
      static bool
   debug_base_recompile(const struct brw_compiler *c, void *log,
               {
         }
      static void
   debug_vs_recompile(const struct brw_compiler *c, void *log,
               {
               for (unsigned i = 0; i < VERT_ATTRIB_MAX; i++) {
                  found |= check("legacy user clipping", nr_userclip_plane_consts);
   found |= check("copy edgeflag", copy_edgeflag);
   found |= check("pointcoord replace", point_coord_replace);
            if (!found) {
            }
      static void
   debug_tcs_recompile(const struct brw_compiler *c, void *log,
               {
               found |= check("input vertices", input_vertices);
   found |= check("outputs written", outputs_written);
   found |= check("patch outputs written", patch_outputs_written);
   found |= check("tes primitive mode", _tes_primitive_mode);
            if (!found) {
            }
      static void
   debug_tes_recompile(const struct brw_compiler *c, void *log,
               {
               found |= check("inputs read", inputs_read);
            if (!found) {
            }
      static void
   debug_gs_recompile(const struct brw_compiler *c, void *log,
               {
               if (!found) {
            }
      static void
   debug_fs_recompile(const struct brw_compiler *c, void *log,
               {
               found |= check("alphatest, computed depth, depth test, or depth write",
         found |= check("depth statistics", stats_wm);
   found |= check("flat shading", flat_shade);
   found |= check("number of color buffers", nr_color_regions);
   found |= check("MRT alpha test", alpha_test_replicate_alpha);
   found |= check("alpha to coverage", alpha_to_coverage);
   found |= check("fragment color clamping", clamp_fragment_color);
   found |= check("per-sample interpolation", persample_interp);
   found |= check("multisampled FBO", multisample_fbo);
   found |= check("line smoothing", line_aa);
   found |= check("force dual color blending", force_dual_color_blend);
   found |= check("coherent fb fetch", coherent_fb_fetch);
   found |= check("ignore sample mask out", ignore_sample_mask_out);
            found |= check("input slots valid", input_slots_valid);
   found |= check("mrt alpha test function", alpha_test_func);
                     if (!found) {
            }
      static void
   debug_cs_recompile(const struct brw_compiler *c, void *log,
               {
               if (!found) {
            }
      void
   brw_debug_key_recompile(const struct brw_compiler *c, void *log,
                     {
      if (!old_key) {
      brw_shader_perf_log(c, log, "  No previous compile found...\n");
               switch (stage) {
   case MESA_SHADER_VERTEX:
      debug_vs_recompile(c, log, (const struct brw_vs_prog_key *)old_key,
            case MESA_SHADER_TESS_CTRL:
      debug_tcs_recompile(c, log, (const struct brw_tcs_prog_key *)old_key,
            case MESA_SHADER_TESS_EVAL:
      debug_tes_recompile(c, log, (const struct brw_tes_prog_key *)old_key,
            case MESA_SHADER_GEOMETRY:
      debug_gs_recompile(c, log, (const struct brw_gs_prog_key *)old_key,
            case MESA_SHADER_FRAGMENT:
      debug_fs_recompile(c, log, (const struct brw_wm_prog_key *)old_key,
            case MESA_SHADER_COMPUTE:
      debug_cs_recompile(c, log, (const struct brw_cs_prog_key *)old_key,
            default:
            }
