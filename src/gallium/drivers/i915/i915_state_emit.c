   /**************************************************************************
   *
   * Copyright 2003 VMware, Inc.
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
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "i915_batch.h"
   #include "i915_context.h"
   #include "i915_debug.h"
   #include "i915_fpc.h"
   #include "i915_reg.h"
   #include "i915_resource.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/format/u_formats.h"
      #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      struct i915_tracked_hw_state {
      const char *name;
   void (*validate)(struct i915_context *, unsigned *batch_space);
   void (*emit)(struct i915_context *);
      };
      static void
   validate_flush(struct i915_context *i915, unsigned *batch_space)
   {
         }
      static void
   emit_flush(struct i915_context *i915)
   {
      /* Cache handling is very cheap atm. State handling can request to flushes:
   * - I915_FLUSH_CACHE which is a flush everything request and
   * - I915_PIPELINE_FLUSH which is specifically for the draw_offset flush.
   * Because the cache handling is so dumb, no explicit "invalidate map cache".
   * Also, the first is a strict superset of the latter, so the following logic
   * works. */
   if (i915->flush_dirty & I915_FLUSH_CACHE)
         else if (i915->flush_dirty & I915_PIPELINE_FLUSH)
      }
      uint32_t invariant_state[] = {
      _3DSTATE_AA_CMD | AA_LINE_ECAAR_WIDTH_ENABLE | AA_LINE_ECAAR_WIDTH_1_0 |
                                       _3DSTATE_COORD_SET_BINDINGS | CSB_TCB(0, 0) | CSB_TCB(1, 1) | CSB_TCB(2, 2) |
      CSB_TCB(3, 3) | CSB_TCB(4, 4) | CSB_TCB(5, 5) | CSB_TCB(6, 6) |
         _3DSTATE_RASTER_RULES_CMD | ENABLE_POINT_RASTER_RULE |
      OGL_POINT_RASTER_RULE | ENABLE_LINE_STRIP_PROVOKE_VRTX |
   ENABLE_TRI_FAN_PROVOKE_VRTX | LINE_STRIP_PROVOKE_VRTX(1) |
                  /* disable indirect state for now
   */
         static void
   emit_invariant(struct i915_context *i915)
   {
      i915_winsys_batchbuffer_write(
      i915->batch, invariant_state,
   }
      static void
   validate_immediate(struct i915_context *i915, unsigned *batch_space)
   {
      unsigned dirty = (1 << I915_IMMEDIATE_S0 | 1 << I915_IMMEDIATE_S1 |
                              if (i915->immediate_dirty & (1 << I915_IMMEDIATE_S0) && i915->vbo)
               }
      static void
   emit_immediate_s5(struct i915_context *i915, uint32_t imm)
   {
               if (surf) {
      uint32_t writemask = imm & S5_WRITEDISABLE_MASK;
            /* The register bits are not in order. */
   static const uint32_t writedisables[4] = {
      S5_WRITEDISABLE_RED,
   S5_WRITEDISABLE_GREEN,
   S5_WRITEDISABLE_BLUE,
               for (int i = 0; i < 4; i++) {
      if (writemask & writedisables[surf->color_swizzle[i]])
                     }
      static void
   emit_immediate(struct i915_context *i915)
   {
      /* remove unwanted bits and S7 */
   unsigned dirty = (1 << I915_IMMEDIATE_S0 | 1 << I915_IMMEDIATE_S1 |
                     1 << I915_IMMEDIATE_S2 | 1 << I915_IMMEDIATE_S3 |
   int i, num = util_bitcount(dirty);
                     if (i915->immediate_dirty & (1 << I915_IMMEDIATE_S0)) {
      if (i915->vbo)
      OUT_RELOC(i915->vbo, I915_USAGE_VERTEX,
      else
               for (i = 1; i < I915_MAX_IMMEDIATE; i++) {
      if (dirty & (1 << i)) {
      if (i == I915_IMMEDIATE_S5)
         else
            }
      static void
   validate_dynamic(struct i915_context *i915, unsigned *batch_space)
   {
      *batch_space =
      }
      static void
   emit_dynamic(struct i915_context *i915)
   {
      int i;
   for (i = 0; i < I915_MAX_DYNAMIC; i++) {
      if (i915->dynamic_dirty & (1 << i))
         }
      static void
   validate_static(struct i915_context *i915, unsigned *batch_space)
   {
               if (i915->current.cbuf_bo && (i915->static_dirty & I915_DST_BUF_COLOR)) {
      i915->validation_buffers[i915->num_validation_buffers++] =
                     if (i915->current.depth_bo && (i915->static_dirty & I915_DST_BUF_DEPTH)) {
      i915->validation_buffers[i915->num_validation_buffers++] =
                     if (i915->static_dirty & I915_DST_VARS)
            if (i915->static_dirty & I915_DST_RECT)
      }
      static void
   emit_static(struct i915_context *i915)
   {
      if (i915->current.cbuf_bo && (i915->static_dirty & I915_DST_BUF_COLOR)) {
      OUT_BATCH(_3DSTATE_BUF_INFO_CMD);
   OUT_BATCH(i915->current.cbuf_flags);
               /* What happens if no zbuf??
   */
   if (i915->current.depth_bo && (i915->static_dirty & I915_DST_BUF_DEPTH)) {
      OUT_BATCH(_3DSTATE_BUF_INFO_CMD);
   OUT_BATCH(i915->current.depth_flags);
               if (i915->static_dirty & I915_DST_VARS) {
      OUT_BATCH(_3DSTATE_DST_BUF_VARS_CMD);
         }
      static void
   validate_map(struct i915_context *i915, unsigned *batch_space)
   {
      const uint32_t enabled = i915->current.sampler_enable_flags;
   uint32_t unit;
            *batch_space = i915->current.sampler_enable_nr
                  for (unit = 0; unit < I915_TEX_UNITS; unit++) {
      if (enabled & (1 << unit)) {
      tex = i915_texture(i915->fragment_sampler_views[unit]->texture);
            }
      static void
   emit_map(struct i915_context *i915)
   {
      const uint32_t nr = i915->current.sampler_enable_nr;
   if (nr) {
      const uint32_t enabled = i915->current.sampler_enable_flags;
   uint32_t unit;
   uint32_t count = 0;
   OUT_BATCH(_3DSTATE_MAP_STATE | (3 * nr));
   OUT_BATCH(enabled);
   for (unit = 0; unit < I915_TEX_UNITS; unit++) {
      if (enabled & (1 << unit)) {
      struct i915_texture *texture =
                                          OUT_RELOC(buf, I915_USAGE_SAMPLER, offset);
   OUT_BATCH(i915->current.texbuffer[unit][0]); /* MS3 */
         }
         }
      static void
   validate_sampler(struct i915_context *i915, unsigned *batch_space)
   {
      *batch_space = i915->current.sampler_enable_nr
            }
      static void
   emit_sampler(struct i915_context *i915)
   {
      if (i915->current.sampler_enable_nr) {
                                 for (i = 0; i < I915_TEX_UNITS; i++) {
      if (i915->current.sampler_enable_flags & (1 << i)) {
      OUT_BATCH(i915->current.sampler[i][0]);
   OUT_BATCH(i915->current.sampler[i][1]);
               }
      static void
   validate_constants(struct i915_context *i915, unsigned *batch_space)
   {
                  }
      static void
   emit_constants(struct i915_context *i915)
   {
      /* Collate the user-defined constants with the fragment shader's
   * immediates according to the constant_flags[] array.
   */
            assert(nr <= I915_MAX_CONSTANT);
   if (nr) {
               OUT_BATCH(_3DSTATE_PIXEL_SHADER_CONSTANTS | (nr * 4));
            for (i = 0; i < nr; i++) {
      const uint32_t *c;
   if (i915->fs->constant_flags[i] == I915_CONSTFLAG_USER) {
      /* grab user-defined constant */
   c = (uint32_t *)i915_buffer(i915->constants[PIPE_SHADER_FRAGMENT])
            } else {
      /* emit program constant */
   #if 0 /* debug */
            {
      float *f = (float *) c;
   printf("Const %2d: %f %f %f %f %s\n", i, f[0], f[1], f[2], f[3],
         #endif
            OUT_BATCH(*c++);
   OUT_BATCH(*c++);
   OUT_BATCH(*c++);
            }
      static void
   validate_program(struct i915_context *i915, unsigned *batch_space)
   {
      /* we need more batch space if we want to emulate rgba framebuffers */
      }
      static void
   emit_program(struct i915_context *i915)
   {
      /* we should always have, at least, a pass-through program */
            /* If we're doing a fixup swizzle, that's 3 more dwords to add. */
   uint32_t additional_size = 0;
   if (i915->current.fixup_swizzle)
            /* output the program: 1 dword of header, then 3 dwords per decl/instruction */
            /* first word has the size, adjust it for fixup swizzle */
            for (int i = 1; i < i915->fs->program_len; i++)
            /* we emit an additional mov with swizzle to fake RGBA framebuffers */
   if (i915->current.fixup_swizzle) {
      /* mov out_color, out_color.zyxw */
   OUT_BATCH(A0_MOV | (REG_TYPE_OC << A0_DEST_TYPE_SHIFT) |
               OUT_BATCH(i915->current.fixup_swizzle);
         }
      static void
   emit_draw_rect(struct i915_context *i915)
   {
      if (i915->static_dirty & I915_DST_RECT) {
      OUT_BATCH(_3DSTATE_DRAW_RECT_CMD);
   OUT_BATCH(DRAW_RECT_DIS_DEPTH_OFS);
   OUT_BATCH(i915->current.draw_offset);
   OUT_BATCH(i915->current.draw_size);
         }
      static bool
   i915_validate_state(struct i915_context *i915, unsigned *batch_space)
   {
               i915->num_validation_buffers = 0;
   if (i915->hardware_dirty & I915_HW_INVARIANT)
         else
         #if 0
   static int counter_total = 0;
   #define VALIDATE_ATOM(atom, hw_dirty)                                          \
      if (i915->hardware_dirty & hw_dirty) {                                      \
      static int counter_##atom = 0;                                           \
   validate_##atom(i915, &tmp);                                             \
   *batch_space += tmp;                                                     \
   counter_##atom += tmp;                                                   \
   counter_total += tmp;                                                    \
   printf("%s: \t%d/%d \t%2.2f\n", #atom, counter_##atom, counter_total,    \
         #else
   #define VALIDATE_ATOM(atom, hw_dirty)                                          \
      if (i915->hardware_dirty & hw_dirty) {                                      \
      validate_##atom(i915, &tmp);                                             \
         #endif
      VALIDATE_ATOM(flush, I915_HW_FLUSH);
   VALIDATE_ATOM(immediate, I915_HW_IMMEDIATE);
   VALIDATE_ATOM(dynamic, I915_HW_DYNAMIC);
   VALIDATE_ATOM(static, I915_HW_STATIC);
   VALIDATE_ATOM(map, I915_HW_MAP);
   VALIDATE_ATOM(sampler, I915_HW_SAMPLER);
   VALIDATE_ATOM(constants, I915_HW_CONSTANTS);
      #undef VALIDATE_ATOM
         if (i915->num_validation_buffers == 0)
            if (!i915_winsys_validate_buffers(i915->batch, i915->validation_buffers,
                     }
      /* Push the state into the sarea and/or texture memory.
   */
   void
   i915_emit_hardware_state(struct i915_context *i915)
   {
      unsigned batch_space;
                     if (I915_DBG_ON(DBG_ATOMS))
            if (!i915_validate_state(i915, &batch_space)) {
      FLUSH_BATCH(NULL, I915_FLUSH_ASYNC);
               if (!BEGIN_BATCH(batch_space)) {
      FLUSH_BATCH(NULL, I915_FLUSH_ASYNC);
   assert(i915_validate_state(i915, &batch_space));
                     #define EMIT_ATOM(atom, hw_dirty)                                              \
      if (i915->hardware_dirty & hw_dirty)                                        \
         EMIT_ATOM(flush, I915_HW_FLUSH);
   EMIT_ATOM(invariant, I915_HW_INVARIANT);
   EMIT_ATOM(immediate, I915_HW_IMMEDIATE);
   EMIT_ATOM(dynamic, I915_HW_DYNAMIC);
   EMIT_ATOM(static, I915_HW_STATIC);
   EMIT_ATOM(map, I915_HW_MAP);
   EMIT_ATOM(sampler, I915_HW_SAMPLER);
   EMIT_ATOM(constants, I915_HW_CONSTANTS);
   EMIT_ATOM(program, I915_HW_PROGRAM);
      #undef EMIT_ATOM
         I915_DBG(DBG_EMIT, "%s: used %lu dwords, %d dwords reserved\n", __func__,
                  i915->hardware_dirty = 0;
   i915->immediate_dirty = 0;
   i915->dynamic_dirty = 0;
   i915->static_dirty = 0;
      }
