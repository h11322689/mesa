   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
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
      /**
   * @file
   * C - JIT interfaces
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
      #include <llvm/Config/llvm-config.h>
      #include "util/u_memory.h"
   #include "gallivm/lp_bld_init.h"
   #include "gallivm/lp_bld_debug.h"
   #include "gallivm/lp_bld_format.h"
   #include "lp_context.h"
   #include "lp_debug.h"
   #include "lp_memory.h"
   #include "lp_screen.h"
   #include "lp_jit.h"
      static void
   lp_jit_create_types(struct lp_fragment_shader_variant *lp)
   {
      struct gallivm_state *gallivm = lp->gallivm;
   LLVMContextRef lc = gallivm->context;
   LLVMTypeRef viewport_type;
            /* struct lp_jit_viewport */
   {
               elem_types[LP_JIT_VIEWPORT_MIN_DEPTH] =
            viewport_type = LLVMStructTypeInContext(lc, elem_types,
            LP_CHECK_MEMBER_OFFSET(struct lp_jit_viewport, min_depth,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_viewport, max_depth,
               LP_CHECK_STRUCT_SIZE(struct lp_jit_viewport,
                  /* struct lp_jit_context */
   {
      LLVMTypeRef elem_types[LP_JIT_CTX_COUNT];
            elem_types[LP_JIT_CTX_ALPHA_REF] = LLVMFloatTypeInContext(lc);
   elem_types[LP_JIT_CTX_SAMPLE_MASK] =
   elem_types[LP_JIT_CTX_STENCIL_REF_FRONT] =
   elem_types[LP_JIT_CTX_STENCIL_REF_BACK] = LLVMInt32TypeInContext(lc);
   elem_types[LP_JIT_CTX_U8_BLEND_COLOR] = LLVMPointerType(LLVMInt8TypeInContext(lc), 0);
   elem_types[LP_JIT_CTX_F_BLEND_COLOR] = LLVMPointerType(LLVMFloatTypeInContext(lc), 0);
            context_type = LLVMStructTypeInContext(lc, elem_types,
            LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, alpha_ref_value,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, stencil_ref_front,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, stencil_ref_back,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, u8_blend_color,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, f_blend_color,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, viewports,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_context, sample_mask,
               LP_CHECK_STRUCT_SIZE(struct lp_jit_context,
            lp->jit_context_type = context_type;
   lp->jit_context_ptr_type = LLVMPointerType(context_type, 0);
   lp->jit_resources_type = lp_build_jit_resources_type(gallivm);
               /* struct lp_jit_thread_data */
   {
      LLVMTypeRef elem_types[LP_JIT_THREAD_DATA_COUNT];
            elem_types[LP_JIT_THREAD_DATA_CACHE] =
         elem_types[LP_JIT_THREAD_DATA_VIS_COUNTER] = LLVMInt64TypeInContext(lc);
   elem_types[LP_JIT_THREAD_DATA_PS_INVOCATIONS] = LLVMInt64TypeInContext(lc);
   elem_types[LP_JIT_THREAD_DATA_RASTER_STATE_VIEWPORT_INDEX] =
   elem_types[LP_JIT_THREAD_DATA_RASTER_STATE_VIEW_INDEX] =
            thread_data_type = LLVMStructTypeInContext(lc, elem_types,
            lp->jit_thread_data_type = thread_data_type;
               /*
   * lp_linear_elem
   *
   * XXX: it can be instanced only once due to the use of opaque types, and
   * the fact that screen->module is also a global.
   */
   {
      LLVMTypeRef ret_type;
   LLVMTypeRef arg_types[1];
                              /* lp_linear_func */
            /*
   * We actually define lp_linear_elem not as a structure but simply as a
   * lp_linear_func pointer
   */
   lp->jit_linear_func_type = func_type;
               /* struct lp_jit_linear_context */
   {
      LLVMTypeRef linear_elem_ptr_type = LLVMPointerType(linear_elem_type, 0);
   LLVMTypeRef elem_types[LP_JIT_LINEAR_CTX_COUNT];
               elem_types[LP_JIT_LINEAR_CTX_CONSTANTS] = LLVMPointerType(LLVMInt8TypeInContext(lc), 0);
   elem_types[LP_JIT_LINEAR_CTX_TEX] =
   lp->jit_linear_textures_type =
            elem_types[LP_JIT_LINEAR_CTX_INPUTS] =
   lp->jit_linear_inputs_type =
         elem_types[LP_JIT_LINEAR_CTX_COLOR0] = LLVMPointerType(LLVMInt8TypeInContext(lc), 0);
   elem_types[LP_JIT_LINEAR_CTX_BLEND_COLOR] = LLVMInt32TypeInContext(lc);
            linear_context_type = LLVMStructTypeInContext(lc, elem_types,
            LP_CHECK_MEMBER_OFFSET(struct lp_jit_linear_context, constants,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_linear_context, tex,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_linear_context, inputs,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_linear_context, color0,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_linear_context, blend_color,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_linear_context, alpha_ref_value,
               LP_CHECK_STRUCT_SIZE(struct lp_jit_linear_context,
            lp->jit_linear_context_type = linear_context_type;
               if (gallivm_debug & GALLIVM_DEBUG_IR) {
      char *str = LLVMPrintModuleToString(gallivm->module);
   fprintf(stderr, "%s", str);
         }
         void
   lp_jit_screen_cleanup(struct llvmpipe_screen *screen)
   {
         }
         bool
   lp_jit_screen_init(struct llvmpipe_screen *screen)
   {
         }
         void
   lp_jit_init_types(struct lp_fragment_shader_variant *lp)
   {
      if (!lp->jit_context_ptr_type)
      }
      static void
   lp_jit_create_cs_types(struct lp_compute_shader_variant *lp)
   {
      struct gallivm_state *gallivm = lp->gallivm;
            /* struct lp_jit_cs_thread_data */
   {
      LLVMTypeRef elem_types[LP_JIT_CS_THREAD_DATA_COUNT];
            elem_types[LP_JIT_CS_THREAD_DATA_CACHE] =
                     elem_types[LP_JIT_CS_THREAD_DATA_PAYLOAD] = LLVMPointerType(LLVMInt8TypeInContext(lc), 0);
   thread_data_type = LLVMStructTypeInContext(lc, elem_types,
            lp->jit_cs_thread_data_type = thread_data_type;
               /* struct lp_jit_cs_context */
   {
      LLVMTypeRef elem_types[LP_JIT_CS_CTX_COUNT];
            elem_types[LP_JIT_CS_CTX_KERNEL_ARGS] = LLVMPointerType(LLVMInt8TypeInContext(lc), 0);
            cs_context_type = LLVMStructTypeInContext(lc, elem_types,
            LP_CHECK_MEMBER_OFFSET(struct lp_jit_cs_context, kernel_args,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_cs_context, shared_size,
               LP_CHECK_STRUCT_SIZE(struct lp_jit_cs_context,
            lp->jit_cs_context_type = cs_context_type;
   lp->jit_cs_context_ptr_type = LLVMPointerType(cs_context_type, 0);
   lp->jit_resources_type = lp_build_jit_resources_type(gallivm);
               if (gallivm_debug & GALLIVM_DEBUG_IR) {
      char *str = LLVMPrintModuleToString(gallivm->module);
   fprintf(stderr, "%s", str);
         }
      void
   lp_jit_init_cs_types(struct lp_compute_shader_variant *lp)
   {
      if (!lp->jit_cs_context_ptr_type)
      }
      void
   lp_jit_buffer_from_pipe(struct lp_jit_buffer *jit, const struct pipe_shader_buffer *buffer)
   {
               /* resource buffer */
   if (buffer->buffer)
            if (current_data) {
      current_data += buffer->buffer_offset;
   jit->u = (const uint32_t *)current_data;
      } else {
      jit->u = NULL;
         }
      void
   lp_jit_buffer_from_bda(struct lp_jit_buffer *jit, void *mem, size_t size)
   {
               if (current_data) {
      jit->u = (const uint32_t *)current_data;
      } else {
      jit->u = NULL;
         }
      void
   lp_jit_buffer_from_pipe_const(struct lp_jit_buffer *jit, const struct pipe_constant_buffer *buffer, struct pipe_screen *screen)
   {
               const uint8_t *current_data = NULL;
   if (buffer->buffer) {
      /* resource buffer */
      } else if (buffer->user_buffer) {
      /* user-space buffer */
               if (current_data && current_size >= sizeof(float)) {
      current_data += buffer->buffer_offset;
   jit->f = (const float *)current_data;
      } else {
      static const float fake_const_buf[4];
   jit->f = fake_const_buf;
         }
      void
   lp_jit_texture_from_pipe(struct lp_jit_texture *jit, const struct pipe_sampler_view *view)
   {
      struct pipe_resource *res = view->texture;
            if (!lp_tex->dt) {
      /* regular texture - setup array of mipmap level offsets */
   unsigned first_level = 0;
            if (llvmpipe_resource_is_texture(res)) {
      first_level = view->u.tex.first_level;
   last_level = view->u.tex.last_level;
   assert(first_level <= last_level);
   assert(last_level <= res->last_level);
      } else {
                  if (LP_PERF & PERF_TEX_MEM) {
      /* use dummy tile memory */
   jit->base = lp_dummy_tile;
   jit->width = TILE_SIZE/8;
   jit->height = TILE_SIZE/8;
   jit->depth = 1;
   jit->first_level = 0;
   jit->last_level = 0;
   jit->mip_offsets[0] = 0;
   jit->row_stride[0] = 0;
   jit->img_stride[0] = 0;
   jit->num_samples = 0;
      } else {
      jit->width = res->width0;
   jit->height = res->height0;
   jit->depth = res->depth0;
   jit->first_level = first_level;
   jit->last_level = last_level;
                  if (llvmpipe_resource_is_texture(res)) {
      for (unsigned j = first_level; j <= last_level; j++) {
      jit->mip_offsets[j] = lp_tex->mip_offsets[j];
                              if (res->target == PIPE_TEXTURE_1D_ARRAY ||
      res->target == PIPE_TEXTURE_2D_ARRAY ||
   res->target == PIPE_TEXTURE_CUBE ||
   res->target == PIPE_TEXTURE_CUBE_ARRAY ||
   (res->target == PIPE_TEXTURE_3D && view->target == PIPE_TEXTURE_2D)) {
   /*
   * For array textures, we don't have first_layer, instead
   * adjust last_layer (stored as depth) plus the mip level
   * offsets (as we have mip-first layout can't just adjust
   * base ptr).  XXX For mip levels, could do something
   * similar.
   */
   jit->depth = view->u.tex.last_layer - view->u.tex.first_layer + 1;
   for (unsigned j = first_level; j <= last_level; j++) {
      jit->mip_offsets[j] += view->u.tex.first_layer *
      }
   if (view->target == PIPE_TEXTURE_CUBE ||
      view->target == PIPE_TEXTURE_CUBE_ARRAY) {
      }
   assert(view->u.tex.first_layer <= view->u.tex.last_layer);
   if (res->target == PIPE_TEXTURE_3D)
         else
         } else {
      /*
   * For tex2d_from_buf, adjust width and height with application
   * values. If is_tex2d_from_buf is false (1D images),
   * adjust using size value (stored as width).
                                 /* If it's not a 2D texture view of a buffer, adjust using size. */
   if (!view->is_tex2d_from_buf) {
                                          /* XXX Unsure if we need to sanitize parameters? */
      } else {
                           jit->base = (uint8_t *)jit->base +
               } else {
      /* display target texture/surface */
   jit->base = llvmpipe_resource_map(res, 0, 0, LP_TEX_USAGE_READ);
   jit->row_stride[0] = lp_tex->row_stride[0];
   jit->img_stride[0] = lp_tex->img_stride[0];
   jit->mip_offsets[0] = 0;
   jit->width = res->width0;
   jit->height = res->height0;
   jit->depth = res->depth0;
   jit->first_level = jit->last_level = 0;
   jit->num_samples = res->nr_samples;
   jit->sample_stride = 0;
         }
      void
   lp_jit_texture_buffer_from_bda(struct lp_jit_texture *jit, void *mem, size_t size, enum pipe_format format)
   {
               if (LP_PERF & PERF_TEX_MEM) {
      /* use dummy tile memory */
   jit->base = lp_dummy_tile;
   jit->width = TILE_SIZE/8;
   jit->height = TILE_SIZE/8;
   jit->depth = 1;
   jit->first_level = 0;
   jit->last_level = 0;
   jit->mip_offsets[0] = 0;
   jit->row_stride[0] = 0;
   jit->img_stride[0] = 0;
   jit->num_samples = 0;
      } else {
      jit->height = 1;
   jit->depth = 1;
   jit->first_level = 0;
   jit->last_level = 0;
   jit->num_samples = 1;
            /*
   * For buffers, we don't have "offset", instead adjust
   * the size (stored as width) plus the base pointer.
   */
   const unsigned view_blocksize = util_format_get_blocksize(format);
   /* probably don't really need to fill that out */
   jit->mip_offsets[0] = 0;
   jit->row_stride[0] = 0;
            /* everything specified in number of elements here. */
         }
      void
   lp_jit_sampler_from_pipe(struct lp_jit_sampler *jit, const struct pipe_sampler_state *sampler)
   {
      jit->min_lod = sampler->min_lod;
   jit->max_lod = sampler->max_lod;
   jit->lod_bias = sampler->lod_bias;
   jit->max_aniso = sampler->max_anisotropy;
      }
      void
   lp_jit_image_from_pipe(struct lp_jit_image *jit, const struct pipe_image_view *view)
   {
      struct pipe_resource *res = view->resource;
            if (!lp_res->dt) {
      /* regular texture - setup array of mipmap level offsets */
   if (llvmpipe_resource_is_texture(res)) {
         } else {
                  jit->width = res->width0;
   jit->height = res->height0;
   jit->depth = res->depth0;
            if (llvmpipe_resource_is_texture(res)) {
                              if (res->target == PIPE_TEXTURE_1D_ARRAY ||
      res->target == PIPE_TEXTURE_2D_ARRAY ||
   res->target == PIPE_TEXTURE_3D ||
   res->target == PIPE_TEXTURE_CUBE ||
   res->target == PIPE_TEXTURE_CUBE_ARRAY) {
   /*
   * For array textures, we don't have first_layer, instead
   * adjust last_layer (stored as depth) plus the mip level offsets
   * (as we have mip-first layout can't just adjust base ptr).
   * XXX For mip levels, could do something similar.
   */
   jit->depth = view->u.tex.last_layer - view->u.tex.first_layer + 1;
                     jit->row_stride = lp_res->row_stride[view->u.tex.level];
   jit->img_stride = lp_res->img_stride[view->u.tex.level];
   jit->sample_stride = lp_res->sample_stride;
      } else {
                        /* If it's not a 2D image view of a buffer, adjust using size. */
   if (!(view->access & PIPE_IMAGE_ACCESS_TEX2D_FROM_BUFFER)) {
      /* everything specified in number of elements here. */
                                 /* XXX Unsure if we need to sanitize parameters? */
      } else {
      jit->width = view->u.tex2d_from_buf.width;
                  jit->base = (uint8_t *)jit->base +
               }
      void
   lp_jit_image_buffer_from_bda(struct lp_jit_image *jit, void *mem, size_t size, enum pipe_format format)
   {
               jit->height = 1;
   jit->depth = 1;
            unsigned view_blocksize = util_format_get_blocksize(format);
      }
