   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "d3d12_blit.h"
   #include "d3d12_cmd_signature.h"
   #include "d3d12_context.h"
   #include "d3d12_compiler.h"
   #include "d3d12_compute_transforms.h"
   #include "d3d12_debug.h"
   #include "d3d12_fence.h"
   #include "d3d12_format.h"
   #include "d3d12_query.h"
   #include "d3d12_resource.h"
   #include "d3d12_root_signature.h"
   #include "d3d12_screen.h"
   #include "d3d12_surface.h"
   #ifdef HAVE_GALLIUM_D3D12_VIDEO
   #include "d3d12_video_dec.h"
   #include "d3d12_video_enc.h"
   #include "d3d12_video_proc.h"
   #include "d3d12_video_buffer.h"
   #endif
   #include "indices/u_primconvert.h"
   #include "util/u_atomic.h"
   #include "util/u_blitter.h"
   #include "util/u_dual_blend.h"
   #include "util/u_framebuffer.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_pstipple.h"
   #include "util/u_sample_positions.h"
   #include "util/u_dl.h"
   #include "nir_to_dxil.h"
      #include <dxguids/dxguids.h>
      #include <string.h>
      #ifdef _WIN32
   #include "dxil_validator.h"
   #endif
      #ifdef _GAMING_XBOX
   typedef D3D12_DEPTH_STENCILOP_DESC d3d12_depth_stencil_op_desc_type;
   #else
   typedef D3D12_DEPTH_STENCILOP_DESC1 d3d12_depth_stencil_op_desc_type;
   #endif
      static void
   d3d12_context_destroy(struct pipe_context *pctx)
   {
               struct d3d12_screen *screen = d3d12_screen(pctx->screen);
   mtx_lock(&screen->submit_mutex);
   list_del(&ctx->context_list_entry);
   if (ctx->id != D3D12_CONTEXT_NO_ID)
               #ifdef _WIN32
         #endif
      #ifndef _GAMING_XBOX
      if (ctx->dev_config)
      #endif
         if (ctx->timestamp_query)
            util_unreference_framebuffer_state(&ctx->fb);
   util_blitter_destroy(ctx->blitter);
   d3d12_end_batch(ctx, d3d12_current_batch(ctx));
   for (unsigned i = 0; i < ARRAY_SIZE(ctx->batches); ++i)
         ctx->cmdlist->Release();
   if (ctx->cmdlist8)
         d3d12_descriptor_pool_free(ctx->sampler_pool);
   util_primconvert_destroy(ctx->primconvert);
   slab_destroy_child(&ctx->transfer_pool);
   slab_destroy_child(&ctx->transfer_pool_unsync);
   d3d12_gs_variant_cache_destroy(ctx);
   d3d12_tcs_variant_cache_destroy(ctx);
   d3d12_gfx_pipeline_state_cache_destroy(ctx);
   d3d12_compute_pipeline_state_cache_destroy(ctx);
   d3d12_root_signature_cache_destroy(ctx);
   d3d12_cmd_signature_cache_destroy(ctx);
   d3d12_compute_transform_cache_destroy(ctx);
   d3d12_context_state_table_destroy(ctx);
   pipe_resource_reference(&ctx->pstipple.texture, nullptr);
   pipe_sampler_view_reference(&ctx->pstipple.sampler_view, nullptr);
   util_dynarray_fini(&ctx->recently_destroyed_bos);
   util_dynarray_fini(&ctx->ended_queries);
                     if (pctx->stream_uploader)
         if (pctx->const_uploader)
               }
      static void *
   d3d12_create_vertex_elements_state(struct pipe_context *pctx,
               {
      struct d3d12_vertex_elements_state *cso = CALLOC_STRUCT(d3d12_vertex_elements_state);
   if (!cso)
            unsigned max_vb = 0;
   for (unsigned i = 0; i < num_elements; ++i) {
      cso->elements[i].SemanticName = "TEXCOORD";
            enum pipe_format format_helper =
         bool needs_emulation = format_helper != elements[i].src_format;
   cso->needs_format_emulation |= needs_emulation;
   cso->format_conversion[i] =
            cso->elements[i].Format = d3d12_get_format(format_helper);
   assert(cso->elements[i].Format != DXGI_FORMAT_UNKNOWN);
   cso->elements[i].InputSlot = elements[i].vertex_buffer_index;
            if (elements[i].instance_divisor) {
      cso->elements[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
      } else {
      cso->elements[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
      }
   max_vb = MAX2(max_vb, elements[i].vertex_buffer_index);
               cso->num_elements = num_elements;
   cso->num_buffers = num_elements ? max_vb + 1 : 0;
      }
      static void
   d3d12_bind_vertex_elements_state(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
   ctx->gfx_pipeline_state.ves = (struct d3d12_vertex_elements_state *)ve;
      }
      static void
   d3d12_delete_vertex_elements_state(struct pipe_context *pctx,
         {
         }
      static D3D12_BLEND
   blend_factor_rgb(enum pipe_blendfactor factor)
   {
      switch (factor) {
   case PIPE_BLENDFACTOR_ZERO: return D3D12_BLEND_ZERO;
   case PIPE_BLENDFACTOR_ONE: return D3D12_BLEND_ONE;
   case PIPE_BLENDFACTOR_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
   case PIPE_BLENDFACTOR_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
   case PIPE_BLENDFACTOR_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
   case PIPE_BLENDFACTOR_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
   case PIPE_BLENDFACTOR_CONST_COLOR: return D3D12_BLEND_BLEND_FACTOR;
   case PIPE_BLENDFACTOR_SRC1_COLOR: return D3D12_BLEND_SRC1_COLOR;
   case PIPE_BLENDFACTOR_SRC1_ALPHA: return D3D12_BLEND_SRC1_ALPHA;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
   case PIPE_BLENDFACTOR_INV_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR: return D3D12_BLEND_INV_BLEND_FACTOR;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR: return D3D12_BLEND_INV_SRC1_COLOR;
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA: return D3D12_BLEND_INV_SRC1_ALPHA;
   case PIPE_BLENDFACTOR_CONST_ALPHA: return D3D12_BLEND_BLEND_FACTOR; /* Doesn't exist in D3D12 */
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA: return D3D12_BLEND_INV_BLEND_FACTOR; /* Doesn't exist in D3D12 */
   }
      }
      static D3D12_BLEND
   blend_factor_alpha(enum pipe_blendfactor factor)
   {
      switch (factor) {
   case PIPE_BLENDFACTOR_ZERO: return D3D12_BLEND_ZERO;
   case PIPE_BLENDFACTOR_ONE: return D3D12_BLEND_ONE;
   case PIPE_BLENDFACTOR_SRC_COLOR:
   case PIPE_BLENDFACTOR_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
   case PIPE_BLENDFACTOR_DST_COLOR:
   case PIPE_BLENDFACTOR_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
   case PIPE_BLENDFACTOR_CONST_COLOR:
   case PIPE_BLENDFACTOR_CONST_ALPHA: return D3D12_BLEND_BLEND_FACTOR;
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_SRC1_ALPHA: return D3D12_BLEND_SRC1_ALPHA;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
   case PIPE_BLENDFACTOR_INV_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA: return D3D12_BLEND_INV_SRC1_ALPHA;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA: return D3D12_BLEND_INV_BLEND_FACTOR;
   }
      }
      static unsigned
   need_blend_factor_rgb(enum pipe_blendfactor factor)
   {
      switch (factor) {
   case PIPE_BLENDFACTOR_CONST_COLOR:
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
         case PIPE_BLENDFACTOR_CONST_ALPHA:
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
            default:
            }
      static unsigned
   need_blend_factor_alpha(enum pipe_blendfactor factor)
   {
      switch (factor) {
   case PIPE_BLENDFACTOR_CONST_COLOR:
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
   case PIPE_BLENDFACTOR_CONST_ALPHA:
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
            default:
            }
      static D3D12_BLEND_OP
   blend_op(enum pipe_blend_func func)
   {
      switch (func) {
   case PIPE_BLEND_ADD: return D3D12_BLEND_OP_ADD;
   case PIPE_BLEND_SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
   case PIPE_BLEND_REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
   case PIPE_BLEND_MIN: return D3D12_BLEND_OP_MIN;
   case PIPE_BLEND_MAX: return D3D12_BLEND_OP_MAX;
   }
      }
      static D3D12_COMPARISON_FUNC
   compare_op(enum pipe_compare_func op)
   {
      switch (op) {
      case PIPE_FUNC_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
   case PIPE_FUNC_LESS: return D3D12_COMPARISON_FUNC_LESS;
   case PIPE_FUNC_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
   case PIPE_FUNC_LEQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
   case PIPE_FUNC_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
   case PIPE_FUNC_NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
   case PIPE_FUNC_GEQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
      }
      }
      static D3D12_LOGIC_OP
   logic_op(enum pipe_logicop func)
   {
      switch (func) {
   case PIPE_LOGICOP_CLEAR: return D3D12_LOGIC_OP_CLEAR;
   case PIPE_LOGICOP_NOR: return D3D12_LOGIC_OP_NOR;
   case PIPE_LOGICOP_AND_INVERTED: return D3D12_LOGIC_OP_AND_INVERTED;
   case PIPE_LOGICOP_COPY_INVERTED: return D3D12_LOGIC_OP_COPY_INVERTED;
   case PIPE_LOGICOP_AND_REVERSE: return D3D12_LOGIC_OP_AND_REVERSE;
   case PIPE_LOGICOP_INVERT: return D3D12_LOGIC_OP_INVERT;
   case PIPE_LOGICOP_XOR: return D3D12_LOGIC_OP_XOR;
   case PIPE_LOGICOP_NAND: return D3D12_LOGIC_OP_NAND;
   case PIPE_LOGICOP_AND: return D3D12_LOGIC_OP_AND;
   case PIPE_LOGICOP_EQUIV: return D3D12_LOGIC_OP_EQUIV;
   case PIPE_LOGICOP_NOOP: return D3D12_LOGIC_OP_NOOP;
   case PIPE_LOGICOP_OR_INVERTED: return D3D12_LOGIC_OP_OR_INVERTED;
   case PIPE_LOGICOP_COPY: return D3D12_LOGIC_OP_COPY;
   case PIPE_LOGICOP_OR_REVERSE: return D3D12_LOGIC_OP_OR_REVERSE;
   case PIPE_LOGICOP_OR: return D3D12_LOGIC_OP_OR;
   case PIPE_LOGICOP_SET: return D3D12_LOGIC_OP_SET;
   }
      }
      static UINT8
   color_write_mask(unsigned colormask)
   {
               if (colormask & PIPE_MASK_R)
         if (colormask & PIPE_MASK_G)
         if (colormask & PIPE_MASK_B)
         if (colormask & PIPE_MASK_A)
               }
      static void *
   d3d12_create_blend_state(struct pipe_context *pctx,
         {
      struct d3d12_blend_state *state = CALLOC_STRUCT(d3d12_blend_state);
   if (!state)
            if (blend_state->logicop_enable) {
      state->desc.RenderTarget[0].LogicOpEnable = true;
                                 int num_targets = 1;
   if (blend_state->independent_blend_enable) {
      state->desc.IndependentBlendEnable = true;
               for (int i = 0; i < num_targets; ++i) {
               if (rt->blend_enable) {
      state->desc.RenderTarget[i].BlendEnable = true;
   state->desc.RenderTarget[i].SrcBlend = blend_factor_rgb((pipe_blendfactor) rt->rgb_src_factor);
   state->desc.RenderTarget[i].DestBlend = blend_factor_rgb((pipe_blendfactor) rt->rgb_dst_factor);
   state->desc.RenderTarget[i].BlendOp = blend_op((pipe_blend_func) rt->rgb_func);
   state->desc.RenderTarget[i].SrcBlendAlpha = blend_factor_alpha((pipe_blendfactor) rt->alpha_src_factor);
                  state->blend_factor_flags |= need_blend_factor_rgb((pipe_blendfactor) rt->rgb_src_factor);
   state->blend_factor_flags |= need_blend_factor_rgb((pipe_blendfactor) rt->rgb_dst_factor);
                  if (state->blend_factor_flags == (D3D12_BLEND_FACTOR_COLOR | D3D12_BLEND_FACTOR_ALPHA) &&
      (d3d12_debug & D3D12_DEBUG_VERBOSE)) {
   /* We can't set a blend factor for both constant color and constant alpha */
               if (util_blend_state_is_dual(blend_state, i))
                              }
      static void
   d3d12_bind_blend_state(struct pipe_context *pctx, void *blend_state)
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_blend_state *new_state = (struct d3d12_blend_state *) blend_state;
            ctx->gfx_pipeline_state.blend = new_state;
   ctx->state_dirty |= D3D12_DIRTY_BLEND;
   if (new_state == NULL || old_state == NULL ||
      new_state->blend_factor_flags != old_state->blend_factor_flags)
         if (new_state == NULL)
         else if (new_state != NULL && (old_state == NULL || old_state->is_dual_src != new_state->is_dual_src))
      }
      static void
   d3d12_delete_blend_state(struct pipe_context *pctx, void *blend_state)
   {
      d3d12_gfx_pipeline_state_cache_invalidate(d3d12_context(pctx), blend_state);
      }
      static D3D12_STENCIL_OP
   stencil_op(enum pipe_stencil_op op)
   {
      switch (op) {
   case PIPE_STENCIL_OP_KEEP: return D3D12_STENCIL_OP_KEEP;
   case PIPE_STENCIL_OP_ZERO: return D3D12_STENCIL_OP_ZERO;
   case PIPE_STENCIL_OP_REPLACE: return D3D12_STENCIL_OP_REPLACE;
   case PIPE_STENCIL_OP_INCR: return D3D12_STENCIL_OP_INCR_SAT;
   case PIPE_STENCIL_OP_DECR: return D3D12_STENCIL_OP_DECR_SAT;
   case PIPE_STENCIL_OP_INCR_WRAP: return D3D12_STENCIL_OP_INCR;
   case PIPE_STENCIL_OP_DECR_WRAP: return D3D12_STENCIL_OP_DECR;
   case PIPE_STENCIL_OP_INVERT: return D3D12_STENCIL_OP_INVERT;
   }
      }
      static d3d12_depth_stencil_op_desc_type
   stencil_op_state(const struct pipe_stencil_state *src)
   {
      d3d12_depth_stencil_op_desc_type ret;
   ret.StencilFailOp = stencil_op((pipe_stencil_op) src->fail_op);
   ret.StencilPassOp = stencil_op((pipe_stencil_op) src->zpass_op);
   ret.StencilDepthFailOp = stencil_op((pipe_stencil_op) src->zfail_op);
      #ifndef _GAMING_XBOX
      ret.StencilReadMask = src->valuemask;
      #endif
         }
      static void *
   d3d12_create_depth_stencil_alpha_state(struct pipe_context *pctx,
         {
      struct d3d12_depth_stencil_alpha_state *dsa = CALLOC_STRUCT(d3d12_depth_stencil_alpha_state);
   if (!dsa)
            if (depth_stencil_alpha->depth_enabled) {
      dsa->desc.DepthEnable = true;
               /* TODO Add support for GL_depth_bound_tests */
   #if 0
   if (depth_stencil_alpha->depth.bounds_test) {
      dsa->desc.DepthBoundsTestEnable = true;
   dsa->min_depth_bounds = depth_stencil_alpha->depth.bounds_min;
      }
            if (depth_stencil_alpha->stencil[0].enabled) {
      dsa->desc.StencilEnable = true;
               if (depth_stencil_alpha->stencil[1].enabled) {
      dsa->backface_enabled = true;
      #ifndef _GAMING_XBOX
                  if (!screen->opts14.IndependentFrontAndBackStencilRefMaskSupported) {
      dsa->desc.BackFace.StencilReadMask = dsa->desc.FrontFace.StencilReadMask;
      #endif
      }
   else {
                              }
      static void
   d3d12_bind_depth_stencil_alpha_state(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
   ctx->gfx_pipeline_state.zsa = (struct d3d12_depth_stencil_alpha_state *) dsa;
      }
      static void
   d3d12_delete_depth_stencil_alpha_state(struct pipe_context *pctx,
         {
      d3d12_gfx_pipeline_state_cache_invalidate(d3d12_context(pctx), dsa_state);
      }
      static D3D12_FILL_MODE
   fill_mode(unsigned mode)
   {
      switch (mode) {
   case PIPE_POLYGON_MODE_FILL:
         case PIPE_POLYGON_MODE_LINE:
         case PIPE_POLYGON_MODE_POINT:
            default:
            }
      static void *
   d3d12_create_rasterizer_state(struct pipe_context *pctx,
         {
      struct d3d12_rasterizer_state *cso = CALLOC_STRUCT(d3d12_rasterizer_state);
   if (!cso)
                              switch (rs_state->cull_face) {
   case PIPE_FACE_NONE:
      if (rs_state->fill_front != rs_state->fill_back) {
      cso->base.cull_face = PIPE_FACE_BACK;
                  /* create a modified CSO for the back-state, so we can draw with
   * either.
   */
   struct pipe_rasterizer_state templ = *rs_state;
   templ.cull_face = PIPE_FACE_FRONT;
                  if (!cso->twoface_back) {
      FREE(cso);
         } else {
      cso->desc.CullMode = D3D12_CULL_MODE_NONE;
      }
         case PIPE_FACE_FRONT:
      cso->desc.CullMode = D3D12_CULL_MODE_FRONT;
   cso->desc.FillMode = fill_mode(rs_state->fill_back);
         case PIPE_FACE_BACK:
      cso->desc.CullMode = D3D12_CULL_MODE_BACK;
   cso->desc.FillMode = fill_mode(rs_state->fill_front);
         case PIPE_FACE_FRONT_AND_BACK:
      /* this is wrong, and we shouldn't actually have to support this! */
   cso->desc.CullMode = D3D12_CULL_MODE_NONE;
   cso->desc.FillMode = D3D12_FILL_MODE_SOLID;
         default:
                  cso->desc.FrontCounterClockwise = rs_state->front_ccw;
   cso->desc.DepthClipEnable = rs_state->depth_clip_near;
   cso->desc.MultisampleEnable = rs_state->multisample;
   cso->desc.AntialiasedLineEnable = rs_state->line_smooth;
   cso->desc.ForcedSampleCount = 0; // TODO
               }
      static void
   d3d12_bind_rasterizer_state(struct pipe_context *pctx, void *rs_state)
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
   ctx->gfx_pipeline_state.rast = (struct d3d12_rasterizer_state *)rs_state;
      }
      static void
   d3d12_delete_rasterizer_state(struct pipe_context *pctx, void *rs_state)
   {
      d3d12_gfx_pipeline_state_cache_invalidate(d3d12_context(pctx), rs_state);
      }
      static D3D12_TEXTURE_ADDRESS_MODE
   sampler_address_mode(enum pipe_tex_wrap wrap, enum pipe_tex_filter filter)
   {
      switch (wrap) {
   case PIPE_TEX_WRAP_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
   case PIPE_TEX_WRAP_CLAMP: return filter == PIPE_TEX_FILTER_NEAREST ?
               case PIPE_TEX_WRAP_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
   case PIPE_TEX_WRAP_MIRROR_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
   case PIPE_TEX_WRAP_MIRROR_CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE; /* not technically correct, but kinda works */
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE; /* FIXME: Doesn't exist in D3D12 */
   }
      }
      static D3D12_FILTER
   get_filter(const struct pipe_sampler_state *state)
   {
      static const D3D12_FILTER lut[16] = {
      D3D12_FILTER_MIN_MAG_MIP_POINT,
   D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
   D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
   D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,
   D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
   D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
   D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
   D3D12_FILTER_MIN_MAG_MIP_LINEAR,
   D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
   D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
   D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
   D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
   D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
   D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
   D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
               static const D3D12_FILTER anisotropic_lut[2] = {
      D3D12_FILTER_ANISOTROPIC,
               if (state->max_anisotropy > 1) {
         } else {
      int idx = (state->mag_img_filter << 1) |
               if (state->min_mip_filter != PIPE_TEX_MIPFILTER_NONE)
               }
      static void *
   d3d12_create_sampler_state(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_screen *screen = d3d12_screen(pctx->screen);
   struct d3d12_sampler_state *ss;
   D3D12_SAMPLER_DESC desc = {};
   if (!state)
            ss = CALLOC_STRUCT(d3d12_sampler_state);
   ss->filter = (pipe_tex_filter)state->min_img_filter;
   ss->wrap_r = (pipe_tex_wrap)state->wrap_r;
   ss->wrap_s = (pipe_tex_wrap)state->wrap_s;
   ss->wrap_t = (pipe_tex_wrap)state->wrap_t;
   ss->lod_bias = state->lod_bias;
   ss->min_lod = state->min_lod;
   ss->max_lod = state->max_lod;
   memcpy(ss->border_color, state->border_color.f, sizeof(float) * 4);
            if (state->min_mip_filter < PIPE_TEX_MIPFILTER_NONE) {
      desc.MinLOD = state->min_lod;
      } else if (state->min_mip_filter == PIPE_TEX_MIPFILTER_NONE) {
      desc.MinLOD = 0;
      } else {
                  if (state->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
         } else if (state->compare_mode == PIPE_TEX_COMPARE_NONE) {
         } else
            desc.MaxAnisotropy = state->max_anisotropy;
            desc.AddressU = sampler_address_mode((pipe_tex_wrap) state->wrap_s,
         desc.AddressV = sampler_address_mode((pipe_tex_wrap) state->wrap_t,
         desc.AddressW = sampler_address_mode((pipe_tex_wrap) state->wrap_r,
         desc.MipLODBias = CLAMP(state->lod_bias, -16.0f, 15.99f);
            // TODO Normalized Coordinates?
   d3d12_descriptor_pool_alloc_handle(ctx->sampler_pool, &ss->handle);
            if (state->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
      desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            d3d12_descriptor_pool_alloc_handle(ctx->sampler_pool,
         screen->dev->CreateSampler(&desc,
                        }
      static void
   d3d12_bind_sampler_states(struct pipe_context *pctx,
                           {
            #define STATIC_ASSERT_PIPE_EQUAL_COMP_FUNC(X) \
               STATIC_ASSERT_PIPE_EQUAL_COMP_FUNC(LESS);
   STATIC_ASSERT_PIPE_EQUAL_COMP_FUNC(GREATER);
   STATIC_ASSERT_PIPE_EQUAL_COMP_FUNC(LEQUAL);
   STATIC_ASSERT_PIPE_EQUAL_COMP_FUNC(GEQUAL);
   STATIC_ASSERT_PIPE_EQUAL_COMP_FUNC(NOTEQUAL);
   STATIC_ASSERT_PIPE_EQUAL_COMP_FUNC(NEVER);
         #undef STATIC_ASSERT_PIPE_EQUAL_COMP_FUNC
         for (unsigned i = 0; i < num_samplers; ++i) {
      d3d12_sampler_state *sampler = (struct d3d12_sampler_state*) samplers[i];
   ctx->samplers[shader][start_slot + i] = sampler;
   dxil_wrap_sampler_state &wrap = ctx->tex_wrap_states[shader][start_slot + i];
   if (sampler) {
      wrap.wrap[0] = sampler->wrap_s;
   wrap.wrap[1] = sampler->wrap_t;
   wrap.wrap[2] = sampler->wrap_r;
   wrap.lod_bias = sampler->lod_bias;
   wrap.min_lod = sampler->min_lod;
   wrap.max_lod = sampler->max_lod;
   memcpy(wrap.border_color, sampler->border_color, 4 * sizeof(float));
      } else {
                     ctx->num_samplers[shader] = start_slot + num_samplers;
      }
      static void
   d3d12_delete_sampler_state(struct pipe_context *pctx,
         {
      struct d3d12_batch *batch = d3d12_current_batch(d3d12_context(pctx));
   struct d3d12_sampler_state *state = (struct d3d12_sampler_state*) ss;
   util_dynarray_append(&batch->zombie_samplers, d3d12_descriptor_handle,
         if (state->is_shadow_sampler)
      util_dynarray_append(&batch->zombie_samplers, d3d12_descriptor_handle,
         }
      static D3D12_SRV_DIMENSION
   view_dimension(enum pipe_texture_target target, unsigned samples)
   {
      switch (target) {
   case PIPE_BUFFER: return D3D12_SRV_DIMENSION_BUFFER;
   case PIPE_TEXTURE_1D: return D3D12_SRV_DIMENSION_TEXTURE1D;
   case PIPE_TEXTURE_1D_ARRAY: return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D:
      return samples > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DMS :
      case PIPE_TEXTURE_2D_ARRAY:
      return samples > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY :
      case PIPE_TEXTURE_CUBE: return D3D12_SRV_DIMENSION_TEXTURECUBE;
   case PIPE_TEXTURE_CUBE_ARRAY: return D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
   case PIPE_TEXTURE_3D: return D3D12_SRV_DIMENSION_TEXTURE3D;
   default:
            }
      static D3D12_SHADER_COMPONENT_MAPPING
   component_mapping(enum pipe_swizzle swizzle, D3D12_SHADER_COMPONENT_MAPPING id)
   {
      switch (swizzle) {
   case PIPE_SWIZZLE_X: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0;
   case PIPE_SWIZZLE_Y: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1;
   case PIPE_SWIZZLE_Z: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2;
   case PIPE_SWIZZLE_W: return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3;
   case PIPE_SWIZZLE_0: return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
   case PIPE_SWIZZLE_1: return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;
   case PIPE_SWIZZLE_NONE: return id;
   default:
            }
      void
   d3d12_init_sampler_view_descriptor(struct d3d12_sampler_view *sampler_view)
   {
      struct pipe_sampler_view *state = &sampler_view->base;
   struct pipe_resource *texture = state->texture;
   struct d3d12_resource *res = d3d12_resource(texture);
            struct d3d12_format_info format_info = d3d12_get_format_info(res->overall_format, state->format, state->target);
   D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
   desc.Format = d3d12_get_resource_srv_format(state->format, state->target);
            /* Integer cube textures are not really supported, because TextureLoad doesn't exist
   * for cube maps, and we sampling is not supported for integer textures, so we have to
   * handle this SRV as if it were a 2D texture array */
   if ((desc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBE ||
      desc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBEARRAY) &&
   util_format_is_pure_integer(state->format)) {
               desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
      component_mapping((pipe_swizzle)sampler_view->swizzle_override_r, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0),
   component_mapping((pipe_swizzle)sampler_view->swizzle_override_g, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1),
   component_mapping((pipe_swizzle)sampler_view->swizzle_override_b, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2),
               uint64_t offset = 0;
   ID3D12Resource *d3d12_res = d3d12_resource_underlying(res, &offset);
            unsigned array_size = state->u.tex.last_layer - state->u.tex.first_layer + 1;
   switch (desc.ViewDimension) {
   case D3D12_SRV_DIMENSION_TEXTURE1D:
      if (state->u.tex.first_layer > 0)
                  desc.Texture1D.MostDetailedMip = state->u.tex.first_level;
   desc.Texture1D.MipLevels = sampler_view->mip_levels;
   desc.Texture1D.ResourceMinLODClamp = 0.0f;
      case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
      desc.Texture1DArray.MostDetailedMip = state->u.tex.first_level;
   desc.Texture1DArray.MipLevels = sampler_view->mip_levels;
   desc.Texture1DArray.ResourceMinLODClamp = 0.0f;
   desc.Texture1DArray.FirstArraySlice = state->u.tex.first_layer;
   desc.Texture1DArray.ArraySize = array_size;
      case D3D12_SRV_DIMENSION_TEXTURE2D:
      if (state->u.tex.first_layer > 0)
                  desc.Texture2D.MostDetailedMip = state->u.tex.first_level;
   desc.Texture2D.MipLevels = sampler_view->mip_levels;
   desc.Texture2D.PlaneSlice = format_info.plane_slice;
   desc.Texture2D.ResourceMinLODClamp = 0.0f;
      case D3D12_SRV_DIMENSION_TEXTURE2DMS:
      if (state->u.tex.first_layer > 0)
      debug_printf("D3D12: can't create 2DMS SRV from layer %d\n",
         case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
      desc.Texture2DArray.MostDetailedMip = state->u.tex.first_level;
   desc.Texture2DArray.MipLevels = sampler_view->mip_levels;
   desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
   desc.Texture2DArray.FirstArraySlice = state->u.tex.first_layer;
   desc.Texture2DArray.PlaneSlice = format_info.plane_slice;
   desc.Texture2DArray.ArraySize = array_size;
      case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
      desc.Texture2DMSArray.FirstArraySlice = state->u.tex.first_layer;
   desc.Texture2DMSArray.ArraySize = array_size;
      case D3D12_SRV_DIMENSION_TEXTURE3D:
      if (state->u.tex.first_layer > 0)
                  desc.Texture3D.MostDetailedMip = state->u.tex.first_level;
   desc.Texture3D.MipLevels = sampler_view->mip_levels;
   desc.Texture3D.ResourceMinLODClamp = 0.0f;
      case D3D12_SRV_DIMENSION_TEXTURECUBE:
      if (state->u.tex.first_layer > 0)
                  desc.TextureCube.MostDetailedMip = state->u.tex.first_level;
   desc.TextureCube.MipLevels = sampler_view->mip_levels;
   desc.TextureCube.ResourceMinLODClamp = 0.0f;
      case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
      assert(array_size % 6 == 0);
   desc.TextureCubeArray.MostDetailedMip = state->u.tex.first_level;
   desc.TextureCubeArray.MipLevels = sampler_view->mip_levels;
   desc.TextureCubeArray.First2DArrayFace = state->u.tex.first_layer;
   desc.TextureCubeArray.NumCubes = array_size / 6;
   desc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
      case D3D12_SRV_DIMENSION_BUFFER:
      offset += state->u.buf.offset;
   desc.Buffer.StructureByteStride = 0;
   desc.Buffer.FirstElement = offset / util_format_get_blocksize(state->format);
   desc.Buffer.NumElements = MIN2(state->u.buf.size / util_format_get_blocksize(state->format),
            default:
                  screen->dev->CreateShaderResourceView(d3d12_res, &desc,
      }
      static struct pipe_sampler_view *
   d3d12_create_sampler_view(struct pipe_context *pctx,
               {
      struct d3d12_screen *screen = d3d12_screen(pctx->screen);
   struct d3d12_resource *res = d3d12_resource(texture);
            sampler_view->base = *state;
   sampler_view->base.texture = NULL;
   pipe_resource_reference(&sampler_view->base.texture, texture);
   sampler_view->base.reference.count = 1;
   sampler_view->base.context = pctx;
   sampler_view->mip_levels = state->u.tex.last_level - state->u.tex.first_level + 1;
   sampler_view->array_size = texture->array_size;
            struct d3d12_format_info format_info = d3d12_get_format_info(res->overall_format, state->format, state->target);
   pipe_swizzle swizzle[4] = {
      format_info.swizzle[sampler_view->base.swizzle_r],
   format_info.swizzle[sampler_view->base.swizzle_g],
   format_info.swizzle[sampler_view->base.swizzle_b],
               sampler_view->swizzle_override_r = swizzle[0];
   sampler_view->swizzle_override_g = swizzle[1];
   sampler_view->swizzle_override_b = swizzle[2];
            mtx_lock(&screen->descriptor_pool_mutex);
   d3d12_descriptor_pool_alloc_handle(screen->view_pool, &sampler_view->handle);
                        }
      static void
   d3d12_increment_sampler_view_bind_count(struct pipe_context *ctx,
      enum pipe_shader_type shader_type,
   struct pipe_sampler_view *view) {
      struct d3d12_resource *res = d3d12_resource(view->texture);
   if (res)
   }
      static void
   d3d12_decrement_sampler_view_bind_count(struct pipe_context *ctx,
                  struct d3d12_resource *res = d3d12_resource(view->texture);
   if (res) {
      assert(res->bind_counts[shader_type][D3D12_RESOURCE_BINDING_TYPE_SRV] > 0);
         }
      static void
   d3d12_set_sampler_views(struct pipe_context *pctx,
                           enum pipe_shader_type shader_type,
   unsigned start_slot,
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
   unsigned shader_bit = (1 << shader_type);
            for (unsigned i = 0; i < num_views; ++i) {
      struct pipe_sampler_view *&old_view = ctx->sampler_views[shader_type][start_slot + i];
   if (old_view)
            struct pipe_sampler_view *new_view = views[i];
   if (new_view)
            if (take_ownership) {
      pipe_sampler_view_reference(&old_view, NULL);
      } else {
                  if (views[i]) {
      dxil_wrap_sampler_state &wss = ctx->tex_wrap_states[shader_type][start_slot + i];
   dxil_texture_swizzle_state &swizzle_state = ctx->tex_swizzle_state[shader_type][i];
   if (util_format_is_pure_integer(views[i]->format)) {
      ctx->has_int_samplers |= shader_bit;
   wss.is_int_sampler = 1;
   wss.last_level = views[i]->texture->last_level;
   /* When we emulate a integer cube texture (array) by using a texture 2d Array
   * the coordinates are evaluated to always reside withing the acceptable range
   * because the 3d ray for picking the texel is always pointing at one cube face,
   * hence we can skip the boundary condition handling when the texture operations are
   * lowered to texel fetches later. */
   wss.skip_boundary_conditions = views[i]->target == PIPE_TEXTURE_CUBE ||
      } else {
         }
   /* We need the swizzle state for compare texture lowering, because it
   * encode the use of the shadow texture lookup result as either luminosity,
   * intensity, or alpha. and we need the swizzle state for applying the
   * boundary color correctly */
   struct d3d12_sampler_view *ss = d3d12_sampler_view(views[i]);
   swizzle_state.swizzle_r = ss->swizzle_override_r;
   swizzle_state.swizzle_g = ss->swizzle_override_g;
   swizzle_state.swizzle_b = ss->swizzle_override_b;
                  for (unsigned i = 0; i < unbind_num_trailing_slots; i++) {
      struct pipe_sampler_view *&old_view = ctx->sampler_views[shader_type][start_slot + num_views + i];
   if (old_view)
            }
   ctx->num_sampler_views[shader_type] = start_slot + num_views;
      }
      static void
   d3d12_destroy_sampler_view(struct pipe_context *pctx,
         {
      struct d3d12_sampler_view *view = d3d12_sampler_view(pview);
   d3d12_descriptor_handle_free(&view->handle);
   pipe_resource_reference(&view->base.texture, NULL);
      }
      static void
   delete_shader(struct d3d12_context *ctx, enum pipe_shader_type stage,
         {
               /* Make sure the pipeline state no longer reference the deleted shader */
   struct d3d12_shader *iter = shader->first;
   while (iter) {
      if (ctx->gfx_pipeline_state.stages[stage] == iter) {
      ctx->gfx_pipeline_state.stages[stage] = NULL;
      }
                  }
      static void
   bind_stage(struct d3d12_context *ctx, enum pipe_shader_type stage,
         {
      assert(stage < D3D12_GFX_SHADER_STAGES);
      }
      static void *
   d3d12_create_vs_state(struct pipe_context *pctx,
         {
         }
      static void
   d3d12_bind_vs_state(struct pipe_context *pctx,
         {
      bind_stage(d3d12_context(pctx), PIPE_SHADER_VERTEX,
      }
      static void
   d3d12_delete_vs_state(struct pipe_context *pctx,
         {
      delete_shader(d3d12_context(pctx), PIPE_SHADER_VERTEX,
      }
      static void *
   d3d12_create_fs_state(struct pipe_context *pctx,
         {
         }
      static void
   d3d12_bind_fs_state(struct pipe_context *pctx,
         {
      struct d3d12_context* ctx = d3d12_context(pctx);
   bind_stage(ctx, PIPE_SHADER_FRAGMENT,
         ctx->has_flat_varyings = has_flat_varyings(ctx);
   ctx->missing_dual_src_outputs = missing_dual_src_outputs(ctx);
      }
      static void
   d3d12_delete_fs_state(struct pipe_context *pctx,
         {
      delete_shader(d3d12_context(pctx), PIPE_SHADER_FRAGMENT,
      }
      static void *
   d3d12_create_gs_state(struct pipe_context *pctx,
         {
         }
      static void
   d3d12_bind_gs_state(struct pipe_context *pctx, void *gss)
   {
      bind_stage(d3d12_context(pctx), PIPE_SHADER_GEOMETRY,
      }
      static void
   d3d12_delete_gs_state(struct pipe_context *pctx, void *gs)
   {
      delete_shader(d3d12_context(pctx), PIPE_SHADER_GEOMETRY,
      }
      static void *
   d3d12_create_tcs_state(struct pipe_context *pctx,
         {
         }
      static void
   d3d12_bind_tcs_state(struct pipe_context *pctx, void *tcss)
   {
      bind_stage(d3d12_context(pctx), PIPE_SHADER_TESS_CTRL,
      }
      static void
   d3d12_delete_tcs_state(struct pipe_context *pctx, void *tcs)
   {
      delete_shader(d3d12_context(pctx), PIPE_SHADER_TESS_CTRL,
      }
      static void *
   d3d12_create_tes_state(struct pipe_context *pctx,
         {
         }
      static void
   d3d12_bind_tes_state(struct pipe_context *pctx, void *tess)
   {
      bind_stage(d3d12_context(pctx), PIPE_SHADER_TESS_EVAL,
      }
      static void
   d3d12_delete_tes_state(struct pipe_context *pctx, void *tes)
   {
      delete_shader(d3d12_context(pctx), PIPE_SHADER_TESS_EVAL,
      }
      static void *
   d3d12_create_compute_state(struct pipe_context *pctx,
         {
         }
      static void
   d3d12_bind_compute_state(struct pipe_context *pctx, void *css)
   {
         }
      static void
   d3d12_delete_compute_state(struct pipe_context *pctx, void *cs)
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_shader_selector *shader = (struct d3d12_shader_selector *)cs;
            /* Make sure the pipeline state no longer reference the deleted shader */
   struct d3d12_shader *iter = shader->first;
   while (iter) {
      if (ctx->compute_pipeline_state.stage == iter) {
      ctx->compute_pipeline_state.stage = NULL;
      }
                  }
      static bool
   d3d12_init_polygon_stipple(struct pipe_context *pctx)
   {
               ctx->pstipple.texture = util_pstipple_create_stipple_texture(pctx, NULL);
   if (!ctx->pstipple.texture)
            ctx->pstipple.sampler_view = util_pstipple_create_sampler_view(pctx, ctx->pstipple.texture);
   if (!ctx->pstipple.sampler_view)
            ctx->pstipple.sampler_cso = (struct d3d12_sampler_state *)util_pstipple_create_sampler(pctx);
   if (!ctx->pstipple.sampler_cso)
               }
      static void
   d3d12_set_polygon_stipple(struct pipe_context *pctx,
         {
      static bool initialized = false;
   static const uint32_t zero[32] = {0};
   static uint32_t undef[32] = {0};
            if (!initialized)
            if (!memcmp(ctx->pstipple.pattern, ps->stipple, sizeof(ps->stipple)))
            memcpy(ctx->pstipple.pattern, ps->stipple, sizeof(ps->stipple));
   ctx->pstipple.enabled = !!memcmp(ps->stipple, undef, sizeof(ps->stipple)) &&
         if (ctx->pstipple.enabled)
      }
      static void
   d3d12_set_vertex_buffers(struct pipe_context *pctx,
                           {
      struct d3d12_context *ctx = d3d12_context(pctx);
   util_set_vertex_buffers_count(ctx->vbs, &ctx->num_vbs,
                        for (unsigned i = 0; i < ctx->num_vbs; ++i) {
      const struct pipe_vertex_buffer* buf = ctx->vbs + i;
   if (!buf->buffer.resource)
         struct d3d12_resource *res = d3d12_resource(buf->buffer.resource);
   ctx->vbvs[i].BufferLocation = d3d12_resource_gpu_virtual_address(res) + buf->buffer_offset;
      }
      }
      static void
   d3d12_set_viewport_states(struct pipe_context *pctx,
                     {
               for (unsigned i = 0; i < num_viewports; ++i) {
      if (state[i].scale[1] < 0) {
      ctx->flip_y = 1.0f;
   ctx->viewports[start_slot + i].TopLeftY = state[i].translate[1] + state[i].scale[1];
      } else {
      ctx->flip_y = -1.0f;
   ctx->viewports[start_slot + i].TopLeftY = state[i].translate[1] - state[i].scale[1];
      }
   ctx->viewports[start_slot + i].TopLeftX = state[i].translate[0] - state[i].scale[0];
            float near_depth = state[i].translate[2];
            /* When the rasterizer is configured for "full" depth clipping ([-1, 1])
   * the viewport that we get is set to cover the positive half of clip space.
   * E.g. a [0, 1] viewport from the GL API will come to the driver as [0.5, 1].
   * Since we halve clipping space from [-1, 1] to [0, 1], we need to double the
   * viewport, treating translate as the center instead of the near plane. When
   * the rasterizer is configured for "half" depth clipping ([0, 1]), the viewport
   * covers the entire clip range, so no fixup is needed.
   * 
   * Note: If halfz mode changes, both the rasterizer and viewport are dirtied,
   * and on the next draw we will get the rasterizer state first, and viewport
   * second, because ST_NEW_RASTERIZER comes before ST_NEW_VIEWPORT.
   */
   if (ctx->gfx_pipeline_state.rast && !ctx->gfx_pipeline_state.rast->base.clip_halfz) {
                  bool reverse_depth_range = near_depth > far_depth;
   if (reverse_depth_range) {
      float tmp = near_depth;
   near_depth = far_depth;
   far_depth = tmp;
      } else
         ctx->viewports[start_slot + i].MinDepth = near_depth;
   ctx->viewports[start_slot + i].MaxDepth = far_depth;
      }
   ctx->num_viewports = start_slot + num_viewports;
      }
         static void
   d3d12_set_scissor_states(struct pipe_context *pctx,
               {
               for (unsigned i = 0; i < num_scissors; i++) {
      ctx->scissors[start_slot + i].left = states[i].minx;
   ctx->scissors[start_slot + i].top = states[i].miny;
   ctx->scissors[start_slot + i].right = states[i].maxx;
   ctx->scissors[start_slot + i].bottom = states[i].maxy;
      }
      }
      static void
   d3d12_decrement_constant_buffer_bind_count(struct d3d12_context *ctx,
                  assert(res->bind_counts[shader][D3D12_RESOURCE_BINDING_TYPE_CBV] > 0);
      }
      static void
   d3d12_increment_constant_buffer_bind_count(struct d3d12_context *ctx,
                     }
      static void
   d3d12_set_constant_buffer(struct pipe_context *pctx,
                     {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_resource *old_buf = d3d12_resource(ctx->cbufs[shader][index].buffer);
   if (old_buf)
            if (buf) {
      unsigned offset = buf->buffer_offset;
   if (buf->user_buffer) {
      u_upload_data(pctx->const_uploader, 0, buf->buffer_size,
               d3d12_increment_constant_buffer_bind_count(ctx, shader,
      } else {
      struct pipe_resource *buffer = buf->buffer;
                  if (take_ownership) {
      pipe_resource_reference(&ctx->cbufs[shader][index].buffer, NULL);
      } else {
                     ctx->cbufs[shader][index].buffer_offset = offset;
   ctx->cbufs[shader][index].buffer_size = buf->buffer_size;
         } else {
      pipe_resource_reference(&ctx->cbufs[shader][index].buffer, NULL);
   ctx->cbufs[shader][index].buffer_offset = 0;
   ctx->cbufs[shader][index].buffer_size = 0;
      }
      }
      static void
   d3d12_set_framebuffer_state(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
            bool prev_cbufs_or_zsbuf = ctx->fb.nr_cbufs || ctx->fb.zsbuf;
   util_copy_framebuffer_state(&d3d12_context(pctx)->fb, state);
            ctx->gfx_pipeline_state.num_cbufs = state->nr_cbufs;
   ctx->gfx_pipeline_state.has_float_rtv = false;
   for (int i = 0; i < state->nr_cbufs; ++i) {
      if (state->cbufs[i]) {
      if (util_format_is_float(state->cbufs[i]->format))
         ctx->gfx_pipeline_state.rtv_formats[i] = d3d12_get_format(state->cbufs[i]->format);
      } else {
                     if (state->zsbuf) {
      ctx->gfx_pipeline_state.dsv_format = d3d12_get_resource_rt_format(state->zsbuf->format);
      } else
            if (samples < 0)
                     ctx->state_dirty |= D3D12_DIRTY_FRAMEBUFFER;
   if (!prev_cbufs_or_zsbuf || !new_cbufs_or_zsbuf)
      }
      static void
   d3d12_set_blend_color(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
   memcpy(ctx->blend_factor, color->color, sizeof(float) * 4);
      }
      static void
   d3d12_set_sample_mask(struct pipe_context *pctx, unsigned sample_mask)
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
   ctx->gfx_pipeline_state.sample_mask = sample_mask;
      }
      static void
   d3d12_set_stencil_ref(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_screen *screen = d3d12_screen(pctx->screen);
   if ((ref.ref_value[0] != ref.ref_value[1]) &&
      (!screen->opts14.IndependentFrontAndBackStencilRefMaskSupported ||
   ctx->cmdlist8 == nullptr) &&
   (d3d12_debug & D3D12_DEBUG_VERBOSE))
      ctx->stencil_ref = ref;
      }
      static void
   d3d12_set_clip_state(struct pipe_context *pctx,
         {
   }
      static struct pipe_stream_output_target *
   d3d12_create_stream_output_target(struct pipe_context *pctx,
                     {
      struct d3d12_resource *res = d3d12_resource(pres);
            if (!cso)
            pipe_reference_init(&cso->base.reference, 1);
   pipe_resource_reference(&cso->base.buffer, pres);
   cso->base.buffer_offset = buffer_offset;
   cso->base.buffer_size = buffer_size;
            if (res->bo && res->bo->buffer && d3d12_buffer(res->bo->buffer)->map)
      util_range_add(pres, &res->valid_buffer_range, buffer_offset,
            }
      static void
   d3d12_stream_output_target_destroy(struct pipe_context *ctx,
         {
      struct d3d12_stream_output_target *target = (struct d3d12_stream_output_target *)state;
   pipe_resource_reference(&target->base.buffer, NULL);
               }
      static void
   fill_stream_output_buffer_view(D3D12_STREAM_OUTPUT_BUFFER_VIEW *view,
         {
      struct d3d12_resource *res = d3d12_resource(target->base.buffer);
            view->SizeInBytes = target->base.buffer_size;
   view->BufferLocation = d3d12_resource_gpu_virtual_address(res) + target->base.buffer_offset;
      }
      static void
   update_so_fill_buffer_count(struct d3d12_context *ctx,
                     {
      struct pipe_transfer *transfer = NULL;
   uint32_t *ptr = (uint32_t *)pipe_buffer_map_range(&ctx->base, fill_buffer,
         *ptr = value;
      }
      static void
   d3d12_set_stream_output_targets(struct pipe_context *pctx,
                     {
                                 for (unsigned i = 0; i < PIPE_MAX_SO_BUFFERS; i++) {
      struct d3d12_stream_output_target *target =
            if (target) {
      /* Sub-allocate a new fill buffer each time to avoid GPU/CPU synchronization */
   if (offsets[i] != ~0u) {
      u_suballocator_alloc(&ctx->so_allocator, sizeof(uint32_t) * 5, 16,
            }
   fill_stream_output_buffer_view(&ctx->so_buffer_views[i], target);
      } else {
      ctx->so_buffer_views[i].BufferLocation = 0;
   ctx->so_buffer_views[i].BufferFilledSizeLocation = 0;
   ctx->so_buffer_views[i].SizeInBytes = 0;
                  ctx->gfx_pipeline_state.num_so_targets = num_targets;
      }
      static void
   d3d12_decrement_ssbo_bind_count(struct d3d12_context *ctx,
                  assert(res->bind_counts[shader][D3D12_RESOURCE_BINDING_TYPE_SSBO] > 0);
      }
      static void
   d3d12_increment_ssbo_bind_count(struct d3d12_context *ctx,
                     }
      static void
   d3d12_set_shader_buffers(struct pipe_context *pctx,
                           {
      struct d3d12_context *ctx = d3d12_context(pctx);
   for (unsigned i = 0; i < count; ++i) {
      struct pipe_shader_buffer *slot = &ctx->ssbo_views[shader][i + start_slot];
   if (slot->buffer) {
      d3d12_decrement_ssbo_bind_count(ctx, shader, d3d12_resource(slot->buffer));
               if (buffers && buffers[i].buffer) {
      pipe_resource_reference(&slot->buffer, buffers[i].buffer);
   slot->buffer_offset = buffers[i].buffer_offset;
   slot->buffer_size = buffers[i].buffer_size;
      } else
               if (buffers) {
         } else {
      ctx->num_ssbo_views[shader] = 0;
   for (int i = start_slot + count - 1; i >= (int)start_slot; --i) {
      if (ctx->ssbo_views[shader][i].buffer) {
      ctx->num_ssbo_views[shader] = i;
            }
      }
      static void
   d3d12_decrement_image_bind_count(struct d3d12_context *ctx,
                  assert(res->bind_counts[shader][D3D12_RESOURCE_BINDING_TYPE_IMAGE] > 0);
      }
      static void
   d3d12_increment_image_bind_count(struct d3d12_context *ctx,
                     }
      static bool
   is_valid_uav_cast(enum pipe_format resource_format, enum pipe_format view_format)
   {
      if (view_format != PIPE_FORMAT_R32_UINT &&
      view_format != PIPE_FORMAT_R32_SINT &&
   view_format != PIPE_FORMAT_R32_FLOAT)
      switch (d3d12_get_typeless_format(resource_format)) {
   case DXGI_FORMAT_R8G8B8A8_TYPELESS:
   case DXGI_FORMAT_B8G8R8A8_TYPELESS:
   case DXGI_FORMAT_B8G8R8X8_TYPELESS:
   case DXGI_FORMAT_R16G16_TYPELESS:
   case DXGI_FORMAT_R10G10B10A2_TYPELESS:
         default:
            }
      static enum pipe_format
   get_shader_image_emulation_format(enum pipe_format resource_format)
   {
   #define CASE(f) case DXGI_FORMAT_##f##_TYPELESS: return PIPE_FORMAT_##f##_UINT
      switch (d3d12_get_typeless_format(resource_format)) {
      CASE(R8);
   CASE(R8G8);
   CASE(R8G8B8A8);
   CASE(R16);
   CASE(R16G16);
   CASE(R16G16B16A16);
   CASE(R32);
   CASE(R32G32);
   CASE(R32G32B32A32);
      case DXGI_FORMAT_R11G11B10_FLOAT:
         default:
            }
      static void
   d3d12_set_shader_images(struct pipe_context *pctx,
                           {
      struct d3d12_context *ctx = d3d12_context(pctx);
   for (unsigned i = 0; i < count + unbind_num_trailing_slots; ++i) {
      struct pipe_image_view *slot = &ctx->image_views[shader][i + start_slot];
   if (slot->resource) {
      d3d12_decrement_image_bind_count(ctx, shader, d3d12_resource(slot->resource));
               ctx->image_view_emulation_formats[shader][i] = PIPE_FORMAT_NONE;
   if (i < count && images && images[i].resource) {
      pipe_resource_reference(&slot->resource, images[i].resource);
                  if (images[i].resource->target != PIPE_BUFFER &&
      !is_valid_uav_cast(images[i].resource->format, images[i].format) &&
   d3d12_get_typeless_format(images[i].format) !=
   d3d12_get_typeless_format(images[i].resource->format)) {
   /* Can't use D3D casting, have to use shader lowering instead */
   ctx->image_view_emulation_formats[shader][i] =
         } else
               if (images) {
         } else {
      ctx->num_image_views[shader] = 0;
   for (int i = start_slot + count - 1; i >= (int)start_slot; --i) {
      if (ctx->image_views[shader][i].resource) {
      ctx->num_image_views[shader] = i;
            }
      }
      void
   d3d12_invalidate_context_bindings(struct d3d12_context *ctx, struct d3d12_resource *res) {
      // For each shader type, if the resource is currently bound as CBV, SRV, or UAV
   // set the context shader_dirty bit.
   for (uint i = 0; i < PIPE_SHADER_TYPES; ++i) {
      if (res->bind_counts[i][D3D12_RESOURCE_BINDING_TYPE_CBV] > 0) {
                  if (res->bind_counts[i][D3D12_RESOURCE_BINDING_TYPE_SRV] > 0) {
                  if (res->bind_counts[i][D3D12_RESOURCE_BINDING_TYPE_SSBO] > 0) {
                  if (res->bind_counts[i][D3D12_RESOURCE_BINDING_TYPE_IMAGE] > 0) {
               }
      bool
   d3d12_enable_fake_so_buffers(struct d3d12_context *ctx, unsigned factor)
   {
      if (ctx->fake_so_buffer_factor == factor)
                     for (unsigned i = 0; i < ctx->gfx_pipeline_state.num_so_targets; ++i) {
      struct d3d12_stream_output_target *target = (struct d3d12_stream_output_target *)ctx->so_targets[i];
            fake_target = CALLOC_STRUCT(d3d12_stream_output_target);
   if (!fake_target)
         pipe_reference_init(&fake_target->base.reference, 1);
                     /* Check if another target is using the same buffer */
   for (unsigned j = 0; j < i; ++j) {
      if (ctx->so_targets[j] && ctx->so_targets[j]->buffer == target->base.buffer) {
      struct d3d12_stream_output_target *prev_target =
         pipe_resource_reference(&fake_target->base.buffer, prev_target->base.buffer);
   pipe_resource_reference(&fake_target->fill_buffer, prev_target->fill_buffer);
   fake_target->fill_buffer_offset = prev_target->fill_buffer_offset;
                  /* Create new SO buffer 6x (2 triangles instead of 1 point) the original size if not */
   if (!fake_target->base.buffer) {
      fake_target->base.buffer = pipe_buffer_create(ctx->base.screen,
                     u_suballocator_alloc(&ctx->so_allocator, sizeof(uint32_t) * 5, 256,
                     fake_target->base.buffer_offset = target->base.buffer_offset * factor;
   /* TODO: This will mess with SO statistics/overflow queries, but we're already missing things there */
   fake_target->base.buffer_size = target->base.buffer_size * factor;
   ctx->fake_so_targets[i] = &fake_target->base;
               ctx->fake_so_buffer_factor = factor;
               }
      bool
   d3d12_disable_fake_so_buffers(struct d3d12_context *ctx)
   {
      if (ctx->fake_so_buffer_factor == 0)
                     bool cs_state_saved = false;
            for (unsigned i = 0; i < ctx->gfx_pipeline_state.num_so_targets; ++i) {
      struct d3d12_stream_output_target *target = (struct d3d12_stream_output_target *)ctx->so_targets[i];
   struct d3d12_stream_output_target *fake_target = (struct d3d12_stream_output_target *)ctx->fake_so_targets[i];
      if (fake_target == NULL)
            if (!cs_state_saved) {
      cs_state_saved = true;
               d3d12_compute_transform_key key;
   memset(&key, 0, sizeof(key));
   key.type = d3d12_compute_transform_type::fake_so_buffer_vertex_count;
            ctx->transform_state_vars[0] = ctx->gfx_pipeline_state.so_info.stride[i];
            pipe_shader_buffer new_cs_ssbos[3];
   new_cs_ssbos[0].buffer = fake_target->fill_buffer;
   new_cs_ssbos[0].buffer_offset = fake_target->fill_buffer_offset;
            new_cs_ssbos[1].buffer = target->fill_buffer;
   new_cs_ssbos[1].buffer_offset = target->fill_buffer_offset;
   new_cs_ssbos[1].buffer_size = target->fill_buffer->width0 - target->fill_buffer_offset;
            pipe_grid_info grid = {};
   grid.block[0] = grid.block[1] = grid.block[2] = 1;
   grid.grid[0] = grid.grid[1] = grid.grid[2] = 1;
            key.type = d3d12_compute_transform_type::fake_so_buffer_copy_back;
   key.fake_so_buffer_copy_back.stride = ctx->gfx_pipeline_state.so_info.stride[i];
   for (unsigned j = 0; j < ctx->gfx_pipeline_state.so_info.num_outputs; ++j) {
      auto& output = ctx->gfx_pipeline_state.so_info.output[j];
                  if (key.fake_so_buffer_copy_back.num_ranges > 0) {
      auto& last_range = key.fake_so_buffer_copy_back.ranges[key.fake_so_buffer_copy_back.num_ranges - 1];
   if (output.dst_offset * 4 == last_range.offset + last_range.size) {
      last_range.size += output.num_components * 4;
                  auto& new_range = key.fake_so_buffer_copy_back.ranges[key.fake_so_buffer_copy_back.num_ranges++];
   new_range.offset = output.dst_offset * 4;
      }
                     new_cs_ssbos[0].buffer = target->base.buffer;
   new_cs_ssbos[0].buffer_offset = target->base.buffer_offset;
   new_cs_ssbos[0].buffer_size = target->base.buffer_size;
   new_cs_ssbos[1].buffer = fake_target->base.buffer;
   new_cs_ssbos[1].buffer_offset = fake_target->base.buffer_offset;
   new_cs_ssbos[1].buffer_size = fake_target->base.buffer_size;
            pipe_constant_buffer cbuf = {};
   cbuf.buffer = fake_target->fill_buffer;
   cbuf.buffer_offset = fake_target->fill_buffer_offset;
   cbuf.buffer_size = fake_target->fill_buffer->width0 - cbuf.buffer_offset;
            grid.indirect = fake_target->fill_buffer;
   grid.indirect_offset = fake_target->fill_buffer_offset + 4;
            pipe_so_target_reference(&ctx->fake_so_targets[i], NULL);
            /* Make sure the buffer is not copied twice */
   for (unsigned j = i + 1; j <= ctx->gfx_pipeline_state.num_so_targets; ++j) {
      if (ctx->so_targets[j] && ctx->so_targets[j]->buffer == target->base.buffer)
                  ctx->fake_so_buffer_factor = 0;
            if (cs_state_saved)
               }
      void
   d3d12_flush_cmdlist(struct d3d12_context *ctx)
   {
               ctx->current_batch_idx++;
   if (ctx->current_batch_idx == ARRAY_SIZE(ctx->batches))
               }
      void
   d3d12_flush_cmdlist_and_wait(struct d3d12_context *ctx)
   {
               d3d12_foreach_submitted_batch(ctx, old_batch)
         d3d12_flush_cmdlist(ctx);
      }
      static void
   d3d12_clear_render_target(struct pipe_context *pctx,
                           struct pipe_surface *psurf,
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
            if (!render_condition_enabled && ctx->current_predication)
            struct d3d12_resource *res = d3d12_resource(psurf->texture);
   d3d12_transition_resource_state(ctx, res,
                        enum pipe_format format = psurf->texture->format;
            if (util_format_is_pure_uint(format)) {
      for (int c = 0; c < 4; ++c)
      } else if (util_format_is_pure_sint(format)) {
      for (int c = 0; c < 4; ++c)
      } else {
      for (int c = 0; c < 4; ++c)
               if (!(util_format_colormask(util_format_description(psurf->texture->format)) &
      PIPE_MASK_A))
         D3D12_RECT rect = { (int)dstx, (int)dsty,
               ctx->cmdlist->ClearRenderTargetView(surf->desc_handle.cpu_handle,
                     if (!render_condition_enabled && ctx->current_predication) {
            }
      static void
   d3d12_clear_depth_stencil(struct pipe_context *pctx,
                           struct pipe_surface *psurf,
   unsigned clear_flags,
   double depth,
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
            if (!render_condition_enabled && ctx->current_predication)
            D3D12_CLEAR_FLAGS flags = (D3D12_CLEAR_FLAGS)0;
   if (clear_flags & PIPE_CLEAR_DEPTH)
         if (clear_flags & PIPE_CLEAR_STENCIL)
            struct d3d12_resource *res = d3d12_resource(psurf->texture);
   d3d12_transition_resource_state(ctx, res,
                        D3D12_RECT rect = { (int)dstx, (int)dsty,
               ctx->cmdlist->ClearDepthStencilView(surf->desc_handle.cpu_handle, flags,
                     if (!render_condition_enabled && ctx->current_predication) {
            }
      static void
   d3d12_clear(struct pipe_context *pctx,
               unsigned buffers,
   const struct pipe_scissor_state *scissor_state,
   {
               if (buffers & PIPE_CLEAR_COLOR) {
      for (int i = 0; i < ctx->fb.nr_cbufs; ++i) {
      if (buffers & (PIPE_CLEAR_COLOR0 << i)) {
      struct pipe_surface *psurf = ctx->fb.cbufs[i];
   d3d12_clear_render_target(pctx, psurf, color,
                           if (buffers & PIPE_CLEAR_DEPTHSTENCIL && ctx->fb.zsbuf) {
      struct pipe_surface *psurf = ctx->fb.zsbuf;
   d3d12_clear_depth_stencil(pctx, psurf,
                           }
      static void
   d3d12_flush(struct pipe_context *pipe,
               {
      struct d3d12_context *ctx = d3d12_context(pipe);
                     if (fence)
      }
      static void
   d3d12_flush_resource(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
            d3d12_transition_resource_state(ctx, res,
                  }
      static void
   d3d12_signal(struct pipe_context *pipe,
         {
      struct d3d12_screen *screen = d3d12_screen(pipe->screen);
   struct d3d12_fence *fence = d3d12_fence(pfence);
   d3d12_flush_cmdlist(d3d12_context(pipe));
      }
      static void
   d3d12_wait(struct pipe_context *pipe, struct pipe_fence_handle *pfence)
   {
      struct d3d12_screen *screen = d3d12_screen(pipe->screen);
   struct d3d12_fence *fence = d3d12_fence(pfence);
   d3d12_flush_cmdlist(d3d12_context(pipe));
      }
      static void
   d3d12_init_null_sampler(struct d3d12_context *ctx)
   {
                        D3D12_SAMPLER_DESC desc;
   desc.Filter = D3D12_FILTER_ANISOTROPIC;
   desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
   desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
   desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
   desc.MipLODBias = 0.0f;
   desc.MaxAnisotropy = 0;
   desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
   desc.MinLOD = 0.0f;
   desc.MaxLOD = 0.0f;
   memset(desc.BorderColor, 0, sizeof(desc.BorderColor));
      }
      static uint64_t
   d3d12_get_timestamp(struct pipe_context *pctx)
   {
               if (!ctx->timestamp_query)
            pipe_query_result result;
   pctx->end_query(pctx, ctx->timestamp_query);
   pctx->get_query_result(pctx, ctx->timestamp_query, true, &result);
      }
      static void
   d3d12_rebind_buffer(struct d3d12_context *ctx, struct d3d12_resource *res)
   {
      if (res->base.b.bind & PIPE_BIND_VERTEX_BUFFER) {
      for (unsigned i = 0; i < ctx->num_vbs; ++i) {
               if (!buf->is_user_buffer && &res->base.b == buf->buffer.resource) {
      ctx->vbvs[i].BufferLocation = d3d12_resource_gpu_virtual_address(res) + buf->buffer_offset;
                     if (res->base.b.bind & PIPE_BIND_STREAM_OUTPUT) {
      for (unsigned i = 0; i < ctx->gfx_pipeline_state.num_so_targets; ++i) {
      struct d3d12_stream_output_target *target = (struct d3d12_stream_output_target *)ctx->so_targets[i];
   assert(!target || target->fill_buffer != &res->base.b);
   if (target && target->base.buffer == &res->base.b) {
      fill_stream_output_buffer_view(&ctx->so_buffer_views[i], target);
                                 }
      static void
   d3d12_replace_buffer_storage(struct pipe_context *pctx,
      struct pipe_resource *pdst,
   struct pipe_resource *psrc,
   unsigned minimum_num_rebinds,
   uint32_t rebind_mask,
      {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_resource *dst = d3d12_resource(pdst);
            struct d3d12_bo *old_bo = dst->bo;
   d3d12_bo_reference(src->bo);
   dst->bo = src->bo;
   p_atomic_inc(&dst->generation_id);
   d3d12_rebind_buffer(ctx, dst);
      }
      static void
   d3d12_memory_barrier(struct pipe_context *pctx, unsigned flags)
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
   if (flags & PIPE_BARRIER_VERTEX_BUFFER)
         if (flags & PIPE_BARRIER_INDEX_BUFFER)
         if (flags & PIPE_BARRIER_FRAMEBUFFER)
         if (flags & PIPE_BARRIER_STREAMOUT_BUFFER)
            /* TODO:
   * PIPE_BARRIER_INDIRECT_BUFFER
            for (unsigned i = 0; i < D3D12_GFX_SHADER_STAGES; ++i) {
      if (flags & PIPE_BARRIER_CONSTANT_BUFFER)
         if (flags & PIPE_BARRIER_TEXTURE)
         if (flags & PIPE_BARRIER_SHADER_BUFFER)
         if (flags & PIPE_BARRIER_IMAGE)
      }
      /* Indicate that UAVs shouldn't override transitions. Ignore barriers that are only
   * for UAVs or other fixed-function state that doesn't need a draw to resolve.
   */
   const unsigned ignored_barrier_flags =
      PIPE_BARRIER_IMAGE |
   PIPE_BARRIER_SHADER_BUFFER |
   PIPE_BARRIER_UPDATE |
   PIPE_BARRIER_MAPPED_BUFFER |
               if (flags & (PIPE_BARRIER_IMAGE | PIPE_BARRIER_SHADER_BUFFER)) {
      D3D12_RESOURCE_BARRIER uavBarrier;
   uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
   uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   uavBarrier.UAV.pResource = nullptr;
         }
      static void
   d3d12_set_patch_vertices(struct pipe_context *pctx, uint8_t patch_vertices)
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
   ctx->patch_vertices = patch_vertices;
      }
      static void
   d3d12_set_tess_state(struct pipe_context *pctx,
               {
      struct d3d12_context *ctx = d3d12_context(pctx);
   memcpy(ctx->default_outer_tess_factor, default_outer_level, sizeof(ctx->default_outer_tess_factor));
      }
      static enum pipe_reset_status
   d3d12_get_reset_status(struct pipe_context *pctx)
   {
      struct d3d12_screen *screen = d3d12_screen(pctx->screen);
   HRESULT hr = screen->dev->GetDeviceRemovedReason();
   switch (hr) {
   case DXGI_ERROR_DEVICE_HUNG:
   case DXGI_ERROR_INVALID_CALL:
         case DXGI_ERROR_DEVICE_RESET:
         default:
            }
      #ifdef HAVE_GALLIUM_D3D12_VIDEO
   struct pipe_video_codec*
   d3d12_video_create_codec(struct pipe_context *context,
         {
      if (templat->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
         } else if (templat->entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
         } else if (templat->entrypoint == PIPE_VIDEO_ENTRYPOINT_PROCESSING) {
         } else {
      debug_printf("D3D12: Unsupported video codec entrypoint %d\n", templat->entrypoint);
         }
   #endif
      struct pipe_context *
   d3d12_context_create(struct pipe_screen *pscreen, void *priv, unsigned flags)
   {
      struct d3d12_screen *screen = d3d12_screen(pscreen);
   if (FAILED(screen->dev->GetDeviceRemovedReason())) {
      /* Attempt recovery, but this may fail */
   screen->deinit(screen);
   if (!screen->init(screen)) {
      debug_printf("D3D12: failed to reset screen\n");
                  struct d3d12_context *ctx = CALLOC_STRUCT(d3d12_context);
   if (!ctx)
            ctx->base.screen = pscreen;
                     ctx->base.create_vertex_elements_state = d3d12_create_vertex_elements_state;
   ctx->base.bind_vertex_elements_state = d3d12_bind_vertex_elements_state;
            ctx->base.create_blend_state = d3d12_create_blend_state;
   ctx->base.bind_blend_state = d3d12_bind_blend_state;
            ctx->base.create_depth_stencil_alpha_state = d3d12_create_depth_stencil_alpha_state;
   ctx->base.bind_depth_stencil_alpha_state = d3d12_bind_depth_stencil_alpha_state;
            ctx->base.create_rasterizer_state = d3d12_create_rasterizer_state;
   ctx->base.bind_rasterizer_state = d3d12_bind_rasterizer_state;
            ctx->base.create_sampler_state = d3d12_create_sampler_state;
   ctx->base.bind_sampler_states = d3d12_bind_sampler_states;
            ctx->base.create_sampler_view = d3d12_create_sampler_view;
   ctx->base.set_sampler_views = d3d12_set_sampler_views;
            ctx->base.create_vs_state = d3d12_create_vs_state;
   ctx->base.bind_vs_state = d3d12_bind_vs_state;
            ctx->base.create_fs_state = d3d12_create_fs_state;
   ctx->base.bind_fs_state = d3d12_bind_fs_state;
            ctx->base.create_gs_state = d3d12_create_gs_state;
   ctx->base.bind_gs_state = d3d12_bind_gs_state;
            ctx->base.create_tcs_state = d3d12_create_tcs_state;
   ctx->base.bind_tcs_state = d3d12_bind_tcs_state;
            ctx->base.create_tes_state = d3d12_create_tes_state;
   ctx->base.bind_tes_state = d3d12_bind_tes_state;
            ctx->base.set_patch_vertices = d3d12_set_patch_vertices;
            ctx->base.create_compute_state = d3d12_create_compute_state;
   ctx->base.bind_compute_state = d3d12_bind_compute_state;
            ctx->base.set_polygon_stipple = d3d12_set_polygon_stipple;
   ctx->base.set_vertex_buffers = d3d12_set_vertex_buffers;
   ctx->base.set_viewport_states = d3d12_set_viewport_states;
   ctx->base.set_scissor_states = d3d12_set_scissor_states;
   ctx->base.set_constant_buffer = d3d12_set_constant_buffer;
   ctx->base.set_framebuffer_state = d3d12_set_framebuffer_state;
   ctx->base.set_clip_state = d3d12_set_clip_state;
   ctx->base.set_blend_color = d3d12_set_blend_color;
   ctx->base.set_sample_mask = d3d12_set_sample_mask;
            ctx->base.create_stream_output_target = d3d12_create_stream_output_target;
   ctx->base.stream_output_target_destroy = d3d12_stream_output_target_destroy;
            ctx->base.set_shader_buffers = d3d12_set_shader_buffers;
                     ctx->base.clear = d3d12_clear;
   ctx->base.clear_render_target = d3d12_clear_render_target;
   ctx->base.clear_depth_stencil = d3d12_clear_depth_stencil;
   ctx->base.draw_vbo = d3d12_draw_vbo;
   ctx->base.launch_grid = d3d12_launch_grid;
   ctx->base.flush = d3d12_flush;
            ctx->base.fence_server_signal = d3d12_signal;
                                                ctx->has_flat_varyings = false;
   ctx->missing_dual_src_outputs = false;
            d3d12_context_surface_init(&ctx->base);
   d3d12_context_resource_init(&ctx->base);
   d3d12_context_query_init(&ctx->base);
         #ifdef HAVE_GALLIUM_D3D12_VIDEO
      // Add d3d12 video functions entrypoints
   ctx->base.create_video_codec = d3d12_video_create_codec;
   ctx->base.create_video_buffer = d3d12_video_buffer_create;
      #endif
      slab_create_child(&ctx->transfer_pool, &d3d12_screen(pscreen)->transfer_pool);
            ctx->base.stream_uploader = u_upload_create_default(&ctx->base);
   ctx->base.const_uploader = u_upload_create_default(&ctx->base);
   u_suballocator_init(&ctx->so_allocator, &ctx->base, 4096, 0,
                  struct primconvert_config cfg = {};
   cfg.primtypes_mask = 1 << MESA_PRIM_POINTS |
                           cfg.restart_primtypes_mask = cfg.primtypes_mask;
   cfg.fixed_prim_restart = true;
   ctx->primconvert = util_primconvert_create_config(&ctx->base, &cfg);
   if (!ctx->primconvert) {
      debug_printf("D3D12: failed to create primconvert\n");
               d3d12_gfx_pipeline_state_cache_init(ctx);
   d3d12_compute_pipeline_state_cache_init(ctx);
   d3d12_root_signature_cache_init(ctx);
   d3d12_cmd_signature_cache_init(ctx);
   d3d12_gs_variant_cache_init(ctx);
   d3d12_tcs_variant_cache_init(ctx);
   d3d12_compute_transform_cache_init(ctx);
            ctx->D3D12SerializeVersionedRootSignature =
      #ifndef _GAMING_XBOX
         #endif
                  for (unsigned i = 0; i < ARRAY_SIZE(ctx->batches); ++i) {
      if (!d3d12_init_batch(ctx, &ctx->batches[i])) {
      FREE(ctx);
         }
            ctx->sampler_pool = d3d12_descriptor_pool_new(screen,
               if (!ctx->sampler_pool) {
      FREE(ctx);
      }
         #ifdef _WIN32
      if (!(d3d12_debug & D3D12_DEBUG_EXPERIMENTAL) ||
      (d3d12_debug & D3D12_DEBUG_DISASS))
   #endif
         ctx->blitter = util_blitter_create(&ctx->base);
   if (!ctx->blitter)
            if (!d3d12_init_polygon_stipple(&ctx->base)) {
      debug_printf("D3D12: failed to initialize polygon stipple resources\n");
   FREE(ctx);
               mtx_lock(&screen->submit_mutex);
   list_addtail(&ctx->context_list_entry, &screen->context_list);
   if (screen->context_id_count > 0)
         else
                  for (unsigned i = 0; i < ARRAY_SIZE(ctx->batches); ++i) {
      ctx->batches[i].ctx_id = ctx->id;
               if (flags & PIPE_CONTEXT_PREFER_THREADED)
      return threaded_context_create(&ctx->base,
      &screen->transfer_pool,
   d3d12_replace_buffer_storage,
               }
      bool
   d3d12_need_zero_one_depth_range(struct d3d12_context *ctx)
   {
               /**
   * OpenGL Compatibility spec, section 15.2.3 (Shader Outputs) says
   * the following:
   *
   *    For fixed-point depth buffers, the final fragment depth written by
   *    a fragment shader is first clamped to [0, 1] and then converted to
   *    fixed-point as if it were a window z value (see section 13.8.1).
   *    For floating-point depth buffers, conversion is not performed but
   *    clamping is. Note that the depth range computation is not applied
   *    here, only the conversion to fixed-point.
   *
   * However, the D3D11.3 Functional Spec, section 17.10 (Depth Clamp) says
   * the following:
   *
   *    Depth values that reach the Output Merger, whether coming from
   *    interpolation or from Pixel Shader output (replacing the
   *    interpolated z), are always clamped:
   *    z = min(Viewport.MaxDepth,max(Viewport.MinDepth,z))
   *    following the D3D11 Floating Point Rules(3.1) for min/max.
   *
   * This means that we can't always use the fixed-function viewport-mapping
   * D3D provides.
   *
   * There's only one case where the difference matters: When the fragment
   * shader writes a non-implicit value to gl_FragDepth. In all other
   * cases, the fragment either shouldn't have been rasterized in the
   * first place, or the implicit gl_FragCoord.z-value should already have
   * been clamped to the depth-range.
   *
   * For simplicity, let's assume that an explicitly written frag-result
   * doesn't simply forward the value of gl_FragCoord.z. If it does, we'll
   * end up generating needless code, but the result will be correct.
               }
