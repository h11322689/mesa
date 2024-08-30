   /*
   * Copyright 2018 Collabora Ltd.
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
   */
      #include "zink_state.h"
      #include "zink_context.h"
   #include "zink_format.h"
   #include "zink_program.h"
   #include "zink_screen.h"
      #include "compiler/shader_enums.h"
   #include "util/u_dual_blend.h"
   #include "util/u_memory.h"
   #include "util/u_helpers.h"
   #include "vulkan/util/vk_format.h"
      #include <math.h>
      static void *
   zink_create_vertex_elements_state(struct pipe_context *pctx,
               {
      struct zink_screen *screen = zink_screen(pctx->screen);
   unsigned int i;
   struct zink_vertex_elements_state *ves = CALLOC_STRUCT(zink_vertex_elements_state);
   if (!ves)
                  int buffer_map[PIPE_MAX_ATTRIBS];
   for (int i = 0; i < ARRAY_SIZE(buffer_map); ++i)
            int num_bindings = 0;
   unsigned num_decomposed = 0;
   uint32_t size8 = 0;
   uint32_t size16 = 0;
   uint32_t size32 = 0;
   uint16_t strides[PIPE_MAX_ATTRIBS];
   for (i = 0; i < num_elements; ++i) {
               int binding = elem->vertex_buffer_index;
   if (buffer_map[binding] < 0) {
      ves->hw_state.binding_map[num_bindings] = binding;
      }
            ves->bindings[binding].binding = binding;
            assert(!elem->instance_divisor || zink_screen(pctx->screen)->info.have_EXT_vertex_attribute_divisor);
   if (elem->instance_divisor > screen->info.vdiv_props.maxVertexAttribDivisor)
                  VkFormat format;
   if (screen->format_props[elem->src_format].bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
         else {
      enum pipe_format new_format = zink_decompose_vertex_format(elem->src_format);
   assert(new_format);
   num_decomposed++;
   assert(screen->format_props[new_format].bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT);
   if (util_format_get_blocksize(new_format) == 4)
         else if (util_format_get_blocksize(new_format) == 2)
         else
         format = zink_get_format(screen, new_format);
   unsigned size;
   if (i < 8)
         else if (i < 16)
         else
         if (util_format_get_nr_components(elem->src_format) == 4) {
      ves->decomposed_attrs |= BITFIELD_BIT(i);
      } else {
      ves->decomposed_attrs_without_w |= BITFIELD_BIT(i);
      }
               if (screen->info.have_EXT_vertex_input_dynamic_state) {
      ves->hw_state.dynattribs[i].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
   ves->hw_state.dynattribs[i].binding = binding;
   ves->hw_state.dynattribs[i].location = i;
   ves->hw_state.dynattribs[i].format = format;
   strides[binding] = elem->src_stride;
   assert(ves->hw_state.dynattribs[i].format != VK_FORMAT_UNDEFINED);
      } else {
      ves->hw_state.attribs[i].binding = binding;
   ves->hw_state.attribs[i].location = i;
   ves->hw_state.attribs[i].format = format;
   ves->hw_state.b.strides[binding] = elem->src_stride;
   assert(ves->hw_state.attribs[i].format != VK_FORMAT_UNDEFINED);
   ves->hw_state.attribs[i].offset = elem->src_offset;
         }
   assert(num_decomposed + num_elements <= PIPE_MAX_ATTRIBS);
   u_foreach_bit(i, ves->decomposed_attrs | ves->decomposed_attrs_without_w) {
      const struct pipe_vertex_element *elem = elements + i;
   const struct util_format_description *desc = util_format_description(elem->src_format);
   unsigned size = 1;
   if (size32 & BITFIELD_BIT(i))
         else if (size16 & BITFIELD_BIT(i))
         else
         for (unsigned j = 1; j < desc->nr_channels; j++) {
      if (screen->info.have_EXT_vertex_input_dynamic_state) {
      memcpy(&ves->hw_state.dynattribs[num_elements], &ves->hw_state.dynattribs[i], sizeof(VkVertexInputAttributeDescription2EXT));
   ves->hw_state.dynattribs[num_elements].location = num_elements;
      } else {
      memcpy(&ves->hw_state.attribs[num_elements], &ves->hw_state.attribs[i], sizeof(VkVertexInputAttributeDescription));
   ves->hw_state.attribs[num_elements].location = num_elements;
      }
         }
   ves->hw_state.num_bindings = num_bindings;
   ves->hw_state.num_attribs = num_elements;
   if (screen->info.have_EXT_vertex_input_dynamic_state) {
      for (int i = 0; i < num_bindings; ++i) {
      ves->hw_state.dynbindings[i].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
   ves->hw_state.dynbindings[i].binding = ves->bindings[i].binding;
   ves->hw_state.dynbindings[i].inputRate = ves->bindings[i].inputRate;
   ves->hw_state.dynbindings[i].stride = strides[i];
   if (ves->divisor[i])
         else
         } else {
      for (int i = 0; i < num_bindings; ++i) {
      ves->hw_state.b.bindings[i].binding = ves->bindings[i].binding;
   ves->hw_state.b.bindings[i].inputRate = ves->bindings[i].inputRate;
   if (ves->divisor[i]) {
      ves->hw_state.b.divisors[ves->hw_state.b.divisors_present].divisor = ves->divisor[i];
   ves->hw_state.b.divisors[ves->hw_state.b.divisors_present].binding = ves->bindings[i].binding;
            }
      }
      static void
   zink_bind_vertex_elements_state(struct pipe_context *pctx,
         {
      struct zink_context *ctx = zink_context(pctx);
   struct zink_gfx_pipeline_state *state = &ctx->gfx_pipeline_state;
   zink_flush_dgc_if_enabled(ctx);
   ctx->element_state = cso;
   if (cso) {
      if (state->element_state != &ctx->element_state->hw_state) {
      ctx->vertex_state_changed = !zink_screen(pctx->screen)->info.have_EXT_vertex_input_dynamic_state;
      }
   state->element_state = &ctx->element_state->hw_state;
   if (zink_screen(pctx->screen)->optimal_keys)
         const struct zink_vs_key *vs = zink_get_vs_key(ctx);
   uint32_t decomposed_attrs = 0, decomposed_attrs_without_w = 0;
   switch (vs->size) {
   case 1:
      decomposed_attrs = vs->u8.decomposed_attrs;
   decomposed_attrs_without_w = vs->u8.decomposed_attrs_without_w;
      case 2:
      decomposed_attrs = vs->u16.decomposed_attrs;
   decomposed_attrs_without_w = vs->u16.decomposed_attrs_without_w;
      case 4:
      decomposed_attrs = vs->u16.decomposed_attrs;
   decomposed_attrs_without_w = vs->u16.decomposed_attrs_without_w;
      }
   if (ctx->element_state->decomposed_attrs != decomposed_attrs ||
      ctx->element_state->decomposed_attrs_without_w != decomposed_attrs_without_w) {
   unsigned size = MAX2(ctx->element_state->decomposed_attrs_size, ctx->element_state->decomposed_attrs_without_w_size);
   struct zink_shader_key *key = (struct zink_shader_key *)zink_set_vs_key(ctx);
   key->size -= 2 * key->key.vs.size;
   switch (size) {
   case 1:
      key->key.vs.u8.decomposed_attrs = ctx->element_state->decomposed_attrs;
   key->key.vs.u8.decomposed_attrs_without_w = ctx->element_state->decomposed_attrs_without_w;
      case 2:
      key->key.vs.u16.decomposed_attrs = ctx->element_state->decomposed_attrs;
   key->key.vs.u16.decomposed_attrs_without_w = ctx->element_state->decomposed_attrs_without_w;
      case 4:
      key->key.vs.u32.decomposed_attrs = ctx->element_state->decomposed_attrs;
   key->key.vs.u32.decomposed_attrs_without_w = ctx->element_state->decomposed_attrs_without_w;
      default: break;
   }
   key->key.vs.size = size;
         } else {
   state->element_state = NULL;
   ctx->vertex_buffers_dirty = false;
      }
      static void
   zink_delete_vertex_elements_state(struct pipe_context *pctx,
         {
         }
      static VkBlendFactor
   blend_factor(enum pipe_blendfactor factor)
   {
      switch (factor) {
   case PIPE_BLENDFACTOR_ONE: return VK_BLEND_FACTOR_ONE;
   case PIPE_BLENDFACTOR_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
   case PIPE_BLENDFACTOR_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
   case PIPE_BLENDFACTOR_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
   case PIPE_BLENDFACTOR_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
         case PIPE_BLENDFACTOR_CONST_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
   case PIPE_BLENDFACTOR_CONST_ALPHA: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
   case PIPE_BLENDFACTOR_SRC1_COLOR: return VK_BLEND_FACTOR_SRC1_COLOR;
                     case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_COLOR:
            case PIPE_BLENDFACTOR_INV_CONST_COLOR:
         case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
         case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
         }
      }
         static VkBlendOp
   blend_op(enum pipe_blend_func func)
   {
      switch (func) {
   case PIPE_BLEND_ADD: return VK_BLEND_OP_ADD;
   case PIPE_BLEND_SUBTRACT: return VK_BLEND_OP_SUBTRACT;
   case PIPE_BLEND_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
   case PIPE_BLEND_MIN: return VK_BLEND_OP_MIN;
   case PIPE_BLEND_MAX: return VK_BLEND_OP_MAX;
   }
      }
      static VkLogicOp
   logic_op(enum pipe_logicop func)
   {
      switch (func) {
   case PIPE_LOGICOP_CLEAR: return VK_LOGIC_OP_CLEAR;
   case PIPE_LOGICOP_NOR: return VK_LOGIC_OP_NOR;
   case PIPE_LOGICOP_AND_INVERTED: return VK_LOGIC_OP_AND_INVERTED;
   case PIPE_LOGICOP_COPY_INVERTED: return VK_LOGIC_OP_COPY_INVERTED;
   case PIPE_LOGICOP_AND_REVERSE: return VK_LOGIC_OP_AND_REVERSE;
   case PIPE_LOGICOP_INVERT: return VK_LOGIC_OP_INVERT;
   case PIPE_LOGICOP_XOR: return VK_LOGIC_OP_XOR;
   case PIPE_LOGICOP_NAND: return VK_LOGIC_OP_NAND;
   case PIPE_LOGICOP_AND: return VK_LOGIC_OP_AND;
   case PIPE_LOGICOP_EQUIV: return VK_LOGIC_OP_EQUIVALENT;
   case PIPE_LOGICOP_NOOP: return VK_LOGIC_OP_NO_OP;
   case PIPE_LOGICOP_OR_INVERTED: return VK_LOGIC_OP_OR_INVERTED;
   case PIPE_LOGICOP_COPY: return VK_LOGIC_OP_COPY;
   case PIPE_LOGICOP_OR_REVERSE: return VK_LOGIC_OP_OR_REVERSE;
   case PIPE_LOGICOP_OR: return VK_LOGIC_OP_OR;
   case PIPE_LOGICOP_SET: return VK_LOGIC_OP_SET;
   }
      }
      /* from iris */
   static enum pipe_blendfactor
   fix_blendfactor(enum pipe_blendfactor f, bool alpha_to_one)
   {
      if (alpha_to_one) {
      if (f == PIPE_BLENDFACTOR_SRC1_ALPHA)
            if (f == PIPE_BLENDFACTOR_INV_SRC1_ALPHA)
                  }
      static void *
   zink_create_blend_state(struct pipe_context *pctx,
         {
      struct zink_blend_state *cso = CALLOC_STRUCT(zink_blend_state);
   if (!cso)
                  if (blend_state->logicop_enable) {
      cso->logicop_enable = VK_TRUE;
               /* TODO: figure out what to do with dither (nothing is probably "OK" for now,
   *       as dithering is undefined in GL
            /* TODO: these are multisampling-state, and should be set there instead of
   *       here, as that's closer tied to the update-frequency
   */
   cso->alpha_to_coverage = blend_state->alpha_to_coverage;
   cso->alpha_to_one = blend_state->alpha_to_one;
            for (int i = 0; i < blend_state->max_rt + 1; ++i) {
      const struct pipe_rt_blend_state *rt = blend_state->rt;
   if (blend_state->independent_blend_enable)
                     if (rt->blend_enable) {
      att.blendEnable = VK_TRUE;
   att.srcColorBlendFactor = blend_factor(fix_blendfactor(rt->rgb_src_factor, cso->alpha_to_one));
   att.dstColorBlendFactor = blend_factor(fix_blendfactor(rt->rgb_dst_factor, cso->alpha_to_one));
   att.colorBlendOp = blend_op(rt->rgb_func);
   att.srcAlphaBlendFactor = blend_factor(fix_blendfactor(rt->alpha_src_factor, cso->alpha_to_one));
   att.dstAlphaBlendFactor = blend_factor(fix_blendfactor(rt->alpha_dst_factor, cso->alpha_to_one));
               if (rt->colormask & PIPE_MASK_R)
         if (rt->colormask & PIPE_MASK_G)
         if (rt->colormask & PIPE_MASK_B)
         if (rt->colormask & PIPE_MASK_A)
            cso->wrmask |= (rt->colormask << i);
   if (rt->blend_enable)
                     cso->ds3.enables[i] = att.blendEnable;
   cso->ds3.eq[i].alphaBlendOp = att.alphaBlendOp;
   cso->ds3.eq[i].dstAlphaBlendFactor = att.dstAlphaBlendFactor;
   cso->ds3.eq[i].srcAlphaBlendFactor = att.srcAlphaBlendFactor;
   cso->ds3.eq[i].colorBlendOp = att.colorBlendOp;
   cso->ds3.eq[i].dstColorBlendFactor = att.dstColorBlendFactor;
   cso->ds3.eq[i].srcColorBlendFactor = att.srcColorBlendFactor;
      }
               }
      static void
   zink_bind_blend_state(struct pipe_context *pctx, void *cso)
   {
      struct zink_context *ctx = zink_context(pctx);
   struct zink_screen *screen = zink_screen(pctx->screen);
   struct zink_gfx_pipeline_state* state = &zink_context(pctx)->gfx_pipeline_state;
   zink_flush_dgc_if_enabled(ctx);
   struct zink_blend_state *blend = cso;
            if (state->blend_state != cso) {
      state->blend_state = cso;
   if (!screen->have_full_ds3) {
      state->blend_id = blend ? blend->hash : 0;
      }
   bool force_dual_color_blend = screen->driconf.dual_color_blend_by_location &&
         if (force_dual_color_blend != zink_get_fs_base_key(ctx)->force_dual_color_blend)
                  #define STATE_CHECK(NAME, FLAG) \
      if ((!old_blend || old_blend->NAME != blend->NAME)) \
                  STATE_CHECK(alpha_to_coverage, A2C);
   if (screen->info.dynamic_state3_feats.extendedDynamicState3AlphaToOneEnable) {
         }
   STATE_CHECK(enables, ON);
   STATE_CHECK(wrmask, WRITE);
   if (old_blend && blend->num_rts == old_blend->num_rts) {
      if (memcmp(blend->ds3.eq, old_blend->ds3.eq, blend->num_rts * sizeof(blend->ds3.eq[0])))
      } else {
         }
         #undef STATE_CHECK
                  }
      static void
   zink_delete_blend_state(struct pipe_context *pctx, void *blend_state)
   {
         }
      static VkCompareOp
   compare_op(enum pipe_compare_func func)
   {
      switch (func) {
   case PIPE_FUNC_NEVER: return VK_COMPARE_OP_NEVER;
   case PIPE_FUNC_LESS: return VK_COMPARE_OP_LESS;
   case PIPE_FUNC_EQUAL: return VK_COMPARE_OP_EQUAL;
   case PIPE_FUNC_LEQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
   case PIPE_FUNC_GREATER: return VK_COMPARE_OP_GREATER;
   case PIPE_FUNC_NOTEQUAL: return VK_COMPARE_OP_NOT_EQUAL;
   case PIPE_FUNC_GEQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
   case PIPE_FUNC_ALWAYS: return VK_COMPARE_OP_ALWAYS;
   }
      }
      static VkStencilOp
   stencil_op(enum pipe_stencil_op op)
   {
      switch (op) {
   case PIPE_STENCIL_OP_KEEP: return VK_STENCIL_OP_KEEP;
   case PIPE_STENCIL_OP_ZERO: return VK_STENCIL_OP_ZERO;
   case PIPE_STENCIL_OP_REPLACE: return VK_STENCIL_OP_REPLACE;
   case PIPE_STENCIL_OP_INCR: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
   case PIPE_STENCIL_OP_DECR: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
   case PIPE_STENCIL_OP_INCR_WRAP: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
   case PIPE_STENCIL_OP_DECR_WRAP: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
   case PIPE_STENCIL_OP_INVERT: return VK_STENCIL_OP_INVERT;
   }
      }
      static VkStencilOpState
   stencil_op_state(const struct pipe_stencil_state *src)
   {
      VkStencilOpState ret;
   ret.failOp = stencil_op(src->fail_op);
   ret.passOp = stencil_op(src->zpass_op);
   ret.depthFailOp = stencil_op(src->zfail_op);
   ret.compareOp = compare_op(src->func);
   ret.compareMask = src->valuemask;
   ret.writeMask = src->writemask;
   ret.reference = 0; // not used: we'll use a dynamic state for this
      }
      static void *
   zink_create_depth_stencil_alpha_state(struct pipe_context *pctx,
         {
      struct zink_depth_stencil_alpha_state *cso = CALLOC_STRUCT(zink_depth_stencil_alpha_state);
   if (!cso)
                     if (depth_stencil_alpha->depth_enabled) {
      cso->hw_state.depth_test = VK_TRUE;
               if (depth_stencil_alpha->depth_bounds_test) {
      cso->hw_state.depth_bounds_test = VK_TRUE;
   cso->hw_state.min_depth_bounds = depth_stencil_alpha->depth_bounds_min;
               if (depth_stencil_alpha->stencil[0].enabled) {
      cso->hw_state.stencil_test = VK_TRUE;
               if (depth_stencil_alpha->stencil[1].enabled)
         else
                        }
      static void
   zink_bind_depth_stencil_alpha_state(struct pipe_context *pctx, void *cso)
   {
               zink_flush_dgc_if_enabled(ctx);
            if (cso) {
      struct zink_gfx_pipeline_state *state = &ctx->gfx_pipeline_state;
   if (state->dyn_state1.depth_stencil_alpha_state != &ctx->dsa_state->hw_state) {
      state->dyn_state1.depth_stencil_alpha_state = &ctx->dsa_state->hw_state;
   state->dirty |= !zink_screen(pctx->screen)->info.have_EXT_extended_dynamic_state;
         }
   if (!ctx->track_renderpasses && !ctx->blitting)
      }
      static void
   zink_delete_depth_stencil_alpha_state(struct pipe_context *pctx,
         {
         }
      static float
   round_to_granularity(float value, float granularity)
   {
         }
      static float
   line_width(float width, float granularity, const float range[2])
   {
      assert(granularity >= 0);
            if (granularity > 0)
               }
      static void *
   zink_create_rasterizer_state(struct pipe_context *pctx,
         {
               struct zink_rasterizer_state *state = CALLOC_STRUCT(zink_rasterizer_state);
   if (!state)
            state->base = *rs_state;
            state->hw_state.line_stipple_enable =
      rs_state->line_stipple_enable &&
         assert(rs_state->depth_clip_far == rs_state->depth_clip_near);
   state->hw_state.depth_clip = rs_state->depth_clip_near;
   state->hw_state.depth_clamp = rs_state->depth_clamp;
   state->hw_state.pv_last = !rs_state->flatshade_first;
            assert(rs_state->fill_front <= PIPE_POLYGON_MODE_POINT);
   if (rs_state->fill_back != rs_state->fill_front)
            if (rs_state->fill_front == PIPE_POLYGON_MODE_POINT &&
      screen->driver_workarounds.no_hw_gl_point) {
   state->hw_state.polygon_mode = VK_POLYGON_MODE_FILL;
      } else {
      state->hw_state.polygon_mode = rs_state->fill_front; // same values
               state->front_face = rs_state->front_ccw ?
                  state->hw_state.line_mode = VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT;
   if (rs_state->line_rectangular) {
      if (rs_state->line_smooth &&
      !screen->driver_workarounds.no_linesmooth)
      else
      } else {
         }
   state->dynamic_line_mode = state->hw_state.line_mode;
   switch (state->hw_state.line_mode) {
   case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT:
      if (!screen->info.line_rast_feats.rectangularLines)
            case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT:
      if (!screen->info.line_rast_feats.smoothLines)
            case VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT:
      if (!screen->info.line_rast_feats.bresenhamLines)
            default: break;
            if (!rs_state->line_stipple_enable) {
      state->base.line_stipple_factor = 1;
               state->offset_fill = util_get_offset(rs_state, rs_state->fill_front);
   state->offset_units = rs_state->offset_units;
   if (!rs_state->offset_units_unscaled)
         state->offset_clamp = rs_state->offset_clamp;
            state->line_width = line_width(rs_state->line_width,
                     }
      static void
   zink_bind_rasterizer_state(struct pipe_context *pctx, void *cso)
   {
      struct zink_context *ctx = zink_context(pctx);
   struct zink_screen *screen = zink_screen(pctx->screen);
   struct zink_rasterizer_state *prev_state = ctx->rast_state;
   bool point_quad_rasterization = ctx->rast_state ? ctx->rast_state->base.point_quad_rasterization : false;
   bool scissor = ctx->rast_state ? ctx->rast_state->base.scissor : false;
   bool pv_last = ctx->rast_state ? ctx->rast_state->hw_state.pv_last : false;
   bool force_persample_interp = ctx->gfx_pipeline_state.force_persample_interp;
   bool clip_halfz = ctx->rast_state ? ctx->rast_state->hw_state.clip_halfz : false;
   bool rasterizer_discard = ctx->rast_state ? ctx->rast_state->base.rasterizer_discard : false;
   bool half_pixel_center = ctx->rast_state ? ctx->rast_state->base.half_pixel_center : true;
   float line_width = ctx->rast_state ? ctx->rast_state->base.line_width : 1.0;
   zink_flush_dgc_if_enabled(ctx);
            if (ctx->rast_state) {
      if (screen->info.have_EXT_provoking_vertex &&
      pv_last != ctx->rast_state->hw_state.pv_last &&
   /* without this prop, change in pv mode requires new rp */
   !screen->info.pv_props.provokingVertexModePerPipeline)
               ctx->gfx_pipeline_state.dirty |= !zink_screen(pctx->screen)->info.have_EXT_extended_dynamic_state3;
            if (clip_halfz != ctx->rast_state->base.clip_halfz) {
      if (screen->info.have_EXT_depth_clip_control)
         else
                     #define STATE_CHECK(NAME, FLAG) \
      if (cso && (!prev_state || prev_state->NAME != ctx->rast_state->NAME)) \
                  if (!screen->driver_workarounds.no_linestipple) {
      if (ctx->rast_state->base.line_stipple_enable) {
      STATE_CHECK(base.line_stipple_factor, STIPPLE);
      } else {
         }
   if (screen->info.dynamic_state3_feats.extendedDynamicState3LineStippleEnable) {
            }
   STATE_CHECK(hw_state.depth_clip, CLIP);
   STATE_CHECK(hw_state.depth_clamp, CLAMP);
   STATE_CHECK(hw_state.polygon_mode, POLYGON);
   STATE_CHECK(hw_state.clip_halfz, HALFZ);
         #undef STATE_CHECK
                  if (fabs(ctx->rast_state->base.line_width - line_width) > FLT_EPSILON)
            bool lower_gl_point = screen->driver_workarounds.no_hw_gl_point;
   lower_gl_point &= ctx->rast_state->base.fill_front == PIPE_POLYGON_MODE_POINT;
   if (zink_get_gs_key(ctx)->lower_gl_point != lower_gl_point)
            if (ctx->gfx_pipeline_state.dyn_state1.front_face != ctx->rast_state->front_face) {
      ctx->gfx_pipeline_state.dyn_state1.front_face = ctx->rast_state->front_face;
      }
   if (ctx->gfx_pipeline_state.dyn_state1.cull_mode != ctx->rast_state->cull_mode) {
      ctx->gfx_pipeline_state.dyn_state1.cull_mode = ctx->rast_state->cull_mode;
      }
   if (!ctx->primitives_generated_active)
         else if (rasterizer_discard != ctx->rast_state->base.rasterizer_discard)
            if (ctx->rast_state->base.point_quad_rasterization ||
      ctx->rast_state->base.point_quad_rasterization != point_quad_rasterization)
      if (ctx->rast_state->base.scissor != scissor)
            if (ctx->rast_state->base.force_persample_interp != force_persample_interp) {
      zink_set_fs_base_key(ctx)->force_persample_interp = ctx->rast_state->base.force_persample_interp;
      }
            if (ctx->rast_state->base.half_pixel_center != half_pixel_center)
            if (!screen->optimal_keys)
         }
      static void
   zink_delete_rasterizer_state(struct pipe_context *pctx, void *rs_state)
   {
         }
      struct pipe_vertex_state *
   zink_create_vertex_state(struct pipe_screen *pscreen,
                           struct pipe_vertex_buffer *buffer,
   {
      struct zink_vertex_state *zstate = CALLOC_STRUCT(zink_vertex_state);
   if (!zstate) {
      mesa_loge("ZINK: failed to allocate zstate!");
               util_init_pipe_vertex_state(pscreen, buffer, elements, num_elements, indexbuf, full_velem_mask,
            /* Initialize the vertex element state in state->element.
   * Do it by creating a vertex element state object and copying it there.
   */
   struct zink_context ctx;
   ctx.base.screen = pscreen;
   struct zink_vertex_elements_state *elems = zink_create_vertex_elements_state(&ctx.base, num_elements, elements);
   zstate->velems = *elems;
               }
      void
   zink_vertex_state_destroy(struct pipe_screen *pscreen, struct pipe_vertex_state *vstate)
   {
      pipe_vertex_buffer_unreference(&vstate->input.vbuffer);
   pipe_resource_reference(&vstate->input.indexbuf, NULL);
      }
      struct pipe_vertex_state *
   zink_cache_create_vertex_state(struct pipe_screen *pscreen,
                                 {
               return util_vertex_state_cache_get(pscreen, buffer, elements, num_elements, indexbuf,
      }
      void
   zink_cache_vertex_state_destroy(struct pipe_screen *pscreen, struct pipe_vertex_state *vstate)
   {
                  }
      void
   zink_context_state_init(struct pipe_context *pctx)
   {
      pctx->create_vertex_elements_state = zink_create_vertex_elements_state;
   pctx->bind_vertex_elements_state = zink_bind_vertex_elements_state;
            pctx->create_blend_state = zink_create_blend_state;
   pctx->bind_blend_state = zink_bind_blend_state;
            pctx->create_depth_stencil_alpha_state = zink_create_depth_stencil_alpha_state;
   pctx->bind_depth_stencil_alpha_state = zink_bind_depth_stencil_alpha_state;
            pctx->create_rasterizer_state = zink_create_rasterizer_state;
   pctx->bind_rasterizer_state = zink_bind_rasterizer_state;
      }
